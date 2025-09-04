/*
 *   cdinfo - CD Information Management Library
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
#ifndef __MOTD_H__
#define __MOTD_H__

#ifndef lint
static char *_motd_h_ident_ = "@(#)motd.h	7.4 04/04/20";
#endif


/* SOCKS support */
#ifdef SOCKS
#define SOCKSINIT(x)	SOCKSinit(x)
#define SOCKET		socket
#define CONNECT		Rconnect
#else
#define SOCKSINIT(x)
#define SOCKET		socket
#define CONNECT		connect
#endif  /* SOCKS */


/* Public functions */
extern void		motd_get(byte_t *);

#endif	/* __MOTD_H__ */
