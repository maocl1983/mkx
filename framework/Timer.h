#pragma once
#include <functional>
#include <map>

typedef std::function<int()> TimerCb;

class TimerMgr;
class TimerEvent;
class IOEventHandler;

class TimerObj {
public:
	TimerObj(TimerMgr* owner, double after, double repeat, const TimerCb& cb);
	~TimerObj();

	int64_t TimerID();
	void Callback();

private:
	int64_t				guid_;
	TimerMgr* 			owner_;
	TimerEvent*  		evtimer_;
	TimerCb				cb_;
	bool				repeatFlag_;
};

class TimerMgr {
public:
	friend TimerObj;

public:
	TimerMgr(IOEventHandler* handler);
	~TimerMgr();

	int64_t AddTimer(const TimerCb& cb, double after, double repeat = -1.0);
	int	DelTimer(int64_t timerId);
	bool CheckTimer(int64_t timerId);

private:
	IOEventHandler* GetHandler();
	int64_t GetTimerID();

private:
	int64_t							timerGuid_;
	IOEventHandler*					evHandler_;
	std::map<int64_t, TimerObj*>	objs_;
};

