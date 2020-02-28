#include "IOEventHandler.h"
#include "Timer.h"

using namespace std;

static void TimeoutCallback(EventBase* ev, int fd, int events, void* arg)
{
	TimerObj* tobj = reinterpret_cast<TimerObj*>(arg);
	tobj->Callback();
}

//==========================================================
TimerObj::TimerObj(TimerMgr* owner, double after, double repeat, const TimerCb& cb)
{
	cb_ = cb;
	owner_ = owner;

	guid_ = owner_->GetTimerID();

	evtimer_ = owner_->GetHandler()->NewTimerEvent();
	evtimer_->SetCb(TimeoutCallback, this);
	evtimer_->Start(after, repeat);

	repeatFlag_ = (repeat > 0.00001);
}

TimerObj::~TimerObj()
{
	if (evtimer_) {
		delete evtimer_;
		evtimer_ = nullptr;
	}

	cb_ = nullptr;
	owner_ = nullptr;
}

int64_t TimerObj::TimerID()
{
	return guid_;
}

void TimerObj::Callback()
{
	cb_();

	if (!repeatFlag_) {
		owner_->DelTimer(guid_);
	}
}

//==========================================================
TimerMgr::TimerMgr(IOEventHandler* handler)
	: timerGuid_(0), evHandler_(handler)
{
	
}

IOEventHandler* TimerMgr::GetHandler()
{
	return evHandler_;
}

int64_t TimerMgr::GetTimerID()
{
	timerGuid_++;
	if (timerGuid_ < 0) {
		timerGuid_ = 1;
	}
	return timerGuid_;
}

int64_t TimerMgr::AddTimer(const TimerCb& cb, double after, double repeat)
{
	TimerObj* tobj = new TimerObj(this, after, repeat, cb);
	objs_[tobj->TimerID()] = tobj;
	return tobj->TimerID();
}

int TimerMgr::DelTimer(int64_t timerId)
{
	map<int64_t, TimerObj*>::iterator it = objs_.find(timerId);
	if (it == objs_.end()) {
		return -1;
	}

	TimerObj* tobj = it->second;
	delete tobj;
	objs_.erase(it);

	return 0;
}

bool TimerMgr::CheckTimer(int64_t timerId)
{
	return objs_.find(timerId) != objs_.end();
}

