AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common
bin_PROGRAMS = xpilot-ng-server
xpilot_ng_server_SOURCES = \
	alliance.c asteroid.c asteroid.h \
	cannon.c cannon.h cell.c cmdline.c collision.c \
	command.c connection.h contact.c \
	defaults.h \
	event.c \
	fileparser.c frame.c gravity.c \
	id.c item.c \
	laser.c \
	map.c map.h metaserver.c modifiers.c modifiers.h \
	netserver.c netserver.h \
	object.c object.h objpos.c objpos.h option.c option.h \
	parser.c player.c player.h polygon.c \
	race.c rank.c rank.h recwrap.c recwrap.h \
	robot.c robot.h robotdef.c rules.c \
	saudio.c saudio.h sched.c sched.h score.c score.h \
	server.c server.h serverconst.h ship.c shot.c \
	showtime.c srecord.c srecord.h suibotdef.c \
	tag.c target.c target.h teamcup.h teamcup.c tuner.c tuner.h \
	treasure.c update.c \
	walls.c walls.h wormhole.c wormhole.h \
	xp2map.c xpmap.c xpserver.h

xpilot_ng_server_LDADD =  $(top_builddir)/src/common/libxpcommon.a @X_EXTRA_LIBS@ @W32_LIBS@
