bin_PROGRAMS = xpilot-infinity-x11
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(top_builddir)/src/common -I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/client \
	@SDL3_CFLAGS@ @SDL3_TTF_CFLAGS@ @NATIVE_TEXT_CFLAGS@ \
	@XRENDER_CFLAGS@
xpilot_infinity_x11_SOURCES = \
	about.c \
	bitmaps.c bitmaps.h \
	colors.c configure.c \
	dbuff.c dbuff.h \
	guimap.c guiobjects.c \
	icon.h \
	join.c \
	paintdata.c paintdata.h painthud.c paintradar.c \
	record.c record.h \
	talk.c \
	welcome.c widget.c widget.h \
	xdefault.c xevent.c xevent.h xeventhandlers.c xeventhandlers.h \
	x11_text.c x11_text.h \
	../xhacks.h xinit.c xinit.h xpaint.c xpaint.h xpclient_x11.h xpilot.c

#xpilot_CFLAGS = @X_CFLAGS@
xpilot_infinity_x11_LDADD = $(top_builddir)/src/client/libxpclient.a $(top_builddir)/src/common/libxpcommon.a @X_LIBS@ @X_PRE_LIBS@ @X_EXTENSIONS_LIB@ @X_EXTRA_LIBS@ -lX11 @XRENDER_LIBS@ @SDL3_TTF_LIBS@ @SDL3_LIBS@ @NATIVE_TEXT_LIBS@ @SOUND_LIBS@
#EXTRA_xpilot_SOURCES = debugaudio.c
