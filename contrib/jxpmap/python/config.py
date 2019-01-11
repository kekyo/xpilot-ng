
# Editor will search for images (both the ones it uses by default such
# as checkpoint bitmaps and those named in map files) from these
# paths. One path should probably point to ../images/ (as stored in
# CVS) from this directory, to find the basic images (checkpoint.gif
# etc). Other paths are needed only if you edit maps with custom textures.

PIXMAP_PATH = ["/home/uau/reiser/xpilot/contrib/jxpmap/images/",
               "/home/uau/games/lib/xpilot/textures/rtc/"]

# Currently undo is done in a simple way, storing a copy of the whole
# map each time certain operations are done (supposedly those you
# might wish to undo). This takes memory and due to the inefficient
# copying algorithm can also take a visible amount of time (depending
# on map complexity and computer speed).
# This limits the number of copies done. Setting it lower means less memory
# used (but doesn't affect the time taken); setting to 0 means undo is
# disabled (and thus no speed penalty either).

UNDO_LEVELS = 100
