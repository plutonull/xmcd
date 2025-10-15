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
static char *_hotkey_c_ident_ = "@(#)hotkey.c	6.46 03/12/12";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/hotkey.h"


#define TOTAL_GRABLISTS	2
#define MAIN_LIST	0
#define KEYPAD_LIST	1
#define TOTAL_MEDIA_GRABS 4

#define MAX_MODSTR_LEN	24
#define MAX_KEYSTR_LEN	24
#define MAX_BTNSTR_LEN	48


typedef struct grablist {
	Widget		gr_widget;
	KeyCode		gr_keycode;
	KeySym		gr_keysym;
	Modifiers	gr_modifier;
	char		*gr_modstr;
	char		*gr_keystr;
	char		*gr_acc;
	struct grablist	*next;
} grablist_t;

typedef struct {
	Widget		me_assoc_widget;
	KeyCode		me_keycode;
	KeySym		me_keysym;
	char		*me_keystr;
} media_grab_t;

typedef struct {
	char		*name;
	Modifiers	mask;
} modtab_t;


extern appdata_t	app_data;
extern widgets_t	widgets;
extern FILE		*errfp;

STATIC modtab_t		modtab[] = {
	{ "Shift",	ShiftMask	},
	{ "Lock",	LockMask	},
	{ "Ctrl",	ControlMask	},
	{ "Mod1",	Mod1Mask	},
	{ "Mod2",	Mod2Mask	},
	{ "Mod3",	Mod3Mask	},
	{ "Mod4",	Mod4Mask	},
	{ "Mod5",	Mod5Mask	},
	{ NULL,		0		}
};

STATIC grablist_t	*grablists[TOTAL_GRABLISTS] = {
	NULL, NULL
};
STATIC media_grab_t	media_grablist[TOTAL_MEDIA_GRABS] = {
	{NULL, 0, 0, "XF86AudioStop"},
	{NULL, 0, 0, "XF86AudioPlay"},
	{NULL, 0, 0, "XF86AudioPrev"},
	{NULL, 0, 0, "XF86AudioNext"}
};

/***********************
 *  internal routines  *
 ***********************/


/*
 * hotkey_override_mnemonic
 *	Apply override mnemonic rules, if any.
 *
 * Args:
 *	g - Pointer to the button's associated grablist structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
hotkey_override_mnemonic(grablist_t *g)
{
	char		*lstr;
	XmString	xs;

	if (g->gr_widget == NULL) {
		/* No associated button */
		g->gr_keysym = XK_VoidSymbol;
		return;
	}

	XtVaGetValues(g->gr_widget, XmNlabelString, &xs, NULL);

	if (XmStringGetLtoR(xs, XmSTRING_DEFAULT_CHARSET, &lstr)) {
		/* Make the first letter of the button label
		 * match if possible, even if the
		 * capitalization is wrong.
		 */
		if (g->gr_keystr != NULL &&
		    toupper(g->gr_keystr[0]) == toupper(lstr[0]) &&
		    !(g->gr_modifier & ~ShiftMask)) {
			char	s[2];

			s[0] = lstr[0];
			s[1] = '\0';

			g->gr_keysym = XStringToKeysym(s);
			XtFree(lstr);
			return;
		}
		XtFree(lstr);
	}
}


/*
 * hotkey_set_mnemonics
 *	Set the mnemonic and accelerator of buttons with hotkey support.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
STATIC void
hotkey_set_mnemonics(void)
{
	int		i;
	grablist_t	*p;
	Widget		prev_w = (Widget) NULL;

	/* Set mnemonics and accelerators */
	for (i = 0; i < TOTAL_GRABLISTS; i++) {
		for (p = grablists[i]; p != NULL; p = p->next) {
			if (p->gr_widget != (Widget) NULL) {
				if (prev_w == p->gr_widget)
					continue;

				if (p->gr_keysym != XK_VoidSymbol) {
				    if (isdigit((int) p->gr_keystr[0])) {
					    /* Just set the accelerator */
					    XtVaSetValues(p->gr_widget,
						    XmNaccelerator, p->gr_acc,
						    NULL
					    );
				    }
				    else {
					    /* Set the accelerator and
					     * mnemonic
					     */
					    XtVaSetValues(p->gr_widget,
						    XmNaccelerator, p->gr_acc,
						    XmNmnemonic, p->gr_keysym,
						    NULL
					    );
				    }
				}
			}
			prev_w = p->gr_widget;
		}
	}
}


/*
 * hotkey_parse_xlat_line
 *	A limited translation string parser.
 *
 * Args:
 *	line - The translation text string line to be parsed
 *	modp - The key modifier return string
 *	keyp - The key event return string
 *	widp - The button event return string
 *
 * Return:
 *	Nothing.
 */
STATIC bool_t
hotkey_parse_xlat_line(
	char *line,
	char **modp,
	char **keyp,
	char **widp
)
{
	char		*p,
			*q;
	static char	modstr[MAX_MODSTR_LEN],
			keystr[MAX_KEYSTR_LEN],
			widstr[MAX_BTNSTR_LEN];

	keystr[0] = modstr[0] = widstr[0] = '\0';

	/* Get modifier specification */
	p = line;
	q = strchr(p, '<');

	if (q == NULL)
		return FALSE;
	else if (q > p) {
		*q = '\0';
		(void) strncpy(modstr, p, MAX_MODSTR_LEN-1);
		modstr[MAX_MODSTR_LEN-1] = '\0';
		*modp = modstr;
		*q = '<';
	}

	/* Get event specification */
	p = q + 1;
	q = strchr(p, '>');

	if (q == NULL || q == p)
		return FALSE;
	else {
		*q = '\0';

		/* We are interested only in key events here */
		if (strncmp(p, "Key", 3) != 0)
			return FALSE;

		*q = '>';
	}

	/* Get key specification */
	p = q + 1;
	q = strchr(p, ':');

	if (q == NULL || q == p)
		return FALSE;
	else {
		*q = '\0';
		(void) strncpy(keystr, p, MAX_KEYSTR_LEN-1);
		keystr[MAX_KEYSTR_LEN-1] = '\0';
		*keyp = keystr;
		*q = ':';
	}

	/* Get associated widget name */
	p = q + 1;
	q = strchr(p, '(');

	if (q == NULL || q == p)
		return FALSE;
	else {
		char	sav;

		p = q + 1;
		q = strchr(p, ',');
		if (q == NULL)
			q = strchr(p, ')');

		if (q == NULL || q == p)
			return FALSE;

		sav = *q;
		*q = '\0';
		(void) strncpy(widstr, p, MAX_BTNSTR_LEN-1);
		widstr[MAX_BTNSTR_LEN-1] = '\0';
		*widp = widstr;
		*q = sav;
	}

	return TRUE;
}


/*
 * hotkey_build_grablist
 *	Build a linked list of widgets which have associated
 *	hotkey support, and information about the hotkey.  These
 *	keys will be grabbed when the parent form window has input
 *	focus.
 *
 * Args:
 *	form - The parent form widget
 *	str - The translation string specifying the hotkey
 *	listhead - The list head (return)
 *
 * Return:
 *	Nothing.
 */
STATIC void
hotkey_build_grablist(
	Widget		form,
	char		*str,
	grablist_t	**listhead
)
{
	int		i;
	char		*p,
			*q,
			*end,
			*modstr,
			*keystr,
			*widstr,
			accstr[MAX_MODSTR_LEN + MAX_KEYSTR_LEN + 8];
	grablist_t	*g,
			*prevg;;

	p = str;
	end = p + strlen(p);
	g = prevg = NULL;

	do {
		while (isspace((int) *p))
			p++;
		q = strchr(p, '\n');

		if (p >= end)
			break;

		if (q == NULL) {
			if (q > end)
				break;
		}
		else
			*q = '\0';

		modstr = keystr = widstr = NULL;

		/* Parse translation line */
		if (*p != '#' &&
		    hotkey_parse_xlat_line(p, &modstr, &keystr, &widstr) &&
		    keystr != NULL) {

			/* Allocate new list element */
			g = (grablist_t *)(void *) MEM_ALLOC(
				"grablist_ent", sizeof(grablist_t)
			);
			if (g == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			(void) memset(g, 0, sizeof(grablist_t));

			if (*listhead == NULL)
				*listhead = g;
			else
				prevg->next = g;
			prevg = g;

			if (!util_newstr(&g->gr_modstr, modstr)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			if (!util_newstr(&g->gr_keystr, keystr)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			(void) sprintf(accstr, "%s<Key>%s",
				modstr == NULL ? "" : modstr, keystr);
			if (!util_newstr(&g->gr_acc, accstr)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			g->gr_keysym = XStringToKeysym(keystr);
			g->gr_keycode = XKeysymToKeycode(
				XtDisplay(widgets.toplevel),
				g->gr_keysym
			);

			g->gr_modifier = 0;
			if (modstr != NULL) {
				for (i = 0; modtab[i].name != NULL; i++) {
					if (strcmp(modtab[i].name,
						   modstr) == 0) {
						g->gr_modifier =
							modtab[i].mask;
						break;
					}
				}
			}

			g->gr_widget = (Widget) NULL;
			if (widstr != NULL) {
				g->gr_widget = XtNameToWidget(form, widstr);

				if (g->gr_widget == (Widget) NULL) {
					char	*cp;

					cp = (char *) MEM_ALLOC(
					    "widstr", strlen(widstr) + 2
					);
					if (cp == NULL) {
					    CD_FATAL(app_data.str_nomemory);
					    return;
					}

					(void) sprintf(cp, "*%s", widstr);
					g->gr_widget = XtNameToWidget(
						form, cp
					);

					MEM_FREE(cp);
				}
			}

			/* Special mnemonic handling */
			hotkey_override_mnemonic(g);
		}

		if (q != NULL) {
			*q = '\n';
			p = q + 1;
		}
		else
			p = end;

	} while (p < end);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * hotkey_init
 *	Top level setup function for the hotkey subsystem.  Called
 *	once at program startup.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
hotkey_init(void)
{
	media_grab_t *p;
	int i;
	Display *dpy = XtDisplay(widgets.toplevel);

	/* Set up hot keys for main window */
	if (app_data.main_hotkeys != NULL &&
	    app_data.main_hotkeys[0] != '\0') {
		XtOverrideTranslations(
			widgets.main.form,
			XtParseTranslationTable(app_data.main_hotkeys)
		);

		hotkey_build_grablist(
			widgets.main.form,
			app_data.main_hotkeys,
			&grablists[MAIN_LIST]
		);
	}

	/* Set up hot keys for keypad window */
	if (app_data.keypad_hotkeys != NULL &&
	    app_data.keypad_hotkeys[0] != '\0') {
		XtOverrideTranslations(
			widgets.keypad.form,
			XtParseTranslationTable(app_data.keypad_hotkeys)
		);

		hotkey_build_grablist(
			widgets.keypad.form,
			app_data.keypad_hotkeys,
			&grablists[KEYPAD_LIST]
		);
	}

	/* Grab media keys */
	for(i=0;i<TOTAL_MEDIA_GRABS;i++){
		p = &media_grablist[i];
		p->me_keysym = XStringToKeysym(p->me_keystr);
		p->me_keycode = XKeysymToKeycode(
				dpy,
				p->me_keysym);
		/* There is probably a better way of assigning widgets to keys, but this is how I'm doing it */
		if(strcmp(p->me_keystr,"XF86AudioStop")) p->me_assoc_widget = widgets.main.stop_btn;
		else if(strcmp(p->me_keystr,"XF86AudioPlay")) p->me_assoc_widget = widgets.main.playpause_btn;
		else if(strcmp(p->me_keystr,"XF86AudioNext")) p->me_assoc_widget = widgets.main.nexttrk_btn;
		else if(strcmp(p->me_keystr,"XF86AudioPrev")) p->me_assoc_widget = widgets.main.prevtrk_btn;
		XGrabKey(
			dpy,
			p->me_keycode,
			AnyModifier,
			XDefaultRootWindow(dpy),
			1,
			GrabModeAsync,
			GrabModeAsync
		);
	}

	/* Set mnemonics on hotkey button faces */
	hotkey_set_mnemonics();
}

/*
 * hotkey_root_ungrabkeys
 *	Ungrab all media keys previously grabbed in hotkey_init
 *
 * Args:
 * 	None.
 *
 * Return:
 * 	Nothing.
 */
void
hotkey_media_ungrabkeys(){

	media_grab_t *p;
	int i;
	Display *dpy = XtDisplay(widgets.toplevel);

	for(i=0;i<TOTAL_MEDIA_GRABS;i++){
		p = &media_grablist[i];
		XUngrabKey(
			dpy,
			p->me_keycode,
			AnyModifier,
			XDefaultRootWindow(dpy)
		);
	}

}

/*
 * hotkey_grabkeys
 *	Grab all keys used as hotkeys in the specified window form.
 *
 * Args:
 *	form - The parent form widget.
 *
 * Return:
 *	Nothing.
 */
void
hotkey_grabkeys(Widget form)
{
	grablist_t	*list,
			*p;

	if (form == (Widget) NULL)
		return;

	if (form == widgets.main.form)
		list = grablists[MAIN_LIST];
	else if (form == widgets.keypad.form)
		list = grablists[KEYPAD_LIST];
	else
		list = NULL;

	for (p = list; p != NULL; p = p->next) {
		XtGrabKey(
			form,
			p->gr_keycode,
			p->gr_modifier,
			True,
			GrabModeAsync,
			GrabModeAsync
		);
	}
}


/*
 * hotkey_ungrabkeys
 *	Ungrab all keys used as hotkeys in the specified window form.
 *
 * Args:
 *	form - The parent form widget.
 *
 * Return:
 *	Nothing.
 */
void
hotkey_ungrabkeys(Widget form)
{
	grablist_t	*list,
			*p;

	if (form == (Widget) NULL)
		return;

	if (form == widgets.main.form)
		list = grablists[MAIN_LIST];
	else if (form == widgets.keypad.form)
		list = grablists[KEYPAD_LIST];
	else
		list = NULL;

	for (p = list; p != NULL; p = p->next)
		XtUngrabKey(form, p->gr_keycode, p->gr_modifier);
}


/*
 * hotkey_tooltip_mnemonic
 *	Set the tooltip mnemonic for the specified pushbutton
 *
 * Args:
 *	w - The widget for which the tooltip is being displayed
 *
 * Return:
 *	Nothing.
 */
void
hotkey_tooltip_mnemonic(Widget w)
{
	grablist_t	*p;

	for (p = grablists[MAIN_LIST]; p != NULL; p = p->next) {
		if (p->gr_widget == w) {
			XtVaSetValues(widgets.tooltip.tooltip_lbl,
				XmNmnemonic, p->gr_keysym,
				NULL
			);
			return;
		}
	}

	/* If not found, simply unset the mnemonic */
	XtVaSetValues(widgets.tooltip.tooltip_lbl,
		XmNmnemonic, XK_VoidSymbol,
		NULL
	);
}


/*
 * hotkey_ckkey
 *	Given a modifier/key pair, check to see if they are unused
 *	in the main window hot key system.
 *
 * Args:
 *	modstr - The modifier string (or NULL if not applicable)
 *	keystr - The key string (must not be NULL)
 *
 * Return:
 *	FALSE - The modifier/key pair is used in main window hotkeys
 *	TRUE - The modifier/key pair is unused in main window hotkeys
 */
bool_t
hotkey_ckkey(char *modstr, char *keystr)
{
	grablist_t	*p;
	int		i;

	if (keystr == NULL)
		return FALSE;

	for (i = 0; i < TOTAL_GRABLISTS; i++) {
		for (p = grablists[i]; p != NULL; p = p->next) {
			if (p->gr_keystr != NULL &&
			    util_strcasecmp(p->gr_keystr, keystr) == 0) {
				if (p->gr_modstr == NULL &&
				    modstr == NULL)
					return FALSE;

				if (p->gr_modstr != NULL &&
				    modstr != NULL &&
				    strcmp(p->gr_modstr, modstr) == 0)
					return FALSE;
			}
		}
	}

	return TRUE;
}


/*
 * hotkey
 *	Widget action routine to handle hotkey events
 */
void
hotkey(Widget w, XEvent *ev, String *args, Cardinal *num_args)
{
	int		i;
	Widget		aw;
	grablist_t	*p;
	bool_t		doit;

	if ((int) *num_args <= 0)
		return;	/* Error: should have at least one arg */

	if (ev->xany.type != KeyPress)
		return;	/* Not a keypress event */

	if ((aw = XtNameToWidget(w, args[0])) == (Widget) NULL) {
		char	*cp;

		cp = (char *) MEM_ALLOC("widstr", strlen(args[0]) + 2);
		if (cp == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		(void) sprintf(cp, "*%s", args[0]);
		aw = XtNameToWidget(w, cp);

		MEM_FREE(cp);

		if (aw == (Widget) NULL)
			return;	/* Can't find widget */
	}

	/* Find the associated grablist entry */
	doit = FALSE;
	for (i = 0; i < TOTAL_GRABLISTS; i++) {
		for (p = grablists[i]; p != NULL; p = p->next) {
			if (p->gr_widget != aw)
				continue;

			/* Check to see if we're called as a result of
			 * the user invoking the correct modifier and key.
			 */

			if ((int) p->gr_keycode == (int) ev->xkey.keycode &&
			    (int) p->gr_modifier == (int) ev->xkey.state) {
				doit = TRUE;
				break;
			}
		}
	}

	if (!doit)
		return;

	/* Switch keyboard focus to the widget of interest */
	XmProcessTraversal(aw, XmTRAVERSE_CURRENT);

	for (i = 1; i < (int) *num_args; i++) {
		/* Invoke the named action of the specified widget */
		XtCallActionProc(aw, args[i], ev, NULL, 0);
	}
}


