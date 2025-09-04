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
static char *_cdfunc_c_ident_ = "@(#)cdfunc.c	7.279 04/03/11";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/callback.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"
#include "xmcd_d/dbprog.h"
#include "xmcd_d/wwwwarp.h"
#include "xmcd_d/geom.h"
#include "xmcd_d/hotkey.h"
#include "xmcd_d/help.h"
#include "xmcd_d/cdfunc.h"
#include "cdda_d/cdda.h"


#define CHGDISC_DELAY	1500			/* Disc chg delay */

#define WM_FUDGE_X	10			/* Window manager decoration */
#define WM_FUDGE_Y	25
#define WIN_MINWIDTH	150			/* Dialog box minimum width */
#define WIN_MINHEIGHT	100			/* Dialog box minimum height */


/* Macro to detemine a valid VMS file name character */
#define IS_LEGAL_VMSFILECHAR(c) \
	(isalnum((int) (c)) || (c) == '$' || (c) == '-' || (c) == '_')


/* Confirmation/Working dialog box callback info structure */
typedef struct {
	Widget		widget0;		/* Button 1 */
	Widget		widget1;		/* Button 2 */
	Widget		widget2;		/* Dialog box */
	String		type;			/* Callback type */
	XtCallbackProc	func;			/* Callback function */
	XtPointer	data;			/* Callback arg */
} cbinfo_t;

/* Structure to save callback function pointer and its args */
typedef struct {
	XtCallbackProc	func;			/* Callback function */
	Widget		w;			/* Widget */
	XtPointer	client_data;		/* Client data */
	XtPointer	call_data;		/* Call data */
} callback_sav_t;


extern widgets_t	widgets;
extern pixmaps_t	pixmaps;
extern appdata_t	app_data;
extern FILE		*errfp;

wlist_t			*cd_brhead = NULL,	/* Bitrates menu head */
			*cd_minbrhead = NULL,	/* Min bitrate menu head */
			*cd_maxbrhead = NULL;	/* Max bitrate menu head */

STATIC char		keystr[3];		/* Keypad number string */
STATIC int		keypad_mode,		/* Keypad mode */
			cdda_fader_mode;	/* CDDA fader mode */
STATIC long		tm_blinkid = -1,	/* Time dpy blink timer ID */
			ab_blinkid = -1,	/* A->B dpy blink timer ID */
			dbmode_blinkid = -1,	/* Dbmode dpy blink timer ID */
			chgdisc_dlyid = -1,	/* Disc chg delay timer ID */
			cdda_fader_id = -1,	/* CDDA fader timer ID */
			infodiag_id = -1,	/* Info dialog timer ID */
			tooltip1_id = -1,	/* Tooltip popup timer ID */
			tooltip2_id = -1;	/* Tooltip popdown timer ID */
STATIC sword32_t	warp_offset = 0;	/* Track warp block offset */
STATIC word32_t		cdda_fader_intvl = 0;	/* CDDA fader interval */
STATIC bool_t		searching = FALSE,	/* Running REW or FF */
			confirm_pend = FALSE,	/* Confirm response pending */
			confirm_resp = FALSE,	/* Confirm dialog response */
			popup_ok = FALSE,	/* Are popups allowed? */
			pseudo_warp = FALSE,	/* Warp slider only flag */
			warp_busy = FALSE,	/* Warp function active */
			chgdelay = FALSE,	/* Disc change delay */
			mode_chg = FALSE,	/* Changing main window mode */
			pathexp_active = FALSE,	/* Showing exp file path */
			tooltip_active = FALSE,	/* Tooltip is active */
			skip_next_tooltip = FALSE;
						/* Skip the next tooltip */
STATIC cdinfo_client_t	cdinfo_cldata;		/* Client info for libcdinfo */
STATIC di_client_t	di_cldata;		/* Client info for libdi */
STATIC callback_sav_t	override_sav;		/* Mode override callback */
STATIC cbinfo_t		cbinfo0,		/* Dialog box cbinfo structs */
			cbinfo1,
			cbinfo2;
STATIC XmString		xs_qual,		/* Label strings */
			xs_algo;


#define OPT_CATEGS	9			/* Max options categories */

/* Options menu category list entries */
STATIC struct {
	char		*name;
	wlist_t		*widgets;
} opt_categ[OPT_CATEGS+1];


/* Forward declaration prototypes */
STATIC void		cd_pause_blink(curstat_t *, bool_t),
			cd_ab_blink(curstat_t *, bool_t),
			cd_dbmode_blink(curstat_t *, bool_t);


/* Macro used in cd_options() */
#define PATHENT_APPEND(i, str, ent, type)				\
	{								\
	    if ((i) < 2 &&						\
		(strcmp((ent), "CDDB") == 0 ||				\
		 strcmp((ent), "CDTEXT") == 0)) {			\
		(void) strcat(						\
		    (str), ((type) == CDINFO_RMT) ? "CDDB" : "CDTEXT"	\
		);							\
		(i)++;							\
	    }								\
	    else							\
		(void) strcat((str), (ent));				\
	}



/***********************
 *  internal routines  *
 ***********************/


/*
 * filefmt_btn
 *	Given a file format code, return its menu selection button
 *	widget.
 *
 * Args:
 *	fmt - file format code
 *
 * Return:
 *	The associated menu button widget
 */
STATIC Widget
filefmt_btn(int fmt)
{
	switch (fmt) {
	case FILEFMT_AU:
		return (widgets.options.mode_fmt_au_btn);
	case FILEFMT_WAV:
		return (widgets.options.mode_fmt_wav_btn);
	case FILEFMT_AIFF:
		return (widgets.options.mode_fmt_aiff_btn);
	case FILEFMT_AIFC:
		return (widgets.options.mode_fmt_aifc_btn);
	case FILEFMT_MP3:
		return (widgets.options.mode_fmt_mp3_btn);
	case FILEFMT_OGG:
		return (widgets.options.mode_fmt_ogg_btn);
	case FILEFMT_FLAC:
		return (widgets.options.mode_fmt_flac_btn);
	case FILEFMT_AAC:
		return (widgets.options.mode_fmt_aac_btn);
	case FILEFMT_MP4:
		return (widgets.options.mode_fmt_mp4_btn);
	case FILEFMT_RAW:
	default:
		return (widgets.options.mode_fmt_raw_btn);
	}
	/*NOTREACHED*/
}


/*
 * filefmt_code
 *	Given a file format menu selection button widget, return its
 *	file format code.
 *
 * Args:
 *	The file format menu button widget
 *
 * Return:
 *	The associated file format code
 */
STATIC int
filefmt_code(Widget w)
{
	if (w == widgets.options.mode_fmt_raw_btn)
		return FILEFMT_RAW;
	else if (w == widgets.options.mode_fmt_au_btn)
		return FILEFMT_AU;
	else if (w == widgets.options.mode_fmt_wav_btn)
		return FILEFMT_WAV;
	else if (w == widgets.options.mode_fmt_aiff_btn)
		return FILEFMT_AIFF;
	else if (w == widgets.options.mode_fmt_aifc_btn)
		return FILEFMT_AIFC;
	else if (w == widgets.options.mode_fmt_mp3_btn)
		return FILEFMT_MP3;
	else if (w == widgets.options.mode_fmt_ogg_btn)
		return FILEFMT_OGG;
	else if (w == widgets.options.mode_fmt_flac_btn)
		return FILEFMT_FLAC;
	else if (w == widgets.options.mode_fmt_aac_btn)
		return FILEFMT_AAC;
	else if (w == widgets.options.mode_fmt_mp4_btn)
		return FILEFMT_MP4;

	return FILEFMT_RAW;
}


/*
 * disc_etime_norm
 *	Return the elapsed time of the disc in seconds.  This is
 *	used during normal playback.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	The disc elapsed time in seconds.
 */
STATIC sword32_t
disc_etime_norm(curstat_t *s)
{
	sword32_t	secs;

	secs = (s->curpos_tot.min * 60 + s->curpos_tot.sec) -
	       (MSF_OFFSET / FRAME_PER_SEC);
	return ((secs >= 0) ? secs : 0);
}


/*
 * disc_etime_prog
 *	Return the elapsed time of the disc in seconds.  This is
 *	used during shuffle or program mode.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	The disc elapsed time in seconds.
 */
STATIC sword32_t
disc_etime_prog(curstat_t *s)
{
	sword32_t	i,
			secs = 0;

	/* Find the time of all played tracks */
	for (i = 0; i < ((int) s->prog_cnt) - 1; i++) {
		secs += ((s->trkinfo[s->trkinfo[i].playorder + 1].min * 60 +
			 s->trkinfo[s->trkinfo[i].playorder + 1].sec) -
		         (s->trkinfo[s->trkinfo[i].playorder].min * 60 +
			 s->trkinfo[s->trkinfo[i].playorder].sec));
	}

	/* Find the elapsed time of the current track */
	for (i = 0; i < MAXTRACK; i++) {
		if (s->trkinfo[i].trkno == LEAD_OUT_TRACK)
			break;

		if (s->trkinfo[i].trkno == s->cur_trk) {
			secs += (s->curpos_trk.min * 60 + s->curpos_trk.sec);
			break;
		}
	}

	return ((secs >= 0) ? secs : 0);
}


/*
 * track_rtime
 *	Return the remaining time of the current playing track in seconds.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	The track remaining time in seconds.
 */
STATIC sword32_t
track_rtime(curstat_t *s)
{
	sword32_t	i,
			secs,
			tot_sec,
			cur_sec;

	if ((i = di_curtrk_pos(s)) < 0)
		return 0;

	tot_sec = (s->trkinfo[i+1].min * 60 + s->trkinfo[i+1].sec) -
		  (s->trkinfo[i].min * 60 + s->trkinfo[i].sec);

	/* "Enhanced CD" / "CD Extra" hack */
	if (s->trkinfo[i+1].addr > s->discpos_tot.addr) {
		tot_sec -= ((s->trkinfo[i+1].addr - s->discpos_tot.addr) /
			    FRAME_PER_SEC);
	}

	cur_sec = s->curpos_trk.min * 60 + s->curpos_trk.sec;
	secs = tot_sec - cur_sec;

	return ((secs >= 0) ? secs : 0);
}


/*
 * disc_rtime_norm
 *	Return the remaining time of the disc in seconds.  This is
 *	used during normal playback.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	The disc remaining time in seconds.
 */
STATIC sword32_t
disc_rtime_norm(curstat_t *s)
{
	sword32_t	secs;

	secs = (s->discpos_tot.min * 60 + s->discpos_tot.sec) -
		(s->curpos_tot.min * 60 + s->curpos_tot.sec);

	return ((secs >= 0) ? secs : 0);
}


/*
 * disc_rtime_prog
 *	Return the remaining time of the disc in seconds.  This is
 *	used during shuffle or program mode.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	The disc remaining time in seconds.
 */
STATIC sword32_t
disc_rtime_prog(curstat_t *s)
{
	sword32_t	i,
			secs = 0;

	/* Find the time of all unplayed tracks */
	for (i = s->prog_cnt; i < (int) s->prog_tot; i++) {
		secs += ((s->trkinfo[s->trkinfo[i].playorder + 1].min * 60 +
			 s->trkinfo[s->trkinfo[i].playorder + 1].sec) -
		         (s->trkinfo[s->trkinfo[i].playorder].min * 60 +
			 s->trkinfo[s->trkinfo[i].playorder].sec));
	}

	/* Find the remaining time of the current track */
	for (i = 0; i < MAXTRACK; i++) {
		if (s->trkinfo[i].trkno == LEAD_OUT_TRACK)
			break;

		if (s->trkinfo[i].trkno == s->cur_trk) {
			secs += ((s->trkinfo[i+1].min * 60 +
				  s->trkinfo[i+1].sec) -
				 (s->curpos_tot.min * 60 + s->curpos_tot.sec));

			/* "Enhanced CD" / "CD Extra" hack */
			if (s->trkinfo[i+1].addr > s->discpos_tot.addr) {
				secs -= ((s->trkinfo[i+1].addr -
					  s->discpos_tot.addr) /
					 FRAME_PER_SEC);
			}

			break;
		}
	}

	return ((secs >= 0) ? secs : 0);
}


/*
 * dpy_time_blink
 *	Make the time indicator region of the main window blink.
 *	This is used when the disc is paused.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
STATIC void
dpy_time_blink(curstat_t *s)
{
	static bool_t	bstate = TRUE;

	if (bstate) {
		tm_blinkid = cd_timeout(
			app_data.blinkoff_interval,
			dpy_time_blink,
			(byte_t *) s
		);
		dpy_time(s, TRUE);
	}
	else {
		tm_blinkid = cd_timeout(
			app_data.blinkon_interval,
			dpy_time_blink,
			(byte_t *) s
		);
		dpy_time(s, FALSE);
	}
	bstate = !bstate;
}


/*
 * dpy_ab_blink
 *	Make the a->b indicator of the main window blink.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
STATIC void
dpy_ab_blink(curstat_t *s)
{
	static bool_t	bstate = TRUE;

	if (bstate) {
		ab_blinkid = cd_timeout(
			app_data.blinkoff_interval,
			dpy_ab_blink,
			(byte_t *) s
		);
		dpy_progmode(s, TRUE);
	}
	else {
		ab_blinkid = cd_timeout(
			app_data.blinkon_interval,
			dpy_ab_blink,
			(byte_t *) s
		);
		dpy_progmode(s, FALSE);
	}
	bstate = !bstate;
}


/*
 * dpy_dbmode_blink
 *	Make the dbmode indicator of the main window blink.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
STATIC void
dpy_dbmode_blink(curstat_t *s)
{
	static bool_t	bstate = TRUE;

	if (bstate) {
		dbmode_blinkid = cd_timeout(
			app_data.blinkoff_interval,
			dpy_dbmode_blink,
			(byte_t *) s
		);
		dpy_dbmode(s, TRUE);
	}
	else {
		dbmode_blinkid = cd_timeout(
			app_data.blinkon_interval,
			dpy_dbmode_blink,
			(byte_t *) s
		);
		dpy_dbmode(s, FALSE);
	}
	bstate = !bstate;
}


/*
 * dpy_keypad_ind
 *	Update the digital indicator on the keypad window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dpy_keypad_ind(curstat_t *s)
{
	char		str[24],
			trk[16],
			time[16];
	byte_t		min,
			sec,
			frame;
	XmString	xs;
	static char	prevstr[24];

	if (!XtIsManaged(widgets.keypad.form))
		return;

	switch (keypad_mode) {
	case KPMODE_DISC:
		if (s->mode == MOD_BUSY)
			(void) strcpy(str, "disc    -");
		else if (keystr[0] == '\0')
			(void) sprintf(str, "disc  %3d", s->cur_disc);
		else
			(void) sprintf(str, "( disc  %3d )", atoi(keystr));
		break;

	case KPMODE_TRACK:
		if (keystr[0] == '\0') {
			if (warp_busy) {
				util_blktomsf(
					warp_offset,
					&min,
					&sec,
					&frame,
					0
				);

				(void) sprintf(trk, "%02u", s->cur_trk);

				(void) sprintf(time, "+%02u:%02u", min, sec);
			}
			else if (di_curtrk_pos(s) < 0) {
				(void) strcpy(trk, "--");
				(void) strcpy(time, " --:--");
			}
			else if (curtrk_type(s) == TYP_DATA) {
				(void) sprintf(trk, "%02u", s->cur_trk);
				(void) strcpy(time, " --:--");
			}
			else if (s->cur_idx == 0 && app_data.subq_lba) {
				/* LBA format doesn't have meaningful lead-in
				 * time, so just display blank.
				 */
				(void) sprintf(trk, "%02u", s->cur_trk);
				(void) strcpy(time, "   :  ");
			}
			else {
				util_blktomsf(
					s->curpos_trk.addr,
					&min,
					&sec,
					&frame,
					0
				);
				(void) sprintf(trk, "%02u", s->cur_trk);
				(void) sprintf(time, "%c%02u:%02u",
				       (s->cur_idx == 0) ? '-' : '+', min, sec);
			}

			if (warp_busy)
				(void) sprintf(str, "( %s  %s )", trk, time);
			else
				(void) sprintf(str, "%s  %s", trk, time);
		}
		else {
			util_blktomsf(warp_offset, &min, &sec, &frame, 0);
			(void) sprintf(trk, "%02u", atoi(keystr));
			(void) sprintf(time, "+%02u:%02u", min, sec);

			(void) sprintf(str, "( %s  %s )", trk, time);
		}
		break;

	default:
		return;
	}

	if (strcmp(str, prevstr) == 0) {
		/* No change */
		return;
	}

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.keypad.keypad_ind,
		XmNlabelString,
		xs,
		NULL
	);

	XmStringFree(xs);
	(void) strcpy(prevstr, str);
}


/*
 * dpy_warp
 *	Update the warp slider position.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dpy_warp(curstat_t *s)
{
	int	i;

	if (!XtIsManaged(widgets.keypad.form) || warp_busy)
		return;

	if ((i = di_curtrk_pos(s)) < 0 || s->cur_idx == 0 ||
	    curtrk_type(s) == TYP_DATA)
		set_warp_slider(0, TRUE);
	else
		set_warp_slider(unscale_warp(s, i, s->curpos_trk.addr), TRUE);
}


/*
 * dpy_cddaperf
 *	Update CDDA performance monitor display
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dpy_cddaperf(curstat_t *s)
{
	XmString	xs;
	char		speedstr[16],
			framesstr[16],
			fpsstr[16],
			ttcstr[16];
	static char	prev_speed[16] = { '-', '\0' },
			prev_frames[16] = { '-', '\0' },
			prev_fps[16] = { '-', '\0' },
			prev_ttc[16] = { '0', 'h', 'r', ' ',
					 '-', '-', ':', '-', '-', '\0' };

	if (PLAYMODE_IS_STD(app_data.play_mode)) {
		(void) strcpy(speedstr, "-");
		(void) strcpy(framesstr, "-");
		(void) strcpy(fpsstr, "-");
		(void) strcpy(ttcstr, "0hr --:--");
	}
	else if (s->mode == MOD_PLAY || s->mode == MOD_PAUSE ||
		 s->mode == MOD_SAMPLE) {
		int	fps,
			ttc,
			hour,
			min,
			sec;

		if (s->tot_frm > 0) {
			fps = s->frm_per_sec;

			/* Special hack for 1.0x situation to keep the
			 * display steady and accurate.  This is to work
			 * around sound card sampling rate tolerances.
			 */
			if (fps >= (FRAME_PER_SEC-1) &&
			    fps <= (FRAME_PER_SEC+1)) {
				fps = FRAME_PER_SEC;
			}

			if (fps > 0)
				ttc = (int) (s->tot_frm - s->frm_played) / fps;
			else
				ttc = 0;

			hour = ttc / 3600;
			ttc %= 3600;
			min = ttc / 60;
			sec = ttc % 60;

			(void) sprintf(speedstr, "%0.1f",
				    (float) fps / (float) FRAME_PER_SEC);
			(void) sprintf(framesstr, "%d   (%d%%)",
				    (int) s->frm_played,
				    (int) (s->frm_played * 100 / s->tot_frm));
			(void) sprintf(fpsstr, "%d", fps);
			(void) sprintf(ttcstr, "%dhr %02d:%02d",
				    hour, min, sec);
		}
		else {
			(void) strcpy(speedstr, "0.0");
			(void) strcpy(framesstr, "0   (0%)");
			(void) strcpy(fpsstr, "-");
			(void) strcpy(ttcstr, "0hr --:--");
		}
	}
	else {
		(void) strcpy(speedstr, "-");
		(void) strcpy(framesstr, "-");
		(void) strcpy(fpsstr, "-");
		(void) strcpy(ttcstr, "0hr --:--");
	}

	if (!XtIsManaged(widgets.perfmon.form))
		return;

	if (strcmp(prev_speed, speedstr) != 0) {
		xs = create_xmstring(
			speedstr, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaSetValues(widgets.perfmon.speed_ind,
			XmNlabelString, xs,
			NULL
		);
		XmStringFree(xs);
		(void) strcpy(prev_speed, speedstr);
	}

	if (strcmp(prev_frames, framesstr) != 0) {
		xs = create_xmstring(
			framesstr, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaSetValues(widgets.perfmon.frames_ind,
			XmNlabelString, xs,
			NULL
		);
		XmStringFree(xs);
		(void) strcpy(prev_frames, framesstr);
	}

	if (strcmp(prev_fps, fpsstr) != 0) {
		xs = create_xmstring(
			fpsstr, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaSetValues(widgets.perfmon.fps_ind,
			XmNlabelString, xs,
			NULL
		);
		XmStringFree(xs);
		(void) strcpy(prev_fps, fpsstr);
	}

	if (strcmp(prev_ttc, ttcstr) != 0) {
		xs = create_xmstring(
			ttcstr, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaSetValues(widgets.perfmon.ttc_ind,
			XmNlabelString, xs,
			NULL
		);
		XmStringFree(xs);
		(void) strcpy(prev_ttc, ttcstr);
	}
}


/*
 * set_btn_color
 *	Set the label color of a pushbutton widget
 *
 * Args:
 *	w - The pushbutton widget.
 *	px - The label pixmap, if applicable.
 *	color - The pixel value of the desired color.
 *
 * Return:
 *	Nothing.
 */
STATIC void
set_btn_color(Widget w, Pixmap px, Pixel color)
{
	unsigned char	labtype;

	XtVaGetValues(w, XmNlabelType, &labtype, NULL);

	if (labtype == XmPIXMAP)
		XtVaSetValues(w, XmNlabelPixmap, px, NULL);
	else
		XtVaSetValues(w, XmNforeground, color, NULL);
}


/*
 * set_scale_color
 *	Set the indicator color of a scale widget
 *
 * Args:
 *	w - The scale widget.
 *	pixmap - not used.
 *	color - The pixel value of the desired color.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
set_scale_color(Widget w, Pixmap px, Pixel color)
{
	XtVaSetValues(w, XmNforeground, color, NULL);
}


/*
 * cd_pause_blink
 *	Disable or enable the time indicator blinking.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	enable - TRUE: start blink, FALSE: stop blink
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_pause_blink(curstat_t *s, bool_t enable)
{
	static bool_t	blinking = FALSE;

	if (enable) {
		if (!blinking) {
			/* Start time display blink */
			blinking = TRUE;
			dpy_time_blink(s);
		}
	}
	else if (blinking) {
		/* Stop time display blink */
		cd_untimeout(tm_blinkid);

		tm_blinkid = -1;
		blinking = FALSE;
	}
}


/*
 * cd_ab_blink
 *	Disable or enable the a->b indicator blinking.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	enable - TRUE: start blink, FALSE: stop blink
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_ab_blink(curstat_t *s, bool_t enable)
{
	static bool_t	blinking = FALSE;

	if (enable) {
		if (!blinking) {
			/* Start A->B display blink */
			blinking = TRUE;
			dpy_ab_blink(s);
		}
	}
	else if (blinking) {
		/* Stop A->B display blink */
		cd_untimeout(ab_blinkid);

		ab_blinkid = -1;
		blinking = FALSE;
	}
}


/*
 * cd_dbmode_blink
 *	Disable or enable the dbmode indicator blinking.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	enable - TRUE: start blink, FALSE: stop blink
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_dbmode_blink(curstat_t *s, bool_t enable)
{
	static bool_t	blinking = FALSE;

	if (enable) {
		if (!blinking) {
			/* Start dbmode display blink */
			blinking = TRUE;
			dpy_dbmode_blink(s);
		}
	}
	else if (blinking) {
		/* Stop A->B display blink */
		cd_untimeout(dbmode_blinkid);

		dbmode_blinkid = -1;
		blinking = FALSE;
	}
}


/*
 * do_chgdisc
 *	Timer function to change discs on a multi-CD changer.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
do_chgdisc(curstat_t *s)
{
	/* Reset timer ID */
	chgdisc_dlyid = -1;
	chgdelay = FALSE;

	/* Change to watch cursor */
	cd_busycurs(TRUE, CURS_ALL);

	/* Do the disc change */
	di_chgdisc(s);

	/* Update display */
	dpy_dbmode(s, FALSE);
	dpy_playmode(s, FALSE);
	dpy_progmode(s, FALSE);

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);
}


/*
 * cdda_fadeout
 *	Timer function to perform CDDA fade-out
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_fadeout(curstat_t *s)
{
	if (s->cdda_att == 0) {
		cdda_fader_mode = CDDA_FADER_NONE;
		return;
	}

	if (s->cdda_att > 50)
		s->cdda_att -= 10;
	else if (s->cdda_att > 30)
		s->cdda_att -= 5;
	else if (s->cdda_att > 10)
		s->cdda_att -= 2;
	else if (s->cdda_att > 4)
		s->cdda_att -= 1;
	else if (s->cdda_att >= 1)
		s->cdda_att -= 1;

	set_att_slider(s->cdda_att);
	cdda_att(s);

	if (s->cdda_att > 0) {
		cdda_fader_id = cd_timeout(
			cdda_fader_intvl,
			cdda_fadeout,
			(byte_t *) s
		);
	}
	else {
		cdda_fader_id = -1;
		cdda_fader_mode = CDDA_FADER_NONE;
	}
}


/*
 * cdda_fadein
 *	Timer function to perform CDDA fade-in
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_fadein(curstat_t *s)
{
	if (s->cdda_att == 100) {
		cdda_fader_mode = CDDA_FADER_NONE;
		return;
	}

	if (s->cdda_att < 10)
		s->cdda_att += 1;
	else if (s->cdda_att < 30)
		s->cdda_att += 2;
	else if (s->cdda_att < 50)
		s->cdda_att += 5;
	else if (s->cdda_att < 90)
		s->cdda_att += 10;
	else
		s->cdda_att = 100;

	set_att_slider(s->cdda_att);
	cdda_att(s);

	if (s->cdda_att < 100) {
		cdda_fader_id = cd_timeout(
			cdda_fader_intvl,
			cdda_fadein,
			(byte_t *) s
		);
	}
	else {
		cdda_fader_id = -1;
		cdda_fader_mode = CDDA_FADER_NONE;
	}
}


/*
 * cd_tooltip_popdown
 *	Pop-down the tool-tip.
 *
 * Args:
 *	w - The tooltip shell.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_tooltip_popdown(Widget w)
{
	/* Cancel pending timers */
	if (tooltip1_id >= 0) {
		cd_untimeout(tooltip1_id);
		tooltip1_id = -1;
	}
	if (tooltip2_id >= 0) {
		cd_untimeout(tooltip2_id);
		tooltip2_id = -1;
	}

	/* Pop down the tooltip */
	if (tooltip_active) {
		tooltip_active = FALSE;
		XtPopdown(w);
	}
}


/*
 * cd_tooltip_sphandler
 *	Special widget-specific handler for tool-tips
 *
 * Args:
 *	w - The associated control widget
 *	ret_tlbl - the return XmString label that will be displayed on the
 *		   tool-tip.  The caller should XmStringFree() this when
 *		   done.
 *
 * Return:
 *	0: Don't pop-up tool-tip
 *	1: Pop-up tooltip with the returned ret_tlbl
 *	2: This is not a special widget: use the default handler.
 */
STATIC int
cd_tooltip_sphandler(Widget w, XmString *ret_tlbl)
{
	char		*ttitle,
			*artist,
			*title,
			dtitle[TITLEIND_LEN];
	XmString	xs1,
			xs2,
			xs3;
	curstat_t	*s = curstat_addr();

	if (w == widgets.main.disc_ind) {
		/* Disc display */
		xs1 = create_xmstring(
			"Disc ", NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaGetValues(w, XmNlabelString, &xs2, NULL);
		*ret_tlbl = XmStringConcat(xs1, xs2);
		XmStringFree(xs1);
		XmStringFree(xs2);
	}
	else if (w == widgets.main.track_ind) {
		/* Track display */
		xs1 = create_xmstring(
			"Track ", NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaGetValues(w, XmNlabelString, &xs2, NULL);
		*ret_tlbl = XmStringConcat(xs1, xs2);
		XmStringFree(xs1);
		XmStringFree(xs2);
	}
	else if (w == widgets.main.index_ind) {
		/* Index display */
		xs1 = create_xmstring(
			"Index ", NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaGetValues(w, XmNlabelString, &xs2, NULL);
		*ret_tlbl = XmStringConcat(xs1, xs2);
		XmStringFree(xs1);
		XmStringFree(xs2);
	}
	else if (w == widgets.main.time_ind) {
		/* Time display */
		xs1 = create_xmstring(
			"Time ", NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaGetValues(widgets.main.timemode_ind,
			XmNlabelString, &xs2,
			NULL
		);
		xs3 = XmStringConcat(xs1, xs2);
		XmStringFree(xs1);
		XmStringFree(xs2);

		switch (geom_main_getmode()) {
		case MAIN_NORMAL:
			/* Normal mode */
			*ret_tlbl = xs3;
			break;

		case MAIN_BASIC:
			/* Basic mode */

			/* Add disc title */
			artist = dbprog_curartist(s);
			title = dbprog_curtitle(s);
			if (artist == NULL && title == NULL) {
				/* No disc artist and title */
				*ret_tlbl = xs3;
				break;
			}

			dtitle[0] = '\0';
			if (artist != NULL && artist[0] != '\0') {
				(void) sprintf(dtitle, "%.127s", artist);
				if (title != NULL && title[0] != '\0')
					(void) sprintf(dtitle, "%s / %.127s",
							dtitle, title);
			}
			else if (title != NULL && title[0] != '\0') {
				(void) sprintf(dtitle, "%.127s", title);
			}
			dtitle[TITLEIND_LEN - 1] = '\0';

			xs1 = XmStringSeparatorCreate();
			xs2 = XmStringConcat(xs3, xs1);
			XmStringFree(xs1);
			XmStringFree(xs3);

			xs1 = create_xmstring(
				dtitle,
				NULL,
				XmSTRING_DEFAULT_CHARSET,
				TRUE
			);
			xs3 = XmStringConcat(xs2, xs1);
			XmStringFree(xs1);
			XmStringFree(xs2);

			/* Add track title */
			ttitle = dbprog_curttitle(s);
			if (ttitle[0] == '\0') {
				/* No track title */
				*ret_tlbl = xs3;
			}
			else {
				xs1 = XmStringSeparatorCreate();
				xs2 = XmStringConcat(xs3, xs1);
				XmStringFree(xs1);
				XmStringFree(xs3);

				xs1 = create_xmstring(
					ttitle,
					NULL,
					XmSTRING_DEFAULT_CHARSET,
					TRUE
				);
				xs3 = XmStringConcat(xs2, xs1);
				XmStringFree(xs1);
				XmStringFree(xs2);

				*ret_tlbl = xs3;
			}
			break;
		}
	}
	else if (w == widgets.main.rptcnt_ind) {
		xs1 = create_xmstring(
			"Repeat count: ", NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaGetValues(w, XmNlabelString, &xs2, NULL);
		*ret_tlbl = XmStringConcat(xs1, xs2);
		XmStringFree(xs1);
		XmStringFree(xs2);
	}
	else if (w == widgets.main.dbmode_ind) {
		if (s->qmode == QMODE_MATCH) {
			xs1 = create_xmstring(
				"CD info source: ", NULL,
				XmSTRING_DEFAULT_CHARSET, FALSE
			);
			XtVaGetValues(w, XmNlabelString, &xs2, NULL);
			*ret_tlbl = XmStringConcat(xs1, xs2);
			XmStringFree(xs1);
			XmStringFree(xs2);
		}
		else if (s->qmode == QMODE_NONE) {
			*ret_tlbl = create_xmstring(
				"CD info: none", NULL,
				XmSTRING_DEFAULT_CHARSET, FALSE
			);
		}
		else {
			xs1 = create_xmstring(
				"CD info status: ", NULL,
				XmSTRING_DEFAULT_CHARSET, FALSE
			);
			XtVaGetValues(w, XmNlabelString, &xs2, NULL);
			*ret_tlbl = XmStringConcat(xs1, xs2);
			XmStringFree(xs1);
			XmStringFree(xs2);
		}
	}
	else if (w == widgets.main.progmode_ind) {
		char	*str;

		if (s->program && !s->onetrk_prog && !s->shuffle)
			str = "Mode: program on";
		else if (s->segplay == SEGP_A)
			str = "Mode: a->?";
		else if (s->segplay == SEGP_AB)
			str = "Mode: a->b";
		else
			str = "Mode: program off";

		*ret_tlbl = create_xmstring(
			str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
	}
	else if (w == widgets.main.timemode_ind) {
		xs1 = create_xmstring(
			"Time display mode: ", NULL,
			XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaGetValues(w, XmNlabelString, &xs2, NULL);
		*ret_tlbl = XmStringConcat(xs1, xs2);
		XmStringFree(xs1);
		XmStringFree(xs2);
	}
	else if (w == widgets.main.playmode_ind) {
		char	str[STR_BUF_SZ];

		(void) strcpy(str, "Playback mode ");

		if (PLAYMODE_IS_STD(app_data.play_mode)) {
			(void) strcat(str, "(Standard): ");
		}
		else {
			(void) strcat(str, "(CDDA ");
			if ((app_data.play_mode & PLAYMODE_CDDA) != 0)
			    (void) strcat(str, "play");
			if ((app_data.play_mode & PLAYMODE_FILE) != 0) {
			    if ((app_data.play_mode & PLAYMODE_CDDA) != 0)
				(void) strcat(str, "/save");
			    else
				(void) strcat(str, "save");
			}
			if ((app_data.play_mode & PLAYMODE_PIPE) != 0) {
			    if ((app_data.play_mode & PLAYMODE_CDDA) != 0 ||
				(app_data.play_mode & PLAYMODE_FILE) != 0)
				(void) strcat(str, "/pipe");
			    else
				(void) strcat(str, "pipe");
			}
			(void) strcat(str, "): ");
		}

		xs1 = create_xmstring(
			str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaGetValues(w, XmNlabelString, &xs2, NULL);
		*ret_tlbl = XmStringConcat(xs1, xs2);
		XmStringFree(xs1);
		XmStringFree(xs2);
	}
	else if (w == widgets.main.dtitle_ind) {
		artist = dbprog_curartist(s);
		title = dbprog_curtitle(s);
		if (artist == NULL && title == NULL) {
			/* No artist / title: don't popup tooltip */
			return 0;
		}
		/* Use label string */
		XtVaGetValues(w, XmNlabelString, ret_tlbl, NULL);
	}
	else if (w == widgets.main.ttitle_ind) {
		ttitle = dbprog_curttitle(s);
		if (ttitle[0] == '\0') {
			/* No track title: don't popup tooltip */
			return 0;
		}
		/* Use label string */
		XtVaGetValues(w, XmNlabelString, ret_tlbl, NULL);
	}
	else
		return 2;

	return 1;
}


/*
 * cd_tooltip_popup
 *	Timer function to pop-up the tool-tip.
 *
 * Args:
 *	w - The associated control widget
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_tooltip_popup(Widget w)
{
	Display			*display;
	int			screen;
	Position		end_x,
				end_y,
				abs_x,
				abs_y,
				x,
				y;
	Dimension		width,
				height,
				off_x,
				off_y,
				swidth,
				sheight;
	WidgetClass		wc;
	XtWidgetGeometry	geom;
	XmString		tlbl;

	tooltip1_id = -1;
	tooltip_active = TRUE;
	display = XtDisplay(widgets.toplevel);
	screen = DefaultScreen(display);
	swidth = (Dimension) XDisplayWidth(display, screen);
	sheight = (Dimension) XDisplayHeight(display, screen);

	XtVaGetValues(w,
		XmNwidth, &width,
		XmNheight, &height,
		NULL
	);

	/* Perform widget-specific handling */
	switch (cd_tooltip_sphandler(w, &tlbl)) {
	case 0:
		/* tooltip popup refused by cd_tooltip_sphandler */
		tooltip_active = FALSE;
		return;
	case 1:
		/* cd_tooltip_sphandler handled it */
		break;
	default:
		wc = XtClass(w);

		if (wc == xmPushButtonWidgetClass ||
		    wc == xmCascadeButtonWidgetClass ||
		    wc == xmToggleButtonWidgetClass) {
			/* Use label string */
			XtVaGetValues(w, XmNlabelString, &tlbl, NULL);
		}
		else if (wc == xmScaleWidgetClass) {
			/* Use title string */
			XtVaGetValues(w, XmNtitleString, &tlbl, NULL);
		}
		else {
			/* Use widget name */
			tlbl = create_xmstring(
				XtName(w), NULL,
				XmSTRING_DEFAULT_CHARSET, FALSE
			);
		}
		break;
	}

	/* Set the tooltip label string and hotkey mnemonic, if any. */
	XtVaSetValues(widgets.tooltip.tooltip_lbl, XmNlabelString, tlbl, NULL);
	hotkey_tooltip_mnemonic(w);

	/* Translate to screen absolute coordinates, and add desired offsets */
	XtTranslateCoords(w, 0, 0, &abs_x, &abs_y);

	/* Make sure that the tooltip window doesn't go beyond
	 * screen boundaries.
	 */
	(void) XtQueryGeometry(widgets.tooltip.tooltip_lbl, NULL, &geom);

	off_x = width / 2;
	end_x = abs_x + geom.width + off_x + 2;
	if (end_x > (Position) swidth)
		off_x = -(end_x - swidth - off_x);
	x = abs_x + off_x;
	if (x < 0)
		x = 2;

	off_y = height + 5;
	end_y = abs_y + geom.height + off_y + 2;
	if (end_y > (Position) sheight)
		off_y = -(geom.height + 5);
	y = abs_y + off_y;

	/* Move tooltip widget to desired location and pop it up */
	XtVaSetValues(widgets.tooltip.shell, XmNx, x, XmNy, y, NULL);
	XtPopup(widgets.tooltip.shell, XtGrabNone);

	/* Set timer for auto-popdown */
	if (app_data.tooltip_time > 0) {
		int	n;

		/* If the tooltip label is longer than 40 characters,
		 * add 50mS to the tooltip time for each additional
		 * character.
		 */
		n = XmStringLength(tlbl);
		if (n <= 40)
			n = 0;
		else
			n = ((n - 40) + 1) * 50;

		tooltip2_id = cd_timeout(
			app_data.tooltip_time + n,
			cd_tooltip_popdown,
			(byte_t *) widgets.tooltip.shell
		);
	}

	XmStringFree(tlbl);
}


/*
 * cd_keypad_ask_dsbl
 *	Prompt the user whether to disable the shuffle or program modes
 *	if the user tries to use the keypad to change the track/disc.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	func - The callback function to call if the user answers yes, and
 *             after the shuffle or program mode is disabled.
 *	w - The widget that normally activates the specified callback function
 *	call_data - The callback structure pointer
 *	call_data_len - The callback structure size
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_keypad_ask_dsbl(
	curstat_t	*s,
	XtCallbackProc	func,
	Widget		w,
	XtPointer	call_data,
	int		call_data_len
)
{
	char	*str;

	override_sav.func = func;
	override_sav.w = w;
	override_sav.client_data = (XtPointer) s;
	override_sav.call_data = (XtPointer) MEM_ALLOC(
		"override_sav.call_data",
		call_data_len
	);
	if (override_sav.call_data == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	memcpy(override_sav.call_data, call_data, call_data_len);

	str = (char *) MEM_ALLOC(
		"ask_dsbl_str",
		strlen(app_data.str_kpmodedsbl) +
		strlen(app_data.str_askproceed) + 20
	);
	if (str == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	(void) sprintf(str, app_data.str_kpmodedsbl,
		       s->shuffle ? "shuffle" : "program");
	(void) sprintf(str, "%s\n%s", str, app_data.str_askproceed);

	(void) cd_confirm_popup(
		app_data.str_confirm,
		str,
		(XtCallbackProc) cd_keypad_dsbl_modes_yes,
		(XtPointer) s,
		(XtCallbackProc) cd_keypad_dsbl_modes_no,
		(XtPointer) s
	);

	MEM_FREE(str);
}


/*
 * cd_show_path
 *	Pop up a dialog box and display the title and file path.  On
 *	UNIX, if the path is a relative path, it will be converted into
 *	an absolute path for display.
 *
 * Args:
 *	title - Title string
 *	path  - Path string
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_show_path(char *title, char *path)
{
	char		*cp = NULL,
			*dpypath,
			cwd[FILE_PATH_SZ * 2] = { '\0' };
	static char	*prevpath = NULL;

	if (!util_newstr(&cp, path)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

#ifndef __VMS
	/* Convert to absolute path if appropriate */
	if (*cp != '/') {
		if (strcmp(cp, CUR_DIR) == 0)
			*cp = '\0';

#if defined(BSDCOMPAT) && defined(USE_GETWD)
		if (getwd(cwd) == NULL)
#else
		if (getcwd(cwd, FILE_PATH_SZ * 2) == NULL)
#endif
		{
			MEM_FREE(cp);
			CD_FATAL(app_data.str_longpatherr);
			return;
		}
	}
#endif

	dpypath = (char *) MEM_ALLOC("dpypath", strlen(cp) + strlen(cwd) + 8);
	if (dpypath == NULL) {
		MEM_FREE(cp);
		CD_FATAL(app_data.str_nomemory);
		return;
	}
#ifndef __VMS
	(void) sprintf(dpypath, "%s%s%s",
			cwd, (*cp == '/' || *cp == '\0') ? "" : "/", cp);
#else
	(void) strcpy(dpypath, cp);
#endif

	MEM_FREE(cp);

	if (prevpath != NULL && strcmp(prevpath, dpypath) == 0) {
		/* No change */
		MEM_FREE(dpypath);
		return;
	}

	DBGPRN(DBG_GEN)(errfp, "\n%s\n%s\n", title, dpypath);
	CD_INFO2(title, dpypath);

	prevpath = dpypath;
}


/*
 * cd_mkdirs
 *	Called at startup time to create some needed directories, if
 *	they aren't already there.
 *	Currently these are:
 *		$HOME/.xmcdcfg
 *		$HOME/.xmcdcfg/prog
 *		/tmp/.cdaudio
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_mkdirs(void)
{
	char		*errmsg,
			*homepath,
			path[FILE_PATH_SZ + 16];
	struct stat	stbuf;
#ifndef __VMS
	pid_t		cpid;
	waitret_t	wstat;
#endif

#ifndef NOMKTMPDIR
	errmsg = (char *) MEM_ALLOC(
		"errmsg",
		strlen(app_data.str_tmpdirerr) + strlen(TEMP_DIR)
	);
	if (errmsg == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	/* Make temporary directory, if needed */
	(void) sprintf(errmsg, app_data.str_tmpdirerr, TEMP_DIR);
	if (util_dirstat(TEMP_DIR, &stbuf, TRUE) < 0) {
		if (!util_mkdir(TEMP_DIR, 0777)) {
			CD_FATAL(errmsg);
			return;
		}
	}
	else if (!S_ISDIR(stbuf.st_mode)) {
		CD_FATAL(errmsg);
		return;
	}

	MEM_FREE(errmsg);
#endif	/* NOMKTMPDIR */

#ifndef __VMS
	switch (cpid = FORK()) {
	case -1:
		/* fork failed */
		DBGPRN(DBG_GEN)(errfp,
				"cd_mkdirs: fork failed (errno=%d)\n",
				errno);
		return;

	case 0:
		/* Child process */
		popup_ok = FALSE;

		/* Force uid and gid to original setting */
		if (!util_set_ougid())
			_exit(1);

		break;

	default:
		/* Parent: wait for child to finish */
		(void) util_waitchild(cpid, app_data.srv_timeout + 5,
				      NULL, 0, FALSE, &wstat);
		return;
	}
#endif

	homepath = util_homedir(util_get_ouid());
	if ((int) strlen(homepath) >= FILE_PATH_SZ) {
		CD_FATAL(app_data.str_longpatherr);
		return;
	}

	/* Create the per-user config directory */
	(void) sprintf(path, USR_CFG_PATH, homepath);
	if (util_dirstat(path, &stbuf, TRUE) < 0) {
		if (errno == ENOENT) {
			if (util_mkdir(path, 0755)) {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: created directory %s\n",
					path
				);
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: cannot mkdir %s\n",
					path
				);
			}
		}
		else {
			DBGPRN(DBG_GEN)(errfp,
				"cd_mkdirs: cannot stat %s\n", path);
		}
	}
	else if (!S_ISDIR(stbuf.st_mode)) {
		DBGPRN(DBG_GEN)(errfp,
			"cd_mkdirs: %s is not a directory.\n", path);
	}

	/* Create the per-user track program directory */
	(void) sprintf(path, USR_PROG_PATH, homepath);
#ifdef __VMS
	(void) sprintf(path, "%s%c", path, DIR_END);
#endif
	if (util_dirstat(path, &stbuf, TRUE) < 0) {
		if (errno == ENOENT) {
			if (util_mkdir(path, 0755)) {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: created directory %s\n",
					path
				);
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: cannot mkdir %s\n",
					path
				);
			}
		}
		else {
			DBGPRN(DBG_GEN)(errfp,
				"cd_mkdirs: cannot stat %s\n", path);
		}
	}
	else if (!S_ISDIR(stbuf.st_mode)) {
		DBGPRN(DBG_GEN)(errfp,
			"cd_mkdirs: %s is not a directory.\n", path);
	}

#ifndef __VMS
	_exit(0);
	/*NOTREACHED*/
#endif
}


/*
 * cd_strcpy
 *	Similar to strcpy(3), but skips some punctuation characters.
 *
 * Args:
 *	tgt - Target string buffer
 *	src - Source string buffer
 *
 * Return:
 *	The number of characters copied
 */
STATIC size_t
cd_strcpy(char *tgt, char *src)
{
	size_t	n = 0;
	bool_t	prev_space = FALSE;

	if (src == NULL)
		return 0;

	for (; *src != '\0'; src++) {
		switch (*src) {
		case '\'':
		case '"':
		case ',':
		case ':':
		case ';':
		case '|':
			/* Skip some punctuation characters */
			break;

		case ' ':
		case '\t':
		case '/':
		case '\\':
#ifdef __VMS
		case '[':
		case ']':
		case '<':
		case '>':
		case '(':
		case ')':
		case '{':
		case '}':
#endif
			/* Substitute these with underscores */
			if (!prev_space) {
				*tgt++ = app_data.subst_underscore ? '_' : ' ';
				n++;
				prev_space = TRUE;
			}
			break;

		default:
#ifdef __VMS
			if (!IS_LEGAL_VMSFILECHAR(*src) && *src != '.') {
				/* Skip illegal characters */
				break;
			}
#else
			if (!isprint((int) *src)) {
				/* Skip unprintable characters */
				break;
			}
#endif

			*tgt++ = *src;
			n++;
			prev_space = FALSE;
			break;
		}
	}
	*tgt = '\0';
	return (n);
}


/*
 * cd_unlink
 *	Unlink the specified file
 *
 * Args:
 *	path - The file path to unlink
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
cd_unlink(char *path)
{
#ifdef __VMS
	if (path == NULL)
		return FALSE;

	if (UNLINK(path) < 0 && errno != ENOENT) {
		DBGPRN(DBG_GEN)(errfp, "unlink failed (errno=%d)\n", errno);
		return FALSE;
	}
	return TRUE;
#else
	pid_t	cpid;
	int	ret,
		wstat;
	bool_t	success;

	if (path == NULL)
		return FALSE;

	switch (cpid = FORK()) {
	case -1:
		DBGPRN(DBG_GEN)(errfp,
				"cd_unlink: fork failed (errno=%d)\n",
				errno);
		success = FALSE;
		break;

	case 0:
		/* Child process */
		popup_ok = FALSE;

		/* Force uid and gid to original setting */
		if (!util_set_ougid())
			_exit(1);

		DBGPRN(DBG_GEN)(errfp, "Unlinking [%s]\n", path);

		if ((ret = UNLINK(path)) != 0 && errno == ENOENT)
			ret = 0;

		if (ret != 0) {
			DBGPRN(DBG_GEN)(errfp,
					"unlink failed (errno=%d)\n",
					errno);
		}
		_exit((int) (ret != 0));
		/*NOTREACHED*/

	default:
		/* Parent: wait for child to finish */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			       NULL, 0, FALSE, &wstat);
		if (ret < 0) {
			DBGPRN(DBG_GEN)(errfp,
					"waitpid failed (errno=%d)\n", errno);
			success = FALSE;
		}
		else if (WIFEXITED(wstat)) {
			if ((ret = WEXITSTATUS(wstat)) != 0) {
				DBGPRN(DBG_GEN)(errfp,
					"unlink child exited (status=%d)\n",
					ret);
			}
			success = (bool_t) (ret == 0);
		}
		else if (WIFSIGNALED(wstat)) {
			DBGPRN(DBG_GEN)(errfp,
					"unlink child killed (signal=%d)\n",
					WTERMSIG(wstat));
			success = FALSE;
		}
		else {
			DBGPRN(DBG_GEN)(errfp, "unlink child error\n");
			success = FALSE;
		}
		break;
	}

	return (success);
#endif
}


#ifdef __VMS

#define VMS_MAX_FILECOMP	39	/* Max component length in VMS */

/*
 * cd_vms_fixfname
 *	VMS only allows a max of 39 characters per file path name
 *	component, so truncate things if needed.
 *
 * Args:
 *	buf - File path name buffer
 *
 * Return:
 *	Nothing.
 */
STATIC void
cd_vms_fixfname(char *buf)
{
	char	*tmpbuf,
		*p,
		*q;
	int	i;

	if ((tmpbuf = (char *) MEM_ALLOC("tmpbuf", strlen(buf) + 1)) == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	for (i = 0, p = buf, q = tmpbuf; *p != '\0'; i++, p++, q++) {
		if (i < VMS_MAX_FILECOMP) {
			*q = *p;
			if (!IS_LEGAL_VMSFILECHAR(*p))
				i = -1;
		}
		else {
			/*
			 * File name component truncation:
			 * Advance to the next component separator
			 */
			while (*p != '\0' && IS_LEGAL_VMSFILECHAR(*p))
				p++;

			if (*p == '\0')
				break;
			*q = *p;
			i = -1;
		}
	}
	*q = '\0';

	(void) strcpy(buf, tmpbuf);

	MEM_FREE(tmpbuf);
}
#endif	/* __VMS */


/*
 * cd_mkfname
 *	Construct a file name based on the supplied template, and append
 *	it to the target string.
 *
 * Args:
 *	s      - Pointer to the curstat_t structure
 *	trkidx - Track index number (0-based), or -1 if not applicable
 *	tgt    - The target string
 *	tmpl   - The template string
 *	tgtlen - Target string buffer size
 *
 * Returns:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
cd_mkfname(curstat_t *s, int trkidx, char *tgt, char *tmpl, int tgtlen)
{
	int		len,
			n,
			remain;
	char		*cp,
			*cp2,
			*p,
			tmp[(STR_BUF_SZ * 2) + 4];
	bool_t		err;
	cdinfo_incore_t	*dbp;

	dbp = dbprog_curdb(s);

	err = FALSE;
	n = 0;
	len = strlen(tgt);
	cp = tgt + len;

	for (cp2 = tmpl; *cp2 != '\0'; cp2++, cp += n, len += n) {
		if ((remain = (tgtlen - len)) <= 0)
			break;

		switch (*cp2) {
		case '%':
			switch (*(++cp2)) {
			case 'X':
				n = strlen(PROGNAME);
				if (n < remain)
					(void) strcpy(cp, PROGNAME);
				else
					err = TRUE;
				break;

			case 'V':
				n = strlen(VERSION_MAJ) +
				    strlen(VERSION_MAJ) + 1;
				if (n < remain)
					(void) sprintf(cp, "%s.%s",
							VERSION_MAJ,
							VERSION_MIN);
				else
					err = TRUE;
				break;

			case 'N':
				p = util_loginname();
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case '~':
				p = util_homedir(util_get_ouid());
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'H':
				p = util_get_uname()->nodename;
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'L':
				p = app_data.libdir;
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'S':
				p = app_data.libdir;
				n = strlen(p) + strlen("discog") + 1;
				if (n < remain) {
					(void) strcpy(cp, p);
#ifdef __VMS
					(void) strcat(cp, ".discog");
#else
					(void) strcat(cp, "/discog");
#endif
				}
				else
					err = TRUE;
				break;

			case 'C':
				p = cdinfo_genre_path(dbp->disc.genre);
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'I':
				n = 8;
				if (n < remain)
					(void) sprintf(cp, "%08x",
							dbp->discid);
				else
					err = TRUE;
				break;

			case 'A':
			case 'a':
				p = dbp->disc.artist;
				if (p == NULL)
					p = "artist";
				if (*cp2 == 'a') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CD_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cd_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'a')
					MEM_FREE(p);
				break;

			case 'D':
			case 'd':
				p = dbp->disc.title;
				if (p == NULL)
					p = "title";
				if (*cp2 == 'd') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CD_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cd_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'd')
					MEM_FREE(p);
				break;

			case 'R':
			case 'r':
				p = dbp->track[trkidx].artist;
				if (p == NULL)
					p = "trackartist";
				if (*cp2 == 'r') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CD_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cd_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'r')
					MEM_FREE(p);
				break;

			case 'T':
			case 't':
				p = dbp->track[trkidx].title;
				if (p == NULL)
					p = "track";
				if (*cp2 == 't') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CD_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cd_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 't')
					MEM_FREE(p);
				break;

			case 'B':
			case 'b':
				(void) sprintf(tmp, "%.127s%s%.127s",
					(dbp->disc.artist == NULL) ?
						"artist" : dbp->disc.artist,
					(dbp->disc.artist != NULL &&
					 dbp->disc.title != NULL) ?
						"-" : "",
					(dbp->disc.title == NULL) ?
						"title" : dbp->disc.title);
				p = tmp;
				if (*cp2 == 'b') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CD_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cd_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'b')
					MEM_FREE(p);
				break;

			case '#':
				n = 2;
				if (n < remain)
					(void) sprintf(cp, "%02d",
						s->trkinfo[trkidx].trkno);
				else
					err = TRUE;
				break;

			case '%':
			case ' ':
			case '\t':
			case '\'':
			case '"':
			case ',':
			case ';':
			case ':':
			case '|':
			case '\\':
				/* Skip some punctuation characters */
				n = 1;
				if (n < remain)
					*cp = '%';
				else
					err = TRUE;
				break;

			default:
				n = 2;
				if (n < remain)
					*cp = '%';
				else
					err = TRUE;
				break;
			}
			break;

		case ' ':
		case '\t':
#ifdef __VMS
		case '<':
		case '>':
		case '(':
		case ')':
		case '{':
		case '}':
#endif
			n = 1;
			if (n < remain) {
				if (app_data.subst_underscore)
					*cp = '_';
				else
					*cp = ' ';
			}
			else
				err = TRUE;
			break;

#ifndef __VMS
		case ';':
#endif
		case '\'':
		case '"':
		case ',':
		case '|':
		case '\\':
			/* Skip some punctuation characters */
			n = 0;
			break;

#ifdef __VMS
		/* Allow these because they are special in VMS file names */
		case '$':
		case '[':
		case ']':
		case '.':
		case ':':
		case ';':
		case '-':
		case '_':
			n = 1;
			if (n < remain)
				*cp = *cp2;
			else
				err = TRUE;
			break;
#endif

		default:
#ifdef __VMS
			if (!isalnum((int) *cp2)) {
				/* Skip illegal characters */
				n = 0;
				break;
			}
#else
			if (!isprint((int) *cp2)) {
				/* Skip unprintable characters */
				n = 0;
				break;
			}
#endif

			n = 1;
			if (n < remain)
				*cp = *cp2;
			else
				err = TRUE;
			break;
		}

		if (err)
			break;
	}
	*cp = '\0';

	if (err) {
		CD_INFO(app_data.str_longpatherr);
		return FALSE;
	}

#ifdef __VMS
	/* Check file path and truncate its components if needed */
	cd_vms_fixfname(tgt);
#endif

	return TRUE;
}


/*
 * cd_mkoutpath
 *	Construct audio output file names and check for collision
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return
 *	TRUE  - passed
 *	FALSE - failed
 */
STATIC bool_t
cd_mkoutpath(curstat_t *s)
{
	char			*str,
				*dpath,
				*path;
	int			i,
				j;
	struct stat		stbuf;
	bool_t			ret;

	if (s->outf_tmpl == NULL) {
		CD_INFO(app_data.str_noaudiopath);
		return FALSE;
	}

	dpath = NULL;

	if (app_data.cdda_trkfile) {
		/* Normal play, program, shuffle, sample and
		 * segment play modes
		 */
		for (i = 0; i < (int) s->tot_trks; i++) {
			if (s->program) {
				bool_t	inprog;

				inprog = FALSE;
				for (j = 0; j < (int) s->prog_tot; j++) {
					if (i == s->trkinfo[j].playorder) {
						inprog = TRUE;
						break;
					}
				}

				if (!inprog)
					/* This track is not in the program */
					continue;
			}
			else if (s->segplay == SEGP_AB) {
				bool_t		inseg;
				sword32_t	sot,
						eot;

				inseg = FALSE;
				sot = s->trkinfo[i].addr;
				eot = s->trkinfo[i+1].addr;

				/* "Enhanced CD" / "CD Extra" hack */
				if (eot > s->discpos_tot.addr)
					eot = s->discpos_tot.addr;

				if (s->bp_startpos_tot.addr >= sot &&
				    s->bp_startpos_tot.addr < eot) {
					/* The segment spans the beginning
					 * of this track
					 */
					inseg = TRUE;
				}
				else if (s->bp_endpos_tot.addr >= sot &&
					 s->bp_endpos_tot.addr < eot) {
					/* The segment spans the end
					 * of this track
					 */
					inseg = TRUE;
				}
				else if (s->bp_startpos_tot.addr >= sot &&
					 s->bp_endpos_tot.addr < eot) {
					/* The segment is contained within
					 * this track
					 */
					inseg = TRUE;
				}

				if (!inseg)
					continue;
			}
			else if (!s->shuffle &&
				 s->cur_trk > s->trkinfo[i].trkno) {
				continue;
			}

			path = (char *) MEM_ALLOC("path", FILE_PATH_SZ);
			if (path == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return FALSE;
			}
			path[0] = '\0';

			if (!cd_mkfname(s, i, path, s->outf_tmpl,
					FILE_PATH_SZ)) {
				MEM_FREE(path);
				return FALSE;
			}

			if (!util_newstr(&s->trkinfo[i].outfile, path)) {
				MEM_FREE(path);
				CD_FATAL(app_data.str_nomemory);
				return FALSE;
			}

			if (dpath == NULL &&
			    (dpath = util_dirname(path)) == NULL) {
				MEM_FREE(path);
				CD_FATAL("File dirname error");
				return FALSE;
			}

			if (LSTAT(path, &stbuf) < 0) {
				MEM_FREE(path);
				continue;
			}

			if (S_ISREG(stbuf.st_mode)) {
				char	*fname;

				if ((fname = util_basename(path)) == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return FALSE;
				}

				str = (char *) MEM_ALLOC("str",
					strlen(app_data.str_overwrite) +
					strlen(fname) + 4
				);
				if (str == NULL) {
					MEM_FREE(fname);
					CD_FATAL(app_data.str_nomemory);
					return FALSE;
				}

				(void) sprintf(str, "%s\n\n%s",
						fname,
						app_data.str_overwrite);

				ret = cd_confirm_popup(
					app_data.str_confirm,
					str,
					(XtCallbackProc) NULL,
					NULL,
					(XtCallbackProc) NULL,
					NULL
				);

				MEM_FREE(str);
				MEM_FREE(fname);

				if (ret)
					ret = cd_unlink(path);

				if (!ret) {
					if (s->onetrk_prog) {
						s->onetrk_prog = FALSE;
						dbprog_progclear(s);
					}

					MEM_FREE(path);
					return FALSE;
				}
			}
			else {
				str = (char *) MEM_ALLOC("str",
					strlen(app_data.str_audiopathexists) +
					strlen(path) + 4
				);
				if (str == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return FALSE;
				}

				(void) sprintf(str, "%s\n\n%s",
						path,
						app_data.str_audiopathexists);

				CD_INFO(str);

				MEM_FREE(str);
				MEM_FREE(path);
				return FALSE;
			}

			MEM_FREE(path);
		}
	}
	else {
		path = (char *) MEM_ALLOC("path", FILE_PATH_SZ);
		if (path == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		path[0] = '\0';

		if (!cd_mkfname(s, -1, path, s->outf_tmpl, FILE_PATH_SZ)) {
			MEM_FREE(path);
			return FALSE;
		}

		if (!util_newstr(&s->trkinfo[0].outfile, path)) {
			MEM_FREE(path);
			CD_FATAL(app_data.str_nomemory);
			return FALSE;
		}

		if (LSTAT(path, &stbuf) == 0) {
			if (S_ISREG(stbuf.st_mode)) {
				char	*fname;

				if ((fname = util_basename(path)) == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return FALSE;
				}

				str = (char *) MEM_ALLOC("str",
					strlen(app_data.str_overwrite) +
					strlen(fname) + 4
				);
				if (str == NULL) {
					MEM_FREE(fname);
					CD_FATAL(app_data.str_nomemory);
					return FALSE;
				}

				(void) sprintf(str, "%s\n\n%s",
					       fname, app_data.str_overwrite);

				ret = cd_confirm_popup(
					app_data.str_confirm,
					str,
					(XtCallbackProc) NULL,
					NULL,
					(XtCallbackProc) NULL,
					NULL
				);

				MEM_FREE(str);
				MEM_FREE(fname);

				if (ret)
					ret = cd_unlink(path);

				if (!ret) {
					if (s->onetrk_prog) {
						s->onetrk_prog = FALSE;
						dbprog_progclear(s);
					}

					MEM_FREE(path);
					return FALSE;
				}
			}
			else {
				str = (char *) MEM_ALLOC("str",
					strlen(app_data.str_audiopathexists) +
					strlen(path) + 4
				);
				if (str == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return FALSE;
				}

				(void) sprintf(str, "%s\n\n%s",
					       path,
					       app_data.str_audiopathexists);

				CD_INFO(str);

				MEM_FREE(str);
				MEM_FREE(path);
				return FALSE;
			}
		}

		if ((dpath = util_dirname(path)) == NULL) {
			MEM_FREE(path);
			CD_FATAL("File dirname error");
			return FALSE;
		}

		MEM_FREE(path);
	}

	/* Show output directory path in pop-up info window */
	cd_show_path(app_data.str_outdir, dpath);

	MEM_FREE(dpath);

	return TRUE;
}


/*
 * cd_ckpipeprog
 *	Construct pipe to program path and check for validity
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return
 *	TRUE  - passed
 *	FALSE - failed
 */
/*ARGSUSED*/
STATIC bool_t
cd_ckpipeprog(curstat_t *s)
{
	char	*str;

	if (app_data.pipeprog == NULL) {
		CD_INFO(app_data.str_noprog);
		return FALSE;
	}

	if (!util_checkcmd(app_data.pipeprog)) {
		str = (char *) MEM_ALLOC("str",
			strlen(app_data.pipeprog) +
			strlen(app_data.str_cannotinvoke) + 8
		);
		if (str == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return FALSE;
		}

		(void) sprintf(str, app_data.str_cannotinvoke,
				app_data.pipeprog);
		CD_INFO(str);

		MEM_FREE(str);
		return FALSE;
	}

	return TRUE;
}


/***********************
 *   public routines   *
 ***********************/


/*
 * fix_outfile_path
 *	Fix the CDDA audio output file path to make sure there is a proper
 *	suffix based on the file format.  Also, replace white spaces in
 *	the file path string with underscores.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
fix_outfile_path(curstat_t *s)
{
	filefmt_t	*fmp;
	char		*suf,
			*cp,
			tmp[FILE_PATH_SZ * 2 + 8];

	/* File suffix */
	if ((fmp = cdda_filefmt(app_data.cdda_filefmt)) == NULL)
		suf = "";
	else
		suf = fmp->suf;

	if (s->outf_tmpl == NULL) {
		/* No default outfile, set it */
		(void) sprintf(tmp,
#ifdef __VMS
			"%%S.%%C.%%I]%s%s",
#else
			"%%S/%%C/%%I/%s%s",
#endif
			app_data.cdda_trkfile ? FILEPATH_TRACK : FILEPATH_DISC,
			suf
		);

		if (!util_newstr(&s->outf_tmpl, tmp)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
	}
	else if ((cp = strrchr(s->outf_tmpl, '.')) != NULL) {
		char	*cp2 = strrchr(s->outf_tmpl, DIR_END);

		if (cp2 == NULL || cp > cp2) {
			/* Change suffix if necessary */
			if (strcmp(cp, suf) != 0) {
				*cp = '\0';
				(void) strncpy(tmp, s->outf_tmpl,
					    FILE_PATH_SZ - strlen(suf) - 1);
				(void) strcat(tmp, suf);

				if (!util_newstr(&s->outf_tmpl, tmp)) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}
			}
		}
		else {
			/* No suffix, add one */
			(void) strncpy(tmp, s->outf_tmpl,
					FILE_PATH_SZ - strlen(suf) - 1);
			(void) strcat(tmp, suf);

			if (!util_newstr(&s->outf_tmpl, tmp)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
		}
	}
	else {
		/* No suffix, add one */
		(void) strncpy(tmp, s->outf_tmpl,
				FILE_PATH_SZ - strlen(suf) - 1);
		(void) strcat(tmp, suf);

		if (!util_newstr(&s->outf_tmpl, tmp)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
	}
}


/*
 * curtrk_type
 *	Return the track type of the currently playing track.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	TYP_AUDIO or TYP_DATA.
 */
byte_t
curtrk_type(curstat_t *s)
{
	sword32_t	i;

	if ((i = di_curtrk_pos(s)) >= 0)
		return (s->trkinfo[i].type);

	return TYP_AUDIO;
}


/*
 * dpy_disc
 *	Update the disc number display region of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_disc(curstat_t *s)
{
	XmString	xs;
	char		str[8];
	static char	prev[8] = { '\0' };

	(void) sprintf(str, "%u", s->cur_disc);

	if (strcmp(str, prev) == 0)
		/* No change, just return */
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.main.disc_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	/* Update the keypad indicator */
	dpy_keypad_ind(s);

	(void) strcpy(prev, str);
}


/*
 * dpy_track
 *	Update the track number display region of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_track(curstat_t *s)
{
	XmString	xs;
	char		str[4];
	static char	prev[4] = { '\0' };
	static int	sav_trk = -1;


	if (s->cur_trk != sav_trk) {
		/* Update CD Info/program window current track display */
		dbprog_curtrkupd(s);
		/* Update main window track title display */
		dpy_ttitle(s);
		/* Update the keypad indicator */
		dpy_keypad_ind(s);
		/* Update warp slider */
		dpy_warp(s);
	}

	sav_trk = s->cur_trk;

	if (s->cur_trk < 0 || s->mode == MOD_BUSY ||
		 s->mode == MOD_NODISC)
		(void) strcpy(str, "--");
	else if (s->time_dpy == T_REMAIN_DISC) {
		if (s->shuffle || s->program) {
			if (s->prog_tot >= s->prog_cnt)
				(void) sprintf(str, "-%u",
					       s->prog_tot - s->prog_cnt);
			else
				(void) strcpy(str, "-0");
		}
		else
			(void) sprintf(str, "-%u",
				       s->tot_trks - di_curtrk_pos(s) - 1);
	}
	else
		(void) sprintf(str, "%02u", s->cur_trk);

	if (strcmp(str, prev) == 0 && !mode_chg)
		/* No change, just return */
		return;

	xs = create_xmstring(
		str, NULL,
		(geom_main_getmode() == MAIN_NORMAL) ? CHSET1 : CHSET2,
		FALSE
	);

	XtVaSetValues(
		widgets.main.track_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strcpy(prev, str);

	/* Update CDDA save-to-file expanded path display if appropriate */
	if (pathexp_active && app_data.cdda_trkfile) {
		cd_filepath_exp(
			widgets.options.mode_pathexp_btn,
			(XtPointer) s,
			NULL
		);
	}
}


/*
 * dpy_index
 *	Update the index number display region of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_index(curstat_t *s)
{
	XmString	xs;
	char		str[4];
	static char	prev[4] = { '\0' };

	if (s->cur_idx <= 0 || s->mode == MOD_BUSY ||
	    s->mode == MOD_NODISC || s->mode == MOD_STOP)
		(void) strcpy(str, "--");
	else
		(void) sprintf(str, "%02u", s->cur_idx);

	if (strcmp(str, prev) == 0)
		/* No change, just return */
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.main.index_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strcpy(prev, str);
}


/*
 * dpy_time
 *	Update the time display region of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	blank - Whether the display region should be blanked.
 *
 * Return:
 *	Nothing
 */
void
dpy_time(curstat_t *s, bool_t blank)
{
	sword32_t	time_sec;
	XmString	xs;
	char		str[16];
	static char	prev[16] = { 'j', 'u', 'n', 'k', '\0' };

	if (blank)
		str[0] = '\0';
	else if (s->mode == MOD_BUSY)
		(void) strcpy(str, app_data.str_busy);
	else if (s->mode == MOD_NODISC)
		(void) strcpy(str, app_data.str_nodisc);
	else if (curtrk_type(s) == TYP_DATA)
		(void) strcpy(str, app_data.str_data);
	else if (s->mode == MOD_STOP)
		(void) strcpy(str, " --:--");
	else {
		switch (s->time_dpy) {
		case T_ELAPSED_TRACK:
			if (app_data.subq_lba && s->cur_idx == 0)
				/* LBA format doesn't have meaningful lead-in
				 * time, so just display blank.
				 */
				(void) strcpy(str, "   :  ");
			else
				(void) sprintf(str, "%s%02u:%02u",
					       (s->cur_idx == 0) ? "-" : "+",
					       s->curpos_trk.min,
					       s->curpos_trk.sec);
			break;

		case T_ELAPSED_SEG:
			if (s->segplay != SEGP_AB) {
				(void) strcpy(str, "   :  ");
				break;
			}
			time_sec = (s->curpos_tot.addr -
				    s->bp_startpos_tot.addr) / FRAME_PER_SEC;
			(void) sprintf(str, "+%02u:%02u",
				       time_sec / 60, time_sec % 60);
			break;

		case T_ELAPSED_DISC:
			if (s->shuffle || s->program) {
				if (s->cur_idx == 0) {
					(void) strcpy(str, "   :  ");
					break;
				}
				else
					time_sec = disc_etime_prog(s);
			}
			else
				time_sec = disc_etime_norm(s);

			(void) sprintf(str, "+%02u:%02u",
				       time_sec / 60, time_sec % 60);
			break;

		case T_REMAIN_TRACK:
			if (s->cur_idx == 0)
				(void) strcpy(str, "   :  ");
			else {
				time_sec = track_rtime(s);
				(void) sprintf(str, "-%02u:%02u",
					       time_sec / 60, time_sec % 60);
			}
			break;

		case T_REMAIN_SEG:
			if (s->segplay != SEGP_AB) {
				(void) strcpy(str, "   :  ");
				break;
			}
			time_sec = (s->bp_endpos_tot.addr -
				    s->curpos_tot.addr) / FRAME_PER_SEC;
			(void) sprintf(str, "-%02u:%02u",
				       time_sec / 60, time_sec % 60);
			break;

		case T_REMAIN_DISC:
			if (s->shuffle || s->program) {
				if (s->cur_idx == 0) {
					(void) strcpy(str, "   :  ");
					break;
				}
				else
					time_sec = disc_rtime_prog(s);
			}
			else
				time_sec = disc_rtime_norm(s);

			(void) sprintf(str, "-%02u:%02u",
				       time_sec / 60, time_sec % 60);
			break;

		default:
			(void) strcpy(str, "??:??");
			break;
		}
	}

	if (s->mode == MOD_PAUSE)
		cd_pause_blink(s, TRUE);
	else
		cd_pause_blink(s, FALSE);

	/* Update the keypad indicator */
	dpy_keypad_ind(s);

	/* Update warp slider */
	dpy_warp(s);

	/* Display CDDA performance */
	dpy_cddaperf(s);

	if (strcmp(str, prev) == 0 && !mode_chg)
		/* No change: just return */
		return;

	xs = create_xmstring(
		str, NULL,
		(geom_main_getmode() == MAIN_NORMAL) ? CHSET1 : CHSET2,
		FALSE
	);

	XtVaSetValues(
		widgets.main.time_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);
	(void) strcpy(prev, str);
}


/*
 * dpy_dtitle
 *	Update the disc title display region of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_dtitle(curstat_t *s)
{
	XmString	xs;
	char		*artist,
			*title,
			str[TITLEIND_LEN],
			icontitle[TITLEIND_LEN + 16];
	static char	prev[TITLEIND_LEN];

	str[0] = '\0';
	if (!chgdelay) {
		artist = dbprog_curartist(s);
		title = dbprog_curtitle(s);
		if (artist != NULL) {
			(void) sprintf(str, "%.127s", artist);
			if (title != NULL)
				(void) sprintf(str, "%s / %.127s", str, title);
		}
		else if (title != NULL)
			(void) sprintf(str, "%.127s", title);

		str[TITLEIND_LEN - 1] = '\0';
	}

	if (strcmp(str, prev) == 0)
		/* No change: just return */
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);

	XtVaSetValues(
		widgets.main.dtitle_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strcpy(prev, str);

	/* Update icon title */
	if (str[0] == '\0')
		(void) strcpy(icontitle, PROGNAME);
	else {
		chset_conv_t	*up;
		char		*a = NULL;

		/* Convert from UTF-8 to local charset if possible */
		if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
			return;

		if (!util_chset_conv(up, str, &a, FALSE) &&
		    !util_newstr(&a, str)) {
			util_chset_close(up);
			return;
		}

		util_chset_close(up);

#ifdef USE_SGI_DESKTOP
		(void) strcpy(icontitle, a);
#else
		(void) sprintf(icontitle, "%s: %s", PROGNAME, a);
#endif

		MEM_FREE(a);
	}

	XtVaSetValues(widgets.toplevel,
		XmNiconName, icontitle,
		NULL
	);
}


/*
 * dpy_ttitle
 *	Update the track title display region of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_ttitle(curstat_t *s)
{
	XmString	xs;
	char		str[TITLEIND_LEN];
	static char	prev[TITLEIND_LEN];

	if (chgdelay)
		str[0] = '\0';
	else {
		(void) strncpy(str, dbprog_curttitle(s), TITLEIND_LEN - 1);
		str[TITLEIND_LEN - 1] = '\0';
	}

	if (strcmp(str, prev) == 0)
		/* No change: just return */
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);

	XtVaSetValues(
		widgets.main.ttitle_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strcpy(prev, str);
}


/*
 * dpy_rptcnt
 *	Update the repeat count indicator of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_rptcnt(curstat_t *s)
{
	XmString		xs;
	char			str[16];
	static char		prevstr[16];

	if (s->repeat && (s->mode == MOD_PLAY || s->mode == MOD_PAUSE))
		(void) sprintf(str, "%u", s->rptcnt);
	else
		(void) strcpy(str, "-");

	if (strcmp(str, prevstr) == 0)
		/* No change */
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.main.rptcnt_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strcpy(prevstr, str);
}


/*
 * dpy_dbmode
 *	Update the cdinfo indicator of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	blank - Whether the indicator should be blanked
 *
 * Return:
 *	Nothing
 */
void
dpy_dbmode(curstat_t *s, bool_t blank)
{
	cdinfo_incore_t	*dbp;
	char		*str;
	XmString	xs;
	static char	prev[16] = { 'j', 'u', 'n', 'k', '\0' };

	dbp = dbprog_curdb(s);

	if (blank)
		str = "";
	else {
		switch (s->qmode) {
		case QMODE_MATCH:
			if (dbp->flags & CDINFO_FROMLOC)
				str = app_data.str_local;
			else if (dbp->flags & CDINFO_FROMCDT)
				str = app_data.str_cdtext;
			else
				str = app_data.str_cddb;
			break;
		case QMODE_WAIT:
			str = app_data.str_query;
			break;
		case QMODE_ERR:
			str = app_data.str_error;
			break;
		case QMODE_NONE:
		default:
			str = "";
			break;
		}
	}

	if (strcmp(prev, str) == 0)
		/* No change: just return */
		return;

	if (str == app_data.str_cddb) {
		char		cddb_str[8];
		XmString	xs1,
				xs2;

		(void) strcpy(cddb_str, "CDDB");
		if (cdinfo_cddb_ver() == 2) {
			xs1 = create_xmstring(cddb_str, NULL, CHSET1, FALSE);
			cddb_str[0] = (char) '\262';	/* like <SUP>2</SUP> */
			cddb_str[1] = '\0';
			xs2 = create_xmstring(cddb_str, NULL, CHSET2, FALSE);
			xs = XmStringConcat(xs1, xs2);
			XmStringFree(xs1);
			XmStringFree(xs2);
		}
		else
			xs = create_xmstring(cddb_str, NULL, CHSET1, FALSE);
	}
	else if (str == app_data.str_cdtext)
		xs = create_xmstring(str, NULL, CHSET1, FALSE);
	else
		xs = create_xmstring(str, NULL, CHSET2, FALSE);

	XtVaSetValues(
		widgets.main.dbmode_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strncpy(prev, str, sizeof(prev) - 1);
	prev[sizeof(prev) - 1] = '\0';

	if (s->qmode == QMODE_WAIT)
		cd_dbmode_blink(s, TRUE);
	else
		cd_dbmode_blink(s, FALSE);
}


/*
 * dpy_progmode
 *	Update the prog indicator of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	blank - Whether the indicator should be blanked
 *
 * Return:
 *	Nothing
 */
void
dpy_progmode(curstat_t *s, bool_t blank)
{
	XmString	xs;
	char		*str;
	bool_t		state;
	static char	prev[16] = { 'j', 'u', 'n', 'k', '\0' };

	state = (bool_t) (s->program && !s->onetrk_prog && !s->shuffle);

	if (blank)
		str = "";
	else {
		if (s->segplay == SEGP_A)
			str = "a->?";
		else if (s->segplay == SEGP_AB)
			str = "a->b";
		else if (state)
			str = app_data.str_progmode;
		else
			str = "";
	}

	if (strcmp(prev, str) == 0)
		/* No change: just return */
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.main.progmode_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strncpy(prev, str, sizeof(prev) - 1);
	prev[sizeof(prev) - 1] = '\0';

	if (s->segplay == SEGP_NONE && strcmp(prev, "a->b") == 0)
		/* Cancel a->b mode in segments window */
		dbprog_segments_cancel(s);

	/* Set segments window set/clear button sensitivity */
	dbprog_segments_setmode(s);

	if (s->segplay == SEGP_A)
		cd_ab_blink(s, TRUE);
	else
		cd_ab_blink(s, FALSE);

}


/*
 * dpy_timemode
 *	Update the time mode indicator of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_timemode(curstat_t *s)
{
	String		str;
	XmString	xs;
	static byte_t	prev = 0xff;

	if (prev == s->time_dpy)
		/* No change: just return */
		return;

	switch (s->time_dpy) {
	case T_ELAPSED_TRACK:
		str = app_data.str_elapse;
		break;

	case T_ELAPSED_SEG:
		str = app_data.str_elapseseg;
		break;

	case T_ELAPSED_DISC:
		str = app_data.str_elapsedisc;
		break;

	case T_REMAIN_TRACK:
		str = app_data.str_remaintrk;
		break;

	case T_REMAIN_SEG:
		str = app_data.str_remainseg;
		break;

	case T_REMAIN_DISC:
		str = app_data.str_remaindisc;
		break;
	default:
		/* Invalid mode */
		return;
	}

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.main.timemode_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	prev = s->time_dpy;
}


/*
 * dpy_playmode
 *	Update the play mode indicator of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	blank - Whether the indicator should be blanked
 *
 * Return:
 *	Nothing
 */
void
dpy_playmode(curstat_t *s, bool_t blank)
{
	char		*str;
	XmString	xs;
	bool_t		expbtn_sens = FALSE;
	static char	prev[64] = { 'j', 'u', 'n', 'k', '\0' };

	if (blank)
		str = "";
	else {
		switch (s->mode) {
		case MOD_PLAY:
			str = app_data.str_play;
			if ((app_data.play_mode & PLAYMODE_FILE) != 0)
				expbtn_sens = TRUE;
			break;
		case MOD_PAUSE:
			str = app_data.str_pause;
			if ((app_data.play_mode & PLAYMODE_FILE) != 0)
				expbtn_sens = TRUE;
			break;
		case MOD_SAMPLE:
			str = app_data.str_sample;
			if ((app_data.play_mode & PLAYMODE_FILE) != 0)
				expbtn_sens = TRUE;
			break;
		case MOD_STOP:
			str = app_data.str_ready;
			break;
		default:
			str = "";
			break;
		}
	}

	if (strcmp(prev, str) == 0)
		/* No change: just return */
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.main.playmode_ind,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);

	(void) strncpy(prev, str, sizeof(prev) - 1);
	prev[sizeof(prev) - 1] = '\0';

	/* Set segments window set button sensitivity */
	dbprog_segments_setmode(s);

	/* Set CDDA file path expand button sensitivity */
	XtSetSensitive(
		widgets.options.mode_pathexp_btn,
		(Boolean) expbtn_sens
	);

	/* Pop down CDDA file path expand dialog if appropriate */
	if (pathexp_active && !expbtn_sens)
		cd_info2_popdown(NULL);
}


/*
 * dpy_all
 *	Update all indicator of the main window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing
 */
void
dpy_all(curstat_t *s)
{
	dpy_disc(s);
	dpy_track(s);
	dpy_index(s);
	dpy_time(s, FALSE);
	dpy_dtitle(s);
	dpy_rptcnt(s);
	dpy_dbmode(s, FALSE);
	dpy_progmode(s, FALSE);
	dpy_timemode(s);
	dpy_playmode(s, FALSE);
}


/*
 * set_lock_btn
 *	Set the lock button state
 *
 * Args:
 *	state - TRUE=in, FALSE=out
 *
 * Return:
 *	Nothing.
 */
void
set_lock_btn(bool_t state)
{
	XmToggleButtonSetState(
		widgets.main.lock_btn, (Boolean) state, False
	);
}


/*
 * set_repeat_btn
 *	Set the repeat button state
 *
 * Args:
 *	state - TRUE=in, FALSE=out
 *
 * Return:
 *	Nothing.
 */
void
set_repeat_btn(bool_t state)
{
	XmToggleButtonSetState(
		widgets.main.repeat_btn, (Boolean) state, False
	);
}


/*
 * set_shuffle_btn
 *	Set the shuffle button state
 *
 * Args:
 *	state - TRUE=in, FALSE=out
 *
 * Return:
 *	Nothing.
 */
void
set_shuffle_btn(bool_t state)
{
	XmToggleButtonSetState(
		widgets.main.shuffle_btn, (Boolean) state, False
	);
}


/*
 * set_vol_slider
 *	Set the volume control slider position
 *
 * Args:
 *	val - The value setting.
 *
 * Return:
 *	Nothing.
 */
void
set_vol_slider(int val)
{
	/* Check bounds */
	if (val > 100)
		val = 100;
	if (val < 0)
		val = 0;

	XmScaleSetValue(widgets.main.level_scale, val);
}


/*
 * set_warp_slider
 *	Set the track warp slider position
 *
 * Args:
 *	val - The value setting.
 *	autoupd - This is an auto-update.
 *
 * Return:
 *	Nothing.
 */
void
set_warp_slider(int val, bool_t autoupd)
{
	if (autoupd && (keypad_mode != KPMODE_TRACK || keystr[0] != '\0')) {
		/* User using keypad: no updates */
		return;
	}

	/* Check bounds */
	if (val > 255)
		val = 255;
	if (val < 0)
		val = 0;

	pseudo_warp = TRUE;
	XmScaleSetValue(widgets.keypad.warp_scale, val);
	pseudo_warp = FALSE;
}


/*
 * set_bal_slider
 *	Set the balance control slider position
 *
 * Args:
 *	val - The value setting.
 *
 * Return:
 *	Nothing.
 */
void
set_bal_slider(int val)
{
	/* Check bounds */
	if (val > 50)
		val = 50;
	if (val < -50)
		val = -50;

	XmScaleSetValue(widgets.options.bal_scale, val);
}


/*
 * set_qualval_slider
 *	Set the VBR quality factor slider position
 *
 * Args:
 *	val - The value setting.
 *
 * Return:
 *	Nothing.
 */
void
set_qualval_slider(int val)
{
	XmString	xs,
			xs2;
	char		str[16];

	/* Check bounds */
	if (val > 10)
		val = 10;
	if (val < 1)
		val = 1;

	(void) sprintf(str, ": %d", val);
	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	xs2 = XmStringConcat(xs_qual, xs);
	XtVaSetValues(widgets.options.qualval_lbl,
		XmNlabelString, xs2,
		NULL
	);
	XmStringFree(xs2);
	XmStringFree(xs);

	XmScaleSetValue(widgets.options.qualval_scl, val);
}


/*
 * set_algo_slider
 *	Set the compression algorithm slider position
 *
 * Args:
 *	val - The value setting.
 *
 * Return:
 *	Nothing.
 */
void
set_algo_slider(int val)
{
	XmString	xs,
			xs2;
	char		str[16];

	/* Check bounds */
	if (val > 10)
		val = 10;
	if (val < 1)
		val = 1;

	(void) sprintf(str, ": %d", val);
	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	xs2 = XmStringConcat(xs_algo, xs);
	XtVaSetValues(widgets.options.compalgo_lbl,
		XmNlabelString, xs2,
		NULL
	);
	XmStringFree(xs2);
	XmStringFree(xs);

	XmScaleSetValue(widgets.options.compalgo_scl, val);
}


/*
 * set_att_slider
 *	Set the CDDA attenuator slider position
 *
 * Args:
 *	val - The value setting.
 *
 * Return:
 *	Nothing.
 */
void
set_att_slider(int val)
{
	/* Check bounds */
	if (val > 100)
		val = 100;
	if (val < 0)
		val = 0;

	XmScaleSetValue(widgets.options.cdda_att_scale, val);
}


/*
 * set_text_string
 *	Set the text widget string.
 *
 * Args:
 *	w    - The text widget
 *	str  - The input text string
 *	utf8 - If TRUE, the input string is UTF-8 encoded and will be
 *	       converted to a local charset before setting the widget
 *
 * Return:
 *	Nothing.
 */
void
set_text_string(Widget w, char *str, bool_t utf8)
{
	chset_conv_t	*up;
	char		*a = NULL;

	if (w == (Widget) NULL)
		return;

	if (str == NULL || *str == '\0' || !utf8) {
		XmTextSetString(w, str);
		return;
	}

	/* Convert from UTF-8 encoding */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return;

	/* We set the roe flag of the util_chset_conv() function to
	 * TRUE here because we don't want to cause a failed chset
	 * conversion to corrupt the string, in case the user submits
	 * the entry back to CDDB without editing these fields.
	 * This also avoids a bogus CDINFO_CHANGED flag from being set.
	 */
	if (!util_chset_conv(up, str, &a, TRUE)) {
		util_chset_close(up);
		return;
	}

	util_chset_close(up);

	if (a != NULL) {
		XmTextSetString(w, a);
		MEM_FREE(a);
	}
}


/*
 * get_text_string
 *	Get the text widget string.
 *
 * Args:
 *	w    - The text widget
 *	utf8 - If TRUE, the output string should be UTF-8 encoded and
 *	       a conversion from local charset will be made.
 *
 * Return:
 *	The return string.  This is internally allocated and should be
 *	freed by the caller via MEM_FREE when done.
 */
char *
get_text_string(Widget w, bool_t utf8)
{
	chset_conv_t	*up;
	char		*str,
			*a = NULL;

	if ((str = XmTextGetString(w)) == NULL)
		return NULL;

	if (*str == '\0' || !utf8) {
		if (!util_newstr(&a, str))
			return NULL;
		XtFree(str);
		return (a);
	}

	/* Convert to UTF-8 encoding */
	if ((up = util_chset_open(CHSET_LOCALE_TO_UTF8)) == NULL) {
		if (!util_newstr(&a, str))
			return NULL;
		XtFree(str);
		return (a);
	}

	if (!util_chset_conv(up, str, &a, TRUE)) {
		util_chset_close(up);
		if (!util_newstr(&a, str))
			return NULL;
		XtFree(str);
		return (a);
	}

	util_chset_close(up);
	XtFree(str);

	return (a);
}


/*
 * create_xmstring
 *	Create a Motif compound string from the input text string.
 *
 * Args:
 *	in_text  - The input UTF-8 text string
 *	def_text - The fallback string to use if in_text is NULL or empty
 *	tag      - The tag component to be associated with the text
 *	utf8     - If TRUE, the input string is UTF-8 encoded and will be
 *		   converted to a local charset.
 *
 * Return:
 *	The compound string.  The caller should call XmStringFree on
 *	this when done using it.
 */
XmString
create_xmstring(char *in_text, char *def_text, char *tag, bool_t utf8)
{
	chset_conv_t	*up;
	char		*a = NULL;
	XmString	xs;

	if (in_text == NULL || *in_text == '\0') {
		if (def_text != NULL)
			xs = XmStringCreateSimple(def_text);
		else
			xs = XmStringCreateSimple("");

		return (xs);
	}

	if (!utf8) {
		xs = XmStringCreateLtoR(in_text, tag);
		return (xs);
	}

	/* Convert from UTF-8 to local charset if possible */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL) {
		xs = XmStringCreateLtoR(in_text, tag); 
		return (xs);
	}
	if (!util_chset_conv(up, in_text, &a, FALSE) &&
	    !util_newstr(&a, in_text)) {
		util_chset_close(up);
		xs = XmStringCreateLtoR(in_text, tag); 
		return (xs);
	}
	util_chset_close(up);

	xs = XmStringCreateLtoR(a, tag); 
	MEM_FREE(a);

	return (xs);
}


/*
 * scale_warp
 *	Scale track warp value (0-255) to the number of CD logical audio
 *	blocks.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	pos - Track position.
 *	val - Warp value.
 *
 * Return:
 *	The number of CD logical audio blocks
 */
int
scale_warp(curstat_t *s, int pos, int val)
{
	int		n;
	sword32_t	eot;

	eot = s->trkinfo[pos+1].addr;

	/* "Enhanced CD" / "CD Extra" hack */
	if (eot > s->discpos_tot.addr)
		eot = s->discpos_tot.addr;

	n = val * (eot - s->trkinfo[pos].addr) / 0xff;
	return ((n > 0) ? (n - 1) : n);
}


/*
 * unscale_warp
 *	Scale the number of CD logical audio blocks to track warp
 *	value (0-255).
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	pos - Track position.
 *	val - Number of logical audio blocks.
 *
 * Return:
 *	The warp value
 */
int
unscale_warp(curstat_t *s, int pos, int val)
{
	sword32_t	eot;

	eot = s->trkinfo[pos+1].addr;

	/* "Enhanced CD" / "CD Extra" hack */
	if (eot > s->discpos_tot.addr)
		eot = s->discpos_tot.addr;

	return (val * 0xff / (eot - s->trkinfo[pos].addr));
}


/*
 * cd_timeout
 *	Alarm clock callback facility
 *
 * Args:
 *	msec - When msec milliseconds has elapsed, the callback
 *		occurs.
 *	handler - Pointer to the callback function.
 *	arg - An argument passed to the callback function.
 *
 * Return:
 *	Timeout ID.
 */
long
cd_timeout(word32_t msec, void (*handler)(), byte_t *arg)
{
	/* Note: This code assumes that sizeof(XtIntervalId) <= sizeof(long)
	 * If this is not true then cd_timeout/cd_untimeout will not work
	 * correctly.
	 */
	return ((long)
		XtAppAddTimeOut(
			XtWidgetToApplicationContext(widgets.toplevel),
			(unsigned long) msec,
			(XtTimerCallbackProc) handler,
			(XtPointer) arg
		)
	);
}


/*
 * cd_untimeout
 *	Cancel a pending alarm configured with cd_timeout.
 *
 * Args:
 *	id - The timeout ID
 *
 * Return:
 *	Nothing.
 */
void
cd_untimeout(long id)
{
	/* Note: This code assumes that sizeof(XtIntervalId) <= sizeof(long)
	 * If this is not true then cd_timeout/cd_untimeout will not work
	 * correctly.
	 */
	XtRemoveTimeOut((XtIntervalId) id);
}


/*
 * cd_beep
 *	Beep the workstation speaker.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
void
cd_beep(void)
{
	XBell(XtDisplay(widgets.toplevel), 50);
}


/*
 * cd_dialog_setpos
 *	Set up a dialog box's position to be near the pointer cursor.
 *	This is used before popping up the dialog boxes.
 *
 * Args:
 *	w - The dialog box shell widget to set up
 *
 * Return:
 *	Nothing.
 */
void
cd_dialog_setpos(Widget w)
{
	Display			*display;
	int			screen,
				junk,
				ptr_x,
				ptr_y;
	unsigned int		keybtn;
	Position		win_x,
				win_y;
	Dimension		win_width,
				win_height,
				swidth,
				sheight;
	Window			rootwin,
				childwin;

	display = XtDisplay(widgets.toplevel);
	screen = DefaultScreen(display);
	swidth = (Dimension) XDisplayWidth(display, screen);
	sheight = (Dimension) XDisplayHeight(display, screen);

	/* Get current pointer location */
	XQueryPointer(
		display,
		XtWindow(widgets.toplevel),
		&rootwin, &childwin,
		&ptr_x, &ptr_y,
		&junk, &junk,
		&keybtn
	);

	/* Get dialog window location and dimensions */
	XtVaGetValues(w,
		XmNx, &win_x,
		XmNy, &win_y,
		XmNwidth, &win_width,
		XmNheight, &win_height,
		NULL
	);

	if (win_width < WIN_MINWIDTH)
		win_width = WIN_MINWIDTH;
	if (win_height < WIN_MINHEIGHT)
		win_height = WIN_MINHEIGHT;

	if ((Position) ptr_x > (Position) win_x &&
	    (Position) ptr_x < (Position) (win_x + win_width) &&
	    (Position) ptr_y > (Position) win_y &&
	    (Position) ptr_y < (Position) (win_y + win_height)) {
		/* Pointer cursor is already within the dialog window */
		return;
	}

	/* Set new position for dialog window */
	win_x = (Position) (ptr_x - (win_width / 2));
	win_y = (Position) (ptr_y - (win_height / 2));

	if (win_x < 0)
		win_x = 0;
	else if ((Position) (win_x + win_width + WM_FUDGE_X) >
		 (Position) swidth)
		win_x = (Position) (swidth - win_width - WM_FUDGE_X);

	if (win_y < 0)
		win_y = 0;
	else if ((Position) (win_y + win_height + WM_FUDGE_Y) >
		 (Position) sheight)
		win_y = (Position) (sheight - win_height - WM_FUDGE_Y);

	XtVaSetValues(w, XmNx, win_x, XmNy, win_y, NULL);
}


/*
 * cd_info_popup
 *	Pop up the information message dialog box.
 *
 * Args:
 *	title - The title bar text string.
 *	msg - The information message text string.
 *
 * Return:
 *	Nothing.
 */
void
cd_info_popup(char *title, char *msg)
{
	XmString	xs;

	if (!popup_ok) {
		(void) fprintf(errfp, "%s %s:\n%s\n", PROGNAME, title, msg);
		return;
	}

	/* Remove pending popdown timeout, if any */
	if (infodiag_id >= 0) {
		cd_untimeout(infodiag_id);
		infodiag_id = -1;
	}

	/* Set the dialog box title */
	xs = create_xmstring(title, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dialog.info, XmNdialogTitle, xs, NULL);
	XmStringFree(xs);

	/* Set the dialog box message */
	xs = create_xmstring(msg, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);
	XtVaSetValues(widgets.dialog.info, XmNmessageString, xs, NULL);
	XmStringFree(xs);

	if (!XtIsManaged(widgets.dialog.info)) {
		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.dialog.info));

		/* Pop up the info dialog */
		XtManageChild(widgets.dialog.info);
	}
	else {
		/* Raise the window to top of stack */
		XRaiseWindow(
			XtDisplay(widgets.dialog.info),
			XtWindow(XtParent(widgets.dialog.info))
		);
	}
}


/*
 * cd_info_popup_auto
 *	Pop up the information message dialog box, which will auto-popdown
 *	in 5 seconds.
 *
 * Args:
 *	title - The title bar text string.
 *	msg - The information message text string.
 *
 * Return:
 *	Nothing.
 */
void
cd_info_popup_auto(char *title, char *msg)
{
	cd_info_popup(title, msg);

	infodiag_id = cd_timeout(
		5000,	/* popup interval */
		cd_info_popdown,
		NULL
	);
}


/*
 * cd_info_popdown
 *	Pop down the information message dialog box.
 *
 * Args:
 *	p - Not used at this time.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
cd_info_popdown(byte_t *p)
{
	if (XtIsManaged(widgets.dialog.info))
		XtUnmanageChild(widgets.dialog.info);

	infodiag_id = -1;
}


/*
 * cd_info2_popup
 *	Pop up the information message dialog box.  This dialog box
 *	differs from the cd_info_popup in that it contains an extra
 *	single line text widget.
 *
 * Args:
 *	title - The title bar text string.
 *	msg - The information message text string.
 *	textmsg - The information message string to put in the text widget.
 *
 * Return:
 *	Nothing.
 */
void
cd_info2_popup(char *title, char *msg, char *textmsg)
{
	XmString	xs;

	if (!popup_ok) {
		(void) fprintf(errfp, "%s %s:\n%s\n%s\n",
				PROGNAME, title, msg, textmsg);
		return;
	}

	/* Set the dialog box title */
	xs = create_xmstring(title, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dialog.info2, XmNdialogTitle, xs, NULL);
	XmStringFree(xs);

	/* Set the dialog box message */
	xs = create_xmstring(msg, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);
#if XmVersion > 1001
	XtVaSetValues(widgets.dialog.info2, XmNmessageString, xs, NULL);
#else
	XtVaSetValues(widgets.dialog.info2, XmNselectionLabelString, xs, NULL);
#endif
	XmStringFree(xs);

	/* Set the text field message */
	set_text_string(widgets.dialog.info2_txt, textmsg, TRUE);

	if (!XtIsManaged(widgets.dialog.info2)) {
		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.dialog.info2));

		/* Pop up the info2 dialog */
		XtManageChild(widgets.dialog.info2);
	}
	else {
		/* Raise the window to top of stack */
		XRaiseWindow(
			XtDisplay(widgets.dialog.info2),
			XtWindow(XtParent(widgets.dialog.info2))
		);
	}
}


/*
 * cd_info2_popdown
 *	Pop down the information message dialog box.
 *
 * Args:
 *	p - Not used at this time.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
cd_info2_popdown(byte_t *p)
{
	if (XtIsManaged(widgets.dialog.info2))
		XtUnmanageChild(widgets.dialog.info2);

	pathexp_active = FALSE;
}


/*
 * cd_warning_popup
 *	Pop up the warning message dialog box.
 *
 * Args:
 *	title - The title bar text string.
 *	msg - The warning message text string.
 *
 * Return:
 *	Nothing.
 */
void
cd_warning_popup(char *title, char *msg)
{
	XmString	xs;

	if (!popup_ok) {
		(void) fprintf(errfp, "%s %s:\n%s\n", PROGNAME, title, msg);
		return;
	}

	/* Set the dialog box title */
	xs = create_xmstring(title, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dialog.warning, XmNdialogTitle, xs, NULL);
	XmStringFree(xs);

	/* Set the dialog box message */
	xs = create_xmstring(msg, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);
	XtVaSetValues(widgets.dialog.warning, XmNmessageString, xs, NULL);
	XmStringFree(xs);

	if (!XtIsManaged(widgets.dialog.warning)) {
		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.dialog.warning));

		/* Pop up the warning dialog */
		XtManageChild(widgets.dialog.warning);
	}
	else {
		/* Raise the window to top of stack */
		XRaiseWindow(
			XtDisplay(widgets.dialog.warning),
			XtWindow(XtParent(widgets.dialog.warning))
		);
	}
}


/*
 * cd_fatal_popup
 *	Pop up the fatal error message dialog box.
 *
 * Args:
 *	title - The title bar text string.
 *	msg - The fatal error message text string.
 *
 * Return:
 *	Nothing.
 */
void
cd_fatal_popup(char *title, char *msg)
{
	XmString	xs;

	if (!popup_ok) {
		(void) fprintf(errfp, "%s %s:\n%s\n", PROGNAME, title, msg);
		exit(1);
	}

	/* Make sure that the cursor is normal */
	cd_busycurs(FALSE, CURS_ALL);

	if (!XtIsManaged(widgets.dialog.fatal)) {
		/* Set the dialog box title */
		xs = create_xmstring(
			title, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
		);
		XtVaSetValues(widgets.dialog.fatal, XmNdialogTitle, xs, NULL);
		XmStringFree(xs);

		/* Set the dialog box message */
		xs = create_xmstring(
			msg, NULL, XmSTRING_DEFAULT_CHARSET, TRUE
		);
		XtVaSetValues(widgets.dialog.fatal, XmNmessageString, xs, NULL);
		XmStringFree(xs);

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.dialog.fatal));

		/* Pop up the error dialog */
		XtManageChild(widgets.dialog.fatal);
	}
}


/*
 * cd_confirm_popup
 *	Pop up the user-confirmation message dialog box.  Note that this
 *	facility is not re-entrant, so that the 'yes' and 'no' pushbutton
 *	callback funcs cannot themselves call cd_confirm_popup.  Note that
 *	this function does not return to the caller until a user response
 *	is received.
 *
 * Args:
 *	title - The title bar text string.
 *	msg - The message text string.
 *	f_yes - Pointer to the callback function if user selects OK
 *	a_yes - Argument passed to f_yes
 *	f_no - Pointer to the callback function if user selects Cancel
 *	a_no - Argument passed to f_no
 *
 * Return:
 *	FALSE - The user selected 'No' or simply closed the dialog
 *	TRUE  - The user selected 'Yes'
 */
bool_t
cd_confirm_popup(
	char		*title,
	char		*msg,
	XtCallbackProc	f_yes,
	XtPointer	a_yes,
	XtCallbackProc	f_no,
	XtPointer	a_no
)
{
	Widget		yes_btn,
			no_btn;
	XmString	xs;
	cbinfo_t	*yes_cbinfo,
			*no_cbinfo;
	static bool_t	first = TRUE;

	if (!popup_ok)
		/* Not allowed */
		return FALSE;

	if (XtIsManaged(widgets.dialog.confirm)) {
		/* Already popped up, pop it down and clean up first */
		cd_confirm_popdown();
	}

	/* Set the dialog box title */
	xs = create_xmstring(title, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dialog.confirm, XmNdialogTitle, xs, NULL);
	XmStringFree(xs);

	/* Set the dialog box message */
	xs = create_xmstring(msg, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);
	XtVaSetValues(widgets.dialog.confirm, XmNmessageString, xs, NULL);
	XmStringFree(xs);

	/* Add callbacks */
	yes_btn = XmMessageBoxGetChild(
		widgets.dialog.confirm,
		XmDIALOG_OK_BUTTON
	);
	no_btn = XmMessageBoxGetChild(
		widgets.dialog.confirm,
		XmDIALOG_CANCEL_BUTTON
	);

	if (f_yes != NULL) {
		yes_cbinfo = &cbinfo0;
		yes_cbinfo->widget0 = yes_btn;
		yes_cbinfo->widget1 = no_btn;
		yes_cbinfo->widget2 = (Widget) NULL;
		yes_cbinfo->type = XmNactivateCallback;
		yes_cbinfo->func = f_yes;
		yes_cbinfo->data = a_yes;

		register_activate_cb(yes_btn, f_yes, a_yes);
		register_activate_cb(yes_btn, cd_rmcallback, yes_cbinfo);
		register_activate_cb(no_btn, cd_rmcallback, yes_cbinfo);
	}

	if (f_no != NULL) {
		no_cbinfo = &cbinfo1;
		no_cbinfo->widget0 = no_btn;
		no_cbinfo->widget1 = yes_btn;
		no_cbinfo->widget2 = XtParent(widgets.dialog.confirm);
		no_cbinfo->type = XmNactivateCallback;
		no_cbinfo->func = f_no;
		no_cbinfo->data = a_no;

		register_activate_cb(no_btn, f_no, a_no);
		register_activate_cb(no_btn, cd_rmcallback, no_cbinfo);
		register_activate_cb(yes_btn, cd_rmcallback, no_cbinfo);

		/* Install WM_DELETE_WINDOW handler */
		add_delw_callback(
			XtParent(widgets.dialog.confirm),
			f_no,
			a_no
		);
	}

	if (first) {
		first = FALSE;
		register_activate_cb(yes_btn, cd_confirm_resp, (XtPointer) 1);
		register_activate_cb(no_btn, cd_confirm_resp, NULL);

		/* Install WM_DELETE_WINDOW handler */
		add_delw_callback(
			XtParent(widgets.dialog.confirm),
			cd_confirm_resp,
			NULL
		);
	}

	/* Set up dialog box position */
	cd_dialog_setpos(XtParent(widgets.dialog.confirm));

	/* Pop up the confirm dialog */
	XtManageChild(widgets.dialog.confirm);

	/* Handle events locally until the confirm window is dismissed.
	 * This is to prevent from returning to the calling function 
	 * until a user response to this dialog is received.
	 */
	confirm_pend = TRUE;
	do {
		util_delayms(10);
		event_loop(0);
	} while (confirm_pend);

	return (confirm_resp);
}


/*
 * cd_confirm_popdown
 *	Pop down the confirm dialog box.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cd_confirm_popdown(void)
{
	cbinfo_t	*yes_cbinfo = &cbinfo0,
			*no_cbinfo = &cbinfo1;

	if (!XtIsManaged(widgets.dialog.confirm))
		/* Not popped up */
		return;

	XtUnmanageChild(widgets.dialog.confirm);

	cd_rmcallback(yes_cbinfo->widget0,
		      (XtPointer) yes_cbinfo, (XtPointer) NULL);
	cd_rmcallback(yes_cbinfo->widget0,
		      (XtPointer) no_cbinfo, (XtPointer) NULL);
	cd_rmcallback(no_cbinfo->widget1,
		      (XtPointer) yes_cbinfo, (XtPointer) NULL);
	cd_rmcallback(no_cbinfo->widget1,
		      (XtPointer) no_cbinfo, (XtPointer) NULL);
}


/*
 * cd_working_popup
 *	Pop up the work-in-progress message dialog box.  Note that this
 *	facility is not re-entrant, so that the 'stop' pushbutton
 *	callback func cannot itself call cd_working_popup.
 *
 * Args:
 *	title - The title bar text string.
 *	msg - The message text string.
 *	f_stop - Pointer to the callback function if user selects Stop
 *	a_stop - Argument passed to f_stop
 *
 * Return:
 *	Nothing.
 */
void
cd_working_popup(
	char		*title,
	char		*msg,
	XtCallbackProc	f_stop,
	XtPointer	a_stop
)
{
	Widget		stop_btn;
	XmString	xs;
	cbinfo_t	*stop_cbinfo;

	if (!popup_ok)
		/* Not allowed */
		return;

	if (XtIsManaged(widgets.dialog.working)) {
		/* Already popped up: pop it down and clean up first */
		cd_working_popdown();
	}

	/* Set the dialog box title */
	xs = create_xmstring(title, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dialog.working, XmNdialogTitle, xs, NULL);
	XmStringFree(xs);

	/* Set the dialog box message */
	xs = create_xmstring(msg, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);
	XtVaSetValues(widgets.dialog.working, XmNmessageString, xs, NULL);
	XmStringFree(xs);

	/* Add callbacks */
	stop_btn = XmMessageBoxGetChild(
		widgets.dialog.working,
		XmDIALOG_CANCEL_BUTTON
	);

	if (f_stop != NULL) {
		stop_cbinfo = &cbinfo2;
		stop_cbinfo->widget0 = stop_btn;
		stop_cbinfo->widget1 = (Widget) NULL;
		stop_cbinfo->widget2 = XtParent(widgets.dialog.working);
		stop_cbinfo->type = XmNactivateCallback;
		stop_cbinfo->func = f_stop;
		stop_cbinfo->data = a_stop;

		register_activate_cb(stop_btn, f_stop, a_stop);
		register_activate_cb(stop_btn, cd_rmcallback, stop_cbinfo);

		/* Install WM_DELETE_WINDOW handler */
		add_delw_callback(
			XtParent(widgets.dialog.working),
			f_stop,
			a_stop
		);
	}

	/* Set up dialog box position */
	cd_dialog_setpos(XtParent(widgets.dialog.working));

	/* Pop up the working dialog */
	XtManageChild(widgets.dialog.working);
}


/*
 * cd_working_popdown
 *	Pop down the working message dialog box.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cd_working_popdown(void)
{
	cbinfo_t	*p = &cbinfo2;

	if (!XtIsManaged(widgets.dialog.working))
		/* Not popped up */
		return;

	XtUnmanageChild(widgets.dialog.working);
	cd_rmcallback(p->widget0, (XtPointer) p, (XtPointer) NULL);
}


/*
 * cd_init
 *	Top level function that initializes all subsystems.  Used on
 *	program startup.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
cd_init(curstat_t *s)
{
	int		i,
			j;
	char		*cp,
			*bdevname,
			*hd;
	char		version[STR_BUF_SZ / 2],
			titlestr[STR_BUF_SZ * 2],
			path[FILE_PATH_SZ * 2];
	struct utsname	*up;

	(void) sprintf(version, "%s.%s", VERSION_MAJ, VERSION_MIN);
	DBGPRN(DBG_ALL)(errfp, "XMCD %s.%s DEBUG MODE\n",
			version, VERSION_TEENY);

	/* app-defaults file check */
	if (app_data.version == NULL || 
	    strncmp(version, app_data.version, strlen(version)) != 0) {
		CD_FATAL(app_data.str_appdef);
		return;
	}

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

	/* Set some defaults */
	app_data.cdinfo_maxhist = 100;
	app_data.aux = (void *) s;

	/* Get system common configuration parameters */
	(void) sprintf(path, SYS_CMCFG_PATH, app_data.libdir);
	di_common_parmload(path, TRUE, FALSE);

	/* Get user common configuration parameters */
	(void) sprintf(path, USR_CMCFG_PATH, hd);
	di_common_parmload(path, FALSE, FALSE);

	/* Paranoia: avoid overflowing buffers */
	if (app_data.device != NULL &&
	    ((int) strlen(app_data.device) >= FILE_PATH_SZ)) {
		CD_FATAL(app_data.str_longpatherr);
		return;
	}

#ifdef __VMS
	bdevname = NULL;
	if (!util_newstr(&bdevname, "device.cfg")) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
#else
	if ((bdevname = util_basename(app_data.device)) == NULL) {
		CD_FATAL("Device path basename error: Aborting.");
		return;
	}
	if ((int) strlen(bdevname) >= FILE_BASE_SZ) {
		MEM_FREE(bdevname);
		CD_FATAL(app_data.str_longpatherr);
		return;
	}
#endif

	/* Initialize CD Information services */
	(void) strcpy(cdinfo_cldata.prog, PROGNAME);
	(void) strcpy(cdinfo_cldata.user, util_loginname());
	cdinfo_cldata.isdemo = di_isdemo;
	cdinfo_cldata.curstat_addr = curstat_addr;
	cdinfo_cldata.fatal_msg = cd_fatal_popup;
	cdinfo_cldata.warning_msg = cd_warning_popup;
	cdinfo_cldata.info_msg = cd_info_popup;
	cdinfo_cldata.info2_msg = cd_info2_popup;
	cdinfo_cldata.workproc = event_loop;
	cdinfo_cldata.arg = 0;
	cdinfo_init(&cdinfo_cldata);


	/* Get system-wide device-specific configuration parameters */
	(void) sprintf(path, SYS_DSCFG_PATH, app_data.libdir, bdevname);
	di_devspec_parmload(path, TRUE, FALSE);

	/* Get user device-specific configuration parameters */
	(void) sprintf(path, USR_DSCFG_PATH, hd, bdevname);
	di_devspec_parmload(path, FALSE, FALSE);

	MEM_FREE(bdevname);

	/* Make some program directories if needed */
	cd_mkdirs();

	/* Initialize help system */
	help_init();

	/* Initialize the CD Information/program subsystem */
	dbprog_init(s);

	/* Initialize the wwwWarp subsystem */
	wwwwarp_init(s);

	/* Initialize the CD interface subsystem */
	di_cldata.curstat_addr = curstat_addr;
	di_cldata.quit = cd_quit;
	di_cldata.timeout = cd_timeout;
	di_cldata.untimeout = cd_untimeout;
	di_cldata.dbclear = dbprog_dbclear;
	di_cldata.dbget = dbprog_dbget;
	di_cldata.progclear = dbprog_progclear;
	di_cldata.progget = dbprog_progget;
	di_cldata.chgr_scan_stop = dbprog_chgr_scan_stop;
	di_cldata.mkoutpath = cd_mkoutpath;
	di_cldata.ckpipeprog = cd_ckpipeprog;
	di_cldata.fatal_msg = cd_fatal_popup;
	di_cldata.warning_msg = cd_warning_popup;
	di_cldata.info_msg = cd_info_popup;
	di_cldata.info2_msg = cd_info2_popup;
	di_cldata.beep = cd_beep;
	di_cldata.set_lock_btn = set_lock_btn;
	di_cldata.set_shuffle_btn = set_shuffle_btn;
	di_cldata.set_vol_slider = set_vol_slider;
	di_cldata.set_bal_slider = set_bal_slider;
	di_cldata.dpy_all = dpy_all;
	di_cldata.dpy_disc = dpy_disc;
	di_cldata.dpy_track = dpy_track;
	di_cldata.dpy_index = dpy_index;
	di_cldata.dpy_time = dpy_time;
	di_cldata.dpy_progmode = dpy_progmode;
	di_cldata.dpy_playmode = dpy_playmode;
	di_cldata.dpy_rptcnt = dpy_rptcnt;
	di_init(&di_cldata);

	/* Set default modes */
	di_repeat(s, app_data.repeat_mode);
	set_repeat_btn(s->repeat);
	di_shuffle(s, app_data.shuffle_mode);
	set_shuffle_btn(s->shuffle);
	keypad_mode = KPMODE_TRACK;
	cdda_fader_mode = CDDA_FADER_NONE;

	/* Set the sensitivity of play mode buttons according to
	 * configuration capabilities
	 */
	if ((di_cldata.capab & CAPAB_RDCDDA) == 0) {
		XtSetSensitive(widgets.options.mode_cdda_btn, False);
		XtSetSensitive(widgets.options.mode_file_btn, False);
		XtSetSensitive(widgets.options.mode_pipe_btn, False);
	}
	else {
		if ((di_cldata.capab & CAPAB_WRDEV) == 0)
			XtSetSensitive(widgets.options.mode_cdda_btn, False);
		if ((di_cldata.capab & CAPAB_WRFILE) == 0)
			XtSetSensitive(widgets.options.mode_file_btn, False);
		if ((di_cldata.capab & CAPAB_WRPIPE) == 0)
			XtSetSensitive(widgets.options.mode_pipe_btn, False);
	}

	/* Play mode and capabilities */
	if ((di_cldata.capab & CAPAB_PLAYAUDIO) == 0) {
		(void) fprintf(errfp,
			       "No CD playback capability.  Aborting.\n");
		cd_quit(s);
		return;
	}

	/* Set optional file formats menu entries sensitivity */
	if (!cdda_filefmt_supp(FILEFMT_MP3))
		XtSetSensitive(widgets.options.mode_fmt_mp3_btn, False);
	if (!cdda_filefmt_supp(FILEFMT_OGG))
		XtSetSensitive(widgets.options.mode_fmt_ogg_btn, False);
	if (!cdda_filefmt_supp(FILEFMT_FLAC))
		XtSetSensitive(widgets.options.mode_fmt_flac_btn, False);
	if (!cdda_filefmt_supp(FILEFMT_AAC))
		XtSetSensitive(widgets.options.mode_fmt_aac_btn, False);
	if (!cdda_filefmt_supp(FILEFMT_MP4))
		XtSetSensitive(widgets.options.mode_fmt_mp4_btn, False);

	/* Get label strings */
	XtVaGetValues(widgets.options.qualval_lbl,
		XmNlabelString, &xs_qual,
		NULL
	);
	XtVaGetValues(widgets.options.compalgo_lbl,
		XmNlabelString, &xs_algo,
		NULL
	);

	/* Create pushbutton widgets in bitrate menu */
	for (j = (cdda_bitrates() - 1); j >= 0; j--) {
		int		k;
		wlist_t		*l,
				**head;
		Widget		w[3],
				pw;
		Arg		arg[5];
		XtCallbackProc	cbfunc;

		if (cdda_bitrate_val(j) < 32)
			continue;

		for (k = 0; k < 3; k++) {
			switch (k) {
			case 1:
				pw = widgets.options.minbrate_menu;
				head = &cd_minbrhead;
				cbfunc = cd_min_bitrate;
				break;
			case 2:
				pw = widgets.options.maxbrate_menu;
				head = &cd_maxbrhead;
				cbfunc = cd_max_bitrate;
				break;
			case 0:
			default:
				pw = widgets.options.bitrate_menu;
				head = &cd_brhead;
				cbfunc = cd_bitrate;
				break;
			}

			i = 0;
			XtSetArg(arg[i], XmNshadowThickness, 2); i++;
			w[k] = XmCreatePushButton(
				pw, cdda_bitrate_name(j), arg, i
			);

			XtManageChild(w[k]);

			/* Register callback */
			register_activate_cb(w[k], cbfunc, s);

			/* Add to list */
			l = (wlist_t *)(void *) MEM_ALLOC(
				"wlist_t", sizeof(wlist_t)
			);
			if (l == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			l->w = w[k];
			l->next = *head;
			*head = l;
		}
	}

	s->time_dpy = (byte_t) app_data.timedpy_mode;

	/* Force CD-TEXT config to False if it doesn't appear in the
	 * CD info path list.
	 */
	if (!cdinfo_cdtext_iscfg())
		app_data.cdtext_dsbl = TRUE;

	/* Set default options */
	cd_options_reset(widgets.options.reset_btn, (XtPointer) s, NULL);

	if (!app_data.mselvol_supp) {
		XtSetSensitive(widgets.options.vol_linear_btn, False);
		XtSetSensitive(widgets.options.vol_square_btn, False);
		XtSetSensitive(widgets.options.vol_invsqr_btn, False);
	}

	if (!app_data.balance_supp)
		XtSetSensitive(widgets.options.bal_scale, False);

	if (!app_data.chroute_supp) {
		XtSetSensitive(widgets.options.chroute_rev_btn, False);
		XtSetSensitive(widgets.options.chroute_left_btn, False);
		XtSetSensitive(widgets.options.chroute_right_btn, False);
		XtSetSensitive(widgets.options.chroute_mono_btn, False);
	}

	/* Add options window category list entries */
	for (i = 0; i < OPT_CATEGS; i++) {
		XmString	xs;
		wlist_t		*wl;
		Widget		*warray;
		int		j;

		switch (i) {
		case 0:
			/* Playback mode */
			opt_categ[i].name = app_data.str_playbackmode;
			warray = &widgets.options.mode_lbl;
			break;
		case 1:
			/* Encoding parameters */
			opt_categ[i].name = app_data.str_encodeparms;
			warray = &widgets.options.method_opt;
			break;
		case 2:
			/* LAME MP3 options */
			opt_categ[i].name = app_data.str_lameopts;
			warray = &widgets.options.lame_lbl;
			break;
		case 3:
			/* CDDA thread schedling parameters */
			opt_categ[i].name = app_data.str_schedparms;
			warray = &widgets.options.sched_lbl;
			break;
		case 4:
			/* Automation */
			opt_categ[i].name = app_data.str_autofuncs;
			warray = &widgets.options.load_lbl;
			break;
		case 5:
			/* CD Changer */
			opt_categ[i].name = app_data.str_cdchanger;
			warray = &widgets.options.chg_lbl;
			break;
		case 6:
			/* Channel routing */
			opt_categ[i].name = app_data.str_chroute;
			warray = &widgets.options.chroute_lbl;
			break;
		case 7:
			/* Volume / Balance */
			opt_categ[i].name = app_data.str_volbal;
			warray = &widgets.options.vol_lbl;
			break;
		case 8:
			/* CDDB / CD-TEXT */
			opt_categ[i].name = app_data.str_cddb_cdtext;
			warray = &widgets.options.cddb_lbl;
			break;
		default:
			opt_categ[i].name = NULL;
			warray = NULL;
			break;
		}

		xs = create_xmstring(opt_categ[i].name, NULL, CHSET1, FALSE);
		XmListAddItemUnselected(widgets.options.categ_list, xs, i+1);
		XmStringFree(xs);

		for (j = 0; warray[j] != (Widget) NULL; j++) {
			wl = (wlist_t *)(void *) MEM_ALLOC(
				"wlist_t", sizeof(wlist_t)
			);
			if (wl == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			wl->next = NULL;
			wl->w = warray[j];

			if (opt_categ[i].widgets == NULL)
				opt_categ[i].widgets = wl;
			else {
				wl->next = opt_categ[i].widgets;
				opt_categ[i].widgets = wl;
			}
		}
	}
	/* Pre-select the first category */
	XmListSelectPos(widgets.options.categ_list, 1, True);

	/* Set the main window and icon titles */
	up = util_get_uname();
	(void) sprintf(titlestr, "%.32s: %.80s/%d",
		app_data.main_title, 
		up->nodename,
		app_data.devnum);
	XtVaSetValues(widgets.toplevel,
		XmNtitle, titlestr,
		XmNiconName, PROGNAME,
		NULL
	);
}


/*
 * cd_start
 *	Secondary startup functions
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
cd_start(curstat_t *s)
{
	/* Allow popup dialogs from here on */
	popup_ok = TRUE;

	/* Start up libutil */
	util_start();

	/* Start up I/O interface */
	di_start(s);

	/* Start up help */
	help_start();
}


/*
 * cd_icon
 *	Main window iconification/deiconification handler.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	iconified - Whether the main window is iconified.
 *
 * Return:
 *	Nothing.
 */
void
cd_icon(curstat_t *s, bool_t iconified)
{
	di_icon(s, iconified);
}


/*
 * cd_halt
 *	Top level function to shut down all subsystems.  Used when
 *	closing the application.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
cd_halt(curstat_t *s)
{
	di_halt(s);
	cdinfo_halt(s);
}


/*
 * cd_quit
 *	Close the application.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
cd_quit(curstat_t *s)
{
	XmAnyCallbackStruct	p;

	if (XtIsRealized(widgets.toplevel))
		XtUnmapWidget(widgets.toplevel);

	/* Cancel asynchronous CDDB load operation, if active */
	cdinfo_load_cancel();

	/* Shut down all xmcd subsystems */
	cd_halt(s);

	/* Uninstall current keyboard grabs */
	p.reason = XmCR_FOCUS;
	cd_shell_focus_chg(
		widgets.toplevel,
		(XtPointer) widgets.toplevel,
		(XtPointer) &p
	);

	/* Let X events drain */
	event_loop(0);

	/* Shut down GUI */
	shutdown_gui();

	exit(0);
}


/*
 * cd_busycurs
 *	Enable/disable the watch cursor.
 *
 * Args:
 *	busy   - Boolean value indicating whether to enable or disable the
 *		 watch cursor.
 *	winmap - Bitmap of form widgets in which the cursor should be
 *		 affected
 *
 * Return:
 *	Nothing.
 */
void
cd_busycurs(bool_t busy, int winmap)
{
	Display		*dpy = XtDisplay(widgets.toplevel);
	Window		win;
	static Cursor	wcur = (Cursor) 0;

	if (wcur == (Cursor) 0)
		wcur = XCreateFontCursor(dpy, XC_watch);

	if (winmap == 0)
		return;

	if (busy) {
		if ((winmap & CURS_MAIN) &&
		    (win = XtWindow(widgets.main.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_KEYPAD) &&
		    (win = XtWindow(widgets.keypad.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_OPTIONS) &&
		    (win = XtWindow(widgets.options.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_DBPROG) &&
		    (win = XtWindow(widgets.dbprog.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_DLIST) &&
		    (win = XtWindow(widgets.dlist.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_DBEXTD) &&
		    (win = XtWindow(widgets.dbextd.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_DBEXTT) &&
		    (win = XtWindow(widgets.dbextt.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_FULLNAME) &&
		    (win = XtWindow(widgets.fullname.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_CREDITS) &&
		    (win = XtWindow(widgets.credits.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_SEGMENTS) &&
		    (win = XtWindow(widgets.segments.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_SUBMITURL) &&
		    (win = XtWindow(widgets.submiturl.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_USERREG) &&
		    (win = XtWindow(widgets.userreg.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
		if ((winmap & CURS_HELP) &&
		    (win = XtWindow(widgets.help.form)) != (Window) 0)
			XDefineCursor(dpy, win, wcur);
	}
	else {
		if ((winmap & CURS_MAIN) &&
		    (win = XtWindow(widgets.main.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_KEYPAD) &&
		    (win = XtWindow(widgets.keypad.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_OPTIONS) &&
		    (win = XtWindow(widgets.options.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_DBPROG) &&
		    (win = XtWindow(widgets.dbprog.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_DLIST) &&
		    (win = XtWindow(widgets.dlist.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_DBEXTD) &&
		    (win = XtWindow(widgets.dbextd.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_DBEXTT) &&
		    (win = XtWindow(widgets.dbextt.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_FULLNAME) &&
		    (win = XtWindow(widgets.fullname.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_CREDITS) &&
		    (win = XtWindow(widgets.credits.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_SEGMENTS) &&
		    (win = XtWindow(widgets.segments.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_SUBMITURL) &&
		    (win = XtWindow(widgets.submiturl.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_USERREG) &&
		    (win = XtWindow(widgets.userreg.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
		if ((winmap & CURS_HELP) &&
		    (win = XtWindow(widgets.help.form)) != (Window) 0)
			XUndefineCursor(dpy, win);
	}
	XFlush(dpy);
}


/*
 * cd_hostname
 *	Return the system's host name (with fully qualified domain name
 *	if possible).  This function can only be used after the call
 *	to cdinfo_init() is completed.
 *
 * Args:
 *	None.
 *
 * Return:
 *	The host name string.
 */
char *
cd_hostname(void)
{
	return (cdinfo_cldata.host);
}


/*
 * onsig
 *	Signal handler.  Causes the application to shut down gracefully.
 *
 * Args:
 *	sig - The signal number received.
 *
 * Return:
 *	Nothing.
 */
void
onsig(int sig)
{
	(void) util_signal(sig, SIG_IGN);
	cd_quit(curstat_addr());
}


/**************** vv Callback routines vv ****************/

/*
 * cd_checkbox
 *	Main window checkbox callback function
 */
/*ARGSUSED*/
void
cd_checkbox(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmRowColumnCallbackStruct	*p =
		(XmRowColumnCallbackStruct *)(void *) call_data;
	XmToggleButtonCallbackStruct	*q;
	curstat_t			*s =
		(curstat_t *)(void *) client_data;
	char				*str;
	bool_t				paused = FALSE,
					need_restart = FALSE;
	size_t				len = 0;

	if (p->reason != XmCR_ACTIVATE)
		return;

	q = (XmToggleButtonCallbackStruct *)(void *) p->callbackstruct;
	if (q == NULL)
		return;

	str = q->set ? "Enable" : "Disable";

	DBGPRN(DBG_UI)(errfp, "\n* CHKBOX: ");

	if (p->widget == widgets.main.lock_btn) {
		DBGPRN(DBG_UI)(errfp, "lock: %s\n", str);

		di_lock(s, (bool_t) q->set);
	}
	else if (p->widget == widgets.main.repeat_btn) {
		DBGPRN(DBG_UI)(errfp, "repeat: %s\n", str);

		di_repeat(s, (bool_t) q->set);
	}
	else if (p->widget == widgets.main.shuffle_btn) {
		DBGPRN(DBG_UI)(errfp, "shuffle: %s\n", str);

		if (!q->set) {
			/* Clear shuffle mode */
			di_shuffle(s, FALSE);
			return;
		}

		switch (s->mode) {
		case MOD_PAUSE:
			paused = TRUE;
			/*FALLTHROUGH*/
		case MOD_PLAY:
		case MOD_SAMPLE:
			need_restart = TRUE;

			if (s->program && !s->onetrk_prog) {
				len = strlen(app_data.str_clrprog) +
				      strlen(app_data.str_restartplay) +
				      strlen(app_data.str_askproceed) + 3;
				str = (char *) MEM_ALLOC("msg", len);
				if (str == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(str, "%s\n%s\n%s",
					app_data.str_clrprog,
					app_data.str_restartplay,
					app_data.str_askproceed
				);
			}
			else if (s->segplay == SEGP_AB) {
				len = strlen(app_data.str_cancelseg) +
				      strlen(app_data.str_restartplay) +
				      strlen(app_data.str_askproceed) + 3;
				str = (char *) MEM_ALLOC("msg", len);
				if (str == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(str, "%s\n%s\n%s",
					app_data.str_cancelseg,
					app_data.str_restartplay,
					app_data.str_askproceed
				);
			}
			else {
				len = strlen(app_data.str_restartplay) +
				      strlen(app_data.str_askproceed) + 2;
				str = (char *) MEM_ALLOC("msg", len);
				if (str == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(str, "%s\n%s",
					app_data.str_restartplay,
					app_data.str_askproceed
				);
			}
			break;

		default:
			if (s->program && !s->onetrk_prog) {
				len = strlen(app_data.str_clrprog) +
				      strlen(app_data.str_askproceed) + 2;
				str = (char *) MEM_ALLOC("msg", len);
				if (str == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(str, "%s\n%s",
					app_data.str_clrprog,
					app_data.str_askproceed
				);
			}
			else if (s->segplay == SEGP_AB) {
				len = strlen(app_data.str_cancelseg) +
				      strlen(app_data.str_askproceed) + 2;
				str = (char *) MEM_ALLOC("msg", len);
				if (str == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(str, "%s\n%s",
					app_data.str_cancelseg,
					app_data.str_askproceed
				);
			}
			else
				str = NULL;
			break;
		}

		if (str == NULL ||
		    cd_confirm_popup(app_data.str_confirm,
					str,
					(XtCallbackProc) NULL, NULL,
					(XtCallbackProc) NULL, NULL)) {

			if (need_restart) {
				/* Stop playback first */
				di_stop(s, FALSE);
			}

			if (s->program) {
				/* Cancel program mode */
				dbprog_clrpgm(w, client_data, NULL);
			}

			if (s->segplay != SEGP_NONE) {
				/* Cancel a->b mode */
				s->segplay = SEGP_NONE;
				dpy_progmode(s, FALSE);
			}

			/* Set shuffle mode */
			di_shuffle(s, TRUE);

			if (need_restart) {
				if (paused)
					/* Mute sound */
					di_mute_on(s);

				/* Restart playback */
				s->cur_trk = -1;
				cd_play_pause(w, client_data, NULL);

				if (paused) {
					/* This will cause the playback
					 * to pause.
					 */
					cd_play_pause(w, client_data, NULL);

					/* Restore sound */
					di_mute_off(s);
				}
			}
		}
		else {
			DBGPRN(DBG_UI)(errfp,
			    "\n* SHUFFLE: user aborted mode change\n");

			/* Restore button state */
			set_shuffle_btn(FALSE);
		}

		if (str != NULL)
			MEM_FREE(str);
	}
}


/*
 * cd_mode
 *	Main window mode button callback function
 */
/*ARGSUSED*/
void
cd_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	DBGPRN(DBG_UI)(errfp, "\n* WINDOW MODE\n");

	/* Change the main window arrangement.  Unmap the form while
	 * the change takes place so that the user does not see a
	 * bunch of jumbled widgets while they adjust to the new size.
	 */
	XtUnmapWidget(widgets.main.form);
	geom_main_chgmode(&widgets);
	XtMapWidget(widgets.main.form);

	/* Force indicator font change */
	mode_chg = TRUE;
	dpy_track(s);
	dpy_time(s, FALSE);
	mode_chg = FALSE;

	/* This is a hack to prevent the tooltip from popping up due to
	 * the main window reconfiguration.
	 */
	skip_next_tooltip = TRUE;

	/* Overload the function of this button to also
	 * dump the contents of the curstat_t structure
	 * in debug mode
	 */
	if (app_data.debug & DBG_ALL)
		di_dump_curstat(s);
}


/*
 * cd_load_eject
 *	Main window load/eject button callback function
 */
/*ARGSUSED*/
void
cd_load_eject(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	DBGPRN(DBG_UI)(errfp, "\n* LOAD_EJECT\n");

	if (searching) {
		cd_beep();
		return;
	}
	if (s->mode == MOD_PAUSE)
		dpy_time(s, FALSE);

	if (s->mode != MOD_BUSY && s->mode != MOD_NODISC) {
		s->flags |= STAT_EJECT;

		/* Ask the user if the changed CD information should be
		 * submitted to CDDB
		 */
		if (!dbprog_chgsubmit(s))
			return;
	}

	/* Change to watch cursor */
	cd_busycurs(TRUE, CURS_ALL);

	/* Cancel asynchronous CDDB load operation, if active */
	cdinfo_load_cancel();

	/* Load/Eject the CD */
	di_load_eject(s);

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);

	s->flags &= ~STAT_EJECT;
}


/*
 * cd_quit_btn
 *	Main window quit button callback function
 */
/*ARGSUSED*/
void
cd_quit_btn(Widget w, XtPointer client_data, XtPointer call_data)
{
	DBGPRN(DBG_UI)(errfp, "\n* QUIT\n");

	(void) cd_confirm_popup(
		app_data.str_confirm,
		app_data.str_quit,
		(XtCallbackProc) cd_exit,
		client_data,
		(XtCallbackProc) NULL,
		NULL
	);
}


/*
 * cd_time
 *	Main window time mode button callback function
 */
/*ARGSUSED*/
void
cd_time(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	switch (s->time_dpy) {
	case T_ELAPSED_TRACK:
		s->time_dpy = T_ELAPSED_SEG;
		break;

	case T_ELAPSED_SEG:
		s->time_dpy = T_ELAPSED_DISC;
		break;

	case T_ELAPSED_DISC:
		s->time_dpy = T_REMAIN_TRACK;
		break;

	case T_REMAIN_TRACK:
		s->time_dpy = T_REMAIN_SEG;
		break;

	case T_REMAIN_SEG:
		s->time_dpy = T_REMAIN_DISC;
		break;

	case T_REMAIN_DISC:
		s->time_dpy = T_ELAPSED_TRACK;
		break;
	}

	dpy_timemode(s);
	dpy_track(s);
	dpy_time(s, FALSE);
}


/*
 * cd_ab
 *	Main window a->b mode button callback function
 */
/*ARGSUSED*/
void
cd_ab(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	char		buf[16];

	DBGPRN(DBG_UI)(errfp, "\n* A->B\n");

	if (searching) {
		cd_beep();
		return;
	}
	switch (s->segplay) {
	case SEGP_NONE:
		if (s->mode != MOD_PLAY || s->program || s->shuffle) {
			/* Must be in normal playing mode */
			cd_beep();
			return;
		}

		/* First click - set start position */

		s->bp_startpos_tot = s->curpos_tot;	/* Structure copy */
		s->bp_startpos_trk = s->curpos_trk;	/* Structure copy */
		s->segplay = SEGP_A;

		/* Set the segment start position if the segments window
		 * is popped up
		 */
		if (XtIsManaged(widgets.segments.form)) {
			(void) sprintf(buf, "%u", s->cur_trk);
			set_text_string(
				widgets.segments.starttrk_txt, buf, FALSE
			);

			(void) sprintf(buf, "%u", s->curpos_trk.addr);
			set_text_string(
				widgets.segments.startfrm_txt, buf, FALSE
			);
		}
		break;

	case SEGP_A:
		if (s->mode != MOD_PLAY || s->program || s->shuffle) {
			/* Must be in normal playing mode */
			cd_beep();
			return;
		}

		/* Second click - set end position and stop playback. */

		/* Get current position */
		di_status_upd(s);

		s->bp_endpos_tot = s->curpos_tot;	/* Structure copy */
		s->bp_endpos_trk = s->curpos_trk;	/* Structure copy */

		/* Set the segment end position if the segments window
		 * is popped up
		 */
		if (XtIsManaged(widgets.segments.form)) {
			(void) sprintf(buf, "%u", s->cur_trk);
			set_text_string(
				widgets.segments.endtrk_txt, buf, FALSE
			);

			(void) sprintf(buf, "%u", s->curpos_trk.addr);
			set_text_string(
				widgets.segments.endfrm_txt, buf, FALSE
			);
		}

		/* Make sure that the start + min_playblks < end  */
		if ((s->bp_startpos_tot.addr + app_data.min_playblks) >=
		    s->bp_endpos_tot.addr) {
			s->segplay = SEGP_NONE;
			CD_INFO(app_data.str_segposerr);
		}

		if (w == widgets.main.ab_btn) {
			s->segplay = SEGP_AB;
			di_stop(s, TRUE);
		}
		else
			s->segplay = SEGP_NONE;

		break;

	case SEGP_AB:
		/* Third click - Disable a->b mode */

		/* Clear the segment start/end positions if the segments
		 * window is popped up
		 */
		if (XtIsManaged(widgets.segments.form)) {
			set_text_string(
				widgets.segments.starttrk_txt, "", FALSE
			);
			set_text_string(
				widgets.segments.startfrm_txt, "", FALSE
			);
			set_text_string(
				widgets.segments.endtrk_txt, "", FALSE
			);
			set_text_string(
				widgets.segments.endfrm_txt, "", FALSE
			);
		}

		s->segplay = SEGP_NONE;
		di_stop(s, TRUE);
		break;
	}

	/* Set segments window set/clear button sensitivity */
	dbprog_segments_setmode(s);

	dpy_progmode(s, FALSE);
	dpy_playmode(s, FALSE);
}


/*
 * cd_sample
 *	Main window sample mode button callback function
 */
/*ARGSUSED*/
void
cd_sample(Widget w, XtPointer client_data, XtPointer call_data)
{
	DBGPRN(DBG_UI)(errfp, "\n* SAMPLE\n");

	if (searching) {
		cd_beep();
		return;
	}
	di_sample((curstat_t *)(void *) client_data);
}


/*
 * cd_level
 *	Main window volume control slider callback function
 */
/*ARGSUSED*/
void
cd_level(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScaleCallbackStruct
			*p = (XmScaleCallbackStruct *)(void *) call_data;

	DBGPRN(DBG_UI)(errfp, "\n* VOL: %d\n", (int) p->value);

	di_level(
		(curstat_t *)(void *) client_data,
		(byte_t) p->value,
		(bool_t) (p->reason != XmCR_VALUE_CHANGED)
	);
}


/*
 * cd_play_pause
 *	Main window play/pause button callback function
 */
/*ARGSUSED*/
void
cd_play_pause(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	sword32_t	sav_trk;

	DBGPRN(DBG_UI)(errfp, "\n* PLAY_PAUSE\n");

	if (searching) {
		cd_beep();
		return;
	}

	if (s->mode == MOD_STOP && s->program) {
		if (!dbprog_pgm_parse(s)) {
			cd_beep();
			return;
		}
		s->segplay = SEGP_NONE;	/* Cancel a->b mode */
		dpy_progmode(s, FALSE);
	}

	sav_trk = s->cur_trk;

	di_play_pause(s);

	if (sav_trk >= 0) {
		/* Update curfile */
		dbprog_curfileupd();
	}

	switch (s->mode) {
	case MOD_PAUSE:
		dpy_time(s, FALSE);
		break;
	case MOD_PLAY:
	case MOD_STOP:
	case MOD_SAMPLE:
		dpy_time(s, FALSE);

		cd_keypad_clear(w, client_data, NULL);
		warp_busy = FALSE;
		break;
	}
}


/*
 * cd_stop
 *	Main window stop button callback function
 */
/*ARGSUSED*/
void
cd_stop(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	DBGPRN(DBG_UI)(errfp, "\n* STOP\n");

	if (searching) {
		cd_beep();
		return;
	}

	/* If the user clicks "stop" while in a->? state, cancel it */
	if (s->segplay == SEGP_A) {
		s->segplay = SEGP_NONE;
		dpy_progmode(s, FALSE);
	}

	dpy_time(s, FALSE);

	di_stop(s, TRUE);
}


/*
 * cd_chgdisc
 *	Main window disc change buttons callback function
 */
/*ARGSUSED*/
void
cd_chgdisc(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	int		newdisc;

	if (s->first_disc == s->last_disc) {
		/* Single disc player */
		cd_beep();
		return;
	}

	newdisc = s->cur_disc;

	if (w == widgets.main.prevdisc_btn) {
		DBGPRN(DBG_UI)(errfp, "\n* PREV_DISC\n");
		if (newdisc > s->first_disc)
			newdisc--;
	}
	else if (w == widgets.main.nextdisc_btn) {
		DBGPRN(DBG_UI)(errfp, "\n* NEXT_DISC\n");
		if (newdisc < s->last_disc)
			newdisc++;
	}
	else
		return;

	if (newdisc == s->cur_disc)
		/* No change */
		return;

	s->prev_disc = s->cur_disc;
	s->cur_disc = newdisc;
	dpy_disc(s);

	s->flags |= STAT_CHGDISC;

	/* Ask the user if the changed disc info should be submitted to CDDB */
	if (!dbprog_chgsubmit(s))
		return;

	s->flags &= ~STAT_CHGDISC;

	/* Update display: clear disc/track titles on the main window */
	chgdelay = TRUE;
	dpy_dtitle(s);
	dpy_ttitle(s);

	/* Use a timer callback routine to do the disc change.
	 * This allows the user to click on the disc change buttons
	 * multiple times to advance/reverse to the desired
	 * target disc without causing the changer to actually
	 * switch through all of them.
	 */
	if (chgdisc_dlyid >= 0)
		cd_untimeout(chgdisc_dlyid);

	chgdisc_dlyid = cd_timeout(CHGDISC_DELAY, do_chgdisc, (byte_t *) s);
}


/*
 * cd_prevtrk
 *	Main window prev track button callback function
 */
/*ARGSUSED*/
void
cd_prevtrk(Widget w, XtPointer client_data, XtPointer call_data)
{
	DBGPRN(DBG_UI)(errfp, "\n* PREVTRK\n");

	if (searching) {
		cd_beep();
		return;
	}
	di_prevtrk((curstat_t *)(void *) client_data);
}


/*
 * cd_nexttrk
 *	Main window next track button callback function
 */
/*ARGSUSED*/
void
cd_nexttrk(Widget w, XtPointer client_data, XtPointer call_data)
{
	DBGPRN(DBG_UI)(errfp, "\n* NEXTTRK\n");

	if (searching) {
		cd_beep();
		return;
	}
	di_nexttrk((curstat_t *)(void *) client_data);
}


/*
 * cd_previdx
 *	Main window prev index button callback function
 */
/*ARGSUSED*/
void
cd_previdx(Widget w, XtPointer client_data, XtPointer call_data)
{
	DBGPRN(DBG_UI)(errfp, "\n* PREVIDX\n");

	if (searching) {
		cd_beep();
		return;
	}
	di_previdx((curstat_t *)(void *) client_data);
}


/*
 * cd_previdx
 *	Main window next index button callback function
 */
/*ARGSUSED*/
void
cd_nextidx(Widget w, XtPointer client_data, XtPointer call_data)
{
	DBGPRN(DBG_UI)(errfp, "\n* NEXTIDX\n");

	if (searching) {
		cd_beep();
		return;
	}
	di_nextidx((curstat_t *)(void *) client_data);
}


/*
 * cd_rew
 *	Main window search rewind button callback function
 */
/*ARGSUSED*/
void
cd_rew(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;
	bool_t		start;
	static bool_t	rew_running = FALSE;

	if (p->reason == XmCR_ARM) {
		DBGPRN(DBG_UI)(errfp, "\n* REW: down\n");

		if (!rew_running) {
			if (searching) {
				/* Release running FF */
				XtCallActionProc(
					widgets.main.ff_btn,
					"Activate",
					p->event,
					NULL,
					0
				);
				XtCallActionProc(
					widgets.main.ff_btn,
					"Disarm",
					p->event,
					NULL,
					0
				);
			}

			rew_running = TRUE;
			searching = TRUE;
			start = TRUE;
		}
		else
			/* Already running REW */
			return;
	}
	else {
		DBGPRN(DBG_UI)(errfp, "\n* REW: up\n");

		if (rew_running) {
			rew_running = FALSE;
			searching = FALSE;
			start = FALSE;
		}
		else
			/* Not running REW */
			return;
	}

	di_rew(s, start);

	dpy_time(s, FALSE);
}


/*
 * cd_ff
 *	Main window search fast-forward button callback function
 */
/*ARGSUSED*/
void
cd_ff(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;
	bool_t		start;
	static bool_t	ff_running = FALSE;

	if (p->reason == XmCR_ARM) {
		DBGPRN(DBG_UI)(errfp, "\n* FF: down\n");

		if (!ff_running) {
			if (searching) {
				/* Release running REW */
				XtCallActionProc(
					widgets.main.rew_btn,
					"Activate",
					p->event,
					NULL,
					0
				);
				XtCallActionProc(
					widgets.main.rew_btn,
					"Disarm",
					p->event,
					NULL,
					0
				);
			}

			ff_running = TRUE;
			searching = TRUE;
			start = TRUE;
		}
		else
			/* Already running FF */
			return;
	}
	else {
		DBGPRN(DBG_UI)(errfp, "\n* FF: up\n");

		if (ff_running) {
			ff_running = FALSE;
			searching = FALSE;
			start = FALSE;
		}
		else
			/* Not running FF */
			return;
	}

	di_ff(s, start);

	dpy_time(s, FALSE);
}


/*
 * cd_keypad_popup
 *	Main window keypad button callback function
 */
void
cd_keypad_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.keypad.form)) {
		/* Pop down keypad window */
		cd_keypad_popdown(w, client_data, call_data);
		return;
	}

	/* Pop up keypad window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason for this
	 * is we want to avoid a screen glitch when we move the window
	 * in cd_dialog_setpos(), so we map the window afterwards.
	 */
	XtManageChild(widgets.keypad.form);
	if (first) {
		first = FALSE;
		/* Set window position */
		cd_dialog_setpos(XtParent(widgets.keypad.form));
	}
	XtMapWidget(XtParent(widgets.keypad.form));

	/* Reset keypad */
	cd_keypad_clear(w, client_data, NULL);

	/* Update warp slider */
	dpy_warp((curstat_t *)(void *) client_data);

	XmProcessTraversal(
		widgets.keypad.cancel_btn,
		XmTRAVERSE_CURRENT
	);
}


/*
 * cd_keypad_popdown
 *	Keypad window popdown callback function
 */
/*ARGSUSED*/
void
cd_keypad_popdown(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop down keypad window */
	if (XtIsManaged(widgets.keypad.form)) {
		XtUnmapWidget(XtParent(widgets.keypad.form));
		XtUnmanageChild(widgets.keypad.form);
	}
}


/*
 * cd_keypad_mode
 *	Keypad window mode selector callback function
 */
void
cd_keypad_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmRowColumnCallbackStruct	*p =
		(XmRowColumnCallbackStruct *)(void *) call_data;
	XmToggleButtonCallbackStruct	*q;
	curstat_t			*s = (curstat_t *)(void *) client_data;

	if (p == NULL)
		return;

	q = (XmToggleButtonCallbackStruct *)(void *) p->callbackstruct;

	if (q == NULL || !q->set)
		return;

	if (p->widget == widgets.keypad.disc_btn) {
		if (keypad_mode == KPMODE_DISC)
			return;	/* No change */

		keypad_mode = KPMODE_DISC;
	}
	else if (p->widget == widgets.keypad.track_btn) {
		if (keypad_mode == KPMODE_TRACK)
			return;	/* No change */

		keypad_mode = KPMODE_TRACK;
	}
	else
		return;	/* Invalid widget */

	cd_keypad_clear(w, (XtPointer) s, NULL);
	warp_busy = FALSE;
}


/*
 * cd_keypad_num
 *	Keypad window number button callback function
 */
/*ARGSUSED*/
void
cd_keypad_num(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = curstat_addr();
	int		n;
	char		tmpstr[2];

	/* The user entered a digit */
	if (strlen(keystr) >= sizeof(keystr) - 1) {
		cd_beep();
		return;
	}

	(void) sprintf(tmpstr, "%lu", (unsigned long) client_data);
	(void) strcat(keystr, tmpstr);

	switch (keypad_mode) {
	case KPMODE_DISC:
		n = atoi(keystr);

		if (n < s->first_disc || n > s->last_disc) {
			/* Illegal disc entered */
			cd_keypad_clear(w, (XtPointer) s, NULL);

			cd_beep();
			return;
		}
		break;

	case KPMODE_TRACK:
		n = s->cur_trk;
		s->cur_trk = (sword32_t) atoi(keystr);

		if (di_curtrk_pos(s) < 0) {
			/* Illegal track entered */
			cd_keypad_clear(w, (XtPointer) s, NULL);
			s->cur_trk = n;

			cd_beep();
			return;
		}
		s->cur_trk = n;
		break;

	default:
		/* Illegal mode */
		return;
	}

	warp_offset = 0;
	set_warp_slider(0, FALSE);
	dpy_keypad_ind(s);
}


/*
 * cd_keypad_clear
 *	Keypad window clear button callback function
 */
/*ARGSUSED*/
void
cd_keypad_clear(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	/* Reset keypad */
	keystr[0] = '\0';

	/* Hack: if the third arg is NULL, then it's an internal
	 * call rather than a callback.  We want to set s->cur_trk
	 * to -1 only for callbacks, so that the keypad indicator
	 * display gets updated correctly.
	 */
	if (call_data != NULL)
		s->cur_trk = -1;

	warp_offset = 0;
	set_warp_slider(0, FALSE);
	dpy_keypad_ind(s);
}


/*
 * cd_keypad_dsbl_modes_yes
 *	User "yes" confirm callback to cancel shuffle or program modes after
 *	activating the keypad.
 */
/*ARGSUSED*/
void
cd_keypad_dsbl_modes_yes(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (override_sav.func == NULL) {
		cd_beep();
		return;
	}

	/* Note: This assumes that shuffle and program modes are
	 * mutually exclusive!
	 */
	if (s->shuffle) {
		/* Disable shuffle mode */
		di_shuffle(s, FALSE);
		set_shuffle_btn(FALSE);
	}
	else if (s->program)
		dbprog_clrpgm(w, client_data, call_data);

	(*override_sav.func)(
		override_sav.w,
		override_sav.client_data,
		override_sav.call_data
	);

	override_sav.func = (XtCallbackProc) NULL;
	if (override_sav.call_data != NULL) {
		MEM_FREE(override_sav.call_data);
		override_sav.call_data = NULL;
	}
}


/*
 * cd_keypad_dsbl_modes_no
 *	User "no" confirm callback to cancel shuffle or program modes after
 *	activating the keypad.
 */
/*ARGSUSED*/
void
cd_keypad_dsbl_modes_no(Widget w, XtPointer client_data, XtPointer call_data)
{
	warp_busy = FALSE;
	cd_keypad_clear(w, client_data, call_data);

	override_sav.func = (XtCallbackProc) NULL;
	if (override_sav.call_data != NULL) {
		MEM_FREE(override_sav.call_data);
		override_sav.call_data = NULL;
	}
}


/*
 * cd_keypad_enter
 *	Keypad window enter button callback function
 */
void
cd_keypad_enter(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	int		i;
	sword32_t	curr,
			next,
			sav_cur_trk;
	bool_t		paused = FALSE;

	/* The user activated the Enter key */
	if (keystr[0] == '\0') {
		/* No numeric input */
		cd_beep();
		return;
	}

	switch (keypad_mode) {
	case KPMODE_DISC:
		/* Use disc number selected on keypad */
		curr = s->cur_disc;
		next = (sword32_t) atoi(keystr);

		if (curr == next) {
			/* Nothing to do */
			cd_keypad_clear(w, client_data, NULL);
			break;
		}

		if (s->program) {
			if (s->onetrk_prog)
				dbprog_clrpgm(w, client_data, call_data);
			else {
				/* Trying to use keypad while in shuffle or
				 * program mode: ask user if shuffle/program
				 * should be disabled.
				 */
				cd_keypad_ask_dsbl(
					s,
					cd_keypad_enter,
					w,
					call_data,
					sizeof(XmPushButtonCallbackStruct)
				);
				return;
			}
		}

		s->prev_disc = curr;
		s->cur_disc = next;

		cd_keypad_clear(w, client_data, NULL);

		s->flags |= STAT_CHGDISC;

		/* Ask the user if the changed CDDB entry
		 * should be saved to file.
		 */
		if (!dbprog_chgsubmit(s))
			return;

		s->flags &= ~STAT_CHGDISC;

		/* Change to watch cursor */
		cd_busycurs(TRUE, CURS_ALL);

		/* Do the disc change */
		di_chgdisc(s);

		/* Update display */
		dpy_dbmode(s, FALSE);
		dpy_playmode(s, FALSE);

		/* Change to normal cursor */
		cd_busycurs(FALSE, CURS_ALL);

		break;

	case KPMODE_TRACK:
		if (s->mode == MOD_NODISC || s->mode == MOD_BUSY) {
			/* Cannot go to a track when the disc is not ready */
			cd_keypad_clear(w, client_data, NULL);
			cd_beep();
			return;
		}

		if (s->shuffle || s->program) {
			if (s->onetrk_prog)
				dbprog_clrpgm(w, client_data, call_data);
			else {
				/* Trying to use keypad while in shuffle or
				 * program mode: ask user if shuffle/program
				 * should be disabled.
				 */
				cd_keypad_ask_dsbl(
					s,
					cd_keypad_enter,
					w,
					call_data,
					sizeof(XmPushButtonCallbackStruct)
				);
				return;
			}
		}

		/* Use track number selected on keypad */
		sav_cur_trk = s->cur_trk;
		s->cur_trk = (word32_t) atoi(keystr);

		if ((i = di_curtrk_pos(s)) < 0) {
			s->cur_trk = sav_cur_trk;
			cd_beep();
			return;
		}

		switch (s->mode) {
		case MOD_PAUSE:
			/* Mute sound */
			di_mute_on(s);
			paused = TRUE;

			/*FALLTHROUGH*/
		case MOD_PLAY:
		case MOD_SAMPLE:
			if (s->segplay == SEGP_AB) {
				s->segplay = SEGP_NONE;	/* Cancel a->b mode */
				dpy_progmode(s, FALSE);
			}

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

		s->curpos_trk.addr = warp_offset;
		util_blktomsf(
			s->curpos_trk.addr,
			&s->curpos_trk.min,
			&s->curpos_trk.sec,
			&s->curpos_trk.frame,
			0
		);
		s->curpos_tot.addr = s->trkinfo[i].addr + warp_offset;
		util_blktomsf(
			s->curpos_tot.addr,
			&s->curpos_tot.min,
			&s->curpos_tot.sec,
			&s->curpos_tot.frame,
			MSF_OFFSET
		);

		/* Start playback at new position */
		cd_play_pause(w, client_data, call_data);

		if (paused) {
			/* This will cause the playback to pause */
			cd_play_pause(w, client_data, call_data);

			/* Restore sound */
			di_mute_off(s);
		}

		break;

	default:
		/* Illegal mode */
		break;
	}
}


/*
 * cd_warp
 *	Track warp function
 */
void
cd_warp(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	XmScaleCallbackStruct
			*p = (XmScaleCallbackStruct *)(void *) call_data;
	int		i;
	sword32_t	sav_cur_trk;

	if (pseudo_warp) {
		warp_busy = FALSE;
		return;
	}

	if (keypad_mode != KPMODE_TRACK ||
	    s->mode == MOD_BUSY || s->mode == MOD_NODISC) {
		warp_offset = 0;
		warp_busy = FALSE;
		set_warp_slider(0, FALSE);
		return;
	}

	sav_cur_trk = s->cur_trk;
	if (keystr[0] != '\0') {
		/* Use track number selected on keypad */
		s->cur_trk = atoi(keystr);
	}

	if ((i = di_curtrk_pos(s)) < 0) {
		warp_offset = 0;
		warp_busy = FALSE;
		set_warp_slider(0, FALSE);
		s->cur_trk = sav_cur_trk;
		return;
	}

	/* Translate slider position to block offset */
	warp_offset = (sword32_t) scale_warp(s, i, p->value);

	if (p->reason == XmCR_VALUE_CHANGED) {
		if ((s->shuffle || s->program) &&
		    s->mode != MOD_STOP && sav_cur_trk != s->cur_trk) {
			if (s->onetrk_prog)
				dbprog_clrpgm(w, client_data, call_data);
			else {
				/* Trying to warp to a different track while
				 * in shuffle or program mode: ask user if
				 * shuffle/program should be disabled.
				 */
				cd_keypad_ask_dsbl(
					s,
					cd_warp,
					w,
					call_data,
					sizeof(XmScaleCallbackStruct)
				);
				return;
			}
		}

		DBGPRN(DBG_UI)(errfp, "\n* TRACK WARP\n");

		s->curpos_trk.addr = warp_offset;
		util_blktomsf(
			s->curpos_trk.addr,
			&s->curpos_trk.min,
			&s->curpos_trk.sec,
			&s->curpos_trk.frame,
			0
		);
		s->curpos_tot.addr = s->trkinfo[i].addr + warp_offset;
		util_blktomsf(
			s->curpos_tot.addr,
			&s->curpos_tot.min,
			&s->curpos_tot.sec,
			&s->curpos_tot.frame,
			MSF_OFFSET
		);

		if (s->mode == MOD_STOP) {
			warp_busy = TRUE;
			dpy_keypad_ind(s);
			warp_busy = FALSE;
			return;
		}

		/* Start playback at new position */
		di_warp(s);

		cd_keypad_clear(w, client_data, NULL);
		warp_offset = 0;
		warp_busy = FALSE;

		/* Update display */
		dpy_track(s);
		dpy_index(s);
		dpy_time(s, FALSE);
	}
	else {
		warp_busy = TRUE;
	}

	dpy_keypad_ind(s);

	/* Restore s->cur_trk to actual */
	s->cur_trk = sav_cur_trk;
}


/*
 * cd_options_popup
 *	Options window popup callback function
 */
/*ARGSUSED*/
void
cd_options_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.options.form)) {
		/* Pop down options window */
		cd_options_popdown(w, client_data, call_data);
		return;
	}

	/* Pop up options window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason for this
	 * is we want to avoid a screen glitch when we move the window
	 * in cd_dialog_setpos(), so we map the window afterwards.
	 */
	XtManageChild(widgets.options.form);
	if (first) {
		first = FALSE;
		/* Set window position */
		cd_dialog_setpos(XtParent(widgets.options.form));
	}
	XtMapWidget(XtParent(widgets.options.form));

	XmProcessTraversal(
		widgets.options.ok_btn,
		XmTRAVERSE_CURRENT
	);
}


/*
 * cd_options_popdown
 *	Options window popdown callback function
 */
/*ARGSUSED*/
void
cd_options_popdown(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop down options window */
	if (XtIsManaged(widgets.options.form)) {
		XtUnmapWidget(XtParent(widgets.options.form));
		XtUnmanageChild(widgets.options.form);
	}
}


/*
 * cd_options_reset
 *	Options window reset button callback function
 */
/*ARGSUSED*/
void
cd_options_reset(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	wlist_t		*l;
	cdinfo_incore_t	*cdp;
	cdinfo_path_t	*pp;
	char		*bdevname,
			*cp,
			str[16],
			path[FILE_PATH_SZ * 2];
	Widget		menuw;
	int		pri;

	if (call_data != NULL) {
		/* Re-read defaults */

		DBGPRN(DBG_UI)(errfp, "\n* OPTIONS RESET\n");

		/* Get system-wide common configuration parameters */
		(void) sprintf(path, SYS_CMCFG_PATH, app_data.libdir);
		di_common_parmload(path, FALSE, TRUE);

		/* Get user common configuration parameters */
		(void) sprintf(path, USR_CMCFG_PATH,
			       util_homedir(util_get_ouid()));
		di_common_parmload(path, FALSE, TRUE);

		/* Reinit CD info subsystem */
		cdinfo_reinit();

#ifdef __VMS
		bdevname = NULL;
		if (!util_newstr(&bdevname, "device.cfg")) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
#else
		if ((bdevname = util_basename(app_data.device)) == NULL) {
			CD_FATAL("Device path basename error: Aborting.");
			return;
		}
#endif

		/* Get system-wide device-specific configuration parameters */
		(void) sprintf(path, SYS_DSCFG_PATH, app_data.libdir, bdevname);
		di_devspec_parmload(path, FALSE, TRUE);

		/* Get user device-specific configuration parameters */
		(void) sprintf(path, USR_DSCFG_PATH,
			       util_homedir(util_get_ouid()), bdevname);
		di_devspec_parmload(path, FALSE, TRUE);

		MEM_FREE(bdevname);

		/* Set the channel routing */
		di_route(s);

		/* Set the volume level */
		di_level(s, (byte_t) s->level, TRUE);
	}

	/* Set default play mode */
	if (!XtIsSensitive(widgets.options.mode_cdda_btn))
		app_data.play_mode &= ~PLAYMODE_CDDA;
	if (!XtIsSensitive(widgets.options.mode_file_btn))
		app_data.play_mode &= ~PLAYMODE_FILE;
	if (!XtIsSensitive(widgets.options.mode_pipe_btn))
		app_data.play_mode &= ~PLAYMODE_PIPE;

	if (PLAYMODE_IS_STD(app_data.play_mode)) {
		XmToggleButtonSetState(
			widgets.options.mode_std_btn,
			True,
			True
		);
	}
	else {
		XmToggleButtonSetState(
			widgets.options.mode_cdda_btn,
			(Boolean) ((app_data.play_mode & PLAYMODE_CDDA) != 0),
			True
		);
		XmToggleButtonSetState(
			widgets.options.mode_file_btn,
			(Boolean) ((app_data.play_mode & PLAYMODE_FILE) != 0),
			True
		);
		XmToggleButtonSetState(
			widgets.options.mode_pipe_btn,
			(Boolean) ((app_data.play_mode & PLAYMODE_PIPE) != 0),
			True
		);
	}

	XmToggleButtonSetState(
		widgets.options.mode_jitter_btn,
		(Boolean) app_data.cdda_jitter_corr,
		(Boolean) (call_data != NULL)
	);

	XmToggleButtonSetState(
		widgets.options.mode_trkfile_btn,
		(Boolean) app_data.cdda_trkfile,
		(Boolean) (call_data != NULL)
	);

	XmToggleButtonSetState(
		widgets.options.mode_subst_btn,
		(Boolean) app_data.subst_underscore,
		(Boolean) (call_data != NULL)
	);

	if (!cdda_filefmt_supp(app_data.cdda_filefmt)) {
		/* Specified format not supported: force to RAW */
		DBGPRN(DBG_GEN)(errfp,
			"\nThe cddaFileFormat (%d) is not supported: "
			"reset to RAW\n", app_data.cdda_filefmt);
		app_data.cdda_filefmt = FILEFMT_RAW;
	}

	/* Set file format button */
	menuw = filefmt_btn(app_data.cdda_filefmt);
	XtVaSetValues(widgets.options.mode_fmt_opt,
		XmNmenuHistory, menuw,
		NULL
	);

	/* Set default output file format and path */
	if (app_data.cdda_tmpl != NULL &&
	    !util_newstr(&s->outf_tmpl, app_data.cdda_tmpl)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	fix_outfile_path(s);
	set_text_string(widgets.options.mode_path_txt, s->outf_tmpl, TRUE);

	/* Set default pipe program path and args */
	set_text_string(widgets.options.mode_prog_txt,
			app_data.pipeprog == NULL ? "" : app_data.pipeprog,
			TRUE);

	/* Set bitrate & quality factor */
	switch (app_data.comp_mode) {
	case COMPMODE_1:
		menuw = widgets.options.mode1_btn;
		XtSetSensitive(widgets.options.qualval_lbl, True);
		XtSetSensitive(widgets.options.qualval1_lbl, True);
		XtSetSensitive(widgets.options.qualval2_lbl, True);
		XtSetSensitive(widgets.options.qualval_scl, True);
		XtSetSensitive(widgets.options.bitrate_opt, False);
		XtSetSensitive(widgets.options.minbrate_opt, True);
		XtSetSensitive(widgets.options.maxbrate_opt, True);
		break;
	case COMPMODE_2:
		menuw = widgets.options.mode2_btn;
		XtSetSensitive(widgets.options.qualval_lbl, True);
		XtSetSensitive(widgets.options.qualval1_lbl, True);
		XtSetSensitive(widgets.options.qualval2_lbl, True);
		XtSetSensitive(widgets.options.qualval_scl, True);
		XtSetSensitive(widgets.options.bitrate_opt, False);
		XtSetSensitive(widgets.options.minbrate_opt, True);
		XtSetSensitive(widgets.options.maxbrate_opt, True);
		break;
	case COMPMODE_3:
		menuw = widgets.options.mode3_btn;
		XtSetSensitive(widgets.options.qualval_lbl, False);
		XtSetSensitive(widgets.options.qualval1_lbl, False);
		XtSetSensitive(widgets.options.qualval2_lbl, False);
		XtSetSensitive(widgets.options.qualval_scl, False);
		XtSetSensitive(widgets.options.bitrate_opt, True);
		XtSetSensitive(widgets.options.minbrate_opt, True);
		XtSetSensitive(widgets.options.maxbrate_opt, True);
		break;
	case COMPMODE_0:
	default:
		menuw = widgets.options.mode0_btn;
		XtSetSensitive(widgets.options.qualval_lbl, False);
		XtSetSensitive(widgets.options.qualval1_lbl, False);
		XtSetSensitive(widgets.options.qualval2_lbl, False);
		XtSetSensitive(widgets.options.qualval_scl, False);
		XtSetSensitive(widgets.options.bitrate_opt, True);
		XtSetSensitive(widgets.options.minbrate_opt, False);
		XtSetSensitive(widgets.options.maxbrate_opt, False);
		break;
	}

	/* Set compression method */
	XtVaSetValues(widgets.options.method_opt,
		XmNmenuHistory, menuw,
		NULL
	);

	/* Set bitrate */
	if (app_data.bitrate == 0) {
		XtVaSetValues(widgets.options.bitrate_opt,
			XmNmenuHistory, widgets.options.bitrate_def_btn,
			NULL
		);
	}
	else for (l = cd_brhead; l != NULL; l = l->next) {
		if (l->w != (Widget) NULL &&
		    atoi(XtName(l->w)) == app_data.bitrate) {
			XtVaSetValues(widgets.options.bitrate_opt,
				XmNmenuHistory, l->w,
				NULL
			);
		}
	}

	/* Set minimum bitrate */
	if (app_data.bitrate_min == 0) {
		XtVaSetValues(widgets.options.minbrate_opt,
			XmNmenuHistory, widgets.options.minbrate_def_btn,
			NULL
		);
	}
	else for (l = cd_minbrhead; l != NULL; l = l->next) {
		if (l->w != (Widget) NULL &&
		    atoi(XtName(l->w)) == app_data.bitrate_min) {
			XtVaSetValues(widgets.options.minbrate_opt,
				XmNmenuHistory, l->w,
				NULL
			);
		}
	}

	/* Set maximum bitrate */
	if (app_data.bitrate_max == 0) {
		XtVaSetValues(widgets.options.maxbrate_opt,
			XmNmenuHistory, widgets.options.maxbrate_def_btn,
			NULL
		);
	}
	else for (l = cd_maxbrhead; l != NULL; l = l->next) {
		if (l->w != (Widget) NULL &&
		    atoi(XtName(l->w)) == app_data.bitrate_max) {
			XtVaSetValues(widgets.options.maxbrate_opt,
				XmNmenuHistory, l->w,
				NULL
			);
		}
	}

	/* Set quality factor */
	set_qualval_slider(app_data.qual_factor);

	/* Set channel mode */
	switch (app_data.chan_mode) {
	case CH_JSTEREO:
		menuw = widgets.options.chmode_jstereo_btn;
		break;
	case CH_FORCEMS:
		menuw = widgets.options.chmode_forcems_btn;
		break;
	case CH_MONO:
		menuw = widgets.options.chmode_mono_btn;
		break;
	case CH_STEREO:
	default:
		menuw = widgets.options.chmode_stereo_btn;
		break;
	}
	XtVaSetValues(widgets.options.chmode_opt,
		XmNmenuHistory, menuw,
		NULL
	);

	/* Set compression algorithm value */
	set_algo_slider(app_data.comp_algo);

	/* Lowpass and highpass filters */
	switch (app_data.lowpass_mode) {
	case FILTER_AUTO:
		menuw = widgets.options.lp_auto_btn;
		break;
	case FILTER_MANUAL:
		menuw = widgets.options.lp_manual_btn;
		break;
	case FILTER_OFF:
	default:
		menuw = widgets.options.lp_off_btn;
		break;
	}
	XtVaSetValues(widgets.options.lp_opt,
		XmNmenuHistory, menuw,
		NULL
	);

	switch (app_data.highpass_mode) {
	case FILTER_AUTO:
		menuw = widgets.options.hp_auto_btn;
		break;
	case FILTER_MANUAL:
		menuw = widgets.options.hp_manual_btn;
		break;
	case FILTER_OFF:
	default:
		menuw = widgets.options.hp_off_btn;
		break;
	}
	XtVaSetValues(widgets.options.hp_opt,
		XmNmenuHistory, menuw,
		NULL
	);

	XtSetSensitive(
		widgets.options.lp_freq_txt,
		(Boolean) (app_data.lowpass_mode == FILTER_MANUAL)
	);
	XtSetSensitive(
		widgets.options.lp_width_txt,
		(Boolean) (app_data.lowpass_mode == FILTER_MANUAL)
	);
	XtSetSensitive(
		widgets.options.hp_freq_txt,
		(Boolean) (app_data.highpass_mode == FILTER_MANUAL)
	);
	XtSetSensitive(
		widgets.options.hp_width_txt,
		(Boolean) (app_data.highpass_mode == FILTER_MANUAL)
	);

	(void) sprintf(str, "%d", app_data.lowpass_freq);
	set_text_string(widgets.options.lp_freq_txt, str, FALSE);
	(void) sprintf(str, "%d", app_data.lowpass_width);
	set_text_string(widgets.options.lp_width_txt, str, FALSE);
	(void) sprintf(str, "%d", app_data.highpass_freq);
	set_text_string(widgets.options.hp_freq_txt, str, FALSE);
	(void) sprintf(str, "%d", app_data.highpass_width);
	set_text_string(widgets.options.hp_width_txt, str, FALSE);

	/* Set flags button states */
	XmToggleButtonSetState(
		widgets.options.copyrt_btn,
		(Boolean) app_data.copyright,
		False
	);
	XmToggleButtonSetState(
		widgets.options.orig_btn,
		(Boolean) app_data.original,
		False
	);
	XmToggleButtonSetState(
		widgets.options.nores_btn,
		(Boolean) app_data.nores,
		False
	);
	XmToggleButtonSetState(
		widgets.options.chksum_btn,
		(Boolean) app_data.checksum,
		False
	);

	/* Set add CD info tag button state */
	XmToggleButtonSetState(
		widgets.options.addtag_btn,
		(Boolean) app_data.add_tag,
		False
	);
	XtSetSensitive(
		widgets.options.id3tag_ver_opt,
		(Boolean) app_data.add_tag
	);

	/* Set ID3 tag mode */
	switch (app_data.id3tag_mode) {
	case ID3TAG_V1:
		menuw = widgets.options.id3tag_v1_btn;
		break;
	case ID3TAG_V2:
		menuw = widgets.options.id3tag_v2_btn;
		break;
	case ID3TAG_BOTH:
	default:
		menuw = widgets.options.id3tag_both_btn;
		break;
	}
	XtVaSetValues(widgets.options.id3tag_ver_opt,
		XmNmenuHistory, menuw,
		NULL
	);

	/* Set LAME options mode buttons */
	XmToggleButtonSetState(
		widgets.options.lame_disable_btn,
		(Boolean) (app_data.lameopts_mode == LAMEOPTS_DISABLE),
		False
	);
	XmToggleButtonSetState(
		widgets.options.lame_insert_btn,
		(Boolean) (app_data.lameopts_mode == LAMEOPTS_INSERT),
		False
	);
	XmToggleButtonSetState(
		widgets.options.lame_append_btn,
		(Boolean) (app_data.lameopts_mode == LAMEOPTS_APPEND),
		False
	);
	XmToggleButtonSetState(
		widgets.options.lame_replace_btn,
		(Boolean) (app_data.lameopts_mode == LAMEOPTS_REPLACE),
		False
	);

	/* Set LAME options string */
	set_text_string(
		widgets.options.lame_opts_txt,
		(app_data.lame_opts == NULL) ? "" : app_data.lame_opts,
		FALSE
	);
	XtSetSensitive(
		widgets.options.lame_opts_lbl,
		(Boolean) (app_data.lameopts_mode != LAMEOPTS_DISABLE)
	);
	XtSetSensitive(
		widgets.options.lame_opts_txt,
		(Boolean) (app_data.lameopts_mode != LAMEOPTS_DISABLE)
	);

	/* Set CDDA read/write scheduling priority buttons */
	XmToggleButtonSetState(
		widgets.options.sched_rd_hipri_btn,
		(Boolean) ((app_data.cdda_sched & 0x01) != 0),
		False
	);
	XmToggleButtonSetState(
		widgets.options.sched_rd_norm_btn,
		(Boolean)
		!XmToggleButtonGetState(widgets.options.sched_rd_hipri_btn),
		False
	);
	XmToggleButtonSetState(
		widgets.options.sched_wr_hipri_btn,
		(Boolean) ((app_data.cdda_sched & 0x02) != 0),
		False
	);
	XmToggleButtonSetState(
		widgets.options.sched_wr_norm_btn,
		(Boolean)
		!XmToggleButtonGetState(widgets.options.sched_wr_hipri_btn),
		False
	);

	(void) sprintf(str, "%d", app_data.hb_timeout);
	set_text_string(widgets.options.hb_tout_txt, str, FALSE);

	/* Set automation states buttons */
	XmToggleButtonSetState(
		widgets.options.load_none_btn,
		(Boolean) (!app_data.load_spindown && !app_data.load_play),
		False
	);
	XmToggleButtonSetState(
		widgets.options.load_spdn_btn,
		(Boolean) app_data.load_spindown,
		False
	);
	XmToggleButtonSetState(
		widgets.options.load_play_btn,
		(Boolean) app_data.load_play,
		False
	);
	XmToggleButtonSetState(
		widgets.options.load_lock_btn,
		(Boolean) app_data.caddy_lock,
		False
	);

	XmToggleButtonSetState(
		widgets.options.exit_none_btn,
		(Boolean) (!app_data.exit_stop && !app_data.exit_eject),
		False
	);
	XmToggleButtonSetState(
		widgets.options.exit_stop_btn,
		(Boolean) app_data.exit_stop,
		False
	);
	XmToggleButtonSetState(
		widgets.options.exit_eject_btn,
		(Boolean) app_data.exit_eject,
		False
	);

	XmToggleButtonSetState(
		widgets.options.done_eject_btn,
		(Boolean) app_data.done_eject,
		False
	);

	XmToggleButtonSetState(
		widgets.options.done_exit_btn,
		(Boolean) app_data.done_exit,
		False
	);

	XmToggleButtonSetState(
		widgets.options.eject_exit_btn,
		(Boolean) app_data.eject_exit,
		False
	);

	/* Set CD changer options buttons */
	XmToggleButtonSetState(
		widgets.options.chg_multiplay_btn,
		(Boolean) app_data.multi_play,
		False
	);

	XmToggleButtonSetState(
		widgets.options.chg_reverse_btn,
		(Boolean) app_data.reverse,
		False
	);

	/* Set volume/balance settings buttons */
	XmToggleButtonSetState(
		widgets.options.vol_linear_btn,
		(Boolean) (app_data.vol_taper == VOLTAPER_LINEAR),
		False
	);
	XmToggleButtonSetState(
		widgets.options.vol_square_btn,
		(Boolean) (app_data.vol_taper == VOLTAPER_SQR),
		False
	);
	XmToggleButtonSetState(
		widgets.options.vol_invsqr_btn,
		(Boolean) (app_data.vol_taper == VOLTAPER_INVSQR),
		False
	);

	/* Set channel routing buttons */
	XmToggleButtonSetState(
		widgets.options.chroute_stereo_btn,
		(Boolean) (app_data.ch_route == CHROUTE_NORMAL),
		False
	);
	XmToggleButtonSetState(
		widgets.options.chroute_rev_btn,
		(Boolean) (app_data.ch_route == CHROUTE_REVERSE),
		False
	);
	XmToggleButtonSetState(
		widgets.options.chroute_left_btn,
		(Boolean) (app_data.ch_route == CHROUTE_L_MONO),
		False
	);
	XmToggleButtonSetState(
		widgets.options.chroute_right_btn,
		(Boolean) (app_data.ch_route == CHROUTE_R_MONO),
		False
	);
	XmToggleButtonSetState(
		widgets.options.chroute_mono_btn,
		(Boolean) (app_data.ch_route == CHROUTE_MONO),
		False
	);

	/* Set CDDA playback output port selection buttons */
	XmToggleButtonSetState(
		widgets.options.outport_spkr_btn,
		(Boolean) ((app_data.outport & CDDA_OUT_SPEAKER) != 0),
		False
	);
	XmToggleButtonSetState(
		widgets.options.outport_phone_btn,
		(Boolean) ((app_data.outport & CDDA_OUT_HEADPHONE) != 0),
		False
	);
	XmToggleButtonSetState(
		widgets.options.outport_line_btn,
		(Boolean) ((app_data.outport & CDDA_OUT_LINEOUT) != 0),
		False
	);

	/* Set CDDB & CD-TEXT config widgets */
	XmToggleButtonSetState(
		widgets.options.autobr_btn,
		(Boolean) app_data.auto_musicbrowser,
		False
	);
	XmToggleButtonSetState(
		widgets.options.manbr_btn,
		(Boolean) !app_data.auto_musicbrowser,
		False
	);
	XtSetSensitive(
		widgets.options.autobr_btn,
		(Boolean) (cdinfo_cddb_ver() == 2)
	);
	XtSetSensitive(
		widgets.options.manbr_btn,
		(Boolean) (cdinfo_cddb_ver() == 2)
	);

	XtSetSensitive(
		widgets.options.cddb_pri_btn,
		(Boolean) cdinfo_cddb_iscfg()
	);
	XtSetSensitive(
		widgets.options.cdtext_pri_btn,
		(Boolean) (cdinfo_cdtext_iscfg() && !app_data.cdtext_dsbl)
	);

	/* Find whether cdinfoPath gives CDDB or CD-TEXT priority */
	pri = CDINFO_RMT;
	cdp = cdinfo_addr();
	for (pp = cdp->pathlist; pp != NULL; pp = pp->next) {
		if (pp->type == CDINFO_RMT || pp->type == CDINFO_CDTEXT) {
			pri = pp->type;
			break;
		}
	}

	/* Override cdinfoPath if necessary */
	if (pri == CDINFO_RMT &&
	    !XtIsSensitive(widgets.options.cddb_pri_btn) &&
	    XtIsSensitive(widgets.options.cdtext_pri_btn)) {
		pri = CDINFO_CDTEXT;
	}
	else if (pri == CDINFO_CDTEXT &&
		 !XtIsSensitive(widgets.options.cdtext_pri_btn) &&
		 XtIsSensitive(widgets.options.cddb_pri_btn)) {
		pri = CDINFO_RMT;
	}

	/* Set the toggle button states */
	XmToggleButtonSetState(
		widgets.options.cddb_pri_btn,
		(Boolean) (pri == CDINFO_RMT),
		False
	);
	XmToggleButtonSetState(
		widgets.options.cdtext_pri_btn,
		(Boolean) (pri == CDINFO_CDTEXT),
		False
	);

	(void) sprintf(str, "%d", app_data.srv_timeout);
	set_text_string(widgets.options.serv_tout_txt, str, FALSE);

	(void) sprintf(str, "%d", app_data.cache_timeout);
	set_text_string(widgets.options.cache_tout_txt, str, FALSE);

	XmToggleButtonSetState(
		widgets.options.use_proxy_btn,
		(Boolean) app_data.use_proxy,
		False
	);
	XmToggleButtonSetState(
		widgets.options.proxy_auth_btn,
		(Boolean) app_data.proxy_auth,
		False
	);

	if ((cp = strrchr(app_data.proxy_server, ':')) != NULL) {
		*cp = '\0';
		set_text_string(
			widgets.options.proxy_srvr_txt,
			app_data.proxy_server,
			FALSE
		);
		set_text_string(widgets.options.proxy_port_txt, ++cp, FALSE);
		cp--;
		*cp = ':';
	}
	else {
		set_text_string(
			widgets.options.proxy_srvr_txt,
			app_data.proxy_server,
			FALSE
		);
		set_text_string(widgets.options.proxy_port_txt, "80", FALSE);
	}

	XtSetSensitive(
		widgets.options.proxy_srvr_lbl,
		(Boolean) app_data.use_proxy
	);
	XtSetSensitive(
		widgets.options.proxy_srvr_txt,
		(Boolean) app_data.use_proxy
	);
	XtSetSensitive(
		widgets.options.proxy_port_lbl,
		(Boolean) app_data.use_proxy
	);
	XtSetSensitive(
		widgets.options.proxy_port_txt,
		(Boolean) app_data.use_proxy
	);
	XtSetSensitive(
		widgets.options.proxy_auth_btn,
		(Boolean) app_data.use_proxy
	);

	/* Make the Reset and Save buttons insensitive */
	XtSetSensitive(widgets.options.reset_btn, False);
	XtSetSensitive(widgets.options.save_btn, False);
}


/*
 * cd_options_save
 *	Options window save button callback function
 */
/*ARGSUSED*/
void
cd_options_save(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	char		*bdevname,
			path[FILE_PATH_SZ + 2];

	DBGPRN(DBG_UI)(errfp, "\n* OPTIONS SAVE\n");

	if (!util_newstr(&app_data.cdda_tmpl, s->outf_tmpl)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	/* Change to watch cursor */
	cd_busycurs(TRUE, CURS_ALL);

#ifdef __VMS
	bdevname = NULL;
	if (!util_newstr(&bdevname, "device.cfg")) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
#else
	if ((bdevname = util_basename(app_data.device)) == NULL) {
		CD_FATAL("Device path basename error: Aborting.");
		return;
	}
#endif

	/* Save user common configuration parameters */
	(void) sprintf(path, USR_CMCFG_PATH,
		       util_homedir(util_get_ouid()));
	di_common_parmsave(path);

	/* Save user device-specific configuration parameters */
	(void) sprintf(path, USR_DSCFG_PATH,
		       util_homedir(util_get_ouid()),
		       bdevname);
	di_devspec_parmsave(path);

	MEM_FREE(bdevname);

	/* Make the Reset and Save buttons insensitive */
	XtSetSensitive(widgets.options.reset_btn, False);
	XtSetSensitive(widgets.options.save_btn, False);

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);
}


/*
 * cd_options
 *	Options window toggle button callback function
 */
void
cd_options(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmRowColumnCallbackStruct	*p =
		(XmRowColumnCallbackStruct *)(void *) call_data;
	XmToggleButtonCallbackStruct	*q;
	curstat_t			*s = (curstat_t *)(void *) client_data;
	cdinfo_incore_t			*cdp;
	cdinfo_path_t			*pp;

	if (p == NULL)
		return;

	q = (XmToggleButtonCallbackStruct *)(void *) p->callbackstruct;

	if (w == widgets.options.mode_chkbox) {
		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			/* Disallow changing the mode while playing */
			cd_beep();

			XmToggleButtonSetState(
				widgets.options.mode_std_btn,
				(Boolean)
				PLAYMODE_IS_STD(app_data.play_mode),
				False
			);
			XmToggleButtonSetState(
				widgets.options.mode_cdda_btn,
				(Boolean)
				((app_data.play_mode & PLAYMODE_CDDA) != 0),
				False
			);
			XmToggleButtonSetState(
				widgets.options.mode_file_btn,
				(Boolean)
				((app_data.play_mode & PLAYMODE_FILE) != 0),
				False
			);
			XmToggleButtonSetState(
				widgets.options.mode_pipe_btn,
				(Boolean)
				((app_data.play_mode & PLAYMODE_PIPE) != 0),
				False
			);
			return;

		default:
			break;
		}

		if (p->widget == widgets.options.mode_std_btn) {
			if (!q->set)
				return;
			DBGPRN(DBG_UI)(errfp, "\n* OPTION: playMode=STD\n");
			app_data.play_mode = PLAYMODE_STD;
		}
		else if (p->widget == widgets.options.mode_cdda_btn) {
			if (q->set) {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: playMode+=CDDA\n");
				app_data.play_mode |= PLAYMODE_CDDA;
			}
			else {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: playMode-=CDDA\n");
				app_data.play_mode &= ~PLAYMODE_CDDA;
			}
		}
		else if (p->widget == widgets.options.mode_file_btn) {
			if (q->set) {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: playMode+=FILE\n");
				app_data.play_mode |= PLAYMODE_FILE;
			}
			else {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: playMode-=FILE\n");
				app_data.play_mode &= ~PLAYMODE_FILE;
			}
		}
		else if (p->widget == widgets.options.mode_pipe_btn) {
			if (q->set) {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: playMode+=PIPE\n");
				app_data.play_mode |= PLAYMODE_PIPE;
			}
			else {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: playMode-=PIPE\n");
				app_data.play_mode &= ~PLAYMODE_PIPE;
			}
		}

		XmToggleButtonSetState(
			widgets.options.mode_std_btn,
			(Boolean) PLAYMODE_IS_STD(app_data.play_mode),
			False
		);

		if (!di_playmode(s)) {
			cd_beep();
			CD_INFO(app_data.str_cddainit_fail);
			XmToggleButtonSetState(
				p->widget,
				(Boolean) !q->set,
				False
			);
			XmToggleButtonSetState(
				widgets.options.mode_std_btn,
				True,
				False
			);
			app_data.play_mode = PLAYMODE_STD;
		}

		if (PLAYMODE_IS_STD(app_data.play_mode)) {
		    /* Playback mode related controls */
		    XmToggleButtonSetState(
			widgets.options.mode_cdda_btn, False, False
		    );
		    XmToggleButtonSetState(
			widgets.options.mode_file_btn, False, False
		    );
		    XmToggleButtonSetState(
			widgets.options.mode_pipe_btn, False, False
		    );
		    XtSetSensitive(widgets.options.mode_jitter_btn, False);
		    XtSetSensitive(widgets.options.mode_trkfile_btn, False);
		    XtSetSensitive(widgets.options.mode_subst_btn, False);
		    XtSetSensitive(widgets.options.mode_fmt_opt, False);
		    XtSetSensitive(widgets.options.mode_path_lbl, False);
		    XtSetSensitive(widgets.options.mode_path_txt, False);
		    XtSetSensitive(widgets.options.mode_prog_lbl, False);
		    XtSetSensitive(widgets.options.mode_prog_txt, False);

		    /* Channel routing controls */
		    XtSetSensitive(
			    widgets.options.chroute_rev_btn,
			    (Boolean) app_data.chroute_supp
		    );
		    XtSetSensitive(
			    widgets.options.chroute_left_btn,
			    (Boolean) app_data.chroute_supp
		    );
		    XtSetSensitive(
			    widgets.options.chroute_right_btn,
			    (Boolean) app_data.chroute_supp
		    );
		    XtSetSensitive(
			    widgets.options.chroute_mono_btn,
			    (Boolean) app_data.chroute_supp
		    );

		    /* Disable CDDA outport port selector */
		    XtSetSensitive(widgets.options.outport_lbl, False);
		    XtSetSensitive(widgets.options.outport_spkr_btn, False);
		    XtSetSensitive(widgets.options.outport_phone_btn, False);
		    XtSetSensitive(widgets.options.outport_line_btn, False);

		    /* Enable square/invsqr volume taper buttons */
		    XtSetSensitive(widgets.options.vol_square_btn, True);
		    XtSetSensitive(widgets.options.vol_invsqr_btn, True);

		    /* Enable balance control */
		    XtSetSensitive(widgets.options.bal_lbl, True);
		    XtSetSensitive(widgets.options.ball_lbl, True);
		    XtSetSensitive(widgets.options.balr_lbl, True);
		    XtSetSensitive(widgets.options.bal_scale, True);
		    XtSetSensitive(widgets.options.balctr_btn, True);

		    /* Disable the CDDA attenuator slider */
		    XtSetSensitive(widgets.options.cdda_att_lbl, False);
		    XtSetSensitive(widgets.options.cdda_att_scale, False);
		    XtSetSensitive(widgets.options.cdda_fadeout_btn, False);
		    XtSetSensitive(widgets.options.cdda_fadein_btn, False);
		    XtSetSensitive(widgets.options.cdda_fadeout_lbl, False);
		    XtSetSensitive(widgets.options.cdda_fadein_lbl, False);
		}
		else {
		    /* Playback mode related controls */
		    XtSetSensitive(widgets.options.mode_jitter_btn, True);
		    XtSetSensitive(widgets.options.mode_trkfile_btn,
			(Boolean) (app_data.play_mode & PLAYMODE_FILE)
		    );
		    XtSetSensitive(widgets.options.mode_subst_btn,
			(Boolean) (app_data.play_mode & PLAYMODE_FILE)
		    );
		    XtSetSensitive(widgets.options.mode_fmt_opt,
			(Boolean)
			(app_data.play_mode & (PLAYMODE_FILE | PLAYMODE_PIPE))
		    );
		    XtSetSensitive(widgets.options.mode_path_lbl,
			(Boolean) (app_data.play_mode & PLAYMODE_FILE)
		    );
		    XtSetSensitive(widgets.options.mode_path_txt,
			(Boolean) (app_data.play_mode & PLAYMODE_FILE)
		    );
		    XtSetSensitive(widgets.options.mode_prog_lbl,
			(Boolean) (app_data.play_mode & PLAYMODE_PIPE)
		    );
		    XtSetSensitive(widgets.options.mode_prog_txt,
			(Boolean) (app_data.play_mode & PLAYMODE_PIPE)
		    );

		    /* Enable channel routing controls */
		    XtSetSensitive(widgets.options.chroute_rev_btn, True);
		    XtSetSensitive(widgets.options.chroute_left_btn, True);
		    XtSetSensitive(widgets.options.chroute_right_btn, True);
		    XtSetSensitive(widgets.options.chroute_mono_btn, True);

		    /* CDDA outport port selector */
		    XtSetSensitive(widgets.options.outport_lbl,
			(Boolean) (app_data.play_mode & PLAYMODE_CDDA)
		    );
		    XtSetSensitive(widgets.options.outport_spkr_btn,
			(Boolean) (app_data.play_mode & PLAYMODE_CDDA)
		    );
		    XtSetSensitive(widgets.options.outport_phone_btn,
			(Boolean) (app_data.play_mode & PLAYMODE_CDDA)
		    );
		    XtSetSensitive(widgets.options.outport_line_btn,
			(Boolean) (app_data.play_mode & PLAYMODE_CDDA)
		    );

		    /* Only allow linear volume taper in CDDA modes */
		    app_data.vol_taper = VOLTAPER_LINEAR;
		    XmToggleButtonSetState(
			    widgets.options.vol_linear_btn, True, False
		    );
		    XmToggleButtonSetState(
			    widgets.options.vol_square_btn, False, False
		    );
		    XmToggleButtonSetState(
			    widgets.options.vol_invsqr_btn, False, False
		    );
		    XtSetSensitive(widgets.options.vol_square_btn, False);
		    XtSetSensitive(widgets.options.vol_invsqr_btn, False);

		    /* Only enable balance control for CDDA playback */
		    XtSetSensitive(
			    widgets.options.bal_lbl,
			    (Boolean)
			    ((app_data.play_mode & PLAYMODE_CDDA) != 0)
		    );
		    XtSetSensitive(
			    widgets.options.ball_lbl,
			    (Boolean)
			    ((app_data.play_mode & PLAYMODE_CDDA) != 0)
		    );
		    XtSetSensitive(
			    widgets.options.balr_lbl,
			    (Boolean)
			    ((app_data.play_mode & PLAYMODE_CDDA) != 0)
		    );
		    XtSetSensitive(
			    widgets.options.bal_scale,
			    (Boolean)
			    ((app_data.play_mode & PLAYMODE_CDDA) != 0)
		    );
		    XtSetSensitive(
			    widgets.options.balctr_btn,
			    (Boolean)
			    ((app_data.play_mode & PLAYMODE_CDDA) != 0)
		    );

		    /* Enable the CDDA attenuator slider */
		    XtSetSensitive(widgets.options.cdda_att_lbl, True);
		    XtSetSensitive(widgets.options.cdda_att_scale, True);
		    XtSetSensitive(widgets.options.cdda_fadeout_btn, True);
		    XtSetSensitive(widgets.options.cdda_fadein_btn, True);
		    XtSetSensitive(widgets.options.cdda_fadeout_lbl, True);
		    XtSetSensitive(widgets.options.cdda_fadein_lbl, True);
		}
	}
	else if (w == widgets.options.lame_radbox) {
		if (!q->set)
			return;

		if (p->widget == widgets.options.lame_disable_btn) {
			if (app_data.lameopts_mode == LAMEOPTS_DISABLE)
				return;		/* No change */
			app_data.lameopts_mode = LAMEOPTS_DISABLE;
		}
		else if (p->widget == widgets.options.lame_insert_btn) {
			if (app_data.lameopts_mode == LAMEOPTS_INSERT)
				return;		/* No change */
			app_data.lameopts_mode = LAMEOPTS_INSERT;
		}
		else if (p->widget == widgets.options.lame_append_btn) {
			if (app_data.lameopts_mode == LAMEOPTS_APPEND)
				return;		/* No change */
			app_data.lameopts_mode = LAMEOPTS_APPEND;
		}
		else if (p->widget == widgets.options.lame_replace_btn) {
			if (app_data.lameopts_mode == LAMEOPTS_REPLACE)
				return;		/* No change */
			app_data.lameopts_mode = LAMEOPTS_REPLACE;
		}

		XtSetSensitive(
			widgets.options.lame_opts_lbl,
			(Boolean) (app_data.lameopts_mode != LAMEOPTS_DISABLE)
		);
		XtSetSensitive(
			widgets.options.lame_opts_txt,
			(Boolean) (app_data.lameopts_mode != LAMEOPTS_DISABLE)
		);
		if (app_data.lameopts_mode != LAMEOPTS_DISABLE) {
			XmProcessTraversal(
				widgets.options.lame_opts_txt,
				XmTRAVERSE_CURRENT
			);
		}

		DBGPRN(DBG_UI)(errfp, "\n* OPTION: lameOptionsMode=%d\n",
			app_data.lameopts_mode);
	}
	else if (w == widgets.options.sched_rd_radbox) {
		if (!q->set)
			return;

		if (p->widget == widgets.options.sched_rd_norm_btn) {
			if ((app_data.cdda_sched & 0x01) == 0)
				return;		/* No change */

			app_data.cdda_sched &= ~0x01;
		}
		else if (p->widget == widgets.options.sched_rd_hipri_btn) {
			if ((app_data.cdda_sched & 0x01) != 0)
				return;		/* No change */

			app_data.cdda_sched |= 0x01;
		}

		DBGPRN(DBG_UI)(errfp, "\n* OPTION: cddaSchedOptions=%d\n",
			app_data.cdda_sched);
	}
	else if (w == widgets.options.sched_wr_radbox) {
		if (!q->set)
			return;

		if (p->widget == widgets.options.sched_wr_norm_btn) {
			if ((app_data.cdda_sched & 0x02) == 0)
				return;		/* No change */

			app_data.cdda_sched &= ~0x02;
		}
		else if (p->widget == widgets.options.sched_wr_hipri_btn) {
			if ((app_data.cdda_sched & 0x02) != 0)
				return;		/* No change */

			app_data.cdda_sched |= 0x02;
		}

		DBGPRN(DBG_UI)(errfp, "\n* OPTION: cddaSchedOptions=%d\n",
			app_data.cdda_sched);
	}
	else if (w == widgets.options.load_chkbox) {
		if (p->widget == widgets.options.load_none_btn) {
			if (!q->set) {
				XmToggleButtonSetState(p->widget, True, False);
				return;
			}
			XmToggleButtonSetState(
				widgets.options.load_spdn_btn,
				False, False
			);
			XmToggleButtonSetState(
				widgets.options.load_play_btn,
				False, False
			);
			DBGPRN(DBG_UI)(errfp,
				"\n* OPTION: spinDownOnLoad=0 playOnLoad=0\n");
			app_data.load_spindown = FALSE;
			app_data.load_play = FALSE;
		}
		else if (p->widget == widgets.options.load_spdn_btn) {
			if (!q->set) {
				XmToggleButtonSetState(p->widget, True, False);
				return;
			}
			XmToggleButtonSetState(
				widgets.options.load_none_btn,
				False, False
			);
			XmToggleButtonSetState(
				widgets.options.load_play_btn,
				False, False
			);
			DBGPRN(DBG_UI)(errfp,
				"\n* OPTION: spinDownOnLoad=1 playOnLoad=0\n");
			app_data.load_spindown = TRUE;
			app_data.load_play = FALSE;
		}
		else if (p->widget == widgets.options.load_play_btn) {
			if (!q->set) {
				XmToggleButtonSetState(p->widget, True, False);
				return;
			}
			XmToggleButtonSetState(
				widgets.options.load_none_btn,
				False, False
			);
			XmToggleButtonSetState(
				widgets.options.load_spdn_btn,
				False, False
			);
			DBGPRN(DBG_UI)(errfp,
				"\n* OPTION: spinDownOnLoad=0 playOnLoad=1\n");
			app_data.load_play = TRUE;
			app_data.load_spindown = FALSE;
		}
		else if (p->widget == widgets.options.load_lock_btn) {
			if (app_data.caddylock_supp) {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: caddyLock=%d\n",
				       q->set);
				app_data.caddy_lock = (bool_t) q->set;
			}
			else {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: caddyLock=0\n");
				cd_beep();
				XmToggleButtonSetState(
					p->widget,
					False,
					False
				);
				return;
			}
		}
	}
	else if (w == widgets.options.exit_radbox) {
		if (!q->set)
			return;

		if (p->widget == widgets.options.exit_none_btn) {
			if (!app_data.exit_stop && !app_data.exit_eject)
				return;		/* No change */

			DBGPRN(DBG_UI)(errfp,
				"\n* OPTION: stopOnExit=0 ejectOnExit=0\n");
			app_data.exit_stop = app_data.exit_eject = FALSE;
		}
		else if (p->widget == widgets.options.exit_stop_btn) {
			if (app_data.exit_stop)
				return;		/* No change */

			DBGPRN(DBG_UI)(errfp,
				"\n* OPTION: stopOnExit=1 ejectOnExit=0\n");
			app_data.exit_stop = TRUE;
			app_data.exit_eject = FALSE;
		}
		else if (p->widget == widgets.options.exit_eject_btn) {
			if (app_data.exit_eject)
				return;		/* No change */

			if (app_data.eject_supp) {
				DBGPRN(DBG_UI)(errfp, "\n* OPTION: "
					"stopOnExit=0 ejectOnExit=1\n");
				app_data.exit_stop = FALSE;
				app_data.exit_eject = TRUE;
			}
			else {
				cd_beep();
				XmToggleButtonSetState(
					p->widget,
					False,
					False
				);
				if (app_data.exit_stop) {
					XmToggleButtonSetState(
						widgets.options.exit_stop_btn,
						True,
						False
					);
				}
				else {
					XmToggleButtonSetState(
						widgets.options.exit_none_btn,
						True,
						False
					);
				}
			}
		}
	}
	else if (w == widgets.options.done_chkbox) {
		if (p->widget == widgets.options.done_eject_btn) {
			if (app_data.eject_supp) {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: ejectOnDone=%d\n",
					q->set);
				app_data.done_eject = (bool_t) q->set;
			}
			else {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: ejectOnDone=0\n");
				cd_beep();
				XmToggleButtonSetState(
					p->widget,
					False,
					False
				);
				return;
			}
		}
		else if (p->widget == widgets.options.done_exit_btn) {
			DBGPRN(DBG_UI)(errfp, "\n* OPTION: exitOnDone=%d\n",
			       q->set);
			app_data.done_exit = (bool_t) q->set;
		}
	}
	else if (w == widgets.options.eject_chkbox) {
		if (p->widget == widgets.options.eject_exit_btn) {
			if (app_data.eject_supp) {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: exitOnEject=%d\n",
					q->set);
				app_data.eject_exit = (bool_t) q->set;
			}
			else {
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: exitOnEject=0\n");
				cd_beep();
				XmToggleButtonSetState(
					p->widget,
					False,
					False
				);
				return;
			}
		}
	}
	else if (w == widgets.options.chg_chkbox) {
		if (s->first_disc == s->last_disc) {
			/* Single-disc player: inhibit any change here */
			cd_beep();
			XmToggleButtonSetState(p->widget, False, False);
			return;
		}

		if (p->widget == widgets.options.chg_multiplay_btn) {
			DBGPRN(DBG_UI)(errfp, "\n* OPTION: multiPlay=%d\n",
			       q->set);
			app_data.multi_play = (bool_t) q->set;

			if (!app_data.multi_play) {
				app_data.reverse = FALSE;
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: reversePlay=0\n");
				XmToggleButtonSetState(
					widgets.options.chg_reverse_btn,
					False,
					False
				);
			}
		}
		else if (p->widget == widgets.options.chg_reverse_btn) {
			DBGPRN(DBG_UI)(errfp, "\n* OPTION: reversePlay=%d\n",
			       q->set);
			app_data.reverse = (bool_t) q->set;

			if (app_data.reverse) {
				app_data.multi_play = TRUE;
				DBGPRN(DBG_UI)(errfp,
					"\n* OPTION: multiPlay=0\n");
				XmToggleButtonSetState(
					widgets.options.chg_multiplay_btn,
					True,
					False
				);
			}
		}
	}
	else if (w == widgets.options.chroute_radbox) {
		if (!q->set)
			return;

		if (p->widget == widgets.options.chroute_stereo_btn) {
			if (app_data.ch_route == CHROUTE_NORMAL)
				return;		/* No change */

			app_data.ch_route = CHROUTE_NORMAL;
		}
		else if (p->widget == widgets.options.chroute_rev_btn) {
			if (app_data.ch_route == CHROUTE_REVERSE)
				return;		/* No change */

			app_data.ch_route = CHROUTE_REVERSE;
		}
		else if (p->widget == widgets.options.chroute_left_btn) {
			if (app_data.ch_route == CHROUTE_L_MONO)
				return;		/* No change */

			app_data.ch_route = CHROUTE_L_MONO;
		}
		else if (p->widget == widgets.options.chroute_right_btn) {
			if (app_data.ch_route == CHROUTE_R_MONO)
				return;		/* No change */

			app_data.ch_route = CHROUTE_R_MONO;
		}
		else if (p->widget == widgets.options.chroute_mono_btn) {
			if (app_data.ch_route == CHROUTE_MONO)
				return;		/* No change */

			app_data.ch_route = CHROUTE_MONO;
		}

		DBGPRN(DBG_UI)(errfp, "\n* OPTION: channelRoute=%d\n",
				app_data.ch_route);
		di_route(s);
	}
	else if (w == widgets.options.outport_chkbox) {
		char	*port;

		if (p->widget == widgets.options.outport_spkr_btn) {
			port = "SPEAKER";

			if (q->set)
				app_data.outport |= CDDA_OUT_SPEAKER;
			else
				app_data.outport &= ~CDDA_OUT_SPEAKER;
		}
		else if (p->widget == widgets.options.outport_phone_btn) {
			port = "HEADPHONE";

			if (q->set)
				app_data.outport |= CDDA_OUT_HEADPHONE;
			else
				app_data.outport &= ~CDDA_OUT_HEADPHONE;
		}
		else if (p->widget == widgets.options.outport_line_btn) {
			port = "LINEOUT";

			if (q->set)
				app_data.outport |= CDDA_OUT_LINEOUT;
			else
				app_data.outport &= ~CDDA_OUT_LINEOUT;
		}

		DBGPRN(DBG_UI)(errfp, "\n* OPTION: outport%s=%s\n",
		       q->set ? "+" : "-", port);

		cdda_outport();
	}
	else if (w == widgets.options.vol_radbox) {
		if (!q->set)
			return;

		if (p->widget == widgets.options.vol_linear_btn) {
			if (app_data.vol_taper == VOLTAPER_LINEAR)
				return;		/* No change */

			app_data.vol_taper = VOLTAPER_LINEAR;
		}
		else if (p->widget == widgets.options.vol_square_btn) {
			if (app_data.vol_taper == VOLTAPER_SQR)
				return;		/* No change */

			app_data.vol_taper = VOLTAPER_SQR;
		}
		else if (p->widget == widgets.options.vol_invsqr_btn) {
			if (app_data.vol_taper == VOLTAPER_INVSQR)
				return;		/* No change */

			app_data.vol_taper = VOLTAPER_INVSQR;
		}

		DBGPRN(DBG_UI)(errfp, "\n* OPTION: volumeControlTaper=%d\n",
				app_data.vol_taper);
		di_level(s, (byte_t) s->level, TRUE);
	}
	else if (w == widgets.options.mbrowser_radbox) {
		if (!q->set)
			return;

		if (p->widget == widgets.options.autobr_btn) {
			if (app_data.auto_musicbrowser)
				return;		/* No change */

			app_data.auto_musicbrowser = TRUE;
		}
		else if (p->widget == widgets.options.manbr_btn) {
			if (!app_data.auto_musicbrowser)
				return;		/* No change */

			app_data.auto_musicbrowser = FALSE;
		}
		DBGPRN(DBG_UI)(errfp, "\n* OPTION: autoMusicBrowser=%d\n",
				(int) app_data.auto_musicbrowser);
	}
	else if (w == widgets.options.lookup_radbox) {
		char	*path,
			*cp,
			*cp2;
		int	i,
			type[2];

		if (!q->set)
			return;

		if (p->widget == widgets.options.cddb_pri_btn) {
			type[0] = CDINFO_RMT;
			type[1] = CDINFO_CDTEXT;
		}
		else if (p->widget == widgets.options.cdtext_pri_btn) {
			type[0] = CDINFO_CDTEXT;
			type[1] = CDINFO_RMT;
		}

		cdp = cdinfo_addr();

		i = 0;
		for (pp = cdp->pathlist; pp != NULL; pp = pp->next) {
			if (pp->type != CDINFO_RMT &&
			    pp->type != CDINFO_CDTEXT)
				continue;

			if (i == 0 && pp->type == type[i])
				return;		/* No change */

			pp->type = type[i];
			if (++i > 1)
				break;
		}

		/* Allocate new cdinfo_path string */
		cp = (char *) MEM_ALLOC(
			"cdinfo_path",
			strlen(app_data.cdinfo_path) + 8
		);
		if (cp == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		cp[0] = '\0';

		i = 0;
		path = app_data.cdinfo_path;
		while ((cp2 = strchr(path, CDINFOPATH_SEPCHAR)) != NULL) {
			*cp2 = '\0';

			PATHENT_APPEND(i, cp, path, type[i]);

			(void) sprintf(cp, "%s%c", cp, CDINFOPATH_SEPCHAR);
			path = cp2 + 1;
		}
		PATHENT_APPEND(i, cp, path, type[i]);

		/* Replace cdinfo_path string */
		MEM_FREE(app_data.cdinfo_path);
		app_data.cdinfo_path = cp;

		DBGPRN(DBG_UI)(errfp, "\n* OPTION: Lookup priority: %s\n",
			(type[0] == CDINFO_RMT) ? "CDDB" : "CD-TEXT");
	}

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_options_categsel
 *	Options window category list selection callback function
 */
/*ARGSUSED*/
void
cd_options_categsel(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct	*p =
		(XmListCallbackStruct *)(void *) call_data;
	int			i;
	wlist_t			*wl;
	static int		prev = -1;

	if (p->reason != XmCR_BROWSE_SELECT && p->reason != XmCR_ACTIVATE)
		return;

	i = p->item_position - 1;
	if (prev == i)
		return;		/* No change */

	if (prev != -1) {
		/* Unmanage the previously selected category */
		for (wl = opt_categ[prev].widgets; wl != NULL; wl = wl->next)
			XtUnmanageChild(wl->w);
	}

	/* Manage the selected category widgets */
	for (wl = opt_categ[i].widgets; wl != NULL; wl = wl->next)
		XtManageChild(wl->w);

	prev = i;
}


/*
 * cd_jitter_corr
 *	Options window CDDA jitter correction toggle button callback
 */
/*ARGSUSED*/
void
cd_jitter_corr(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	if ((bool_t) p->set == app_data.cdda_jitter_corr)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp,
		"\n* CDDA Jitter Correction: %s\n", p->set ? "On" : "Off");

	if (di_isdemo())
		return;	/* Don't actually do anything in demo mode */

	app_data.cdda_jitter_corr = (bool_t) p->set;

	/* Notify device interface of the change */
	di_cddajitter(s);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_file_per_trk
 *	Options window CDDA file-per-track toggle button callback
 */
/*ARGSUSED*/
void
cd_file_per_trk(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	if ((bool_t) p->set == app_data.cdda_trkfile)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp,
		"\n* CDDA File-per-track: %s\n", p->set ? "On" : "Off");

	app_data.cdda_trkfile = (bool_t) p->set;

	if (s->outf_tmpl != NULL) {
		MEM_FREE(s->outf_tmpl);
		s->outf_tmpl = NULL;
	}

	/* Set new output file path template */
	fix_outfile_path(s);

	set_text_string(widgets.options.mode_path_txt, s->outf_tmpl, TRUE);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_subst
 *	Options window CDDA file name underscore substitution
 *	toggle button callback
 */
/*ARGSUSED*/
void
cd_subst(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

#ifdef __VMS
	/* On VMS, force this toggle to be enabled at all times,
	 * because VMS file names do not allow spaces and tabs.
	 */
	if (!p->set) {
		cd_beep();
		XmToggleButtonSetState(w, True, False);
	}
#else
	if ((bool_t) p->set == app_data.subst_underscore)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp,
		"\n* CDDA Underscore Substitution: %s\n",
		p->set ? "On" : "Off");

	app_data.subst_underscore = (bool_t) p->set;

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
#endif
}


/*
 * cd_filefmt_mode
 *	File format mode selector callback function
 */
/*ARGSUSED*/
void
cd_filefmt_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;
	int		newfmt;

	if (p->reason != XmCR_ACTIVATE)
		return;

	newfmt = filefmt_code(w);

	if (newfmt == app_data.cdda_filefmt)
		return;	/* No change */

	app_data.cdda_filefmt = newfmt;

	/* Fix output file suffix to match the file type */
	fix_outfile_path(s);

	set_text_string(widgets.options.mode_path_txt, s->outf_tmpl, TRUE);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_filepath_new
 *	Save-to-file path text widget callback function
 */
/*ARGSUSED*/
void
cd_filepath_new(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;
	filefmt_t		*fmp,
				*fmp2;
	char			*cp,
				*cp2,
				*cp3,
				*suf,
				tmp[FILE_PATH_SZ * 2 + 8];
	int			i,
				newfmt;
	Widget			menuw;

	switch (p->reason) {
	case XmCR_VALUE_CHANGED:
	case XmCR_ACTIVATE:
	case XmCR_LOSING_FOCUS:
		if ((cp = get_text_string(w, TRUE)) == NULL)
			return;

		if (s->outf_tmpl != NULL && strcmp(cp, s->outf_tmpl) == 0) {
			/* Not changed */
			MEM_FREE(cp);

			if (p->reason == XmCR_ACTIVATE ||
			    p->reason == XmCR_LOSING_FOCUS) {
				/* Set cursor to beginning of text */
				XmTextSetInsertionPosition(w, 0);
			}
			return;
		}

		newfmt = app_data.cdda_filefmt;

		/* File suffix */
		if ((fmp = cdda_filefmt(app_data.cdda_filefmt)) == NULL)
			suf = "";
		else
			suf = fmp->suf;

		/* Check if file suffix indicates a change in file format */
		menuw = (Widget) NULL;
		if ((cp2 = strrchr(cp, '.')) != NULL) {
			if ((cp3 = strrchr(cp, DIR_END)) != NULL &&
			    cp2 > cp3) {
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

			menuw = filefmt_btn(newfmt);
		}
		else
			cp2 = "";

		if (!cdda_filefmt_supp(newfmt)) {
			/* Specified format not supported */
			if (p->reason == XmCR_ACTIVATE ||
			    p->reason == XmCR_LOSING_FOCUS) {
				/* Force back to original format */
				fix_outfile_path(s);
				set_text_string(w, s->outf_tmpl, TRUE);

				/* Set cursor to beginning of text */
				XmTextSetInsertionPosition(w, 0);
			}
			return;
		}

		if (!util_newstr(&s->outf_tmpl, cp)) {
			MEM_FREE(cp);
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		MEM_FREE(cp);

		/* File format changed */
		if (newfmt != app_data.cdda_filefmt) {
			XtVaSetValues(widgets.options.mode_fmt_opt,
				XmNmenuHistory, menuw,
				NULL
			);
			app_data.cdda_filefmt = newfmt;
		}

		/* Fix output file suffix to match the file type */
		fix_outfile_path(s);

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

		if (p->reason == XmCR_ACTIVATE ||
		    p->reason == XmCR_LOSING_FOCUS) {
			set_text_string(w, s->outf_tmpl, TRUE);

			/* Set cursor to beginning of text */
			XmTextSetInsertionPosition(w, 0);

			DBGPRN(DBG_UI)(errfp, "\n* CDDA file template: %s\n",
				       s->outf_tmpl);
		}

		break;

	default:
		cp2 = "";
		break;
	}

	(void) sprintf(tmp,
#ifdef __VMS
		"%%S.%%C.%%I]%s%s",
#else
		"%%S/%%C/%%I/%s%s",
#endif
		app_data.cdda_trkfile ? FILEPATH_TRACK : FILEPATH_DISC,
		cp2
	);

	if (strcmp(s->outf_tmpl, tmp) != 0) {
		/* Make the Reset and Save buttons sensitive */
		XtSetSensitive(widgets.options.reset_btn, True);
		XtSetSensitive(widgets.options.save_btn, True);
	}
}


/*
 * cd_filepath_exp
 *	File path expand pushbutton callback function
 */
/*ARGSUSED*/
void
cd_filepath_exp(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	chset_conv_t	*up;
	char		*a = NULL;
	int		i;
	bool_t		ret;

	/* call_data is NULL when this function is called via
	 * an update rather than a button click.
	 */
	if (call_data != NULL) {
		ret = pathexp_active;
		cd_info2_popdown(NULL);
		if (ret)
			return;
	}

	switch (s->mode) {
	case MOD_PLAY:
	case MOD_PAUSE:
	case MOD_SAMPLE:
		if ((app_data.play_mode & PLAYMODE_FILE) == 0)
			return;
		break;
	default:
		return;
	}

	if (app_data.cdda_trkfile) {
		if ((i = di_curtrk_pos(s)) < 0)
			return;
	}
	else
		i = 0;

	if (s->trkinfo[i].outfile == NULL)
		return;

	pathexp_active = TRUE;

	/* Convert text from UTF-8 to local charset if possible */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return;

	if (!util_chset_conv(up, s->trkinfo[i].outfile, &a, FALSE) &&
	    !util_newstr(&a, s->trkinfo[i].outfile)) {
		util_chset_close(up);
		return;
	}
	util_chset_close(up);

	CD_INFO2(app_data.str_pathexp, a);
	MEM_FREE(a);
}


/*
 * cd_pipeprog_new
 *	Pipe-to-program path text widget callback function
 */
/*ARGSUSED*/
void
cd_pipeprog_new(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	char			*cp;

	switch (p->reason) {
	case XmCR_VALUE_CHANGED:
	case XmCR_ACTIVATE:
	case XmCR_LOSING_FOCUS:
		if ((cp = get_text_string(w, TRUE)) == NULL)
			return;

		if (app_data.pipeprog != NULL &&
		    strcmp(cp, app_data.pipeprog) == 0) {
			/* Not changed */
			MEM_FREE(cp);

			if (p->reason == XmCR_ACTIVATE ||
			    p->reason == XmCR_LOSING_FOCUS) {
				/* Set cursor to beginning of text */
				XmTextSetInsertionPosition(w, 0);
			}
			return;
		}

		if (!util_newstr(&app_data.pipeprog, cp)) {
			MEM_FREE(cp);
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		MEM_FREE(cp);

		if (p->reason == XmCR_ACTIVATE ||
		    p->reason == XmCR_LOSING_FOCUS) {
			/* Set cursor to beginning of text */
			XmTextSetInsertionPosition(w, 0);

			DBGPRN(DBG_UI)(errfp, "\n* CDDA pipe program: %s\n",
				       app_data.pipeprog);
		}

		/* Make the Reset and Save buttons sensitive */
		XtSetSensitive(widgets.options.reset_btn, True);
		XtSetSensitive(widgets.options.save_btn, True);

		break;

	default:
		break;
	}
}


/*
 * cd_comp_mode
 *	Options window encoding method menu entry callback
 */
/*ARGSUSED*/
void
cd_comp_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct	*p =
		(XmPushButtonCallbackStruct *)(void *) call_data;
	int				newmode;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.mode0_btn)
		newmode = COMPMODE_0;
	else if (w == widgets.options.mode1_btn)
		newmode = COMPMODE_1;
	else if (w == widgets.options.mode2_btn)
		newmode = COMPMODE_2;
	else if (w == widgets.options.mode3_btn)
		newmode = COMPMODE_3;
	else
		return;

	if (app_data.comp_mode == newmode)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* Compression mode: Mode %d\n", newmode);
	app_data.comp_mode = newmode;

	switch (newmode) {
	case COMPMODE_1:
	case COMPMODE_2:
		XtSetSensitive(widgets.options.qualval_lbl, True);
		XtSetSensitive(widgets.options.qualval1_lbl, True);
		XtSetSensitive(widgets.options.qualval2_lbl, True);
		XtSetSensitive(widgets.options.qualval_scl, True);
		XtSetSensitive(widgets.options.bitrate_opt, False);
		XtSetSensitive(widgets.options.minbrate_opt, True);
		XtSetSensitive(widgets.options.maxbrate_opt, True);
		break;

	case COMPMODE_3:
		XtSetSensitive(widgets.options.qualval_lbl, False);
		XtSetSensitive(widgets.options.qualval1_lbl, False);
		XtSetSensitive(widgets.options.qualval2_lbl, False);
		XtSetSensitive(widgets.options.qualval_scl, False);
		XtSetSensitive(widgets.options.bitrate_opt, True);
		XtSetSensitive(widgets.options.minbrate_opt, True);
		XtSetSensitive(widgets.options.maxbrate_opt, True);
		break;

	case COMPMODE_0:
	default:
		XtSetSensitive(widgets.options.qualval_lbl, False);
		XtSetSensitive(widgets.options.qualval1_lbl, False);
		XtSetSensitive(widgets.options.qualval2_lbl, False);
		XtSetSensitive(widgets.options.qualval_scl, False);
		XtSetSensitive(widgets.options.bitrate_opt, True);
		XtSetSensitive(widgets.options.minbrate_opt, False);
		XtSetSensitive(widgets.options.maxbrate_opt, False);
		break;
	}

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_bitrate
 *	CBR/ABR bitrate menu callback function
 */
/*ARGSUSED*/
void
cd_bitrate(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	wlist_t		*l;
	Widget		menuw = (Widget) NULL;
	int		m,
			n;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.bitrate_def_btn)
		n = 0;
	else if ((n = atoi(XtName(w))) == 0)
		return;

	DBGPRN(DBG_UI)(errfp, "\n* Bitrate: %d kb/s\n", n);
	app_data.bitrate = n;

	if (n == 0) {
		/* Bitrate set to default: set the minimum and maximum to
		 * default too.
		 */
		if (app_data.bitrate_min > 0) {
			XtVaSetValues(widgets.options.minbrate_opt,
				XmNmenuHistory,
				widgets.options.minbrate_def_btn,
				NULL
			);
			app_data.bitrate_min = 0;
			DBGPRN(DBG_UI)(errfp,
					"  Minimum bitrate reset to %d\n",
					app_data.bitrate_min);
		}
		if (app_data.bitrate_max > 0) {
			XtVaSetValues(widgets.options.maxbrate_opt,
				XmNmenuHistory,
				widgets.options.maxbrate_def_btn,
				NULL
			);
			app_data.bitrate_max = 0;
			DBGPRN(DBG_UI)(errfp,
					"  Maximum bitrate reset to %d\n",
					app_data.bitrate_max);
		}
	}
	else {
		/* Adjust minimum and maximum bitrates accordingly */
		if (app_data.bitrate_min > 0 && n <= app_data.bitrate_min) {
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
					DBGPRN(DBG_UI)(errfp,
					    "  Minimum bitrate reset to %d\n",
					    n);
				}
			}
		}

		n = app_data.bitrate;
		if (app_data.bitrate_max > 0 && n >= app_data.bitrate_max) {
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
					DBGPRN(DBG_UI)(errfp,
					    "  Maximum bitrate reset to %d\n",
					    n);
				}
			}
		}
	}

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_min_bitrate
 *	Minimum bitrate menu callback function
 */
/*ARGSUSED*/
void
cd_min_bitrate(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	wlist_t		*l;
	Widget		menuw = (Widget) NULL;
	int		n;
	bool_t		err = FALSE;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.minbrate_def_btn)
		n = 0;
	else if ((n = atoi(XtName(w))) == 0)
		return;

	if (app_data.comp_mode == COMPMODE_3 &&
	    app_data.bitrate > 0 && n > app_data.bitrate) {
		/* Error: minimum bitrate must be less than
		 * or equal to the average bitrate.
		 */ 
		CD_INFO(app_data.str_invbr_minmean);
		err = TRUE;
	}

	if (n > 0 && app_data.bitrate_max > 0 && n > app_data.bitrate_max) {
		/* Error: minimum bitrate must be less than or
		 * equal to the max bitrate.
		 */ 
		CD_INFO(app_data.str_invbr_minmax);
		err = TRUE;
	}

	if (err) {
		/* Restore setting */
		if (app_data.bitrate_min == 0) {
			menuw = widgets.options.minbrate_def_btn;
		}
		else for (l = cd_minbrhead; l != NULL; l = l->next) {
			if (l->w != (Widget) NULL &&
			    atoi(XtName(l->w)) == app_data.bitrate_min) {
				menuw = l->w;
				break;
			}
		}
		if (menuw != NULL) {
			XtVaSetValues(widgets.options.minbrate_opt,
				XmNmenuHistory, menuw,
				NULL
			);
		}
		return;
	}

	DBGPRN(DBG_UI)(errfp, "\n* Minimum bitrate: %d kb/s\n", n);
	app_data.bitrate_min = n;

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_max_bitrate
 *	Maximum bitrate menu callback function
 */
/*ARGSUSED*/
void
cd_max_bitrate(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	wlist_t		*l;
	Widget		menuw = (Widget) NULL;
	int		n;
	bool_t		err = FALSE;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.minbrate_def_btn)
		n = 0;
	else if ((n = atoi(XtName(w))) == 0)
		return;

	if (app_data.comp_mode == COMPMODE_3 &&
	    app_data.bitrate > 0 && n < app_data.bitrate) {
		/* Error: maximum bitrate must be greater than
		 * or equal to the average bitrate.
		 */ 
		CD_INFO(app_data.str_invbr_maxmean);
		err = TRUE;
	}

	if (n > 0 && app_data.bitrate_min > 0 && n < app_data.bitrate_min) {
		/* Error: maximum bitrate must be greater than
		 * or equal to the min bitrate.
		 */ 
		CD_INFO(app_data.str_invbr_maxmin);
		err = TRUE;
	}

	if (err) {
		/* Restore setting */
		if (app_data.bitrate_max == 0) {
			menuw = widgets.options.maxbrate_def_btn;
		}
		else for (l = cd_maxbrhead; l != NULL; l = l->next) {
			if (l->w != (Widget) NULL &&
			    atoi(XtName(l->w)) == app_data.bitrate_max) {
				menuw = l->w;
				break;
			}
		}
		if (menuw != NULL) {
			XtVaSetValues(widgets.options.maxbrate_opt,
				XmNmenuHistory, menuw,
				NULL
			);
		}
		return;
	}

	DBGPRN(DBG_UI)(errfp, "\n* Maximum bitrate: %d kb/s\n", n);
	app_data.bitrate_max = n;

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_qualfactor
 *	VBR Quality factor slider callback function
 */
/*ARGSUSED*/
void
cd_qualfactor(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScaleCallbackStruct
			*p = (XmScaleCallbackStruct *)(void *) call_data;
	XmString	xs,
			xs2;
	char		str[16];

	if (p->value < 1 || p->value > 10)
		return;

	if (p->value == app_data.qual_factor)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* Quality factor: %d\n", p->value);
	app_data.qual_factor = p->value;

	(void) sprintf(str, ": %d", app_data.qual_factor);
	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	xs2 = XmStringConcat(xs_qual, xs);
	XtVaSetValues(widgets.options.qualval_lbl,
		XmNlabelString, xs2,
		NULL
	);
	XmStringFree(xs2);
	XmStringFree(xs);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_chanmode
 *	Channel mode menu callback function
 */
/*ARGSUSED*/
void
cd_chanmode(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	char		*modestr;
	int		newmode;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.chmode_stereo_btn) {
		newmode = CH_STEREO;
		modestr = "stereo";
	}
	else if (w == widgets.options.chmode_jstereo_btn) {
		newmode = CH_JSTEREO;
		modestr = "joint-stereo";
	}
	else if (w == widgets.options.chmode_forcems_btn) {
		newmode = CH_FORCEMS;
		modestr = "force-ms";
	}
	else if (w == widgets.options.chmode_mono_btn) {
		newmode = CH_MONO;
		modestr = "mono";
	}
	else
		return;

	if (app_data.chan_mode == newmode)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* Channel mode: %s\n", modestr);
	app_data.chan_mode = newmode;

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_compalgo
 *	Compression algorithm slider callback function
 */
/*ARGSUSED*/
void
cd_compalgo(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScaleCallbackStruct
			*p = (XmScaleCallbackStruct *)(void *) call_data;
	XmString	xs,
			xs2;
	char		str[16];

	if (p->value < 1 || p->value > 10)
		return;

	if (p->value == app_data.comp_algo)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* Compression algorithm: %d\n", p->value);
	app_data.comp_algo = p->value;

	(void) sprintf(str, ": %d", app_data.comp_algo);
	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	xs2 = XmStringConcat(xs_algo, xs);
	XtVaSetValues(widgets.options.compalgo_lbl,
		XmNlabelString, xs2,
		NULL
	);
	XmStringFree(xs2);
	XmStringFree(xs);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_lowpass_mode
 *	Lowpass filter mode menu callback function
 */
/*ARGSUSED*/
void
cd_lowpass_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	char		*modestr;
	int		newmode;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.lp_off_btn) {
		newmode = FILTER_OFF;
		modestr = "Off";
	}
	else if (w == widgets.options.lp_auto_btn) {
		newmode = FILTER_AUTO;
		modestr = "Auto";
	}
	else if (w == widgets.options.lp_manual_btn) {
		newmode = FILTER_MANUAL;
		modestr = "Manual";
	}
	else
		return;

	if (app_data.lowpass_mode == newmode)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* Lowpass filter: %s\n", modestr);
	app_data.lowpass_mode = newmode;

	XtSetSensitive(
		widgets.options.lp_freq_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);
	XtSetSensitive(
		widgets.options.lp_width_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_highpass_mode
 *	Highpass filter mode menu callback function
 */
/*ARGSUSED*/
void
cd_highpass_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	char		*modestr;
	int		newmode;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.hp_off_btn) {
		newmode = FILTER_OFF;
		modestr = "Off";
	}
	else if (w == widgets.options.hp_auto_btn) {
		newmode = FILTER_AUTO;
		modestr = "Auto";
	}
	else if (w == widgets.options.hp_manual_btn) {
		newmode = FILTER_MANUAL;
		modestr = "Manual";
	}
	else
		return;

	if (app_data.highpass_mode == newmode)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* Highpass filter: %s\n", modestr);
	app_data.highpass_mode = newmode;

	XtSetSensitive(
		widgets.options.hp_freq_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);
	XtSetSensitive(
		widgets.options.hp_width_txt,
		(Boolean) (newmode == FILTER_MANUAL)
	);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_filter_freq
 *	Lowpass and highpass filter frequency text field callback
 */
/*ARGSUSED*/
void
cd_filter_freq(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	char			*cp,
				str[16];
	int			newfreq;

	if (p->reason == XmCR_VALUE_CHANGED) {
		/* Make the Reset and Save buttons sensitive */
		XtSetSensitive(widgets.options.reset_btn, True);
		XtSetSensitive(widgets.options.save_btn, True);
		return;
	}
	else if (p->reason != XmCR_ACTIVATE && p->reason != XmCR_LOSING_FOCUS)
		return;

	if ((cp = get_text_string(w, FALSE)) != NULL) {
		newfreq = atoi(cp);
		MEM_FREE(cp);
	}
	else
		newfreq = 0;

	/* Set cursor to beginning of text */
	XmTextSetInsertionPosition(w, 0);

	if (w == widgets.options.lp_freq_txt) {
		if (app_data.lowpass_freq == newfreq)
			return;	/* No change */

		if (newfreq < MIN_LOWPASS_FREQ ||
		    newfreq > MAX_LOWPASS_FREQ) {
			(void) sprintf(str, "%d", app_data.lowpass_freq);
			set_text_string(w, str, FALSE);

			/* Set cursor to beginning of text */
			XmTextSetInsertionPosition(w, 0);

			cp = (char *) MEM_ALLOC(
				"str_invfreq_lp",
				strlen(app_data.str_invfreq_lp) + 16
			);
			if (cp == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, app_data.str_invfreq_lp,
					MIN_LOWPASS_FREQ, MAX_LOWPASS_FREQ);
			CD_INFO(cp);
			MEM_FREE(cp);
			return;
		}

		app_data.lowpass_freq = newfreq;
		cp = "Lowpass";
	}
	else if (w == widgets.options.hp_freq_txt) {
		if (app_data.highpass_freq == newfreq)
			return;	/* No change */

		if (newfreq < MIN_HIGHPASS_FREQ ||
		    newfreq > MAX_HIGHPASS_FREQ) {
			(void) sprintf(str, "%d", app_data.highpass_freq);
			set_text_string(w, str, FALSE);

			/* Set cursor to beginning of text */
			XmTextSetInsertionPosition(w, 0);

			cp = (char *) MEM_ALLOC(
				"str_invfreq_hp",
				strlen(app_data.str_invfreq_hp) + 16
			);
			if (cp == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, app_data.str_invfreq_hp,
					MIN_HIGHPASS_FREQ, MAX_HIGHPASS_FREQ);
			CD_INFO(cp);
			MEM_FREE(cp);
			return;
		}

		app_data.highpass_freq = newfreq;
		cp = "Highpass";
	}

	DBGPRN(DBG_UI)(errfp, "\n* %s filter frequency: %d\n", cp, newfreq);
}


/*
 * cd_filter_width
 *	Lowpass and highpass filter width text field callback
 */
/*ARGSUSED*/
void
cd_filter_width(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	char			*cp;
	int			newwidth;

	if (p->reason == XmCR_VALUE_CHANGED) {
		/* Make the Reset and Save buttons sensitive */
		XtSetSensitive(widgets.options.reset_btn, True);
		XtSetSensitive(widgets.options.save_btn, True);
		return;
	}
	else if (p->reason != XmCR_ACTIVATE && p->reason != XmCR_LOSING_FOCUS)
		return;

	if ((cp = get_text_string(w, FALSE)) != NULL) {
		newwidth = atoi(cp);
		MEM_FREE(cp);
	}
	else
		newwidth = 0;

	/* Set cursor to beginning of text */
	XmTextSetInsertionPosition(w, 0);

	if (newwidth < 0) {
		set_text_string(w, "0", FALSE);

		/* Set cursor to beginning of text */
		XmTextSetInsertionPosition(w, 0);

		XmProcessTraversal(w, XmTRAVERSE_CURRENT);
		return;
	}

	if (w == widgets.options.lp_width_txt) {
		if (app_data.lowpass_width == newwidth)
			return;	/* No change */

		app_data.lowpass_width = newwidth;
		cp = "Lowpass";
	}
	else if (w == widgets.options.hp_width_txt) {
		if (app_data.highpass_width == newwidth)
			return;	/* No change */

		app_data.highpass_width = newwidth;
		cp = "Highpass";
	}

	DBGPRN(DBG_UI)(errfp, "\n* %s filter width: %d\n", cp, newwidth);
}


/*
 * cd_addflag
 *	Options window Copyright/Original/No res/Checksum flag
 *	toggle buttons callback
 */
/*ARGSUSED*/
void
cd_addflag(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;
	char				*item;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	if (w == widgets.options.copyrt_btn) {
		app_data.copyright = (bool_t) p->set;
		item = "Copyright";
	}
	else if (w == widgets.options.orig_btn) {
		app_data.original = (bool_t) p->set;
		item = "Original";
	}
	else if (w == widgets.options.nores_btn) {
		app_data.nores = (bool_t) p->set;
		item = "No bit reservoir";
	}
	else if (w == widgets.options.chksum_btn) {
		app_data.checksum = (bool_t) p->set;
		item = "Checksum";
	}
	else if (w == widgets.options.iso_btn) {
		app_data.strict_iso = (bool_t) p->set;
		item = "Strict-ISO";
	}
	else {
		/* Invalid button */
		return;
	}

	DBGPRN(DBG_UI)(errfp, "\n* %s flag: %s\n",
			item, p->set ? "Enable" : "Disable");

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_addtag
 *	Options window Add ID3/comment tag toggle buttons callback
 */
/*ARGSUSED*/
void
cd_addtag(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	if (app_data.add_tag == (bool_t) p->set)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* Add ID3/comment tag: %s\n",
			p->set ? "Enable" : "Disable");
	app_data.add_tag = (bool_t) p->set;

	/* Change the ID3 tag version option menu sensitivity */
	XtSetSensitive(widgets.options.id3tag_ver_opt, p->set);

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_id3tag_mode
 *	ID3 tag mode menu callback function
 */
/*ARGSUSED*/
void
cd_id3tag_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	char		*verstr;
	int		newver;

	if (p->reason != XmCR_ACTIVATE)
		return;

	if (w == widgets.options.id3tag_v1_btn) {
		newver = ID3TAG_V1;
		verstr = "version 1";
	}
	else if (w == widgets.options.id3tag_v2_btn) {
		newver = ID3TAG_V2;
		verstr = "version 2";
	}
	else if (w == widgets.options.id3tag_both_btn) {
		newver = ID3TAG_BOTH;
		verstr = "version 1 and 2";
	}
	else
		return;

	if (app_data.id3tag_mode == newver)
		return;	/* No change */

	DBGPRN(DBG_UI)(errfp, "\n* ID3 tag: %s\n", verstr);
	app_data.id3tag_mode = newver;

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_set_lameopts
 *	LAME options text widget callback function
 */
/*ARGSUSED*/
void
cd_set_lameopts(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	char			*cp;

	if (p->reason == XmCR_VALUE_CHANGED) {
		/* Make the Reset and Save buttons sensitive */
		XtSetSensitive(widgets.options.reset_btn, True);
		XtSetSensitive(widgets.options.save_btn, True);
		return;
	}
	if (p->reason != XmCR_ACTIVATE && p->reason != XmCR_LOSING_FOCUS)
		return;

	if ((cp = get_text_string(w, FALSE)) == NULL)
		return;

	/* Set cursor to beginning of text */
	XmTextSetInsertionPosition(w, 0);

	if (app_data.lame_opts != NULL &&
	    strcmp(app_data.lame_opts, cp) == 0) {
		MEM_FREE(cp);
		return;		/* No change */
	}

	if (!util_newstr(&app_data.lame_opts, cp)) {
		MEM_FREE(cp);
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	DBGPRN(DBG_UI)(errfp, "\n* LAME options: %s\n", cp);

	MEM_FREE(cp);
}


/*
 * cd_balance
 *	Balance control slider callback function
 */
/*ARGSUSED*/
void
cd_balance(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScaleCallbackStruct
			*p = (XmScaleCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (p->value == 0) {
		/* Center setting */
		s->level_left = s->level_right = 100;
	}
	else if (p->value < 0) {
		/* Attenuate the right channel */
		s->level_left = 100;
		s->level_right = 100 + (p->value * 2);
	}
	else {
		/* Attenuate the left channel */
		s->level_left = 100 - (p->value * 2);
		s->level_right = 100;
	}

	di_level(
		s,
		(byte_t) s->level,
		(bool_t) (p->reason != XmCR_VALUE_CHANGED)
	);

	DBGPRN(DBG_UI)(errfp, "\n* BALANCE: %d/%d\n",
		       (int) s->level_left, (int) s->level_right);
}


/*
 * cd_balance_center
 *	Balance control center button callback function
 */
/*ARGSUSED*/
void
cd_balance_center(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	/* Force the balance control to the center position */
	set_bal_slider(0);

	s->level_left  = 100;
	s->level_right = 100;

	di_level(s, (byte_t) s->level, FALSE);

	DBGPRN(DBG_UI)(errfp, "\n* BALANCE: center\n");
}


/*
 * cd_cdda_att
 *	CDDA attentuator slider callback function
 */
/*ARGSUSED*/
void
cd_cdda_att(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScaleCallbackStruct
			*p = (XmScaleCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (cdda_fader_id > 0) {
		cd_untimeout(cdda_fader_id);
		cdda_fader_mode = CDDA_FADER_NONE;
	}

	if ((int) s->cdda_att == p->value)
		return;	/* No change */

	s->cdda_att = (byte_t) p->value;
	cdda_att(s);

	DBGPRN(DBG_UI)(errfp, "\n* CDDA ATTENUATOR: %d\n", (int) s->cdda_att);
}


/*
 * cd_cdda_fade
 *	CDDA fader buttons callback function
 */
/*ARGSUSED*/
void
cd_cdda_fade(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	bool_t		fadeout = FALSE;

	fadeout = (bool_t) (w != widgets.options.cdda_fadein_btn);

	if (fadeout && (int) s->cdda_att == 0)
		return;	/* No change */
	else if (!fadeout && (int) s->cdda_att == 100)
		return;	/* No change */

	if ((fadeout && cdda_fader_mode == CDDA_FADER_OUT) ||
	    (!fadeout && cdda_fader_mode == CDDA_FADER_IN)) {
		/* Already fading: speed up fader */
		if (cdda_fader_intvl > 20)
			cdda_fader_intvl >>= 1;
		DBGPRN(DBG_UI)(errfp, "\n* CDDA FADE-%s (speedup)\n",
				fadeout ? "OUT" : "IN");
	}
	else {
		/* Start fading at default speed */
		cdda_fader_intvl = CDDA_FADER_INTVL;
		DBGPRN(DBG_UI)(errfp, "\n* CDDA FADE-%s\n",
				fadeout ? "IN" : "OUT");
	}

	cdda_fader_mode = fadeout ? CDDA_FADER_OUT : CDDA_FADER_IN;

	if (cdda_fader_id > 0)
		cd_untimeout(cdda_fader_id);

	cdda_fader_id = cd_timeout(
		cdda_fader_intvl,
		fadeout ? cdda_fadeout : cdda_fadein,
		(byte_t *) s
	);
}


/*
 * cd_set_timeouts
 *	Heartbeat/server/cache timeout text widgets callback
 */
/*ARGSUSED*/
void
cd_set_timeouts(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	int			*valp,
				val,
				min_val;
	char			*cp,
				*name,
				str[16];

	if (p->reason == XmCR_VALUE_CHANGED) {
		/* Make the Reset and Save buttons sensitive */
		XtSetSensitive(widgets.options.reset_btn, True);
		XtSetSensitive(widgets.options.save_btn, True);
		return;
	}
	else if (p->reason != XmCR_ACTIVATE && p->reason != XmCR_LOSING_FOCUS)
		return;

	if ((cp = get_text_string(w, FALSE)) == NULL)
		return;

	/* Set cursor to beginning of text */
	XmTextSetInsertionPosition(w, 0);

	if (w == widgets.options.hb_tout_txt) {
		valp = &app_data.hb_timeout;
		min_val = MIN_HB_TIMEOUT;
		name = "Heartbeat";
	}
	else if (w == widgets.options.serv_tout_txt) {
		valp = &app_data.srv_timeout;
		min_val = 0;
		name = "Service";
	}
	else if (w == widgets.options.cache_tout_txt) {
		valp = &app_data.cache_timeout;
		min_val = 0;
		name = "Cache";
	}
	else {
		MEM_FREE(cp);
		return;
	}

	if ((val = atoi(cp)) < min_val) {
		cd_beep();

		(void) sprintf(str, "%d", *valp);
		set_text_string(w, str, FALSE);

		/* Set cursor to beginning of text */
		XmTextSetInsertionPosition(w, 0);

		MEM_FREE(cp);
		return;
	}
	else if (val == *valp) {
		MEM_FREE(cp);
		return;		/* No change */
	}

	DBGPRN(DBG_UI)(errfp, "\n* %s timeout: %d\n", name, val);
	*valp = val;

	MEM_FREE(cp);
}


/*
 * cd_set_servers
 *	Proxy server/port text widgets callback
 */
/*ARGSUSED*/
void
cd_set_servers(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	char			*cp,
				*cp2 = NULL,
				str[STR_BUF_SZ * 2];
	int			port;

	if (p->reason == XmCR_VALUE_CHANGED) {
		/* Make the Reset and Save buttons sensitive */
		XtSetSensitive(widgets.options.reset_btn, True);
		XtSetSensitive(widgets.options.save_btn, True);
		return;
	}
	if (p->reason != XmCR_ACTIVATE && p->reason != XmCR_LOSING_FOCUS)
		return;

	if ((cp = get_text_string(w, FALSE)) == NULL)
		return;

	if (app_data.proxy_server != NULL)
		cp2 = strrchr(app_data.proxy_server, ':');

	if (cp2 != NULL)
		*cp2++ = '\0';
	else
		cp2 = "80";

	/* Set cursor to beginning of text */
	XmTextSetInsertionPosition(w, 0);

	if (w == widgets.options.proxy_srvr_txt) {
		if ((port = atoi(cp2)) < 0)
			port = 80;

		(void) sprintf(str, "%.64s:%d", cp, port);
	}
	else if (w == widgets.options.proxy_port_txt) {
		if ((port = atoi(cp)) < 0) {
			cd_beep();

			set_text_string(w, cp2, FALSE);

			/* Set cursor to beginning of text */
			XmTextSetInsertionPosition(w, 0);
			return;
		}

		(void) sprintf(str, "%.64s:%d",
			(app_data.proxy_server == NULL) ?
				"" : app_data.proxy_server,
			port
		);
	}

	MEM_FREE(cp);

	if (app_data.proxy_server != NULL &&
	    strcmp(app_data.proxy_server, str) == 0)
		return;		/* No change */

	if (!util_newstr(&app_data.proxy_server, str)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	DBGPRN(DBG_UI)(errfp, "\n* Proxy server: %s\n", str);
}


/*
 * cd_set_proxy
 *	CDDB proxy server toggle buttons callback
 */
/*ARGSUSED*/
void
cd_set_proxy(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	if (w == widgets.options.use_proxy_btn) {
		if (app_data.use_proxy == (bool_t) p->set)
			return;	/* No change */

		DBGPRN(DBG_UI)(errfp, "\n* Use proxy: %s\n",
				p->set ? "Enable" : "Disable");
		app_data.use_proxy = (bool_t) p->set;

		/* Set the sensitivity of related widgets */
		XtSetSensitive(widgets.options.proxy_srvr_lbl, p->set);
		XtSetSensitive(widgets.options.proxy_srvr_txt, p->set);
		XtSetSensitive(widgets.options.proxy_port_lbl, p->set);
		XtSetSensitive(widgets.options.proxy_port_txt, p->set);
		XtSetSensitive(widgets.options.proxy_auth_btn, p->set);
	}
	else if (w == widgets.options.proxy_auth_btn) {
		if (app_data.proxy_auth == (bool_t) p->set)
			return;	/* No change */

		DBGPRN(DBG_UI)(errfp, "\n* Proxy authorization: %s\n",
				p->set ? "Enable" : "Disable");
		app_data.proxy_auth = (bool_t) p->set;
	}

	/* Make the Reset and Save buttons sensitive */
	XtSetSensitive(widgets.options.reset_btn, True);
	XtSetSensitive(widgets.options.save_btn, True);
}


/*
 * cd_perfmon
 *	CDDA Performance Monitor button callback function
 */
/*ARGSUSED*/
void
cd_perfmon_popdown(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (XtIsManaged(widgets.perfmon.form)) {
		/* Pop down performance monitor window */
		XtUnmapWidget(XtParent(widgets.perfmon.form));
		XtUnmanageChild(widgets.perfmon.form);
	}
}


/*
 * cd_perfmon
 *	CDDA Performance Monitor button callback function
 */
/*ARGSUSED*/
void
cd_perfmon(Widget w, XtPointer client_data, XtPointer call_data)
{
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.perfmon.form)) {
		cd_perfmon_popdown(w, client_data, call_data);
		return;
	}

	/* Pop up performance monitor window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason for this
	 * is we want to avoid a screen glitch when we move the window
	 * in cd_dialog_setpos(), so we map the window afterwards.
	 */
	XtManageChild(widgets.perfmon.form);
	if (first) {
		first = FALSE;
		/* Set window position */
		cd_dialog_setpos(XtParent(widgets.perfmon.form));
	}
	XtMapWidget(XtParent(widgets.perfmon.form));

	XmProcessTraversal(
		widgets.perfmon.cancel_btn,
		XmTRAVERSE_CURRENT
	);
}


/*
 * cd_txtline_vfy
 *	Single-line text widget user-input verification callback.
 */
/*ARGSUSED*/
void
cd_txtline_vfy(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTextVerifyCallbackStruct
		*p = (XmTextVerifyCallbackStruct *)(void *) call_data;
	int	i;

	if (p->reason != XmCR_MODIFYING_TEXT_VALUE)
		return;

	p->doit = True;

	if (p->startPos != p->endPos)
		/* Deleting text, no verification needed */
		return;

	switch (p->text->format) {
	case XmFMT_8_BIT:
		for (i = 0; i < p->text->length; i++) {
			/* This is a single-line text widget, so a
			 * newline is not allowed.
			 */
			if (p->text->ptr[i] == '\n' ||
			    p->text->ptr[i] == '\r') {
				p->doit = False;
				break;
			}
		}
		break;

	case XmFMT_16_BIT:
	default:
		/* Nothing to do here */
		break;
	}
}


/*
 * cd_txtnline_vfy
 *	Single-line text widget numerical user-input verification callback.
 */
/*ARGSUSED*/
void
cd_txtnline_vfy(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTextVerifyCallbackStruct
		*p = (XmTextVerifyCallbackStruct *)(void *) call_data;
	int	i;

	if (p->reason != XmCR_MODIFYING_TEXT_VALUE)
		return;

	p->doit = True;

	if (p->startPos != p->endPos)
		/* Deleting text, no verification needed */
		return;

	switch (p->text->format) {
	case XmFMT_8_BIT:
		for (i = 0; i < p->text->length; i++) {
			/* Only digits are allowed */
			if (!isdigit((int) p->text->ptr[i]) &&
			    p->text->ptr[i] != '+' &&
			    p->text->ptr[i] != '-') {
				p->doit = False;
				break;
			}
		}
		break;

	case XmFMT_16_BIT:
	default:
		/* Nothing to do here */
		break;
	}
}


/*
 * cd_about
 *	Program information popup callback function
 */
/*ARGSUSED*/
void
cd_about(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		allocsz;
	char		*txt,
			*ctrlver;
	XmString	xs_desc,
			xs_info,
			xs;
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (XtIsManaged(widgets.dialog.about)) {
		/* Pop down the about dialog box */
		XtUnmanageChild(widgets.dialog.about);

		if (w == widgets.help.about_btn)
			return;
	}

	allocsz = STR_BUF_SZ * 32;

	if ((txt = (char *) MEM_ALLOC("about_allocsz", allocsz)) == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	(void) sprintf(txt, "%s version %s.%s.%s\n%s\n\n",
		       PROGNAME, VERSION_MAJ, VERSION_MIN, VERSION_TEENY,
		       "Motif\256 CD Audio Player / Ripper");

	xs_desc = create_xmstring(txt, NULL, CHSET2, FALSE);

	(void) sprintf(txt,
			"%s\nURL: %s\nE-mail: %s\n\n%s\n\n%s\n"
			"Drive: %s %s %s%s%s\nDevice: %s\n",
		       COPYRIGHT,
		       XMCD_URL,
		       EMAIL,
		       GNU_BANNER,
		       di_methodstr(),
		       (s->vendor[0] == '\0') ? "??" : s->vendor,
		       s->prod,
		       (s->revnum[0] == '\0') ? "" : "(",
		       s->revnum,
		       (s->revnum[0] == '\0') ? "" : ")",
		       s->curdev
	);

	ctrlver = cdinfo_cddbctrl_ver();
	(void) sprintf(txt, "%s\nCDDB%s service%s%s\n%s", txt,
		       (cdinfo_cddb_ver() == 2) ? "\262" : " \"classic\"",
		       (ctrlver[0] == '\0') ? "" : ": ",
		       (ctrlver[0] == '\0') ? "\n" : ctrlver,
		       CDDB_BANNER);

	xs_info = create_xmstring(txt, NULL, CHSET1, FALSE);

	/* Set the dialog box title */
	xs = create_xmstring(
		app_data.str_about, NULL, XmSTRING_DEFAULT_CHARSET, FALSE
	);
	XtVaSetValues(widgets.dialog.about, XmNdialogTitle, xs, NULL);
	XmStringFree(xs);

	/* Set the dialog box message */
	xs = XmStringConcat(xs_desc, xs_info);
	XtVaSetValues(widgets.dialog.about, XmNmessageString, xs, NULL);
	XmStringFree(xs_desc);
	XmStringFree(xs_info);
	XmStringFree(xs);

	MEM_FREE(txt);

	/* Set up dialog box position */
	cd_dialog_setpos(XtParent(widgets.dialog.about));

	/* Pop up the about dialog box */
	XtManageChild(widgets.dialog.about);
}


/*
 * cd_help_popup
 *	Program help window popup callback function
 */
/*ARGSUSED*/
void
cd_help_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop up help window */
	help_popup(w);
}


/*
 * cd_help_cancel
 *	Program help window popdown callback function
 */
/*ARGSUSED*/
void
cd_help_cancel(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop down help window */
	if (help_isactive())
		help_popdown();
}


/*
 * cd_info_ok
 *	Information message dialog box OK button callback function.
 */
/*ARGSUSED*/
void
cd_info_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Remove pending popdown timeout, if any */
	if (infodiag_id >= 0) {
		cd_untimeout(infodiag_id);
		infodiag_id = -1;
	}

	/* Pop down the info window */
	cd_info_popdown(NULL);
}


/*
 * cd_info2_ok
 *	Info2 message dialog box OK button callback function.
 */
/*ARGSUSED*/
void
cd_info2_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop down the info2 window */
	cd_info2_popdown(NULL);
}


/*
 * cd_warning_ok
 *	Warning message dialog box OK button callback function
 */
/*ARGSUSED*/
void
cd_warning_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop down the warning dialog */
	if (XtIsManaged(widgets.dialog.warning))
		XtUnmanageChild(widgets.dialog.warning);
}


/*
 * cd_fatal_ok
 *	Fatal error message dialog box OK button callback function.
 *	This causes the application to terminate.
 */
/*ARGSUSED*/
void
cd_fatal_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop down the error dialog */
	if (XtIsManaged(widgets.dialog.fatal))
		XtUnmanageChild(widgets.dialog.fatal);

	/* Quit */
	cd_quit((curstat_t *)(void *) client_data);
}


/*
 * cd_confirm_resp
 *	Set the confirm dialog user response state
 */
/*ARGSUSED*/
void
cd_confirm_resp(Widget w, XtPointer client_data, XtPointer call_data)
{
	confirm_resp = (bool_t) (client_data != NULL);
	confirm_pend = FALSE;
}


/*
 * cd_rmcallback
 *	Remove callback function specified in cdinfo_t.
 */
/*ARGSUSED*/
void
cd_rmcallback(Widget w, XtPointer client_data, XtPointer call_data)
{
	cbinfo_t	*cb = (cbinfo_t *)(void *) client_data;

	if (cb == NULL)
		return;

	if (cb->widget0 != (Widget) NULL) {
		XtRemoveCallback(
			cb->widget0,
			cb->type,
			(XtCallbackProc) cb->func,
			(XtPointer) cb->data
		);

		XtRemoveCallback(
			cb->widget0,
			cb->type,
			(XtCallbackProc) cd_rmcallback,
			client_data
		);

		cb->widget0 = (Widget) NULL;
	}

	if (cb->widget1 != (Widget) NULL) {
		XtRemoveCallback(
			cb->widget1,
			cb->type,
			(XtCallbackProc) cb->func,
			(XtPointer) cb->data
		);

		XtRemoveCallback(
			cb->widget1,
			cb->type,
			(XtCallbackProc) cd_rmcallback,
			client_data
		);

		cb->widget1 = (Widget) NULL;
	}

	/* Remove WM_DELETE_WINDOW handler */
	if (cb->widget2 != (Widget) NULL) {
		rm_delw_callback(
			cb->widget2,
			(XtCallbackProc) cb->func,
			(XtPointer) cb->data
		);
		cb->widget2 = (Widget) NULL;
	}
}


/*
 * cd_shell_focus_chg
 *	Focus change callback.  Used to implement keyboard grabs for
 *	hotkey handling.
 */
/*ARGSUSED*/
void
cd_shell_focus_chg(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	Widget			shell = (Widget) client_data;
	static Widget		prev_shell = (Widget) NULL;

	if (p->reason != XmCR_FOCUS || shell == (Widget) NULL)
		return;

	if (prev_shell != NULL) {
		if (shell == prev_shell)
			return;
		else
			hotkey_ungrabkeys(prev_shell);
	}

	if (shell != widgets.toplevel) {
		hotkey_grabkeys(shell);
		prev_shell = shell;
	}
}


/*
 * cd_exit
 *	Shut down the application gracefully.
 */
/*ARGSUSED*/
void
cd_exit(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	/* Shut down CDDB - if there are more processing needed in the
	 * CDDB, they will call us back.
	 */
	s->flags |= STAT_EXIT;
	if (!dbprog_chgsubmit(s))
		return;

	cd_quit(s);
}


/*
 * cd_tooltip_cancel
 *	Cancel the tooltip window
 */
/*ARGSUSED*/
void
cd_tooltip_cancel(Widget w, XtPointer client_data, XtPointer call_data)
{
	cd_tooltip_popdown(widgets.tooltip.shell);
}


/*
 * cd_not_implemented
 *	Pop up a "not yet implemented" message.  The client data
 *	may point to a feature name string which will be displayed
 *	along with the message.
 */
/*ARGSUSED*/
void
cd_not_implemented(Widget w, XtPointer client_data, XtPointer call_data)
{
	char	*feature = (char *) client_data;
	char	msg[STR_BUF_SZ * 2];

	(void) sprintf(msg,
		"%s%sNot yet implemented.",
		feature != NULL ? feature : "",
		feature != NULL ? ": " : ""
	);

	CD_INFO_AUTO(msg);
}


/**************** ^^ Callback routines ^^ ****************/

/***************** vv Event Handlers vv ******************/


/* Mapping table for main window controls and their label color change
 * function pointer, and associated label pixmaps.
 */
struct wpix_tab {
	Widget	*wptr;
	void	(*set_func)(Widget w, Pixmap px, Pixel color);
	Pixmap	*fpx;
	Pixmap	*hpx;
} wpix_tab[] = {
    { &widgets.main.mode_btn, set_btn_color,
      &pixmaps.main.mode_pixmap, &pixmaps.main.mode_hlpixmap },
    { &widgets.main.eject_btn, set_btn_color,
      &pixmaps.main.eject_pixmap, &pixmaps.main.eject_hlpixmap },
    { &widgets.main.quit_btn, set_btn_color,
      &pixmaps.main.quit_pixmap, &pixmaps.main.quit_hlpixmap },
    { &widgets.main.dbprog_btn, set_btn_color,
      &pixmaps.main.dbprog_pixmap, &pixmaps.main.dbprog_hlpixmap },
    { &widgets.main.wwwwarp_btn, set_btn_color,
      &pixmaps.main.world_pixmap, &pixmaps.main.world_hlpixmap },
    { &widgets.main.options_btn, set_btn_color,
      &pixmaps.main.options_pixmap, &pixmaps.main.options_hlpixmap },
    { &widgets.main.time_btn, set_btn_color,
      &pixmaps.main.time_pixmap, &pixmaps.main.time_hlpixmap },
    { &widgets.main.ab_btn, set_btn_color,
      &pixmaps.main.ab_pixmap, &pixmaps.main.ab_hlpixmap },
    { &widgets.main.sample_btn, set_btn_color,
      &pixmaps.main.sample_pixmap, &pixmaps.main.sample_hlpixmap },
    { &widgets.main.keypad_btn, set_btn_color,
      &pixmaps.main.keypad_pixmap, &pixmaps.main.keypad_hlpixmap },
    { &widgets.main.level_scale, set_scale_color,
      NULL, NULL },
    { &widgets.main.playpause_btn, set_btn_color,
      &pixmaps.main.playpause_pixmap, &pixmaps.main.playpause_hlpixmap },
    { &widgets.main.stop_btn, set_btn_color,
      &pixmaps.main.stop_pixmap, &pixmaps.main.stop_hlpixmap },
    { &widgets.main.prevdisc_btn, set_btn_color,
      &pixmaps.main.prevdisc_pixmap, &pixmaps.main.prevdisc_hlpixmap },
    { &widgets.main.nextdisc_btn, set_btn_color,
      &pixmaps.main.nextdisc_pixmap, &pixmaps.main.nextdisc_hlpixmap },
    { &widgets.main.prevtrk_btn, set_btn_color,
      &pixmaps.main.prevtrk_pixmap, &pixmaps.main.prevtrk_hlpixmap },
    { &widgets.main.nexttrk_btn, set_btn_color,
      &pixmaps.main.nexttrk_pixmap, &pixmaps.main.nexttrk_hlpixmap },
    { &widgets.main.previdx_btn, set_btn_color,
      &pixmaps.main.previdx_pixmap, &pixmaps.main.previdx_hlpixmap },
    { &widgets.main.nextidx_btn, set_btn_color,
      &pixmaps.main.nextidx_pixmap, &pixmaps.main.nextidx_hlpixmap },
    { &widgets.main.rew_btn, set_btn_color,
      &pixmaps.main.rew_pixmap, &pixmaps.main.rew_hlpixmap },
    { &widgets.main.ff_btn, set_btn_color,
      &pixmaps.main.ff_pixmap, &pixmaps.main.ff_hlpixmap },
    { NULL, NULL, 0, 0 }
};


/*
 * cd_focus_chg
 *	Widget keyboard focus change event handler.  Used to change
 *	the main window controls' label color.
 */
/*ARGSUSED*/
void
cd_focus_chg(Widget w, XtPointer client_data, XEvent *ev)
{
	int		i;
	unsigned char	focuspolicy;		
	static bool_t	first = TRUE;
	static int	count = 0;
	static Pixel	fg,
			hl;

	if (!app_data.main_showfocus)
		return;

	if (first) {
		first = FALSE;

		XtVaGetValues(
			widgets.toplevel,
			XmNkeyboardFocusPolicy,
			&focuspolicy,
			NULL
		);
		if (focuspolicy != XmEXPLICIT) {
			app_data.main_showfocus = FALSE;
			return;
		}

		XtVaGetValues(w, XmNforeground, &fg, NULL);
		XtVaGetValues(w, XmNhighlightColor, &hl, NULL);
	}

	if (ev->xfocus.mode != NotifyNormal ||
	    ev->xfocus.detail != NotifyAncestor)
		return;

	if (ev->type == FocusOut) {
		if (count <= 0)
			return;

		/* Restore original foreground pixmap */
		for (i = 0; wpix_tab[i].set_func != NULL; i++) {
			if (w == *(wpix_tab[i].wptr)) {
				wpix_tab[i].set_func(w,
					(wpix_tab[i].fpx == NULL) ?
					    (Pixmap) 0 : *(wpix_tab[i].fpx),
					fg
				);
				break;
			}
		}
		count--;
	}
	else if (ev->type == FocusIn) {
		if (count >= 1)
			return;

		/* Set new highlighted foreground pixmap */
		for (i = 0; wpix_tab[i].set_func != NULL; i++) {
			if (w == *(wpix_tab[i].wptr)) {
				wpix_tab[i].set_func(w,
					(wpix_tab[i].fpx == NULL) ?
					    (Pixmap) 0 : *(wpix_tab[i].hpx),
					hl
				);
				break;
			}
		}
		count++;
	}
}

/*
 * cd_xing_chg
 *	Widget enter/leave crossing event handler.  Used to manage
 *	pop-up tool-tips.
 */
/*ARGSUSED*/
void
cd_xing_chg(Widget w, XtPointer client_data, XEvent *ev)
{
	if (!app_data.tooltip_enable ||
	    ev->xcrossing.mode != NotifyNormal ||
	    ev->xcrossing.detail == NotifyInferior)
		return;

	if (ev->type == EnterNotify) {
		if (skip_next_tooltip) {
			skip_next_tooltip = FALSE;
			return;
		}

		if (tooltip1_id < 0) {
			tooltip1_id = cd_timeout(
				app_data.tooltip_delay,
				cd_tooltip_popup,
				(byte_t *) w
			);
		}
	}
	else if (ev->type == LeaveNotify) {
		cd_tooltip_popdown(widgets.tooltip.shell);
	}
}


/*
 * cd_dbmode_ind
 *      Main window dbmode indicator button release event callback function
 */
/*ARGSUSED*/
void
cd_dbmode_ind(Widget w, XtPointer client_data, XEvent *ev, Boolean *cont)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	*cont = True;

	if (ev->xany.type != ButtonRelease || ev->xbutton.button != Button1)
		return;

	if (s->qmode == QMODE_WAIT) {
		(void) dbprog_stopload_active(1, TRUE);

		(void) cd_confirm_popup(
			app_data.str_confirm,
			app_data.str_stopload,
			(XtCallbackProc) dbprog_stop_load_yes, client_data,
			(XtCallbackProc) dbprog_stop_load_no, client_data
		);
	}
	else if (XtIsSensitive(widgets.dbprog.reload_btn)) {
		(void) cd_confirm_popup(
			app_data.str_confirm,
			app_data.str_reload,
			(XtCallbackProc) dbprog_load, client_data,
			(XtCallbackProc) NULL, NULL
		);
	}
}


/***************** ^^ Event Handlers ^^ ******************/

