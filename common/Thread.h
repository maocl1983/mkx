#pragma once
#include <pthread.h>

class Thread {
public:
	Thread();
	virtual ~Thread();
	virtual void Run() = 0;
	pthread_t ThreadID();

	bool Start();
	bool Join();
	void Exit();

private:
	pthread_t	threadId_;
};
