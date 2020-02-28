#pragma once

class Server;
class IProcessor {
public:
	friend class Server;
public:
	IProcessor(int fd);
	virtual ~IProcessor();

	int SendMsg(const char* msg, int msglen);
	virtual int OnRecvMsg(const char* msg, int msglen) = 0;
	virtual int OnConnected(int fd) = 0;
	virtual int OnClosed(int fd) = 0;

private:
	void SetHandler(Server* handler);

private:
	int 		fd_;
	Server*		handler_;
};
