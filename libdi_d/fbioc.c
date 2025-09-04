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
 * FreeBSD/NetBSD/OpenBSD ioctl method module
 *
 * Contributing author: Gennady B. Sorokopud
 * E-mail: gena@NetVision.net.il
 *
 * Older NetBSD systems may need -DNETBSD_OLDIOC to compile.
 */

#ifndef lint
static char *_fbioc_c_ident_ = "@(#)fbioc.c	6.199 04/04/08";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/fbioc.h"
#include "cdda_d/cdda.h"

#if defined(DI_FBIOC) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;
extern sword32_t	di_clip_frames;


STATIC bool_t	fbioc_pause_resume(bool_t),
		fbioc_run_ab(curstat_t *),
		fbioc_run_sample(curstat_t *),
		fbioc_run_prog(curstat_t *),
		fbioc_run_repeat(curstat_t *),
		fbioc_disc_ready(curstat_t *),
		fbioc_disc_present(bool_t);
STATIC int	fbioc_cfg_vol(int, curstat_t *, bool_t);
STATIC void	fbioc_stat_poll(curstat_t *),
		fbioc_insert_poll(curstat_t *);


STATIC di_dev_t	*fbioc_devp = NULL;		/* CD device descriptor */
STATIC int	fbioc_stat_interval,		/* Status poll interval */
		fbioc_ins_interval;		/* Insert poll interval */
STATIC long	fbioc_stat_id,			/* Play status poll timer id */
		fbioc_insert_id,		/* Disc insert poll timer id */
		fbioc_search_id;		/* FF/REW timer id */
STATIC byte_t	fbioc_tst_status = MOD_NODISC;	/* Playback status on load */
STATIC bool_t	fbioc_not_open = TRUE,		/* Device not opened yet */
		fbioc_stat_polling,		/* Polling play status */
		fbioc_insert_polling,		/* Polling disc insert */
		fbioc_new_progshuf,		/* New program/shuffle seq */
		fbioc_start_search,		/* Start FF/REW play segment */
		fbioc_idx_pause,		/* Prev/next index pausing */
		fbioc_fake_stop,		/* Force a completion status */
		fbioc_playing,			/* Currently playing */
		fbioc_paused,			/* Currently paused */
		fbioc_bcd_hack,			/* Track numbers in BCD hack */
		fbioc_override_ap,		/* Override auto-play */
		fbioc_use_pread;		/* Use pread(2) to read CDDA */
STATIC sword32_t fbioc_sav_end_addr;		/* Err recov saved end addr */
STATIC word32_t	fbioc_next_sam;			/* Next SAMPLE track */
STATIC msf_t	fbioc_sav_end_msf;		/* Err recov saved end MSF */
STATIC byte_t	fbioc_sav_end_fmt;		/* Err recov saved end fmt */
STATIC cdda_client_t fbioc_cdda_client;		/* CDDA client struct */


/* FreeBSD/NetBSD/OpenBSD CDROM ioctl names */
STATIC iocname_t iname[] = {
	{ CDIOCREADSUBCHANNEL,	"CDIOCREADSUBCHANNEL"	},
	{ CDIOREADTOCHEADER,	"CDIOREADTOCHEADER"	},
	{ CDIOREADTOCENTRYS,	"CDIOREADTOCENTRYS"	},
	{ CDIOCEJECT,		"CDIOCEJECT"		},
	{ CDIOCSTART,		"CDIOCSTART"		},
	{ CDIOCSTOP,		"CDIOCSTOP"		},
	{ CDIOCPAUSE,		"CDIOCPAUSE"		},
	{ CDIOCRESUME,		"CDIOCRESUME"		},
	{ CDIOCGETVOL,		"CDIOCGETVOL"		},
	{ CDIOCSETVOL,		"CDIOCSETVOL"		},
	{ CDIOCPLAYTRACKS,	"CDIOCPLAYTRACKS"	},
	{ CDIOCPLAYMSF,		"CDIOCPLAYMSF"		},
	{ CDIOCALLOW,		"CDIOCALLOW"		},
	{ CDIOCPREVENT,		"CDIOCPREVENT"		},
	{ CDIOCREADAUDIO,	"CDIOCREADAUDIO"	},
	{ 0,			NULL			},
};


/***********************
 *  internal routines  *
 ***********************/


/*
 * fbioc_close
 *	Close CD device
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_close(void)
{
	if (fbioc_devp != NULL) {
		DBGPRN(DBG_DEVIO)(errfp, "\nClose device: %s\n",
				  fbioc_devp->path);

		if (fbioc_devp->fd > 0)
			(void) close(fbioc_devp->fd);
		di_devfree(fbioc_devp);
		fbioc_devp = NULL;
	}
}


/*
 * fbioc_open
 *	Open CD device
 *
 * Args:
 *	path - device path name string
 *
 * Return:
 *	TRUE - open successful
 *	FALSE - open failed
 */
STATIC bool_t
fbioc_open(char *path)
{
	struct stat	stbuf;
	char		errstr[ERR_BUF_SZ];

	DBGPRN(DBG_DEVIO)(errfp, "\nOpen device: %s\n", path);

	/* Check for validity of device node */
	if (stat(path, &stbuf) < 0) {
		(void) sprintf(errstr, app_data.str_staterr, path);
		DI_FATAL(errstr);
		return FALSE;
	}

	if (!S_ISCHR(stbuf.st_mode)) {
		(void) sprintf(errstr, app_data.str_noderr, path);
		DI_FATAL(errstr);
		return FALSE;
	}

	if ((fbioc_devp = di_devalloc(path)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	if ((fbioc_devp->fd = open(path, O_RDONLY | O_EXCL)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", path, errno);
		di_devfree(fbioc_devp);
		fbioc_devp = NULL;
		return FALSE;
	}

	return TRUE;
}


/*
 * fbioc_init_vol
 *	Initialize volume, balance and channel routing controls
 *	to match the settings in the hardware, or to the preset
 *	value, if specified.
 *
 * Args:
 *	s      - Pointer to the curstat_t structure.
 *	preset - If a preset value is configured, whether to force to
 *               the preset.
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_init_vol(curstat_t *s, bool_t preset)
{
	int	vol;

	/* Query current volume/balance settings */
	if ((vol = fbioc_cfg_vol(0, s, TRUE)) >= 0)
		s->level = (byte_t) vol;
	else
		s->level = 0;

	/* Set volume to preset value, if so configured */
	if (app_data.startup_vol > 0 && preset) {
		s->level_left = s->level_right = 100;

		if ((vol = fbioc_cfg_vol(app_data.startup_vol, s, FALSE)) >= 0)
			s->level = (byte_t) vol;
	}

	/* Initialize sliders */
	SET_VOL_SLIDER(s->level);
	SET_BAL_SLIDER((int) (s->level_right - s->level_left) / 2);

	/* Set up channel routing */
	fbioc_route(s);
}


/*
 * fbioc_rdsubq
 *	Send Read Subchannel command to the device
 *
 * Args:
 *	sp - Pointer to the caller-supplied cdstat_t return structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_rdsubq(cdstat_t *sp)
{
	struct ioc_read_subchannel	subchnl;
	struct cd_sub_channel_info	sub;
	bool_t				ret;

	memset((byte_t *) &sub, 0, sizeof(struct cd_sub_channel_info));

	subchnl.address_format = app_data.subq_lba ?
		CD_LBA_FORMAT : CD_MSF_FORMAT;
	subchnl.data_format = CD_CURRENT_POSITION;
	subchnl.track = 0;
	subchnl.data_len = sizeof(struct cd_sub_channel_info);
	subchnl.data = &sub;

	DBGDUMP(DBG_DEVIO)("ioc_read_subchannel data bytes",
		(byte_t *) &subchnl, sizeof(struct ioc_read_subchannel));

	ret = fbioc_send(DI_ROLE_MAIN, CDIOCREADSUBCHANNEL, &subchnl, TRUE);

	if (!ret)
		return FALSE;

	DBGDUMP(DBG_DEVIO)("cd_sub_channel_info data bytes", (byte_t *) &sub,
		sizeof(struct cd_sub_channel_info));

	/* Hack: to work around firmware anomalies in some CD drives. */
	if (sub.what.position.track_number >= MAXTRACK &&
	    sub.what.position.track_number != LEAD_OUT_TRACK) {
		sp->status = CDSTAT_NOSTATUS;
		return TRUE;
	}

	/* Map the subchannel data into cdstat_t form */

	switch (sub.header.audio_status) {
	case CD_AS_PLAY_IN_PROGRESS:
		sp->status = CDSTAT_PLAYING;
		break;
	case CD_AS_PLAY_PAUSED:
		sp->status = CDSTAT_PAUSED;
		break;
	case CD_AS_PLAY_COMPLETED:
		sp->status = CDSTAT_COMPLETED;
		break;
	case CD_AS_PLAY_ERROR:
		sp->status = CDSTAT_FAILED;
		break;
	case CD_AS_AUDIO_INVALID:
	case CD_AS_NO_STATUS:
	default:
		sp->status = CDSTAT_NOSTATUS;
		break;
	}

	if (fbioc_bcd_hack) {
		/* Hack: BUGLY CD drive firmware */
		sp->track = (int) util_bcdtol(sub.what.position.track_number);
		sp->index = (int) util_bcdtol(sub.what.position.index_number);
	}
	else {
		sp->track = (int) sub.what.position.track_number;
		sp->index = (int) sub.what.position.index_number;
	}

	if (app_data.subq_lba) {
		/* LBA mode */
#ifdef NETBSD_OLDIOC
		sp->abs_addr.addr = util_xlate_blk((sword32_t)
			(sub.what.position.absaddr[3] << 24) |
			(sub.what.position.absaddr[2] << 16) |
			(sub.what.position.absaddr[1] << 8) |
			sub.what.position.absaddr[0]
		);
		sp->rel_addr.addr = util_xlate_blk((sword32_t)
			(sub.what.position.reladdr[3] << 24) |
			(sub.what.position.reladdr[2] << 16) |
			(sub.what.position.reladdr[1] << 8) |
			sub.what.position.reladdr[0]
		);
#else
		sp->abs_addr.addr = util_xlate_blk((sword32_t)
			util_bswap32(sub.what.position.absaddr.lba)
		);
		sp->rel_addr.addr = util_xlate_blk((sword32_t)
			util_bswap32(sub.what.position.reladdr.lba)
		);
#endif
		util_blktomsf(
			sp->abs_addr.addr,
			&sp->abs_addr.min,
			&sp->abs_addr.sec,
			&sp->abs_addr.frame,
			MSF_OFFSET
		);
		util_blktomsf(
			sp->rel_addr.addr,
			&sp->rel_addr.min,
			&sp->rel_addr.sec,
			&sp->rel_addr.frame,
			0
		);
	}
	else {
		/* MSF mode */
#ifdef NETBSD_OLDIOC
		sp->abs_addr.min = sub.what.position.absaddr[1];
		sp->abs_addr.sec = sub.what.position.absaddr[2];
		sp->abs_addr.frame = sub.what.position.absaddr[3];

		sp->rel_addr.min = sub.what.position.reladdr[1];
		sp->rel_addr.sec = sub.what.position.reladdr[2];
		sp->rel_addr.frame = sub.what.position.reladdr[3];
#else
		sp->abs_addr.min = sub.what.position.absaddr.msf.minute;
		sp->abs_addr.sec = sub.what.position.absaddr.msf.second;
		sp->abs_addr.frame = sub.what.position.absaddr.msf.frame;

		sp->rel_addr.min = sub.what.position.reladdr.msf.minute;
		sp->rel_addr.sec = sub.what.position.reladdr.msf.second;
		sp->rel_addr.frame = sub.what.position.reladdr.msf.frame;
#endif
		util_msftoblk(
			sp->abs_addr.min,
			sp->abs_addr.sec,
			sp->abs_addr.frame,
			&sp->abs_addr.addr,
			MSF_OFFSET
		);
		util_msftoblk(
			sp->rel_addr.min,
			sp->rel_addr.sec,
			sp->rel_addr.frame,
			&sp->rel_addr.addr,
			0
		);
	}

	return (ret);
}


/*
 * fbioc_rdtoc
 *	Send Read TOC command to the device
 *
 * Args:
 *	buf - Pointer to the return data toc header
 *	h - address of pointer to array of toc entrys will be allocated
 *	    by this routine
 *	start - Starting track number for which the TOC data is returned
 *	msf - Whether to use MSF or logical block address data format
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_rdtoc(struct ioc_toc_header *h, struct cd_toc_entry **e,
	    int start, bool_t msf)
{
	int				i,
					j,
					k,
					allocsz;
	struct cd_toc_entry		*t;
	struct ioc_read_toc_entry	tocentry;

	/* Read the TOC header first */
	if (!fbioc_send(DI_ROLE_MAIN, CDIOREADTOCHEADER, h, TRUE))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("ioc_toc_header data bytes", (byte_t *) h,
		sizeof(struct ioc_toc_header));

	if (start == 0)
		start = h->starting_track;

	if (start > (int) h->ending_track)
		return FALSE;

	allocsz = (h->ending_track - start + 2) * sizeof(struct cd_toc_entry);

	*e = (struct cd_toc_entry *)(void *) MEM_ALLOC(
		"cd_toc_entry",
		allocsz
	);
	t = (struct cd_toc_entry *)(void *) MEM_ALLOC(
		"cd_toc_entry",
		allocsz
	);

	if (*e == NULL || t == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	(void) memset((byte_t *) *e, 0, allocsz);
	(void) memset((byte_t *) t, 0, allocsz);
	tocentry.starting_track = start;
	tocentry.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	tocentry.data_len = allocsz;
	tocentry.data = t;

	DBGDUMP(DBG_DEVIO)("ioc_read_toc_entry data bytes",
		(byte_t *) &tocentry, sizeof( struct ioc_read_toc_entry));

	if (!fbioc_send(DI_ROLE_MAIN, CDIOREADTOCENTRYS, &tocentry, TRUE)) {
		MEM_FREE(*e);
		MEM_FREE(t);
		return FALSE;
	}

	DBGDUMP(DBG_DEVIO)("cd_toc_entry data bytes", (byte_t *) t, allocsz);

	for (i = start; i <= (int) (h->ending_track + 1); i++) {
		j = i - start;

		/* Hack: workaround CD drive firmware bug
		 * Some CD drives return track numbers in BCD
		 * rather than binary.
		 */
#ifdef NETBSD_OLDIOC
		if ((int) (t[j].track & 0xf) > 0x9 &&
		    (int) (t[j].track & 0xf) < 0x10 &&
		    t[j].addr[0] == 0 && t[j].addr[1] == 0 &&
		    t[j].addr[2] == 0 && t[j].addr[3] == 0)
#else
		if ((int) (t[j].track & 0xf) > 0x9 &&
		    (int) (t[j].track & 0xf) < 0x10 &&
		    t[j].addr.lba == 0)
#endif
		{
			/* BUGLY CD drive firmware detected! */
			fbioc_bcd_hack = TRUE;
		}

		/* Sanity check */
#ifdef NETBSD_OLDIOC
		if (t[j].track == LEAD_OUT_TRACK &&
		    t[j].addr[0] == t[0].addr[0] &&
		    t[j].addr[1] == t[1].addr[1] &&
		    t[j].addr[2] == t[2].addr[2] &&
		    t[j].addr[3] == t[3].addr[3])
#else
		if (t[j].track == LEAD_OUT_TRACK &&
		    t[j].addr.lba == t[0].addr.lba)
#endif
		{
			MEM_FREE(*e);
			MEM_FREE(t);
			return FALSE;
		}
	}

	/* Fix up TOC data */
	for (i = start; i <= (int) (h->ending_track + 1); i++) {
		if (fbioc_bcd_hack && (i & 0xf) > 0x9 && (i & 0xf) < 0x10)
			continue;

		j = i - start;
		k = (fbioc_bcd_hack ? util_bcdtol(i) : i) - start;

		if (i == (h->ending_track + 1)) {
			/* FreeBSD HACK: Fix up lead-out track number */
			t[j].track = LEAD_OUT_TRACK;
		}

		(*e)[k].control = t[j].control;
		(*e)[k].addr_type = t[j].addr_type;
#ifdef NETBSD_OLDIOC
		(*e)[k].addr[0] = t[j].addr[0];
		(*e)[k].addr[1] = t[j].addr[1];
		(*e)[k].addr[2] = t[j].addr[2];
		(*e)[k].addr[3] = t[j].addr[3];
#else
		(*e)[k].addr.lba = t[j].addr.lba;
#endif
		if (t[j].track == LEAD_OUT_TRACK) {
			(*e)[k].track = t[j].track;
			break;
		}
		else {
			if (fbioc_bcd_hack)
				(*e)[k].track = (byte_t)
					util_bcdtol(t[j].track);
			else
				(*e)[k].track = t[j].track;
		}
	}

	if (fbioc_bcd_hack)
		h->ending_track = (byte_t) util_bcdtol(h->ending_track);

	MEM_FREE(t);

	return TRUE;
}


/*
 * fbioc_disc_present
 *	Check if a CD is loaded.
 *
 * Args:
 *	savstat - Whether to save start-up status in fbioc_tst_status.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure (drive not ready)
 */
STATIC bool_t
fbioc_disc_present(bool_t savstat)
{
	int				allocsz;
	sword32_t			a1,
					a2;
	struct cd_toc_entry		*e;
	struct cd_sub_channel_info	sub;
	struct ioc_read_subchannel	subchnl;
	struct ioc_toc_header		h;
	struct ioc_read_toc_entry	tocentry;
	static int			tot_trks = 0;
	static sword32_t		sav_a1 = 0,
					sav_a2 = 0;

	if (savstat)
		fbioc_tst_status = MOD_NODISC;

	/* Fake it with CDIOCREADSUBCHANNEL */
	(void) memset((byte_t *) &sub, 0, sizeof(struct cd_sub_channel_info));
	subchnl.address_format = CD_MSF_FORMAT;
	subchnl.data_format = CD_CURRENT_POSITION;
	subchnl.track = 0;
	subchnl.data_len = sizeof(struct cd_sub_channel_info);
	subchnl.data = &sub;

	if (!fbioc_send(DI_ROLE_MAIN, CDIOCREADSUBCHANNEL, &subchnl,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)))
		return FALSE;

	switch (sub.header.audio_status) {
	case CD_AS_PLAY_IN_PROGRESS:
		if (savstat) {
			DBGPRN(DBG_DEVIO)(errfp,
				"\nstatus=CD_AS_PLAY_IN_PROGRESS\n");
			fbioc_tst_status = MOD_PLAY;
			return TRUE;
		}
		break;
	case CD_AS_PLAY_PAUSED:
		if (savstat) {
			DBGPRN(DBG_DEVIO)(errfp,
				"\nstatus=CD_AS_PLAY_PAUSED\n");
			fbioc_tst_status = MOD_PAUSE;
			return TRUE;
		}
		break;
	case CD_AS_PLAY_ERROR:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CD_AS_PLAY_ERROR\n");
		break;
	case CD_AS_PLAY_COMPLETED:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CD_AS_PLAY_COMPLETED\n");
		break;
	case CD_AS_NO_STATUS:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CD_AS_NO_STATUS\n");
		break;
	case CD_AS_AUDIO_INVALID:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CD_AS_AUDIO_INVALID\n");
		break;
	default:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=unknown (%d)\n",
			sub.header.audio_status);
		return FALSE;
	}

	if (savstat)
		fbioc_tst_status = MOD_STOP;

	/* CDIOCREADSUBCHANNEL didn't give useful info.
	 * Try CDIOREADTOCHEADER and CDIOREADTOCENTRYS.
	 */
	(void) memset((byte_t *) &h, 0, sizeof(struct ioc_toc_header));

	if (!fbioc_send(DI_ROLE_MAIN, CDIOREADTOCHEADER, &h,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0)))
		return FALSE;

	if (h.starting_track == 0 && h.ending_track == 0)
		return FALSE;

	if ((h.ending_track - h.starting_track + 1) != tot_trks) {
		/* Disc changed */
		tot_trks = h.ending_track - h.starting_track + 1;
		return FALSE;
	}

	allocsz = (tot_trks + 1) * sizeof(struct cd_toc_entry);
	e = (struct cd_toc_entry *)(void *) MEM_ALLOC("cd_toc_entry", allocsz);
	if (e == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	(void) memset((byte_t *) e, 0, allocsz);
	tocentry.starting_track = (unsigned char) h.starting_track;
	tocentry.address_format = CD_MSF_FORMAT;
	tocentry.data_len = allocsz;
	tocentry.data = e;

	if (!fbioc_send(DI_ROLE_MAIN, CDIOREADTOCENTRYS, &tocentry,
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0))) {
		MEM_FREE(e);
		return FALSE;
	}

#ifdef NETBSD_OLDIOC
	a1 = (sword32_t) (
		(e[h.starting_track - 1].addr[3] << 24) |
		(e[h.starting_track - 1].addr[2] << 16) |
		(e[h.starting_track - 1].addr[1] << 8) |
		(e[h.starting_track - 1].addr[0])
	);
	a2 = (sword32_t) (
		(e[h.ending_track - 1].addr[3] << 24) |
		(e[h.ending_track - 1].addr[2] << 16) |
		(e[h.ending_track - 1].addr[1] << 8) |
		(e[h.ending_track - 1].addr[0])
	);
#else
	a1 = (sword32_t) e[h.starting_track - 1].addr.lba;
	a2 = (sword32_t) e[h.ending_track - 1].addr.lba;
#endif

	DBGPRN(DBG_DEVIO)(errfp, "\na1=0x%x a2=0x%x\n", a1, a2);
	MEM_FREE(e);

	if (a1 != sav_a1 || a2 != sav_a2) {
		/* Disc changed */
		sav_a1 = a1;
		sav_a2 = a2;
		return FALSE;
	}

	if (tot_trks > 1 && a1 == a2)
		return FALSE;

	return TRUE;
}


/*
 * fbioc_playmsf
 *	Send Play Audio MSF command to the device
 *
 * Args:
 *	start - Pointer to the starting position MSF data
 *	end - Pointer to the ending position MSF data
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_playmsf(msf_t *start, msf_t *end)
{
	struct ioc_play_msf	m;

	/* If the start or end positions are less than the minimum
	 * position, patch them to the minimum positions.
	 */
	if (start->min == 0 && start->sec < 2) {
		m.start_m = 0;
		m.start_s = 2;
		m.start_f = 0;
	}
	else {
		m.start_m = start->min;
		m.start_s = start->sec;
		m.start_f = start->frame;
	}

	if (end->min == 0 && end->sec < 2) {
		m.end_m = 0;
		m.end_s = 2;
		m.end_f = 0;
	}
	else {
		m.end_m = end->min;
		m.end_s = end->sec;
		m.end_f = end->frame;
	}

	/* If start == end, just return success */
	if (m.start_m == m.end_m &&
	    m.start_s == m.end_s &&
	    m.start_f == m.end_f)
		return TRUE;

	DBGDUMP(DBG_DEVIO)("ioc_play_msf data bytes", (byte_t *) &m,
		sizeof(struct ioc_play_msf));

	return (fbioc_send(DI_ROLE_MAIN, CDIOCPLAYMSF, &m, TRUE));
}


/*
 * fbioc_play_trkidx
 *	Send Play Audio Track/Index command to the device
 *
 * Args:
 *	start_trk - Starting track number
 *	start_idx - Starting index number
 *	end_trk - Ending track number
 *	end_idx - Ending index number
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
fbioc_play_trkidx(int start_trk, int start_idx, int end_trk, int end_idx)
{
	struct ioc_play_track	t;

	if (fbioc_bcd_hack) {
		/* Hack: BUGLY CD drive firmware */
		t.start_track = util_ltobcd(start_trk);
		t.start_index = util_ltobcd(start_idx);
		t.end_track = util_ltobcd(end_trk);
		t.end_index = util_ltobcd(end_idx);
	}
	else {
		t.start_track = start_trk;
		t.start_index = start_idx;
		t.end_track = end_trk;
		t.end_index = end_idx;
	}

	DBGDUMP(DBG_DEVIO)("ioc_play_track data bytes", (byte_t *) &t,
		sizeof(struct ioc_play_track));

	return (fbioc_send(DI_ROLE_MAIN, CDIOCPLAYTRACKS, &t, TRUE));
}


/*
 * fbioc_start_stop
 *	Send Start/Stop Unit command to the device
 *
 * Args:
 *	start - Whether to start unit or stop unit
 *	loej - Whether caddy load/eject operation should be performed
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_start_stop(bool_t start, bool_t loej)
{
	bool_t		ret;
	curstat_t	*s = di_clinfo->curstat_addr();

	if (app_data.strict_pause_resume && fbioc_paused)
		(void) fbioc_pause_resume(TRUE);

	if (start) {
		if (loej)
			ret = FALSE;
		else
			ret = fbioc_send(DI_ROLE_MAIN, CDIOCSTART, NULL, TRUE);
	}
	else {
		fbioc_playing = FALSE;

		if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
			(void) cdda_stop(fbioc_devp, s);

			if (!fbioc_is_enabled(DI_ROLE_MAIN)) {
				/* Enable I/O from current process */
				fbioc_enable(DI_ROLE_MAIN);
			}
		}

		if (loej)
			ret = fbioc_send(DI_ROLE_MAIN, CDIOCEJECT, NULL, TRUE);
		else
			ret = fbioc_send(DI_ROLE_MAIN, CDIOCSTOP, NULL, TRUE);
	}

	/* Delay a bit to let the CD load or eject.  This is a hack to
	 * work around firmware bugs in some CD drives.  These drives
	 * don't handle new commands well when the CD is loading/ejecting
	 * with the IMMED bit set in the Start/Stop Unit command.
	 */
	if (ret) {
		if (loej) {
			int	n;

			n = (app_data.ins_interval + 1000 - 1) / 1000;
			if (start)
				n *= 2;

			util_delayms(n * 1000);
		}
		else if (start && app_data.spinup_interval > 0)
			util_delayms(app_data.spinup_interval * 1000);
	}

	return (ret);

}


/*
 * fbioc_pause_resume
 *	Send Pause/Resume command to the device
 *
 * Args:
 *	resume - Whether to resume or pause
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_pause_resume(bool_t resume)
{
	bool_t		ret;
	curstat_t	*s = di_clinfo->curstat_addr();

	if (!app_data.pause_supp)
		return FALSE;

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		ret = cdda_pause_resume(fbioc_devp, s, resume);
	}
	else {
		ret = fbioc_send(
			DI_ROLE_MAIN, resume ? CDIOCRESUME : CDIOCPAUSE,
			NULL, TRUE
		);
	}
	if (ret)
		fbioc_paused = !resume;

	return (ret);
}


/*
 * fbioc_do_playaudio
 *	General top-level play audio function
 *
 * Args:
 *	addr_fmt - The address formats specified:
 *		ADDR_BLK: logical block address (not supported)
 *		ADDR_MSF: MSF address
 *		ADDR_TRKIDX: Track/index numbers
 *		ADDR_OPTEND: Ending address can be ignored
 *	start_addr - Starting logical block address (not supported)
 *	end_addr - Ending logical block address (not supported)
 *	start_msf - Pointer to start address MSF data
 *	end_msf - Pointer to end address MSF data
 *	trk - Starting track number
 *	idx - Starting index number
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_do_playaudio(
	byte_t		addr_fmt,
	sword32_t	start_addr,
	sword32_t	end_addr,
	msf_t		*start_msf,
	msf_t		*end_msf,
	byte_t		trk,
	byte_t		idx
)
{
	msf_t		emsf,
			*emsfp = NULL;
	sword32_t	tmp_saddr,
			tmp_eaddr;
	bool_t		ret = FALSE,
			do_playmsf,
			do_play10,
			do_play12,
			do_playti;
	curstat_t	*s = di_clinfo->curstat_addr();

	/* Fix addresses: Some CD drives will only allow playing to
	 * the last frame minus a few frames.
	 */
	if ((addr_fmt & ADDR_MSF) && end_msf != NULL) {
		emsf = *end_msf;	/* Structure copy */
		emsfp = &emsf;

		util_msftoblk(
			start_msf->min,
			start_msf->sec,
			start_msf->frame,
			&tmp_saddr,
			MSF_OFFSET
		);

		util_msftoblk(
			emsfp->min,
			emsfp->sec,
			emsfp->frame,
			&tmp_eaddr,
			MSF_OFFSET
		);

		if (tmp_eaddr > s->discpos_tot.addr)
			tmp_eaddr = s->discpos_tot.addr;

		if (tmp_eaddr >= di_clip_frames)
			tmp_eaddr -= di_clip_frames;
		else
			tmp_eaddr = 0;

		util_blktomsf(
			tmp_eaddr,
			&emsfp->min,
			&emsfp->sec,
			&emsfp->frame,
			MSF_OFFSET
		);

		if (tmp_saddr >= tmp_eaddr)
			return FALSE;

		emsfp->res = start_msf->res = 0;

		/* Save end address for error recovery */
		fbioc_sav_end_msf = *end_msf;
	}
	if (addr_fmt & ADDR_BLK) {
		if (end_addr > s->discpos_tot.addr)
			end_addr = s->discpos_tot.addr;

		if (end_addr >= di_clip_frames)
			end_addr -= di_clip_frames;
		else
			end_addr = 0;

		if (start_addr >= end_addr)
			return FALSE;

		/* Save end address for error recovery */
		fbioc_sav_end_addr = end_addr;
	}

	/* Save end address format for error recovery */
	fbioc_sav_end_fmt = addr_fmt;

	do_playmsf = (addr_fmt & ADDR_MSF) && app_data.playmsf_supp;
	do_play10 = (addr_fmt & ADDR_BLK) && app_data.play10_supp;
	do_play12 = (addr_fmt & ADDR_BLK) && app_data.play12_supp;
	do_playti = (addr_fmt & ADDR_TRKIDX) && app_data.playti_supp;

	if (do_playmsf || do_playti) {
		if (fbioc_paused) {
			if (app_data.strict_pause_resume) {
				/* Resume first */
				(void) fbioc_pause_resume(TRUE);
			}
		}
		else if (fbioc_playing) {
			if (app_data.play_pause_play) {
				/* Pause first */
				(void) fbioc_pause_resume(FALSE);
			}
		}
		else {
			/* Spin up CD */
			(void) fbioc_start_stop(TRUE, FALSE);
		}
	}

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		if (do_play12 || do_play10) {
			if (fbioc_is_enabled(DI_ROLE_MAIN)) {
				/* Disable I/O from current process */
				fbioc_disable(DI_ROLE_MAIN);
			}

			ret = cdda_play(fbioc_devp, s, start_addr, end_addr);
		}
		else if (do_playmsf) {
			util_msftoblk(
				start_msf->min,
				start_msf->sec,
				start_msf->frame,
				&start_addr,
				MSF_OFFSET
			);
			util_msftoblk(
				emsfp->min,
				emsfp->sec,
				emsfp->frame,
				&end_addr,
				MSF_OFFSET
			);
			
			ret = cdda_play(fbioc_devp, s, start_addr, end_addr);
		}
	}
	else {
		if (do_playmsf)
			ret = fbioc_playmsf(start_msf, emsfp);
	
		if (!ret && do_playti)
			ret = fbioc_play_trkidx(trk, idx, trk, idx);
	}

	if (ret) {
		fbioc_playing = TRUE;
		fbioc_paused = FALSE;
	}

	return (ret);
}


/*
 * fbioc_play_recov
 *	Playback interruption recovery handler: Restart playback after
 *	skipping some frames.
 *
 * Args:
 *	blk   - Interruption frame address
 *	iserr - Whether interruption is due to an error
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_play_recov(sword32_t blk, bool_t iserr)
{
	msf_t		recov_start_msf;
	sword32_t	recov_start_addr;
	bool_t		ret;

	ret = TRUE;

	recov_start_addr = blk + ERR_SKIPBLKS;
	util_blktomsf(
		recov_start_addr,
		&recov_start_msf.min,
		&recov_start_msf.sec,
		&recov_start_msf.frame,
		MSF_OFFSET
	);

	/* Check to see if we have skipped past
	 * the end.
	 */
	if (recov_start_msf.min > fbioc_sav_end_msf.min)
		ret = FALSE;
	else if (recov_start_msf.min == fbioc_sav_end_msf.min) {
		if (recov_start_msf.sec > fbioc_sav_end_msf.sec)
			ret = FALSE;
		else if ((recov_start_msf.sec ==
			  fbioc_sav_end_msf.sec) &&
			 (recov_start_msf.frame >
			  fbioc_sav_end_msf.frame)) {
			ret = FALSE;
		}
	}
	if (recov_start_addr >= fbioc_sav_end_addr)
		ret = FALSE;

	if (ret) {
		/* Restart playback */
		if (iserr) {
			(void) fprintf(errfp,
				"CD audio: %s (%02u:%02u.%02u)\n",
				app_data.str_recoverr,
				recov_start_msf.min,
				recov_start_msf.sec,
				recov_start_msf.frame
			);
		}

		ret = fbioc_do_playaudio(
			fbioc_sav_end_fmt,
			recov_start_addr, fbioc_sav_end_addr,
			&recov_start_msf, &fbioc_sav_end_msf,
			0, 0
		);
	}

	return (ret);
}


/*
 * fbioc_get_playstatus
 *	Obtain and update current playback status information
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - Audio playback is in progress
 *	FALSE - Audio playback stopped or command failure
 */
STATIC bool_t
fbioc_get_playstatus(curstat_t *s)
{
	cdstat_t		cdstat;
	word32_t		curtrk,
				curidx;
	bool_t			ret,
				done;
	static int		errcnt = 0,
				nostatcnt = 0;
	static sword32_t	errblk = 0;
	static bool_t		in_fbioc_get_playstatus = FALSE;

	/* Lock this routine from multiple entry */
	if (in_fbioc_get_playstatus)
		return TRUE;

	in_fbioc_get_playstatus = TRUE;

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		ret = cdda_getstatus(fbioc_devp, s, &cdstat);
		if (ret &&
		    (cdstat.level != s->level ||
		     cdstat.level_left  != s->level_left ||
		     cdstat.level_right != s->level_right)) {
			int	vol;

			/* Update volume & balance level */
			s->level_left = cdstat.level_left;
			s->level_right = cdstat.level_right;
			vol = fbioc_cfg_vol((int) cdstat.level, s, FALSE);
			if (vol >= 0) {
				s->level = vol;
				SET_VOL_SLIDER(s->level);
				SET_BAL_SLIDER((int)
					(s->level_right - s->level_left) / 2
				);
			}
		}
	}
	else
		ret = fbioc_rdsubq(&cdstat);

	if (!ret) {
		/* Check to see if the disc had been manually ejected */
		if (!fbioc_disc_ready(s)) {
			fbioc_sav_end_addr = 0;
			fbioc_sav_end_msf.min = 0;
			fbioc_sav_end_msf.sec = 0;
			fbioc_sav_end_msf.frame = 0;
			fbioc_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_fbioc_get_playstatus = FALSE;
			return FALSE;
		}

		/* The read subchannel command failed for some
		 * unknown reason.  Just return success and
		 * hope the next poll succeeds.  We don't want
		 * to return FALSE here because that would stop
		 * the poll.
		 */
		in_fbioc_get_playstatus = FALSE;
		return TRUE;
	}

	curtrk = cdstat.track;
	curidx = cdstat.index;
	s->curpos_tot = cdstat.abs_addr;	/* structure copy */
	s->curpos_trk = cdstat.rel_addr;	/* structure copy */

	s->tot_frm = cdstat.tot_frm;
	s->frm_played = cdstat.frm_played;
	s->frm_per_sec = cdstat.frm_per_sec;

	/* Update time display */
	DPY_TIME(s, FALSE);

	if (curtrk != s->cur_trk) {
		s->cur_trk = curtrk;
		/* Update track number display */
		DPY_TRACK(s);
	}

	if (curidx != s->cur_idx) {
		s->cur_idx = curidx;
		s->sav_iaddr = s->curpos_tot.addr;
		/* Update index number display */
		DPY_INDEX(s);
	}

	/* Update play mode display */
	DPY_PLAYMODE(s, FALSE);

	/* Hack: to work around the fact that some CD drives return
	 * CD_AS_PLAY_PAUSED status after issuing a Stop Unit command.
	 * Just treat the status as completed if we get a paused status
	 * and we don't expect the drive to be paused.
	 */
	if (cdstat.status == CDSTAT_PAUSED && s->mode != MOD_PAUSE &&
	    !fbioc_idx_pause)
		cdstat.status = CDSTAT_COMPLETED;

	/* Force completion status */
	if (fbioc_fake_stop)
		cdstat.status = CDSTAT_COMPLETED;

	/* Deal with playback status */
	switch (cdstat.status) {
	case CDSTAT_PLAYING:
	case CDSTAT_PAUSED:
		nostatcnt = 0;
		done = FALSE;

		/* If we haven't encountered an error for a while, then
		 * clear the error count.
		 */
		if (errcnt > 0 &&
		    (s->curpos_tot.addr - errblk) > ERR_CLRTHRESH)
			errcnt = 0;
		break;

	case CDSTAT_FAILED:
		nostatcnt = 0;
		/* Check to see if the disc had been manually ejected */
		if (!fbioc_disc_ready(s)) {
			fbioc_sav_end_addr = 0;
			fbioc_sav_end_msf.min = 0;
			fbioc_sav_end_msf.sec = 0;
			fbioc_sav_end_msf.frame = 0;
			fbioc_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_fbioc_get_playstatus = FALSE;
			return FALSE;
		}

		/* Audio playback stopped due to a disc error.  We will
		 * try to restart the playback by skipping a few frames
		 * and continuing.  This will cause a glitch in the sound
		 * but is better than just stopping.
		 */
		done = FALSE;

		/* Check for max errors limit */
		if (++errcnt > MAX_RECOVERR) {
			done = TRUE;
			(void) fprintf(errfp, "CD audio: %s\n",
				       app_data.str_maxerr);
		}

		errblk = s->curpos_tot.addr;

		if (!done && fbioc_play_recov(errblk, TRUE)) {
			in_fbioc_get_playstatus = FALSE;
			return TRUE;
		}

		/*FALLTHROUGH*/

	case CDSTAT_COMPLETED:
	case CDSTAT_NOSTATUS:
	default:
		if (cdstat.status == CDSTAT_NOSTATUS && nostatcnt++ < 20) {
			/* Allow 20 occurrences of nostatus, then stop */
			done = FALSE;
			break;
		}
		nostatcnt = 0;
		done = TRUE;

		if (!fbioc_fake_stop)
			fbioc_playing = FALSE;

		fbioc_fake_stop = FALSE;

		switch (s->mode) {
		case MOD_SAMPLE:
			done = !fbioc_run_sample(s);
			break;

		case MOD_PLAY:
		case MOD_PAUSE:
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			if (s->shuffle || s->program)
				done = !fbioc_run_prog(s);

			if (s->repeat)
				done = !fbioc_run_repeat(s);

			if (s->repeat && s->segplay == SEGP_AB)
				done = !fbioc_run_ab(s);

			break;
		}

		break;
	}

	if (done) {
		/* Reset states */
		di_reset_curstat(s, FALSE, FALSE);
		s->mode = MOD_STOP;

		/* Cancel a->? if the user didn't select an end point */
		if (s->segplay == SEGP_A) {
			s->segplay = SEGP_NONE;
			DPY_PROGMODE(s, FALSE);
		}

		fbioc_sav_end_addr = 0;
		fbioc_sav_end_msf.min = fbioc_sav_end_msf.sec =
			fbioc_sav_end_msf.frame = 0;
		fbioc_sav_end_fmt = 0;
		errcnt = 0;
		errblk = 0;
		s->rptcnt = 0;
		DPY_ALL(s);

		if (app_data.done_eject) {
			/* Eject the disc */
			fbioc_load_eject(s);
		}
		else {
			/* Spin down the disc */
			(void) fbioc_start_stop(FALSE, FALSE);
		}
		if (app_data.done_exit) {
			/* Exit */
			di_clinfo->quit(s);
		}

		in_fbioc_get_playstatus = FALSE;
		return FALSE;
	}

	in_fbioc_get_playstatus = FALSE;
	return TRUE;
}


/*
 * fbioc_cfg_vol
 *	Audio volume control function
 *
 * Args:
 *	vol - Logical volume value to set to
 *	s - Pointer to the curstat_t structure
 *	query - If TRUE, query current volume only
 *
 * Return:
 *	The current logical volume value, or -1 on failure.
 */
STATIC int
fbioc_cfg_vol(int vol, curstat_t *s, bool_t query)
{
	int		vol1,
			vol2;
	struct ioc_vol	volctrl;
	static bool_t	first = TRUE;

	if (!app_data.mselvol_supp)
		return 0;

	if (s->mode == MOD_BUSY)
		return -1;

	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		return (cdda_vol(fbioc_devp, s, vol, query));

	(void) memset((byte_t *) &volctrl, 0, sizeof(struct ioc_vol));

	if (query) {
		if (first) {
			first = FALSE;

			/* Try using the CDIOCGETVOL ioctl */
			if (fbioc_send(DI_ROLE_MAIN, CDIOCGETVOL,
				       &volctrl, FALSE)) {
				DBGDUMP(DBG_DEVIO)("cdrom_volctrl data bytes",
					(byte_t *) &volctrl,
					sizeof(struct ioc_vol));
				vol1 = util_untaper_vol(
				    util_unscale_vol((int) volctrl.vol[0])
				);
				vol2 = util_untaper_vol(
				    util_unscale_vol((int) volctrl.vol[1])
				);

				if (vol1 == vol2) {
					s->level_left = s->level_right = 100;
					vol = vol1;
				}
				else if (vol1 > vol2) {
					s->level_left = 100;
					s->level_right =
						(byte_t)((vol2 * 100) / vol1);
					vol = vol1;
				}
				else {
					s->level_left =
						(byte_t) ((vol1 * 100) / vol2);
					s->level_right = 100;
					vol = vol2;
				}

				return (vol);
			}

			/* There is no way to read volume setting via
			 * CDROM ioctl.  Force the setting to maximum.
			 */
			vol = 100;
			s->level_left = s->level_right = 100;

			(void) fbioc_cfg_vol(vol, s, FALSE);
		}
		return (vol);
	}
	else {
		volctrl.vol[0] = util_scale_vol(
			util_taper_vol(vol * (int) s->level_left / 100)
		);
		volctrl.vol[1] = util_scale_vol(
			util_taper_vol(vol * (int) s->level_right / 100)
		);

		DBGDUMP(DBG_DEVIO)("ioc_vol data bytes", (byte_t *) &volctrl,
			sizeof(struct ioc_vol));

		if (fbioc_send(DI_ROLE_MAIN, CDIOCSETVOL, &volctrl, TRUE))
			return (vol);
		else if (volctrl.vol[0] != volctrl.vol[1]) {
			/* Set the balance to the center
			 * and retry.
			 */
			volctrl.vol[0] = volctrl.vol[1] =
				util_scale_vol(util_taper_vol(vol));

			DBGDUMP(DBG_DEVIO)("ioc_vol data bytes",
				(byte_t *) &volctrl, sizeof(struct ioc_vol));

			if (fbioc_send(DI_ROLE_MAIN, CDIOCSETVOL,
				       &volctrl, TRUE)) {
				/* Success: Warp balance control */
				s->level_left = s->level_right = 100;
				SET_BAL_SLIDER(0);

				return (vol);
			}

			/* Still failed: just drop through */
		}
	}

	return -1;
}


/*
 * fbioc_vendor_model
 *	Query and update CD drive vendor/model/revision information
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_vendor_model(curstat_t *s)
{
	/*
	 * There is currently no way to get this info,
	 * so just fill in some default info.
	 */
	(void) strcpy(s->vendor, "standard");
	(void) strcpy(s->prod, "CD-ROM drive");
	s->revnum[0] = '\0';
}


/*
 * fbioc_fix_toc
 *	CD Table Of Contents post-processing function.  This is to patch
 *	the end-of-audio position to handle "enhanced CD" or "CD Extra"
 *	multisession CDs.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_fix_toc(curstat_t *s)
{
	int	i;

	/*
	 * Set the end-of-audio to the first data track after the first
	 * track, minus 02:32:00, if applicable.  The 02:32:00 is the
	 * inter-session lead-out and lead-in time plus the 2-second
	 * pre-gap for the last session.
	 */
	for (i = 1; i < (int) s->tot_trks; i++) {
		if (s->trkinfo[i].type == TYP_DATA) {
			s->discpos_tot.addr = s->trkinfo[i].addr;
			s->discpos_tot.addr -= 11400;
			util_blktomsf(
				s->discpos_tot.addr,
				&s->discpos_tot.min,
				&s->discpos_tot.sec,
				&s->discpos_tot.frame,
				MSF_OFFSET
			);
			break;
		}
	}
}


/*
 * fbioc_get_toc
 *	Query and update the CD Table Of Contents
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_get_toc(curstat_t *s)
{
	struct ioc_toc_header	h;
	struct cd_toc_entry	*e,
				*p;
	int			i,
				ntrks;

	(void) memset((byte_t *) &h, 0, sizeof(struct ioc_toc_header));
	if (!fbioc_rdtoc(&h, &e, 0, !app_data.toc_lba))
		return FALSE;

	/* Fill curstat structure with TOC data */
	s->first_trk = h.starting_track;
	ntrks = (int) (h.ending_track - h.starting_track) + 1;

	p = e;

	for (i = 0; i <= ntrks; i++) {
		s->trkinfo[i].trkno = p->track;
		s->trkinfo[i].type =
			(p->control & 0x04) ? TYP_DATA : TYP_AUDIO;

		if (app_data.toc_lba) {
			/* LBA mode */
#ifdef NETBSD_OLDIOC
			s->trkinfo[i].addr = util_xlate_blk((sword32_t)
				(p->addr[3] << 24) | (p->addr[2] << 16) |
				(p->addr[2] << 8)  | p->addr[0]
			);
#else
			s->trkinfo[i].addr = util_xlate_blk((sword32_t)
				util_bswap32(p->addr.lba)
			);
#endif
			util_blktomsf(
				s->trkinfo[i].addr,
				&s->trkinfo[i].min,
				&s->trkinfo[i].sec,
				&s->trkinfo[i].frame,
				MSF_OFFSET
			);
		}
		else {
			/* MSF mode */
#ifdef NETBSD_OLDIOC
			s->trkinfo[i].min = p->addr[1];
			s->trkinfo[i].sec = p->addr[2];
			s->trkinfo[i].frame = p->addr[3];
#else
			s->trkinfo[i].min = p->addr.msf.minute;
			s->trkinfo[i].sec = p->addr.msf.second;
			s->trkinfo[i].frame = p->addr.msf.frame;
#endif
			util_msftoblk(
				s->trkinfo[i].min,
				s->trkinfo[i].sec,
				s->trkinfo[i].frame,
				&s->trkinfo[i].addr,
				MSF_OFFSET
			);
		}

		if (p->track == LEAD_OUT_TRACK || i == (MAXTRACK - 1)) {
			s->discpos_tot.min = s->trkinfo[i].min;
			s->discpos_tot.sec = s->trkinfo[i].sec;
			s->discpos_tot.frame = s->trkinfo[i].frame;
			s->tot_trks = (byte_t) i;
			s->discpos_tot.addr = s->trkinfo[i].addr;
			s->last_trk = s->trkinfo[i-1].trkno;

			break;
		}

		p++;
	}

	MEM_FREE(e);

	fbioc_fix_toc(s);
	return TRUE;
}


/*
 * fbioc_start_stat_poll
 *	Start polling the drive for current playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_start_stat_poll(curstat_t *s)
{
	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		fbioc_stat_id = di_clinfo->timeout(
			fbioc_stat_interval,
			fbioc_stat_poll,
			(byte_t *) s
		);

		if (fbioc_stat_id != 0)
			fbioc_stat_polling = TRUE;
	}
}


/*
 * fbioc_stop_stat_poll
 *	Stop polling the drive for current playback status
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_stop_stat_poll(void)
{
	if (fbioc_stat_polling) {
		/* Stop poll timer */
		if (di_clinfo->untimeout != NULL)
			di_clinfo->untimeout(fbioc_stat_id);

		fbioc_stat_polling = FALSE;
	}
}


/*
 * fbioc_start_insert_poll
 *	Start polling the drive for disc insertion
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_start_insert_poll(curstat_t *s)
{
	int	delay;
	bool_t	first = TRUE;

	if (fbioc_insert_polling || app_data.ins_disable ||
	    (s->mode != MOD_BUSY && s->mode != MOD_NODISC))
		return;

	if (app_data.numdiscs > 1 && app_data.multi_play)
		fbioc_ins_interval =
			app_data.ins_interval / app_data.numdiscs;
	else
		fbioc_ins_interval = app_data.ins_interval;

	if (fbioc_ins_interval < 500)
		fbioc_ins_interval = 500;

	if (first) {
		first = FALSE;
		delay = 50;
	}
	else
		delay = fbioc_ins_interval;

	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		fbioc_insert_id = di_clinfo->timeout(
			delay,
			fbioc_insert_poll,
			(byte_t *) s
		);

		if (fbioc_insert_id != 0)
			fbioc_insert_polling = TRUE;
	}
}


/*
 * fbioc_stat_poll
 *	The playback status polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_stat_poll(curstat_t *s)
{
	if (!fbioc_stat_polling)
		return;

	/* Get current audio playback status */
	if (fbioc_get_playstatus(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			fbioc_stat_id = di_clinfo->timeout(
				fbioc_stat_interval,
				fbioc_stat_poll,
				(byte_t *) s
			);
		}
	}
	else
		fbioc_stat_polling = FALSE;
}


/*
 * fbioc_insert_poll
 *	The disc insertion polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_insert_poll(curstat_t *s)
{
	/* Check to see if a disc is inserted */
	if (!fbioc_disc_ready(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			fbioc_insert_id = di_clinfo->timeout(
				fbioc_ins_interval,
				fbioc_insert_poll,
				(byte_t *) s
			);
		}
	}
	else
		fbioc_insert_polling = FALSE;
}


/*
 * fbioc_disc_ready
 *	Check if the disc is loaded and ready for use, and update
 *	curstat table.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - Disc is ready
 *	FALSE - Disc is not ready
 */
STATIC bool_t
fbioc_disc_ready(curstat_t *s)
{
	bool_t		no_disc;
	static bool_t	first_open = TRUE,
			in_fbioc_disc_ready = FALSE;

	/* Lock this routine from multiple entry */
	if (in_fbioc_disc_ready)
		return TRUE;

	in_fbioc_disc_ready = TRUE;

	/* If device has not been opened, attempt to open it */
	if (fbioc_not_open) {
		/* Check for another copy of the CD player running on
		 * the specified device.
		 */
		if (!s->devlocked && !di_devlock(s, app_data.device)) {
			s->mode = MOD_BUSY;
			DPY_TIME(s, FALSE);
			fbioc_start_insert_poll(s);
			in_fbioc_disc_ready = FALSE;
			return FALSE;
		}

		s->devlocked = TRUE;
		s->mode = MOD_NODISC;

		if (fbioc_open(s->curdev)) {
			fbioc_not_open = FALSE;

			if (!fbioc_is_enabled(DI_ROLE_MAIN)) {
				/* Enable device for I/O */
				fbioc_enable(DI_ROLE_MAIN);
			}

			/* Check if a disc is loaded and ready */
			no_disc = !fbioc_disc_present(first_open);
		}
		else {
			DBGPRN(DBG_DEVIO)(errfp,
				"Open of %s failed\n", s->curdev);
			no_disc = TRUE;
		}
	}
	else {
		/* Just return success if we're playing CDDA */
		if (!fbioc_is_enabled(DI_ROLE_MAIN)) {
			no_disc = FALSE;
		}
		else if ((no_disc = !fbioc_disc_present(FALSE)) == TRUE) {
			/* The disc was manually ejected */
			s->mode = MOD_NODISC;
			di_clear_cdinfo(s, FALSE);
		}
	}

	if (!no_disc) {
		if (first_open) {
			first_open = FALSE;

			/* Fill in inquiry data */
			fbioc_vendor_model(s);

			/* Initialize volume/balance/routing controls */
			fbioc_init_vol(s, TRUE);
		}
		else {
			/* Force to current settings */
			(void) fbioc_cfg_vol(s->level, s, FALSE);

			/* Set up channel routing */
			fbioc_route(s);
		}
	}

	/* Read disc table of contents if a new disc was detected */
	if (fbioc_not_open || no_disc ||
	    (s->mode == MOD_NODISC && !fbioc_get_toc(s))) {
		if (fbioc_devp != NULL && app_data.eject_close) {
			fbioc_close();
			fbioc_not_open = TRUE;
		}

		di_reset_curstat(s, TRUE, TRUE);
		DPY_ALL(s);

		fbioc_start_insert_poll(s);
		in_fbioc_disc_ready = FALSE;
		return FALSE;
	}

	if (s->mode != MOD_NODISC) {
		in_fbioc_disc_ready = FALSE;
		return TRUE;
	}

	/* Load saved track program, if any */
	PROGGET(s);

	s->mode = MOD_STOP;
	DPY_ALL(s);

	/* Set caddy lock configuration */
	if (app_data.caddylock_supp)
		fbioc_lock(s, app_data.caddy_lock);

	if (app_data.load_play) {
		/* Start auto-play */
		if (!fbioc_override_ap)
			fbioc_play_pause(s);
	}
	else if (app_data.load_spindown) {
		/* Spin down disc in case the user isn't going to
		 * play anything for a while.  This reduces wear and
		 * tear on the drive.
		 */
		(void) fbioc_start_stop(FALSE, FALSE);
	}
	else {
		switch (fbioc_tst_status) {
		case MOD_PLAY:
		case MOD_PAUSE:
			/* Drive is current playing audio or paused:
			 * act appropriately.
			 */
			s->mode = fbioc_tst_status;
			(void) fbioc_get_playstatus(s);
			DPY_ALL(s);
			if (s->mode == MOD_PLAY)
				fbioc_start_stat_poll(s);
			break;
		default:
			/* Drive is stopped: do nothing */
			break;
		}
	}

	in_fbioc_disc_ready = FALSE;

	/* Load CD information for this disc.
	 * This operation has to be done outside the scope of
	 * in_fbioc_disc_ready because it may recurse
	 * back into this function.
	 */
	(void) di_get_cdinfo(s);

	return TRUE;
}


/*
 * fbioc_run_rew
 *	Run search-rewind operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_run_rew(curstat_t *s)
{
	int			i,
				skip_blks;
	sword32_t		addr,
				end_addr;
	msf_t			smsf,
				emsf;
	static sword32_t	start_addr,
				seq;

	/* Find out where we are */
	if (!fbioc_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (fbioc_start_search) {
		fbioc_start_search = FALSE;
		seq = 0;
		i = (int) (addr - skip_blks);
	}
	else {
		if (app_data.skip_spdup > 0 && seq > app_data.skip_spdup)
			/* Speed up search */
			skip_blks *= 3;

		i = (int) (start_addr - skip_blks);
	}

	start_addr = (sword32_t) ((i > di_clip_frames) ? i : di_clip_frames);

	seq++;

	if (s->shuffle || s->program) {
		if ((i = di_curtrk_pos(s)) < 0)
			i = 0;

		if (start_addr < s->trkinfo[i].addr)
			start_addr = s->trkinfo[i].addr;
	}
	else if (s->segplay == SEGP_AB && start_addr < s->bp_startpos_tot.addr)
		start_addr = s->bp_startpos_tot.addr;

	end_addr = start_addr + MAX_SRCH_BLKS;

	util_blktomsf(
		start_addr,
		&smsf.min,
		&smsf.sec,
		&smsf.frame,
		MSF_OFFSET
	);
	util_blktomsf(
		end_addr,
		&emsf.min,
		&emsf.sec,
		&emsf.frame,
		MSF_OFFSET
	);

	/* Play next search interval */
	(void) fbioc_do_playaudio(
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		fbioc_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			fbioc_run_rew,
			(byte_t *) s
		);
	}
}


/*
 * fbioc_stop_rew
 *	Stop search-rewind operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
fbioc_stop_rew(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(fbioc_search_id);
}


/*
 * fbioc_run_ff
 *	Run search-fast-forward operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
fbioc_run_ff(curstat_t *s)
{
	int			i,
				skip_blks;
	sword32_t		addr,
				end_addr;
	msf_t			smsf,
				emsf;
	static sword32_t	start_addr,
				seq;

	/* Find out where we are */
	if (!fbioc_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (fbioc_start_search) {
		fbioc_start_search = FALSE;
		seq = 0;
		start_addr = addr + skip_blks;
	}
	else {
		if (app_data.skip_spdup > 0 && seq > app_data.skip_spdup)
			/* Speed up search */
			skip_blks *= 3;

		start_addr += skip_blks;
	}

	seq++;

	if (s->shuffle || s->program) {
		if ((i = di_curtrk_pos(s)) < 0)
			i = s->tot_trks - 1;
		else if (s->cur_idx == 0)
			/* We're in the lead-in: consider this to be
			 * within the previous track.
			 */
			i--;
	}
	else
		i = s->tot_trks - 1;

	end_addr = start_addr + MAX_SRCH_BLKS;

	if (end_addr >= s->trkinfo[i+1].addr) {
		end_addr = s->trkinfo[i+1].addr;
		start_addr = end_addr - skip_blks;
	}

	if (s->segplay == SEGP_AB && end_addr > s->bp_endpos_tot.addr) {
		end_addr = s->bp_endpos_tot.addr;
		start_addr = end_addr - skip_blks;
	}

	util_blktomsf(
		start_addr,
		&smsf.min,
		&smsf.sec,
		&smsf.frame,
		MSF_OFFSET
	);
	util_blktomsf(
		end_addr,
		&emsf.min,
		&emsf.sec,
		&emsf.frame,
		MSF_OFFSET
	);

	/* Play next search interval */
	(void) fbioc_do_playaudio(
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		fbioc_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			fbioc_run_ff,
			(byte_t *) s
		);
	}
}


/*
 * fbioc_stop_ff
 *	Stop search-fast-forward operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
fbioc_stop_ff(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(fbioc_search_id);
}


/*
 * fbioc_run_ab
 *	Run a->b segment play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
STATIC bool_t
fbioc_run_ab(curstat_t *s)
{
	msf_t	start_msf,
		end_msf;

	if ((s->bp_startpos_tot.addr + app_data.min_playblks) >=
	    s->bp_endpos_tot.addr) {
		DO_BEEP();
		return FALSE;
	}

	start_msf.min = s->bp_startpos_tot.min;
	start_msf.sec = s->bp_startpos_tot.sec;
	start_msf.frame = s->bp_startpos_tot.frame;
	end_msf.min = s->bp_endpos_tot.min;
	end_msf.sec = s->bp_endpos_tot.sec;
	end_msf.frame = s->bp_endpos_tot.frame;
	s->mode = MOD_PLAY;
	DPY_ALL(s);
	fbioc_start_stat_poll(s);

	return (
		fbioc_do_playaudio(
			ADDR_BLK | ADDR_MSF,
			s->bp_startpos_tot.addr, s->bp_endpos_tot.addr,
			&start_msf, &end_msf,
			0, 0
		)
	);
}


/*
 * fbioc_run_sample
 *	Run sample play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_run_sample(curstat_t *s)
{
	sword32_t	saddr,
			eaddr;
	msf_t		smsf,
			emsf;

	if (fbioc_next_sam < s->tot_trks) {
		saddr = s->trkinfo[fbioc_next_sam].addr;
		eaddr = saddr + app_data.sample_blks,

		util_blktomsf(
			saddr,
			&smsf.min,
			&smsf.sec,
			&smsf.frame,
			MSF_OFFSET
		);
		util_blktomsf(
			eaddr,
			&emsf.min,
			&emsf.sec,
			&emsf.frame,
			MSF_OFFSET
		);

		if (s->trkinfo[fbioc_next_sam].type != TYP_AUDIO ||
		    fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				       saddr, eaddr, &smsf, &emsf, 0, 0)) {
			fbioc_next_sam++;
			return TRUE;
		}
	}

	fbioc_next_sam = 0;
	return FALSE;
}


/*
 * fbioc_run_prog
 *	Run program/shuffle play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_run_prog(curstat_t *s)
{
	sword32_t	i,
			start_addr,
			end_addr;
	msf_t		start_msf,
			end_msf;
	bool_t		hasaudio,
			ret;

	if (!s->shuffle && !s->program)
		return FALSE;

	if (fbioc_new_progshuf) {
		fbioc_new_progshuf = FALSE;

		if (s->shuffle)
			/* New shuffle sequence needed */
			di_reset_shuffle(s);
		else
			/* Program play: simply reset the count */
			s->prog_cnt = 0;

		/* Do not allow a program that contains only data tracks */
		hasaudio = FALSE;
		for (i = 0; i < (int) s->prog_tot; i++) {
			if (s->trkinfo[s->trkinfo[i].playorder].type ==
			    TYP_AUDIO) {
				hasaudio = TRUE;
				break;
			}
		}

		if (!hasaudio) {
			DO_BEEP();
			return FALSE;
		}
	}

	if (s->prog_cnt >= s->prog_tot)
		/* Done with program/shuffle play cycle */
		return FALSE;

	if ((i = di_curprog_pos(s)) < 0)
		return FALSE;

	if (s->trkinfo[i].trkno == LEAD_OUT_TRACK)
		return FALSE;

	s->prog_cnt++;
	s->cur_trk = s->trkinfo[i].trkno;
	s->cur_idx = 1;

	start_addr = s->trkinfo[i].addr + s->curpos_trk.addr;
	util_blktomsf(
		start_addr,
		&s->curpos_tot.min,
		&s->curpos_tot.sec,
		&s->curpos_tot.frame,
		MSF_OFFSET
	);
	start_msf.min = s->curpos_tot.min;
	start_msf.sec = s->curpos_tot.sec;
	start_msf.frame = s->curpos_tot.frame;

	end_addr = s->trkinfo[i+1].addr;
	end_msf.min = s->trkinfo[i+1].min;
	end_msf.sec = s->trkinfo[i+1].sec;
	end_msf.frame = s->trkinfo[i+1].frame;

	s->curpos_tot.addr = start_addr;

	if (s->mode != MOD_PAUSE)
		s->mode = MOD_PLAY;

	DPY_ALL(s);

	if (s->trkinfo[i].type == TYP_DATA)
		/* Data track: just fake it */
		return TRUE;

	ret = fbioc_do_playaudio(
		ADDR_BLK | ADDR_MSF,
		start_addr, end_addr,
		&start_msf, &end_msf,
		0, 0
	);

	if (s->mode == MOD_PAUSE) {
		(void) fbioc_pause_resume(FALSE);

		/* Restore volume */
		fbioc_mute_off(s);
	}

	return (ret);
}


/*
 * fbioc_run_repeat
 *	Run repeat play operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
fbioc_run_repeat(curstat_t *s)
{
	msf_t	start_msf,
		end_msf;
	bool_t	ret;

	if (!s->repeat)
		return FALSE;

	if (s->shuffle || s->program) {
		ret = TRUE;

		if (s->prog_cnt < s->prog_tot)
			/* Not done with program/shuffle sequence yet */
			return (ret);

		fbioc_new_progshuf = TRUE;
		s->rptcnt++;
	}
	else {
		s->cur_trk = s->first_trk;
		s->cur_idx = 1;

		s->curpos_tot.addr = 0;
		s->curpos_tot.min = 0;
		s->curpos_tot.sec = 0;
		s->curpos_tot.frame = 0;
		s->rptcnt++;
		DPY_ALL(s);

		start_msf.min = s->trkinfo[0].min;
		start_msf.sec = s->trkinfo[0].sec;
		start_msf.frame = s->trkinfo[0].frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		ret = fbioc_do_playaudio(
			ADDR_BLK | ADDR_MSF,
			s->trkinfo[0].addr, s->discpos_tot.addr,
			&start_msf, &end_msf, 0, 0
		);

		if (s->mode == MOD_PAUSE) {
			(void) fbioc_pause_resume(FALSE);

			/* Restore volume */
			fbioc_mute_off(s);
		}

	}

	return (ret);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * fbioc_enable
 *	Enable device in this process for I/O
 *
 * Args:
 *	role - Role id for which I/O is to be enabled
 *
 * Return:
 *	Nothing.
 */
void
fbioc_enable(int role)
{
	DBGPRN(DBG_DEVIO)(errfp, "Enable device: %s role: %d\n",
			  fbioc_devp->path, role);
	fbioc_devp->role = role;
}


/*
 * fbioc_disable
 *	Disable device in this process for I/O
 *
 * Args:
 *	role - Role id for which I/O is to be disabled
 *
 * Return:
 *	Nothing.
 */
void
fbioc_disable(int role)
{
	if (fbioc_devp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp, "fbioc_disable: invalid role: %d\n",
				  role);
		return;
	}

	DBGPRN(DBG_DEVIO)(errfp, "Disable device: %s role: %d\n",
			  fbioc_devp->path, role);
	fbioc_devp->role = 0;
}


/*
 * fbioc_is_enabled
 *	Check whether device is enabled for I/O in this process
 *
 * Args:
 *	role - Role id for which to check
 *
 * Return:
 *	TRUE  - enabled
 *	FALSE - disabled
 */
bool_t
fbioc_is_enabled(int role)
{
	return ((bool_t) (fbioc_devp->role == role));
}


/*
 * fbioc_send
 *	Issue ioctl command.
 *
 * Args:
 *	role - role id for which command is to be sent
 *	cmd - ioctl command
 *	arg - ioctl argument
 *	prnerr - whether an error message is to be displayed if the ioctl fails
 *
 * Return:
 *	TRUE - ioctl successful
 *	FALSE - ioctl failed
 */
bool_t
fbioc_send(int role, unsigned int cmd, void *arg, bool_t prnerr)
{
	int		i,
			ret;
	curstat_t	*s = di_clinfo->curstat_addr();

	if (fbioc_devp == NULL || fbioc_devp->fd <= 0)
		return FALSE;

	if (fbioc_devp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp,
			"NOTICE: fbioc_send: disabled role! (%d)\n",
			role);
		return FALSE;
	}

	if (cmd == CDIOCREADAUDIO && fbioc_use_pread) {
		struct ioc_read_audio	*p = (struct ioc_read_audio *) arg;
		int			offset,
					nbytes;

		offset = p->address.lba * CDDA_BLKSZ;
		nbytes = p->nframes * CDDA_BLKSZ;

		DBGPRN(DBG_DEVIO)(errfp, "\nPREAD: offset=%d nbytes=%d\n",
				  offset, nbytes);

		ret = pread(fbioc_devp->fd, p->buffer, nbytes, offset);
		if (ret == nbytes)
			return TRUE;

		if (prnerr) {
			(void) fprintf(errfp,
				"PREAD error on %s: ret=%d, errno=%d\n",
				s->curdev, ret, errno
			);
		}
		return FALSE;
	}

	if (app_data.debug & DBG_DEVIO) {
		for (i = 0; iname[i].name != NULL; i++) {
			if (iname[i].cmd == cmd) {
				(void) fprintf(errfp, "\nIOCTL: %s arg=0x%lx ",
					       iname[i].name, (long) arg);
				break;
			}
		}
		if (iname[i].name == NULL)
			(void) fprintf(errfp, "\nIOCTL: 0x%x arg=0x%lx ",
				       cmd, (long) arg);
	}

	ret = ioctl(fbioc_devp->fd, cmd, arg);

	DBGPRN(DBG_DEVIO)(errfp, "ret=%d\n", ret);

	if (ret < 0) {
		if (prnerr) {
			(void) fprintf(errfp, "CD audio: ioctl error on %s: ",
				       s->curdev);

			for (i = 0; iname[i].name != NULL; i++) {
				if (iname[i].cmd == cmd) {
					(void) fprintf(errfp,
						       "cmd=%s errno=%d\n",
						       iname[i].name, errno);
					break;
				}
			}
			if (iname[i].name == NULL)
				(void) fprintf(errfp, "cmd=0x%x errno=%d\n",
					       cmd, errno);
		}
		return FALSE;
	}

	return TRUE;
}


/*
 * fbioc_init
 *	Top-level function to initialize the FreeBSD/NetBSD/OpenBSD
 *	ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_init(curstat_t *s, di_tbl_t *dt)
{
	char	*cp;
	int	kver,
		len;

	if (app_data.di_method != DI_FBIOC)
		/* FreeBSD/NetBSD/OpenBSD ioctl method not configured */
		return;

	/* Set the default CDDA read method based on running kernel version */
	len = sizeof(kver);
	if (sysctlbyname("kern.osreldate", &kver, &len, NULL, 0) < 0 ||
	    kver < 501106)
		fbioc_use_pread = FALSE;
	else
		fbioc_use_pread = TRUE;

	/* Allow the user to override the default CDDA read method via the
	 * CDDA_USE_PREAD environment variable
	 */
	if ((cp = getenv("CDDA_USE_PREAD")) != NULL)
		fbioc_use_pread = (bool_t) (atoi(cp) > 0);

	/* Initialize libdi calling table */
	dt->load_cdtext = NULL;
	dt->playmode = fbioc_playmode;
	dt->check_disc = fbioc_check_disc;
	dt->status_upd = fbioc_status_upd;
	dt->lock = fbioc_lock;
	dt->repeat = fbioc_repeat;
	dt->shuffle = fbioc_shuffle;
	dt->load_eject = fbioc_load_eject;
	dt->ab = fbioc_ab;
	dt->sample = fbioc_sample;
	dt->level = fbioc_level;
	dt->play_pause = fbioc_play_pause;
	dt->stop = fbioc_stop;
	dt->chgdisc = fbioc_chgdisc;
	dt->prevtrk = fbioc_prevtrk;
	dt->nexttrk = fbioc_nexttrk;
	dt->previdx = fbioc_previdx;
	dt->nextidx = fbioc_nextidx;
	dt->rew = fbioc_rew;
	dt->ff = fbioc_ff;
	dt->warp = fbioc_warp;
	dt->route = fbioc_route;
	dt->mute_on = fbioc_mute_on;
	dt->mute_off = fbioc_mute_off;
	dt->cddajitter = fbioc_cddajitter;
	dt->debug = fbioc_debug;
	dt->start = fbioc_start;
	dt->icon = fbioc_icon;
	dt->halt = fbioc_halt;
	dt->methodstr = fbioc_methodstr;

	/* Hardwire some unsupported features */
	app_data.chroute_supp = FALSE;

	/* Initalize FreeBSD/NetBSD/OpenBSD ioctl method */
	fbioc_stat_polling = FALSE;
	fbioc_stat_interval = app_data.stat_interval;
	fbioc_insert_polling = FALSE;
	fbioc_next_sam = FALSE;
	fbioc_new_progshuf = FALSE;
	fbioc_sav_end_addr = 0;
	fbioc_sav_end_msf.min = fbioc_sav_end_msf.sec =
		fbioc_sav_end_msf.frame = 0;
	fbioc_sav_end_fmt = 0;

	/* Initialize curstat structure */
	di_reset_curstat(s, TRUE, TRUE);

	if (app_data.numdiscs > 1) {
		/* There is currently no changer support */
		DBGPRN(DBG_DEVIO)(errfp, "CD changer not supported:\n%s\n",
			"Setting to single disc mode.");

		app_data.numdiscs = 1;
		app_data.chg_method = CHG_NONE;
		app_data.multi_play = FALSE;
		app_data.reverse = FALSE;
		s->first_disc = s->last_disc = s->cur_disc = 1;
	}

#ifdef __FreeBSD__
	DBGPRN(DBG_DEVIO)(errfp, "libdi: FreeBSD ioctl method\n");
#endif
#ifdef __NetBSD__
	DBGPRN(DBG_DEVIO)(errfp, "libdi: NetBSD ioctl method\n");
#endif
#ifdef __OpenBSD__
	DBGPRN(DBG_DEVIO)(errfp, "libdi: OpenBSD ioctl method\n");
#endif
}


/*
 * fbioc_playmode
 *	Init/halt CDDA mode
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
fbioc_playmode(curstat_t *s)
{
	bool_t		ret,
			cdda;
	static bool_t	prev_cdda = FALSE;

	cdda = (bool_t) PLAYMODE_IS_CDDA(app_data.play_mode);

	if (cdda == prev_cdda)
		return TRUE;	/* No change */

	if (cdda) {
		fbioc_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		fbioc_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		fbioc_cdda_client.warning_msg = di_clinfo->warning_msg;
		fbioc_cdda_client.info_msg = di_clinfo->info_msg;
		fbioc_cdda_client.info2_msg = di_clinfo->info2_msg;

		ret = cdda_init(s, &fbioc_cdda_client);
	}
	else {
		cdda_halt(fbioc_devp, s);
		ret = TRUE;

		/* Initialize volume/balance/routing controls */
		fbioc_init_vol(s, FALSE);
	}

	if (ret)
		prev_cdda = cdda;

	return (ret);
}


/*
 * fbioc_check_disc
 *	Check if disc is ready for use
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
fbioc_check_disc(curstat_t *s)
{
	return (fbioc_disc_ready(s));
}


/*
 * fbioc_status_upd
 *	Force update of playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_status_upd(curstat_t *s)
{
	(void) fbioc_get_playstatus(s);
}


/*
 * fbioc_lock
 *	Caddy lock function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable caddy lock
 *
 * Return:
 *	Nothing.
 */
void
fbioc_lock(curstat_t *s, bool_t enable)
{
	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC) {
		SET_LOCK_BTN(FALSE);
		return;
	}
	else if (s->mode != MOD_STOP) {
		/* Only allow changing lock status when stopped */
		DO_BEEP();
		SET_LOCK_BTN((bool_t) !enable);
		return;
	}

	if (!fbioc_send(DI_ROLE_MAIN, enable ? CDIOCPREVENT : CDIOCALLOW,
			NULL, TRUE)) {
		/* Cannot lock/unlock caddy */
		DO_BEEP();
		SET_LOCK_BTN((bool_t) !enable);
		return;
	}

	s->caddy_lock = enable;
	SET_LOCK_BTN(enable);
}


/*
 * fbioc_repeat
 *	Repeat mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable repeat mode
 *
 * Return:
 *	Nothing.
 */
void
fbioc_repeat(curstat_t *s, bool_t enable)
{
	s->repeat = enable;

	if (!enable && fbioc_new_progshuf) {
		fbioc_new_progshuf = FALSE;
		if (s->rptcnt > 0)
			s->rptcnt--;
	}
	DPY_RPTCNT(s);
}


/*
 * fbioc_shuffle
 *	Shuffle mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable shuffle mode
 *
 * Return:
 *	Nothing.
 */
void
fbioc_shuffle(curstat_t *s, bool_t enable)
{
	if (s->segplay == SEGP_A) {
		/* Can't set shuffle during a->? mode */
		DO_BEEP();
		SET_SHUFFLE_BTN((bool_t) !enable);
		return;
	}

	switch (s->mode) {
	case MOD_STOP:
	case MOD_BUSY:
	case MOD_NODISC:
		if (s->program) {
			/* Currently in program mode: can't enable shuffle */
			DO_BEEP();
			SET_SHUFFLE_BTN((bool_t) !enable);
			return;
		}
		break;
	default:
		if (enable) {
			/* Can't enable shuffle unless when stopped */
			DO_BEEP();
			SET_SHUFFLE_BTN((bool_t) !enable);
			return;
		}
		break;
	}

	s->segplay = SEGP_NONE;	/* Cancel a->b mode */
	DPY_PROGMODE(s, FALSE);

	s->shuffle = enable;
	if (!s->shuffle)
		s->prog_tot = 0;
}


/*
 * fbioc_load_eject
 *	CD caddy load and eject function.  If disc caddy is not
 *	loaded, it will attempt to load it.  Otherwise, it will be
 *	ejected.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_load_eject(curstat_t *s)
{
	bool_t	ret = FALSE;

	if (fbioc_devp == NULL)
		return;

	if (fbioc_is_enabled(DI_ROLE_MAIN) && !fbioc_disc_present(FALSE)) {
		/* No disc */
		if (app_data.load_supp) {
			/* Try loading the disc */
			ret = fbioc_start_stop(TRUE, TRUE);
		}

		if (!ret && app_data.eject_supp) {
			/* Cannot load, maybe the tray is already closed
			 * but empty.
			 */

			/* Unlock caddy if supported */
			if (app_data.caddylock_supp)
				fbioc_lock(s, FALSE);

			/* Try opening the tray */
			if (fbioc_start_stop(FALSE, TRUE))
				DO_BEEP();
		}

		fbioc_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (fbioc_devp != NULL && app_data.eject_close) {
			fbioc_close();
			fbioc_not_open = TRUE;
		}

		fbioc_start_insert_poll(s);
		return;
	}

	/* Eject the disc */

	/* Spin down the CD */
	(void) fbioc_start_stop(FALSE, FALSE);

	if (!app_data.eject_supp) {
		DO_BEEP();

		fbioc_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (fbioc_devp != NULL && app_data.eject_close) {
			fbioc_close();
			fbioc_not_open = TRUE;
		}

		fbioc_start_insert_poll(s);
		return;
	}

	/* Unlock caddy if supported */
	if (app_data.caddylock_supp)
		fbioc_lock(s, FALSE);

	fbioc_stop_stat_poll();
	di_reset_curstat(s, TRUE, TRUE);
	s->mode = MOD_NODISC;

	di_clear_cdinfo(s, FALSE);
	DPY_ALL(s);

	/* Eject the CD */
	(void) fbioc_start_stop(FALSE, TRUE);

	if (app_data.eject_exit)
		di_clinfo->quit(s);
	else {
		if (fbioc_devp != NULL && app_data.eject_close) {
			fbioc_close();
			fbioc_not_open = TRUE;
		}
			
		fbioc_start_insert_poll(s);
	}
}


/*
 * fbioc_ab
 *	A->B segment play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_ab(curstat_t *s)
{
	if (!fbioc_run_ab(s))
		DO_BEEP();
}


/*
 * fbioc_sample
 *	Sample play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_sample(curstat_t *s)
{
	int	i;

	if (!fbioc_disc_ready(s)) {
		DO_BEEP();
		return;
	}

	if (s->shuffle || s->program || s->segplay != SEGP_NONE) {
		/* Sample is not supported in program/shuffle or a->b modes */
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_STOP:
		fbioc_start_stat_poll(s);
		/*FALLTHROUGH*/
	case MOD_PLAY:
		/* If already playing a track, start sampling the track after
		 * the current one.  Otherwise, sample from the beginning.
		 */
		if (s->cur_trk > 0 && s->cur_trk != s->last_trk) {
			i = di_curtrk_pos(s) + 1;
			s->cur_trk = s->trkinfo[i].trkno;
			fbioc_next_sam = (byte_t) i;
		}
		else {
			s->cur_trk = s->first_trk;
			fbioc_next_sam = 0;
		}
		
		s->cur_idx = 1;

		s->mode = MOD_SAMPLE;
		DPY_ALL(s);

		if (!fbioc_run_sample(s))
			return;

		break;

	case MOD_SAMPLE:
		/* Currently doing Sample playback, just call fbioc_play_pause
		 * to resume normal playback.
		 */
		fbioc_play_pause(s);
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * fbioc_level
 *	Audio volume control function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	level - The volume level to set to
 *	drag - Whether this is an update due to the user dragging the
 *		volume control slider thumb.  If this is FALSE, then
 *		a final volume setting has been found.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
fbioc_level(curstat_t *s, byte_t level, bool_t drag)
{
	int	actual;

	/* Set volume level */
	if ((actual = fbioc_cfg_vol((int) level, s, FALSE)) >= 0)
		s->level = (byte_t) actual;
}


/*
 * fbioc_play_pause
 *	Audio playback and pause function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_play_pause(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;

	fbioc_override_ap = TRUE;

	if (!fbioc_disc_ready(s)) {
		fbioc_override_ap = FALSE;
		DO_BEEP();
		return;
	}

	fbioc_override_ap = FALSE;

	if (s->mode == MOD_NODISC)
		s->mode = MOD_STOP;

	switch (s->mode) {
	case MOD_PLAY:
		/* Currently playing: go to pause mode */

		if (!fbioc_pause_resume(FALSE)) {
			DO_BEEP();
			return;
		}
		fbioc_stop_stat_poll();
		s->mode = MOD_PAUSE;
		DPY_PLAYMODE(s, FALSE);
		break;

	case MOD_PAUSE:
		/* Currently paused: resume play */

		if (!fbioc_pause_resume(TRUE)) {
			DO_BEEP();
			return;
		}
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		fbioc_start_stat_poll(s);
		break;

	case MOD_STOP:
		/* Currently stopped: start play */

		if (!di_prepare_cdda(s))
			return;

		if (s->shuffle || s->program) {
			fbioc_new_progshuf = TRUE;

			/* Start shuffle/program play */
			if (!fbioc_run_prog(s))
				return;
		}
		else if (s->segplay == SEGP_AB) {
			/* Play defined segment */
			if (!fbioc_run_ab(s))
				return;
		}
		else {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */

			/* Start normal play */
			if ((i = di_curtrk_pos(s)) < 0 || s->cur_trk <= 0) {
				/* Start play from the beginning */
				i = 0;
				s->cur_trk = s->first_trk;
				start_addr = s->trkinfo[0].addr +
					     s->curpos_trk.addr;
				util_blktomsf(
					start_addr,
					&start_msf.min,
					&start_msf.sec,
					&start_msf.frame,
					MSF_OFFSET
				);
			}
			else {
				/* User has specified a starting track */
				start_addr = s->trkinfo[i].addr +
					     s->curpos_trk.addr;
			}

			util_blktomsf(
				start_addr,
				&start_msf.min,
				&start_msf.sec,
				&start_msf.frame,
				MSF_OFFSET
			);

			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			if (s->trkinfo[i].type == TYP_DATA) {
				DPY_TRACK(s);
				DPY_TIME(s, FALSE);
				DO_BEEP();
				return;
			}

			s->cur_idx = 1;
			s->mode = MOD_PLAY;

			if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				s->mode = MOD_STOP;
				return;
			}
		}

		DPY_ALL(s);
		fbioc_start_stat_poll(s);
		break;

	case MOD_SAMPLE:
		/* Force update of curstat */
		if (!fbioc_get_playstatus(s)) {
			DO_BEEP();
			return;
		}

		/* Currently doing a->b or sample playback: just resume play */
		if (s->shuffle || s->program) {
			if ((i = di_curtrk_pos(s)) < 0 ||
			    s->trkinfo[i].trkno == LEAD_OUT_TRACK)
				return;

			start_msf.min = s->curpos_tot.min;
			start_msf.sec = s->curpos_tot.sec;
			start_msf.frame = s->curpos_tot.frame;
			end_msf.min = s->trkinfo[i+1].min;
			end_msf.sec = s->trkinfo[i+1].sec;
			end_msf.frame = s->trkinfo[i+1].frame;

			if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  s->curpos_tot.addr,
					  s->trkinfo[i+1].addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}
		}
		else {
			start_msf.min = s->curpos_tot.min;
			start_msf.sec = s->curpos_tot.sec;
			start_msf.frame = s->curpos_tot.frame;
			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  s->curpos_tot.addr,
					  s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}
		}
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * fbioc_stop
 *	Stop function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	stop_disc - Whether to actually spin down the disc or just
 *		update status.
 *
 * Return:
 *	Nothing.
 */
void
fbioc_stop(curstat_t *s, bool_t stop_disc)
{
	/* The stop_disc parameter will cause the disc to spin down.
	 * This is usually set to TRUE, but can be FALSE if the caller
	 * just wants to set the current state to stop but will
	 * immediately go into play state again.  Not spinning down
	 * the drive makes things a little faster...
	 */
	if (!fbioc_disc_ready(s))
		return;

	switch (s->mode) {
	case MOD_PLAY:
	case MOD_PAUSE:
	case MOD_SAMPLE:
	case MOD_STOP:
		/* Currently playing or paused: stop */

		if (stop_disc && !fbioc_start_stop(FALSE, FALSE)) {
			DO_BEEP();
			return;
		}
		fbioc_stop_stat_poll();

		di_reset_curstat(s, FALSE, FALSE);
		s->mode = MOD_STOP;
		s->rptcnt = 0;

		DPY_ALL(s);
		break;

	default:
		break;
	}
}


/*
 * fbioc_chgdisc
 *	Change disc function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
fbioc_chgdisc(curstat_t *s)
{
	/* Disc change is not implemented in this module */
	DO_BEEP();
}


/*
 * fbioc_prevtrk
 *	Previous track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_prevtrk(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;
	bool_t		go_prev;

	if (!fbioc_disc_ready(s)) {
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		/* Find appropriate track to start */
		if (s->shuffle || s->program) {
			if (s->prog_cnt > 0) {
				s->prog_cnt--;
				fbioc_new_progshuf = FALSE;
			}
			i = di_curprog_pos(s);
		}
		else
			i = di_curtrk_pos(s);

		if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		go_prev = FALSE;

		if (i == 0 && s->cur_idx == 0) {
			i = 0;
			start_addr = di_clip_frames;
			util_blktomsf(
				start_addr,
				&start_msf.min,
				&start_msf.sec,
				&start_msf.frame,
				MSF_OFFSET
			);
			s->cur_trk = s->trkinfo[i].trkno;
			s->cur_idx = 0;
		}
		else {
			start_addr = s->trkinfo[i].addr;
			start_msf.min = s->trkinfo[i].min;
			start_msf.sec = s->trkinfo[i].sec;
			start_msf.frame = s->trkinfo[i].frame;
			s->cur_trk = s->trkinfo[i].trkno;
			s->cur_idx = 1;

			/* If the current track has been playing for less
			 * than app_data.prev_threshold blocks, then go
			 * to the beginning of the previous track (if we
			 * are not already on the first track).
			 */
			if ((s->curpos_tot.addr - start_addr) <=
			    app_data.prev_threshold)
				go_prev = TRUE;
		}

		if (go_prev) {
			if (s->shuffle || s->program) {
				if (s->prog_cnt > 0) {
					s->prog_cnt--;
					fbioc_new_progshuf = FALSE;
				}
				if ((i = di_curprog_pos(s)) < 0)
					return;

				start_addr = s->trkinfo[i].addr;
				start_msf.min = s->trkinfo[i].min;
				start_msf.sec = s->trkinfo[i].sec;
				start_msf.frame = s->trkinfo[i].frame;
				s->cur_trk = s->trkinfo[i].trkno;
			}
			else if (i == 0) {
				/* Go to the very beginning: this may be
				 * a lead-in area before the start of track 1.
				 */
				start_addr = di_clip_frames;
				util_blktomsf(
					start_addr,
					&start_msf.min,
					&start_msf.sec,
					&start_msf.frame,
					MSF_OFFSET
				);
				s->cur_trk = s->trkinfo[i].trkno;
			}
			else if (i > 0) {
				i--;

				/* Skip over data tracks */
				while (s->trkinfo[i].type == TYP_DATA) {
					if (i <= 0)
						break;
					i--;
				}

				if (s->trkinfo[i].type != TYP_DATA) {
					start_addr = s->trkinfo[i].addr;
					start_msf.min = s->trkinfo[i].min;
					start_msf.sec = s->trkinfo[i].sec;
					start_msf.frame = s->trkinfo[i].frame;
					s->cur_trk = s->trkinfo[i].trkno;
				}
			}
		}

		if (s->mode == MOD_PAUSE)
			/* Mute: so we don't get a transient */
			fbioc_mute_on(s);

		if (s->shuffle || s->program) {
			/* Program/Shuffle mode: just stop the playback
			 * and let fbioc_run_prog go to the previous track
			 */
			fbioc_fake_stop = TRUE;

			/* Force status update */
			(void) fbioc_get_playstatus(s);
		}
		else {
			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			s->curpos_tot.addr = start_addr;
			s->curpos_tot.min = start_msf.min;
			s->curpos_tot.sec = start_msf.sec;
			s->curpos_tot.frame = start_msf.frame;
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			DPY_TRACK(s);
			DPY_INDEX(s);
			DPY_TIME(s, FALSE);

			if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();

				/* Restore volume */
				fbioc_mute_off(s);
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) fbioc_pause_resume(FALSE);

				/* Restore volume */
				fbioc_mute_off(s);
			}
		}

		break;

	case MOD_STOP:
		if (s->shuffle || s->program) {
			/* Pre-selecting tracks not supported in shuffle
			 * or program mode.
			 */
			DO_BEEP();
			return;
		}

		/* Find previous track */
		if (s->cur_trk <= 0) {
			s->cur_trk = s->trkinfo[0].trkno;
			DPY_TRACK(s);
		}
		else {
			i = di_curtrk_pos(s);

			if (i > 0) {
				s->cur_trk = s->trkinfo[i-1].trkno;
				DPY_TRACK(s);
			}
		}
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * fbioc_nexttrk
 *	Next track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_nexttrk(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;

	if (!fbioc_disc_ready(s)) {
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->shuffle || s->program) {
			if (s->prog_cnt >= s->prog_tot) {
				/* Disallow advancing beyond current
				 * shuffle/program sequence if
				 * repeat mode is not on.
				 */
				if (s->repeat)
					fbioc_new_progshuf = TRUE;
				else
					return;
			}

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				fbioc_mute_on(s);

			/* Program/Shuffle mode: just stop the playback
			 * and let fbioc_run_prog go to the next track.
			 */
			fbioc_fake_stop = TRUE;

			/* Force status update */
			(void) fbioc_get_playstatus(s);

			return;
		}
		else if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		/* Find next track */
		if ((i = di_curtrk_pos(s)) < 0)
			return;

		if (i > 0 || s->cur_idx > 0)
			i++;

		/* Skip over data tracks */
		while (i < MAXTRACK && s->trkinfo[i].type == TYP_DATA)
			i++;

		if (i < MAXTRACK &&
		    s->trkinfo[i].trkno >= 0 &&
		    s->trkinfo[i].trkno != LEAD_OUT_TRACK) {

			start_addr = s->trkinfo[i].addr;
			start_msf.min = s->trkinfo[i].min;
			start_msf.sec = s->trkinfo[i].sec;
			start_msf.frame = s->trkinfo[i].frame;
			s->cur_trk = s->trkinfo[i].trkno;
			s->cur_idx = 1;

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				fbioc_mute_on(s);

			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;

			s->curpos_tot.addr = start_addr;
			s->curpos_tot.min = start_msf.min;
			s->curpos_tot.sec = start_msf.sec;
			s->curpos_tot.frame = start_msf.frame;
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			DPY_TRACK(s);
			DPY_INDEX(s);
			DPY_TIME(s, FALSE);

			if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) fbioc_pause_resume(FALSE);

				/* Restore volume */
				fbioc_mute_off(s);
			}
		}

		break;

	case MOD_STOP:
		if (s->shuffle || s->program) {
			/* Pre-selecting tracks not supported in shuffle
			 * or program mode.
			 */
			DO_BEEP();
			return;
		}

		/* Find next track */
		if (s->cur_trk <= 0) {
			s->cur_trk = s->trkinfo[0].trkno;
			DPY_TRACK(s);
		}
		else {
			i = di_curtrk_pos(s) + 1;

			if (i > 0 && s->trkinfo[i].trkno != LEAD_OUT_TRACK) {
				s->cur_trk = s->trkinfo[i].trkno;
				DPY_TRACK(s);
			}
		}
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * fbioc_previdx
 *	Previous index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_previdx(curstat_t *s)
{
	msf_t		start_msf,
			end_msf;
	byte_t		idx;

	if (s->shuffle || s->program) {
		/* Index search is not supported in program/shuffle mode */
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		/* Find appropriate index to start */
		if (s->cur_idx > 1 &&
		    (s->curpos_tot.addr - s->sav_iaddr) <=
			    app_data.prev_threshold)
			idx = s->cur_idx - 1;
		else
			idx = s->cur_idx;
		
		/* This is a Hack...
		 * Since there is no standard command to start
		 * playback on an index boundary and then go on playing
		 * until the end of the disc, we will use the PLAY AUDIO
		 * TRACK/INDEX command to go to where we want to start,
		 * immediately followed by a PAUSE.  We then find the
		 * current block position and issue a PLAY AUDIO MSF
		 * or PLAY AUDIO(12) command to start play there.
		 * We mute the audio in between these operations to
		 * prevent unpleasant transients.
		 */

		/* Mute */
		fbioc_mute_on(s);

		if (!fbioc_do_playaudio(ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk, idx)) {
			/* Restore volume */
			fbioc_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		fbioc_idx_pause = TRUE;

		if (!fbioc_pause_resume(FALSE)) {
			/* Restore volume */
			fbioc_mute_off(s);
			fbioc_idx_pause = FALSE;
			return;
		}

		/* Use fbioc_get_playstatus to update the current status */
		if (!fbioc_get_playstatus(s)) {
			/* Restore volume */
			fbioc_mute_off(s);
			fbioc_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			fbioc_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			fbioc_idx_pause = FALSE;
			return;
		}

		fbioc_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) fbioc_pause_resume(FALSE);

			/* Restore volume */
			fbioc_mute_off(s);

			/* Force update of curstat */
			(void) fbioc_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * fbioc_nextidx
 *	Next index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_nextidx(curstat_t *s)
{
	msf_t		start_msf,
			end_msf;

	if (s->shuffle || s->program) {
		/* Index search is not supported in program/shuffle mode */
		DO_BEEP();
		return;
	}

	switch (s->mode) {
	case MOD_SAMPLE:
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE;	/* Cancel a->b mode */
			DPY_PROGMODE(s, FALSE);
		}

		/* Find appropriate index to start */
		
		/* This is a Hack...
		 * Since there is no standard command to start
		 * playback on an index boundary and then go on playing
		 * until the end of the disc, we will use the PLAY AUDIO
		 * TRACK/INDEX command to go to where we want to start,
		 * immediately followed by a PAUSE.  We then find the
		 * current block position and issue a PLAY AUDIO MSF
		 * or PLAY AUDIO(12) command to start play there.
		 * We mute the audio in between these operations to
		 * prevent unpleasant transients.
		 */

		/* Mute */
		fbioc_mute_on(s);

		if (!fbioc_do_playaudio(ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk,
				  (byte_t) (s->cur_idx + 1))) {
			/* Restore volume */
			fbioc_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		fbioc_idx_pause = TRUE;

		if (!fbioc_pause_resume(FALSE)) {
			/* Restore volume */
			fbioc_mute_off(s);
			fbioc_idx_pause = FALSE;
			return;
		}

		/* Use fbioc_get_playstatus to update the current status */
		if (!fbioc_get_playstatus(s)) {
			/* Restore volume */
			fbioc_mute_off(s);
			fbioc_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			fbioc_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			fbioc_idx_pause = FALSE;
			return;
		}

		fbioc_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) fbioc_pause_resume(FALSE);

			/* Restore volume */
			fbioc_mute_off(s);

			/* Force update of curstat */
			(void) fbioc_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * fbioc_rew
 *	Search-rewind function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_rew(curstat_t *s, bool_t start)
{
	sword32_t	i;
	msf_t		start_msf,
			end_msf;
	byte_t		vol;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		fbioc_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				fbioc_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) fbioc_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE
				);
			}

			/* Start search rewind */
			fbioc_start_search = TRUE;
			fbioc_run_rew(s);
		}
		else {
			/* Button release */

			fbioc_stop_rew(s);

			/* Update display */
			(void) fbioc_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				fbioc_mute_on(s);
			else
				/* Restore volume */
				fbioc_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == LEAD_OUT_TRACK) {
					/* Restore volume */
					fbioc_mute_off(s);
					return;
				}

				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_msf.min = s->trkinfo[i+1].min;
				end_msf.sec = s->trkinfo[i+1].sec;
				end_msf.frame = s->trkinfo[i+1].frame;

				if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  s->curpos_tot.addr,
						  s->trkinfo[i+1].addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					fbioc_mute_off(s);
					return;
				}
			}
			else {
				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_msf.min = s->discpos_tot.min;
				end_msf.sec = s->discpos_tot.sec;
				end_msf.frame = s->discpos_tot.frame;

				if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  s->curpos_tot.addr,
						  s->discpos_tot.addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					fbioc_mute_off(s);
					return;
				}
			}

			if (s->mode == MOD_PAUSE) {
				(void) fbioc_pause_resume(FALSE);

				/* Restore volume */
				fbioc_mute_off(s);
			}
			else
				fbioc_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * fbioc_ff
 *	Search-fast-forward function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_ff(curstat_t *s, bool_t start)
{
	sword32_t	i,
			start_addr,
			end_addr;
	msf_t		start_msf,
			end_msf;
	byte_t		vol;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		fbioc_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				fbioc_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) fbioc_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE
				);
			}

			/* Start search forward */
			fbioc_start_search = TRUE;
			fbioc_run_ff(s);
		}
		else {
			/* Button release */

			fbioc_stop_ff(s);

			/* Update display */
			(void) fbioc_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				fbioc_mute_on(s);
			else
				/* Restore volume */
				fbioc_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == LEAD_OUT_TRACK) {
					/* Restore volume */
					fbioc_mute_off(s);
					return;
				}

				start_addr = s->curpos_tot.addr;
				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_addr = s->trkinfo[i+1].addr;
				end_msf.min = s->trkinfo[i+1].min;
				end_msf.sec = s->trkinfo[i+1].sec;
				end_msf.frame = s->trkinfo[i+1].frame;

				if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  start_addr, end_addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					fbioc_mute_off(s);
					return;
				}
			}
			else if (s->segplay == SEGP_AB &&
				 (s->curpos_tot.addr + app_data.min_playblks) >
					s->bp_endpos_tot.addr) {
				/* No more left to play */
				/* Restore volume */
				fbioc_mute_off(s);
				return;
			}
			else {
				start_addr = s->curpos_tot.addr;
				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;

				if (s->segplay == SEGP_AB) {
					end_addr = s->bp_endpos_tot.addr;
					end_msf.min = s->bp_endpos_tot.min;
					end_msf.sec = s->bp_endpos_tot.sec;
					end_msf.frame = s->bp_endpos_tot.frame;
				}
				else {
					end_addr = s->discpos_tot.addr;
					end_msf.min = s->discpos_tot.min;
					end_msf.sec = s->discpos_tot.sec;
					end_msf.frame = s->discpos_tot.frame;
				}

				if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  start_addr, end_addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					fbioc_mute_off(s);
					return;
				}
			}
			if (s->mode == MOD_PAUSE) {
				(void) fbioc_pause_resume(FALSE);

				/* Restore volume */
				fbioc_mute_off(s);
			}
			else
				fbioc_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * fbioc_warp
 *	Track warp function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_warp(curstat_t *s)
{
	sword32_t	start_addr,
			end_addr;
	msf_t		start_msf,
			end_msf;
	int		i;

	start_addr = s->curpos_tot.addr;
	start_msf.min = s->curpos_tot.min;
	start_msf.sec = s->curpos_tot.sec;
	start_msf.frame = s->curpos_tot.frame;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		fbioc_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (s->shuffle || s->program) {
			if ((i = di_curtrk_pos(s)) < 0) {
				DO_BEEP();
				return;
			}

			end_addr = s->trkinfo[i+1].addr;
			end_msf.min = s->trkinfo[i+1].min;
			end_msf.sec = s->trkinfo[i+1].sec;
			end_msf.frame = s->trkinfo[i+1].frame;
		}
		else {
			end_addr = s->discpos_tot.addr;
			end_msf.min = s->discpos_tot.min;
			end_msf.sec = s->discpos_tot.sec;
			end_msf.frame = s->discpos_tot.frame;
		}

		if (s->segplay == SEGP_AB) {
			if (start_addr < s->bp_startpos_tot.addr) {
				start_addr = s->bp_startpos_tot.addr;
				start_msf.min = s->bp_startpos_tot.min;
				start_msf.sec = s->bp_startpos_tot.sec;
				start_msf.frame = s->bp_startpos_tot.frame;
			}

			if (end_addr > s->bp_endpos_tot.addr) {
				end_addr = s->bp_endpos_tot.addr;
				end_msf.min = s->bp_endpos_tot.min;
				end_msf.sec = s->bp_endpos_tot.sec;
				end_msf.frame = s->bp_endpos_tot.frame;
			}
		}

		if ((end_addr - app_data.min_playblks) < start_addr) {
			/* No more left to play: just stop */
			if (!fbioc_start_stop(FALSE, FALSE))
				DO_BEEP();
		}
		else {
			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				fbioc_mute_on(s);

			if (!fbioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						start_addr, end_addr,
						&start_msf, &end_msf,
						0, 0)) {
				DO_BEEP();

				/* Restore volume */
				fbioc_mute_off(s);
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) fbioc_pause_resume(FALSE);

				/* Restore volume */
				fbioc_mute_off(s);
			}
		}
		break;

	default:
		break;
	}
}


/*
 * fbioc_route
 *	Channel routing function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_route(curstat_t *s)
{
	if (!app_data.chroute_supp)
		return;

	/* Only CDDA mode supports channel routing on FreeBSD/OpenBSD/NetBSD */
	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		(void) cdda_chroute(fbioc_devp, s);
}


/*
 * fbioc_mute_on
 *	Mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_mute_on(curstat_t *s)
{
	(void) fbioc_cfg_vol(0, s, FALSE);
}


/*
 * fbioc_mute_off
 *	Un-mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_mute_off(curstat_t *s)
{
	(void) fbioc_cfg_vol((int) s->level, s, FALSE);
}


/*
 * fbioc_cddajitter
 *	CDDA jitter correction setting change notification
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_cddajitter(curstat_t *s)
{
	sword32_t	curblk = 0;
	byte_t		omode;
	
	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		omode = s->mode;
		if (omode == MOD_PAUSE)
			omode = MOD_PLAY;

		/* Save state */
		switch (omode) {
		case MOD_PLAY:
		case MOD_SAMPLE:
			curblk = s->curpos_tot.addr;
			fbioc_stop_stat_poll();
			break;

		default:
			break;
		}

		/* Restart CDDA */
		cdda_halt(fbioc_devp, s);
		util_delayms(1000);

		fbioc_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		fbioc_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		fbioc_cdda_client.warning_msg = di_clinfo->warning_msg;
		fbioc_cdda_client.info_msg = di_clinfo->info_msg;
		fbioc_cdda_client.info2_msg = di_clinfo->info2_msg;

		(void) cdda_init(s, &fbioc_cdda_client);

		/* Restore old state */
		switch (omode) {
		case MOD_PLAY:
		case MOD_SAMPLE:
			if (fbioc_play_recov(curblk, FALSE)) {
				fbioc_start_stat_poll(s);

				if (omode == MOD_PAUSE)
					(void) fbioc_pause_resume(FALSE);
			}
			break;

		default:
			break;
		}

		s->mode = omode;
	}
}


/*
 * fbioc_debug
 *	Debug level change notification
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
fbioc_debug(void)
{
	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		cdda_debug(app_data.debug);
}


/*
 * fbioc_start
 *	Start the FreeBSD/NetBSD/OpenBSD ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_start(curstat_t *s)
{
	/* Check to see if disc is ready */
	if (di_clinfo->timeout != NULL)
		fbioc_start_insert_poll(s);
	else
		(void) fbioc_disc_ready(s);

	/* Update display */
	DPY_ALL(s);
}


/*
 * fbioc_icon
 *	Handler for main window iconification/de-iconification
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	iconified - Whether the main window is iconified
 *
 * Return:
 *	Nothing.
 */
void
fbioc_icon(curstat_t *s, bool_t iconified)
{
	/* This function attempts to reduce the status polling frequency
	 * when possible to cut down on CPU and bus usage.  This is
	 * done when the CD player is iconified.
	 */

	/* Increase status polling interval by 4 seconds when iconified */
	if (iconified)
		fbioc_stat_interval = app_data.stat_interval + 4000;
	else
		fbioc_stat_interval = app_data.stat_interval;

	switch (s->mode) {
	case MOD_BUSY:
	case MOD_NODISC:
	case MOD_STOP:
	case MOD_PAUSE:
		break;

	case MOD_SAMPLE:
		/* No optimization in these modes */
		fbioc_stat_interval = app_data.stat_interval;
		break;

	case MOD_PLAY:
		if (!iconified) {
			/* Force an immediate update */
			fbioc_stop_stat_poll();
			fbioc_start_stat_poll(s);
		}
		break;
	}
}


/*
 * fbioc_halt
 *	Shut down the FreeBSD/NetBSD/OpenBSD ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
fbioc_halt(curstat_t *s)
{
	/* If playing CDDA, stop it */
	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			(void) fbioc_start_stop(FALSE, FALSE);
			s->mode = MOD_STOP;
			fbioc_stop_stat_poll();
			break;
		default:
			break;
		}
	}

	/* Shut down CDDA */
	cdda_halt(fbioc_devp, s);
	app_data.play_mode = PLAYMODE_STD;

	/* Re-enable front-panel eject button */
	if (app_data.caddylock_supp)
		fbioc_lock(s, FALSE);

	if (s->mode != MOD_BUSY && s->mode != MOD_NODISC) {
		if (app_data.exit_eject && app_data.eject_supp) {
			/* User closing application: Eject disc */
			(void) fbioc_start_stop(FALSE, TRUE);
		}
		else {
			if (app_data.exit_stop)
				/* User closing application: Stop disc */
				fbioc_start_stop(FALSE, FALSE);

			switch (s->mode) {
			case MOD_PLAY:
			case MOD_PAUSE:
			case MOD_SAMPLE:
				fbioc_stop_stat_poll();
				break;
			default:
				break;
			}
		}
	}

	if (fbioc_devp != NULL) {
		fbioc_close();
		fbioc_not_open = TRUE;
	}
}


/*
 * fbioc_methodstr
 *	Return a text string indicating the current method.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Method text string.
 */
char *
fbioc_methodstr(void)
{
#ifdef __FreeBSD__
	return ("FreeBSD ioctl\n");
#endif
#ifdef __NetBSD__
	return ("NetBSD ioctl\n");
#endif
#ifdef __OpenBSD__
	return ("OpenBSD ioctl\n");
#endif
}


#endif	/* DI_FBIOC DEMO_ONLY */

