#pragma once
#include <string>

class IRpc;
class Server;

typedef std::function<void(int)> SvrClosedCb;

class SvrConnectorMgr;
class SvrConnector {
public:
	SvrConnector(int protocol, const char* ip, int port, Server* server);
	~SvrConnector();

	void SetFd(int fd) {
		fd_ = fd;
	}

	int GetProtocol() {
		return protocol_;
	}

	uint64_t GetRemote() {
		return remoteAddr_;
	}

	void AttachRpc(IRpc* rpc);
	int OnRecvMsg(const char* msg, int msglen);
	void SetClosedCb(SvrClosedCb cb);

private:
	int 				fd_;
	int					protocol_;
	std::string			ip_;
	int					port_;
	uint64_t			remoteAddr_;
	Server*				server_;
	IRpc*				rpc_;
	SvrClosedCb			closedCb_;
};

