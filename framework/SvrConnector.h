#pragma once
#include <string>

class IRpc;
class Server;

typedef std::function<void(int)> SvrClosedCb;

class SvrConnectorMgr;
class SvrConnector {
public:
	SvrConnector(int fd, Server* server);
	~SvrConnector();

	void AttachRpc(IRpc* rpc);
	int OnRecvMsg(const char* msg, int msglen);
	void SetClosedCb(SvrClosedCb cb);

private:
	int 				fd_;
	Server*				server_;
	IRpc*				rpc_;
	SvrClosedCb			closedCb_;
};

