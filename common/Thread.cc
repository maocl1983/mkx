#include "Thread.h"

static void* ThreadEntry(void* arg) 
{
	Thread* thread = (Thread*)arg;
	thread->Run();
	return nullptr;
}

Thread::Thread()
{
	threadId_ = 0;
}

Thread::~Thread()
{

}

pthread_t Thread::ThreadID()
{
	return threadId_;
}

bool Thread::Start()
{
	if (pthread_create(&threadId_, NULL, ThreadEntry, this) != 0) {
		return false;
	}

	return true;
}

bool Thread::Join()
{
	return (pthread_join(threadId_, NULL) == 0);
}

void Thread::Exit()
{
	pthread_exit(NULL);
}
