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
static char *_wwwwarp_c_ident_ = "@(#)wwwwarp.c	7.68 04/04/20";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "cdinfo_d/cdinfo.h"
#include "cdinfo_d/motd.h"
#include "xmcd_d/callback.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/dbprog.h"
#include "xmcd_d/hotkey.h"
#include "xmcd_d/geom.h"
#include "xmcd_d/userreg.h"
#include "xmcd_d/wwwwarp.h"


extern widgets_t	widgets;
extern appdata_t	app_data;
extern char		*cdisplay;
extern FILE		*errfp;

STATIC w_ent_t		*wwwwarp_menu = NULL;	 /* Ptr to head of menu */
STATIC bool_t		wwwwarp_initted = FALSE; /* Initialization done */


/***********************
 *  internal routines  *
 ***********************/


/*
 * wwwwarp_menu_setup
 *	Function to set up the initial wwwwarp popup menu, based on
 *	the definition in the wwwwarp.cfg file, parsed in libcdinfo.
 *	This function is recursively called to build submenu levels.
 *
 * Args:
 *	parentw - The parent menu widget
 *	menu - The menu entries list as defined by w_ent_t *
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
wwwwarp_menu_setup(Widget parentw, w_ent_t *menu, curstat_t *s)
{
	w_ent_t		*p;
	char		*cp;
	Widget		w,
			w1;
	int		i;
	Arg		arg[10];

	for (p = menu; p != NULL; p = p->nextent) {
		cp = p->desc == NULL ? "" : p->desc;

		/* Set up common resources */
		i = 0;
		XtSetArg(arg[i], XmNinitialResourcesPersistent, False); i++;
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
		if (p->modifier != NULL && p->keyname != NULL &&
		    hotkey_ckkey(p->modifier, p->keyname)) {
			char	acc[STR_BUF_SZ];

			(void) sprintf(acc, "%s<Key>%s",
					p->modifier, p->keyname);

			XtSetArg(arg[i], XmNaccelerator, acc); i++;
			XtSetArg(arg[i], XmNmnemonic,
				 XStringToKeysym(p->keyname)); i++;
		}

		switch (p->type) {
		case WTYPE_TITLE:
			w = XmCreateLabel(parentw, cp, NULL, 0);
			XtManageChild(w);
			p->aux = (void *) w;
			break;

		case WTYPE_SEP:
			XtSetArg(arg[i], XmNseparatorType,
				 XmSHADOW_ETCHED_IN); i++;
			w = XmCreateSeparator(parentw, "separator", arg, i);
			XtManageChild(w);
			p->aux = (void *) w;
			break;

		case WTYPE_SEP2:
			XtSetArg(arg[i], XmNseparatorType, XmDOUBLE_LINE); i++;
			w = XmCreateSeparator(parentw, "separator2", arg, i);
			XtManageChild(w);
			p->aux = (void *) w;
			break;

		case WTYPE_SUBMENU:
			w1 = XmCreatePulldownMenu(parentw, cp, arg, i);

			XtSetArg(arg[i], XmNsubMenuId, w1); i++;
			w = XmCreateCascadeButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;

			/* Recurse into submenu */
			if (!wwwwarp_menu_setup(w1, p->submenu, s))
				return FALSE;
			break;

		case WTYPE_GOTO:
		case WTYPE_DISCOG:
			w = XmCreatePushButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;

			register_activate_cb(w, wwwwarp_go_url, p);
			break;

		case WTYPE_ABOUT:
			w = XmCreatePushButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;

			register_activate_cb(w, cd_about, s);
			break;

		case WTYPE_HELP:
			w = XmCreatePushButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;

			register_activate_cb(w, cd_help_popup, s);
			break;

		case WTYPE_MOTD:
			w = XmCreatePushButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;

			register_activate_cb(w, wwwwarp_motd, s);
			break;

		case WTYPE_BROWSER:
			w = XmCreatePushButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;

			register_activate_cb(w, wwwwarp_go_musicbrowser, p);
			break;

		case WTYPE_GEN:
			XtSetArg(arg[i], XmNsubMenuId,
				 widgets.wwwwarp.genurls_menu); i++;
			w = XmCreateCascadeButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;
			break;

		case WTYPE_ALBUM:
			XtSetArg(arg[i], XmNsubMenuId,
				 widgets.wwwwarp.discurls_menu); i++;
			w = XmCreateCascadeButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;
			break;

		case WTYPE_SUBMITURL:
			w = XmCreatePushButton(parentw, cp, arg, i);
			XtManageChild(w);
			p->aux = (void *) w;

			register_activate_cb(w, wwwwarp_submit_url, s);
			break;

		case WTYPE_NULL:
		default:
			/* Do nothing */
			break;
		}
	}
	return TRUE;
}


/*
 * wwwwarp_url_ckkey
 *	Check a shortcut key string against existing shortcut key strings
 *	in the provided URL list to see if it's been used.
 *
 * Args:
 *	list - The URL list
 *	keyname - The key name string
 *
 * Return:
 *	FALSE - The key name string has been used
 *	TRUE -  The key string has not been used
 */
STATIC bool_t
wwwwarp_url_ckkey(cdinfo_url_t *list, char *keyname)
{
	cdinfo_url_t	*up;

	for (up = list; up != NULL; up = up->next) {
		if (up->keyname != NULL &&
		    util_strcasecmp(up->keyname, keyname) == 0)
			return FALSE;
	}
	return TRUE;
}


/*
 * wwwwarp_set_shortcut
 *	Auto-generate a unique shortcut key if possible, based on the
 *	menu entry's description string.
 *
 * Args:
 *	ulist - The cdinfo_url_list list head
 *	dbp - Pointer to the cdinfo_incore_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
wwwwarp_set_shortcut(cdinfo_url_t *up, cdinfo_incore_t *dbp)
{
	char		*modifier,
			c,
			keystr[2];
	int		i;

	if (up->disptext[0] == '\0')
		return;	/* No description */

	if (up->keyname != NULL)
		return;	/* A shortcut is already assigned */

	/* Pick a unique shortcut character */
	modifier = NULL;
	c = '\0';
	for (i = 0; up->disptext[i] != '\0'; i++) {
		if (!isalnum((int) up->disptext[i]))
			continue;

		keystr[0] = up->disptext[i];
		keystr[1] = '\0';
		modifier = isupper((int) keystr[0]) ? "Shift" : "Alt";

		/* Match against existing wwwWarp menu entries and
		 * main window hotkeys
		 */
		if (cdinfo_wwwwarp_ckkey(keystr) &&
		    hotkey_ckkey(modifier, keystr) &&
		    wwwwarp_url_ckkey(dbp->gen_url_list, keystr) &&
		    wwwwarp_url_ckkey(dbp->disc_url_list, keystr)) {
			c = keystr[0];
			break;
		}
	}

	/* Set the shortcut */
	if (c != '\0') {
		if (!util_newstr(&up->modifier, modifier)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		if (!util_newstr(&up->keyname, keystr)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
	}
}



/*
 * wwwwarp_sel_cfg_menu
 *	Set the sensitivity of the wwwWarp menu entries
 *
 * Args:
 *	menu - Pointer to the menu entry list being configured
 *	dbp - Pointer to the incore cdinfo structure
 *	s - Pointer to the curstat_t structure
 * 
 * Return:
 *	Nothing.
 */
void
wwwwarp_sel_cfg_menu(w_ent_t *menu, cdinfo_incore_t *dbp, curstat_t *s)
{
	cdinfo_url_t	*u,
			*l;
	w_ent_t		*p;
	char		*cp;
	Widget		w;
	int		i,
			len;
	Arg		arg[10];
	Boolean		sens;

	for (p = menu; p != NULL; p = p->nextent) {
		if (p->type == WTYPE_NULL  || p->type == WTYPE_HELP ||
		    p->type == WTYPE_TITLE || p->type == WTYPE_SEP  ||
		    p->type == WTYPE_SEP2)
			continue;

		sens = True;

		if (p->attrib.acnt > 0 && dbp->disc.artist == NULL &&
		    p->attrib.dcnt == 0)
			sens = False;

		if (p->attrib.dcnt > 0 && dbp->disc.title == NULL &&
		    p->attrib.acnt == 0)
			sens = False;

		if (p->attrib.acnt > 0 && dbp->disc.artist == NULL &&
		    p->attrib.dcnt > 0 && dbp->disc.title == NULL)
			sens = False;

		if (p->attrib.tcnt > 0 && dbp->disc.title == NULL)
			sens = False;

		if (p->attrib.ccnt > 0 && dbp->disc.genre == NULL)
			sens = False;

		if (p->attrib.icnt > 0 &&
		    (s->mode == MOD_NODISC || s->mode == MOD_BUSY))
			sens = False;

		if (p->type == WTYPE_GOTO &&
		    app_data.cdinfo_inetoffln && p->arg != NULL &&
		    (util_urlchk(p->arg, &cp, &len) & IS_REMOTE_URL))
			sens = False;

		if (p->type == WTYPE_BROWSER) {
			if (cdinfo_cddb_ver() != 2 ||
			    s->qmode != QMODE_MATCH ||
			    app_data.cdinfo_inetoffln)
				sens = False;
			else if (app_data.auto_musicbrowser) {
				/* Auto-launch the Music Browser */
				wwwwarp_go_musicbrowser(
					(Widget) p->aux,
					(XtPointer) p,
					NULL
				);
			}
		}

		if (p->type == WTYPE_GEN || p->type == WTYPE_ALBUM) {
			l = (p->type == WTYPE_GEN) ?
				dbp->gen_url_list : dbp->disc_url_list;

			if (l == NULL || app_data.cdinfo_inetoffln)
				sens = False;
			else for (u = l; u != NULL; u = u->next) {
				char	*title;
				int	categ_len,
					disptext_len;

				if (u->aux != NULL)
					/* Menu entry already exists */
					continue;

				/* Create menu entry */

				categ_len = (u->categ == NULL) ?
					0 : strlen(u->categ);
				disptext_len = (u->disptext == NULL) ?
					0 : strlen(u->disptext);

				/* Change "Sites" to "Site" if applicable */
				if (categ_len > 0 &&
				    (strcmp(u->categ + categ_len - 5,
					    "Sites") == 0 ||
				     strcmp(u->categ + categ_len - 5,
					    "sites") == 0))
					*(u->categ + categ_len - 1) = '\0';

				title = (char *) MEM_ALLOC("wwwwarp_cddbent",
					categ_len + disptext_len + 4
				);
				if (title == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(title, "%s%s%s%s",
					u->disptext == NULL ? "" : u->disptext,
					u->categ == NULL ? "" : " (",
					u->categ == NULL ? "" : u->categ,
					u->categ == NULL ? "" : ")"
				);

				wwwwarp_set_shortcut(u, dbp);

				i = 0;
				XtSetArg(arg[i], XmNinitialResourcesPersistent,
					 False); i++;
				XtSetArg(arg[i], XmNshadowThickness, 2); i++;
				if (u->modifier != NULL && u->keyname != NULL){
				    char acc[STR_BUF_SZ];

				    (void) sprintf(acc, "%s<Key>%s",
						   u->modifier, u->keyname);

				    XtSetArg(arg[i], XmNaccelerator, acc); i++;
				    XtSetArg(arg[i], XmNmnemonic,
					 XStringToKeysym(u->keyname)); i++;
				}
				w = XmCreatePushButton(
					(p->type == WTYPE_GEN) ?
						widgets.wwwwarp.genurls_menu :
						widgets.wwwwarp.discurls_menu,
					title,
					arg,
					i
				);
				XtManageChild(w);
				u->aux = (void *) w;

				MEM_FREE(title);

				register_activate_cb(w, wwwwarp_go_cddburl, u);
			}
		}

		if (p->type == WTYPE_SUBMITURL) {
			if (cdinfo_cddb_ver() != 2 ||
			    s->qmode != QMODE_MATCH ||
			    app_data.cdinfo_inetoffln)
				sens = False;
		}

		XtSetSensitive((Widget) p->aux, sens);

		if (p->submenu != NULL)
			/* Recurse into submenu */
			wwwwarp_sel_cfg_menu(p->submenu, dbp, s);
	}
}


/***********************
 *   public routines   *
 ***********************/


/*
 * wwwwarp_init
 *	Initialize the wwwWarp subsystem.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
wwwwarp_init(curstat_t *s)
{
	cdinfo_incore_t	*dbp;
	w_ent_t		*p;
	int		i;
	Arg		arg[10];

	/* Set up wwwWarp menu basic structure */
	cdinfo_wwwwarp_parmload();

	dbp = dbprog_curdb(s);

	/* Find the root wwwWarp menu */
	for (p = dbp->wwwwarp_list; p != NULL; p = p->nextmenu) {
		if (strcmp(p->name, "wwwWarp") == 0) {
			wwwwarp_menu = p;
			break;
		}
	}

	if (wwwwarp_menu == NULL) {
		DBGPRN(DBG_GEN)(errfp, "No wwwWarp menu defined in %s.\n",
			WWWWARP_CFG);
		return;
	}

	/* Create pulldown menu widget for the top level wwwWarp selector */
	i = 0;
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
	widgets.wwwwarp.menu = XmCreatePulldownMenu(
		widgets.main.wwwwarp_btn,
		"wwwWarpMenu",
		arg,
		i
	);

	XtVaSetValues(widgets.main.wwwwarp_btn,
		XmNsubMenuId, widgets.wwwwarp.menu,
		NULL
	);

	/* Create general sites menu */
	i = 0;
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
	widgets.wwwwarp.genurls_menu = XmCreatePulldownMenu(
		widgets.main.dbmode_ind,
		"genUrlsMenu",
		arg,
		i
	);

	/* Create album-specific sites menu */
	i = 0;
	XtSetArg(arg[i], XmNshadowThickness, 2); i++;
	widgets.wwwwarp.discurls_menu = XmCreatePulldownMenu(
		widgets.main.dbmode_ind,
		"discUrlsMenu",
		arg,
		i
	);

	/* Set up the menu entries */
	if (!wwwwarp_menu_setup(widgets.wwwwarp.menu, p, s))
		return;

	wwwwarp_initted = TRUE;

	/* Configure the wwwWarp menu entries */
	wwwwarp_sel_cfg(s);
}


/*
 * wwwwarp_sel_cfg
 *	Configure the wwwwarp menu
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 * 
 * Return:
 *	Nothing.
 */
void
wwwwarp_sel_cfg(curstat_t *s)
{
	cdinfo_incore_t	*dbp;
	cdinfo_ret_t	ret;

	if (!wwwwarp_initted)
		return;

	dbp = dbprog_curdb(s);

	/* Pop down wwwWarp menu if necessary */
	if (XtIsManaged(widgets.wwwwarp.menu))
		XtUnmanageChild(widgets.wwwwarp.menu);

	/* Configure the menu items' sensitivity */
	wwwwarp_sel_cfg_menu(wwwwarp_menu, dbp, s);

	if (s->qmode == QMODE_MATCH &&
	    (app_data.discog_mode & DISCOG_GEN_CDINFO)) {
		/* New disc inserted and a CD info query is successful:
		 * generate new discography
		 */
		if ((ret = cdinfo_gen_discog(s)) != 0) {
			DBGPRN(DBG_CDI)(errfp,
				"cdinfo_gen_discog: status=%d arg=%d\n",
				CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
		}
	}
}


/*
 * wwwwarp_disc_url_clear
 *	Clear out album-specific URLs menu entries.  Used when ejecting
 *	the CD, changing CDs, or re-loading the CD info.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
wwwwarp_disc_url_clear(curstat_t *s)
{
	cdinfo_incore_t	*dbp;
	cdinfo_url_t	*p;

	if (!wwwwarp_initted)
		return;

	dbp = dbprog_curdb(s);

	/* Pop down wwwWarp menu if necessary */
	if (XtIsManaged(widgets.wwwwarp.menu))
		XtUnmanageChild(widgets.wwwwarp.menu);

	for (p = dbp->disc_url_list; p != NULL; p = p->next) {
		if (p->aux == NULL)
			continue;

		XtUnmanageChild((Widget) p->aux);
		XtDestroyWidget((Widget) p->aux);
		p->aux = NULL;
	}
}


/**************** vv Callback routines vv ****************/


/*
 * wwwwarp_motd
 *	Message of the day menu button callback function
 */
/*ARGSUSED*/
void
wwwwarp_motd(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Change to busy cursor */
	cd_busycurs(TRUE, CURS_ALL);

	/* Get MOTD */
	motd_get((byte_t *) 1);

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);
}


/*
 * wwwwarp_go_musicbrowser
 *	Music Browser menu button callback function
 */
/*ARGSUSED*/
void
wwwwarp_go_musicbrowser(Widget w, XtPointer client_data, XtPointer call_data)
{
	w_ent_t		*p = (w_ent_t *)(void *) client_data;
	curstat_t	*s = curstat_addr();
	char		*errmsg;
	cdinfo_incore_t	*dbp;
	cdinfo_ret_t	ret;
	int		msglen;

	/* Change to busy cursor */
	cd_busycurs(TRUE, CURS_ALL);

	/* Load CD informtion */
	if ((ret = cdinfo_go_musicbrowser(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_go_musicbrowser: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);

	if (ret == 0) {
		dbp = dbprog_curdb(s);

		if ((dbp->flags & CDINFO_NEEDREG) != 0) {
			/* User not registered with CDDB:
                         * Pop up CDDB user registration dialog
			 */
                        userreg_do_popup(s, TRUE);
                }
	}
	else {
		msglen = strlen(app_data.str_cannotinvoke) +
			 strlen(p->desc) + 4;

		if ((errmsg = (char *) MEM_ALLOC("errmsg", msglen)) == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		(void) sprintf(errmsg, app_data.str_cannotinvoke, p->desc);

		/* Pop up info message dialog */
		CD_INFO(errmsg);

		MEM_FREE(errmsg);
	}
}


/*
 * wwwwarp_go_url
 *	Goto URL menu button callback function
 */
/*ARGSUSED*/
void
wwwwarp_go_url(Widget w, XtPointer client_data, XtPointer call_data)
{
	w_ent_t		*menu = (w_ent_t *)(void *) client_data;
	char		*url,
			*errmsg;
	curstat_t	*s = curstat_addr();
	cdinfo_ret_t	ret;
	int		n,
			trkpos,
			sel_pos,
			msglen;

	if (menu == NULL)
		return;

	/* Change to busy cursor */
	cd_busycurs(TRUE, CURS_ALL);

	if (menu->type == WTYPE_DISCOG &&
	    (app_data.discog_mode & DISCOG_GEN_WWWWARP)) {
		/* Generate new discography */
		if ((ret = cdinfo_gen_discog(s)) != 0) {
			DBGPRN(DBG_CDI)(errfp,
				"cdinfo_gen_discog: status=%d arg=%d\n",
				CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
		}
	}

	/* Do template to URL conversion */
	sel_pos = dbprog_curseltrk(s);
	
	if (sel_pos > 0)
		/* A track is selected: use it for the track title */
		trkpos = sel_pos - 1;
	else
		/* Use current playing track for the track title, if playing */
		trkpos = di_curtrk_pos(s);

	n = cdinfo_url_len(menu->arg, &menu->attrib, &trkpos);

	if ((url = (char *) MEM_ALLOC("wwwwarp_go_url", n)) == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	/* Make the URL from template */
	cdinfo_tmpl_to_url(s, menu->arg, url, trkpos);

	/* Load CD informtion */
	if ((ret = cdinfo_go_url(url)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_go_url: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	MEM_FREE(url);

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);

	if (ret != 0) {
		msglen = strlen(app_data.str_cannotinvoke) +
			 strlen(menu->desc) + 4;

		if ((errmsg = (char *) MEM_ALLOC("errmsg", msglen)) == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		(void) sprintf(errmsg, app_data.str_cannotinvoke, menu->desc);

		/* Pop up info message dialog */
		CD_INFO(errmsg);

		MEM_FREE(errmsg);
	}
}


/*
 * wwwwarp_go_cddburl
 *	CDDB URL menu button callback function
 */
/*ARGSUSED*/
void
wwwwarp_go_cddburl(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = curstat_addr();
	cdinfo_incore_t	*dbp;
	cdinfo_url_t	*p;
	cdinfo_ret_t	ret;
	char		*errmsg;
	int		i;
	int		msglen;

	dbp = dbprog_curdb(s);

	/* Try the general URLs list */
	for (i = 1, p = dbp->gen_url_list; p != NULL; p = p->next, i++) {
		if (p == (cdinfo_url_t *)(void *) client_data)
			break;
	}
	if (p == NULL) {
		/* Try the album-specific URLs list */
		for (i = 1, p = dbp->disc_url_list; p != NULL;
		     p = p->next, i++) {
			if (p == (cdinfo_url_t *)(void *) client_data)
				break;
		}
	}

	if (p == NULL)
		/* This shouldn't happen */
		return;

	/* Change to busy cursor */
	cd_busycurs(TRUE, CURS_ALL);

	/* Load CD informtion */
	if ((ret = cdinfo_go_cddburl(s, p->wtype, i)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_go_cddburl: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);

	if (ret == 0) {
		if ((dbp->flags & CDINFO_NEEDREG) != 0) {
			/* User not registered with CDDB:
                         * Pop up CDDB user registration dialog
			 */
                        userreg_do_popup(s, TRUE);
                }
	}
	else {
		msglen = strlen(app_data.str_cannotinvoke) +
			 strlen(p->disptext) + 4;

		if ((errmsg = (char *) MEM_ALLOC("errmsg", msglen)) == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		(void) sprintf(errmsg, app_data.str_cannotinvoke, p->disptext);

		/* Pop up info message dialog */
		CD_INFO(errmsg);

		MEM_FREE(errmsg);
	}
}


/*
 * wwwwarp_submit_url
 *	CDDB Submit URL menu button callback.
 */
/*ARGSUSED*/
void
wwwwarp_submit_url(Widget w, XtPointer client_data, XtPointer call_data)
{
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.submiturl.form))
		/* Already popped up */
		return;

	set_text_string(widgets.submiturl.categ_txt, "Fan Site", FALSE);
	set_text_string(widgets.submiturl.name_txt, "", FALSE);
	set_text_string(widgets.submiturl.url_txt, "http://", FALSE);
	XmTextSetInsertionPosition(widgets.submiturl.url_txt, 7);
	set_text_string(widgets.submiturl.desc_txt, "", FALSE);
	XtSetSensitive(widgets.submiturl.submit_btn, False);

	XtManageChild(widgets.submiturl.form);

	if (first) {
		first = FALSE;

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.submiturl.form));
	}

	XtMapWidget(XtParent(widgets.submiturl.form));

	/* Put focus on category text */
	XmProcessTraversal(widgets.submiturl.categ_txt, XmTRAVERSE_CURRENT);
}


/*
 * wwwwarp_submit_url_chg
 *	Text field change callback for submit URL window
 */
/*ARGSUSED*/
void
wwwwarp_submit_url_chg(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtSetSensitive(widgets.submiturl.submit_btn, True);
}


/*
 * wwwwarp_submit_url_ok
 *	CDDB Submit URL window Submit button callback.
 */
/*ARGSUSED*/
void
wwwwarp_submit_url_submit(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t		*s = (curstat_t *)(void *) client_data;
	cdinfo_url_t		*up;
	char			*categ,
				*name,
				*url,
				*desc,
				*p;
	cdinfo_ret_t		ret;
	int			len;

	/* Get category and do some checking */
	categ = get_text_string(widgets.submiturl.categ_txt, TRUE);
	if (categ == NULL || *categ == '\0') {
		CD_INFO(app_data.str_nocateg);

		XmProcessTraversal(
			widgets.submiturl.categ_txt, XmTRAVERSE_CURRENT
		);
		return;
	}

	/* Get name and do some checking */
	name = get_text_string(widgets.submiturl.name_txt, TRUE);
	if (name == NULL || *name == '\0') {
		CD_INFO(app_data.str_noname);

		XmProcessTraversal(
			widgets.submiturl.name_txt, XmTRAVERSE_CURRENT
		);
		return;
	}

	/* Get URL and do some checking */
	url = get_text_string(widgets.submiturl.url_txt, TRUE);
	if (url == NULL || *url == '\0' ||
	    ((ret = util_urlchk(url, &p, &len) & IS_REMOTE_URL) == 0)) {
		CD_INFO(app_data.str_invalurl);

		XmProcessTraversal(
			widgets.submiturl.url_txt, XmTRAVERSE_CURRENT
		);
		return;
	}

	/* Get description.  This field is optional */
	desc = get_text_string(widgets.submiturl.desc_txt, TRUE);

	/* Allocate a submiturl structure */
	up = (cdinfo_url_t *)(void *) MEM_ALLOC(
		"cdinfo_submiturl_t", sizeof(cdinfo_url_t)
	);
	if (up == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memset(up, 0, sizeof(cdinfo_url_t));

	/* Fill with data */
	if (!util_newstr(&up->categ, categ)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	MEM_FREE(categ);

	if (!util_newstr(&up->disptext, name)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	MEM_FREE(name);

	if (ret & NEED_PREPEND_HTTP) {
		p = (char *) MEM_ALLOC("url", strlen(url) + 8);
		if (p == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		(void) sprintf(p, "http://%s", url);
		up->href = p;
		set_text_string(widgets.submiturl.url_txt, p, TRUE);
	}
	else if (ret & NEED_PREPEND_FTP) {
		p = (char *) MEM_ALLOC("url", strlen(url) + 7);
		if (p == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		(void) sprintf(p, "ftp://%s", url);
		up->href = p;
		set_text_string(widgets.submiturl.url_txt, p, TRUE);
	}
	else if (ret & NEED_PREPEND_MAILTO) {
		p = (char *) MEM_ALLOC("url", strlen(url) + 8);
		if (p == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		(void) sprintf(p, "mailto:%s", url);
		up->href = p;
		set_text_string(widgets.submiturl.url_txt, p, TRUE);
	}
	else if (!util_newstr(&up->href, url)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	MEM_FREE(url);

	if (!util_newstr(&up->disptext, desc)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	if (desc != NULL)
		MEM_FREE(desc);

	up->type = "menu";

	/* Change to watch cursor */
	cd_busycurs(TRUE, CURS_ALL);

	/* Submit the URL */
	if ((ret = cdinfo_submit_url(s, up)) != 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_submit_url: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));

		CD_INFO(app_data.str_submiterr);

		/* Put focus on OK button */
		XmProcessTraversal(
			widgets.submiturl.ok_btn, XmTRAVERSE_CURRENT
		);
	}
	else
		CD_INFO_AUTO(app_data.str_submitok);

	cd_busycurs(FALSE, CURS_ALL);

	/* Free data structure */
	MEM_FREE(up->categ);
	MEM_FREE(up->disptext);
	MEM_FREE(up->href);
	if (up->desc != NULL)
		MEM_FREE(up->desc);
	MEM_FREE(up);

	if (ret == 0) {
		/* Revert to default */
		set_text_string(
			widgets.submiturl.categ_txt, "Fan Site", FALSE
		);
		set_text_string(
			widgets.submiturl.name_txt, "", FALSE
		);
		set_text_string(
			widgets.submiturl.url_txt, "http://", FALSE
		);
		XmTextSetInsertionPosition(widgets.submiturl.url_txt, 7);
		set_text_string(widgets.submiturl.desc_txt, "", FALSE);
		XtSetSensitive(widgets.submiturl.submit_btn, False);

		/* Put focus on categ text */
		XmProcessTraversal(
			widgets.submiturl.categ_txt, XmTRAVERSE_CURRENT
		);
	}
}


/*
 * wwwwarp_submit_url_ok
 *	CDDB Submit URL window OK button callback.
 */
/*ARGSUSED*/
void
wwwwarp_submit_url_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtUnmapWidget(XtParent(widgets.submiturl.form));
	XtUnmanageChild(widgets.submiturl.form);
}


/*
 * wwwwarp_focus_next
 *	Change focus to the next widget on done
 */
/*ARGSUSED*/
void
wwwwarp_focus_next(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget	nextw;

	if (w == widgets.submiturl.categ_txt)
		nextw = widgets.submiturl.name_txt;
	else if (w == widgets.submiturl.name_txt)
		nextw = widgets.submiturl.url_txt;
	else if (w == widgets.submiturl.url_txt)
		nextw = widgets.submiturl.desc_txt;
	else
		return;

	/* Put the input focus on the next widget */
	XmProcessTraversal(nextw, XmTRAVERSE_CURRENT);
}


/**************** ^^ Callback routines ^^ ****************/

