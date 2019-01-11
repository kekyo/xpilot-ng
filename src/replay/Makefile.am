
bin_PROGRAMS = xpilot-ng-replay
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\"
INCLUDES = -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client
xpilot_ng_replay_SOURCES = buttons.c buttons.h xp-replay.c xp-replay.h
xpilot_ng_replay_LDADD = $(top_builddir)/src/common/libxpcommon.a @X_LIBS@ @X_PRE_LIBS@ @X_EXTRA_LIBS@ -lX11
SUBDIRS = tools

