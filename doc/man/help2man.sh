#! /bin/sh

# This script assumes you have the programs you want to make man pages for
# (xpilot-ng-x11, xpilot-ng-sdl, xpilot-ng-server and/or xpilot-ng-replay)
# in your path.

test -z "$(which help2man)" && echo "help2man not found." && exit 1

OPTS="--no-info"

test ! -z "$(which xpilot-ng-x11)" && \
echo "Making manpage for xpilot-ng-x11 ..." && \
help2man $OPTS --name="X11 client for multiplayer space war game." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-ng-x11 > xpilot-ng-x11.man

test ! -z "$(which xpilot-ng-sdl)" && \
echo "Making manpage for xpilot-ng-sdl ..." && \
help2man $OPTS --name="an SDL/OpenGL XPilot client." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-ng-sdl > xpilot-ng-sdl.man

test ! -z "$(which xpilot-ng-server)" && \
echo "Making manpage for xpilot-ng-server ..." && \
help2man $OPTS --name="server for multiplayer space war game." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-ng-server > xpilot-ng-server.man

test ! -z "$(which xpilot-ng-replay)" && \
echo "Making manpage for xpilot-ng-replay ..." && \
help2man $OPTS --name="Playback an XPilot session." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-ng-replay > xpilot-ng-replay.man

test ! -z "$(which xpilot-ng-xp-mapedit)" && \
echo "Making manpage for xpilot-ng-xp-mapedit ..." && \
help2man $OPTS --name="Edit block based XPilot maps." --section=6 --manual=Games --source=xpilot.sourceforge.net xpilot-ng-xp-mapedit > xpilot-ng-xp-mapedit.man
# change /home/kps/install to /usr/local in man files
# assuming PREFIX was /home/kps/install
for i in *.man; do sed -i 's/\/home\/kps\/install/\/usr\/local/g' "$i"; done
