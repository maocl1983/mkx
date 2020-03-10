#!/usr/bin/python
# -*- coding: UTF-8 -*-

import client
import game_pb2

sock = client.Socket("172.27.0.7", 8812)
sock.connect()
for i in range(1000):
    request = game_pb2.LoginRequest()
    request.player_id = 110022 + i
    msgstr = request.SerializeToString()
    sock.send(0, "GameService::login", msgstr)
    (ret, name, datalen, data) = sock.recv()
    response = game_pb2.LoginResponse()
    response.ParseFromString(data)
    print response

