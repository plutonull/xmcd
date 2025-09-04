/*
 *   version.h - Common version number header file for the xmcd package
 *
 *	xmcd  - Motif(R) CD Audio Player/Ripper
 *	cda   - Command-line CD Audio Player/Ripper
 *	libdi - CD Audio Device Interface Library
 *	cdda  - CD Digital Audio support
 *
 *
 *   Copyright (C) 1993-2004  Ti Kan
 *   E-mail: xmcd@amb.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __VERSION_H__
#define __VERSION_H__

#ifndef lint
static char *_version_h_ident_ = "@(#)version.h	7.29 04/04/20";
#endif

#define VERSION_MAJ	"3"		/* Major version */
#define VERSION_MIN	"3"		/* Minor version */
#define VERSION_TEENY	"2"		/* Teeny version */
#define COPYRIGHT	"Copyright \251 1993-2004  Ti Kan"
#define EMAIL		"xmcd@amb.org"
#define XMCD_URL	"http://www.amb.org/xmcd/"
#define GNU_BANNER	\
"This is free software and comes with no warranty.\n\
See the GNU General Public License for details."
#define CDDB_BANNER	\
"This software contains support for the Gracenote\n\
CDDB\256 Music Recognition Service.  Please see\n\
the \"CDDB\" file for information."

#endif	/* __VERSION_H__ */
