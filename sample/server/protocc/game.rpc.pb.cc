#include "game.rpc.pb.h"
#include "common/Log.h"
#include "framework/IRpc.h"

using namespace std::placeholders;

namespace gs {

GameServiceClient::GameServiceClient(IRpc* handler)
{
	handler_ = handler;
}

GameServiceClient::~GameServiceClient()
{

}

int GameServiceClient::login(int fd, const gs::LoginRequest& req)
{
	size_t __size = req.ByteSize();
	char* __buff = handler_->GetDataBuff(__size);
	if (!__buff) {
		PLOG_ERROR("login GetDataBuff error! len=%u", __size);
		return -1;
	}

	if (!req.SerializeToArray(__buff, __size)) {
		PLOG_ERROR("login SerializeToArray error! len=%u", __size);
		return -2;
	}
	
	return handler_->SendRequest(fd, "GameService::login", __size);
}


__GameServiceClientSkeleton::__GameServiceClientSkeleton(IRpc* rpc, GameServiceClientInterface* iface)
	: IRpcService(rpc), iface_(iface)
{

}

__GameServiceClientSkeleton::~__GameServiceClientSkeleton()
{

}

int __GameServiceClientSkeleton::process_login_cb(int fd, int ret, const char* buff, uint32_t bufflen)
{
	gs::LoginResponse __rsp;
	if (ret == 0 && !__rsp.ParseFromArray((const void*)buff, bufflen)) {
		PLOG_ERROR("process_login_cb ParseFromArray error! len=%u", bufflen);
		return -1;
	}

	iface_->login_cb(fd, ret, __rsp);
	return 0;
}

int __GameServiceClientSkeleton::RegisterServiceFunction()
{
	if (!handler_) {
		return -1;
	}

	IRpc::OnRpcRespond func = std::bind(&__GameServiceClientSkeleton::process_login_cb, this, _1, _2, _3, _4);
	int ret = handler_->AddRespondFunction("GameService::login", func);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

std::string __GameServiceClientSkeleton::Name()
{
	return "GameServiceClient";
}



__GameServiceSkeleton::__GameServiceSkeleton(IRpc* rpc, GameServiceServerInterface* iface)
	: IRpcService(rpc), iface_(iface)
{
	
}

__GameServiceSkeleton::~__GameServiceSkeleton()
{

}

int __GameServiceSkeleton::process_login(int fd, uint64_t remote, const char* buff, uint32_t bufflen)
{
	gs::LoginRequest __req;
	if (!__req.ParseFromArray((const void*)buff, bufflen)) {
		PLOG_ERROR("process_login ParseFromArray error! len=%u", bufflen);
		handler_->SendRespond(fd, remote, 1, Name(), 0);
		return -1;
	}
	PLOG_DEBUG("process_login fd=%d len=%u plid=%u", fd, bufflen, __req.player_id());

	GameServiceServerInterface::RspCb __rspcb = 
		std::bind(&__GameServiceSkeleton::return_login, this, _1, _2, _3, _4);
	iface_->login(fd, remote, __req, __rspcb);
	return 0;
}
	
void __GameServiceSkeleton::return_login(int fd, uint64_t remote, int ret, const gs::LoginResponse& rsp)
{
	PLOG_DEBUG("return_login fd=%d ret=%u", fd, ret);
	if (ret > 0) {
		handler_->SendRespond(fd, remote, ret, Name(), 0);
		return;
	}

	size_t __size = rsp.ByteSize();
	char* __buff = handler_->GetDataBuff(__size);
	if (!__buff) {
		PLOG_ERROR("return_login GetDataBuff error! len=%u", __size);
		handler_->SendRespond(fd, remote, 1, Name(), 0);
		return;
	}

	if (!rsp.SerializeToArray(__buff, __size)) {
		PLOG_ERROR("return_login SerializeToArray error! len=%u", __size);
		handler_->SendRespond(fd, remote, 2, Name(), 0);
		return;
	}

	handler_->SendRespond(fd, remote, 0, Name(), __size);
}

int __GameServiceSkeleton::RegisterServiceFunction()
{
	if (!handler_) {
		return -1;
	}

	IRpc::OnRpcRequest func = std::bind(&__GameServiceSkeleton::process_login, this, _1, _2, _3, _4);
	int ret = handler_->AddRequestFunction("GameService::login", func);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

std::string __GameServiceSkeleton::Name()
{
	return "GameService";
}

}// namespace gs
