## Process this file with automake to generate Makefile.in
# 
#   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


AUTOMAKE_OPTIONS = 

VPATH += $(srcdir)/flash

# if the compiler has broken float support when optimized
if BROKEN_FLOAT
AM_CXXFLAGS = -ffloat-store
endif

# The built sources get filled in by each subdirectory's .am
# Makefile fragment. These are the geerated headers that contain
# the array of function pointers to each _init() function in each
# class. The header file is generated at build time.
BUILT_SOURCES =

pkglib_LTLIBRARIES = libgnashasobjs.la 

AM_CPPFLAGS = \
	-I$(top_srcdir)/librender \
	-I$(top_srcdir)/libcore \
	-I$(top_srcdir)/libcore/swf \
	-I$(top_srcdir)/libcore/abc \
	-I$(top_srcdir)/libcore/parser \
	-I$(top_srcdir)/libcore/vm \
	-I$(top_srcdir)/libcore/asobj/flash \
	-I$(top_srcdir)/libbase \
	-I$(top_srcdir)/libltdl \
	-I$(top_srcdir)/libmedia \
	-I$(top_srcdir)/libsound \
	$(PTHREAD_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(BOOST_CFLAGS) \
	$(FREETYPE2_CFLAGS)

libgnashasobjs_la_SOURCES = \
	AsBroadcaster.cpp \
	Array_as.cpp \
	Boolean_as.cpp \
	Camera.cpp \
	Color_as.cpp \
	CustomActions.cpp\
	Date_as.cpp \
	Error_as.cpp \
	Globals.cpp \
	int_as.cpp \
	LoadVars_as.cpp \
	Math_as.cpp \
	QName_as.cpp \
	Microphone.cpp	\
	NetConnection_as.cpp \
	NetStream_as.cpp \
	Number_as.cpp \
	PlayHead.cpp \
	Selection_as.cpp \
	Namespace_as.cpp \
	TextSnapshot_as.cpp \
	MovieClipLoader.cpp\
	String_as.cpp \
	LoadableObject.cpp \
	Object.cpp

noinst_HEADERS = \
	AsBroadcaster.h \
	Boolean_as.h \
	Camera.h \
	Color_as.h	\
	CustomActions.h	\
	Date_as.h \
	Error_as.h	\
	Math_as.h	\
	Globals.h\
	Global_as.h\
	int_as.h \
	LoadVars_as.h \
	Microphone.h \
	MovieClipLoader.h \
	NetConnection_as.h	\
	NetStream_as.h \
	Number_as.h \
	PlayHead.h \
	Selection_as.h \
	TextSnapshot_as.h \
	String_as.h \
	LoadableObject.h \
	Namespace_as.h \
	QName_as.h \
	Object.h

EXTENSIONS_API = \
	Array_as.h \
	Key_as.h \
	$(NULL)

CLEANFILES = *classes.h

if SDKINSTALL
noinst_HEADERS += $(EXTENSIONS_API)
else
instdir = $(includedir)/gnash
inst_HEADERS = $(EXTENSIONS_API)
endif

libgnashasobjs_la_LIBADD = \
	$(top_builddir)/libmedia/libgnashmedia.la \
	$(top_builddir)/libbase/libgnashbase.la

if USE_FFMPEG_ENGINE
AM_CPPFLAGS += $(FFMPEG_CFLAGS) \
		$(SDL_CFLAGS) \
		-I$(top_srcdir)/libmedia/ffmpeg
libgnashasobjs_la_LIBADD += $(FFMPEG_LIBS) \
				$(SDL_LIBS)
endif

if USE_GST_ENGINE
AM_CPPFLAGS += $(GSTREAMER_CFLAGS) \
		-I$(top_srcdir)/libmedia/gst
libgnashasobjs_la_LIBADD += $(GSTREAMER_LIBS) \
				$(SDL_LIBS)
endif

if ENABLE_PCH
AM_CXXFLAGS = $(PCH_FLAGS)
endif

# These makefile fragments build the ActionScript library for
# Gnash, and supports both AS2 as well as AS3, so we always
# want to build.
include $(srcdir)/flash/accessibility/accessibility.am
include $(srcdir)/flash/desktop/desktop.am
include $(srcdir)/flash/display/display.am
include $(srcdir)/flash/errors/errors.am
include $(srcdir)/flash/events/events.am
include $(srcdir)/flash/external/external.am
include $(srcdir)/flash/filters/filters.am
include $(srcdir)/flash/geom/geom.am
include $(srcdir)/flash/media/media.am
include $(srcdir)/flash/net/net.am
include $(srcdir)/flash/printing/printing.am
include $(srcdir)/flash/sampler/sampler.am
include $(srcdir)/flash/system/system.am
include $(srcdir)/flash/text/text.am
include $(srcdir)/flash/utils/utils.am
include $(srcdir)/flash/ui/ui.am
include $(srcdir)/flash/xml/xml.am

libgnashasobjs_la_SOURCES += \
	flash/flash_pkg.cpp \
	$(NULL)

noinst_HEADERS += \
	flash/flash_pkg.h \
	flash/flashclasses.h \
	$(NULL)

#libgnashasobjs_la_LDFLAGS = -release $(VERSION) -no-undefined -export-dynamic

if WIN32
  libgnashasobjs_la_LDFLAGS = -no-undefined
endif

# Rebuild with GCC 4.x Mudflap support
mudflap:
	@echo "Rebuilding with GCC Mudflap support"
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -fmudflap" LDFLAGS="$(LDFLAGS) -lmudflap"

clean-hook:
	-rm -f core.*

