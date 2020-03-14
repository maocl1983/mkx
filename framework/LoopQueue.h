#pragma once
#include <stdint.h>

enum MessageType {
	MT_ADD_SVR_FD	= 1,
	MT_ADD_CLI_FD	= 2,
	MT_DEL_SVR_FD	= 3,
	MT_DEL_CLI_FD	= 4,
	MT_SVR_MESSAGE	= 5,
	MT_CLI_MESSAGE	= 6,
};

#pragma pack(1)
typedef struct block {
	int			len;
	bool		discard;
	int			fd;
	uint64_t	remote;
	uint8_t		type;
	int			datalen;
	char		data[];
} block_t;
#pragma pack()

class LoopQueue {
public:
	LoopQueue();
	~LoopQueue();

	bool Init(int bufflen = 1024*1024);

	bool Push(int fd, uint64_t remote, int type, const char* msg = nullptr, int msglen = 0);

	char* BufferForPush(int fd, uint64_t remote, int type, int msglen);
	void PushFinish(int copylen);

	bool FrontBlock(block_t** block);
	void PopBlock(int len);

private:
	char*				buff_;
	int					bufflen_;
	int					head_;
	int					tail_;
};
