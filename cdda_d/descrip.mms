#
#   @(#)descrip.mms	1.17 04/01/10
#
#   OpenVMS MMS "Makefile" for cdda
#
#	cdda - CD Digital Audio support
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
cflags= /stand=rel/extern=strict/include=("../","mme")-
	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads)
objects= CDDA,COMMON,PTHR,RD_SCSI,WR_FP,WR_GEN,WR_OSF1,IF_LAME

all : libcdda($(objects))
	! done

libcdda(CDDA) : CDDA.obj
libcdda(COMMON) : COMMON.obj
libcdda(PTHR) : PTHR.obj
libcdda(RD_SCSI) : RD_SCSI.obj
libcdda(WR_FP) : WR_FP.obj
libcdda(WR_GEN) : WR_GEN.obj
libcdda(WR_OSF1) : WR_OSF1.obj
libcdda(IF_LAME) : IF_LAME.obj

CDDA.OBJ :	CDDA.C
CDDA.OBJ :	[-.COMMON_D]APPENV.H
CDDA.OBJ :	[-.COMMON_D]UTIL.H
CDDA.OBJ :	[-.LIBDI_D]LIBDI.H
CDDA.OBJ :	CDDA.H
CDDA.OBJ :	COMMON.H
CDDA.OBJ :	SYSVIPC.H
CDDA.OBJ :	WR_GEN.H
CDDA.OBJ :	IF_LAME.H
COMMON.OBJ :	COMMON.C
COMMON.OBJ :	[-.COMMON_D]APPENV.H
COMMON.OBJ :	[-.COMMON_D]UTIL.H
COMMON.OBJ :	[-.LIBDI_D]LIBDI.H
COMMON.OBJ :	CDDA.H
COMMON.OBJ :	COMMON.H
COMMON.OBJ :	SYSVIPC.H
COMMON.OBJ :	PTHR.H
PTHR.OBJ :	PTHR.C
PTHR.OBJ :	[-.COMMON_D]APPENV.H
PTHR.OBJ :	[-.COMMON_D]UTIL.H
PTHR.OBJ :	[-.LIBDI_D]LIBDI.H
PTHR.OBJ :	CDDA.H
PTHR.OBJ :	COMMON.H
PTHR.OBJ :	PTHR.H
RD_SCSI.OBJ :	RD_SCSI.C
RD_SCSI.OBJ :	[-.COMMON_D]APPENV.H
RD_SCSI.OBJ :	[-.COMMON_D]UTIL.H
RD_SCSI.OBJ :	[-.LIBDI_D]LIBDI.H
RD_SCSI.OBJ :	CDDA.H
RD_SCSI.OBJ :	COMMON.H
RD_SCSI.OBJ :	RD_SCSI.H
WR_FP.OBJ :	WR_FP.C
WR_FP.OBJ :	[-.COMMON_D]APPENV.H
WR_FP.OBJ :	[-.COMMON_D]UTIL.H
WR_FP.OBJ :	[-.LIBDI_D]LIBDI.H
WR_FP.OBJ :	CDDA.H
WR_FP.OBJ :	COMMON.H
WR_FP.OBJ :	WR_FP.H
WR_GEN.OBJ :	WR_GEN.C
WR_GEN.OBJ :	[-.COMMON_D]APPENV.H
WR_GEN.OBJ :	[-.COMMON_D]VERSION.H
WR_GEN.OBJ :	[-.COMMON_D]UTIL.H
WR_GEN.OBJ :	[-.LIBDI_D]LIBDI.H
WR_GEN.OBJ :	[-.CDINFO_D]CDINFO.H
WR_GEN.OBJ :	CDDA.H
WR_GEN.OBJ :	COMMON.H
WR_GEN.OBJ :	WR_GEN.H
WR_GEN.OBJ :	AU.H
WR_GEN.OBJ :	WAV.H
WR_GEN.OBJ :	AIFF.H
WR_GEN.OBJ :	IF_LAME.H
WR_GEN.OBJ :	ID3MAP.H
WR_OSF1.OBJ :	WR_OSF1.C
WR_OSF1.OBJ :	[-.COMMON_D]APPENV.H
WR_OSF1.OBJ :	[-.COMMON_D]UTIL.H
WR_OSF1.OBJ :	[-.LIBDI_D]LIBDI.H
WR_OSF1.OBJ :	CDDA.H
WR_OSF1.OBJ :	COMMON.H
WR_OSF1.OBJ :	WR_GEN.H
WR_OSF1.OBJ :	WR_OSF1.H
IF_LAME.OBJ :	IF_LAME.C
IF_LAME.OBJ :	[-.COMMON_D]APPENV.H
IF_LAME.OBJ :	[-.COMMON_D]VERSION.H
IF_LAME.OBJ :	[-.LIBDI_D]LIBDI.H
IF_LAME.OBJ :	[-.COMMON_D]UTIL.H
IF_LAME.OBJ :	CDDA.H
IF_LAME.OBJ :	WR_GEN.H
IF_LAME.OBJ :	IF_LAME.H
