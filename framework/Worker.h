#pragma once

#include "Thread.h"

class Server;
class IOEventHandler;
class UserEvent;

class Worker : public Thread {
public:
	Worker(Server* server);
	~Worker();
	
	void Init(int evType);
	void OnMessage();

public:
	void Run() override;
	void EvAsyncSend();

private:
	Server*				server_;
	IOEventHandler*		evHandler_;
	UserEvent*			msgEvent_;
};
