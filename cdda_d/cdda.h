/*
 *   cdda - CD Digital Audio support
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
#ifndef __CDDA_H__
#define __CDDA_H__

#ifndef lint
static char *_cdda_h_ident_ = "@(#)cdda.h	7.114 04/03/05";
#endif


/*
 * Whether to enable the SYSV IPC and/or the POSIX threads support
 * based on OS feature capability.
 */
#if defined(SVR4) || defined(SYSV) || defined(_LINUX) || \
    defined(_SUNOS4) || defined(_SOLARIS) || defined(sgi) || \
    defined(__hpux) || defined(_AIX) || \
    defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || \
    defined(__bsdi__) || defined(_OSF1)
#define HAS_SYSVIPC
#endif

#if defined(_POSIX_THREADS) && !defined(NO_PTHREADS)
/* If <unistd.h> defines _POSIX_THREADS, then POSIX 1003.1c threads
 * are supported.  NO_PTHREADS allows a user to disable POSIX thread
 * support code from the makefile.
 */
#define HAS_PTHREADS
#endif

#ifdef HAS_PTHREADS
/* Hack: Some platforms lack the necessary pthread_cancel(3) */
#include <pthread.h>
#ifndef PTHREAD_CANCELED
#undef HAS_PTHREADS
#endif
#endif


/*
 * CDDA methods
 */
#define CDDA_METHODS	3	/* Total number of CDDA methods */

#define CDDA_NONE	0	/* Don't use CDDA method */
#ifdef HAS_SYSVIPC
#define CDDA_SYSVIPC	1	/* SYSV IPC method */
#endif
#ifdef HAS_PTHREADS
#define CDDA_PTHREADS	2	/* POSIX threads method */
#endif


/*
 * CDDA read methods
 */
#define CDDA_RD_METHODS	6	/* Total number of CDDA read methods */

#define CDDA_RD_NONE	0	/* Don't use CDDA read method */
#define CDDA_RD_SCSIPT	1	/* SCSI pass-through method */
#ifdef _SOLARIS
#define CDDA_RD_SOL	2	/* Solaris ioctl method */
#endif
#ifdef _LINUX
#define CDDA_RD_LINUX	3	/* Linux ioctl method */
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define CDDA_RD_FBSD	4	/* FreeBSD/OpenBSD/NetBSD ioctl method */
#endif
#if defined(_AIX) && defined(AIX_IDE)
#define CDDA_RD_AIX	5	/* AIX ATAPI/IDE ioctl method */
#endif


/*
 * CDDA write methods
 */
#define CDDA_WR_METHODS	9	/* Total number of CDDA write methods */

#define CDDA_WR_NONE	0	/* Don't use CDDA write method */
#ifndef __VMS
#define CDDA_WR_OSS	1	/* OSS & Linux sound driver method */
#endif
#ifdef _SOLARIS
#define CDDA_WR_SOL	2	/* Solaris audio driver method */
#endif
#ifdef sgi
#define CDDA_WR_IRIX	3	/* IRIX audio driver method */
#endif
#ifdef __hpux
#define CDDA_WR_HPUX	4	/* HP-UX audio driver method */
#endif
#ifdef _AIX
#define CDDA_WR_AIX	5	/* AIX Ultimedia audio driver method */
#endif
#if defined(_LINUX) && defined(HAS_ALSA)
#define CDDA_WR_ALSA	6	/* ALSA sound driver method */
#endif
#if defined(_OSF1) || defined(__VMS)
#define CDDA_WR_OSF1	7	/* Tru64 UNIX and OpenVMS MME method */
#endif
#define CDDA_WR_FP	8	/* File & pipe method: No audio playback */


/* Whether a CDDA method is configured */
#if defined(CDDA_SYSVIPC) || defined(CDDA_PTHREADS)
#define CDDA_SUPPORTED
#endif


/* Check for ALSA version due to API changes */
#ifdef CDDA_WR_ALSA
#include <alsa/asoundlib.h>
#if !defined(SND_LIB_MAJOR) || !defined(SND_LIB_MINOR) || \
    (SND_LIB_MAJOR == 0 && SND_LIB_MINOR < 9)
#undef CDDA_WR_ALSA		/* ALSA version prior to 0.9.0 not supported */
#endif
#endif	/* CDDA_WR_ALSA */


/* Basic CDDA read unit (number of bytes per CDDA sector) */
#define	CDDA_BLKSZ		2352

/* Default CDDA read size: Note that different platforms' CDROM drivers
 * may impose different limits which require that this be externally
 * tuned via the cddaReadChunkBlocks parameter.
 */
#define	DEF_CDDA_CHUNK_BLKS	8			/* 8 blocks */
#define MIN_CDDA_CHUNK_BLKS	1			/* 1 block */
#define MAX_CDDA_CHUNK_BLKS	(3 * FRAME_PER_SEC)	/* 3 seconds */

/* CDDA data is queued up in memory. Here we define (in
 * seconds) the amount of queued data. This is not a
 * modifiable resource.
 */
#define	DEF_CDDA_BUFFER_SECS	5

/* During jitter correction we search through DEFAULT_SEARCH_BLOCKS
 * for a match.  This is not a modifiable resource.
 */
#define	DEF_CDDA_SRCH_BLKS	2

/* In order to perform jitter correction we need to
 * read more CDDA data than we "need". Below we define
 * the amount of extra data required. So each CDDA
 * read will comprise CHUNK_BLOCKS + OLAP_BLOCKS.
 */
#define	DEF_CDDA_OLAP_BLKS	(DEF_CDDA_SRCH_BLKS * 2)

/* We have found a match when DEFAULT_STR_LENGTH bytes are
 * identical in the data and the overlap.
 */
#define	DEF_CDDA_STR_LEN	20

/* Maximum number of interrupts before system call fails */
#define	CDDA_INTR_MAX		200

/* Maximum number of times to retry a failed CDDA read */
#define MAX_CDDAREAD_RETRIES	3

/* CDDA heartbeat types */
#define CDDA_HB_READER		1		/* Reader */
#define CDDA_HB_WRITER		2		/* Writer */

/* Reader/writer heartbeat timeout values in seconds */
#define MIN_HB_TIMEOUT		5		/* Minimum */
#define DEF_HB_TIMEOUT		15		/* Default */

/* Number of heartbeat checks to skip after a pause/resume operation,
 * in order to allow the reader and writer threads to catch up.
 */
#define CDDA_HB_SKIP		5

/* CDDA supported features bitmask values, returned by cdda_capab()
 * These should match the similar CAPAB_xxx defines in libdi_d/libdi.h
 */
#define CDDA_READAUDIO		0x02
#define CDDA_WRITEDEV		0x10
#define CDDA_WRITEFILE		0x20
#define CDDA_WRITEPIPE		0x40

/* CDDA audio output port bitmask values */
#define CDDA_OUT_SPEAKER	0x01		/* Internal speaker */
#define CDDA_OUT_HEADPHONE	0x02		/* Headphone */
#define CDDA_OUT_LINEOUT	0x04		/* Line Out */

/* Read and write process priority flags */
#define CDDA_RDPRI		0x01		/* Elevate read priority */
#define CDDA_WRPRI		0x02		/* Elevate write priority */

/* Macros for encoder version storage */
#define ENCVER_MK(x,y,z)	(((x) * 1000000) + ((y) * 1000) + (z))
#define ENCVER_MAJ(v)		((v) / 1000000)
#define ENCVER_MIN(v)		(((v) / 1000) % 1000)
#define ENCVER_TINY(v)		((v) % 1000)


/* Call table structure for branching into a read-method. */
typedef struct cdda_rd_tbl {
	word32_t 	(*readinit)(void);
	bool_t	 	(*readfunc)(di_dev_t *);
	void	 	(*readdone)(bool_t);
	void	 	(*readinfo)(char *);
} cdda_rd_tbl_t;


/* Call table structure for branching into a write-method. */
typedef struct cdda_wr_tbl {
	word32_t 	(*writeinit)(void);
	bool_t	 	(*writefunc)(curstat_t *);
	void	 	(*writedone)(bool_t);
	void	 	(*writeinfo)(char *);
} cdda_wr_tbl_t;


/* Generic thread or process ID type */
typedef void *		thid_t;


/* Buffer for audio data transfer */
typedef struct cd_buffer {
	int		occupied;
	unsigned int	nextin;
	unsigned int	nextout;
	byte_t		*olap;
	byte_t		*data;
	byte_t		*b;
} cd_buffer_t;


/* Various sizes */
typedef struct cd_size {
	/* Chunk */
	unsigned int	chunk_blocks;
	unsigned int	chunk_bytes;

	/* Buffer */
	unsigned int	buffer_chunks;
	unsigned int	buffer_blocks;
	unsigned int	buffer_bytes;

	/* Overlap */
	unsigned int	olap_blocks;
	unsigned int	olap_bytes;

	/* Searching */
	unsigned int	search_blocks;
	unsigned int	search_bytes;
	unsigned int	str_length;

	/* Overall size */
	size_t		size;
} cd_size_t;


/* CD info */
typedef struct cd_info {
	int		state;			/* CDSTAT_PLAYING, etc. */
	int		chroute;		/* CHROUTE_NORMAL etc. */
	int		att;			/* CDDA attenuator value */
	int		outport;		/* CDDA output port */
	int		vol_taper;		/* Volume control taper */
	int		vol;			/* Current volume level */
	int		vol_left;		/* Current left vol % */
	int		vol_right;		/* Current right vol % */
	int		jitter;			/* 1 = on, 0 = off */
	int		cdda_done;		/* 1 = yes, 0 = no */
	word32_t	debug;			/* Debug level */

	sword32_t	start_lba;		/* Start position */
	sword32_t	end_lba;		/* End position */
	word32_t	frm_played;		/* Frames written */
	word32_t	frm_per_sec;		/* Frames per sec stats */
	size_t		trk_len;		/* Track length in bytes */
	int		trk_idx;		/* Current track array index */

	thid_t		reader;			/* Reader thread id */
	thid_t		writer;			/* Writer thread id */

	time_t		reader_hb;		/* Reader heartbeat */
	time_t		writer_hb;		/* Writer heartbeat */
	int		reader_hb_timeout;	/* reader heartbeat timeout */
	int		writer_hb_timeout;	/* writer heartbeat timeout */

	char		msgbuf[ERR_BUF_SZ];	/* Message buffer */
} cd_info_t;


/* CD state structure - pointers to the size, info and buffer areas */
typedef struct cd_state {
	cd_size_t	*cds;			/* Sizes */
	cd_info_t	*i;			/* Info */
	cd_buffer_t	*cdb;			/* Data buffer */
} cd_state_t ;


/* Message dialog macros */
#define CDDA_FATAL(msg)		{					\
	if (cdda_clinfo != NULL && cdda_clinfo->fatal_msg != NULL)	\
		cdda_clinfo->fatal_msg(app_data.str_fatal, (msg));	\
	else {								\
		(void) fprintf(errfp, "Fatal: %s\n", (msg));		\
		_exit(1);						\
	}								\
}
#define CDDA_WARNING(msg)	{					\
	if (cdda_clinfo != NULL && cdda_clinfo->warning_msg != NULL)	\
		cdda_clinfo->warning_msg(app_data.str_warning, (msg));	\
	else								\
		(void) fprintf(errfp, "Warning: %s\n", (msg));		\
}
#define CDDA_INFO(msg)		{					\
	if (cdda_clinfo != NULL && cdda_clinfo->info_msg != NULL)	\
		cdda_clinfo->info_msg(app_data.str_info, (msg));	\
	else								\
		(void) fprintf(errfp, "Info: %s\n", (msg));		\
}
#define CDDA_INFO2(msg, txt)	{					\
	if (cdda_clinfo != NULL && cdda_clinfo->info2_msg != NULL)	\
		cdda_clinfo->info2_msg(app_data.str_info,(msg),(txt));	\
	else								\
		(void) fprintf(errfp, "Info: %s\n%s\n", (msg), (txt));	\
}


/* Client registration structure */
typedef struct cdda_client {
	curstat_t *	(*curstat_addr)(void);
	void		(*fatal_msg)(char *, char *);
	void		(*warning_msg)(char *, char *);
	void		(*info_msg)(char *, char *);
	void		(*info2_msg)(char *, char *, char *);
} cdda_client_t;


/* File format description structure */
typedef struct filefmt {
	int		fmt;			/* Format code */
	char		*name;			/* Format name */
	char		*suf;			/* File Suffix */
	char		*desc;			/* Description */
} filefmt_t;


/* Function prototypes */
extern word32_t		cdda_capab(void);
extern void		cdda_preinit(void);
extern bool_t		cdda_init(curstat_t *, cdda_client_t *);
extern void		cdda_halt(di_dev_t *, curstat_t *);
extern bool_t		cdda_play(di_dev_t *, curstat_t *, sword32_t,
				  sword32_t);
extern bool_t		cdda_pause_resume(di_dev_t *, curstat_t *, bool_t);
extern bool_t		cdda_stop(di_dev_t *, curstat_t *);
extern int		cdda_vol(di_dev_t *, curstat_t *, int, bool_t);
extern bool_t		cdda_chroute(di_dev_t *, curstat_t *);
extern void		cdda_att(curstat_t *);
extern void		cdda_outport(void);
extern bool_t		cdda_getstatus(di_dev_t *, curstat_t *, cdstat_t *);
extern void		cdda_debug(word32_t);
extern char		*cdda_info(void);
extern int		cdda_initipc(cd_state_t *);
extern void		cdda_waitsem(int, int);
extern void		cdda_postsem(int, int);
extern void		cdda_yield(void);
extern void		cdda_kill(thid_t, int);
extern bool_t		cdda_filefmt_supp(int);
extern filefmt_t	*cdda_filefmt(int);
extern int		cdda_bitrates(void);
extern int		cdda_bitrate_val(int);
extern char		*cdda_bitrate_name(int);
extern int		cdda_heartbeat_interval(int);
extern void		cdda_heartbeat(time_t *, int);

#endif	/* __CDDA_H__ */

