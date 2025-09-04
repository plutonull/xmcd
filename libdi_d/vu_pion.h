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
 *   Pioneer vendor-unique support
 *
 *   The name "Pioneer" is a trademark of Pioneer Corporation, and is
 *   used here for identification purposes only.
 */
#ifndef __VU_PION_H__
#define __VU_PION_H__

#ifdef VENDOR_PIONEER

#ifndef lint
static char *_vu_pion_h_ident_ = "@(#)vu_pion.h	6.19 03/12/12";
#endif


/* Pioneer vendor-unique commands */
#define OP_VP_EJECT		0xc0	/* Pioneer magazine eject */
#define OP_VP_RDTOC		0xc1	/* Pioneer read TOC */
#define OP_VP_RDSUBQ		0xc2	/* Pioneer read subcode Q */
#define OP_VP_RDHDR		0xc3	/* Pioneer read header */
#define OP_VP_AUDSRCH		0xc8	/* Pioneer audio search */
#define OP_VP_AUDPLAY		0xc9	/* Pioneer play audio */
#define OP_VP_PAUSE		0xca	/* Pioneer pause */
#define OP_VP_AUDSTOP		0xcb	/* Pioneer audio stop */
#define OP_VP_AUDSTAT		0xcc	/* Pioneer audio status */
#define OP_VP_AUDSCAN		0xcd	/* Pioneer audio scan */
#define OP_VP_RDDSTAT		0xe0	/* Pioneer read drive status */
#define OP_VP_WRCDP		0xe3	/* Pioneer write CDP */
#define OP_VP_RDCDP		0xe4	/* Pioneer read CDP */


/* Pioneer audio status codes */
#define PAUD_PLAYING		0x00
#define PAUD_PAUSED		0x01
#define PAUD_MUTEPLAY		0x02
#define PAUD_COMPLETED		0x03
#define PAUD_ERROR		0x04
#define PAUD_NOSTATUS		0x05


/* Return data lengths */
#define SZ_VP_RDTOC		4	/* Pioneer read TOC data size */
#define SZ_VP_RDSUBQ		9	/* Pioneer read subcode Q data size */
#define SZ_VP_AUDSTAT		6	/* Pioneer audio status data size */


/* Pioneer read audio status command data */
typedef struct paudstat_data {
	unsigned int	status:8;	/* audio status */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	mode:4;		/* play mode */
	unsigned int	res1:4;		/* reserved */
	unsigned int	preemph:1;	/* preemphasis off=0 on=1 */
	unsigned int	copyallow:1;	/* digital copy allowed off=0 on=1 */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	res2:4;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res1:4;		/* reserved */
	unsigned int	mode:4;		/* play mode */
	unsigned int	res2:4;		/* reserved */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allowed off=0 on=1 */
	unsigned int	preemph:1;	/* preemphasis off=0 on=1 */
#endif	/* _BYTE_ORDER_ */
	unsigned int	abs_min:8;	/* absolute minutes (BCD) */

	unsigned int	abs_sec:8;	/* absolute seconds (BCD) */
	unsigned int	abs_frame:8;	/* absolute frame (BCD) */
	unsigned int	pad1:16;	/* pad for alignment */
} paudstat_data_t;



/* Pioneer read subcode Q command data */
typedef struct psubq_data {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	preemph:1;	/* preemphasis off=0 on=1 */
	unsigned int	copyallow:1;	/* digital copy allowed off=0 on=1 */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	res:4;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res:4;		/* reserved */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allowed off=0 on=1 */
	unsigned int	preemph:1;	/* preemphasis off=0 on=1 */
#endif	/* _BYTE_ORDER_ */
	unsigned int	trkno:8;	/* track number (BCD) */
	unsigned int	idxno:8;	/* index number (BCD) */
	unsigned int	rel_min:8;	/* trk relative minutes (BCD) */

	unsigned int	rel_sec:8;	/* trk relative seconds (BCD) */
	unsigned int	rel_frame:8;	/* trk relative frame (BCD) */
	unsigned int	abs_min:8;	/* absolute minutes (BCD) */
	unsigned int	abs_sec:8;	/* absolute seconds (BCD) */

	unsigned int	abs_frame:8;	/* absolute frame (BCD) */
	unsigned int	pad1:24;	/* pad for alignment */
} psubq_data_t;


/* Pioneer read TOC command data */
typedef struct pinfo_00 {		/* track information */
	byte_t		first_trk;	/* first track (BCD) */
	byte_t		last_trk;	/* last track (BCD) */
	byte_t		reserved[2];	/* reserved */
} pinfo_00_t;

typedef struct pinfo_01 {		/* lead-out track starting time */
	byte_t		min;		/* minutes (BCD) */
	byte_t		sec;		/* seconds (BCD) */
	byte_t		frame;		/* frame (BCD) */
	byte_t		reserved;	/* reserved */
} pinfo_01_t;

typedef struct pinfo_02 {		/* specified track starting time */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	preemph:1;	/* preemphasis off=0 on=1 */
	unsigned int	copyallow:1;	/* digital copy allowed off=0 on=1 */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	res:4;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res:4;		/* reserved */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allowed off=0 on=1 */
	unsigned int	preemph:1;	/* preemphasis off=0 on=1 */
#endif	/* _BYTE_ORDER_ */

	unsigned int	min:8;		/* minutes (BCD) */
	unsigned int	sec:8;		/* seconds (BCD) */
	unsigned int	frame:8;	/* frame (BCD) */
} pinfo_02_t;


/* Public function prototypes */
extern bool_t	pion_playaudio(byte_t, sword32_t, sword32_t, msf_t *, msf_t *,
			byte_t, byte_t);
extern bool_t	pion_pause_resume(bool_t);
extern bool_t	pion_get_playstatus(cdstat_t *);
extern bool_t	pion_get_toc(curstat_t *);
extern bool_t	pion_eject(void);
extern void	pion_init(void);

#else

#define pion_init	NULL

#endif	/* VENDOR_PIONEER */

#endif	/* __VU_PION_H__ */

