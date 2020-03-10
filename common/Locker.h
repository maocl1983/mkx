#pragma once
#include <pthread.h>

class MutexLocker {
public:
	MutexLocker();
	~MutexLocker();

	int Lock();
	int Unlock();
	int Wait();
	int WaitTimeout(long ms);
	int Signal();
	int Broadcast();

private:
	pthread_mutex_t		mutex_;
	pthread_cond_t		cond_;
};
