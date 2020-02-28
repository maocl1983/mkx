#include "Server.h"
#include "IProcessor.h"

IProcessor::IProcessor(int fd)
	: fd_(fd)
{

}

IProcessor::~IProcessor()
{

}

void IProcessor::SetHandler(Server* handler)
{
	handler_ = handler;
}

int IProcessor::SendMsg(const char* msg, int msglen)
{
	if (handler_) {
		return handler_->AsyncSendMsg(fd_, msg, msglen);
	}
	return 0;
}
