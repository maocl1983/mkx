#include <arpa/inet.h>
#include "Log.h"
#include "IRpcService.h"
#include "Server.h"
#include "IRpc.h"

using namespace std::placeholders;
const int MAX_HEAD_LEN = 256;
const int BUFF_LEN = 1024 * 1024 * 1; 

RpcHead::RpcHead()
{
	Len = 0;
}

uint32_t RpcHead::HeadLen()
{
	return 12 + Name.size();
}

uint32_t RpcHead::MaxHeadLen()
{
	return MAX_HEAD_LEN;
}

int RpcHead::Decode(const char* msg, int msglen)
{
	Len = ntohl(*((uint32_t*)msg));
	Ret = ntohl(*((uint32_t*)(msg+4)));
	uint32_t nameLen = ntohl(*((uint32_t*)(msg+8)));

	uint32_t hlen = 12 + nameLen;
	if (hlen > MaxHeadLen()) {
		nameLen -= (hlen - MaxHeadLen());
	}

	if (Len != (uint32_t)msglen || Len < hlen) {
		PLOG_ERROR("RpcHead Decode error! len=%u %u %u", Len, msglen, hlen);
		return -1;
	}
	Name.clear();
	Name.append(msg+12, nameLen);

	if (Len == hlen) {
		Data = nullptr;
	} else {
		Data = msg + hlen;
	}
	DataLen = msglen - hlen;

	return 0;
}

int RpcHead::Encode(char* data, int datalen)
{
	uint32_t hlen = HeadLen();
	DataLen = datalen;
	Len = hlen + DataLen;

	if (hlen > MaxHeadLen()) {
		PLOG_ERROR("RpcHead Encode error! len=%u %u %u", Len, hlen, datalen);
		return -1;
	}

	uint32_t sidx = MaxHeadLen() - hlen;
	*((uint32_t*)(data + sidx)) = htonl(Len);
	*((uint32_t*)(data + sidx + 4)) = htonl(Ret);
	*((uint32_t*)(data + sidx + 8)) = htonl(Name.size());
	memcpy(data + sidx + 12, Name.data(), Name.size());

	return sidx;
}

//////////////////////////////////////////////////
IRpc::IRpc(Server* server, std::shared_ptr<RpcHead> rhead)
	: server_(server), rhead_(rhead)
{
	databuff_ = new char[BUFF_LEN];
	baseRequest_ = nullptr;
}

IRpc::~IRpc()
{
	if (databuff_) {
		delete databuff_;
		databuff_ = nullptr;
	}
}

int IRpc::SendRespond(int fd, uint64_t remote, int ret, const std::string& name, int datalen)
{
	rhead_->Ret = ret;
	rhead_->Name = name;
	int sidx = rhead_->Encode(databuff_, datalen);
	if (sidx < 0) {
		PLOG_ERROR("IRpc SendRespond error! sidx=%u", sidx);
		return -1;
	}
	
	PLOG_DEBUG("IRpc::SendRespond len=%d", rhead_->Len);
	return server_->SendMsg(fd, remote, databuff_ + sidx, rhead_->Len);
}

int IRpc::SendRequest(int fd, const std::string& name, int datalen)
{
	rhead_->Ret = 0;
	rhead_->Name = name;
	int sidx = rhead_->Encode(databuff_, datalen);
	if (sidx < 0) {
		PLOG_ERROR("IRpc SendRespond error! sidx=%u", sidx);
		return -1;
	}
	
	return server_->SendMsg(fd, 0, databuff_ + sidx, rhead_->Len);
}

char* IRpc::GetDataBuff(size_t size)
{
	if (size > BUFF_LEN - rhead_->MaxHeadLen()) {
		PLOG_ERROR("IRpc GetDataBuff error! size=%u", size);
		return nullptr;
	}

	return databuff_ + rhead_->MaxHeadLen();
}

int IRpc::AddService(IRpcService* service)
{
	std::string sname = service->Name();
	if (sname.empty()) {
		PLOG_ERROR("IRpc AddService error! Name empty");
		return -1;
	}

	ServiceMap::iterator it = serviceMap_.end();
	if (it != serviceMap_.end()) {
		PLOG_ERROR("IRpc AddService already exists! Name=%s", sname.c_str());
		return -1;
	}

	service->AttachHandler(this);
	service->RegisterServiceFunction();

	serviceMap_.emplace(sname, service);
	return 0;
}

int IRpc::AddRequestFunction(const std::string& name, const OnRpcRequest& func)
{
	if (name.empty() || !func) {
		PLOG_ERROR("IRpc AddRequestFunction error! Name=%s", name.c_str());
		return -1;
	}

	RequestMap::iterator it = requestMap_.find(name);
	if (it != requestMap_.end()) {
		PLOG_ERROR("IRpc AddRequestFunction already exists! Name=%s", name.c_str());
		return -1;
	}

	requestMap_.emplace(name, func);
	return 0;
}

int IRpc::AddRespondFunction(const std::string& name, const OnRpcRespond& func)
{
	if (name.empty() || !func) {
		PLOG_ERROR("IRpc AddRespondFunction error! Name=%s", name.c_str());
		return -1;
	}

	RespondMap::iterator it = respondMap_.find(name);
	if (it != respondMap_.end()) {
		PLOG_ERROR("IRpc AddRespondFunction already exists! Name=%s", name.c_str());
		return -1;
	}

	respondMap_.emplace(name, func);
	return 0;
}

int IRpc::SetRequestFunction(const OnRequest& func)
{
	baseRequest_ = func;
	return 0;
}

int IRpc::OnRecvCliMsg(int fd, uint64_t remote, const char* msg, int msglen)
{
	if (!msg) {
		PLOG_ERROR("IRpc OnRecvMsg msg nullptr!");
		return -1;
	}

	int hret = rhead_->Decode(msg, msglen);
	if (hret != 0) {
		PLOG_ERROR("IRpc head decode error!");
		return hret;
	}

	RequestMap::iterator it = requestMap_.find(rhead_->Name);
	if (it == requestMap_.end()) {
		if (baseRequest_) {
			return baseRequest_(fd, rhead_->Name, rhead_->Data, rhead_->DataLen);
		}
		PLOG_ERROR("IRpc OnRecvMsg cannot find request! Name=%s", rhead_->Name.c_str());
		int errcode = 101;
		SendRespond(fd, remote, errcode, rhead_->Name, 0);
		return -1;
	}

	//PLOG_DEBUG("IRpc::OnRecvMsg fd=%d len=%u", fd, msglen);
	return (it->second)(fd, remote, rhead_->Data, rhead_->DataLen);
}

int IRpc::OnRecvSvrMsg(int fd, const char* msg, int msglen)
{
	if (!msg) {
		PLOG_ERROR("IRpc OnRecvSvrMsg msg nullptr!");
		return -1;
	}

	int hret = rhead_->Decode(msg, msglen);
	if (hret != 0) {
		PLOG_ERROR("IRpc head decode error!");
		return hret;
	}

	RespondMap::iterator it = respondMap_.find(rhead_->Name);
	if (it == respondMap_.end()) {
		PLOG_ERROR("IRpc OnRecvSvrMsg cannot find request! Name=%s", rhead_->Name.c_str());
		//SendRespond(fd, 1, rhead_->Name, 0);
		return -1;
	}

	//PLOG_DEBUG("IRpc::OnRecvMsg fd=%d len=%u", fd, msglen);
	return (it->second)(fd, rhead_->Ret, rhead_->Data, rhead_->DataLen);
}

