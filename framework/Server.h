#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>

class IRpc;
class Worker;
class TimerMgr;
class SvrConnector;
class IniConfig;
class NetMessage;
class LoopQueue;
class MutexLocker;
class IOEventHandler;

enum EventType {
	IOEVENT_TYPE_EV		= 1,
	IOEVENT_TYPE_LEV	= 2,
};

enum ServerModeType {
	SERVER_MODE_SINGLE	= 1,
	SERVER_MODE_MULTI	= 2,
};

class Server {
public:
	Server();
	~Server();

	void Init(int evType = IOEVENT_TYPE_EV);
	void Start();
	void Stop();

	ServerModeType ModeType();
	bool IsMainThread();

	int LoadCfg(const std::string& filename);
	int Bind(const std::string& url);
	int ConnectSvr(const std::string& url, std::function<void(int)> closeCb);
	void CloseSvr(int fd);

	TimerMgr* GetTimerMgr();
	IRpc* GetRpc();
	const IniConfig* GetCfg();
	IOEventHandler*	GetIOEvHandler();

	LoopQueue* GetRecvQue();
	LoopQueue* GetSendQue();
	MutexLocker* GetRecvLocker();
	MutexLocker* GetSendLocker();

	int SendMsgToSvr(int fd, const char* msg, int msglen);
	int SendMsg(int fd, uint64_t remote, const char* msg, int msglen);
	int OnRecvMsg(int fd, uint64_t remote, const char* msg, int msglen, bool cli);
	void OnCliConnected(int fd);
	void OnCliClosed(int fd);
	void OnSvrClosed(int fd);

private:
	SvrConnector* GetSvrConnector(int fd);

	void daemonize();
	void setSignal();

private:
	int onTimeEvent(int event);

private:
	ServerModeType					modeType_;
	pthread_t						mainThreadId_;
	IRpc*							rpc_;
	IniConfig*						iniCfg_;
	NetMessage*						netMessage_;
	TimerMgr*						timerMgr_;
	IOEventHandler*					evHandler_;
	std::vector<Worker*>			workers_;
	std::map<int, SvrConnector*>	svrConnectors_;

	LoopQueue*						recvQue_;
	LoopQueue*						sendQue_;
	MutexLocker*					recvLocker_;
	MutexLocker*					sendLocker_;
};

