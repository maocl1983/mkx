#include "Timer.h"
#include "IRpc.h"
#include "Server.h"
#include "LoopQueue.h"
#include "SvrClient.h"

SvrClient::SvrClient(Server* server)
{
	fd_ = -1;
	server_ = server;
	reconTimer_ = 0;
	reconnect_ = false;
	rpc_ = nullptr;
}

SvrClient::~SvrClient()
{
	if (fd_ > 0) {
		server_->CloseSvr(fd_);
	}

	if (reconTimer_) {
		TimerMgr* timer = server_->GetTimerMgr();
		timer->DelTimer(reconTimer_);
	}

	fd_ = -1;
	server_ = nullptr;
	reconTimer_ = 0;
	reconnect_ = false;
}

int SvrClient::ConnectSvr(const std::string& url, bool reconnect)
{

	url_ = url;
	fd_ = server_->ConnectSvr(url_);
	TimerMgr* timer = server_->GetTimerMgr();
	if (fd_ < 0 && reconnect_ && reconTimer_ == 0) {
		reconTimer_ = timer->AddTimer(std::bind(&SvrClient::Reconnect, this), 1.0, 2.0);
	} else if (fd_ > 0) {
		server_->AttachSvrClient(fd_, this);
		if (reconTimer_) {
			timer->DelTimer(reconTimer_);
			reconTimer_ = 0;
		}
		// noti work thread
		// TODO:if failed?
		if (server_->GetRecvQue()->Push(fd_, MT_ADD_SVR_FD)) {
			server_->MessageRecvNoti();
		}
	}

	return fd_;
}

void SvrClient::AttachRpc(IRpc* rpc)
{
	rpc_ = rpc;
}

int SvrClient::OnRecvMsg(const char* msg, int msglen)
{
	if (rpc_) {
		return rpc_->OnRecvSvrMsg(fd_, msg, msglen);
	}

	return 0;
}

void SvrClient::OnClosed()
{
	if (reconnect_ && reconTimer_ == 0) {
		TimerMgr* timer = server_->GetTimerMgr();
		reconTimer_ = timer->AddTimer(std::bind(&SvrClient::Reconnect, this), 1.0, 2.0);
	}
		
	// noti work thread
	// TODO:if failed?
	if (server_->GetRecvQue()->Push(fd_, MT_DEL_SVR_FD)) {
		server_->MessageRecvNoti();
	}
	
	fd_ = -1;
}

int SvrClient::Reconnect()
{
	if (fd_ < 0) {
		ConnectSvr(url_);
	}

	return 0;
}

