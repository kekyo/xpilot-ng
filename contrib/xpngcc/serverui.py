#!/usr/bin/env python

import wx
import string
import xputil
import time

class Console(wx.Frame):
	def __init__(self, parent, client):
		wx.Frame.__init__(self, parent, -1, "XPilot NG Server",
						  pos=wx.DefaultPosition,
						  size=(600,300), style=wx.DEFAULT_FRAME_STYLE)
		self.client = client
		bsz = wx.BoxSizer(wx.HORIZONTAL)
		b = wx.Button(self, -1, "Stop")
		b.Disable()
		self.stop = b
		self.Bind(wx.EVT_BUTTON, self.on_close, b)
		bsz.Add(b, 0, wx.ALL, 5)
                if self.client.prog:
		        b = wx.Button(self, -1, "Join")
		        self.Bind(wx.EVT_BUTTON, self.on_join, b)
		        bsz.Add(b, 0, wx.ALL, 5)
		        b.Disable()
		        self.join = b
		sz = wx.BoxSizer(wx.VERTICAL)
		sz.Add(bsz, 0, 0, 0)
		self.area = wx.TextCtrl(self, -1, style=wx.TE_READONLY|wx.TE_MULTILINE)
		sz.Add(self.area, wx.EXPAND, wx.EXPAND|wx.ALL, 5)
		self.SetSizer(sz)
		self.SetAutoLayout(True)
		self.Bind(wx.EVT_CLOSE, self.on_close)
		self.server = None
		self.Bind(wx.EVT_IDLE, self.on_idle)
	def on_close(self, evt):
		self.server.win = None
		if self.server: self.server.kill()
		self.Destroy()
	def on_join(self, evt):
		if self.server: self.client.join("localhost", self.server.port)
	def on_idle(self, evt):
		if self.server:
			stream = self.server.GetInputStream()
			if stream.CanRead():
				self.area.AppendText(stream.readline().decode("iso-8859-1"))
	def attach_server(self, server):
		self.server = server
                if self.client.prog:
		        self.join.Enable(True)
		self.stop.Enable(True)


class Server(xputil.Process):
	def __init__(self, win, argv):
		xputil.Process.__init__(self, win, argv)
		self.port = 15345
		for i in range(len(argv)):
			if argv[i] == '-port':
				self.port = string.atoi(argv[i+1])
				break
	def OnTerminate(self, pid, status):
		if self.win: self.win.Close()
		xputil.Process.OnTerminate(self, pid, status)
		

