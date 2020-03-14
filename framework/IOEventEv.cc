#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "Log.h"
#include "BlockBuffer.h"
#include "IOEventEv.h"

static int setIOBlock(int fd, int nonblock)
{
	int val;
	if (nonblock) {
		val = (O_NONBLOCK | fcntl(fd, F_GETFL));
	} else {
		val = (~O_NONBLOCK & fcntl(fd, F_GETFL));
	}
	return fcntl(fd, F_SETFL, val);
}

static void
userCb(EV_P_ ev_async* w, int revents)
{
	UserEventEv* uev = (UserEventEv*)(w->data);
	uev->DoCallback(revents);
}

static void
signalCb(EV_P_ ev_signal* w, int revents)
{
	SignalEventEv* sev = (SignalEventEv*)(w->data);
	sev->DoCallback();
}

static void
timerCb(EV_P_ ev_timer* w, int revents)
{
	TimerEventEv* tev = (TimerEventEv*)(w->data);
	tev->DoCallback();
}

static void 
readCb(EV_P_ ev_io* w, int revents)
{
	BufferEventEv* bev = (BufferEventEv*)(w->data);
	bev->RecvData();
}

static void 
writeCb(EV_P_ ev_io* w, int revents)
{
	BufferEventEv* bev = (BufferEventEv*)(w->data);
	bev->FlushSendBuffer();
}

static void 
listenCb(EV_P_ ev_io* w, int revents)
{
	ListenEventEv* lev = (ListenEventEv*)(w->data);
	
	struct sockaddr_in peer;
	int newfd = -1;
	while (1) {
		socklen_t peerlen = sizeof(peer);
		newfd = accept(w->fd, reinterpret_cast<struct sockaddr*>(&peer), &peerlen);
		if (newfd > 0) {
			break;
		} else if (errno != EINTR) {
			//return -1;
			return; //TODO:
		}
	}
	setIOBlock(newfd, 1);

	lev->DoCallback(newfd);
}

////////////////////////////////////////////////////////////
UserEventEv::UserEventEv(IOEventHandler* ev)
	: UserEvent(ev)
{
	evasync_ = (ev_async*)malloc(sizeof(ev_async));
	evasync_->data = (void*)this;
}

UserEventEv::~UserEventEv()
{
	if (evasync_) {
		ev_async_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evasync_);
		delete evasync_;
	}
}

void UserEventEv::Start()
{
	ev_async_init(evasync_, userCb);
	ev_async_start(((IOEventEvHandler*)handler_)->GetEvLoop(), evasync_);
}

void UserEventEv::Trigger()
{
	ev_async_send(((IOEventEvHandler*)handler_)->GetEvLoop(), evasync_);
}

void UserEventEv::DoCallback(int shorts)
{
	if (usercb_) {
		usercb_(this, shorts, udata_);
	}
}

////////////////////////////////////////////////////////////
SignalEventEv::SignalEventEv(IOEventHandler* ev, int signal)
	: SignalEvent(ev, signal)
{
	evsignal_ = (ev_signal*)malloc(sizeof(ev_signal));
	evsignal_->data = (void*)this;
}

SignalEventEv::~SignalEventEv()
{
	if (evsignal_) {
		ev_signal_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evsignal_);
		free(evsignal_);
	}

	evsignal_ = nullptr;
}

void SignalEventEv::Start()
{
	ev_signal_init(evsignal_, signalCb, signal_);
	ev_signal_start(((IOEventEvHandler*)handler_)->GetEvLoop(), evsignal_);
}

void SignalEventEv::DoCallback()
{
	if (signalcb_) {
		signalcb_(this, signal_, udata_);
	}
}

////////////////////////////////////////////////////////////
TimerEventEv::TimerEventEv(IOEventHandler* ev)
	: TimerEvent(ev)
{
	evtimer_ = (ev_timer*)malloc(sizeof(ev_timer));
	evtimer_->data = (void*)this;

	started_ = false;
	repeat_ = -1.0;
}

TimerEventEv::~TimerEventEv()
{
	if (evtimer_) {
		ev_timer_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evtimer_);
		free(evtimer_);
	}

	evtimer_ = nullptr;
}

void TimerEventEv::Start(double after, double repeat)
{
	ev_timer_init(evtimer_, timerCb, after, repeat);
	ev_timer_start(((IOEventEvHandler*)handler_)->GetEvLoop(), evtimer_);

	repeat_ = repeat;
	started_ = true;
}

void TimerEventEv::DoCallback()
{
	if (timercb_) {
		timercb_(this, 0, udata_);
	}
}

////////////////////////////////////////////////////////////
BufferEventEv::BufferEventEv(IOEventHandler* ev, int protocol, int fd)
	: BufferEvent(ev, protocol, fd)
{
	evrd_ = nullptr;
	evwr_ = nullptr;
	recvBuffer_ = new BlockBuffer();
	sendBuffer_ = new BlockBuffer();
}

BufferEventEv::~BufferEventEv()
{
	if (evrd_) {
		ev_io_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evrd_);
		free(evrd_);
		evrd_ = nullptr;
	}
	if (evwr_) {
		ev_io_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evwr_);
		free(evwr_);
		evwr_ = nullptr;
	}

	delete recvBuffer_;
	delete sendBuffer_;

	close(fd_);
	fd_ = -1;
}

void BufferEventEv::Enable(int flag)
{
	if (flag & IOEV_READ) {
		if (evrd_ == nullptr) {
			evrd_ = (ev_io*)malloc(sizeof(ev_io));
			evrd_->data = this;
			ev_io_init(evrd_, readCb, fd_, EV_READ);
		}
		ev_io_start(((IOEventEvHandler*)handler_)->GetEvLoop(), evrd_);
	}
	if (flag & IOEV_WRITE) {
		if (evwr_ == nullptr) {
			evwr_ = (ev_io*)malloc(sizeof(ev_io));
			evwr_->data = this;
			ev_io_init(evwr_, writeCb, fd_, EV_WRITE);
		}
		//ev_io_start(((IOEventEvHandler*)handler_)->GetEvLoop(), evwr_);
	}
}

void BufferEventEv::Disable(int flag)
{
	if (flag & IOEV_READ) {
		ev_io_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evrd_);
	}
	if (flag & IOEV_WRITE) {
		ev_io_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evwr_);
	}
}

int BufferEventEv::CopyReadBuff(char* copy, size_t size)
{
	assert(size > 0);
	return recvBuffer_->Copy(copy, size);
}

size_t BufferEventEv::GetReadBuffLen()
{
	return recvBuffer_->GetTotalLen();
}

size_t BufferEventEv::Read(void* data, size_t size)
{
	assert(size > 0);
	return recvBuffer_->Move(data, size);
}

int BufferEventEv::Write(uint64_t remote, const void* data, size_t size)
{
	assert(size > 0);

	if (protocol_ == IOEV_TCP_PROTOCOL) {
		sendTcpData(data, size);
	} else {
		sendUdpData(remote, data, size);
	}
	return 0;
}

int BufferEventEv::RecvData()
{
	if (protocol_ == IOEV_TCP_PROTOCOL) {
		return recvTcpData();
	} else {
		return recvUdpData();
	}

	return 0;
}

int BufferEventEv::recvTcpData()
{
	while (true) {
		int slen = 0;
		if (ioctl(fd_, FIONREAD, &slen) < 0) {
			goto recv_failed;
		}
		char* buffer = recvBuffer_->GetBufferToWrite(slen);
		int rlen = recv(fd_, buffer, slen, 0);
		if (rlen == 0) {
			// disconnected:
			goto recv_failed;
		} else if (rlen < 0) {
			if (errno == EINTR) {
				continue;
			}
		} else {
			recvBuffer_->FinishBufferWrite(rlen);
		}
		break;
	}

	if (rdcb_) {
		rdcb_(this, fd_, 0, 0, udata_);
	}
	return 0;

recv_failed:
	if (evcb_) {
		evcb_(this, fd_, IOEV_EVENT_EOF, 0, udata_);
	}
	return -1;
}

int BufferEventEv::recvUdpData()
{
	while (true) {
		int slen = 0;
		if (ioctl(fd_, FIONREAD, &slen) < 0) {
			goto recv_failed;
		}

		struct sockaddr_in remote;
		socklen_t rmtlen = sizeof(remote);
		uint64_t rmtaddr = 0;
		char* buffer = recvBuffer_->GetBufferToWrite(slen);
		int rlen = recvfrom(fd_, buffer, slen, 0, (struct sockaddr*)(&remote), &rmtlen);
		if (rlen < 0) {
			if (errno == EINTR) {
				continue;
			}
		}

		rmtaddr = (uint64_t)remote.sin_addr.s_addr << 32;
		rmtaddr |= remote.sin_port;
		recvBuffer_->FinishBufferWrite(rlen);

		if (rdcb_) {
			rdcb_(this, fd_, 0, rmtaddr, udata_);
		}

		break;
	}

	return 0;

recv_failed:
	if (evcb_) {
		evcb_(this, fd_, IOEV_EVENT_EOF, 0, udata_);
	}
	return -1;
}

int BufferEventEv::FlushSendBuffer()
{
	while (sendBuffer_->GetTotalLen() > 0) {
		int bufferlen = 0, sendbytes = 0;
		char* buffer = sendBuffer_->GetBufferToRead(&bufferlen);
		assert(buffer && bufferlen > 0);
		while (sendbytes < bufferlen) {
			int len = send(fd_, buffer + sendbytes, bufferlen - sendbytes, 0);
			if (len < 0) {
				if (errno == EINTR) {
					len = 0;
				} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
					break;
				} else {
					if (evcb_) {
						evcb_(this, fd_, IOEV_EVENT_EOF, 0, udata_);
					}
					return -1;
				}
			}
			sendbytes += len;
			sendBuffer_->Move(nullptr, len);
		}
	}

	return 0;
}

int BufferEventEv::sendTcpData(const void* data, size_t size)
{
	int ret = FlushSendBuffer();
	if (ret < 0) {
		return ret;
	}

	if (sendBuffer_->GetTotalLen() > 0) {
		sendBuffer_->Insert(data, size);
		return 0;
	} else {
		if (ev_is_active(evwr_)) {
			ev_io_stop(((IOEventEvHandler*)handler_)->GetEvLoop(), evwr_);
		}
	}

	size_t sendbytes = 0;
	while (sendbytes < size) {
		int len = send(fd_, (char*)data + sendbytes, size - sendbytes, 0);
		if (len < 0) {
			if (errno == EINTR) {
				len = 0;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			} else {
				if (evcb_) {
					evcb_(this, fd_, IOEV_EVENT_EOF, 0, udata_);
				}
				return -1;
			}
		}
		sendbytes += len;
	}
	if (sendbytes < size) {
		sendBuffer_->Insert((char*)data + sendbytes, size - sendbytes);
		ev_io_start(((IOEventEvHandler*)handler_)->GetEvLoop(), evwr_);
	}

	return 0;
}

int BufferEventEv::sendUdpData(uint64_t remote, const void* data, size_t size)
{
	struct sockaddr_in rmtaddr;
	rmtaddr.sin_family = AF_INET;
	rmtaddr.sin_addr.s_addr = remote >> 32;
	rmtaddr.sin_port = remote & 0x00000000FFFFFFFF;
	socklen_t rmtlen = sizeof(rmtaddr);

	size_t sendlen = 0;
	while (sendlen < size) {
		sendlen = sendto(fd_, (char*)data + sendlen, size - sendlen, 0, (struct sockaddr*)&rmtaddr, rmtlen);
		if (sendlen < 0) {
			if (evcb_) {
				evcb_(this, fd_, IOEV_EVENT_EOF, 0, udata_);
				return -1;
			}
		}
	}

	if (wrcb_) {
		wrcb_(this, fd_, 0, 0, udata_);
	}

	return 0;
}

////////////////////////////////////////////////////////////
ListenEventEv::ListenEventEv(IOEventHandler* ev, int protocol, int lfd)
	: ListenEvent(ev, protocol, lfd)
{
	evio_ = nullptr;
}

ListenEventEv::~ListenEventEv()
{

}

void ListenEventEv::Start()
{
	evio_ = (ev_io*)malloc(sizeof(ev_io));
	evio_->data = (void*)this;
	ev_io_init(evio_, listenCb, lfd_, EV_READ);
	ev_io_start(((IOEventEvHandler*)handler_)->GetEvLoop(), evio_);
}

void ListenEventEv::DoCallback(int sockfd)
{
	listencb_(this, sockfd, udata_);
}

////////////////////////////////////////////////////////////
IOEventEvHandler::IOEventEvHandler()
{
	evloop_ = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);
}

IOEventEvHandler::~IOEventEvHandler()
{

}
	
void IOEventEvHandler::EvRun()
{
	ev_run(evloop_, 0);
}
	
void IOEventEvHandler::EvStop()
{
	ev_break(evloop_, EVBREAK_ALL);
}
	
int IOEventEvHandler::Bind(int protocol, const char* ip, int port)
{
	struct sockaddr_in addr;
	memset((char*)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &addr.sin_addr) < 0) {
		return -1;
	}

	return doBind(protocol, (struct sockaddr*)&addr, sizeof(addr));
}

BufferEvent* IOEventEvHandler::Connect(int protocol, const char* ip, int port)
{
	struct sockaddr_in peer;
	memset(&peer, 0, sizeof(peer));
	peer.sin_family = AF_INET;
	peer.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &peer.sin_addr) < 0) {
		return nullptr;
	}

	return doConnect(protocol, (struct sockaddr*)(&peer), sizeof(peer));
}

ListenEvent* IOEventEvHandler::NewListenEvent(int protocol, int sockfd)
{
	ListenEventEv* lev = new ListenEventEv(this, protocol, sockfd);
	return lev;
}

BufferEvent* IOEventEvHandler::NewBufferEvent(int protocol, int sockfd)
{
	BufferEventEv* bev = new BufferEventEv(this, protocol, sockfd);
	return bev;
}

TimerEvent* IOEventEvHandler::NewTimerEvent()
{
	TimerEventEv* bev = new TimerEventEv(this);
	return bev;
}

SignalEvent* IOEventEvHandler::NewSignalEvent(int signal)
{
	SignalEventEv* bev = new SignalEventEv(this, signal);
	return bev;
}

UserEvent* IOEventEvHandler::NewUserEvent()
{
	UserEventEv* bev = new UserEventEv(this);
	return bev;
}

BufferEvent* IOEventEvHandler::doConnect(int protocol, const struct sockaddr* sa, int socklen)
{
	int socktype = protocol == IOEV_TCP_PROTOCOL ? SOCK_STREAM : SOCK_DGRAM;
	int fd = socket(PF_INET, socktype, 0);
	if (fd < 0) {
		return nullptr;
	}

	if (protocol == IOEV_TCP_PROTOCOL && connect(fd, sa, socklen) == -1) {
		close(fd);
		return nullptr;
	}
	setIOBlock(fd, 1);

	BufferEvent* bev = NewBufferEvent(protocol, fd);
	return bev;
}

int IOEventEvHandler::doBind(int protocol, const struct sockaddr* sa, int socklen)
{
	int socktype = protocol == IOEV_TCP_PROTOCOL ? SOCK_STREAM : SOCK_DGRAM;
	int lfd = socket(AF_INET, socktype, 0);
	if (lfd < 0) {
		PLOG_ERROR("socket error in %d", errno);
		return -1;
	}
	
	int flag = 1;
	int err = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if (err == -1) {
		PLOG_ERROR("setsockopt error in %d", errno);
		close(lfd);
		return -1;
	}

	int ret = bind(lfd, reinterpret_cast<const struct sockaddr*>(sa), socklen);

	if (ret == 0 && protocol == IOEV_TCP_PROTOCOL) {
		ret = listen(lfd, 1024);
	}

	if (ret < 0) {
		PLOG_ERROR("listen error in %d", errno);
		close(lfd);
		return -1;
	}

	PLOG_DEBUG("server bind succ! fd=%d", lfd);
	return lfd;
}


