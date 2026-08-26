import wx
import config

class Process(wx.Process):
	def __init__(self, win, argv):
		wx.Process.__init__(self, win)
		self.Redirect()
		self.win = win
		self.cmd = ' '.join(argv)
		self.pid = None
	def run(self):
		print self.cmd
		self.pid = wx.Execute(self.cmd, wx.EXEC_ASYNC, self)
		if not self.pid:
			wx.MessageDialog(self.win,
							 "Failed to execute command \"%s\". " 
							 % self.cmd,
							 "Error", 
							 wx.OK|wx.ICON_ERROR)
	def kill(self):
		if self.pid:
			wx.Process.Kill(self.pid, wx.SIGINT)
	def OnTerminate(self, pid, status):
		if status:
			wx.MessageDialog(self.win, 
							 "Command \"%s\" exited with error code %d. "
							 % (self.cmd, status),
							 "Error",
							 wx.OK|wx.ICON_ERROR).ShowModal()

class Client(Process):
	def __init__(self, win, prog):
		Process.__init__(self, win, ())
		self.prog = prog
	def join(self, host, port, nick=None):
                if config.is_muted:
                        sound = "no"
                else:
                        sound = "yes"
		if nick:
			self.cmd = "%s -name \"%s\" -port %d -join %s -sound %s" % (self.prog, 
								  nick, port,
                                                                  host, sound)
		else:
			self.cmd = "%s -port %d -join %s -sound %s" % (self.prog, port,
                                        host, sound)
		self.run()

