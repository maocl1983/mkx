#pragma once

#include "Thread.h"

class Server;
struct block;

class Worker : public Thread {
public:
	Worker(Server* server);
	~Worker();
	
	void Init();

public:
	void Run() override;

private:
	void dealRecvQue();
	void dealMessage(struct block* msg);

private:
	Server*				server_;
	char*				recvBuffer_;
	int					maxMsglen_;
	int					statCnt_;
};
