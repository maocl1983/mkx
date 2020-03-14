#!/usr/bin/python
# -*- coding: UTF-8 -*-
import socket
import errno 
import struct
import time
import threading
import thread

class Socket:
    def __init__(self, ip, port, guid=0):
        self.ip_ = ip
        self.port_ = port
        self.guid_ = guid
        try:
            self.s = socket.socket() 
        except socket.error, msg:
            print("create socket error! guid=%d"%self.guid_)

    def __del__(self):
        if self.s:
            self.s.close()
            self.s = None

    def connect(self):
        if not self.s:
            self.s= socket.socket()
        try :
            self.s.connect((self.ip_, self.port_))
            self.s.settimeout(5)
        except socket.error, msg:
            self.close()
            print("socket connet err! ip=%s pt=%d guid=%d"%(self.ip_, self.port_, self.guid_))

    def send(self, ret, name, data):
        # print [msg]
        if not self.s:
            #self.connect()
            return False
        try:
            bufflen = 12 + len(name) + len(data)
            format_str = ">III%ds%ds"%(len(name), len(data))
            buff = struct.pack(format_str, bufflen, ret, len(name), name, data)
            self.s.sendall(buff)
        except socket.error, errmsg:
            print ("socket send err! ip=%s pt=%d err=%s"%(self.ip_, self.port_, errmsg))
            self.close()
            return False
        return True

    def recv(self, recv_len=1024):
        datalen = 0
        data = ""
        if not self.s:
            return data
        try:
            hstr = self.s.recv(12)
            (datalen, ret, namelen) = struct.unpack(">III", hstr)
            #print (datalen, ret, namelen)
            if namelen > 0:
                namestr = self.s.recv(namelen)
            if datalen > 12 + namelen:
                datalen = datalen - 12 - namelen
                data = self.s.recv(datalen)
        except socket.error, msg:
            print("socket error=%s guid=%d"%(msg, self.guid_))
        if not data:
            self.close()
            print("socket recv err! ip=%s pt=%d guid=%d"%(self.ip_, self.port_, self.guid_))
        return (ret, namestr, datalen, data)

    def getfd(self):
        if not self.s:
            return -1;
        return self.s.fileno()

    def close(self):
        if self.s:
            self.s.close()
            self.s = None 

class UdpSocket:
    def __init__(self, ip, port, guid=0):
        self.ip_ = ip
        self.port_ = port
        self.guid_ = guid
        try:
            self.s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
        except socket.error, msg:
            print("create socket error! guid=%d"%self.guid_)

    def __del__(self):
        if self.s:
            self.s.close()
            self.s = None

    def send(self, ret, name, data):
        # print [msg]
        if not self.s:
            return False
        try:
            bufflen = 12 + len(name) + len(data)
            format_str = ">III%ds%ds"%(len(name), len(data))
            buff = struct.pack(format_str, bufflen, ret, len(name), name, data)
            slen = self.s.sendto(buff, (self.ip_, self.port_))
            #print ("socket send! ip=%s pt=%d len=%d"%(self.ip_, self.port_, slen))
        except socket.error, errmsg:
            print ("socket send err! ip=%s pt=%d err=%s"%(self.ip_, self.port_, errmsg))
            self.close()
            return False
        return True

    def recv(self, recv_len=2048):
        datalen = 0
        data = ""
        if not self.s:
            return data
        try:
            hstr, addr = self.s.recvfrom(recv_len)
            (datalen, ret, namelen) = struct.unpack(">III", hstr[:12])
            print (datalen, ret, namelen)
            if namelen > 0:
                namestr = hstr[12:12+namelen]
            if datalen > 12 + namelen:
                data = hstr[12+namelen:datalen]
        except socket.error, msg:
            print("socket error=%s guid=%d"%(msg, self.guid_))
        if not data:
            self.close()
            print("socket recv err! ip=%s pt=%d guid=%d"%(self.ip_, self.port_, self.guid_))
        return (ret, namestr, datalen, data)

    def getfd(self):
        if not self.s:
            return -1;
        return self.s.fileno()

    def close(self):
        if self.s:
            self.s.close()
            self.s = None 

