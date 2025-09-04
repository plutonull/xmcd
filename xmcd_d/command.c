/*
 *   xmcd - Motif(R) CD Audio Player/Ripper
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
#ifndef lint
static char *_command_c_ident_ = "@(#)command.c	7.92 04/04/20";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdinfo_d/cdinfo.h"
#include "cdinfo_d/motd.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/dbprog.h"
#include "xmcd_d/command.h"


extern appdata_t	app_data;
extern widgets_t	widgets;
extern FILE		*errfp;
extern wlist_t		*cd_brhead,
			*cd_minbrhead,
			*cd_maxbrhead;

/* Time to wait for quit command (mS) */
#define QUIT_DELAY	1000

/* Log file banner */
#define RMTLOG_BANNER1	"# xmcd %s.%s Remote control log file\n# %s\n#\n"
#define RMTLOG_BANNER2	"# YY/MM/DD HH:MM:SS host/dev/pid/display \
user@host/pid command\n#\n"

#ifdef __VMS
/* VMS: the device and host names should be case-insensitive */
#define DEV_STRCMP	util_strcasecmp
#define HOST_STRCMP	util_strcasecmp
#else
/* UNIX */
#define DEV_STRCMP	strcmp
#define HOST_STRCMP	strcmp
#endif

/* X event macros */
#define EV_PROP_NOTIFY(e, w, s, a)		\
	((e)->xany.type == PropertyNotify &&	\
	 (e)->xproperty.window == (w) &&	\
	 (e)->xproperty.state == (s) &&		\
	 (e)->xproperty.atom == (a))
#define EV_DESTROY_WIN(e, w)			\
	((e)->xany.type == DestroyNotify &&	\
	 (e)->xdestroywindow.window == (w))

/* X property atoms */
STATIC Atom		cmd_ident;
STATIC Atom		cmd_att;
STATIC Atom		cmd_req;
STATIC Atom		cmd_ack;
STATIC char		cmd_attstr[MAX_ATT_LEN + 1];


/***********************
 *  internal routines  *
 ***********************/


/*
 * cmd_do_stop
 *	Function to execute the "stop" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_stop(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tstop\n");
		return CMD_STAT_SUCCESS;
	}

	XtCallCallbacks(
		widgets.main.stop_btn,
		XmNactivateCallback, (XtPointer) s
	);
	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_play
 *	Function to execute the "play" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_play(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	int		i;
	unsigned int	min,
			sec;
	sword32_t	trk,
			sav_cur_trk;
	sword32_t	addr,
			eot;

	if (prn_usage) {
		(void) fprintf(errfp,
			"\tplay [track# | min:sec | track#:min:sec]\n");
		return CMD_STAT_SUCCESS;
	}
	if (!di_check_disc(s))
		/* Cannot go to a track when the disc is not ready */
		return CMD_STAT_INVAL;

	if (arg != NULL) {
		trk = 0;
		min = sec = 0;

		if (sscanf(arg, "%u:%u:%u", &trk, &min, &sec) != 3) {
			trk = 0;
			min = sec = 0;

			if (sscanf(arg, "%u:%u", &min, &sec) != 2) {
				min = sec = 0;

				if (sscanf(arg, "%u", &trk) != 1) {
					/* Invalid syntax */
					return CMD_STAT_BADARG;
				}
			}
		}

		if (min > 60 || sec > 60)
			return CMD_STAT_BADARG;

		util_msftoblk(
			(byte_t) min,
			(byte_t) sec,
			0,
			&addr,
			0
		);

		if (trk == 0) {
			if (s->cur_trk < 0)
				trk = s->first_trk;
			else
				trk = s->cur_trk;
		}
		sav_cur_trk = s->cur_trk;
		s->cur_trk = trk;

		if ((i = di_curtrk_pos(s)) < 0) {
			s->cur_trk = sav_cur_trk;
			return CMD_STAT_BADARG;
		}

		eot = s->trkinfo[i+1].addr;

		/* "Enhanced CD" / "CD Extra" hack */
		if (eot > s->discpos_tot.addr)
			eot = s->discpos_tot.addr;

		if ((s->trkinfo[i].addr + addr) >= eot) {
			s->cur_trk = sav_cur_trk;
			return CMD_STAT_BADARG;
		}

		/* Note: This assumes that shuffle and program modes
		 * are mutually exclusive!
		 */
		if (s->shuffle) {
			/* Disable shuffle mode */
			XmToggleButtonSetState(
				widgets.main.shuffle_btn,
				False,
				True
			);
		}
		else if (s->program) {
			XtCallCallbacks(
				widgets.dbprog.clrpgm_btn,
				XmNactivateCallback, (XtPointer) s
			);
		}

		switch (s->mode) {
		case MOD_PAUSE:
		case MOD_PLAY:
		case MOD_SAMPLE:
			sav_cur_trk = s->cur_trk;

			/* Set play status to stop */
			di_stop(s, FALSE);

			/* Restore s->cur_trk because di_stop
			 * resets it
			 */
			s->cur_trk = sav_cur_trk;
			break;

		default:
			break;
		}

		s->curpos_trk.min = (byte_t) min;
		s->curpos_trk.sec = (byte_t) sec;
		s->curpos_trk.frame = 0;
		s->curpos_trk.addr = addr;
		s->curpos_tot.addr = s->trkinfo[i].addr + s->curpos_trk.addr;
		util_blktomsf(
			s->curpos_tot.addr,
			&s->curpos_tot.min,
			&s->curpos_tot.sec,
			&s->curpos_tot.frame,
			MSF_OFFSET
		);
	}

	XtCallCallbacks(
		widgets.main.playpause_btn,
		XmNactivateCallback, (XtPointer) s
	);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_pause
 *	Function to execute the "pause" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_pause(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tpause\n");
		return CMD_STAT_SUCCESS;
	}
	if (s->mode == MOD_PLAY || s->mode == MOD_PAUSE) {
		XtCallCallbacks(
			widgets.main.playpause_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_INVAL;
}


/*
 * cmd_do_sample
 *	Function to execute the "sample" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_sample(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tsample\n");
		return CMD_STAT_SUCCESS;
	}
	XtCallCallbacks(
		widgets.main.sample_btn,
		XmNactivateCallback, (XtPointer) s
	);
	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_disc
 *	Function to execute the "disc" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_disc(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	sword32_t	disc;

	if (prn_usage) {
		(void) fprintf(errfp,
			"\tdisc <load | eject | prev | next | disc#>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "load") == 0) {
		if (s->mode != MOD_NODISC)
			return CMD_STAT_INVAL;

		XtCallCallbacks(
			widgets.main.eject_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "eject") == 0) {
		if (s->mode == MOD_NODISC || s->mode == MOD_BUSY)
			return CMD_STAT_INVAL;

		XtCallCallbacks(
			widgets.main.eject_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "prev") == 0)
		disc = s->cur_disc - 1;
	else if (strcmp(arg, "next") == 0)
		disc = s->cur_disc + 1;
	else if ((disc = atoi(arg)) == 0)
		return CMD_STAT_BADARG;

	if (s->cur_disc == disc)
		/* Already there: nothing to do */
		return CMD_STAT_SUCCESS;

	if (disc < s->first_disc || disc > s->last_disc)
		return CMD_STAT_BADARG;

	if (s->program) {
		/* Disable program mode */
		XtCallCallbacks(
			widgets.dbprog.clrpgm_btn,
			XmNactivateCallback, (XtPointer) s
		);
	}

	s->prev_disc = s->cur_disc;
	s->cur_disc = disc;

	/* Do the disc change */
	di_chgdisc(s);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_track
 *	Function to execute the "track" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_track(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	int		i;
	sword32_t	trk,
			sav_cur_trk;
	bool_t		paused,
			playing;

	if (prn_usage) {
		(void) fprintf(errfp, "\ttrack <prev | next | track#>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "prev") == 0) {
		XtCallCallbacks(
			widgets.main.prevtrk_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "next") == 0) {
		XtCallCallbacks(
			widgets.main.nexttrk_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}
	if ((trk = atoi(arg)) == 0)
		return CMD_STAT_BADARG;

	sav_cur_trk = s->cur_trk;
	s->cur_trk = trk;

	if ((i = di_curtrk_pos(s)) < 0) {
		s->cur_trk = sav_cur_trk;
		return CMD_STAT_BADARG;
	}

	/* Note: This assumes that shuffle and program modes
	 * are mutually exclusive!
	 */
	if (s->shuffle) {
		/* Disable shuffle mode */
		XmToggleButtonSetState(
			widgets.main.shuffle_btn,
			False,
			True
		);
	}
	else if (s->program) {
		XtCallCallbacks(
			widgets.dbprog.clrpgm_btn,
			XmNactivateCallback, (XtPointer) s
		);
	}

	playing = paused = FALSE;

	switch (s->mode) {
	case MOD_PAUSE:
		/* Mute sound */
		di_mute_on(s);
		paused = TRUE;

		/*FALLTHROUGH*/
	case MOD_PLAY:
	case MOD_SAMPLE:
		playing = TRUE;
		sav_cur_trk = s->cur_trk;

		/* Set play status to stop */
		di_stop(s, FALSE);

		/* Restore s->cur_trk because di_stop
		 * resets it
		 */
		s->cur_trk = sav_cur_trk;

		break;

	default:
		break;
	}

	s->segplay = SEGP_NONE;	/* Cancel a->b mode */
	dpy_progmode(s, FALSE);

	if (playing) {
		s->curpos_trk.min = 0;
		s->curpos_trk.sec = 0;
		s->curpos_trk.frame = 0;
		s->curpos_trk.addr = 0;
		s->curpos_tot.min = s->trkinfo[i].min;
		s->curpos_tot.sec = s->trkinfo[i].sec;
		s->curpos_tot.frame = s->trkinfo[i].frame;;
		s->curpos_tot.addr = 0;
		util_msftoblk(
			s->curpos_tot.min,
			s->curpos_tot.sec,
			s->curpos_tot.frame,
			&s->curpos_tot.addr,
			MSF_OFFSET
		);

		/* Start playback at new track */
		XtCallCallbacks(
			widgets.main.playpause_btn,
			XmNactivateCallback, (XtPointer) s
		);
	}
	else
		dpy_track(s);

	if (paused) {
		/* This will cause the playback to pause */
		XtCallCallbacks(
			widgets.main.playpause_btn,
			XmNactivateCallback, (XtPointer) s
		);

		/* Restore sound */
		di_mute_off(s);
	}

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_index
 *	Function to execute the "index" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_index(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tindex <prev | next>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "prev") == 0) {
		XtCallCallbacks(
			widgets.main.previdx_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "next") == 0) {
		XtCallCallbacks(
			widgets.main.nextidx_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_lock
 *	Function to execute the "lock" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_lock(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tlock <on | off>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "on") == 0) {
		XmToggleButtonSetState(
			widgets.main.lock_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	else if (strcmp(arg, "off") == 0) {
		XmToggleButtonSetState(
			widgets.main.lock_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_shuffle
 *	Function to execute the "shuffle" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_shuffle(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tshuffle <on | off>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "on") == 0) {
		switch (s->mode) {
		case MOD_PAUSE:
		case MOD_PLAY:
		case MOD_SAMPLE:
			/* Disallow enabling shuffle while playing */
			return CMD_STAT_INVAL;

		default:
			break;
		}

		if (s->program) {
			/* Disable program mode */
			XtCallCallbacks(
				widgets.dbprog.clrpgm_btn,
				XmNactivateCallback, (XtPointer) s
			);
		}
		if (s->segplay != SEGP_NONE) {
			s->segplay = SEGP_NONE;
			dpy_progmode(s, FALSE);
		}

		XmToggleButtonSetState(
			widgets.main.shuffle_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "off") == 0) {
		XmToggleButtonSetState(
			widgets.main.shuffle_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_repeat
 *	Function to execute the "repeat" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_repeat(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\trepeat <on | off>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "on") == 0) {
		XmToggleButtonSetState(
			widgets.main.repeat_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "off") == 0) {
		XmToggleButtonSetState(
			widgets.main.repeat_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_program
 *	Function to execute the "program" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_program(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp,
			"\tprogram <clear | save | track# ...>\n");
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "clear") == 0) {
		if (s->program) {
			XtCallCallbacks(
				widgets.dbprog.clrpgm_btn,
				XmNactivateCallback, (XtPointer) s
			);
		}
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "save") == 0) {
		if (s->program) {
			XtCallCallbacks(
				widgets.dbprog.savepgm_btn,
				XmNactivateCallback, (XtPointer) s
			);
		}
		return CMD_STAT_SUCCESS;
	}

	if (s->mode != MOD_NODISC && s->mode != MOD_BUSY) {
		if (s->program) {
			/* Clear program first */
			XtCallCallbacks(
				widgets.dbprog.clrpgm_btn,
				XmNactivateCallback, (XtPointer) s
			);
		}

		/* Set specified program */
		set_text_string(widgets.dbprog.pgmseq_txt, arg, FALSE);

		return CMD_STAT_SUCCESS;
	}

	return CMD_STAT_INVAL;
}


/*
 * cmd_do_volume
 *	Function to execute the "volume" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_volume(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	int	val;

	if (prn_usage) {
		(void) fprintf(errfp, "\tvolume %s  %s\n",
			"<value# | linear | square | invsqr>",
			"(value 0-100)");
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "linear") == 0) {
		XmToggleButtonSetState(
			widgets.options.vol_linear_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "square") == 0) {
		XmToggleButtonSetState(
			widgets.options.vol_square_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "invsqr") == 0) {
		XmToggleButtonSetState(
			widgets.options.vol_invsqr_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}

	if (!isdigit((int) arg[0]))
		return CMD_STAT_BADARG;

	val = atoi(arg);
	if (val < 0 || val > 100)
		return CMD_STAT_BADARG;

	set_vol_slider(val);
	di_level(s, (byte_t) val, FALSE);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_balance
 *	Function to execute the "balance" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_balance(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	int	val;

	if (prn_usage) {
		(void) fprintf(errfp, "\tbalance %s\t\t\t    %s\n",
		    "<value#>", "(value 0-100, center:50)");
		return CMD_STAT_SUCCESS;
	}

	if (!isdigit((int) arg[0]))
		return CMD_STAT_BADARG;

	val = atoi(arg);
	if (val < 0 || val > 100)
		return CMD_STAT_BADARG;

	if (val == 50) {
		/* Center setting */
		s->level_left = s->level_right = 100;
	}
	else if (val < 50) {
		/* Attenuate the right channel */
		s->level_left = 100;
		s->level_right = 100 - ((50 - val) * 2);
	}
	else {
		/* Attenuate the left channel */
		s->level_left = 100 - ((val - 50) * 2);
		s->level_right = 100;
	}

	set_bal_slider(val - 50);
	di_level(s, (byte_t) s->level, FALSE);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_route
 *	Function to execute the "route" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_route(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\troute %s\n",
		    "<stereo | reverse | mono-l | mono-r | mono | value#>");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "stereo") == 0 || strcmp(arg, "0") == 0) {
		XmToggleButtonSetState(
			widgets.options.chroute_stereo_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "reverse") == 0 || strcmp(arg, "1") == 0) {
		XmToggleButtonSetState(
			widgets.options.chroute_rev_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "mono-l") == 0 || strcmp(arg, "2") == 0) {
		XmToggleButtonSetState(
			widgets.options.chroute_left_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "mono-r") == 0 || strcmp(arg, "3") == 0) {
		XmToggleButtonSetState(
			widgets.options.chroute_right_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "mono") == 0 || strcmp(arg, "4") == 0) {
		XmToggleButtonSetState(
			widgets.options.chroute_mono_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_outport
 *	Function to execute the "outport" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_outport(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	word32_t	outport;

	if (prn_usage) {
		(void) fprintf(errfp, "\toutport %s\n",
		    "<speaker | headphone | line-out | value#>");
		return CMD_STAT_SUCCESS;
	}

	if (isdigit((int) arg[0])) {
		outport = (word32_t) atoi(arg);
		if (app_data.outport == outport)
			/* No change */
			return CMD_STAT_SUCCESS;

		XmToggleButtonSetState(
			widgets.options.outport_spkr_btn,
			(Boolean) (outport & CDDA_OUT_SPEAKER),
			True
		);
		XmToggleButtonSetState(
			widgets.options.outport_phone_btn,
			(Boolean) (outport & CDDA_OUT_HEADPHONE),
			True
		);
		XmToggleButtonSetState(
			widgets.options.outport_line_btn,
			(Boolean) (outport & CDDA_OUT_LINEOUT),
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "speaker") == 0) {
		XmToggleButtonSetState(
			widgets.options.outport_spkr_btn,
			(Boolean)
			    ((app_data.outport & CDDA_OUT_SPEAKER) == 0),
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "headphone") == 0) {
		XmToggleButtonSetState(
			widgets.options.outport_phone_btn,
			(Boolean)
			    ((app_data.outport & CDDA_OUT_HEADPHONE) == 0),
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "line-out") == 0) {
		XmToggleButtonSetState(
			widgets.options.outport_line_btn,
			(Boolean)
			    ((app_data.outport & CDDA_OUT_LINEOUT) == 0),
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_cdda_att
 *	Function to execute the "cdda-att" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_cdda_att(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	int	val;

	if (prn_usage) {
		(void) fprintf(errfp, "\tcdda-att %s\t\t\t    %s\n",
				"<value#>", "(value 0-100)");
		return CMD_STAT_SUCCESS;
	}

	if (!isdigit((int) arg[0]))
		return CMD_STAT_BADARG;

	val = atoi(arg);
	if (val < 0 || val > 100)
		return CMD_STAT_BADARG;

	set_att_slider(val);
	s->cdda_att = (byte_t) val;
	cdda_att(s);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_time
 *	Function to execute the "time" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_time(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\ttime %s\n",
		    "<elapse | e-seg | e-disc | r-trac | r-seg | r-disc>");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "elapse") == 0)
		s->time_dpy = T_ELAPSED_TRACK;
	else if (strcmp(arg, "e-seg") == 0)
		s->time_dpy = T_ELAPSED_SEG;
	else if (strcmp(arg, "e-disc") == 0)
		s->time_dpy = T_ELAPSED_DISC;
	else if (strcmp(arg, "r-track") == 0)
		s->time_dpy = T_REMAIN_TRACK;
	else if (strcmp(arg, "r-seg") == 0)
		s->time_dpy = T_REMAIN_SEG;
	else if (strcmp(arg, "r-disc") == 0)
		s->time_dpy = T_REMAIN_DISC;
	else
		return CMD_STAT_BADARG;

	dpy_timemode(s);
	dpy_track(s);
	dpy_time(s, FALSE);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_onload
 *	Function to execute the "onload" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_onload(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\ton-load %s\n",
		    "<none | spindown | autoplay | autolock | noautolock>");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "none") == 0) {
		XmToggleButtonSetState(
			widgets.options.load_none_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "spindown") == 0) {
		XmToggleButtonSetState(
			widgets.options.load_spdn_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "autoplay") == 0) {
		XmToggleButtonSetState(
			widgets.options.load_play_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "autolock") == 0) {
		XmToggleButtonSetState(
			widgets.options.load_lock_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "noautolock") == 0) {
		XmToggleButtonSetState(
			widgets.options.load_lock_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_onexit
 *	Function to execute the "onexit" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_onexit(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp,
			"\ton-exit <none | autostop | autoeject>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "none") == 0) {
		XmToggleButtonSetState(
			widgets.options.exit_none_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "autostop") == 0) {
		XmToggleButtonSetState(
			widgets.options.exit_stop_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "autoeject") == 0) {
		XmToggleButtonSetState(
			widgets.options.exit_eject_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_ondone
 *	Function to execute the "on-done" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_ondone(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\ton-done %s\n",
			"<autoeject | noautoeject | autoexit | noautoexit>");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "autoeject") == 0) {
		XmToggleButtonSetState(
			widgets.options.done_eject_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "noautoeject") == 0) {
		XmToggleButtonSetState(
			widgets.options.done_eject_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "autoexit") == 0) {
		XmToggleButtonSetState(
			widgets.options.done_exit_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "noautoexit") == 0) {
		XmToggleButtonSetState(
			widgets.options.done_exit_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_oneject
 *	Function to execute the "on-eject" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_oneject(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\ton-eject <autoexit | noautoexit>\n");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "autoexit") == 0) {
		XmToggleButtonSetState(
			widgets.options.eject_exit_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "noautoexit") == 0) {
		XmToggleButtonSetState(
			widgets.options.eject_exit_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_changer
 *	Function to execute the "changer" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_changer(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tchanger %s\n",
			"<multiplay | nomultiplay | reverse | noreverse>");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "multiplay") == 0) {
		XmToggleButtonSetState(
			widgets.options.chg_multiplay_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "nomultiplay") == 0) {
		XmToggleButtonSetState(
			widgets.options.chg_multiplay_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "reverse") == 0) {
		XmToggleButtonSetState(
			widgets.options.chg_reverse_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "noreverse") == 0) {
		XmToggleButtonSetState(
			widgets.options.chg_reverse_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_mode
 *	Function to execute the "mode" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_mode(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tmode %s\n",
			"<standard | cdda-play | cdda-save | cdda-pipe>");
		return CMD_STAT_SUCCESS;
	}

	switch (s->mode) {
	case MOD_PLAY:
	case MOD_PAUSE:
	case MOD_SAMPLE:
		/* Disallow changing the mode while playing */
		return CMD_STAT_INVAL;

	default:
		break;
	}

	if (strcmp(arg, "standard") == 0) {
		XmToggleButtonSetState(
			widgets.options.mode_std_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	else if (strcmp(arg, "cdda-play") == 0 &&
		 XtIsSensitive(widgets.options.mode_cdda_btn)) {
		XmToggleButtonSetState(
			widgets.options.mode_cdda_btn,
			(Boolean)
			!XmToggleButtonGetState(widgets.options.mode_cdda_btn),
			True
		);
		return CMD_STAT_SUCCESS;
	}
	else if (strcmp(arg, "cdda-save") == 0 &&
		 XtIsSensitive(widgets.options.mode_file_btn)) {
		XmToggleButtonSetState(
			widgets.options.mode_file_btn,
			(Boolean)
			!XmToggleButtonGetState(widgets.options.mode_file_btn),
			True
		);
		return CMD_STAT_SUCCESS;
	}
	else if (strcmp(arg, "cdda-pipe") == 0 &&
		 XtIsSensitive(widgets.options.mode_pipe_btn)) {
		XmToggleButtonSetState(
			widgets.options.mode_pipe_btn,
			(Boolean)
			!XmToggleButtonGetState(widgets.options.mode_pipe_btn),
			True
		);
		return CMD_STAT_SUCCESS;
	}

	return CMD_STAT_BADARG;
}


/*
 * cmd_do_jitter
 *	Function to execute the "jittercorr" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_jitter(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tjittercorr %s\n",
			"<on | off>");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.mode_jitter_btn))
		return CMD_STAT_INVAL;

	if (strcmp(arg, "on") == 0) {
		XmToggleButtonSetState(
			widgets.options.mode_jitter_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "off") == 0) {
		XmToggleButtonSetState(
			widgets.options.mode_jitter_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_trkfile
 *	Function to execute the "trackfile" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_trkfile(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\ttrackfile %s\n",
			"<on | off>");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.mode_trkfile_btn))
		return CMD_STAT_INVAL;

	if (strcmp(arg, "on") == 0) {
		XmToggleButtonSetState(
			widgets.options.mode_trkfile_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "off") == 0) {
		XmToggleButtonSetState(
			widgets.options.mode_trkfile_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_subst
 *	Function to execute the "subst" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_subst(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tsubst %s\n",
			"<on | off>");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.mode_subst_btn))
		return CMD_STAT_INVAL;

	if (strcmp(arg, "on") == 0) {
		XmToggleButtonSetState(
			widgets.options.mode_subst_btn,
			True,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "off") == 0) {
		XmToggleButtonSetState(
			widgets.options.mode_subst_btn,
			False,
			True
		);
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_filefmt
 *	Function to execute the "filefmt" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_filefmt(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	int	newfmt;
	Widget	menuw;

	if (prn_usage) {
		(void) fprintf(errfp, "\tfilefmt %s\n",
			"<raw | au | wav | aiff | aiff-c | "
			"mp3 | ogg | flac | aac | mp4>");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.mode_fmt_opt))
		return CMD_STAT_INVAL;

	if (strcmp(arg, "raw") == 0) {
		menuw = widgets.options.mode_fmt_raw_btn;
		newfmt = FILEFMT_RAW;
	}
	else if (strcmp(arg, "au") == 0) {
		menuw = widgets.options.mode_fmt_au_btn;
		newfmt = FILEFMT_AU;
	}
	else if (util_strcasecmp(arg, "wav") == 0 ||
		 util_strcasecmp(arg, "wave") == 0) {
		menuw = widgets.options.mode_fmt_wav_btn;
		newfmt = FILEFMT_WAV;
	}
	else if (util_strcasecmp(arg, "aiff") == 0) {
		menuw = widgets.options.mode_fmt_aiff_btn;
		newfmt = FILEFMT_AIFF;
	}
	else if (util_strcasecmp(arg, "aiff-c") == 0 ||
		 util_strcasecmp(arg, "aifc") == 0) {
		menuw = widgets.options.mode_fmt_aifc_btn;
		newfmt = FILEFMT_AIFC;
	}
	else if (util_strcasecmp(arg, "mp3") == 0) {
		menuw = widgets.options.mode_fmt_mp3_btn;
		newfmt = FILEFMT_MP3;
	}
	else if (util_strcasecmp(arg, "ogg") == 0 ||
		 util_strcasecmp(arg, "vorbis") == 0) {
		menuw = widgets.options.mode_fmt_ogg_btn;
		newfmt = FILEFMT_OGG;
	}
	else if (util_strcasecmp(arg, "flac") == 0) {
		menuw = widgets.options.mode_fmt_flac_btn;
		newfmt = FILEFMT_FLAC;
	}
	else if (util_strcasecmp(arg, "aac") == 0) {
		menuw = widgets.options.mode_fmt_aac_btn;
		newfmt = FILEFMT_AAC;
	}
	else if (util_strcasecmp(arg, "mp4") == 0) {
		menuw = widgets.options.mode_fmt_mp4_btn;
		newfmt = FILEFMT_MP4;
	}
	else
		return CMD_STAT_BADARG;

	if (!cdda_filefmt_supp(newfmt)) {
		DBGPRN(DBG_GEN)(errfp, "Error: %s %d\n%s\n",
				"Support for the requested file format ",
				newfmt,
				"is not compiled-in.");
		return CMD_STAT_BADARG;
	}

	app_data.cdda_filefmt = newfmt;

	/* Fix output file suffix to match the file type */
	fix_outfile_path(s);

	XtVaSetValues(widgets.options.mode_fmt_opt,
		XmNmenuHistory, menuw,
		NULL
	);

	set_text_string(widgets.options.mode_path_txt, s->outf_tmpl, TRUE);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_outfile
 *	Function to execute the "outfile" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_outfile(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	int		i;
	filefmt_t	*fmp,
			*fmp2;
	char		*cp,
			*cp2,
			*cp3,
			*suf;
	int		newfmt;
	Widget		menuw;

	if (prn_usage) {
		(void) fprintf(errfp, "\toutfile <\"template\">\n");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.mode_path_txt))
		return CMD_STAT_INVAL;

	if (s->outf_tmpl != NULL && strcmp(arg, s->outf_tmpl) == 0)
		/* Not changed */
		return CMD_STAT_SUCCESS;

	/* File suffix */
	if ((fmp = cdda_filefmt(app_data.cdda_filefmt)) == NULL)
		suf = "";
	else
		suf = fmp->suf;

	/* Check if file suffix indicates a change in file format */
	newfmt = app_data.cdda_filefmt;
	menuw = (Widget) NULL;
	cp = arg;
	if ((cp2 = strrchr(cp, '.')) != NULL) {
		if ((cp3 = strrchr(cp, DIR_END)) != NULL && cp2 > cp3) {
			if (strcmp(suf, cp2) != 0) {
			    for (i = 0; i < MAX_FILEFMTS; i++) {
				if ((fmp2 = cdda_filefmt(i)) == NULL)
				    continue;

				if (util_strcasecmp(cp2, fmp2->suf) == 0)
				    newfmt = fmp2->fmt;
			    }
			}
		}
		else if (strcmp(suf, cp2) != 0) {
			for (i = 0; i < MAX_FILEFMTS; i++) {
			    if ((fmp2 = cdda_filefmt(i)) == NULL)
				continue;

			    if (util_strcasecmp(cp2, fmp2->suf) == 0)
				newfmt = fmp2->fmt;
			}
		}

		switch (newfmt) {
		case FILEFMT_RAW:
			menuw = widgets.options.mode_fmt_raw_btn;
			break;
		case FILEFMT_AU:
			menuw = widgets.options.mode_fmt_au_btn;
			break;
		case FILEFMT_WAV:
			menuw = widgets.options.mode_fmt_wav_btn;
			break;
		case FILEFMT_AIFF:
			menuw = widgets.options.mode_fmt_aiff_btn;
			break;
		case FILEFMT_AIFC:
			menuw = widgets.options.mode_fmt_aifc_btn;
			break;
		case FILEFMT_MP3:
			menuw = widgets.options.mode_fmt_mp3_btn;
			break;
		case FILEFMT_OGG:
			menuw = widgets.options.mode_fmt_ogg_btn;
			break;
		default:
			break;
		}
	}

	if (!util_newstr(&s->outf_tmpl, cp))
		return CMD_STAT_INVAL;

	app_data.cdda_filefmt = newfmt;

	/* Fix output file suffix to match the file type */
	fix_outfile_path(s);

	/* File format changed */
	if (menuw != (Widget) NULL && newfmt != app_data.cdda_filefmt) {
		XtVaSetValues(widgets.options.mode_fmt_opt,
			XmNmenuHistory, menuw,
			NULL
		);
	}

	if (util_strstr(s->outf_tmpl, "%T") != NULL ||
	    util_strstr(s->outf_tmpl, "%t") != NULL ||
	    util_strstr(s->outf_tmpl, "%R") != NULL ||
	    util_strstr(s->outf_tmpl, "%r") != NULL ||
	    util_strstr(s->outf_tmpl, "%#") != NULL) {
		if (!app_data.cdda_trkfile) {
			XmToggleButtonSetState(
				widgets.options.mode_trkfile_btn,
				True,
				False
			);
			app_data.cdda_trkfile = TRUE;
		}
	}
	else if (app_data.cdda_trkfile) {
		XmToggleButtonSetState(
			widgets.options.mode_trkfile_btn,
			False,
			False
		);
		app_data.cdda_trkfile = FALSE;
	}

	set_text_string(widgets.options.mode_path_txt, s->outf_tmpl, TRUE);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_pipeprog
 *	Function to execute the "pipeprog" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_pipeprog(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tpipeprog <\"path [arg ...]\">\n");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.mode_prog_txt))
		return CMD_STAT_INVAL;

	set_text_string(widgets.options.mode_prog_txt, arg, TRUE);
	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_compress
 *	Function to execute the "compress" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_compress(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	char	*arg1;
	int	mode,
		m,
		n;

	if (prn_usage) {
		(void) fprintf(errfp,
			"\tcompress <<0 | 3> [bitrate#] | "
			"<1 | 2> [qual#]>\n");
		return CMD_STAT_SUCCESS;
	}

	if (!isdigit((int) arg[0]))
		return CMD_STAT_BADARG;

	mode = atoi(arg);

	if (mode < 0 || mode > 3)
		return CMD_STAT_BADARG;

	if (mode == 0 || mode == 3) {
		wlist_t	*l = NULL;
		Widget	menuw = (Widget) NULL;

		/* Skip to next arg */
		if ((arg1 = strchr(arg, ' ')) != NULL && *++arg1 != '\0') {
			if (strcmp(arg1, "0") == 0) {
				menuw = widgets.options.bitrate_def_btn;
			}
			else for (l = cd_brhead; l != NULL; l = l->next) {
				if (l->w != (Widget) NULL &&
				    strcmp(arg1, XtName(l->w)) == 0) {
					menuw = l->w;
					break;
				}
			}
			if (menuw == NULL)
				return CMD_STAT_BADARG;
		}

		XtVaSetValues(widgets.options.method_opt,
			XmNmenuHistory,
			(mode == 0) ?
			    widgets.options.mode0_btn :
			    widgets.options.mode3_btn,
			NULL
		);
		app_data.comp_mode = mode;

		if (menuw != NULL) {
			XtVaSetValues(widgets.options.bitrate_opt,
				XmNmenuHistory, menuw,
				NULL
			);
			app_data.bitrate = atoi(arg1);
		}

		if (app_data.bitrate == 0) {
			/* Bitrate has been set to 0, set the min and max
			 * bitrates to 0 too.
			 */
			if (app_data.bitrate_min > 0) {
				XtVaSetValues(widgets.options.minbrate_opt,
					XmNmenuHistory,
					widgets.options.minbrate_def_btn,
					NULL
				);
				app_data.bitrate_min = 0;
			}
			if (app_data.bitrate_max > 0) {
				XtVaSetValues(widgets.options.maxbrate_opt,
					XmNmenuHistory,
					widgets.options.maxbrate_def_btn,
					NULL
				);
				app_data.bitrate_max = 0;
			}
		}
		else {
			/* Adjust min and max bitrates if necessary */
			n = app_data.bitrate;
			if (app_data.bitrate_min > 0 &&
			    n <= app_data.bitrate_min) {
				for (l = cd_minbrhead; l != NULL; l = l->next) {
					if (l->w != (Widget) NULL) {
					    m = atoi(XtName(l->w));
					    if (m < n) {
						menuw = l->w;
						n = m;
					    }
					}
				}
				if (menuw != NULL) {
					n = atoi(XtName(menuw));
					if (n != app_data.bitrate_min) {
					    XtVaSetValues(
						widgets.options.minbrate_opt,
						XmNmenuHistory, menuw,
						NULL
					    );
					    app_data.bitrate_min = n;
					}
				}
			}

			n = app_data.bitrate;
			if (app_data.bitrate_max > 0 &&
			    n >= app_data.bitrate_max) {
				for (l = cd_maxbrhead; l != NULL; l = l->next) {
					if (l->w != (Widget) NULL) {
					    m = atoi(XtName(l->w));
					    if (m > n) {
						menuw = l->w;
						n = m;
					    }
					}
				}
				if (menuw != NULL) {
					n = atoi(XtName(menuw));
					if (n != app_data.bitrate_max) {
					    XtVaSetValues(
						widgets.options.maxbrate_opt,
						XmNmenuHistory, menuw,
						NULL
					    );
					    app_data.bitrate_max = n;
					}
				}
			}
		}

		XtSetSensitive(widgets.options.qualval_lbl, False);
		XtSetSensitive(widgets.options.qualval1_lbl, False);
		XtSetSensitive(widgets.options.qualval2_lbl, False);
		XtSetSensitive(widgets.options.qualval_scl, False);
		XtSetSensitive(widgets.options.bitrate_opt, True);
		XtSetSensitive(widgets.options.minbrate_opt,
				(Boolean) (mode == 3));
		XtSetSensitive(widgets.options.maxbrate_opt,
				(Boolean) (mode == 3));
	}
	else if (mode == 1 || mode == 2) {
		int	qual = 0;

		/* Skip to next arg */
		if ((arg1 = strchr(arg, ' ')) != NULL && *++arg1 != '\0') {
			qual = atoi(arg1);
			if (qual <= 0 || qual > 10)
				return CMD_STAT_BADARG;
		}

		XtVaSetValues(widgets.options.method_opt,
			XmNmenuHistory,
			(mode == 2) ?
			    widgets.options.mode2_btn :
			    widgets.options.mode1_btn,
			NULL
		);
		app_data.comp_mode = mode;

		if (qual > 0) {
			set_qualval_slider(qual);
			app_data.qual_factor = qual;
		}

		XtSetSensitive(widgets.options.qualval_lbl, True);
		XtSetSensitive(widgets.options.qualval1_lbl, True);
		XtSetSensitive(widgets.options.qualval2_lbl, True);
		XtSetSensitive(widgets.options.qualval_scl, True);
		XtSetSensitive(widgets.options.bitrate_opt, False);
		XtSetSensitive(widgets.options.minbrate_opt, True);
		XtSetSensitive(widgets.options.maxbrate_opt, True);
	}
	else
		return CMD_STAT_BADARG;

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_minbrate
 *	Function to execute the "min-bitrate" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_minbrate(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	wlist_t	*l;
	Widget	menuw = (Widget) NULL;
	int	n;

	if (prn_usage) {
		(void) fprintf(errfp, "\tmin-brate <bitrate#>\n");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.minbrate_opt))
		return CMD_STAT_INVAL;

	n = atoi(arg);

	if (n == 0) {
		menuw = widgets.options.minbrate_def_btn;
	}
	else for (l = cd_minbrhead; l != NULL; l = l->next) {
		if (l->w != (Widget) NULL && n == atoi(XtName(l->w))) {
			menuw = l->w;
			break;
		}
	}

	if (menuw == NULL)
		return CMD_STAT_BADARG;

	if (n > 0 && app_data.bitrate > 0 && n > app_data.bitrate)
		return CMD_STAT_BADARG;

	if (n > 0 && app_data.bitrate_max > 0 && n > app_data.bitrate_max)
		return CMD_STAT_BADARG;

	XtVaSetValues(widgets.options.minbrate_opt,
		XmNmenuHistory, menuw,
		NULL
	);
	app_data.bitrate_min = n;

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_maxbrate
 *	Function to execute the "max-bitrate" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_maxbrate(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	wlist_t	*l;
	Widget	menuw = (Widget) NULL;
	int	n;

	if (prn_usage) {
		(void) fprintf(errfp, "\tmax-brate <bitrate#>\n");
		return CMD_STAT_SUCCESS;
	}

	if (!XtIsSensitive(widgets.options.maxbrate_opt))
		return CMD_STAT_INVAL;

	n = atoi(arg);

	if (n == 0) {
		menuw = widgets.options.maxbrate_def_btn;
	}
	else for (l = cd_maxbrhead; l != NULL; l = l->next) {
		if (l->w != (Widget) NULL && n == atoi(XtName(l->w))) {
			menuw = l->w;
			break;
		}
	}

	if (menuw == NULL)
		return CMD_STAT_BADARG;

	if (n > 0 && app_data.bitrate > 0 && n < app_data.bitrate)
		return CMD_STAT_BADARG;

	if (n > 0 && app_data.bitrate_min > 0 && n < app_data.bitrate_min)
		return CMD_STAT_BADARG;

	XtVaSetValues(widgets.options.maxbrate_opt,
		XmNmenuHistory, menuw,
		NULL
	);
	app_data.bitrate_max = n;

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_coding
 *	Function to execute the "coding" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_coding(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp,
		    "\tcoding <stereo | j-stereo | force-ms | mono | algo#>\n");
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "stereo") == 0) {
		XtVaSetValues(widgets.options.chmode_opt,
			XmNmenuHistory, widgets.options.chmode_stereo_btn,
			NULL
		);
		app_data.chan_mode = CH_STEREO;
	}
	else if (strcmp(arg, "j-stereo") == 0) {
		XtVaSetValues(widgets.options.chmode_opt,
			XmNmenuHistory, widgets.options.chmode_jstereo_btn,
			NULL
		);
		app_data.chan_mode = CH_JSTEREO;
	}
	else if (strcmp(arg, "force-ms") == 0) {
		XtVaSetValues(widgets.options.chmode_opt,
			XmNmenuHistory, widgets.options.chmode_forcems_btn,
			NULL
		);
		app_data.chan_mode = CH_FORCEMS;
	}
	else if (strcmp(arg, "mono") == 0) {
		XtVaSetValues(widgets.options.chmode_opt,
			XmNmenuHistory, widgets.options.chmode_mono_btn,
			NULL
		);
		app_data.chan_mode = CH_MONO;
	}
	else {
		int	algo = atoi(arg);

		if (algo <= 0 || algo > 10)
			return CMD_STAT_BADARG;

		set_algo_slider(algo);
		app_data.comp_algo = algo;
	}

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_lowpass
 *	Function to execute the "lowpass" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_lowpass(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	Widget	menuw = (Widget) NULL;
	int	newmode,
		m,
		n;
	char	*arg1,
		str[16];

	if (prn_usage) {
		(void) fprintf(errfp,
			    "\tlowpass <off | auto | freq# [width#]>\n");
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "off") == 0) {
		menuw = widgets.options.lp_off_btn;
		newmode = FILTER_OFF;
	}
	else if (strcmp(arg, "auto") == 0) {
		menuw = widgets.options.lp_auto_btn;
		newmode = FILTER_AUTO;
	}
	else {
		menuw = widgets.options.lp_manual_btn;
		newmode = FILTER_MANUAL;

		m = atoi(arg);
		if (m < MIN_LOWPASS_FREQ || m > MAX_LOWPASS_FREQ)
			return CMD_STAT_BADARG;

		/* Skip to next arg */
		if ((arg1 = strchr(arg, ' ')) != NULL && *++arg1 != '\0') {
			if ((n = atoi(arg1)) < 0)
				return CMD_STAT_BADARG;

			(void) sprintf(str, "%d", n);
			set_text_string(
				widgets.options.lp_width_txt, str, FALSE
			);
		}

		(void) sprintf(str, "%d", m);
		set_text_string(widgets.options.lp_freq_txt, str, FALSE);
	}

	XtVaSetValues(widgets.options.lp_opt, XmNmenuHistory, menuw, NULL);
	app_data.lowpass_mode = newmode;

	XtSetSensitive(
		widgets.options.lp_freq_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);
	XtSetSensitive(
		widgets.options.lp_width_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_highpass
 *	Function to execute the "highpass" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_highpass(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	Widget	menuw = (Widget) NULL;
	int	newmode,
		m,
		n;
	char	*arg1,
		str[16];

	if (prn_usage) {
		(void) fprintf(errfp,
			    "\thighpass <off | auto | freq# [width#]>\n");
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "off") == 0) {
		menuw = widgets.options.hp_off_btn;
		newmode = FILTER_OFF;
	}
	else if (strcmp(arg, "auto") == 0) {
		menuw = widgets.options.hp_auto_btn;
		newmode = FILTER_AUTO;
	}
	else {
		menuw = widgets.options.hp_manual_btn;
		newmode = FILTER_MANUAL;

		m = atoi(arg);
		if (m < MIN_HIGHPASS_FREQ || m > MAX_HIGHPASS_FREQ)
			return CMD_STAT_BADARG;

		/* Skip to next arg */
		if ((arg1 = strchr(arg, ' ')) != NULL && *++arg1 != '\0') {
			if ((n = atoi(arg1)) < 0)
				return CMD_STAT_BADARG;

			(void) sprintf(str, "%d", n);
			set_text_string(
				widgets.options.hp_width_txt, str, FALSE
			);
		}

		(void) sprintf(str, "%d", m);
		set_text_string(widgets.options.hp_freq_txt, str, FALSE);
	}

	XtVaSetValues(widgets.options.hp_opt, XmNmenuHistory, menuw, NULL);
	app_data.highpass_mode = newmode;

	XtSetSensitive(
		widgets.options.hp_freq_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);
	XtSetSensitive(
		widgets.options.hp_width_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_flags
 *	Function to execute the "flags" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_flags(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tflags <[C|c][O|o][N|n][E|e][I|i]>\n");
		return CMD_STAT_SUCCESS;
	}

	if (strchr(arg, 'C') != 0)
	    XmToggleButtonSetState(widgets.options.copyrt_btn, True, True);
	else if (strchr(arg, 'c') != 0)
	    XmToggleButtonSetState(widgets.options.copyrt_btn, False, True);

	if (strchr(arg, 'O') != 0)
	    XmToggleButtonSetState(widgets.options.orig_btn, True, True);
	else if (strchr(arg, 'o') != 0)
	    XmToggleButtonSetState(widgets.options.orig_btn, False, True);

	if (strchr(arg, 'P') != 0)
	    XmToggleButtonSetState(widgets.options.nores_btn, True, True);
	else if (strchr(arg, 'p') != 0)
	    XmToggleButtonSetState(widgets.options.nores_btn, False, True);

	if (strchr(arg, 'E') != 0)
	    XmToggleButtonSetState(widgets.options.chksum_btn, True, True);
	else if (strchr(arg, 'e') != 0)
	    XmToggleButtonSetState(widgets.options.chksum_btn, False, True);

	if (strchr(arg, 'I') != 0)
	    XmToggleButtonSetState(widgets.options.iso_btn, True, True);
	else if (strchr(arg, 'i') != 0)
	    XmToggleButtonSetState(widgets.options.iso_btn, False, True);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_tag
 *	Function to execute the "tag" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_tag(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp,
			"\ttag <off | v1 | v2 | both>\n");
		return CMD_STAT_SUCCESS;
	}

	if (strcmp(arg, "off") == 0) {
		XmToggleButtonSetState(
			widgets.options.addtag_btn, False, True
		);
	}
	else if (strcmp(arg, "v1") == 0) {
		XmToggleButtonSetState(
			widgets.options.addtag_btn, True, True
		);
		XtVaSetValues(widgets.options.id3tag_ver_opt,
			XmNmenuHistory, widgets.options.id3tag_v1_btn,
			NULL
		);
		app_data.id3tag_mode = ID3TAG_V1;
	}
	else if (strcmp(arg, "v2") == 0) {
		XmToggleButtonSetState(
			widgets.options.addtag_btn, True, True
		);
		XtVaSetValues(widgets.options.id3tag_ver_opt,
			XmNmenuHistory, widgets.options.id3tag_v2_btn,
			NULL
		);
		app_data.id3tag_mode = ID3TAG_V2;
	}
	else if (strcmp(arg, "both") == 0) {
		XmToggleButtonSetState(
			widgets.options.addtag_btn, True, True
		);
		XtVaSetValues(widgets.options.id3tag_ver_opt,
			XmNmenuHistory, widgets.options.id3tag_both_btn,
			NULL
		);
		app_data.id3tag_mode = ID3TAG_BOTH;
	}
	else
		return CMD_STAT_BADARG;

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_lameopts
 *	Function to execute the "lameopts" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_lameopts(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	char	*arg1;

	if (prn_usage) {
		(void) fprintf(errfp, "\tlameopts <<%s> [%s]>\n",
			"disable | insert | append | replace", "options"
		);
		return CMD_STAT_SUCCESS;
	}

	if (strncmp(arg, "disable", 7) == 0) {
		XmToggleButtonSetState(
			widgets.options.lame_disable_btn, True, True
		);
	}
	else if (strncmp(arg, "insert", 6) == 0) {
		XmToggleButtonSetState(
			widgets.options.lame_insert_btn, True, True
		);
	}
	else if (strncmp(arg, "append", 6) == 0) {
		XmToggleButtonSetState(
			widgets.options.lame_append_btn, True, True
		);
	}
	else if (strncmp(arg, "replace", 7) == 0) {
		XmToggleButtonSetState(
			widgets.options.lame_replace_btn, True, True
		);
	}
	else
		return CMD_STAT_BADARG;

	/* Skip to next arg if specified */
	if ((arg1 = strchr(arg, ' ')) != NULL && *++arg1 != '\0')
		set_text_string(widgets.options.lame_opts_txt, arg1, FALSE);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_motd
 *	Function to execute the "motd" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_motd(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tmotd\n");
		return CMD_STAT_SUCCESS;
	}

	/* Display MOTD messages if any */
	motd_get((byte_t *) 1);

	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_window
 *	Function to execute the "window" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
STATIC int
cmd_do_window(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\twindow %s\n",
			"<modechg | iconify | deiconify | raise | lower>");
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "modechg") == 0) {
		XtCallCallbacks(
			widgets.main.mode_btn,
			XmNactivateCallback, (XtPointer) s
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "iconify") == 0) {
		XIconifyWindow(
			dpy,
			XtWindow(widgets.toplevel),
			DefaultScreen(dpy)
		);
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "deiconify") == 0) {
		XMapWindow(dpy, XtWindow(widgets.toplevel));
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "raise") == 0) {
		XRaiseWindow(dpy, XtWindow(widgets.toplevel));
		return CMD_STAT_SUCCESS;
	}
	if (strcmp(arg, "lower") == 0) {
		XLowerWindow(dpy, XtWindow(widgets.toplevel));
		return CMD_STAT_SUCCESS;
	}
	return CMD_STAT_BADARG;
}


/*
 * cmd_do_quit
 *	Function to execute the "quit" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_quit(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tquit\n");
		return CMD_STAT_SUCCESS;
	}
	(void) cd_timeout(QUIT_DELAY, cd_quit, (byte_t *) s);
	return CMD_STAT_SUCCESS;
}


/*
 * cmd_do_debug
 *	Function to execute the "debug" request
 *
 * Args:
 *	dpy - The X display.
 *	s - Pointer to the curstat_t structure.
 *	arg - Argument string.
 *	prn_usage - if TRUE, display argument usage help.
 *
 * Return:
 *	status code CMD_STAT_*
 */
/*ARGSUSED*/
STATIC int
cmd_do_debug(Display *dpy, curstat_t *s, char *arg, bool_t prn_usage)
{
	if (prn_usage) {
		(void) fprintf(errfp, "\tdebug <level#>\n");
		return CMD_STAT_SUCCESS;
	}

	if (!isdigit((int) arg[0]))
		return CMD_STAT_BADARG;

	app_data.debug = (word32_t) atoi(arg);
	if (app_data.debug != 0)
		(void) fprintf(errfp, "Enabling DEBUG: level=0x%x.\n",
				app_data.debug);
	else
		(void) fprintf(errfp, "Disabling DEBUG.\n");

	/* Debug level change notification */
	di_debug();
	return CMD_STAT_SUCCESS;
}


/* Service function mapping table */
struct {
	char		*name;
	int		(*func)(Display *, curstat_t *, char *, bool_t);
	bool_t		arg_required;
} cmd_fmtab[] = {
	{ "stop",	cmd_do_stop,		FALSE,	},
	{ "play",	cmd_do_play,		FALSE	},
	{ "pause",	cmd_do_pause,		FALSE	},
	{ "sample",	cmd_do_sample,		FALSE	},
	{ "disc",	cmd_do_disc,		TRUE	},
	{ "track",	cmd_do_track,		TRUE	},
	{ "index",	cmd_do_index,		TRUE	},
	{ "lock",	cmd_do_lock,		TRUE	},
	{ "shuffle",	cmd_do_shuffle,		TRUE	},
	{ "repeat",	cmd_do_repeat,		TRUE	},
	{ "program",	cmd_do_program,		TRUE	},
	{ "volume",	cmd_do_volume,		TRUE	},
	{ "balance",	cmd_do_balance,		TRUE	},
	{ "route",	cmd_do_route,		TRUE	},
	{ "outport",	cmd_do_outport,		TRUE	},
	{ "cdda-att",	cmd_do_cdda_att,	TRUE	},
	{ "time",	cmd_do_time,		TRUE	},
	{ "on-load",	cmd_do_onload,		TRUE	},
	{ "on-exit",	cmd_do_onexit,		TRUE	},
	{ "on-done",	cmd_do_ondone,		TRUE	},
	{ "on-eject",	cmd_do_oneject,		TRUE	},
	{ "changer",	cmd_do_changer,		TRUE	},
	{ "mode",	cmd_do_mode,		TRUE	},
	{ "jittercorr",	cmd_do_jitter,		TRUE	},
	{ "trackfile",	cmd_do_trkfile,		TRUE	},
	{ "subst",	cmd_do_subst,		TRUE	},
	{ "filefmt",	cmd_do_filefmt,		TRUE	},
	{ "outfile",	cmd_do_outfile,		TRUE	},
	{ "pipeprog",	cmd_do_pipeprog,	TRUE	},
	{ "compress",	cmd_do_compress,	TRUE	},
	{ "min-brate",	cmd_do_minbrate,	TRUE	},
	{ "max-brate",	cmd_do_maxbrate,	TRUE	},
	{ "coding",	cmd_do_coding,		TRUE	},
	{ "lowpass",	cmd_do_lowpass,		TRUE	},
	{ "highpass",	cmd_do_highpass,	TRUE	},
	{ "flags",	cmd_do_flags,		TRUE	},
	{ "tag",	cmd_do_tag,		TRUE	},
	{ "lameopts",	cmd_do_lameopts,	TRUE	},
	{ "motd",	cmd_do_motd,		FALSE	},
	{ "window",	cmd_do_window,		TRUE	},
	{ "quit",	cmd_do_quit,		FALSE	},
	{ "debug",	cmd_do_debug,		TRUE	},
	{ NULL,		NULL,			FALSE	}
};


/*
 * cmd_parse_str
 *	Parse command string and perform requested operation.
 *
 * Args:
 *	req - The request string.
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	status code CMD_STAT_*
 */
STATIC int
cmd_parse_str(Display *dpy, char *str, curstat_t *s)
{
	char		*p,
			*argstr;
	int		i,
			ret;

	/* Set parameter string, if found */
	argstr = NULL;
	if ((p = strchr(str, ' ')) != NULL) {
		*p = '\0';
		argstr = p + 1;
	}

	ret = CMD_STAT_UNSUPP;

	/* Map the request to its service function and call it */
	for (i = 0; cmd_fmtab[i].name != NULL; i++) {
		if (strcmp(cmd_fmtab[i].name, str) == 0) {
			if (cmd_fmtab[i].arg_required && argstr == NULL)
				return CMD_STAT_BADARG;

			/* Change to watch cursor */
			cd_busycurs(TRUE, CURS_ALL);

			ret = (*cmd_fmtab[i].func)(dpy, s, argstr, FALSE);

			/* Change to normal cursor */
			cd_busycurs(FALSE, CURS_ALL);
			break;
		}
	}

	if (p != NULL)
		*p = ' ';
	return (ret);
}


/*
 * cmd_clientwin_r
 *	Traverse a window hierarchy looking for a client's
 *	top-level window, and return its window ID.  Used by
 *	cmd_clientwin() to descend the window tree.
 *
 * Args:
 *	dpy - The X display
 *	clwin - The top level client window ID
 *	wm_state - property atom
 *
 * Return:
 *	The window ID, or 0 if not found.
 */
STATIC Window
cmd_clientwin_r(Display *dpy, Window clwin, Atom wm_state)
{
	Window		root,
			parent,
			*ch;
	Atom		type = None;
	int		i,
			fmt;
	unsigned int	nch;
	unsigned long	n,
			resid;
	unsigned char	*data;
	Window		retwin;

	if (!XQueryTree(dpy, clwin, &root, &parent, &ch, &nch) ||
	    ch == NULL || nch == 0)
		return (Window) 0;

	for (retwin = 0, i = 0; i < (int) nch; i++) {
		if (XGetWindowProperty(dpy, ch[i], wm_state, 0, 0,
				   False, AnyPropertyType, &type, &fmt,
				   &n, &resid, &data) != Success || type == 0)
			continue;

		retwin = ch[i];

		if (retwin != 0)
			break;
	}

	for (i = 0; retwin == 0 && i < (int) nch; i++) {
		if ((retwin = cmd_clientwin_r(dpy, ch[i], wm_state)) != 0)
			break;
	}

	if (ch != NULL)
		XFree((char *) ch);

	return (retwin);
}


/*
 * cmd_clientwin
 *	Find a top-level client window within the window hierarchy
 *	whose root is clwin.
 *
 * Args:
 *	dpy - The X display
 *	clwin - The top level client window ID
 *
 * Return:
 *	The window ID, or 0 if not found.
 */
STATIC Window
cmd_clientwin(Display *dpy, Window clwin)
{
	Atom		wm_state,
			type;
	int		fmt;
	unsigned long	n,
			resid;
	unsigned char	*data;
	Window		retwin;

	/* ICCCM says that windows with the WM_STATE property are
	 * top level windows.
	 */
	if ((wm_state = XmInternAtom(dpy, "WM_STATE", True)) == 0)
		return (clwin);

	if (XGetWindowProperty(dpy, clwin, wm_state,
			0, 0, False, AnyPropertyType, &type,
			&fmt, &n, &resid, &data) != Success || type != 0)
		/* clwin is the top-level client window */
		return (clwin);

	/* Traverse clwin's children */
	if ((retwin = cmd_clientwin_r(dpy, clwin, wm_state)) == 0)
		retwin = clwin;

	return (retwin);
}


/*
 * cmd_getwin
 *	Locate an xmcd top level window running on the specified display.
 *
 * Args:
 *	dpy - The X display
 *
 * Return:
 *	The window ID, or 0 if not found.
 */
STATIC Window
cmd_getwin(Display *dpy)
{
	Window		root,
			parent,
			w,
			*ch;
	Atom		type;
	int		i,
			j,
			fmt;
	unsigned int	nch;
	unsigned long	n,
			resid;
	char		*p,
			*idstr,
			*host,
			*device;
#ifdef __VMS
	char		dev[FILE_PATH_SZ];
#endif

	ch = NULL;
	idstr = NULL;
	nch = 0;

	/* Get list of children of the root window */
	if (!XQueryTree(dpy, RootWindowOfScreen(DefaultScreenOfDisplay(dpy)),
			&root, &parent, &ch, &nch) ||
	    ch == NULL || nch == 0)
		return (Window) 0;

	/* For each children, find the top-level window of a client */
	for (i = 0; i < (int) nch; i++) {
		w = cmd_clientwin(dpy, ch[i]);

		/* Check for xmcd ident */
		if (XGetWindowProperty(dpy, w, cmd_ident,
			0, MAX_IDENT_LEN, False, XA_STRING,
			&type, &fmt, &n, &resid,
			(unsigned char **) &idstr) != Success ||
		    type != XA_STRING || idstr == NULL)
			continue;

		DBGPRN(DBG_RMT)(errfp,
			"Found \"%s\" window 0x%x\n", idstr, (int) w);

		/* Locate the host and device */
		for (p = idstr, j = 0; j < 2; j++, p++) {
			if ((p = strchr(p, ' ')) == NULL)
				break;
		}
		if (p == NULL) {
			/* Invalid ident string */
			XFree(idstr);
			DBGPRN(DBG_RMT)(errfp,
				"Invalid ident string: no host.\n");
			continue;
		}

		host = p;
		if ((p = strchr(p, ' ')) == NULL) {
			/* Invalid ident string */
			XFree(idstr);
			DBGPRN(DBG_RMT)(errfp,
				"Invalid ident string: no device.\n");
			continue;
		}

		*p++ = '\0';
		device = p;

		/* Match the remote client host */
		if (app_data.remotehost != NULL &&
		    app_data.remotehost[0] != '\0') {

			DBGPRN(DBG_RMT)(errfp,
				"Comparing host \"%s\" : \"%s\"\n",
				host, app_data.remotehost);

			if (HOST_STRCMP(host, app_data.remotehost) != 0) {
				/* No match */
				XFree(idstr);
				continue;
			}
		}

		/* Match the remote client device */
#ifdef __VMS
		if (strchr(app_data.device, '$') != NULL) {
			if (strchr(device, '$') == NULL)
				(void) sprintf(dev, "%s$%s", host, device);
			else
				(void) strcpy(dev, device);

			DBGPRN(DBG_RMT)(errfp,
				"Comparing device \"%s\" : \"%s\"\n",
				dev, app_data.device);

			if (DEV_STRCMP(dev, app_data.device) != 0) {
				/* No match */
				XFree(idstr);
				continue;
			}
		}
		else {
			if (strchr(device, '$') == NULL)
				(void) strcpy(dev, app_data.device);
			else
				(void) sprintf(dev, "%s$%s",
						host, app_data.device);

			DBGPRN(DBG_RMT)(errfp,
				"Comparing device \"%s\" : \"%s\"\n",
				device, dev);

			if (DEV_STRCMP(device, dev) != 0) {
				/* No match */
				XFree(idstr);
				continue;
			}
		}
#else
		DBGPRN(DBG_RMT)(errfp, "Comparing device \"%s\" : \"%s\"\n",
			device, app_data.device);

		if (DEV_STRCMP(device, app_data.device) != 0) {
			/* No match */
			XFree(idstr);
			continue;
		}
#endif

		XFree(idstr);

		/* Found client */
		DBGPRN(DBG_RMT)(errfp, "Using window 0x%x\n", (int) w);
		return (w);
 	}

	/* Cannot find client */
	(void) fprintf(errfp, "%s (%s%s%s): %s %s\n",
			PROGNAME,
			app_data.remotehost == NULL ? "" : app_data.remotehost,
			app_data.remotehost == NULL ? "" : ":",
			app_data.device,
			app_data.str_noclient,
			DisplayString(dpy));
	return (Window) 0;
}


/*
 * cmd_attach
 *	Attach to the remote xmcd client for exclusive control.
 *
 * Args:
 *	dpy - The X display.
 *	win - The xmcd client window ID.
 *
 * Return:
 *	0 - success
 *	1 - failure.
 */
STATIC int
cmd_attach(Display *dpy, Window win)
{
	int		fmt;
	unsigned long	n,
			resid;
	Atom		type;
	struct utsname	*up;
	char		*attstr = NULL;
	XEvent		ev;

	up = util_get_uname();
	(void) sprintf(cmd_attstr,
#ifdef __VMS
			"%s@%s/%x",
#else
			"%s@%s/%d",
#endif
			util_loginname(), up->nodename, (int) getpid());

	XSelectInput(dpy, win, PropertyChangeMask | StructureNotifyMask);

	for (;;) {
		XGrabServer(dpy);

		if (XGetWindowProperty(dpy, win, cmd_att,
			0, MAX_ATT_LEN, False, XA_STRING,
			&type, &fmt, &n, &resid,
			(unsigned char **) &attstr) != Success ||
		    type != XA_STRING || attstr == NULL) {
			DBGPRN(DBG_RMT)(errfp, "Attaching to window 0x%x\n",
				(int) win);

			XChangeProperty(
				dpy, win,
				cmd_att, XA_STRING, 8,
				PropModeReplace,
				(unsigned char *) cmd_attstr,
				strlen(cmd_attstr)
			);

			XUngrabServer(dpy);
			break;
		}

		XUngrabServer(dpy);
		XSync(dpy, False);

		if (attstr != NULL)
			XFree(attstr);

		DBGPRN(DBG_RMT)(errfp,
			"Window 0x%x is attached by %s; Waiting...\n",
			(int) win, attstr);

		/* Wait until detach or destroy */
		for (;;) {
			XNextEvent(dpy, &ev);

			if (EV_PROP_NOTIFY(&ev, win, PropertyDelete, cmd_att)) {
				DBGPRN(DBG_RMT)(errfp,
					"Window 0x%x detached.\n",
					(int) win);
				break;
			}
			else if (EV_DESTROY_WIN(&ev, win)) {
				DBGPRN(DBG_RMT)(errfp,
					"Window 0x%x was destroyed.\n",
					(int) win);
				return 1;
			}
		}
	}

	return 0;
}


/* 
 * cmd_detach
 *	Detach from the remote xmcd client.
 *
 * Args:
 *	dpy - The X display.
 *	win - The xmcd client window ID
 *
 * Return:
 *	Nothing.
 */
STATIC void
cmd_detach(Display *dpy, Window win)
{
	Atom		type;
	int		fmt;
	unsigned long	n,
			resid;
	char		*attstr = NULL;

	if (XGetWindowProperty(dpy, win, cmd_att, 0, MAX_ATT_LEN,
		True, XA_STRING,  &type, &fmt, &n, &resid,
		(unsigned char **) &attstr) != Success ||
	    type != XA_STRING || attstr == NULL) {
		DBGPRN(DBG_RMT)(errfp,
			"Cannot detach from window 0x%x.\n", (int) win);
		return;
	}

	if (app_data.debug & DBG_RMT) {
		if (attstr == NULL || *attstr == '\0') {
			(void) fprintf(errfp,
				"Null att property data on window 0x%x.\n",
				(int) win
			);
		}
		else if (strcmp(attstr, cmd_attstr) != 0) {
			(void) fprintf(errfp,
				"Detach: expect \"%s\", found \"%s\"\n",
				cmd_attstr, attstr
			);
		}
	}

	if (attstr != NULL)
		XFree(attstr);

	DBGPRN(DBG_RMT)(errfp, "Detached from window 0x%x\n", (int) win);
}


/*
 * cmd_putreq
 *	Send request to remote client window.
 *
 * Args:
 *	dpy - The X display.
 *	win - The xmcd client window ID.
 *	str - The request string.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cmd_putreq(Display *dpy, Window win, char *str)
{
	DBGPRN(DBG_RMT)(errfp,
		"Sending request (%s): %s\n", DisplayString(dpy), str);

	XChangeProperty(
		dpy, win,
		cmd_req, XA_STRING, 8,
		PropModeReplace,
		(unsigned char *) str, strlen(str)
	);
}


/*
 * cmd_getack
 *	Get acknowledgement and status from remote client window
 *	after sending a request to it.
 *
 * Args:
 *	dpy - The X display.
 *	win - The xmcd client window ID.
 *	str - The request string.
 *
 * Return:
 *	status code CMD_STAT_*
 */
STATIC int
cmd_getack(Display *dpy, Window win, char *str)
{
	int		ret,
			fmt;
	unsigned long	n,
			resid;
	int		*ackp = NULL;
	Atom		type;
	XEvent		ev;

	/* Wait for ack */
	for (;;) {
		XNextEvent(dpy, &ev);

		if (EV_PROP_NOTIFY(&ev, win, PropertyNewValue, cmd_ack)) {
			if ((ret = XGetWindowProperty(dpy, win, cmd_ack,
					0, MAX_ACK_LEN, True, XA_INTEGER,
					&type, &fmt, &n, &resid,
					(unsigned char **)
					&ackp)) != Success ||
			    fmt != 32 ||
			    type != XA_INTEGER ||
			    ackp == NULL) {
				DBGPRN(DBG_RMT)(errfp,
					"Cannot get ack from window 0x%0x.\n",
					(int) win
				);
				return CMD_STAT_ACKFAIL;
			}

			ret = *ackp;
			XFree((char *) ackp);

			DBGPRN(DBG_RMT)(errfp,
				"Remote request status code %d: ", ret);

			switch (ret) {
			case CMD_STAT_SUCCESS:
				DBGPRN(DBG_RMT)(errfp, "Completed\n");
				return (ret);

			case CMD_STAT_PROCESSING:
				DBGPRN(DBG_RMT)(errfp, "Processing\n");
				break;

			case CMD_STAT_UNSUPP:
				(void) fprintf(errfp, "%s: %s: %s\n",
					app_data.str_cmdfail,
					app_data.str_unsuppcmd,
					str
				);
				return (ret);

			case CMD_STAT_BADARG:
				(void) fprintf(errfp, "%s: %s\n",
					app_data.str_cmdfail,
					app_data.str_badarg
				);
				return (ret);

			case CMD_STAT_INVAL:
				(void) fprintf(errfp, "%s: %s\n",
					app_data.str_cmdfail,
					app_data.str_invcmd
				);
				return (ret);

			default:
				DBGPRN(DBG_RMT)(errfp,
				    "Unrecognized ack from window 0x%x: %d\n",
				    (int) win, ret);
				break;
			}
		}
		else if (EV_DESTROY_WIN(&ev, win)) {
			DBGPRN(DBG_RMT)(errfp,
				"%s: window 0x%x was destroyed.\n",
				"Failed to send remote request",
				(int) win
			);
			return CMD_STAT_DESTROYED;
		}
	}
	/*NOTREACHED*/
}


/*
 * cmd_rmtlog
 *	Log remote activity to remote log file ($HOME/.xmcdcfg/remote.log)
 *
 * Args:
 *	dpy - The X display
 *	attstr - The attach info string
 *	reqstr - The request string
 *
 * Return:
 *	Nothing.
 */
STATIC void
cmd_rmtlog(Display *dpy, char *attstr, char *reqstr)
{
	int		ret;
	FILE		*fp;
	struct tm	*tm;
	struct utsname	*up;
	char		*cp,
			*homep,
       			path[FILE_PATH_SZ + 32];
	time_t		t;
	struct stat	stbuf;
	pid_t		ppid;
	bool_t		newfile;
#ifndef __VMS
	pid_t		cpid;
	waitret_t	wstat;
#endif

	DBGPRN(DBG_RMT)(errfp, "%s\n", reqstr);

	if (!app_data.remote_log)
		return;

	ppid = getpid();

	/*
	 * Write entry to log file
	 */

#ifndef __VMS
	/* Fork child to perform actual I/O */
	switch (cpid = FORK()) {
	case -1:
		DBGPRN(DBG_RMT)(errfp,
			"cmd_rmtlog: fork failed (errno=%d)\n", errno);
		return;

	case 0:
		/* Child process */

		/* Force uid and gid to original setting */
		if (!util_set_ougid())
			_exit(1);

		break;

	default:
		/* Parent process: wait for child to exit */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     event_loop, 0, FALSE, &wstat);
		if (app_data.debug & DBG_RMT) {
			if (ret < 0) {
				(void) fprintf(errfp,
					"cmd_rmtlog: waitpid fail errno=%d\n",
					errno);
			}
			else if (WIFEXITED(wstat)) {
				(void) fprintf(errfp,
					"cmd_rmtlog: child exit status %d\n",
					WEXITSTATUS(wstat));
			}
			else if (WIFSIGNALED(wstat)) {
				(void) fprintf(errfp,
					"cmd_rmtlog: child killed sig=%d\n",
					WTERMSIG(wstat));
			}
		}
		return;
	}
#endif	/* __VMS */

	homep = util_homedir(util_get_ouid());
	if ((int) strlen(homep) >= FILE_PATH_SZ)
		/* Path name too long */
		RL_RET(1);

	(void) sprintf(path, USR_RMTLOG_PATH, homep);

	DBGPRN(DBG_GEN)(errfp, "Writing remote log file %s\n", path);

	/* Check the path */
	newfile = FALSE;
	if (stat(path, &stbuf) < 0) {
		if (errno == ENOENT)
			newfile = TRUE;
		else {
			DBGPRN(DBG_GEN)(errfp, "Cannot stat %s (errno=%d)\n",
				path, errno);
			RL_RET(1);
		}
	}
	else {
		if (!S_ISREG(stbuf.st_mode)) {
			DBGPRN(DBG_GEN)(errfp,
				"Not a regular file error: %s\n", path);
			RL_RET(1);
		}

		if (stbuf.st_size == 0)
			newfile = TRUE;
	}

	/* Get a log file handle */
	if ((fp = fopen(path, "a")) == NULL) {
		DBGPRN(DBG_GEN)(errfp, "Cannot open file: %s\n", path);
		RL_RET(1);
	}

	/* Set file permissions */
	util_setperm(path, app_data.hist_filemode);

	if (newfile)
		/* Write remote log file header */
		(void) fprintf(fp, RMTLOG_BANNER1 RMTLOG_BANNER2,
			       VERSION_MAJ, VERSION_MIN, COPYRIGHT);

	t = time(NULL);
#if !defined(__VMS) || ((__VMS_VER >= 70000000) && (__DECC_VER > 50230003))
	tzset();
#endif
	tm = localtime(&t);
	up = util_get_uname();
	cp = NULL;
	if (app_data.device == NULL || app_data.device[0] == '\0' ||
	    (cp = util_basename(app_data.device)) == NULL) {
		if (!util_newstr(&cp, "???")) {
			DBGPRN(DBG_GEN)(errfp, "%s\n", app_data.str_nomemory);
			RL_RET(1);
		}
	}

	/* Write log file entry */
	ret = fprintf(fp,
#ifdef __VMS
		"%02d/%02d/%02d %02d:%02d:%02d %s/%s/%x/%s %s %s\n",
#else
		"%02d/%02d/%02d %02d:%02d:%02d %s/%s/%d/%s %s %s\n",
#endif
		(tm->tm_year + 1900) % 100,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec,
		(up->nodename == NULL || up->nodename[0] == '\0') ?
			"???" : up->nodename,
		cp,
		(int) ppid,
		DisplayString(dpy),
		attstr,
		reqstr);

	MEM_FREE(cp);

	if (ret < 0) {
		ret = errno;
		(void) fclose(fp);
		errno = ret;

		DBGPRN(DBG_GEN)(errfp, "File write error: %s (errno=%d)\n",
			path, errno);
		RL_RET(1);
	}

	/* Close file */
	if (fclose(fp) != 0) {
		DBGPRN(DBG_GEN)(errfp, "File close error: %s (errno=%d)\n",
			path, errno);
		RL_RET(1);
	}

	/* Child exits here */
	RL_RET(0);
	/*NOTREACHED*/
}


/***********************
 *   public routines   *
 ***********************/


/*
 * cmd_init
 *	Initialize the command subsystem.
 *
 * Args:
 *	dpy - The X display.
 *
 * Return:
 *	Nothing.
 */
void
cmd_init(curstat_t *s, Display *dpy, bool_t sender)
{
	struct utsname	*up;
	char		*cp,
			*hd,
			str[MAX_IDENT_LEN + 1];

	if (sender) {
		DBGPRN(DBG_ALL)(errfp,
			"XMCD %s.%s.%s DEBUG MODE (remote sender)\n",
			VERSION_MAJ, VERSION_MIN, VERSION_TEENY);

		if ((cp = (char *) getenv("XMCD_LIBDIR")) == NULL) {
			/* No library directory specified */
			if (di_isdemo()) {
				/* Demo mode: just fake it */
				app_data.libdir = CUR_DIR;
			}
			else {
				/* Real application: this is a fatal error */
				CD_FATAL(app_data.str_libdirerr);
				return;
			}
		}
		else if (!util_newstr(&app_data.libdir, cp)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		/* Paranoia: avoid overflowing buffers */
		if ((int) strlen(app_data.libdir) >= FILE_PATH_SZ) {
			CD_FATAL(app_data.str_longpatherr);
			return;
		}
		hd = util_homedir(util_get_ouid());
		if ((int) strlen(hd) >= FILE_PATH_SZ) {
			CD_FATAL(app_data.str_longpatherr);
			return;
		}

		/* Get system common configuration parameters */
		(void) sprintf(str, SYS_CMCFG_PATH, app_data.libdir);
		di_common_parmload(str, TRUE, FALSE);

		/* Get user common configuration parameters */
		(void) sprintf(str, USR_CMCFG_PATH, hd);
		di_common_parmload(str, FALSE, FALSE);

		/* Paranoia: avoid overflowing buffers */
		if (app_data.device != NULL &&
		    (int) strlen(app_data.device) >= FILE_PATH_SZ) {
			CD_FATAL(app_data.str_longpatherr);
			return;
		}

		if ((cp = util_basename(app_data.device)) == NULL) {
			CD_FATAL("Device path basename error");
			return;
		}
		if ((int) strlen(cp) >= FILE_BASE_SZ) {
			MEM_FREE(cp);
			CD_FATAL(app_data.str_longpatherr);
			return;
		}
		MEM_FREE(cp);

		if (!app_data.remote_enb)
			return;
	}

	if (app_data.remote_enb) {
		cmd_ident = XmInternAtom(dpy, CMD_IDENT, False);
		cmd_att = XmInternAtom(dpy, CMD_ATT, False);
		cmd_req = XmInternAtom(dpy, CMD_REQ, False);
		cmd_ack = XmInternAtom(dpy, CMD_ACK, False);

		if (!sender) {
			/* Add callback for property change */
			XtAddEventHandler(
				widgets.toplevel,
				PropertyChangeMask,
				False,
				(XtEventHandler) cmd_propchg,
				(XtPointer) s
			);

			up = util_get_uname();

			/* Set identification property */
			(void) sprintf(str, "%s %s.%s %s %s",
					PROGNAME, VERSION_MAJ, VERSION_MIN,
					up->nodename, app_data.device);

			XChangeProperty(
				dpy, XtWindow(widgets.toplevel),
				cmd_ident, XA_STRING, 8,
				PropModeReplace,
				(unsigned char *) str, strlen(str)
			);
		}
	}
}


/*
 * cmd_startup
 *	Perform specified command during startup
 *
 * Args:
 *	cmdstr - The command string.
 *
 * Return:
 *	status code CMD_STAT_*
 */
void
cmd_startup(byte_t *cmdstr)
{
	Display		*dpy = XtDisplay(widgets.toplevel);
	curstat_t	*s = curstat_addr();
	int		ret;

	DBGPRN(DBG_RMT)(errfp,
		"\nExecuting startup command: %s\n", (char *) cmdstr);

	ret = cmd_parse_str(dpy, (char *) cmdstr, s);

	DBGPRN(DBG_RMT)(errfp, "Command status %d: ", ret);

	switch (ret) {
	case CMD_STAT_SUCCESS:
		DBGPRN(DBG_RMT)(errfp, "Success\n");
		break;

	case CMD_STAT_UNSUPP:
		(void) fprintf(errfp, "%s: %s: %s\n",
			app_data.str_cmdfail, app_data.str_unsuppcmd,
			(char *) cmdstr);
		break;

	case CMD_STAT_BADARG:
		(void) fprintf(errfp, "%s: %s\n",
			app_data.str_cmdfail, app_data.str_badarg);
		break;

	case CMD_STAT_INVAL:
		(void) fprintf(errfp, "%s: %s\n",
			app_data.str_cmdfail, app_data.str_invcmd);
		break;

	default:
		/* These should not happen in this mode */
		DBGPRN(DBG_RMT)(errfp, "???\n");
		break;
	}

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);
}


/*
 * cmd_sendrmt
 *	Send a command to a remote xmcd client.
 *
 * Args:
 *	cmd - Command string.
 *
 * Return:
 *	status code CMD_STAT_*
 */
int
cmd_sendrmt(Display *dpy, char *cmd)
{
	int	ret;
	Window	win;

	if (!app_data.remote_enb) {
		(void) fprintf(errfp, "%s.\n", app_data.str_rmt_notenb);
		return CMD_STAT_DISABLED;
	}

	if (cmd == NULL || cmd[0] == '\0') {
		(void) fprintf(errfp, "%s.\n", app_data.str_rmt_nocmd);
		return CMD_STAT_NOCMD;
	}

	DBGPRN(DBG_RMT)(errfp, "\n* REMOTE: Sender\n\n");

	/* Locate appropriate remote client window */
	if ((win = cmd_getwin(dpy)) == 0)
		return CMD_STAT_NOCLIENT;

	/* Attach to remote client window */
	if (cmd_attach(dpy, win) != 0)
		return CMD_STAT_ATTFAIL;

	/* Send the request */
	cmd_putreq(dpy, win, cmd);

	/* Get acknowledgement */
	ret = cmd_getack(dpy, win, cmd);

	if (ret != CMD_STAT_DESTROYED)
		/* Detach from remote client window */
		cmd_detach(dpy, win);

	return (ret);
}


/*
 * cmd_usage
 *	Display command usage information
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cmd_usage(void)
{
	int	i;

	(void) fprintf(errfp, "Valid commands are:\n");

	/* Call each service function prn_usage */
	for (i = 0; cmd_fmtab[i].name != NULL; i++)
		(void) (*cmd_fmtab[i].func)(NULL, NULL, NULL, TRUE);
}


/**************** vv Callback routines vv ****************/


/*
 * cmd_propchg
 *      Property change callback function
 */
void
cmd_propchg(Widget w, XtPointer client_data, XEvent *ev, Boolean *cont)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	Display		*dpy = XtDisplay(w);
	int		ret,
			fmt;
	unsigned long	n,
			resid;
	Atom		type;
	Window		win;
	char		*reqstr;
	static char	*attstr = NULL;

	*cont = True;
	if (w != widgets.toplevel)
		return;

	win = XtWindow(w);

	if (EV_PROP_NOTIFY(ev, win, PropertyNewValue, cmd_req)) {
		DBGPRN(DBG_RMT)(errfp, "\n* REMOTE: Request received: ");

		if ((ret = XGetWindowProperty(dpy, XtWindow(w), cmd_req,
				0, MAX_REQ_LEN, True, XA_STRING,
				&type, &fmt, &n, &resid,
				(unsigned char **) &reqstr)) != Success ||
		    type != XA_STRING || reqstr == NULL) {
			DBGPRN(DBG_RMT)(errfp,
				"Cannot get window 0x%x req property.\n",
				(int) XtWindow(w));
		}
		else {
			/* Log remote activity */
			cmd_rmtlog(dpy,
				attstr != NULL ? attstr : "(unknown)",
				reqstr != NULL ? reqstr : "(none)"
			);

			/* Send received ack to remote sender */
			ret = CMD_STAT_PROCESSING;
			XChangeProperty(
				dpy, XtWindow(w),
				cmd_ack, XA_INTEGER, 32,
				PropModeReplace,
				(unsigned char *) &ret, MAX_ACK_LEN
			);

			/* Perform the request and get status */
			ret = cmd_parse_str(dpy, reqstr, s);

			/* Send completion ack to remote sender */
			XChangeProperty(
				dpy, XtWindow(w),
				cmd_ack, XA_INTEGER, 32,
				PropModeReplace,
				(unsigned char *) &ret, MAX_ACK_LEN
			);
		}

		if (reqstr != NULL)
			XFree(reqstr);
	}
	else if (EV_PROP_NOTIFY(ev, win, PropertyNewValue, cmd_att)) {
		if ((ret = XGetWindowProperty(dpy, XtWindow(w), cmd_att,
				0, MAX_ATT_LEN, False, XA_STRING,
				&type, &fmt, &n, &resid,
				(unsigned char **) &attstr)) != Success ||
		    type != XA_STRING || attstr == NULL) {
			DBGPRN(DBG_RMT)(errfp,
				"Cannot get window 0x%x att property.\n",
				(int) XtWindow(w));
		}
		else {
			DBGPRN(DBG_RMT)(errfp,
				"\n* REMOTE: Attach %s\n", attstr);
		}
	}
	else if (EV_PROP_NOTIFY(ev, win, PropertyDelete, cmd_att)) {
		if (attstr != NULL) {
			DBGPRN(DBG_RMT)(errfp,
				"\n* REMOTE: Detach %s\n", attstr);
			XFree(attstr);
			attstr = NULL;
		}
	}
}


