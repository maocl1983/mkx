#pragma once
#include <string>

class IRpc;
class IRpcService {
public:
	IRpcService() {}
	IRpcService(IRpc* handler) : handler_(handler) {}
	virtual ~IRpcService(){}

	void AttachHandler(IRpc* rpc) {
		handler_ = rpc;
	}
	virtual int RegisterServiceFunction() = 0;
	virtual std::string Name() = 0;

protected:
	IRpc*	handler_;
};

