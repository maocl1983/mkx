#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "BlockBuffer.h"

#define MIN_BLOCK_SIZE 512
#define MID_BLOCK_SIZE 1024

#define MOVE_BLOCK_SIZE 2048
#define COPY_BLOCK_SIZE 4096

typedef struct buffer_block {
	char*					buffer;
	int						len;
	int						rlen;
	int						rdpos;
	int						wrpos;
	struct buffer_block*	next;
} buffer_block_t;

static BufferBlockMgr blockMgr;

//////////////////////////////////////////////////////////////////////////////////////
BlockBuffer::BlockBuffer()
{
	head_ = nullptr;
	tail_ = nullptr;
	totallen_ = 0;
}

BlockBuffer::~BlockBuffer()
{
	while (head_) {
		buffer_block_t* block = head_;
		head_ = head_->next;
		blockMgr.Recycle(block);
	}
	tail_ = nullptr;
	totallen_ = 0;
}

int BlockBuffer::Insert(const void* data, size_t len)
{
	if (head_ == nullptr) {
		buffer_block_t* block = blockMgr.Alloc(len);
		head_ = block;
		tail_ = block;
	}

	buffer_block_t* bpos = tail_;
	const char* data_copy = (const char*)data;
	size_t data_copy_len = len; 
	while (true) {
		if (bpos == nullptr) {
			buffer_block_t* block = blockMgr.Alloc(data_copy_len);
			tail_->next = block;
			tail_ = block;
			bpos = block;
		}

		if (bpos->wrpos >= bpos->len) {
			bpos = bpos->next;
			continue;
		}

		size_t bleft = bpos->len - bpos->wrpos;
		if (bleft >= data_copy_len) {
			memcpy(bpos->buffer + bpos->wrpos, data_copy, data_copy_len);
			bpos->wrpos += data_copy_len;
			break;
		} else {
			memcpy(bpos->buffer + bpos->wrpos, data_copy, bleft);
			data_copy += bleft;
			data_copy_len -= bleft;
			bpos->wrpos = bpos->len;
			bpos = bpos->next;
			continue;
		}
	}

	totallen_ += len;
	return 0;
}

size_t BlockBuffer::Copy(void* data, size_t len)
{
	if (head_ == nullptr) {
		return 0;
	}

	buffer_block_t* bpos = head_;
	size_t copy_len = 0;
	while (bpos) {
		size_t canread = bpos->wrpos - bpos->rdpos;
		if (len < canread) {
			memcpy(data, bpos->buffer + bpos->rdpos, len);
			copy_len += len;
			break;
		} else {
			if (canread > 0) {
				memcpy(data, bpos->buffer + bpos->rdpos, canread);
				copy_len += canread;
				len -= canread;
			}
			if (bpos->wrpos >= bpos->len) {
				bpos = bpos->next;
				continue;
			}
			break;
		}
	}

	return copy_len;
}

size_t BlockBuffer::Move(void* data, size_t len)
{
	if (head_ == nullptr) {
		return 0;
	}

	buffer_block_t* bpos = head_;
	size_t move_len = 0;
	while (bpos && len > 0) {
		size_t canread = bpos->wrpos - bpos->rdpos;
		if (len < canread) {
			if (data) {
				memcpy(data, bpos->buffer + bpos->rdpos, len);
			}
			move_len += len;
			bpos->rdpos += len;
			break;
		} else {
			if (canread > 0) {
				if (data) {
					memcpy(data, bpos->buffer + bpos->rdpos, canread);
				}
				move_len += canread;
				len -= canread;
				bpos->rdpos = bpos->wrpos;
			}
			if (bpos->wrpos >= bpos->len && bpos->next) {
				buffer_block_t* tmpb = bpos;
				bpos = bpos->next;
				head_ = bpos;
				blockMgr.Recycle(tmpb);
				continue;
			}
			break;
		}
	}

	if (bpos && bpos->rdpos == bpos->wrpos) {
		if (bpos->len <= MID_BLOCK_SIZE) {
			bpos->rdpos = 0;
			bpos->wrpos = 0;
			bpos->len = bpos->rlen;
		} else {
			if (head_ == bpos) {
				head_ = bpos->next;
			}
			tail_ = head_ ? tail_ : nullptr;
			blockMgr.Recycle(bpos);
		}
	}

	totallen_ -= move_len;
	return move_len;
}

size_t BlockBuffer::GetTotalLen()
{
#if 1
	int len = 0;
	buffer_block_t* bpos = head_;
	while (bpos) {
		size_t ulen = bpos->wrpos - bpos->rdpos;
		len += ulen;
		if (bpos->wrpos >= bpos->len) {
			bpos = bpos->next;
			continue;
		}
		break;
	}
	assert(len == totallen_);
#endif
	return totallen_;
}

size_t BlockBuffer::GetLeftLen()
{
	buffer_block_t* block = tail_;
	if (block && block->wrpos < block->len) {
		return block->len - block->wrpos;
	}

	return 0;
}

char* BlockBuffer::GetBufferToWrite(int needlen)
{
	buffer_block_t* block = tail_;
	if (block && block->len - block->wrpos >= needlen) {
		return block->buffer + block->wrpos;
	}

	if (expand(needlen) == 0) {
		block = tail_;
		return block->buffer + block->wrpos;
	}

	return nullptr;
}

int BlockBuffer::FinishBufferWrite(int wlen)
{
	assert(tail_);
	assert(tail_->wrpos + wlen <= tail_->len);

	tail_->wrpos += wlen;
	totallen_ += wlen;
	return 0;
}

char* BlockBuffer::GetBufferToRead(int* rlen)
{
	if (head_ == nullptr) {
		return nullptr;
	}

	if (rlen) {
		*rlen = head_->wrpos - head_->rdpos;
	}

	return head_->buffer + head_->rdpos;
}

void BlockBuffer::PrintStatus()
{
	printf("len=%lu %lu, head", GetTotalLen(), GetLeftLen());
	buffer_block_t* block = head_;
	while (block) {
		printf("->[len=%d %d pos=%d %d]", 
				block->rlen, block->len, 
				block->rdpos, block->wrpos);
		if (tail_ == block) {
			printf("->tail");
		}
		block = block->next;
	}
	printf("\n");
}

int BlockBuffer::expand(size_t len)
{
	if (tail_ == nullptr) {
		head_ = blockMgr.Alloc(len);
		tail_ = head_;
		return 0;
	}

	buffer_block_t* block = tail_;
	int freelen = block->len - block->wrpos + block->rdpos;
	int usedlen = block->wrpos - block->rdpos;

	if (freelen >= (int)len && usedlen <= MOVE_BLOCK_SIZE) {
		memmove(block->buffer, block->buffer + block->rdpos, usedlen);
		block->rdpos = 0;
		block->wrpos = usedlen;
		return 0;
	}

	if (usedlen < COPY_BLOCK_SIZE) {
		buffer_block_t* newblock = blockMgr.Alloc(usedlen + len);
		replaceBlock(tail_, newblock);
		return 0;
	}

	block->len = block->wrpos; //stop
	buffer_block_t* newblock = blockMgr.Alloc(len);
	tail_->next = newblock;
	tail_ = newblock;
	return 0;
}

int BlockBuffer::replaceBlock(buffer_block_t* dst, buffer_block_t* src)
{
	assert(dst->wrpos - dst->rdpos <= src->len);

	memcpy(src->buffer, dst->buffer + dst->rdpos, dst->wrpos - dst->rdpos);
	src->wrpos = dst->wrpos - dst->rdpos;

	if (head_ == dst) {
		src->next = head_->next;
		head_ = src;
		tail_ = tail_ == dst ? src : tail_;
		blockMgr.Recycle(dst);
		return 0;
	}

	buffer_block_t* block = head_;
	while (block) {
		if (block->next == dst) {
			block->next = src;
			src->next = dst->next;
			tail_ = tail_ == dst ? src : tail_;
			blockMgr.Recycle(dst);
			return 0;
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
BufferBlockMgr::BufferBlockMgr()
{

}

BufferBlockMgr::~BufferBlockMgr()
{
	std::map<size_t, std::list<buffer_block_t*>>::iterator it;
	for (it = blocks_.begin(); it != blocks_.end(); it++) {
		while (it->second.size() > 0) {
			buffer_block_t* block = it->second.front();
			delete block;
			it->second.pop_front();
		}
	}
	blocks_.clear();
}

buffer_block_t* BufferBlockMgr::Alloc(size_t size)
{
	size_t alloclen = MIN_BLOCK_SIZE;
	while (alloclen < size) {
		alloclen = alloclen << 1;
	}

	buffer_block_t* block = nullptr;
	std::list<buffer_block_t*>& blist = blocks_[alloclen];
	if (blist.size() > 0) {
		block = blist.front();
		blist.pop_front();
		//printf("alloc block size=%lu\n", alloclen);
	}

	if (!block) {
		block = allocNew(alloclen);
	}

	return block;
}

void BufferBlockMgr::Recycle(buffer_block_t* block)
{
	block->rdpos = 0;
	block->wrpos = 0;
	block->next = nullptr;
	block->len = block->rlen;

	blocks_[block->rlen].push_back(block);
	//printf("recycle block size=%d\n", block->len);
}

void BufferBlockMgr::PrintStatus()
{
	printf("blockmgr:[ ");
	std::map<size_t, std::list<buffer_block_t*>>::iterator it;
	for (it = blocks_.begin(); it != blocks_.end(); it++) {
		printf("(%lu, %lu) ", it->first, it->second.size());
	}
	printf("]\n");
}

buffer_block_t* BufferBlockMgr::allocNew(size_t size)
{
	buffer_block_t* block = (buffer_block_t*)malloc(sizeof(buffer_block_t));
	assert(block);
	
	block->buffer = (char*)malloc(size);
	block->len = size;
	block->rlen = size;
	block->rdpos = 0;
	block->wrpos = 0;
	block->next = nullptr;

	//printf("alloc new block size=%lu\n", size);
	return block;
}

