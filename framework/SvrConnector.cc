#include <netinet/in.h>
#include <arpa/inet.h>
#include "IRpc.h"
#include "Server.h"
#include "SvrConnector.h"

using namespace std;

SvrConnector::SvrConnector(int protocol, const char* ip, int port, Server* server)
	: fd_(-1), protocol_(protocol), ip_(ip), port_(port), server_(server)
{
	uint32_t ipInt = 0;
	inet_pton(AF_INET, ip_.c_str(), &ipInt);
	remoteAddr_ = (uint64_t)ipInt << 32;
	remoteAddr_ |= htons(port_);
}

SvrConnector::~SvrConnector()
{
	if (closedCb_) {
		closedCb_(fd_);
	}

	closedCb_ = nullptr;
	server_ = nullptr;
	fd_ = -1;
	rpc_ = nullptr;
}

void SvrConnector::AttachRpc(IRpc* rpc)
{
	rpc_ = rpc;
}

int SvrConnector::OnRecvMsg(const char* msg, int msglen)
{
	if (rpc_) {
		return rpc_->OnRecvSvrMsg(fd_, msg, msglen);
	}

	return 0;
}

void SvrConnector::SetClosedCb(SvrClosedCb cb)
{
	closedCb_ = cb;
}
