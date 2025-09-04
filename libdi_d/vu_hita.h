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
 *   Hitachi SCSI-1 vendor-unique support
 *
 *   The name "Hitachi" is a trademark of Hitachi Corporation, and is
 *   used here for identification purposes only.
 */
#ifndef __VU_HITA_H__
#define __VU_HITA_H__

#ifdef VENDOR_HITACHI

#ifndef lint
static char *_vu_hita_h_ident_ = "@(#)vu_hita.h	6.20 03/12/12";
#endif


/* Hitachi vendor-unique commands */
#define OP_VH_AUDPLAY		0xe0	/* Hitachi play audio */
#define OP_VH_PAUSE		0xe1	/* Hitachi pause */
#define OP_VH_PLAYTRK		0xe2	/* Hitachi play audio track */
#define OP_VH_RDINFO		0xe3	/* Hitachi read disk info */
#define OP_VH_EJECT		0xe4	/* Hitachi eject */
#define OP_VH_RDSTAT		0xe5	/* Hitachi read audio status */
#define OP_VH_PWRSAVE		0xe6	/* Hitachi power save mode */
#define OP_VH_RDXINFO		0xe8	/* Hitachi read extended disk info */


/* Return data lengths */
#define SZ_VH_PAUSE		3	/* Hitachi pause data size */
#define SZ_VH_RDINFO		303	/* Hitachi disk info data size */
#define SZ_VH_TOCHDR		3	/* Hitachi disc info header size */
#define SZ_VH_TOCENT		3	/* Hitachi disc info per-track
					 * entry size
					 */
#define SZ_VH_RDSTAT		11	/* Hitachi audio status data size */
#define SZ_VH_RDXINFO		404	/* Hitachi ext disk info data size */
#define SZ_VH_XTOCHDR		4	/* Hitachi ext disc info header size */
#define SZ_VH_XTOCENT		4	/* Hitachi ext disc info per-track
					 * entry size
					 */

/* Macros to compute the real offset of the data area in several
 * data structures.  The pad byte in the beginning of some structures
 * are there to prevent the compiler from inserting a pad byte in
 * the middle of the structure.  The alternative is to use #pragma pack(1),
 * but that's not very portable.
 */
#define AD_VH_PAUSE(x)		(x)	/* Hitachi pause data addr */
#define AD_VH_RDINFO(x)		((x)+1)	/* Hitachi disk info data addr */
#define AD_VH_RDSTAT(x)		(x)	/* Hitachi audio status data addr */
#define AD_VH_RDXINFO(x)	(x)	/* Hitachi ext disk info data addr */


/* Hitachi MSF location */
typedef struct hmsf {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	min:7;			/* minute */
	unsigned int	data:1;			/* 0=audio 1=data */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	data:1;			/* 0=audio 1=data */
	unsigned int	min:7;			/* minute */
#endif	/* _BYTE_ORDER_ */
	unsigned int	sec:8;			/* second */
	unsigned int	frame:8;		/* frame */
	unsigned int	res:8;			/* reserved */
} hmsf_t;


/* Hitachi extended MSF location */
typedef struct hxmsf {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	adr:4;			/* ADR code */
        unsigned int	preemph:1;		/* preemphasis */
	unsigned int	copyallow:1;		/* digital copy allow */
	unsigned int	trktype:1;		/* 0=audio 1=data */
	unsigned int	audioch:1;		/* 0=2ch 1=4ch */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	audioch:1;		/* 0=2ch 1=4ch */
	unsigned int	trktype:1;		/* 0=audio 1=data */
	unsigned int	copyallow:1;		/* digital copy allow */
        unsigned int	preemph:1;		/* preemphasis */
	unsigned int	adr:4;			/* ADR code */
#endif	/* _BYTE_ORDER_ */
	unsigned int	min:8;			/* minute */
	unsigned int	sec:8;			/* second */
	unsigned int	frame:8;		/* frame */
} hxmsf_t;


/* Hitachi Disk Info data */
typedef struct hdiscinfo {
	unsigned int	pad0:8;			/* pad byte for alignment */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	audio:1;		/* disc has audio tracks */
	unsigned int	data:1;			/* disc has data tracks */
	unsigned int	res:6;			/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res:6;			/* reserved */
	unsigned int	data:1;			/* disc has data tracks */
	unsigned int	audio:1;		/* disc has audio tracks */
#endif	/* _BYTE_ORDER_ */
	unsigned int	first_trk:8;		/* first track number */
	unsigned int	last_trk:8;		/* last track number */

	byte_t		msfdata[300];		/* Track MSF address data */
} hdiscinfo_t;


/* Hitachi Audio Status data */
typedef struct haudstat {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	playing:1;		/* Audio playing */
	unsigned int	res1:7;			/* Reserved */
	unsigned int	adr:4;			/* ADR code */
        unsigned int	preemph:1;		/* preemphasis */
	unsigned int	copyallow:1;		/* digital copy allow */
	unsigned int	trktype:1;		/* 0=audio 1=data */
	unsigned int	audioch:1;		/* 0=2ch 1=4ch */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res1:7;			/* Reserved */
	unsigned int	playing:1;		/* Audio playing */
	unsigned int	audioch:1;		/* 0=2ch 1=4ch */
	unsigned int	trktype:1;		/* 0=audio 1=data */
	unsigned int	copyallow:1;		/* digital copy allow */
        unsigned int	preemph:1;		/* preemphasis */
	unsigned int	adr:4;			/* ADR code */
#endif	/* _BYTE_ORDER_ */
	unsigned int	trkno:8;		/* track number */
	unsigned int	res2:8;			/* reserved */

	hmsf_t		rel_addr;		/* relative address */
	hmsf_t		abs_addr;		/* absolute address */
} haudstat_t;


/* Hitachi Extended Disk Info data */
typedef struct hxdiscinfo {
	byte_t		res[2];			/* reserved */
	byte_t		first_trk;		/* first track number */
	byte_t		last_trk;		/* last track number */

	byte_t		xmsfdata[400];		/* track MSF address data */
} hxdiscinfo_t;


/* Public function prototypes */
extern bool_t	hita_playaudio(byte_t, sword32_t, sword32_t, msf_t *, msf_t *,
			byte_t, byte_t);
extern bool_t	hita_pause_resume(bool_t);
extern bool_t	hita_start_stop(bool_t, bool_t);
extern bool_t	hita_get_playstatus(cdstat_t *);
extern bool_t	hita_get_toc(curstat_t *);
extern bool_t	hita_mute(bool_t);
extern bool_t	hita_eject(void);
extern void	hita_init(void);

#else

#define hita_init	NULL

#endif	/* VENDOR_HITACHI */

#endif	/* __VU_HITA_H__ */

