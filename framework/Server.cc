#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "Log.h"
#include "Timer.h"
#include "IRpc.h"
#include "SvrClient.h"
#include "Worker.h"
#include "IniConfig.h"
#include "NetMessage.h"
#include "LoopQueue.h"
#include "IProcessor.h"
#include "IOEventHandler.h"
#include "Server.h"

using namespace std;
using namespace std::placeholders;

static void sigclose(EventBase* ev, int fd, int events, void* arg)
{
	Server* svr = (Server*)arg;
	svr->Stop();
}

Server::Server()
{
	evHandler_ = nullptr;
	worker_ = nullptr;
	sendQue_ = nullptr;
	recvQue_ = nullptr;
	netMessage_ = nullptr;
	iniCfg_ = nullptr;
}

Server::~Server()
{
	if (iniCfg_) {
		delete iniCfg_;
		iniCfg_ = nullptr;
	}
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

	recvQue_ = new LoopQueue();
	sendQue_ = new LoopQueue();
	recvQue_->Init();
	sendQue_->Init();

	timerMgr_ = new TimerMgr(evHandler_);
	rpc_ = new IRpc(this);

	worker_ = new Worker(this);
	worker_->Init(evType);
	worker_->Start();

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

bool Server::IsInWorkThread()
{
	return pthread_self() == worker_->ThreadID();
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
	string::size_type n = url.find(":");
	if (n == string::npos) {
		return -1;
	}
	
	string ipStr = url.substr(0, n);
	string portStr = url.substr(n+1);

	return netMessage_->Listen(ipStr.c_str(), atoi(portStr.c_str()));
}

int Server::ConnectSvr(const std::string& url)
{
	string::size_type n = url.find(":");
	if (n == string::npos) {
		return -1;
	}
	
	string ipStr = url.substr(0, n);
	string portStr = url.substr(n+1);

	return netMessage_->Connect(ipStr.c_str(), atoi(portStr.c_str()));
}

void Server::CloseSvr(int fd)
{
	svrClients_.erase(fd);
	netMessage_->CloseConn(fd);
}

SvrClient* Server::GetSvrClient(int fd)
{
	std::map<int, SvrClient*>::iterator it = svrClients_.find(fd);
	if (it == svrClients_.end()) {
		return nullptr;
	}

	return it->second;
}

void Server::Attach(int fd, IProcessor* processor)
{
	processors_[fd] = processor;
	processor->SetHandler(this);
}

void Server::AttachSvrClient(int fd, SvrClient* scli)
{
	svrClients_[fd] = scli;
}

int Server::AsyncSendMsg(int fd, const char* msg, int msglen)
{
	LoopQueue* sendq = GetSendQue();
	sendq->Push(fd, MT_SVR_MESSAGE, msg, msglen);
	MessageSendNoti();
	return 0;
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

int Server::OnRecvMsg(int fd, const char* msg, int msglen, bool cli)
{
	if (cli) {
		return rpc_->OnRecvCliMsg(fd, msg, msglen);
	}
		
	std::map<int, SvrClient*>::iterator it = svrClients_.find(fd);
	if (it == svrClients_.end()) {
		return -1;
	}
	return it->second->OnRecvMsg(msg, msglen);
}

void Server::OnSvrClosed(int fd)
{
	std::map<int, SvrClient*>::iterator it = svrClients_.find(fd);
	if (it != svrClients_.end()) {
		it->second->OnClosed();
		svrClients_.erase(fd);
	}
}

void Server::MessageRecvNoti()
{
	if (worker_) {
		worker_->EvAsyncSend();
	}
}

void Server::MessageSendNoti()
{
	if (netMessage_) {
		netMessage_->EvAsyncSend();
	}
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

IProcessor* Server::GetProcessor(int fd)
{
	std::map<int, IProcessor*>::iterator it = processors_.find(fd);
	if (it != processors_.end()) {
		return it->second;
	}
	return nullptr;
}

int Server::onTimeEvent(int event)
{
	PLOG_DEBUG("onTimeEvent event=%u", event);
	return 0;
}

