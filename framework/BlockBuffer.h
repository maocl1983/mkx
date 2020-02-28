#pragma once
#include <map>
#include <list>

struct buffer_block;

// BlockBuffer, unsafe in mutilthread
class BlockBuffer {
public:
	BlockBuffer();
	~BlockBuffer();

	int Insert(const void* data, size_t len);
	size_t Copy(void* data, size_t len);
	size_t Move(void* data, size_t len);
	
	size_t GetTotalLen();
	size_t GetLeftLen();

	char* GetBufferToWrite(int needlen);
	int FinishBufferWrite(int wlen);
	char* GetBufferToRead(int* rlen);

	void PrintStatus();

private:
	int expand(size_t len);
	int replaceBlock(struct buffer_block* dst, struct buffer_block* src);

private:
	struct buffer_block*	head_;
	struct buffer_block*	tail_;
	int						totallen_;
};

class BufferBlockMgr {
public:
	BufferBlockMgr();
	~BufferBlockMgr();

	buffer_block* Alloc(size_t size);
	void Recycle(struct buffer_block* block);

	void PrintStatus();
private:
	struct buffer_block* allocNew(size_t size);

private:
	std::map<size_t, std::list<struct buffer_block*>> blocks_;
};

