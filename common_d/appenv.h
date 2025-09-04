/*
 *   appenv - Common header file for xmcd, cda and libdi.
 *
 *	xmcd  - Motif(R) CD Audio Player/Ripper
 *	cda   - Command-line CD Audio Player/Ripper
 *	libdi - CD Audio Device Interface Library
 *
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
 *
 */
#ifndef __APPENV_H__
#define __APPENV_H__

#ifndef lint
static char *_appenv_h_ident_ = "@(#)appenv.h	7.183 04/04/20";
#endif


/*
 * Whether STATIC should really be...
 */
#ifdef DEBUG
#define STATIC
#else
#define STATIC		static
#endif


/*
 * Basic constants
 */
#define STR_BUF_SZ	64			/* Generic string buf size */
#define ERR_BUF_SZ	512			/* Generic errmsg buf size */
#define FILE_PATH_SZ	256			/* Max file path length */
#define FILE_BASE_SZ	64			/* Max base name length */
#define HOST_NAM_SZ	64			/* Max host name length */
#define MAXTRACK	100			/* Max number of tracks */
#define LEAD_OUT_TRACK	0xaa			/* Lead-out track number */
#define STD_CDROM_BLKSZ	2048			/* Standard CD-ROM blocksize */
#define FRAME_PER_SEC	75			/* Frames per second */
#define MSF_OFFSET	150			/* Starting MSF offset */
#define MAX_VOL		100			/* Max logical audio volume */


/*
 * Common header files to be included for all modules
 */

#ifndef __VMS
/* UNIX */

/*
 * General header files
 */
#include <sys/types.h>
#include <sys/param.h>

#ifndef NO_STDLIB_H
#include <stdlib.h>
#endif
#ifndef NO_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <memory.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

/*
 * ----- Platform identification -----
 * The following section consolidates complex platform identification
 * into one place.
 */

/* Indentify a SunOS 4.x or SunOS 5.x (Solaris) platform */
#if defined(sun) || defined(__sun__)
#ifdef SVR4
#define _SOLARIS
#else
#define _SUNOS4
#endif
#endif

/* Identify a Linux platform */
#if defined(linux) || defined(__linux__)
#define _LINUX
#endif

/* Identify a Digital/Compaq/HP (Tru64) UNIX/OSF1 platform */
#if defined(__alpha) && defined(__osf__)
#define _OSF1
#endif

/* Identify a Digital Ultrix platform */
#if defined(ultrix) || defined(__ultrix)
#define _ULTRIX
#endif

/* Identify an SCO UNIX/Open Desktop/Open Server platform */
#if defined(sco) || defined(SCO) || defined(M_UNIX) || defined(_M_UNIX)
#define _SCO_SVR3
#endif

/* Identify a UnixWare or Caldera Open UNIX platform */
#if (defined(SVR4) || defined(__svr4__) || \
     defined(SVR5) || defined(__svr5__)) && \
    (defined(i386) || defined(__i386__)) && \
    !defined(_SOLARIS)
#define _UNIXWARE
#include <sys/scsi.h>		/* This is hacky */
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#if defined(PDI_VERSION) && (PDI_VERSION >= PDI_UNIXWARE20)
#define _UNIXWARE2		/* UnixWare 2 or later */
#endif
#endif

#include "common_d/config.h"			/* Platform configuration */

/*
 * ----- End of platform identification -----
 */

#ifdef BSDCOMPAT
#include <strings.h>
#else
#include <string.h>
#endif

#define PIPE		pipe
#define LINK		link
#define UNLINK		unlink
#define OPENDIR		opendir
#define READDIR		readdir
#define CLOSEDIR	closedir

#ifdef NO_SETSID
#define SETSID		setpgrp
#else
#define SETSID		setsid
#endif

#else
/* OpenVMS */

#define NOMKTMPDIR			/* Do not create TEMP_DIR */
#define CADDR_T		1		/* To avoid redundant typedefs */

#define _WSTOPPED	0177		/* Bit set if stopped */
#define _W_INT(i)	(i)
#define _WSTATUS(x)	(_W_INT(x) & _WSTOPPED)

#define WIFSIGNALED(x)	(_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x)	(WIFSIGNALED(x) ? _WSTATUS(x) : -1)
#define WIFEXITED(x)	(_WSTATUS(x) == 0)
#define WEXITSTATUS(x)	(WIFEXITED(x) ? ((_W_INT(x) >> 8) & 0377) : -1)

/*
 * General header files
 */
#include <types.h>

#ifndef NO_STDLIB_H
#include <stdlib.h>
#endif
#ifndef NO_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <times.h>
#include <stat.h>
#include <string.h>
#include <utsname.h>
#include <descrip.h>
#include <dvidef.h>
#include <lnmdef.h>
#include <processes.h>

#ifndef __DEV_T
typedef unsigned int	uint_t;
typedef uint_t		uid_t;
typedef uint_t		gid_t;
typedef int		pid_t;
typedef uint_t		mode_t;
#endif

#ifndef S_IXUSR
#define S_IXUSR		0000100
#endif
#ifndef S_IXGRP
#define S_IXGRP		0000010
#endif
#ifndef S_IXOTH
#define S_IXOTH		0000001
#endif
#ifndef S_IRUSR
#define S_IRUSR		0000400
#endif

#ifndef SIGALRM
#define SIGALRM		14
#endif

/* Prototype some external library functions */
extern uid_t		getuid(void);
extern gid_t		getgid(void);
extern int		setuid(uid_t),
			setgid(gid_t);
extern char		*getlogin(void);
extern int		usleep(unsigned int);
extern int		decc$pipe(int[2], int, int);

/* _POSIX_C_SOURCE defines pipe with one argument, but the VMS
 * implementation accepts a buffer size, which is nice to use.
 */
#define PIPE(fds)	decc$pipe(fds, 0, 2048)
#define LINK		util_link
#define UNLINK		util_unlink

#ifdef VMS_USE_OWN_DIRENT

#define OPENDIR		util_opendir
#define READDIR		util_readdir
#define CLOSEDIR	util_closedir

struct dirent {
	char		d_name[FILE_PATH_SZ];
};

typedef struct {
	struct dirent	*dd_buf;
} DIR;

#else

#include <dirent.h>

#define OPENDIR		opendir
#define READDIR		readdir
#define CLOSEDIR	closedir

#endif	/* VMS_USE_OWN_DIRENT */

#include "common_d/config.h"			/* Platform configuration */

#endif	/* __VMS */


/*
 * Define these just in case the OS does not support the POSIX definitions
 */
#if defined(S_IFIFO) && !defined(S_ISFIFO)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif
#ifndef SIGCHLD
#define SIGCHLD		SIGCLD
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG	78
#endif


/*
 * Data type definitions: for portability
 */
typedef char			bool_t;		/* Boolean */
typedef unsigned char		byte_t;		/* 8-bit unsigned */
typedef signed char		sbyte_t;	/* 8-bit signed */
typedef unsigned short		word16_t;	/* 16-bit unsigned */
typedef signed short		sword16_t;	/* 16-bit signed */
typedef unsigned int		word32_t;	/* 32-bit unsigned */
typedef signed int		sword32_t;	/* 32-bit signed */

#ifdef __alpha
#ifdef __VMS
typedef unsigned long long	word64_t;	/* 64-bit unsigned */
typedef signed long long	sword64_t;	/* 64-bit signed */
#else
typedef unsigned long		word64_t;	/* 64-bit unsigned */
typedef signed long		sword64_t;	/* 64-bit signed */
#endif	/* __VMS */
#else
#if (defined(__GNUC__) && __GNUC__ > 1) || defined(HAS_LONG_LONG)
typedef unsigned long long	word64_t;	/* 64-bit unsigned */
typedef signed long long	sword64_t;	/* 64-bit signed */
#else
#define FAKE_64BIT
typedef struct { word32_t v[2]; } word64_t;	/* 64-bit unsigned */
typedef struct { word32_t v[2]; } sword64_t;	/* 64-bit signed */
#endif	/* __GNUC__ HAS_LONG_LONG */
#endif

/* Variable assignment macro for 64-bit targets */
#ifdef FAKE_64BIT
#define ASSIGN64(x)		util_assign64(x)
#define ASSIGN32(x)		util_assign32(x)
#else
#define ASSIGN64(x)		((word64_t) (x))
#define ASSIGN32(x)		((word32_t) (x))
#endif


/*
 * Endianess
 */
#define _L_ENDIAN_	1234
#define _B_ENDIAN_	4321

#if defined(i286) || defined(__i286__) || \
    defined(i386) || defined(__i386__) || \
    defined(i486) || defined(__i486__) || \
    defined(__alpha) || defined(__alpha__) || \
    defined(vax) || defined(__vax__) || \
    (defined(MIPSEL) || defined(__MIPSEL__) || \
     defined(__MIPSEL) || defined(_MIPSEL))
#define _BYTE_ORDER_	_L_ENDIAN_
#else
#define _BYTE_ORDER_	_B_ENDIAN_
#endif


/*
 * Platform-specific stuff
 */
#if defined(macII) || defined(sony_news)

#ifndef WEXITSTATUS
#define WEXITSTATUS(x)	(((x.w_status) >> 8) & 0377)
#endif
#ifndef WTERMSIG
#define WTERMSIG(x)	((x.w_status) & 0177)
#endif

typedef union wait	waitret_t;

#else	/* macII */

typedef int		waitret_t;

#endif	/* macII */


#ifdef __VMS
#define WAITPID			util_waitpid
#else
#ifdef sony_news
typedef int			pid_t;
typedef int			mode_t;
#define WAITPID(pid, svp, op)	wait4(pid, svp, 0, 0)
#else
#define WAITPID			waitpid
#endif	/* sony_news */
#endif	/* __VMS */


#if defined(macII) || defined(sony_news)
#define MKFIFO(path, mode)	mknod((path), S_IFIFO | (mode), 0)
#else
#define MKFIFO(path, mode)	mkfifo((path), (mode))
#endif


#ifdef __VMS
#define FORK		vfork
#else
#define FORK		fork
#endif


#if defined(S_IFLNK) && !defined(__VMS)
#define LSTAT		lstat
#else
#define LSTAT		stat
#endif


#ifdef BSDCOMPAT
#ifndef strchr
#define strchr		index
#endif
#ifndef strrchr
#define strrchr		rindex
#endif
#endif	/* BSDCOMPAT */


/*
 * Errno compatibility
 */
#if !defined(ETIME) && defined(ETIMEDOUT)
#define ETIME		ETIMEDOUT
#endif


/*
 * Boolean flags
 */
#ifndef FALSE
#define FALSE		0
#endif
#ifndef TRUE
#define TRUE		1
#endif


/*
 * File path definitions
 */
#ifndef __VMS
#define TEMP_DIR	"/tmp/.cdaudio"		/* Temporary directory */
#define SYS_CMCFG_PATH	"%s/config/common.cfg"	/* System common cfg file */
#define USR_CFG_PATH	"%s/.xmcdcfg"		/* User config dir */
#define USR_PROG_PATH	"%s/.xmcdcfg/prog"	/* User track prog dir */
#define USR_CMCFG_PATH	"%s/.xmcdcfg/common.cfg"/* User common cfg file */
#define SYS_DSCFG_PATH	"%s/config/%s"		/* System devspec cfg file */
#define USR_DSCFG_PATH	"%s/.xmcdcfg/%s"	/* User devspec cfg file */
#define USR_VINIT_PATH	"%s/.xmcdcfg/%s"	/* User version init file */
#define USR_HIST_PATH	"%s/.xmcdcfg/history"	/* User xmcd history file */
#define USR_RMTLOG_PATH	"%s/.xmcdcfg/remote.log"/* User xmcd rmtlog file */
#define REL_DBDIR_PATH  "%s/cdinfo/%s"		/* CD info rel dir path */
#define CDINFOFILE_PATH	"%s/%08x"		/* CD info file path */
#define HELPFILE_PATH	"%s/help/%s"		/* Help file path */
#define DOCFILE_PATH	"%s/docs/"		/* Documentation file path */
#define CONCAT_PATH	"%s/%s"			/* Concatenation of path */
#else
#define TEMP_DIR	"SYS$LOGIN:TMP.DIR"	/* Temporary directory */
#define USR_CFG_PATH	"%s.xmcdcfg]"		/* User config dir */
#define USR_PROG_PATH	"%s.xmcdcfg.prog"	/* User track prog dir */
#define SYS_CMCFG_PATH	"%s]common.cfg"		/* System common cfg file */
#define USR_CMCFG_PATH	"%s.xmcdcfg]common.cfg"	/* User common cfg file */
#define SYS_DSCFG_PATH	"%s]%s"			/* System devspec cfg file */
#define USR_DSCFG_PATH	"%s.xmcdcfg]%s"		/* User devspec cfg file */
#define USR_VINIT_PATH	"%s.xmcdcfg]%s"		/* User version init file */
#define USR_HIST_PATH	"%s.xmcdcfg]history"	/* User xmcd history file */
#define USR_RMTLOG_PATH	"%s.xmcdcfg]remote.log"	/* User xmcd rmtlog file */
#define REL_DBDIR_PATH  "%s.cdinfo.%s]"		/* CD info rel dir path */
#define CDINFOFILE_PATH	"%s%08x."		/* CD info file path */
#define HELPFILE_PATH	"%s.help.%s"		/* Help file path */
#define DOCFILE_PATH	"%s.docs]"		/* Documentation file path */
#define CONCAT_PATH	"%s%s"			/* Concatenation of path */
#endif


/* 
 * URL template attribute information 
 */
typedef struct url_attrib {
	int		xcnt;			/* Count of %X */
	int		vcnt;			/* Count of %V */
	int		ncnt;			/* Count of %N */
	int		hcnt;			/* Count of %H */
	int		lcnt;			/* Count of %L */
	int		ccnt;			/* Count of %C */
	int		icnt;			/* Count of %I */
	int		acnt;			/* Count of %A and %a */
	int		dcnt;			/* Count of %D and %d */
	int		rcnt;			/* Count of %R and %r */
	int		tcnt;			/* Count of %T and %t */
	int		pcnt;			/* Count of %# */
} url_attrib_t;


/*
 * Defines for channel routing modes
 */
#define CHROUTE_NORMAL		0		/* Normal stereo */
#define CHROUTE_REVERSE		1		/* Reverse stereo */
#define CHROUTE_L_MONO		2		/* Left channel mono */
#define CHROUTE_R_MONO		3		/* Right channel mono */
#define CHROUTE_MONO		4		/* Left+Right channel mono */


/*
 * Defines for volume control taper
 */
#define VOLTAPER_LINEAR		0		/* Linear taper */
#define VOLTAPER_SQR		1		/* Square taper */
#define VOLTAPER_INVSQR		2		/* Inverse square */


/*
 * Defines for the type field in trkinfo_t
 */
#define TYP_AUDIO		1		/* Audio track */
#define TYP_DATA		2		/* Data track */

/*
 * Defines for the mode field in curstat_t
 */
#define MOD_NODISC		0		/* No disc loaded */
#define MOD_PLAY		1		/* Play mode */
#define MOD_PAUSE		2		/* Pause mode */
#define MOD_STOP		3		/* Stop mode */
#define MOD_SAMPLE		4		/* Sample mode */
#define MOD_BUSY		5		/* Drive is busy */

/*
 * Defines for the segplay field in curstat_t
 */
#define SEGP_NONE		0		/* Normal play state */
#define SEGP_A			1		/* a->? state */
#define SEGP_AB			2		/* a->b state */


/*
 * Defines for the flags field in curstat_t
 */
#define STAT_SUBMIT		0x01		/* Submit info before clear */
#define STAT_EJECT		0x02		/* Eject CD after clear */
#define STAT_EXIT		0x04		/* Exit program after clear */
#define STAT_CHGDISC		0x08		/* Change disc after clear */


/*
 * Defines for the time_dpy field in curstat_t
 */
#define TIMEDPY_MAX_MODES	6		/* Max number of modes */
#define T_ELAPSED_TRACK		0		/* Per-track elapsed time */
#define T_ELAPSED_SEG		1		/* Per-seg elapsed time */
#define T_ELAPSED_DISC		2		/* Whole-disc elapsed time */
#define T_REMAIN_TRACK		3		/* Per-track remaining time */
#define T_REMAIN_SEG		4		/* Per-seg remaining time */
#define T_REMAIN_DISC		5		/* Whole-disc remaining time */

/*
 * Defines for the qmode field in curstat_t
 */
#define QMODE_NONE		0		/* No status */
#define QMODE_WAIT		1		/* Waiting for CD info */
#define QMODE_MATCH		2		/* CD info match found */
#define QMODE_ERR		3		/* CD info error */

/*
 * Defines for the status field in cdstat_t
 */
#define CDSTAT_PLAYING		0x11		/* Audio play in progress */
#define CDSTAT_PAUSED		0x12		/* Audio play paused */
#define CDSTAT_COMPLETED	0x13		/* Audio play completed */
#define CDSTAT_FAILED		0x14		/* Audio play error */
#define CDSTAT_NOSTATUS		0x15		/* No audio status */

/*
 * Defines for the play_mode field in appdata_t
 * More than one CDDA mode can be enabled at a time.
 */
#define PLAYMODE_STD		0x00		/* Standard playback mode */
#define PLAYMODE_CDDA		0x01		/* CDDA playback mode */
#define PLAYMODE_FILE		0x02		/* CDDA save-to-file mode */
#define PLAYMODE_PIPE		0x04		/* CDDA pipe-to-program mode */

#define PLAYMODE_IS_STD(x)	((x) == 0)
#define PLAYMODE_IS_CDDA(x)	((x) != 0)

/*
 * Defines for the cdda_filefmt field in appdata_t
 */
#define FILEFMT_RAW		0		/* RAW (headerless) format */
#define FILEFMT_AU		1		/* AU format */
#define FILEFMT_WAV		2		/* WAV format */
#define FILEFMT_AIFF		3		/* AIFF format */
#define FILEFMT_AIFC		4		/* AIFF-C format */
#define FILEFMT_MP3		5		/* MPEG-1 Layer 3 format */
#define FILEFMT_OGG		6		/* Ogg Vorbis format */
#define FILEFMT_FLAC		7		/* FLAC format */
#define FILEFMT_AAC		8		/* AAC (MPEG-2,4) format */
#define FILEFMT_MP4		9		/* MP4 format */
#define MAX_FILEFMTS		10		/* Max number of formats */

/* Default CDDA output file path templates */
#define FILEPATH_DISC		"%A-%D"		/* For whole disc */
#define FILEPATH_TRACK		"%#-%T"		/* For each track */

/* Defines for the lameopts_mode field in appdata_t */
#define LAMEOPTS_DISABLE	0		/* Disable */
#define LAMEOPTS_INSERT		1		/* Insert before std opts */
#define LAMEOPTS_APPEND		2		/* Append after std opts */
#define LAMEOPTS_REPLACE	3		/* Replace std opts */

/*
 * Defines for the chset_xlat field in appdata_t
 */
#define CHSET_XLAT_NONE		0		/* No translation */
#define CHSET_XLAT_ISO8859	1		/* UTF-8 <-> ISO8859 */
#define CHSET_XLAT_ICONV	2		/* Use iconv(3) */


/*
 * CD position MSF structure
 */
typedef struct msf {
	byte_t		res;		/* reserved */
	byte_t		min;		/* minutes */
	byte_t		sec;		/* seconds */
	byte_t		frame;		/* frame */
} msf_t;


/*
 * Combined MSF and logical address union
 */
typedef union lmsf {
	msf_t		msf;		/* MSF address */
	sword32_t	logical;	/* logical address */
} lmsf_t;


/*
 * CD per-track information
 */
typedef struct {
	sword32_t	trkno;			/* Track number */
	sword32_t	addr;			/* Absolute offset block */
	byte_t		min;			/* Absolute offset minutes */
	byte_t		sec;			/* Absolute offset seconds */
	byte_t		frame;			/* Absolute offset frame */
	byte_t		type;			/* track type */
	sword32_t	playorder;		/* Prog/Shuf sequence */
	char		*outfile;		/* Cooked audio file paths */
	char		isrc[16];		/* ISRC */
} trkinfo_t;


/*
 * CD position information
 */
typedef struct {
	sword32_t	addr;
	byte_t		min;
	byte_t		sec;
	byte_t		frame;
	byte_t		pad;
} cdpos_t;


/* CDDA status structure */
typedef struct {
	byte_t		status;			/* Play status */
	byte_t		track;			/* Track number */
	byte_t		index;			/* Index number */
	byte_t		rsvd;			/* reserved */
	cdpos_t		abs_addr;		/* Absolute address */
	cdpos_t		rel_addr;		/* Track-relative address */
	byte_t		level;			/* Current volume level */
	byte_t		level_left;		/* Current left volume % */
	byte_t		level_right;		/* Current right volume % */
	byte_t		rsvd2;			/* reserved */
	word32_t	tot_frm;		/* Total frames */
	word32_t	frm_played;		/* Frames played */
	word32_t	frm_per_sec;		/* Frames per sec */
} cdstat_t;


/*
 * Structure containing current status information
 */
typedef struct {
	char		*curdev;		/* Current device */
	byte_t		mode;			/* Player mode */
	byte_t		segplay;		/* a->b mode state */
	byte_t		time_dpy;		/* Time display state */
	byte_t		flags;			/* State flags */

	sword32_t	first_disc;		/* First disc */
	sword32_t	last_disc;		/* Last disc */
	sword32_t	cur_disc;		/* Current disc */
	sword32_t	prev_disc;		/* Previous disc */
	sword32_t	first_trk;		/* First track */
	sword32_t	last_trk;		/* Last track */
	sword32_t	tot_trks;		/* Total number of tracks */

	sword32_t	cur_trk;		/* Current track */
	sword32_t	cur_idx;		/* Current index */

	cdpos_t		discpos_tot;		/* Total CD size */
	cdpos_t		curpos_tot;		/* Current absolute pos */
	cdpos_t		curpos_trk;		/* Current trk relative pos */
	cdpos_t		bp_startpos_tot;	/* Block play start abs pos */
	cdpos_t		bp_endpos_tot;		/* Block play end abs pos */
	cdpos_t		bp_startpos_trk;	/* Block play start trk pos */
	cdpos_t		bp_endpos_trk;		/* Block play end trk pos */

	char		mcn[16];		/* Media catalog number */
	byte_t		aux[4];			/* Auxiliary */

	sword32_t	sav_iaddr;		/* Saved index abs frame */
	word32_t	tot_frm;		/* Total frames */
	word32_t	frm_played;		/* Frames played */
	word32_t	frm_per_sec;		/* Frames per second */
	word32_t	rptcnt;			/* Repeat iteration count */

	trkinfo_t	trkinfo[MAXTRACK];	/* Per-track information */

	bool_t		devlocked;		/* Device locked */
	bool_t		repeat;			/* Repeat mode */
	bool_t		shuffle;		/* Shuffle mode */
	bool_t		program;		/* Program mode */
	bool_t		onetrk_prog;		/* Trk list 1-track program */
	bool_t		caddy_lock;		/* Caddy lock */
	bool_t		chgrscan;		/* Changer scan in process */
	bool_t		rsvd0;			/* reserved */
	byte_t		qmode;			/* CD info query mode */
	byte_t		prog_tot;		/* Prog/Shuf total tracks */
	byte_t		prog_cnt;		/* Prog/Shuf track counter */
	byte_t		level;			/* Current volume level */
	byte_t		level_left;		/* Left channel vol percent */
	byte_t		level_right;		/* Right channel vol percent */
	byte_t		cdda_att;		/* CDDA attentuator percent */
	byte_t		rsvd1;			/* reserved */
	char		*outf_tmpl;		/* Audio file path template */
	char		vendor[9];		/* CD-ROM drive vendor */
	char		prod[17];		/* CD-ROM drive model */
	char		revnum[5];		/* CD-ROM firmware revision */
} curstat_t;


/* flags bits that control when to generate local discography file
 * This is for the app_data.discog_mode parameter
 */
#define DISCOG_GEN_CDINFO	0x1	/* After a successful CD info load */
#define DISCOG_GEN_WWWWARP	0x2	/* When user invokes local discog */


/* This is for the app_data.comp_mode parameter */
#define COMPMODE_0		0	/* Format-specific */
#define COMPMODE_1		1	/* Format-specific */
#define COMPMODE_2		2	/* Format-specific */
#define COMPMODE_3		3	/* Format-specific */


/* This is for the app_data.chan_mode parameter */
#define CH_STEREO		0	/* Stereo */
#define CH_JSTEREO		1	/* Joint-stereo */
#define CH_FORCEMS		2	/* Forced mid/side-stereo */
#define CH_MONO			3	/* Mono */

/* This is for the app_data.id3tag_mode parameter */
#define ID3TAG_V1		0x1	/* ID3 tag version 1 */
#define ID3TAG_V2		0x2	/* ID3 tag version 2 */
#define ID3TAG_BOTH		0x3	/* ID3 tag both versions */

/* This is for the app_data.lowpass_mode and app_data.highpass_mode fields */
#define FILTER_OFF		0	/* Off */
#define FILTER_AUTO		1	/* Auto */
#define FILTER_MANUAL		2	/* Manual */

#define MIN_LOWPASS_FREQ	16	/* Minimum lowpass filter frequency */
#define MAX_LOWPASS_FREQ	50000	/* Maximum lowpass filter frequency */
#define MIN_HIGHPASS_FREQ	500	/* Minimum highpass filter frequency */
#define MAX_HIGHPASS_FREQ	50000	/* Maximum highpass filter frequency */


/*
 * Default message strings
 */
#define MAIN_TITLE		"xmcd"
#define STR_LOCAL		"local"
#define STR_CDDB		"CDDB\262"
#define STR_CDTEXT		"CDtext"
#define STR_QUERY		"query"
#define STR_PROGMODE		"prog"
#define STR_ELAPSE		"elapse"
#define STR_ELAPSE_SEG		"e-seg"
#define STR_ELAPSE_DISC		"e-disc"
#define STR_REMAIN_TRK		"r-trac"
#define STR_REMAIN_SEG		"r-seg"
#define STR_REMAIN_DISC		"r-disc"
#define STR_PLAY		"play"
#define STR_PAUSE		"pause"
#define STR_READY		"ready"
#define STR_SAMPLE		"samp"
#define STR_USAGE		"Usage:"
#define STR_BADOPTS		"The following option is unrecognized"
#define STR_NODISC		"no disc"
#define STR_BUSY		"cd busy"
#define STR_UNKNARTIST		"unknown artist"
#define STR_UNKNDISC		"unknown disc title"
#define STR_UNKNTRK		"unknown track title"
#define STR_DATA		"data"
#define STR_INFO		"Information"
#define STR_WARNING		"Warning"
#define STR_FATAL		"Fatal Error"
#define STR_CONFIRM		"Confirm"
#define STR_WORKING		"Working"
#define STR_ABOUT		"About"
#define STR_QUIT		"Really Quit?"
#define STR_ASKPROCEED		"Do you want to proceed?"
#define STR_CLRPROG		"The play program will be cleared."
#define STR_CANCELSEG		"Segment play will be canceled."
#define STR_RESTARTPLAY		"Playback will be restarted."
#define STR_NOMEMORY		"Out of memory!"
#define STR_TMPDIRERR		"Cannot create or access directory %s!"
#define STR_LIBDIRERR		\
"Cannot determine the xmcd library directory.\n\
You cannot run the program without going through a full installation.\n\
If you downloaded the binary distribution from the official xmcd web site,\n\
be sure to read the installation procedure displayed on the web.\n\
If you compiled xmcd/cda from source, be sure to read the INSTALL file for\n\
installation instructions."
#define STR_LONGPATHERR		"Path or message too long."
#define STR_NOMETHOD		"Unsupported deviceInterfaceMethod parameter!"
#define STR_NOVU		"Unsupported driveVendorCode parameter!"
#define STR_NOHELP		"The help file on this topic is not installed!"
#define STR_NOCFG		"Cannot open configuration file \"%s\"."
#define STR_NOINFO		"No information available."
#define STR_NOTROM		"Device is not a CD-ROM!"
#define STR_NOTSCSI2		"Device is not SCSI-II compliant."
#define STR_SUBMIT		"Submit current disc information to CDDB?"
#define STR_SUBMITOK		"Submission to CDDB succeeded."
#define STR_SUBMITCORR		\
"CDDB has found an inexact match.\n\
If this is not the correct album,\n\
please submit corrections."
#define STR_SUBMITERR		"Submission to CDDB failed."
#define STR_MODERR		"Binary permissions error."
#define STR_STATERR		"Cannot stat device \"%s\"."
#define STR_NODERR		"\"%s\" is not the appropriate device type!"
#define STR_SEQFMTERR		"Program sequence string format error."
#define STR_INVPGMTRK		"Invalid track(s) deleted from program."
#define STR_RECOVERR		"Recovering from audio playback error..."
#define STR_MAXERR		"Too many errors."
#define STR_SAVERR_FORK		"File not saved:\nCannot fork. (errno %d)"
#define STR_SAVERR_SUID		"File not saved:\nCannot setuid to %d."
#define STR_SAVERR_OPEN		"File not saved:\nCannot open file for writing."
#define STR_SAVERR_CLOSE	"File not saved:\nCannot save file."
#define STR_SAVERR_WRITE	"File save error!"
#define STR_SAVERR_KILLED	"File not saved:\nChild killed. (signal %d)"
#define STR_CHGSUBMIT		\
"%s\n%s\n\n\
The on-screen CD database information has changed.\n\
Do you want to submit it to CDDB now?"
#define STR_DEVLIST_UNDEF	"The deviceList parameter is undefined."
#define STR_DEVLIST_COUNT	\
"The number of devices in the deviceList parameter is incorrect."
#define STR_MEDCHG_NOINIT	\
"Cannot initialize medium changer device:\n\
Running as a single-disc player"
#define STR_AUTHFAIL		\
"Proxy authorization failed.\n\
Do you want to re-enter your user name and password?"
#define STR_NOCLIENT		"Not running on display"
#define STR_UNSUPPCMD		"Unsupported command"
#define STR_BADARG		"The specified argument is invalid"
#define STR_INVCMD		"Invalid command for the current state"
#define STR_CMDFAIL		"Command failed"
#define STR_RMT_NOTENB		"Remote control is not enabled"
#define STR_RMT_NOCMD		"Remote control: No command specified"
#ifdef __VMS
#define STR_APPDEF		\
"The XMCD.DAT resource file cannot be located, or is the wrong\n\
version.  A correct version of this file must be present in the\n\
appropriate directory in order for xmcd to run.  Please check\n\
your xmcd installation."
#else
#define STR_APPDEF		\
"The app-defaults/XMcd file cannot be located, or is the wrong version.\n\
You cannot run xmcd without going through a full installation.\n\
If you downloaded the binary distribution from the official xmcd web site,\n\
be sure to read the installation procedure displayed on the web.\n\
If you compiled xmcd/cda from source, be sure to read the INSTALL file for\n\
installation instructions."
#endif
#define STR_KPMODEDSBL		\
"The %s mode is enabled.\n\
Using the keypad will disable it."
#define STR_DLIST_DELALL	\
"This will delete all entries of your disc history."
#define STR_CHGRSCAN		"Scanning CD changer slots..."
#define STR_THE			"The"
#define STR_NONEOFABOVE		"None of the above"
#define STR_ERROR		"error"
#define STR_HANDLEREQ		"You must enter a user handle."
#define STR_HANDLEERR		\
"Either you got your password wrong, or someone else\n\
is already using this handle.\n\n\
Please re-enter the user handle and/or password,\n\
or click the \"E-mail to me\" button to have your\n\
password hint sent to you via e-mail."
#define STR_PASSWDREQ		"You must enter a password."
#define STR_PASSWDMATCHERR	"The passwords do not match.  Try again."
#define STR_MAILINGHINT		"Your password hint is being e-mailed to you."
#define STR_UNKNHANDLE		\
"CDDB does not have a record of this user handle.\n\
Try again."
#define STR_NOHINT		\
"You did not specify a hint when you first registered.\n\
There is nothing to e-mail to you."
#define STR_NOMAILHINT		\
"You did not specify a e-mail address when you first registered.\n\
CDDB cannot e-mail your password hint to you."
#define STR_HINTERR		"Failed to get password hint."
#define STR_USERREGFAIL		"User registration failed."
#define STR_NOWWWWARP		\
"No wwwWarp menu was set up in the wwwwarp.cfg file."
#define STR_CANNOTINVOKE	"Cannot invoke \"%s\""
#define STR_STOPLOAD		"Stop CD information load?"
#define STR_RELOAD		"Re-load CD information?"
#define STR_NEEDROLE		"You must select a role."
#define STR_NEEDROLENAME	"You must enter a name for the specified role."
#define STR_DUPCREDIT		"This credit is already in the list."
#define STR_DUPTRKCREDIT	\
"This credit is a duplicate of a track credit (track %s).\n\
Since an album credit applies to all tracks, it is\n\
ambiguous to have a track credit to be the same as\n\
the album credit."
#define STR_DUPDISCCREDIT	\
"This credit is a duplicate of an album credit.\n\
Since an album credit applies to all tracks, it is\n\
ambiguous to have a track credit to be the same as\n\
the album credit."
#define STR_NOFIRST		\
"The last name is filled in but the first name is not."
#define STR_NOFIRSTLAST		\
"\"The\" is enabled, but there is no text in the first name\n\
field or the last name field."
#define STR_ALBUMARTIST		"Album artist"
#define STR_TRACKARTIST		"Track %d artist"
#define STR_CREDIT		"Credit"
#define STR_FNAMEGUIDE \
"Click mouse button 3 on any field for usage guidelines"
#define STR_NOCATEG		"You must enter category information."
#define STR_NONAME		"You must enter a name for the site."
#define STR_INVALURL		"The URL is not valid."
#define STR_SEGPOSERR		\
"The end position must be at least a number of\n\
frames past the start position."
#define STR_INCSEGINFO		\
"Incomplete segment information.\n\
You must enter a segment name, and the start\n\
and end point track and frame numbers."
#define STR_INVSEGINFO		\
"Invalid segment information.\n\
Check the start and end point track and frame\n\
numbers."
#define STR_DISCARDCHG		"Do you want to discard your changes?"
#define STR_NOAUDIOPATH		"No output audio file name specified."
#define STR_AUDIOPATHEXISTS	"Cannot write audio file: file exists."
#define STR_PLAYBACKMODE	"Playback mode"
#define STR_ENCODEPARMS		"Encoding parameters"
#define STR_LAMEOPTS		"LAME MP3 options"
#define STR_SCHEDPARMS		"CDDA scheduling"
#define STR_AUTOFUNCS		"Automated functions"
#define STR_CDCHANGER		"CD changer"
#define STR_CHROUTE		"Channel routing"
#define STR_VOLBAL		"Volume & Balance"
#define STR_CDDB_CDTEXT		"CDDB & CD-Text"
#define STR_CDDAINIT_FAIL	\
"Cannot initialize CDDA.  Possible causes are:\n\n\
- An error condition as described above, if displayed.\n\
- The cddaMethod, cddaReadMethod or cddaWriteMethod\n\
  parameters may be misconfigured.\n\
- There is no CDDA support for your CD drive or\n\
  OS platform in this version.\n\n\
See your xmcd documentation for information."
#define STR_FILENAMEREQ		"You must select or enter a file name."
#define STR_NOTREGFILE		\
"The file name you selected or entered\n\
is not a regular file."
#define STR_NOPROG		"CDDA pipe to program: No program specified."
#define STR_OVERWRITE		\
"The file exists in the output directory.\n\
Do you want to overwrite it?"
#define STR_OUTDIR		\
"CDDA save to file: Writing audio files in the following directory:"
#define STR_PATHEXP		\
"CDDA save to file: The expanded output file path name:"
#define STR_INVBR_MINMEAN	\
"Invalid minimum bitrate: It must be less than\n\
or equal to the average bitrate."
#define STR_INVBR_MINMAX	\
"Invalid minimum bitrate: It must be less than\n\
or equal to the maximum bitrate."
#define STR_INVBR_MAXMEAN	\
"Invalid maximum bitrate: It must be greater than\n\
or equal to the average bitrate."
#define STR_INVBR_MAXMIN	\
"Invalid maximum bitrate: It must be greater than\n\
or equal to the minimum bitrate."
#define STR_INVFREQ_LP		\
"Invalid lowpass filter frequency.\n\
The valid range is %d to %d Hz."
#define STR_INVFREQ_HP		\
"Invalid highpass filter frequency.\n\
The valid range is %d to %d Hz."
#define STR_NOMSG		"No messages."

/*
 * Application resource/option data
 */
typedef struct {
	char		*libdir;		/* Library path */

	/* X resources */
	char		*version;		/* app-defaults file version */
	int		main_mode;		/* Default main window mode */
	int		modechg_grav;		/* Which corner is fixed */
	int		normal_width;		/* Normal mode width */
	int		normal_height;		/* Normal mode height */
	int		basic_width;		/* Basic mode width */
	int		basic_height;		/* Basic mode height */
	int		blinkon_interval;	/* Display blink on (ms) */
	int		blinkoff_interval;	/* Display blink off (ms) */
	bool_t		main_showfocus;		/* Highlight kbd focus? */
	bool_t		instcmap;		/* Install colormap? */
	bool_t		remotemode;		/* Remote ctrl mode */
	bool_t		rsvd;			/* reserved */
	char		*remotehost;		/* Remote ctrl client host */

	/*
	 * Common config parameters
	 */
	char		*device;		/* Default CD-ROM device */
	char		*cdinfo_path;		/* CD info paths */
	char		*cdinfo_filemode;	/* CD info file permissions */
	char		*proxy_server;		/* http proxy host:port */
	char		*hist_filemode;		/* History file permissions */
	char		*exclude_words;		/* Keywords to exclude */
	char		*cdda_tmpl;		/* CDDA output file template */
	char		*pipeprog;		/* Pgm to pipe audio data to */
	char		*lame_opts;		/* Direct LAME cmdline */
	char		*discog_url_pfx;	/* Local discog URL prefix */
	char		*lang_utf8;		/* The LANG env for UTF-8 */

	int		chset_xlat;		/* Charset translation mode */
	int		cache_timeout;		/* Local cache timeout (days)*/
	int		srv_timeout;		/* Service timeout (secs) */
	int		discog_mode;		/* Local discography mode */
	int		cdinfo_maxhist;		/* Max history count */
	int		stat_interval;		/* Status poll interval (ms) */
	int		ins_interval;		/* Insert poll interval (ms) */
	int		prev_threshold;		/* Previous track/index
						 * threshold (blocks)
						 */
	int		sample_blks;		/* Sample play blocks */
	int		timedpy_mode;		/* Default time display mode */
	int		tooltip_delay;		/* Tool-tip delay interval */
	int		tooltip_time;		/* Tool-tip active interval */
	int		cdda_sched;		/* CDDA sched option flags */
	int		hb_timeout;		/* Heartbeat timeout (secs) */
	int		cdda_filefmt;		/* Output file format */
	int		comp_mode;		/* MP3/OGG compression mode */
	int		bitrate;		/* MP3/OGG bitrate */
	int		bitrate_min;		/* MP3/OGG minimum bitrate */
	int		bitrate_max;		/* MP3/OGG maximum bitrate */
	int		qual_factor;		/* MP3/OGG quality factor */
	int		chan_mode;		/* channel mode */
	int		comp_algo;		/* Comp algorithm tuning */
	int		lowpass_mode;		/* lowpass filter mode */
	int		lowpass_freq;		/* lowpass filter frequency */
	int		lowpass_width;		/* lowpass filter width */
	int		highpass_mode;		/* highpass filter mode */
	int		highpass_freq;		/* highpass filter frequency */
	int		highpass_width;		/* highpass filter width */
	int		lameopts_mode;		/* Direct LAME cmdline mode */
	int		id3tag_mode;		/* MP3 ID3tag mode */

	word32_t	outport;		/* CDDA playback output port */
	word32_t	debug;			/* Debug output level */

	bool_t		ins_disable;		/* Insert poll disable */
	bool_t		cdinfo_inetoffln;	/* Internet offline */
	bool_t		use_proxy;		/* Using proxy server */
	bool_t		proxy_auth;		/* Use proxy authorization */
	bool_t		auto_musicbrowser;	/* Auto CDDB Music Browser */
	bool_t		scsierr_msg;		/* Print SCSI error msg? */
	bool_t		sol2_volmgt;		/* Solaris 2.x Vol Mgr */
	bool_t		write_curfile;		/* Enable curr.XXX output */
	bool_t		tooltip_enable;		/* Enable tool-tips */
	bool_t		histfile_dsbl;		/* Disable history file */
	bool_t		remote_enb;		/* Enable remote control */
	bool_t		remote_log;		/* Enable rmtctl logfile */
	bool_t		cdda_trkfile;		/* One output file per track */
	bool_t		subst_underscore;	/* Substitute space to '_' */
	bool_t		copyright;		/* Add copyright flag */
	bool_t		original;		/* Add original flag */
	bool_t		nores;			/* Disable bit reservoir */
	bool_t		checksum;		/* Do checksum */
	bool_t		strict_iso;		/* Strict ISO MP3 compliance */
	bool_t		add_tag;		/* Add MP3/OGG comment tag */
	bool_t		load_spindown;		/* Spin down disc on CD load */
	bool_t		load_play;		/* Auto play on CD load */
	bool_t		done_eject;		/* Auto eject on done */
	bool_t		done_exit;		/* Auto exit on done */
	bool_t		exit_eject;		/* Eject disc on exit? */
	bool_t		exit_stop;		/* Stop disc on exit? */
	bool_t		eject_exit;		/* Exit upon disc eject? */
	bool_t		repeat_mode;		/* Repeat enable on startup */
	bool_t		shuffle_mode;		/* Shuffle enable on startup */
	bool_t		single_fuzzy;		/* Accept single fuzzy match */
	bool_t		automotd_dsbl;		/* Disable automatic-MOTD */
	bool_t		rsvd1[4];		/* Reserved */

	/*
	 * Device-specific config parameters
	 */

	/* Privileged */
	int		devnum;			/* Logical device number */
	char		*devlist;		/* CD-ROM device list */
	int		di_method;		/* Device interface method */
	int		vendor_code;		/* Vendor command set code */
	int		numdiscs;		/* Number of discs */
	int		chg_method;		/* Medium change method */
	int		base_scsivol;		/* SCSI volume value base */
	int		min_playblks;		/* Minimum play blocks */
	int		cdda_method;		/* CDDA method */
	int		cdda_rdmethod;		/* CDDA read method */
	int		cdda_wrmethod;		/* CDDA write method */
	int		cdda_scsidensity;	/* CDDA SCSI modesel density */
	int		cdda_scsireadcmd;	/* CDDA SCSI vendor cmd set */
	int		cdda_readchkblks;	/* CDDA read chunk */
	bool_t		scsiverck;		/* SCSI version check */
	bool_t		play10_supp;		/* Play Audio (10) supported */
	bool_t		play12_supp;		/* Play Audio (12) supported */
	bool_t		playmsf_supp;		/* Play Audio MSF supported */
	bool_t		playti_supp;		/* Play Audio T/I supported */
	bool_t		load_supp;		/* Motorized load supported */
	bool_t		eject_supp;		/* Motorized eject supported */
	bool_t		msen_dbd;		/* Set DBD bit for msense */
	bool_t		msen_10;		/* Use 10-byte mode sen/sel */
	bool_t		mselvol_supp;		/* Audio vol chg supported */
	bool_t		balance_supp;		/* Indep vol chg supported */
	bool_t		chroute_supp;		/* Channel routing support */
	bool_t		pause_supp;		/* Pause/Resume supported */
	bool_t		strict_pause_resume;	/* Must resume after pause */
	bool_t		play_pause_play;	/* Must pause before play */
	bool_t		caddylock_supp;		/* Caddy lock supported */
	bool_t		curpos_fmt;		/* Fmt 1 of RdSubch command */
	bool_t		play_notur;		/* No Tst U Rdy when playing */
	bool_t		toc_lba;		/* Rd TOC in LBA mode */
	bool_t		subq_lba;		/* Rd Subchan in LBA mode */
	bool_t		mcn_dsbl;		/* Disable MCN read */
	bool_t		isrc_dsbl;		/* Disable ISRC read */
	bool_t		cdtext_dsbl;		/* Disable CD-TEXT read */
	bool_t		cdda_bigendian;		/* CDDA is in big endian */
	bool_t		cdda_modesel;		/* Do mode select for CDDA */
	bool_t		rsvd2[3];		/* reserved */

	int		drv_blksz;		/* Drive native block size */
	int		spinup_interval;	/* spin up delay (sec) */

	/* User-modifiable */
	int		play_mode;		/* Playback mode */
	int		vol_taper;		/* Volume control taper */
	int		startup_vol;		/* Startup volume preset */
	int		ch_route;		/* Channel routing */
	int		skip_blks;		/* FF/REW skip blocks */
	int		skip_pause;		/* FF/REW pause (msec) */
	int		skip_spdup;		/* FF/REW speedup count */
	int		skip_vol;		/* FF/REW percent volume */
	int		skip_minvol;		/* FF/REW minimum volume */

	bool_t		eject_close;		/* Close upon disc eject? */
	bool_t		caddy_lock;		/* Lock caddy on CD load? */
	bool_t		multi_play;		/* Multi-CD playback */
	bool_t		reverse;		/* Multi-CD reverse playback */
	bool_t		cdda_jitter_corr;	/* Do CDDA jitter correction */
	bool_t		rsvd3[3];		/* reserved */

	/*
	 * Various application message strings
	 */
	char		*main_title;		/* Main window title */
	char		*str_local;		/* local */
	char		*str_cddb;		/* CDDB */
	char		*str_cdtext;		/* CD-Text */
	char		*str_query;		/* query */
	char		*str_progmode;		/* prog */
	char		*str_elapse;		/* elapse */
	char		*str_elapseseg;		/* e-seg */
	char		*str_elapsedisc;	/* e-disc */
	char		*str_remaintrk;		/* r-trac */
	char		*str_remainseg;		/* r-seg */
	char		*str_remaindisc;	/* r-disc */
	char		*str_play;		/* play */
	char		*str_pause;		/* pause */
	char		*str_ready;		/* ready */
	char		*str_sample;		/* sample */
	char		*str_badopts;		/* Bad command-line options */
	char		*str_nodisc;		/* No disc */
	char		*str_busy;		/* Device busy */
	char		*str_unknartist;	/* unknown artist */
	char		*str_unkndisc;		/* unknown disc title */
	char		*str_unkntrk;		/* unknown track title */
	char		*str_data;		/* Data */
	char		*str_info;		/* Information */
	char		*str_warning;		/* Warning */
	char		*str_fatal;		/* Fatal error */
	char		*str_confirm;		/* Confirm */
	char		*str_working;		/* Working */
	char		*str_about;		/* About */
	char		*str_quit;		/* Really Quit? */
	char		*str_askproceed;	/* Do you want to proceed? */
	char		*str_clrprog;		/* Program will be cleared */
	char		*str_cancelseg;		/* Seg play will be canceled */
	char		*str_restartplay;	/* Playback will restart */
	char		*str_nomemory;		/* Out of memory */
	char		*str_tmpdirerr;		/* tempdir problem */
	char		*str_libdirerr;		/* libdir not defined */
	char		*str_longpatherr;	/* Path or message too long */
	char		*str_nomethod;		/* Invalid di_method */
	char		*str_novu;		/* Invalid vendor code */
	char		*str_nohelp;		/* No help available on item */
	char		*str_nodb;		/* No CDDB directory */
	char		*str_nocfg;		/* Can't open config file */
	char		*str_noinfo;		/* No information avail */
	char		*str_notrom;		/* Not a CD-ROM device */
	char		*str_notscsi2;		/* Not SCSI-II compliant */
	char		*str_submit;		/* Submit CDDB confirm msg */
	char		*str_submitok;		/* Submit CDDB succeeded */
	char		*str_submitcorr;	/* Please submit corrections */
	char		*str_submiterr;		/* Submit CDDB failed */
	char		*str_moderr;		/* Binary perms error */
	char		*str_staterr;		/* Can't stat device */
	char		*str_noderr;		/* Not a character device */
	char		*str_seqfmterr;		/* Pgm sequence format err */
	char		*str_invpgmtrk;		/* Inv program trk deleted */
	char		*str_recoverr;		/* Recovering audio play err */
	char		*str_maxerr;		/* Too many errors */
	char		*str_saverr_fork;	/* File save err: fork */
	char		*str_saverr_suid;	/* File save err: setuid */
	char		*str_saverr_open;	/* File save err: open */
	char		*str_saverr_close;	/* File save err: close */
	char		*str_saverr_write;	/* File save err: write */
	char		*str_saverr_killed;	/* File save err: child kill */
	char		*str_chgsubmit;		/* Submit change dialog msg */
	char		*str_devlist_undef;	/* deviceList no defined */
	char		*str_devlist_count;	/* deviceList count wrong */
	char		*str_medchg_noinit;	/* Cannot init medium chgr */
	char		*str_authfail;		/* Proxy auth failure */
	char		*str_noclient;		/* Can't find client */
	char		*str_unsuppcmd;		/* Unsupp remote command */
	char		*str_badarg;		/* Specified arg is bad */
	char		*str_invcmd;		/* Invalid cmd for current */
	char		*str_cmdfail;		/* Command failed */
	char		*str_rmt_notenb;	/* Remote ctl not enabled */
	char		*str_rmt_nocmd;		/* Remote ctl no command */
	char		*str_appdef;		/* app-defaults file error */
	char		*str_kpmodedsbl;	/* Keypad mode dsbl prompt */
	char		*str_dlist_delall;	/* Delete all dlist entries */
	char		*str_chgrscan;		/* CD Changer scanning */
	char		*str_the;		/* The */
	char		*str_noneofabove;	/* None of the above */
	char		*str_error;		/* error */
	char		*str_handlereq;		/* User handle required */
	char		*str_handleerr;		/* User handle error */
	char		*str_passwdreq;		/* Password required */
	char		*str_passwdmatcherr;	/* Passwords do not match */
	char		*str_mailinghint;	/* Mailing passwd hint */
	char		*str_unknhandle;	/* Unknown handle */
	char		*str_nohint;		/* No passwd hint available */
	char		*str_nomailhint;	/* Cannot email passwd hint */
	char		*str_hinterr;		/* Cannot get passwd hint */
	char		*str_userregfail;	/* User registration failed */
	char		*str_nowwwwarp;		/* No wwwwarp menu defs */
	char		*str_cannotinvoke;	/* Cannot invoke destination */
	char		*str_stopload;		/* Stop CD info load prompt */
	char		*str_reload;		/* CD info reload prompt */
	char		*str_needrole;		/* Need to select role */
	char		*str_needrolename;	/* Need name for role */
	char		*str_dupcredit;		/* Credit already in list */
	char		*str_duptrkcredit;	/* Credit already in tracks */
	char		*str_dupdisccredit;	/* Credit already in disc */
	char		*str_nofirst;		/* Last but no first name */
	char		*str_nofirstlast;	/* No first or last name */
	char		*str_albumartist;	/* Album artist */
	char		*str_trackartist;	/* Track # artist */
	char		*str_credit;		/* Credit */
	char		*str_fnameguide;	/* Fullname guidelines */
	char		*str_nocateg;		/* Category info needed */
	char		*str_noname;		/* Name needed */
	char		*str_invalurl;		/* Invalid URL */
	char		*str_segposerr;		/* Segment start/end error */
	char		*str_incseginfo;	/* Incomplete segment info */
	char		*str_invseginfo;	/* Invalid segment info */
	char		*str_discardchg;	/* Discard change? */
	char		*str_noaudiopath;	/* Audio file path missing */
	char		*str_audiopathexists;	/* Audio file exists */
	char		*str_playbackmode;	/* Playback mode */
	char		*str_encodeparms;	/* Encoding parameters */
	char		*str_lameopts;		/* LAME MP3 options */
	char		*str_schedparms;	/* CDDA sched parameters */
	char		*str_autofuncs;		/* Automated functions */
	char		*str_cdchanger;		/* CD changer */
	char		*str_chroute;		/* Channel routing */
	char		*str_volbal;		/* Volume / Balance */
	char		*str_cddb_cdtext;	/* CDDB / CD-TEXT */
	char		*str_cddainit_fail;	/* CDDA init failure */
	char		*str_filenamereq;	/* File name required */
	char		*str_notregfile;	/* Not a regular file */
	char		*str_noprog;		/* No CDDA program specified */
	char		*str_overwrite;		/* Overwrite file? */
	char		*str_outdir;		/* CDDA file directory */
	char		*str_pathexp;		/* CDDA expanded file path */
	char		*str_invbr_minmean;	/* Invalid bitrate */
	char		*str_invbr_minmax;	/* Invalid bitrate */
	char		*str_invbr_maxmean;	/* Invalid bitrate */
	char		*str_invbr_maxmin;	/* Invalid bitrate */
	char		*str_invfreq_lp;	/* Invalid lowpass freq */
	char		*str_invfreq_hp;	/* Invalid highpass freq */
	char		*str_nomsg;		/* No messages */

	/*
	 * Short-cut key translations
	 */
	char		*main_hotkeys;		/* Main window */
	char		*keypad_hotkeys;	/* Keypad window */

	/*
	 * Miscellaneous
	 */
	void		*aux;			/* Auxiliary */
} appdata_t;


#endif	/* __APPENV_H__ */

