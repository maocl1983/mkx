#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "common/Log.h"
#include "framework/Common.h"
#include "framework/IRpc.h"
#include "framework/IniConfig.h"
#include "protocc/game.pb.h"
#include "protocc/game.rpc.pb.h"

class GameServiceImp : public gs::GameServiceServerInterface {
public:
	typedef gs::GameServiceServerInterface::RspCb RspCb;
	virtual void login(int fd, uint64_t remote, const gs::LoginRequest& req, const RspCb& cb) override {
		PLOG_DEBUG("login fd=%d plid=%u", fd, req.player_id());
		gs::LoginResponse rsp;
		gs::PlayerInfo* pinfo = rsp.mutable_pinfo();
		pinfo->set_id(req.player_id());
		pinfo->set_nick("markmao");
		pinfo->set_age(24);
		cb(fd, remote, 0, rsp);
	}
};

int main()
{
	Server server;
	server.LoadCfg("cfg.ini");
	INSTALL_IOEVENTEV;
	server.Init();

	std::string url = server.GetCfg()->GetStr("bindurl");
	server.Bind(url);

	GameServiceImp gameservice;
	server.GetRpc()->AddService(&gameservice);

	server.Start();

	return 0;
}

