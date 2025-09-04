/*
 *   libdi - scsipt SCSI Device Interface Library
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
#ifndef __CDSIM_H__
#define __CDSIM_H__

#if (defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)) && defined(DEMO_ONLY)

#ifndef lint
static char *_cdsim_h_ident_ = "@(#)cdsim.h	6.19 03/12/12";
#endif


#define CDSIM_MAGIC		0x584d4344	/* Magic number */
#define CDSIM_MAX_DATALEN	8192		/* Max I/O data length */

/* Return status codes */
#define CDSIM_COMPOK		0	/* Command completed OK */
#define CDSIM_COMPERR		1	/* Command completed with error */
#define CDSIM_NOTSUPP		2	/* Command not supported */
#define CDSIM_PARMERR		3	/* Command parameter error */
#define CDSIM_PKTERR		4	/* Command packet error */


/* CD simulator IPC packet structure (1024 bytes in size) */
typedef struct simpkt {
	word32_t	magic;		/* Magic number */

	byte_t		pktid;		/* Packet id */
	byte_t		retcode;	/* Return code */
	byte_t		dir;		/* Data direction: OP_NODATA,
					 * OP_DATAIN or OP_DATAOUT
					 */
	byte_t		cdbsz;		/* CDB size */

	word32_t	len;		/* Data length in bytes */

	byte_t		cdb[12];	/* CDB data */

	byte_t		data[CDSIM_MAX_DATALEN];
					/* I/O Data */
} simpkt_t;


#define CDSIM_NODISC	0x00		/* CD simulator no disc */
#define CDSIM_STOPPED	0x01		/* CD simulator stopped */
#define CDSIM_PAUSED	0x02		/* CD simulator paused */
#define CDSIM_PLAYING	0x03		/* CD simulator playing audio */

#define CDSIM_NTRKS	10		/* Number of tracks on simulated CD */
#define CDSIM_NIDXS	4		/* Number of index/track on sim CD */
#define CDSIM_TRKLEN	4500		/* Length of each simulated track */
#define CDSIM_IDXLEN	1125		/* Length of each simulated track */

/* CD simulator internal status structures */

typedef struct trkstat {
	word32_t	addr;		/* Starting absolute addr of track */
	byte_t		nidxs;		/* Number of indices */
	byte_t		rsvd[3];	/* Reserved */
	word32_t	iaddr[CDSIM_NIDXS];
					/* Index absolute addresses */
} trkstat_t;

typedef struct simstat {
	byte_t		status;		/* Current mode status flag */
	byte_t		ntrks;		/* Number of tracks */
	byte_t		trkno;		/* Current track */
	byte_t		idxno;		/* Current index */

	word32_t	absaddr;	/* Current absolute address */
	word32_t	reladdr;	/* Current relative address */
	trkstat_t	trk[MAXTRACK];	/* Per-track information */
	sword32_t	startaddr;	/* Start play address */
	sword32_t	endaddr;	/* End play address */

	bool_t		caddylock;	/* Caddy locked */
} simstat_t;


#define CDSIM_PKTSZ	sizeof(simpkt_t)
#define CDSIM_INQSZ	sizeof(inquiry_data_t)


/* Public function prototypes */
extern bool_t	cdsim_sendpkt(char *, int, simpkt_t *);
extern bool_t	cdsim_getpkt(char *, int, simpkt_t *);
extern void	cdsim_main(int, int);

#endif	/* DI_SCSIPT CDDA_RD_SCSIPT DEMO_ONLY */

#endif	/* __CDSIM_H__ */

