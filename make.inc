#
#   @(#)make.inc	7.43 04/03/16
#
#   make.inc for xmcd, cda and libdi
#
#	xmcd  - Motif(R) CD Audio Player/Ripper
#	cda   - Command-line CD Audio Player/Ripper
#	libdi - CD Audio Device Interface Library
#	cddb  - CD Database Management Library
#
#   NOTE: You should use this only if you don't have imake!
#
#   Copyright (C) 1993-2004  Ti Kan
#   E-mail: xmcd@amb.org
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

#
# This sample make.inc file is set up for compilation on a UnixWare 7
# system.  Modify these as you see fit.  You will most certainly need to
# make some changes here in order to successfully build xmcd/cda
# on your platform.
#

#
# For the MISC= line, you should use any of the following if appropriate
# for your platform:
#
#	-DNO_STDLIB_H
#	-DNO_UNISTD_H
#	-DNO_PTHREADS
#	-DNO_LABELH
#	-DHAS_ALSA		(Linux only)
#	-DHAS_MME		(Tru64 UNIX only)
#	-DFREEBSD_CAM		(FreeBSD only)
#	-DSOCKS
#	-DNOREMOTE
#	-DEDITRES
#
# For the ENCODERS= line, you should use any of the following to enable
# the associated functionality.
#
#	-DHAS_LAME
#	-DHAS_VORBIS
#	-DHAS_FLAC
#	-DHAS_FAAC
#
# See the Imakefiles in each of the subdirectories for an explanation of
# these defines.
#
# All other platform-dependent feature configuration is set up in
# common_d/config.h.
#
SHELL=		/bin/sh
MAKE=		make
CC=		cc
RANLIB=		/bin/true
AR=		ar cq
RM=		rm -f
LN=		ln -s
CFLAGS=		$(DEBUGFLAGS) $(DEFINES) $(INCDIRS) $(ARCH) $(ENCODERS) $(MISC)
DEBUGFLAGS=	-O
ARCH=		-DSVR4 -Di386 -Kpthread
MISC=		
ENCODERS=	-DHAS_LAME -DHAS_VORBIS -DHAS_FLAC -DHAS_FAAC
INCDIRS=	-I.. -I/usr/X/include -I/usr/local/include
CDDBLIB=	-L../cddb_d -lcddb
LOCAL_LIBDIR=	-L/usr/local/lib
VORBLIB=	-lvorbisenc -lvorbis -logg
FLACLIB=	-lFLAC
ENCLIBS=	$(LOCAL_LIBDIR) $(FLACLIB) $(VORBLIB)
# AUDIOLIB=	-lasound
XLIBS=		-L/usr/X/lib -lXm -lXt -lX11 -lgen
CURSESLIB=	-lcurses
SOCKETLIB=	-lsocket -lnsl
MATHLIB=	-lm
PTHREADLIB=	
BINDIR=		/usr/X/bin
LIBDIR=		/usr/X/lib
MANDIR=		/usr/share/man/man1
MANSUF=		.1
DEPEND=		makedepend
DEPENDFLAGS=
TAGS=		ctags

