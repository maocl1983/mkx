#pragma once
#include <string>
#include <functional>
#include "game.pb.h"
#include "framework/IRpcService.h"

namespace gs {

class GameServiceClientInterface {
public:
	virtual ~GameServiceClientInterface() {}

	virtual void login_cb(int fd, int ret, const gs::LoginResponse& rsp) = 0;

	typedef GameServiceClientInterface __InterfaceType;
};

class GameServiceClient {
public:
	GameServiceClient(IRpc* handler);
	~GameServiceClient();

	int login(int fd, const gs::LoginRequest& req);
	
private:
	std::string Name();

private:
	IRpc*	handler_;
};

class __GameServiceClientSkeleton : public IRpcService{
public:
	__GameServiceClientSkeleton(IRpc* rpc, GameServiceClientInterface* iface);
	virtual ~__GameServiceClientSkeleton();

	int process_login_cb(int fd, int ret, const char* buff, uint32_t bufflen);
	
	virtual int RegisterServiceFunction() override;
	virtual std::string Name() override;

private:
	GameServiceClientInterface*		iface_;
};

class GameServiceServerInterface {
public:
	typedef std::function<void(int, uint64_t, int, const gs::LoginResponse&)> RspCb;
public:
	virtual ~GameServiceServerInterface() {}

	virtual void login(int fd, uint64_t remote, const gs::LoginRequest& req, const RspCb& cb) = 0;

	typedef GameServiceServerInterface __InterfaceType;
};

class __GameServiceSkeleton : public IRpcService {
public:
	__GameServiceSkeleton(IRpc* rpc, GameServiceServerInterface* iface);
	virtual ~__GameServiceSkeleton();

	int process_login(int fd, uint64_t remote, const char* buff, uint32_t bufflen);
	void return_login(int fd, uint64_t remote, int ret, const gs::LoginResponse& rsp);

	virtual int RegisterServiceFunction() override;
	virtual std::string Name() override;

private:
	GameServiceServerInterface* 	iface_;
};

} // namespace gs

template <typename Class> class GenServiceHandler;

template<>
class GenServiceHandler<gs::GameServiceServerInterface> {
public:
	IRpcService* operator()(IRpc* rpc, gs::GameServiceServerInterface* iface) {
		return new gs::__GameServiceSkeleton(rpc, iface);
	}
};

template<>
class GenServiceHandler<gs::GameServiceClientInterface> {
public:
	IRpcService* operator()(IRpc* rpc, gs::GameServiceClientInterface* iface) {
		return new gs::__GameServiceClientSkeleton(rpc, iface);
	}
};
