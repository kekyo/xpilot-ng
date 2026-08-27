#! /bin/sh

# This script assumes you have the programs you want to make man pages for
# (xpilot-infinity-x11, xpilot-infinity-sdl, xpilot-infinity-server and/or xpilot-infinity-replay)
# in your path.

test -z "$(which help2man)" && echo "help2man not found." && exit 1

OPTS="--no-info"

test ! -z "$(which xpilot-infinity-x11)" && \
echo "Making manpage for xpilot-infinity-x11 ..." && \
help2man $OPTS --name="X11 client for multiplayer space war game." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-infinity-x11 > xpilot-infinity-x11.man

test ! -z "$(which xpilot-infinity-sdl)" && \
echo "Making manpage for xpilot-infinity-sdl ..." && \
help2man $OPTS --name="an SDL/OpenGL XPilot client." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-infinity-sdl > xpilot-infinity-sdl.man

test ! -z "$(which xpilot-infinity-server)" && \
echo "Making manpage for xpilot-infinity-server ..." && \
help2man $OPTS --name="server for multiplayer space war game." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-infinity-server > xpilot-infinity-server.man

test ! -z "$(which xpilot-infinity-replay)" && \
echo "Making manpage for xpilot-infinity-replay ..." && \
help2man $OPTS --name="Playback an XPilot session." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-infinity-replay > xpilot-infinity-replay.man

test ! -z "$(which xpilot-infinity-xp-mapedit)" && \
echo "Making manpage for xpilot-infinity-xp-mapedit ..." && \
help2man $OPTS --name="Edit block based XPilot maps." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-infinity-xp-mapedit > xpilot-infinity-xp-mapedit.man
# change /home/kps/install to /usr/local in man files
# assuming PREFIX was /home/kps/install
for i in *.man; do sed -i 's/\/home\/kps\/install/\/usr\/local/g' "$i"; done
