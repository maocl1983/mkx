#pragma once
#include <functional>

enum EventWatchType {
	IOEV_READ = 0x01,
	IOEV_WRITE = 0x02,
};

enum EventStateType {
	IOEV_EVENT_EOF			= 0x01,
	IOEV_EVENT_ERROR		= 0x02,
	IOEV_EVENT_TIMEOUT		= 0x04,
	IOEV_EVENT_CONNECTED	= 0x08,
};

enum ProtocolType {
	IOEV_TCP_PROTOCOL		= 0x01,
	IOEV_UDP_PROTOCOL		= 0x02,
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

typedef std::function<void(EventBase*, int, void*)> OnEventCb;
typedef std::function<void(EventBase*, int, int, uint64_t, void*)> OnBufferCb;

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
	BufferEvent(IOEventHandler* handler, int protocol, int fd) 
		: EventBase(handler), fd_(fd), protocol_(protocol) {}
	virtual ~BufferEvent() {}

	int GetFd() {
		return fd_;
	}

	virtual void SetCb(const OnBufferCb& rcb, const OnBufferCb& wcb, const OnBufferCb& evcb, void* arg) {
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
	virtual int Write(uint64_t remote, const void* data, size_t size) = 0;

protected:
	int					fd_;
	int					protocol_;
	OnBufferCb			rdcb_;
	OnBufferCb			wrcb_;
	OnBufferCb			evcb_;
	void*				udata_;
};

// ListenEvent
class ListenEvent : public EventBase {
public:
	ListenEvent(IOEventHandler* handler, int protocol, int lfd)
		: EventBase(handler), lfd_(lfd), protocol_(protocol) {}
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
	int					protocol_;
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

	virtual int Bind(int protcol, const char* ip, int port) = 0;
	virtual BufferEvent* Connect(int protocol, const char* ip, int port) = 0;

	virtual ListenEvent* NewListenEvent(int protocol, int fd) = 0;
	virtual BufferEvent* NewBufferEvent(int protocol, int fd) = 0;
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
