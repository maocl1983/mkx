#include <map>
#include "IOEventHandler.h"

std::map<int, IOEventHandlerFactory*> gFactorys;

int SetIOEventHandlerFactory(int type, IOEventHandlerFactory* factory)
{
	std::map<int, IOEventHandlerFactory*>::iterator it;
	it = gFactorys.find(type);
	if (it != gFactorys.end()) {
		return -1;
	}

	gFactorys[type] = factory;
	return 0;
}

IOEventHandlerFactory* GetIOEventHandlerFactory(int type)
{
	std::map<int, IOEventHandlerFactory*>::iterator it;
	it = gFactorys.find(type);
	if (it != gFactorys.end()) {
		return it->second;
	}

	return nullptr;
}
