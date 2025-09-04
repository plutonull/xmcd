/*
 *   cdda - CD Digital Audio support
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
 */

/*
 *   ALSA (Advanced Linux Sound Architecture) sound driver support
 *   (For ALSA 0.9.x and later only)
 *
 *   This program will exit with success status if ALSA support is compiled
 *   in to libcdda.a, or failure if not.  It is used by libdi_d/config.sh to
 *   set an appropriate configuration.
 */
#ifndef lint
static char *_has_alsa_c_ident_ = "@(#)has_alsa.c	7.6 03/12/12";
#endif

#include "common_d/appenv.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"

int
main()
{
#if defined(CDDA_WR_ALSA) && defined(CDDA_SYSVIPC)
	exit(0);
#else
	exit(1);
#endif
	/*NOTREACHED*/
}

