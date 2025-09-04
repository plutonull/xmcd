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
static char *_widget_c_ident_ = "@(#)widget.c	7.114 04/02/13";
#endif

#include "common_d/appenv.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/callback.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdinfo_d/cdinfo.h"
#include "xmcd_d/dbprog.h"
#include "xmcd_d/wwwwarp.h"
#include "xmcd_d/userreg.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/geom.h"
#include "xmcd_d/help.h"
#include "xmcd_d/hotkey.h"

#include "bitmaps/mode.xbm"
#include "bitmaps/lock.xbm"
#include "bitmaps/repeat.xbm"
#include "bitmaps/shuffle.xbm"
#include "bitmaps/eject.xbm"
#include "bitmaps/quit.xbm"
#include "bitmaps/dbprog.xbm"
#include "bitmaps/world.xbm"
#include "bitmaps/options.xbm"
#include "bitmaps/time.xbm"
#include "bitmaps/ab.xbm"
#include "bitmaps/sample.xbm"
#include "bitmaps/keypad.xbm"
#include "bitmaps/playpaus.xbm"
#include "bitmaps/stop.xbm"
#include "bitmaps/prevdisc.xbm"
#include "bitmaps/nextdisc.xbm"
#include "bitmaps/prevtrk.xbm"
#include "bitmaps/nexttrk.xbm"
#include "bitmaps/previdx.xbm"
#include "bitmaps/nextidx.xbm"
#include "bitmaps/rew.xbm"
#include "bitmaps/ff.xbm"
#include "bitmaps/logo.xbm"
#include "bitmaps/xmcd.xbm"
#include "bitmaps/about.xbm"
#include "bitmaps/cddblogo.xbm"


extern appdata_t	app_data;
extern widgets_t	widgets;

STATIC XtCallbackRec	main_chkbox_cblist[] = {
	{ (XtCallbackProc) cd_checkbox,		NULL },
	{ (XtCallbackProc) NULL,		NULL },
};
STATIC XtCallbackRec	keypad_radbox_cblist[] = {
	{ (XtCallbackProc) cd_keypad_mode,	NULL },
	{ (XtCallbackProc) NULL,		NULL },
};
STATIC XtCallbackRec	options_cblist[] = {
	{ (XtCallbackProc) cd_options,		NULL },
	{ (XtCallbackProc) NULL,		NULL },
};
STATIC XtCallbackRec	dbprog_radbox_cblist[] = {
	{ (XtCallbackProc) dbprog_timedpy,	NULL },
	{ (XtCallbackProc) NULL,		NULL },
};
STATIC XtCallbackRec	gender_radbox_cblist[] = {
	{ (XtCallbackProc) userreg_gender_sel,	NULL },
	{ (XtCallbackProc) NULL,		NULL },
};
STATIC XtCallbackRec	help_cblist[] = {
	{ (XtCallbackProc) cd_help_popup,	NULL },
	{ (XtCallbackProc) NULL,		NULL },
};


/***********************
 *  internal routines  *
 ***********************/


/*
 * Action routines
 */

/*
 * focuschg
 *	Widget action routine to handle keyboard focus change events
 *	This is used to change the label color of widgets in the
 *	main window.
 */
/*ARGSUSED*/
STATIC void
focuschg(Widget w, XEvent *ev, String *args, Cardinal *num_args)
{
	Widget	action_widget;

	if ((int) *num_args != 1)
		return;	/* Error: should have one arg */

	action_widget = XtNameToWidget(widgets.main.form, args[0]);
	if (action_widget == NULL)
		return;	/* Can't find widget */

	cd_focus_chg(action_widget, NULL, ev);
}


/*
 * mainmap
 *	Widget action routine to handle the map and unmap events
 *	on the main window.  This is used to perform certain
 *	optimizations when the user iconifies the application.
 */
/*ARGSUSED*/
STATIC void
mainmap(Widget w, XEvent *ev, String *args, Cardinal *num_args)
{
	curstat_t	*s = curstat_addr();

	if (w != widgets.toplevel)
		return;

	if (ev->type == MapNotify)
		cd_icon(s, FALSE);
	else if (ev->type == UnmapNotify)
		cd_icon(s, TRUE);
}


/*
 * Widget-related functions
 */


/*
 * create_main_widgets
 *	Create all widgets in the main window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_main_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];
	curstat_t	*s = curstat_addr();
	XmString	dash,
			blank;

	dash = create_xmstring("-", NULL, XmSTRING_DEFAULT_CHARSET, FALSE),
	blank = create_xmstring("", NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	main_chkbox_cblist[0].closure = (XtPointer) s;

	/* Create form widget as container */
	m->main.form = XmCreateForm(
		m->toplevel,
		"mainForm",
		NULL,
		0
	);

#if !defined(LesstifVersion) && !defined(NO_LABELH) && XmVersion > 1001
	/* Create an instance of the LabelHack widget.  This is to give
	 * insensitive label widgets a "3D-look" label text.  Doesn't work
	 * with recent versions of LessTif so exclude that.
	 */
	(void) XtCreateWidget(
		"LabelH",
		xmLabelHackWidgetClass,
		m->main.form,
		NULL,
		0
	);
#endif

	/* Create pushbutton widget for Mode button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.mode_btn = XmCreatePushButton(
		m->main.form,
		"modeButton",
		arg,
		i
	);

	/* Create frame for check box */
	m->main.chkbox_frm = XmCreateFrame(
		m->main.form,
		"checkBoxFrame",
		NULL,
		0
	);

	/* Create check box */
	i = 0;
	XtSetArg(arg[i], XmNrowColumnType, XmWORK_AREA); i++;
	XtSetArg(arg[i], XmNnumColumns, 1); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNspacing, 0); i++;
	XtSetArg(arg[i], XmNmarginWidth, 0); i++;
#else
	XtSetArg(arg[i], XmNspacing, 2); i++;
	XtSetArg(arg[i], XmNmarginHeight, 4); i++;
#endif
	XtSetArg(arg[i], XmNorientation, XmVERTICAL); i++;
	XtSetArg(arg[i], XmNisHomogeneous, True); i++;
	XtSetArg(arg[i], XmNentryClass, xmToggleButtonWidgetClass); i++;
	XtSetArg(arg[i], XmNentryCallback, main_chkbox_cblist); i++;
	m->main.check_box = XmCreateRowColumn(
		m->main.chkbox_frm,
		"checkBox",
		arg,
		i
	);

	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNspacing, 2); i++;
#endif
	m->main.lock_btn = XmCreateToggleButton(
		m->main.check_box,
		"button_0",
		arg,
		i
	);
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNspacing, 2); i++;
#endif
	m->main.repeat_btn = XmCreateToggleButton(
		m->main.check_box,
		"button_1",
		arg,
		i
	);
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNspacing, 2); i++;
#endif
	m->main.shuffle_btn = XmCreateToggleButton(
		m->main.check_box,
		"button_2",
		arg,
		i
	);

	/* Create pushbutton widget for Eject button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.eject_btn = XmCreatePushButton(
		m->main.form,
		"ejectButton",
		arg,
		i
	);

	/* Create pushbutton widget for Quit button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.quit_btn = XmCreatePushButton(
		m->main.form,
		"quitButton",
		arg,
		i
	);

	/* Create label widget as disc indicator */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, dash); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.disc_ind = XmCreateLabel(
		m->main.form,
		"discIndicator",
		arg,
		i
	);

	/* Create label widget as track indicator */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, dash); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.track_ind = XmCreateLabel(
		m->main.form,
		"trackIndicator",
		arg,
		i
	);

	/* Create label widget as index indicator */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, dash); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.index_ind = XmCreateLabel(
		m->main.form,
		"indexIndicator",
		arg,
		i
	);

	/* Create label widget as time indicator */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, dash); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.time_ind = XmCreateLabel(
		m->main.form,
		"timeIndicator",
		arg,
		i
	);

	/* Create label widget as Repeat count indicator */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, dash); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.rptcnt_ind = XmCreateLabel(
		m->main.form,
		"repeatCountIndicator",
		arg,
		i
	);

	/* Create label widget as CDDB indicator label */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, blank); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.dbmode_ind = XmCreateLabel(
		m->main.form,
		"dbModeIndicator",
		arg,
		i
	);

	/* Create label widget as Program Mode indicator label */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, blank); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.progmode_ind = XmCreateLabel(
		m->main.form,
		"progModeIndicator",
		arg,
		i
	);

	/* Create label widget as Time Mode indicator label */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, blank); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.timemode_ind = XmCreateLabel(
		m->main.form,
		"timeModeIndicator",
		arg,
		i
	);

	/* Create label widget as Play Mode indicator label */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, blank); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.playmode_ind = XmCreateLabel(
		m->main.form,
		"playModeIndicator",
		arg,
		i
	);

	/* Create label widget as disc title indicator */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, blank); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.dtitle_ind = XmCreateLabel(
		m->main.form,
		"discTitleIndicator",
		arg,
		i
	);

	/* Create label widget as track title indicator */
	i = 0;
	XtSetArg(arg[i], XmNlabelString, blank); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.ttitle_ind = XmCreateLabel(
		m->main.form,
		"trackTitleIndicator",
		arg,
		i
	);

	/* Create pushbutton widget for CDDB/Program button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.dbprog_btn = XmCreatePushButton(
		m->main.form,
		"dbprogButton",
		arg,
		i
	);

	/* Create pushbutton widget as Options button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.options_btn = XmCreatePushButton(
		m->main.form,
		"optionsButton",
		arg,
		i
	);

	/* Create pushbutton widgets for A->B button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.ab_btn = XmCreatePushButton(
		m->main.form,
		"abButton",
		arg,
		i
	);

	/* Create pushbutton widget for Sample button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.sample_btn = XmCreatePushButton(
		m->main.form,
		"sampleButton",
		arg,
		i
	);

	/* Create pushbutton widget for Time button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.time_btn = XmCreatePushButton(
		m->main.form,
		"timeButton",
		arg,
		i
	);

	/* Create pushbutton widget for Keypad button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.keypad_btn = XmCreatePushButton(
		m->main.form,
		"keypadButton",
		arg,
		i
	);

	/* Create menu bar for wwwWarp menu button */
	i = 0;
	XtSetArg(arg[i], XmNisAligned, True); i++;
	XtSetArg(arg[i], XmNentryAlignment, XmALIGNMENT_CENTER); i++;
	XtSetArg(arg[i], XmNorientation, XmVERTICAL); i++;
	XtSetArg(arg[i], XmNmarginWidth, 0); i++;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->main.wwwwarp_bar = XmCreateMenuBar(
		m->main.form,
		"wwwWarpMenuBar",
		arg,
		i
	);

	/* Create cascade button widget for wwwWarp menu button */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_CENTER); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.wwwwarp_btn = XmCreateCascadeButton(
		m->main.wwwwarp_bar,
		"wwwWarpButton",
		arg,
		i
	);

	/* Create scale widget for level slider */
	i = 0;
	XtSetArg(arg[i], XmNshowValue, True); i++;
	XtSetArg(arg[i], XmNminimum, 0); i++;
	XtSetArg(arg[i], XmNmaximum, 100); i++;
	XtSetArg(arg[i], XmNorientation, XmHORIZONTAL); i++;
	XtSetArg(arg[i], XmNprocessingDirection, XmMAX_ON_RIGHT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.level_scale = XmCreateScale(
		m->main.form,
		"levelScale",
		arg,
		i
	);

	/* Create pushbutton widget for Play-Pause button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.playpause_btn = XmCreatePushButton(
		m->main.form,
		"playPauseButton",
		arg,
		i
	);

	/* Create pushbutton widget for Stop button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.stop_btn = XmCreatePushButton(
		m->main.form,
		"stopButton",
		arg,
		i
	);

	/* Create pushbutton widget for Prev-Disc button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.prevdisc_btn = XmCreatePushButton(
		m->main.form,
		"prevDiscButton",
		arg,
		i
	);

	/* Create pushbutton widget for Next-Disc button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.nextdisc_btn = XmCreatePushButton(
		m->main.form,
		"nextDiscButton",
		arg,
		i
	);

	/* Create pushbutton widget for Prev Track button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.prevtrk_btn = XmCreatePushButton(
		m->main.form,
		"prevTrackButton",
		arg,
		i
	);

	/* Create pushbutton widget for Next Track button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.nexttrk_btn = XmCreatePushButton(
		m->main.form,
		"nextTrackButton",
		arg,
		i
	);

	/* Create pushbutton widget for Prev Index button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.previdx_btn = XmCreatePushButton(
		m->main.form,
		"prevIndexButton",
		arg,
		i
	);

	/* Create pushbutton widget for Next Index button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.nextidx_btn = XmCreatePushButton(
		m->main.form,
		"nextIndexButton",
		arg,
		i
	);

	/* Create pushbutton widget for REW button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.rew_btn = XmCreatePushButton(
		m->main.form,
		"rewButton",
		arg,
		i
	);

	/* Create pushbutton widget for FF button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->main.ff_btn = XmCreatePushButton(
		m->main.form,
		"ffButton",
		arg,
		i
	);

	/* Manage the widgets */
	XtManageChild(m->main.form);
	XtManageChild(m->main.mode_btn);
	XtManageChild(m->main.chkbox_frm);
	XtManageChild(m->main.check_box);
	XtManageChild(m->main.lock_btn);
	XtManageChild(m->main.repeat_btn);
	XtManageChild(m->main.shuffle_btn);
	XtManageChild(m->main.eject_btn);
	XtManageChild(m->main.quit_btn);
	XtManageChild(m->main.disc_ind);
	XtManageChild(m->main.track_ind);
	XtManageChild(m->main.index_ind);
	XtManageChild(m->main.time_ind);
	XtManageChild(m->main.rptcnt_ind);
	XtManageChild(m->main.dbmode_ind);
	XtManageChild(m->main.progmode_ind);
	XtManageChild(m->main.timemode_ind);
	XtManageChild(m->main.playmode_ind);
	XtManageChild(m->main.dtitle_ind);
	XtManageChild(m->main.ttitle_ind);
	XtManageChild(m->main.dbprog_btn);
	XtManageChild(m->main.options_btn);
	XtManageChild(m->main.ab_btn);
	XtManageChild(m->main.sample_btn);
	XtManageChild(m->main.time_btn);
	XtManageChild(m->main.keypad_btn);
	XtManageChild(m->main.wwwwarp_btn);
	XtManageChild(m->main.wwwwarp_bar);
	XtManageChild(m->main.level_scale);
	XtManageChild(m->main.playpause_btn);
	XtManageChild(m->main.stop_btn);
	XtManageChild(m->main.prevdisc_btn);
	XtManageChild(m->main.nextdisc_btn);
	XtManageChild(m->main.prevtrk_btn);
	XtManageChild(m->main.nexttrk_btn);
	XtManageChild(m->main.previdx_btn);
	XtManageChild(m->main.nextidx_btn);
	XtManageChild(m->main.rew_btn);
	XtManageChild(m->main.ff_btn);

	XmStringFree(dash);
	XmStringFree(blank);
}


/*
 * create_keypad_widgets
 *	Create all widgets in the keypad window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_keypad_widgets(widgets_t *m)
{
	int		i, j;
	Arg		arg[10];
	char		btn_name[20];
	curstat_t	*s = curstat_addr();

	keypad_radbox_cblist[0].closure = (XtPointer) s;

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->keypad.form = XmCreateFormDialog(
		m->toplevel,
		"keypadForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->keypad.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as keypad label */
	m->keypad.keypad_lbl = XmCreateLabel(
		m->keypad.form,
		"keypadLabel",
		NULL,
		0
	);

	/* Create label widget as keypad indicator */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->keypad.keypad_ind = XmCreateLabel(
		m->keypad.form,
		"keypadIndicator",
		arg,
		i
	);

	/* Create frame for radio box */
	m->keypad.radio_frm = XmCreateFrame(
		m->keypad.form,
		"keypadSelectFrame",
		NULL,
		0
	);

	/* Create radio box widget as keypad mode selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNbuttonSet, 1); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, keypad_radbox_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->keypad.radio_box = XmCreateSimpleRadioBox(
		m->keypad.radio_frm,
		"keypadSelectBox",
		arg,
		i
	);
	m->keypad.disc_btn = XtNameToWidget(m->keypad.radio_box, "button_0");
	m->keypad.track_btn = XtNameToWidget(m->keypad.radio_box, "button_1");

	/* Create pushbutton widgets as number keys */
	for (j = 0; j < 10; j++) {
		(void) sprintf(btn_name, "keypadNumButton%u", j);

		i = 0;
		XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
		m->keypad.num_btn[j] = XmCreatePushButton(
			m->keypad.form,
			btn_name,
			arg,
			i
		);
	}

	/* Create pushbutton widget as clear button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->keypad.clear_btn = XmCreatePushButton(
		m->keypad.form,
		"keypadClearButton",
		arg,
		i
	);

	/* Create pushbutton widget as enter button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->keypad.enter_btn = XmCreatePushButton(
		m->keypad.form,
		"keypadEnterButton",
		arg,
		i
	);

	/* Create label widget as track warp label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->keypad.warp_lbl = XmCreateLabel(
		m->keypad.form,
		"trackWarpLabel",
		arg,
		i
	);

	/* Create scale widget for track warp slider */
	i = 0;
	XtSetArg(arg[i], XmNshowValue, False); i++;
	XtSetArg(arg[i], XmNminimum, 0); i++;
	XtSetArg(arg[i], XmNmaximum, 255); i++;
	XtSetArg(arg[i], XmNorientation, XmHORIZONTAL); i++;
	XtSetArg(arg[i], XmNprocessingDirection, XmMAX_ON_RIGHT); i++;
	XtSetArg(arg[i], XmNhighlightOnEnter, True); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->keypad.warp_scale = XmCreateScale(
		m->keypad.form,
		"trackWarpScale",
		arg,
		i
	);

	/* Create separator bar widget */
	m->keypad.keypad_sep = XmCreateSeparator(
		m->keypad.form,
		"keypadSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as cancel button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->keypad.cancel_btn = XmCreatePushButton(
		m->keypad.form,
		"keypadCancelButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->keypad.keypad_lbl);
	XtManageChild(m->keypad.keypad_ind);
	XtManageChild(m->keypad.radio_frm);
	XtManageChild(m->keypad.radio_box);
	for (i = 0; i < 10; i++)
		XtManageChild(m->keypad.num_btn[i]);
	XtManageChild(m->keypad.clear_btn);
	XtManageChild(m->keypad.enter_btn);
	XtManageChild(m->keypad.warp_lbl);
	XtManageChild(m->keypad.warp_scale);
	XtManageChild(m->keypad.keypad_sep);
	XtManageChild(m->keypad.cancel_btn);
}


/*
 * create_options_widgets
 *	Create all widgets in the options window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_options_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];
	curstat_t	*s = curstat_addr();

	options_cblist[0].closure = (XtPointer) s;

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->options.form = XmCreateFormDialog(
		m->toplevel,
		"optionsForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->options.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as category label */
	m->options.categ_lbl = XmCreateLabel(
		m->options.form,
		"categoryLabel",
		NULL,
		0
	);

	/* Create list widget as category list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.categ_list = XmCreateList(
		m->options.form,
		"categoryList",
		arg,
		i
	);

	/* Create vertical separator bar widget */
	i = 0;
	XtSetArg(arg[i], XmNorientation, XmVERTICAL); i++;
	m->options.categ_sep = XmCreateSeparator(
		m->options.form,
		"categorySeparator",
		arg,
		i
	);

	/* Create label widget as play mode label */
	m->options.mode_lbl = XmCreateLabel(
		m->options.form,
		"modeLabel",
		NULL,
		0
	);

	/* Create frame for play mode radio box */
	m->options.mode_chkbox_frm = XmCreateFrame(
		m->options.form,
		"modeCheckBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as play mode selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 4); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 0); i++;
	XtSetArg(arg[i], XmNmarginHeight, 2); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_chkbox = XmCreateSimpleCheckBox(
		m->options.mode_chkbox_frm,
		"modeCheckBox",
		arg,
		i
	);
	m->options.mode_std_btn =
		XtNameToWidget(m->options.mode_chkbox, "button_0");
	m->options.mode_cdda_btn =
		XtNameToWidget(m->options.mode_chkbox, "button_1");
	m->options.mode_file_btn =
		XtNameToWidget(m->options.mode_chkbox, "button_2");
	m->options.mode_pipe_btn =
		XtNameToWidget(m->options.mode_chkbox, "button_3");

	/* Create toggle button for jitter correction */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_jitter_btn = XmCreateToggleButton(
		m->options.form,
		"jitterCorrectionButton",
		arg,
		i
	);

	/* Create toggle button for file per track */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_trkfile_btn = XmCreateToggleButton(
		m->options.form,
		"filePerTrackButton",
		arg,
		i
	);

	/* Create toggle button for underscore substitution */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_subst_btn = XmCreateToggleButton(
		m->options.form,
		"underscoreSubstitutionButton",
		arg,
		i
	);

	/* Create pulldown menu widget for file format selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_menu = XmCreatePulldownMenu(
		m->options.form,
		"modeFileFormatPulldownMenu",
		arg,
		i
	);

	/* Create label widget for file format menu title */
	m->options.mode_fmt_menu_lbl = XmCreateLabel(
		m->options.mode_fmt_menu,
		"modeFileFormatMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.mode_fmt_menu_sep = XmCreateSeparator(
		m->options.mode_fmt_menu,
		"modeFileFormatMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for RAW menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_raw_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_RAW)->name,
		arg,
		i
	);

	/* Create pushbutton widget for AU menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_au_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_AU)->name,
		arg,
		i
	);

	/* Create pushbutton widget for WAV menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_wav_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_WAV)->name,
		arg,
		i
	);

	/* Create pushbutton widget for AIFF menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_aiff_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_AIFF)->name,
		arg,
		i
	);

	/* Create pushbutton widget for AIFF-C menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_aifc_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_AIFC)->name,
		arg,
		i
	);

	/* Create pushbutton widget for MP3 menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_mp3_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_MP3)->name,
		arg,
		i
	);

	/* Create pushbutton widget for Ogg Vorbis menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_ogg_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_OGG)->name,
		arg,
		i
	);

	/* Create pushbutton widget for FLAC menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_flac_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_FLAC)->name,
		arg,
		i
	);

	/* Create pushbutton widget for AAC menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_aac_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_AAC)->name,
		arg,
		i
	);

	/* Create pushbutton widget for MP4 menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode_fmt_mp4_btn = XmCreatePushButton(
		m->options.mode_fmt_menu,
		cdda_filefmt(FILEFMT_MP4)->name,
		arg,
		i
	);

	/* Create option menu widget for file format selector */
	i = 0;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNsubMenuId, m->options.mode_fmt_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_fmt_opt = XmCreateOptionMenu(
		m->options.form,
		"modeFileFormatOptionMenu",
		arg,
		i
	);

	/* Create pushbutton widget as CDDA performance monitor button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_perfmon_btn = XmCreatePushButton(
		m->options.form,
		"performanceMonitorButton",
		arg,
		i
	);

	/* Create label widget as file path label */
	m->options.mode_path_lbl = XmCreateLabel(
		m->options.form,
		"modeFilePathLabel",
		NULL,
		0
	);

	/* Create text widget for file path text */
	i = 0;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_path_txt = XmCreateText(
		m->options.form,
		"modeFilePathText",
		arg,
		i
	);

	/* Create pushbutton widget as file path expand button */
	i = 0;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_pathexp_btn = XmCreatePushButton(
		m->options.form,
		"modeFilePathExpandButton",
		arg,
		i
	);

	/* Create label widget as program path label */
	m->options.mode_prog_lbl = XmCreateLabel(
		m->options.form,
		"modeProgramPathLabel",
		NULL,
		0
	);

	/* Create text widget for program path text */
	i = 0;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mode_prog_txt = XmCreateText(
		m->options.form,
		"modeProgramPathText",
		arg,
		i
	);

	/* Create pulldown menu widget for encoding method selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.method_menu = XmCreatePulldownMenu(
		m->options.form,
		"encodeMethodPulldownMenu",
		arg,
		i
	);

	/* Create label widget for encoding method menu title */
	m->options.method_menu_lbl = XmCreateLabel(
		m->options.method_menu,
		"encodeMethodMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.method_menu_sep = XmCreateSeparator(
		m->options.method_menu,
		"encodeMethodMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for CBR menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode0_btn = XmCreatePushButton(
		m->options.method_menu,
		"mode0Button",
		arg,
		i
	);

	/* Create pushbutton widget for VBR menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode1_btn = XmCreatePushButton(
		m->options.method_menu,
		"mode1Button",
		arg,
		i
	);

	/* Create pushbutton widget for VBR-2 menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode2_btn = XmCreatePushButton(
		m->options.method_menu,
		"mode2Button",
		arg,
		i
	);

	/* Create pushbutton widget for ABR menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.mode3_btn = XmCreatePushButton(
		m->options.method_menu,
		"mode3Button",
		arg,
		i
	);

	/* Create option menu widget for encode method selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.method_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.method_opt = XmCreateOptionMenu(
		m->options.form,
		"encodeMethodOptionMenu",
		arg,
		i
	);

	/* Create label widget as quality factor label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.qualval_lbl = XmCreateLabel(
		m->options.form,
		"qualityFactorLabel",
		arg,
		i
	);

	/* Create label widget as quality factor label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.qualval1_lbl = XmCreateLabel(
		m->options.form,
		"qualityFactorLabel1",
		arg,
		i
	);

	/* Create label widget as quality factor label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.qualval2_lbl = XmCreateLabel(
		m->options.form,
		"qualityFactorLabel2",
		arg,
		i
	);

	/* Create scale widget for quality factor slider */
	i = 0;
	XtSetArg(arg[i], XmNshowValue, False); i++;
	XtSetArg(arg[i], XmNminimum, 1); i++;
	XtSetArg(arg[i], XmNmaximum, 10); i++;
	XtSetArg(arg[i], XmNorientation, XmHORIZONTAL); i++;
	XtSetArg(arg[i], XmNprocessingDirection, XmMAX_ON_RIGHT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.qualval_scl = XmCreateScale(
		m->options.form,
		"qualityFactorScale",
		arg,
		i
	);

	/* Create pulldown menu widget for bitrate selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.bitrate_menu = XmCreatePulldownMenu(
		m->options.form,
		"bitratePulldownMenu",
		arg,
		i
	);

	/* Create label widget for bitrate menu title */
	m->options.bitrate_menu_lbl = XmCreateLabel(
		m->options.bitrate_menu,
		"bitrateMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.bitrate_menu_sep = XmCreateSeparator(
		m->options.bitrate_menu,
		"bitrateMenuSeparator",
		arg,
		i
	);

	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.bitrate_def_btn = XmCreatePushButton(
		m->options.bitrate_menu,
		"bitrateDefaultButton",
		arg,
		i
	);

	/* Create option menu widget for bitrate selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.bitrate_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.bitrate_opt = XmCreateOptionMenu(
		m->options.form,
		"bitrateOptionMenu",
		arg,
		i
	);

	/* Create pulldown menu widget for min bitrate selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.minbrate_menu = XmCreatePulldownMenu(
		m->options.form,
		"minBitratePulldownMenu",
		arg,
		i
	);

	/* Create label widget for min bitrate menu title */
	m->options.minbrate_menu_lbl = XmCreateLabel(
		m->options.minbrate_menu,
		"minBitrateMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.minbrate_menu_sep = XmCreateSeparator(
		m->options.minbrate_menu,
		"minBitrateMenuSeparator",
		arg,
		i
	);

	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.minbrate_def_btn = XmCreatePushButton(
		m->options.minbrate_menu,
		"minBitrateDefaultButton",
		arg,
		i
	);

	/* Create option menu widget for min bitrate selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.minbrate_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.minbrate_opt = XmCreateOptionMenu(
		m->options.form,
		"minBitrateOptionMenu",
		arg,
		i
	);

	/* Create pulldown menu widget for max bitrate selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.maxbrate_menu = XmCreatePulldownMenu(
		m->options.form,
		"maxBitratePulldownMenu",
		arg,
		i
	);

	/* Create label widget for max bitrate menu title */
	m->options.maxbrate_menu_lbl = XmCreateLabel(
		m->options.maxbrate_menu,
		"maxBitrateMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.maxbrate_menu_sep = XmCreateSeparator(
		m->options.maxbrate_menu,
		"maxBitrateMenuSeparator",
		arg,
		i
	);

	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.maxbrate_def_btn = XmCreatePushButton(
		m->options.maxbrate_menu,
		"maxBitrateDefaultButton",
		arg,
		i
	);

	/* Create option menu widget for bitrate selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.maxbrate_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.maxbrate_opt = XmCreateOptionMenu(
		m->options.form,
		"maxBitrateOptionMenu",
		arg,
		i
	);

	/* Create pulldown menu widget for channel mode selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.chmode_menu = XmCreatePulldownMenu(
		m->options.form,
		"channelModePulldownMenu",
		arg,
		i
	);

	/* Create label widget for compression algorithm title */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.compalgo_lbl = XmCreateLabel(
		m->options.form,
		"compressionAlgorithmLabel",
		arg,
		i
	);

	/* Create label widget for compression algorithm "fastest" title */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.compalgo1_lbl = XmCreateLabel(
		m->options.form,
		"compressionAlgorithmFastestLabel",
		arg,
		i
	);

	/* Create label widget for compression algorithm "quality" title */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.compalgo2_lbl = XmCreateLabel(
		m->options.form,
		"compressionAlgorithmBestQualityLabel",
		arg,
		i
	);

	/* Create scale widget for compression algorithm slider */
	i = 0;
	XtSetArg(arg[i], XmNshowValue, False); i++;
	XtSetArg(arg[i], XmNminimum, 1); i++;
	XtSetArg(arg[i], XmNmaximum, 10); i++;
	XtSetArg(arg[i], XmNorientation, XmHORIZONTAL); i++;
	XtSetArg(arg[i], XmNprocessingDirection, XmMAX_ON_RIGHT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.compalgo_scl = XmCreateScale(
		m->options.form,
		"compressionAlgorithmScale",
		arg,
		i
	);

	/* Create label widget for channel mode menu title */
	m->options.chmode_menu_lbl = XmCreateLabel(
		m->options.chmode_menu,
		"channelModeMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.chmode_menu_sep = XmCreateSeparator(
		m->options.chmode_menu,
		"channelModeMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for stereo menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.chmode_stereo_btn = XmCreatePushButton(
		m->options.chmode_menu,
		"channelModeStereoButton",
		arg,
		i
	);

	/* Create pushbutton widget for joint-stereo menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.chmode_jstereo_btn = XmCreatePushButton(
		m->options.chmode_menu,
		"channelModeJointStereoButton",
		arg,
		i
	);

	/* Create pushbutton widget for forced msstereo menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.chmode_forcems_btn = XmCreatePushButton(
		m->options.chmode_menu,
		"channelModeForceMidSideButton",
		arg,
		i
	);

	/* Create pushbutton widget for mono menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.chmode_mono_btn = XmCreatePushButton(
		m->options.chmode_menu,
		"channelModeMonoButton",
		arg,
		i
	);

	/* Create option menu widget for channel mode selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.chmode_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.chmode_opt = XmCreateOptionMenu(
		m->options.form,
		"channelModeOptionMenu",
		arg,
		i
	);

	/* Create separator bar widget */
	m->options.encode_sep = XmCreateSeparator(
		m->options.form,
		"encodeSeparator",
		NULL,
		0
	);

	/* Create label widget for filters title */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.filters_lbl = XmCreateLabel(
		m->options.form,
		"filtersLabel",
		arg,
		i
	);

	/* Create label widget for freq title */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.freq_lbl = XmCreateLabel(
		m->options.form,
		"frequencyLabel",
		arg,
		i
	);

	/* Create label widget for width title */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.width_lbl = XmCreateLabel(
		m->options.form,
		"widthLabel",
		arg,
		i
	);

	/* Create pulldown menu widget for lowpass filter selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.lp_menu = XmCreatePulldownMenu(
		m->options.form,
		"lowpassPulldownMenu",
		arg,
		i
	);

	/* Create label widget for lowpass menu title */
	m->options.lp_menu_lbl = XmCreateLabel(
		m->options.lp_menu,
		"lowpassMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.lp_menu_sep = XmCreateSeparator(
		m->options.lp_menu,
		"lowpassMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for lowpass off menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.lp_off_btn = XmCreatePushButton(
		m->options.lp_menu,
		"lowpassOffButton",
		arg,
		i
	);

	/* Create pushbutton widget for lowpass auto menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.lp_auto_btn = XmCreatePushButton(
		m->options.lp_menu,
		"lowpassAutoButton",
		arg,
		i
	);

	/* Create pushbutton widget for lowpass manual menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.lp_manual_btn = XmCreatePushButton(
		m->options.lp_menu,
		"lowpassManualButton",
		arg,
		i
	);

	/* Create option menu widget for lowpass filter selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.lp_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.lp_opt = XmCreateOptionMenu(
		m->options.form,
		"lowpassOptionMenu",
		arg,
		i
	);

	/* Create text widget as lowpass frequency text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.lp_freq_txt = XmCreateText(
		m->options.form,
		"lowpassFrequencyText",
		arg,
		i
	);

	/* Create text widget as lowpass width text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.lp_width_txt = XmCreateText(
		m->options.form,
		"lowpassWidthText",
		arg,
		i
	);

	/* Create pulldown menu widget for highpass filter selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.hp_menu = XmCreatePulldownMenu(
		m->options.form,
		"highpassPulldownMenu",
		arg,
		i
	);

	/* Create label widget for highpass menu title */
	m->options.hp_menu_lbl = XmCreateLabel(
		m->options.hp_menu,
		"highpassMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.hp_menu_sep = XmCreateSeparator(
		m->options.hp_menu,
		"highpassMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for highpass off menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.hp_off_btn = XmCreatePushButton(
		m->options.hp_menu,
		"highpassOffButton",
		arg,
		i
	);

	/* Create pushbutton widget for highpass auto menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.hp_auto_btn = XmCreatePushButton(
		m->options.hp_menu,
		"highpassAutoButton",
		arg,
		i
	);

	/* Create pushbutton widget for highpass manual menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.hp_manual_btn = XmCreatePushButton(
		m->options.hp_menu,
		"highpassManualButton",
		arg,
		i
	);

	/* Create option menu widget for highpass filter selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.hp_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.hp_opt = XmCreateOptionMenu(
		m->options.form,
		"highpassOptionMenu",
		arg,
		i
	);

	/* Create text widget as highpass frequency text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.hp_freq_txt = XmCreateText(
		m->options.form,
		"highpassFrequencyText",
		arg,
		i
	);

	/* Create text widget as highpass width text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.hp_width_txt = XmCreateText(
		m->options.form,
		"highpassWidthText",
		arg,
		i
	);

	/* Create separator bar widget */
	m->options.encode_sep2 = XmCreateSeparator(
		m->options.form,
		"encodeSeparator2",
		NULL,
		0
	);

	/* Create toggle button for copyright button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.copyrt_btn = XmCreateToggleButton(
		m->options.form,
		"copyrightButton",
		arg,
		i
	);

	/* Create toggle button for original button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.orig_btn = XmCreateToggleButton(
		m->options.form,
		"originalButton",
		arg,
		i
	);

	/* Create toggle button for nores button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.nores_btn = XmCreateToggleButton(
		m->options.form,
		"noReservoirButton",
		arg,
		i
	);

	/* Create toggle button for checksum button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.chksum_btn = XmCreateToggleButton(
		m->options.form,
		"checksumButton",
		arg,
		i
	);

	/* Create toggle button for the strict ISO compliance button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.iso_btn = XmCreateToggleButton(
		m->options.form,
		"strictISOButton",
		arg,
		i
	);

	/* Create toggle button for Add CD info tag */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.addtag_btn = XmCreateToggleButton(
		m->options.form,
		"addTagButton",
		arg,
		i
	);

	/* Create pulldown menu widget for ID3tag version selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.id3tag_ver_menu = XmCreatePulldownMenu(
		m->options.form,
		"id3tagVersionPulldownMenu",
		arg,
		i
	);

	/* Create label widget for ID3tag version title */
	m->options.id3tag_ver_menu_lbl = XmCreateLabel(
		m->options.id3tag_ver_menu,
		"id3tagVersionMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->options.id3tag_ver_menu_sep = XmCreateSeparator(
		m->options.id3tag_ver_menu,
		"id3tagVersionMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for ID3tag v1 menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.id3tag_v1_btn = XmCreatePushButton(
		m->options.id3tag_ver_menu,
		"id3tagVersion1Button",
		arg,
		i
	);

	/* Create pushbutton widget for ID3tag v2 menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.id3tag_v2_btn = XmCreatePushButton(
		m->options.id3tag_ver_menu,
		"id3tagVersion2Button",
		arg,
		i
	);

	/* Create pushbutton widget for ID3tag both versions menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->options.id3tag_both_btn = XmCreatePushButton(
		m->options.id3tag_ver_menu,
		"id3tagBothVersionsButton",
		arg,
		i
	);

	/* Create option menu widget for ID3tag version selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->options.id3tag_ver_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.id3tag_ver_opt = XmCreateOptionMenu(
		m->options.form,
		"id3tagVersionOptionMenu",
		arg,
		i
	);

	/* Create label widget as LAME options label */
	m->options.lame_lbl = XmCreateLabel(
		m->options.form,
		"lameLabel",
		NULL,
		0
	);

	/* Create label widget as LAME options mode label */
	m->options.lame_mode_lbl = XmCreateLabel(
		m->options.form,
		"lameModeLabel",
		NULL,
		0
	);

	/* Create frame for LAME options mode radio box */
	m->options.lame_radbox_frm = XmCreateFrame(
		m->options.form,
		"lameRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as LAME options mode radio box */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 4); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.lame_radbox = XmCreateSimpleRadioBox(
		m->options.lame_radbox_frm,
		"lameRadioBox",
		arg,
		i
	);
	m->options.lame_disable_btn =
		XtNameToWidget(m->options.lame_radbox, "button_0");
	m->options.lame_insert_btn =
		XtNameToWidget(m->options.lame_radbox, "button_1");
	m->options.lame_append_btn =
		XtNameToWidget(m->options.lame_radbox, "button_2");
	m->options.lame_replace_btn =
		XtNameToWidget(m->options.lame_radbox, "button_3");

	/* Create label widget as LAME options label */
	m->options.lame_opts_lbl = XmCreateLabel(
		m->options.form,
		"lameOptionsLabel",
		NULL,
		0
	);

	/* Create text widget as LAME options text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.lame_opts_txt = XmCreateText(
		m->options.form,
		"lameOptionsText",
		arg,
		i
	);

	/* Create label widget as sched options label */
	m->options.sched_lbl = XmCreateLabel(
		m->options.form,
		"schedLabel",
		NULL,
		0
	);

	/* Create label widget as read sched label */
	m->options.sched_rd_lbl = XmCreateLabel(
		m->options.form,
		"schedReadLabel",
		NULL,
		0
	);

	/* Create frame for read sched radio box */
	m->options.sched_rd_radbox_frm = XmCreateFrame(
		m->options.form,
		"schedReadRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as read sched radio box */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.sched_rd_radbox = XmCreateSimpleRadioBox(
		m->options.sched_rd_radbox_frm,
		"schedReadRadioBox",
		arg,
		i
	);
	m->options.sched_rd_norm_btn =
		XtNameToWidget(m->options.sched_rd_radbox, "button_0");
	m->options.sched_rd_hipri_btn =
		XtNameToWidget(m->options.sched_rd_radbox, "button_1");

	/* Create label widget as write sched label */
	m->options.sched_wr_lbl = XmCreateLabel(
		m->options.form,
		"schedWriteLabel",
		NULL,
		0
	);

	/* Create frame for write sched radio box */
	m->options.sched_wr_radbox_frm = XmCreateFrame(
		m->options.form,
		"schedWriteRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as write sched radio box */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.sched_wr_radbox = XmCreateSimpleRadioBox(
		m->options.sched_wr_radbox_frm,
		"schedWriteRadioBox",
		arg,
		i
	);
	m->options.sched_wr_norm_btn =
		XtNameToWidget(m->options.sched_wr_radbox, "button_0");
	m->options.sched_wr_hipri_btn =
		XtNameToWidget(m->options.sched_wr_radbox, "button_1");

	/* Create label widget as heartbeat timeout label */
	m->options.hb_tout_lbl = XmCreateLabel(
		m->options.form,
		"cddaHeartbeatTimeoutLabel",
		NULL,
		0
	);

	/* Create text widget as heartbeat timeout text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.hb_tout_txt = XmCreateText(
		m->options.form,
		"cddaHeartbeatTimeoutText",
		arg,
		i
	);

	/* Create label widget as load options label */
	m->options.load_lbl = XmCreateLabel(
		m->options.form,
		"onLoadLabel",
		NULL,
		0
	);

	/* Create frame for load options checkbox */
	m->options.load_chkbox_frm = XmCreateFrame(
		m->options.form,
		"onLoadCheckBoxFrame",
		NULL,
		0
	);

	/* Create check box widget as load options checkbox */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 4); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.load_chkbox = XmCreateSimpleCheckBox(
		m->options.load_chkbox_frm,
		"onLoadCheckBox",
		arg,
		i
	);
	m->options.load_none_btn =
		XtNameToWidget(m->options.load_chkbox, "button_0");
	m->options.load_spdn_btn =
		XtNameToWidget(m->options.load_chkbox, "button_1");
	m->options.load_play_btn =
		XtNameToWidget(m->options.load_chkbox, "button_2");
	m->options.load_lock_btn =
		XtNameToWidget(m->options.load_chkbox, "button_3");

	/* Create label widget as exit options label */
	m->options.exit_lbl = XmCreateLabel(
		m->options.form,
		"onExitLabel",
		NULL,
		0
	);

	/* Create frame for exit options radio box */
	m->options.exit_radbox_frm = XmCreateFrame(
		m->options.form,
		"onExitRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as exit options selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 3); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.exit_radbox = XmCreateSimpleRadioBox(
		m->options.exit_radbox_frm,
		"onExitRadioBox",
		arg,
		i
	);
	m->options.exit_none_btn =
		XtNameToWidget(m->options.exit_radbox, "button_0");
	m->options.exit_stop_btn =
		XtNameToWidget(m->options.exit_radbox, "button_1");
	m->options.exit_eject_btn =
		XtNameToWidget(m->options.exit_radbox, "button_2");

	/* Create label widget as done options label */
	m->options.done_lbl = XmCreateLabel(
		m->options.form,
		"onDoneLabel",
		NULL,
		0
	);

	/* Create frame for done options radio box */
	m->options.done_chkbox_frm = XmCreateFrame(
		m->options.form,
		"onDoneCheckBoxFrame",
		NULL,
		0
	);

	/* Create check box widget as done options checkbox */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.done_chkbox = XmCreateSimpleCheckBox(
		m->options.done_chkbox_frm,
		"onDoneCheckBox",
		arg,
		i
	);
	m->options.done_eject_btn =
		XtNameToWidget(m->options.done_chkbox, "button_0");
	m->options.done_exit_btn =
		XtNameToWidget(m->options.done_chkbox, "button_1");

	/* Create label widget as eject options label */
	m->options.eject_lbl = XmCreateLabel(
		m->options.form,
		"onEjectLabel",
		NULL,
		0
	);

	/* Create frame for done options radio box */
	m->options.eject_chkbox_frm = XmCreateFrame(
		m->options.form,
		"onEjectCheckBoxFrame",
		NULL,
		0
	);

	/* Create check box widget as eject options checkbox */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 1); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.eject_chkbox = XmCreateSimpleCheckBox(
		m->options.eject_chkbox_frm,
		"onEjectCheckBox",
		arg,
		i
	);
	m->options.eject_exit_btn =
		XtNameToWidget(m->options.eject_chkbox, "button_0");

	/* Create label widget as changer options label */
	m->options.chg_lbl = XmCreateLabel(
		m->options.form,
		"changerLabel",
		NULL,
		0
	);

	/* Create frame for changer options radio box */
	m->options.chg_chkbox_frm = XmCreateFrame(
		m->options.form,
		"changerCheckBoxFrame",
		NULL,
		0
	);

	/* Create check box widget as changer options checkbox */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.chg_chkbox = XmCreateSimpleCheckBox(
		m->options.chg_chkbox_frm,
		"changerCheckBox",
		arg,
		i
	);
	m->options.chg_multiplay_btn =
		XtNameToWidget(m->options.chg_chkbox, "button_0");
	m->options.chg_reverse_btn =
		XtNameToWidget(m->options.chg_chkbox, "button_1");

	/* Create label widget as channel route options label */
	m->options.chroute_lbl = XmCreateLabel(
		m->options.form,
		"channelRouteLabel",
		NULL,
		0
	);

	/* Create frame for channel route options radio box */
	m->options.chroute_radbox_frm = XmCreateFrame(
		m->options.form,
		"channelRouteRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as channel route options selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 5); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.chroute_radbox = XmCreateSimpleRadioBox(
		m->options.chroute_radbox_frm,
		"channelRouteRadioBox",
		arg,
		i
	);
	m->options.chroute_stereo_btn =
		XtNameToWidget(m->options.chroute_radbox, "button_0");
	m->options.chroute_rev_btn =
		XtNameToWidget(m->options.chroute_radbox, "button_1");
	m->options.chroute_left_btn =
		XtNameToWidget(m->options.chroute_radbox, "button_2");
	m->options.chroute_right_btn =
		XtNameToWidget(m->options.chroute_radbox, "button_3");
	m->options.chroute_mono_btn =
		XtNameToWidget(m->options.chroute_radbox, "button_4");

	/* Create label widget as output port selector label */
	m->options.outport_lbl = XmCreateLabel(
		m->options.form,
		"outputPortLabel",
		NULL,
		0
	);

	/* Create frame for output port selector check box */
	m->options.outport_chkbox_frm = XmCreateFrame(
		m->options.form,
		"outputPortCheckBoxFrame",
		NULL,
		0
	);

	/* Create check box widget as output port selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 3); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.outport_chkbox = XmCreateSimpleCheckBox(
		m->options.outport_chkbox_frm,
		"outputPortCheckBox",
		arg,
		i
	);
	m->options.outport_spkr_btn =
		XtNameToWidget(m->options.outport_chkbox, "button_0");
	m->options.outport_phone_btn =
		XtNameToWidget(m->options.outport_chkbox, "button_1");
	m->options.outport_line_btn =
		XtNameToWidget(m->options.outport_chkbox, "button_2");

	/* Create label widget as vol taper options label */
	m->options.vol_lbl = XmCreateLabel(
		m->options.form,
		"volTaperLabel",
		NULL,
		0
	);

	/* Create frame for vol taper options radio box */
	m->options.vol_radbox_frm = XmCreateFrame(
		m->options.form,
		"volTaperRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as vol taper options selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 3); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.vol_radbox = XmCreateSimpleRadioBox(
		m->options.vol_radbox_frm,
		"volTaperRadioBox",
		arg,
		i
	);
	m->options.vol_linear_btn =
		XtNameToWidget(m->options.vol_radbox, "button_0");
	m->options.vol_square_btn =
		XtNameToWidget(m->options.vol_radbox, "button_1");
	m->options.vol_invsqr_btn =
		XtNameToWidget(m->options.vol_radbox, "button_2");

	/* Create label widget as balance control label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.bal_lbl = XmCreateLabel(
		m->options.form,
		"balanceLabel",
		arg,
		i
	);

	/* Create label widget as balance control L label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.ball_lbl = XmCreateLabel(
		m->options.form,
		"balanceLeftLabel",
		arg,
		i
	);

	/* Create label widget as balance control R label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.balr_lbl = XmCreateLabel(
		m->options.form,
		"balanceRightLabel",
		arg,
		i
	);

	/* Create scale widget for balance control slider */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	XtSetArg(arg[i], XmNshowValue, False); i++;
	XtSetArg(arg[i], XmNminimum, -50); i++;
	XtSetArg(arg[i], XmNmaximum, 50); i++;
	XtSetArg(arg[i], XmNvalue, 0); i++;
	XtSetArg(arg[i], XmNorientation, XmHORIZONTAL); i++;
	XtSetArg(arg[i], XmNprocessingDirection, XmMAX_ON_RIGHT); i++;
	XtSetArg(arg[i], XmNhighlightOnEnter, True); i++;
	m->options.bal_scale = XmCreateScale(
		m->options.form,
		"balanceScale",
		arg,
		i
	);

	/* Create pushbutton widget as balance center button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.balctr_btn = XmCreatePushButton(
		m->options.form,
		"balanceCenterButton",
		arg,
		i
	);

	/* Create label widget as CDDA attenuator label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.cdda_att_lbl = XmCreateLabel(
		m->options.form,
		"cddaAttenuatorLabel",
		arg,
		i
	);

	/* Create scale widget for CDDA attenuator slider */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	XtSetArg(arg[i], XmNshowValue, True); i++;
	XtSetArg(arg[i], XmNminimum, 0); i++;
	XtSetArg(arg[i], XmNmaximum, 100); i++;
	XtSetArg(arg[i], XmNvalue, 100); i++;
	XtSetArg(arg[i], XmNorientation, XmHORIZONTAL); i++;
	XtSetArg(arg[i], XmNprocessingDirection, XmMAX_ON_RIGHT); i++;
	XtSetArg(arg[i], XmNhighlightOnEnter, True); i++;
	m->options.cdda_att_scale = XmCreateScale(
		m->options.form,
		"cddaAttenuatorScale",
		arg,
		i
	);

	/* Create arrow button widget as CDDA fade-out button */
	i = 0;
	XtSetArg(arg[i], XmNtraversalOn, True); i++;
	XtSetArg(arg[i], XmNarrowDirection, XmARROW_LEFT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.cdda_fadeout_btn = XmCreateArrowButton(
		m->options.form,
		"cddaFadeOutButton",
		arg,
		i
	);

	/* Create arrow button widget as CDDA fade-in button */
	i = 0;
	XtSetArg(arg[i], XmNtraversalOn, True); i++;
	XtSetArg(arg[i], XmNarrowDirection, XmARROW_RIGHT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.cdda_fadein_btn = XmCreateArrowButton(
		m->options.form,
		"cddaFadeInButton",
		arg,
		i
	);

	/* Create label widget as CDDA fade-out label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.cdda_fadeout_lbl = XmCreateLabel(
		m->options.form,
		"cddaFadeOutButtonLabel",
		arg,
		i
	);

	/* Create label widget as CDDA fade-in label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.cdda_fadein_lbl = XmCreateLabel(
		m->options.form,
		"cddaFadeInButtonLabel",
		arg,
		i
	);

	/* Create label widget as CDDB options label */
	m->options.cddb_lbl = XmCreateLabel(
		m->options.form,
		"cddbOptionsLabel",
		NULL,
		0
	);

	/* Create label widget as CDDB music browser label */
	m->options.mbrowser_lbl = XmCreateLabel(
		m->options.form,
		"musicBrowserLabel",
		NULL,
		0
	);

	/* Create frame for CDDB music browser radio box */
	m->options.mbrowser_frm = XmCreateFrame(
		m->options.form,
		"musicBrowserRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as CDDB music browser selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.mbrowser_radbox = XmCreateSimpleRadioBox(
		m->options.mbrowser_frm,
		"musicBrowserRadioBox",
		arg,
		i
	);
	m->options.autobr_btn =
		XtNameToWidget(m->options.mbrowser_radbox, "button_0");
	m->options.manbr_btn =
		XtNameToWidget(m->options.mbrowser_radbox, "button_1");

	/* Create label widget as Lookup priority label */
	m->options.lookup_lbl = XmCreateLabel(
		m->options.form,
		"lookupLabel",
		NULL,
		0
	);

	/* Create frame for Lookup priority radio box */
	m->options.lookup_frm = XmCreateFrame(
		m->options.form,
		"lookupRadioBoxFrame",
		NULL,
		0
	);

	/* Create radio box widget as Lookup priority selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNbuttonSet, 0); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, options_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.lookup_radbox = XmCreateSimpleRadioBox(
		m->options.lookup_frm,
		"lookupRadioBox",
		arg,
		i
	);
	m->options.cddb_pri_btn =
		XtNameToWidget(m->options.lookup_radbox, "button_0");
	m->options.cdtext_pri_btn =
		XtNameToWidget(m->options.lookup_radbox, "button_1");

	/* Create separator bar widget */
	m->options.cddb_sep1 = XmCreateSeparator(
		m->options.form,
		"cddbSeparator1",
		NULL,
		0
	);

	/* Create label widget as service timeout label */
	m->options.serv_tout_lbl = XmCreateLabel(
		m->options.form,
		"cddbServiceTimeoutLabel",
		NULL,
		0
	);

	/* Create text widget as service timeout text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.serv_tout_txt = XmCreateText(
		m->options.form,
		"cddbServiceTimeoutText",
		arg,
		i
	);

	/* Create label widget as cache timeout label */
	m->options.cache_tout_lbl = XmCreateLabel(
		m->options.form,
		"cddbCacheTimeoutLabel",
		NULL,
		0
	);

	/* Create text widget as cache timeout text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.cache_tout_txt = XmCreateText(
		m->options.form,
		"cddbCacheTimeoutText",
		arg,
		i
	);

	/* Create separator bar widget */
	m->options.cddb_sep2 = XmCreateSeparator(
		m->options.form,
		"cddbSeparator2",
		NULL,
		0
	);

	/* Create toggle button for CDDB use proxy button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.use_proxy_btn = XmCreateToggleButton(
		m->options.form,
		"cddbUseProxyButton",
		arg,
		i
	);

	/* Create label widget as proxy server label */
	m->options.proxy_srvr_lbl = XmCreateLabel(
		m->options.form,
		"cddbProxyServerLabel",
		NULL,
		0
	);

	/* Create text widget as proxy server text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.proxy_srvr_txt = XmCreateText(
		m->options.form,
		"cddbProxyServerText",
		arg,
		i
	);

	/* Create label widget as proxy port label */
	m->options.proxy_port_lbl = XmCreateLabel(
		m->options.form,
		"cddbProxyPortLabel",
		NULL,
		0
	);

	/* Create text widget as proxy port text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.proxy_port_txt = XmCreateText(
		m->options.form,
		"cddbProxyPortText",
		arg,
		i
	);

	/* Create toggle button for proxy authorization button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.proxy_auth_btn = XmCreateToggleButton(
		m->options.form,
		"cddbProxyAuthButton",
		arg,
		i
	);

	/* Create separator bar widget */
	m->options.options_sep = XmCreateSeparator(
		m->options.form,
		"optionsSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as reset button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.reset_btn = XmCreatePushButton(
		m->options.form,
		"resetButton",
		arg,
		i
	);

	/* Create pushbutton widget as save button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.save_btn = XmCreatePushButton(
		m->options.form,
		"saveButton",
		arg,
		i
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->options.ok_btn = XmCreatePushButton(
		m->options.form,
		"okButton",
		arg,
		i
	);

	/* Manage the widgets (except the form).  Also, individual
	 * options widgets are managed as needed depending on which
	 * category is selected by the user.
	 */
	XtManageChild(m->options.categ_lbl);
	XtManageChild(m->options.categ_list);
	XtManageChild(m->options.categ_sep);

	XtManageChild(m->options.mode_fmt_menu_lbl);
	XtManageChild(m->options.mode_fmt_menu_sep);
	XtManageChild(m->options.mode_fmt_raw_btn);
	XtManageChild(m->options.mode_fmt_au_btn);
	XtManageChild(m->options.mode_fmt_wav_btn);
	XtManageChild(m->options.mode_fmt_aiff_btn);
	XtManageChild(m->options.mode_fmt_aifc_btn);
	XtManageChild(m->options.mode_fmt_mp3_btn);
	XtManageChild(m->options.mode_fmt_ogg_btn);
	XtManageChild(m->options.mode_fmt_flac_btn);
	XtManageChild(m->options.mode_fmt_aac_btn);
	XtManageChild(m->options.mode_fmt_mp4_btn);

	XtManageChild(m->options.method_menu_lbl);
	XtManageChild(m->options.method_menu_sep);
	XtManageChild(m->options.mode0_btn);
	XtManageChild(m->options.mode1_btn);
	XtManageChild(m->options.mode2_btn);
	XtManageChild(m->options.mode3_btn);

	XtManageChild(m->options.bitrate_menu_lbl);
	XtManageChild(m->options.bitrate_menu_sep);
	XtManageChild(m->options.bitrate_def_btn);
	XtManageChild(m->options.minbrate_menu_lbl);
	XtManageChild(m->options.minbrate_menu_sep);
	XtManageChild(m->options.minbrate_def_btn);
	XtManageChild(m->options.maxbrate_menu_lbl);
	XtManageChild(m->options.maxbrate_menu_sep);
	XtManageChild(m->options.maxbrate_def_btn);

	XtManageChild(m->options.chmode_menu_lbl);
	XtManageChild(m->options.chmode_menu_sep);
	XtManageChild(m->options.chmode_stereo_btn);
	XtManageChild(m->options.chmode_jstereo_btn);
	XtManageChild(m->options.chmode_forcems_btn);
	XtManageChild(m->options.chmode_mono_btn);

	XtManageChild(m->options.lp_menu_lbl);
	XtManageChild(m->options.lp_menu_sep);
	XtManageChild(m->options.lp_auto_btn);
	XtManageChild(m->options.lp_manual_btn);
	XtManageChild(m->options.lp_off_btn);
	XtManageChild(m->options.hp_menu_lbl);
	XtManageChild(m->options.hp_menu_sep);
	XtManageChild(m->options.hp_auto_btn);
	XtManageChild(m->options.hp_manual_btn);
	XtManageChild(m->options.hp_off_btn);

	XtManageChild(m->options.id3tag_ver_menu_lbl);
	XtManageChild(m->options.id3tag_ver_menu_sep);
	XtManageChild(m->options.id3tag_v1_btn);
	XtManageChild(m->options.id3tag_v2_btn);
	XtManageChild(m->options.id3tag_both_btn);

	XtManageChild(m->options.mode_chkbox);
	XtManageChild(m->options.lame_radbox);
	XtManageChild(m->options.sched_rd_radbox);
	XtManageChild(m->options.sched_wr_radbox);
	XtManageChild(m->options.load_chkbox);
	XtManageChild(m->options.exit_radbox);
	XtManageChild(m->options.done_chkbox);
	XtManageChild(m->options.eject_chkbox);
	XtManageChild(m->options.chg_chkbox);
	XtManageChild(m->options.chroute_radbox);
	XtManageChild(m->options.outport_chkbox);
	XtManageChild(m->options.vol_radbox);
	XtManageChild(m->options.mbrowser_radbox);
	XtManageChild(m->options.lookup_radbox);

	XtManageChild(m->options.options_sep);
	XtManageChild(m->options.reset_btn);
	XtManageChild(m->options.save_btn);
	XtManageChild(m->options.ok_btn);
}


/*
 * create_perfmon_widgets
 *	Create all widgets in the CDDA performance monitor window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_perfmon_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->perfmon.form = XmCreateFormDialog(
		m->toplevel,
		"perfmonForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->perfmon.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as speed factor label */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.speed_lbl = XmCreateLabel(
		m->perfmon.form,
		"speedFactorLabel",
		arg,
		i
	);

	/* Create label widget as speed factor indicator */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.speed_ind = XmCreateLabel(
		m->perfmon.form,
		"speedFactorIndicator",
		arg,
		i
	);

	/* Create label widget as frames transferred label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.frames_lbl = XmCreateLabel(
		m->perfmon.form,
		"framesXferLabel",
		arg,
		i
	);

	/* Create label widget as frames transferred indicator */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.frames_ind = XmCreateLabel(
		m->perfmon.form,
		"framesXferIndicator",
		arg,
		i
	);

	/* Create label widget as frames per sec label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.fps_lbl = XmCreateLabel(
		m->perfmon.form,
		"framesPerSecondLabel",
		arg,
		i
	);

	/* Create label widget as frames per sec indicator */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.fps_ind = XmCreateLabel(
		m->perfmon.form,
		"framesPerSecondIndicator",
		arg,
		i
	);

	/* Create label widget as time to completion label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.ttc_lbl = XmCreateLabel(
		m->perfmon.form,
		"timeToCompletionLabel",
		arg,
		i
	);

	/* Create label widget as time to completion indicator */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.ttc_ind = XmCreateLabel(
		m->perfmon.form,
		"timeToCompletionIndicator",
		arg,
		i
	);

	/* Create separator bar widget */
	m->perfmon.perfmon_sep = XmCreateSeparator(
		m->perfmon.form,
		"perfmonSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as cancel button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->perfmon.cancel_btn = XmCreatePushButton(
		m->perfmon.form,
		"perfmonCancelButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->perfmon.speed_lbl);
	XtManageChild(m->perfmon.speed_ind);
	XtManageChild(m->perfmon.frames_lbl);
	XtManageChild(m->perfmon.frames_ind);
	XtManageChild(m->perfmon.fps_lbl);
	XtManageChild(m->perfmon.fps_ind);
	XtManageChild(m->perfmon.ttc_lbl);
	XtManageChild(m->perfmon.ttc_ind);
	XtManageChild(m->perfmon.perfmon_sep);
	XtManageChild(m->perfmon.cancel_btn);
}
/*
 * create_dbprog_widgets
 *	Create all widgets in the CD information/program window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_dbprog_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];
	curstat_t	*s = curstat_addr();

	dbprog_radbox_cblist[0].closure = (XtPointer) s;

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->dbprog.form = XmCreateFormDialog(
		m->toplevel,
		"dbprogForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->dbprog.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as total time indicator */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.tottime_ind = XmCreateLabel(
		m->dbprog.form,
		"totalTimeIndicator",
		arg,
		i
	);

	/* Create toggle button for CDDB remote offline */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNheight, 18); i++;
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.inetoffln_btn = XmCreateToggleButton(
		m->dbprog.form,
		"internetOfflineButton",
		arg,
		i
	);

	/* Create label widget as logo */
	m->dbprog.logo_lbl = XmCreateLabel(
		m->dbprog.form,
		"logoLabel",
		NULL,
		0
	);

	/* Create pushbutton widget as disc list window popup button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.dlist_btn = XmCreatePushButton(
		m->dbprog.form,
		"discListButton",
		arg,
		i
	);

	/* Create label widget as disc artist display/editor label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.artist_lbl = XmCreateLabel(
		m->dbprog.form,
		"discArtistLabel",
		arg,
		i
	);

	/* Create text widget as disc artist display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.artist_txt = XmCreateText(
		m->dbprog.form,
		"discArtistText",
		arg,
		i
	);

	/* Create label widget as disc title display/editor label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.title_lbl = XmCreateLabel(
		m->dbprog.form,
		"discTitleLabel",
		arg,
		i
	);

	/* Create text widget as disc title display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.title_txt = XmCreateText(
		m->dbprog.form,
		"discTitleText",
		arg,
		i
	);

	/* Create pushbutton widget as artist ext descr popup button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.fullname_btn = XmCreatePushButton(
		m->dbprog.form,
		"artistFullNameButton",
		arg,
		i
	);

	/* Create label widget as disc label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.extd_lbl = XmCreateLabel(
		m->dbprog.form,
		"discLabel",
		arg,
		i
	);

	/* Create pushbutton widget as disc details popup button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.extd_btn = XmCreatePushButton(
		m->dbprog.form,
		"extDiscInfoButton",
		arg,
		i
	);

	/* Create pushbutton widget as disc credits button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.dcredits_btn = XmCreatePushButton(
		m->dbprog.form,
		"discCreditsButton",
		arg,
		i
	);

	/* Create pushbutton widget as Segments button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.segments_btn = XmCreatePushButton(
		m->dbprog.form,
		"segmentsButton",
		arg,
		i
	);

	/* Create separator bar widget */
	m->dbprog.dbprog_sep1 = XmCreateSeparator(
		m->dbprog.form,
		"dbprogSeparator1",
		NULL,
		0
	);

	/* Create label widget as track list label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.trklist_lbl = XmCreateLabel(
		m->dbprog.form,
		"trackListLabel",
		arg,
		i
	);

	/* Create scrolled window widget for track list */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	m->dbprog.trklist_sw = XmCreateScrolledWindow(
		m->dbprog.form,
		"trackListScrolledWindow",
		arg,
		i
	);

	/* Create list widget as track list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.trk_list = XmCreateList(
		m->dbprog.trklist_sw,
		"trackList",
		arg,
		i
	);

	/* Create label widget as time mode selector label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.radio_lbl = XmCreateLabel(
		m->dbprog.form,
		"timeSelectLabel",
		arg,
		i
	);

	/* Create frame for radio box */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.radio_frm = XmCreateFrame(
		m->dbprog.form,
		"timeSelectFrame",
		arg,
		i
	);

	/* Create radio box widget as time mode selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 2); i++;
	XtSetArg(arg[i], XmNbuttonSet, 1); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 2); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, dbprog_radbox_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.radio_box = XmCreateSimpleRadioBox(
		m->dbprog.radio_frm,
		"timeSelectBox",
		arg,
		i
	);
	m->dbprog.tottime_btn = XtNameToWidget(m->dbprog.radio_box, "button_0");
	m->dbprog.trktime_btn = XtNameToWidget(m->dbprog.radio_box, "button_1");

	XtVaSetValues(m->dbprog.tottime_btn, XmNmarginHeight, 0, NULL);
	XtVaSetValues(m->dbprog.trktime_btn, XmNmarginHeight, 0, NULL);

	/* Create label widget as track title display/editor label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.ttitle_lbl = XmCreateLabel(
		m->dbprog.form,
		"trackTitleLabel",
		arg,
		i
	);

	/* Create text widget as track title display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.ttitle_txt = XmCreateText(
		m->dbprog.form,
		"trackTitleText",
		arg,
		i
	);

	/* Create pushbutton widget as track title apply button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.apply_btn = XmCreatePushButton(
		m->dbprog.form,
		"trackTitleApplyButton",
		arg,
		i
	);

	/* Create label widget as track label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.extt_lbl = XmCreateLabel(
		m->dbprog.form,
		"trackLabel",
		arg,
		i
	);

	/* Create pushbutton widget as track title ext descr popup button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.extt_btn = XmCreatePushButton(
		m->dbprog.form,
		"extTrackInfoButton",
		arg,
		i
	);

	/* Create pushbutton widget as track credits button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.tcredits_btn = XmCreatePushButton(
		m->dbprog.form,
		"trackCreditsButton",
		arg,
		i
	);

	/* Create label widget as program pushbuttons label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.pgm_lbl = XmCreateLabel(
		m->dbprog.form,
		"programLabel",
		arg,
		i
	);

	/* Create pushbutton widget as Add PGM button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.addpgm_btn = XmCreatePushButton(
		m->dbprog.form,
		"addProgramButton",
		arg,
		i
	);

	/* Create pushbutton widget as Clear PGM button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.clrpgm_btn = XmCreatePushButton(
		m->dbprog.form,
		"clearProgramButton",
		arg,
		i
	);

	/* Create pushbutton widget as Save PGM button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.savepgm_btn = XmCreatePushButton(
		m->dbprog.form,
		"saveProgramButton",
		arg,
		i
	);

	/* Create label widget as program sequence display/editor label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dbprog.pgmseq_lbl = XmCreateLabel(
		m->dbprog.form,
		"programSequenceLabel",
		arg,
		i
	);

	/* Create text widget as program sequence display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.pgmseq_txt = XmCreateText(
		m->dbprog.form,
		"programSequenceText",
		arg,
		i
	);

	/* Create pushbutton widget as Registration button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.userreg_btn = XmCreatePushButton(
		m->dbprog.form,
		"userRegistrationButton",
		arg,
		i
	);

	/* Create separator bar widget */
	m->dbprog.dbprog_sep2 = XmCreateSeparator(
		m->dbprog.form,
		"dbprogSeparator2",
		NULL,
		0
	);

	/* Create pushbutton widget as CDDB Submit button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.submit_btn = XmCreatePushButton(
		m->dbprog.form,
		"submitButton",
		arg,
		i
	);

	/* Create pushbutton widget as CDDB flush cache button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.flush_btn = XmCreatePushButton(
		m->dbprog.form,
		"flushButton",
		arg,
		i
	);

	/* Create pushbutton widget as Reload button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.reload_btn = XmCreatePushButton(
		m->dbprog.form,
		"reloadButton",
		arg,
		i
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbprog.ok_btn = XmCreatePushButton(
		m->dbprog.form,
		"dbprogOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->dbprog.logo_lbl);
	XtManageChild(m->dbprog.tottime_ind);
	XtManageChild(m->dbprog.inetoffln_btn);
	XtManageChild(m->dbprog.dlist_btn);
	XtManageChild(m->dbprog.artist_lbl);
	XtManageChild(m->dbprog.artist_txt);
	XtManageChild(m->dbprog.title_lbl);
	XtManageChild(m->dbprog.title_txt);
	XtManageChild(m->dbprog.fullname_btn);
	XtManageChild(m->dbprog.extd_lbl);
	XtManageChild(m->dbprog.extd_btn);
	XtManageChild(m->dbprog.dcredits_btn);
	XtManageChild(m->dbprog.segments_btn);
	XtManageChild(m->dbprog.dbprog_sep1);
	XtManageChild(m->dbprog.trklist_lbl);
	XtManageChild(m->dbprog.trklist_sw);
	XtManageChild(m->dbprog.trk_list);
	XtManageChild(m->dbprog.radio_lbl);
	XtManageChild(m->dbprog.radio_frm);
	XtManageChild(m->dbprog.radio_box);
	XtManageChild(m->dbprog.ttitle_lbl);
	XtManageChild(m->dbprog.ttitle_txt);
	XtManageChild(m->dbprog.apply_btn);
	XtManageChild(m->dbprog.extt_lbl);
	XtManageChild(m->dbprog.extt_btn);
	XtManageChild(m->dbprog.tcredits_btn);
	XtManageChild(m->dbprog.pgm_lbl);
	XtManageChild(m->dbprog.addpgm_btn);
	XtManageChild(m->dbprog.clrpgm_btn);
	XtManageChild(m->dbprog.savepgm_btn);
	XtManageChild(m->dbprog.pgmseq_lbl);
	XtManageChild(m->dbprog.pgmseq_txt);
	XtManageChild(m->dbprog.userreg_btn);
	XtManageChild(m->dbprog.dbprog_sep2);
	XtManageChild(m->dbprog.submit_btn);
	XtManageChild(m->dbprog.flush_btn);
	XtManageChild(m->dbprog.reload_btn);
	XtManageChild(m->dbprog.ok_btn);
}


/*
 * create_dlist_widgets
 *	Create all widgets in the disc list window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_dlist_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];

	/* Disc list window */

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->dlist.form = XmCreateFormDialog(
		m->toplevel,
		"discListForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->dlist.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create pulldown menu widget for disc list selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->dlist.type_menu = XmCreatePulldownMenu(
		m->dlist.form,
		"discListTypePulldownMenu",
		arg,
		i
	);

	/* Create label widget for disc list type menu title */
	m->dlist.menu_lbl = XmCreateLabel(
		m->dlist.type_menu,
		"discListTypeLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->dlist.menu_sep = XmCreateSeparator(
		m->dlist.type_menu,
		"discListTypeSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for history menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->dlist.hist_btn = XmCreatePushButton(
		m->dlist.type_menu,
		"discListHistoryButton",
		arg,
		i
	);

	/* Create pushbutton widget for changer slots menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->dlist.chgr_btn = XmCreatePushButton(
		m->dlist.type_menu,
		"discListChangerButton",
		arg,
		i
	);

	/* Create option menu widget for topic selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->dlist.type_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.type_opt = XmCreateOptionMenu(
		m->dlist.form,
		"discListTypeOptionMenu",
		arg,
		i
	);

	/* Create label widget as disc list label */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->dlist.disclist_lbl = XmCreateLabel(
		m->dlist.form,
		"discListLabel",
		arg,
		i
	);

	/* Create scrolled window widget for disc list */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	m->dlist.disclist_sw = XmCreateScrolledWindow(
		m->dlist.form,
		"discListScrolledWindow",
		arg,
		i
	);

	/* Create list widget as disc list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.disc_list = XmCreateList(
		m->dlist.disclist_sw,
		"discList",
		arg,
		i
	);

	/* Create pushbutton widget as Show button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.show_btn = XmCreatePushButton(
		m->dlist.form,
		"discListShowButton",
		arg,
		i
	);

	/* Create pushbutton widget as Change to button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.goto_btn = XmCreatePushButton(
		m->dlist.form,
		"discListChangeToButton",
		arg,
		i
	);

	/* Create pushbutton widget as Delete button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.del_btn = XmCreatePushButton(
		m->dlist.form,
		"discListDeleteButton",
		arg,
		i
	);

	/* Create pushbutton widget as Delete All button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.delall_btn = XmCreatePushButton(
		m->dlist.form,
		"discListDeleteAllButton",
		arg,
		i
	);

	/* Create pushbutton widget as Rescan button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.rescan_btn = XmCreatePushButton(
		m->dlist.form,
		"discListRescanButton",
		arg,
		i
	);

	/* Create separator bar widget */
	m->dlist.dlist_sep = XmCreateSeparator(
		m->dlist.form,
		"discListSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as Cancel button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dlist.cancel_btn = XmCreatePushButton(
		m->dlist.form,
		"discListCancelButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->dlist.menu_lbl);
	XtManageChild(m->dlist.menu_sep);
	XtManageChild(m->dlist.hist_btn);
	XtManageChild(m->dlist.chgr_btn);
	XtManageChild(m->dlist.type_opt);
	XtManageChild(m->dlist.disclist_lbl);
	XtManageChild(m->dlist.disclist_sw);
	XtManageChild(m->dlist.disc_list);
	XtManageChild(m->dlist.show_btn);
	XtManageChild(m->dlist.goto_btn);
	XtManageChild(m->dlist.rescan_btn);
	XtManageChild(m->dlist.del_btn);
	XtManageChild(m->dlist.delall_btn);
	XtManageChild(m->dlist.dlist_sep);
	XtManageChild(m->dlist.cancel_btn);
}


/*
 * create_fullname_widgets
 *	Create all widgets in the full name window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_fullname_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];

	/* Fullname editor window */

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->fullname.form = XmCreateFormDialog(
		m->toplevel,
		"fullNameForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->fullname.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as head label */
	m->fullname.head_lbl = XmCreateLabel(
		m->fullname.form,
		"fullNameHeadLabel",
		NULL,
		0
	);

	/* Create label widget as display name label */
	m->fullname.dispname_lbl = XmCreateLabel(
		m->fullname.form,
		"fullNameDisplayNameLabel",
		NULL,
		0
	);

	/* Create text widget as display name display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->fullname.dispname_txt = XmCreateText(
		m->fullname.form,
		"fullNameDisplayNameText",
		arg,
		i
	);

	/* Create toggle button for the auto-gen button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->fullname.autogen_btn = XmCreateToggleButton(
		m->fullname.form,
		"fullNameAutoGenButton",
		arg,
		i
	);

	/* Create label widget as last name label */
	m->fullname.lastname_lbl = XmCreateLabel(
		m->fullname.form,
		"fullNameLastNameLabel",
		NULL,
		0
	);

	/* Create text widget as last name display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->fullname.lastname_txt = XmCreateText(
		m->fullname.form,
		"fullNameLastNameText",
		arg,
		i
	);

	/* Create label widget as first name label */
	m->fullname.firstname_lbl = XmCreateLabel(
		m->fullname.form,
		"fullNameFirstNameLabel",
		NULL,
		0
	);

	/* Create text widget as first name display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->fullname.firstname_txt = XmCreateText(
		m->fullname.form,
		"fullNameFirstNameText",
		arg,
		i
	);

	/* Create toggle button for name "the" button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->fullname.the_btn = XmCreateToggleButton(
		m->fullname.form,
		"fullNameTheButton",
		arg,
		i
	);

	/* Create text widget as name the display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->fullname.the_txt = XmCreateText(
		m->fullname.form,
		"fullNameTheText",
		arg,
		i
	);

	/* Create separator bar widget */
	m->fullname.fullname_sep = XmCreateSeparator(
		m->fullname.form,
		"fullNameSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->fullname.ok_btn = XmCreatePushButton(
		m->fullname.form,
		"fullNameOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->fullname.head_lbl);
	XtManageChild(m->fullname.dispname_lbl);
	XtManageChild(m->fullname.dispname_txt);
	XtManageChild(m->fullname.autogen_btn);
	XtManageChild(m->fullname.lastname_lbl);
	XtManageChild(m->fullname.lastname_txt);
	XtManageChild(m->fullname.firstname_lbl);
	XtManageChild(m->fullname.firstname_txt);
	XtManageChild(m->fullname.the_btn);
	XtManageChild(m->fullname.the_txt);
	XtManageChild(m->fullname.fullname_sep);
	XtManageChild(m->fullname.ok_btn);
}


/*
 * create_extd_widgets
 *	Create all widgets in the disc details window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_extd_widgets(widgets_t *m)
{
	int		i,
			n;
	Arg		arg[10];

	/* Disc details window */

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->dbextd.form = XmCreateFormDialog(
		m->toplevel,
		"extDiscInfoForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->dbextd.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as disc number label */
	m->dbextd.discno_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscNumberLabel",
		NULL,
		0
	);

	/* Create label widget as disc artist/title label */
	m->dbextd.disc_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoLabel",
		NULL,
		0
	);

	/* Create separator bar widget */
	m->dbextd.dbextd_sep = XmCreateSeparator(
		m->dbextd.form,
		"extDiscInfoSeparator",
		NULL,
		0
	);

	/* Create label widget as sort title label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->dbextd.sorttitle_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoSortTitleLabel",
		arg,
		i
	);

	/* Create text widget as sort title display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.sorttitle_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoSortTitleText",
		arg,
		i
	);

	/* Create toggle button for sort title "the" button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.the_btn = XmCreateToggleButton(
		m->dbextd.form,
		"extDiscInfoTheButton",
		arg,
		i
	);

	/* Create text widget as name the display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.the_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoTheText",
		arg,
		i
	);

	/* Create label widget as year label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->dbextd.year_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoYearLabel",
		arg,
		i
	);

	/* Create text widget as year display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.year_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoYearText",
		arg,
		i
	);

	/* Create label widget as Label label */
	m->dbextd.label_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoLabelLabel",
		NULL,
		0
	);

	/* Create text widget as Label display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.label_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoLabelText",
		arg,
		i
	);

	/* Create toggle button for compilation button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.comp_btn = XmCreateToggleButton(
		m->dbextd.form,
		"extDiscInfoCompilationButton",
		arg,
		i
	);

	for (n = 0; n < 2; n++) {
		char	name[STR_BUF_SZ];

		/* Create pulldown menu widget for Genre selector */
		(void) sprintf(name, "extDiscInfoGenre%dMenu", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextd.genre_menu[n] = XmCreatePulldownMenu(
			m->dbextd.form,
			name,
			arg,
			i
		);

		/* Create option menu widget for Genre selector */
		(void) sprintf(name, "extDiscInfoGenre%dOptionMenu", n);
		i = 0;
		XtSetArg(arg[i], XmNsubMenuId, m->dbextd.genre_menu[n]); i++;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
		XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
		m->dbextd.genre_opt[n] = XmCreateOptionMenu(
			m->dbextd.form,
			name,
			arg,
			i
		);

		/* Create label widget as Genre menu label */
		(void) sprintf(name, "extDiscInfoGenre%dLabel", n);
		m->dbextd.genre_lbl[n] = XmCreateLabel(
			m->dbextd.genre_menu[n],
			name,
			NULL,
			0
		);

		/* Create separator bar widget */
		(void) sprintf(name, "extDiscInfoGenre%dSeparator", n);
		i = 0;
		XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
		m->dbextd.genre_sep[n] = XmCreateSeparator(
			m->dbextd.genre_menu[n],
			name,
			arg,
			i
		);

		/* Create pushbutton widget as Genre "none" button */
		(void) sprintf(name, "extDiscInfoGenre%dNoneButton", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextd.genre_none_btn[n] = XmCreatePushButton(
			m->dbextd.genre_menu[n],
			name,
			arg,
			i
		);

		/* Create pulldown menu widget for Subgenre selector */
		(void) sprintf(name, "extDiscInfoSubgenre%dMenu", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextd.subgenre_menu[n] = XmCreatePulldownMenu(
			m->dbextd.form,
			name,
			arg,
			i
		);

		/* Create option menu widget for Subgenre selector */
		(void) sprintf(name, "extDiscInfoSubgenre%dOptionMenu", n);
		i = 0;
		XtSetArg(arg[i], XmNsubMenuId,
			 m->dbextd.subgenre_menu[n]); i++;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
		XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
		m->dbextd.subgenre_opt[n] = XmCreateOptionMenu(
			m->dbextd.form,
			name,
			arg,
			i
		);

		/* Create label widget as Subgenre menu label */
		(void) sprintf(name, "extDiscInfoSubgenre%dLabel", n);
		m->dbextd.subgenre_lbl[n] = XmCreateLabel(
			m->dbextd.subgenre_menu[n],
			name,
			NULL,
			0
		);

		/* Create separator bar widget */
		(void) sprintf(name, "extDiscInfoSubgenre%dSeparator", n);
		i = 0;
		XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
		m->dbextd.subgenre_sep[n] = XmCreateSeparator(
			m->dbextd.subgenre_menu[n],
			name,
			arg,
			i
		);

		/* Create pushbutton widget as Subgenre "none" button */
		(void) sprintf(name, "extDiscInfoSubgenre%dNoneButton", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextd.subgenre_none_btn[n] = XmCreatePushButton(
			m->dbextd.subgenre_menu[n],
			name,
			arg,
			i
		);
	}

	/* Create label widget as Disc number in set label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->dbextd.dnum_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoDiscNumberInSetLabel",
		arg,
		i
	);

	/* Create text widget as Disc number in set display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.dnum_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoDiscNumberInSetText",
		arg,
		i
	);

	/* Create label widget as "of" label */
	m->dbextd.of_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoOfLabel",
		NULL,
		0
	);

	/* Create text widget as Total number in set display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.tnum_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoTotalNumberInSetText",
		arg,
		i
	);

	/* Create label widget for Region label */
	m->dbextd.region_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoRegionLabel",
		NULL,
		0
	);

	/* Create text widget as Region display */
	i = 0;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.region_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoRegionText",
		arg,
		i
	);

	/* Create pushbutton widget for Region change button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.region_chg_btn = XmCreatePushButton(
		m->dbextd.form,
		"extDiscInfoRegionChangeButton",
		arg,
		i
	);

	/* Create label widget for Language label */
	m->dbextd.lang_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoLanguageLabel",
		NULL,
		0
	);

	/* Create text widget as Language display */
	i = 0;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.lang_txt = XmCreateText(
		m->dbextd.form,
		"extDiscInfoLanguageText",
		arg,
		i
	);

	/* Create pushbutton widget for Language change button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.lang_chg_btn = XmCreatePushButton(
		m->dbextd.form,
		"extDiscInfoLanguageChangeButton",
		arg,
		i
	);

	/* Create separator bar widget */
	m->dbextd.dbextd_sep2 = XmCreateSeparator(
		m->dbextd.form,
		"extDiscInfoSeparator2",
		NULL,
		0
	);

	/* Create label widget as Notes label */
	m->dbextd.notes_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoNotesLabel",
		NULL,
		0
	);

	/* Create text widget as Disc Notes editor/viewer */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.notes_txt = XmCreateScrolledText(
		m->dbextd.form,
		"extDiscInfoNotesText",
		arg,
		i
	);

	/* Create label widget as Xmcd disc ID label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.discid_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoDiscIdLabel",
		arg,
		i
	);
	/* Create label widget as Xmcd disc ID indicator label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.discid_ind = XmCreateLabel(
		m->dbextd.form,
		"",
		arg,
		i
	);

	/* Create label widget as Media catalog number label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.mcn_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoMcnLabel",
		arg,
		i
	);
	/* Create label widget as Media catalog number indicator label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.mcn_ind = XmCreateLabel(
		m->dbextd.form,
		"",
		arg,
		i
	);

	/* Create label widget as Revision label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.rev_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoRevisionLabel",
		arg,
		i
	);
	/* Create label widget as Revision indicator label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.rev_ind = XmCreateLabel(
		m->dbextd.form,
		"",
		arg,
		i
	);

	/* Create label widget as Certifier label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.cert_lbl = XmCreateLabel(
		m->dbextd.form,
		"extDiscInfoCertifierLabel",
		arg,
		i
	);

	/* Create label widget as Certifier indicator label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.cert_ind = XmCreateLabel(
		m->dbextd.form,
		"",
		arg,
		i
	);

	/* Create separator bar widget */
	m->dbextd.dbextd_sep3 = XmCreateSeparator(
		m->dbextd.form,
		"extDiscInfoSeparator3",
		NULL,
		0
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextd.ok_btn = XmCreatePushButton(
		m->dbextd.form,
		"extDiscInfoOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->dbextd.discno_lbl);
	XtManageChild(m->dbextd.disc_lbl);
	XtManageChild(m->dbextd.dbextd_sep);
	XtManageChild(m->dbextd.the_txt);
	XtManageChild(m->dbextd.the_btn);
	XtManageChild(m->dbextd.sorttitle_txt);
	XtManageChild(m->dbextd.sorttitle_lbl);
	XtManageChild(m->dbextd.year_lbl);
	XtManageChild(m->dbextd.year_txt);
	XtManageChild(m->dbextd.label_lbl);
	XtManageChild(m->dbextd.label_txt);
	XtManageChild(m->dbextd.comp_btn);
	for (n = 0; n < 2; n++) {
		XtManageChild(m->dbextd.genre_opt[n]);
		XtManageChild(m->dbextd.genre_lbl[n]);
		XtManageChild(m->dbextd.genre_sep[n]);
		XtManageChild(m->dbextd.genre_none_btn[n]);
		XtManageChild(m->dbextd.subgenre_opt[n]);
		XtManageChild(m->dbextd.subgenre_lbl[n]);
		XtManageChild(m->dbextd.subgenre_sep[n]);
		XtManageChild(m->dbextd.subgenre_none_btn[n]);
	}
	XtManageChild(m->dbextd.dnum_lbl);
	XtManageChild(m->dbextd.dnum_txt);
	XtManageChild(m->dbextd.of_lbl);
	XtManageChild(m->dbextd.tnum_txt);
	XtManageChild(m->dbextd.region_lbl);
	XtManageChild(m->dbextd.region_txt);
	XtManageChild(m->dbextd.region_chg_btn);
	XtManageChild(m->dbextd.lang_lbl);
	XtManageChild(m->dbextd.lang_txt);
	XtManageChild(m->dbextd.lang_chg_btn);
	XtManageChild(m->dbextd.dbextd_sep2);
	XtManageChild(m->dbextd.notes_lbl);
	XtManageChild(m->dbextd.notes_txt);
	XtManageChild(m->dbextd.discid_lbl);
	XtManageChild(m->dbextd.discid_ind);
	XtManageChild(m->dbextd.mcn_lbl);
	XtManageChild(m->dbextd.mcn_ind);
	XtManageChild(m->dbextd.rev_lbl);
	XtManageChild(m->dbextd.rev_ind);
	XtManageChild(m->dbextd.cert_lbl);
	XtManageChild(m->dbextd.cert_ind);
	XtManageChild(m->dbextd.dbextd_sep3);
	XtManageChild(m->dbextd.ok_btn);
}


/*
 * create_extt_widgets
 *	Create all widgets in the track details window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_extt_widgets(widgets_t *m)
{
	int		i,
			n;
	Arg		arg[10];

	/* Track details window */

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->dbextt.form = XmCreateFormDialog(
		m->toplevel,
		"extTrackInfoForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->dbextt.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create toggle button for auto-track */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.autotrk_btn = XmCreateToggleButton(
		m->dbextt.form,
		"extTrackAutoTrackButton",
		arg,
		i
	);

	/* Create arrow button widget as prev track button */
	i = 0;
	XtSetArg(arg[i], XmNtraversalOn, True); i++;
	XtSetArg(arg[i], XmNarrowDirection, XmARROW_LEFT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.prev_btn = XmCreateArrowButton(
		m->dbextt.form,
		"extTrackPrevButton",
		arg,
		i
	);

	/* Create arrow button widget as next track button */
	i = 0;
	XtSetArg(arg[i], XmNtraversalOn, True); i++;
	XtSetArg(arg[i], XmNarrowDirection, XmARROW_RIGHT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.next_btn = XmCreateArrowButton(
		m->dbextt.form,
		"extTrackNextButton",
		arg,
		i
	);

	/* Create label widget as track number label */
	m->dbextt.trkno_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackNumberLabel",
		NULL,
		0
	);

	/* Create label widget as track info label */
	m->dbextt.trk_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoLabel",
		NULL,
		0
	);

	/* Create separator bar widget */
	m->dbextt.dbextt_sep = XmCreateSeparator(
		m->dbextt.form,
		"extTrackInfoSeparator",
		NULL,
		0
	);

	/* Create label widget as sort title label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->dbextt.sorttitle_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoSortTitleLabel",
		arg,
		i
	);

	/* Create text widget as sort title display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.sorttitle_txt = XmCreateText(
		m->dbextt.form,
		"extTrackInfoSortTitleText",
		arg,
		i
	);

	/* Create toggle button for sort title "the" button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.the_btn = XmCreateToggleButton(
		m->dbextt.form,
		"extTrackInfoTheButton",
		arg,
		i
	);

	/* Create text widget as name the display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNsensitive, False); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.the_txt = XmCreateText(
		m->dbextt.form,
		"extTrackInfoTheText",
		arg,
		i
	);

	/* Create label widget as artist label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->dbextt.artist_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoArtistLabel",
		arg,
		i
	);

	/* Create text widget as artist display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.artist_txt = XmCreateText(
		m->dbextt.form,
		"extTrackInfoArtistText",
		arg,
		i
	);

	/* Create pushbutton widget as artist fullname button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.fullname_btn = XmCreatePushButton(
		m->dbextt.form,
		"extTrackInfoFullNameButton",
		arg,
		i
	);

	/* Create label widget as year label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->dbextt.year_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoYearLabel",
		arg,
		i
	);

	/* Create text widget as year display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.year_txt = XmCreateText(
		m->dbextt.form,
		"extTrackInfoYearText",
		arg,
		i
	);

	/* Create label widget as Label label */
	m->dbextt.label_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoLabelLabel",
		NULL,
		0
	);

	/* Create text widget as Label display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.label_txt = XmCreateText(
		m->dbextt.form,
		"extTrackInfoLabelText",
		arg,
		i
	);

	/* Create label widget as BPM label */
	m->dbextt.bpm_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoBPMLabel",
		NULL,
		0
	);

	/* Create text widget as BPM display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.bpm_txt = XmCreateText(
		m->dbextt.form,
		"extTrackInfoBPMText",
		arg,
		i
	);

	for (n = 0; n < 2; n++) {
		char	name[STR_BUF_SZ];

		/* Create pulldown menu widget for Genre selector */
		(void) sprintf(name, "extTrackInfoGenre%dMenu", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextt.genre_menu[n] = XmCreatePulldownMenu(
			m->dbextt.form,
			name,
			arg,
			i
		);

		/* Create option menu widget for Genre selector */
		(void) sprintf(name, "extTrackInfoGenre%dOptionMenu", n);
		i = 0;
		XtSetArg(arg[i], XmNsubMenuId, m->dbextt.genre_menu[n]); i++;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
		XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
		m->dbextt.genre_opt[n] = XmCreateOptionMenu(
			m->dbextt.form,
			name,
			arg,
			i
		);

		/* Create label widget as Genre menu label */
		(void) sprintf(name, "extTrackInfoGenre%dLabel", n);
		m->dbextt.genre_lbl[n] = XmCreateLabel(
			m->dbextt.genre_menu[n],
			name,
			NULL,
			0
		);

		/* Create separator bar widget */
		(void) sprintf(name, "extTrackInfoGenre%dSeparator", n);
		i = 0;
		XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
		m->dbextt.genre_sep[n] = XmCreateSeparator(
			m->dbextt.genre_menu[n],
			name,
			arg,
			i
		);

		/* Create pushbutton widget as Genre "none" button */
		(void) sprintf(name, "extTrackInfoGenre%dNoneButton", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextt.genre_none_btn[n] = XmCreatePushButton(
			m->dbextt.genre_menu[n],
			name,
			arg,
			i
		);

		/* Create pulldown menu widget for Subgenre selector */
		(void) sprintf(name, "extTrackInfoSubgenre%dMenu", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextt.subgenre_menu[n] = XmCreatePulldownMenu(
			m->dbextt.form,
			name,
			arg,
			i
		);

		/* Create option menu widget for Subgenre selector */
		(void) sprintf(name, "extTrackInfoSubgenre%dOptionMenu", n);
		i = 0;
		XtSetArg(arg[i], XmNsubMenuId,
			 m->dbextt.subgenre_menu[n]); i++;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
		XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
		m->dbextt.subgenre_opt[n] = XmCreateOptionMenu(
			m->dbextt.form,
			name,
			arg,
			i
		);

		/* Create label widget as Subgenre menu label */
		(void) sprintf(name, "extTrackInfoSubgenre%dLabel", n);
		m->dbextt.subgenre_lbl[n] = XmCreateLabel(
			m->dbextt.subgenre_menu[n],
			name,
			NULL,
			0
		);

		/* Create separator bar widget */
		(void) sprintf(name, "extTrackInfoSubgenre%dSeparator", n);
		i = 0;
		XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
		m->dbextt.subgenre_sep[n] = XmCreateSeparator(
			m->dbextt.subgenre_menu[n],
			name,
			arg,
			i
		);

		/* Create pushbutton widget as Subgenre "none" button */
		(void) sprintf(name, "extTrackInfoSubgenre%dNoneButton", n);
		i = 0;
#ifndef USE_SGI_DESKTOP
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
		m->dbextt.subgenre_none_btn[n] = XmCreatePushButton(
			m->dbextt.subgenre_menu[n],
			name,
			arg,
			i
		);
	}

	/* Create separator bar widget */
	m->dbextt.dbextt_sep2 = XmCreateSeparator(
		m->dbextt.form,
		"extTrackInfoSeparator2",
		NULL,
		0
	);

	/* Create label widget as Track Notes label */
	m->dbextt.notes_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoNotesLabel",
		NULL,
		0
	);

	/* Create text widget as Track Notes editor/viewer */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.notes_txt = XmCreateScrolledText(
		m->dbextt.form,
		"extTrackInfoNotesText",
		arg,
		i
	);

	/* Create label widget as ISRC label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.isrc_lbl = XmCreateLabel(
		m->dbextt.form,
		"extTrackInfoISRCLabel",
		arg,
		i
	);

	/* Create label widget as ISRC indicator label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.isrc_ind = XmCreateLabel(
		m->dbextt.form,
		"",
		arg,
		i
	);

	/* Create separator bar widget */
	m->dbextt.dbextt_sep3 = XmCreateSeparator(
		m->dbextt.form,
		"extTrackInfoSeparator3",
		NULL,
		0
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->dbextt.ok_btn = XmCreatePushButton(
		m->dbextt.form,
		"extTrackInfoOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->dbextt.autotrk_btn);
	XtManageChild(m->dbextt.prev_btn);
	XtManageChild(m->dbextt.next_btn);
	XtManageChild(m->dbextt.trkno_lbl);
	XtManageChild(m->dbextt.trk_lbl);
	XtManageChild(m->dbextt.dbextt_sep);
	XtManageChild(m->dbextt.sorttitle_lbl);
	XtManageChild(m->dbextt.sorttitle_txt);
	XtManageChild(m->dbextt.the_btn);
	XtManageChild(m->dbextt.the_txt);
	XtManageChild(m->dbextt.artist_lbl);
	XtManageChild(m->dbextt.artist_txt);
	XtManageChild(m->dbextt.fullname_btn);
	XtManageChild(m->dbextt.year_lbl);
	XtManageChild(m->dbextt.year_txt);
	XtManageChild(m->dbextt.label_lbl);
	XtManageChild(m->dbextt.label_txt);
	XtManageChild(m->dbextt.bpm_lbl);
	XtManageChild(m->dbextt.bpm_txt);
	for (n = 0; n < 2; n++) {
		XtManageChild(m->dbextt.genre_opt[n]);
		XtManageChild(m->dbextt.genre_lbl[n]);
		XtManageChild(m->dbextt.genre_sep[n]);
		XtManageChild(m->dbextt.genre_none_btn[n]);
		XtManageChild(m->dbextt.subgenre_opt[n]);
		XtManageChild(m->dbextt.subgenre_lbl[n]);
		XtManageChild(m->dbextt.subgenre_sep[n]);
		XtManageChild(m->dbextt.subgenre_none_btn[n]);
	}
	XtManageChild(m->dbextt.dbextt_sep2);
	XtManageChild(m->dbextt.notes_lbl);
	XtManageChild(m->dbextt.notes_txt);
	XtManageChild(m->dbextt.isrc_lbl);
	XtManageChild(m->dbextt.isrc_ind);
	XtManageChild(m->dbextt.dbextt_sep3);
	XtManageChild(m->dbextt.ok_btn);
}


/*
 * create_credits_widgets
 *	Create all widgets in the credits window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_credits_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->credits.form = XmCreateFormDialog(
		m->toplevel,
		"creditsForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->credits.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create toggle button for auto-track */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.autotrk_btn = XmCreateToggleButton(
		m->credits.form,
		"creditsAutoTrackButton",
		arg,
		i
	);

	/* Create arrow button widget as prev track button */
	i = 0;
	XtSetArg(arg[i], XmNtraversalOn, True); i++;
	XtSetArg(arg[i], XmNarrowDirection, XmARROW_LEFT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.prev_btn = XmCreateArrowButton(
		m->credits.form,
		"creditsPrevTrackButton",
		arg,
		i
	);

	/* Create arrow button widget as next track button */
	i = 0;
	XtSetArg(arg[i], XmNtraversalOn, True); i++;
	XtSetArg(arg[i], XmNarrowDirection, XmARROW_RIGHT); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.next_btn = XmCreateArrowButton(
		m->credits.form,
		"creditsNextTrackButton",
		arg,
		i
	);

	/* Create label widget as disc/track number label */
	m->credits.disctrk_lbl = XmCreateLabel(
		m->credits.form,
		"creditsDiscTrackNumberLabel",
		NULL,
		0
	);

	/* Create label widget as title label */
	m->credits.title_lbl = XmCreateLabel(
		m->credits.form,
		"creditsTitleLabel",
		NULL,
		0
	);

	/* Create label widget as credit list label */
	m->credits.credlist_lbl = XmCreateLabel(
		m->credits.form,
		"creditsListLabel",
		NULL,
		0
	);

	/* Create scrolled list widget as credits list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 3); i++;
#else
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 2); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.cred_list = XmCreateScrolledList(
		m->credits.form,
		"creditsList",
		arg,
		i
	);

	/* Create separator bar widget */
	m->credits.cred_sep1 = XmCreateSeparator(
		m->credits.form,
		"creditsSeparator1",
		NULL,
		0
	);

	/* Create label widget as credit list label */
	m->credits.crededit_lbl = XmCreateLabel(
		m->credits.form,
		"creditsEditorLabel",
		NULL,
		0
	);

	/* Create pulldown menu widget for Role category selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->credits.prirole_menu = XmCreatePulldownMenu(
		m->credits.form,
		"creditsPrimaryRolePulldownMenu",
		arg,
		i
	);

	/* Create option menu widget for Role category selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->credits.prirole_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.prirole_opt = XmCreateOptionMenu(
		m->credits.form,
		"creditsPrimaryRoleOptionMenu",
		arg,
		i
	);

	/* Create label widget as Role category menu label */
	m->credits.prirole_lbl = XmCreateLabel(
		m->credits.prirole_menu,
		"creditsPrimaryRoleMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->credits.prirole_sep = XmCreateSeparator(
		m->credits.prirole_menu,
		"creditsPrimaryRoleMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget as Role category "none" button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->credits.prirole_none_btn = XmCreatePushButton(
		m->credits.prirole_menu,
		"creditsPrimaryRoleNoneButton",
		arg,
		i
	);

	/* Create pulldown menu widget for Sub-role selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->credits.subrole_menu = XmCreatePulldownMenu(
		m->credits.form,
		"creditsSubRolePulldownMenu",
		arg,
		i
	);

	/* Create option menu widget for Sub-role selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->credits.subrole_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.subrole_opt = XmCreateOptionMenu(
		m->credits.form,
		"creditsSubRoleOptionMenu",
		arg,
		i
	);

	/* Create label widget as Sub-role menu label */
	m->credits.subrole_lbl = XmCreateLabel(
		m->credits.subrole_menu,
		"creditsSubRoleMenuLabel",
		NULL,
		0
	);

	/* Create separator bar widget */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->credits.subrole_sep = XmCreateSeparator(
		m->credits.subrole_menu,
		"creditsSubRoleMenuSeparator",
		arg,
		i
	);

	/* Create pushbutton widget as Sub-role "none" button */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->credits.subrole_none_btn = XmCreatePushButton(
		m->credits.subrole_menu,
		"creditsSubRoleNoneButton",
		arg,
		i
	);

	/* Create label widget as contributor name label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->credits.name_lbl = XmCreateLabel(
		m->credits.form,
		"creditsNameLabel",
		arg,
		i
	);

	/* Create text widget as contributor name display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.name_txt = XmCreateText(
		m->credits.form,
		"creditsNameText",
		arg,
		i
	);

	/* Create pushbutton widget as contributor fullname button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.fullname_btn = XmCreatePushButton(
		m->credits.form,
		"creditsFullNameButton",
		arg,
		i
	);

	/* Create label widget as Credit Notes label */
	m->credits.notes_lbl = XmCreateLabel(
		m->credits.form,
		"creditsNotesLabel",
		NULL,
		0
	);

	/* Create text widget as Credit Notes editor/viewer */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.notes_txt = XmCreateScrolledText(
		m->credits.form,
		"creditsNotesText",
		arg,
		i
	);

	/* Create separator bar widget */
	m->credits.cred_sep2 = XmCreateSeparator(
		m->credits.form,
		"creditsSeparator2",
		NULL,
		0
	);

	/* Create pushbutton widget as Add button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.add_btn = XmCreatePushButton(
		m->credits.form,
		"creditsAddButton",
		arg,
		i
	);

	/* Create pushbutton widget as Modify button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.mod_btn = XmCreatePushButton(
		m->credits.form,
		"creditsModifyButton",
		arg,
		i
	);

	/* Create pushbutton widget as Delete button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.del_btn = XmCreatePushButton(
		m->credits.form,
		"creditsDeleteButton",
		arg,
		i
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->credits.ok_btn = XmCreatePushButton(
		m->credits.form,
		"creditsOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->credits.autotrk_btn);
	XtManageChild(m->credits.prev_btn);
	XtManageChild(m->credits.next_btn);
	XtManageChild(m->credits.disctrk_lbl);
	XtManageChild(m->credits.title_lbl);
	XtManageChild(m->credits.credlist_lbl);
	XtManageChild(m->credits.cred_list);
	XtManageChild(m->credits.cred_sep1);
	XtManageChild(m->credits.crededit_lbl);
	XtManageChild(m->credits.prirole_opt);
	XtManageChild(m->credits.prirole_lbl);
	XtManageChild(m->credits.prirole_sep);
	XtManageChild(m->credits.prirole_none_btn);
	XtManageChild(m->credits.subrole_opt);
	XtManageChild(m->credits.subrole_lbl);
	XtManageChild(m->credits.subrole_sep);
	XtManageChild(m->credits.subrole_none_btn);
	XtManageChild(m->credits.name_lbl);
	XtManageChild(m->credits.name_txt);
	XtManageChild(m->credits.fullname_btn);
	XtManageChild(m->credits.notes_lbl);
	XtManageChild(m->credits.notes_txt);
	XtManageChild(m->credits.cred_sep2);
	XtManageChild(m->credits.add_btn);
	XtManageChild(m->credits.mod_btn);
	XtManageChild(m->credits.del_btn);
	XtManageChild(m->credits.ok_btn);
}


/*
 * create_segments_widgets
 *	Create all widgets in the segments window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_segments_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->segments.form = XmCreateFormDialog(
		m->toplevel,
		"segmentsForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->segments.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as disc number label */
	m->segments.discno_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsDiscNumberLabel",
		NULL,
		0
	);

	/* Create label widget as artist/title label */
	m->segments.disc_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsDiscLabel",
		NULL,
		0
	);

	/* Create label widget as segment list label */
	m->segments.seglist_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsListLabel",
		NULL,
		0
	);

	/* Create separator bar widget */
	m->segments.seg_sep1 = XmCreateSeparator(
		m->segments.form,
		"segmentsSeparator1",
		NULL,
		0
	);

	/* Create label widget as segments list label */
	m->segments.seglist_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsListLabel",
		NULL,
		0
	);

	/* Create scrolled list widget as segments list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 3); i++;
#else
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 2); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.seg_list = XmCreateScrolledList(
		m->segments.form,
		"segmentsList",
		arg,
		i
	);

	/* Create label widget as segments view/editor label */
	m->segments.segedit_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsEditorLabel",
		NULL,
		0
	);

	/* Create label widget as segment name label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->segments.name_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsNameLabel",
		arg,
		i
	);

	/* Create text widget as segment name display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.name_txt = XmCreateText(
		m->segments.form,
		"segmentsNameText",
		arg,
		i
	);

	/* Create label widget as segment start label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->segments.start_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsStartLabel",
		arg,
		i
	);

	/* Create label widget as segment end label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->segments.end_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsEndLabel",
		arg,
		i
	);

	/* Create label widget as track label */
	m->segments.track_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsTrackLabel",
		NULL,
		0
	);

	/* Create label widget as frame label */
	m->segments.frame_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsFrameLabel",
		NULL,
		0
	);

	/* Create text widget as start track display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.starttrk_txt = XmCreateText(
		m->segments.form,
		"segmentsStartTrackText",
		arg,
		i
	);

	/* Create text widget as start frame display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.startfrm_txt = XmCreateText(
		m->segments.form,
		"segmentsStartFrameText",
		arg,
		i
	);

	/* Create text widget as end track display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.endtrk_txt = XmCreateText(
		m->segments.form,
		"segmentsEndTrackText",
		arg,
		i
	);

	/* Create text widget as end frame display/editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.endfrm_txt = XmCreateText(
		m->segments.form,
		"segmentsEndFrameText",
		arg,
		i
	);

	/* Create label widget as start pointer label */
	m->segments.startptr_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsStartPointerLabel",
		NULL,
		0
	);

	/* Create label widget as end pointer label */
	m->segments.endptr_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsEndPointerLabel",
		NULL,
		0
	);

	/* Create pushbutton widget as set button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.set_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsSetButton",
		arg,
		i
	);

	/* Create label widget as play-set label */
	m->segments.playset_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsPlaySetLabel",
		NULL,
		0
	);

	/* Create pushbutton widget as play/pause button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.playpaus_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsPlayPauseButton",
		arg,
		i
	);

	/* Create pushbutton widget as stop button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.stop_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsStopButton",
		arg,
		i
	);

	/* Create label widget as segment notes label */
	m->segments.notes_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsNotesLabel",
		NULL,
		0
	);

	/* Create text widget as segment notes editor/viewer */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.notes_txt = XmCreateScrolledText(
		m->segments.form,
		"segmentsNotesText",
		arg,
		i
	);

	/* Create pushbutton widget as credits button */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.credits_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsCreditsButton",
		arg,
		i
	);

	/* Create label widget as segment label */
	m->segments.segment_lbl = XmCreateLabel(
		m->segments.form,
		"segmentsSegmentLabel",
		NULL,
		0
	);

	/* Create separator bar widget */
	m->segments.seg_sep2 = XmCreateSeparator(
		m->segments.form,
		"segmentsSeparator2",
		NULL,
		0
	);

	/* Create pushbutton widget as Add button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.add_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsAddButton",
		arg,
		i
	);

	/* Create pushbutton widget as Modify button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.mod_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsModifyButton",
		arg,
		i
	);

	/* Create pushbutton widget as Delete button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.del_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsDeleteButton",
		arg,
		i
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->segments.ok_btn = XmCreatePushButton(
		m->segments.form,
		"segmentsOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->segments.discno_lbl);
	XtManageChild(m->segments.disc_lbl);
	XtManageChild(m->segments.seg_sep1);
	XtManageChild(m->segments.seglist_lbl);
	XtManageChild(m->segments.seg_list);
	XtManageChild(m->segments.segedit_lbl);
	XtManageChild(m->segments.name_lbl);
	XtManageChild(m->segments.name_txt);
	XtManageChild(m->segments.start_lbl);
	XtManageChild(m->segments.end_lbl);
	XtManageChild(m->segments.track_lbl);
	XtManageChild(m->segments.frame_lbl);
	XtManageChild(m->segments.starttrk_txt);
	XtManageChild(m->segments.endtrk_txt);
	XtManageChild(m->segments.startfrm_txt);
	XtManageChild(m->segments.endfrm_txt);
	XtManageChild(m->segments.startptr_lbl);
	XtManageChild(m->segments.endptr_lbl);
	XtManageChild(m->segments.set_btn);
	XtManageChild(m->segments.playset_lbl);
	XtManageChild(m->segments.playpaus_btn);
	XtManageChild(m->segments.stop_btn);
	XtManageChild(m->segments.notes_lbl);
	XtManageChild(m->segments.notes_txt);
	XtManageChild(m->segments.segment_lbl);
	XtManageChild(m->segments.credits_btn);
	XtManageChild(m->segments.seg_sep2);
	XtManageChild(m->segments.add_btn);
	XtManageChild(m->segments.mod_btn);
	XtManageChild(m->segments.del_btn);
	XtManageChild(m->segments.ok_btn);
}


/*
 * create_submiturl_widgets
 *	Create all widgets in the submiturl window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_submiturl_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->submiturl.form = XmCreateFormDialog(
		m->toplevel,
		"submitURLForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->submiturl.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as logo */
	m->submiturl.logo_lbl = XmCreateLabel(
		m->submiturl.form,
		"submitURLLogoLabel",
		NULL,
		0
	);

	/* Create label widget as submiturl heading label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	m->submiturl.heading_lbl = XmCreateLabel(
		m->submiturl.form,
		"submitURLHeadingLabel",
		arg,
		i
	);

	/* Create label widget as URL category label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->submiturl.categ_lbl = XmCreateLabel(
		m->submiturl.form,
		"submitURLCategoryLabel",
		arg,
		i
	);

	/* Create text widget for URL category */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->submiturl.categ_txt = XmCreateText(
		m->submiturl.form,
		"submitURLCategoryText",
		arg,
		i
	);

	/* Create label widget as URL name label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->submiturl.name_lbl = XmCreateLabel(
		m->submiturl.form,
		"submitURLNameLabel",
		arg,
		i
	);

	/* Create text widget for URL name */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->submiturl.name_txt = XmCreateText(
		m->submiturl.form,
		"submitURLNameText",
		arg,
		i
	);

	/* Create label widget as URL label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->submiturl.url_lbl = XmCreateLabel(
		m->submiturl.form,
		"submitURLLabel",
		arg,
		i
	);

	/* Create text widget for URL text */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->submiturl.url_txt = XmCreateText(
		m->submiturl.form,
		"submitURLText",
		arg,
		i
	);

	/* Create label widget as Descr label */
	m->submiturl.desc_lbl = XmCreateLabel(
		m->submiturl.form,
		"submitURLDescrLabel",
		NULL,
		0
	);

	/* Create text widget for Descr text */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->submiturl.desc_txt = XmCreateScrolledText(
		m->submiturl.form,
		"submitURLDescrText",
		arg,
		i
	);

	/* Create separator bar widget */
	m->submiturl.submiturl_sep = XmCreateSeparator(
		m->submiturl.form,
		"submitURLSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as Submit button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->submiturl.submit_btn = XmCreatePushButton(
		m->submiturl.form,
		"submitURLSubmitButton",
		arg,
		i
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->submiturl.ok_btn = XmCreatePushButton(
		m->submiturl.form,
		"submitURLOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->submiturl.logo_lbl);
	XtManageChild(m->submiturl.heading_lbl);
	XtManageChild(m->submiturl.categ_lbl);
	XtManageChild(m->submiturl.categ_txt);
	XtManageChild(m->submiturl.name_lbl);
	XtManageChild(m->submiturl.name_txt);
	XtManageChild(m->submiturl.url_lbl);
	XtManageChild(m->submiturl.url_txt);
	XtManageChild(m->submiturl.desc_lbl);
	XtManageChild(m->submiturl.desc_txt);
	XtManageChild(m->submiturl.submiturl_sep);
	XtManageChild(m->submiturl.submit_btn);
	XtManageChild(m->submiturl.ok_btn);
}


/*
 * create_regionsel_widgets
 *	Create all widgets in the regionsel window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_regionsel_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); i++;
	m->regionsel.form = XmCreateFormDialog(
		m->toplevel,
		"regionSelectForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->regionsel.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as region selector label */
	m->regionsel.region_lbl = XmCreateLabel(
		m->regionsel.form,
		"regionSelectLabel",
		NULL,
		0
	);

	/* Create scrolled list widget as region list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 3); i++;
#else
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 2); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->regionsel.region_list = XmCreateScrolledList(
		m->regionsel.form,
		"regionSelectList",
		arg,
		i
	);

	/* Create separator bar widget */
	m->regionsel.region_sep = XmCreateSeparator(
		m->regionsel.form,
		"regionSelectSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->regionsel.ok_btn = XmCreatePushButton(
		m->regionsel.form,
		"regionSelectOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->regionsel.region_lbl);
	XtManageChild(m->regionsel.region_list);
	XtManageChild(m->regionsel.region_sep);
	XtManageChild(m->regionsel.ok_btn);
}


/*
 * create_langsel_widgets
 *	Create all widgets in the langsel window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_langsel_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); i++;
	m->langsel.form = XmCreateFormDialog(
		m->toplevel,
		"languageSelectForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->langsel.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as language selector label */
	m->langsel.lang_lbl = XmCreateLabel(
		m->langsel.form,
		"languageSelectLabel",
		NULL,
		0
	);

	/* Create scrolled list widget as language list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 3); i++;
#else
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 2); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->langsel.lang_list = XmCreateScrolledList(
		m->langsel.form,
		"languageSelectList",
		arg,
		i
	);

	/* Create separator bar widget */
	m->langsel.lang_sep = XmCreateSeparator(
		m->langsel.form,
		"languageSelectSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->langsel.ok_btn = XmCreatePushButton(
		m->langsel.form,
		"languageSelectOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->langsel.lang_lbl);
	XtManageChild(m->langsel.lang_list);
	XtManageChild(m->langsel.lang_sep);
	XtManageChild(m->langsel.ok_btn);
}


/*
 * create_matchsel_widgets
 *	Create all widgets in the matchsel window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_matchsel_widgets(widgets_t *m)
{
	int		i;
	Arg		arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); i++;
	m->matchsel.form = XmCreateFormDialog(
		m->toplevel,
		"matchSelectForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->matchsel.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as logo */
	m->matchsel.logo_lbl = XmCreateLabel(
		m->matchsel.form,
		"matchSelectLogoLabel",
		NULL,
		0
	);

	/* Create label widget as match selector label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	m->matchsel.matchsel_lbl = XmCreateLabel(
		m->matchsel.form,
		"matchSelectLabel",
		arg,
		i
	);

	/* Create scrolled list widget as match list */
	i = 0;
	XtSetArg(arg[i], XmNautomaticSelection, False); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmCONSTANT); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy, XmSTATIC); i++;
#ifdef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 3); i++;
#else
	XtSetArg(arg[i], XmNscrolledWindowMarginWidth, 2); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->matchsel.matchsel_list = XmCreateScrolledList(
		m->matchsel.form,
		"matchSelectList",
		arg,
		i
	);

	/* Create separator bar widget */
	m->matchsel.matchsel_sep = XmCreateSeparator(
		m->matchsel.form,
		"matchSelectSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->matchsel.ok_btn = XmCreatePushButton(
		m->matchsel.form,
		"matchSelectOkButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->matchsel.logo_lbl);
	XtManageChild(m->matchsel.matchsel_lbl);
	XtManageChild(m->matchsel.matchsel_list);
	XtManageChild(m->matchsel.matchsel_sep);
	XtManageChild(m->matchsel.ok_btn);
}


/*
 * create_userreg_widgets
 *	Create all widgets in the userreg window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_userreg_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->userreg.form = XmCreateFormDialog(
		m->toplevel,
		"userRegForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->userreg.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label for the logo pixmap */
	m->userreg.logo_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegLogoLabel",
		NULL,
		0
	);

	/* Create label for the window caption */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	m->userreg.caption_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegCaptionLabel",
		arg,
		i
	);

	/* Create label for the handle label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.handle_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegHandleLabel",
		arg,
		i
	);

	/* Create text widget as handle editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.handle_txt = XmCreateText(
		m->userreg.form,
		"userRegHandleText",
		arg,
		i
	);

	/* Create label for the passwd label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.passwd_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegPasswdLabel",
		arg,
		i
	);

	/* Create text widget as passwd editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.passwd_txt = XmCreateText(
		m->userreg.form,
		"userRegPasswdText",
		arg,
		i
	);

	/* Create label for the verify passwd label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.vpasswd_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegVerifyPasswdLabel",
		arg,
		i
	);

	/* Create text widget as passwd editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.vpasswd_txt = XmCreateText(
		m->userreg.form,
		"userRegVerifyPasswdText",
		arg,
		i
	);

	/* Create label for the passwd hint label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.hint_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegPasswdHintLabel",
		arg,
		i
	);

	/* Create text widget as passwd hint editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.hint_txt = XmCreateText(
		m->userreg.form,
		"userRegPasswdHintText",
		arg,
		i
	);

	/* Create pushbutton widget for get password hint button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.gethint_btn = XmCreatePushButton(
		m->userreg.form,
		"userRegGetHintButton",
		arg,
		i
	);

	/* Create label for the email addr label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.email_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegEmailLabel",
		arg,
		i
	);

	/* Create text widget as email addr editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.email_txt = XmCreateText(
		m->userreg.form,
		"userRegEmailText",
		arg,
		i
	);

	/* Create label widget for Region label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.region_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegRegionLabel",
		arg,
		i
	);

	/* Create text widget as Region display */
	i = 0;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.region_txt = XmCreateText(
		m->userreg.form,
		"userRegRegionText",
		arg,
		i
	);

	/* Create pushbutton widget for Region change button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.region_chg_btn = XmCreatePushButton(
		m->userreg.form,
		"userRegRegionChangeButton",
		arg,
		i
	);

	/* Create label for the postal code label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.postal_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegPostalCodeLabel",
		arg,
		i
	);

	/* Create text widget as postal code editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.postal_txt = XmCreateText(
		m->userreg.form,
		"userRegPostalCodeText",
		arg,
		i
	);

	/* Create label for the age label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.age_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegAgeLabel",
		arg,
		i
	);

	/* Create text widget as age editor */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.age_txt = XmCreateText(
		m->userreg.form,
		"userRegAgeText",
		arg,
		i
	);

	/* Create label for the gender label */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	m->userreg.gender_lbl = XmCreateLabel(
		m->userreg.form,
		"userRegGenderLabel",
		arg,
		i
	);

	/* Create frame for radio box */
	i = 0;
	XtSetArg(arg[i], XmNmarginHeight, 0); i++;
	m->userreg.gender_frm = XmCreateFrame(
		m->userreg.form,
		"userRegGenderFrame",
		arg,
		i
	);

	/* Create radio box widget as gender selector */
	i = 0;
	XtSetArg(arg[i], XmNbuttonCount, 3); i++;
	XtSetArg(arg[i], XmNbuttonSet, 2); i++;
	XtSetArg(arg[i], XmNspacing, 1); i++;
	XtSetArg(arg[i], XmNmarginHeight, 2); i++;
	XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
	XtSetArg(arg[i], XmNentryCallback, gender_radbox_cblist); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.gender_radbox = XmCreateSimpleRadioBox(
		m->userreg.gender_frm,
		"userRegGenderRadioBox",
		arg,
		i
	);
	m->userreg.male_btn = XtNameToWidget(
		m->userreg.gender_radbox, "button_0"
	);
	m->userreg.female_btn = XtNameToWidget(
		m->userreg.gender_radbox, "button_1"
	);
	m->userreg.unspec_btn = XtNameToWidget(
		m->userreg.gender_radbox, "button_2"
	);

	XtVaSetValues(m->userreg.male_btn, XmNmarginHeight, 0, NULL);
	XtVaSetValues(m->userreg.female_btn, XmNmarginHeight, 0, NULL);
	XtVaSetValues(m->userreg.unspec_btn, XmNmarginHeight, 0, NULL);

	/* Create toggle button for the allow mail button */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.allowmail_btn = XmCreateToggleButton(
		m->userreg.form,
		"userRegAllowMailButton",
		arg,
		i
	);

	/* Create toggle button for the allow stats button */
	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 0); i++;
#endif
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.allowstats_btn = XmCreateToggleButton(
		m->userreg.form,
		"userRegAllowStatsButton",
		arg,
		i
	);

	/* Create separator bar */
	m->userreg.userreg_sep = XmCreateSeparator(
		m->userreg.form,
		"userRegSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget for the OK button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.ok_btn = XmCreatePushButton(
		m->userreg.form,
		"userRegOkButton",
		arg,
		i
	);

	/* Create pushbutton widget for the Privacy info button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.priv_btn = XmCreatePushButton(
		m->userreg.form,
		"userRegPrivacyInfoButton",
		arg,
		i
	);

	/* Create pushbutton widget for the Cancel button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->userreg.cancel_btn = XmCreatePushButton(
		m->userreg.form,
		"userRegCancelButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->userreg.logo_lbl);
	XtManageChild(m->userreg.caption_lbl);
	XtManageChild(m->userreg.handle_lbl);
	XtManageChild(m->userreg.handle_txt);
	XtManageChild(m->userreg.passwd_lbl);
	XtManageChild(m->userreg.passwd_txt);
	XtManageChild(m->userreg.vpasswd_lbl);
	XtManageChild(m->userreg.vpasswd_txt);
	XtManageChild(m->userreg.hint_lbl);
	XtManageChild(m->userreg.hint_txt);
	XtManageChild(m->userreg.gethint_btn);
	XtManageChild(m->userreg.email_lbl);
	XtManageChild(m->userreg.email_txt);
	XtManageChild(m->userreg.region_lbl);
	XtManageChild(m->userreg.region_txt);
	XtManageChild(m->userreg.region_chg_btn);
	XtManageChild(m->userreg.postal_lbl);
	XtManageChild(m->userreg.postal_txt);
	XtManageChild(m->userreg.age_lbl);
	XtManageChild(m->userreg.age_txt);
	XtManageChild(m->userreg.gender_lbl);
	XtManageChild(m->userreg.gender_frm);
	XtManageChild(m->userreg.gender_radbox);
	XtManageChild(m->userreg.male_btn);
	XtManageChild(m->userreg.female_btn);
	XtManageChild(m->userreg.unspec_btn);
	XtManageChild(m->userreg.allowmail_btn);
	XtManageChild(m->userreg.allowstats_btn);
	XtManageChild(m->userreg.userreg_sep);
	XtManageChild(m->userreg.ok_btn);
	XtManageChild(m->userreg.priv_btn);
	XtManageChild(m->userreg.cancel_btn);
}


/*
 * create_help_widgets
 *	Create all widgets in the help text display window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_help_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];

	/* Help popup window */

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->help.form = XmCreateFormDialog(
		m->toplevel,
		"helpForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->help.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create pulldown menu widget for topic selector */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->help.topic_menu = XmCreatePulldownMenu(
		m->help.form,
		"topicPulldownMenu",
		arg,
		i
	);

	/* Create label widget for topic menu title */
	m->help.topic_lbl = XmCreateLabel(
		m->help.topic_menu,
		"topicLabel",
		NULL,
		0
	);

	/* Create separator bar widget as menu separator */
	i = 0;
	XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
	m->help.topic_sep = XmCreateSeparator(
		m->help.topic_menu,
		"topicSeparator",
		arg,
		i
	);

	/* Create pushbutton widget for online help menu entry */
	i = 0;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	m->help.online_btn = XmCreatePushButton(
		m->help.topic_menu,
		"onlineHelpButton",
		arg,
		i
	);

	/* Create option menu widget for topic selector */
	i = 0;
	XtSetArg(arg[i], XmNsubMenuId, m->help.topic_menu); i++;
#ifndef USE_SGI_DESKTOP
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
#endif
	XtSetArg(arg[i], XmNnavigationType, XmTAB_GROUP); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->help.topic_opt = XmCreateOptionMenu(
		m->help.form,
		"topicOptionMenu",
		arg,
		i
	);

	/* Create text widget as help text viewer */
	i = 0;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->help.help_txt = XmCreateScrolledText(
		m->help.form,
		"helpText",
		arg,
		i
	);

	/* Create separator bar widget */
	m->help.help_sep = XmCreateSeparator(
		m->help.form,
		"helpSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as about button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->help.about_btn = XmCreatePushButton(
		m->help.form,
		"aboutButton",
		arg,
		i
	);

	/* Create pushbutton widget as Cancel button */
	i = 0;
	XtSetArg(arg[i], XmNhelpCallback, help_cblist); i++;
	m->help.cancel_btn = XmCreatePushButton(
		m->help.form,
		"helpCancelButton",
		arg,
		i
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->help.topic_lbl);
	XtManageChild(m->help.topic_sep);
	XtManageChild(m->help.online_btn);
	XtManageChild(m->help.help_txt);
	XtManageChild(m->help.help_sep);
	XtManageChild(m->help.topic_opt);
	XtManageChild(m->help.about_btn);
	XtManageChild(m->help.cancel_btn);
}


/*
 * create_auth_widgets
 *	Create all widgets in the authorization window.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_auth_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];

	/* Authorization popup window */

	/* Create form widget as container */
	i = 0;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, False); i++;
	XtSetArg(arg[i], XmNresizePolicy, XmRESIZE_NONE); i++;
	m->auth.form = XmCreateFormDialog(
		m->toplevel,
		"authForm",
		arg,
		i
	);
	XtVaSetValues(XtParent(m->auth.form),
		XmNmappedWhenManaged, False,
		NULL
	);

	/* Create label widget as auth label */
	m->auth.auth_lbl = XmCreateLabel(
		m->auth.form,
		"authLabel",
		NULL,
		0
	);

	/* Create label widget as name label */
	m->auth.name_lbl = XmCreateLabel(
		m->auth.form,
		"nameLabel",
		NULL,
		0
	);

	/* Create text widget as name input text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	m->auth.name_txt = XmCreateText(
		m->auth.form,
		"nameText",
		arg,
		i
	);

	/* Create label widget as password label */
	m->auth.pass_lbl = XmCreateLabel(
		m->auth.form,
		"passwordLabel",
		NULL,
		0
	);

	/* Create text widget as password input text field */
	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, True); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	m->auth.pass_txt = XmCreateText(
		m->auth.form,
		"passwordText",
		arg,
		i
	);

	/* Create separator bar widget */
	m->auth.auth_sep = XmCreateSeparator(
		m->auth.form,
		"authSeparator",
		NULL,
		0
	);

	/* Create pushbutton widget as OK button */
	m->auth.ok_btn = XmCreatePushButton(
		m->auth.form,
		"authOkButton",
		NULL,
		0
	);

	/* Create pushbutton widget as Cancel button */
	m->auth.cancel_btn = XmCreatePushButton(
		m->auth.form,
		"authCancelButton",
		NULL,
		0
	);

	/* Manage the widgets (except the form) */
	XtManageChild(m->auth.auth_lbl);
	XtManageChild(m->auth.name_lbl);
	XtManageChild(m->auth.name_txt);
	XtManageChild(m->auth.pass_lbl);
	XtManageChild(m->auth.pass_txt);
	XtManageChild(m->auth.auth_sep);
	XtManageChild(m->auth.ok_btn);
	XtManageChild(m->auth.cancel_btn);
}


/*
 * create_dialog_widgets
 *	Create all widgets in the dialog box windows.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_dialog_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];
	Widget	btn1,
		btn2;

	/* Create info dialog widget for information messages */
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_MODELESS); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.info = XmCreateInformationDialog(
		m->toplevel,
		"infoPopup",
		arg,
		i
	);

	/* Remove unused buttons in the info dialog widget */
	btn1 = XmMessageBoxGetChild(
		m->dialog.info,
		XmDIALOG_HELP_BUTTON
	);
	btn2 = XmMessageBoxGetChild(
		m->dialog.info,
		XmDIALOG_CANCEL_BUTTON
	);

	XtUnmanageChild(btn1);
	XtUnmanageChild(btn2);

#if XmVersion > 1001
	/* Create info2 dialog widget for information messages */
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_MODELESS); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.info2 = XmCreateInformationDialog(
		m->toplevel,
		"info2Popup",
		arg,
		i
	);

	/* Remove unused buttons in the info dialog widget */
	btn1 = XmMessageBoxGetChild(
		m->dialog.info2,
		XmDIALOG_HELP_BUTTON
	);
	btn2 = XmMessageBoxGetChild(
		m->dialog.info2,
		XmDIALOG_CANCEL_BUTTON
	);

	XtUnmanageChild(btn1);
	XtUnmanageChild(btn2);

	/* Create text widget for info2 dialog */
	i = 0;
	XtSetArg(arg[i], XmNsensitive, True); i++;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNeditMode, XmSINGLE_LINE_EDIT); i++;
	XtSetArg(arg[i], XmNcursorPositionVisible, False); i++;
	XtSetArg(arg[i], XmNcursorPosition, 0); i++;
	m->dialog.info2_txt = XmCreateText(
		m->dialog.info2,
		"info2DialogText",
		arg,
		i
	);
	XtManageChild(m->dialog.info2_txt);
#else
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_MODELESS); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.info2 = XmCreatePromptDialog(
		m->toplevel,
		"info2Popup",
		arg,
		i
	);

	/* Remove unused buttons in the info dialog widget */
	btn1 = XmSelectionBoxGetChild(
		m->dialog.info2,
		XmDIALOG_HELP_BUTTON
	);
	btn2 = XmSelectionBoxGetChild(
		m->dialog.info2,
		XmDIALOG_CANCEL_BUTTON
	);

	XtUnmanageChild(btn1);
	XtUnmanageChild(btn2);

	m->dialog.info2_txt = XmSelectionBoxGetChild(
		m->dialog.info2,
		XmDIALOG_TEXT
	);

	XtVaSetValues(m->dialog.info2_txt,
		XmNeditable, False,
		XmNeditMode, XmSINGLE_LINE_EDIT,
		XmNcursorPositionVisible, False,
		XmNcursorPosition, 0,
		NULL
	);
#endif

	/* Create warning dialog widget for warning messages */
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.warning = XmCreateWarningDialog(
		m->toplevel,
		"warningPopup",
		arg,
		i
	);

	/* Remove unused buttons in the warning dialog widget */
	btn1 = XmMessageBoxGetChild(
		m->dialog.warning,
		XmDIALOG_HELP_BUTTON
	);
	btn2 = XmMessageBoxGetChild(
		m->dialog.warning,
		XmDIALOG_CANCEL_BUTTON
	);

	XtUnmanageChild(btn1);
	XtUnmanageChild(btn2);

	/* Create error dialog widget for fatal error messages */
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.fatal = XmCreateErrorDialog(
		m->toplevel,
		"fatalPopup",
		arg,
		i
	);

	/* Remove unused buttons in the error dialog widget */
	btn1 = XmMessageBoxGetChild(
		m->dialog.fatal,
		XmDIALOG_HELP_BUTTON
	);
	btn2 = XmMessageBoxGetChild(
		m->dialog.fatal,
		XmDIALOG_CANCEL_BUTTON
	);

	XtUnmanageChild(btn1);
	XtUnmanageChild(btn2);

	/* Create question dialog widget for confirm messages */
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.confirm = XmCreateQuestionDialog(
		m->toplevel,
		"questionPopup",
		arg,
		i
	);

	/* Remove unused buttons in the question dialog widget */
	btn1 = XmMessageBoxGetChild(
		m->dialog.confirm,
		XmDIALOG_HELP_BUTTON
	);

	XtUnmanageChild(btn1);

	/* Create working dialog widget for work-in-progress messages */
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_MODELESS); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.working = XmCreateWorkingDialog(
		m->toplevel,
		"workingPopup",
		arg,
		i
	);

	/* Remove unused buttons in the working dialog widget */
	btn1 = XmMessageBoxGetChild(
		m->dialog.working,
		XmDIALOG_HELP_BUTTON
	);
	btn2 = XmMessageBoxGetChild(
		m->dialog.working,
		XmDIALOG_OK_BUTTON
	);

	XtUnmanageChild(btn1);
	XtUnmanageChild(btn2);

	/* Create info dialog widget for the About popup */
	i = 0;
	XtSetArg(arg[i], XmNdialogStyle, XmDIALOG_MODELESS); i++;
	XtSetArg(arg[i], XmNdefaultPosition, False); i++;
	XtSetArg(arg[i], XmNautoUnmanage, True); i++;
	m->dialog.about = XmCreateInformationDialog(
		m->toplevel,
		"aboutPopup",
		arg,
		i
	);

	/* Remove unused buttons in the about popup */
	btn1 = XmMessageBoxGetChild(
		m->dialog.about,
		XmDIALOG_HELP_BUTTON
	);
	btn2 = XmMessageBoxGetChild(
		m->dialog.about,
		XmDIALOG_CANCEL_BUTTON
	);

	XtUnmanageChild(btn1);
	XtUnmanageChild(btn2);
}


/*
 * create_tooltip_widgets
 *	Create all widgets for the tooltip popup feature.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
create_tooltip_widgets(widgets_t *m)
{
	int	i;
	Arg	arg[10];

	/* Create shell widget */
	i = 0;
	XtSetArg(arg[i], XmNborderWidth, 1); i++;
	XtSetArg(arg[i], XmNallowShellResize, True); i++;
	XtSetArg(arg[i], XmNoverrideRedirect, True); i++;
	m->tooltip.shell = XtCreatePopupShell(
		"tooltipShell",
		/* Some versions of LessTif will crash if we use
		 * transientShell.  But overrideShell seems to work
		 * fine.  However, the reverse is true for some
		 * versions of Motif on X11R6.  Sigh.
		 */
#ifdef LesstifVersion
		overrideShellWidgetClass,
#else
		transientShellWidgetClass,
#endif
		m->toplevel,
		arg,
		i
	);

	/* Create label widget for tooltip label string */
	i = 0;
	XtSetArg(arg[i], XmNrecomputeSize, True); i++;
	XtSetArg(arg[i], XmNmarginWidth, 4); i++;
	XtSetArg(arg[i], XmNmarginHeight, 3); i++;
	XtSetArg(arg[i], XmNlabelType, XmSTRING); i++;
	m->tooltip.tooltip_lbl = XmCreateLabel(
		m->tooltip.shell,
		"tooltipLabel",
		arg,
		i
	);

	/* Manage the widgets */
	XtManageChild(m->tooltip.tooltip_lbl);
}


/*
 * make_pixmaps
 *	Create pixmaps from bitmap data and set up various widgets to
 *	use them.
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure
 *	p - The main pixmaps placeholder structure
 *	depth - The desired depth of the pixmap
 *
 * Return:
 *	Nothing.
 */
STATIC void
make_pixmaps(widgets_t *m, pixmaps_t *p, int depth)
{
#ifdef USE_SGI_DESKTOP
	/* When compiling on SGI with 4Dwm window manager, use the capability
	 * of 4Dwm to display a colorful iconified window (needs an external
	 * SGI formatted image, called XMcd.icon, and placed in
	 * /usr/lib/images)
	 */
#else
	/* Set icon pixmap */
	p->main.icon_pixmap = bm_to_px(
		m->toplevel,
		xmcd_bits,
		xmcd_width,
		xmcd_height,
		1,
		BM_PX_BWREV
	);
	XtVaSetValues(m->toplevel, XmNiconPixmap, p->main.icon_pixmap, NULL);
#endif

	/*
	 * The following puts proper pixmaps on button faces
	 */

	p->main.mode_pixmap = bm_to_px(
		m->main.mode_btn,
		mode_bits,
		mode_width,
		mode_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.mode_hlpixmap = bm_to_px(
		m->main.mode_btn,
		mode_bits,
		mode_width,
		mode_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.mode_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.mode_pixmap,
		NULL
	);

	p->main.lock_pixmap = bm_to_px(
		m->main.check_box,
		lock_bits,
		lock_width,
		lock_height,
		depth,
		BM_PX_NORMAL
	);
	XtVaSetValues(m->main.lock_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.lock_pixmap,
		NULL
	);

	p->main.repeat_pixmap = bm_to_px(
		m->main.check_box,
		repeat_bits,
		repeat_width,
		repeat_height,
		depth,
		BM_PX_NORMAL
	);
	XtVaSetValues(m->main.repeat_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.repeat_pixmap,
		NULL
	);

	p->main.shuffle_pixmap = bm_to_px(
		m->main.check_box,
		shuffle_bits,
		shuffle_width,
		shuffle_height,
		depth,
		BM_PX_NORMAL
	);
	XtVaSetValues(m->main.shuffle_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.shuffle_pixmap,
		NULL
	);

	p->main.eject_pixmap = bm_to_px(
		m->main.eject_btn,
		eject_bits,
		eject_width,
		eject_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.eject_hlpixmap = bm_to_px(
		m->main.eject_btn,
		eject_bits,
		eject_width,
		eject_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.eject_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.eject_pixmap,
		NULL
	);

	p->main.quit_pixmap = bm_to_px(
		m->main.quit_btn,
		quit_bits,
		quit_width,
		quit_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.quit_hlpixmap = bm_to_px(
		m->main.quit_btn,
		quit_bits,
		quit_width,
		quit_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.quit_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.quit_pixmap,
		NULL
	);

	p->main.dbprog_pixmap = bm_to_px(
		m->main.dbprog_btn,
		dbprog_bits,
		dbprog_width,
		dbprog_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.dbprog_hlpixmap = bm_to_px(
		m->main.dbprog_btn,
		dbprog_bits,
		dbprog_width,
		dbprog_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.dbprog_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.dbprog_pixmap,
		NULL
	);

	p->main.options_pixmap = bm_to_px(
		m->main.options_btn,
		options_bits,
		options_width,
		options_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.options_hlpixmap = bm_to_px(
		m->main.options_btn,
		options_bits,
		options_width,
		options_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.options_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.options_pixmap,
		NULL
	);

	p->main.ab_pixmap = bm_to_px(
		m->main.ab_btn,
		ab_bits,
		ab_width,
		ab_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.ab_hlpixmap = bm_to_px(
		m->main.ab_btn,
		ab_bits,
		ab_width,
		ab_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.ab_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.ab_pixmap,
		NULL
	);

	p->main.sample_pixmap = bm_to_px(
		m->main.sample_btn,
		sample_bits,
		sample_width,
		sample_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.sample_hlpixmap = bm_to_px(
		m->main.sample_btn,
		sample_bits,
		sample_width,
		sample_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.sample_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.sample_pixmap,
		NULL
	);

	p->main.time_pixmap = bm_to_px(
		m->main.time_btn,
		time_bits,
		time_width,
		time_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.time_hlpixmap = bm_to_px(
		m->main.time_btn,
		time_bits,
		time_width,
		time_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.time_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.time_pixmap,
		NULL
	);

	p->main.keypad_pixmap = bm_to_px(
		m->main.keypad_btn,
		keypad_bits,
		keypad_width,
		keypad_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.keypad_hlpixmap = bm_to_px(
		m->main.keypad_btn,
		keypad_bits,
		keypad_width,
		keypad_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.keypad_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.keypad_pixmap,
		NULL
	);

	p->main.world_pixmap = bm_to_px(
		m->main.wwwwarp_btn,
		world_bits,
		world_width,
		world_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.world_hlpixmap = bm_to_px(
		m->main.wwwwarp_btn,
		world_bits,
		world_width,
		world_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.wwwwarp_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.world_pixmap,
		NULL
	);

	p->main.playpause_pixmap = bm_to_px(
		m->main.playpause_btn,
		playpause_bits,
		playpause_width,
		playpause_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.playpause_hlpixmap = bm_to_px(
		m->main.playpause_btn,
		playpause_bits,
		playpause_width,
		playpause_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.playpause_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.playpause_pixmap,
		NULL
	);

	p->main.stop_pixmap = bm_to_px(
		m->main.stop_btn,
		stop_bits,
		stop_width,
		stop_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.stop_hlpixmap = bm_to_px(
		m->main.stop_btn,
		stop_bits,
		stop_width,
		stop_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.stop_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.stop_pixmap,
		NULL
	);

	p->main.prevdisc_pixmap = bm_to_px(
		m->main.prevdisc_btn,
		prevdisc_bits,
		prevdisc_width,
		prevdisc_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.prevdisc_hlpixmap = bm_to_px(
		m->main.prevdisc_btn,
		prevdisc_bits,
		prevdisc_width,
		prevdisc_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.prevdisc_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.prevdisc_pixmap,
		NULL
	);

	p->main.nextdisc_pixmap = bm_to_px(
		m->main.nextdisc_btn,
		nextdisc_bits,
		nextdisc_width,
		nextdisc_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.nextdisc_hlpixmap = bm_to_px(
		m->main.nextdisc_btn,
		nextdisc_bits,
		nextdisc_width,
		nextdisc_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.nextdisc_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.nextdisc_pixmap,
		NULL
	);

	p->main.prevtrk_pixmap = bm_to_px(
		m->main.prevtrk_btn,
		prevtrk_bits,
		prevtrk_width,
		prevtrk_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.prevtrk_hlpixmap = bm_to_px(
		m->main.prevtrk_btn,
		prevtrk_bits,
		prevtrk_width,
		prevtrk_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.prevtrk_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.prevtrk_pixmap,
		NULL
	);

	p->main.nexttrk_pixmap = bm_to_px(
		m->main.nexttrk_btn,
		nexttrk_bits,
		nexttrk_width,
		nexttrk_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.nexttrk_hlpixmap = bm_to_px(
		m->main.nexttrk_btn,
		nexttrk_bits,
		nexttrk_width,
		nexttrk_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.nexttrk_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.nexttrk_pixmap,
		NULL
	);

	p->main.previdx_pixmap = bm_to_px(
		m->main.previdx_btn,
		previdx_bits,
		previdx_width,
		previdx_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.previdx_hlpixmap = bm_to_px(
		m->main.previdx_btn,
		previdx_bits,
		previdx_width,
		previdx_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.previdx_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.previdx_pixmap,
		NULL
	);

	p->main.nextidx_pixmap = bm_to_px(
		m->main.nextidx_btn,
		nextidx_bits,
		nextidx_width,
		nextidx_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.nextidx_hlpixmap = bm_to_px(
		m->main.nextidx_btn,
		nextidx_bits,
		nextidx_width,
		nextidx_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.nextidx_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.nextidx_pixmap,
		NULL
	);

	p->main.rew_pixmap = bm_to_px(
		m->main.rew_btn,
		rew_bits,
		rew_width,
		rew_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.rew_hlpixmap = bm_to_px(
		m->main.rew_btn,
		rew_bits,
		rew_width,
		rew_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.rew_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.rew_pixmap,
		NULL
	);

	p->main.ff_pixmap = bm_to_px(
		m->main.ff_btn,
		ff_bits,
		ff_width,
		ff_height,
		depth,
		BM_PX_NORMAL
	);
	p->main.ff_hlpixmap = bm_to_px(
		m->main.ff_btn,
		ff_bits,
		ff_width,
		ff_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->main.ff_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->main.ff_pixmap,
		NULL
	);

	p->dbprog.logo_pixmap = bm_to_px(
#ifdef USE_SGI_DESKTOP
		m->main.level_scale,
#else
		m->main.dbprog_btn,
#endif
		logo_bits,
		logo_width,
		logo_height,
		depth,
		BM_PX_NORMAL
	);
	XtVaSetValues(m->dbprog.logo_lbl,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->dbprog.logo_pixmap,
		XmNlabelInsensitivePixmap, p->dbprog.logo_pixmap,
		NULL
	);

	p->matchsel.logo_pixmap = bm_to_px(
#ifdef USE_SGI_DESKTOP
		m->main.level_scale,
#else
		m->main.dtitle_ind,
#endif
		cddblogo_bits,
		cddblogo_width,
		cddblogo_height,
		depth,
		BM_PX_NORMAL
	);
	XtVaSetValues(m->matchsel.logo_lbl,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->matchsel.logo_pixmap,
		XmNlabelInsensitivePixmap, p->matchsel.logo_pixmap,
		NULL
	);

	p->userreg.logo_pixmap = bm_to_px(
#ifdef USE_SGI_DESKTOP
		m->main.level_scale,
#else
		m->main.dtitle_ind,
#endif
		cddblogo_bits,
		cddblogo_width,
		cddblogo_height,
		depth,
		BM_PX_NORMAL
	);
	XtVaSetValues(m->userreg.logo_lbl,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->userreg.logo_pixmap,
		XmNlabelInsensitivePixmap, p->userreg.logo_pixmap,
		NULL
	);

	p->segments.playpause_pixmap = bm_to_px(
		m->main.playpause_btn,
		playpause_bits,
		playpause_width,
		playpause_height,
		depth,
		BM_PX_NORMAL
	);
	p->segments.playpause_hlpixmap = bm_to_px(
		m->main.playpause_btn,
		playpause_bits,
		playpause_width,
		playpause_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->segments.playpaus_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->segments.playpause_pixmap,
		NULL
	);

	p->segments.stop_pixmap = bm_to_px(
		m->main.stop_btn,
		stop_bits,
		stop_width,
		stop_height,
		depth,
		BM_PX_NORMAL
	);
	p->segments.stop_hlpixmap = bm_to_px(
		m->main.stop_btn,
		stop_bits,
		stop_width,
		stop_height,
		depth,
		BM_PX_HIGHLIGHT
	);
	XtVaSetValues(m->segments.stop_btn,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->segments.stop_pixmap,
		NULL
	);

	p->submiturl.logo_pixmap = bm_to_px(
#ifdef USE_SGI_DESKTOP
		m->main.level_scale,
#else
		m->main.dtitle_ind,
#endif
		cddblogo_bits,
		cddblogo_width,
		cddblogo_height,
		depth,
		BM_PX_NORMAL
	);
	XtVaSetValues(m->submiturl.logo_lbl,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, p->submiturl.logo_pixmap,
		XmNlabelInsensitivePixmap, p->submiturl.logo_pixmap,
		NULL
	);

	p->dialog.about_pixmap = bm_to_px(
		m->main.dtitle_ind,
		about_bits,
		about_width,
		about_height,
		depth,
		BM_PX_REV
	);
	XtVaSetValues(m->dialog.about,
		XmNsymbolPixmap, p->dialog.about_pixmap,
		NULL
	);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * bm_to_px
 *	Convert a bitmap into a pixmap.
 *
 * Args:
 *	w - A widget the pixmap should be associated with
 *	bits - Pointer to the raw bitmap data
 *	width, height - The resultant pixmap dimensions
 *	depth - The depth of the desired pixmap
 *	mode - The desired color characteristics of the pixmap
 *		BM_PX_BW	foreground: black, background: white
 *		BM_PX_BWREV 	foreground: white, background: black
 *		BM_PX_WHITE 	foreground: white, background: bg of w
 *		BM_PX_BLACK 	foreground: black, background: bg of w
 *		BM_PX_HIGHLIGHT	foreground: hl of w, background: bg of w
 *		BM_PX_NORMAL	foreground: fg of w, background: bg of w
 *		BM_PX_REV	foreground: bg of w, background: fg of w
 *
 * Return:
 *	The pixmap ID, or NULL if failure.
 */
Pixmap
bm_to_px(
	Widget	w,
	void	*bits,
	int	width,
	int	height,
	int	depth,
	int	mode
)
{
	Display		*display = XtDisplay(w);
	Window		window	 = XtWindow(w);
	int		screen	 = DefaultScreen(display);
	Pixmap		ret_pixmap;
	Pixel		fg,
			bg;

	/* Allocate colors for pixmap if on color screen */
	if (DisplayCells(display, screen) > 2 && depth > 1) {
		/* Get pixmap color configuration */
		switch (mode) {
		case BM_PX_BW:
			fg = BlackPixel(display, screen);
			bg = WhitePixel(display, screen);
			break;

		case BM_PX_BWREV:
			fg = WhitePixel(display, screen);
			bg = BlackPixel(display, screen);
			break;

		case BM_PX_WHITE:
			fg = WhitePixel(display, screen);
			XtVaGetValues(w,
				XmNbackground, &bg,
				NULL
			);
			break;

		case BM_PX_BLACK:
			fg = BlackPixel(display, screen);
			XtVaGetValues(w,
				XmNbackground, &bg,
				NULL
			);
			break;

		case BM_PX_HIGHLIGHT:
			XtVaGetValues(w,
				XmNhighlightColor, &fg,
				XmNbackground, &bg,
				NULL
			);
			break;

		case BM_PX_REV:
			XtVaGetValues(w,
				XmNforeground, &bg,
				XmNbackground, &fg,
				NULL
			);
			break;

		case BM_PX_NORMAL:
		default:
			XtVaGetValues(w,
				XmNforeground, &fg,
				XmNbackground, &bg,
				NULL
			);
			break;
		}
	}
	else {
		fg = BlackPixel(display, screen);
		bg = WhitePixel(display, screen);
	}

	ret_pixmap = XCreatePixmapFromBitmapData(
		display, window, (char *) bits, width, height, fg, bg, depth
	);

	if (ret_pixmap == (Pixmap) NULL)
		return ((Pixmap) NULL);

	return (ret_pixmap);
}


/*
 * create_widgets
 *	Top-level function to create all widgets
 *
 * Args:
 *	m - Pointer to the main widgets placeholder structure.
 *
 * Return:
 *	Nothing.
 */
void
create_widgets(widgets_t *m)
{
	create_main_widgets(m);
	create_keypad_widgets(m);
	create_options_widgets(m);
	create_perfmon_widgets(m);
	create_dbprog_widgets(m);
	create_dlist_widgets(m);
	create_fullname_widgets(m);
	create_extd_widgets(m);
	create_extt_widgets(m);
	create_credits_widgets(m);
	create_segments_widgets(m);
	create_submiturl_widgets(m);
	create_regionsel_widgets(m);
	create_langsel_widgets(m);
	create_matchsel_widgets(m);
	create_userreg_widgets(m);
	create_help_widgets(m);
	create_auth_widgets(m);
	create_dialog_widgets(m);
	create_tooltip_widgets(m);
}


/*
 * pre_realize_config
 *	Top-level function to perform set-up and initialization tasks
 *	prior to realizing all widgets.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *
 * Return:
 *	Nothing.
 */
void
pre_realize_config(widgets_t *m)
{
	static XtActionsRec	actions[] = {
		{ "hotkey",	hotkey },
		{ "focuschg",	focuschg },
		{ "mainmap",	mainmap }
	};

	/* Set geometry and location of all widgets */
	geom_force(m);

	/* Register action routines */
	XtAppAddActions(
		XtWidgetToApplicationContext(m->toplevel),
		actions,
		XtNumber(actions)
	);

	/* Add translations for iconification handling */
	XtOverrideTranslations(
		m->toplevel,
		XtParseTranslationTable(
			"<MapNotify>: mainmap()\n<UnmapNotify>: mainmap()"
		)
	);

	/* Add translations for shortcut keys */
	hotkey_init();

#if defined(EDITRES) && (XtSpecificationRelease >= 5)
	/* Enable editres interaction (see editres(1)) */
	{
		extern void _XEditResCheckMessages();

		XtAddEventHandler(
			m->toplevel,
			(EventMask) 0,
			True,
			_XEditResCheckMessages,
			(XtPointer) NULL
		);
	}
#endif
}


/*
 * post_realize_config
 *	Top-level function to perform set-up and initialization tasks
 *	after realizing all widgets.
 *
 * Args:
 *	m - Pointer to the main widgets structure.
 *	p - Pointer to the main pixmaps structure.
 *
 * Return:
 *	Nothing.
 */
void
post_realize_config(widgets_t *m, pixmaps_t *p)
{
	Display		*display = XtDisplay(m->toplevel);
	int		depth = DefaultDepth(display, DefaultScreen(display));

	/* Make pixmaps for all the button tops */
	make_pixmaps(m, p, depth);

	/* Get WM_DELETE_WINDOW atom */
	set_delw_atom(XmInternAtom(display, "WM_DELETE_WINDOW", False));

	/* HACK: Force mode changes before mapping the main window
	 * to make sure that all widgets are properly managed.
	 */
	geom_main_chgmode(m);
	geom_main_chgmode(m);

	XmProcessTraversal(m->main.playpause_btn, XmTRAVERSE_CURRENT);
}


