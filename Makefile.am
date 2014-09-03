bin_PROGRAMS =

all-local :

AM_CFLAGS = $(GCC_CFLAGS)

AM_CPPFLAGS = 					\
	-I$(top_srcdir)/toytoolkit		\
	-DDATADIR='"$(datadir)"'

CLEANFILES =

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libwlmessage.pc

lib_LTLIBRARIES = libwlmessage.la

include_HEADERS = libwlmessage.h

libwlmessage_la_SOURCES =

if ENABLE_X11
libwlmessage_la_SOURCES +=				\
	libwlmessage-x11.c
endif

if ENABLE_WAYLAND
libwlmessage_la_SOURCES +=				\
	libwlmessage-wl.c				\
	toytoolkit/shared/frame.c			\
	toytoolkit/shared/image-loader.c		\
	toytoolkit/shared/cairo-util.c			\
	toytoolkit/shared/os-compatibility.c		\
	toytoolkit/xdg-shell-protocol.c			\
	toytoolkit/text-cursor-position-protocol.c	\
	toytoolkit/text-protocol.c			\
	toytoolkit/workspaces-protocol.c		\
	toytoolkit/window.c
endif

libwlmessage_la_LDFLAGS = -export-dynamic
libwlmessage_la_CPPFLAGS = $(AM_CPPFLAGS) -Wno-unused-result
libwlmessage_la_CFLAGS = $(GCC_CFLAGS) $(PNG_CFLAGS) $(PIXMAN_CFLAGS) $(CLIENT_CFLAGS) $(CAIRO_EGL_CFLAGS) $(GLIB_CFLAGS)
libwlmessage_la_LIBADD = $(DLOPEN_LIBS) $(PNG_LIBS) $(PIXMAN_LIBS) $(CLIENT_LIBS) $(CAIRO_EGL_LIBS) $(JPEG_LIBS) $(GLIB_LIBS) -lm

noinst_PROGRAMS = wlmessage_test

wlmessage_test_LDFLAGS = -export-dynamic
wlmessage_test_CPPFLAGS = $(AM_CPPFLAGS) -Wno-unused-result
wlmessage_test_CFLAGS = $(GCC_CFLAGS)
wlmessage_test_LDADD = $(DLOPEN_LIBS) libwlmessage.la

wlmessage_test_SOURCES =					\
	wlmessage_test.c
