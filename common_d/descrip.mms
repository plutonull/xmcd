#
#   @(#)descrip.mms	1.6 04/03/07
#
#   OpenVMS MMS "Makefile" for libutil
#
#	libutil - Utility functions
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
cflags= /stand=rel/extern=strict/include=("../")-
	/def=(_POSIX_C_SOURCE,has_pthreads)

all :	libutil(UTIL)
	! done 

libutil(UTIL) :	UTIL.obj

UTIL.obj :	UTIL.C
UTIL.obj :	APPENV.H
UTIL.obj :	UTIL.H
