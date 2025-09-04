/*
 *   Xmcd - Motif(R) CD Audio Player/Ripper
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
static char *_geom_c_ident_ = "@(#)geom.c	7.83 04/02/13";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/geom.h"


#define WLIST_SIZE	30		/* Widget list size */


extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		main_mode;	/* Main window mode */


/***********************
 *  internal routines  *
 ***********************/


/*
 * geom_normal_force
 *	Set the geometry of the widgets in the main window in normal mode.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_normal_force(widgets_t *m)
{
	XtVaSetValues(m->main.mode_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_MODE,
		XmNrightPosition, RIGHT_MODE,
		XmNtopPosition, TOP_MODE,
		XmNbottomPosition, BOTTOM_MODE,
		NULL
	);
	XtVaSetValues(m->main.chkbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_CHECKBOX,
		XmNrightPosition, RIGHT_CHECKBOX,
		XmNtopPosition, TOP_CHECKBOX,
		XmNbottomPosition, BOTTOM_CHECKBOX,
		XmNshadowType, XmSHADOW_OUT,
		NULL
	);
	XtVaSetValues(m->main.lock_btn,
		XmNheight, TOGGLE_HEIGHT1,
		XmNrecomputeSize, False,
		NULL
	);
	if (!app_data.main_showfocus) {
		XtVaSetValues(m->main.lock_btn,
			XmNhighlightThickness, 0,
			NULL
		);
	}
	XtVaSetValues(m->main.repeat_btn,
		XmNheight, TOGGLE_HEIGHT1,
		XmNrecomputeSize, False,
		NULL
	);
	if (!app_data.main_showfocus) {
		XtVaSetValues(m->main.repeat_btn,
			XmNhighlightThickness, 0,
			NULL
		);
	}
	XtVaSetValues(m->main.shuffle_btn,
		XmNheight, TOGGLE_HEIGHT1,
		XmNrecomputeSize, False,
		NULL
	);
	if (!app_data.main_showfocus) {
		XtVaSetValues(m->main.shuffle_btn,
			XmNhighlightThickness, 0,
			NULL
		);
	}
	XtVaSetValues(m->main.eject_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_EJECT,
		XmNrightPosition, RIGHT_EJECT,
		XmNtopPosition, TOP_EJECT,
		XmNbottomPosition, BOTTOM_EJECT,
		NULL
	);
	XtVaSetValues(m->main.quit_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_QUIT,
		XmNrightPosition, RIGHT_QUIT,
		XmNtopPosition, TOP_QUIT,
		XmNbottomPosition, BOTTOM_QUIT,
		NULL
	);
	XtVaSetValues(m->main.disc_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_DISCIND,
		XmNrightPosition, RIGHT_DISCIND,
		XmNtopPosition, TOP_DISCIND,
		XmNbottomPosition, BOTTOM_DISCIND,
		NULL
	);
	XtVaSetValues(m->main.track_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_TRACKIND,
		XmNrightPosition, RIGHT_TRACKIND,
		XmNtopPosition, TOP_TRACKIND,
		XmNbottomPosition, BOTTOM_TRACKIND,
		NULL
	);
	XtVaSetValues(m->main.index_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_INDEXIND,
		XmNrightPosition, RIGHT_INDEXIND,
		XmNtopPosition, TOP_INDEXIND,
		XmNbottomPosition, BOTTOM_INDEXIND,
		NULL
	);
	XtVaSetValues(m->main.time_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_TIMEIND,
		XmNrightPosition, RIGHT_TIMEIND,
		XmNtopPosition, TOP_TIMEIND,
		XmNbottomPosition, BOTTOM_TIMEIND,
		NULL
	);
	XtVaSetValues(m->main.rptcnt_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_RPTCNTIND,
		XmNrightPosition, RIGHT_RPTCNTIND,
		XmNtopPosition, TOP_RPTCNTIND,
		XmNbottomPosition, BOTTOM_RPTCNTIND,
		NULL
	);
	XtVaSetValues(m->main.dbmode_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_DBMODEIND,
		XmNrightPosition, RIGHT_DBMODEIND,
		XmNtopPosition, TOP_DBMODEIND,
		XmNbottomPosition, BOTTOM_DBMODEIND,
		NULL
	);
	XtVaSetValues(m->main.progmode_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_PROGMODEIND,
		XmNrightPosition, RIGHT_PROGMODEIND,
		XmNtopPosition, TOP_PROGMODEIND,
		XmNbottomPosition, BOTTOM_PROGMODEIND,
		NULL
	);
	XtVaSetValues(m->main.timemode_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_TIMEMODEIND,
		XmNrightPosition, RIGHT_TIMEMODEIND,
		XmNtopPosition, TOP_TIMEMODEIND,
		XmNbottomPosition, BOTTOM_TIMEMODEIND,
		NULL
	);
	XtVaSetValues(m->main.playmode_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_PLAYMODEIND,
		XmNrightPosition, RIGHT_PLAYMODEIND,
		XmNtopPosition, TOP_PLAYMODEIND,
		XmNbottomPosition, BOTTOM_PLAYMODEIND,
		NULL
	);
	XtVaSetValues(m->main.dtitle_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_DTITLEIND,
		XmNrightPosition, RIGHT_DTITLEIND,
		XmNtopPosition, TOP_DTITLEIND,
		XmNbottomPosition, BOTTOM_DTITLEIND,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL
	);
	XtVaSetValues(m->main.ttitle_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_TTITLEIND,
		XmNrightPosition, RIGHT_TTITLEIND,
		XmNtopPosition, TOP_TTITLEIND,
		XmNbottomPosition, BOTTOM_TTITLEIND,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL
	);
	XtVaSetValues(m->main.dbprog_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_DBPROG,
		XmNrightPosition, RIGHT_DBPROG,
		XmNtopPosition, TOP_DBPROG,
		XmNbottomPosition, BOTTOM_DBPROG,
		NULL
	);
	XtVaSetValues(m->main.options_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_OPTIONS,
		XmNrightPosition, RIGHT_OPTIONS,
		XmNtopPosition, TOP_OPTIONS,
		XmNbottomPosition, BOTTOM_OPTIONS,
		NULL
	);
	XtVaSetValues(m->main.ab_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_AB,
		XmNrightPosition, RIGHT_AB,
		XmNtopPosition, TOP_AB,
		XmNbottomPosition, BOTTOM_AB,
		NULL
	);
	XtVaSetValues(m->main.sample_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_SAMPLE,
		XmNrightPosition, RIGHT_SAMPLE,
		XmNtopPosition, TOP_SAMPLE,
		XmNbottomPosition, BOTTOM_SAMPLE,
		NULL
	);
	XtVaSetValues(m->main.time_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_TIME,
		XmNrightPosition, RIGHT_TIME,
		XmNtopPosition, TOP_TIME,
		XmNbottomPosition, BOTTOM_TIME,
		NULL
	);
	XtVaSetValues(m->main.keypad_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEYPAD,
		XmNrightPosition, RIGHT_KEYPAD,
		XmNtopPosition, TOP_KEYPAD,
		XmNbottomPosition, BOTTOM_KEYPAD,
		NULL
	);
	XtVaSetValues(m->main.wwwwarp_bar,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_WWWWARP,
		XmNrightPosition, RIGHT_WWWWARP,
		XmNtopPosition, TOP_WWWWARP,
		XmNbottomPosition, BOTTOM_WWWWARP,
		NULL
	);
	XtVaSetValues(m->main.level_scale,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_LEVEL,
		XmNrightPosition, RIGHT_LEVEL,
		XmNtopPosition, TOP_LEVEL,
		NULL
	);
	XtVaSetValues(m->main.playpause_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_PLAYPAUSE,
		XmNrightPosition, RIGHT_PLAYPAUSE,
		XmNtopPosition, TOP_PLAYPAUSE,
		XmNbottomPosition, BOTTOM_PLAYPAUSE,
		NULL
	);
	XtVaSetValues(m->main.stop_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_STOP,
		XmNrightPosition, RIGHT_STOP,
		XmNtopPosition, TOP_STOP,
		XmNbottomPosition, BOTTOM_STOP,
		NULL
	);
	XtVaSetValues(m->main.prevdisc_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_PREVDISC,
		XmNrightPosition, RIGHT_PREVDISC,
		XmNtopPosition, TOP_PREVDISC,
		XmNbottomPosition, BOTTOM_PREVDISC,
		NULL
	);
	XtVaSetValues(m->main.nextdisc_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_NEXTDISC,
		XmNrightPosition, RIGHT_NEXTDISC,
		XmNtopPosition, TOP_NEXTDISC,
		XmNbottomPosition, BOTTOM_NEXTDISC,
		NULL
	);
	XtVaSetValues(m->main.prevtrk_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_PREVTRK,
		XmNrightPosition, RIGHT_PREVTRK,
		XmNtopPosition, TOP_PREVTRK,
		XmNbottomPosition, BOTTOM_PREVTRK,
		NULL
	);
	XtVaSetValues(m->main.nexttrk_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_NEXTTRK,
		XmNrightPosition, RIGHT_NEXTTRK,
		XmNtopPosition, TOP_NEXTTRK,
		XmNbottomPosition, BOTTOM_NEXTTRK,
		NULL
	);
	XtVaSetValues(m->main.previdx_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_PREVIDX,
		XmNrightPosition, RIGHT_PREVIDX,
		XmNtopPosition, TOP_PREVIDX,
		XmNbottomPosition, BOTTOM_PREVIDX,
		NULL
	);
	XtVaSetValues(m->main.nextidx_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_NEXTIDX,
		XmNrightPosition, RIGHT_NEXTIDX,
		XmNtopPosition, TOP_NEXTIDX,
		XmNbottomPosition, BOTTOM_NEXTIDX,
		NULL
	);
	XtVaSetValues(m->main.rew_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_REW,
		XmNrightPosition, RIGHT_REW,
		XmNtopPosition, TOP_REW,
		XmNbottomPosition, BOTTOM_REW,
		NULL
	);
	XtVaSetValues(m->main.ff_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_FF,
		XmNrightPosition, RIGHT_FF,
		XmNtopPosition, TOP_FF,
		XmNbottomPosition, BOTTOM_FF,
		NULL
	);
}


/*
 * geom_basic_force
 *	Set the geometry of the widgets in the main window in basic mode.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_basic_force(widgets_t *m)
{
	XtVaSetValues(m->main.mode_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BMODE,
		XmNrightPosition, RIGHT_BMODE,
		XmNtopPosition, TOP_BMODE,
		XmNbottomPosition, BOTTOM_BMODE,
		NULL
	);
	XtVaSetValues(m->main.eject_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BEJECT,
		XmNrightPosition, RIGHT_BEJECT,
		XmNtopPosition, TOP_BEJECT,
		XmNbottomPosition, BOTTOM_BEJECT,
		NULL
	);
	XtVaSetValues(m->main.quit_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BQUIT,
		XmNrightPosition, RIGHT_BQUIT,
		XmNtopPosition, TOP_BQUIT,
		XmNbottomPosition, BOTTOM_BQUIT,
		NULL
	);
	XtVaSetValues(m->main.track_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BTRACKIND,
		XmNrightPosition, RIGHT_BTRACKIND,
		XmNtopPosition, TOP_BTRACKIND,
		XmNbottomPosition, BOTTOM_BTRACKIND,
		NULL
	);
	XtVaSetValues(m->main.time_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BTIMEIND,
		XmNrightPosition, RIGHT_BTIMEIND,
		XmNtopPosition, TOP_BTIMEIND,
		XmNbottomPosition, BOTTOM_BTIMEIND,
		NULL
	);
	XtVaSetValues(m->main.level_scale,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_BLEVEL,
		XmNrightPosition, RIGHT_BLEVEL,
		XmNtopPosition, TOP_BLEVEL,
		NULL
	);
	XtVaSetValues(m->main.playpause_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BPLAYPAUSE,
		XmNrightPosition, RIGHT_BPLAYPAUSE,
		XmNtopPosition, TOP_BPLAYPAUSE,
		XmNbottomPosition, BOTTOM_BPLAYPAUSE,
		NULL
	);
	XtVaSetValues(m->main.stop_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BSTOP,
		XmNrightPosition, RIGHT_BSTOP,
		XmNtopPosition, TOP_BSTOP,
		XmNbottomPosition, BOTTOM_BSTOP,
		NULL
	);
	XtVaSetValues(m->main.prevtrk_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BPREVTRK,
		XmNrightPosition, RIGHT_BPREVTRK,
		XmNtopPosition, TOP_BPREVTRK,
		XmNbottomPosition, BOTTOM_BPREVTRK,
		NULL
	);
	XtVaSetValues(m->main.nexttrk_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_BNEXTTRK,
		XmNrightPosition, RIGHT_BNEXTTRK,
		XmNtopPosition, TOP_BNEXTTRK,
		XmNbottomPosition, BOTTOM_BNEXTTRK,
		NULL
	);
}


/*
 * geom_main_force
 *	Set the geometry of the widgets in the main window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_main_force(widgets_t *m)
{
	Display		*display = XtDisplay(m->toplevel);
	int		screen = DefaultScreen(display),
			i,
			x_delta,
			y_delta;
	Dimension	width,
			height,
			cwidth,
			cheight,
			swidth,
			sheight;
	Position	abs_x,
			abs_y,
			new_x,
			new_y,
			wmcorr_x,
			wmcorr_y;
	static int	wlist_size = 0;
	static Widget	wlist[WLIST_SIZE];
	static bool_t	first = TRUE;

	swidth = (Dimension) XDisplayWidth(display, screen);
	sheight = (Dimension) XDisplayHeight(display, screen);
	XtTranslateCoords(m->toplevel, 0, 0, &abs_x, &abs_y);

	if (first) {
		/* Set up the list of main window widgets that gets
		 * managed and unmanaged when switching modes.
		 */
		i = 0;
		wlist[i++] = m->main.chkbox_frm;
		wlist[i++] = m->main.disc_ind;
		wlist[i++] = m->main.index_ind;
		wlist[i++] = m->main.rptcnt_ind;
		wlist[i++] = m->main.dbmode_ind;
		wlist[i++] = m->main.timemode_ind;
		wlist[i++] = m->main.playmode_ind;
		wlist[i++] = m->main.dtitle_ind;
		wlist[i++] = m->main.ttitle_ind;
		wlist[i++] = m->main.dbprog_btn;
		wlist[i++] = m->main.dbprog_btn;
		wlist[i++] = m->main.options_btn;
		wlist[i++] = m->main.wwwwarp_bar;
		wlist[i++] = m->main.ab_btn;
		wlist[i++] = m->main.sample_btn;
		wlist[i++] = m->main.time_btn;
		wlist[i++] = m->main.keypad_btn;
		wlist[i++] = m->main.prevdisc_btn;
		wlist[i++] = m->main.nextdisc_btn;
		wlist[i++] = m->main.previdx_btn;
		wlist[i++] = m->main.nextidx_btn;
		wlist[i++] = m->main.rew_btn;
		wlist[i++] = m->main.ff_btn;
		wlist_size = i;

		wmcorr_x = wmcorr_y = 0;

		/* Current window width */
		switch (main_mode) {
		case MAIN_BASIC:
			cwidth = (Dimension) app_data.basic_width;
			cheight = (Dimension) app_data.basic_height;
			break;
		case MAIN_NORMAL:
		default:
			cwidth = (Dimension) app_data.normal_width;
			cheight = (Dimension) app_data.normal_height;
			break;
		}
	}
	else {
		/* Hack: Get window manager correction factor.
		 * This works around the problem with some window
		 * managers where setting the XmNx and XmNy positions
		 * of the toplevel does not take into account of the
		 * window manager decorations.
		 */
		XtVaSetValues(m->toplevel,
			XmNx, abs_x + 1,
			XmNy, abs_y + 1,
			NULL
		);
		for (i = 0; i < 10; i++)
			event_loop(0);

		XtTranslateCoords(m->toplevel, 0, 0, &new_x, &new_y);

		wmcorr_x = new_x - abs_x - 1;
		wmcorr_y = new_y - abs_y - 1;

		XtVaSetValues(m->toplevel,
			XmNx, abs_x - wmcorr_x,
			XmNy, abs_y - wmcorr_y,
			NULL
		);

		if (wmcorr_x != 0 || wmcorr_y != 0) {
			DBGPRN(DBG_GEN)(errfp,
				"Window manager correction factor: (%d,%d)\n",
				wmcorr_x, wmcorr_y);
		}

		/* Get current window width */
		XtVaGetValues(m->toplevel,
			XmNwidth, &cwidth,
			XmNheight, &cheight,
			NULL
		);
	}

	switch (main_mode) {
	case MAIN_BASIC:
		if (first) {
			/* HACK: on startup, if the mode is basic,
			 * set to normal first so that all widgets
			 * get properly managed.
			 */
			geom_normal_force(m);
		}
		width = (Dimension) app_data.basic_width;
		height = (Dimension) app_data.basic_height;
		x_delta = (int) (cwidth - width);
		y_delta = (int) (cheight - height);
		XtUnmanageChildren((WidgetList) wlist, wlist_size);
		geom_basic_force(m);
		break;
	case MAIN_NORMAL:
	default:
		width = (Dimension) app_data.normal_width;
		height = (Dimension) app_data.normal_height;
		x_delta = (int) (width - cwidth);
		y_delta = (int) (height - cheight);
		if (!first)
			XtManageChildren((WidgetList) wlist, wlist_size);
		geom_normal_force(m);
		break;
	}

	/* Set window coordinatess according to the
	 * modeChangeGravity parameter
	 */
	switch (app_data.modechg_grav) {
	case 1:
		/* Top edge */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x - (x_delta / 2);
			new_y = abs_y;
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x + (x_delta / 2);
			new_y = abs_y;
		}
		break;
	case 2:
		/* Upper right corner */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x - x_delta;
			new_y = abs_y;
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x + x_delta;
			new_y = abs_y;
		}
		break;
	case 3:
		/* Right edge */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x - x_delta;
			new_y = abs_y - (y_delta / 2);
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x + x_delta;
			new_y = abs_y + (y_delta / 2);
		}
		break;
	case 4:
		/* Lower right corner */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x - x_delta;
			new_y = abs_y - y_delta;
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x + x_delta;
			new_y = abs_y + y_delta;
		}
		break;
	case 5:
		/* Bottom edge */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x - (x_delta / 2);
			new_y = abs_y - y_delta;
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x + (x_delta / 2);
			new_y = abs_y + y_delta;
		}
		break;
	case 6:
		/* Lower left corner */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x;
			new_y = abs_y - y_delta;
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x;
			new_y = abs_y + y_delta;
		}
		break;
	case 7:
		/* Left edge */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x;
			new_y = abs_y - (y_delta / 2);
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x;
			new_y = abs_y + (y_delta / 2);
		}
		break;
	case 8:
		/* Center of window */
		if (main_mode == MAIN_NORMAL) {
			new_x = abs_x - (x_delta / 2);
			new_y = abs_y - (y_delta / 2);
		}
		else if (main_mode == MAIN_BASIC) {
			new_x = abs_x + (x_delta / 2);
			new_y = abs_y + (y_delta / 2);
		}
		break;
	case 0:
	default:
		/* Upper left corner */
		new_x = abs_x;
		new_y = abs_y;
		break;
	}

	/* Make sure the window will appear within
	 * the screen boundaries after resizing.
	 */
	if (new_x < wmcorr_x)
		new_x = wmcorr_x;
	else if ((Dimension) (new_x + width + wmcorr_x) > swidth)
		new_x = swidth - width - wmcorr_x;

	if (new_y < wmcorr_y)
		new_y = wmcorr_y;
	else if ((Dimension) (new_y + height + wmcorr_x) > sheight)
		new_y = sheight - height - wmcorr_x;

	/* Set new main window location and size */
	if (new_x == abs_x && new_y == abs_y) {
		/* Set size only */
		XtVaSetValues(m->toplevel,
			XmNwidth, width,
			XmNheight, height,
			NULL
		);
	}
	else {
		/* Set location and size */
		XtVaSetValues(m->toplevel,
			XmNx, new_x - wmcorr_x,
			XmNy, new_y - wmcorr_y,
			XmNwidth, width,
			XmNheight, height,
			NULL
		);
	}

	first = FALSE;
}


/*
 * geom_keypad_force
 *	Set the geometry of the widgets in the keypad window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_keypad_force(widgets_t *m)
{
	XtVaSetValues(m->keypad.keypad_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEYPADLBL,
		XmNrightPosition, RIGHT_KEYPADLBL,
		XmNtopPosition, TOP_KEYPADLBL,
		XmNbottomPosition, BOTTOM_KEYPADLBL,
		NULL
	);

	XtVaSetValues(m->keypad.keypad_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEYPADIND,
		XmNrightPosition, RIGHT_KEYPADIND,
		XmNtopPosition, TOP_KEYPADIND,
		XmNbottomPosition, BOTTOM_KEYPADIND,
		NULL
	);

	XtVaSetValues(m->keypad.radio_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_KPMODEBOX,
		XmNrightPosition, RIGHT_KPMODEBOX,
		XmNtopPosition, TOP_KPMODEBOX,
		XmNheight, HEIGHT_KPMODEBOX,
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);

	XtVaSetValues(m->keypad.track_btn,
		XmNheight, TOGGLE_HEIGHT2,
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->keypad.disc_btn,
		XmNheight, TOGGLE_HEIGHT2,
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[0],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY0,
		XmNrightPosition, RIGHT_KEY0,
		XmNtopPosition, TOP_KEY0,
		XmNbottomPosition, BOTTOM_KEY0,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[1],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY1,
		XmNrightPosition, RIGHT_KEY1,
		XmNtopPosition, TOP_KEY1,
		XmNbottomPosition, BOTTOM_KEY1,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[2],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY2,
		XmNrightPosition, RIGHT_KEY2,
		XmNtopPosition, TOP_KEY2,
		XmNbottomPosition, BOTTOM_KEY2,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[3],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY3,
		XmNrightPosition, RIGHT_KEY3,
		XmNtopPosition, TOP_KEY3,
		XmNbottomPosition, BOTTOM_KEY3,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[4],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY4,
		XmNrightPosition, RIGHT_KEY4,
		XmNtopPosition, TOP_KEY4,
		XmNbottomPosition, BOTTOM_KEY4,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[5],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY5,
		XmNrightPosition, RIGHT_KEY5,
		XmNtopPosition, TOP_KEY5,
		XmNbottomPosition, BOTTOM_KEY5,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[6],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY6,
		XmNrightPosition, RIGHT_KEY6,
		XmNtopPosition, TOP_KEY6,
		XmNbottomPosition, BOTTOM_KEY6,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[7],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY7,
		XmNrightPosition, RIGHT_KEY7,
		XmNtopPosition, TOP_KEY7,
		XmNbottomPosition, BOTTOM_KEY7,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[8],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY8,
		XmNrightPosition, RIGHT_KEY8,
		XmNtopPosition, TOP_KEY8,
		XmNbottomPosition, BOTTOM_KEY8,
		NULL
	);

	XtVaSetValues(m->keypad.num_btn[9],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_KEY9,
		XmNrightPosition, RIGHT_KEY9,
		XmNtopPosition, TOP_KEY9,
		XmNbottomPosition, BOTTOM_KEY9,
		NULL
	);

	XtVaSetValues(m->keypad.clear_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_CLEAR,
		XmNrightPosition, RIGHT_CLEAR,
		XmNtopPosition, TOP_CLEAR,
		XmNbottomPosition, BOTTOM_CLEAR,
		NULL
	);

	XtVaSetValues(m->keypad.enter_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_ENTER,
		XmNrightPosition, RIGHT_ENTER,
		XmNtopPosition, TOP_ENTER,
		XmNbottomPosition, BOTTOM_ENTER,
		NULL
	);

	XtVaSetValues(m->keypad.warp_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_WARPLBL,
		XmNrightPosition, RIGHT_WARPLBL,
		XmNtopPosition, TOP_WARPLBL,
		XmNbottomPosition, BOTTOM_WARPLBL,
		NULL
	);

	XtVaSetValues(m->keypad.warp_scale,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNleftPosition, LEFT_WARPSCALE,
		XmNrightPosition, RIGHT_WARPSCALE,
		XmNtopPosition, TOP_WARPSCALE,
		XmNbottomPosition, BOTTOM_WARPSCALE,
		NULL
	);

	XtVaSetValues(m->keypad.keypad_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_KEYPADSEP,
		XmNrightPosition, RIGHT_KEYPADSEP,
		XmNtopPosition, TOP_KEYPADSEP,
		NULL
	);

	XtVaSetValues(m->keypad.cancel_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_KPCANCEL,
		XmNrightPosition, RIGHT_KPCANCEL,
		XmNtopPosition, TOP_KPCANCEL,
		NULL
	);
}


/*
 * geom_options_force
 *	Set the geometry of the widgets in the options window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_options_force(widgets_t *m)
{
	XtVaSetValues(m->options.reset_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_RESET_BTN,
		XmNrightPosition, RIGHT_RESET_BTN,
		XmNbottomOffset, BOFF_RESET_BTN,
		NULL
	);

	XtVaSetValues(m->options.save_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SAVE_BTN,
		XmNrightPosition, RIGHT_SAVE_BTN,
		XmNbottomOffset, BOFF_SAVE_BTN,
		NULL
	);

	XtVaSetValues(m->options.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_OPTOK_BTN,
		XmNrightPosition, RIGHT_OPTOK_BTN,
		XmNbottomOffset, BOFF_OPTOK_BTN,
		NULL
	);

	XtVaSetValues(m->options.options_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_OPTSEP,
		XmNrightPosition, RIGHT_OPTSEP,
		XmNbottomWidget, m->options.ok_btn,
		XmNbottomOffset, BOFF_OPTSEP,
		NULL
	);

	XtVaSetValues(m->options.categ_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftOffset, LOFF_CATEG_LBL,
		XmNrightPosition, RIGHT_CATEG_LBL,
		XmNtopOffset, TOFF_CATEG_LBL,
		NULL
	);

	XtVaSetValues(m->options.categ_list,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftOffset, LOFF_CATEG_LIST,
		XmNrightPosition, RIGHT_CATEG_LIST,
		XmNtopWidget, m->options.categ_lbl,
		XmNtopOffset, TOFF_CATEG_LIST,
		XmNbottomWidget, m->options.options_sep,
		XmNbottomOffset, BOFF_CATEG_LIST,
		NULL
	);

	XtVaSetValues(m->options.categ_sep,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftWidget, m->options.categ_list,
		XmNleftOffset, LOFF_CATEG_SEP,
		XmNbottomWidget, m->options.options_sep,
		NULL
	);

	XtVaSetValues(m->options.mode_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MODE_LBL,
		XmNrightPosition, RIGHT_MODE_LBL,
		XmNtopOffset, TOFF_MODE_LBL,
		NULL
	);

	XtVaSetValues(m->options.mode_chkbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MODE_CHKFRM,
		XmNrightPosition, RIGHT_MODE_CHKFRM,
		XmNtopWidget, m->options.mode_lbl,
		XmNtopOffset, TOFF_MODE_CHKFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_MODE_CHKFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.mode_std_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmONE_OF_MANY,
		NULL
	);
	XtVaSetValues(m->options.mode_cdda_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmN_OF_MANY,
		NULL
	);
	XtVaSetValues(m->options.mode_file_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmN_OF_MANY,
		NULL
	);
	XtVaSetValues(m->options.mode_pipe_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmN_OF_MANY,
		NULL
	);

	XtVaSetValues(m->options.mode_jitter_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_JITTER_BTN,
		XmNtopWidget, m->options.mode_chkbox_frm,
		XmNtopOffset, TOFF_JITTER_BTN,
		NULL
	);

	XtVaSetValues(m->options.mode_trkfile_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.mode_jitter_btn,
		XmNleftOffset, LOFF_TRKFILE_BTN,
		XmNtopWidget, m->options.mode_chkbox_frm,
		XmNtopOffset, TOFF_TRKFILE_BTN,
		NULL
	);

	XtVaSetValues(m->options.mode_subst_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.mode_trkfile_btn,
		XmNleftOffset, LOFF_SUBST_BTN,
		XmNtopWidget, m->options.mode_chkbox_frm,
		XmNtopOffset, TOFF_SUBST_BTN,
		NULL
	);

	XtVaSetValues(m->options.mode_fmt_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MODEFMT_OPT,
		XmNtopWidget, m->options.mode_jitter_btn,
		XmNtopOffset, TOFF_MODEFMT_OPT,
		NULL
	);

	XtVaSetValues(m->options.mode_perfmon_btn,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.mode_fmt_opt,
		XmNtopWidget, m->options.mode_fmt_opt,
		XmNrightOffset, ROFF_PERFMON_BTN,
		XmNtopOffset, TOFF_PERFMON_BTN,
		NULL
	);

	XtVaSetValues(m->options.mode_path_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNleftOffset, LOFF_PATH_LBL,
		XmNrightOffset, ROFF_PATH_LBL,
		XmNtopWidget, m->options.mode_fmt_opt,
		XmNtopOffset, TOFF_PATH_LBL,
		NULL
	);

	XtVaSetValues(m->options.mode_path_txt,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNleftOffset, LOFF_PATH_TXT,
		XmNrightOffset, ROFF_PATH_TXT,
		XmNtopWidget, m->options.mode_path_lbl,
		XmNtopOffset, TOFF_PATH_TXT,
		NULL
	);

	XtVaSetValues(m->options.mode_pathexp_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->options.mode_path_txt,
		XmNrightOffset, ROFF_PATHEXP_BTN,
		XmNtopWidget, m->options.mode_path_txt,
		XmNbottomWidget, m->options.mode_path_txt,
		NULL
	);

	XtVaSetValues(m->options.mode_prog_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNleftOffset, LOFF_PROG_LBL,
		XmNrightOffset, ROFF_PROG_LBL,
		XmNtopWidget, m->options.mode_path_txt,
		XmNtopOffset, TOFF_PROG_LBL,
		NULL
	);

	XtVaSetValues(m->options.mode_prog_txt,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNleftOffset, LOFF_PROG_TXT,
		XmNrightOffset, ROFF_PROG_TXT,
		XmNtopWidget, m->options.mode_prog_lbl,
		XmNtopOffset, TOFF_PROG_TXT,
		NULL
	);

	XtVaSetValues(m->options.method_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_METHOD_OPT,
		XmNtopOffset, TOFF_METHOD_OPT,
		NULL
	);

	XtVaSetValues(m->options.qualval_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.method_opt,
		XmNrightPosition, RIGHT_QUALVAL_LBL,
		XmNleftOffset, LOFF_QUALVAL_LBL,
		XmNtopOffset, TOFF_QUALVAL_LBL,
		NULL
	);

	XtVaSetValues(m->options.qualval_scl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_QUALVAL_SCL,
		XmNrightPosition, RIGHT_QUALVAL_SCL,
		XmNtopWidget, m->options.qualval_lbl,
		NULL
	);

	XtVaSetValues(m->options.qualval1_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->options.qualval_scl,
		XmNtopWidget, m->options.qualval_scl,
		XmNbottomWidget, m->options.qualval_scl,
		NULL
	);

	XtVaSetValues(m->options.qualval2_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->options.qualval_scl,
		XmNtopWidget, m->options.qualval_scl,
		XmNbottomWidget, m->options.qualval_scl,
		NULL
	);

	XtVaSetValues(m->options.bitrate_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_BITRATE_OPT,
		XmNtopWidget, m->options.method_opt,
		XmNtopOffset, TOFF_BITRATE_OPT,
		NULL
	);

	XtVaSetValues(m->options.minbrate_opt,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.bitrate_opt,
		XmNtopWidget, m->options.method_opt,
		XmNleftOffset, LOFF_MINBRATE_OPT,
		XmNtopOffset, TOFF_MINBRATE_OPT,
		NULL
	);

	XtVaSetValues(m->options.maxbrate_opt,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNleftWidget, m->options.minbrate_opt,
		XmNtopWidget, m->options.method_opt,
		XmNleftOffset, LOFF_MAXBRATE_OPT,
		XmNtopOffset, TOFF_MAXBRATE_OPT,
		NULL
	);

	XtVaSetValues(m->options.chmode_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MP3MODE_OPT,
		XmNtopWidget, m->options.bitrate_opt,
		XmNtopOffset, TOFF_MP3MODE_OPT,
		NULL
	);

	XtVaSetValues(m->options.compalgo_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.chmode_opt,
		XmNrightPosition, RIGHT_MP3ALGO_LBL,
		XmNtopWidget, m->options.maxbrate_opt,
		XmNtopOffset, TOFF_MP3ALGO_LBL,
		NULL
	);

	XtVaSetValues(m->options.compalgo_scl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MP3ALGO_SCL,
		XmNrightPosition, RIGHT_MP3ALGO_SCL,
		XmNtopWidget, m->options.compalgo_lbl,
		NULL
	);

	XtVaSetValues(m->options.compalgo1_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->options.compalgo_scl,
		XmNtopWidget, m->options.compalgo_scl,
		XmNbottomWidget, m->options.compalgo_scl,
		NULL
	);

	XtVaSetValues(m->options.compalgo2_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->options.compalgo_scl,
		XmNtopWidget, m->options.compalgo_scl,
		XmNbottomWidget, m->options.compalgo_scl,
		NULL
	);

	XtVaSetValues(m->options.encode_sep,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNtopWidget, m->options.compalgo_scl,
		XmNtopOffset, TOFF_ENCODE_SEP,
		NULL
	);

	XtVaSetValues(m->options.lp_freq_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LPFREQ_TXT,
		XmNrightPosition, RIGHT_LPFREQ_TXT,
		XmNtopWidget, m->options.encode_sep,
		XmNtopOffset, TOFF_LPFREQ_TXT,
		NULL
	);

	XtVaSetValues(m->options.lp_width_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LPWIDTH_TXT,
		XmNrightPosition, RIGHT_LPWIDTH_TXT,
		XmNtopWidget, m->options.encode_sep,
		XmNtopOffset, TOFF_LPWIDTH_TXT,
		NULL
	);

	XtVaSetValues(m->options.lp_opt,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->options.lp_freq_txt,
		XmNtopWidget, m->options.lp_freq_txt,
		XmNtopOffset, TOFF_LP_OPT,
		XmNrightOffset, ROFF_LP_OPT,
		XmNmarginHeight, 0,
		NULL
	);

	XtVaSetValues(m->options.hp_freq_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_HPFREQ_TXT,
		XmNrightPosition, RIGHT_HPFREQ_TXT,
		XmNtopWidget, m->options.lp_freq_txt,
		XmNtopOffset, TOFF_HPFREQ_TXT,
		NULL
	);

	XtVaSetValues(m->options.hp_width_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_HPWIDTH_TXT,
		XmNrightPosition, RIGHT_HPWIDTH_TXT,
		XmNtopWidget, m->options.lp_width_txt,
		XmNtopOffset, TOFF_HPWIDTH_TXT,
		NULL
	);

	XtVaSetValues(m->options.hp_opt,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->options.hp_freq_txt,
		XmNtopWidget, m->options.hp_freq_txt,
		XmNtopOffset, TOFF_HP_OPT,
		XmNrightOffset, ROFF_HP_OPT,
		XmNmarginHeight, 0,
		NULL
	);

	XtVaSetValues(m->options.freq_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_FREQ_LBL,
		XmNrightPosition, RIGHT_FREQ_LBL,
		XmNbottomWidget, m->options.lp_freq_txt,
		NULL
	);

	XtVaSetValues(m->options.width_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_WIDTH_LBL,
		XmNrightPosition, RIGHT_WIDTH_LBL,
		XmNbottomWidget, m->options.lp_width_txt,
		NULL
	);

	XtVaSetValues(m->options.filters_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_FILTERS_LBL,
		XmNtopWidget, m->options.freq_lbl,
		XmNbottomWidget, m->options.freq_lbl,
		NULL
	);

	XtVaSetValues(m->options.encode_sep2,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNtopWidget, m->options.hp_opt,
		XmNtopOffset, TOFF_ENCODE_SEP2,
		NULL
	);

	XtVaSetValues(m->options.copyrt_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_COPYRT_BTN,
		XmNtopWidget, m->options.encode_sep2,
		XmNtopOffset, TOFF_COPYRT_BTN,
		XmNleftOffset, LOFF_COPYRT_BTN,
		NULL
	);

	XtVaSetValues(m->options.orig_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.copyrt_btn,
		XmNtopWidget, m->options.encode_sep2,
		XmNtopOffset, TOFF_ORIG_BTN,
		XmNleftOffset, LOFF_ORIG_BTN,
		NULL
	);

	XtVaSetValues(m->options.nores_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.orig_btn,
		XmNtopWidget, m->options.encode_sep2,
		XmNtopOffset, TOFF_NORES_BTN,
		XmNleftOffset, LOFF_NORES_BTN,
		NULL
	);

	XtVaSetValues(m->options.chksum_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.nores_btn,
		XmNtopWidget, m->options.encode_sep2,
		XmNtopOffset, TOFF_CHKSUM_BTN,
		XmNleftOffset, LOFF_CHKSUM_BTN,
		NULL
	);

	XtVaSetValues(m->options.id3tag_ver_opt,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightOffset, ROFF_ID3TAG_OPT,
		XmNtopWidget, m->options.chksum_btn,
		XmNtopOffset, TOFF_ID3TAG_OPT,
		NULL
	);

	XtVaSetValues(m->options.addtag_btn,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->options.orig_btn,
		XmNtopWidget, m->options.id3tag_ver_opt,
		XmNbottomWidget, m->options.id3tag_ver_opt,
		NULL
	);

	XtVaSetValues(m->options.iso_btn,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->options.copyrt_btn,
		XmNtopWidget, m->options.addtag_btn,
		XmNbottomWidget, m->options.addtag_btn,
		NULL
	);

	XtVaSetValues(m->options.lame_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LAME_LBL,
		XmNrightPosition, RIGHT_LAME_LBL,
		XmNtopOffset, TOFF_LAME_LBL,
		NULL
	);

	XtVaSetValues(m->options.lame_mode_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LAME_MODE_LBL,
		XmNrightPosition, RIGHT_LAME_MODE_LBL,
		XmNtopWidget, m->options.lame_lbl,
		XmNtopOffset, TOFF_LAME_MODE_LBL,
		NULL
	);

	XtVaSetValues(m->options.lame_radbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LAME_RADFRM,
		XmNrightPosition, RIGHT_LAME_RADFRM,
		XmNtopWidget, m->options.lame_mode_lbl,
		XmNtopOffset, TOFF_LAME_RADFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_LAME_RADFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.lame_disable_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.lame_insert_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.lame_append_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.lame_replace_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.lame_opts_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LAME_OPTS_LBL,
		XmNrightPosition, RIGHT_LAME_OPTS_LBL,
		XmNtopWidget, m->options.lame_radbox_frm,
		XmNtopOffset, TOFF_LAME_OPTS_LBL,
		NULL
	);

	XtVaSetValues(m->options.lame_opts_txt,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNleftOffset, LOFF_LAME_OPTS_TXT,
		XmNrightOffset, ROFF_LAME_OPTS_TXT,
		XmNtopWidget, m->options.lame_opts_lbl,
		XmNtopOffset, TOFF_LAME_OPTS_TXT,
		NULL
	);

	XtVaSetValues(m->options.sched_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SCHED_LBL,
		XmNrightPosition, RIGHT_SCHED_LBL,
		XmNtopOffset, TOFF_SCHED_LBL,
		NULL
	);

	XtVaSetValues(m->options.sched_rd_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SCHED_RD_LBL,
		XmNrightPosition, RIGHT_SCHED_RD_LBL,
		XmNtopWidget, m->options.sched_lbl,
		XmNtopOffset, TOFF_SCHED_RD_LBL,
		NULL
	);

	XtVaSetValues(m->options.sched_rd_radbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SCHED_RD_RADFRM,
		XmNrightPosition, RIGHT_SCHED_RD_RADFRM,
		XmNtopWidget, m->options.sched_rd_lbl,
		XmNtopOffset, TOFF_SCHED_RD_RADFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_SCHED_RD_RADFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.sched_rd_norm_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.sched_rd_hipri_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.sched_wr_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SCHED_WR_LBL,
		XmNrightPosition, RIGHT_SCHED_WR_LBL,
		XmNtopWidget, m->options.sched_rd_radbox_frm,
		XmNtopOffset, TOFF_SCHED_WR_LBL,
		NULL
	);

	XtVaSetValues(m->options.sched_wr_radbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SCHED_WR_RADFRM,
		XmNrightPosition, RIGHT_SCHED_WR_RADFRM,
		XmNtopWidget, m->options.sched_wr_lbl,
		XmNtopOffset, TOFF_SCHED_WR_RADFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_SCHED_WR_RADFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.sched_wr_norm_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.sched_wr_hipri_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.hb_tout_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_HB_TOUT_LBL,
		XmNrightPosition, RIGHT_HB_TOUT_LBL,
		XmNtopWidget, m->options.sched_wr_radbox_frm,
		XmNtopOffset, TOFF_HB_TOUT_LBL,
		NULL
	);

	XtVaSetValues(m->options.hb_tout_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_HB_TOUT_TXT,
		XmNrightPosition, RIGHT_HB_TOUT_TXT,
		XmNtopWidget, m->options.hb_tout_lbl,
		XmNtopOffset, TOFF_HB_TOUT_TXT,
		NULL
	);

	XtVaSetValues(m->options.load_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LOAD_LBL,
		XmNrightPosition, RIGHT_LOAD_LBL,
		XmNtopOffset, TOFF_LOAD_LBL,
		NULL
	);

	XtVaSetValues(m->options.load_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LOAD_LBL,
		XmNrightPosition, RIGHT_LOAD_LBL,
		XmNtopOffset, TOFF_LOAD_LBL,
		NULL
	);

	XtVaSetValues(m->options.load_chkbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LOAD_CHKFRM,
		XmNrightPosition, RIGHT_LOAD_CHKFRM,
		XmNtopWidget, m->options.load_lbl,
		XmNtopOffset, TOFF_LOAD_CHKFRM,
		XmNshadowType, XmSHADOW_ETCHED_IN,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_LOAD_CHKFRM,
#endif
		NULL
	);
	XtVaSetValues(m->options.load_none_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmONE_OF_MANY,
		NULL
	);
	XtVaSetValues(m->options.load_spdn_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmONE_OF_MANY,
		NULL
	);
	XtVaSetValues(m->options.load_play_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmONE_OF_MANY,
		NULL
	);
	XtVaSetValues(m->options.load_lock_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		XmNindicatorType, XmN_OF_MANY,
		NULL
	);

	XtVaSetValues(m->options.eject_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_EJECT_LBL,
		XmNrightPosition, RIGHT_EJECT_LBL,
		XmNtopWidget, m->options.load_chkbox_frm,
		XmNtopOffset, TOFF_EJECT_LBL,
		NULL
	);

	XtVaSetValues(m->options.eject_chkbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_EJECT_CHKFRM,
		XmNrightPosition, RIGHT_EJECT_CHKFRM,
		XmNtopWidget, m->options.eject_lbl,
		XmNtopOffset, TOFF_EJECT_CHKFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_EJECT_CHKFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.eject_exit_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.done_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DONE_LBL,
		XmNrightPosition, RIGHT_DONE_LBL,
		XmNtopOffset, TOFF_DONE_LBL,
		NULL
	);

	XtVaSetValues(m->options.done_chkbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DONE_CHKFRM,
		XmNrightPosition, RIGHT_DONE_CHKFRM,
		XmNtopWidget, m->options.done_lbl,
		XmNtopOffset, TOFF_DONE_CHKFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_DONE_CHKFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.done_eject_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.done_exit_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.exit_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_EXIT_LBL,
		XmNrightPosition, RIGHT_EXIT_LBL,
		XmNtopWidget, m->options.done_chkbox_frm,
		XmNtopOffset, TOFF_EXIT_LBL,
		NULL
	);

	XtVaSetValues(m->options.exit_radbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_EXIT_RADFRM,
		XmNrightPosition, RIGHT_EXIT_RADFRM,
		XmNtopWidget, m->options.exit_lbl,
		XmNtopOffset, TOFF_EXIT_RADFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_EXIT_RADFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.exit_none_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.exit_stop_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.exit_eject_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.chg_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CHG_LBL,
		XmNrightPosition, RIGHT_CHG_LBL,
		XmNtopOffset, TOFF_CHG_LBL,
		NULL
	);

	XtVaSetValues(m->options.chg_chkbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CHG_CHKFRM,
		XmNrightPosition, RIGHT_CHG_CHKFRM,
		XmNtopWidget, m->options.chg_lbl,
		XmNtopOffset, TOFF_CHG_CHKFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_CHG_CHKFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.chg_multiplay_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.chg_reverse_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.chroute_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CHROUTE_LBL,
		XmNrightPosition, RIGHT_CHROUTE_LBL,
		XmNtopOffset, TOFF_CHROUTE_LBL,
		NULL
	);

	XtVaSetValues(m->options.chroute_radbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CHROUTE_RADFRM,
		XmNrightPosition, RIGHT_CHROUTE_RADFRM,
		XmNtopWidget, m->options.chroute_lbl,
		XmNtopOffset, TOFF_CHROUTE_RADFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_CHROUTE_RADFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.chroute_stereo_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.chroute_rev_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.chroute_left_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.chroute_right_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.chroute_mono_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.outport_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_OUTPORT_LBL,
		XmNrightPosition, RIGHT_OUTPORT_LBL,
		XmNtopWidget, m->options.chroute_radbox_frm,
		XmNtopOffset, TOFF_OUTPORT_LBL,
		NULL
	);

	XtVaSetValues(m->options.outport_chkbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_OUTPORT_RADFRM,
		XmNrightPosition, RIGHT_OUTPORT_RADFRM,
		XmNtopWidget, m->options.outport_lbl,
		XmNtopOffset, TOFF_OUTPORT_RADFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_OUTPORT_RADFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.outport_spkr_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.outport_phone_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.outport_line_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.vol_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_VOLTP_LBL,
		XmNrightPosition, RIGHT_VOLTP_LBL,
		XmNtopOffset, TOFF_VOLTP_LBL,
		NULL
	);

	XtVaSetValues(m->options.vol_radbox_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_VOLTP_RADFRM,
		XmNrightPosition, RIGHT_VOLTP_RADFRM,
		XmNtopWidget, m->options.vol_lbl,
		XmNtopOffset, TOFF_VOLTP_RADFRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_VOLTP_RADFRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.vol_linear_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.vol_square_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.vol_invsqr_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.bal_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_BAL_LBL,
		XmNrightPosition, RIGHT_BAL_LBL,
		XmNtopWidget, m->options.vol_radbox_frm,
		XmNtopOffset, TOFF_BAL_LBL,
		NULL
	);

	XtVaSetValues(m->options.bal_scale,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_BAL_SCALE,
		XmNrightPosition, RIGHT_BAL_SCALE,
		XmNtopWidget, m->options.bal_lbl,
		XmNtopOffset, TOFF_BAL_SCALE,
		NULL
	);

	XtVaSetValues(m->options.ball_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_BALL_LBL,
		XmNrightPosition, RIGHT_BALL_LBL,
		XmNtopWidget, m->options.bal_scale,
		XmNbottomWidget, m->options.bal_scale,
		NULL
	);

	XtVaSetValues(m->options.balr_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_BALR_LBL,
		XmNrightPosition, RIGHT_BALR_LBL,
		XmNtopWidget, m->options.bal_scale,
		XmNbottomWidget, m->options.bal_scale,
		NULL
	);

	XtVaSetValues(m->options.balctr_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_BALCTR_BTN,
		XmNrightPosition, RIGHT_BALCTR_BTN,
		XmNtopWidget, m->options.bal_scale,
		XmNtopOffset, TOFF_BALCTR_BTN,
		NULL
	);

	XtVaSetValues(m->options.cdda_att_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CDDA_ATT_LBL,
		XmNrightPosition, RIGHT_CDDA_ATT_LBL,
		XmNtopWidget, m->options.balctr_btn,
		XmNtopOffset, TOFF_CDDA_ATT_LBL,
		NULL
	);

	XtVaSetValues(m->options.cdda_att_scale,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CDDA_ATT_SCALE,
		XmNrightPosition, RIGHT_CDDA_ATT_SCALE,
		XmNtopWidget, m->options.cdda_att_lbl,
		XmNtopOffset, TOFF_CDDA_ATT_SCALE,
		NULL
	);

	XtVaSetValues(m->options.cdda_fadeout_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_FADEOUT_BTN,
		XmNrightPosition, RIGHT_FADEOUT_BTN,
		XmNtopWidget, m->options.cdda_att_scale,
		XmNbottomWidget, m->options.cdda_att_scale,
		XmNtopOffset, TOFF_FADEOUT_BTN,
		NULL
	);

	XtVaSetValues(m->options.cdda_fadein_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_FADEIN_BTN,
		XmNrightPosition, RIGHT_FADEIN_BTN,
		XmNtopWidget, m->options.cdda_att_scale,
		XmNbottomWidget, m->options.cdda_att_scale,
		XmNtopOffset, TOFF_FADEIN_BTN,
		NULL
	);

	XtVaSetValues(m->options.cdda_fadeout_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_FADEOUT_LBL,
		XmNrightPosition, RIGHT_FADEOUT_LBL,
		XmNtopWidget, m->options.cdda_fadeout_btn,
		XmNtopOffset, TOFF_FADEOUT_LBL,
		NULL
	);

	XtVaSetValues(m->options.cdda_fadein_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_FADEIN_LBL,
		XmNrightPosition, RIGHT_FADEIN_LBL,
		XmNtopWidget, m->options.cdda_fadein_btn,
		XmNtopOffset, TOFF_FADEIN_LBL,
		NULL
	);

	XtVaSetValues(m->options.cddb_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CDDB_LBL,
		XmNrightPosition, RIGHT_CDDB_LBL,
		XmNtopOffset, TOFF_CDDB_LBL,
		NULL
	);

	XtVaSetValues(m->options.mbrowser_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MBROWSER_LBL,
		XmNrightPosition, RIGHT_MBROWSER_LBL,
		XmNtopWidget, m->options.cddb_lbl,
		XmNtopOffset, TOFF_MBROWSER_LBL,
		NULL
	);

	XtVaSetValues(m->options.mbrowser_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MBROWSER_FRM,
		XmNrightPosition, RIGHT_MBROWSER_FRM,
		XmNtopWidget, m->options.mbrowser_lbl,
		XmNtopOffset, TOFF_MBROWSER_FRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_MBROWSER_FRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.autobr_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.manbr_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.lookup_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LOOKUP_LBL,
		XmNrightPosition, RIGHT_LOOKUP_LBL,
		XmNtopWidget, m->options.cddb_lbl,
		XmNtopOffset, TOFF_LOOKUP_LBL,
		NULL
	);

	XtVaSetValues(m->options.lookup_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LOOKUP_FRM,
		XmNrightPosition, RIGHT_LOOKUP_FRM,
		XmNtopWidget, m->options.lookup_lbl,
		XmNtopOffset, TOFF_LOOKUP_FRM,
#ifndef USE_SGI_DESKTOP
		XmNheight, HEIGHT_LOOKUP_FRM,
#endif
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->options.cddb_pri_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->options.cdtext_pri_btn,
#ifndef USE_SGI_DESKTOP
		XmNheight, TOGGLE_HEIGHT2,
#endif
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->options.cddb_sep1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->options.categ_sep,
		XmNtopWidget, m->options.mbrowser_frm,
		XmNtopOffset, TOFF_CDDB_SEP1,
		NULL
	);

	XtVaSetValues(m->options.serv_tout_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SERV_TOUT_LBL,
		XmNrightPosition, RIGHT_SERV_TOUT_LBL,
		XmNtopWidget, m->options.cddb_sep1,
		XmNtopOffset, TOFF_SERV_TOUT_LBL,
		NULL
	);

	XtVaSetValues(m->options.serv_tout_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SERV_TOUT_TXT,
		XmNrightPosition, RIGHT_SERV_TOUT_TXT,
		XmNtopWidget, m->options.serv_tout_lbl,
		XmNtopOffset, TOFF_SERV_TOUT_TXT,
		NULL
	);

	XtVaSetValues(m->options.cache_tout_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CACHE_TOUT_LBL,
		XmNrightPosition, RIGHT_CACHE_TOUT_LBL,
		XmNtopWidget, m->options.cddb_sep1,
		XmNtopOffset, TOFF_CACHE_TOUT_LBL,
		NULL
	);

	XtVaSetValues(m->options.cache_tout_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CACHE_TOUT_TXT,
		XmNrightPosition, RIGHT_CACHE_TOUT_TXT,
		XmNtopWidget, m->options.cache_tout_lbl,
		XmNtopOffset, TOFF_CACHE_TOUT_TXT,
		NULL
	);

	XtVaSetValues(m->options.cddb_sep2,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNleftWidget, m->options.categ_sep,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNtopWidget, m->options.cache_tout_txt,
		XmNtopOffset, TOFF_CDDB_SEP2,
		NULL
	);

	XtVaSetValues(m->options.proxy_srvr_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_PROXY_SRVR_LBL,
		XmNrightPosition, RIGHT_PROXY_SRVR_LBL,
		XmNtopWidget, m->options.cddb_sep2,
		XmNtopOffset, TOFF_PROXY_SRVR_LBL,
		NULL
	);

	XtVaSetValues(m->options.proxy_port_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_PROXY_PORT_LBL,
		XmNrightPosition, RIGHT_PROXY_PORT_LBL,
		XmNtopWidget, m->options.cddb_sep2,
		XmNtopOffset, TOFF_PROXY_PORT_LBL,
		NULL
	);

	XtVaSetValues(m->options.proxy_srvr_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_PROXY_SRVR_TXT,
		XmNrightPosition, RIGHT_PROXY_SRVR_TXT,
		XmNtopWidget, m->options.proxy_srvr_lbl,
		XmNtopOffset, TOFF_PROXY_SRVR_TXT,
		NULL
	);

	XtVaSetValues(m->options.proxy_port_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_PROXY_PORT_TXT,
		XmNrightPosition, RIGHT_PROXY_PORT_TXT,
		XmNtopWidget, m->options.proxy_port_lbl,
		XmNtopOffset, TOFF_PROXY_PORT_TXT,
		NULL
	);

	XtVaSetValues(m->options.use_proxy_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_USE_PROXY_BTN,
		XmNrightWidget, m->options.proxy_srvr_txt,
		XmNtopWidget, m->options.proxy_srvr_txt,
		XmNbottomWidget, m->options.proxy_srvr_txt,
		XmNrightOffset, ROFF_USE_PROXY_BTN,
		NULL
	);

	XtVaSetValues(m->options.proxy_auth_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_PROXY_AUTH_BTN,
		XmNtopWidget, m->options.use_proxy_btn,
		XmNtopOffset, TOFF_PROXY_AUTH_BTN,
		NULL
	);
}


/*
 * geom_perfmon_force
 *	Set the geometry of the widgets in the CDDA perf monitor window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_perfmon_force(widgets_t *m)
{
	XtVaSetValues(m->perfmon.speed_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SPEEDLBL,
		XmNrightPosition, RIGHT_SPEEDLBL,
		XmNtopOffset, TOFF_SPEEDLBL,
		NULL
	);

	XtVaSetValues(m->perfmon.speed_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SPEEDIND,
		XmNrightPosition, RIGHT_SPEEDIND,
		XmNtopWidget, m->perfmon.speed_lbl,
		XmNtopOffset, TOFF_SPEEDIND,
		NULL
	);

	XtVaSetValues(m->perfmon.frames_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_FRAMESLBL,
		XmNrightPosition, RIGHT_FRAMESLBL,
		XmNtopWidget, m->perfmon.speed_ind,
		XmNtopOffset, TOFF_FRAMESLBL,
		NULL
	);

	XtVaSetValues(m->perfmon.frames_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_FRAMESIND,
		XmNrightPosition, RIGHT_FRAMESIND,
		XmNtopWidget, m->perfmon.frames_lbl,
		XmNbottomWidget, m->perfmon.frames_lbl,
		NULL
	);

	XtVaSetValues(m->perfmon.fps_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_FPSLBL,
		XmNrightPosition, RIGHT_FPSLBL,
		XmNtopWidget, m->perfmon.frames_lbl,
		XmNtopOffset, TOFF_FPSLBL,
		NULL
	);

	XtVaSetValues(m->perfmon.fps_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_FPSIND,
		XmNrightPosition, RIGHT_FPSIND,
		XmNtopWidget, m->perfmon.fps_lbl,
		XmNbottomWidget, m->perfmon.fps_lbl,
		NULL
	);

	XtVaSetValues(m->perfmon.ttc_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TTCLBL,
		XmNrightPosition, RIGHT_TTCLBL,
		XmNtopWidget, m->perfmon.fps_lbl,
		XmNtopOffset, TOFF_TTCLBL,
		NULL
	);

	XtVaSetValues(m->perfmon.ttc_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_TTCIND,
		XmNrightPosition, RIGHT_TTCIND,
		XmNtopWidget, m->perfmon.ttc_lbl,
		XmNbottomWidget, m->perfmon.ttc_lbl,
		NULL
	);

	XtVaSetValues(m->perfmon.cancel_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_PERFMONCANCEL,
		XmNrightPosition, RIGHT_PERFMONCANCEL,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, BOFF_PERFMONCANCEL,
		NULL
	);

	XtVaSetValues(m->perfmon.perfmon_sep,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, m->perfmon.cancel_btn,
		XmNbottomOffset, BOFF_PERFMONSEP,
		NULL
	);
}


/*
 * geom_dbprog_force
 *	Set the geometry of the widgets in the CD info/program window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_dbprog_force(widgets_t *m)
{
	XtVaSetValues(m->dbprog.tottime_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TOTTIMEIND,
		XmNtopOffset, TOFF_TOTTIMEIND,
		XmNbottomOffset, BOFF_TOTTIMEIND,
		NULL
	);

	XtVaSetValues(m->dbprog.inetoffln_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_INETOFFLN,
		XmNtopWidget, m->dbprog.tottime_ind,
		XmNtopOffset, TOFF_INETOFFLN,
		NULL
	);

	XtVaSetValues(m->dbprog.logo_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LOGO,
		XmNrightPosition, RIGHT_LOGO,
		XmNtopOffset, TOFF_LOGO,
		NULL
	);

	XtVaSetValues(m->dbprog.dlist_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLIST,
		XmNrightPosition, RIGHT_DLIST,
		XmNtopOffset, TOFF_DLIST,
		NULL
	);

	XtVaSetValues(m->dbprog.artist_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_ARTISTLBL,
		XmNrightPosition, RIGHT_ARTISTLBL,
		XmNtopWidget, m->dbprog.logo_lbl,
		XmNtopOffset, TOFF_ARTISTLBL,
		NULL
	);

	XtVaSetValues(m->dbprog.artist_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_ARTIST,
		XmNrightPosition, RIGHT_ARTIST,
		XmNtopWidget, m->dbprog.artist_lbl,
		NULL
	);

	XtVaSetValues(m->dbprog.title_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TITLELBL,
		XmNrightPosition, RIGHT_TITLELBL,
		XmNtopWidget, m->dbprog.artist_txt,
		XmNtopOffset, TOFF_TITLELBL,
		NULL
	);

	XtVaSetValues(m->dbprog.title_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TITLE,
		XmNrightPosition, RIGHT_TITLE,
		XmNtopWidget, m->dbprog.title_lbl,
		NULL
	);

	XtVaSetValues(m->dbprog.fullname_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbprog.artist_txt,
		XmNleftOffset, LOFF_FULLNAME,
		XmNrightPosition, RIGHT_FULLNAME,
		XmNtopWidget, m->dbprog.artist_txt,
		XmNbottomWidget, m->dbprog.artist_txt,
		NULL
	);

	XtVaSetValues(m->dbprog.segments_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_SEGMENTS,
		XmNrightPosition, RIGHT_SEGMENTS,
		XmNbottomWidget, m->dbprog.title_txt,
		XmNbottomOffset, BOFF_SEGMENTS,
		NULL
	);

	XtVaSetValues(m->dbprog.dcredits_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DCRED,
		XmNrightPosition, RIGHT_DCRED,
		XmNbottomWidget, m->dbprog.segments_btn,
		XmNbottomOffset, BOFF_DCRED,
		NULL
	);

	XtVaSetValues(m->dbprog.extd_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_EXTD,
		XmNrightPosition, RIGHT_EXTD,
		XmNbottomWidget, m->dbprog.dcredits_btn,
		XmNbottomOffset, BOFF_EXTD,
		NULL
	);

	XtVaSetValues(m->dbprog.extd_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_EXTDLBL,
		XmNrightPosition, RIGHT_EXTDLBL,
		XmNbottomWidget, m->dbprog.extd_btn,
		XmNbottomOffset, BOFF_EXTDLBL,
		NULL
	);

	XtVaSetValues(m->dbprog.dbprog_sep1,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DBPROGSEP1,
		XmNrightPosition, RIGHT_DBPROGSEP1,
		XmNtopWidget, m->dbprog.title_txt,
		XmNtopOffset, TOFF_DBPROGSEP1,
		NULL
	);

	XtVaSetValues(m->dbprog.trklist_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TRKLISTLBL,
		XmNrightPosition, RIGHT_TRKLISTLBL,
		XmNtopWidget, m->dbprog.dbprog_sep1,
		XmNtopOffset, TOFF_TRKLISTLBL,
		NULL
	);

	XtVaSetValues(m->dbprog.pgm_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_PGMLBL,
		XmNrightPosition, RIGHT_PGMLBL,
		XmNtopWidget, m->dbprog.dbprog_sep1,
		XmNtopOffset, TOFF_PGMLBL,
		NULL
	);

	XtVaSetValues(m->dbprog.addpgm_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_ADDPGM,
		XmNrightPosition, RIGHT_ADDPGM,
		XmNtopWidget, m->dbprog.pgm_lbl,
		NULL
	);

	XtVaSetValues(m->dbprog.clrpgm_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CLRPGM,
		XmNrightPosition, RIGHT_CLRPGM,
		XmNtopWidget, m->dbprog.addpgm_btn,
		NULL
	);

	XtVaSetValues(m->dbprog.savepgm_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SAVEPGM,
		XmNrightPosition, RIGHT_SAVEPGM,
		XmNtopWidget, m->dbprog.clrpgm_btn,
		NULL
	);

	XtVaSetValues(m->dbprog.radio_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_RADIOLBL,
		XmNrightPosition, RIGHT_RADIOLBL,
		XmNtopWidget, m->dbprog.savepgm_btn,
		XmNtopOffset, TOFF_RADIOLBL,
		NULL
	);

	XtVaSetValues(m->dbprog.radio_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_RADIOBOX,
		XmNrightPosition, RIGHT_RADIOBOX,
		XmNtopWidget, m->dbprog.radio_lbl,
		XmNheight, HEIGHT_RADIOBOX,
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL
	);
	XtVaSetValues(m->dbprog.tottime_btn,
		XmNheight, TOGGLE_HEIGHT2,
		XmNrecomputeSize, False,
		NULL
	);
	XtVaSetValues(m->dbprog.trktime_btn,
		XmNheight, TOGGLE_HEIGHT2,
		XmNrecomputeSize, False,
		NULL
	);

	XtVaSetValues(m->dbprog.extt_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_EXTTLBL,
		XmNrightPosition, RIGHT_EXTTLBL,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNtopWidget, m->dbprog.radio_frm,
		XmNtopOffset, TOFF_EXTTLBL,
		NULL
	);

	XtVaSetValues(m->dbprog.extt_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_EXTT,
		XmNrightPosition, RIGHT_EXTT,
		XmNtopWidget, m->dbprog.extt_lbl,
		XmNtopOffset, TOFF_EXTT,
		NULL
	);

	XtVaSetValues(m->dbprog.tcredits_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TCRED,
		XmNrightPosition, RIGHT_TCRED,
		XmNtopWidget, m->dbprog.extt_btn,
		XmNtopOffset, TOFF_TCRED,
		NULL
	);

	XtVaSetValues(m->dbprog.userreg_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_USERREG,
		XmNrightPosition, RIGHT_USERREG,
		XmNbottomWidget, m->dbprog.pgmseq_txt,
		NULL
	);

	XtVaSetValues(m->dbprog.submit_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SUBMIT,
		XmNrightPosition, RIGHT_SUBMIT,
		XmNbottomOffset, BOFF_SUBMIT,
		NULL
	);

	XtVaSetValues(m->dbprog.flush_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_FLUSH,
		XmNrightPosition, RIGHT_FLUSH,
		XmNbottomOffset, BOFF_FLUSH,
		NULL
	);

	XtVaSetValues(m->dbprog.reload_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_RELOAD,
		XmNrightPosition, RIGHT_RELOAD,
		XmNbottomOffset, BOFF_RELOAD,
		NULL
	);

	XtVaSetValues(m->dbprog.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_DPOK,
		XmNrightPosition, RIGHT_DPOK,
		XmNbottomOffset, BOFF_DPOK,
		NULL
	);

	XtVaSetValues(m->dbprog.dbprog_sep2,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DBPROGSEP2,
		XmNrightPosition, RIGHT_DBPROGSEP2,
		XmNbottomWidget, m->dbprog.ok_btn,
		XmNbottomOffset, BOFF_DBPROGSEP2,
		NULL
	);

	XtVaSetValues(m->dbprog.pgmseq_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_PGMSEQ,
		XmNrightPosition, RIGHT_PGMSEQ,
		XmNbottomWidget, m->dbprog.dbprog_sep2,
		XmNbottomOffset, BOFF_PGMSEQLBL,
		NULL
	);

	XtVaSetValues(m->dbprog.pgmseq_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_PGMSEQLBL,
		XmNrightPosition, RIGHT_PGMSEQLBL,
		XmNbottomWidget, m->dbprog.pgmseq_txt,
		NULL
	);

	XtVaSetValues(m->dbprog.ttitle_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_TTITLE,
		XmNrightPosition, RIGHT_TTITLE,
		XmNbottomWidget, m->dbprog.pgmseq_lbl,
		XmNbottomOffset, BOFF_TTITLE,
		NULL
	);

	XtVaSetValues(m->dbprog.apply_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbprog.ttitle_txt,
		XmNleftOffset, LOFF_APPLYBTN,
		XmNrightPosition, RIGHT_APPLYBTN,
		XmNtopWidget, m->dbprog.ttitle_txt,
		XmNbottomWidget, m->dbprog.ttitle_txt,
		NULL
	);

	XtVaSetValues(m->dbprog.ttitle_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_TTITLELBL,
		XmNrightPosition, RIGHT_TTITLELBL,
		XmNbottomWidget, m->dbprog.ttitle_txt,
		XmNbottomOffset, BOFF_TTITLELBL,
		NULL
	);

	XtVaSetValues(XtParent(m->dbprog.trk_list),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_TRKLIST,
		XmNrightPosition, RIGHT_TRKLIST,
		XmNtopWidget, m->dbprog.trklist_lbl,
		XmNbottomWidget, m->dbprog.ttitle_lbl,
		XmNleftOffset, LOFF_TRKLIST,
		XmNbottomOffset, BOFF_TRKLIST,
		NULL
	);
}


/*
 * geom_dlist_force
 *	Set the geometry of the widgets in the Disc List window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_dlist_force(widgets_t *m)
{
	XtVaSetValues(m->dlist.type_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLISTOPT,
		XmNtopOffset, TOFF_DLISTOPT,
		NULL
	);

	XtVaSetValues(m->dlist.disclist_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLISTLBL,
		XmNrightPosition, RIGHT_DLISTLBL,
		XmNtopWidget, m->dlist.type_opt,
		XmNtopOffset, TOFF_DLISTLBL,
		NULL
	);

	XtVaSetValues(m->dlist.show_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLSHOWBTN,
		XmNrightPosition, RIGHT_DLSHOWBTN,
		XmNtopWidget, m->dlist.disclist_lbl,
		XmNtopOffset, TOFF_DLSHOWBTN,
		NULL
	);

	XtVaSetValues(m->dlist.goto_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLGOTOBTN,
		XmNrightPosition, RIGHT_DLGOTOBTN,
		XmNtopWidget, m->dlist.show_btn,
		XmNtopOffset, TOFF_DLGOTOBTN,
		NULL
	);

	XtVaSetValues(m->dlist.del_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLDELBTN,
		XmNrightPosition, RIGHT_DLDELBTN,
		XmNtopWidget, m->dlist.goto_btn,
		XmNtopOffset, TOFF_DLDELBTN,
		NULL
	);

	XtVaSetValues(m->dlist.delall_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLDELALLBTN,
		XmNrightPosition, RIGHT_DLDELALLBTN,
		XmNtopWidget, m->dlist.del_btn,
		XmNtopOffset, TOFF_DLDELALLBTN,
		NULL
	);

	XtVaSetValues(m->dlist.rescan_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DLRESCANBTN,
		XmNrightPosition, RIGHT_DLRESCANBTN,
		XmNtopWidget, m->dlist.delall_btn,
		XmNtopOffset, TOFF_DLRESCANBTN,
		NULL
	);

	XtVaSetValues(m->dlist.dlist_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DLISTSEP,
		XmNrightPosition, RIGHT_DLISTSEP,
		XmNbottomWidget, m->dlist.cancel_btn,
		XmNbottomOffset, BOFF_DLISTSEP,
		NULL
	);

	XtVaSetValues(m->dlist.cancel_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_DLISTCANCEL,
		XmNrightPosition, RIGHT_DLISTCANCEL,
		XmNbottomOffset, BOFF_DLISTCANCEL,
		NULL
	);

	XtVaSetValues(XtParent(m->dlist.disc_list),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DISCLIST,
		XmNrightPosition, RIGHT_DISCLIST,
		XmNtopWidget, m->dlist.disclist_lbl,
		XmNbottomWidget, m->dlist.dlist_sep,
		XmNleftOffset, LOFF_DISCLIST,
		XmNtopOffset, TOFF_DISCLIST,
		XmNbottomOffset, BOFF_DISCLIST,
		NULL
	);
}


/*
 * geom_fullname_force
 *	Set the geometry of the widgets in the full name window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_fullname_force(widgets_t *m)
{
	XtVaSetValues(m->fullname.head_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DAHEADLBL,
		XmNrightPosition, RIGHT_DAHEADLBL,
		XmNtopOffset, TOFF_DAHEADLBL,
		NULL
	);

	XtVaSetValues(m->fullname.dispname_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DADISPNAMETXT,
		XmNrightPosition, RIGHT_DADISPNAMETXT,
		XmNtopWidget, m->fullname.head_lbl,
		XmNtopOffset, TOFF_DADISPNAMETXT,
		NULL
	);

	XtVaSetValues(m->fullname.dispname_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->fullname.dispname_txt,
		XmNrightOffset, ROFF_DADISPNAMELBL,
		XmNtopWidget, m->fullname.dispname_txt,
		XmNbottomWidget, m->fullname.dispname_txt,
		NULL
	);

	XtVaSetValues(m->fullname.autogen_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->fullname.dispname_txt,
		XmNleftOffset, LOFF_DAAUTOGENBTN,
		XmNrightPosition, RIGHT_DAAUTOGENBTN,
		XmNtopWidget, m->fullname.dispname_txt,
		XmNbottomWidget, m->fullname.dispname_txt,
		NULL
	);

	XtVaSetValues(m->fullname.firstname_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DAFIRSTNAMETXT,
		XmNrightPosition, RIGHT_DAFIRSTNAMETXT,
		XmNtopWidget, m->fullname.dispname_txt,
		XmNtopOffset, TOFF_DAFIRSTNAMETXT,
		NULL
	);

	XtVaSetValues(m->fullname.firstname_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->fullname.firstname_txt,
		XmNrightOffset, ROFF_DAFIRSTNAMELBL,
		XmNtopWidget, m->fullname.firstname_txt,
		XmNbottomWidget, m->fullname.firstname_txt,
		NULL
	);

	XtVaSetValues(m->fullname.lastname_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DALASTNAMETXT,
		XmNrightPosition, RIGHT_DALASTNAMETXT,
		XmNtopWidget, m->fullname.firstname_txt,
		XmNtopOffset, TOFF_DALASTNAMETXT,
		NULL
	);

	XtVaSetValues(m->fullname.lastname_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->fullname.lastname_txt,
		XmNrightOffset, ROFF_DALASTNAMELBL,
		XmNtopWidget, m->fullname.lastname_txt,
		XmNbottomWidget, m->fullname.lastname_txt,
		NULL
	);

	XtVaSetValues(m->fullname.the_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DATHETXT,
		XmNrightPosition, RIGHT_DATHETXT,
		XmNtopWidget, m->fullname.lastname_txt,
		XmNtopOffset, TOFF_DATHETXT,
		NULL
	);

	XtVaSetValues(m->fullname.the_btn,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->fullname.the_txt,
		XmNtopWidget, m->fullname.lastname_txt,
		XmNrightOffset, ROFF_DATHEBTN,
		XmNtopOffset, TOFF_DATHEBTN,
		NULL
	);

	XtVaSetValues(m->fullname.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_DAOK,
		XmNrightPosition, RIGHT_DAOK,
		XmNbottomOffset, BOFF_DAOK,
		NULL
	);

	XtVaSetValues(m->fullname.fullname_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DASEP,
		XmNrightPosition, RIGHT_DASEP,
		XmNbottomWidget, m->fullname.ok_btn,
		XmNbottomOffset, BOFF_DASEP,
		NULL
	);
}


/*
 * geom_extd_force
 *	Set the geometry of the widgets in the disc details window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_extd_force(widgets_t *m)
{
	XtVaSetValues(m->dbextd.discno_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDDISCNUM,
		XmNrightPosition, RIGHT_DDDISCNUM,
		XmNtopOffset, TOFF_DDDISCNUM,
		NULL
	);
	
	XtVaSetValues(m->dbextd.disc_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDDISCLBL,
		XmNrightPosition, RIGHT_DDDISCLBL,
		XmNtopWidget, m->dbextd.discno_lbl,
		XmNtopOffset, TOFF_DDDISCLBL,
		NULL
	);

	XtVaSetValues(m->dbextd.dbextd_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDSEP,
		XmNrightPosition, RIGHT_DDSEP,
		XmNtopWidget, m->dbextd.disc_lbl,
		XmNtopOffset, TOFF_DDSEP,
		NULL
	);

	XtVaSetValues(m->dbextd.the_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDTHETXT,
		XmNrightPosition, RIGHT_DDTHETXT,
		XmNtopWidget, m->dbextd.dbextd_sep,
		XmNtopOffset, TOFF_DDTHETXT,
		NULL
	);

	XtVaSetValues(m->dbextd.the_btn,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextd.the_txt,
		XmNtopWidget, m->dbextd.the_txt,
		XmNbottomWidget, m->dbextd.the_txt,
		XmNrightOffset, ROFF_DDTHELBL,
		NULL
	);

	XtVaSetValues(m->dbextd.sorttitle_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDSORTTITLE_TXT,
		XmNrightWidget, m->dbextd.the_btn,
		XmNtopWidget, m->dbextd.dbextd_sep,
		XmNtopOffset, TOFF_DDSORTTITLE_TXT,
		XmNrightOffset, ROFF_DDSORTTITLE_TXT,
		NULL
	);

	XtVaSetValues(m->dbextd.sorttitle_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextd.sorttitle_txt,
		XmNrightOffset, ROFF_DDSORTTITLE_LBL,
		XmNtopWidget, m->dbextd.sorttitle_txt,
		XmNbottomWidget, m->dbextd.sorttitle_txt,
		NULL
	);
	
	XtVaSetValues(m->dbextd.year_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDYEARTXT,
		XmNrightPosition, RIGHT_DDYEARTXT,
		XmNtopWidget, m->dbextd.sorttitle_txt,
		XmNtopOffset, TOFF_DDYEARTXT,
		NULL
	);

	XtVaSetValues(m->dbextd.year_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextd.year_txt,
		XmNrightOffset, ROFF_DDYEARLBL,
		XmNtopWidget, m->dbextd.year_txt,
		XmNbottomWidget, m->dbextd.year_txt,
		NULL
	);

	XtVaSetValues(m->dbextd.label_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbextd.year_txt,
		XmNtopWidget, m->dbextd.year_txt,
		XmNbottomWidget, m->dbextd.year_txt,
		XmNleftOffset, LOFF_DDLABELLBL,
		XmNrightOffset, ROFF_DDLABELLBL,
		NULL
	);

	XtVaSetValues(m->dbextd.comp_btn,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, m->dbextd.label_lbl,
		XmNbottomWidget, m->dbextd.label_lbl,
		XmNrightOffset, ROFF_DDCOMP,
		NULL
	);
	
	XtVaSetValues(m->dbextd.label_txt,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->dbextd.label_lbl,
		XmNrightWidget, m->dbextd.comp_btn,
		XmNtopWidget, m->dbextd.sorttitle_txt,
		XmNleftOffset, LOFF_DDLABELTXT,
		XmNrightOffset, ROFF_DDLABELTXT,
		XmNtopOffset, TOFF_DDLABELTXT,
		NULL
	);

	XtVaSetValues(m->dbextd.subgenre_opt[0],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDSUBGENRE0OPT,
		XmNtopWidget, m->dbextd.year_txt,
		XmNleftOffset, LOFF_DDSUBGENRE0OPT,
		XmNrightOffset, ROFF_DDSUBGENRE0OPT,
		XmNtopOffset, TOFF_DDSUBGENRE0OPT,
		NULL
	);

	XtVaSetValues(m->dbextd.genre_opt[0],
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->dbextd.subgenre_opt[0],
		XmNtopWidget, m->dbextd.year_txt,
		XmNleftOffset, LOFF_DDGENRE0OPT,
		XmNrightOffset, ROFF_DDGENRE0OPT,
		XmNtopOffset, TOFF_DDGENRE0OPT,
		NULL
	);

	XtVaSetValues(m->dbextd.subgenre_opt[1],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNtopWidget, m->dbextd.subgenre_opt[0],
		XmNleftPosition, LEFT_DDSUBGENRE1OPT,
		XmNleftOffset, LOFF_DDSUBGENRE1OPT,
		XmNrightOffset, ROFF_DDSUBGENRE1OPT,
		XmNtopOffset, TOFF_DDSUBGENRE1OPT,
		NULL
	);

	XtVaSetValues(m->dbextd.genre_opt[1],
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->dbextd.subgenre_opt[1],
		XmNtopWidget, m->dbextd.genre_opt[0],
		XmNleftOffset, LOFF_DDGENRE1OPT,
		XmNrightOffset, ROFF_DDGENRE1OPT,
		XmNtopOffset, TOFF_DDGENRE1OPT,
		NULL
	);

	XtVaSetValues(m->dbextd.region_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDREGIONTXT,
		XmNrightPosition, RIGHT_DDREGIONTXT,
		XmNtopWidget, m->dbextd.genre_opt[1],
		XmNtopOffset, TOFF_DDREGIONTXT,
		NULL
	);

	XtVaSetValues(m->dbextd.region_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextd.region_txt,
		XmNrightOffset, ROFF_DDREGIONLBL,
		XmNtopWidget, m->dbextd.region_txt,
		XmNbottomWidget, m->dbextd.region_txt,
		NULL
	);

	XtVaSetValues(m->dbextd.region_chg_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbextd.region_txt,
		XmNtopWidget, m->dbextd.region_txt,
		XmNbottomWidget, m->dbextd.region_txt,
		XmNleftOffset, LOFF_DDREGIONCHGBTN,
		NULL
	);

	XtVaSetValues(m->dbextd.lang_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDLANGTXT,
		XmNrightPosition, RIGHT_DDLANGTXT,
		XmNtopWidget, m->dbextd.region_txt,
		XmNtopOffset, TOFF_DDLANGTXT,
		NULL
	);

	XtVaSetValues(m->dbextd.lang_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextd.lang_txt,
		XmNrightOffset, ROFF_DDLANGLBL,
		XmNtopWidget, m->dbextd.lang_txt,
		XmNbottomWidget, m->dbextd.lang_txt,
		NULL
	);

	XtVaSetValues(m->dbextd.lang_chg_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbextd.lang_txt,
		XmNtopWidget, m->dbextd.lang_txt,
		XmNbottomWidget, m->dbextd.lang_txt,
		XmNleftOffset, LOFF_DDLANGCHGBTN,
		NULL
	);

	XtVaSetValues(m->dbextd.dnum_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDDNUMTXT,
		XmNrightPosition, RIGHT_DDDNUMTXT,
		XmNtopWidget, m->dbextd.subgenre_opt[1],
		XmNtopOffset, TOFF_DDDNUMTXT,
		NULL
	);

	XtVaSetValues(m->dbextd.dnum_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextd.dnum_txt,
		XmNrightOffset, ROFF_DDDNUMLBL,
		XmNtopWidget, m->dbextd.dnum_txt,
		XmNbottomWidget, m->dbextd.dnum_txt,
		NULL
	);

	XtVaSetValues(m->dbextd.tnum_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDTNUMTXT,
		XmNrightPosition, RIGHT_DDTNUMTXT,
		XmNtopWidget, m->dbextd.genre_opt[1],
		XmNtopOffset, TOFF_DDTNUMTXT,
		NULL
	);

	XtVaSetValues(m->dbextd.of_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbextd.dnum_txt,
		XmNrightWidget, m->dbextd.tnum_txt,
		XmNtopWidget, m->dbextd.dnum_txt,
		XmNbottomWidget, m->dbextd.dnum_txt,
		NULL
	);

	XtVaSetValues(m->dbextd.dbextd_sep2,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDSEP2,
		XmNrightPosition, RIGHT_DDSEP2,
		XmNtopWidget, m->dbextd.lang_txt,
		XmNtopOffset, TOFF_DDSEP2,
		NULL
	);

	XtVaSetValues(m->dbextd.notes_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DDNOTESLBL,
		XmNrightPosition, RIGHT_DDNOTESLBL,
		XmNtopWidget, m->dbextd.dbextd_sep2,
		XmNtopOffset, TOFF_DDNOTESLBL,
		NULL
	);

	XtVaSetValues(m->dbextd.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_DDOK,
		XmNrightPosition, RIGHT_DDOK,
		XmNbottomOffset, BOFF_DDOK,
		NULL
	);

	XtVaSetValues(m->dbextd.dbextd_sep3,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DDSEP3,
		XmNrightPosition, RIGHT_DDSEP3,
		XmNbottomWidget, m->dbextd.ok_btn,
		XmNbottomOffset, BOFF_DDSEP3,
		NULL
	);

	XtVaSetValues(m->dbextd.cert_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DDCERTLBL,
		XmNrightPosition, RIGHT_DDCERTLBL,
		XmNbottomWidget, m->dbextd.dbextd_sep3,
		XmNbottomOffset, BOFF_DDCERTLBL,
		NULL
	);
	XtVaSetValues(m->dbextd.cert_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_DDCERTIND,
		XmNrightPosition, RIGHT_DDCERTIND,
		XmNtopWidget, m->dbextd.cert_lbl,
		XmNbottomWidget, m->dbextd.cert_lbl,
		NULL
	);

	XtVaSetValues(m->dbextd.rev_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DDREVLBL,
		XmNrightPosition, RIGHT_DDREVLBL,
		XmNbottomWidget, m->dbextd.cert_lbl,
		XmNbottomOffset, BOFF_DDREVLBL,
		NULL
	);
	XtVaSetValues(m->dbextd.rev_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_DDREVIND,
		XmNrightPosition, RIGHT_DDREVIND,
		XmNtopWidget, m->dbextd.rev_lbl,
		XmNbottomWidget, m->dbextd.rev_lbl,
		NULL
	);

	XtVaSetValues(m->dbextd.discid_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DDDISCIDLBL,
		XmNrightPosition, RIGHT_DDDISCIDLBL,
		XmNbottomWidget, m->dbextd.rev_lbl,
		XmNbottomOffset, BOFF_DDDISCIDLBL,
		NULL
	);
	XtVaSetValues(m->dbextd.discid_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_DDDISCIDIND,
		XmNrightPosition, RIGHT_DDDISCIDIND,
		XmNtopWidget, m->dbextd.discid_lbl,
		XmNbottomWidget, m->dbextd.discid_lbl,
		NULL
	);

	XtVaSetValues(m->dbextd.mcn_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_DDMCNLBL,
		XmNrightPosition, RIGHT_DDMCNLBL,
		XmNtopWidget, m->dbextd.discid_ind,
		XmNbottomWidget, m->dbextd.discid_ind,
		NULL
	);
	XtVaSetValues(m->dbextd.mcn_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_DDMCNIND,
		XmNrightPosition, RIGHT_DDMCNIND,
		XmNtopWidget, m->dbextd.mcn_lbl,
		XmNbottomWidget, m->dbextd.mcn_lbl,
		NULL
	);

	XtVaSetValues(XtParent(m->dbextd.notes_txt),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DDNOTESTXT,
		XmNrightPosition, RIGHT_DDNOTESTXT,
		XmNtopWidget, m->dbextd.notes_lbl,
		XmNbottomWidget, m->dbextd.discid_lbl,
		XmNbottomOffset, BOFF_DDNOTESTXT,
		NULL
	);
}


/*
 * geom_extt_force
 *	Set the geometry of the widgets in the track details window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_extt_force(widgets_t *m)
{
	XtVaSetValues(m->dbextt.autotrk_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTAUTOTRKBTN,
		XmNtopOffset, TOFF_DTAUTOTRKBTN,
		NULL
	);

	XtVaSetValues(m->dbextt.prev_btn,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->dbextt.trkno_lbl,
		XmNtopOffset, TOFF_DTPREV,
		XmNrightOffset, ROFF_DTPREV,
		NULL
	);

	XtVaSetValues(m->dbextt.next_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->dbextt.trkno_lbl,
		XmNtopOffset, TOFF_DTNEXT,
		XmNleftOffset, LOFF_DTNEXT,
		NULL
	);
	
	XtVaSetValues(m->dbextt.trkno_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTTRKNUMLBL,
		XmNrightPosition, RIGHT_DTTRKNUMLBL,
		XmNtopOffset, TOFF_DTTRKNUMLBL,
		NULL
	);

	XtVaSetValues(m->dbextt.trk_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TRKLBL,
		XmNrightPosition, RIGHT_TRKLBL,
		XmNtopWidget, m->dbextt.trkno_lbl,
		XmNtopOffset, TOFF_TRKLBL,
		NULL
	);

	XtVaSetValues(m->dbextt.dbextt_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTSEP,
		XmNrightPosition, RIGHT_DTSEP,
		XmNtopWidget, m->dbextt.trk_lbl,
		XmNtopOffset, TOFF_DTSEP,
		NULL
	);

	XtVaSetValues(m->dbextt.the_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTTHETXT,
		XmNrightPosition, RIGHT_DTTHETXT,
		XmNtopWidget, m->dbextt.dbextt_sep,
		XmNtopOffset, TOFF_DTTHETXT,
		NULL
	);

	XtVaSetValues(m->dbextt.the_btn,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextt.the_txt,
		XmNtopWidget, m->dbextt.the_txt,
		XmNbottomWidget, m->dbextt.the_txt,
		XmNrightOffset, ROFF_DTTHELBL,
		NULL
	);

	XtVaSetValues(m->dbextt.sorttitle_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTSORTTITLETXT,
		XmNrightWidget, m->dbextt.the_btn,
		XmNtopWidget, m->dbextt.dbextt_sep,
		XmNtopOffset, TOFF_DTSORTTITLETXT,
		XmNrightOffset, ROFF_DTSORTTITLETXT,
		NULL
	);

	XtVaSetValues(m->dbextt.sorttitle_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextt.sorttitle_txt,
		XmNrightOffset, ROFF_DTSORTTITLELBL,
		XmNtopWidget, m->dbextt.sorttitle_txt,
		XmNbottomWidget, m->dbextt.sorttitle_txt,
		NULL
	);
	
	XtVaSetValues(m->dbextt.artist_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTARTISTTXT,
		XmNrightPosition, RIGHT_DTARTISTTXT,
		XmNtopWidget, m->dbextt.sorttitle_txt,
		XmNtopOffset, TOFF_DTARTISTTXT,
		NULL
	);

	XtVaSetValues(m->dbextt.fullname_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbextt.artist_txt,
		XmNtopWidget, m->dbextt.artist_txt,
		XmNbottomWidget, m->dbextt.artist_txt,
		NULL
	);

	XtVaSetValues(m->dbextt.artist_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextt.artist_txt,
		XmNrightOffset, ROFF_DTARTISTLBL,
		XmNtopWidget, m->dbextt.artist_txt,
		XmNbottomWidget, m->dbextt.artist_txt,
		NULL
	);

	XtVaSetValues(m->dbextt.year_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTYEARTXT,
		XmNrightPosition, RIGHT_DTYEARTXT,
		XmNtopWidget, m->dbextt.artist_txt,
		XmNtopOffset, TOFF_DTYEARTXT,
		NULL
	);

	XtVaSetValues(m->dbextt.bpm_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTBPMTXT,
		XmNrightPosition, RIGHT_DTBPMTXT,
		XmNtopWidget, m->dbextt.artist_txt,
		XmNtopOffset, TOFF_DTBPMTXT,
		NULL
	);

	XtVaSetValues(m->dbextt.year_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextt.year_txt,
		XmNrightOffset, ROFF_DTYEARLBL,
		XmNtopWidget, m->dbextt.year_txt,
		XmNbottomWidget, m->dbextt.year_txt,
		NULL
	);

	XtVaSetValues(m->dbextt.label_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->dbextt.year_txt,
		XmNtopWidget, m->dbextt.year_txt,
		XmNbottomWidget, m->dbextt.year_txt,
		XmNleftOffset, LOFF_DTLABELLBL,
		XmNrightOffset, ROFF_DTLABELLBL,
		NULL
	);

	XtVaSetValues(m->dbextt.bpm_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->dbextt.bpm_txt,
		XmNtopWidget, m->dbextt.label_lbl,
		XmNbottomWidget, m->dbextt.label_lbl,
		XmNrightOffset, ROFF_DTBPMLBL,
		NULL
	);

	XtVaSetValues(m->dbextt.label_txt,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->dbextt.label_lbl,
		XmNrightWidget, m->dbextt.bpm_lbl,
		XmNtopWidget, m->dbextt.artist_txt,
		XmNleftOffset, LOFF_DTLABELTXT,
		XmNrightOffset, ROFF_DTLABELTXT,
		XmNtopOffset, TOFF_DTLABELTXT,
		NULL
	);

	XtVaSetValues(m->dbextt.subgenre_opt[0],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTSUBGENRE0OPT,
		XmNtopWidget, m->dbextt.year_txt,
		XmNleftOffset, LOFF_DTSUBGENRE0OPT,
		XmNrightOffset, ROFF_DTSUBGENRE0OPT,
		XmNtopOffset, TOFF_DTSUBGENRE0OPT,
		NULL
	);

	XtVaSetValues(m->dbextt.genre_opt[0],
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->dbextt.subgenre_opt[0],
		XmNtopWidget, m->dbextt.year_txt,
		XmNleftOffset, LOFF_DTGENRE0OPT,
		XmNrightOffset, ROFF_DTGENRE0OPT,
		XmNtopOffset, TOFF_DTGENRE0OPT,
		NULL
	);

	XtVaSetValues(m->dbextt.subgenre_opt[1],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTSUBGENRE1OPT,
		XmNtopWidget, m->dbextt.subgenre_opt[0],
		XmNleftOffset, LOFF_DTSUBGENRE1OPT,
		XmNrightOffset, ROFF_DTSUBGENRE1OPT,
		XmNtopOffset, TOFF_DTSUBGENRE1OPT,
		NULL
	);

	XtVaSetValues(m->dbextt.genre_opt[1],
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->dbextt.subgenre_opt[1],
		XmNtopWidget, m->dbextt.genre_opt[0],
		XmNleftOffset, LOFF_DTGENRE1OPT,
		XmNrightOffset, ROFF_DTGENRE1OPT,
		XmNtopOffset, TOFF_DTGENRE1OPT,
		NULL
	);

	XtVaSetValues(m->dbextt.dbextt_sep2,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTSEP2,
		XmNrightPosition, RIGHT_DTSEP2,
		XmNtopWidget, m->dbextt.genre_opt[1],
		XmNtopOffset, TOFF_DTSEP2,
		NULL
	);

	XtVaSetValues(m->dbextt.notes_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_DTNOTESLBL,
		XmNrightPosition, RIGHT_DTNOTESLBL,
		XmNtopWidget, m->dbextt.dbextt_sep2,
		XmNtopOffset, TOFF_DTNOTESLBL,
		NULL
	);

	XtVaSetValues(m->dbextt.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_DTOK,
		XmNrightPosition, RIGHT_DTOK,
		XmNbottomOffset, BOFF_DTOK,
		NULL
	);

	XtVaSetValues(m->dbextt.dbextt_sep3,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DTSEP3,
		XmNrightPosition, RIGHT_DTSEP3,
		XmNbottomWidget, m->dbextt.ok_btn,
		XmNbottomOffset, BOFF_DTSEP3,
		NULL
	);

	XtVaSetValues(m->dbextt.isrc_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DTISRCLBL,
		XmNrightPosition, RIGHT_DTISRCLBL,
		XmNbottomWidget, m->dbextt.dbextt_sep3,
		XmNbottomOffset, BOFF_DTISRCLBL,
		NULL
	);
	XtVaSetValues(m->dbextt.isrc_ind,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DTISRCIND,
		XmNrightPosition, RIGHT_DTISRCIND,
		XmNbottomWidget, m->dbextt.dbextt_sep3,
		XmNbottomOffset, BOFF_DTISRCIND,
		NULL
	);

	XtVaSetValues(XtParent(m->dbextt.notes_txt),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_DTNOTESTXT,
		XmNrightPosition, RIGHT_DTNOTESTXT,
		XmNtopWidget, m->dbextt.notes_lbl,
		XmNbottomWidget, m->dbextt.isrc_lbl,
		XmNbottomOffset, BOFF_DTNOTESTXT,
		NULL
	);
}


/*
 * geom_credits_force
 *	Set the geometry of the widgets in the credits window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_credits_force(widgets_t *m)
{
	XtVaSetValues(m->credits.autotrk_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRAUTOTRKBTN,
		XmNtopOffset, TOFF_CRAUTOTRKBTN,
		NULL
	);

	XtVaSetValues(m->credits.disctrk_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRDISCTRKLBL,
		XmNrightPosition, RIGHT_CRDISCTRKLBL,
		XmNtopOffset, TOFF_CRDISCTRKLBL,
		NULL
	);

	XtVaSetValues(m->credits.prev_btn,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNrightWidget, m->credits.disctrk_lbl,
		XmNtopOffset, TOFF_CRPREV,
		XmNrightOffset, ROFF_CRPREV,
		NULL
	);

	XtVaSetValues(m->credits.next_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftWidget, m->credits.disctrk_lbl,
		XmNtopOffset, TOFF_CRNEXT,
		XmNleftOffset, LOFF_CRNEXT,
		NULL
	);
	
	XtVaSetValues(m->credits.title_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRTITLELBL,
		XmNrightPosition, RIGHT_CRTITLELBL,
		XmNtopWidget, m->credits.disctrk_lbl,
		XmNtopOffset, TOFF_CRTITLELBL,
		NULL
	);

	XtVaSetValues(m->credits.credlist_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRLISTLBL,
		XmNrightPosition, RIGHT_CRLISTLBL,
		XmNtopWidget, m->credits.title_lbl,
		XmNtopOffset, TOFF_CRLISTLBL,
		NULL
	);

	XtVaSetValues(m->credits.cred_sep1,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRSEP1,
		XmNrightPosition, RIGHT_CRSEP1,
		XmNtopPosition, TOP_CRSEP1,
		NULL
	);

	XtVaSetValues(XtParent(m->credits.cred_list),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_CRLIST,
		XmNrightPosition, RIGHT_CRLIST,
		XmNtopWidget, m->credits.credlist_lbl,
		XmNbottomWidget, m->credits.cred_sep1,
		XmNtopOffset, TOFF_CRLIST,
		XmNbottomOffset, BOFF_CRLIST,
		NULL
	);

	XtVaSetValues(m->credits.crededit_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CREDITORLBL,
		XmNrightPosition, RIGHT_CREDITORLBL,
		XmNtopWidget, m->credits.cred_sep1,
		XmNtopOffset, TOFF_CREDITORLBL,
		NULL
	);

	XtVaSetValues(m->credits.subrole_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRSUBROLEOPT,
		XmNtopWidget, m->credits.crededit_lbl,
		XmNleftOffset, LOFF_CRSUBROLEOPT,
		XmNrightOffset, ROFF_CRSUBROLEOPT,
		XmNtopOffset, TOFF_CRSUBROLEOPT,
		NULL
	);

	XtVaSetValues(m->credits.prirole_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRPRIROLEOPT,
		XmNrightWidget, m->credits.subrole_opt,
		XmNtopWidget, m->credits.crededit_lbl,
		XmNrightOffset, ROFF_CRPRIROLEOPT,
		XmNtopOffset, TOFF_CRPRIROLEOPT,
		NULL
	);

	XtVaSetValues(m->credits.name_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRNAMETXT,
		XmNrightPosition, RIGHT_CRNAMETXT,
		XmNtopWidget, m->credits.prirole_opt,
		XmNtopOffset, TOFF_CRNAMETXT,
		NULL
	);

	XtVaSetValues(m->credits.fullname_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->credits.name_txt,
		XmNtopWidget, m->credits.name_txt,
		XmNbottomWidget, m->credits.name_txt,
		NULL
	);

	XtVaSetValues(m->credits.name_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->credits.name_txt,
		XmNrightOffset, ROFF_CRNAMELBL,
		XmNtopWidget, m->credits.name_txt,
		XmNbottomWidget, m->credits.name_txt,
		NULL
	);

	XtVaSetValues(m->credits.notes_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_CRNOTESLBL,
		XmNrightPosition, RIGHT_CRNOTESLBL,
		XmNtopWidget, m->credits.name_txt,
		XmNtopOffset, TOFF_CRNOTESLBL,
		NULL
	);

	XtVaSetValues(m->credits.add_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_CRADD,
		XmNrightPosition, RIGHT_CRADD,
		XmNbottomOffset, BOFF_CRADD,
		NULL
	);

	XtVaSetValues(m->credits.mod_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_CRMOD,
		XmNrightPosition, RIGHT_CRMOD,
		XmNbottomOffset, BOFF_CRMOD,
		NULL
	);

	XtVaSetValues(m->credits.del_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_CRDEL,
		XmNrightPosition, RIGHT_CRDEL,
		XmNbottomOffset, BOFF_CRDEL,
		NULL
	);

	XtVaSetValues(m->credits.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_CROK,
		XmNrightPosition, RIGHT_CROK,
		XmNbottomOffset, BOFF_CROK,
		NULL
	);

	XtVaSetValues(m->credits.cred_sep2,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_CRSEP2,
		XmNrightPosition, RIGHT_CRSEP2,
		XmNbottomOffset, BOFF_CRSEP2,
		XmNbottomWidget, m->credits.ok_btn,
		NULL
	);

	XtVaSetValues(XtParent(m->credits.notes_txt),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_CRNOTESTXT,
		XmNrightPosition, RIGHT_CRNOTESTXT,
		XmNtopWidget, m->credits.notes_lbl,
		XmNbottomWidget, m->credits.cred_sep2,
		XmNtopOffset, TOFF_CRNOTESTXT,
		XmNbottomOffset, BOFF_CRNOTESTXT,
		NULL
	);
}


/*
 * geom_segments_force
 *	Set the geometry of the widgets in the segments window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_segments_force(widgets_t *m)
{
	XtVaSetValues(m->segments.discno_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGDISCNOLBL,
		XmNrightPosition, RIGHT_SGDISCNOLBL,
		XmNtopOffset, TOFF_SGDISCNOLBL,
		NULL
	);

	XtVaSetValues(m->segments.disc_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGDISCLBL,
		XmNrightPosition, RIGHT_SGDISCLBL,
		XmNtopWidget, m->segments.discno_lbl,
		XmNtopOffset, TOFF_SGDISCLBL,
		NULL
	);

	XtVaSetValues(m->segments.seglist_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGLISTLBL,
		XmNrightPosition, RIGHT_SGLISTLBL,
		XmNtopWidget, m->segments.disc_lbl,
		XmNtopOffset, TOFF_SGLISTLBL,
		NULL
	);

	XtVaSetValues(m->segments.seg_sep1,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGSEP1,
		XmNrightPosition, RIGHT_SGSEP1,
		XmNtopPosition, TOP_SGSEP1,
		NULL
	);

	XtVaSetValues(XtParent(m->segments.seg_list),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SGLIST,
		XmNrightPosition, RIGHT_SGLIST,
		XmNtopWidget, m->segments.seglist_lbl,
		XmNbottomWidget, m->segments.seg_sep1,
		XmNtopOffset, TOFF_SGLIST,
		XmNbottomOffset, BOFF_SGLIST,
		NULL
	);

	XtVaSetValues(m->segments.segedit_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGEDITORLBL,
		XmNrightPosition, RIGHT_SGEDITORLBL,
		XmNtopWidget, m->segments.seg_sep1,
		XmNtopOffset, TOFF_SGEDITORLBL,
		NULL
	);

	XtVaSetValues(m->segments.name_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGNAMETXT,
		XmNrightPosition, RIGHT_SGNAMETXT,
		XmNtopWidget, m->segments.segedit_lbl,
		XmNtopOffset, TOFF_SGNAMETXT,
		NULL
	);

	XtVaSetValues(m->segments.name_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->segments.name_txt,
		XmNrightOffset, ROFF_SGNAMELBL,
		XmNtopWidget, m->segments.name_txt,
		XmNbottomWidget, m->segments.name_txt,
		NULL
	);

	XtVaSetValues(m->segments.track_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGTRACKLBL,
		XmNrightPosition, RIGHT_SGTRACKLBL,
		XmNtopWidget, m->segments.name_txt,
		XmNtopOffset, TOFF_SGTRACKLBL,
		NULL
	);

	XtVaSetValues(m->segments.frame_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGFRAMELBL,
		XmNrightPosition, RIGHT_SGFRAMELBL,
		XmNtopWidget, m->segments.name_txt,
		XmNtopOffset, TOFF_SGFRAMELBL,
		NULL
	);

	XtVaSetValues(m->segments.starttrk_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGSTARTTRKTXT,
		XmNrightPosition, RIGHT_SGSTARTTRKTXT,
		XmNtopWidget, m->segments.track_lbl,
		XmNtopOffset, TOFF_SGSTARTTRKTXT,
		NULL
	);

	XtVaSetValues(m->segments.start_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->segments.starttrk_txt,
		XmNrightOffset, ROFF_SGSTARTLBL,
		XmNtopWidget, m->segments.starttrk_txt,
		XmNbottomWidget, m->segments.starttrk_txt,
		NULL
	);

	XtVaSetValues(m->segments.startfrm_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGSTARTFRMTXT,
		XmNrightPosition, RIGHT_SGSTARTFRMTXT,
		XmNtopWidget, m->segments.frame_lbl,
		XmNtopOffset, TOFF_SGSTARTFRMTXT,
		NULL
	);

	XtVaSetValues(m->segments.endtrk_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGENDTRKTXT,
		XmNrightPosition, RIGHT_SGENDTRKTXT,
		XmNtopWidget, m->segments.starttrk_txt,
		XmNtopOffset, TOFF_SGENDTRKTXT,
		NULL
	);

	XtVaSetValues(m->segments.end_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->segments.endtrk_txt,
		XmNrightOffset, ROFF_SGENDLBL,
		XmNtopWidget, m->segments.endtrk_txt,
		XmNbottomWidget, m->segments.endtrk_txt,
		NULL
	);

	XtVaSetValues(m->segments.endfrm_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGENDFRMTXT,
		XmNrightPosition, RIGHT_SGENDFRMTXT,
		XmNtopWidget, m->segments.startfrm_txt,
		XmNtopOffset, TOFF_SGENDFRMTXT,
		NULL
	);

	XtVaSetValues(m->segments.startptr_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_SGSTARTPTRLBL,
		XmNrightPosition, RIGHT_SGSTARTPTRLBL,
		XmNtopWidget, m->segments.startfrm_txt,
		XmNbottomWidget, m->segments.startfrm_txt,
		NULL
	);

	XtVaSetValues(m->segments.endptr_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_SGENDPTRLBL,
		XmNrightPosition, RIGHT_SGENDPTRLBL,
		XmNtopWidget, m->segments.endfrm_txt,
		XmNbottomWidget, m->segments.endfrm_txt,
		NULL
	);

	XtVaSetValues(m->segments.set_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftPosition, LEFT_SGSETBTN,
		XmNrightPosition, RIGHT_SGSETBTN,
		XmNtopWidget, m->segments.startptr_lbl,
		XmNbottomWidget, m->segments.endptr_lbl,
		NULL
	);

	XtVaSetValues(m->segments.playset_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SGPLAYSETLBL,
		XmNrightPosition, RIGHT_SGPLAYSETLBL,
		XmNbottomWidget, m->segments.set_btn,
		XmNbottomOffset, BOFF_SGPLAYSETLBL,
		NULL
	);

	XtVaSetValues(m->segments.playpaus_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGPLAYPAUSBTN,
		XmNrightPosition, RIGHT_SGPLAYPAUSBTN,
		XmNtopWidget, m->segments.seg_sep1,
		XmNtopOffset, TOFF_SGPLAYPAUSBTN,
		NULL
	);

	XtVaSetValues(m->segments.stop_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGSTOPBTN,
		XmNrightPosition, RIGHT_SGSTOPBTN,
		XmNtopWidget, m->segments.playpaus_btn,
		XmNtopOffset, TOFF_SGSTOPBTN,
		NULL
	);

	XtVaSetValues(m->segments.notes_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SGNOTESLBL,
		XmNrightPosition, RIGHT_SGNOTESLBL,
		XmNtopWidget, m->segments.endtrk_txt,
		XmNtopOffset, TOFF_SGNOTESLBL,
		NULL
	);

	XtVaSetValues(m->segments.credits_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SGCREDITSBTN,
		XmNrightPosition, RIGHT_SGCREDITSBTN,
		XmNbottomWidget, m->segments.seg_sep2,
		XmNbottomOffset, BOFF_SGCREDITSBTN,
		NULL
	);

	XtVaSetValues(m->segments.segment_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SGSEGMENTLBL,
		XmNrightPosition, RIGHT_SGSEGMENTLBL,
		XmNbottomWidget, m->segments.credits_btn,
		XmNbottomOffset, BOFF_SGSEGMENTLBL,
		NULL
	);

	XtVaSetValues(m->segments.add_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SGADD,
		XmNrightPosition, RIGHT_SGADD,
		XmNbottomOffset, BOFF_SGADD,
		NULL
	);

	XtVaSetValues(m->segments.mod_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SGMOD,
		XmNrightPosition, RIGHT_SGMOD,
		XmNbottomOffset, BOFF_SGMOD,
		NULL
	);

	XtVaSetValues(m->segments.del_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SGDEL,
		XmNrightPosition, RIGHT_SGDEL,
		XmNbottomOffset, BOFF_SGDEL,
		NULL
	);

	XtVaSetValues(m->segments.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SGOK,
		XmNrightPosition, RIGHT_SGOK,
		XmNbottomOffset, BOFF_SGOK,
		NULL
	);

	XtVaSetValues(m->segments.seg_sep2,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SGSEP2,
		XmNrightPosition, RIGHT_SGSEP2,
		XmNbottomOffset, BOFF_SGSEP2,
		XmNbottomWidget, m->segments.ok_btn,
		NULL
	);

	XtVaSetValues(XtParent(m->segments.notes_txt),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SGNOTESTXT,
		XmNrightPosition, RIGHT_SGNOTESTXT,
		XmNtopWidget, m->segments.notes_lbl,
		XmNbottomWidget, m->segments.seg_sep2,
		XmNtopOffset, TOFF_SGNOTESTXT,
		XmNbottomOffset, BOFF_SGNOTESTXT,
		NULL
	);
}


/*
 * geom_submiturl_force
 *	Set the geometry of the widgets in the submiturl window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_submiturl_force(widgets_t *m)
{
	XtVaSetValues(m->submiturl.logo_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftOffset, LOFF_SULOGO,
		XmNrightOffset, ROFF_SULOGO,
		XmNtopOffset, TOFF_SULOGO,
		NULL
	);

	XtVaSetValues(m->submiturl.heading_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SUHEADINGLBL,
		XmNrightPosition, RIGHT_SUHEADINGLBL,
		XmNtopWidget, m->submiturl.logo_lbl,
		XmNtopOffset, TOFF_SUHEADINGLBL,
		NULL
	);

	XtVaSetValues(m->submiturl.categ_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SUCATEGTXT,
		XmNrightPosition, RIGHT_SUCATEGTXT,
		XmNtopWidget, m->submiturl.heading_lbl,
		XmNtopOffset, TOFF_SUCATEGTXT,
		NULL
	);

	XtVaSetValues(m->submiturl.categ_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->submiturl.categ_txt,
		XmNrightOffset, ROFF_SUCATEGLBL,
		XmNtopWidget, m->submiturl.categ_txt,
		XmNbottomWidget, m->submiturl.categ_txt,
		NULL
	);

	XtVaSetValues(m->submiturl.name_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SUNAMETXT,
		XmNrightPosition, RIGHT_SUNAMETXT,
		XmNtopWidget, m->submiturl.categ_txt,
		XmNtopOffset, TOFF_SUNAMETXT,
		NULL
	);

	XtVaSetValues(m->submiturl.name_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->submiturl.name_txt,
		XmNrightOffset, ROFF_SUNAMELBL,
		XmNtopWidget, m->submiturl.name_txt,
		XmNbottomWidget, m->submiturl.name_txt,
		NULL
	);

	XtVaSetValues(m->submiturl.url_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SUURLTXT,
		XmNrightPosition, RIGHT_SUURLTXT,
		XmNtopWidget, m->submiturl.name_txt,
		XmNtopOffset, TOFF_SUURLTXT,
		NULL
	);

	XtVaSetValues(m->submiturl.url_lbl,
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->submiturl.url_txt,
		XmNrightOffset, ROFF_SUURLLBL,
		XmNtopWidget, m->submiturl.url_txt,
		XmNbottomWidget, m->submiturl.url_txt,
		NULL
	);

	XtVaSetValues(m->submiturl.desc_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_SUDESCLBL,
		XmNrightPosition, RIGHT_SUDESCLBL,
		XmNtopWidget, m->submiturl.url_txt,
		XmNtopOffset, TOFF_SUDESCLBL,
		NULL
	);

	XtVaSetValues(m->submiturl.submit_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SUSUBMITBTN,
		XmNrightPosition, RIGHT_SUSUBMITBTN,
		XmNbottomOffset, BOFF_SUSUBMITBTN,
		NULL
	);

	XtVaSetValues(m->submiturl.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_SUOKBTN,
		XmNrightPosition, RIGHT_SUOKBTN,
		XmNbottomOffset, BOFF_SUOKBTN,
		NULL
	);

	XtVaSetValues(m->submiturl.submiturl_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SUSEP,
		XmNrightPosition, RIGHT_SUSEP,
		XmNbottomWidget, m->submiturl.submit_btn,
		XmNbottomOffset, BOFF_SUSEP,
		NULL
	);

	XtVaSetValues(XtParent(m->submiturl.desc_txt),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_SUDESCTXT,
		XmNrightPosition, RIGHT_SUDESCTXT,
		XmNtopWidget, m->submiturl.desc_lbl,
		XmNbottomWidget, m->submiturl.submiturl_sep,
		XmNtopOffset, TOFF_SUDESCTXT,
		XmNbottomOffset, BOFF_SUDESCTXT,
		NULL
	);
}


/*
 * geom_regionsel_force
 *	Set the geometry of the widgets in the region selector window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_regionsel_force(widgets_t *m)
{
	XtVaSetValues(m->regionsel.region_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_REGIONSELLBL,
		XmNrightPosition, RIGHT_REGIONSELLBL,
		XmNtopOffset, TOFF_REGIONSELLBL,
		NULL
	);
	
	XtVaSetValues(m->regionsel.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_REGIONOK,
		XmNrightPosition, RIGHT_REGIONOK,
		XmNbottomOffset, BOFF_REGIONOK,
		NULL
	);
	
	XtVaSetValues(m->regionsel.region_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_REGIONSEP,
		XmNrightPosition, RIGHT_REGIONSEP,
		XmNbottomWidget, m->regionsel.ok_btn,
		XmNbottomOffset, BOFF_REGIONSEP,
		NULL
	);
	
	XtVaSetValues(XtParent(m->regionsel.region_list),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_REGIONLIST,
		XmNrightPosition, RIGHT_REGIONLIST,
		XmNtopWidget, m->regionsel.region_lbl,
		XmNbottomWidget, m->regionsel.region_sep,
		XmNtopOffset, TOFF_REGIONLIST,
		XmNbottomOffset, BOFF_REGIONLIST,
		NULL
	);
}


/*
 * geom_langsel_force
 *	Set the geometry of the widgets in the lang selector window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_langsel_force(widgets_t *m)
{
	XtVaSetValues(m->langsel.lang_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_LANGSELLBL,
		XmNrightPosition, RIGHT_LANGSELLBL,
		XmNtopOffset, TOFF_LANGSELLBL,
		NULL
	);
	
	XtVaSetValues(m->langsel.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_LANGOK,
		XmNrightPosition, RIGHT_LANGOK,
		XmNbottomOffset, BOFF_LANGOK,
		NULL
	);
	
	XtVaSetValues(m->langsel.lang_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_LANGSEP,
		XmNrightPosition, RIGHT_LANGSEP,
		XmNbottomWidget, m->langsel.ok_btn,
		XmNbottomOffset, BOFF_LANGSEP,
		NULL
	);
	
	XtVaSetValues(XtParent(m->langsel.lang_list),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_LANGLIST,
		XmNrightPosition, RIGHT_LANGLIST,
		XmNtopWidget, m->langsel.lang_lbl,
		XmNbottomWidget, m->langsel.lang_sep,
		XmNtopOffset, TOFF_LANGLIST,
		XmNbottomOffset, BOFF_LANGLIST,
		NULL
	);
}


/*
 * geom_matchsel_force
 *	Set the geometry of the widgets in the match selector window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_matchsel_force(widgets_t *m)
{
	XtVaSetValues(m->matchsel.logo_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftOffset, LOFF_MATCHSELLOGO,
		XmNrightOffset, ROFF_MATCHSELLOGO,
		XmNtopOffset, TOFF_MATCHSELLOGO,
		NULL
	);

	XtVaSetValues(m->matchsel.matchsel_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_MATCHSELLBL,
		XmNrightPosition, RIGHT_MATCHSELLBL,
		XmNtopWidget, m->matchsel.logo_lbl,
		XmNtopOffset, TOFF_MATCHSELLBL,
		NULL
	);
	
	XtVaSetValues(m->matchsel.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_MATCHOK,
		XmNrightPosition, RIGHT_MATCHOK,
		XmNbottomOffset, BOFF_MATCHOK,
		NULL
	);
	
	XtVaSetValues(m->matchsel.matchsel_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_MATCHSEP,
		XmNrightPosition, RIGHT_MATCHSEP,
		XmNbottomWidget, m->matchsel.ok_btn,
		XmNbottomOffset, BOFF_MATCHSEP,
		NULL
	);
	
	XtVaSetValues(XtParent(m->matchsel.matchsel_list),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_MATCHLIST,
		XmNrightPosition, RIGHT_MATCHLIST,
		XmNtopWidget, m->matchsel.matchsel_lbl,
		XmNbottomWidget, m->matchsel.matchsel_sep,
		XmNtopOffset, TOFF_MATCHLIST,
		XmNbottomOffset, BOFF_MATCHLIST,
		NULL
	);
}


/*
 * geom_userreg_force
 *	Set the geometry of the widgets in the userreg window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_userreg_force(widgets_t *m)
{
	XtVaSetValues(m->userreg.logo_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftOffset, LOFF_URLOGO,
		XmNrightOffset, ROFF_URLOGO,
		XmNtopOffset, TOFF_URLOGO,
		NULL
	);

	XtVaSetValues(m->userreg.caption_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URCAPTIONLBL,
		XmNrightPosition, RIGHT_URCAPTIONLBL,
		XmNtopWidget, m->userreg.logo_lbl,
		XmNtopOffset, TOFF_URCAPTIONLBL,
		NULL
	);
	
	XtVaSetValues(m->userreg.handle_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URHANDLETXT,
		XmNrightPosition, RIGHT_URHANDLETXT,
		XmNtopWidget, m->userreg.caption_lbl,
		XmNtopOffset, TOFF_URHANDLETXT,
		NULL
	);

	XtVaSetValues(m->userreg.handle_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.handle_txt,
		XmNtopWidget, m->userreg.handle_txt,
		XmNbottomWidget, m->userreg.handle_txt,
		XmNleftOffset, LOFF_URHANDLELBL,
		XmNrightOffset, ROFF_URHANDLELBL,
		NULL
	);

	XtVaSetValues(m->userreg.passwd_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URPASSWDTXT,
		XmNrightPosition, RIGHT_URPASSWDTXT,
		XmNtopWidget, m->userreg.handle_txt,
		XmNtopOffset, TOFF_URPASSWDTXT,
		NULL
	);

	XtVaSetValues(m->userreg.passwd_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.passwd_txt,
		XmNtopWidget, m->userreg.passwd_txt,
		XmNbottomWidget, m->userreg.passwd_txt,
		XmNleftOffset, LOFF_URPASSWDLBL,
		XmNrightOffset, ROFF_URPASSWDLBL,
		NULL
	);

	XtVaSetValues(m->userreg.vpasswd_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URVPASSWDTXT,
		XmNrightPosition, RIGHT_URVPASSWDTXT,
		XmNtopWidget, m->userreg.passwd_txt,
		XmNtopOffset, TOFF_URVPASSWDTXT,
		NULL
	);

	XtVaSetValues(m->userreg.vpasswd_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.vpasswd_txt,
		XmNtopWidget, m->userreg.vpasswd_txt,
		XmNbottomWidget, m->userreg.vpasswd_txt,
		XmNleftOffset, LOFF_URVPASSWDLBL,
		XmNrightOffset, ROFF_URVPASSWDLBL,
		NULL
	);

	XtVaSetValues(m->userreg.hint_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URHINTTXT,
		XmNrightPosition, RIGHT_URHINTTXT,
		XmNtopWidget, m->userreg.vpasswd_txt,
		XmNtopOffset, TOFF_URHINTTXT,
		NULL
	);

	XtVaSetValues(m->userreg.gethint_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->userreg.hint_txt,
		XmNtopWidget, m->userreg.hint_txt,
		XmNbottomWidget, m->userreg.hint_txt,
		XmNleftOffset, LOFF_URGETHINTBTN,
		NULL
	);

	XtVaSetValues(m->userreg.hint_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.hint_txt,
		XmNtopWidget, m->userreg.hint_txt,
		XmNbottomWidget, m->userreg.hint_txt,
		XmNleftOffset, LOFF_URHINTLBL,
		XmNrightOffset, ROFF_URHINTLBL,
		NULL
	);

	XtVaSetValues(m->userreg.email_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_UREMAILTXT,
		XmNrightPosition, RIGHT_UREMAILTXT,
		XmNtopWidget, m->userreg.hint_txt,
		XmNtopOffset, TOFF_UREMAILTXT,
		NULL
	);

	XtVaSetValues(m->userreg.email_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.email_txt,
		XmNtopWidget, m->userreg.email_txt,
		XmNbottomWidget, m->userreg.email_txt,
		XmNleftOffset, LOFF_UREMAILLBL,
		XmNrightOffset, ROFF_UREMAILLBL,
		NULL
	);

	XtVaSetValues(m->userreg.region_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URREGIONTXT, 
		XmNrightPosition, RIGHT_URREGIONTXT,
		XmNtopWidget, m->userreg.email_txt,
		XmNtopOffset, TOFF_URREGIONTXT,
		NULL
	);

	XtVaSetValues(m->userreg.region_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.region_txt,
		XmNtopWidget, m->userreg.region_txt,
		XmNbottomWidget, m->userreg.region_txt,
		XmNleftOffset, LOFF_URREGIONLBL,
		XmNrightOffset, ROFF_URREGIONLBL,
		NULL
	);

	XtVaSetValues(m->userreg.region_chg_btn,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->userreg.region_txt,
		XmNtopWidget, m->userreg.region_txt,
		XmNbottomWidget, m->userreg.region_txt,
		XmNleftOffset, LOFF_URREGIONCHGBTN,
		NULL
	);

	XtVaSetValues(m->userreg.postal_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URPOSTALTXT,
		XmNrightPosition, RIGHT_URPOSTALTXT,
		XmNtopWidget, m->userreg.region_txt,
		XmNtopOffset, TOFF_URPOSTALTXT,
		NULL
	);

	XtVaSetValues(m->userreg.postal_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.postal_txt,
		XmNtopWidget, m->userreg.postal_txt,
		XmNbottomWidget, m->userreg.postal_txt,
		XmNleftOffset, LOFF_URPOSTALLBL,
		XmNrightOffset, ROFF_URPOSTALLBL,
		NULL
	);

	XtVaSetValues(m->userreg.age_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URAGETXT,
		XmNrightPosition, RIGHT_URAGETXT,
		XmNtopWidget, m->userreg.postal_txt,
		XmNtopOffset, TOFF_URAGETXT,
		NULL
	);

	XtVaSetValues(m->userreg.age_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->userreg.age_txt,
		XmNtopWidget, m->userreg.age_txt,
		XmNbottomWidget, m->userreg.age_txt,
		XmNleftOffset, LOFF_URAGELBL,
		XmNrightOffset, ROFF_URAGELBL,
		NULL
	);

	XtVaSetValues(m->userreg.gender_frm,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_URGENDERFRM,
		XmNtopWidget, m->userreg.postal_txt,
		XmNtopOffset, TOFF_URGENDERFRM,
		NULL
	);

	XtVaSetValues(m->userreg.gender_lbl,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, m->userreg.age_txt,
		XmNrightWidget, m->userreg.gender_frm,
		XmNtopWidget, m->userreg.age_txt,
		XmNbottomWidget, m->userreg.age_txt,
		XmNleftOffset, LOFF_URGENDERLBL,
		XmNrightOffset, ROFF_URGENDERLBL,
		NULL
	);

	XtVaSetValues(m->userreg.allowmail_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNtopWidget, m->userreg.gender_frm,
		XmNleftPosition, LEFT_URALLOWMAILBTN,
		XmNtopOffset, TOFF_URALLOWMAILBTN,
		NULL
	);

	XtVaSetValues(m->userreg.allowstats_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNtopWidget, m->userreg.allowmail_btn,
		XmNleftPosition, LEFT_URALLOWSTATSBTN,
		XmNtopOffset, TOFF_URALLOWSTATSBTN,
		NULL
	);

	XtVaSetValues(m->userreg.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_UROKBTN,
		XmNrightPosition, RIGHT_UROKBTN,
		XmNbottomOffset, BOFF_UROKBTN,
		NULL
	);

	XtVaSetValues(m->userreg.priv_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_URPRIVBTN,
		XmNrightPosition, RIGHT_URPRIVBTN,
		XmNbottomOffset, BOFF_URPRIVBTN,
		NULL
	);

	XtVaSetValues(m->userreg.cancel_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_URCANCELBTN,
		XmNrightPosition, RIGHT_URCANCELBTN,
		XmNbottomOffset, BOFF_URCANCELBTN,
		NULL
	);

	XtVaSetValues(m->userreg.userreg_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_URSEP,
		XmNrightPosition, RIGHT_URSEP,
		XmNbottomWidget, m->userreg.ok_btn,
		XmNbottomOffset, BOFF_URSEP,
		NULL
	);
}


/*
 * geom_help_force
 *	Set the geometry of the widgets in the help display window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_help_force(widgets_t *m)
{
	XtVaSetValues(m->help.topic_opt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_NONE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_TOPIC,
		XmNtopOffset, TOFF_TOPIC,
		NULL
	);

	XtVaSetValues(m->help.about_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_ABOUT,
		XmNrightPosition, RIGHT_ABOUT,
		XmNbottomOffset, BOFF_ABOUT,
		NULL
	);

	XtVaSetValues(m->help.cancel_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_HELPCANCEL,
		XmNrightPosition, RIGHT_HELPCANCEL,
		XmNbottomOffset, BOFF_HELPCANCEL,
		NULL
	);

	XtVaSetValues(m->help.help_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_HELPSEP,
		XmNrightPosition, RIGHT_HELPSEP,
		XmNbottomWidget, m->help.cancel_btn,
		XmNbottomOffset, BOFF_HELPSEP,
		NULL
	);
	
	XtVaSetValues(XtParent(m->help.help_txt),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_HELPTXT,
		XmNrightPosition, RIGHT_HELPTXT,
		XmNtopWidget, m->help.topic_opt,
		XmNtopOffset, TOFF_HELPTXT,
		XmNbottomWidget, m->help.help_sep,
		XmNbottomOffset, BOFF_HELPTXT,
		NULL
	);
}


/*
 * geom_auth_force
 *	Set the geometry of the widgets in the authorization window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
geom_auth_force(widgets_t *m)
{
	XtVaSetValues(m->auth.auth_lbl,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_AUTHLBL,
		XmNrightPosition, RIGHT_AUTHLBL,
		XmNtopOffset, TOFF_AUTHLBL,
		NULL
	);
	XtVaSetValues(m->auth.name_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_AUTHNAMETXT,
		XmNrightPosition, RIGHT_AUTHNAMETXT,
		XmNtopWidget, m->auth.auth_lbl,
		XmNtopOffset, TOFF_AUTHNAMETXT,
		NULL
	);
	XtVaSetValues(m->auth.name_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->auth.name_txt,
		XmNtopWidget, m->auth.name_txt,
		XmNbottomWidget, m->auth.name_txt,
		XmNleftOffset, LOFF_AUTHNAMELBL,
		XmNrightOffset, ROFF_AUTHNAMELBL,
		NULL
	);
	XtVaSetValues(m->auth.pass_txt,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftPosition, LEFT_AUTHPASSTXT,
		XmNrightPosition, RIGHT_AUTHPASSTXT,
		XmNtopWidget, m->auth.name_txt,
		XmNtopOffset, TOFF_AUTHPASSTXT,
		NULL
	);
	XtVaSetValues(m->auth.pass_lbl,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, m->auth.pass_txt,
		XmNtopWidget, m->auth.pass_txt,
		XmNbottomWidget, m->auth.pass_txt,
		XmNleftOffset, LOFF_AUTHPASSLBL,
		XmNrightOffset, ROFF_AUTHPASSLBL,
		NULL
	);

	XtVaSetValues(m->auth.ok_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_AUTHOK,
		XmNrightPosition, RIGHT_AUTHOK,
		XmNbottomOffset, BOFF_AUTHOK,
		NULL
	);
	XtVaSetValues(m->auth.cancel_btn,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftPosition, LEFT_AUTHCANCEL,
		XmNrightPosition, RIGHT_AUTHCANCEL,
		XmNbottomOffset, BOFF_AUTHCANCEL,
		NULL
	);
	XtVaSetValues(m->auth.auth_sep,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNleftPosition, LEFT_AUTHSEP,
		XmNrightPosition, RIGHT_AUTHSEP,
		XmNbottomWidget, m->auth.ok_btn,
		XmNbottomOffset, BOFF_AUTHSEP,
		NULL
	);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * geom_force
 *	Top level function to set the geometry of the widgets in each
 *	main and sub-window.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
void
geom_force(widgets_t *m)
{
	main_mode = app_data.main_mode;

	geom_main_force(m);
	geom_keypad_force(m);
	geom_options_force(m);
	geom_perfmon_force(m);
	geom_dbprog_force(m);
	geom_dlist_force(m);
	geom_fullname_force(m);
	geom_extd_force(m);
	geom_extt_force(m);
	geom_credits_force(m);
	geom_segments_force(m);
	geom_submiturl_force(m);
	geom_regionsel_force(m);
	geom_langsel_force(m);
	geom_matchsel_force(m);
	geom_userreg_force(m);
	geom_help_force(m);
	geom_auth_force(m);
}


/*
 * geom_main_chgmode
 *	Change the main window mode
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
void
geom_main_chgmode(widgets_t *m)
{
	if (main_mode == MAIN_NORMAL)
		main_mode = MAIN_BASIC;
	else
		main_mode = MAIN_NORMAL;

	geom_main_force(m);
}


/*
 * geom_main_getmode
 *	Return the current main window mode
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
int
geom_main_getmode(void)
{
	return (main_mode);
}

