#pragma once
#include <string>
#include <map>

class IRpc;
class Worker;
class TimerMgr;
class SvrClient;
class IniConfig;
class NetMessage;
class LoopQueue;
class IProcessor;
class IOEventHandler;

enum {
	kIOEvent_Ev		= 1,
	KIOEvent_Lev	= 2,
};

class Server {
public:
	Server();
	~Server();

	void Init(int evType = kIOEvent_Ev);
	void Start();
	void Stop();

	bool IsInWorkThread();

	int LoadCfg(const std::string& filename);
	int Bind(const std::string& url);
	int ConnectSvr(const std::string& url);
	void Attach(int fd, IProcessor* processor);

	void AttachSvrClient(int fd, SvrClient* scli);
	void CloseSvr(int fd);
	SvrClient* GetSvrClient(int fd);

	int AsyncSendMsg(int fd, const char* msg, int msglen);

	TimerMgr* GetTimerMgr();
	IRpc* GetRpc();
	LoopQueue* GetRecvQue();
	LoopQueue* GetSendQue();
	const IniConfig* GetCfg();
	IOEventHandler*	GetIOEvHandler();

	int OnRecvMsg(int fd, const char* msg, int msglen, bool cli);
	void OnSvrClosed(int fd);
	void MessageRecvNoti();
	void MessageSendNoti();

private:
	void daemonize();
	void setSignal();
	IProcessor* GetProcessor(int fd);

private:
	int onTimeEvent(int event);

private:
	IRpc*						rpc_;
	Worker*						worker_;
	IniConfig*					iniCfg_;
	NetMessage*					netMessage_;
	LoopQueue*					recvQue_;
	LoopQueue*					sendQue_;
	TimerMgr*					timerMgr_;
	IOEventHandler*				evHandler_;
	std::map<int, IProcessor*>	processors_;
	std::map<int, SvrClient*>	svrClients_;
};

