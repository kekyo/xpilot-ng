#!/usr/bin/env python

import wxversion
try:
	wxversion.select('2.6')
except wxversion.VersionError:
	try:
		wxversion.select('2.4')
	except wxversion.VersionError:
		wxversion.select('2.5')
import wx
import wx.html as html
import os
import urllib
import metaui
import config
import options
import ircui
import xputil
import serverui

nick = None
def get_nick():
	global nick
	if nick: return nick
	if os.path.exists(config.xpilotrc):
		opts = options.parse_xpilotrc(config.xpilotrc)
		try:
			nick = opts['name']
		except KeyError:
			pass
	if nick: return nick
	try:
		nick = os.environ['USER']
	except KeyError:
		nick = os.environ.get('USERNAME', 'xpilot-user')
	return nick

class RecordingsPanel(html.HtmlWindow):
	def __init__(self, parent):
		html.HtmlWindow.__init__(self, parent)
		self.baseurl = config.record_url
		self.LoadPage(self.baseurl)
	def OnLinkClicked(self, link):
		file = urllib.urlretrieve(self.baseurl + link.GetHref())[0]
		xputil.Process(self, (config.xpreplay, file)).run()

class MenuPanel(wx.Panel):
	def __init__(self, parent, items):
		wx.Panel.__init__(self, parent)
		self.SetBackgroundColour(wx.BLACK)
		self.frame = parent
		p = wx.Panel(self)
		p.SetBackgroundColour(wx.BLACK)
		s = wx.BoxSizer(wx.VERTICAL)
		p.SetSizer(s)
		items.insert(0, ())
		for item in items:
			if len(item):
				b = wx.Button(p, -1, item[0])
				b.SetSize((300, 50))
				b.SetForegroundColour(wx.Colour(255, 255, 255))
				b.SetBackgroundColour(wx.Colour(50, 50, 204))
				b.SetFont(wx.Font(20, wx.DEFAULT, wx.SLANT, wx.NORMAL, 0, ""))
				if item[1]:
					self.Bind(wx.EVT_BUTTON, item[1], b)
				else:
					b.Disable()
				s.Add(b, 0, wx.ALL|wx.EXPAND|wx.ALIGN_CENTER_HORIZONTAL, 10)
			else:
				s.Add((0,30), 0, wx.ALL, 10)
		s = wx.BoxSizer(wx.VERTICAL)
		self.SetSizer(s)
		s.Add(p, wx.EXPAND, wx.ALIGN_CENTER_HORIZONTAL, 0)
		s.Add(self.makeBottomPanel(), 0, wx.EXPAND, 0)	
	def makeBottomPanel(self):
		p = wx.Panel(self)
		p.SetBackgroundColour(wx.BLACK)
		s = wx.FlexGridSizer(1, 3)
		s.Add(wx.StaticBitmap(
			p, -1,
			wx.Bitmap(os.path.join(config.png_path, "swgrid.png"),
			wx.BITMAP_TYPE_ANY)), 0, 0, 0)
		s.Add((0,0), 0, 0, 0)
		s.Add(wx.StaticBitmap(
			p, -1,
			wx.Bitmap(os.path.join(config.png_path, "segrid.png"),
			wx.BITMAP_TYPE_ANY)), 0, 0, 0)
		s.AddGrowableCol(1);
		p.SetSizer(s)
		return p
	def show(self, panel):
		self.frame.setContentPanel(panel)

class  MapEditorMenu(MenuPanel):
	def __init__(self, parent):
		b = []
		if config.javaws:
			b.append(("  Polygon map editor  ", self.onPoly))
		if config.mapedit:
			b.append(("Block map editor", self.onBlock))
		if b:
			MenuPanel.__init__(self, parent, b)
		else:
			# FIXME: raise some error; we shouldn't even create this menu if
			# we have nothing to put in it
			pass
	def onPoly(self, evt):
		xputil.Process(self, (config.javaws, config.jxpmap_url)).run()
	def onBlock(self, evt):
		xputil.Process(self, (config.mapedit,)).run()

class ToolsMenu(MenuPanel):
	def __init__(self, parent):
		b = []
		if config.client:
			b.append(("  Client configuration  ", self.onClientConfig))
		if config.xpreplay:
			b.append(("XP-Replay", self.onXPReplay))
			b.append(("Recordings", self.onRecordings))
		if config.mapedit and config.javaws:
			b.append(("Map editor", self.onMapEditor))
		elif config.mapedit:
			b.append(("Block map editor", self.onBlock))
		elif config.javaws:
			b.append(("Polygon map editor", self.onPoly))
		MenuPanel.__init__(self, parent, b)
	def onMapEditor(self, evt):
		self.show(MapEditorMenu(self.frame))
	def onRecordings(self, evt):
		self.show(RecordingsPanel(self.frame))
	def onXPReplay(self, evt):
		dlg = wx.FileDialog(
            None, message="Choose a recording", defaultDir=os.getcwd(), 
            defaultFile="", wildcard="*.xpr*", style=wx.OPEN | wx.CHANGE_DIR)
		if dlg.ShowModal() == wx.ID_OK:
			xputil.Process(self, (config.xpreplay, dlg.GetPath())).run()
	def onClientConfig(self, evt):
		self.show(options.ClientOptionsPanel(
				self.frame, config.client, config.xpilotrc))
	def onPoly(self, evt):
		xputil.Process(self, (config.javaws, config.jxpmap_url)).run()
	def onBlock(self, evt):
		xputil.Process(self, (config.mapedit,)).run()

class MainMenu(MenuPanel):
	def __init__(self, parent):
		b = []
		b.append(("    Internet servers    ", self.onInternet))
		if config.server:
			b.append(("Start server", self.onStart))
		if config.client or config.xpreplay or config.mapedit or config.javaws:
			b.append(("Tools", self.onTools))
		b.append(("Support and Chat", self.onChat))
# FIXME: This should be a fullscreen widget in the corner instead.
#		b.append(("Windowed", self.onWindowed))
		b.append(("Quit", self.onQuit))
		MenuPanel.__init__(self, parent, b)
	def onInternet(self, evt):
		meta = metaui.Panel(self.frame,
							config.meta,
							xputil.Client(self, config.client))
		self.show(meta)
		meta.RefreshList()
	def onTools(self, evt):
		self.show(ToolsMenu(self.frame))
	def onQuit(self, evt):
		self.frame.Close()
	def onStart(self, evt):
		self.show(options.ServerOptionsPanel(
				self.frame, 
				xputil.Client(self, config.client), 
				config.server,
				config.mapdir))
	def onChat(self, evt):
		self.show(ircui.IrcPanel(self.frame, config.irc_server, get_nick(), 
								 config.irc_channel))
	def onWindowed(self, evt):
		# FIXME: assert self.frame.fullscreen=True
		# FIXME: remove "Windowed" button & replace with "Fullscreen"
		self.frame.ShowFullScreen(False)
		self.frame.fullscreen=False
	def onFullscreen(self, evt):
		# FIXME: assert self.frame.fullscreen=False
		# FIXME: remove "Fullscreen" button & replace with "Windowed"
		self.frame.ShowFullScreen(True)
		self.frame.fullscreen=True

class MainFrame(wx.Frame):
	def __init__(self, *args, **kwds):
		kwds["style"] = wx.DEFAULT_FRAME_STYLE
		wx.Frame.__init__(self, *args, **kwds)
		self.SetSize((800, 650))
		self.SetBackgroundColour(wx.BLACK)
		s = wx.BoxSizer(wx.VERTICAL)
		s.Add(self.makeTopPanel(), 0, wx.EXPAND, 0)
		self.SetAutoLayout(True)
		self.SetSizer(s)
		self.history = []
		self.contentPanel = None
		self.setContentPanel(MainMenu(self))
		self.fullscreen = None
	def setContentPanel(self, p):
		if self.contentPanel:
			self.GetSizer().Detach(self.contentPanel)
			# reusing panel instances causes flickering
			# so I save the class instead of the instance
			self.history.append(self.contentPanel.__class__)
			self.contentPanel.Destroy()
		self.contentPanel = p
		self.GetSizer().Insert(1, p, wx.EXPAND, wx.EXPAND)
		self.backButton.Show(len(self.history) > 0)
		self.Layout()
	def makeTopPanel(self):
		p = wx.Panel(self)
		p.SetBackgroundColour(wx.BLACK)
		s = wx.FlexGridSizer(1, 5)
		s.Add(wx.StaticBitmap(
			p, -1,
			wx.Bitmap(os.path.join(config.png_path, "nwgrid.png"),wx.BITMAP_TYPE_ANY)),
			0, 0, 0)
		s.Add((0,0), 0, 0, 0)
		s.Add(wx.StaticBitmap(
			p, -1,
			wx.Bitmap(os.path.join(config.png_path, "logo.png"),wx.BITMAP_TYPE_ANY)),
			0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 1)
		s.Add((0,0), 0, 0, 0)
		s.Add(wx.StaticBitmap(
			p, -1,
			wx.Bitmap(os.path.join(config.png_path, "negrid.png"), wx.BITMAP_TYPE_ANY)), 
			0, 0, 0)
		s.AddGrowableCol(1);
		s.AddGrowableCol(3);
		p.SetSizer(s)
		b = wx.Button(p, -1, "Back")
		b.SetForegroundColour(wx.Colour(255, 255, 255))
		b.SetBackgroundColour(wx.Colour(50, 50, 204))
		b.SetPosition((10,10))
		b.Bind(wx.EVT_BUTTON, self.onBack, b)
		b.Show(False)
		self.backButton = b
		return p
	def onBack(self, evt):
		if len(self.history) == 0:
			return
		old = self.contentPanel
		self.GetSizer().Detach(old)
		old.Destroy()
		pc = self.history.pop()
		p = pc(self)
		self.contentPanel = p
		self.GetSizer().Insert(1, p, wx.EXPAND, wx.EXPAND)
		self.backButton.Show(len(self.history) > 0)
		self.Layout()

class App(wx.App):
	def OnInit(self):
		frame = MainFrame(None, -1, "XPilot NG Control Center")
		self.SetTopWindow(frame)
		frame.Show(True)
# FIXME: We can only default to fullscreen when we support switching back
#        and forth between fullscreen & windowed.
#		frame.fullscreen=True
#		frame.ShowFullScreen(True)
		return True

def main():
	app = App(0)
	app.MainLoop()

if __name__ == '__main__':
	main()
