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
#ifndef __LIBDI_H__
#define __LIBDI_H__

#ifndef lint
static char *_libdi_h_ident_ = "@(#)libdi.h	6.94 04/03/05";
#endif

/* Max number of libdi modules */
#define MAX_METHODS	4

/*
 * Supported libdi methods
 *
 * Comment out any of these to remove support for a method.
 * Removing unused methods can reduce the executable
 * binary size and run-time memory usage.  At least one
 * method must be defined.
 *
 * Note: If compiling for DEMO_ONLY or on a non-supported OS
 * platform, DI_SCSIPT must be defined.
 */
#if !defined(__QNX__)
#define DI_SCSIPT	0	/* SCSI pass-through method */
#endif
#if defined(_SUNOS4) || defined(_SOLARIS) || \
    defined(_LINUX)  || defined(__QNX__)
#define DI_SLIOC	1	/* SunOS/Solaris/Linux/QNX ioctl method */
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define DI_FBIOC	2	/* FreeBSD/NetBSD/OpenBSD ioctl method */
#endif
#if defined(_AIX) && defined(AIX_IDE)
#define DI_AIXIOC	3	/* AIX IDE ioctl method */
#endif


/*
 * Multi-CD medium change methods
 */
#define MAX_CHG_METHODS	4

#define CHG_NONE	0	/* Changer controls not supported */
#define CHG_SCSI_LUN	1	/* SCSI LUN addressing method */
#define CHG_SCSI_MEDCHG	2	/* SCSI Medium changer device method */
#define CHG_OS_IOCTL	3	/* OS-ioctl method */


/* Application name */
#define APPNAME		"CD audio"


/* Play audio format codes */
#define ADDR_BLK	0x01	/* block address specified */
#define ADDR_MSF	0x02	/* MSF address specified */
#define ADDR_TRKIDX	0x04	/* track/index numbers specified */
#define ADDR_OPTEND	0x80	/* End address can be ignored */


/* Slider control flags */
#define WARP_VOL	0x1	/* Set volume slider thumb */
#define WARP_BAL	0x2	/* Set balance slider thumb */


/* Device capabilities: These should match similar defines in cdda_d/cdda.h */
#define CAPAB_PLAYAUDIO	0x01	/* Can play CD audio */
#define CAPAB_RDCDDA	0x02	/* Can read CDDA from drive */
#define CAPAB_WRDEV	0x10	/* Can write audio stream to sound device */
#define CAPAB_WRFILE	0x20	/* Can write audio stream to file */
#define CAPAB_WRPIPE	0x40	/* Can pipe audio stream to a program */


/* Misc constants */
#define MAX_SRCH_BLKS	225	/* max search play blks per sample */
#define MAX_RECOVERR	20	/* Max number of err recovery tries */
#define ERR_SKIPBLKS	10	/* Number of frame to skip on error */
#define ERR_CLRTHRESH	1500	/* If there hasn't been any errors
				 * for this many blocks of audio
				 * playback, then the previous errors
				 * count is cleared.
				 */
#define CLIP_FRAMES	10	/* Frames to clip from end */
#define DEF_CACHE_TIMEOUT 7	/* Default local cache timeout (days) */
#define DEF_SRV_TIMEOUT	60	/* Default service timeout (seconds) */



/* CDDB/Program macros */
#define DBCLEAR(s, b)		{					\
	if (di_clinfo != NULL && di_clinfo->dbclear != NULL)		\
		di_clinfo->dbclear((s), (b));				\
}
#define DBGET(s)		{					\
	if (di_clinfo != NULL && di_clinfo->dbget != NULL)		\
		di_clinfo->dbget(s);					\
}
#define PROGCLEAR(s)		{					\
	if (di_clinfo != NULL && di_clinfo->progclear != NULL)		\
		di_clinfo->progclear(s);				\
}
#define PROGGET(s)		{					\
	if (di_clinfo != NULL && di_clinfo->progget != NULL)		\
		di_clinfo->progget(s);					\
}
#define STOPSCAN(s)		{					\
	if (di_clinfo != NULL && di_clinfo->chgr_scan_stop != NULL)	\
		di_clinfo->chgr_scan_stop(s);				\
}


/* Message dialog macros */
#define DI_FATAL(msg)		{					\
	if (di_clinfo != NULL && di_clinfo->fatal_msg != NULL)		\
		di_clinfo->fatal_msg(app_data.str_fatal, (msg));	\
	else {								\
		(void) fprintf(errfp, "Fatal: %s\n", (msg));		\
		_exit(1);						\
	}								\
}
#define DI_WARNING(msg)		{					\
	if (di_clinfo != NULL && di_clinfo->warning_msg != NULL)	\
		di_clinfo->warning_msg(app_data.str_warning, (msg));	\
	else								\
		(void) fprintf(errfp, "Warning: %s\n", (msg));		\
}
#define DI_INFO(msg)		{					\
	if (di_clinfo != NULL && di_clinfo->info_msg != NULL)		\
		di_clinfo->info_msg(app_data.str_info, (msg));		\
	else								\
		(void) fprintf(errfp, "Info: %s\n", (msg));		\
}
#define DI_INFO2(msg, txt)	{					\
	if (di_clinfo != NULL && di_clinfo->info2_msg != NULL)		\
		di_clinfo->info2_msg(app_data.str_info, (msg), (txt));	\
	else								\
		(void) fprintf(errfp, "Info: %s\n%s\n", (msg), (txt));	\
}


/* GUI macros */
#define DO_BEEP()		{					\
	if (di_clinfo != NULL && di_clinfo->beep != NULL)		\
		di_clinfo->beep();					\
}
#define SET_LOCK_BTN(state)	{					\
	if (di_clinfo != NULL && di_clinfo->set_lock_btn != NULL)	\
		di_clinfo->set_lock_btn(state);				\
}
#define SET_SHUFFLE_BTN(state)	{					\
	if (di_clinfo != NULL && di_clinfo->set_shuffle_btn != NULL)	\
		di_clinfo->set_shuffle_btn(state);			\
}
#define SET_VOL_SLIDER(val)	{					\
	if (di_clinfo != NULL && di_clinfo->set_vol_slider != NULL)	\
		di_clinfo->set_vol_slider(val);				\
}
#define SET_BAL_SLIDER(val)	{					\
	if (di_clinfo != NULL && di_clinfo->set_bal_slider != NULL)	\
		di_clinfo->set_bal_slider(val);				\
}
#define DPY_ALL(s)		{					\
	if (di_clinfo != NULL && di_clinfo->dpy_all != NULL)		\
		di_clinfo->dpy_all(s);					\
}
#define DPY_DISC(s)		{					\
	if (di_clinfo != NULL && di_clinfo->dpy_disc != NULL)		\
		di_clinfo->dpy_disc(s);				\
}
#define DPY_TRACK(s)		{					\
	if (di_clinfo != NULL && di_clinfo->dpy_track != NULL)		\
		di_clinfo->dpy_track(s);				\
}
#define DPY_INDEX(s)		{					\
	if (di_clinfo != NULL && di_clinfo->dpy_index != NULL)		\
		di_clinfo->dpy_index(s);				\
}
#define DPY_TIME(s, b)		{					\
	if (di_clinfo != NULL && di_clinfo->dpy_time != NULL)		\
		di_clinfo->dpy_time((s), (b));				\
}
#define DPY_PROGMODE(s, b)		{				\
	if (di_clinfo != NULL && di_clinfo->dpy_progmode != NULL)	\
		di_clinfo->dpy_progmode((s), (b));			\
}
#define DPY_PLAYMODE(s, b)		{				\
	if (di_clinfo != NULL && di_clinfo->dpy_playmode != NULL)	\
		di_clinfo->dpy_playmode((s), (b));			\
}
#define DPY_RPTCNT(s)		{					\
	if (di_clinfo != NULL && di_clinfo->dpy_rptcnt != NULL)		\
		di_clinfo->dpy_rptcnt(s);				\
}


/*
 * CD-TEXT information structures
 */
typedef struct {
	char		*title;			/* Title */
	char		*performer;		/* Performer name */
	char		*songwriter;		/* Songwriter name */
	char		*composer;		/* Composer name */
	char		*arranger;		/* Arranger name */
	char		*message;		/* Message */
	char		*catno;			/* UPC/EAN or ISRC code */
} di_cdtext_inf_t;

typedef struct {
	bool_t		cdtext_valid;		/* Has CD-TEXT data */
	bool_t		rsvd[3];		/* reserved */
	char		*ident;			/* Disc identification info */
	di_cdtext_inf_t	disc;			/* Disc information */
	di_cdtext_inf_t	track[MAXTRACK];	/* Track information */
} di_cdtext_t;


/*
 * Jump table structure for libdi interface
 */
typedef struct {
	void		(*load_cdtext)(curstat_t *, di_cdtext_t *);
	bool_t		(*playmode)(curstat_t *);
	bool_t		(*check_disc)(curstat_t *);
	void		(*status_upd)(curstat_t *);
	void		(*lock)(curstat_t *, bool_t);
	void		(*repeat)(curstat_t *, bool_t);
	void		(*shuffle)(curstat_t *, bool_t);
	void		(*load_eject)(curstat_t *);
	void		(*ab)(curstat_t *);
	void		(*sample)(curstat_t *);
	void		(*level)(curstat_t *, byte_t, bool_t);
	void		(*play_pause)(curstat_t *);
	void		(*stop)(curstat_t *, bool_t);
	void		(*chgdisc)(curstat_t *);
	void		(*prevtrk)(curstat_t *);
	void		(*nexttrk)(curstat_t *);
	void		(*previdx)(curstat_t *);
	void		(*nextidx)(curstat_t *);
	void		(*rew)(curstat_t *, bool_t);
	void		(*ff)(curstat_t *, bool_t);
	void		(*warp)(curstat_t *);
	void		(*route)(curstat_t *);
	void		(*mute_on)(curstat_t *);
	void		(*mute_off)(curstat_t *);
	void		(*cddajitter)(curstat_t *);
	void		(*debug)(void);
	void		(*start)(curstat_t *);
	void		(*icon)(curstat_t *, bool_t);
	void		(*halt)(curstat_t *);
	char *		(*methodstr)(void);
} di_tbl_t;


/*
 * Jump table for libdi initialization
 */
typedef struct {
	void	(*init)(curstat_t *, di_tbl_t *);
} diinit_tbl_t;


/*
 * Callbacks into the application
 */
typedef struct {
	word32_t	capab;		/* Device capabilities bitmask */
	curstat_t *	(*curstat_addr)(void);			/* Required */
	void		(*quit)(curstat_t *);			/* Required */
	long		(*timeout)(word32_t, void (*)(), byte_t *);
	void		(*untimeout)(long);
	void		(*dbclear)(curstat_t *, bool_t);
	void		(*dbget)(curstat_t *);
	void		(*progclear)(curstat_t *);
	void		(*progget)(curstat_t *);
	void		(*chgr_scan_stop)(curstat_t *);
	bool_t		(*mkoutpath)(curstat_t *);
	bool_t		(*ckpipeprog)(curstat_t *);
	void		(*fatal_msg)(char *, char *);
	void		(*warning_msg)(char *, char *);
	void		(*info_msg)(char *, char *);
	void		(*info2_msg)(char *, char *, char *);
	void		(*beep)(void);
	void		(*set_lock_btn)(bool_t);
	void		(*set_shuffle_btn)(bool_t);
	void		(*set_vol_slider)(int);
	void		(*set_bal_slider)(int);
	void		(*dpy_all)(curstat_t *);
	void		(*dpy_disc)(curstat_t *);
	void		(*dpy_track)(curstat_t *);
	void		(*dpy_index)(curstat_t *);
	void		(*dpy_time)(curstat_t *, bool_t);
	void		(*dpy_progmode)(curstat_t *, bool_t);
	void		(*dpy_playmode)(curstat_t *, bool_t);
	void		(*dpy_rptcnt)(curstat_t *);
} di_client_t;


/* Used for the role field of di_dev_t */
#define DI_ROLE_MAIN	1		/* main control thread */
#define DI_ROLE_READER	2		/* CD reader thread */


/*
 * Device descriptor structure, returned by pthru_open()
 */
typedef struct di_dev {
	int		fd;		/* primary file descriptor */
	int		fd2;		/* secondary file descriptor */
	int		role;		/* enable I/O for which role */
	word32_t	flags;		/* flags */
	char		*path;		/* path name string */
	char		*auxpath;	/* secondary path name string */
	void		*auxdesc;	/* secondary descriptor */
	int		bus;		/* bus number */
	int		target;		/* target id number */
	int		lun;		/* logical unit number */
	int		val;		/* platform-specific field */
	int		val2;		/* platform-specific field */
} di_dev_t;


/*
 * Public function prototypes: libdi external calling interface.
 */
extern void		di_init(di_client_t *);
extern bool_t		di_bitrate_valid(int);
extern void		di_load_cdtext(curstat_t *, di_cdtext_t *);
extern void		di_clear_cdtext(di_cdtext_t *);
extern bool_t		di_playmode(curstat_t *);
extern bool_t		di_check_disc(curstat_t *);
extern void		di_status_upd(curstat_t *);
extern void		di_lock(curstat_t *, bool_t);
extern void		di_repeat(curstat_t *, bool_t);
extern void		di_shuffle(curstat_t *, bool_t);
extern void		di_load_eject(curstat_t *);
extern void		di_ab(curstat_t *);
extern void		di_sample(curstat_t *);
extern void		di_level(curstat_t *, byte_t, bool_t);
extern void		di_play_pause(curstat_t *);
extern void		di_stop(curstat_t *, bool_t);
extern void		di_chgdisc(curstat_t *);
extern void		di_prevtrk(curstat_t *);
extern void		di_nexttrk(curstat_t *);
extern void		di_previdx(curstat_t *);
extern void		di_nextidx(curstat_t *);
extern void		di_rew(curstat_t *, bool_t);
extern void		di_ff(curstat_t *, bool_t);
extern void		di_warp(curstat_t *);
extern void		di_route(curstat_t *);
extern void		di_mute_on(curstat_t *);
extern void		di_mute_off(curstat_t *);
extern void		di_cddajitter(curstat_t *);
extern void		di_debug(void);
extern void		di_start(curstat_t *);
extern void		di_icon(curstat_t *, bool_t);
extern void		di_halt(curstat_t *);
extern void		di_dump_curstat(curstat_t *);
extern void		di_common_parmload(char *, bool_t, bool_t);
extern void		di_devspec_parmload(char *, bool_t, bool_t);
extern void		di_common_parmsave(char *);
extern void		di_devspec_parmsave(char *);
extern bool_t		di_devlock(curstat_t *, char *);
extern void		di_devunlock(curstat_t *);
extern di_dev_t		*di_devalloc(char *);
extern void		di_devfree(di_dev_t *);
extern char		*di_methodstr(void);
extern bool_t		di_isdemo(void);
extern int		di_curtrk_pos(curstat_t *);
extern int		di_curprog_pos(curstat_t *);
extern void		di_clear_cdinfo(curstat_t *, bool_t);
extern byte_t		di_get_cdinfo(curstat_t *);
extern bool_t		di_prepare_cdda(curstat_t *);
extern void		di_reset_curstat(curstat_t *, bool_t, bool_t);
extern void		di_reset_shuffle(curstat_t *);

#endif	/* __LIBDI_H__ */

