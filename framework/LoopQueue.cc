#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "Log.h"
#include "LoopQueue.h"

LoopQueue::LoopQueue()
{
	buff_ = nullptr;
	bufflen_ = 0;
	head_ = -1;
	tail_ = -1;
}

LoopQueue::~LoopQueue()
{
	if (buff_) {
		delete buff_;
	}
	
	buff_ = nullptr;
	bufflen_ = 0;
	head_ = -1;
	tail_ = -1;
}

bool LoopQueue::Init(int bufflen)
{
	if (bufflen <= 0) {
		return false;
	}

	buff_ = (char*)malloc(bufflen);
	memset(buff_, 0x00, bufflen);

	bufflen_ = bufflen;
	head_ = 0;
	tail_ = 0;

	return true;
}
	
bool LoopQueue::Push(int fd, MsgType type, const char* msg, int msglen)
{
	char* prebuff = BufferForPush(fd, type, msglen);
	if (!prebuff) {
		return false;
	}

	if (msg && msglen > 0) {
		memcpy(prebuff, msg, msglen);
	}

	PushFinish(msglen);

	return true;
}

char* LoopQueue::BufferForPush(int fd, MsgType type, int msglen)
{
	int blocklen = sizeof(block_t);

	if (tail_ >= head_ && tail_ + blocklen > bufflen_) {
		PLOG_ERROR("queue loop to head of queue! tail=%u!", tail_);
		tail_ = 0;
	}

	if (tail_ >= head_ && tail_ + blocklen + msglen > bufflen_) {
		block_t* bt = (block_t*)(buff_ + tail_);
		bt->len = blocklen;
		bt->discard = true;
		PLOG_ERROR("queue loop to head of queue for discard tail=%u!", tail_);
		tail_ = 0;
	}

	if (tail_ < head_ && tail_ + blocklen + msglen >= head_) {
		PLOG_ERROR("queue loop full! head=%u tail=%u!", head_, tail_);
		return nullptr;
	}

	block_t* bt = (block_t*)(buff_ + tail_);
	bt->len = blocklen + msglen;
	bt->discard = false;
	bt->fd = fd;
	bt->type = type;
	bt->datalen = msglen;

	return bt->data;
}

void LoopQueue::PushFinish(int copylen)
{
	tail_ += sizeof(block_t) + copylen;
}

bool LoopQueue::FrontBlock(block_t** block)
{
	int blen = sizeof(block_t);

	if (head_ == tail_) {
		return false;
	}

	if (head_ + blen > bufflen_) {
		head_ = 0;
	}

	block_t* bt = (block_t*)(buff_ + head_);
	if (bt->len == blen && bt->discard) {
		head_ = 0;
		bt = (block_t*)buff_;
	}

	// unlikely
	if (head_ < tail_ && head_ + bt->len > tail_) {
		PLOG_ERROR("queue pop unlikey error! head=%u tail=%u len=%u", head_, tail_, bt->len);
		return false;
	}

	// unlikely
	if (bt->len < blen) {
		PLOG_ERROR("queue pop unlikey error! len=%u %u", bt->len, blen);
		return false;
	}

	*block = bt;

	return true;
}

void LoopQueue::PopBlock(int len)
{
	head_ += len;
}

