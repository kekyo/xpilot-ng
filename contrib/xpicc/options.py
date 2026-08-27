import re
import os
import wx
import wx.lib.mixins.listctrl as listmix
import serverui

class OptionListCtrl(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
	def __init__(self, parent, ID, pos=wx.DefaultPosition,
				 size=wx.DefaultSize, style=wx.LC_REPORT|wx.SUNKEN_BORDER):
		wx.ListCtrl.__init__(self, parent, ID, pos, size, style)
		listmix.ListCtrlAutoWidthMixin.__init__(self)
		self.InsertColumn(0, 'Name')
		self.InsertColumn(1, 'Value')
		self.InsertColumn(2, 'Description')
	def set_value(self, row, newval):
		if newval: self.SetStringItem(row, 1, newval)
		else: self.SetStringItem(row, 1, '<default>')
	def set_options(self, opts):
		self.DeleteAllItems()
		row = 0
		for opt in opts:
			self.InsertStringItem(row, opt.names[0])
			if opt.value: self.SetStringItem(row, 1, opt.value)
			else: self.SetStringItem(row, 1, '<default>')
			self.SetStringItem(row, 2, opt.desc)
			row += 1
		if row:
			self.SetColumnWidth(0, wx.LIST_AUTOSIZE)
			self.resizeLastColumn(100)

class OptionViewPanel(wx.Panel):
	def __init__(self, parent):
		wx.Panel.__init__(self, parent, -1)
		self.option = None
		sz = wx.BoxSizer(wx.HORIZONTAL)
		self.SetSizer(sz)
		p = wx.Panel(self)
		ps = wx.FlexGridSizer(2,2)
		ps.AddGrowableCol(1)
		p.SetSizer(ps)
		l = wx.StaticText(p, -1, "Name:")
		l.SetForegroundColour(wx.Color(255,255,255))
		ps.Add(l, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		self.name = wx.TextCtrl(p, style=wx.TE_READONLY)
		ps.Add(self.name, 0, 
			   wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		l = wx.StaticText(p, -1, "Value:")
		l.SetForegroundColour(wx.Color(255,255,255))
		ps.Add(l, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		self.value = wx.TextCtrl(p)
		ps.Add(self.value, 0, 
			   wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		sz.Add(p, wx.EXPAND, wx.EXPAND, 0)
		self.desc = wx.TextCtrl(self, -1, 
								style=wx.TE_READONLY|wx.TE_MULTILINE)
		sz.Add(self.desc, wx.EXPAND, wx.EXPAND|wx.ALL, 5)
		p = wx.Panel(self)
		ps = wx.BoxSizer(wx.VERTICAL)
		p.SetSizer(ps)
		b = wx.Button(p, -1, "Apply")
		self.Bind(wx.EVT_BUTTON, self.on_apply, b)
		self.apply = b
		ps.Add(b, 0, wx.BOTTOM, 2)
		b = wx.Button(p, -1, "Default")
		self.Bind(wx.EVT_BUTTON, self.on_default, b)
		self.default = b
		ps.Add(b, 0, wx.TOP, 2)
		sz.Add(p, 0, wx.ALL, 5)
		self.clear()
	def clear(self):
		self.option = None
		self.name.SetValue('')
		self.value.SetValue('')
		self.desc.SetValue('')
		self.apply.Disable()
		self.default.Disable()
	def show_option(self, opt):
		self.apply.Enable(True)
		self.default.Enable(True)
		self.option = opt
		self.name.SetValue(str(opt.names)[1:-1])
		if opt.value: self.value.SetValue(opt.value)
		else: self.value.SetValue('')
		self.desc.SetValue(opt.desc)
	def on_apply(self, evt):
		self.option.value = self.value.GetValue()
		self.GetParent().apply()
	def on_default(self, evt):
		self.option.value = None
		self.value.SetValue('')
		self.GetParent().apply()

class FilterPanel(wx.Panel):
	def __init__(self, parent, action):
		wx.Panel.__init__(self, parent, -1)
		self.action = action
		sz = wx.BoxSizer(wx.HORIZONTAL)
		self.SetSizer(sz)
		l = wx.StaticText(self, -1, "Filter:")
		l.SetForegroundColour(wx.Color(255,255,255))
		sz.Add(l, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		self.text = wx.TextCtrl(self, -1, '')
		sz.Add(self.text, wx.EXPAND, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		b = wx.Button(self, -1, "Filter")
		self.Bind(wx.EVT_BUTTON, self.on_filter, b)
		sz.Add(b, 0, wx.ALL, 5)
		b = wx.Button(self, -1, "Show All")
		self.Bind(wx.EVT_BUTTON, self.on_show_all, b)
		sz.Add(b, 0, wx.ALL, 5)
		b = wx.Button(self, -1, action[0])
		self.Bind(wx.EVT_BUTTON, self.on_action, b)
		sz.Add(b, 0, wx.ALL, 5)
	def on_filter(self, evt):
		self.GetParent().filter(self.text.GetValue())
	def on_show_all(self, evt):
		self.text.SetValue('')
		self.GetParent().filter(None)
	def on_action(self, evt):
		self.action[1]()
	def set_filter(self, str):
		if str:	self.text.SetValue(str)

class OptionsPanel(wx.Panel):
	def __init__(self, parent, options, action):
		wx.Panel.__init__(self, parent, -1)
		self.options = options
		self.list = OptionListCtrl(self, -1)
		self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.on_select, self.list)
		sz = wx.BoxSizer(wx.VERTICAL)
		self.filter_panel = FilterPanel(self, action)
		sz.Add(self.filter_panel, 0, wx.EXPAND, 0)
		sz.Add(self.list, wx.EXPAND, wx.EXPAND|wx.ALL, 5)
		self.view = OptionViewPanel(self)
		sz.Add(self.view, 0, wx.EXPAND, 0)
		self.SetSizer(sz)
	def apply(self):
		self.list.set_value(self.row, self.active.value)
	def filter(self, str):
		self.filter_panel.set_filter(str)
		self.view.clear()
		if str:
			self.active_options = [o for o in self.options if o.matches(str) ]
		else: self.active_options = self.options
		self.list.set_options(self.active_options)
	def on_select(self, evt):
		self.row = evt.GetIndex()
		self.active = self.active_options[self.row]
		self.view.show_option(self.active)

class ClientOptionsPanel(OptionsPanel):
	def __init__(self, parent, client, xpilotrc):
		self.xpilotrc = xpilotrc
		opts = parse_options(self, [client, '-help'])
		if os.path.exists(xpilotrc):
			vals = parse_xpilotrc(xpilotrc)
			join_options_with_values(opts, vals)
		sort_options(opts)
		OptionsPanel.__init__(self, parent, opts, ('Save', self.save))
		self.filter(None)
	def save(self):
		if wx.MessageDialog(self, 
							"Save current settings to %s?" % self.xpilotrc,
							"Are you sure?").ShowModal() == wx.ID_OK:
			f = file(self.xpilotrc,'w')
			for o in self.options:
				if o.value:
					o.write(f)
			f.close()

class ServerOptionsPanel(wx.Notebook):
	def __init__(self, parent, client, server, mapdir):
		wx.Notebook.__init__(self, parent, -1, style=wx.BOTTOM|wx.RIGHT)
		self.SetBackgroundColour(wx.Color(0,0,128))
		self.SetForegroundColour(wx.Color(255,255,255))
		self.AddPage(BasicServerOptionsPanel(self, client, server, mapdir),
					 "Basic")
		self.AddPage(AdvancedServerOptionsPanel(self, client, server),
					 "Advanced")
					 
class BasicServerOptionsPanel(wx.Panel):
	def __init__(self, parent, client, server, mapdir):
		wx.Panel.__init__(self, parent, -1)
		self.client = client
		self.server = server
		self.mapdir = mapdir
		sz1 = wx.BoxSizer(wx.HORIZONTAL)
		l = wx.StaticText(self, -1, "Select a map:")
		l.SetForegroundColour(wx.Color(255,255,255))
		sz1.Add((0,0), wx.EXPAND, 0, 0)
		sz1.Add(l, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		maps = filter(lambda x: x[-4:-1] == '.xp' or x[-3:] == '.xp', 
					  os.listdir(mapdir))
		if maps:
			self.selected = maps[0]
			self.choice = wx.Choice(self, -1, choices=maps)
			self.Bind(wx.EVT_CHOICE, self.on_select, self.choice)
			sz1.Add(self.choice, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
			b = wx.Button(self, -1, "Start")
			self.Bind(wx.EVT_BUTTON, self.start, b)
			sz1.Add(b, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
			sz1.Add((0,0), wx.EXPAND, 0, 0)
			self.SetSizer(sz1)
	def on_select(self, evt):
		self.selected = evt.GetString()
	def start(self, evt):
		opts = [self.server, '-map', os.path.join(self.mapdir, self.selected)]
		console = serverui.Console(self, self.client)
		server = serverui.Server(console, opts)
		console.attach_server(server)
		console.Show()
		server.run()		
		
class AdvancedServerOptionsPanel(OptionsPanel):
	def __init__(self, parent, client, server):
		self.client = client
		self.server = server
		opts = parse_options(self, [server, '-help'])
		sort_options(opts)
		OptionsPanel.__init__(self, parent, opts, ('Start', self.start))
		self.filter('[ Flags: command,')
	def start(self):
		opts = [self.server]
		for o in self.options:
			if o.value:
				if o.type == '<bool>':
					if o.value == 'true':
						opts.append('-' + o.names[0])
					else:
						opts.append('+' + o.names[0])
				else:
					opts.append('-' + o.names[0])
					opts.append(o.value)
		console = serverui.Console(self, self.client)
		server = serverui.Server(console, opts)
		console.attach_server(server)
		console.Show()
		server.run()

class Option:
	def __init__(self, names, type, desc, value):
		self.names = names
		self.type = type
		self.desc = desc
		self.value = value
	def __str__(self):
		return str((self.names, self.type, self.desc, self.value))
	def matches(self, query):
		for name in self.names:
			if name.find(query) != -1: return True
		if self.desc.find(query) != -1: return True
		return False
	def write(self, f):
		f.write('xpilot.' + self.names[0] + ': ' + self.value + '\n')

def parse_options(win, cmd):
	opts = []
	r = re.compile('    -(/\+)?([^<]*)(<\w*>)?')
	p = os.popen(' '.join(cmd))
	try:
		for line in p:
			m = r.match(line)
			if not m: continue
			if m.group(1): type = '<bool>'
			elif m.group(3): type = m.group(3)
			else: continue
			names = [x for x in m.group(2).split() if x != 'or']
			desc = ''
			while 1:
				line = p.next().strip()
				if not line: break
				desc += ' ' + line
			opts.append(Option(names, type, desc.lstrip(), None))
	finally:
		p.close()
	return opts

def parse_xpilotrc(fn):
	vals = {}
	r = re.compile('xpilot\.(\w*)[\s:=]*(.*)')
	f = file(fn)
	try:
		for line in f:
			line = line.strip()
			m = r.match(line)
			if not m: continue
			vals[m.group(1)] = m.group(2)
		return vals
	finally:
		f.close()

def join_options_with_values(opts, vals):
	for opt in opts:
		for name in opt.names:
			if name not in vals:
				continue
			opt.value = vals[name]
			break

def sort_options(opts):
	opts.sort(lambda o1,o2: cmp(o1.names[0], o2.names[0]))
