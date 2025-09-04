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
 *   Toshiba vendor-unique support
 *
 *   The name "Toshiba" is a trademark of Toshiba Corporation, and is
 *   used here for identification purposes only.
 */
#ifndef __VU_TOSH_H__
#define __VU_TOSH_H__

#ifdef VENDOR_TOSHIBA

#ifndef lint
static char *_vu_tosh_h_ident_ = "@(#)vu_tosh.h	6.19 03/12/12";
#endif


/* Toshiba vendor-unique commands */
#define OP_VT_AUDSRCH		0xc0	/* Toshiba audio track search */
#define OP_VT_AUDPLAY		0xc1	/* Toshiba play audio */
#define OP_VT_STILL		0xc2	/* Toshiba still */
#define OP_VT_SETSTOP		0xc3	/* Toshiba set stop time */
#define OP_VT_EJECT		0xc4	/* Toshiba caddy eject */
#define OP_VT_RDSUBQ		0xc6	/* Toshiba read status */
#define OP_VT_RDINFO		0xc7	/* Toshiba read disk info */
#define OP_VT_RDMODE		0xc8	/* Toshiba read CDROM mode */


/* Toshiba audio status codes */
#define TAUD_PLAYING		0x00
#define TAUD_PAUSED		0x01
#define TAUD_SRCH_PAUSED	0x02
#define TAUD_OTHER		0x03


/* Return data lengths */
#define SZ_VT_RDSUBQ		10	/* Toshiba read sub-code Q data size */
#define SZ_VT_RDINFO		4	/* Toshiba read disc info data size */
#define SZ_VT_RDMODE		1	/* Toshiba read cdrom mode data size */


/* Toshiba read sub-code Q command data */
typedef struct tsubq_data {
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
} tsubq_data_t;


/* Toshiba read disc information command data */
typedef struct tinfo_00 {		/* track information */
	byte_t		first_trk;	/* first track (BCD) */
	byte_t		last_trk;	/* last track (BCD) */
	byte_t		reserved[2];	/* reserved */
} tinfo_00_t;

typedef struct tinfo_01 {		/* lead-out track starting time */
	byte_t		min;		/* minutes (BCD) */
	byte_t		sec;		/* seconds (BCD) */
	byte_t		frame;		/* frame (BCD) */
	byte_t		reserved;	/* reserved */
} tinfo_01_t;

typedef struct tinfo_02 {		/* specified track starting time */
	byte_t		min;		/* minutes (BCD) */
	byte_t		sec;		/* seconds (BCD) */
	byte_t		frame;		/* frame (BCD) */
	byte_t		reserved;	/* reserved */
} tinfo_02_t;


/* Public function prototypes */
extern bool_t	tosh_playaudio(byte_t, sword32_t, sword32_t, msf_t *, msf_t *,
			byte_t, byte_t);
extern bool_t	tosh_pause_resume(bool_t);
extern bool_t	tosh_get_playstatus(cdstat_t *);
extern bool_t	tosh_get_toc(curstat_t *);
extern bool_t	tosh_mute(bool_t);
extern bool_t	tosh_eject(void);
extern void	tosh_init(void);

#else

#define tosh_init	NULL

#endif	/* VENDOR_TOSHIBA */

#endif	/* __VU_TOSH_H__ */

