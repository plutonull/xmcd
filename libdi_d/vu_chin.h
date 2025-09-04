/*
 *   libdi - CD Audio Device Interface Library
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
 *   Chinon CDx-431 and CDx-435 support
 *
 *   The name "Chinon" is a trademark of Chinon Industries, and is
 *   used here for identification purposes only.
 */
#ifndef __VU_CHIN_H__
#define __VU_CHIN_H__

#ifdef VENDOR_CHINON

#ifndef lint
static char *_vu_chin_h_ident_ = "@(#)vu_chin.h	6.18 03/12/12";
#endif


/* Chinon vendor-unique commands */
#define OP_VC_EJECT		0xc0	/* Chinon eject disc */
#define OP_VC_STOP		0xc6	/* Chinon audio stop */


/* Chinon audio status codes */
#define CAUD_INVALID		0x00
#define CAUD_PLAYING		0x11
#define CAUD_PAUSED		0x15


/* Public function prototypes */
extern bool_t	chin_playaudio(byte_t, sword32_t, sword32_t, msf_t *, msf_t *,
			byte_t, byte_t);
extern bool_t	chin_start_stop(bool_t, bool_t);
extern bool_t	chin_get_playstatus(cdstat_t *);
extern bool_t	chin_get_toc(curstat_t *);
extern bool_t	chin_eject(void);
extern void	chin_init(void);

#else

#define chin_init	NULL

#endif	/* VENDOR_CHINON */

#endif	/* __VU_CHIN_H__ */

