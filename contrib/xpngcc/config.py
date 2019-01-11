import os
import sys

def which(app):
	# Does this work on Windows?
	dirnames = os.environ['PATH'].split(os.path.pathsep)
	for dirname in dirnames:
		filename = os.path.join(dirname, app)
		if (os.access(filename, os.X_OK) and
		    os.path.isfile(filename)):
			return filename
	return os.environ.get(app.replace('-', '_'), None)

meta = ('meta.xpilot.org', 4401)
client_x11 = which('xpilot-ng-x11')
client_sdl = which('xpilot-ng-sdl')
if client_sdl:
	client = client_sdl
else:
	client = client_x11
if (sys.platform == 'win32'):
	server = r'"C:\Program Files\XPilotNG-SDL\xpilot-ng-server"'
else:
	server = which('xpilot-ng-server')
if (sys.platform == 'win32'):
	xpilotrc = r'C:\Progra~1\XPilotNG-SDL\xpilotrc.txt'
else:
	xpilotrc = os.path.expanduser('~/.xpilotrc')
record_url = 'http://xpilot.sourceforge.net/ballruns/'
xpreplay = which('xpilot-ng-replay')
jxpmap_url = 'http://xpilot.sf.net/jxpmap/jxpmap.jnlp'
javaws = which('javaws')
mapedit = which('xpilot-ng-xp-mapedit')
irc_server = 'irc.freenode.net'
irc_channel = '#xpilot'
if (sys.platform == 'win32'):
	mapdir = r'C:\Progra~1\XPilotNG-SDL\lib\maps'
else:
	mapdir = '/usr/local/share/xpilot-ng/maps'
png_path = os.path.dirname(os.path.abspath(__file__))
is_muted = False
