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
	int OnConnClosed(int fd);

public:
	int OpenConn(int fd, BufferEvent* bev, int listenfd = -1);
	struct conn_info* GetConn(int fd);
	int CloseConn(int fd);
	int DealRecvMsg(int fd, const char* msg, int msglen);
	int RecvMsg(int fd);
	void EvAsyncSend();
	void OnSendMsg();

private:
	Server*				server_;
	UserEvent*			msgEvent_;
	struct conn_info*	conns_;
	LoopQueue*			sendQue_;
	LoopQueue*			recvQue_;
	int					maxfds_;
};
