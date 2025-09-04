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
 *   NEC vendor-unique support
 *
 *   The name "NEC" is a trademark of NEC Corporation, and is used here
 *   for identification purposes only.
 */
#ifndef __VU_NEC_H__
#define __VU_NEC_H__

#ifdef VENDOR_NEC

#ifndef lint
static char *_vu_nec_h_ident_ = "@(#)vu_nec.h	6.20 03/12/12";
#endif


/* NEC vendor-unique commands */
#define OP_VN_READCDDA		0xd4	/* NEC read CD-DA (10) */
#define OP_VN_READCDDAMSF	0xd5	/* NEC read CD-DA MSF */
#define OP_VN_AUDSRCH		0xd8	/* NEC audio search */
#define OP_VN_AUDPLAY		0xd9	/* NEC play audio */
#define OP_VN_STILL		0xda	/* NEC still */
#define OP_VN_SETSTOP		0xd8	/* NEC set stop time */
#define OP_VN_EJECT		0xdc	/* NEC eject */
#define OP_VN_RDSUBQ		0xdd	/* NEC read subcode Q */
#define OP_VN_RDTOC		0xde	/* NEC read TOC */


/* NEC audio status codes */
#define NAUD_PLAYING		0x00
#define NAUD_PAUSED		0x01
#define NAUD_SRCH_PAUSED	0x02
#define NAUD_COMPLETED		0x03


/* Return data lengths */
#define SZ_VN_RDSUBQ		10	/* NEC read subcode Q data size */
#define SZ_VN_RDTOC		4	/* NEC read TOC data size */


/* NEC read sub-code Q command data */
typedef struct nsubq_data {
	unsigned int	audio_status:8;	/* audio status */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	preemph:1;	/* preemphasis */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	reserved:4;	/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	reserved:4;	/* reserved */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	preemph:1;	/* preemphasis */
#endif	/* _BYTE_ORDER_ */
	unsigned int	trkno:8;	/* track number (BCD) */
	unsigned int	idxno:8;	/* index number (BCD) */

	byte_t		rel_min;	/* trk relative minutes (BCD) */
	byte_t		rel_sec;	/* trk relative seconds (BCD) */
	byte_t		rel_frame;	/* trk relative frame (BCD) */
	byte_t		abs_min;	/* absolute minutes (BCD) */

	byte_t		abs_sec;	/* absolute seconds (BCD) */
	byte_t		abs_frame;	/* absolute frame (BCD) */
	byte_t		pad1[2];	/* pad for alignment */
} nsubq_data_t;


/* NEC read TOC command data */
typedef struct ninfo_00 {		/* track information */
	byte_t		first_trk;	/* first track (BCD) */
	byte_t		last_trk;	/* last track (BCD) */
	byte_t		reserved[2];	/* reserved */
} ninfo_00_t;

typedef struct ninfo_01 {		/* lead-out track starting time */
	byte_t		min;		/* minutes (BCD) */
	byte_t		sec;		/* seconds (BCD) */
	byte_t		frame;		/* frame (BCD) */
	byte_t		reserved;	/* reserved */
} ninfo_01_t;

typedef struct ninfo_02 {		/* specified track starting time */
	byte_t		min;		/* minutes (BCD) */
	byte_t		sec;		/* seconds (BCD) */
	byte_t		frame;		/* frame (BCD) */
	byte_t		trktype;	/* track type */
} ninfo_02_t;


/* Public function prototypes */
extern bool_t	nec_playaudio(byte_t, sword32_t, sword32_t, msf_t *, msf_t *,
			byte_t, byte_t);
extern bool_t	nec_pause_resume(bool_t);
extern bool_t	nec_get_playstatus(cdstat_t *);
extern bool_t	nec_get_toc(curstat_t *);
extern bool_t	nec_mute(bool_t);
extern bool_t	nec_eject(void);
extern void	nec_init(void);

#else

#define nec_init	NULL

#endif	/* VENDOR_NEC */

#endif	/* __VU_NEC_H__ */

