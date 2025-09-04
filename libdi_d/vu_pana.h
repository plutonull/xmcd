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
 *   Panasonic/Matsushita vendor-qunie support
 *
 *   The name "Panasonic" is a trademark of Matsushita Electric
 *   Corporation, and is used here for identification purposes only.
 */
#ifndef __VU_PANA_H__
#define __VU_PANA_H__

#ifdef VENDOR_PANASONIC

#ifndef lint
static char *_vu_pana_h_ident_ = "@(#)vu_pana.h	6.23 03/12/12";
#endif


/* Panasonic/Matsushita vendor-unique commands */
#define OP_VM_RDSUBQ		0xc2	/* Read subchannel */
#define OP_VM_RDTOC		0xc3	/* Read TOC */
#define OP_VM_PLAY		0xc5	/* Play audio */
#define OP_VM_PLAYMSF		0xc7	/* Play audio MSF */
#define OP_VM_PAUSE		0xcb	/* Pause/Resume */


/* Return data lengths */
#define SZ_VM_RDSUBQ		16	/* Read subchannel data size */
#define SZ_VM_RDTOC		804	/* Read TOC data size */
#define SZ_VM_TOCHDR		4	/* Read TOC header data size */
#define SZ_VM_TOCENT		8	/* Read TOC per-track entry size */
#define SZ_VM_AUDCTL		16	/* Audio control page size */


/* Mode Sense/Mode Select page code */
#define PG_VM_AUDCTL		0x2e	/* Audio control page */


/* Public function prototypes */
extern bool_t	pana_playaudio(byte_t, sword32_t, sword32_t, msf_t *, msf_t *,
			byte_t, byte_t);
extern bool_t	pana_pause_resume(bool_t);
extern bool_t	pana_get_playstatus(cdstat_t *);
extern bool_t	pana_get_toc(curstat_t *);
extern int	pana_volume(int, curstat_t *, bool_t);
extern bool_t	pana_route(curstat_t *);
extern void	pana_init(void);

#else

#define pana_init	NULL

#endif	/* VENDOR_PANASONIC */

#endif	/* __VU_PANA_H__ */

