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
#ifndef __SCSIPT_H__
#define __SCSIPT_H__

#if defined(DI_SCSIPT) || defined(CDDA_RD_SCSIPT)

#ifndef lint
static char *_scsipt_h_ident_ = "@(#)scsipt.h	6.78 04/01/14";
#endif


#define MAX_VENDORS		8	/* Max number of vendors */

/*
 * Vendor-unique modules
 *
 * Undefine any of these (except VENDOR_SCSI2) to remove
 * vendor-unique support for a particular vendor.  Removing
 * unused vendor-unique modules can reduce the executable
 * binary size and run-time memory usage.
 */
#define VENDOR_SCSI2		0	/* SCSI-2 mode */
#define VENDOR_CHINON		1	/* Chinon vendor-unique mode */
#define VENDOR_HITACHI		2	/* Hitachi vendor-unique mode */
#define VENDOR_NEC		3	/* NEC vendor-unique mode */
#define VENDOR_PIONEER		4	/* Pioneer vendor-unique mode */
#define VENDOR_SONY		5	/* Sony vendor-unique mode */
#define VENDOR_TOSHIBA		6	/* Toshiba vendor-unique mode */
#define VENDOR_PANASONIC	7	/* Panasonic vendor-unique mode */


/* Data direction code */
#define OP_NODATA		0	/* SCSI no data */
#define OP_DATAIN		1	/* SCSI data-in */
#define OP_DATAOUT		2	/* SCSI data-out */


/* Default command timeout interval */
#define DFLT_CMD_TIMEOUT	10	/* seconds */


/* Read Sub-channel audio status */
#define AUDIO_NOTVALID		0x00	/* audio status not valid */
#define AUDIO_PLAYING		0x11	/* audio play in progress */
#define AUDIO_PAUSED		0x12	/* audio play paused */
#define AUDIO_COMPLETED		0x13	/* audio play successfully completed */
#define AUDIO_FAILED		0x14	/* audio played stopped due to error */
#define AUDIO_NOSTATUS		0x15	/* no audio status */


/* SCSI command opcodes */

/* 6-byte commands */
#define OP_S_TEST		0x00	/* test unit ready */
#define OP_S_REZERO		0x01	/* rezero */
#define OP_S_RSENSE		0x03	/* request sense */
#define OP_S_READ		0x08	/* read (8) */
#define OP_S_SEEK		0x0b	/* seek */
#define OP_S_INQUIR		0x12	/* inquiry */
#define OP_S_MSELECT		0x15	/* mode select (6) */
#define OP_S_MSENSE		0x1a	/* mode sense (6) */
#define OP_S_START		0x1b	/* start/stop unit */
#define OP_S_PREVENT		0x1e	/* prevent/allow medium removal */
#define OP_S_INITELEMSTAT	0x07	/* initialize element status */

/* 10-byte commands */
#define OP_M_RDCAP		0x25	/* read capacity */
#define OP_M_READ		0x28	/* read (10) */
#define OP_M_RDSUBQ		0x42	/* read subchannel */
#define OP_M_RDTOC		0x43	/* read TOC */
#define OP_M_RDHDR		0x44	/* read header */
#define OP_M_PLAY		0x45	/* play audio (10) */
#define OP_M_PLAYMSF		0x47	/* play audio MSF */
#define OP_M_PLAYTI		0x48	/* play audio track/index */
#define OP_M_PLAYTR		0x49	/* play audio track relative (10) */
#define OP_M_PAUSE		0x4b	/* pause/resume */
#define OP_M_MSELECT		0x55	/* mode select (10) */
#define OP_M_MSENSE		0x5a	/* mode sense (10) */

/* 12-byte commands */
#define OP_L_PLAY		0xa5	/* play audio (12) */
#define OP_L_READ		0xa8	/* read (12) */
#define OP_L_PLAYTR		0xa9	/* play audio track relative (12) */
#define OP_L_MOVEMED		0xa5	/* move medium */
#define OP_L_READELEMSTAT	0xb8	/* read element status */
#define OP_L_READCD		0xbe	/* MMC read CD */


/* Data lengths */
#define SZ_RDSUBQ		48	/* max read sub-channel Q data size */
#define SZ_MCNDATA		13	/* media catalog number data size */
#define SZ_ISRCDATA		12	/* intl std recording code data size */
#define SZ_RDTOC		804	/* max read TOC (tracks) data size */
#define SZ_RDCDTEXT		8192	/* max read TOC (CD-TEXT) data size */
#define SZ_CDTEXTPKD		12	/* CDTEXT per-pack data size */
#define SZ_CDTEXTINFO		512	/* CDTEXT scratch buffer size */
#define SZ_TOCHDR		4	/* TOC header size */
#define SZ_TOCENT		8	/* TOC per-track entry size */
#define SZ_MSENSE		60	/* max mode sense/mode sel data size */
#define SZ_AUDIOCTL		16	/* audio control page size */
#define SZ_DEVCAPAB		16	/* device capab page size */
#define SZ_ELEMADDR		20	/* element addr page size */
#define SZ_RSENSE		18	/* max request sense data size */
#define SZ_ELEMHDR		8	/* med chgr element status header */
#define SZ_ELEMSTAT		8	/* med chgr element status */


/* CD-ROM Mode sense/mode select page codes */
#define PG_ERRECOV		0x01	/* error recovery parameters page */
#define PG_DISCONN		0x02	/* disconn/conn parameters page */
#define PG_CDROMCTL		0x0d	/* cd-rom control parameters page */
#define PG_AUDIOCTL		0x0e	/* audio control parameters page */
#define PG_ALL			0x3f	/* 0x01, 0x02, 0x0d and 0x0e */

/* Medium-changer Mode sense/mode select page codes */
#define PG_DEVCAPAB		0x1f	/* device capabilities page */
#define PG_ELEMADDR		0x1d	/* element address assignments  page */


/* Read sub-channel format codes */
#define SUB_ALL			0x00	/* sub-Q channel data */
#define SUB_CURPOS		0x01	/* current CD-ROM position */
#define SUB_MCN			0x02	/* media catalog num (UPC/bar code) */
#define SUB_ISRC		0x03	/* track ISRC code */


/* Inquiry data misc definitions */
#define DEV_WORM		0x04	/* WORM/CD-R peripheral device type */
#define DEV_ROM			0x05	/* ROM peripheral device type */
#define DEV_CHANGER		0x08	/* Medium changer device type */
#define DEV_CONNECTED		0x00	/* peripheral qualifier */


/* SCSI CDB handling */
#define MAX_CMDLEN		12	/* Max SCSI CDB length */
#define SCSICDB_RESET(p)	(void) memset(p, 0, MAX_CMDLEN);


/* CDDA-over-SCSI read method code */
#define CDDA_MMC		0	/* Use MMC read command */
#define CDDA_STD		1	/* Use SCSI read command */
#define CDDA_NEC		2	/* Use NEC vendor-unique */
#define CDDA_SONY		3	/* Use Sony vendor-unique */


/*
 * SCSI inquiry data
 */
typedef struct inqry_data {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	type:5;		/* peripheral device type */
	unsigned int	pqual:3;	/* peripheral qualifier */
	unsigned int	qualif:7;	/* device type qualifier */
	unsigned int	rmb:1;		/* removable media */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	pqual:3;	/* peripheral qualifier */
	unsigned int	type:5;		/* peripheral device type */
	unsigned int	rmb:1;		/* removable media */
	unsigned int	qualif:7;	/* device type qualifier */
#endif	/* _BYTE_ORDER_ */

	unsigned int	ver:8;		/* SCSI version */
	unsigned int	res1:8;		/* reserved */

	byte_t		len;		/* length of additional data */
	byte_t		res2[3];	/* reserved */
	byte_t		vendor[8];	/* vendor ID */
	byte_t		prod[16];	/* product ID */
	byte_t		revnum[4];	/* revision number */
} inquiry_data_t;


/*
 * Mode Sense/Mode Select data
 */

/* Mode Sense/Mode Select (6 byte) data */
typedef struct mode_sense_6_data {
	/* mode header */
	unsigned int	data_len:8;	/* data length */
	unsigned int	medium:8;	/* medium */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	speed:4;	/* speed */
	unsigned int	buffered:3;	/* buffered */
	unsigned int	wprot:1;	/* write protected */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	wprot:1;	/* write protected */
	unsigned int	buffered:3;	/* buffered */
	unsigned int	speed:4;	/* speed */
#endif	/* _BYTE_ORDER_ */
	unsigned int	bdescr_len:8;	/* block descriptor length */

	byte_t		data[48];	/* block desc/page desc data */
} mode_sense_6_data_t;


/* Mode Sense/Mode Select (10 byte) data */
typedef struct mode_sense_10_data {
	/* mode header */
	unsigned int	data_len:16;	/* data length */
	unsigned int	medium:8;	/* medium */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	speed:4;	/* speed */
	unsigned int	buffered:3;	/* buffered */
	unsigned int	wprot:1;	/* write protected */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	wprot:1;	/* write protected */
	unsigned int	buffered:3;	/* buffered */
	unsigned int	speed:4;	/* speed */
#endif	/* _BYTE_ORDER_ */

	unsigned int	res:16;		/* reserved */
	unsigned int	bdescr_len:16;	/* block descriptor length */

	byte_t		data[48];	/* block desc/page desc data */
} mode_sense_10_data_t;


/* Block descriptor data */
typedef struct blk_desc {
	unsigned int	dens_code:8;	/* density code */
	unsigned int	num_blks:24;	/* number of blocks */

	unsigned int	res:8;		/* reserved */
	unsigned int	blk_len:24;	/* block length */
} blk_desc_t;


/* CD-ROM Audio-parameters page (0x0e) data */
typedef struct audio_pg {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	pg_code:6;	/* page code */
	unsigned int	res:2;		/* reserved */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	res1:1;		/* reserved */
	unsigned int	sotc:1;		/* SOTC */
	unsigned int	immed:1;	/* immediate */
	unsigned int	res2:5;		/* reserved */
	unsigned int	res3:8;		/* reserved */

	unsigned int	res4:16;	/* reserved */
	unsigned int	audio_bps:16;	/* logical blocks per second */

	unsigned int	p0_ch_ctrl:4;	/* port 0 channel control */
	unsigned int	res5:4;		/* reserved */
	unsigned int	p0_vol:8;	/* port 0 volume */
	unsigned int	p1_ch_ctrl:4;	/* port 1 channel control */
	unsigned int	res6:4;		/* reserved */
	unsigned int	p1_vol:8;	/* port 1 volume */

	unsigned int	p2_ch_ctrl:4;	/* port 2 channel control */
	unsigned int	res7:4;		/* reserved */
	unsigned int	p2_vol:8;	/* port 2 volume */
	unsigned int	p3_ch_ctrl:4;	/* port 3 channel control */
	unsigned int	res8:4;		/* reserved */
	unsigned int	p3_vol:8;	/* port 3 volume */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res:2;		/* reserved */
	unsigned int	pg_code:6;	/* page code */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	res2:5;		/* reserved */
	unsigned int	immed:1;	/* immediate */
	unsigned int	sotc:1;		/* SOTC */
	unsigned int	res1:1;		/* reserved */
	unsigned int	res3:8;		/* reserved */

	unsigned int	res4:16;	/* reserved */
	unsigned int	audio_bps:16;	/* logical blocks per second */

	unsigned int	res5:4;		/* reserved */
	unsigned int	p0_ch_ctrl:4;	/* port 0 channel control */
	unsigned int	p0_vol:8;	/* port 0 volume */
	unsigned int	res6:4;		/* reserved */
	unsigned int	p1_ch_ctrl:4;	/* port 1 channel control */
	unsigned int	p1_vol:8;	/* port 1 volume */

	unsigned int	res7:4;		/* reserved */
	unsigned int	p2_ch_ctrl:4;	/* port 2 channel control */
	unsigned int	p2_vol:8;	/* port 2 volume */
	unsigned int	res8:4;		/* reserved */
	unsigned int	p3_ch_ctrl:4;	/* port 3 channel control */
	unsigned int	p3_vol:8;	/* port 3 volume */
#endif	/* _BYTE_ORDER_ */
} audio_pg_t;


/* Medium-changer Device capabilities page (0x1f) data */
typedef struct dev_capab {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	pg_code:6;	/* page code */
	unsigned int	res:2;		/* reserved */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	stor_mt:1;	/* medium transport element */
	unsigned int	stor_st:1;	/* storage element */
	unsigned int	stor_ie:1;	/* import/export element */
	unsigned int	stor_dt:1;	/* data transfer element */
	unsigned int	res1:4;		/* reserved */
	unsigned int	res2:8;		/* reserved */

	unsigned int	move_mt_mt:1;	/* move medium: MT->MT */
	unsigned int	move_mt_st:1;	/* move medium: MT->ST */
	unsigned int	move_mt_ie:1;	/* move medium: MT->IE */
	unsigned int	move_mt_dt:1;	/* move medium: MT->DT */
	unsigned int	res3:4;		/* reserved */
	unsigned int	move_st_mt:1;	/* move medium: ST->MT */
	unsigned int	move_st_st:1;	/* move medium: ST->ST */
	unsigned int	move_st_ie:1;	/* move medium: ST->IE */
	unsigned int	move_st_dt:1;	/* move medium: ST->DT */
	unsigned int	res4:4;		/* reserved */
	unsigned int	move_ie_mt:1;	/* move medium: IE->MT */
	unsigned int	move_ie_st:1;	/* move medium: IE->ST */
	unsigned int	move_ie_ie:1;	/* move medium: IE->IE */
	unsigned int	move_ie_dt:1;	/* move medium: IE->DT */
	unsigned int	res5:4;		/* reserved */
	unsigned int	move_dt_mt:1;	/* move medium: DT->MT */
	unsigned int	move_dt_st:1;	/* move medium: DT->ST */
	unsigned int	move_dt_ie:1;	/* move medium: DT->IE */
	unsigned int	move_dt_dt:1;	/* move medium: DT->DT */
	unsigned int	res6:4;		/* reserved */

	unsigned int	res7:32;	/* reserved */

	unsigned int	xchg_mt_mt:1;	/* exchange medium: MT<->MT */
	unsigned int	xchg_mt_st:1;	/* exchange medium: MT<->ST */
	unsigned int	xchg_mt_ie:1;	/* exchange medium: MT<->IE */
	unsigned int	xchg_mt_dt:1;	/* exchange medium: MT<->DT */
	unsigned int	res8:4;		/* reserved */
	unsigned int	xchg_st_mt:1;	/* exchange medium: ST<->MT */
	unsigned int	xchg_st_st:1;	/* exchange medium: ST<->ST */
	unsigned int	xchg_st_ie:1;	/* exchange medium: ST<->IE */
	unsigned int	xchg_st_dt:1;	/* exchange medium: ST<->DT */
	unsigned int	res9:4;		/* reserved */
	unsigned int	xchg_ie_mt:1;	/* exchange medium: IE<->MT */
	unsigned int	xchg_ie_st:1;	/* exchange medium: IE<->ST */
	unsigned int	xchg_ie_ie:1;	/* exchange medium: IE<->IE */
	unsigned int	xchg_ie_dt:1;	/* exchange medium: IE<->DT */
	unsigned int	res10:4;	/* reserved */
	unsigned int	xchg_dt_mt:1;	/* exchange medium: DT<->MT */
	unsigned int	xchg_dt_st:1;	/* exchange medium: DT<->ST */
	unsigned int	xchg_dt_ie:1;	/* exchange medium: DT<->IE */
	unsigned int	xchg_dt_dt:1;	/* exchange medium: DT<->DT */
	unsigned int	res11:4;	/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res:2;		/* reserved */
	unsigned int	pg_code:6;	/* page code */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	res1:4;		/* reserved */
	unsigned int	stor_dt:1;	/* data transfer element */
	unsigned int	stor_ie:1;	/* import/export element */
	unsigned int	stor_st:1;	/* storage element */
	unsigned int	stor_mt:1;	/* medium transport element */
	unsigned int	res2:8;		/* reserved */

	unsigned int	res3:4;		/* reserved */
	unsigned int	move_mt_dt:1;	/* move medium: MT->DT */
	unsigned int	move_mt_ie:1;	/* move medium: MT->IE */
	unsigned int	move_mt_st:1;	/* move medium: MT->ST */
	unsigned int	move_mt_mt:1;	/* move medium: MT->MT */
	unsigned int	res4:4;		/* reserved */
	unsigned int	move_st_dt:1;	/* move medium: ST->DT */
	unsigned int	move_st_ie:1;	/* move medium: ST->IE */
	unsigned int	move_st_st:1;	/* move medium: ST->ST */
	unsigned int	move_st_mt:1;	/* move medium: ST->MT */
	unsigned int	res5:4;		/* reserved */
	unsigned int	move_ie_dt:1;	/* move medium: IE->DT */
	unsigned int	move_ie_ie:1;	/* move medium: IE->IE */
	unsigned int	move_ie_st:1;	/* move medium: IE->ST */
	unsigned int	move_ie_mt:1;	/* move medium: IE->MT */
	unsigned int	res6:4;		/* reserved */
	unsigned int	move_dt_dt:1;	/* move medium: DT->DT */
	unsigned int	move_dt_ie:1;	/* move medium: DT->IE */
	unsigned int	move_dt_st:1;	/* move medium: DT->ST */
	unsigned int	move_dt_mt:1;	/* move medium: DT->MT */

	unsigned int	res7:32;	/* reserved */

	unsigned int	res8:4;		/* reserved */
	unsigned int	xchg_mt_dt:1;	/* exchange medium: MT<->DT */
	unsigned int	xchg_mt_ie:1;	/* exchange medium: MT<->IE */
	unsigned int	xchg_mt_st:1;	/* exchange medium: MT<->ST */
	unsigned int	xchg_mt_mt:1;	/* exchange medium: MT<->MT */
	unsigned int	res9:4;		/* reserved */
	unsigned int	xchg_st_dt:1;	/* exchange medium: ST<->DT */
	unsigned int	xchg_st_ie:1;	/* exchange medium: ST<->IE */
	unsigned int	xchg_st_st:1;	/* exchange medium: ST<->ST */
	unsigned int	xchg_st_mt:1;	/* exchange medium: ST<->MT */
	unsigned int	res10:4;	/* reserved */
	unsigned int	xchg_ie_dt:1;	/* exchange medium: IE<->DT */
	unsigned int	xchg_ie_ie:1;	/* exchange medium: IE<->IE */
	unsigned int	xchg_ie_st:1;	/* exchange medium: IE<->ST */
	unsigned int	xchg_ie_mt:1;	/* exchange medium: IE<->MT */
	unsigned int	res11:4;	/* reserved */
	unsigned int	xchg_dt_dt:1;	/* exchange medium: DT<->DT */
	unsigned int	xchg_dt_ie:1;	/* exchange medium: DT<->IE */
	unsigned int	xchg_dt_st:1;	/* exchange medium: DT<->ST */
	unsigned int	xchg_dt_mt:1;	/* exchange medium: DT<->MT */
#endif	/* _BYTE_ORDER_ */
} dev_capab_t;


/* Medium-changer Element address assignments page (0x1d) data */
typedef struct elem_addr {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	pg_code:6;	/* page code */
	unsigned int	res:2;		/* reserved */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	mt_addr:16;	/* medium transport element address */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res:2;		/* reserved */
	unsigned int	pg_code:6;	/* page code */
	unsigned int	pg_len:8;	/* page length */
	unsigned int	mt_addr:16;	/* medium transport element address */
#endif	/* _BYTE_ORDER_ */

	unsigned int	mt_num:16;	/* medium transport elements */
	unsigned int	st_addr:16;	/* storage element address */

	unsigned int	st_num:16;	/* storage elements */
	unsigned int	ie_addr:16;	/* import/export element address */

	unsigned int	ie_num:16;	/* import/export elements */
	unsigned int	dt_addr:16;	/* data transfer element address */

	unsigned int	dt_num:16;	/* data transfer elements */
	unsigned int	res1:16;	/* reserved */
} elem_addr_t;


/*
 * Request Sense data
 */
typedef struct req_sense_data {
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	errcode:7;	/* error code */
	unsigned int	valid:1;	/* valid bit */
	unsigned int	segno:8;	/* segment number */
	unsigned int	key:4;		/* sense key */
	unsigned int	res:1;		/* reserved */
	unsigned int	ili:1;		/* ILI */
	unsigned int	eom:1;		/* end-of-medium */
	unsigned int	fm:1;		/* filemark */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	valid:1;	/* valid bit */
	unsigned int	errcode:7;	/* error code */
	unsigned int	segno:8;	/* segment number */
	unsigned int	fm:1;		/* filemark */
	unsigned int	eom:1;		/* end-of-medium */
	unsigned int	ili:1;		/* ILI */
	unsigned int	res:1;		/* reserved */
	unsigned int	key:4;		/* sense key */
#endif	/* _BYTE_ORDER_ */
	unsigned int	info0:8;	/* information */

	unsigned int	info1:8;	/* information */
	unsigned int	info2:8;	/* information */
	unsigned int	info3:8;	/* information */
	unsigned int 	addl_len:8;	/* additional sense length */

	unsigned int	cmd_spec0:8;	/* command specific information */
	unsigned int	cmd_spec1:8;	/* command specific information */
	unsigned int	cmd_spec2:8;	/* command specific information */
	unsigned int	cmd_spec3:8;	/* command specific information */

	unsigned int	code:8;		/* additional sense code */
	unsigned int	qual:8;		/* additional sense code qualifier */
	unsigned int	fruc:8;		/* field replaceable unit code */
	unsigned int	key_spec0:8;	/* sense-key specific */

	unsigned int	key_spec1:8;	/* sense-key specific */
	unsigned int	key_spec2:8;	/* sense-key specific */
	unsigned int	pad1:16;	/* pad for alignment */
} req_sense_data_t;


/* Structure to map request sense key code to a descriptive string */
typedef struct req_sense_keymap {
	int		key;		/* The sense key */
	char		*text;		/* The description text string */
} req_sense_keymap_t;


/*
 * Read subchannel Q data header
 */
typedef struct subq_hdr {
	byte_t		reserved;	/* reserved */
	byte_t		audio_status;	/* audio status */
	word16_t	subch_len;	/* subchannel data length */
} subq_hdr_t;


/*
 * Subchannel Q data - format 00 (All information)
 */
typedef struct subq_00 {
	unsigned int	fmt_code:8;	/* format code */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	preemph:1;	/* preemphasis */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	adr:4;		/* ADR */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	adr:4;		/* ADR */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	preemph:1;	/* preemphasis */
#endif	/* _BYTE_ORDER_ */
	unsigned int	trkno:8;	/* track number */
	unsigned int	idxno:8;	/* index number */

	lmsf_t		abs_addr;	/* absolute address */
	lmsf_t		rel_addr;	/* track-relative address */

#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	reserved:7;	/* reserved */
	unsigned int	mcval:1;	/* MCN is valid */
#else
	unsigned int	mcval:1;	/* MCN is valid */
	unsigned int	reserved:7;	/* reserved */
#endif
	byte_t		mcn[15];	/* MCN data */

#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	reserved2:7;	/* reserved */
	unsigned int	tcval:1;	/* ISRC is valid */
#else
	unsigned int	tcval:1;	/* ISRC is valid */
	unsigned int	reserved2:7;	/* reserved */
#endif
	byte_t		isrc[15];	/* ISRC data */
} subq_00_t;


/*
 * Subchannel Q data - format 01 (CD-ROM Current Position)
 */
typedef struct subq_01 {
	unsigned int	fmt_code:8;	/* format code */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	preemph:1;	/* preemphasis */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	adr:4;		/* ADR */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	adr:4;		/* ADR */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	preemph:1;	/* preemphasis */
#endif	/* _BYTE_ORDER_ */
	unsigned int	trkno:8;	/* track number */
	unsigned int	idxno:8;	/* index number */

	lmsf_t		abs_addr;	/* absolute address */
	lmsf_t		rel_addr;	/* track-relative address */
} subq_01_t;


/*
 * Subchannel Q data - format 02 (Media Catalog Number)
 */
typedef struct subq_02 {
	unsigned int	fmt_code:8;	/* format code */
	unsigned int	reserved:24;	/* reserved */		

#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	reserved2:7;	/* reserved */
	unsigned int	mcval:1;	/* MCN is valid */
#else
	unsigned int	mcval:1;	/* MCN is valid */
	unsigned int	reserved2:7;	/* reserved */
#endif
	byte_t		mcn[15];	/* MCN data */
} subq_02_t;


/*
 * Subchannel Q data - format 03 (International Standard Recording Code)
 */
typedef struct subq_03 {
	unsigned int	fmt_code:8;	/* format code */
	unsigned int	reserved:24;	/* reserved */		

#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	reserved2:7;	/* reserved */
	unsigned int	tcval:1;	/* ISRC is valid */
#else
	unsigned int	tcval:1;	/* ISRC is valid */
	unsigned int	reserved2:7;	/* reserved */
#endif
	byte_t		isrc[15];	/* ISRC data */
} subq_03_t;


/*
 * Read TOC command data header
 */
typedef struct toc_hdr {
	word16_t	data_len;	/* TOC data length */
	byte_t		first_trk;	/* first track number */
	byte_t		last_trk;	/* last track number */
} toc_hdr_t;


/*
 * Read TOC command track descriptor
 */
typedef struct toc_trk_descr {
	unsigned int	res1:8;		/* reserved */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	preemph:1;	/* preemphasis */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	adr:4;		/* ADR */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	adr:4;		/* ADR */
	unsigned int	audioch:1;	/* 0=2ch 1=4ch */
	unsigned int	trktype:1;	/* 0=audio 1=data */
	unsigned int	copyallow:1;	/* digital copy allow */
	unsigned int	preemph:1;	/* preemphasis */
#endif	/* _BYTE_ORDER_ */
	unsigned int	trkno:8;	/* track number */
	unsigned int	res2:8;		/* reserved */

	lmsf_t		abs_addr;	/* absolute address */
} toc_trk_descr_t;


/* Values for the pack_type field of the toc_cdtext_pack_t structure */
#define PACK_TITLE	0x80		/* Album or track title */
#define PACK_PERFORMER	0x81		/* Name of performer(s) */
#define PACK_SONGWRITER	0x82		/* Name of songwriter(s) */
#define PACK_COMPOSER	0x83		/* Name of composer(s) */
#define PACK_ARRANGER	0x84		/* Name of arranger(s) */
#define PACK_MESSAGE	0x85		/* Message(s) to user */
#define PACK_IDENT	0x86		/* Disc identification info */
#define PACK_GENRE	0x87		/* Genre identification and info */
#define PACK_TOC	0x88		/* TOC info */
#define PACK_TOC2	0x89		/* Second TOC info */
#define PACK_UPCEAN	0x8e		/* UPC/EAN code, ISRC code (track) */
#define PACK_SIZEINFO	0x8f		/* Size info of the block */

/*
 * Read TOC command CD-TEXT pack descriptor
 */
typedef struct toc_cdtext_pack {
	byte_t		pack_type;	/* Pack type */
	byte_t		trk_no;		/* Track Number */
	byte_t		seq_no;		/* Sequence number */
	byte_t		blk_char;	/* Block character */
	byte_t		data[SZ_CDTEXTPKD];
					/* Text data */
	byte_t		crc0;		/* CRC info */
	byte_t		crc1;		/* CRC info */
} toc_cdtext_pack_t;


/*
 * Medium changer Read Element Status data structures
 */

/* Element status header */
typedef struct {
	unsigned int	firstelem:16;	/* first elem addr reported */
	unsigned int	numelem:16;	/* number of elems reported */

	unsigned int	datalen:32;	/* report byte cnt max=0xffffff */
} elemhdr_t;


/* Element types */
#define ELEMTYP_ALL	0		/* all types */
#define ELEMTYP_MT	1		/* medium transport */
#define ELEMTYP_ST	2		/* storage */
#define ELEMTYP_IE	3		/* import export */
#define ELEMTYP_DT	4		/* data transfer */


/* Element status page */
typedef struct {
	unsigned int	type:8;		/* elem type code */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	res1:6;		/* reserved */
	unsigned int	avoltag:1;	/* avoltag */
	unsigned int	pvoltag:1;	/* pvoltag */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	pvoltag:1;	/* pvoltag */
	unsigned int	avoltag:1;	/* avoltag */
	unsigned int	res1:6;		/* reserved */
#endif	/* _BYTE_ORDER_ */
	unsigned int	desclen:16;	/* elem descriptor length each */

	unsigned int	pagelen:32;	/* descriptors byte cnt max=0xffffff */
} elemstat_t;


/* Medium transport element descriptor */
typedef struct {
	unsigned int	elemaddr:16;	/* element address */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	full:1;		/* full */
	unsigned int	res2:1;		/* reserved */
	unsigned int	excpt:1;	/* except */
	unsigned int	res1:5;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res1:5;		/* reserved */
	unsigned int	excpt:1;	/* except */
	unsigned int	res2:1;		/* reserved */
	unsigned int	full:1;		/* full */
#endif	/* _BYTE_ORDER_ */
	unsigned int	res3:8;		/* reserved */

	unsigned int	asc:8;		/* additional sense code */
	unsigned int	ascq:8;		/* additional sense code qualifier */
	unsigned int	res4:16;	/* reserved */

	unsigned int	res5:8;		/* reserved */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	res6:6;		/* reserved */
	unsigned int	invert:1;	/* invert */
	unsigned int	svalid:1;	/* svalid */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	svalid:1;	/* svalid */
	unsigned int	invert:1;	/* invert */
	unsigned int	res6:6;		/* reserved */
#endif	/* _BYTE_ORDER_ */
	unsigned int	srcaddr:16;	/* source storage elem address */

	byte_t		pri_voltag[35];	/* primary volume tag info */
	byte_t		alt_voltag[15];	/* alternate volume tag info */
} elemdesc_mt_t;


/* Storage element descriptor */
typedef struct {
	unsigned int	elemaddr:16;	/* element address */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	full:1;		/* full */
	unsigned int	res2:1;		/* reserved */
	unsigned int	excpt:1;	/* except */
	unsigned int	access:1;	/* access */
	unsigned int	res1:4;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res1:4;		/* reserved */
	unsigned int	access:1;	/* access */
	unsigned int	excpt:1;	/* except */
	unsigned int	res2:1;		/* reserved */
	unsigned int	full:1;		/* full */
#endif	/* _BYTE_ORDER_ */
	unsigned int	res3:8;		/* reserved */

	unsigned int	asc:8;		/* additional sense code */
	unsigned int	ascq:8;		/* additional sense code qualifier */
	unsigned int	res4:16;	/* reserved */

	unsigned int	res5:8;		/* reserved */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	res6:6;		/* reserved */
	unsigned int	invert:1;	/* invert */
	unsigned int	svalid:1;	/* svalid */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	svalid:1;	/* svalid */
	unsigned int	invert:1;	/* invert */
	unsigned int	res6:6;		/* reserved */
#endif	/* _BYTE_ORDER_ */
	unsigned int	srcaddr:16;	/* source storage elem address */

	byte_t		pri_voltag[35];	/* primary volume tag info */
	byte_t		alt_voltag[15];	/* alternate volume tag info */
} elemdesc_st_t;


/* Import/export element descriptor */
typedef struct {
	unsigned int	elemaddr:16;	/* element address */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	full:1;		/* full */
	unsigned int	impexp:1;	/* impexp */
	unsigned int	excpt:1;	/* except */
	unsigned int	access:1;	/* access */
	unsigned int	exenab:1;	/* exenab */
	unsigned int	inenab:1;	/* inenab */
	unsigned int	res1:2;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res1:2;		/* reserved */
	unsigned int	inenab:1;	/* inenab */
	unsigned int	exenab:1;	/* exenab */
	unsigned int	access:1;	/* access */
	unsigned int	excpt:1;	/* except */
	unsigned int	impexp:1;	/* impexp */
	unsigned int	full:1;		/* full */
#endif	/* _BYTE_ORDER_ */
	unsigned int	res3:8;		/* reserved */

	unsigned int	asc:8;		/* additional sense code */
	unsigned int	ascq:8;		/* additional sense code qualifier */
	unsigned int	res4:16;	/* reserved */

	unsigned int	res5:8;		/* reserved */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	res6:6;		/* reserved */
	unsigned int	invert:1;	/* invert */
	unsigned int	svalid:1;	/* svalid */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	svalid:1;	/* svalid */
	unsigned int	invert:1;	/* invert */
	unsigned int	res6:6;		/* reserved */
#endif	/* _BYTE_ORDER_ */
	unsigned int	srcaddr:16;	/* source storage elem address */

	byte_t		pri_voltag[35];	/* primary volume tag info */
	byte_t		alt_voltag[15];	/* alternate volume tag info */
} elemdesc_ie_t;


/* Data transfer element descriptor */
typedef struct {
	unsigned int	elemaddr:16;	/* element address */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	full:1;		/* full */
	unsigned int	res2:1;		/* reserved */
	unsigned int	excpt:1;	/* except */
	unsigned int	access:1;	/* access */
	unsigned int	res1:4;		/* reserved */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	res1:4;		/* reserved */
	unsigned int	access:1;	/* access */
	unsigned int	excpt:1;	/* except */
	unsigned int	res2:1;		/* reserved */
	unsigned int	full:1;		/* full */
#endif	/* _BYTE_ORDER_ */
	unsigned int	res3:8;		/* reserved */

	unsigned int	asc:8;		/* additional sense code */
	unsigned int	ascq:8;		/* additional sense code qualifier */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	lun:3;		/* lun */
	unsigned int	res5:1;		/* reserved */
	unsigned int	luvalid:1;	/* luvalid */
	unsigned int	idvalid:1;	/* idvalid */
	unsigned int	res6:1;		/* reserved */
	unsigned int	notbus:1;	/* notbus */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	notbus:1;	/* notbus */
	unsigned int	res6:1;		/* reserved */
	unsigned int	idvalid:1;	/* idvalid */
	unsigned int	luvalid:1;	/* luvalid */
	unsigned int	res5:1;		/* reserved */
	unsigned int	lun:3;		/* lun */
#endif	/* _BYTE_ORDER_ */
	unsigned int	busaddr:8;	/* scsi bus addr */

	unsigned int	res7:8;		/* reserved */
#if _BYTE_ORDER_ == _L_ENDIAN_
	unsigned int	res8:6;		/* reserved */
	unsigned int	invert:1;	/* invert */
	unsigned int	svalid:1;	/* svalid */
#else	/* _BYTE_ORDER_ == _B_ENDIAN_ */
	unsigned int	svalid:1;	/* svalid */
	unsigned int	invert:1;	/* invert */
	unsigned int	res8:6;		/* reserved */
#endif	/* _BYTE_ORDER_ */
	unsigned int	srcaddr:16;	/* source storage elem address */

	byte_t		pri_voltag[35];	/* primary volume tag info */
	byte_t		alt_voltag[15];	/* alternate volume tag info */
} elemdesc_dt_t;


/* Union of all element descriptors */
typedef union {
	elemdesc_mt_t	mt;		/* medium transport elem descriptor */
	elemdesc_st_t	st;		/* storage elem descriptor */
	elemdesc_ie_t	ie;		/* import/export elem descriptor */
	elemdesc_dt_t	dt;		/* data transfer elem descriptor */
} elemdesc_t;


/*
 * Medium changer parameters structure
 */
typedef struct {
	word16_t	mtbase;		/* Medium transport element base addr */
	word16_t	stbase;		/* Storage element base addr */
	word16_t	iebase;		/* Import/export element base addr */
	word16_t	dtbase;		/* Data transfer element base addr */
} mcparm_t;


/* Defines for the addrfmt field of the cdda_req_t structure */
#define READFMT_LBA	0x1
#define READFMT_MSF	0x2

/*
 * Read CDDA request structure
 */
typedef struct cdda_req {
	lmsf_t		pos;		/* Address location to read */
	int		nframes;	/* Number of frames to read */
	byte_t		*buf;		/* Data buffer pointer */
	byte_t		addrfmt;	/* Address format */
} cdda_req_t;



/***** Additional include files *****/

/* OS interface library headers */
#include "libdi_d/os_aix.h"		/* IBM AIX */
#include "libdi_d/os_aux.h"		/* Apple A/UX */
#include "libdi_d/os_bsdi.h"		/* BSDI BSD/OS */
#include "libdi_d/os_dec.h"		/* Digital OSF/1 & Ultrix */
#include "libdi_d/os_dgux.h"		/* Data General DG/UX */
#include "libdi_d/os_fnbsd.h"		/* FreeBSD/NetBSD */
#include "libdi_d/os_hpux.h"		/* HP-UX */
#include "libdi_d/os_irix.h"		/* SGI IRIX */
#include "libdi_d/os_linux.h"		/* Linux */
#include "libdi_d/os_news.h"		/* Sony NEWS-OS */
#include "libdi_d/os_sco.h"		/* SCO UNIX/Open Desktop/Open Server */
#include "libdi_d/os_sinix.h"		/* SNI SINIX */
#include "libdi_d/os_sun.h"		/* SunOS */
#include "libdi_d/os_svr4.h"		/* SVR4 */
#include "libdi_d/os_vms.h"		/* Digital OpenVMS */

/* If compiling on a non-supported OS, force demo-only mode */
#if !defined(OS_MODULE) && !defined(DEMO_ONLY)
#define DEMO_ONLY
#endif

/* If demo-only, no need to compile in the vendor-unique modules */
#ifdef DEMO_ONLY
#ifdef VENDOR_CHINON
#undef VENDOR_CHINON
#endif
#ifdef VENDOR_HITACHI
#undef VENDOR_HITACHI
#endif
#ifdef VENDOR_NEC
#undef VENDOR_NEC
#endif
#ifdef VENDOR_PIONEER
#undef VENDOR_PIONEER
#endif
#ifdef VENDOR_SONY
#undef VENDOR_SONY
#endif
#ifdef VENDOR_TOSHIBA
#undef VENDOR_TOSHIBA
#endif
#ifdef VENDOR_PANASONIC
#undef VENDOR_PANASONIC
#endif
#endif	/* DEMO_ONLY */

/* Demo mode support header */
#include "libdi_d/os_demo.h"

/* Vendor-unique library headers */
#include "libdi_d/vu_chin.h"		/* Chinon vendor-unique header */
#include "libdi_d/vu_hita.h"		/* Hitachi vendor-unique header */
#include "libdi_d/vu_nec.h"		/* NEC vendor-unique header */
#include "libdi_d/vu_pana.h"		/* Panasonic vendor-unique header */
#include "libdi_d/vu_pion.h"		/* Pioneer vendor-unique header */
#include "libdi_d/vu_sony.h"		/* Sony vendor-unique header */
#include "libdi_d/vu_tosh.h"		/* Toshiba vendor-unique header */

/*
 * Functions for vendor-unique module use only
 */
extern bool_t	scsipt_rezero_unit(di_dev_t *, int);
extern bool_t	scsipt_tst_unit_rdy(di_dev_t *, int, req_sense_data_t *,
				    bool_t);
extern bool_t	scsipt_request_sense(di_dev_t *, int, byte_t *, size_t);
extern bool_t	scsipt_rdsubq(di_dev_t *, int, byte_t, byte_t, cdstat_t *,
			      char *, char *, bool_t);
extern bool_t	scsipt_modesense(di_dev_t *, int, byte_t *, byte_t,
				 byte_t, size_t);
extern bool_t	scsipt_modesel(di_dev_t *, int, byte_t *, byte_t, size_t);
extern bool_t	scsipt_inquiry(di_dev_t *, int, byte_t *, size_t);
extern bool_t	scsipt_rdtoc(di_dev_t *, int, byte_t *, size_t, int, int,
			     bool_t, bool_t);
extern bool_t	scsipt_playmsf(di_dev_t *, int, msf_t *, msf_t *);
extern bool_t	scsipt_play10(di_dev_t *, int, sword32_t, sword32_t);
extern bool_t	scsipt_play12(di_dev_t *, int, sword32_t, sword32_t);
extern bool_t	scsipt_play_trkidx(di_dev_t *, int, int, int, int, int);
extern bool_t	scsipt_read_cdda(di_dev_t *, int, cdda_req_t *, size_t, int,
				 req_sense_data_t *);
extern bool_t	scsipt_prev_allow(di_dev_t *, int, bool_t);
extern bool_t	scsipt_start_stop(di_dev_t *, int, bool_t, bool_t, bool_t);
extern bool_t	scsipt_pause_resume(di_dev_t *, int, bool_t);
extern bool_t	scsipt_move_medium(di_dev_t *, int,
				   word16_t, word16_t, word16_t);
extern bool_t	scsipt_init_elemstat(di_dev_t *, int);
extern bool_t	scsipt_read_elemstat(di_dev_t *, int, byte_t *, size_t, int);
extern bool_t	scsipt_disc_present(di_dev_t *, int, curstat_t *,
				    req_sense_data_t *);

#endif	/* DI_SCSIPT CDDA_RD_SCSIPT */

#ifdef DI_SCSIPT

/*
 * Vendor-unique module initialization jump table
 */
typedef struct {
	void		(*init)(void);	/* VU init function */
} vuinit_tbl_t;


/*
 * Vendor-unique module entry jump table
 */
typedef struct {
	/* Vendor name string */
	char		*vendor;

	/* Play audio function */
	bool_t		(*playaudio)(byte_t, sword32_t, sword32_t,
				     msf_t *, msf_t *, byte_t, byte_t);

	/* Pause/resume function */
	bool_t		(*pause_resume)(bool_t);

	/* Start/stop function */
	bool_t		(*start_stop)(bool_t, bool_t);

	/* Playback status function */
	bool_t		(*get_playstatus)(cdstat_t *);

	/* Playback volume function */
	int		(*volume)(int, curstat_t *, bool_t);

	/* Channel routing function */
	bool_t		(*route)(curstat_t *);

	/* Playback mute function */
	bool_t		(*mute)(bool_t);

	/* Read TOC function */
	bool_t		(*get_toc)(curstat_t *);

	/* Eject function */
	bool_t		(*eject)(void);

	/* Module start function */
	void		(*start)(void);

	/* Module halt function */
	void		(*halt)(void);
} vu_tbl_t;


/*
 * Public functions
 */
extern char	*scsipt_reqsense_keystr(int);
extern void	scsipt_enable(di_dev_t *, int);
extern void	scsipt_disable(di_dev_t *, int);
extern bool_t	scsipt_is_enabled(di_dev_t *, int);
extern void	scsipt_init(curstat_t *, di_tbl_t *);
extern void	scsipt_load_cdtext(curstat_t *, di_cdtext_t *);
extern bool_t	scsipt_playmode(curstat_t *);
extern bool_t	scsipt_check_disc(curstat_t *);
extern void	scsipt_status_upd(curstat_t *);
extern void	scsipt_lock(curstat_t *, bool_t);
extern void	scsipt_repeat(curstat_t *, bool_t);
extern void	scsipt_shuffle(curstat_t *, bool_t);
extern void	scsipt_load_eject(curstat_t *);
extern void	scsipt_ab(curstat_t *);
extern void	scsipt_sample(curstat_t *);
extern void	scsipt_level(curstat_t *, byte_t, bool_t);
extern void	scsipt_play_pause(curstat_t *);
extern void	scsipt_stop(curstat_t *, bool_t);
extern void	scsipt_chgdisc(curstat_t *);
extern void	scsipt_prevtrk(curstat_t *);
extern void	scsipt_nexttrk(curstat_t *);
extern void	scsipt_previdx(curstat_t *);
extern void	scsipt_nextidx(curstat_t *);
extern void	scsipt_rew(curstat_t *, bool_t);
extern void	scsipt_ff(curstat_t *, bool_t);
extern void	scsipt_warp(curstat_t *);
extern void	scsipt_route(curstat_t *);
extern void	scsipt_mute_on(curstat_t *);
extern void	scsipt_mute_off(curstat_t *);
extern void	scsipt_cddajitter(curstat_t *);
extern void	scsipt_debug(void);
extern void	scsipt_start(curstat_t *);
extern void	scsipt_icon(curstat_t *, bool_t);
extern void	scsipt_halt(curstat_t *);
extern char	*scsipt_methodstr(void);

#else

#define scsipt_init	NULL

#endif	/* DI_SCSIPT */

#endif	/* __SCSIPT_H__ */

