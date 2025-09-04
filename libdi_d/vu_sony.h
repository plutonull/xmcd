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
 *   Sony vendor-unique support
 *
 *   The name "Sony" is a trademark of Sony Corporation, and is used
 *   here for identification purposes only.
 */
#ifndef __VU_SONY_H__
#define __VU_SONY_H__

#ifdef VENDOR_SONY

#ifndef lint
static char *_vu_sony_h_ident_ = "@(#)vu_sony.h	6.21 03/12/12";
#endif


/* Sony vendor-unique commands */
#define OP_VS_SETADDRFMT	0xc0	/* Sony set address format */
#define OP_VS_RDTOC		0xc1	/* Sony read TOC */
#define OP_VS_RDSUBQ		0xc2	/* Sony read subchannel */
#define OP_VS_RDHDR		0xc3	/* Sony read header */
#define OP_VS_PLAYSTAT		0xc4	/* Sony playback status */
#define OP_VS_PAUSE		0xc5	/* Sony pause */
#define OP_VS_PLAYTRK		0xc6	/* Sony play track */
#define OP_VS_PLAYMSF		0xc7	/* Sony play MSF */
#define OP_VS_PLAYAUD		0xc8	/* Sony play audio */
#define OP_VS_PLAYCTL		0xc9	/* Sony playback control */
#define OP_VS_READCDDA		0xd8	/* Sony read CD-DA (12) */
#define OP_VS_READCDDAMSF	0xd9	/* Sony read CD-DA (12) */


/* Sony audio status codes */
#define SAUD_PLAYING		0x00	/* audio play in progress */
#define SAUD_PAUSED		0x01	/* audio pause in progress */
#define SAUD_MUTED		0x02	/* audio muting on */
#define SAUD_COMPLETED		0x03	/* audio play successfully completed */
#define SAUD_ERROR		0x04	/* error occurred during audio play */
#define SAUD_NOTREQ		0x05	/* audio play not requested */


/* Return data lengths */
#define SZ_VS_RDTOC		604	/* Sony read TOC data size */
#define SZ_VS_TOCHDR		4	/* Sony read TOC header data size */
#define SZ_VS_TOCENT		6	/* Sony read TOC header entry size */
#define SZ_VS_RDSUBQ		9	/* Sony read sub-code Q data size */
#define SZ_VS_PLAYSTAT		18	/* Sony read disc info data size */
#define SZ_VS_CDPARM		4	/* CD-ROM parameters data size */

/* Mode Sense/Mode Select page code */
#define PG_VS_CDPARM		0x8	/* CD-ROM parameters page */


/* Sony read subchannel command data */
typedef struct ssubq_data {
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
	unsigned int	trkno:8;	/* track number */
	unsigned int	idxno:8;	/* index number */
	unsigned int	rel_min:8;	/* trk relative minutes */

	unsigned int	rel_sec:8;	/* trk relative seconds */
	unsigned int	rel_frame:8;	/* trk relative frame */
	unsigned int	abs_min:8;	/* absolute minutes */
	unsigned int	abs_sec:8;	/* absolute seconds */

	unsigned int	abs_frame:8;	/* absolute frame */
	unsigned int	pad1:24;	/* pad for alignment */
} ssubq_data_t;


/* Sony read TOC command data */
typedef struct stoc_data {
	unsigned int	data_len:16;	/* data length */
	unsigned int	first_trk:8;	/* first track */
	unsigned int	last_trk:8;	/* last track */

	byte_t		trkdata[600];	/* track info */
} stoc_data_t;

typedef struct stoc_ent {
	unsigned int	trkno:8;	/* track number */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	preemph:1;	/* preemphasis */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	res1:4;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res1:4;		/* reserved */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	preemph:1;	/* preemphasis */
#endif	/* _BYTE_ORDER_ */
	unsigned int	res2:8;		/* reserved */
	unsigned int	min:8;		/* minutes */

	unsigned int	sec:8;		/* seconds */
	unsigned int	frame:8;	/* frame */
	unsigned int	pad1:16;	/* pad bytes for alignment */
} stoc_ent_t;


/* Sony play status command data */
typedef struct sstat_data {
	unsigned int	res1:8;		/* reserved */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	lbamsf:1;	/* address format bit */
	unsigned int	res2:7;		/* reserved */
	unsigned int	data_len:16;	/* audio status data length */

	unsigned int	audio_status:8;	/* audio status */
	unsigned int	preemph:1;	/* preemphasis */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	res3:4;		/* reserved */
	unsigned int	res4:8;		/* reserved */
	unsigned int	min:8;		/* CD address: minute */

	unsigned int	sec:8;		/* CD address: second */
	unsigned int	frame:8;	/* CD address: frame */
	unsigned int	sel0:4;		/* output select for channel 0 */
	unsigned int	res5:4;		/* reserved */
	unsigned int	vol0:8;		/* channel 0 volume */

	unsigned int	sel1:4;		/* output select for channel 1 */
	unsigned int	res6:4;		/* reserved */
	unsigned int	vol1:8;		/* channel 1 volume */
	unsigned int	sel2:4;		/* output select for channel 2 */
	unsigned int	res7:4;		/* reserved */
	unsigned int	vol2:8;		/* channel 2 volume */

	unsigned int	sel3:4;		/* output select for channel 3 */
	unsigned int	res8:4;		/* reserved */
	unsigned int	vol3:8;		/* channel 3 volume */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res2:7;		/* reserved */
	unsigned int	lbamsf:1;	/* address format bit */
	unsigned int	data_len:16;	/* audio status data length */

	unsigned int	audio_status:8;	/* audio status */
	unsigned int	res3:4;		/* reserved */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	preemph:1;	/* preemphasis */
	unsigned int	res4:8;		/* reserved */
	unsigned int	min:8;		/* CD address: minute */

	unsigned int	sec:8;		/* CD address: second */
	unsigned int	frame:8;	/* CD address: frame */
	unsigned int	res5:4;		/* reserved */
	unsigned int	sel0:4;		/* output select for channel 0 */
	unsigned int	vol0:8;		/* channel 0 volume */

	unsigned int	res6:4;		/* reserved */
	unsigned int	sel1:4;		/* output select for channel 1 */
	unsigned int	vol1:8;		/* channel 1 volume */
	unsigned int	res7:4;		/* reserved */
	unsigned int	sel2:4;		/* output select for channel 2 */
	unsigned int	vol2:8;		/* channel 2 volume */

	unsigned int	res8:4;		/* reserved */
	unsigned int	sel3:4;		/* output select for channel 3 */
	unsigned int	vol3:8;		/* channel 3 volume */
#endif	/* _BYTE_ORDER_ */
	unsigned int	pad1:16;	/* pad for alignment */
} sstat_data_t;


/* Mode Sense/Mode Select CD-ROM parameters page data structure */
typedef struct cdparm_pg {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	pg_code:6;	/* page code */
	unsigned int	res0:2;		/* reserved */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	lbamsf:1;	/* LBA/MSF addr format ctrl bit */
	unsigned int	res1:7;		/* reserved */
	unsigned int	inact_mult:4;	/* inactivity timer multiplier */
	unsigned int	res2:4;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res0:2;		/* reserved */
	unsigned int	pg_code:6;	/* page code */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	res1:7;		/* reserved */
	unsigned int	lbamsf:1;	/* LBA/MSF addr format ctrl bit */
	unsigned int	res2:4;		/* reserved */
	unsigned int	inact_mult:4;	/* inactivity timer multiplier */
#endif	/* _BYTE_ORDER_ */
} cdparm_pg_t;


/* Public function prototypes */
extern bool_t	sony_playaudio(byte_t, sword32_t, sword32_t, msf_t *, msf_t *,
			byte_t, byte_t);
extern bool_t	sony_pause_resume(bool_t);
extern bool_t	sony_get_playstatus(cdstat_t *);
extern bool_t	sony_get_toc(curstat_t *);
extern int	sony_volume(int, curstat_t *, bool_t);
extern bool_t	sony_route(curstat_t *);
extern void	sony_init(void);
extern void	sony_start(void);

#else

#define sony_init	NULL

#endif	/* VENDOR_SONY */

#endif	/* __VU_SONY_H__ */

