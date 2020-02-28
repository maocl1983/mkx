#pragma once
#include <string>

class Server;
class IRpc;
class SvrClient {
public:
	SvrClient(Server* server);
	~SvrClient();

	int ConnectSvr(const std::string& url, bool reconnect = true);

	void AttachRpc(IRpc* rpc);
	int OnRecvMsg(const char* msg, int msglen);
	void OnClosed();

	//virtual void OnClosedInThread() {}
	virtual void OnConnectedInThread() {}

private:
	int Reconnect();

private:
	int 			fd_;
	std::string		url_;
	bool			reconnect_;
	int64_t			reconTimer_;
	Server* 		server_;
	IRpc*			rpc_;
};
