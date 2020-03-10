#pragma once
#include <stdint.h>
#include <functional>

class LoopQueue;
class Server;
class IOEventHandler;
class UserEvent;
class BufferEvent;
struct conn_info;

class NetMessage {
public:
	NetMessage(Server* server);
	~NetMessage();

	IOEventHandler* GetIOEventHandler();	

	int Listen(const char* ip, int port);
	int Connect(const char* ip, int port);
	int SendMsg(int fd, const char* msg, int msglen);

public:
	int OpenConn(int fd, BufferEvent* bev, int listenfd = -1);
	struct conn_info* GetConn(int fd);
	int CloseConn(int fd);
	int RecvMsg(int fd);

	int OnConnClosed(int fd);
	void OnSendMsgEvent();
	void SendQueNoti();

private:
	Server*				server_;
	UserEvent*			msgEvent_;
	struct conn_info*	conns_;

	char*				recvBuffer_;
	LoopQueue*			sendQue_;
	LoopQueue*			recvQue_;

	int					maxfds_;
	uint32_t			maxMsglen_;
};
