#include "IRpc.h"
#include "Server.h"
#include "SvrConnector.h"

using namespace std;

SvrConnector::SvrConnector(int fd, Server* server)
	: fd_(fd), server_(server)
{

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
