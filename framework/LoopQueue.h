#pragma once
#include <stdint.h>

typedef enum {
	MT_ADD_SVR_FD	= 1,
	MT_ADD_CLI_FD	= 2,
	MT_DEL_SVR_FD	= 3,
	MT_DEL_CLI_FD	= 4,
	MT_SVR_MESSAGE	= 5,
	MT_CLI_MESSAGE	= 6,
} MsgType;

typedef struct block {
	int			len;
	bool		discard;
	int			fd;
	uint8_t		type;
	int			datalen;
	char		data[];
} __attribute__ ((packed)) block_t;

class LoopQueue {
public:
	LoopQueue();
	~LoopQueue();

	bool Init(int bufflen = 1024*1024);

	bool Push(int fd, MsgType type, const char* msg = nullptr, int msglen = 0);

	char* BufferForPush(int fd, MsgType type, int msglen);
	void PushFinish(int copylen);

	bool FrontBlock(block_t** block);
	void PopBlock(int len);

private:
	char*				buff_;
	int					bufflen_;
	int					head_;
	int					tail_;
};
