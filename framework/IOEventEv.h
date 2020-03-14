#pragma once

#include <functional>
#include <ev.h>
#include "IOEventHandler.h"

// UserEvent
class UserEventEv : public UserEvent {
public:
	UserEventEv(IOEventHandler* ev);
	~UserEventEv();

	virtual void Start() override;
	virtual void Trigger() override;
	void DoCallback(int shorts);

private:
	ev_async*			evasync_;
};

// SignalEvent
class SignalEventEv : public SignalEvent {
public:
	SignalEventEv(IOEventHandler* ev, int signal);
	~SignalEventEv();

	virtual void Start() override;
	void DoCallback();

private:
	ev_signal*			evsignal_;
};

// TimerEvent
class TimerEventEv : public TimerEvent {
public:
	TimerEventEv(IOEventHandler* ev);
	~TimerEventEv();

	virtual void Start(double after, double repeat) override;
	void DoCallback();

	bool Repeated() {
		return repeat_ > 0.0;
	}
private:
	ev_timer*			evtimer_;
	bool				started_;
	double				repeat_;
};

// BufferEvent
class BlockBuffer;
class BufferEventEv : public BufferEvent {
public:
	BufferEventEv(IOEventHandler* ev, int protocol, int fd);
	~BufferEventEv();
	
	virtual void Enable(int flag) override;
	virtual void Disable(int flag) override;

	virtual int CopyReadBuff(char* copy, size_t size) override;
	virtual size_t GetReadBuffLen() override;

	virtual size_t Read(void* data, size_t size) override;
	virtual int Write(uint64_t remote, const void* data, size_t size) override;

public:
	int RecvData();
	int FlushSendBuffer();

private:
	int recvTcpData();
	int recvUdpData();
	int sendTcpData(const void* data, size_t size);
	int sendUdpData(uint64_t remote, const void* data, size_t size);

private:
	ev_io*					evrd_;
	ev_io*					evwr_;
	BlockBuffer*			recvBuffer_;
	BlockBuffer*			sendBuffer_;
};

// ListenEventEv
class ListenEventEv : public ListenEvent {
public:
	ListenEventEv(IOEventHandler* ev, int protocol, int lfd);
	~ListenEventEv();

	virtual void Start() override;
	void DoCallback(int sockfd);

private:
	ev_io*				evio_;
};

// IOEventHandler
class IOEventEvHandler : public IOEventHandler {
public:
	IOEventEvHandler();
	~IOEventEvHandler();

	struct ev_loop* GetEvLoop() {
		return evloop_;
	}

	virtual void EvRun() override;
	virtual void EvStop() override;

	virtual int Bind(int protocol, const char* ip, int port) override;
	virtual BufferEvent* Connect(int protocol, const char* ip, int port) override;

	virtual ListenEvent* NewListenEvent(int protocol, int fd) override;
	virtual BufferEvent* NewBufferEvent(int protocol, int fd) override;
	virtual TimerEvent* NewTimerEvent() override;
	virtual SignalEvent* NewSignalEvent(int signal) override;
	virtual UserEvent* NewUserEvent() override;

private:
	BufferEvent* doConnect(int protocol, const struct sockaddr* sa, int socklen);
	int doBind(int protocol, const struct sockaddr* sa, int socklen);

private:
	struct ev_loop*			evloop_;
};

// Factory
class IOEventEvHandlerFactory : public IOEventHandlerFactory {
public:
	IOEventEvHandlerFactory() : IOEventHandlerFactory() {}
	~IOEventEvHandlerFactory() {}

	virtual IOEventHandler* GetIOEventHandler() override {
		return new IOEventEvHandler();
	}
};

