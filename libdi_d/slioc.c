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
 * SunOS/Solaris/Linux/QNX ioctl method module
 *
 * Contributing author: Peter Bauer
 * E-mail: 100136.3530@compuserve.com
 *
 * QNX support added by: D. J. Hawkey Jr. (hints provided by W. A. Flowers)
 * E-mail: hawkeyd@visi.com
 */

#ifndef lint
static char *_slioc_c_ident_ = "@(#)slioc.c	6.237 04/01/14";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/slioc.h"
#include "cdda_d/cdda.h"

#if defined(DI_SLIOC) && !defined(DEMO_ONLY)

extern appdata_t	app_data;
extern FILE		*errfp;
extern di_client_t	*di_clinfo;
extern sword32_t	di_clip_frames;


STATIC bool_t	slioc_pause_resume(bool_t),
		slioc_run_ab(curstat_t *),
		slioc_run_sample(curstat_t *),
		slioc_run_prog(curstat_t *),
		slioc_run_repeat(curstat_t *),
		slioc_disc_ready(curstat_t *),
		slioc_disc_present(bool_t);
STATIC int	slioc_cfg_vol(int, curstat_t *, bool_t);
STATIC void	slioc_stat_poll(curstat_t *),
		slioc_insert_poll(curstat_t *);


STATIC di_dev_t	*slioc_devp = NULL;		/* CD device descriptor */
STATIC int	slioc_stat_interval,		/* Status poll interval */
		slioc_ins_interval;		/* Insert poll interval */
STATIC long	slioc_stat_id,			/* Play status poll timer id */
		slioc_insert_id,		/* Disc insert poll timer id */
		slioc_search_id;		/* FF/REW timer id */
STATIC byte_t	slioc_tst_status = MOD_NODISC;	/* Playback status on load */
STATIC bool_t	slioc_not_open = TRUE,		/* Device not opened yet */
		slioc_stat_polling,		/* Polling play status */
		slioc_insert_polling,		/* Polling disc insert */
		slioc_new_progshuf,		/* New program/shuffle seq */
		slioc_start_search,		/* Start FF/REW play segment */
		slioc_idx_pause,		/* Prev/next index pausing */
		slioc_fake_stop,		/* Force a completion status */
		slioc_playing,			/* Currently playing */
		slioc_paused,			/* Currently paused */
		slioc_bcd_hack,			/* Track numbers in BCD hack */
		slioc_mult_autoplay,		/* Auto-play after disc chg */
		slioc_mult_pause,		/* Pause after disc chg */
		slioc_override_ap;		/* Override auto-play */
STATIC sword32_t slioc_sav_end_addr;		/* Err recov saved end addr */
STATIC word32_t	slioc_next_sam;			/* Next SAMPLE track */
STATIC msf_t	slioc_sav_end_msf;		/* Err recov saved end MSF */
STATIC byte_t	slioc_sav_end_fmt;		/* Err recov saved end fmt */
STATIC cdda_client_t slioc_cdda_client;		/* CDDA client struct */


/* SunOS/Solaris/Linux CDROM ioctl names */
STATIC iocname_t iname[] = {
	{ CDROMSUBCHNL,		"CDROMSUBCHNL"		},
	{ CDROMREADTOCHDR,	"CDROMREADTOCHDR"	},
	{ CDROMREADTOCENTRY,	"CDROMREADTOCENTRY"	},
	{ CDROMEJECT,		"CDROMEJECT"		},
	{ CDROMSTART,		"CDROMSTART"		},
	{ CDROMSTOP,		"CDROMSTOP"		},
	{ CDROMPAUSE,		"CDROMPAUSE"		},
	{ CDROMRESUME,		"CDROMRESUME"		},
	{ CDROMVOLCTRL,		"CDROMVOLCTRL"		},
#ifdef CDROMVOLREAD
	{ CDROMVOLREAD,		"CDROMVOLREAD"		},
#endif
	{ CDROMPLAYTRKIND,	"CDROMPLAYTRKIND"	},
	{ CDROMPLAYMSF,		"CDROMPLAYMSF"		},
#ifdef CDROMLOAD
	{ CDROMLOAD,		"CDROMLOAD"		},
#endif
#ifdef CDROMMULTISESSION
	{ CDROMMULTISESSION,	"CDROMMULTISESSION"	},
#endif
#ifdef CDROM_GET_MCN
	{ CDROM_GET_MCN,	"CDROM_GET_MCN"		},
#endif
#ifdef CDROMCDDA
	{ CDROMCDDA,		"CDROMCDDA"		},
#endif
#ifdef _LINUX
	{ CDROMREADAUDIO,	"CDROMREADAUDIO"	},
	{ CDROMCLOSETRAY,	"CDROMCLOSETRAY"	},
	{ CDROM_CHANGER_NSLOTS,	"CDROM_CHANGER_NSLOTS"	},
	{ CDROM_SELECT_DISC,	"CDROM_SELECT_DISC"	},
	{ CDROM_DRIVE_STATUS,	"CDROM_DRIVE_STATUS"	},
	{ CDROM_MEDIA_CHANGED,	"CDROM_MEDIA_CHANGED"	},
#endif
	{ 0,			NULL			},
};



/***********************
 *  internal routines  *
 ***********************/


/*
 * slioc_close
 *	Close CD device
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_close(void)
{
	if (slioc_devp != NULL) {
		DBGPRN(DBG_DEVIO)(errfp, "\nClose device: %s\n",
				slioc_devp->path);

		if (slioc_devp->fd > 0)
			(void) close(slioc_devp->fd);
		di_devfree(slioc_devp);
		slioc_devp = NULL;
	}
}


/*
 * slioc_open
 *	Open CD device
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - open successful
 *	FALSE - open failed
 */
STATIC bool_t
slioc_open(curstat_t *s)
{
	struct stat	stbuf;
	char		errstr[ERR_BUF_SZ];
	int		openflgs;
#ifdef _LINUX
	int		nslots,
			curslot;
	static bool_t	first = TRUE;
#endif

	DBGPRN(DBG_DEVIO)(errfp, "\nOpen device: %s\n", s->curdev);

	/* Check for validity of device node */
	if (stat(s->curdev, &stbuf) < 0) {
#ifdef SOL2_VOLMGT
		if (app_data.sol2_volmgt) {
			switch (errno) {
			case ENOENT:
			case EINTR:
			case ESTALE:
				return FALSE;
				/*NOTREACHED*/
			default:
				break;
			}
		}
#endif
		(void) sprintf(errstr, app_data.str_staterr, s->curdev);
		DI_FATAL(errstr);
		return FALSE;
	}

#if defined(_LINUX) || defined(__QNX__)
	/* Linux and QNX CD devices are block special file! */
	if (!S_ISBLK(stbuf.st_mode))
#else
	if (!S_ISCHR(stbuf.st_mode))
#endif
	{
#ifdef SOL2_VOLMGT
		/* Some CDs have multiple slices (partitions),
		 * so the device node becomes a directory when
		 * vold mounts each slice.
		 */
		if (app_data.sol2_volmgt && S_ISDIR(stbuf.st_mode))
			return FALSE;
#endif
		(void) sprintf(errstr, app_data.str_noderr, s->curdev);
		DI_FATAL(errstr);
		return FALSE;
	}

#ifdef _LINUX
	openflgs = O_RDONLY | O_EXCL | O_NONBLOCK;
#else
	openflgs = O_RDONLY;
#endif

	if ((slioc_devp = di_devalloc(s->curdev)) == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	if ((slioc_devp->fd = open(s->curdev, openflgs)) < 0) {
		DBGPRN(DBG_DEVIO)(errfp,
			"Cannot open %s: errno=%d\n", s->curdev, errno);

		di_devfree(slioc_devp);
		slioc_devp = NULL;
		return FALSE;
	}

#ifdef _LINUX
	/* CD changer setup */
	if (app_data.numdiscs > 1 && first) {
		first = FALSE;

		/* Find out how many discs the drive says it supports.
		 * Note: Only works on Linux 2.1.x and later.
		 */
		if (slioc_send(DI_ROLE_MAIN, CDROM_CHANGER_NSLOTS, NULL, 0,
				(bool_t) ((app_data.debug & DBG_DEVIO) != 0),
				&nslots)) {
			if (nslots != app_data.numdiscs) {
				DBGPRN(DBG_DEVIO)(errfp,
					"CD changer: Setting numdiscs to %d\n",
					nslots);
				app_data.numdiscs = s->last_disc = nslots;
			}
		}

		if (app_data.numdiscs == 1) {
			/* Not a CD changer:
			 * Set to single-disc mode.
			 */
			app_data.numdiscs = 1;
			app_data.chg_method = CHG_NONE;
			app_data.multi_play = FALSE;
			app_data.reverse = FALSE;
			s->first_disc = s->last_disc = s->cur_disc = 1;

			DBGPRN(DBG_DEVIO)(errfp, "Not a CD changer: %s\n",
				"Setting to single disc mode");

			DI_INFO(app_data.str_medchg_noinit);
		}
	}

	if (app_data.numdiscs > 1) {
		/* Find out which disc is loaded.
		 * Note: Only works on Linux 2.1.x and later.
		 */
		if (slioc_send(DI_ROLE_MAIN, CDROM_SELECT_DISC,
				(void *) CDSL_CURRENT, 0,
				(bool_t) ((app_data.debug & DBG_DEVIO) != 0),
				&curslot)) {
			DBGPRN(DBG_DEVIO)(errfp,
				"Current disc=%d\n", curslot + 1);
		}
		else {
			switch (errno) {
			case EINVAL:
				/* Cannot query current disc:
				 * Set to single-disc mode.
				 */
				app_data.numdiscs = 1;
				app_data.chg_method = CHG_NONE;
				app_data.multi_play = FALSE;
				app_data.reverse = FALSE;
				s->first_disc = s->last_disc = s->cur_disc = 1;

				DBGPRN(DBG_DEVIO)(errfp,
					"Cannot determine current disc: %s\n",
					"Setting to single disc mode");

				DI_INFO(app_data.str_medchg_noinit);
				break;

			case ENOENT:
				/* No disc in current slot */
				DBGPRN(DBG_DEVIO)(errfp,
					"No disc in current slot.\n");
				/*FALLTHROUGH*/

			default:
				curslot = -1;
				break;
			}
		}
	}

	/* Change to the desired disc */
	if (app_data.numdiscs > 1 && curslot != (s->cur_disc - 1)) {
#ifdef NOT_NEEDED
	    {
		int	slotstat;

		DBGPRN(DBG_DEVIO)(errfp, "Checking disc %d\n", s->cur_disc);
		if (slioc_send(DI_ROLE_MAIN, CDROM_DRIVE_STATUS,
				(void *) (s->cur_disc - 1), 0,
				(bool_t) ((app_data.debug & DBG_DEVIO) != 0),
				&slotstat)) {
			DBGPRN(DBG_DEVIO)(errfp,
				"CDROM_DRIVE_STATUS return=0x%x\n", slotstat);

			switch (slotstat) {
			case CDS_NO_DISC:
				if (slioc_devp != NULL) {
					DBGPRN(DBG_DEVIO)(errfp,
						"\n%s: No disc in slot.\n",
						s->curdev);
					slioc_close();
					slioc_not_open = TRUE;
				}
				return FALSE;

			case CDS_TRAY_OPEN:
			case CDS_DRIVE_NOT_READY:
				if (slioc_devp != NULL) {
					DBGPRN(DBG_DEVIO)(errfp,
						"\n%s: Drive not ready.\n",
						s->curdev);
					slioc_close();
					slioc_not_open = TRUE;
				}
				return FALSE;

			case CDS_DISC_OK:
			case CDS_NO_INFO:
			default:
				/* Just proceed */
				break;
			}
		}
	    }
#endif

		DBGPRN(DBG_DEVIO)(errfp,
			"Changing to disc %d\n", s->cur_disc);

		if (!slioc_send(DI_ROLE_MAIN, CDROM_SELECT_DISC,
				(void *)(long) (s->cur_disc - 1), 0,
				(bool_t) ((app_data.debug & DBG_DEVIO) != 0),
				NULL)) {
			if (slioc_devp != NULL) {
				DBGPRN(DBG_DEVIO)(errfp,
					"\n%s: Disc change failed.\n",
					s->curdev);
				slioc_close();
				slioc_not_open = TRUE;
			}
			return FALSE;
		}
	}
#endif	/* _LINUX */

	return TRUE;
}


/*
 * slioc_init_vol
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
slioc_init_vol(curstat_t *s, bool_t preset)
{
	int	vol;

	/* Query current volume/balance settings */
	if ((vol = slioc_cfg_vol(0, s, TRUE)) >= 0)
		s->level = (byte_t) vol;
	else
		s->level = 0;

	/* Set volume to preset value, if so configured */
	if (app_data.startup_vol > 0 && preset) {
		s->level_left = s->level_right = 100;

		if ((vol = slioc_cfg_vol(app_data.startup_vol, s, FALSE)) >= 0)
			s->level = (byte_t) vol;
	}

	/* Initialize sliders */
	SET_VOL_SLIDER(s->level);
	SET_BAL_SLIDER((int) (s->level_right - s->level_left) / 2);

	/* Set up channel routing */
	slioc_route(s);
}


/*
 * slioc_getmcn
 *	Get the CD's media catalog number from the device.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
slioc_getmcn(curstat_t *s)
{
#ifdef CDROM_GET_MCN
	struct cdrom_mcn	mcn;
	int			i;
	bool_t			bad_data,
				ret;

	if (app_data.mcn_dsbl)
		return FALSE;

	(void) memset((byte_t *) &mcn, 0, sizeof(struct cdrom_mcn));
	ret = slioc_send(DI_ROLE_MAIN, CDROM_GET_MCN, &mcn,
			 sizeof(struct cdrom_mcn), FALSE, NULL);
	if (!ret) {
		(void) memset(s->mcn, 0, sizeof(s->mcn));
		return FALSE;
	}

	bad_data = FALSE;
	for (i = 0; i < 13; i++) {
		if (!isdigit((int) mcn.medium_catalog_number[i])) {
			bad_data = TRUE;
			break;
		}
	}

	if (!bad_data)
		(void) strncpy(s->mcn, (char *) mcn.medium_catalog_number, 13);
	else
		(void) memset(s->mcn, 0, sizeof(s->mcn));
#else
	(void) memset(s->mcn, 0, sizeof(s->mcn));
#endif
	return TRUE;
}


/*
 * slioc_rdsubq
 *	Send Read Subchannel command to the device
 *
 * Args:
 *	sp  - Pointer to the caller-supplied cdstat_t return structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
slioc_rdsubq(cdstat_t *sp)
{
	struct cdrom_subchnl	sub;
	bool_t			ret;

	(void) memset((byte_t *) &sub, 0, sizeof(struct cdrom_subchnl));

	sub.cdsc_format = app_data.subq_lba ? CDROM_LBA : CDROM_MSF;
	ret = slioc_send(DI_ROLE_MAIN, CDROMSUBCHNL, &sub,
			 sizeof(struct cdrom_subchnl), TRUE, NULL);
	if (!ret)
		return FALSE;

	DBGDUMP(DBG_DEVIO)("cdrom_subchnl data bytes", (byte_t *) &sub,
		sizeof(struct cdrom_subchnl));

	/* Hack: to work around firmware anomalies in some CD drives. */
	if (sub.cdsc_trk >= MAXTRACK && sub.cdsc_trk != LEAD_OUT_TRACK) {
		sp->status = CDSTAT_NOSTATUS;
		return TRUE;
	}

	/* Map the subchannel data into cdstat_t form */

	switch (sub.cdsc_audiostatus) {
	case CDROM_AUDIO_PLAY:
		sp->status = CDSTAT_PLAYING;
		break;
	case CDROM_AUDIO_PAUSED:
		sp->status = CDSTAT_PAUSED;
		break;
	case CDROM_AUDIO_COMPLETED:
		sp->status = CDSTAT_COMPLETED;
		break;
	case CDROM_AUDIO_ERROR:
		sp->status = CDSTAT_FAILED;
		break;
	case CDROM_AUDIO_INVALID:
	case CDROM_AUDIO_NO_STATUS:
	default:
		sp->status = CDSTAT_NOSTATUS;
		break;
	}

	if (slioc_bcd_hack) {
		/* Hack: BUGLY CD drive firmware */
		sp->track = (int) util_bcdtol(sub.cdsc_trk);
		sp->index = (int) util_bcdtol(sub.cdsc_ind);
	}
	else {
		sp->track = (int) sub.cdsc_trk;
		sp->index = (int) sub.cdsc_ind;
	}

	if (sub.cdsc_format == CDROM_LBA) {
		/* LBA mode */
		sp->abs_addr.addr = util_xlate_blk(sub.cdsc_absaddr.lba);
		util_blktomsf(
			sp->abs_addr.addr,
			&sp->abs_addr.min,
			&sp->abs_addr.sec,
			&sp->abs_addr.frame,
			MSF_OFFSET
		);

		sp->rel_addr.addr = util_xlate_blk(sub.cdsc_reladdr.lba);
		util_blktomsf(
			sp->rel_addr.addr,
			&sp->rel_addr.min,
			&sp->rel_addr.sec,
			&sp->rel_addr.frame,
#ifdef _LINUX
			/* Note: Linux pre-adjusts for MSF_OFFSET */
			MSF_OFFSET
#else
			0
#endif
		);
	}
	else {
		/* MSF mode */
		sp->abs_addr.min = sub.cdsc_absaddr.msf.minute;
		sp->abs_addr.sec = sub.cdsc_absaddr.msf.second;
		sp->abs_addr.frame = sub.cdsc_absaddr.msf.frame;
		util_msftoblk(
			sp->abs_addr.min,
			sp->abs_addr.sec,
			sp->abs_addr.frame,
			&sp->abs_addr.addr,
			MSF_OFFSET
		);

		sp->rel_addr.min = sub.cdsc_reladdr.msf.minute;
		sp->rel_addr.sec = sub.cdsc_reladdr.msf.second;
		sp->rel_addr.frame = sub.cdsc_reladdr.msf.frame;
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
 * slioc_rdtoc
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
slioc_rdtoc(struct cdrom_tochdr *h, struct cdrom_tocentry **e,
	    int start, bool_t msf)
{
	int			i,
				j,
				k,
				allocsz;
	struct cdrom_tocentry	*t;

	/* Read the TOC header first */
	if (!slioc_send(DI_ROLE_MAIN, CDROMREADTOCHDR, h,
			sizeof(struct cdrom_tochdr), TRUE, NULL))
		return FALSE;

	DBGDUMP(DBG_DEVIO)("cdrom_tochdr data bytes", (byte_t *) h,
		sizeof(struct cdrom_tochdr));

	if (start == 0)
		start = h->cdth_trk0;

	if (start > (int) h->cdth_trk1)
		return FALSE;

	allocsz = (h->cdth_trk1 - start + 2) * sizeof(struct cdrom_tocentry);

	*e = (struct cdrom_tocentry *)(void *) MEM_ALLOC(
		"cdrom_tocentry",
		allocsz
	);
	t = (struct cdrom_tocentry *)(void *) MEM_ALLOC(
		"cdrom_tocentry",
		allocsz
	);

	if (*e == NULL || t == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	(void) memset((byte_t *) *e, 0, allocsz);
	(void) memset((byte_t *) t, 0, allocsz);

	for (i = start; i <= (int) (h->cdth_trk1 + 1); i++) {
		j = i - start;

		t[j].cdte_track =
			(i <= (int) h->cdth_trk1) ? i : CDROM_LEADOUT;

		t[j].cdte_format = msf ? CDROM_MSF : CDROM_LBA;

		if (!slioc_send(DI_ROLE_MAIN, CDROMREADTOCENTRY, &t[j],
				sizeof(struct cdrom_tocentry), TRUE, NULL)) {
			MEM_FREE(*e);
			MEM_FREE(t);
			return FALSE;
		}

		DBGDUMP(DBG_DEVIO)("cdrom_tocentry data bytes",
			(byte_t *) &t[j], sizeof(struct cdrom_tocentry));

		/* Hack: workaround CD drive firmware bug
		 * Some CD drives return track numbers in BCD
		 * rather than binary.
		 */
		if ((int) (t[j].cdte_track & 0xf) > 0x9 &&
		    (int) (t[j].cdte_track & 0xf) < 0x10 &&
		    t[j].cdte_addr.lba == 0) {
			/* BUGLY CD drive firmware detected! */
			slioc_bcd_hack = TRUE;
		}

		/* Sanity check */
		if (t[j].cdte_track == CDROM_LEADOUT &&
		    t[j].cdte_addr.lba == t[0].cdte_addr.lba) {
			MEM_FREE(*e);
			MEM_FREE(t);
			return FALSE;
		}
	}

	/* Fix up TOC data */
	for (i = start; i <= (int) (h->cdth_trk1 + 1); i++) {
		if (slioc_bcd_hack && (i & 0xf) > 0x9 && (i & 0xf) < 0x10)
			continue;

		j = i - start;
		k = (slioc_bcd_hack ? util_bcdtol(i) : i) - start;

		(*e)[k].cdte_adr = t[j].cdte_adr;
		(*e)[k].cdte_ctrl = t[j].cdte_ctrl;
		(*e)[k].cdte_format = t[j].cdte_format;
		(*e)[k].cdte_addr.lba = t[j].cdte_addr.lba;
		(*e)[k].cdte_datamode = t[j].cdte_datamode;
		if (t[j].cdte_track == CDROM_LEADOUT) {
			(*e)[k].cdte_track = t[j].cdte_track;
			break;
		}
		else {
			if (slioc_bcd_hack)
				(*e)[k].cdte_track = (byte_t)
					util_bcdtol(t[j].cdte_track);
			else
				(*e)[k].cdte_track = t[j].cdte_track;
		}
	}
	if (slioc_bcd_hack)
		h->cdth_trk1 = (byte_t) util_bcdtol(h->cdth_trk1);

	MEM_FREE(t);

	return TRUE;
}


/*
 * slioc_disc_present
 *	Check if a CD is loaded.  Much of the complication in this
 *	routine stems from the fact that the SunOS and the Linux
 *	CD drivers behave differently when a CD is ejected.
 *	In fact, the scsi, mcd, sbpcd, cdu31a drivers under Linux
 *	are also inconsistent amongst each other (Argh!).
 *
 * Args:
 *	savstat - Whether to save start-up status in slioc_tst_status.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure (drive not ready)
 */
STATIC bool_t
slioc_disc_present(bool_t savstat)
{
	struct cdrom_subchnl	sub;
	struct cdrom_tochdr	h;
	struct cdrom_tocentry	e;
	sword32_t		a1,
				a2;
#ifdef _LINUX
	int			ret;
#endif
	static int		tot_trks = 0;
	static sword32_t	sav_a1 = 0,
				sav_a2 = 0;

#ifdef _LINUX
	/* Use new method to get drive status */
	if (slioc_send(DI_ROLE_MAIN, CDROM_DRIVE_STATUS, (void *) CDSL_CURRENT,
		       0, (bool_t) ((app_data.debug & DBG_DEVIO) != 0),
		       &ret)) {
		DBGPRN(DBG_DEVIO)(errfp,
			"CDROM_DRIVE_STATUS return=0x%x\n", ret);

		switch (ret) {
		case CDS_DISC_OK:
			return TRUE;
		case CDS_NO_DISC:
		case CDS_TRAY_OPEN:
		case CDS_DRIVE_NOT_READY:
			return FALSE;
		case CDS_NO_INFO:
			break;
		default:
			return FALSE;
		}
	}
	/* CDROM_DRIVE_STATUS failed: try the old way */
#endif

	if (savstat)
		slioc_tst_status = MOD_NODISC;

	/* Fake it with CDROMSUBCHNL */
	(void) memset((byte_t *) &sub, 0, sizeof(struct cdrom_subchnl));
	sub.cdsc_format = app_data.subq_lba ? CDROM_LBA : CDROM_MSF;
	if (!slioc_send(DI_ROLE_MAIN, CDROMSUBCHNL, &sub,
			sizeof(struct cdrom_subchnl),
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0), NULL))
		return FALSE;

	switch (sub.cdsc_audiostatus) {
	case CDROM_AUDIO_PLAY:
		if (savstat) {
			DBGPRN(DBG_DEVIO)(errfp,
				"\nstatus=CDROM_AUDIO_PLAY\n");
			slioc_tst_status = MOD_PLAY;
			return TRUE;
		}
		break;
	case CDROM_AUDIO_PAUSED:
		if (savstat) {
			DBGPRN(DBG_DEVIO)(errfp,
				"\nstatus=CDROM_AUDIO_PAUSED\n");
			slioc_tst_status = MOD_PAUSE;
			return TRUE;
		}
		break;
	case CDROM_AUDIO_ERROR:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CDROM_AUDIO_ERROR\n");
		break;
	case CDROM_AUDIO_COMPLETED:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CDROM_AUDIO_COMPLETED\n");
		break;
	case CDROM_AUDIO_NO_STATUS:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CDROM_AUDIO_NO_STATUS\n");
		break;
	case CDROM_AUDIO_INVALID:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=CDROM_AUDIO_INVALID\n");
		break;
	default:
		DBGPRN(DBG_DEVIO)(errfp, "\nstatus=unknown (%d)\n",
			sub.cdsc_audiostatus);
		return FALSE;
	}

	if (savstat)
		slioc_tst_status = MOD_STOP;

	/* CDROMSUBCHNL didn't give useful info.
	 * Try CDROMREADTOCHDR and CDROMREADTOCENTRY.
	 */
	(void) memset((byte_t *) &h, 0, sizeof(struct cdrom_tochdr));
	if (!slioc_send(DI_ROLE_MAIN, CDROMREADTOCHDR, &h,
			sizeof(struct cdrom_tochdr),
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0), NULL))
		return FALSE;

	if (h.cdth_trk0 == 0 && h.cdth_trk1 == 0)
		return FALSE;

	if ((h.cdth_trk1 - h.cdth_trk0 + 1) != tot_trks) {
		/* Disc changed */
		tot_trks = h.cdth_trk1 - h.cdth_trk0 + 1;
		return FALSE;
	}

	(void) memset((byte_t *) &e, 0, sizeof(struct cdrom_tocentry));
	e.cdte_format = app_data.toc_lba ? CDROM_LBA : CDROM_MSF;
	e.cdte_track = h.cdth_trk0;
	if (!slioc_send(DI_ROLE_MAIN, CDROMREADTOCENTRY, &e,
			sizeof(struct cdrom_tocentry),
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0), NULL))
		return FALSE;

	a1 = (sword32_t) e.cdte_addr.lba;

	(void) memset((byte_t *) &e, 0, sizeof(struct cdrom_tocentry));
	e.cdte_format = app_data.toc_lba ? CDROM_LBA : CDROM_MSF;
	e.cdte_track = h.cdth_trk1;
	if (!slioc_send(DI_ROLE_MAIN, CDROMREADTOCENTRY, &e,
			sizeof(struct cdrom_tocentry),
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0), NULL))
		return FALSE;

	a2 = (sword32_t) e.cdte_addr.lba;

	DBGPRN(DBG_DEVIO)(errfp, "\na1=0x%x a2=0x%x\n", a1, a2);

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
 * slioc_playmsf
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
slioc_playmsf(msf_t *start, msf_t *end)
{
	struct cdrom_msf	m;

	/* If the start or end positions are less than the minimum
	 * position, patch them to the minimum positions.
	 */
	if (start->min == 0 && start->sec < 2) {
		m.cdmsf_min0 = 0;
		m.cdmsf_sec0 = 2;
		m.cdmsf_frame0 = 0;
	}
	else {
		m.cdmsf_min0 = start->min;
		m.cdmsf_sec0 = start->sec;
		m.cdmsf_frame0 = start->frame;
	}

	if (end->min == 0 && end->sec < 2) {
		m.cdmsf_min1 = 0;
		m.cdmsf_sec1 = 2;
		m.cdmsf_frame1 = 0;
	}
	else {
		m.cdmsf_min1 = end->min;
		m.cdmsf_sec1 = end->sec;
		m.cdmsf_frame1 = end->frame;
	}

	/* If start == end, just return success */
	if (m.cdmsf_min0 == m.cdmsf_min1 &&
	    m.cdmsf_sec0 == m.cdmsf_sec1 &&
	    m.cdmsf_frame0 == m.cdmsf_frame1)
		return TRUE;

	DBGDUMP(DBG_DEVIO)("cdrom_msf data bytes", (byte_t *) &m,
		sizeof(struct cdrom_msf));

	return (
		slioc_send(DI_ROLE_MAIN, CDROMPLAYMSF, &m,
			   sizeof(struct cdrom_msf), TRUE, NULL)
	);
}


/*
 * slioc_play_trkidx
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
slioc_play_trkidx(int start_trk, int start_idx, int end_trk, int end_idx)
{
	struct cdrom_ti	t;

	if (slioc_bcd_hack) {
		/* Hack: BUGLY CD drive firmware */
		t.cdti_trk0 = util_ltobcd(start_trk);
		t.cdti_ind0 = util_ltobcd(start_idx);
		t.cdti_trk1 = util_ltobcd(end_trk);
		t.cdti_ind1 = util_ltobcd(end_idx);
	}
	else {
		t.cdti_trk0 = start_trk;
		t.cdti_ind0 = start_idx;
		t.cdti_trk1 = end_trk;
		t.cdti_ind1 = end_idx;
	}

	DBGDUMP(DBG_DEVIO)("cdrom_ti data bytes",
		(byte_t *) &t, sizeof(struct cdrom_ti));

	return (
		slioc_send(DI_ROLE_MAIN, CDROMPLAYTRKIND, &t,
			   sizeof(struct cdrom_ti), TRUE, NULL)
	);
}


/*
 * slioc_start_stop
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
slioc_start_stop(bool_t start, bool_t loej)
{
	bool_t		ret;
	curstat_t	*s = di_clinfo->curstat_addr();

	if (app_data.strict_pause_resume && slioc_paused)
		(void) slioc_pause_resume(TRUE);

	if (start) {
		if (loej)
#ifdef CDROMCLOSETRAY
			ret = slioc_send(DI_ROLE_MAIN, CDROMCLOSETRAY,
					 NULL, 0, TRUE, NULL);
#else
#ifdef CDROMLOAD
			ret = slioc_send(DI_ROLE_MAIN, CDROMLOAD,
					 NULL, 0, TRUE, NULL);
#else
			ret = FALSE;
#endif	/* CDROMLOAD */
#endif	/* CDROMCLOSETRAY */
		else
			ret = slioc_send(DI_ROLE_MAIN, CDROMSTART,
					 NULL, 0, TRUE, NULL);
	}
	else {
		slioc_playing = FALSE;

		if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
			(void) cdda_stop(slioc_devp, s);

			if (!slioc_is_enabled(DI_ROLE_MAIN)) {
				/* Enable I/O from current process */
				slioc_enable(DI_ROLE_MAIN);
			}
		}

		if (loej)
			ret = slioc_send(DI_ROLE_MAIN, CDROMEJECT,
					 NULL, 0, TRUE, NULL);
		else
			ret = slioc_send(DI_ROLE_MAIN, CDROMSTOP,
					 NULL, 0, TRUE, NULL);
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
 * slioc_pause_resume
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
slioc_pause_resume(bool_t resume)
{
	bool_t		ret;
	curstat_t	*s = di_clinfo->curstat_addr();

	if (!app_data.pause_supp)
		return FALSE;

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		ret = cdda_pause_resume(slioc_devp, s, resume);
	}
	else {
		ret = slioc_send(
			DI_ROLE_MAIN, resume ? CDROMRESUME : CDROMPAUSE,
			NULL, 0, TRUE, NULL
		);
	}
	if (ret)
		slioc_paused = !resume;

	return (ret);
}


/*
 * slioc_do_playaudio
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
slioc_do_playaudio(
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
		slioc_sav_end_msf = *end_msf;
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
		slioc_sav_end_addr = end_addr;
	}

	/* Save end address format for error recovery */
	slioc_sav_end_fmt = addr_fmt;

	do_playmsf = (addr_fmt & ADDR_MSF) && app_data.playmsf_supp;
	do_play10 = (addr_fmt & ADDR_BLK) && app_data.play10_supp;
	do_play12 = (addr_fmt & ADDR_BLK) && app_data.play12_supp;
	do_playti = (addr_fmt & ADDR_TRKIDX) && app_data.playti_supp;

	if (do_playmsf || do_playti) {
		if (slioc_paused) {
			if (app_data.strict_pause_resume) {
				/* Resume first */
				(void) slioc_pause_resume(TRUE);
			}
		}
		else if (slioc_playing) {
			if (app_data.play_pause_play) {
				/* Pause first */
				(void) slioc_pause_resume(FALSE);
			}
		}
		else {
			/* Spin up CD */
			(void) slioc_start_stop(TRUE, FALSE);
		}
	}

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		if (do_play12 || do_play10) {
			if (slioc_is_enabled(DI_ROLE_MAIN)) {
				/* Disable I/O from current process */
				slioc_disable(DI_ROLE_MAIN);
			}

			ret = cdda_play(slioc_devp, s, start_addr, end_addr);
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
			
			ret = cdda_play(slioc_devp, s, start_addr, end_addr);
		}
	}
	else {
		if (do_playmsf)
			ret = slioc_playmsf(start_msf, emsfp);
		
		if (!ret && do_playti)
			ret = slioc_play_trkidx(trk, idx, trk, idx);
	}

	if (ret) {
		slioc_playing = TRUE;
		slioc_paused = FALSE;
	}

	return (ret);
}


/*
 * slioc_play_recov
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
slioc_play_recov(sword32_t blk, bool_t iserr)
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
	if (recov_start_msf.min > slioc_sav_end_msf.min)
		ret = FALSE;
	else if (recov_start_msf.min == slioc_sav_end_msf.min) {
		if (recov_start_msf.sec > slioc_sav_end_msf.sec)
			ret = FALSE;
		else if ((recov_start_msf.sec ==
			  slioc_sav_end_msf.sec) &&
			 (recov_start_msf.frame >
			  slioc_sav_end_msf.frame)) {
			ret = FALSE;
		}
	}
	if (recov_start_addr >= slioc_sav_end_addr)
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

		ret = slioc_do_playaudio(
			slioc_sav_end_fmt,
			recov_start_addr, slioc_sav_end_addr,
			&recov_start_msf, &slioc_sav_end_msf,
			0, 0
		);
	}

	return (ret);
}


/*
 * slioc_get_playstatus
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
slioc_get_playstatus(curstat_t *s)
{
	cdstat_t		cdstat;
	word32_t		curtrk,
				curidx;
	bool_t			ret,
				done;
	static int		errcnt = 0,
				nostatcnt = 0;
	static sword32_t	errblk = 0;
	static bool_t		in_slioc_get_playstatus = FALSE;


	/* Lock this routine from multiple entry */
	if (in_slioc_get_playstatus)
		return TRUE;

	in_slioc_get_playstatus = TRUE;

	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		ret = cdda_getstatus(slioc_devp, s, &cdstat);
		if (ret &&
		    (cdstat.level != s->level ||
		     cdstat.level_left  != s->level_left ||
		     cdstat.level_right != s->level_right)) {
			int	vol;

			/* Update volume & balance level */
			s->level_left = cdstat.level_left;
			s->level_right = cdstat.level_right;
			vol = slioc_cfg_vol((int) cdstat.level, s, FALSE);
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
		ret = slioc_rdsubq(&cdstat);
		
	if (!ret) {
		/* Check to see if the disc had been manually ejected */
		if (!slioc_disc_ready(s)) {
			slioc_sav_end_addr = 0;
			slioc_sav_end_msf.min = 0;
			slioc_sav_end_msf.sec = 0;
			slioc_sav_end_msf.frame = 0;
			slioc_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_slioc_get_playstatus = FALSE;
			return FALSE;
		}

		/* Can't get playback status for some unknown reason.
		 * Just return success and hope the next poll succeeds.
		 * We don't want to return FALSE here because that would
		 * stop the poll.
		 */
		in_slioc_get_playstatus = FALSE;
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
	 * CDROM_AUDIO_PAUSED status after issuing a Stop Unit command.
	 * Just treat the status as completed if we get a paused status
	 * and we don't expect the drive to be paused.
	 */
	if (cdstat.status == CDSTAT_PAUSED && s->mode != MOD_PAUSE &&
	    !slioc_idx_pause)
		cdstat.status = CDSTAT_COMPLETED;

	/* Force completion status */
	if (slioc_fake_stop)
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
		if (!slioc_disc_ready(s)) {
			slioc_sav_end_addr = 0;
			slioc_sav_end_msf.min = 0;
			slioc_sav_end_msf.sec = 0;
			slioc_sav_end_msf.frame = 0;
			slioc_sav_end_fmt = 0;
			errcnt = 0;
			errblk = 0;

			in_slioc_get_playstatus = FALSE;
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

		if (!done && slioc_play_recov(errblk, TRUE)) {
			in_slioc_get_playstatus = FALSE;
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

		if (!slioc_fake_stop)
			slioc_playing = FALSE;

		slioc_fake_stop = FALSE;

		switch (s->mode) {
		case MOD_SAMPLE:
			done = !slioc_run_sample(s);
			break;

		case MOD_PLAY:
		case MOD_PAUSE:
			s->curpos_trk.addr = 0;
			s->curpos_trk.min = 0;
			s->curpos_trk.sec = 0;
			s->curpos_trk.frame = 0;

			if (s->shuffle || s->program)
				done = !slioc_run_prog(s);

			if (s->repeat && (s->program || !app_data.multi_play))
				done = !slioc_run_repeat(s);

			if (s->repeat && s->segplay == SEGP_AB)
				done = !slioc_run_ab(s);

			break;
		}

		break;
	}

	if (done) {
		byte_t	omode;
		bool_t	prog;

		/* Save some old states */
		omode = s->mode;
		prog = (s->program || s->onetrk_prog);

		/* Reset states */
		di_reset_curstat(s, FALSE, FALSE);
		s->mode = MOD_STOP;

		/* Cancel a->? if the user didn't select an end point */
		if (s->segplay == SEGP_A) {
			s->segplay = SEGP_NONE;
			DPY_PROGMODE(s, FALSE);
		}

		slioc_sav_end_addr = 0;
		slioc_sav_end_msf.min = slioc_sav_end_msf.sec =
			slioc_sav_end_msf.frame = 0;
		slioc_sav_end_fmt = 0;
		errcnt = 0;
		errblk = 0;

		if (app_data.multi_play && omode == MOD_PLAY && !prog) {
			bool_t	cont;

			s->prev_disc = s->cur_disc;

			if (app_data.reverse) {
				if (s->cur_disc > s->first_disc) {
					/* Play the previous disc */
					s->cur_disc--;
					slioc_mult_autoplay = TRUE;
				}
				else {
					/* Go to the last disc */
					s->cur_disc = s->last_disc;

					if (s->repeat) {
						s->rptcnt++;
						slioc_mult_autoplay = TRUE;
					}
				}
			}
			else {
				if (s->cur_disc < s->last_disc) {
					/* Play the next disc */
					s->cur_disc++;
					slioc_mult_autoplay = TRUE;
				}
				else {
					/* Go to the first disc.  */
					s->cur_disc = s->first_disc;

					if (s->repeat) {
						s->rptcnt++;
						slioc_mult_autoplay = TRUE;
					}
				}
			}

			if ((cont = slioc_mult_autoplay) == TRUE) {
				/* Allow recursion from this point */
				in_slioc_get_playstatus = FALSE;
			}

			/* Change disc */
			slioc_chgdisc(s);

			if (cont)
				return TRUE;
		}

		s->rptcnt = 0;
		DPY_ALL(s);

		if (app_data.done_eject) {
			/* Eject the disc */
			slioc_load_eject(s);
		}
		else {
			/* Spin down the disc */
			(void) slioc_start_stop(FALSE, FALSE);
		}
		if (app_data.done_exit) {
			/* Exit */
			di_clinfo->quit(s);
		}

		in_slioc_get_playstatus = FALSE;
		return FALSE;
	}

	in_slioc_get_playstatus = FALSE;
	return TRUE;
}


/*
 * slioc_cfg_vol
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
slioc_cfg_vol(int vol, curstat_t *s, bool_t query)
{
#ifdef CDROMVOLREAD
	int			vol1,
				vol2;
#endif
	struct cdrom_volctrl	volctrl;
	static bool_t		first = TRUE;

	if (!app_data.mselvol_supp)
		return 0;

	if (s->mode == MOD_BUSY)
		return -1;

	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		return (cdda_vol(slioc_devp, s, vol, query));

	(void) memset((byte_t *) &volctrl, 0, sizeof(struct cdrom_volctrl));

	if (query) {
		if (first) {
			first = FALSE;

#ifdef CDROMVOLREAD
			/* Try using the CDROMVOLREAD ioctl */
			if (slioc_send(DI_ROLE_MAIN, CDROMVOLREAD, &volctrl,
					sizeof(struct cdrom_volctrl),
					FALSE, NULL)) {
				DBGDUMP(DBG_DEVIO)("cdrom_volctrl data bytes",
					(byte_t *) &volctrl,
					sizeof(struct cdrom_volctrl));
				vol1 = util_untaper_vol(
				    util_unscale_vol((int) volctrl.channel0)
				);
				vol2 = util_untaper_vol(
				    util_unscale_vol((int) volctrl.channel1)
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
#endif	/* CDROMVOLREAD */

			/* There is no way to read volume setting via
			 * CDROM ioctl.  Force the setting to maximum.
			 */
			vol = 100;
			s->level_left = s->level_right = 100;

			(void) slioc_cfg_vol(vol, s, FALSE);
		}
		return (vol);
	}
	else {
		volctrl.channel0 = util_scale_vol(
			util_taper_vol(vol * (int) s->level_left / 100)
		);
		volctrl.channel1 = util_scale_vol(
			util_taper_vol(vol * (int) s->level_right / 100)
		);

		DBGDUMP(DBG_DEVIO)("cdrom_volctrl data bytes",
			(byte_t *) &volctrl,
			sizeof(struct cdrom_volctrl));

		if (slioc_send(DI_ROLE_MAIN, CDROMVOLCTRL, &volctrl,
			       sizeof(struct cdrom_volctrl), TRUE, NULL))
			return (vol);
		else if (volctrl.channel0 != volctrl.channel1) {
			/* Set the balance to the center
			 * and retry.
			 */
			volctrl.channel0 = volctrl.channel1 =
				util_scale_vol(util_taper_vol(vol));

			DBGDUMP(DBG_DEVIO)("cdrom_volctrl data bytes",
				(byte_t *) &volctrl,
				sizeof(struct cdrom_volctrl));

			if (slioc_send(DI_ROLE_MAIN, CDROMVOLCTRL, &volctrl,
				       sizeof(struct cdrom_volctrl),
				       TRUE, NULL)) {
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
 * slioc_vendor_model
 *	Query and update CD drive vendor/model/revision information
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_vendor_model(curstat_t *s)
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
 * slioc_fix_toc
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
slioc_fix_toc(curstat_t *s)
{
	int				i;
#ifdef CDROMMULTISESSION
	struct cdrom_multisession	m;
#endif

	/*
	 * Try the primitive method first.
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

#ifdef CDROMMULTISESSION
	(void) memset(&m, 0, sizeof(struct cdrom_multisession));
	m.addr_format = CDROM_LBA;

	if (!slioc_send(DI_ROLE_MAIN, CDROMMULTISESSION, &m,
			sizeof(struct cdrom_multisession),
			(bool_t) ((app_data.debug & DBG_DEVIO) != 0),
			NULL))
		return;		/* Multi-session not supported */

	if (m.xa_flag == 1) {
		/* Multi-session disc detected */
		s->discpos_tot.addr = (sword32_t) m.addr.lba;

		/* Subtract 02:32:00 from the start of last session
		 * and fix up end of audio location.  The 02:32:00
		 * is the inter-session lead-out and lead-in time plus
		 * the 2 second pre-gap for the last session.
		 */
		s->discpos_tot.addr -= 11400;

		util_blktomsf(
			s->discpos_tot.addr,
			&s->discpos_tot.min,
			&s->discpos_tot.sec,
			&s->discpos_tot.frame,
			MSF_OFFSET
		);
	}
#endif
}


/*
 * slioc_get_toc
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
slioc_get_toc(curstat_t *s)
{
	int			i,
				ntrks;
	struct cdrom_tochdr	h;
	struct cdrom_tocentry	*e,
				*p;

	if (!slioc_rdtoc(&h, &e, 0, !app_data.toc_lba))
		return FALSE;

	/* Fill curstat structure with TOC data */
	s->first_trk = h.cdth_trk0;
	ntrks = (int) (h.cdth_trk1 - h.cdth_trk0) + 1;

	p = e;

	for (i = 0; i <= (int) ntrks; i++) {
		s->trkinfo[i].trkno = p->cdte_track;
		s->trkinfo[i].type =
		    (p->cdte_ctrl & CDROM_DATA_TRACK) ? TYP_DATA : TYP_AUDIO;

		if (p->cdte_format == CDROM_LBA) {
			/* LBA mode */
			s->trkinfo[i].addr = util_xlate_blk(p->cdte_addr.lba);
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
			s->trkinfo[i].min = p->cdte_addr.msf.minute;
			s->trkinfo[i].sec = p->cdte_addr.msf.second;
			s->trkinfo[i].frame = p->cdte_addr.msf.frame;
			util_msftoblk(
				s->trkinfo[i].min,
				s->trkinfo[i].sec,
				s->trkinfo[i].frame,
				&s->trkinfo[i].addr,
				MSF_OFFSET
			);
		}

		if (p->cdte_track == CDROM_LEADOUT || i == (MAXTRACK - 1)) {
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

	slioc_fix_toc(s);
	return TRUE;
}


/*
 * slioc_start_stat_poll
 *	Start polling the drive for current playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_start_stat_poll(curstat_t *s)
{
	slioc_stat_polling = TRUE;

	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		slioc_stat_id = di_clinfo->timeout(
			slioc_stat_interval,
			slioc_stat_poll,
			(byte_t *) s
		);
	}
}


/*
 * slioc_stop_stat_poll
 *	Stop polling the drive for current playback status
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_stop_stat_poll(void)
{
	if (slioc_stat_polling) {
		/* Stop poll timer */
		if (di_clinfo->untimeout != NULL)
			di_clinfo->untimeout(slioc_stat_id);

		slioc_stat_polling = FALSE;
	}
}


/*
 * slioc_start_insert_poll
 *	Start polling the drive for disc insertion
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_start_insert_poll(curstat_t *s)
{
	int	delay;
	bool_t	first = TRUE;

	if (slioc_insert_polling || app_data.ins_disable ||
	    (s->mode != MOD_BUSY && s->mode != MOD_NODISC))
		return;

	if (app_data.numdiscs > 1 && app_data.multi_play)
		slioc_ins_interval =
			app_data.ins_interval / app_data.numdiscs;
	else
		slioc_ins_interval = app_data.ins_interval;

	if (slioc_ins_interval < 500)
		slioc_ins_interval = 500;

	if (first) {
		first = FALSE;
		delay = 50;
	}
	else
		delay = slioc_ins_interval;

	/* Start poll timer */
	if (di_clinfo->timeout != NULL) {
		slioc_insert_id = di_clinfo->timeout(
			delay,
			slioc_insert_poll,
			(byte_t *) s
		);

		if (slioc_insert_id != 0)
			slioc_insert_polling = TRUE;
	}
}


/*
 * slioc_stat_poll
 *	The playback status polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_stat_poll(curstat_t *s)
{
	if (!slioc_stat_polling)
		return;

	/* Get current audio playback status */
	if (slioc_get_playstatus(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			slioc_stat_id = di_clinfo->timeout(
				slioc_stat_interval,
				slioc_stat_poll,
				(byte_t *) s
			);
		}
	}
	else
		slioc_stat_polling = FALSE;
}


/*
 * slioc_insert_poll
 *	The disc insertion polling function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_insert_poll(curstat_t *s)
{
	/* Check to see if a disc is inserted */
	if (!slioc_disc_ready(s)) {
		/* Register next poll interval */
		if (di_clinfo->timeout != NULL) {
			slioc_insert_id = di_clinfo->timeout(
				slioc_ins_interval,
				slioc_insert_poll,
				(byte_t *) s
			);
		}
	}
	else
		slioc_insert_polling = FALSE;
}


/*
 * slioc_disc_ready
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
slioc_disc_ready(curstat_t *s)
{
	bool_t		no_disc = FALSE;
	static bool_t	first_open = TRUE,
			in_slioc_disc_ready = FALSE;

	/* Lock this routine from multiple entry */
	if (in_slioc_disc_ready)
		return TRUE;

	in_slioc_disc_ready = TRUE;

	/* If device has not been opened, attempt to open it */
	if (slioc_not_open) {
		/* Check for another copy of the CD player running on
		 * the specified device.
		 */
		if (!s->devlocked && !di_devlock(s, app_data.device)) {
			s->mode = MOD_BUSY;
			DPY_TIME(s, FALSE);
			slioc_start_insert_poll(s);
			in_slioc_disc_ready = FALSE;
			return FALSE;
		}

		s->devlocked = TRUE;
		s->mode = MOD_NODISC;
		DPY_DISC(s);

		if (slioc_open(s)) {
			slioc_not_open = FALSE;

			if (!slioc_is_enabled(DI_ROLE_MAIN)) {
				/* Enable device for I/O */
				slioc_enable(DI_ROLE_MAIN);
			}

			/* Check if a disc is loaded and ready */
			no_disc = !slioc_disc_present(first_open);
		}
		else {
			DBGPRN(DBG_DEVIO)(errfp,
				"Open of %s failed\n", s->curdev);
			no_disc = TRUE;
		}
	}
	else {
		/* Just return success if we're playing CDDA */
		if (!slioc_is_enabled(DI_ROLE_MAIN)) {
			no_disc = FALSE;
		}
		else if ((no_disc = !slioc_disc_present(FALSE)) == TRUE) {
			/* The disc was manually ejected */
			s->mode = MOD_NODISC;
			di_clear_cdinfo(s, FALSE);
		}
	}

	if (!no_disc) {
		if (first_open) {
			first_open = FALSE;

			/* Fill in inquiry data */
			slioc_vendor_model(s);

			/* Initializa volume/balance/routing controls */
			slioc_init_vol(s, TRUE);
		}
		else {
			/* Force to current settings */
			(void) slioc_cfg_vol(s->level, s, FALSE);

			/* Set up channel routing */
			slioc_route(s);
		}
	}

	/* Read disc table of contents if a new disc was detected */
	if (slioc_not_open || no_disc ||
	    (s->mode == MOD_NODISC && !slioc_get_toc(s))) {
		if (slioc_devp != NULL && app_data.eject_close) {
			slioc_close();
			slioc_not_open = TRUE;
		}

		di_reset_curstat(s, TRUE, (bool_t) !app_data.multi_play);
		DPY_ALL(s);

		if (app_data.multi_play) {
			/* This requested disc is not there
			 * or not ready.
			 */
			if (app_data.reverse) {
				if (s->cur_disc > s->first_disc) {
					/* Try the previous disc */
					s->cur_disc--;
				}
				else {
					/* Go to the last disc */
					s->cur_disc = s->last_disc;

					if (slioc_mult_autoplay) {
					    if (s->repeat)
						s->rptcnt++;
					    else
						slioc_mult_autoplay = FALSE;
					}
				}
			}
			else {
				if (s->cur_disc < s->last_disc) {
					/* Try the next disc */
					s->cur_disc++;
				}
				else if (!s->chgrscan) {
					/* Go to the first disc */
					s->cur_disc = s->first_disc;

					if (slioc_mult_autoplay) {
					    if (s->repeat)
						s->rptcnt++;
					    else
						slioc_mult_autoplay = FALSE;
					}
				}
				else {
					/* Done scanning - no disc */
					STOPSCAN(s);
				}
			}
		}

		slioc_start_insert_poll(s);
		in_slioc_disc_ready = FALSE;
		return FALSE;
	}

	if (s->mode != MOD_NODISC) {
		in_slioc_disc_ready = FALSE;
		return TRUE;
	}

	/* Load saved track program, if any */
	PROGGET(s);

	s->mode = MOD_STOP;
	DPY_ALL(s);

	if (!app_data.mcn_dsbl) {
		/* Get Media catalog number of CD, if available */
		(void) slioc_getmcn(s);
	}

	if (app_data.load_play || slioc_mult_autoplay) {
		slioc_mult_autoplay = FALSE;

		/* Wait a little while for things to settle */
		util_delayms(1000);

		/* Start auto-play */
		if (!slioc_override_ap)
			slioc_play_pause(s);

		if (slioc_mult_pause) {
			slioc_mult_pause = FALSE;

			if (slioc_pause_resume(FALSE)) {
				slioc_stop_stat_poll();
				s->mode = MOD_PAUSE;
				DPY_PLAYMODE(s, FALSE);
			}
		}
	}
	else if (app_data.load_spindown) {
		/* Spin down disc in case the user isn't going to
		 * play anything for a while.  This reduces wear and
		 * tear on the drive.
		 */
		(void) slioc_start_stop(FALSE, FALSE);
	}
	else {
		switch (slioc_tst_status) {
		case MOD_PLAY:
		case MOD_PAUSE:
			/* Drive is current playing audio or paused:
			 * act appropriately.
			 */
			s->mode = slioc_tst_status;
			(void) slioc_get_playstatus(s);
			DPY_ALL(s);
			if (s->mode == MOD_PLAY)
				slioc_start_stat_poll(s);
			break;
		default:
			/* Drive is stopped: do nothing */
			break;
		}
	}

	in_slioc_disc_ready = FALSE;

	/* Load CD information for this disc.
	 * This operation has to be done outside the scope of
	 * in_slioc_disc_ready because it may recurse
	 * back into this function.
	 */
	(void) di_get_cdinfo(s);

	return TRUE;
}


/*
 * slioc_run_rew
 *	Run search-rewind operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_run_rew(curstat_t *s)
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
	if (!slioc_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (slioc_start_search) {
		slioc_start_search = FALSE;
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
	(void) slioc_do_playaudio(
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		slioc_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			slioc_run_rew,
			(byte_t *) s
		);
	}
}


/*
 * slioc_stop_rew
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
slioc_stop_rew(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(slioc_search_id);
}


/*
 * slioc_run_ff
 *	Run search-fast-forward operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
slioc_run_ff(curstat_t *s)
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
	if (!slioc_get_playstatus(s)) {
		DO_BEEP();
		return;
	}

	skip_blks = app_data.skip_blks;
	addr = s->curpos_tot.addr;

	if (slioc_start_search) {
		slioc_start_search = FALSE;
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
	(void) slioc_do_playaudio(
		ADDR_BLK | ADDR_MSF | ADDR_OPTEND,
		start_addr, end_addr,
		&smsf, &emsf,
		0, 0
	);

	if (di_clinfo->timeout != NULL) {
		slioc_search_id = di_clinfo->timeout(
			app_data.skip_pause,
			slioc_run_ff,
			(byte_t *) s
		);
	}
}


/*
 * slioc_stop_ff
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
slioc_stop_ff(curstat_t *s)
{
	if (di_clinfo->untimeout != NULL)
		di_clinfo->untimeout(slioc_search_id);
}


/*
 * slioc_run_ab
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
slioc_run_ab(curstat_t *s)
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
	slioc_start_stat_poll(s);

	return (
		slioc_do_playaudio(
			ADDR_BLK | ADDR_MSF,
			s->bp_startpos_tot.addr, s->bp_endpos_tot.addr,
			&start_msf, &end_msf,
			0, 0
		)
	);
}


/*
 * slioc_run_sample
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
slioc_run_sample(curstat_t *s)
{
	sword32_t	saddr,
			eaddr;
	msf_t		smsf,
			emsf;

	if (slioc_next_sam < s->tot_trks) {
		saddr = s->trkinfo[slioc_next_sam].addr;
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

		/* Sample only audio tracks */
		if (s->trkinfo[slioc_next_sam].type != TYP_AUDIO ||
		    slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				       saddr, eaddr, &smsf, &emsf, 0, 0)) {
			slioc_next_sam++;
			return TRUE;
		}
	}

	slioc_next_sam = 0;
	return FALSE;
}


/*
 * slioc_run_prog
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
slioc_run_prog(curstat_t *s)
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

	if (slioc_new_progshuf) {
		slioc_new_progshuf = FALSE;

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

	if (s->trkinfo[i].trkno == CDROM_LEADOUT)
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

	ret = slioc_do_playaudio(
		ADDR_BLK | ADDR_MSF,
		start_addr, end_addr,
		&start_msf, &end_msf,
		0, 0
	);

	if (s->mode == MOD_PAUSE) {
		(void) slioc_pause_resume(FALSE);

		/* Restore volume */
		slioc_mute_off(s);
	}

	return (ret);
}


/*
 * slioc_run_repeat
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
slioc_run_repeat(curstat_t *s)
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

		slioc_new_progshuf = TRUE;
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

		ret = slioc_do_playaudio(
			ADDR_BLK | ADDR_MSF,
			s->trkinfo[0].addr, s->discpos_tot.addr,
			&start_msf, &end_msf, 0, 0
		);

		if (s->mode == MOD_PAUSE) {
			(void) slioc_pause_resume(FALSE);

			/* Restore volume */
			slioc_mute_off(s);
		}

	}

	return (ret);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * slioc_enable
 *	Enable device in this process for I/O
 *
 * Args:
 *	role - Role id for which I/O is to be enabled
 *
 * Return:
 *	Nothing.
 */
void
slioc_enable(int role)
{
	DBGPRN(DBG_DEVIO)(errfp, "Enable device: %s role: %d\n",
			  slioc_devp->path, role);
	slioc_devp->role = role;
}


/*
 * slioc_disable
 *	Disable device in this process for I/O
 *
 * Args:
 *	role - Role id for which I/O is to be disabled
 *
 * Return:
 *	Nothing.
 */
void
slioc_disable(int role)
{
	if (slioc_devp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp, "slioc_disable: invalid role: %d\n",
			  	  role);
		return;
	}

	DBGPRN(DBG_DEVIO)(errfp, "Disable device: %s role: %d\n",
			  slioc_devp->path, role);
	slioc_devp->role = 0;
}


/*
 * slioc_is_enabled
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
slioc_is_enabled(int role)
{
	return ((bool_t) (slioc_devp->role == role));
}


/*
 * slioc_send
 *	Issue ioctl command.
 *
 * Args:
 *	role - role id for which command is to be sent
 *	cmd - ioctl command
 *	arg - ioctl argument
 *	arglen - sizeof(*arg)
 *	prnerr - whether an error message is to be displayed if the ioctl fails
 *	retloc - Pointer to storage where the return value of the ioctl can be
 *		 stored, or NULL.
 *
 * Return:
 *	TRUE - ioctl successful
 *	FALSE - ioctl failed
 */
bool_t
slioc_send(
	int	role,
	int	cmd,
	void	*arg,
	size_t	arglen,
	bool_t	prnerr,
	int	*retloc
)
{
	int		i,
			ret,
			saverrno;
	curstat_t	*s = di_clinfo->curstat_addr();

	if (slioc_devp == NULL || slioc_devp->fd <= 0)
		return FALSE;

	if (slioc_devp->role != role) {
		DBGPRN(DBG_DEVIO)(errfp,
			"NOTICE: slioc_send: disabled role! (%d)\n",
			role);
		return FALSE;
	}

	if (app_data.debug & DBG_DEVIO) {
		for (i = 0; iname[i].name != NULL; i++) {
			if (iname[i].cmd == cmd) {
				(void) fprintf(errfp, "\nIOCTL: %s arg=0x%lx ",
					       iname[i].name,
					       (unsigned long) arg);
				break;
			}
		}
		if (iname[i].name == NULL)
			(void) fprintf(errfp, "\nIOCTL: 0x%x arg=0x%lx ",
				       cmd, (unsigned long) arg);
	}

#ifdef __QNX__
	switch (cmd) {
	case CDROMVOLCTRL:
	case CDROMPLAYTRKIND:
#ifdef CDROMLOAD
	case CDROMLOAD:
#endif
		ret = ioctl(slioc_devp->fd, cmd, arg);
		break;
	default:
		/* Use proprietary QNX ioctl */
		ret = qnx_ioctl(slioc_devp->fd, cmd, arg, arglen, arg, arglen);
		break;
	}
#else
	ret = ioctl(slioc_devp->fd, cmd, arg);
#endif

	DBGPRN(DBG_DEVIO)(errfp, "ret=%d\n", ret);

	if (ret < 0) {
		if (prnerr) {
			saverrno = errno;

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

			errno = saverrno;
		}

		return FALSE;
	}

	if (retloc != NULL)
		*retloc = ret;

	return TRUE;
}


/*
 * slioc_init
 *	Top-level function to initialize the SunOS/Solaris/Linux ioctl
 *	method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_init(curstat_t *s, di_tbl_t *dt)
{
	if (app_data.di_method != DI_SLIOC)
		/* SunOS/Solaris/Linux/QNX ioctl method not configured */
		return;

	/* Initialize libdi calling table */
	dt->load_cdtext = NULL;
	dt->playmode = slioc_playmode;
	dt->check_disc = slioc_check_disc;
	dt->status_upd = slioc_status_upd;
	dt->lock = slioc_lock;
	dt->repeat = slioc_repeat;
	dt->shuffle = slioc_shuffle;
	dt->load_eject = slioc_load_eject;
	dt->ab = slioc_ab;
	dt->sample = slioc_sample;
	dt->level = slioc_level;
	dt->play_pause = slioc_play_pause;
	dt->stop = slioc_stop;
	dt->chgdisc = slioc_chgdisc;
	dt->prevtrk = slioc_prevtrk;
	dt->nexttrk = slioc_nexttrk;
	dt->previdx = slioc_previdx;
	dt->nextidx = slioc_nextidx;
	dt->rew = slioc_rew;
	dt->ff = slioc_ff;
	dt->warp = slioc_warp;
	dt->route = slioc_route;
	dt->mute_on = slioc_mute_on;
	dt->mute_off = slioc_mute_off;
	dt->cddajitter = slioc_cddajitter;
	dt->debug = slioc_debug;
	dt->start = slioc_start;
	dt->icon = slioc_icon;
	dt->halt = slioc_halt;
	dt->methodstr = slioc_methodstr;

	/* Hardwire some unsupported features */
	app_data.caddylock_supp = FALSE;
	app_data.caddy_lock = FALSE;

	/* Initalize SunOS/Solaris/Linux ioctl method */
	slioc_stat_polling = FALSE;
	slioc_stat_interval = app_data.stat_interval;
	slioc_insert_polling = FALSE;
	slioc_next_sam = FALSE;
	slioc_new_progshuf = FALSE;
	slioc_sav_end_addr = 0;
	slioc_sav_end_msf.min = slioc_sav_end_msf.sec =
		slioc_sav_end_msf.frame = 0;
	slioc_sav_end_fmt = 0;

	/* Initialize curstat structure */
	di_reset_curstat(s, TRUE, TRUE);

	/* Sanity check CD changer configuration */
	if (app_data.numdiscs > 1) {
#ifdef _LINUX
		if (app_data.chg_method != CHG_OS_IOCTL) {
			app_data.numdiscs = 1;
			DBGPRN(DBG_DEVIO)(errfp,
				"CD changer: invalid method %d:\n%s\n%s\n",
				app_data.chg_method,
				"Only CHG_OS_IOCTL is supported in this mode.",
				"Setting to single disc mode.");
			DI_INFO(app_data.str_medchg_noinit);
		}
#else
		/* Changer support is only for Linux */
		app_data.numdiscs = 1;
		DBGPRN(DBG_DEVIO)(errfp,
			"CD changer: only supported on Linux:\n%s\n",
			"Setting to single disc mode.");
#endif
		/* Configuration error: Set to * single-disc mode. */
		if (app_data.numdiscs == 1) {
			app_data.chg_method = CHG_NONE;
			app_data.multi_play = FALSE;
			app_data.reverse = FALSE;
			s->first_disc = s->last_disc = s->cur_disc = 1;
		}
	}

#ifdef _LINUX
	DBGPRN(DBG_DEVIO)(errfp, "libdi: Linux ioctl method\n");
#endif
#if defined(_SUNOS4) || defined(_SOLARIS)
	DBGPRN(DBG_DEVIO)(errfp, "libdi: SunOS/Solaris ioctl method\n");
#endif
#ifdef __QNX__
	DBGPRN(DBG_DEVIO)(errfp, "libdi: QNX ioctl method\n");
#endif
}


/*
 * slioc_playmode
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
slioc_playmode(curstat_t *s)
{
	bool_t		ret,
			cdda;
	static bool_t	prev_cdda = FALSE;

	cdda = (bool_t) PLAYMODE_IS_CDDA(app_data.play_mode);

	if (cdda == prev_cdda)
		return TRUE;	/* No change */

	if (cdda) {
		slioc_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		slioc_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		slioc_cdda_client.warning_msg = di_clinfo->warning_msg;
		slioc_cdda_client.info_msg = di_clinfo->info_msg;
		slioc_cdda_client.info2_msg = di_clinfo->info2_msg;

		ret = cdda_init(s, &slioc_cdda_client);
	}
	else {
		cdda_halt(slioc_devp, s);
		ret = TRUE;

		/* Initialize volume/balance/routing controls */
		slioc_init_vol(s, FALSE);
	}

	if (ret)
		prev_cdda = cdda;

	return (ret);
}


/*
 * slioc_check_disc
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
slioc_check_disc(curstat_t *s)
{
	return (slioc_disc_ready(s));
}


/*
 * slioc_status_upd
 *	Force update of playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_status_upd(curstat_t *s)
{
	(void) slioc_get_playstatus(s);
}


/*
 * slioc_lock
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
slioc_lock(curstat_t *s, bool_t enable)
{
	/* Caddy lock function currently not supported
	 * under SunOS/Solaris/Linux ioctl method
	 */
	if (enable) {
		DO_BEEP();
		SET_LOCK_BTN(FALSE);
	}
}


/*
 * slioc_repeat
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
slioc_repeat(curstat_t *s, bool_t enable)
{
	s->repeat = enable;

	if (!enable && slioc_new_progshuf) {
		slioc_new_progshuf = FALSE;
		if (s->rptcnt > 0)
			s->rptcnt--;
	}
	DPY_RPTCNT(s);
}


/*
 * slioc_shuffle
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
slioc_shuffle(curstat_t *s, bool_t enable)
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
 * slioc_load_eject
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
slioc_load_eject(curstat_t *s)
{
	bool_t	ret = FALSE;

	if (slioc_devp == NULL)
		return;

	if (slioc_is_enabled(DI_ROLE_MAIN) && !slioc_disc_present(FALSE)) {
		/* No disc */
		if (app_data.load_supp) {
			/* Try loading the disc */
			ret = slioc_start_stop(TRUE, TRUE);
		}

		if (!ret && app_data.eject_supp) {
			/* Cannot load, maybe the tray is already closed
			 * but empty.  Try opening the tray.
			 */
			if (slioc_start_stop(FALSE, TRUE))
				DO_BEEP();
		}

		slioc_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (slioc_devp != NULL && app_data.eject_close) {
			slioc_close();
			slioc_not_open = TRUE;
		}

		slioc_start_insert_poll(s);
		return;
	}

	/* Eject the disc */

	/* Spin down the CD */
	(void) slioc_start_stop(FALSE, FALSE);

	if (!app_data.eject_supp) {
		DO_BEEP();

		slioc_stop_stat_poll();
		di_reset_curstat(s, TRUE, TRUE);
		s->mode = MOD_NODISC;

		di_clear_cdinfo(s, FALSE);
		DPY_ALL(s);

		if (slioc_devp != NULL && app_data.eject_close) {
			slioc_close();
			slioc_not_open = TRUE;
		}

		slioc_start_insert_poll(s);
		return;
	}

	slioc_stop_stat_poll();
	di_reset_curstat(s, TRUE, TRUE);
	s->mode = MOD_NODISC;

	di_clear_cdinfo(s, FALSE);
	DPY_ALL(s);

	/* Eject the CD */
	(void) slioc_start_stop(FALSE, TRUE);

	if (app_data.eject_exit)
		di_clinfo->quit(s);
	else {
		if (slioc_devp != NULL && app_data.eject_close) {
			slioc_close();
			slioc_not_open = TRUE;
		}
			
		slioc_start_insert_poll(s);
	}
}


/*
 * slioc_ab
 *	A->B segment play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_ab(curstat_t *s)
{
	if (!slioc_run_ab(s))
		DO_BEEP();
}


/*
 * slioc_sample
 *	Sample play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_sample(curstat_t *s)
{
	int	i;

	if (!slioc_disc_ready(s)) {
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
		slioc_start_stat_poll(s);
		/*FALLTHROUGH*/
	case MOD_PLAY:
		/* If already playing a track, start sampling the track after
		 * the current one.  Otherwise, sample from the beginning.
		 */
		if (s->cur_trk > 0 && s->cur_trk != s->last_trk) {
			i = di_curtrk_pos(s) + 1;
			s->cur_trk = s->trkinfo[i].trkno;
			slioc_next_sam = (byte_t) i;
		}
		else {
			s->cur_trk = s->first_trk;
			slioc_next_sam = 0;
		}
		
		s->cur_idx = 1;

		s->mode = MOD_SAMPLE;
		DPY_ALL(s);

		if (!slioc_run_sample(s))
			return;

		break;

	case MOD_SAMPLE:
		/* Currently doing Sample playback, just call slioc_play_pause
		 * to resume normal playback.
		 */
		slioc_play_pause(s);
		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * slioc_level
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
slioc_level(curstat_t *s, byte_t level, bool_t drag)
{
	int	actual;

	/* Set volume level */
	if ((actual = slioc_cfg_vol((int) level, s, FALSE)) >= 0)
		s->level = (byte_t) actual;
}


/*
 * slioc_play_pause
 *	Audio playback and pause function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_play_pause(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;

	slioc_override_ap = TRUE;

	if (!slioc_disc_ready(s)) {
		slioc_override_ap = FALSE;
		DO_BEEP();
		return;
	}

	slioc_override_ap = FALSE;

	if (s->mode == MOD_NODISC)
		s->mode = MOD_STOP;

	switch (s->mode) {
	case MOD_PLAY:
		/* Currently playing: go to pause mode */

		if (!slioc_pause_resume(FALSE)) {
			DO_BEEP();
			return;
		}
		slioc_stop_stat_poll();
		s->mode = MOD_PAUSE;
		DPY_PLAYMODE(s, FALSE);
		break;

	case MOD_PAUSE:
		/* Currently paused: resume play */

		if (!slioc_pause_resume(TRUE)) {
			DO_BEEP();
			return;
		}
		s->mode = MOD_PLAY;
		DPY_PLAYMODE(s, FALSE);
		slioc_start_stat_poll(s);
		break;

	case MOD_STOP:
		/* Currently stopped: start play */

		if (!di_prepare_cdda(s))
			return;

		if (s->shuffle || s->program) {
			slioc_new_progshuf = TRUE;

			/* Start shuffle/program play */
			if (!slioc_run_prog(s))
				return;
		}
		else if (s->segplay == SEGP_AB) {
			/* Play defined segment */
			if (!slioc_run_ab(s))
				return;
		}
		else {
			s->segplay = SEGP_NONE; /* Cancel a->b mode */

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

			if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				s->mode = MOD_STOP;
				return;
			}
		}

		DPY_ALL(s);
		slioc_start_stat_poll(s);
		break;

	case MOD_SAMPLE:
		/* Force update of curstat */
		if (!slioc_get_playstatus(s)) {
			DO_BEEP();
			return;
		}

		/* Currently doing a->b or sample playback: just resume play */
		if (s->shuffle || s->program) {
			if ((i = di_curtrk_pos(s)) < 0 ||
			    s->trkinfo[i].trkno == CDROM_LEADOUT)
				return;

			start_msf.min = s->curpos_tot.min;
			start_msf.sec = s->curpos_tot.sec;
			start_msf.frame = s->curpos_tot.frame;
			end_msf.min = s->trkinfo[i+1].min;
			end_msf.sec = s->trkinfo[i+1].sec;
			end_msf.frame = s->trkinfo[i+1].frame;

			if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
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

			if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
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
 * slioc_stop
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
slioc_stop(curstat_t *s, bool_t stop_disc)
{
	/* The stop_disc parameter will cause the disc to spin down.
	 * This is usually set to TRUE, but can be FALSE if the caller
	 * just wants to set the current state to stop but will
	 * immediately go into play state again.  Not spinning down
	 * the drive makes things a little faster...
	 */
	if (!slioc_disc_ready(s))
		return;

	switch (s->mode) {
	case MOD_PLAY:
	case MOD_PAUSE:
	case MOD_SAMPLE:
	case MOD_STOP:
		/* Currently playing or paused: stop */

		if (stop_disc && !slioc_start_stop(FALSE, FALSE)) {
			DO_BEEP();
			return;
		}
		slioc_stop_stat_poll();

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
 * slioc_chgdisc
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
slioc_chgdisc(curstat_t *s)
{
	int	sav_rptcnt;

	if (s->first_disc == s->last_disc) {
		/* Single-CD drive: cannot change discs */
		DO_BEEP();
		return;
	}

	if (s->cur_disc < s->first_disc || s->cur_disc > s->last_disc) {
		/* Bogus disc number */
		s->cur_disc = s->first_disc;
		return;
	}

	if (s->segplay != SEGP_NONE) {
		s->segplay = SEGP_NONE; /* Cancel a->b mode */
		DPY_PROGMODE(s, FALSE);
	}

	/* If we're currently in normal playback mode, after we change
	 * disc we want to automatically start playback.
	 */
	if ((s->mode == MOD_PLAY || s->mode == MOD_PAUSE) && !s->program)
		slioc_mult_autoplay = TRUE;

	/* If we're currently paused, go to pause mode after disc change */
	slioc_mult_pause = (s->mode == MOD_PAUSE);

	sav_rptcnt = s->rptcnt;

	/* Stop the CD first */
	slioc_stop(s, TRUE);

	di_reset_curstat(s, TRUE, FALSE);
	s->mode = MOD_NODISC;
	di_clear_cdinfo(s, FALSE);

	s->rptcnt = sav_rptcnt;

	if (slioc_devp != NULL && app_data.eject_close) {
		slioc_close();
		slioc_not_open = TRUE;
	}

	/* Load desired disc */
	slioc_disc_ready(s);
}


/*
 * slioc_prevtrk
 *	Previous track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_prevtrk(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;
	bool_t		go_prev;

	if (!slioc_disc_ready(s)) {
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
				slioc_new_progshuf = FALSE;
			}
			i = di_curprog_pos(s);
		}
		else
			i = di_curtrk_pos(s);

		if (s->segplay == SEGP_AB) {
			s->segplay = SEGP_NONE; /* Cancel a->b mode */
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
					slioc_new_progshuf = FALSE;
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
			slioc_mute_on(s);

		if (s->shuffle || s->program) {
			/* Program/Shuffle mode: just stop the playback
			 * and let slioc_run_prog go to the previous track
			 */
			slioc_fake_stop = TRUE;

			/* Force status update */
			(void) slioc_get_playstatus(s);
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

			if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();

				/* Restore volume */
				slioc_mute_off(s);
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) slioc_pause_resume(FALSE);

				/* Restore volume */
				slioc_mute_off(s);
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
 * slioc_nexttrk
 *	Next track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_nexttrk(curstat_t *s)
{
	sword32_t	i,
			start_addr;
	msf_t		start_msf,
			end_msf;

	if (!slioc_disc_ready(s)) {
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
				if (s->repeat && !app_data.multi_play)
					slioc_new_progshuf = TRUE;
				else
					return;
			}

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				slioc_mute_on(s);

			/* Program/Shuffle mode: just stop the playback
			 * and let slioc_run_prog go to the next track.
			 */
			slioc_fake_stop = TRUE;

			/* Force status update */
			(void) slioc_get_playstatus(s);

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
		    s->trkinfo[i].trkno != CDROM_LEADOUT) {

			start_addr = s->trkinfo[i].addr;
			start_msf.min = s->trkinfo[i].min;
			start_msf.sec = s->trkinfo[i].sec;
			start_msf.frame = s->trkinfo[i].frame;
			s->cur_trk = s->trkinfo[i].trkno;
			s->cur_idx = 1;

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				slioc_mute_on(s);

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

			if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
					  start_addr, s->discpos_tot.addr,
					  &start_msf, &end_msf, 0, 0)) {
				DO_BEEP();
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) slioc_pause_resume(FALSE);

				/* Restore volume */
				slioc_mute_off(s);
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

			if (i > 0 && s->trkinfo[i].trkno != CDROM_LEADOUT) {
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
 * slioc_previdx
 *	Previous index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_previdx(curstat_t *s)
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
			s->segplay = SEGP_NONE; /* Cancel a->b mode */
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
		slioc_mute_on(s);

		if (!slioc_do_playaudio(ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk, idx)) {
			/* Restore volume */
			slioc_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		slioc_idx_pause = TRUE;

		if (!slioc_pause_resume(FALSE)) {
			/* Restore volume */
			slioc_mute_off(s);
			slioc_idx_pause = FALSE;
			return;
		}

		/* Use slioc_get_playstatus to update the current status */
		if (!slioc_get_playstatus(s)) {
			/* Restore volume */
			slioc_mute_off(s);
			slioc_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			slioc_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			slioc_idx_pause = FALSE;
			return;
		}

		slioc_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) slioc_pause_resume(FALSE);

			/* Restore volume */
			slioc_mute_off(s);

			/* Force update of curstat */
			(void) slioc_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * slioc_nextidx
 *	Next index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_nextidx(curstat_t *s)
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
		slioc_mute_on(s);

		if (!slioc_do_playaudio(ADDR_TRKIDX, 0, 0, NULL, NULL,
				  (byte_t) s->cur_trk,
				  (byte_t) (s->cur_idx + 1))) {
			/* Restore volume */
			slioc_mute_off(s);
			DO_BEEP();
			return;
		}

		/* A small delay to make sure the command took effect */
		util_delayms(10);

		slioc_idx_pause = TRUE;

		if (!slioc_pause_resume(FALSE)) {
			/* Restore volume */
			slioc_mute_off(s);
			slioc_idx_pause = FALSE;
			return;
		}

		/* Use slioc_get_playstatus to update the current status */
		if (!slioc_get_playstatus(s)) {
			/* Restore volume */
			slioc_mute_off(s);
			slioc_idx_pause = FALSE;
			return;
		}

		/* Save starting block addr of this index */
		s->sav_iaddr = s->curpos_tot.addr;

		if (s->mode != MOD_PAUSE)
			/* Restore volume */
			slioc_mute_off(s);

		start_msf.min = s->curpos_tot.min;
		start_msf.sec = s->curpos_tot.sec;
		start_msf.frame = s->curpos_tot.frame;
		end_msf.min = s->discpos_tot.min;
		end_msf.sec = s->discpos_tot.sec;
		end_msf.frame = s->discpos_tot.frame;

		if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
				  s->curpos_tot.addr, s->discpos_tot.addr,
				  &start_msf, &end_msf, 0, 0)) {
			DO_BEEP();
			slioc_idx_pause = FALSE;
			return;
		}

		slioc_idx_pause = FALSE;

		if (s->mode == MOD_PAUSE) {
			(void) slioc_pause_resume(FALSE);

			/* Restore volume */
			slioc_mute_off(s);

			/* Force update of curstat */
			(void) slioc_get_playstatus(s);
		}

		break;

	default:
		DO_BEEP();
		break;
	}
}


/*
 * slioc_rew
 *	Search-rewind function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_rew(curstat_t *s, bool_t start)
{
	sword32_t	i;
	msf_t		start_msf,
			end_msf;
	byte_t		vol;

	switch (s->mode) {
	case MOD_SAMPLE:
		/* Go to normal play mode first */
		slioc_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				slioc_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) slioc_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE
				);
			}

			/* Start search rewind */
			slioc_start_search = TRUE;
			slioc_run_rew(s);
		}
		else {
			/* Button release */

			slioc_stop_rew(s);

			/* Update display */
			(void) slioc_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				slioc_mute_on(s);
			else
				/* Restore volume */
				slioc_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == CDROM_LEADOUT) {
					/* Restore volume */
					slioc_mute_off(s);
					return;
				}

				start_msf.min = s->curpos_tot.min;
				start_msf.sec = s->curpos_tot.sec;
				start_msf.frame = s->curpos_tot.frame;
				end_msf.min = s->trkinfo[i+1].min;
				end_msf.sec = s->trkinfo[i+1].sec;
				end_msf.frame = s->trkinfo[i+1].frame;

				if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  s->curpos_tot.addr,
						  s->trkinfo[i+1].addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					slioc_mute_off(s);
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

				if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  s->curpos_tot.addr,
						  s->discpos_tot.addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					slioc_mute_off(s);
					return;
				}
			}

			if (s->mode == MOD_PAUSE) {
				(void) slioc_pause_resume(FALSE);

				/* Restore volume */
				slioc_mute_off(s);
			}
			else
				slioc_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * slioc_ff
 *	Search-fast-forward function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_ff(curstat_t *s, bool_t start)
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
		slioc_play_pause(s);

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_PAUSE:
		if (start) {
			/* Button press */

			if (s->mode == MOD_PLAY)
				slioc_stop_stat_poll();

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				/* Reduce volume */
				vol = (byte_t) ((int) s->level *
					app_data.skip_vol / 100);

				(void) slioc_cfg_vol((int)
					((vol < (byte_t)app_data.skip_minvol) ?
					 (byte_t) app_data.skip_minvol : vol),
					s,
					FALSE
				);
			}

			/* Start search forward */
			slioc_start_search = TRUE;
			slioc_run_ff(s);
		}
		else {
			/* Button release */

			slioc_stop_ff(s);

			/* Update display */
			(void) slioc_get_playstatus(s);

			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				slioc_mute_on(s);
			else
				/* Restore volume */
				slioc_mute_off(s);

			if (s->shuffle || s->program) {
				if ((i = di_curtrk_pos(s)) < 0 ||
				    s->trkinfo[i].trkno == CDROM_LEADOUT) {
					/* Restore volume */
					slioc_mute_off(s);
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

				if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  start_addr, end_addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					slioc_mute_off(s);
					return;
				}
			}
			else if (s->segplay == SEGP_AB &&
				 (s->curpos_tot.addr + app_data.min_playblks) >
					s->bp_endpos_tot.addr) {
				/* No more left to play */
				/* Restore volume */
				slioc_mute_off(s);
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

				if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						  start_addr, end_addr,
						  &start_msf, &end_msf,
						  0, 0)) {
					DO_BEEP();

					/* Restore volume */
					slioc_mute_off(s);
					return;
				}
			}
			if (s->mode == MOD_PAUSE) {
				(void) slioc_pause_resume(FALSE);

				/* Restore volume */
				slioc_mute_off(s);
			}
			else
				slioc_start_stat_poll(s);
		}
		break;

	default:
		if (start)
			DO_BEEP();
		break;
	}
}


/*
 * slioc_warp
 *	Track warp function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_warp(curstat_t *s)
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
		slioc_play_pause(s);

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
			if (!slioc_start_stop(FALSE, FALSE))
				DO_BEEP();
		}
		else {
			if (s->mode == MOD_PAUSE)
				/* Mute: so we don't get a transient */
				slioc_mute_on(s);

			if (!slioc_do_playaudio(ADDR_BLK | ADDR_MSF,
						start_addr, end_addr,
						&start_msf, &end_msf,
						0, 0)) {
				DO_BEEP();

				/* Restore volume */
				slioc_mute_off(s);
				return;
			}

			if (s->mode == MOD_PAUSE) {
				(void) slioc_pause_resume(FALSE);

				/* Restore volume */
				slioc_mute_off(s);
			}
		}
		break;

	default:
		break;
	}
}


/*
 * slioc_route
 *	Channel routing function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_route(curstat_t *s)
{
	if (!app_data.chroute_supp)
		return;

	/* Only CDDA mode supports channel routing on Linux/SunOS */
	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		(void) cdda_chroute(slioc_devp, s);
}


/*
 * slioc_mute_on
 *	Mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_mute_on(curstat_t *s)
{
	(void) slioc_cfg_vol(0, s, FALSE);
}


/*
 * slioc_mute_off
 *	Un-mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_mute_off(curstat_t *s)
{
	(void) slioc_cfg_vol((int) s->level, s, FALSE);
}


/*
 * slioc_cddajitter
 *	CDDA jitter correction setting change notification
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_cddajitter(curstat_t *s)
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
			slioc_stop_stat_poll();
			break;

		default:
			break;
		}

		/* Restart CDDA */
		cdda_halt(slioc_devp, s);
		util_delayms(1000);

		slioc_cdda_client.curstat_addr = di_clinfo->curstat_addr;
		slioc_cdda_client.fatal_msg = di_clinfo->fatal_msg;
		slioc_cdda_client.warning_msg = di_clinfo->warning_msg;
		slioc_cdda_client.info_msg = di_clinfo->info_msg;
		slioc_cdda_client.info2_msg = di_clinfo->info2_msg;

		(void) cdda_init(s, &slioc_cdda_client);

		/* Restore old state */
		switch (omode) {
		case MOD_PLAY:
		case MOD_SAMPLE:
			if (slioc_play_recov(curblk, FALSE))
				slioc_start_stat_poll(s);
			break;

		default:
			break;
		}

		s->mode = omode;
	}
}


/*
 * slioc_debug
 *	Debug level change notification
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
slioc_debug(void)
{
	if (PLAYMODE_IS_CDDA(app_data.play_mode))
		cdda_debug(app_data.debug);
}


/*
 * slioc_start
 *	Start the SunOS/Solaris/Linux ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_start(curstat_t *s)
{
	/* Check to see if disc is ready */
	if (di_clinfo->timeout != NULL)
		slioc_start_insert_poll(s);
	else
		(void) slioc_disc_ready(s);

	/* Update display */
	DPY_ALL(s);
}


/*
 * slioc_icon
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
slioc_icon(curstat_t *s, bool_t iconified)
{
	/* This function attempts to reduce the status polling frequency
	 * when possible to cut down on CPU and bus usage.  This is
	 * done when the CD player is iconified.
	 */

	/* Increase status polling interval by 4 seconds when iconified */
	if (iconified)
		slioc_stat_interval = app_data.stat_interval + 4000;
	else
		slioc_stat_interval = app_data.stat_interval;

	switch (s->mode) {
	case MOD_BUSY:
	case MOD_NODISC:
	case MOD_STOP:
	case MOD_PAUSE:
		break;

	case MOD_SAMPLE:
		/* No optimization in these modes */
		slioc_stat_interval = app_data.stat_interval;
		break;

	case MOD_PLAY:
		if (!iconified) {
			/* Force an immediate update */
			slioc_stop_stat_poll();
			slioc_start_stat_poll(s);
		}
		break;

	default:
		break;
	}
}


/*
 * slioc_halt
 *	Shut down the SunOS/Solaris/Linux ioctl method.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
slioc_halt(curstat_t *s)
{
	/* If playing CDDA, stop it */
	if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			(void) slioc_start_stop(FALSE, FALSE);
			s->mode = MOD_STOP;
			slioc_stop_stat_poll();
			break;
		default:
			break;
		}
	}

	/* Shut down CDDA */
	cdda_halt(slioc_devp, s);
	app_data.play_mode = PLAYMODE_STD;

	if (s->mode != MOD_BUSY && s->mode != MOD_NODISC) {
		if (app_data.exit_eject && app_data.eject_supp) {
			/* User closing application: Eject disc */
			(void) slioc_start_stop(FALSE, TRUE);
		}
		else {
			if (app_data.exit_stop)
				/* User closing application: Stop disc */
				(void) slioc_start_stop(FALSE, FALSE);

			switch (s->mode) {
			case MOD_PLAY:
			case MOD_PAUSE:
			case MOD_SAMPLE:
				slioc_stop_stat_poll();
				break;
			default:
				break;
			}
		}
	}
	
	if (slioc_devp != NULL) {
		slioc_close();
		slioc_not_open = TRUE;
	}
}


/*
 * slioc_methodstr
 *	Return a text string indicating the current method.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Method text string.
 */
char *
slioc_methodstr(void)
{
#ifdef _LINUX
	return ("Linux ioctl\n");
#endif
#if defined(_SUNOS4) || defined(_SOLARIS)
	return ("SunOS/Solaris ioctl\n");
#endif
#ifdef __QNX__
	return ("QNX ioctl\n");
#endif
}


#endif	/* DI_SLIOC DEMO_ONLY */

