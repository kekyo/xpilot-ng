#!/usr/bin/env python

import sys
import os
import wx
import wx.lib.mixins.listctrl as listmix
import socket
import StringIO
from string import atoi
import config

def fetch(meta):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		s.connect(meta)
		data = ""
		while True:
			buf = s.recv(1024)
			if not buf:	return parse(data)
			data += buf
	finally:
		s.close()
		
def parse(data):
	lines = StringIO.StringIO(data).readlines()
	servers = []
	for line in lines: servers.append(Server(line.decode('iso-8859-1')))
	return servers

class Server:
	def __init__(self, line):
		self.line = line.strip()
		fields = self.line.split(":")
		self.version = fields[0]
		self.host = fields[1]
		self.port = atoi(fields[2])
		self.count = atoi(fields[3])
		self.mapname = fields[4]
		self.mapsize = fields[5]
		self.author = fields[6]
		self.status = fields[7]
		self.bases = atoi(fields[8])
		self.fps = atoi(fields[9])
		self.playlist = fields[10]
		self.sound = fields[11]
		self.teambases = atoi(fields[13])
		self.uptime = atoi(fields[12])
		self.timing = fields[14]
		self.ip = fields[15]
		self.freebases = fields[16]
		self.queue = atoi(fields[17])
        
	def __str__(self):
		return "Server:" + self.line

class AutoSizeListCtrl(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
	def __init__(self, parent, ID, pos=wx.DefaultPosition,
				 size=wx.DefaultSize, style=wx.LC_REPORT|wx.SUNKEN_BORDER):
		wx.ListCtrl.__init__(self, parent, ID, pos, size, style)
		listmix.ListCtrlAutoWidthMixin.__init__(self)

class Panel(wx.Panel):
	def __init__(self, parent, meta, client):
		wx.Panel.__init__(self, parent, -1)
		self.meta = meta
		self.client = client
		self.serverList = AutoSizeListCtrl(self, -1)
		for label in [ "Server", "Version", "#", "Map" ]:
			self.serverList.InsertColumn(sys.maxint, label)
		self.detailList = AutoSizeListCtrl(self, -1)
		for label in [ "Property", "Value" ]:
			self.detailList.InsertColumn(sys.maxint, label)
		for label in [ "Address", "Port", "Size", "Author", "Status", "Bases",
					   "Teams", "Free bases", "Queue", "FPS", "Sound", "Timing",
					   "Uptime"]:
			self.detailList.InsertStringItem(sys.maxint, label)
		self.detailList.Layout()
		self.playerList = AutoSizeListCtrl(self, -1)
		self.playerList.InsertColumn(0, "Players")
		box0 = wx.BoxSizer(wx.HORIZONTAL)
		# FIXME: add a "mute" checkbox to box0
		if (config.client_sdl and config.client_x11):
			self.joinclient = wx.RadioBox(self, label="Client", 
				choices=["Modern", "Classic"])
                        if config.client == config.client_x11:
                                self.joinclient.SetSelection(1)
                        self.client.prog = config.client
			self.Bind(wx.EVT_RADIOBOX, self.OnJoinClient, self.joinclient)
			box0.Add(self.joinclient, 0, wx.ALL)
                        self.mute = wx.CheckBox(self, label="Mute")
                        self.Bind(wx.EVT_CHECKBOX, self.OnMute, self.mute)
			box0.Add((5,0))
			box0.Add(self.mute, 0, wx.ALL)
		elif config.client_sdl:
			self.client.prog = config.client = config.client_sdl
		else:
			self.client.prog = config.client = config.client_x11
		refresh = wx.Button(self, -1, "Refresh")
		self.Bind(wx.EVT_BUTTON, self.OnRefresh, refresh)
		box1 = wx.BoxSizer(wx.HORIZONTAL)
		if config.client:
			join = wx.Button(self, -1, "Join")
			self.Bind(wx.EVT_BUTTON, self.OnJoin, join)
			box1.Add(join, 0, wx.ALL, 5)
			box1.Add((5,0))
		else:
			box1.Add((100,0))
		box1.Add(refresh, 0, wx.ALL, 5)
		box2 = wx.BoxSizer(wx.VERTICAL)
		box2.Add(self.detailList, 2, wx.EXPAND)
		box2.Add(self.playerList, 1, wx.EXPAND)
		box2.Add((1, 5))
		box2.Add(box0)
		box2.Add((1, 5))
		box2.Add(box1)
		box3 = wx.BoxSizer(wx.HORIZONTAL)
		box3.Add(self.serverList, wx.EXPAND, wx.EXPAND|wx.ALL, 5)
		box3.Add(box2, 0, wx.EXPAND|wx.ALL, 5)
		self.SetSizer(box3)
		self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSelect, self.serverList)
	
	def RefreshList(self):
		list = self.serverList
		list.DeleteAllItems()
		self.servers = fetch(self.meta)
		self.servers.sort(lambda x, y: y.count-x.count)
		row = 0
		for server in self.servers:
			list.InsertStringItem(row, server.host)
			list.SetStringItem(row, 1, server.version)
			list.SetStringItem(row, 2, str(server.count))
			list.SetStringItem(row, 3, server.mapname)
			row = row + 1
		for i in range(4):
			list.SetColumnWidth(i, wx.LIST_AUTOSIZE)
		self.serverList.SetItemState(0, wx.LIST_STATE_SELECTED, 
									 wx.LIST_STATE_SELECTED )
		self.Layout()

	def OnJoinClient(self, evt):
		preferredClientString = self.joinclient.GetStringSelection()
		if preferredClientString == 'Modern':
			self.client.prog = config.client = config.client_sdl
		else:
			self.client.prog = config.client = config.client_x11

	def OnMute(self, evt):
		config.is_muted = self.mute.IsChecked()

	def OnSelect(self, evt):
		s = self.servers[evt.GetIndex()]
		self.selected = s
		items = [ s.ip, str(s.port), s.mapsize, s.author, s.status, 
				  str(s.bases), str(s.teambases), str(s.freebases), 
				  str(s.queue), str(s.fps), s.sound, s.timing, 
				  "%.1f days" % (s.uptime/(3600*24.0)) ]
		for i in range(len(items)):
			self.detailList.SetStringItem(i, 1, items[i])
		self.detailList.SetColumnWidth(1, wx.LIST_AUTOSIZE)
		self.playerList.DeleteAllItems()
		if len(s.playlist) > 0:
			for p in s.playlist.split(","):
				self.playerList.InsertStringItem(sys.maxint, p)
			self.playerList.SetColumnWidth(0, wx.LIST_AUTOSIZE)
	
	def OnJoin(self, evt):
		if self.client: 
			self.client.join(self.selected.ip, self.selected.port)
	
	def OnRefresh(self, evt):
		self.RefreshList()
		
class Frame(wx.Frame):
	    def __init__(
			self, parent, ID, title, pos=wx.DefaultPosition,
            size=(800,600), style=wx.DEFAULT_FRAME_STYLE
			):
			wx.Frame.__init__(self, parent, ID, title, pos, size, style)
			panel = Panel(self, ("meta.xpilot.org", 4401), None)
			panel.RefreshList()
			box = wx.BoxSizer(wx.HORIZONTAL)
			box.Add(panel, wx.EXPAND, wx.EXPAND, 0, 0)
			self.SetSizer(box)

class App(wx.App):
	def OnInit(self):
		frame = Frame(None, -1, "XPilot Meta Interface")
		self.SetTopWindow(frame)
		frame.Show(True)
		return True

def main():
	app = App(0)
	app.MainLoop()

if __name__ == '__main__':
	main()
