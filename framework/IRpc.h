#pragma once
#include <stdint.h>
#include <map>
#include <string>
#include <functional>
#include <memory>

class RpcHead {
public:
	RpcHead();
	virtual uint32_t HeadLen();
	virtual uint32_t MaxHeadLen();
	virtual int Decode(const char* msg, int msglen);
	virtual int Encode(char* msg, int msglen);

public:
	uint32_t 		Len;
	uint32_t		Ret;
	std::string		Name;
	uint32_t		DataLen;
	const char* 	Data;
};

class Server;
class IRpcService;
class IRpc {
public:
	typedef std::function<int(int, const char*, int)> OnRpcRequest;
	typedef std::function<int(int, int, const char*, int)> OnRpcRespond;
	typedef std::function<int(int, std::string&, const char*, int)> OnRequest;
	typedef std::map<std::string, OnRpcRequest> RequestMap;
	typedef std::map<std::string, OnRpcRespond> RespondMap;
	typedef std::map<std::string, IRpcService*> ServiceMap;

public:
	IRpc(Server* server, std::shared_ptr<RpcHead> rhead = std::make_shared<RpcHead>());
	~IRpc();

	char* GetDataBuff(size_t size);
	int AddRequestFunction(const std::string& name, const OnRpcRequest& func);
	int AddRespondFunction(const std::string& name, const OnRpcRespond& func);
	int SetRequestFunction(const OnRequest& func);
	int OnRecvCliMsg(int fd, const char* msg, int msglen);
	int OnRecvSvrMsg(int fd, const char* msg, int msglen);

	template<typename Class>
	int AddService(Class* service);
	
	int SendRespond(int fd, int ret, const std::string& name, int datalen);
	int SendRequest(int fd, const std::string& name, int datalen);

private:
	int AddService(IRpcService* service);

private:
	Server*						server_;
	RequestMap					requestMap_;
	RespondMap					respondMap_;
	OnRequest					baseRequest_;
	ServiceMap					serviceMap_;
	char*						databuff_;
	std::shared_ptr<RpcHead>	rhead_;
};

template<typename Class> class GenServiceHandler;
template<typename Class>
int IRpc::AddService(Class* service)
{
	if (!service) {
		return -1;
	}

	return AddService(GenServiceHandler<typename Class::__InterfaceType>()(this, service));
}

