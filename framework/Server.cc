#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "Log.h"
#include "Locker.h"
#include "Timer.h"
#include "IRpc.h"
#include "SvrConnector.h"
#include "Worker.h"
#include "IniConfig.h"
#include "NetMessage.h"
#include "LoopQueue.h"
#include "IOEventHandler.h"
#include "Server.h"

using namespace std;
using namespace std::placeholders;

static void sigclose(EventBase* ev, int events, void* arg)
{
	Server* svr = (Server*)arg;
	svr->Stop();
}

Server::Server()
{
	modeType_ = SERVER_MODE_SINGLE;
	mainThreadId_ = pthread_self();

	evHandler_ = nullptr;
	netMessage_ = nullptr;
	iniCfg_ = nullptr;
	
	sendQue_ = nullptr;
	recvQue_ = nullptr;
	recvLocker_ = nullptr;
	sendLocker_ = nullptr;
}

Server::~Server()
{
	if (iniCfg_) {
		delete iniCfg_;
		iniCfg_ = nullptr;
	}

	for (auto w : workers_) {
		delete w;
	}
	workers_.clear();
}

void Server::Init(int evType)
{
	evHandler_ = GetIOEventHandlerFactory(evType)->GetIOEventHandler();

	setSignal();
	daemonize();

	if (!iniCfg_) {
		iniCfg_ = new IniConfig();
	}

	Log::Instance().SetLogDir(iniCfg_->GetStr("log_dir").c_str());
	Log::Instance().SetLogSize(iniCfg_->GetInt("log_size", 0) * 1024 * 1024);
	Log::Instance().SetLogPriority((LogPriority)iniCfg_->GetInt("log_level", 2));

	int workCnt = iniCfg_->GetInt("work_num", 0);
	if (workCnt > 0) {
		modeType_ = SERVER_MODE_MULTI;
		recvQue_ = new LoopQueue();
		sendQue_ = new LoopQueue();
		recvQue_->Init();
		sendQue_->Init();
		recvLocker_ = new MutexLocker();
		sendLocker_ = new MutexLocker();
	}

	timerMgr_ = new TimerMgr(evHandler_);
	rpc_ = new IRpc(this);

	for (int i = 0; i < workCnt; i++) {
		Worker* worker = new Worker(this);
		worker->Init();
		worker->Start();
		workers_.push_back(worker);
	}

	netMessage_ = new NetMessage(this);

	//timer_->AddTimer(bind(&Server::onTimeEvent, this, 1), 1.0, 3.0);
}

void Server::Start()
{
	evHandler_->EvRun();
}
	
void Server::Stop()
{
	evHandler_->EvStop();
}

ServerModeType Server::ModeType()
{
	return modeType_;
}

bool Server::IsMainThread()
{
	return pthread_self() == mainThreadId_;
}

int Server::LoadCfg(const std::string& filename)
{
	if (!iniCfg_) {
		iniCfg_ = new IniConfig();
	}

	return iniCfg_->Parse(filename);
}

int Server::Bind(const string& url)
{
	string::size_type n1 = url.find("://");
	if (n1 == string::npos) {
		return -1;
	}
	string protocolStr = url.substr(0, n1);

	string::size_type ns = n1 + 3;
	string::size_type n2 = url.find(":", ns);
	if (n2 == string::npos) {
		return -1;
	}
	string ipStr = url.substr(ns, n2 - ns);
	string portStr = url.substr(n2 + 1);

	int protocol = IOEV_TCP_PROTOCOL;
	if (protocolStr.compare("udp") == 0) {
		protocol = IOEV_UDP_PROTOCOL;
	}
	return netMessage_->Bind(protocol, ipStr.c_str(), atoi(portStr.c_str()));
}

int Server::ConnectSvr(const std::string& url, std::function<void(int)> closeCb)
{
	string::size_type n1 = url.find("://");
	if (n1 == string::npos) {
		return -1;
	}
	string protocolStr = url.substr(0, n1);

	string::size_type ns = n1 + 3;
	string::size_type n2 = url.find(":", ns);
	if (n2 == string::npos) {
		return -1;
	}
	string ipStr = url.substr(ns, n2 - ns);
	string portStr = url.substr(n2 + 1);

	int protocol = IOEV_TCP_PROTOCOL;
	if (protocolStr.compare("udp") == 0) {
		protocol = IOEV_UDP_PROTOCOL;
	}

	const char* ip = ipStr.c_str();
	int port = atoi(portStr.c_str());
	int fd = netMessage_->Connect(protocol, ip, port);
	if (fd > 0) {
		SvrConnector* connector = new SvrConnector(protocol, ip, port, this);
		connector->SetFd(fd);
		connector->SetClosedCb(closeCb);
		svrConnectors_.emplace(fd, connector);
	}

	return fd;
}

void Server::CloseSvr(int fd)
{
	netMessage_->CloseConn(fd);
}

int Server::SendMsgToSvr(int fd, const char* msg, int msglen) 
{
	std::map<int, SvrConnector*>::iterator it = svrConnectors_.find(fd);
	if (it == svrConnectors_.end()) {
		return -1;
	}

	uint64_t remote = 0;
	if (it->second->GetProtocol() == IOEV_UDP_PROTOCOL) {
		remote = it->second->GetRemote();
	}

	return SendMsg(fd, remote, msg, msglen);
}

int Server::SendMsg(int fd, uint64_t remote, const char* msg, int msglen)
{
	if (modeType_ == SERVER_MODE_SINGLE || IsMainThread()) {
		return netMessage_->SendMsg(fd, remote, msg, msglen);
	}

	sendLocker_->Lock();
	bool ret = sendQue_->Push(fd, remote, MT_SVR_MESSAGE, msg, msglen);
	sendLocker_->Unlock();
	if (ret) {
		netMessage_->SendQueNoti();
	}

	return ret ? 0 : -1;
}

IOEventHandler* Server::GetIOEvHandler()
{
	return evHandler_;
}

LoopQueue* Server::GetRecvQue()
{
	return recvQue_;
}

LoopQueue* Server::GetSendQue()
{
	return sendQue_;
}

MutexLocker* Server::GetRecvLocker()
{
	return recvLocker_;
}

MutexLocker* Server::GetSendLocker()
{
	return sendLocker_;
}

IRpc* Server::GetRpc()
{
	return rpc_;
}

TimerMgr* Server::GetTimerMgr()
{
	return timerMgr_;
}

const IniConfig* Server::GetCfg()
{
	return iniCfg_;
}

int Server::OnRecvMsg(int fd, uint64_t remote, const char* msg, int msglen, bool cli)
{
	if (cli) {
		return rpc_->OnRecvCliMsg(fd, remote, msg, msglen);
	}

	std::map<int, SvrConnector*>::iterator it = svrConnectors_.find(fd);
	if (it != svrConnectors_.end()) {
		return it->second->OnRecvMsg(msg, msglen);
	}

	return -1;
}

void Server::OnCliConnected(int fd)
{
	PLOG_DEBUG("OnCliConnected fd=%d", fd);
}

void Server::OnCliClosed(int fd)
{
	PLOG_DEBUG("OnCliClosed fd=%d", fd);
}

void Server::OnSvrClosed(int fd)
{
	std::map<int, SvrConnector*>::iterator it = svrConnectors_.find(fd);
	if (it != svrConnectors_.end()) {
		delete it->second;
		svrConnectors_.erase(it);
	}
}

SvrConnector* Server::GetSvrConnector(int fd)
{
	std::map<int, SvrConnector*>::iterator it = svrConnectors_.find(fd);
	if (it != svrConnectors_.end()) {
		return it->second;
	}
	return nullptr;
}

void Server::daemonize()
{
	int fd;

	if (fork() != 0) exit(0);
	setsid();

	if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			close(fd);
	}
}

void Server::setSignal()
{
	int signal = SIGINT | SIGTERM;
	SignalEvent* evsignal = evHandler_->NewSignalEvent(signal);
	evsignal->SetCb(sigclose, this);
	evsignal->Start();
}

int Server::onTimeEvent(int event)
{
	PLOG_DEBUG("onTimeEvent event=%u", event);
	return 0;
}

