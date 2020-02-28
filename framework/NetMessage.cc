#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "Log.h"
#include "NetMessage.h"
#include "LoopQueue.h"
#include "IOEventHandler.h"
#include "Server.h"

const int fdmax_size = 10240;

typedef enum {
	CONN_CLI = 1,
	CONN_SVR = 2,
} ConnType;

typedef struct conn_info {
	int				fd;
	int				ctype;
	BufferEvent*	bev;
} conn_info_t;

static void msgEventCb(EventBase* ev, int fd, int events, void* arg)
{
	NetMessage* net = (NetMessage*)arg;
	net->OnSendMsg();
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

	recvQue_ = server_->GetRecvQue();
	sendQue_ = server_->GetSendQue();

	maxfds_ = fdmax_size;
	conns_ = (conn_info_t*)malloc(sizeof(conn_info_t) * maxfds_);
	for (int i = 0; i < maxfds_; i++) {
		conns_[i].fd = -1;
		conns_[i].bev = nullptr;
	}
}

NetMessage::~NetMessage()
{
	if (msgEvent_) {
		delete msgEvent_;
		msgEvent_ = nullptr;
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

	ConnType ctype = CONN_SVR;
	if (listenfd > 0) {
		ctype = CONN_CLI;
		if (!recvQue_->Push(fd, MT_ADD_CLI_FD)) {
			PLOG_ERROR("new fd error while push to queue! fd=%d", fd);
			return -1;
		}
		server_->MessageRecvNoti();
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

	if (conns_[fd].ctype == CONN_CLI) {
		if (!recvQue_->Push(fd, MT_DEL_CLI_FD)) {
			PLOG_ERROR("close cli fd error while push to queue! fd=%d", fd);
			return -1;
		}
		server_->MessageRecvNoti();
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

int NetMessage::DealRecvMsg(int fd, const char* msg, int msglen)
{
	conn_info_t* conn = GetConn(fd);
	if (!conn) {
		CloseConn(fd);
		return 0;
	}

	// TODO: check len
	if (msglen < 4) {
		return 0;
	}

	uint32_t reallen = ntohl(*(uint32_t*)msg);
	PLOG_DEBUG("NetMessage::DealRecvMsg fd=%u reallen=%u", fd, reallen);
	if (reallen > 8103808) {
		return -1;
	}
	if (reallen > (uint32_t)msglen) {
		return 0;
	}

	MsgType mtype = conns_[fd].ctype == CONN_CLI ? MT_CLI_MESSAGE : MT_SVR_MESSAGE;
	if (!recvQue_->Push(fd, mtype, msg, reallen)) {
		PLOG_ERROR("deal msg error while push to queue! fd=%d", fd);
		return -1;
	}

	server_->MessageRecvNoti();
	return reallen;
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
	if (reallen > 8103808) {
		return -1;
	}
	if (reallen > bev->GetReadBuffLen()) {
		return 0;
	}

	MsgType mtype = conns_[fd].ctype == CONN_CLI ? MT_CLI_MESSAGE : MT_SVR_MESSAGE;
	char* buffer = recvQue_->PreAlloc(fd, mtype, reallen);
	if (!buffer) {
		PLOG_ERROR("deal msg error while push to queue! fd=%d", fd);
		return -1;
	}
	bev->Read(buffer, reallen);
	recvQue_->FinishCopy(reallen);

	server_->MessageRecvNoti();
	return reallen;
}

void NetMessage::EvAsyncSend()
{
	PLOG_DEBUG("NetMessage::EvAsyncSend");
	if (msgEvent_) {
		msgEvent_->Trigger();
	}
}

void NetMessage::OnSendMsg()
{
	LoopQueue* sendq = server_->GetSendQue();

	block_t* block;
	while (sendq->Pop(&block)) {
		if (block->type == MT_SVR_MESSAGE) {
			SendMsg(block->fd, block->data, block->datalen);
		}
	}
}


