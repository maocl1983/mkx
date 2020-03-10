#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>

#include "Log.h"
#include "Locker.h"
#include "IniConfig.h"
#include "IRpc.h"
#include "Server.h"
#include "LoopQueue.h"
#include "Worker.h"

Worker::Worker(Server* server)
	: server_(server)
{
	recvBuffer_ = nullptr;
	maxMsglen_ = 0;
	statCnt_ = 0;
}

Worker::~Worker()
{
	if (recvBuffer_) {
		free(recvBuffer_);
		recvBuffer_ = nullptr;
	}
}

void Worker::Init()
{
	maxMsglen_ = server_->GetCfg()->GetInt("max_msglen", 81920);
	recvBuffer_ = (char*)malloc(maxMsglen_);
}

void Worker::Run()
{
	dealRecvQue();
}

void Worker::dealRecvQue()
{
	LoopQueue* recvq = server_->GetRecvQue();
	MutexLocker* locker = server_->GetRecvLocker();

	while (true) {
		block_t* block = nullptr;
		locker->Lock();
		recvq->FrontBlock(&block);
		if (!block && locker->WaitTimeout(10) == 0) {
			recvq->FrontBlock(&block);
		}

		if (block) {
			assert(block->len <= maxMsglen_);
			memcpy(recvBuffer_, (char*)block, block->len);
			recvq->PopBlock(block->len);
		}
		locker->Unlock();

		if (block) {
			dealMessage(block);
		}
	}
}

void Worker::dealMessage(block_t* msg)
{
	block_t* block = msg;
		

	if (block->type == MT_CLI_MESSAGE){
		//PLOG_DEBUG("dealMessage cli fd=%d msglen=%u pid=%lu", block->fd, block->datalen, pthread_self());
		server_->OnRecvMsg(block->fd, block->data, block->datalen, true);
	} else if (block->type == MT_SVR_MESSAGE){
		//PLOG_DEBUG("dealMessage svr fd=%d msglen=%u", block->fd, block->datalen);
		server_->OnRecvMsg(block->fd, block->data, block->datalen, false);
	} else {
		PLOG_ERROR("dealMessage error! fd=%d type=%d", block->fd, block->type);
	}
}

