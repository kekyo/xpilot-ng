AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(srcdir)/third_party/wslay/include \
	-I$(srcdir)/third_party/wslay \
	-DWSLAY_VERSION=\"1.1.1\"

if COND_WINDOWS
AM_CPPFLAGS += -DHAVE_WINSOCK2_H
endif

noinst_LIBRARIES = libxpcommon.a
libxpcommon_a_SOURCES = \
	arraylist.c arraylist.h astershape.h audio.h \
	bit.h \
	checknames.c checknames.h click.h commonproto.h config.c const.h \
	error.c error.h \
	game_transport.c game_transport.h \
	item.h \
	keys.h \
	list.c list.h \
	math.c metaserver.h \
	net.c net.h record_drain.c record_drain.h \
	recording_input.c recording_input.h \
	record_session.c record_session.h record_transport.c record_transport.h \
	pack.h packet.h portability.c portability.h \
	randommt.c rules.h \
	secure_random.c secure_random.h \
	session_protocol.c session_protocol.h \
	session_token.c session_token.h \
	websocket_http.c websocket_http.h \
	websocket_transport.c websocket_transport.h \
	setup.h shipshape.c shipshape.h socklib.c socklib.h \
	strcasecmp.c strdup.c strlcpy.c \
	third_party/wslay/wslay_event.c third_party/wslay/wslay_event.h \
	third_party/wslay/wslay_frame.c third_party/wslay/wslay_frame.h \
	third_party/wslay/wslay_net.c third_party/wslay/wslay_net.h \
	third_party/wslay/wslay_queue.c third_party/wslay/wslay_queue.h \
	third_party/wslay/include/wslay/wslay.h \
	types.h win32.h \
	version.h \
	wreckshape.h \
	xpcommon.h xpconfig.h xpmap.h xpmemory.c

EXTRA_DIST = third_party/wslay/LICENSE third_party/wslay/UPSTREAM.md
