#! /bin/sh

# This script assumes xpilot-ng-sdl and/or xpilot-ng-server are in PATH.

if ! command -v help2man >/dev/null 2>&1; then
    echo "help2man not found." >&2
    exit 1
fi

OPTS="--no-info"

command -v xpilot-ng-sdl >/dev/null 2>&1 && \
echo "Making manpage for xpilot-ng-sdl ..." && \
help2man $OPTS --name="an SDL/OpenGL XPilot client." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-ng-sdl > xpilot-ng-sdl.man

command -v xpilot-ng-server >/dev/null 2>&1 && \
echo "Making manpage for xpilot-ng-server ..." && \
help2man $OPTS --name="server for multiplayer space war game." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-ng-server > xpilot-ng-server.man
# change /home/kps/install to /usr/local in man files
# assuming PREFIX was /home/kps/install
for i in *.man; do sed -i 's/\/home\/kps\/install/\/usr\/local/g' "$i"; done
