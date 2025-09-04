#
#   @(#)descrip.mms	1.4 04/04/20
#
#   OpenVMS MMS "Makefile" for cdinfo
#
#	cdinfo  - CD Information Management Library
#
#   Contributing author: Hartmut Becker
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
cc= cc
cflags= /stand=rel/extern=strict/include=("../")/name=shortened
objects= CDINFO_I,CDINFO_X,HIST

all : libcdinfo($(objects))
	! done

libcdinfo(CDINFO_I) : CDINFO_I.obj
libcdinfo(CDINFO_X) : CDINFO_X.obj
libcdinfo(HIST) : HIST.obj
libcdinfo(MOTD) : MOTD.obj

CDINFO_I.OBJ :	CDINFO_I.C
CDINFO_I.OBJ :	[-.COMMON_D]APPENV.H
CDINFO_I.OBJ :	[-.COMMON_D]VERSION.H
CDINFO_I.OBJ :	[-.COMMON_D]UTIL.H
CDINFO_I.OBJ :	[-.LIBDI_D]LIBDI.H
CDINFO_I.OBJ :	[-.CDDA_D]CDDA.H
CDINFO_I.OBJ :	CDINFO.H
CDINFO_I.OBJ :	[-.CDDB_D]CDDB2API.H
CDINFO_I.OBJ :	   [-.CDDB_D]CDDBDEFS.H
CDINFO_X.OBJ :	CDINFO_X.C
CDINFO_X.OBJ :	[-.COMMON_D]APPENV.H
CDINFO_X.OBJ :	[-.COMMON_D]VERSION.H
CDINFO_X.OBJ :	[-.COMMON_D]UTIL.H
CDINFO_X.OBJ :	[-.LIBDI_D]LIBDI.H
CDINFO_X.OBJ :	CDINFO.H
HIST.OBJ :	HIST.C
HIST.OBJ :	[-.COMMON_D]APPENV.H
HIST.OBJ :	[-.COMMON_D]VERSION.H
HIST.OBJ :	[-.COMMON_D]UTIL.H
HIST.OBJ :	[-.LIBDI_D]LIBDI.H
HIST.OBJ :	CDINFO.H
MOTD.OBJ :	MOTD.C
MOTD.OBJ :	[-.COMMON_D]APPENV.H
MOTD.OBJ :	[-.COMMON_D]UTIL.H
MOTD.OBJ :	[-.COMMON_D]VERSION.H
MOTD.OBJ :	[-.LIBDI_D]LIBDI.H
MOTD.OBJ :	CDINFO.H
MOTD.OBJ :	MOTD.H

