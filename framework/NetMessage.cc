#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "Log.h"
#include "Locker.h"
#include "IniConfig.h"
#include "NetMessage.h"
#include "LoopQueue.h"
#include "IOEventHandler.h"
#include "Server.h"

enum ConnType {
	CONN_TYPE_CLI = 1,
	CONN_TYPE_SVR = 2,
};

typedef struct conn_info {
	int				fd;
	int				ctype;
	BufferEvent*	bev;
} conn_info_t;

static void msgEventCb(EventBase* ev, int fd, int events, void* arg)
{
	NetMessage* net = (NetMessage*)arg;
	net->OnSendMsgEvent();
}

static void eventCb(EventBase* ev, int fd, int events, void* arg)
{
	NetMessage* net = (NetMessage*)arg;
	if (events == IOEV_EVENT_EOF) {
		net->OnConnClosed(fd);
	}
}

static void writeCb(EventBase* ev, int fd, int events, void* arg)
{
	//NetMessage* net = (NetMessage*)arg;
	//net->RecvMsg(fd);
}

static void readCb(EventBase* ev, int fd, int events, void* arg)
{
	NetMessage* net = (NetMessage*)arg;
	net->RecvMsg(fd);
}

static void listenCb(EventBase* ev, int fd, int events, void* arg)
{
	NetMessage* net = (NetMessage*)arg;
	ListenEvent* lev = (ListenEvent*)ev;

	IOEventHandler* evhandler = net->GetIOEventHandler();
	BufferEvent* bev = evhandler->NewBufferEvent(fd);
	if (!bev) {
		return;
	}

	if (net->OpenConn(fd, bev, lev->GetListenFd()) != 0) {
		delete bev;
		return;
	}

	bev->SetCb(readCb, writeCb, eventCb, net);
	bev->Enable(IOEV_READ | IOEV_WRITE);
}

//==========================================================
NetMessage::NetMessage(Server* server)
	: server_(server)
{
	msgEvent_ = GetIOEventHandler()->NewUserEvent();
	msgEvent_->SetCb(msgEventCb, this);
	msgEvent_->Start();


	maxMsglen_ = server_->GetCfg()->GetInt("max_msglen", 81920);

	maxfds_ = server_->GetCfg()->GetInt("max_fd", 10240);
	conns_ = (conn_info_t*)malloc(sizeof(conn_info_t) * maxfds_);
	for (int i = 0; i < maxfds_; i++) {
		conns_[i].fd = -1;
		conns_[i].bev = nullptr;
	}

	if (server_->ModeType() == SERVER_MODE_SINGLE) {
		recvQue_ = nullptr;
		sendQue_ = nullptr;
		recvBuffer_ = (char*)malloc(maxMsglen_);
	} else {
		recvQue_ = server_->GetRecvQue();
		sendQue_ = server_->GetSendQue();
		recvBuffer_ = nullptr;
	}
}

NetMessage::~NetMessage()
{
	if (msgEvent_) {
		delete msgEvent_;
		msgEvent_ = nullptr;
	}

	if (recvBuffer_) {
		free(recvBuffer_);
	}
}

IOEventHandler* NetMessage::GetIOEventHandler()
{
	return server_->GetIOEvHandler();
}

int NetMessage::Listen(const char* ip, int port)
{
	if (GetIOEventHandler()->EvListenIP(ip, port, listenCb, this) != 0) {
		return -1;
	}

	return 0;
}

int NetMessage::Connect(const char* ip, int port)
{
	BufferEvent* bev = GetIOEventHandler()->ConnectIP(ip, port);
	if (!bev) {
		return -1;
	}
	
	int fd = bev->GetFd();
	if (OpenConn(fd, bev) != 0) {
		delete bev;
		fd = -1;
	}

	return fd;
}

int NetMessage::SendMsg(int fd, const char* msg, int msglen)
{
	conn_info_t* conn = GetConn(fd);
	if (!conn) {
		return -1;
	}
	assert(conn->bev->GetFd() == fd);

	return conn->bev->Write(msg, msglen);
}

int NetMessage::OnConnClosed(int fd)
{
	return CloseConn(fd);
}

int NetMessage::OpenConn(int fd, BufferEvent* bev, int listenfd)
{
	if (fd >= maxfds_ || fd < 0) {
		return -1;
	}

	if (conns_[fd].fd > 0) {
		return -1;
	}

	ConnType ctype = CONN_TYPE_SVR;
	if (listenfd > 0) {
		ctype = CONN_TYPE_CLI;
		server_->OnCliConnected(fd);
	}

	conns_[fd].fd = fd;
	conns_[fd].bev = bev;
	conns_[fd].ctype = ctype;

	return 0;
}

conn_info_t* NetMessage::GetConn(int fd)
{
	if (fd >= maxfds_ || fd < 0) {
		return NULL;
	}

	if (conns_[fd].fd != fd) {
		return NULL;
	}

	return &conns_[fd];
}

int NetMessage::CloseConn(int fd)
{
	if (fd >= maxfds_ || fd < 0) {
		return -1;
	}

	if (conns_[fd].fd != fd) {
		return -1;
	}

	close(fd);
	if (conns_[fd].ctype == CONN_TYPE_CLI) {
		server_->OnCliClosed(fd);
	} else {
		server_->OnSvrClosed(fd);
	}

	if (conns_[fd].bev) {
		delete conns_[fd].bev;
	}

	conns_[fd].fd = -1;
	conns_[fd].bev = NULL;

	return 0;
}

int NetMessage::RecvMsg(int fd)
{
	conn_info_t* conn = GetConn(fd);
	if (!conn) {
		return -1;
	}

	char lenbuff[4];
	BufferEvent* bev = conn->bev;
	int copylen = bev->CopyReadBuff(lenbuff, 4);
	if (copylen != 4) {
		return -1;
	}

	uint32_t reallen = ntohl(*(uint32_t*)lenbuff);
	if (reallen > maxMsglen_) {
		PLOG_ERROR("recv msg len error! len=%u maxlen=%u", reallen, maxMsglen_);
		return -1;
	}
	if (reallen > bev->GetReadBuffLen()) {
		return 0;
	}

	MsgType mtype = MT_CLI_MESSAGE;
	if (conns_[fd].ctype == CONN_TYPE_SVR) {
		mtype = MT_SVR_MESSAGE;
	}

	if (server_->ModeType() == SERVER_MODE_SINGLE) {
		bev->Read(recvBuffer_, reallen);
		server_->OnRecvMsg(fd, recvBuffer_, reallen, mtype == MT_CLI_MESSAGE);
	} else {
		char* buffer = recvQue_->BufferForPush(fd, mtype, reallen);
		if (!buffer) {
			PLOG_ERROR("deal msg error while push to queue! fd=%d", fd);
			return -1;
		}
		bev->Read(buffer, reallen);
		recvQue_->PushFinish(reallen);

		MutexLocker* locker = server_->GetRecvLocker();
		locker->Lock();
		locker->Signal();
		locker->Unlock();
	}
	return reallen;
}

void NetMessage::SendQueNoti()
{
	//PLOG_DEBUG("NetMessage::EvAsyncSend");
	if (msgEvent_) {
		msgEvent_->Trigger();
	}
}

void NetMessage::OnSendMsgEvent()
{
	LoopQueue* sendq = server_->GetSendQue();

	block_t* block;
	while (sendq->FrontBlock(&block)) {
		if (block->type == MT_SVR_MESSAGE) {
			SendMsg(block->fd, block->data, block->datalen);
		}
		sendq->PopBlock(block->len);
	}
}


