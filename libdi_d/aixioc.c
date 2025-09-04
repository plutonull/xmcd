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
 * IBM AIX IDE ioctl method module
 *
 * Contributing author: Cloyce D. Spradling
 * E-mail: cloyce@mail.utexas.edu
 *
 * This software fragment contains code that interfaces the CD player
 * application to IDE CD drives on the IBM AIX operating system.
 * The name "IBM" is used here for identification purposes only.
 */

#ifndef lint
static char *_aixioc_c_ident_ = "@(#)aixioc.c	6.133 04/02/08";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/aixioc.h"
#include "cdda_d/cdda.h"

#if defined(DI_AIXIOC) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;
extern sword32_t	di_clip_frames;


STATIC bool_t	aixioc_pause_resume(bool_t),
		aixioc_run_ab(curstat_t *),
		aixioc_run_sample(curstat_t *),
		aixioc_run_prog(curstat_t *),
		aixioc_run_repeat(curstat_t *),
		aixioc_disc_ready(curstat_t *),
		aixioc_disc_present(bool_t);
STATIC int	aixioc_cfg_vol(int, curstat_t *, bool_t);
STATIC void	aixioc_stat_poll(curstat_t *),
		aixioc_insert_poll(curstat_t *),
		aixioc_wait_display(sword32_t, curstat_t *),
		aixioc_dumpcmd(struct cd_audio_cmd *, int, int);


STATIC di_dev_t	*aixioc_devp = NULL;		/* CD device descriptor */
STATIC int	aixioc_stat_interval,		/* Status poll interval */
		aixioc_ins_interval;		/* Insert poll interval */
STATIC long	aixioc_stat_id,			/* Play status poll timer id */
		aixioc_insert_id,		/* Disc insert poll timer id */
		aixioc_search_id;		/* FF/REW timer id */
STATIC byte_t	aixioc_tst_status = MOD_NODISC,	/* Playback status on load */
		aixioc_sav_end_fmt,		/* Err recov saved end fmt */
		aixioc_sav_vol[4];		/* Saved vol for mute/unmute */
STATIC bool_t	aixioc_not_open = TRUE,		/* Device not opened yet */
		aixioc_stat_polling,		/* Polling play status */
		aixioc_insert_polling,		/* Polling disc insert */
		aixioc_new_progshuf,		/* New program/shuffle seq */
		aixioc_start_search,		/* Start FF/REW play segment */
		aixioc_idx_pause,		/* Prev/next index pausing */
		aixioc_fake_stop,		/* Force a completion status */
		aixioc_playing,			/* Currently playing */
		aixioc_paused,			/* Currently paused */
		aixioc_override_ap;		/* Override auto-play */
STATIC sword32_t aixioc_sav_end_addr;		/* Err recov saved end addr */
STATIC word32_t	aixioc_next_sam;		/* Next SAMPLE track */
STATIC msf_t	aixioc_sav_end_msf;		/* Err recov saved end MSF */
STATIC cdda_client_t aixioc_cdda_client;	/* CDDA client struct */


/* AIX IDE CDROM ioctl names */
STATIC iocname_t iname[] = {
	{ IDE_SINGLE,		"IDE_SINGLE" },	 /* Exclusive access mode */
	{ IOCINFO,		"IOCINFO" },	 /* Get device information */
	{ IDE_CDPMR,		"IDE_CDPMR" },	 /* Prohibit media removal */
	{ IDE_CDAMR,		"IDE_CDAMR" },	 /* Allow media removal */
	{ IDE_CDEJECT,		"IDE_CDEJECT" }, /* Eject media */
	{ IDE_CDAUDIO,		"IDE_CDAUDIO" }, /* Issue audio ctl commands */
	{ IDE_CDMODE,		"IDE_CDMODE" },	 /* Change CD-ROM data mode */
	{ IDE_CDIORDSE,		"IDE_CDIORDSE" },/* Read CD audio */
	{ 0,			NULL },
};

STATIC iocname_t cdcmds[] = {
	{ CD_PAUSE_AUDIO,	"CD_PAUSE_AUDIO" },
	{ CD_RESUME_AUDIO,	"CD_RESUME_AUDIO" },
	{ CD_STOP_AUDIO,	"CD_STOP_AUDIO" },
	{ CD_INFO_AUDIO,	"CD_INFO_AUDIO" },
	{ CD_TRK_INFO_AUDIO,	"CD_TRK_INFO_AUDIO" },
	{ CD_GET_TRK_MSF,	"CD_GET_TRK_MSF" },
	{ CD_SET_VOLUME,	"CD_SET_VOLUME" },
	{ 0,			NULL },
};

/* Selection flags for the aixioc_dumpcmd routine */
#define SEL_CMD		0x01
#define SEL_MSF		0x02
#define SEL_INDEXING	0x04
#define SEL_STATUS	0x08
#define SEL_VOLUME	0x10
#define SEL_CHANNEL	0x20	
#define SEL_DUMPALL	0xff

/* Subsection selection flags for SEL_INDEXING */
#define SUB_TRKIDX	0x01
#define SUB_MSF		0x02
#define SUB_INFO	0x04
#define SUB_TRKMSF	0x08
#define SUB_DUMPALL	0xff

/* Max number of frames to wait in aixioc_wait_display() */
#define MAX_WAIT_FRAMES	60


/***********************
 *  internal routines  *
 ***********************/


/*
 * aixioc_close
 *	Close CD device
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_close(void)
{
	if (aixioc_devp != NULL) {
		DBGPRN(DBG_DEVIO)(errfp, "\nClose device: %s\n",
			aixioc_devp->path);

		if (aixioc_devp->fd > 0)
			(void) close(aixioc_devp->fd);
		di_devfree(aixioc_devp);
		aixioc_devp = NULL;
	}
}


/*
 * aixioc_open
 *	Open CD device
 *
 * Args:
 *	path - device path name string
 *
 * Return:
 *	TRUE - open successful
 *	FALSE - open failed
 */
STATIC int
aixioc_open(char *path)
{
	struct stat	stbuf;
	char		errstr[ERR_BUF_SZ];

	DBGPRN(DBG_DEVIO)(errfp, "\nOpen device: %s\n", path);

	/* Check for validity of device node */
	if (stat(path, &stbuf) < 0) {
		(void) sprintf(errstr, app_data.str_staterr, path);
		DI_FATAL(errstr);
		return ENOENT;
	}

	if (!S_ISCHR(stbuf.st_mode)) {
		(void) sprintf(errstr, app_data.str_noderr, path);
		DI_FATAL(errstr);
		return EPERM;
	}

	if ((aixioc_devp = di_devalloc(path)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return ENOMEM;
	}

	if ((aixioc_devp->fd = openx(path, O_RDONLY, 0, IDE_SINGLE)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp, "Cannot open %s: errno=%d (%s)\n",
			path, errno, strerror(errno));
		di_devfree(aixioc_devp);
		aixioc_devp = NULL;
		return (errno);
	}

	return 0;
}


/*
 * aixioc_init_vol
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
aixioc_init_vol(curstat_t *s, bool_t preset)
{
	int	vol;

	/* Query current volume/balance settings */
	if ((vol = aixioc_cfg_vol(0, s, TRUE)) >= 0)
		s->level = (byte_t) vol;
	else
		s->level = 0;

	/* Set volume to preset value, if so configured */
	if (app_data.startup_vol > 0 && preset) {
		s->level_left = s->level_right = 100;

		if ((vol = aixioc_cfg_vol(app_data.startup_vol, s,
					  FALSE)) >= 0)
			s->level = (byte_t) vol;
	}

	/* Initialize sliders */
	SET_VOL_SLIDER(s->level);
	SET_BAL_SLIDER((int) (s->level_right - s->level_left) / 2);

	/* Set up channel routing */
	aixioc_route(s);
}


/*
 * aixioc_rdsubq
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
aixioc_rdsubq(cdstat_t *sp)
{
	sword32_t		tmpblk;
	struct cd_audio_cmd	cmd;
	curstat_t		*s = di_clinfo->curstat_addr();

	(void) memset(&cmd, 0, sizeof(cmd));
	cmd.audio_cmds = CD_INFO_AUDIO;
	if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE))
		return FALSE;

	/* Hack: to work around firmware anomalies in some CD drives. */
	if (cmd.indexing.info_audio.current_track >= MAXTRACK &&
	    cmd.indexing.info_audio.current_track != LEAD_OUT_TRACK) {
		sp->status = CDSTAT_NOSTATUS;
		return TRUE;
	}

	/* Map the subchannel data into cdstat_t form */

	switch (cmd.status) {
	case CD_PLAY_AUDIO:
		sp->status = CDSTAT_PLAYING;
		break;
	case CD_PAUSE_AUDIO:
		sp->status = CDSTAT_PAUSED;
		break;
	case CD_COMPLETED:
		sp->status = CDSTAT_COMPLETED;
		break;
	case CD_STATUS_ERROR:
		sp->status = CDSTAT_FAILED;
		break;
	case CD_NO_AUDIO:
	case CD_NOT_VALID:
	default:
		sp->status = CDSTAT_NOSTATUS;
		break;
	}

	sp->track = (int) cmd.indexing.info_audio.current_track;
	sp->index = (int) cmd.indexing.info_audio.current_index;

	/* Figure out the current absolute block */
	util_msftoblk(
		cmd.indexing.info_audio.current_mins,
		cmd.indexing.info_audio.current_secs,
		cmd.indexing.info_audio.current_frames,
		&tmpblk,
		MSF_OFFSET
	);

	/* Figure out MSF within the current track from the relative block */
	if (sp->index == 0)
		sp->rel_addr.addr =
			s->trkinfo[s->cur_trk - s->first_trk].addr - tmpblk;
	else
		sp->rel_addr.addr =
			tmpblk - s->trkinfo[s->cur_trk - s->first_trk].addr;

	if (sp->rel_addr.addr < 0)
		sp->rel_addr.addr = 0;
	util_blktomsf(
		sp->rel_addr.addr,
		&(sp->rel_addr.min),
		&(sp->rel_addr.sec),
		&(sp->rel_addr.frame),
		0
	);

	sp->abs_addr.frame = cmd.indexing.info_audio.current_frames;
	sp->abs_addr.sec = cmd.indexing.info_audio.current_secs;
	sp->abs_addr.min = cmd.indexing.info_audio.current_mins;
	sp->abs_addr.addr = tmpblk;

	return TRUE;
}


/*
 * aixioc_disc_present
 *	Check if a CD is loaded.
 *
 * Args:
 *	savstat - Whether to save start-up status in aixioc_tst_status.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure (drive not ready)
 */
STATIC bool_t
aixioc_disc_present(bool_t savstat)
{
	struct cd_audio_cmd	cmd;
	unsigned char		t1 = 0,
				t2 = 0;
	sword32_t		a1 = 0,
				a2 = 0;
	static int		tot_trks = 0;
	static sword32_t	sav_a1 = 0,
				sav_a2 = 0;

	if (savstat)
		aixioc_tst_status = MOD_NODISC;

	cmd.audio_cmds = CD_INFO_AUDIO;
	if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd,
			 (bool_t) ((app_data.debug & DBG_DEVIO) != 0))) {
		if (errno == ENOTREADY || (cmd.status & CD_NOT_VALID))
			return FALSE;	/* No disc */
		else {
			/* I don't know what happened */
			DBGPRN(DBG_DEVIO)(errfp,
				"CD_INFO_AUDIO errno=%d status=%x\n",
				errno, cmd.status);
			return FALSE;
		}
	}

	DBGPRN(DBG_DEVIO)(errfp, "disc_present: status=%x\n", cmd.status);

	switch (cmd.status) {
	case CD_PLAY_AUDIO:
		if (savstat) {
			aixioc_tst_status = MOD_PLAY;
			return TRUE;
		}
		break;
	case CD_PAUSE_AUDIO:
		if (savstat) {
			aixioc_tst_status = MOD_PAUSE;
			return TRUE;
		}
		break;
	case CD_STATUS_ERROR:
	case CD_NO_AUDIO:
	case CD_NOT_VALID:
	case CD_COMPLETED:
		break;
	default:
		return FALSE;
	}

	if (savstat)
		aixioc_tst_status = MOD_STOP;

	/* We didn't get anything useful from CD_INFO_AUDIO (unlikely),
	 * so try to read a TOC entry...
	 */
	cmd.audio_cmds = CD_TRK_INFO_AUDIO;
	cmd.msf_flag = FALSE;
	if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE))
		return FALSE;

	t1 = cmd.indexing.track_index.first_track;
	t2 = cmd.indexing.track_index.last_track;

	if (t1 == 0 && t2 == 0)
		return FALSE;

	if ((t2 - t1 + 1) != tot_trks) {
		/* Disc changed */
		tot_trks = t2 - t1 + 1;
		return FALSE;
	}

	cmd.msf_flag = TRUE;
	cmd.audio_cmds = CD_GET_TRK_MSF;
	cmd.indexing.track_msf.track = t1;
	if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE))
		return FALSE;

	util_msftoblk(
		cmd.indexing.track_msf.mins,
		cmd.indexing.track_msf.secs,
		cmd.indexing.track_msf.frames,
		&a1,
		0
	);

	cmd.msf_flag = TRUE;
	cmd.audio_cmds = CD_GET_TRK_MSF;
	cmd.indexing.track_msf.track = t2;
	if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE))
		return FALSE;

	util_msftoblk(
		cmd.indexing.track_msf.mins,
		cmd.indexing.track_msf.secs,
		cmd.indexing.track_msf.frames,
		&a2,
		0
	);

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
 * aixioc_playmsf
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
aixioc_playmsf(msf_t *start, msf_t *end)
{
	struct cd_audio_cmd	cmd;
#ifdef BUGGY_CDROM
	word32_t		stmpblk;

	/* Drives operating under this interface seem to be unable to
	 * start playing at *exactly* the right frame, so fudge it up a little.
	 * The maximum deviance that I saw on my small test sample was 16
	 * frames, and we'll fudge 16 (it's over 1/5th second, so I don't want
	 * to do much more than that).
	 * To ensure that the secs and mins are updated correctly (i.e. we
	 * don't try to play frame 85 somewhere), we convert it to a block
	 * address, add the fudge factor, and then reconvert to MSF.  Yuck!
	 */
	util_msftoblk(start->min, start->sec, start->frame, &tmpblk, 0);
	tmpblk += 16;
	util_blktomsf(tmpblk, &(start->min),
		      &(start->sec), &(start->frame), 0);
#endif

	cmd.msf_flag = TRUE;
	cmd.audio_cmds = CD_PLAY_AUDIO;

	/* If the start or end positions are less than the minimum
	 * position, patch them to the minimum positions.
	 */
	if (start->min == 0 && start->sec < 2) {
		cmd.indexing.msf.first_mins = 0;
		cmd.indexing.msf.first_secs = 2;
		cmd.indexing.msf.first_frames = 0;
	}
	else {
		cmd.indexing.msf.first_mins = start->min;
		cmd.indexing.msf.first_secs = start->sec;
		cmd.indexing.msf.first_frames = start->frame;
	}

	if (end->min == 0 && end->sec < 2) {
		cmd.indexing.msf.last_mins = 0;
		cmd.indexing.msf.last_secs = 2;
		cmd.indexing.msf.last_frames = 0;
	}
	else {
		cmd.indexing.msf.last_mins = end->min;
		cmd.indexing.msf.last_secs = end->sec;
		cmd.indexing.msf.last_frames = end->frame;
	}

	/* If start == end, just return success */
	if (cmd.indexing.msf.first_mins == cmd.indexing.msf.last_mins &&
	    cmd.indexing.msf.first_secs == cmd.indexing.msf.last_secs &&
	    cmd.indexing.msf.first_frames == cmd.indexing.msf.last_frames)
		return TRUE;

	return (aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE));
}


/*
 * aixioc_play_trkidx
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
aixioc_play_trkidx(int start_trk, int start_idx, int end_trk, int end_idx)
{
	struct cd_audio_cmd	cmd;

	cmd.audio_cmds = CD_PLAY_AUDIO;
	cmd.msf_flag = FALSE;
	cmd.indexing.track_index.first_track = start_trk;
	cmd.indexing.track_index.last_track = end_trk;
	cmd.indexing.track_index.first_index = start_idx;
	cmd.indexing.track_index.last_index = end_idx;

	if (app_data.debug & DBG_DEVIO)
		aixioc_dumpcmd(&cmd, SEL_CMD | SEL_INDEXING, SUB_TRKIDX);

	return (aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE));
}


/*
 * aixioc_start_stop
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
aixioc_start_stop(bool_t start, bool_t loej)
{
	struct cd_audio_cmd	cmd;
	bool_t			ret = FALSE;
	curstat_t		*s = di_clinfo->curstat_addr();

	if (app_data.strict_pause_resume && aixioc_paused)
		(void) aixioc_pause_resume(TRUE);

	if (start) {
		/* Can't do anything about it */
		return (!loej);
	}
	else {
		aixioc_playing = FALSE;

		if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
			(void) cdda_stop(aixioc_devp, s);

			if (!aixioc_is_enabled(DI_ROLE_MAIN)) {
				/* Enable I/O from current process */
				aixioc_enable(DI_ROLE_MAIN);
			}
		}

		cmd.audio_cmds = CD_STOP_AUDIO;
		ret = aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO,
				  (void *) &cmd, TRUE);
		if (ret && loej) {
			ret = aixioc_send(
				DI_ROLE_MAIN, IDE_CDEJECT, 0,
				(bool_t) ((app_data.debug & DBG_DEVIO) != 0)
			);

			/* Delay a bit to let the CD load or eject.
			 * This is a hack to work around firmware bugs
			 * in some CD drives.  These drives don't
			 * handle new commands well when the CD is
			 * loading/ejecting with the IMMED bit set in
			 * the Start/Stop Unit command.
			 */
			util_delayms((app_data.ins_interval + 1) * 1000);
		}
	}

	return (ret);
}


/*
 * aixioc_pause_resume
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
aixioc_pause_resume(bool_t resume)
{
	struct cd_audio_cmd	cmd;
	bool_t			ret;
	curstat_t		*s = di_clinfo->curstat_addr();

	if (!app_data.pause_supp)
		return FALSE;

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		ret = cdda_pause_resume(aixioc_devp, s, resume);
	}
	else {
		cmd.audio_cmds = resume ? CD_RESUME_AUDIO : CD_PAUSE_AUDIO;

		ret = aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO,
				  (void *) &cmd, TRUE);
	}
	if (ret)
		aixioc_paused = !resume;

	return (ret);
}


/*
 * aixioc_do_playaudio
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
aixioc_do_playaudio(
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
		aixioc_sav_end_msf = *end_msf;
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
		aixioc_sav_end_addr = end_addr;
	}

	/* Save end address format for error recovery */
	aixioc_sav_end_fmt = addr_fmt;

	do_playmsf = (addr_fmt & ADDR_MSF) && app_data.playmsf_supp;
	do_play10 = (addr_fmt & ADDR_BLK) && app_data.play10_supp;
	do_play12 = (addr_fmt & ADDR_BLK) && app_data.play12_supp;
	do_playti = (addr_fmt & ADDR_TRKIDX) && app_data.playti_supp;

	if (do_playmsf || do_playti) {
		if (aixioc_paused) {
			if (app_data.strict_pause_resume) {
				/* Resume first */
				(void) aixioc_pause_resume(TRUE);
			}
		}
		else if (aixioc_playing) {
			if (app_data.play_pause_play) {
				/* Pause first */
				(void) aixioc_pause_resume(FALSE);
			}
		}
	}

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		if (do_play12 || do_play10) {
			if (aixioc_is_enabled(DI_ROLE_MAIN)) {
				/* Disable I/O from current process */
				aixioc_disable(DI_ROLE_MAIN);
			}

			ret = cdda_play(aixioc_devp, s, start_addr, end_addr);
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
			
			ret = cdda_play(aixioc_devp, s, start_addr, end_addr);
		}
	}
	else {
		if (do_playmsf)
			ret = aixioc_playmsf(start_msf, emsfp);

		if (!ret && do_playti)
			ret = aixioc_play_trkidx(trk, idx, trk, idx);
	}

	if (ret) {
		aixioc_playing = TRUE;
		aixioc_paused = FALSE;
	}

	return (ret);
}


/*
 * aixioc_play_recov
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
aixioc_play_recov(sword32_t blk, bool_t iserr)
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
	if (recov_start_msf.min > aixioc_sav_end_msf.min)
		ret = FALSE;
	else if (recov_start_msf.min == aixioc_sav_end_msf.min) {
		if (recov_start_msf.sec > aixioc_sav_end_msf.sec)
			ret = FALSE;
		else if ((recov_start_msf.sec ==
			  aixioc_sav_end_msf.sec) &&
			 (recov_start_msf.frame >
			  aixioc_sav_end_msf.frame)) {
			ret = FALSE;
		}
	}
	if (recov_start_addr >= aixioc_sav_end_addr)
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

		ret = aixioc_do_playaudio(
			aixioc_sav_end_fmt,
			recov_start_addr, aixioc_sav_end_addr,
			&recov_start_msf, &aixioc_sav_end_msf,
			0, 0
		);
	}

	return (ret);
}


/*
 * aixioc_get_playstatus
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
aixioc_get_playstatus(curstat_t *s)
{
	cdstat_t		cdstat;
	word32_t		curtrk,
	    			curidx;
	bool_t			ret,
				done;
	static int		errcnt = 0,
				nostatcnt = 0;
	static sword32_t	errblk = 0;
	static bool_t		in_aixioc_get_playstatus = FALSE;

	/* Lock this routine from multiple entry */
	if (in_aixioc_get_playstatus)
		return TRUE;

	in_aixioc_get_playstatus = TRUE;

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		ret = cdda_getstatus(aixioc_devp, s, &cdstat);
		if (ret &&
		    (cdstat.level != s->level ||
		     cdstat.level_left  != s->level_left ||
		     cdstat.level_right != s->level_right)) {
			int	vol;

			/* Update volume & balance level */
			s->level_left = cdstat.level_left;
			s->level_right = cdstat.level_right;
			vol = aixioc_cfg_vol((int) cdstat.level, s, FALSE);
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
		ret = aixioc_rdsubq(&cdstat);

	if (!ret) {
		/* Check to see if the disc had been manually ejected */
		if (!aixioc_disc_ready(s)) {
			aixioc_sav_end_addr = 0;
			aixioc_sav_end_msf.min = 0;
			aixioc_sav_end_msf.sec = 0;
			aixioc_sav_end_msf.frame = 0;
			aixioc_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_aixioc_get_playstatus = FALSE;
			return FALSE;
		}

		/* The read subchannel command failed for some
		 * unknown reason.  Just return success and
		 * hope the next poll succeeds.  We don't want
		 * to return FALSE here because that would stop
		 * the poll.
		 */
		in_aixioc_get_playstatus = FALSE;
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

	/* Hack: to work around the fact that some CD drives
	 * return CD_PAUSE_AUDIO status after issuing a CD_STOP_AUDIO command.
	 * Just treat the status as completed if we get a paused status
	 * and we don't expect the drive to be paused.
	 */
	if (cdstat.status == CDSTAT_PAUSED && s->mode != MOD_PAUSE &&
	    !aixioc_idx_pause)
		cdstat.status = CDSTAT_COMPLETED;

	/* Force completion status */
	if (aixioc_fake_stop)
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
		if (!aixioc_disc_ready(s)) {
			aixioc_sav_end_addr = 0;
			aixioc_sav_end_msf.min = 0;
			aixioc_sav_end_msf.sec = 0;
			aixioc_sav_end_msf.frame = 0;
			aixioc_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_aixioc_get_playstatus = FALSE;
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

		if (!done && aixioc_play_recov(errblk, TRUE)) {
			in_aixioc_get_playstatus = FALSE;
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

		if (!aixioc_fake_stop)
			aixioc_playing = FALSE;

		aixioc_fake_stop = FALSE;

		switch (s->mode) {
		case MOD_SAMPLE:
			done = !aixioc_run_sample(s);
			break;

		case MOD_PLAY:
		case MOD_PAUSE:
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			if (s->shuffle || s->program)
				done = !aixioc_run_prog(s);

			if (s->repeat)
				done = !aixioc_run_repeat(s);

			if (s->repeat && s->segplay == SEGP_AB)
				done = !aixioc_run_ab(s);

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

		aixioc_sav_end_addr = 0;
		aixioc_sav_end_msf.min = aixioc_sav_end_msf.sec =
			aixioc_sav_end_msf.frame = 0;
		aixioc_sav_end_fmt = 0;
		errcnt = 0;
		errblk = 0;
		s->rptcnt = 0;
		DPY_ALL(s);

		if (app_data.done_eject) {
			/* Eject the disc */
			aixioc_load_eject(s);
		}
		else {
			/* Spin down the disc */
			(void) aixioc_start_stop(FALSE, FALSE);
		}
		if (app_data.done_exit) {
			/* Exit */
			di_clinfo->quit(s);
		}

		in_aixioc_get_playstatus = FALSE;
		return FALSE;
	}

	in_aixioc_get_playstatus = FALSE;
	return TRUE;
}


/*
 * aixioc_cfg_vol
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
aixioc_cfg_vol(int vol, curstat_t *s, bool_t query)
{
	struct cd_audio_cmd	cmd;
	int			vol1,
				vol2;

	if (!app_data.mselvol_supp)
		return 0;

	if (s->mode == MOD_BUSY)
		return -1;

	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		return (cdda_vol(aixioc_devp, s, vol, query));

	if (query) {
		cmd.audio_cmds = CD_INFO_AUDIO;

		if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO,
				 (void *) &cmd, TRUE))
			return 0;

		vol1 = util_untaper_vol(
			util_unscale_vol(cmd.out_port_0_vol)
		);
		vol2 = util_untaper_vol(
			util_unscale_vol(cmd.out_port_1_vol)
		);

		if (vol1 == vol2) {
			s->level_left = s->level_right = 100;
			vol = vol1;
		}
		else if (vol1 > vol2) {
			s->level_left = 100;
			s->level_right = (byte_t) ((vol2 * 100) / vol1);
			vol = vol1;
		}
		else {
			s->level_left = (byte_t) ((vol1 * 100) / vol2);
			s->level_right = 100;
			vol = vol2;
		}
		return (vol);
	}
	else {
		cmd.audio_cmds = CD_SET_VOLUME;
		cmd.volume_type = CD_VOLUME_CHNLS;
		cmd.out_port_0_vol = util_scale_vol(
			util_taper_vol(vol * (int) s->level_left / 100)
		);
		cmd.out_port_1_vol = util_scale_vol(
			util_taper_vol(vol * (int) s->level_right / 100)
		);

		if (aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO,
				(void *) &cmd, TRUE)) {
			if (app_data.debug & DBG_DEVIO) {
				cmd.audio_cmds = CD_INFO_AUDIO;
				(void) aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO,
						   (void *) &cmd, TRUE);
				aixioc_dumpcmd(&cmd,
					       SEL_CMD | SEL_VOLUME,
					       SUB_DUMPALL);
			}
			return (vol);
		}
		else if (cmd.out_port_0_vol != cmd.out_port_1_vol) {
			/* Set the balance to the center
			 * and retry.
			 */
			cmd.audio_cmds = CD_SET_VOLUME;
			cmd.volume_type = CD_VOLUME_ALL;
			cmd.all_channel_vol = util_scale_vol(
				util_taper_vol(vol)
			);

			if (aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO,
					(void *) &cmd, TRUE)) {
				/* Success: Warp balance control */
				s->level_left = s->level_right = 100;
				SET_BAL_SLIDER(0);

				if (app_data.debug & DBG_DEVIO) {
					cmd.audio_cmds = CD_INFO_AUDIO;
					(void) aixioc_send(DI_ROLE_MAIN,
							   IDE_CDAUDIO,
							   (void *) &cmd,
							   TRUE);
					aixioc_dumpcmd(&cmd,
						       SEL_CMD | SEL_VOLUME,
						       SUB_DUMPALL);
				}

				return (vol);
			}

			/* Still failed: just drop through */
		}
	}

	return -1;
}


/*
 * aixioc_vendor_model
 *	Query and update CD drive vendor/model/revision information
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_vendor_model(curstat_t *s)
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
 * aixioc_fix_toc
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
aixioc_fix_toc(curstat_t *s)
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
 * aixioc_get_toc
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
aixioc_get_toc(curstat_t *s)
{
	int			i = 0,
				currtrk = 0,
				ntrks = 0;
	struct cd_audio_cmd	cmdbuf;
	struct devinfo		cdinfo;

	cmdbuf.msf_flag = FALSE;
	cmdbuf.audio_cmds = CD_TRK_INFO_AUDIO;
	if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmdbuf, TRUE))
		return FALSE;

	if (app_data.debug & DBG_DEVIO) {
		(void) fprintf(errfp, "\nBegin get TOC operation");
		aixioc_dumpcmd(&cmdbuf, SEL_INDEXING | SEL_CMD, SUB_TRKIDX);
	}

	/* Fill curstat structure with TOC data */
	s->first_trk = cmdbuf.indexing.track_index.first_track;
	s->last_trk = cmdbuf.indexing.track_index.last_track;

	ntrks = (int) (s->last_trk - s->first_trk) + 1;

	/* Do it again to get the last MSF data */
	cmdbuf.msf_flag = TRUE;
	aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmdbuf, TRUE);

	if (app_data.debug & DBG_DEVIO)
		aixioc_dumpcmd(&cmdbuf, SEL_INDEXING | SEL_CMD, SUB_MSF);

	/* We're going to want MSF now */
	cmdbuf.audio_cmds = CD_GET_TRK_MSF;

	for (i = 0; i < ntrks; i++) {
		currtrk = i + s->first_trk;
		s->trkinfo[i].trkno = currtrk;
		s->trkinfo[i].type = TYP_AUDIO;	/* shrug */

		/* Get the track info */
		cmdbuf.indexing.track_msf.track = currtrk;
		aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmdbuf, TRUE);
		if (app_data.debug & DBG_DEVIO) {
			(void) fprintf(errfp, "\nTrack #%d", currtrk);
			aixioc_dumpcmd(&cmdbuf, SEL_INDEXING | SEL_CMD,
				       SUB_TRKMSF);
		}

		s->trkinfo[i].min = cmdbuf.indexing.track_msf.mins;
		s->trkinfo[i].sec = cmdbuf.indexing.track_msf.secs;
		s->trkinfo[i].frame = cmdbuf.indexing.track_msf.frames;
		util_msftoblk(
			s->trkinfo[i].min,
			s->trkinfo[i].sec,
			s->trkinfo[i].frame,
			&s->trkinfo[i].addr,
			MSF_OFFSET
		);

		if (s->trkinfo[i-1].trkno == s->last_trk ||
		    i == (MAXTRACK - 1)) {
			s->discpos_tot.min = s->trkinfo[i].min;
			s->discpos_tot.sec = s->trkinfo[i].sec;
			s->discpos_tot.frame = s->trkinfo[i].frame;
			s->tot_trks = (byte_t) i;
			s->discpos_tot.addr = s->trkinfo[i].addr;
			s->last_trk = s->trkinfo[i-1].trkno;

			aixioc_fix_toc(s);
			return TRUE;
		}
	}

	(void) aixioc_send(DI_ROLE_MAIN, IOCINFO, (void *) &cdinfo, TRUE);
	/* Fake up a lead-out track */
	util_blktomsf(
		cdinfo.un.idecd.numblks,
		&(s->trkinfo[i].min),
		&(s->trkinfo[i].sec),
		&(s->trkinfo[i].frame),
		MSF_OFFSET
	);
	s->trkinfo[i].trkno = LEAD_OUT_TRACK;

	s->discpos_tot.addr = s->trkinfo[i].addr = cdinfo.un.idecd.numblks;
	/* may need to add offset */
	s->discpos_tot.min = s->trkinfo[i].min;
	s->discpos_tot.sec = s->trkinfo[i].sec;
	s->discpos_tot.frame = s->trkinfo[i].frame;
	s->tot_trks = (byte_t) i;

	aixioc_fix_toc(s);
	return TRUE;
}


/*
 * aixioc_start_stat_poll
 *	Start polling the drive for current playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_start_stat_poll(curstat_t *s)
{
	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		aixioc_stat_id = di_clinfo->timeout(
			aixioc_stat_interval,
			aixioc_stat_poll,
			(byte_t *) s
		);

		if (aixioc_stat_id != 0)
			aixioc_stat_polling = TRUE;
	}
}


/*
 * aixioc_stop_stat_poll
 *	Stop polling the drive for current playback status
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_stop_stat_poll(void)
{
	if (aixioc_stat_polling) {
		/* Stop poll timer */
		if (di_clinfo->untimeout != NULL)
			di_clinfo->untimeout(aixioc_stat_id);

		aixioc_stat_polling = FALSE;
	}
}


/*
 * aixioc_start_insert_poll
 *	Start polling the drive for disc insertion
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_start_insert_poll(curstat_t *s)
{
	int	delay;
	bool_t	first = TRUE;

	if (aixioc_insert_polling || app_data.ins_disable ||
	    (s->mode != MOD_BUSY && s->mode != MOD_NODISC))
		return;

	if (app_data.numdiscs > 1 && app_data.multi_play)
		aixioc_ins_interval =
			app_data.ins_interval / app_data.numdiscs;
	else
		aixioc_ins_interval = app_data.ins_interval;

	if (aixioc_ins_interval < 500)
		aixioc_ins_interval = 500;

	if (first) {
		first = FALSE;
		delay = 50;
	}
	else
		delay = aixioc_ins_interval;

	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		aixioc_insert_id = di_clinfo->timeout(
			delay,
			aixioc_insert_poll,
			(byte_t *) s
		);

		if (aixioc_insert_id != 0)
			aixioc_insert_polling = TRUE;
	}
}


/*
 * aixioc_stat_poll
 *	The playback status polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_stat_poll(curstat_t *s)
{
	if (!aixioc_stat_polling)
		return;

	/* Get current audio playback status */
	if (aixioc_get_playstatus(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			aixioc_stat_id = di_clinfo->timeout(
				aixioc_stat_interval,
				aixioc_stat_poll,
				(byte_t *) s
			);
		}
	}
	else
		aixioc_stat_polling = FALSE;
}


/*
 * aixioc_insert_poll
 *	The disc insertion polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_insert_poll(curstat_t *s)
{
	/* Check to see if a disc is inserted */
	if (!aixioc_disc_ready(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			aixioc_insert_id = di_clinfo->timeout(
				aixioc_ins_interval,
				aixioc_insert_poll,
				(byte_t *) s
			);
		}
	}
	else
		aixioc_insert_polling = FALSE;
}


/*
 * aixioc_disc_ready
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
aixioc_disc_ready(curstat_t *s)
{
	int		ret;
	bool_t		no_disc;
	static bool_t	first_open = TRUE,
			in_aixioc_disc_ready = FALSE;

	/* Lock this routine from multiple entry */
	if (in_aixioc_disc_ready)
		return TRUE;

	in_aixioc_disc_ready = TRUE;

	/* If device has not been opened, attempt to open it */
	if (aixioc_not_open) {
		/* Check for another copy of the CD player running on
		 * the specified device.
		 */
		if (!s->devlocked && !di_devlock(s, app_data.device)) {
			s->mode = MOD_BUSY;
			DPY_TIME(s, FALSE);
			aixioc_start_insert_poll(s);
			in_aixioc_disc_ready = FALSE;
			return FALSE;
		}

		s->devlocked = TRUE;
		s->mode = MOD_NODISC;

		if ((ret = aixioc_open(s->curdev)) == 0) {
			aixioc_not_open = FALSE;

			if (!aixioc_is_enabled(DI_ROLE_MAIN)) {
				/* Enable device for I/O */
				aixioc_enable(DI_ROLE_MAIN);
			}

			/* Check if a disc is loaded and ready */
			no_disc = !aixioc_disc_present(first_open);
		}
		else {
			DBGPRN(DBG_DEVIO)(errfp,
				"Open of %s failed\n", s->curdev);
			no_disc = TRUE;
			if (ret == EACCES || ret == EBUSY)
				s->mode = MOD_BUSY;	/* Already in use */
		}
	}
	else {
		/* Just return success if we're playing CDDA */
		if (!aixioc_is_enabled(DI_ROLE_MAIN)) {
			no_disc = FALSE;
		}
		else if ((no_disc = !aixioc_disc_present(FALSE)) == TRUE) {
			/* The disc was manually ejected */
			s->mode = MOD_NODISC;
			di_clear_cdinfo(s, FALSE);
		}
	}

	if (!no_disc) {
		if (first_open) {
			first_open = FALSE;

			/* Fill in inquiry data */
			aixioc_vendor_model(s);

			/* Initialize volume/balance/routing controls */
			aixioc_init_vol(s, TRUE);
		}
		else {
			/* Force to current settings */
			(void) aixioc_cfg_vol(s->level, s, FALSE);

			/* Set up channel routing */
			aixioc_route(s);
		}
	}

	/* Read disc table of contents if a new disc was detected */
	if (aixioc_not_open || no_disc ||
	    (s->mode == MOD_NODISC && !aixioc_get_toc(s))) {
		if (aixioc_devp != NULL && app_data.eject_close) {
			aixioc_close();
			aixioc_not_open = TRUE;
		}
			
		di_reset_curstat(s, TRUE, TRUE);
		DPY_ALL(s);

		aixioc_start_insert_poll(s);
		in_aixioc_disc_ready = FALSE;
		return FALSE;
	}

	if (s->mode != MOD_NODISC) {
		in_aixioc_disc_ready = FALSE;
		return TRUE;
	}

	/* Load saved track program, if any */
	PROGGET(s);

	s->mode = MOD_STOP;
	DPY_ALL(s);

	/* Set caddy lock configuration */
	if (app_data.caddylock_supp)
		aixioc_lock(s, app_data.caddy_lock);

	if (app_data.load_play) {
		/* Start auto-play */
		if (!aixioc_override_ap)
			aixioc_play_pause(s);
	}
	else if (app_data.load_spindown) {
		/* Spin down disc in case the user isn't going to
		 * play anything for a while.  This reduces wear and
		 * tear on the drive.
		 */
		(void) aixioc_start_stop(FALSE, FALSE);
	}
	else {
		switch (aixioc_tst_status) {
		case MOD_PLAY:
		case MOD_PAUSE:
			/* Drive is current playing audio or paused:
			 * act appropriately.
			 */
			s->mode = aixioc_tst_status;
			(void) aixioc_get_playstatus(s);
			DPY_ALL(s);
			if (s->mode == MOD_PLAY)
				aixioc_start_stat_poll(s);
			break;
		default:
			/* Drive is stopped: do nothing */
			break;
		}
	}

	in_aixioc_disc_ready = FALSE;

	/* Load CD information for this disc.
	 * This operation has to be done outside the scope of
	 * in_aixioc_disc_ready because it may recurse
	 * back into this function.
	 */
	(void) di_get_cdinfo(s);

	return TRUE;
}


/*
 * aixioc_run_rew
 *	Run search-rewind operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_run_rew(curstat_t *s)
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
	if (!aixioc_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (aixioc_start_search) {
		aixioc_start_search = FALSE;
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
	(void) aixioc_do_playaudio(
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		aixioc_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			aixioc_run_rew,
			(byte_t *) s
		);
	}
}


/*
 * aixioc_stop_rew
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
aixioc_stop_rew(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(aixioc_search_id);
}


/*
 * aixioc_run_ff
 *	Run search-fast-forward operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_run_ff(curstat_t *s)
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
	if (!aixioc_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (aixioc_start_search) {
		aixioc_start_search = FALSE;
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
	(void) aixioc_do_playaudio(
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		aixioc_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			aixioc_run_ff,
			(byte_t *) s
		);
	}
}


/*
 * aixioc_stop_ff
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
aixioc_stop_ff(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(aixioc_search_id);
}


/*
 * aixioc_run_ab
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
aixioc_run_ab(curstat_t *s)
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
	aixioc_start_stat_poll(s);

	return (
		aixioc_do_playaudio(
			ADDR_BLK | ADDR_MSF,
			s->bp_startpos_tot.addr, s->bp_endpos_tot.addr,
			&start_msf, &end_msf,
			0, 0
		)
	);
}


/*
 * aixioc_run_sample
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
aixioc_run_sample(curstat_t *s)
{
	sword32_t	saddr,
			eaddr;
	msf_t		smsf,
			emsf;

	if (aixioc_next_sam < s->tot_trks) {
		saddr = s->trkinfo[aixioc_next_sam].addr;
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

		if (s->trkinfo[aixioc_next_sam].type != TYP_AUDIO ||
		    aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				        saddr, eaddr, &smsf, &emsf, 0, 0)) {
			aixioc_next_sam++;
			return TRUE;
		}
	}

	aixioc_next_sam = 0;
	return FALSE;
}


/*
 * aixioc_run_prog
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
aixioc_run_prog(curstat_t *s)
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

	if (aixioc_new_progshuf) {
		aixioc_new_progshuf = FALSE;

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

	ret = aixioc_do_playaudio(
		ADDR_BLK | ADDR_MSF,
		start_addr, end_addr,
		&start_msf, &end_msf,
		0, 0
	);

	/* Wait for the CD to play to where we tell it to... */
	aixioc_wait_display(start_addr, s);

	if (s->mode == MOD_PAUSE) {
		(void) aixioc_pause_resume(FALSE);

		/* Restore volume */
		aixioc_mute_off(s);
	}

	return (ret);
}


/*
 * aixioc_run_repeat
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
aixioc_run_repeat(curstat_t *s)
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

		aixioc_new_progshuf = TRUE;
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

		ret = aixioc_do_playaudio(
			ADDR_BLK | ADDR_MSF,
			s->trkinfo[0].addr, s->discpos_tot.addr,
			&start_msf, &end_msf, 0, 0
		);

		if (s->mode == MOD_PAUSE) {
			(void) aixioc_pause_resume(FALSE);

			/* Restore volume */
			aixioc_mute_off(s);
		}

	}

	return (ret);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * aixioc_enable
 *	Enable device in this process for I/O
 *
 * Args:
 *	role - Role id for which I/O is to be enabled
 *
 * Return:
 *	Nothing.
 */
void
aixioc_enable(int role)
{
	DBGPRN(DBG_DEVIO)(errfp, "Enable device: %s role: %d\n",
			  aixioc_devp->path, role);
	aixioc_devp->role = role;
}


/*
 * aixioc_disable
 *	Disable device in this process for I/O
 *
 * Args:
 *	role - Role id for which I/O is to be disabled
 *
 * Return:
 *	Nothing.
 */
void
aixioc_disable(int role)
{
	if (aixioc_devp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp, "aixioc_disable: invalid role: %d\n",
				  role);
		return;
	}

	DBGPRN(DBG_DEVIO)(errfp, "Disable device: %s role: %d\n",
			  aixioc_devp->path, role);
	aixioc_devp->role = 0;
}


/*
 * aixioc_is_enabled
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
aixioc_is_enabled(int role)
{
	return ((bool_t) (aixioc_devp->role == role));
}


/*
 * aixioc_send
 *	Issue ioctl command.
 *
 * Args:
 *	role - role id for which I/O is to be performed
 *	cmd - ioctl command
 *	arg - ioctl argument
 *	prnerr - whether an error message is to be displayed if the ioctl fails
 *
 * Return:
 *	TRUE - ioctl successful
 *	FALSE - ioctl failed
 */
bool_t
aixioc_send(int role, int cmd, void *arg, bool_t prnerr)
{
	int			i,
				j;
	struct cd_audio_cmd	*ac = NULL;
	curstat_t		*s = di_clinfo->curstat_addr();

	if (aixioc_devp == NULL || aixioc_devp->fd <= 0)
		return FALSE;

	if (aixioc_devp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp,
			"NOTICE: aixioc_send: disabled role! (%d)\n",
			role);
		return FALSE;
	}

	if (cmd == IDE_CDAUDIO) {
		ac = (struct cd_audio_cmd *) arg;
		ac->status = 0;	/* Nuke status for audio cmds */
	}

	if (app_data.debug & DBG_DEVIO) {
		for (i = 0; iname[i].name != NULL; i++) {
			if (iname[i].cmd == cmd) {
			    (void) fprintf(errfp, "\nIOCTL: %s",
				           iname[i].name);
			    if (cmd == IDE_CDAUDIO) {
				/* arg will be cd_audio_cmd struct */
				ac = (struct cd_audio_cmd *) arg;
				for (j = 0; cdcmds[j].name != NULL; j++) {
				    if (cdcmds[j].cmd == ac->audio_cmds) {
					    (void) fprintf(errfp, " subcmd=%s",
							   cdcmds[j].name);
					    break;
				    }
				}
			    }
			    (void) fprintf(errfp, "\n");
			    break;
			}
		}
		if (iname[i].name == NULL)
			(void) fprintf(errfp, "\nIOCTL: 0x%x\n", cmd);
	}

	if (ioctl(aixioc_devp->fd, cmd, arg) < 0) {
		if (prnerr) {
			(void) fprintf(errfp, "CD audio: ioctl error on %s:\n",
				s->curdev);

			for (i = 0; iname[i].name != NULL; i++) {
			    if (iname[i].cmd == cmd) {
				(void) fprintf(errfp, "cmd=%s ",
						iname[i].name);
				if (cmd == IDE_CDAUDIO) {
				    /* arg will be cd_audio_cmd struct */
				    ac = (struct cd_audio_cmd *) arg;
				    for (j = 0; cdcmds[j].name != NULL; j++) {
					if (cdcmds[j].cmd == ac->audio_cmds) {
						(void) fprintf(errfp,
							      "subcmd=%s ",
							      cdcmds[j].name);
						break;
					}
				    }
				}
				(void) fprintf(errfp, "errno=%d\n(%s)\n",
					       errno, strerror(errno));
				if (cmd == IDE_CDAUDIO)
					aixioc_dumpcmd(ac, SEL_DUMPALL,
						       SUB_DUMPALL);
				break;
			    }
			}
			if (iname[i].name == NULL)
				(void) fprintf(errfp,
					       "cmd=0x%x errno=%d\n(%s)\n",
					       cmd, errno, strerror(errno));
		}
		return FALSE;
	}

	return TRUE;
}


/*
 * aixioc_init
 *	Top-level function to initialize the AIX IDE ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_init(curstat_t *s, di_tbl_t *dt)
{
	if (app_data.di_method != DI_AIXIOC)
		/* AIX IDE ioctl method not configured */
		return;

	/* Initialize libdi calling table */
	dt->load_cdtext = NULL;
	dt->playmode = aixioc_playmode;
	dt->check_disc = aixioc_check_disc;
	dt->status_upd = aixioc_status_upd;
	dt->lock = aixioc_lock;
	dt->repeat = aixioc_repeat;
	dt->shuffle = aixioc_shuffle;
	dt->load_eject = aixioc_load_eject;
	dt->ab = aixioc_ab;
	dt->sample = aixioc_sample;
	dt->level = aixioc_level;
	dt->play_pause = aixioc_play_pause;
	dt->stop = aixioc_stop;
	dt->chgdisc = aixioc_chgdisc;
	dt->prevtrk = aixioc_prevtrk;
	dt->nexttrk = aixioc_nexttrk;
	dt->previdx = aixioc_previdx;
	dt->nextidx = aixioc_nextidx;
	dt->rew = aixioc_rew;
	dt->ff = aixioc_ff;
	dt->warp = aixioc_warp;
	dt->route = aixioc_route;
	dt->mute_on = aixioc_mute_on;
	dt->mute_off = aixioc_mute_off;
	dt->cddajitter = aixioc_cddajitter;
	dt->debug = aixioc_debug;
	dt->start = aixioc_start;
	dt->icon = aixioc_icon;
	dt->halt = aixioc_halt;
	dt->methodstr = aixioc_methodstr;

	/* Initalize AIX IDE ioctl method */
	aixioc_stat_polling = FALSE;
	aixioc_stat_interval = app_data.stat_interval;
	aixioc_insert_polling = FALSE;
	aixioc_next_sam = FALSE;
	aixioc_new_progshuf = FALSE;
	aixioc_sav_end_addr = 0;
	aixioc_sav_end_msf.min = aixioc_sav_end_msf.sec =
		aixioc_sav_end_msf.frame = 0;
	aixioc_sav_end_fmt = 0;

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

	DBGPRN(DBG_DEVIO)(errfp, "libdi: AIX IDE ioctl method\n");
}


/*
 * aixioc_playmode
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
aixioc_playmode(curstat_t *s)
{
	bool_t		ret,
			cdda;
	static bool_t	prev_cdda = FALSE;

	cdda = (bool_t) PLAYMODE_IS_CDDA(app_data.play_mode);

	if (cdda == prev_cdda)
		return TRUE;	/* No change */

	if (cdda) {
		aixioc_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		aixioc_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		aixioc_cdda_client.warning_msg = di_clinfo->warning_msg;
		aixioc_cdda_client.info_msg = di_clinfo->info_msg;
		aixioc_cdda_client.info2_msg = di_clinfo->info2_msg;

		ret = cdda_init(s, &aixioc_cdda_client);
	}
	else {
		cdda_halt(aixioc_devp, s);
		ret = TRUE;

		/* Initialize volume/balance/routing controls */
		aixioc_init_vol(s, FALSE);
	}

	if (ret)
		prev_cdda = cdda;

	return (ret);
}


/*
 * aixioc_check_disc
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
aixioc_check_disc(curstat_t *s)
{
	return (aixioc_disc_ready(s));
}


/*
 * aixioc_status_upd
 *	Force update of playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_status_upd(curstat_t *s)
{
	(void) aixioc_get_playstatus(s);
}


/*
 * aixioc_lock
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
aixioc_lock(curstat_t *s, bool_t enable)
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

	if (!aixioc_send(DI_ROLE_MAIN, enable ? IDE_CDPMR : IDE_CDAMR,
			 0, TRUE)) {
		/* Cannot lock/unlock caddy */
		DO_BEEP();
		SET_LOCK_BTN((bool_t) !enable);
		return;
	}

	s->caddy_lock = enable;
	SET_LOCK_BTN(enable);
}


/*
 * aixioc_repeat
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
aixioc_repeat(curstat_t *s, bool_t enable)
{
	s->repeat = enable;

	if (!enable && aixioc_new_progshuf) {
		aixioc_new_progshuf = FALSE;
		if (s->rptcnt > 0)
			s->rptcnt--;
	}
	DPY_RPTCNT(s);
}


/*
 * aixioc_shuffle
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
aixioc_shuffle(curstat_t *s, bool_t enable)
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

	s->segplay = SEGP_NONE; /* Cancel a->b mode */
	DPY_PROGMODE(s, FALSE);

	s->shuffle = enable;
	if (!s->shuffle)
		s->prog_tot = 0;
}


/*
 * aixioc_load_eject
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
aixioc_load_eject(curstat_t *s)
{
	bool_t	ret = FALSE;

	if (aixioc_devp == NULL)
		return;

	if (aixioc_is_enabled(DI_ROLE_MAIN) && !aixioc_disc_present(FALSE)) {
		/* No disc */
		if (app_data.load_supp) {
			/* Try loading the disc */
			ret = aixioc_start_stop(TRUE, TRUE);
		}

		if (!ret && app_data.eject_supp) {
			/* Cannot load, maybe the tray is already closed
			 * but empty.
			 */

			/* Unlock caddy if supported */
			if (app_data.caddylock_supp)
				aixioc_lock(s, FALSE);

			/* Try opening the tray */
			if (aixioc_start_stop(FALSE, TRUE))
				DO_BEEP();
		}

		aixioc_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (aixioc_devp != NULL && app_data.eject_close) {
			aixioc_close();
			aixioc_not_open = TRUE;
		}

		aixioc_start_insert_poll(s);
		return;
	}

	/* Eject the disc */

	/* Spin down the CD */
	(void) aixioc_start_stop(FALSE, FALSE);

	if (!app_data.eject_supp) {
		DO_BEEP();

		aixioc_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (aixioc_devp != NULL && app_data.eject_close) {
			aixioc_close();
			aixioc_not_open = TRUE;
		}

		aixioc_start_insert_poll(s);
		return;
	}

	/* Unlock caddy if supported */
	if (app_data.caddylock_supp)
		aixioc_lock(s, FALSE);

	aixioc_stop_stat_poll();
	di_reset_curstat(s, TRUE, TRUE);
	s->mode = MOD_NODISC;

	di_clear_cdinfo(s, FALSE);
	DPY_ALL(s);

	/* Eject the CD */
	(void) aixioc_start_stop(FALSE, TRUE);

	if (app_data.eject_exit)
		di_clinfo->quit(s);
	else {
		if (aixioc_devp != NULL && app_data.eject_close) {
			aixioc_close();
			aixioc_not_open = TRUE;
		}
			
		aixioc_start_insert_poll(s);
	}
}


/*
 * aixioc_ab
 *	A->B segment play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_ab(curstat_t *s)
{
	if (!aixioc_run_ab(s))
		DO_BEEP();
}


/*
 * aixioc_sample
 *	Sample play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_sample(curstat_t *s)
{
	int	i;

	if (!aixioc_disc_ready(s)) {
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
		aixioc_start_stat_poll(s);
		/*FALLTHROUGH*/
	case MOD_PLAY:
		/* If already playing a track, start sampling the track after
		 * the current one.  Otherwise, sample from the beginning.
		 */
		if (s->cur_trk > 0 && s->cur_trk != s->last_trk) {
			i = di_curtrk_pos(s) + 1;
			s->cur_trk = s->trkinfo[i].trkno;
			aixioc_next_sam = (byte_t) i;
		}
		else {
			s->cur_trk = s->first_trk;
			aixioc_next_sam = 0;
		}
		
		s->cur_idx = 1;

		s->mode = MOD_SAMPLE;
		DPY_ALL(s);

		if (!aixioc_run_sample(s))
			return;

		break;

	case MOD_SAMPLE:
		/* Currently doing Sample playback, just call aixioc_play_pause
		 * to resume normal playback.
		 */
		aixioc_play_pause(s);
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * aixioc_level
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
aixioc_level(curstat_t *s, byte_t level, bool_t drag)
{
	int	actual;

	/* Set volume level */
	if ((actual = aixioc_cfg_vol((int) level, s, FALSE)) >= 0)
		s->level = (byte_t) actual;
}


/*
 * aixioc_play_pause
 *	Audio playback and pause function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_play_pause(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;

	aixioc_override_ap = TRUE;

	if (!aixioc_disc_ready(s)) {
		aixioc_override_ap = FALSE;
		DO_BEEP();
		return;
	}

	aixioc_override_ap = FALSE;

	if (s->mode == MOD_NODISC)
		s->mode = MOD_STOP;

	switch (s->mode) {
	case MOD_PLAY:
		/* Currently playing: go to pause mode */

		if (!aixioc_pause_resume(FALSE)) {
			DO_BEEP();
			return;
		}
		aixioc_stop_stat_poll();
		s->mode = MOD_PAUSE;
		DPY_PLAYMODE(s, FALSE);
		break;

	case MOD_PAUSE:
		/* Currently paused: resume play */

		if (!aixioc_pause_resume(TRUE)) {
			DO_BEEP();
			return;
		}
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		aixioc_start_stat_poll(s);
		break;

	case MOD_STOP:
		/* Currently stopped: start play */

		if (!di_prepare_cdda(s))
			return;

		if (s->shuffle || s->program) {
			aixioc_new_progshuf = TRUE;

			/* Start shuffle/program play */
			if (!aixioc_run_prog(s))
				return;
		}
		else if (s->segplay == SEGP_AB) {
			/* Play defined segment */
			if (!aixioc_run_ab(s))
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

			if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				s->mode = MOD_STOP;
				return;
			}
		}

		DPY_ALL(s);
		aixioc_start_stat_poll(s);
		break;

	case MOD_SAMPLE:
		/* Force update of curstat */
		if (!aixioc_get_playstatus(s)) {
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

			if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
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

			if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
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
 * aixioc_stop
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
aixioc_stop(curstat_t *s, bool_t stop_disc)
{
	/* The stop_disc parameter will cause the disc to spin down.
	 * This is usually set to TRUE, but can be FALSE if the caller
	 * just wants to set the current state to stop but will
	 * immediately go into play state again.  Not spinning down
	 * the drive makes things a little faster...
	 */
	if (!aixioc_disc_ready(s))
		return;

	switch (s->mode) {
	case MOD_PLAY:
	case MOD_PAUSE:
	case MOD_SAMPLE:
	case MOD_STOP:
		/* Currently playing or paused: stop */

		if (stop_disc && !aixioc_start_stop(FALSE, FALSE)) {
			DO_BEEP();
			return;
		}
		aixioc_stop_stat_poll();

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
 * aixioc_chgdisc
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
aixioc_chgdisc(curstat_t *s)
{
	/* Disc change is not implemented in this module */
	DO_BEEP();
}


/*
 * aixioc_prevtrk
 *	Previous track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_prevtrk(curstat_t *s)
{
	sword32_t		i,
				start_addr;
	msf_t			start_msf,
				end_msf;
	bool_t			go_prev;

	if (!aixioc_disc_ready(s)) {
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
				aixioc_new_progshuf = FALSE;
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
					aixioc_new_progshuf = FALSE;
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
			aixioc_mute_on(s);

		if (s->shuffle || s->program) {
			/* Program/Shuffle mode: just stop the playback
			 * and let aixioc_run_prog go to the previous track
			 */
			aixioc_fake_stop = TRUE;

			/* Force status update */
			(void) aixioc_get_playstatus(s);
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

			if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();

				/* Restore volume */
				aixioc_mute_off(s);
				return;
			}

			/* Wait until the CD is where we told it to be */
			aixioc_wait_display(start_addr, s);

			if (s->mode == MOD_PAUSE) {
				(void) aixioc_pause_resume(FALSE);

				/* Restore volume */
				aixioc_mute_off(s);
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
 * aixioc_nexttrk
 *	Next track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_nexttrk(curstat_t *s)
{
	sword32_t		i,
				start_addr;
	msf_t			start_msf,
				end_msf;

	if (!aixioc_disc_ready(s)) {
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
					aixioc_new_progshuf = TRUE;
				else
					return;
			}

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				aixioc_mute_on(s);

			/* Program/Shuffle mode: just stop the playback
			 * and let aixioc_run_prog go to the next track.
			 */
			aixioc_fake_stop = TRUE;

			/* Force status update */
			(void) aixioc_get_playstatus(s);

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
				aixioc_mute_on(s);

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

			if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}

			/* Wait until the CD is where we told it to be */
			aixioc_wait_display(start_addr, s);

			if (s->mode == MOD_PAUSE) {
				(void) aixioc_pause_resume(FALSE);

				/* Restore volume */
				aixioc_mute_off(s);
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
 * aixioc_previdx
 *	Previous index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_previdx(curstat_t *s)
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
		aixioc_mute_on(s);

		if (!aixioc_do_playaudio(ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk, idx)) {
			/* Restore volume */
			aixioc_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		aixioc_idx_pause = TRUE;

		if (!aixioc_pause_resume(FALSE)) {
			/* Restore volume */
			aixioc_mute_off(s);
			aixioc_idx_pause = FALSE;
			return;
		}

		/* Use aixioc_get_playstatus to update the current status */
		if (!aixioc_get_playstatus(s)) {
			/* Restore volume */
			aixioc_mute_off(s);
			aixioc_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			aixioc_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			aixioc_idx_pause = FALSE;
			return;
		}

		aixioc_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) aixioc_pause_resume(FALSE);

			/* Restore volume */
			aixioc_mute_off(s);

			/* Force update of curstat */
			(void) aixioc_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * aixioc_nextidx
 *	Next index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_nextidx(curstat_t *s)
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
		aixioc_mute_on(s);

		if (!aixioc_do_playaudio(ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk,
				  (byte_t) (s->cur_idx + 1))) {
			/* Restore volume */
			aixioc_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		aixioc_idx_pause = TRUE;

		if (!aixioc_pause_resume(FALSE)) {
			/* Restore volume */
			aixioc_mute_off(s);
			aixioc_idx_pause = FALSE;
			return;
		}

		/* Use aixioc_get_playstatus to update the current status */
		if (!aixioc_get_playstatus(s)) {
			/* Restore volume */
			aixioc_mute_off(s);
			aixioc_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			aixioc_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			aixioc_idx_pause = FALSE;
			return;
		}

		aixioc_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) aixioc_pause_resume(FALSE);

			/* Restore volume */
			aixioc_mute_off(s);

			/* Force update of curstat */
			(void) aixioc_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * aixioc_rew
 *	Search-rewind function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_rew(curstat_t *s, bool_t start)
{
	sword32_t	i;
	msf_t		start_msf,
			end_msf;
	byte_t		vol;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		aixioc_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				aixioc_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) aixioc_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE
				);
			}

			/* Start search rewind */
			aixioc_start_search = TRUE;
			aixioc_run_rew(s);
		}
		else {
			/* Button release */

			aixioc_stop_rew(s);

			/* Update display */
			(void) aixioc_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				aixioc_mute_on(s);
			else
				/* Restore volume */
				aixioc_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == LEAD_OUT_TRACK) {
					/* Restore volume */
					aixioc_mute_off(s);
					return;
				}

				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_msf.min = s->trkinfo[i+1].min;
				end_msf.sec = s->trkinfo[i+1].sec;
				end_msf.frame = s->trkinfo[i+1].frame;

				if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  s->curpos_tot.addr,
						  s->trkinfo[i+1].addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					aixioc_mute_off(s);
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

				if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  s->curpos_tot.addr,
						  s->discpos_tot.addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					aixioc_mute_off(s);
					return;
				}
			}

			if (s->mode == MOD_PAUSE) {
				(void) aixioc_pause_resume(FALSE);

				/* Restore volume */
				aixioc_mute_off(s);
			}
			else
				aixioc_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * aixioc_ff
 *	Search-fast-forward function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_ff(curstat_t *s, bool_t start)
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
		aixioc_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				aixioc_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) aixioc_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE
				);
			}

			/* Start search forward */
			aixioc_start_search = TRUE;
			aixioc_run_ff(s);
		}
		else {
			/* Button release */

			aixioc_stop_ff(s);

			/* Update display */
			(void) aixioc_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				aixioc_mute_on(s);
			else
				/* Restore volume */
				aixioc_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == LEAD_OUT_TRACK) {
					/* Restore volume */
					aixioc_mute_off(s);
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

				if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  start_addr, end_addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					aixioc_mute_off(s);
					return;
				}
			}
			else if (s->segplay == SEGP_AB &&
				 (s->curpos_tot.addr + app_data.min_playblks) >
					s->bp_endpos_tot.addr) {
				/* No more left to play */
				/* Restore volume */
				aixioc_mute_off(s);
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

				if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  start_addr, end_addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					aixioc_mute_off(s);
					return;
				}
			}
			if (s->mode == MOD_PAUSE) {
				(void) aixioc_pause_resume(FALSE);

				/* Restore volume */
				aixioc_mute_off(s);
			}
			else
				aixioc_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * aixioc_warp
 *	Track warp function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_warp(curstat_t *s)
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
		aixioc_play_pause(s);

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
			if (!aixioc_start_stop(FALSE, FALSE))
				DO_BEEP();
		}
		else {
			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				aixioc_mute_on(s);

			if (!aixioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						start_addr, end_addr,
						&start_msf, &end_msf,
						0, 0)) {
				DO_BEEP();

				/* Restore volume */
				aixioc_mute_off(s);
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) aixioc_pause_resume(FALSE);

				/* Restore volume */
				aixioc_mute_off(s);
			}
		}
		break;

	default:
		break;
	}
}


/*
 * aixioc_route
 *	Channel routing function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_route(curstat_t *s)
{
	if (!app_data.chroute_supp)
		return;

	/* Only CDDA mode supports channel routing on AIX */
	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		(void) cdda_chroute(aixioc_devp, s);
}


/*
 * aixioc_mute_on
 *	Mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_mute_on(curstat_t *s)
{
	struct cd_audio_cmd cmd;

	/* Get the current volumes */
	cmd.audio_cmds = CD_INFO_AUDIO;
	(void) aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE);
	aixioc_sav_vol[0] = cmd.out_port_0_vol;
	aixioc_sav_vol[1] = cmd.out_port_1_vol;
	aixioc_sav_vol[2] = cmd.out_port_2_vol;
	aixioc_sav_vol[3] = cmd.out_port_3_vol;

	/* Mute all the channels */
	cmd.audio_cmds = CD_SET_VOLUME;
	cmd.volume_type = CD_ALL_AUDIO_MUTE;
	(void) aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE);
}


/*
 * aixioc_mute_off
 *	Un-mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_mute_off(curstat_t *s)
{
	struct cd_audio_cmd cmd;

	/* Restore the saved volumes */
	cmd.out_port_0_vol = aixioc_sav_vol[0];
	cmd.out_port_1_vol = aixioc_sav_vol[1];
	cmd.out_port_2_vol = aixioc_sav_vol[2];
	cmd.out_port_3_vol = aixioc_sav_vol[3];

	cmd.audio_cmds = CD_SET_VOLUME;
	cmd.volume_type = CD_VOLUME_CHNLS;
	(void) aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE);
	(void) aixioc_cfg_vol((int) s->level, s, FALSE);
}


/*
 * aixioc_cddajitter
 *	CDDA jitter correction setting change notification
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_cddajitter(curstat_t *s)
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
			aixioc_stop_stat_poll();
			break;

		default:
			break;
		}

		/* Restart CDDA */
		cdda_halt(aixioc_devp, s);
		util_delayms(1000);

		aixioc_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		aixioc_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		aixioc_cdda_client.warning_msg = di_clinfo->warning_msg;
		aixioc_cdda_client.info_msg = di_clinfo->info_msg;
		aixioc_cdda_client.info2_msg = di_clinfo->info2_msg;

		(void) cdda_init(s, &aixioc_cdda_client);

		/* Restore old state */
		switch (omode) {
		case MOD_PLAY:
		case MOD_SAMPLE:
			if (aixioc_play_recov(curblk, FALSE)) {
				aixioc_start_stat_poll(s);

				if (omode == MOD_PAUSE)
					(void) aixioc_pause_resume(FALSE);
			}
			break;

		default:
			break;
		}

		s->mode = omode;
	}
}


/*
 * aixioc_debug
 *	Debug level change notification
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
aixioc_debug(void)
{
	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		cdda_debug(app_data.debug);
}


/*
 * aixioc_start
 *	Start the AIX IDE ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_start(curstat_t *s)
{
	/* Check to see if disc is ready */
	if (di_clinfo->timeout != NULL)
		aixioc_start_insert_poll(s);
	else
		(void) aixioc_disc_ready(s);

	/* Update display */
	DPY_ALL(s);
}


/*
 * aixioc_icon
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
aixioc_icon(curstat_t *s, bool_t iconified)
{
	/* This function attempts to reduce the status polling frequency
	 * when possible to cut down on CPU and bus usage.  This is
	 * done when the CD player is iconified.
	 */

	/* Increase status polling interval by 4 seconds when iconified */
	if (iconified)
		aixioc_stat_interval = app_data.stat_interval + 4000;
	else
		aixioc_stat_interval = app_data.stat_interval;

	switch (s->mode) {
	case MOD_BUSY:
	case MOD_NODISC:
	case MOD_STOP:
	case MOD_PAUSE:
		break;

	case MOD_SAMPLE:
		/* No optimization in these modes */
		aixioc_stat_interval = app_data.stat_interval;
		break;

	case MOD_PLAY:
		if (!iconified) {
			/* Force an immediate update */
			aixioc_stop_stat_poll();
			aixioc_start_stat_poll(s);
		}
		break;

	default:
		break;
	}
}


/*
 * aixioc_halt
 *	Shut down the AIX IDE ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
aixioc_halt(curstat_t *s)
{
	/* If playing CDDA, stop it */
	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			(void) aixioc_start_stop(FALSE, FALSE);
			s->mode = MOD_STOP;
			aixioc_stop_stat_poll();
			break;
		default:
			break;
		}
	}

	/* Shut down CDDA */
	cdda_halt(aixioc_devp, s);
	app_data.play_mode = PLAYMODE_STD;

	/* Re-enable front-panel eject button */
	if (app_data.caddylock_supp)
		aixioc_lock(s, FALSE);

	if (s->mode != MOD_BUSY && s->mode != MOD_NODISC) {
		if (app_data.exit_eject && app_data.eject_supp) {
			/* User closing application: Eject disc */
			(void) aixioc_start_stop(FALSE, TRUE);
		}
		else {
			if (app_data.exit_stop)
				/* User closing application: Stop disc */
				(void) aixioc_start_stop(FALSE, FALSE);

			switch (s->mode) {
			case MOD_PLAY:
			case MOD_PAUSE:
			case MOD_SAMPLE:
				aixioc_stop_stat_poll();
				break;
			default:
				break;
			}
		}
	}

	if (aixioc_devp != NULL) {
		aixioc_close();
		aixioc_not_open = TRUE;
	}
}


/*
 * aixioc_methodstr
 *	Return a text string indicating the current method.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Method text string.
 */
char *
aixioc_methodstr(void)
{
	return ("AIX IDE ioctl\n");
}


/*
 * aixioc_wait_display
 *	Wait until the CD has played past the specified frame offset.
 *	This is used to stall updating the track/time display, because
 *	the drive seems to start playing some blocks *before* the
 *	specified starting location, causing the track/time display
 *	to indicate the "wrong" information for a brief moment.
 *
 * Args:
 *	time - frame offset to wait until
 *	s - pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_wait_display(sword32_t offset, curstat_t *s)
{
	sword32_t		cur_addr,
				ref_addr,
				max_addr;
	struct cd_audio_cmd	cmd;

	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		return;

	cmd.audio_cmds = CD_INFO_AUDIO;
	if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO, (void *) &cmd, TRUE))
		return; 

	/* If we subtract MSF_OFFSET from 0, it wraps back around to some
	 * large value (under AIX) since it's unsigned, so just add
	 * MSF_OFFSET to the desired location and forget the translation.
	 */
	offset += MSF_OFFSET;

	util_msftoblk(
		cmd.indexing.info_audio.current_mins,
		cmd.indexing.info_audio.current_secs,
		cmd.indexing.info_audio.current_frames,
		&ref_addr,
		0
	);

	if (ref_addr >= offset)
		return;		/* Already there, no need to wait */

	max_addr = ref_addr + MAX_WAIT_FRAMES;

	/* Wait loop */
	do {
		util_delayms(10);	/* Sleep for 10ms to reduce load */

		if (!aixioc_send(DI_ROLE_MAIN, IDE_CDAUDIO,
				 (void *) &cmd, TRUE))
			break;	/* shrug */

		util_msftoblk(
			cmd.indexing.info_audio.current_mins,
			cmd.indexing.info_audio.current_secs,
			cmd.indexing.info_audio.current_frames,
			&cur_addr,
			0
		);

		if (cur_addr >= offset)
			break;	/* We're past the desired offset */

	} while (cur_addr < max_addr);
}


/*
 * aixioc_dumpcmd
 *	Dumps part or all of the contents of a cd_audio_cmd struct.
 *
 * Args:
 *	cmd - Pointer to a cd_audio_cmd struct
 *	sel - Flags indicating which major sections to print out
 *	sub - If sel includes SEL_INDEXING, the members of union indexing to
 *	      print out.
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixioc_dumpcmd(struct cd_audio_cmd *cmd, int sel, int sub)
{
	if (sel & SEL_CMD) {
		(void) fprintf(errfp, "\nCommand: %d - ", cmd->audio_cmds);
		if (cmd->audio_cmds & CD_SET_VOLUME) {
			/* Can be OR'd with other cmds */
			(void) fprintf(errfp, "set volume, ");
			cmd->audio_cmds &= ~CD_SET_VOLUME;
		}

		switch(cmd->audio_cmds) {
		case CD_PLAY_AUDIO: 
			(void) fprintf(errfp, "play\n");
			break;
		case CD_PAUSE_AUDIO: 
			(void) fprintf(errfp, "pause\n");
			break;
		case CD_RESUME_AUDIO: 
			(void) fprintf(errfp, "resume\n");
			break;
		case CD_STOP_AUDIO: 
			(void) fprintf(errfp, "stop\n");
			break;
		case CD_INFO_AUDIO: 
			(void) fprintf(errfp, "get info\n");
			break;
		case CD_TRK_INFO_AUDIO: 
			(void) fprintf(errfp, "get track info\n");
			break;
		case CD_GET_TRK_MSF: 
			(void) fprintf(errfp, "get track MSF\n");
			break;
		case CD_SET_VOLUME: 
			/* This shouldn't happen */
			(void) fprintf(errfp, "set volume\n");
			break;
		default: 
			(void) fprintf(errfp, "unknown\n");
			break;
		}
	}

	if (sel & SEL_MSF) {
		(void) fprintf(errfp, "MSF mode: %s\n",
			       (cmd->msf_flag) ? "TRUE" : "FALSE");
	}

	if (sel & SEL_INDEXING) {
		(void) fprintf(errfp, "Indexing:\n");
		if (sub & SUB_TRKIDX) {
			(void) fprintf(errfp,
				"track_index: first track: %-4d  index: %d\n",
				cmd->indexing.track_index.first_track,
				cmd->indexing.track_index.first_index);
			(void) fprintf(errfp,
				"              last track: %-4d  index: %d\n",
				cmd->indexing.track_index.last_track,
				cmd->indexing.track_index.last_index);
		}
		if (sub & SUB_MSF) {
			(void) fprintf(errfp,
				"msf: first is %02d:%02d.%04d\n",
				cmd->indexing.msf.first_mins,
				cmd->indexing.msf.first_secs,
				cmd->indexing.msf.first_frames);
			(void) fprintf(errfp,
				"      last is %02d:%02d.%04d\n",
				cmd->indexing.msf.last_mins,
				cmd->indexing.msf.last_secs,
				cmd->indexing.msf.last_frames);
		}
		if (sub & SUB_INFO) {
			(void) fprintf(errfp,
				"info_audio: current track: %-4d  index: %d\n", 
				cmd->indexing.info_audio.current_track,
				cmd->indexing.info_audio.current_index);
			(void) fprintf(errfp,
				"              current MSF: %02d:%02d.%04d\n",
				cmd->indexing.info_audio.current_mins,
				cmd->indexing.info_audio.current_secs,
				cmd->indexing.info_audio.current_frames);
		}
		if (sub & SUB_TRKMSF) {
			(void) fprintf(errfp,
				"track_msf: track: %d\n",
				cmd->indexing.track_msf.track);
			(void) fprintf(errfp,
				"             MSF: %02d:%02d.%04d\n",
				cmd->indexing.track_msf.mins,
				cmd->indexing.track_msf.secs,
				cmd->indexing.track_msf.frames);
		}
	}

	if (sel & SEL_STATUS) {
		(void) fprintf(errfp, "Status: ");
		switch(cmd->status) {
		case CD_PLAY_AUDIO: 
			(void) fprintf(errfp, "Currently playing audio\n");
		case CD_PAUSE_AUDIO: 
			(void) fprintf(errfp, "Currently paused\n");
		case CD_RESUME_AUDIO: 
			(void) fprintf(errfp, "Currently resumed\n");
		case CD_STOP_AUDIO: 
			(void) fprintf(errfp, "Currently stopped\n");
		case CD_NO_AUDIO: 
			(void) fprintf(errfp,
			    "No CD_PLAY_AUDIO cmd currently in progress\n");
			break;
		case CD_COMPLETED: 
			(void) fprintf(errfp,
			    "Last CD_PLAY_AUDIO cmd completed successfully\n");
			break;
		case CD_STATUS_ERROR: 
			(void) fprintf(errfp,
			    "Invalid audio status/Play stopped due to error\n");
			break;
		case CD_NOT_VALID: 
			(void) fprintf(errfp,
			    "Audio status not supported or not valid\n");
			break;
		default: 
			(void) fprintf(errfp, "unknown (%d)\n", cmd->status);
		}
	}

	if (sel & SEL_VOLUME) {
		(void) fprintf(errfp, "Volume control type: ");
		switch(cmd->volume_type) {
		case CD_VOLUME_ALL: 
			(void) fprintf(errfp, "All ports same\n");
			break;
		case CD_VOLUME_CHNLS: 
			(void) fprintf(errfp, "Each port unique\n");
			break;
		case CD_ALL_AUDIO_MUTE: 
			(void) fprintf(errfp, "Mute all output ports\n");
			break;
		case CD_SET_AUDIO_CHNLS: 
			(void) fprintf(errfp,
				       "Change channel<->port mapping\n");
			break;
		default: 
			(void) fprintf(errfp, "dunno\n");
			break;
		}
		(void) fprintf(errfp, "all_channel_vol: %d\n",
			       cmd->all_channel_vol);

		(void) fprintf(errfp,
			"Output port volumes: 0=%03d  1=%03d  2=%03d  3=%03d\n",
			cmd->out_port_0_vol,
			cmd->out_port_1_vol,
			cmd->out_port_2_vol,
			cmd->out_port_3_vol);
	}

	if (sel & SEL_CHANNEL) {
		(void) fprintf(errfp, "Output port select: 0: ");
		switch(cmd->out_port_0_sel) {
		case CD_MUTE_PORT: 
			(void) fprintf(errfp, "muted\n");
			break;
		case CD_AUDIO_CHNL_0: 
			(void) fprintf(errfp, "map to channel 0\n");
			break;
		case CD_AUDIO_CHNL_1: 
			(void) fprintf(errfp, "map to channel 1\n");
			break;
		case CD_AUDIO_CHNL_2: 
			(void) fprintf(errfp, "map to channel 2\n");
			break;
		case CD_AUDIO_CHNL_3: 
			(void) fprintf(errfp, "map to channel 3\n");
			break;
		default: 
			(void) fprintf(errfp, "unknown\n");
			break;
		}
		(void) fprintf(errfp, "                    1: ");
		switch(cmd->out_port_1_sel) {
		case CD_MUTE_PORT: 
			(void) fprintf(errfp, "muted\n");
			break;
		case CD_AUDIO_CHNL_0: 
			(void) fprintf(errfp, "map to channel 0\n");
			break;
		case CD_AUDIO_CHNL_1: 
			(void) fprintf(errfp, "map to channel 1\n");
			break;
		case CD_AUDIO_CHNL_2: 
			(void) fprintf(errfp, "map to channel 2\n");
			break;
		case CD_AUDIO_CHNL_3: 
			(void) fprintf(errfp, "map to channel 3\n");
			break;
		default: 
			(void) fprintf(errfp, "unknown\n");
			break;
		}
		(void) fprintf(errfp, "                    2: ");
		switch(cmd->out_port_2_sel) {
		case CD_MUTE_PORT: 
			(void) fprintf(errfp, "muted\n");
			break;
		case CD_AUDIO_CHNL_0: 
			(void) fprintf(errfp, "map to channel 0\n");
			break;
		case CD_AUDIO_CHNL_1: 
			(void) fprintf(errfp, "map to channel 1\n");
			break;
		case CD_AUDIO_CHNL_2: 
			(void) fprintf(errfp, "map to channel 2\n");
			break;
		case CD_AUDIO_CHNL_3: 
			(void) fprintf(errfp, "map to channel 3\n");
			break;
		default: 
			(void) fprintf(errfp, "unknown\n");
			break;
		}
		(void) fprintf(errfp, "                    3: ");
		switch(cmd->out_port_3_sel) {
		case CD_MUTE_PORT: 
			(void) fprintf(errfp, "muted\n");
			break;
		case CD_AUDIO_CHNL_0: 
			(void) fprintf(errfp, "map to channel 0\n");
			break;
		case CD_AUDIO_CHNL_1: 
			(void) fprintf(errfp, "map to channel 1\n");
			break;
		case CD_AUDIO_CHNL_2: 
			(void) fprintf(errfp, "map to channel 2\n");
			break;
		case CD_AUDIO_CHNL_3: 
			(void) fprintf(errfp, "map to channel 3\n");
			break;
		default: 
			(void) fprintf(errfp, "unknown\n");
			break;
		}
	}
}
#endif	/* DI_AIXIOC DEMO_ONLY */

