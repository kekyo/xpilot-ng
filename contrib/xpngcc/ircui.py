import wx
import irclib
import threading
import wx.lib.newevent
import time

(AppendMsgEvent, EVT_APPEND_MSG) = wx.lib.newevent.NewEvent()

class IrcPanel(wx.Panel):
	def __init__(self, parent, server, nick, channel):
		wx.Panel.__init__(self, parent, -1)
		p = wx.Panel(self)
		ps = wx.BoxSizer(wx.HORIZONTAL)
		p.SetSizer(ps)
		l = wx.StaticText(p, -1, "Message:")
		l.SetForegroundColour(wx.Color(255,255,255))
		ps.Add(l, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		self.message = wx.TextCtrl(p, -1, '', style=wx.TE_PROCESS_ENTER)
		self.Bind(wx.EVT_TEXT_ENTER, self.on_say, self.message)
		ps.Add(self.message, wx.EXPAND, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		b = wx.Button(p, -1, "Say")
		self.Bind(wx.EVT_BUTTON, self.on_say, b)
		ps.Add(b, 0, wx.ALL, 5)
		sz = wx.BoxSizer(wx.VERTICAL)
		self.SetSizer(sz)
		self.area = wx.TextCtrl(self, -1, style=wx.TE_READONLY|wx.TE_MULTILINE)
		sz.Add(self.area, wx.EXPAND, wx.EXPAND|wx.ALL, 5)
		sz.Add(p, 0, wx.EXPAND|wx.ALL, 5)
		self.Bind(EVT_APPEND_MSG, self.on_append)
		self.worker = WorkerThread(self, server, nick, channel)
		self.worker.start()
	def on_say(self, evt):
		self.worker.send_message(self.message.GetValue())
		self.message.SetValue('')
	def on_append(self, evt):
		self.area.AppendText("[%s] %s\n" 
							 % (time.strftime("%H:%M:%S"), evt.message))
	def append(self, str):
		wx.PostEvent(self, AppendMsgEvent(message = str))
	def Destroy(self):
		self.worker.kill()
		wx.Panel.Destroy(self)

class WorkerThread(threading.Thread):
	def __init__(self, panel, server, nick, channel):
		threading.Thread.__init__(self)
		self.ui = panel
		self.server = server
		self.nick = nick
		self.realnick = nick
		self.channel = channel
		self.index = 0
		self.namreply = ''
		self.die = False
	def kill(self):
		self.die = True
		self.connection.quit()
	def run(self):
		irc = irclib.IRC()
		for m in filter(lambda x: x.startswith('on_'), dir(self)):
			irc.add_global_handler(m[3:], getattr(self, m))
		c = irc.server()
		self.connection = c
		c.connect(self.server, 6667, self.nick)
		while not self.die:
			irc.process_once(0.2)
		c.disconnect()
	def send_message(self, str):
		self.connection.privmsg(self.channel, str)
		msg = "%s: %s" % (self.realnick, str)
		self.ui.append(msg)
	def on_welcome(self, c, evt):
		self.ui.append("Welcome to channel %s on %s. Here you can ask questions about xpilot or just chat with the friendly xpilot people." % (self.channel, self.server))
		c.join(self.channel)
	def on_nicknameinuse(self, c, evt):
		self.index += 1
		self.realnick = "%s_%d" % (self.nick, self.index)
		c.nick(self.realnick)
	def on_namreply(self, c, evt):
		self.namreply += evt.arguments()[2]
	def on_endofnames(self, c, evt):
		msg = "Currently online: %s" % self.namreply
		self.namreply = ''
		self.ui.append(msg)
	def on_privmsg(self, c, evt):
		msg = "%s->%s: %s" % (irclib.nm_to_n(evt.source()), 
							  evt.target(), evt.arguments()[0])
		self.ui.append(msg)
	def on_pubmsg(self, c, evt):
		msg = "%s: %s" % (irclib.nm_to_n(evt.source()), evt.arguments()[0])
		self.ui.append(msg)
	def on_join(self, c, evt):
		msg = "%s has joined %s" % (irclib.nm_to_n(evt.source()), evt.target())
		self.ui.append(msg)
	def on_part(self, c, evt):
		msg = "%s has left %s" % (irclib.nm_to_n(evt.source()), evt.target())
		self.ui.append(msg)
	def on_quit(self, c, evt):
		msg = "%s has quit" % irclib.nm_to_n(evt.source())
		self.ui.append(msg)

class Frame(wx.Frame):
    def __init__(
		self, parent, ID, title, pos=wx.DefaultPosition,
		size=(800,600), style=wx.DEFAULT_FRAME_STYLE):
		wx.Frame.__init__(self, parent, ID, title, pos, size, style)
		box = wx.BoxSizer(wx.VERTICAL)
		box.Add(IrcPanel(self, "irc.freenode.net", "foobar", "#xpilottest"),
				wx.EXPAND, wx.EXPAND, 0, 0)
		self.SetSizer(box)

class App(wx.App):
	def OnInit(self):
		frame = Frame(None, -1, "")
		self.SetTopWindow(frame)
		frame.Show(True)
		return True

if __name__ == '__main__':
	app = App(0)
	app.MainLoop()
