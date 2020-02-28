#pragma once

#include "IOEventHandler.h"
#include "IOEventEv.h"
#include "Server.h"

#define INSTALL_IOEVENTEV \
	do { \
		if (!GetIOEventHandlerFactory(kIOEvent_Ev)) { \
			SetIOEventHandlerFactory(kIOEvent_Ev, new IOEventEvHandlerFactory()); \
		} \
	} while(0)


