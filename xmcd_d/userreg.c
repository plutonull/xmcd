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
static char *_userreg_c_ident_ = "@(#)userreg.c	7.31 03/12/12";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/dbprog.h"
#include "xmcd_d/help.h"
#include "xmcd_d/userreg.h"


extern widgets_t	widgets;
extern appdata_t	app_data;
extern FILE		*errfp;


/***********************
 *  internal routines  *
 ***********************/



/***********************
 *   public routines   *
 ***********************/

/*
 * dbprog_structupd
 *	Update userreg window widgets to match the userreg structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
userreg_structupd(curstat_t *s)
{
	cdinfo_incore_t	*dbp = cdinfo_addr();
	char		*p,
			*str;
	int		n;

	if (dbp->userreg.handle != NULL) {
		p = get_text_string(widgets.userreg.handle_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->userreg.handle) != 0) {
			set_text_string(
				widgets.userreg.handle_txt,
				dbp->userreg.handle,
				TRUE
			);

			str = XmTextGetString(widgets.userreg.handle_txt);
			n = strlen(str);
			XtFree(str);

			XmTextSetInsertionPosition(
				widgets.userreg.handle_txt,
				n
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.userreg.handle_txt, "", FALSE);
	}

	if (dbp->userreg.hint != NULL) {
		p = get_text_string(widgets.userreg.hint_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->userreg.hint) != 0) {
			set_text_string(
				widgets.userreg.hint_txt,
				dbp->userreg.hint,
				TRUE
			);

			str = XmTextGetString(widgets.userreg.hint_txt);
			n = strlen(str);
			XtFree(str);

			XmTextSetInsertionPosition(
				widgets.userreg.hint_txt,
				n
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.userreg.hint_txt, "", FALSE);
	}

	if (dbp->userreg.email != NULL) {
		p = get_text_string(widgets.userreg.email_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->userreg.email) != 0) {
			set_text_string(
				widgets.userreg.email_txt,
				dbp->userreg.email,
				TRUE
			);

			str = XmTextGetString(widgets.userreg.email_txt);
			n = strlen(str);
			XtFree(str);

			XmTextSetInsertionPosition(
				widgets.userreg.email_txt,
				n
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.userreg.email_txt, "", FALSE);
	}

	if (dbp->userreg.region != NULL) {
		p = get_text_string(widgets.userreg.region_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->userreg.region) != 0) {
			set_text_string(
				widgets.userreg.region_txt,
				cdinfo_region_name(dbp->userreg.region),
				TRUE
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.userreg.region_txt, "", FALSE);
	}

	if (dbp->userreg.age != NULL) {
		p = get_text_string(widgets.userreg.age_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->userreg.age) != 0) {
			set_text_string(
				widgets.userreg.age_txt,
				dbp->userreg.age,
				TRUE
			);

			str = XmTextGetString(widgets.userreg.age_txt);
			n = strlen(str);
			XtFree(str);

			XmTextSetInsertionPosition(
				widgets.userreg.age_txt,
				n
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.userreg.age_txt, "", FALSE);
	}

	if (dbp->userreg.gender != NULL) {
		if (dbp->userreg.gender[0] == 'm') {
			XmToggleButtonSetState(
				widgets.userreg.male_btn, True, False
			);
		}
		else if (dbp->userreg.gender[0] == 'f') {
			XmToggleButtonSetState(
				widgets.userreg.female_btn, True, False
			);
		}
		else {
			XmToggleButtonSetState(
				widgets.userreg.unspec_btn, True, False
			);
		}
	}
	else {
		XmToggleButtonSetState(
			widgets.userreg.unspec_btn, True, False
		);
	}

	XmToggleButtonSetState(
		widgets.userreg.allowmail_btn,
		(Boolean) dbp->userreg.allowemail,
		False
	);
	XmToggleButtonSetState(
		widgets.userreg.allowstats_btn,
		(Boolean) dbp->userreg.allowstats,
		False
	);
}


/*
 * userreg_do_popup
 *	User registration window pop up
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	from_prog - Denotes that this is a program-initiated popup
 *
 * Return:
 *	Nothing
 */
void
userreg_do_popup(curstat_t *s, bool_t from_prog)
{
	static bool_t	first = TRUE;

	if (!from_prog && XtIsManaged(widgets.userreg.form)) {
		XtUnmapWidget(XtParent(widgets.userreg.form));
		XtUnmanageChild(widgets.userreg.form);
		return;
	}

	/* Update userreg widgets */
	userreg_structupd(s);

	/* The userreg popup has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason for this
	 * is we want to avoid a screen glitch when we move the window
	 * in cd_dialog_setpos(), so we map the window afterwards.
	 */
	XtManageChild(widgets.userreg.form);
	if (first) {
		first = FALSE;

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.userreg.form));
	}
	XtMapWidget(XtParent(widgets.userreg.form));

	/* Put focus on the user handle text field */
	XmProcessTraversal(widgets.userreg.handle_txt, XmTRAVERSE_CURRENT);

	/* Handle X events locally.  This is to prevent from returning
	 * to the caller until registration is completed or canceled by
	 * the user.
	 */
	do {
		util_delayms(10);
		event_loop(0);
	} while (XtIsManaged(widgets.userreg.form));
}


/**************** vv Callback routines vv ****************/


/*
 * userreg_popup
 *	Callback function for the "Register..." button
 */
/*ARGSUSED*/
void
userreg_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	userreg_do_popup(s, FALSE);
}


/*
 * userreg_focus_next
 *	Change focus to the next widget on done
 */
/*ARGSUSED*/
void
userreg_focus_next(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget	nextw;

	if (w == widgets.userreg.handle_txt)
		nextw = widgets.userreg.passwd_txt;
	else if (w == widgets.userreg.passwd_txt)
		nextw = widgets.userreg.vpasswd_txt;
	else if (w == widgets.userreg.vpasswd_txt)
		nextw = widgets.userreg.hint_txt;
	else if (w == widgets.userreg.hint_txt)
		nextw = widgets.userreg.email_txt;
	else if (w == widgets.userreg.email_txt)
		nextw = widgets.userreg.region_chg_btn;
	else if (w == widgets.userreg.postal_txt)
		nextw = widgets.userreg.age_txt;
	else if (w == widgets.userreg.age_txt)
		nextw = widgets.userreg.male_btn;
	else
		return;

	/* Put focus on the next widget */
	XmProcessTraversal(nextw, XmTRAVERSE_CURRENT);
}


/*
 * userreg_gender_sel
 *	User registration gender selector callback
 */
/*ARGSUSED*/
void
userreg_gender_sel(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmRowColumnCallbackStruct	*p =
		(XmRowColumnCallbackStruct *)(void *) call_data;
	XmToggleButtonCallbackStruct	*q;
	cdinfo_incore_t			*dbp = cdinfo_addr();
	char				*str;

	if (p == NULL)
		return;

	q = (XmToggleButtonCallbackStruct *)(void *) p->callbackstruct;

	if (!q->set)
		return;

	if (p->widget == widgets.userreg.male_btn)
		str = "m";
	else if (p->widget == widgets.userreg.female_btn)
		str = "f";
	else
		str = "";

	if (!util_newstr(&dbp->userreg.gender, str)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	/* Put focus on the next widget */
	XmProcessTraversal(widgets.userreg.allowmail_btn, XmTRAVERSE_CURRENT);
}


/*
 * userreg_gethint
 *	User registration "gethint" button callback.
 */
/*ARGSUSED*/
void
userreg_gethint(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_incore_t	*dbp = cdinfo_addr();
	cdinfo_ret_t	ret;

	dbp->userreg.handle = get_text_string(
		widgets.userreg.handle_txt, TRUE
	);
	if (dbp->userreg.handle == NULL) {
		/* Put input focus on the handle text widget */
		XmProcessTraversal(
			widgets.userreg.handle_txt,
			XmTRAVERSE_CURRENT
		);

		CD_INFO(app_data.str_handlereq);
		return;
	}

	/* Switch to watch cursor */
	cd_busycurs(TRUE, CURS_USERREG);

	/* Get password hint */
	if ((ret = cdinfo_getpasshint(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_getpasshint: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Switch to normal cursor */
	cd_busycurs(FALSE, CURS_USERREG);

	switch (CDINFO_GET_STAT(ret)) {
	case 0:
		/* Success */
		CD_INFO(app_data.str_mailinghint);
		break;

	case NAME_ERR:
		CD_INFO(app_data.str_unknhandle);
		break;

	case HINT_ERR:
		CD_INFO(app_data.str_nohint);
		break;

	case MAIL_ERR:
		CD_INFO(app_data.str_nomailhint);
		break;

	default:
		CD_INFO(app_data.str_hinterr);
		break;
	}
}


/*
 * userreg_ok
 *	User registration OK button callback
 */
/*ARGSUSED*/
void
userreg_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_incore_t	*dbp = cdinfo_addr();
	cdinfo_ret_t	ret;

	dbp->userreg.handle = get_text_string(
		widgets.userreg.handle_txt, TRUE
	);
	dbp->userreg.hint = get_text_string(
		widgets.userreg.hint_txt, TRUE
	);
	dbp->userreg.email = get_text_string(
		widgets.userreg.email_txt, TRUE
	);
	dbp->userreg.email = get_text_string(
		widgets.userreg.email_txt, TRUE
	);
	dbp->userreg.postal = get_text_string(
		widgets.userreg.postal_txt, TRUE
	);
	dbp->userreg.age = get_text_string(
		widgets.userreg.age_txt, TRUE
	);

	dbp->userreg.allowemail = (bool_t) XmToggleButtonGetState(
		widgets.userreg.allowmail_btn
	);

	dbp->userreg.allowstats = (bool_t) XmToggleButtonGetState(
		widgets.userreg.allowstats_btn
	);

	/* Sanity check */
	if (dbp->userreg.handle == NULL) {
		/* Put input focus on the user handle text widget */
		XmProcessTraversal(
			widgets.userreg.handle_txt,
			XmTRAVERSE_CURRENT
		);

		CD_INFO(app_data.str_handlereq);
		return;
	}
	else if (dbp->userreg.passwd == NULL) {
		/* Put input focus on the passwd text widget */
		XmProcessTraversal(
			widgets.userreg.passwd_txt,
			XmTRAVERSE_CURRENT
		);

		CD_INFO(app_data.str_passwdreq);
		return;
	}
	else if (dbp->userreg.vpasswd == NULL ||
	         strcmp(dbp->userreg.passwd, dbp->userreg.vpasswd) != 0) {
		/* Put input focus on the passwd text widget */
		XmProcessTraversal(
			widgets.userreg.passwd_txt,
			XmTRAVERSE_CURRENT
		);

		CD_INFO(app_data.str_passwdmatcherr);

		/* Clear out the password fields */
		set_text_string(widgets.userreg.passwd_txt, "", FALSE);
		set_text_string(widgets.userreg.vpasswd_txt, "", FALSE);
		return;
	}

	/* Switch to watch cursor */
	cd_busycurs(TRUE, CURS_USERREG);

	/* Register the user */
	if ((ret = cdinfo_reguser(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_reguser: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Switch to normal cursor */
	cd_busycurs(FALSE, CURS_USERREG);

	switch (CDINFO_GET_STAT(ret)) {
	case 0:
		/* Success */
		break;

	case NAME_ERR:
		/* User handle or password error */
		CD_INFO(app_data.str_handleerr);

		/* Put focus on the user handle text field */
		XmProcessTraversal(
			widgets.userreg.handle_txt,
			XmTRAVERSE_CURRENT
		);
		return;

	default:
		/* Failure */
		CD_INFO(app_data.str_userregfail);
		return;
	}

	/* Registration success - pop down registration form */
	XtUnmapWidget(XtParent(widgets.userreg.form));
	XtUnmanageChild(widgets.userreg.form);

	/* Clear out passwords */
	set_text_string(widgets.userreg.passwd_txt, "", FALSE);
	set_text_string(widgets.userreg.vpasswd_txt, "", FALSE);
	if (dbp->userreg.passwd != NULL) {
		memset(dbp->userreg.passwd, 0, strlen(dbp->userreg.passwd));
		MEM_FREE(dbp->userreg.passwd);
		dbp->userreg.passwd = NULL;
	}
	if (dbp->userreg.vpasswd != NULL) {
		memset(dbp->userreg.vpasswd, 0, strlen(dbp->userreg.vpasswd));
		MEM_FREE(dbp->userreg.vpasswd);
		dbp->userreg.vpasswd = NULL;
	}

	if (s->qmode != QMODE_MATCH) {
		/* Re-load from CDDB */
		dbprog_dbclear(s, TRUE);
		dbprog_dbget(s);
	}
}


/*
 * userreg_privacy
 *	User registration privacy info button callback
 */
/*ARGSUSED*/
void
userreg_privacy(Widget w, XtPointer client_data, XtPointer call_data)
{
	char	path[FILE_PATH_SZ];

	(void) sprintf(path, DOCFILE_PATH, app_data.libdir);;
	(void) strcat(path, "PRIVACY");
	help_loadfile(path);
}


/*
 * userreg_cancel
 *	User registration cancel button callback
 */
/*ARGSUSED*/
void
userreg_cancel(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtUnmapWidget(XtParent(widgets.userreg.form));
	XtUnmanageChild(widgets.userreg.form);
}


