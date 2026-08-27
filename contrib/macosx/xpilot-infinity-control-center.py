#!/usr/bin/env pythonw

import os

resources_subpath = os.path.join('..', 'Resources')
xpicc_subpath = os.path.join('xpicc', 'xpicc.py')
client_subpath = 'XPilotInfinity.app/Contents/MacOS/XPilotInfinity'
server_subpath = 'xpilot-infinity-server'

my_path = __file__
resources_path = os.path.abspath(os.path.join(os.path.dirname(my_path), resources_subpath))
xpicc_path = os.path.abspath(os.path.join(resources_path, xpicc_subpath))
client_path = os.path.abspath(os.path.join(resources_path, client_subpath))
server_path = os.path.abspath(os.path.join(resources_path, server_subpath))

os.environ['xpilot_infinity_sdl'] = client_path
os.environ['xpilot_infinity_server'] = server_path

os.execl('/usr/bin/env', 'pythonw', xpicc_path)