#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "Log.h"
#include "IRpc.h"
#include "Server.h"
#include "SvrClient.h"
#include "LoopQueue.h"
#include "IOEventHandler.h"
#include "Worker.h"

static void msgEventCb(EventBase* ev, int fd, int events, void* arg)
{
	PLOG_DEBUG("Worker msgEventCb");
	Worker* worker = (Worker*)arg;
	worker->OnMessage();
}


Worker::Worker(Server* server)
	: server_(server)
{
	evHandler_ = nullptr;
	msgEvent_ = nullptr;
}

Worker::~Worker()
{
	if (msgEvent_) {
		delete msgEvent_;
		msgEvent_ = nullptr;
	}
}

void Worker::Init(int evType)
{
	evHandler_ = GetIOEventHandlerFactory(evType)->GetIOEventHandler();

	msgEvent_ = evHandler_->NewUserEvent();
	msgEvent_->SetCb(msgEventCb, this);
	msgEvent_->Start();
}

void Worker::OnMessage()
{
	LoopQueue* recvq = server_->GetRecvQue();

	block_t* block;
	while (recvq->Pop(&block)) {
		if (block->type == MT_ADD_CLI_FD) {
			PLOG_DEBUG("Worker::OnMessage cli fd connected fd=%d", block->fd);
		} else if (block->type == MT_ADD_SVR_FD) {
			PLOG_DEBUG("Worker::OnMessage svr fd connected fd=%d", block->fd);
			SvrClient* scli = server_->GetSvrClient(block->fd);
			if (scli) {
				scli->OnConnectedInThread();
			}
		} else if (block->type == MT_DEL_CLI_FD) {
			PLOG_DEBUG("Worker::OnMessage cli fd closed fd=%d", block->fd);
		} else if (block->type == MT_DEL_SVR_FD) {
			PLOG_DEBUG("Worker::OnMessage svr fd closed fd=%d", block->fd);
		} else if (block->type == MT_CLI_MESSAGE){
			PLOG_DEBUG("Worker::OnMessage cli fd=%d msglen=%u", block->fd, block->datalen);
			server_->OnRecvMsg(block->fd, block->data, block->datalen, true);
		} else if (block->type == MT_SVR_MESSAGE){
			PLOG_DEBUG("Worker::OnMessage svr fd=%d msglen=%u", block->fd, block->datalen);
			server_->OnRecvMsg(block->fd, block->data, block->datalen, false);
		}
	}
}

void Worker::Run()
{
	evHandler_->EvRun();
}

void Worker::EvAsyncSend()
{
	msgEvent_->Trigger();
}

