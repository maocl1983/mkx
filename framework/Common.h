#pragma once

#include "IOEventHandler.h"
#include "IOEventEv.h"
#include "Server.h"

#define INSTALL_IOEVENTEV \
	do { \
		if (!GetIOEventHandlerFactory(IOEVENT_TYPE_EV)) { \
			SetIOEventHandlerFactory(IOEVENT_TYPE_EV, new IOEventEvHandlerFactory()); \
		} \
	} while(0)


