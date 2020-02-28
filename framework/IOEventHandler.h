#pragma once
#include <functional>

enum {
	IOEV_READ = 0x01,
	IOEV_WRITE = 0x02,
};

enum {
	IOEV_EVENT_EOF			= 0x01,
	IOEV_EVENT_ERROR		= 0x02,
	IOEV_EVENT_TIMEOUT		= 0x04,
	IOEV_EVENT_CONNECTED	= 0x08,
};

class IOEventHandler;
// EventBase
class EventBase {
public:
	EventBase(IOEventHandler* handler) {
		handler_ = handler;
	}
	virtual ~EventBase() {}

	IOEventHandler* GetHandler() {
		return handler_;
	}

protected:
	IOEventHandler*	handler_;
};
typedef std::function<void(EventBase*, int, int, void*)> OnEventCb;

// UserEvent
class UserEvent : public EventBase {
public:
	UserEvent(IOEventHandler* handler)
		: EventBase(handler) {}
	virtual ~UserEvent() {}

	virtual void SetCb(const OnEventCb& cb, void* arg) {
		usercb_ = cb;
		udata_ = arg;
	}
	virtual void Start() = 0;
	virtual void Trigger() = 0;

protected:
	OnEventCb			usercb_;
	void*				udata_;

};

// SignalEvent
class SignalEvent : public EventBase {
public:
	SignalEvent(IOEventHandler* handler, int signal)
		: EventBase(handler), signal_(signal) {}
	virtual ~SignalEvent() {}

	virtual void SetCb(const OnEventCb& cb, void* arg) {
		signalcb_ = cb;
		udata_ = arg;
	}
	virtual void Start() = 0;

protected:
	int					signal_;
	OnEventCb			signalcb_;
	void*				udata_;
};

// TimerEvent
class TimerEvent : public EventBase {
public:
	TimerEvent(IOEventHandler* handler)
		: EventBase(handler) {}
	virtual ~TimerEvent() {}
	
	virtual void SetCb(const OnEventCb& cb, void* arg) {
		timercb_ = cb;
		udata_ = arg;
	}
	virtual void Start(double after, double repeat) = 0;

protected:
	OnEventCb			timercb_;
	void*				udata_;
};

// BufferEvent
class BufferEvent : public EventBase {
public:
	BufferEvent(IOEventHandler* handler, int fd) 
		: EventBase(handler), fd_(fd) {}
	virtual ~BufferEvent() {}

	int GetFd() {
		return fd_;
	}

	virtual void SetCb(const OnEventCb& rcb, const OnEventCb& wcb, const OnEventCb& evcb, void* arg) {
		rdcb_ = rcb;
		wrcb_ = wcb;
		evcb_ = evcb;
		udata_ = arg;
	}

	virtual void Enable(int flag) = 0;
	virtual void Disable(int flag) = 0;
	
	virtual int CopyReadBuff(char* copy, size_t size) = 0;
	virtual size_t GetReadBuffLen() = 0;

	virtual size_t Read(void* data, size_t size) = 0;
	virtual int Write(const void* data, size_t size) = 0;

protected:
	int					fd_;
	OnEventCb			rdcb_;
	OnEventCb			wrcb_;
	OnEventCb			evcb_;
	void*				udata_;
};

// ListenEvent
class ListenEvent : public EventBase {
public:
	ListenEvent(IOEventHandler* handler, int lfd)
		: EventBase(handler), lfd_(lfd) {}
	~ListenEvent() {}

	void SetCb(const OnEventCb& lcb, void* arg) {
		listencb_ = lcb;
		udata_ = arg;
	}

	int GetListenFd() {
		return lfd_;
	}

	virtual void Start() = 0;

protected:
	int					lfd_;
	OnEventCb			listencb_;
	void* 				udata_;
};

// IOEventHandler
class IOEventHandler {
public:
	IOEventHandler() {}
	virtual ~IOEventHandler() {}

	virtual void EvRun() = 0;
	virtual void EvStop() = 0;

	virtual int EvListenIP(const char* ip, int port, const OnEventCb& lcb, void* udata) = 0;
	virtual BufferEvent* ConnectIP(const char* ip, int port) = 0;

	virtual BufferEvent* NewBufferEvent(int fd) = 0;
	virtual TimerEvent* NewTimerEvent() = 0;
	virtual SignalEvent* NewSignalEvent(int signal) = 0;
	virtual UserEvent* NewUserEvent() = 0;
};

// Factory
class IOEventHandlerFactory {
public:
	IOEventHandlerFactory() {}
	virtual ~IOEventHandlerFactory() {}

	virtual IOEventHandler* GetIOEventHandler() = 0;
};

int SetIOEventHandlerFactory(int type, IOEventHandlerFactory* factory);
IOEventHandlerFactory* GetIOEventHandlerFactory(int type);
