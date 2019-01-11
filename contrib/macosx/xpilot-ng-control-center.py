#!/usr/bin/env pythonw

import os

resources_subpath = os.path.join('..', 'Resources')
xpngcc_subpath = os.path.join('xpngcc', 'xpngcc.py')
client_subpath = 'XPilotNG.app/Contents/MacOS/XPilotNG'
server_subpath = 'xpilot-ng-server'

my_path = __file__
resources_path = os.path.abspath(os.path.join(os.path.dirname(my_path), resources_subpath))
xpngcc_path = os.path.abspath(os.path.join(resources_path, xpngcc_subpath))
client_path = os.path.abspath(os.path.join(resources_path, client_subpath))
server_path = os.path.abspath(os.path.join(resources_path, server_subpath))

os.environ['xpilot_ng_sdl'] = client_path
os.environ['xpilot_ng_server'] = server_path

os.execl('/usr/bin/env', 'pythonw', xpngcc_path)