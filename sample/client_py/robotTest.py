#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os
import time
import random
import threading
import client

stopflag = False
def stop(a, b):
    global stopflag
    stopflag = True
    os.kill(os.getpid(), signal.SIGKILL)

class Robot:
    def __init__(self, ip, port, guid):
        self.ip = ip
        self.port = port
        self.guid = guid
        self.cli = client.Socket(ip, port)
        self.checkcnt = 0

    def Connect(self):
        self.cli.connect()

    def DoAction(self):
        sendbuff = ""
        lettercnt = random.randint(16, 64)
        for i in range(lettercnt):
            letter = random.choice('abcdefghijklmnopkrstuvwxyz0123456789')
            sendbuff = sendbuff + letter
        self.cli.send(sendbuff)
        #print sendbuff
        (rlen, recvbuff) = self.cli.recv()
        slen = len(sendbuff)
        #rlen = len(recvbuff)
        if rlen / slen <> 10:
            print "check error! guid=%d len=%d %d"%(self.guid, slen, rlen)
            return
        for i in range(10):
            sidx = i * slen
            eidx = sidx + slen
            if recvbuff.find(sendbuff, sidx, eidx) == -1:
                print "check error! guid=%d stop=%d"%(self.guid, i)
                return
        self.checkcnt += 1
        if self.checkcnt % 100 == 0:
            print "action ok! guid=%u cnt=%d"%(self.guid, self.checkcnt)
        #print recvbuff

class WorkThread(threading.Thread):
    def __init__(self, thread_id):
        threading.Thread.__init__(self)
        self.thread_id = thread_id
        self.robots = []

    def AddRobot(self, robot):
        self.robots.append(robot)

    def run(self):
        global stopflag
        while not stopflag:
            for r in self.robots:
                r.DoAction()
            time.sleep(random.choice([0.1,0.2,0.3]))
        


if __name__ == "__main__":
    works = []
    for i in range(10):
        work = WorkThread(i)
        for r in range(10):
            robot = Robot("172.27.0.7", 8812, i*100+r)
            robot.Connect()
            work.AddRobot(robot)
        work.setDaemon(True)
        works.append(work)

    for work in works:
        work.start()

    while True:
        alive = True
        for work in works:
            alive = alive or work.isAlive()
        if not alive:
            break

