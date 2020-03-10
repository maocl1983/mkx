#include <sys/time.h>
#include "Locker.h"

MutexLocker::MutexLocker()
{
	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&cond_, NULL);
}

MutexLocker::~MutexLocker()
{
	pthread_mutex_destroy(&mutex_);
	pthread_cond_destroy(&cond_);
}

int MutexLocker::Lock()
{
	return pthread_mutex_lock(&mutex_);
}

int MutexLocker::Unlock()
{
	return pthread_mutex_unlock(&mutex_);
}

int MutexLocker::Wait()
{
	return pthread_cond_wait(&cond_, &mutex_);
}

int MutexLocker::WaitTimeout(long ms)
{
	struct timespec abstime;
	struct timeval now;
	gettimeofday(&now, nullptr);
	long nsec = now.tv_usec * 1000 + ms * 1000000;
	abstime.tv_sec= now.tv_sec + (nsec / 1000000000);
	abstime.tv_nsec= nsec % 1000000000;
	return pthread_cond_timedwait(&cond_, &mutex_, &abstime);
}

int MutexLocker::Signal()
{
	return pthread_cond_signal(&cond_);
}

int MutexLocker::Broadcast()
{
	return pthread_cond_broadcast(&cond_);
}

