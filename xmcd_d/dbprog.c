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
static char *_dbprog_c_ident_ = "@(#)dbprog.c	7.215 04/04/20";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"
#include "cdinfo_d/motd.h"
#include "xmcd_d/callback.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/wwwwarp.h"
#include "xmcd_d/userreg.h"
#include "xmcd_d/dbprog.h"


extern widgets_t	widgets;
extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		time_mode,		/* Time display mode flag */
			sel_pos = -1,		/* Track list select position */
			ind_pos = -1,		/* Track list highlight pos */
			extt_pos = -1,		/* Selected track position */
			cred_pos = -1,		/* Cred list select position */
			seg_pos = -1,		/* Seg list select position */
			dlist_pos = -1,		/* Disc list select position */
			dlist_mode,		/* Disc list mode */
			fname_mode,		/* Full name window mode */
			reg_mode,		/* Region selector mode */
			credits_mode,		/* Credits window mode */
			hist_cnt = 0,		/* Hist list count */
			reg_cnt = 0,		/* Region list count */
			lang_cnt = 0,		/* Language list count */
			match_cnt = 0,		/* Matchsel list count */
			start_slot = -1,	/* Changer scan start slot */
			scan_slot = -1;		/* Changer scan current slot */
STATIC long		scan_id = -1;		/* Changer scan timeout ID */
STATIC bool_t		title_edited = FALSE,	/* Track title edited flag */
			extt_setup = FALSE,	/* Track details setup */
			auth_initted = FALSE,	/* User entered auth info */
			hist_initted = FALSE,	/* History initialized */
			auto_trk = FALSE,	/* Auto-track */
			sav_mplay = FALSE,	/* Original multiplay mode */
			sav_rev = FALSE,	/* Original reverse mode */
			wwwwarp_cleared = FALSE,/* wwwWarp menu cleared */
			stopload_active = FALSE,/* Stop load dialog active */
			fname_changed = FALSE;	/* Full name text changed */
STATIC XmString		xs_extd_lbl,		/* Disc details lbl */
			xs_extd_lblx,		/* Disc details lbl (*) */
			xs_dcred_lbl,		/* Disc cred lbl */
			xs_dcred_lblx,		/* Disc cred lbl (*) */
			xs_seg_lbl,		/* Segments lbl */
			xs_seg_lblx,		/* Segments lbl (*) */
			xs_extt_lbl,		/* Track details lbl */
			xs_extt_lblx,		/* Track details lbl (*) */
			xs_tcred_lbl,		/* Track credits lbl */
			xs_tcred_lblx,		/* Track credits lbl (*) */
			xs_scred_lbl,		/* Segment credits lbl */
			xs_scred_lblx;		/* Segment credits lbl (*) */
STATIC char		*sav_ttitle;		/* Saved track title */
STATIC cdinfo_incore_t	*dbp;			/* Pointer to CD info struct */
STATIC cdinfo_credit_t	w_cred;			/* Work credit struct */
STATIC cdinfo_segment_t	w_seg;			/* Work segment struct */

/* A copy of the track list data - needed because Motif does not
 * provide something like XmListGetItem(w, pos)
 */
STATIC struct {
	char		*title;
	bool_t		highlighted;
} trklist_copy[MAXTRACK];

/* Function prototypes */
STATIC void		dbprog_hist_new(curstat_t *),
			dbprog_chgr_new(curstat_t *);


/***********************
 *  internal routines  *
 ***********************/


/*
 * dbprog_fullname_ck
 *	Check the fullname structure fields for correctness.  Pop up a
 *	informational message dialog box if it does not pass.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	TRUE  - passed
 *	FALSE - failed
 */
STATIC bool_t
dbprog_fullname_ck(curstat_t *s)
{
	cdinfo_fname_t	*fnp;
	char		*msg,
			buf[STR_BUF_SZ],
			buf2[STR_BUF_SZ];

	switch (fname_mode) {
	case FNAME_DISC:
		fnp = &dbp->disc.artistfname;
		(void) sprintf(buf, "%.31s", app_data.str_albumartist);
		break;

	case FNAME_TRACK:
		if (extt_pos < 0)
			return TRUE;	/* Nothing to check */

		fnp = &dbp->track[extt_pos].artistfname;
		(void) sprintf(buf2, "%.31s", app_data.str_trackartist);
		(void) sprintf(buf, buf2, s->trkinfo[extt_pos].trkno);
		break;

	case FNAME_CREDITS:
		fnp = &w_cred.crinfo.fullname;
		(void) sprintf(buf, "%.31s", app_data.str_credit);
		break;

	default:
		return TRUE;		/* Nothing to check */
	}
	(void) strcat(buf, " full name:\n");

	/* Check fullname for correctness */
	if (fnp->firstname == NULL && fnp->lastname != NULL) {
		msg = (char *) MEM_ALLOC(
			"fullname_ck_msg",
			strlen(buf) + strlen(app_data.str_nofirst) + 1
		);
		if (msg == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) strcpy(msg, buf);
		(void) strcat(msg, app_data.str_nofirst);
		CD_INFO(msg);
		MEM_FREE(msg);

		/* Put the focus on the first name field */
		XmProcessTraversal(
			widgets.fullname.firstname_txt,
			XmTRAVERSE_CURRENT
		);
		return FALSE;
	}
	if (fnp->the != NULL && fnp->firstname == NULL &&
	    fnp->lastname == NULL) {
		msg = (char *) MEM_ALLOC(
			"fullname_ck_msg",
			strlen(buf) + strlen(app_data.str_nofirstlast) + 1
		);
		if (msg == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) strcpy(msg, buf);
		(void) strcat(msg, app_data.str_nofirstlast);
		CD_INFO(msg);
		MEM_FREE(msg);

		/* Put the focus on the first name field */
		XmProcessTraversal(
			widgets.fullname.firstname_txt,
			XmTRAVERSE_CURRENT
		);
		return FALSE;
	}

	return TRUE;
}


/*
 * dbprog_dpytottime
 *	Display the disc total time in the total time indicator
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_dpytottime(curstat_t *s)
{
	XmString	xs;
	char		str[STR_BUF_SZ];
	static char	prev[STR_BUF_SZ];

	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC)
		(void) strcpy(str, "Total: --:--");
	else
		(void) sprintf(str, "Total: %02d:%02d",
				s->discpos_tot.min, s->discpos_tot.sec);

	if (strcmp(str, prev) == 0)
		return;

	xs = create_xmstring(str, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);

	XtVaSetValues(
		widgets.dbprog.tottime_ind,
		XmNlabelString,
		xs,
		NULL
	);

	XmStringFree(xs);

	(void) strcpy(prev, str);
}


/*
 * dbprog_list_autoscroll
 *	Scroll a list if necessary to make the specified item visible.
 *
 * Args:
 *	list - The list widget to operate on
 *	nitems - The total number of items in the list
 *	pos - The list position to make visible
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_list_autoscroll(Widget list, int nitems, int pos)
{
	int	n,
		top_pos,
		bottom_pos,
		vis_cnt;

	if (nitems == 0)
		return;	/* Nothing to do */

	/* Determine range of items visible in current list */
	XtVaGetValues(
		list,
		XmNtopItemPosition, &top_pos,
		XmNvisibleItemCount, &vis_cnt,
		NULL
	);

	bottom_pos = top_pos + vis_cnt - 1;

	/* Try to keep the desired items near the middle of the visible
	 * portion of the list.
	 */
	if (pos < top_pos) {
		/* Scroll up */
		n = pos - (vis_cnt / 2);
		if (n < 1)
			n = 1;
		XmListSetPos(list, n);
	}
	else if (pos > bottom_pos) {
		/* Scroll down */
		n = pos + (vis_cnt / 2);
		if (n > nitems)
			n = nitems;
		XmListSetBottomPos(list, n);
	}
}


/*
 * dbprog_listupd_ent
 *	Update a track entry in the track list.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	pos - Track position to update.
 *	title - Track title string.
 *	newent - Whether this is a new entry or a replacement entry.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_listupd_ent(curstat_t *s, int pos, char *title, bool_t newent)
{
	int		n,
			secs,
			min,
			sec;
	char		*str;
	XmString	xs;
	bool_t		highlighted = FALSE;

	if (s->trkinfo[pos].trkno < 0)
		return;

	if (time_mode == TIME_TOTAL) {
		min = s->trkinfo[pos].min;
		sec = s->trkinfo[pos].sec;
	}
	else {
		secs = ((s->trkinfo[pos+1].min * 60 + s->trkinfo[pos+1].sec) - 
			(s->trkinfo[pos].min * 60 + s->trkinfo[pos].sec));

		/* "Enhanced CD" / "CD Extra" hack */
		if (s->trkinfo[pos].type == TYP_AUDIO &&
		    s->trkinfo[pos+1].addr > s->discpos_tot.addr) {
		    secs -= ((s->trkinfo[pos+1].addr - s->discpos_tot.addr) /
			     FRAME_PER_SEC);
		}

		min = (byte_t) (secs / 60);
		sec = (byte_t) (secs % 60);
	}

	n = strlen(title) + strlen(TRKLIST_FMT) + 1;
	if ((str = (char *) MEM_ALLOC("listupd_ent_str", n)) == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	(void) sprintf(str, TRKLIST_FMT,
		       s->trkinfo[pos].trkno, min, sec, title,
		       (dbp->track[pos].notes != NULL ||
			dbp->track[pos].credit_list != NULL) ?
				ASTERISK_STR : "");

	highlighted = (s->mode != MOD_BUSY &&
		       s->mode != MOD_NODISC &&
		       s->cur_trk >= 0 &&
		       di_curtrk_pos(s) == pos);

	if (!newent && trklist_copy[pos].title != NULL &&
	    strcmp(trklist_copy[pos].title, str) == 0 &&
	    highlighted == trklist_copy[pos].highlighted) {
		/* No change: don't update list to avoid unnecessary flicker */
		MEM_FREE(str);
		return;
	}

	/* Save private copy of tracklist data entry */
	if (!util_newstr(&trklist_copy[pos].title, str)) {
		MEM_FREE(str);
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	trklist_copy[pos].highlighted = highlighted;

	xs = create_xmstring(str, NULL, highlighted ? CHSET2 : CHSET1, TRUE);

	if (newent)
		XmListAddItemUnselected(widgets.dbprog.trk_list, xs, pos+1);
	else
		XmListReplaceItemsPos(widgets.dbprog.trk_list, &xs, 1, pos+1);

	XmStringFree(xs);
	MEM_FREE(str);
}


/*
 * dbprog_listupd_all
 *	Update the track list display to reflect the contents of
 *	the trkinfo table in the curstat_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	newent - Whether this is a new entry or a replacement entry.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_listupd_all(curstat_t *s, bool_t newent)
{
	int	i;

	if (newent)
		XmListDeleteAllItems(widgets.dbprog.trk_list);

	for (i = 0; i < (int) s->tot_trks; i++) {
		/* Update track list entry */
		dbprog_listupd_ent(
			s, i,
			(dbp->track[i].title != NULL) ?
				dbp->track[i].title : UNDEF_STR,
			newent
		);
	}

	/* If this item is previously selected, re-select it */
	if (sel_pos > 0)
		XmListSelectPos(widgets.dbprog.trk_list, sel_pos, False);
	else if (ind_pos > 0)
		XmListSelectPos(widgets.dbprog.trk_list, ind_pos, False);
}


/*
 * dbprog_extd_lblupd
 *	Update the disc details button label to indicate whether there is
 *	text contained in the disc notes.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_extd_lblupd(void)
{
	XmString	xs;
	bool_t		newstate;
	static bool_t	state = FALSE;

	newstate = (bool_t) (dbp->disc.notes != NULL);
	if (newstate) {
		if (state)
			return;		/* no change */
		xs = xs_extd_lblx;
	}
	else {
		if (!state)
			return;		/* no change */
		xs = xs_extd_lbl;
	}

	XtVaSetValues(widgets.dbprog.extd_btn,
		XmNlabelString, xs,
		NULL
	);

	state = newstate;
}


/*
 * dbprog_dcred_lblupd
 *	Update the disc credits button label to indicate whether there are
 *	disc credits defined.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_dcred_lblupd(void)
{
	XmString	xs;
	bool_t		newstate;
	static bool_t	state = FALSE;

	newstate = (bool_t) (dbp->disc.credit_list != NULL);
	if (newstate) {
		if (state)
			return;		/* no change */
		xs = xs_dcred_lblx;
	}
	else {
		if (!state)
			return;		/* no change */
		xs = xs_dcred_lbl;
	}

	XtVaSetValues(widgets.dbprog.dcredits_btn,
		XmNlabelString, xs,
		NULL
	);

	state = newstate;
}


/*
 * dbprog_seg_lblupd
 *	Update the segments button label to indicate whether there are
 *	segments defined.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_seg_lblupd(void)
{
	XmString	xs;
	bool_t		newstate;
	static bool_t	state = FALSE;

	newstate = (bool_t) (dbp->disc.segment_list != NULL);
	if (newstate) {
		if (state)
			return;		/* no change */
		xs = xs_seg_lblx;
	}
	else {
		if (!state)
			return;		/* no change */
		xs = xs_seg_lbl;
	}

	XtVaSetValues(widgets.dbprog.segments_btn,
		XmNlabelString, xs,
		NULL
	);

	state = newstate;
}


/*
 * dbprog_extt_lblupd
 *	Update the track details button label to indicate whether there is
 *	text contained in the track notes or track credits section.
 *
 * Args:
 *	t - Pointer to the cdinfo_track_t structure pertaining to the
 *	    currently selected track.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_extt_lblupd(cdinfo_track_t *t)
{
	XmString	xs;
	bool_t		newstate;
	static bool_t	state = FALSE;

	newstate = (bool_t) (t != NULL && t->notes != NULL);
	if (newstate) {
		if (state)
			return;		/* no change */
		xs = xs_extt_lblx;
	}
	else {
		if (!state)
			return;		/* no change */
		xs = xs_extt_lbl;
	}

	XtVaSetValues(widgets.dbprog.extt_btn,
		XmNlabelString, xs,
		NULL
	);

	state = newstate;
}


/*
 * dbprog_tcred_lblupd
 *	Update the track credits button label to indicate whether there are
 *	track credits defined for the designated track.
 *
 * Args:
 *	t - Pointer to the cdinfo_track_t structure pertaining to the
 *	    currently selected track.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_tcred_lblupd(cdinfo_track_t *t)
{
	XmString	xs;
	bool_t		newstate;
	static bool_t	state = FALSE;

	newstate = (bool_t) (t != NULL && t->credit_list != NULL);
	if (newstate) {
		if (state)
			return;		/* no change */
		xs = xs_tcred_lblx;
	}
	else {
		if (!state)
			return;		/* no change */
		xs = xs_tcred_lbl;
	}

	XtVaSetValues(widgets.dbprog.tcredits_btn,
		XmNlabelString, xs,
		NULL
	);

	state = newstate;
}


/*
 * dbprog_scred_lblupd
 *	Update the segment credits button label to indicate whether there are
 *	segment credits defined.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_scred_lblupd(void)
{
	XmString	xs;
	bool_t		newstate;
	static bool_t	state = FALSE;

	newstate = (bool_t) (w_seg.credit_list != NULL);
	if (newstate) {
		if (state)
			return;		/* no change */
		xs = xs_scred_lblx;
	}
	else {
		if (!state)
			return;		/* no change */
		xs = xs_scred_lbl;
	}

	XtVaSetValues(widgets.segments.credits_btn,
		XmNlabelString, xs,
		NULL
	);

	state = newstate;
}


/*
 * dbprog_set_disc_title
 *	Set the disc title label on the disc details and credits windows
 *
 * Args:
 *	discno - The disc number
 *	artist_str - The artist string
 *	title_str - The title string
 *
 * Return:
 *	Nothing;
 */
STATIC void
dbprog_set_disc_title(int discno, char *artist, char *title)
{
	XmString	xs;
	char		buf[16],
			dtitle[TITLEIND_LEN];

	(void) sprintf(buf, "Disc %d", discno);
	(void) sprintf(dtitle, "%.127s%s%.127s",
			(artist == NULL) ? "" : artist,
			(artist != NULL && title != NULL) ? " / " : "",
			(title == NULL) ? app_data.str_unkndisc : title);

	xs = create_xmstring(buf, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dbextd.discno_lbl,
		XmNlabelString, xs,
		NULL
	);
	XtVaSetValues(widgets.segments.discno_lbl,
		XmNlabelString, xs,
		NULL
	);
	if (credits_mode == CREDITS_DISC) {
		XtVaSetValues(widgets.credits.disctrk_lbl,
			XmNlabelString, xs,
			NULL
		);
	}
	XmStringFree(xs);

	xs = create_xmstring(dtitle, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);

	XtVaSetValues(widgets.dbextd.disc_lbl,
		XmNlabelString, xs,
		NULL
	);
	XtVaSetValues(widgets.segments.disc_lbl,
		XmNlabelString, xs,
		NULL
	);
	if (credits_mode == CREDITS_DISC) {
		XtVaSetValues(widgets.credits.title_lbl,
			XmNlabelString, xs,
			NULL
		);
	}
	XmStringFree(xs);
}


/*
 * dbprog_set_track_title
 *	Set the track title label on the track details window
 *
 * Args:
 *	trkno - The track number
 *	str - The label string
 *
 * Return:
 *	Nothing;
 */
STATIC void
dbprog_set_track_title(int trkno, char *str)
{
	XmString	xs;
	char		buf[16];

	(void) sprintf(buf, "Track %d", trkno);

	xs = create_xmstring(buf, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dbextt.trkno_lbl,
		XmNlabelString, xs,
		NULL
	);
	if (credits_mode == CREDITS_TRACK) {
		XtVaSetValues(widgets.credits.disctrk_lbl,
			XmNlabelString, xs,
			NULL
		);
	}
	XmStringFree(xs);

	xs = create_xmstring(
		str, "<Untitled>", XmSTRING_DEFAULT_CHARSET, TRUE
	);

	XtVaSetValues(widgets.dbextt.trk_lbl,
		XmNlabelString, xs,
		NULL
	);
	if (credits_mode == CREDITS_TRACK) {
		XtVaSetValues(widgets.credits.title_lbl,
			XmNlabelString, xs,
			NULL
		);
	}
	XmStringFree(xs);
}


/*
 * dbprog_set_seg_title
 *	Set the segment title label on the credits window
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing;
 */
STATIC void
dbprog_set_seg_title(void)
{
	XmString	xs;

	if (credits_mode != CREDITS_SEG)
		return;

	XtVaGetValues(widgets.segments.segment_lbl,
		XmNlabelString, &xs,
		NULL
	);
	XtVaSetValues(widgets.credits.disctrk_lbl,
		XmNlabelString, xs,
		NULL
	);
	XmStringFree(xs);

	xs = create_xmstring(
		w_seg.name, "<new_segment>", XmSTRING_DEFAULT_CHARSET, TRUE
	);

	XtVaSetValues(widgets.credits.title_lbl,
		XmNlabelString, xs,
		NULL
	);

	XmStringFree(xs);
}


/*
 * dbprog_extt_autotrk_upd
 *	If auto-track is enabled, display the new track details and
 *	credits info.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	new_pos - The new track list position to go to
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_extt_autotrk_upd(curstat_t *s, int new_pos)
{
	if (auto_trk && new_pos > 0 && sel_pos != new_pos) {
		/* Make track details window go to the new track */
		if (sel_pos > 0) {
			XmListDeselectPos(widgets.dbprog.trk_list, sel_pos);
			sel_pos = -1;
		}
		XmListSelectPos(widgets.dbprog.trk_list, new_pos, True);

		/* Scroll track list if necessary */
		dbprog_list_autoscroll(
			widgets.dbprog.trk_list,
			(int) s->tot_trks,
			new_pos
		);
	}
}


/*
 * dbprog_curgenre
 *	Return genre structure pointers for the current genre setting.
 *
 * Args:
 *	trkpos - -1 for the album genre information, or track position
 *		 (0 based) for genre information about a track.
 *	genrep - Return structure pointer for the primary genre.
 *	subgenrep - Return structure pointer for the primary subgenre.
 *	genre2p - Return structure pointer for the secondary genre.
 *	subgenre2p - Return structure pointer for the secondary subgenre.
 *
 *	Each of the return pointers may be set to NULL if no genre is
 *	currently set.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_curgenre(
	int		trkpos,
	cdinfo_genre_t	**genrep,
	cdinfo_genre_t	**subgenrep,
	cdinfo_genre_t	**genre2p,
	cdinfo_genre_t	**subgenre2p
)
{
	cdinfo_genre_t	*p,
			*q;
	char		*genre,
			*genre2;

	*genrep = *subgenrep = *genre2p = *subgenre2p = NULL;

	if (trkpos == -1) {
		genre = dbp->disc.genre;
		genre2 = dbp->disc.genre2;
	}
	else {
		genre = dbp->track[trkpos].genre;
		genre2 = dbp->track[trkpos].genre2;
	}

	if (genre != NULL) {
		for (p = dbp->genrelist; p != NULL; p = p->next) {
			if (strcmp(p->id, genre) == 0) {
				*genrep = p;
				*subgenrep = NULL;
				break;
			}
			for (q = p->child; q != NULL; q = q->next) {
				if (strcmp(q->id, genre) == 0) {
					*genrep = q->parent;
					*subgenrep = q;
					break;
				}
			}
			if (*genrep != NULL)
				break;
		}
	}
	if (genre2 != NULL) {
		for (p = dbp->genrelist; p != NULL; p = p->next) {
			if (strcmp(p->id, genre2) == 0) {
				*genre2p = p;
				*subgenre2p = NULL;
				break;
			}
			for (q = p->child; q != NULL; q = q->next) {
				if (strcmp(q->id, genre2) == 0) {
					*genre2p = q->parent;
					*subgenre2p = q;
					break;
				}
			}
			if (*genre2p != NULL)
				break;
		}
	}
}


/*
 * dbprog_genreupd
 *	Update the genre selector menus to indicate the currently set
 *	genres.
 *
 * Args:
 *	trkpos - If -1, update the album genre widgets, or specify a
 *		 track position (0 based) to update the track genre
 *		 widgets.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_genreupd(int trkpos)
{
	int			i;
	Dimension		x;
	Arg			arg[10];
	Widget			w;
	cdinfo_genre_t		*p,
				*q,
				*sg,
				*sg2,
				*genrep,
				*subgenrep,
				*genre2p,
				*subgenre2p;
	static cdinfo_genre_t	*save_genre = NULL,
				*save_genre2 = NULL,
				*save_tgenre = NULL,
				*save_tgenre2 = NULL;
	static bool_t		first = TRUE;

	if (dbp->genrelist == NULL)
		return;

	if (first) {
		first = FALSE;

		/* Create primary genre menu entries */
		for (p = dbp->genrelist; p != NULL; p = p->next) {
			/* Create primary genre entries */
			i = 0;
			XtSetArg(arg[i], XmNinitialResourcesPersistent,
				 False); i++;
			XtSetArg(arg[i], XmNshadowThickness, 2); i++;
			p->aux = (void *) XmCreatePushButton(
				widgets.dbextd.genre_menu[0],
				p->name,
				arg,
				i
			);
			p->aux2 = (void *) XmCreatePushButton(
				widgets.dbextd.genre_menu[1],
				p->name,
				arg,
				i
			);
			p->aux3 = (void *) XmCreatePushButton(
				widgets.dbextt.genre_menu[0],
				p->name,
				arg,
				i
			);
			p->aux4 = (void *) XmCreatePushButton(
				widgets.dbextt.genre_menu[1],
				p->name,
				arg,
				i
			);

			XtManageChild((Widget) p->aux);
			XtManageChild((Widget) p->aux2);
			XtManageChild((Widget) p->aux3);
			XtManageChild((Widget) p->aux4);

			register_activate_cb((Widget) p->aux,
					     dbprog_genre_sel, p);
			register_activate_cb((Widget) p->aux2,
					     dbprog_genre_sel, p);
			register_activate_cb((Widget) p->aux3,
					     dbprog_genre_sel, p);
			register_activate_cb((Widget) p->aux4,
					     dbprog_genre_sel, p);
		}
	}

	/* Find current genre and subgenre entries */
	dbprog_curgenre(trkpos, &genrep, &subgenrep, &genre2p, &subgenre2p);

	if (trkpos == -1) {
		sg = save_genre;
		sg2 = save_genre2;
	}
	else {
		sg = save_tgenre;
		sg2 = save_tgenre2;
	}

	/* If the primary genre changed, re-create the associated
	 * sub-genre menu entries
	 */
	if (sg != NULL &&
	    (genrep == NULL || subgenrep == NULL || genrep != sg)) {
		for (q = sg->child; q != NULL; q = q->next) {
			/* Delete old sub-genre entries */
			if (trkpos == -1) {
				if (q->aux != NULL) {
					XtUnmanageChild((Widget) q->aux);
					XtDestroyWidget((Widget) q->aux);
					q->aux = NULL;
				}
			}
			else {
				if (q->aux3 != NULL) {
					XtUnmanageChild((Widget) q->aux3);
					XtDestroyWidget((Widget) q->aux3);
					q->aux3 = NULL;
				}
			}
		}
	}
	if (sg2 != NULL &&
	    (genre2p == NULL || subgenre2p == NULL || genre2p != sg2)) {
		for (q = sg2->child; q != NULL; q = q->next) {
			/* Delete old sub-genre entries */
			if (trkpos == -1) {
				if (q->aux2 != NULL) {
					XtUnmanageChild((Widget) q->aux2);
					XtDestroyWidget((Widget) q->aux2);
					q->aux2 = NULL;
				}
			}
			else {
				if (q->aux4 != NULL) {
					XtUnmanageChild((Widget) q->aux4);
					XtDestroyWidget((Widget) q->aux4);
					q->aux4 = NULL;
				}
			}
		}
	}

	if (genrep != NULL && genrep != sg) {
		for (q = genrep->child; q != NULL; q = q->next) {
			/* Create new sub-genre entries */
			i = 0;
			XtSetArg(arg[i], XmNinitialResourcesPersistent,
				 False); i++;
			XtSetArg(arg[i], XmNshadowThickness, 2); i++;
			if (trkpos == -1) {
				q->aux = (void *) XmCreatePushButton(
					widgets.dbextd.subgenre_menu[0],
					q->name,
					arg,
					i
				);
				XtManageChild((Widget) q->aux);
				register_activate_cb((Widget) q->aux,
						     dbprog_subgenre_sel, q);
			}
			else {
				q->aux3 = (void *) XmCreatePushButton(
					widgets.dbextt.subgenre_menu[0],
					q->name,
					arg,
					i
				);
				XtManageChild((Widget) q->aux3);
				register_activate_cb((Widget) q->aux3,
						     dbprog_subgenre_sel, q);
			}
		}
	}

	if (genre2p != NULL && genre2p != sg2) {
		for (q = genre2p->child; q != NULL; q = q->next) {
			/* Create new sub-genre entries */
			i = 0;
			XtSetArg(arg[i], XmNinitialResourcesPersistent,
				 False); i++;
			XtSetArg(arg[i], XmNshadowThickness, 2); i++;
			if (trkpos == -1) {
				q->aux2 = (void *) XmCreatePushButton(
					widgets.dbextd.subgenre_menu[1],
					q->name,
					arg,
					i
				);
				XtManageChild((Widget) q->aux2);
				register_activate_cb((Widget) q->aux2,
						     dbprog_subgenre_sel, q);
			}
			else {
				q->aux4 = (void *) XmCreatePushButton(
					widgets.dbextt.subgenre_menu[1],
					q->name,
					arg,
					i
				);
				XtManageChild((Widget) q->aux4);
				register_activate_cb((Widget) q->aux4,
						     dbprog_subgenre_sel, q);
			}
		}
	}

	if (genrep != NULL) {
		if (trkpos == -1) {
			if (genrep->aux != NULL)
				XtVaSetValues(widgets.dbextd.genre_opt[0],
					XmNmenuHistory, genrep->aux,
					NULL
				);
		}
		else {
			if (genrep->aux3 != NULL)
				XtVaSetValues(widgets.dbextt.genre_opt[0],
					XmNmenuHistory, genrep->aux3,
					NULL
				);
		}
	}
	else {
		if (trkpos == -1) {
			XtVaSetValues(widgets.dbextd.genre_opt[0],
				XmNmenuHistory,
				widgets.dbextd.genre_none_btn[0],
				NULL
			);
		}
		else {
			XtVaSetValues(widgets.dbextt.genre_opt[0],
				XmNmenuHistory,
				widgets.dbextt.genre_none_btn[0],
				NULL
			);
		}
	}

	if (subgenrep != NULL) {
		if (trkpos == -1) {
			if (subgenrep->aux != NULL)
				XtVaSetValues(widgets.dbextd.subgenre_opt[0],
					XmNmenuHistory, subgenrep->aux,
					NULL
				);
		}
		else {
			if (subgenrep->aux3 != NULL)
				XtVaSetValues(widgets.dbextt.subgenre_opt[0],
					XmNmenuHistory, subgenrep->aux3,
					NULL
				);
		}
	}
	else {
		if (trkpos == -1) {
			XtVaSetValues(widgets.dbextd.subgenre_opt[0],
				XmNmenuHistory,
				widgets.dbextd.subgenre_none_btn[0],
				NULL
			);
		}
		else {
			XtVaSetValues(widgets.dbextt.subgenre_opt[0],
				XmNmenuHistory,
				widgets.dbextt.subgenre_none_btn[0],
				NULL
			);
		}
	}

	if (genre2p != NULL) {
		if (trkpos == -1) {
			if (genre2p->aux2 != NULL)
				XtVaSetValues(widgets.dbextd.genre_opt[1],
					XmNmenuHistory, genre2p->aux2,
					NULL
				);
		}
		else {
			if (genre2p->aux4 != NULL)
				XtVaSetValues(widgets.dbextt.genre_opt[1],
					XmNmenuHistory, genre2p->aux4,
					NULL
				);
		}
	}
	else {
		if (trkpos == -1) {
			XtVaSetValues(widgets.dbextd.genre_opt[1],
				XmNmenuHistory,
				widgets.dbextd.genre_none_btn[1],
				NULL
			);
		}
		else {
			XtVaSetValues(widgets.dbextt.genre_opt[1],
				XmNmenuHistory,
				widgets.dbextt.genre_none_btn[1],
				NULL
			);
		}
	}

	if (subgenre2p != NULL) {
		if (trkpos == -1) {
			if (subgenre2p->aux2 != NULL)
				XtVaSetValues(widgets.dbextd.subgenre_opt[1],
					XmNmenuHistory, subgenre2p->aux2,
					NULL
				);
		}
		else {
			if (subgenre2p->aux4 != NULL)
				XtVaSetValues(widgets.dbextt.subgenre_opt[1],
					XmNmenuHistory, subgenre2p->aux4,
					NULL
				);
		}
	}
	else {
		if (trkpos == -1) {
			XtVaSetValues(widgets.dbextd.subgenre_opt[1],
				XmNmenuHistory,
				widgets.dbextd.subgenre_none_btn[1],
				NULL
			);
		}
		else {
			XtVaSetValues(widgets.dbextt.subgenre_opt[1],
				XmNmenuHistory,
				widgets.dbextt.subgenre_none_btn[1],
				NULL
			);
		}
	}

	if (trkpos == -1) {
		save_genre = genrep;
		save_genre2 = genre2p;
		w = widgets.dbextd.form;
	}
	else {
		save_tgenre = genrep;
		save_tgenre2 = genre2p;
		w = widgets.dbextt.form;
	}

	if (genrep != NULL || genre2p != NULL) {
		/* Hack: Force the subgenre option menu to resize to
		 * match the selected label.
		 */
		XtVaGetValues(w, XmNwidth, &x, NULL);
		XtVaSetValues(w, XmNwidth, x+1, NULL);
		XtVaSetValues(w, XmNwidth, x, NULL);
	}
}


/*
 * dbprog_regionupd
 *	Update the region display to match the current region setting.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_regionupd(void)
{
	cdinfo_region_t	*rp;
	XmString	xs;
	int		i;
	static bool_t	first = TRUE;

	if (dbp->regionlist == NULL) {
		XtSetSensitive(widgets.dbextd.region_chg_btn, False);
		XtSetSensitive(widgets.userreg.region_chg_btn, False);
		return;
	}

	if (first) {
		first = FALSE;
		/* Set up region selector */
		for (i = 1, rp = dbp->regionlist; rp != NULL;
		     i++, rp = rp->next) {
			xs = create_xmstring(rp->name, NULL, CHSET1, TRUE);

			XmListAddItemUnselected(widgets.regionsel.region_list,
						xs, i);
			XmStringFree(xs);
		}
		reg_cnt = i - 1;

		if (reg_cnt > 0) {
			XtSetSensitive(widgets.dbextd.region_chg_btn, True);
			XtSetSensitive(widgets.userreg.region_chg_btn, True);
		}
	}

	if (dbp->disc.region != NULL)
		set_text_string(widgets.dbextd.region_txt,
				cdinfo_region_name(dbp->disc.region),
				TRUE);
	else
		set_text_string(widgets.dbextd.region_txt, "", FALSE);
}


/*
 * dbprog_langupd
 *	Update the language display to match the current language setting.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_langupd(void)
{
	cdinfo_lang_t	*lp;
	XmString	xs;
	int		i;
	static bool_t	first = TRUE;

	if (dbp->langlist == NULL) {
		XtSetSensitive(widgets.dbextd.lang_chg_btn, False);
		return;
	}

	if (first) {
		first = FALSE;
		/* Set up language selector */
		for (i = 1, lp = dbp->langlist; lp != NULL;
		     i++, lp = lp->next) {
			xs = create_xmstring(lp->name, NULL, CHSET1, TRUE);

			XmListAddItemUnselected(widgets.langsel.lang_list,
						xs, i);
			XmStringFree(xs);
		}
		lang_cnt = i - 1;

		if (lang_cnt > 0)
			XtSetSensitive(widgets.dbextd.lang_chg_btn, True);
	}

	if (dbp->disc.lang != NULL)
		set_text_string(widgets.dbextd.lang_txt,
				cdinfo_lang_name(dbp->disc.lang),
				TRUE);
	else
		set_text_string(widgets.dbextd.lang_txt, "", FALSE);
}


/*
 * dbprog_creditupd
 *	Update the credits window widgets
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	pos - Track position (0-based) for track credit, or -1 for
 *	      album credit.
 *
 * Return:
 *	Nothing
 */
STATIC void
dbprog_creditupd(curstat_t *s, int pos)
{
	int			i;
	Arg			arg[10];
	cdinfo_role_t		*p;
	cdinfo_credit_t		*q;
	char			*str;
	XmString		xs;
	static bool_t		first = TRUE;

	if (dbp->rolelist != NULL && first) {
		first = FALSE;

		/* Create primary role menu entries */
		for (p = dbp->rolelist; p != NULL; p = p->next) {
			/* Create primary role entries */
			i = 0;
			XtSetArg(arg[i], XmNinitialResourcesPersistent,
				 False); i++;
			XtSetArg(arg[i], XmNshadowThickness, 2); i++;
			p->aux = (void *) XmCreatePushButton(
				widgets.credits.prirole_menu,
				p->name,
				arg,
				i
			);
			XtManageChild((Widget) p->aux);
			register_activate_cb((Widget) p->aux,
					     dbprog_role_sel, p);
		}
	}

	switch (credits_mode) {
	case CREDITS_DISC:
		q = dbp->disc.credit_list;
		dbprog_set_disc_title(s->cur_disc,
				      dbp->disc.artist,
				      dbp->disc.title);
		break;

	case CREDITS_TRACK:
		if (pos < 0)
			return;	/* Invalid track */
		q = dbp->track[pos].credit_list;
		dbprog_set_track_title(
			s->trkinfo[pos].trkno,
			dbp->track[pos].title
		);
		break;

	case CREDITS_SEG:
		q = w_seg.credit_list;
		dbprog_set_seg_title();
		break;

	default:
		return;	/* Invalid mode */
	}

	/* Update credits list widget */
	XmListDeleteAllItems(widgets.credits.cred_list);
	cred_pos = -1;

	for (i = 1; q != NULL; q = q->next, i++) {
		char	*rolename,
			*name;

		rolename = q->crinfo.role == NULL ? "unknown" :
				cdinfo_role_name(q->crinfo.role->id);
		name = q->crinfo.name == NULL ? "unknown" : q->crinfo.name;

		str = (char *) MEM_ALLOC("credlist_ent",
			strlen(rolename) + strlen(name) + 8
		);
		if (str == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		(void) sprintf(str, CREDITLIST_FMT,
			       name, rolename,
			       q->notes != NULL ? ASTERISK_STR : "");

		xs = create_xmstring(str, NULL, CHSET1, TRUE);

		XmListAddItemUnselected(widgets.credits.cred_list, xs, i);

		XmStringFree(xs);
		MEM_FREE(str);
	}

	/* Set role selectors to "None" */
	XtVaSetValues(
		widgets.credits.prirole_opt,
		XmNmenuHistory, widgets.credits.prirole_none_btn,
		NULL
	);
	XtCallCallbacks(
		widgets.credits.prirole_none_btn,
		XmNactivateCallback, (XtPointer) NULL
	);

	w_cred.crinfo.role = NULL;

	/* Set fields to null */
	/* Explicitly set fullname fields */
	if (w_cred.crinfo.fullname.dispname != NULL) {
		MEM_FREE(w_cred.crinfo.fullname.dispname);
		w_cred.crinfo.fullname.dispname = NULL;
	}
	if (w_cred.crinfo.fullname.lastname != NULL) {
		MEM_FREE(w_cred.crinfo.fullname.lastname);
		w_cred.crinfo.fullname.lastname = NULL;
	}
	if (w_cred.crinfo.fullname.firstname != NULL) {
		MEM_FREE(w_cred.crinfo.fullname.firstname);
		w_cred.crinfo.fullname.firstname = NULL;
	}
	/* The fields are set via callback for these */
	set_text_string(widgets.credits.name_txt, "", FALSE);
	set_text_string(widgets.credits.notes_txt, "", FALSE);

	if (fname_mode == FNAME_CREDITS) {
		/* Update fullname window: note that call_data is passed
		 * a NULL pointer.  The dbprog_fullname function doesn't
		 * use that field currently, but if that is changed in the
		 * future this will have to change to match.
		 */
		fname_changed = FALSE;
		dbprog_fullname(
			widgets.credits.fullname_btn,
			(XtPointer) FALSE, NULL
		);
	}

	XtSetSensitive(widgets.credits.add_btn, False);
	XtSetSensitive(widgets.credits.del_btn, False);
	XtSetSensitive(widgets.credits.mod_btn, False);

	switch (credits_mode) {
	case CREDITS_DISC:
		dbprog_dcred_lblupd();
		break;
	case CREDITS_TRACK:
		dbprog_tcred_lblupd(&dbp->track[pos]);
		dbprog_listupd_ent(s, pos,
				   (dbp->track[pos].title != NULL) ?
					    dbp->track[pos].title : UNDEF_STR,
				   FALSE);
		/* Re-select the list entry if necessary */
		if ((pos + 1) == sel_pos) {
			XmListSelectPos(widgets.dbprog.trk_list,
					sel_pos, False);
		}
		else if ((pos + 1) == ind_pos) {
			XmListSelectPos(widgets.dbprog.trk_list,
					ind_pos, False);
		}
		break;
	case CREDITS_SEG:
		dbprog_scred_lblupd();
		break;
	default:
		break;
	}
}


/*
 * dbprog_credit_ck
 *	Perform sanity checking of a credit structure
 *
 * Args:
 *	p - Pointer to the credit structure to be checked
 *	pos - Track position (0-based) for the credit, or -1 for album credit
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
dbprog_credit_ck(cdinfo_credit_t *cp, int pos, curstat_t *s)
{
	cdinfo_credit_t	*list,
			*p;
	char		buf[16],
			*str;
	int		i,
			j;

	if (cp->crinfo.role == NULL) {
		CD_INFO(app_data.str_needrole);
		return FALSE;
	}
	if (cp->crinfo.name == NULL) {
		CD_INFO(app_data.str_needrolename);
		return FALSE;
	}

	switch (credits_mode) {
	case CREDITS_DISC:
		list = dbp->disc.credit_list;
		break;
	case CREDITS_TRACK:
		if (pos < 0)
			return FALSE;
		list = dbp->track[pos].credit_list;
		break;
	case CREDITS_SEG:
		list = w_seg.credit_list;
		break;
	default:
		return FALSE;
	}

	/* Check to make sure this credit is not a duplicate of an
	 * another credit in the same list
	 */
	i = 0;
	for (p = list; p != NULL; p = p->next) {
		if (cred_pos > 0 && ++i == cred_pos)
			continue; /* Don't check the one being modified */

		if (cp->crinfo.role == p->crinfo.role &&
		    strcmp(cp->crinfo.name, p->crinfo.name) == 0) {
			if (cp->notes != NULL && p->notes != NULL) {
			    if (strcmp(cp->notes, p->notes) == 0) {
				CD_INFO(app_data.str_dupcredit);
				return FALSE;
			    }
			}
			else if (cp->notes == NULL && p->notes == NULL) {
			    CD_INFO(app_data.str_dupcredit);
			    return FALSE;
			}
		}
	}

	if (credits_mode == CREDITS_DISC) {
		/* Check to make sure that the credit is not a duplicate
		 * of a track credit
		 */
		for (j = 0; j < (int) s->tot_trks; j++) {
		    for (p = dbp->track[j].credit_list; p != NULL;
			 p = p->next) {
			if (cp->crinfo.role == p->crinfo.role &&
			    p->crinfo.name != NULL &&
			    strcmp(cp->crinfo.name, p->crinfo.name) == 0) {
			    if (cp->notes != NULL && p->notes != NULL) {
				if (strcmp(cp->notes, p->notes) == 0) {
				    str = (char *) MEM_ALLOC(
					"str_duptrkcredit",
					strlen(app_data.str_duptrkcredit) + 8
				    );
				    (void) sprintf(buf, "%d",
						   s->trkinfo[j].trkno);
				    (void) sprintf(str,
						   app_data.str_duptrkcredit,
						   buf);
				    CD_INFO(str);
				    MEM_FREE(str);
				    return FALSE;
				}
			    }
			    else if (cp->notes == NULL && p->notes == NULL) {
				str = (char *) MEM_ALLOC(
				    "str_duptrkcredit",
				    strlen(app_data.str_duptrkcredit) + 8
				);
				(void) sprintf(buf, "%d", s->trkinfo[j].trkno);
				(void) sprintf(str, app_data.str_duptrkcredit,
					       buf);
				CD_INFO(str);
				MEM_FREE(str);
				return FALSE;
			    }
			}
		    }
		}
	}
	else if (credits_mode == CREDITS_TRACK) {
		/* Check to make sure the credit is not a duplicate of
		 * a disc credit
		 */
		for (p = dbp->disc.credit_list; p != NULL; p = p->next) {
		    if (cp->crinfo.role == p->crinfo.role &&
			p->crinfo.name != NULL &&
			strcmp(cp->crinfo.name, p->crinfo.name) == 0) {
			if (cp->notes != NULL && p->notes != NULL) {
			    if (strcmp(cp->notes, p->notes) == 0) {
				CD_INFO(app_data.str_dupdisccredit);
				return FALSE;
			    }
			}
			else if (cp->notes == NULL && p->notes == NULL) {
			    CD_INFO(app_data.str_dupdisccredit);
			    return FALSE;
			}
		    }
		}
	}

	return TRUE;
}


/*
 * dbprog_segmentupd
 *	Update the segments window widgets
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing
 */
STATIC void
dbprog_segmentupd(curstat_t *s)
{
	int			i;
	cdinfo_segment_t	*q;
	cdinfo_credit_t		*cp,
				*cp2;
	char			*str;
	XmString		xs;

	if (!XtIsManaged(widgets.segments.form))
		/* No need to do anything now */
		return;

	/* Set credits window title if appropriate */
	dbprog_set_seg_title();

	/* Update segment window set button mode */
	dbprog_segments_setmode(s);

	/* Update segments list widget */
	XmListDeleteAllItems(widgets.segments.seg_list);
	seg_pos = -1;

	for (q = dbp->disc.segment_list, i = 1; q != NULL; q = q->next, i++) {
		char	*segname,
			*st,
			*sf,
			*et,
			*ef;

		segname = q->name == NULL ? "unknown" : q->name;
		st = q->start_track == NULL ? "??" : q->start_track;
		sf = q->start_frame == NULL ? "?" : q->start_frame;
		et = q->end_track == NULL ? "??" : q->end_track;
		ef = q->end_frame == NULL ? "?" : q->end_frame;

		str = (char *) MEM_ALLOC("seglist_ent",
			strlen(segname) + strlen(st) + strlen(sf) +
			strlen(et) + strlen(ef) + 8
		);
		if (str == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		(void) sprintf(str, SEGLIST_FMT,
			       segname, st, sf, et, ef,
			       (q->notes != NULL || q->credit_list != NULL) ?
					ASTERISK_STR : "");

		xs = create_xmstring(str, NULL, CHSET1, TRUE);

		XmListAddItemUnselected(widgets.segments.seg_list, xs, i);

		XmStringFree(xs);
		MEM_FREE(str);
	}

	/* Set fields to null.  The fields are set via callback for these */
	set_text_string(widgets.segments.name_txt, "", FALSE);
	set_text_string(widgets.segments.starttrk_txt, "", FALSE);
	set_text_string(widgets.segments.startfrm_txt, "", FALSE);
	set_text_string(widgets.segments.endtrk_txt, "", FALSE);
	set_text_string(widgets.segments.endfrm_txt, "", FALSE);
	set_text_string(widgets.segments.notes_txt, "", FALSE);

	/* Clear credit list */
	for (cp = cp2 = w_seg.credit_list; cp != NULL; cp = cp2) {
		cp2 = cp->next;

		if (cp->crinfo.name != NULL)
			MEM_FREE(cp->crinfo.name);
		if (cp->crinfo.fullname.dispname != NULL)
			MEM_FREE(cp->crinfo.fullname.dispname);
		if (cp->crinfo.fullname.lastname != NULL)
			MEM_FREE(cp->crinfo.fullname.lastname);
		if (cp->crinfo.fullname.firstname != NULL)
			MEM_FREE(cp->crinfo.fullname.firstname);
		if (cp->crinfo.fullname.the != NULL)
			MEM_FREE(cp->crinfo.fullname.the);
		if (cp->notes != NULL)
			MEM_FREE(cp->notes);
		MEM_FREE(cp);
	}
	w_seg.credit_list = NULL;

	XtSetSensitive(widgets.segments.add_btn, False);
	XtSetSensitive(widgets.segments.del_btn, False);
	XtSetSensitive(widgets.segments.mod_btn, False);

	/* Update segments button label if needed */
	dbprog_seg_lblupd();

	s->segplay = SEGP_NONE;
	dpy_progmode(s, FALSE);
}


/*
 * dbprog_segment_ck
 *	Perform sanity checking of a segment structure
 *
 * Args:
 *	p - Pointer to the segment structure to be checked
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
dbprog_segment_ck(cdinfo_segment_t *cp, curstat_t *s)
{
	int		i;
	sword32_t	st,
			et,
			sf,
			ef,
			startaddr,
			endaddr,
			eot;

	if (cp->name == NULL ||
	    cp->start_track == NULL || cp->start_frame == NULL ||
	    cp->end_track == NULL || cp->end_frame == NULL) {
		CD_INFO(app_data.str_incseginfo);
		return FALSE;
	}

	st = atoi(cp->start_track);
	et = atoi(cp->end_track);
	sf = atoi(cp->start_frame);
	ef = atoi(cp->end_frame);

	/* Check start track and frame numbers */
	for (i = 0; i < (int) s->tot_trks; i++) {
		if (s->trkinfo[i].trkno == st)
			break;
	}

	eot = s->trkinfo[i+1].addr;

	/* "Enhanced CD" / "CD Extra" hack */
	if (eot > s->discpos_tot.addr)
		eot = s->discpos_tot.addr;

	if (i == s->tot_trks || sf >= (eot - s->trkinfo[i].addr)) {
		CD_INFO(app_data.str_invseginfo);
		return FALSE;
	}
	startaddr = s->trkinfo[i].addr + sf;

	/* Check end track and frame numbers */
	for (i = 0; i < (int) s->tot_trks; i++) {
		if (s->trkinfo[i].trkno == et)
			break;
	}

	eot = s->trkinfo[i+1].addr;

	/* "Enhanced CD" / "CD Extra" hack */
	if (eot > s->discpos_tot.addr)
		eot = s->discpos_tot.addr;

	if (i == s->tot_trks || ef >= (eot - s->trkinfo[i].addr)) {
		CD_INFO(app_data.str_invseginfo);
		return FALSE;
	}
	endaddr = s->trkinfo[i].addr + ef;

	/* Check start < end */
	if (endaddr <= (startaddr + app_data.min_playblks)) {
		CD_INFO(app_data.str_segposerr);
		return FALSE;
	}

	return TRUE;
}


/*
 * dbprog_extdupd
 *	Update the state of the disc details window widgets to match that
 *	of the cdinfo_incore_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_extdupd(curstat_t *s)
{
	char		*p,
			discid[16];
	XmString	xs;

	/* Sort title */
	if (dbp->disc.sorttitle != NULL) {
		p = get_text_string(widgets.dbextd.sorttitle_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.sorttitle) != 0) {
			set_text_string(widgets.dbextd.sorttitle_txt,
					dbp->disc.sorttitle,
					TRUE);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextd.sorttitle_txt, "", FALSE);

	/* Title "The" */
	if (dbp->disc.title_the != NULL) {
		p = get_text_string(widgets.dbextd.the_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.title_the) != 0) {
			set_text_string(widgets.dbextd.the_txt,
					dbp->disc.title_the,
					TRUE);
			XmToggleButtonSetState(widgets.dbextd.the_btn,
						True, False);
			XtSetSensitive(widgets.dbextd.the_txt, True);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.dbextd.the_txt, "", FALSE);
		XmToggleButtonSetState(widgets.dbextd.the_btn, False, False);
		XtSetSensitive(widgets.dbextd.the_txt, False);
	}

	/* Year */
	if (dbp->disc.year != NULL) {
		p = get_text_string(widgets.dbextd.year_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.year) != 0) {
			set_text_string(widgets.dbextd.year_txt,
					dbp->disc.year,
					TRUE);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextd.year_txt, "", FALSE);

	/* Label */
	if (dbp->disc.label != NULL) {
		p = get_text_string(widgets.dbextd.label_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.label) != 0) {
			set_text_string(widgets.dbextd.label_txt,
					dbp->disc.label,
					TRUE);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextd.label_txt, "", FALSE);

	/* Compilation */
	XmToggleButtonSetState(widgets.dbextd.comp_btn,
			       dbp->disc.compilation, False);

	/* Genres and subgenres */
	dbprog_genreupd(-1);

	/* Disc number in set */
	if (dbp->disc.dnum != NULL) {
		p = get_text_string(widgets.dbextd.dnum_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.dnum) != 0)
			set_text_string(widgets.dbextd.dnum_txt,
					dbp->disc.dnum,
					TRUE);
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextd.dnum_txt, "", FALSE);

	/* Total discs in set */
	if (dbp->disc.tnum != NULL) {
		p = get_text_string(widgets.dbextd.tnum_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.tnum) != 0)
			set_text_string(widgets.dbextd.tnum_txt,
					dbp->disc.tnum,
					TRUE);
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextd.tnum_txt, "", FALSE);

	/* Album Region */
	dbprog_regionupd();

	/* Album Language */
	dbprog_langupd();

	/* Disc notes */
	if (dbp->disc.notes != NULL) {
		p = get_text_string(widgets.dbextd.notes_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.notes) != 0)
			set_text_string(widgets.dbextd.notes_txt,
					dbp->disc.notes,
					TRUE);
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextd.notes_txt, "", FALSE);

	/* Disc ID */
	if (dbp->discid != 0)
		(void) sprintf(discid, "%08x", dbp->discid);
	else
		discid[0] = '\0';
	xs = create_xmstring(discid, NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	XtVaSetValues(widgets.dbextd.discid_ind,
		XmNlabelString, xs,
		NULL
	);
	XmStringFree(xs);

	/* Media catalog number */
	xs = create_xmstring(
		(char *) s->mcn, "-", XmSTRING_DEFAULT_CHARSET, FALSE
	);
	XtVaSetValues(widgets.dbextd.mcn_ind,
		XmNlabelString, xs,
		NULL
	);
	XmStringFree(xs);

	/* Revision */
	xs = create_xmstring(
		dbp->disc.revision, "-", XmSTRING_DEFAULT_CHARSET, TRUE
	);
	XtVaSetValues(widgets.dbextd.rev_ind,
		XmNlabelString, xs,
		NULL
	);
	XmStringFree(xs);

	/* Certifier */
	xs = create_xmstring(
		dbp->disc.certifier, "-", XmSTRING_DEFAULT_CHARSET, TRUE
	);

	XtVaSetValues(widgets.dbextd.cert_ind,
		XmNlabelString, xs,
		NULL
	);
	XmStringFree(xs);

	/* Update disc details window and credits window headings */
	dbprog_set_disc_title(s->cur_disc, dbp->disc.artist, dbp->disc.title);
	dbprog_set_seg_title();

	/* Update button labels */
	dbprog_extd_lblupd();
	dbprog_dcred_lblupd();
	dbprog_seg_lblupd();
}


/*
 * dbprog_exttupd
 *	Update the state of the track details window widgets to match
 *	that of the cdinfo_incore_t structure for the specified track.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	trkpos - Track position.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_exttupd(curstat_t *s, int trkpos)
{
	char		*p;
	XmString	xs;

	/* Set track details track title */
	dbprog_set_track_title(
		s->trkinfo[trkpos].trkno,
		dbp->track[trkpos].title
	);

	/* Sort title */
	if (dbp->track[trkpos].sorttitle != NULL) {
		p = get_text_string(widgets.dbextt.sorttitle_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->track[trkpos].sorttitle) != 0){
			set_text_string(
				widgets.dbextt.sorttitle_txt,
				dbp->track[trkpos].sorttitle,
				TRUE
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextt.sorttitle_txt, "", FALSE);

	/* Title "The" */
	if (dbp->track[trkpos].title_the != NULL) {
		p = get_text_string(widgets.dbextt.the_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->track[trkpos].title_the) != 0){
			set_text_string(widgets.dbextt.the_txt,
					dbp->track[trkpos].title_the,
					TRUE);
			XmToggleButtonSetState(widgets.dbextt.the_btn,
						True, False);
			XtSetSensitive(widgets.dbextt.the_txt, True);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.dbextt.the_txt, "", FALSE);
		XmToggleButtonSetState(widgets.dbextt.the_btn, False, False);
		XtSetSensitive(widgets.dbextt.the_txt, False);
	}

	/* Artist */
	if (dbp->track[trkpos].artist != NULL) {
		p = get_text_string(widgets.dbextt.artist_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->track[trkpos].artist) != 0) {
			set_text_string(
				widgets.dbextt.artist_txt,
				dbp->track[trkpos].artist,
				TRUE
			);
			XtVaSetValues(widgets.dbextt.artist_txt,
				XmNcursorPosition, 0,
				NULL
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextt.artist_txt, "", FALSE);

	/* Year */
	if (dbp->track[trkpos].year != NULL) {
		p = get_text_string(widgets.dbextt.year_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->track[trkpos].year) != 0) {
			set_text_string(
				widgets.dbextt.year_txt,
				dbp->track[trkpos].year,
				TRUE
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextt.year_txt, "", FALSE);

	/* Label */
	if (dbp->track[trkpos].label != NULL) {
		p = get_text_string(widgets.dbextt.label_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->track[trkpos].label) != 0) {
			set_text_string(
				widgets.dbextt.label_txt,
				dbp->track[trkpos].label,
				TRUE
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextt.label_txt, "", FALSE);

	/* BPM */
	if (dbp->track[trkpos].bpm != NULL) {
		p = get_text_string(widgets.dbextt.bpm_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->track[trkpos].bpm) != 0) {
			set_text_string(
				widgets.dbextt.bpm_txt,
				dbp->track[trkpos].bpm,
				TRUE
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextt.bpm_txt, "", FALSE);

	/* Genres and subgenres */
	dbprog_genreupd(trkpos);

	/* Track notes */
	if (dbp->track[trkpos].notes != NULL) {
		p = get_text_string(widgets.dbextt.notes_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->track[trkpos].notes) != 0) {
			set_text_string(
				widgets.dbextt.notes_txt,
				dbp->track[trkpos].notes,
				TRUE
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else
		set_text_string(widgets.dbextt.notes_txt, "", FALSE);

	/* ISRC */
	xs = create_xmstring(
		dbp->track[trkpos].isrc, "-", XmSTRING_DEFAULT_CHARSET, TRUE
	);
	XtVaSetValues(widgets.dbextt.isrc_ind,
		XmNlabelString, xs,
		NULL
	);
	XmStringFree(xs);
}


/*
 * dbprog_structupd
 *	Update the state of the various widgets fields in the
 *	CD info/program window to match that of the cdinfo_incore_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_structupd(curstat_t *s)
{
	char	*p;

	/* Total time */
	dbprog_dpytottime(s);

	/* Disc artist */
	if (dbp->disc.artist != NULL) {
		p = get_text_string(widgets.dbprog.artist_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.artist) != 0) {
			set_text_string(widgets.dbprog.artist_txt,
					dbp->disc.artist,
					TRUE);
			XtVaSetValues(widgets.dbprog.artist_txt,
				XmNcursorPosition, 0,
				NULL
			);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.dbprog.artist_txt, "", FALSE);
	}

	/* Disc title */
	if (dbp->disc.title != NULL) {
		p = get_text_string(widgets.dbprog.title_txt, TRUE);
		if (p == NULL || strcmp(p, dbp->disc.title) != 0) {
			set_text_string(widgets.dbprog.title_txt,
					dbp->disc.title,
					TRUE);
		}
		if (p != NULL)
			MEM_FREE(p);
	}
	else {
		set_text_string(widgets.dbprog.title_txt, "", FALSE);
	}

	/* Track title list */
	sel_pos = -1;
	dbprog_listupd_all(s, TRUE);

	/* Program sequence */
	if (dbp->playorder != NULL && !s->onetrk_prog) {
		p = get_text_string(widgets.dbprog.pgmseq_txt, FALSE);
		if (p == NULL || strcmp(p, dbp->playorder) != 0) {
			char	*str;
			int	n;

			set_text_string(widgets.dbprog.pgmseq_txt,
					dbp->playorder,
					FALSE);

			str = XmTextGetString(widgets.dbprog.pgmseq_txt);
			n = strlen(str);
			XtFree(str);

			XmTextSetInsertionPosition(
				widgets.dbprog.pgmseq_txt,
				n
			);
		}
		if (p != NULL)
			MEM_FREE(p);
		XtSetSensitive(widgets.dbprog.clrpgm_btn, True);
	}
	else {
		set_text_string(widgets.dbprog.pgmseq_txt, "", FALSE);
		XtSetSensitive(widgets.dbprog.clrpgm_btn, False);
		XtSetSensitive(widgets.dbprog.savepgm_btn, False);
	}

	/* Update the disc details window widgets */
	dbprog_extdupd(s);

	/* Update button labels */
	dbprog_extt_lblupd(NULL);
	dbprog_tcred_lblupd(NULL);

	/* Update segments window if needed */
	dbprog_segmentupd(s);

	/* Update credits window if needed */
	switch (credits_mode) {
	case CREDITS_DISC:
	case CREDITS_SEG:
		dbprog_creditupd(s, -1);
		break;
	case CREDITS_TRACK:
		dbprog_creditupd(s, extt_pos);
		break;
	default:
		break;
	}

	/* Note: extt, credits and fullname window widgets are updated when
	 * the user pops up these windows, repectively.
	 */

	dbp->flags &= ~CDINFO_CHANGED;
	XtSetSensitive(widgets.dbprog.submit_btn, False);
	XtSetSensitive(widgets.dbprog.addpgm_btn, False);
	XtSetSensitive(widgets.dbprog.fullname_btn, True);
	XtSetSensitive(widgets.dbprog.extd_btn, True);
	XtSetSensitive(widgets.dbprog.dcredits_btn, True);
	XtSetSensitive(widgets.dbprog.segments_btn, True);
	XtSetSensitive(widgets.dbprog.extt_btn, False);
	XtSetSensitive(widgets.dbprog.tcredits_btn, False);

	/* Update display */
	dpy_dbmode(s, FALSE);
}


/*
 * dbprog_do_submit_popup
 *	Pop up the CDDB submit prompt dialog.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
dbprog_do_submit_popup(void)
{
	char	*msg;

	msg = (char *) MEM_ALLOC(
		"msg",
		strlen(app_data.str_chgsubmit) +
		(dbp->disc.artist == NULL ? 0 : strlen(dbp->disc.artist)) +
		(dbp->disc.title == NULL ? 0 : strlen(dbp->disc.title)) + 4
	);

	if (msg == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	(void) sprintf(msg, app_data.str_chgsubmit,
		       dbp->disc.artist == NULL ?
				app_data.str_unknartist : dbp->disc.artist,
		       dbp->disc.title == NULL ?
				app_data.str_unkndisc : dbp->disc.title);

	/* Pop-up the dialog box */
	(void) cd_confirm_popup(
		app_data.str_confirm, msg,
		dbprog_do_clear, (XtPointer) STAT_SUBMIT,
		dbprog_do_clear, (XtPointer) 0
	);

	MEM_FREE(msg);
}


/*
 * dbprog_dbsubmit
 *	Submit current CD info to CDDB server.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
dbprog_dbsubmit(curstat_t *s)
{
	cdinfo_ret_t	ret;
	bool_t		err;

	/* Check fullname for correctness, if needed */
	if (!dbprog_fullname_ck(s))
		return FALSE;

	/* Change to the watch cursor */
	cd_busycurs(TRUE, CURS_ALL);

	/* Configure the wwwWarp menu */
	wwwwarp_sel_cfg(s);

	/* Submit the CD information */
	if ((ret = cdinfo_submit(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_submit: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Change to the normal cursor */
	cd_busycurs(FALSE, CURS_ALL);

	switch (CDINFO_GET_STAT(ret)) {
	case 0:
		if ((dbp->flags & CDINFO_NEEDREG) != 0) {
			err = TRUE;

			/* User not registered with CDDB:
			 * Pop up CDDB user registration dialog
			 */
			userreg_do_popup(s, TRUE);
		}
		else {
			err = FALSE;

			/* Clear changed flag */
			dbp->flags &= ~CDINFO_CHANGED;

			/* Make the submit button insensitive */
			XtSetSensitive(widgets.dbprog.submit_btn, False);

			CD_INFO_AUTO(app_data.str_submitok);
		}
		break;

	case SUBMIT_ERR:
	default:
		err = TRUE;

		CD_INFO(app_data.str_submiterr);
		break;
	}

	if (XtIsManaged(widgets.dbprog.form))
		/* Put focus on the OK button */
		XmProcessTraversal(widgets.dbprog.ok_btn, XmTRAVERSE_CURRENT);

	return (!err);
}


/*
 * dbprog_pgm_active
 *	Indicate whether a play program is currently defined.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE = program is active,
 *	FALSE = program is not active.
 */
STATIC bool_t
dbprog_pgm_active(void)
{
	return (dbp->playorder != NULL && dbp->playorder[0] != '\0');
}


/*
 * dbprog_hist_addent
 *	Add a new entry to the disc list widget in history mode.
 *
 * Args:
 *	hp - Pointer to the associated cdinfo_dlist_t structure.
 *	s - Pointer to the curstat_t structure.
 *	pos - The list position to add to.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
dbprog_hist_addent(cdinfo_dlist_t *hp, curstat_t *s, int pos)
{
	struct tm	*tm;
	XmString	xs;
	char		*cp,
			str[DLIST_BUF_SZ + STR_BUF_SZ];

#if !defined(__VMS) || ((__VMS_VER >= 70000000) && (__DECC_VER > 50230003))
	tzset();
#endif
	tm = localtime(&hp->time);
	cp = util_monname(tm->tm_mon);

	(void) sprintf(str, HISTLIST_FMT,
		cp,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		(hp->artist == NULL) ? "" : hp->artist,
		(hp->artist != NULL && hp->title != NULL) ? " / " : "",
		(hp->title == NULL) ? "-" : hp->title
	);

	xs = create_xmstring(str, NULL, CHSET1, TRUE);

	XmListAddItemUnselected(widgets.dlist.disc_list, xs, pos);

	XmStringFree(xs);

	/* Check against max history limit */
	if (++hist_cnt > app_data.cdinfo_maxhist) {
		XmListDeletePos(widgets.dlist.disc_list, hist_cnt);
		hist_cnt--;
	}
}


/*
 * dbprog_hist_addall
 *	Make the disc list widget display the in-core history list.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_hist_addall(curstat_t *s)
{
	int		i;
	cdinfo_dlist_t	*hp;

	/* Put the history list in the list widget */
	for (i = 1, hp = cdinfo_hist_list(); hp != NULL; hp = hp->next, i++) {
		dbprog_hist_addent(hp, s, i);

		if (i == 1)
			XtSetSensitive(widgets.dlist.delall_btn, True);
	}
}


/*
 * dbprog_hist_new
 *	Add current CD to the history list.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_hist_new(curstat_t *s)
{
	cdinfo_dlist_t	h1,
			h2,
			*hp;

	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY || s->chgrscan)
		/* Don't add to history if no disc or changer scanning */
		return;

	h1.device = s->curdev;
	h1.discno = (int) s->cur_disc;
	h1.type = (s->qmode != QMODE_MATCH) ? 0 :
		((dbp->flags & (CDINFO_FROMLOC | CDINFO_FROMCDT)) ?
			CDINFO_DLIST_LOCAL : CDINFO_DLIST_REMOTE);
	h1.discid = dbp->discid;
	h1.genre = dbp->disc.genre;
	h1.artist = dbp->disc.artist;
	h1.title = dbp->disc.title;
	h1.time = time(NULL);

	/* Save a copy of the old entry for comparison later */
	if ((hp = cdinfo_hist_list()) != NULL)
		h2 = *hp;	/* Structure copy */
	else
		(void) memset(&h2, 0, sizeof(cdinfo_dlist_t));

	/* Add to in-core history list */
	(void) cdinfo_hist_addent(&h1, TRUE);

	/* If in history mode, add to disc list widget */
	if (dlist_mode == DLIST_HIST) {
		if (hist_initted && h1.discid != h2.discid) {
			/* Add entry to the list widget,
			 * if it's different than what's
			 * already there.
			 */
			dbprog_hist_addent(&h1, s, 1);
		}

		if (!XtIsSensitive(widgets.dlist.delall_btn))
			XtSetSensitive(widgets.dlist.delall_btn, True);
	}
}


/*
 * dbprog_chgr_scan_next
 *	Scan next slot of the CD changer.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_chgr_scan_next(curstat_t *s)
{
	scan_id = -1;

	if (scan_slot < 0)
		return;	/* Error */

	/* Skip forward to the next loaded slot */
	if (scan_slot > 0 && s->cur_disc > scan_slot)
		scan_slot = s->cur_disc;

	if (++scan_slot > app_data.numdiscs) {
		/* Done scanning */
		scan_slot = -1;
		dbprog_chgr_scan_stop(s);
		return;
	}

	/* Go to next slot */
	s->cur_disc = scan_slot;

	s->flags |= STAT_CHGDISC;

	/* Ask the user if the changed CD information
	 * should be submitted to CDDB.
	 */
	if (!dbprog_chgsubmit(s))
		return;

	s->flags &= ~STAT_CHGDISC;

	/* Do the disc change */
	di_chgdisc(s);

	/* Update display */
	dpy_dbmode(s, FALSE);
	dpy_playmode(s, FALSE);
}


/*
 * dbprog_chgr_addent
 *	Add or update an entry in the disc list widget in CD changer mode
 *
 * Args:
 *	cp - Pointer to the associated cdinfo_dlist_t structure.
 *	s - Pointer to the curstat_t structure.
 *	pos - List position to add to.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_chgr_addent(cdinfo_dlist_t *cp, curstat_t *s, int pos)
{
	XmString	xs;
	char		str[DLIST_BUF_SZ + STR_BUF_SZ];

	(void) sprintf(str, CHGRLIST_FMT,
		cp->discno,
		(cp->artist == NULL) ? "" : cp->artist,
		(cp->artist != NULL && cp->title != NULL) ? " / " : "",
		(cp->title == NULL) ? "-" : cp->title
	);

	xs = create_xmstring(
		str, NULL, (pos == s->cur_disc) ? CHSET2 : CHSET1, TRUE
	);

	if (cp->aux == NULL) {
		/* New entry */
		cp->aux = (void *) widgets.dlist.disc_list;
		XmListAddItemUnselected(widgets.dlist.disc_list, xs, pos);
	}
	else {
		/* Replace existing entry */
		XmListReplaceItemsPos(widgets.dlist.disc_list, &xs, 1, pos);
	}

	XmStringFree(xs);

	if (dlist_pos >= 0)
		/* This entry was previously selected */
		XmListSelectPos(widgets.dlist.disc_list, dlist_pos, False);
}


/*
 * dbprog_chgr_addall
 *	Make the disc list widget display the in-core CD changer list.
 *
 * Args:
 *	s- Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_chgr_addall(curstat_t *s)
{
	cdinfo_dlist_t	*cp;

	/* Put the CD changer list in the list widget */
	for (cp = cdinfo_chgr_list(); cp != NULL; cp = cp->next) {
		cp->aux = NULL;	/* Denote new entry */

		dbprog_chgr_addent(cp, s, cp->discno);
	}

	dbprog_list_autoscroll(
		widgets.dlist.disc_list,
		(int) app_data.numdiscs,
		s->cur_disc
	);
}


/*
 * dbprog_chgr_new
 *	Add current CD to the CD changer list.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_chgr_new(curstat_t *s)
{
	int		i;
	cdinfo_dlist_t	ch,
			*cp;

	if (s->cur_disc == 0)
		return;

	ch.device = s->curdev;
	ch.discno = (int) s->cur_disc;
	ch.type = (s->qmode != QMODE_MATCH) ? 0 :
		((dbp->flags & (CDINFO_FROMLOC | CDINFO_FROMCDT)) ?
			CDINFO_DLIST_LOCAL : CDINFO_DLIST_REMOTE);
	ch.discid = dbp->discid;
	ch.genre = dbp->disc.genre;
	ch.artist = dbp->disc.artist;
	ch.title = dbp->disc.title;
	ch.time = time(NULL);

	/* Add to in-core CD changer list */
	(void) cdinfo_chgr_addent(&ch);

	if (dlist_mode != DLIST_CHGR)
		return;

	/* If in changer mode, add to disc list widget */
	for (i = 1, cp = cdinfo_chgr_list(); cp != NULL; cp = cp->next, i++)
		dbprog_chgr_addent(cp, s, i);

	/* Scroll to make current disc visible if no item
	 * of the list is currently selected.
	 */
	if (dlist_pos < 0) {
		dbprog_list_autoscroll(
			widgets.dlist.disc_list,
			(int) app_data.numdiscs,
			s->cur_disc
		);
	}

	if (s->chgrscan && s->mode != MOD_NODISC && s->mode != MOD_BUSY) {
		/* Schedule for the next slot */
		scan_id = cd_timeout(
			1000,
			dbprog_chgr_scan_next,
			(byte_t *) s
		);
	}
}


/*
 * dbprog_matchsel_popup
 *	Pop up the match selector window.  Note that this function
 *	does not return to the caller until a user response is received.
 *
 * Args:
 *	s- Pointer to the curstat_t structure.
 *
 * Return:
 *	None.
 */
STATIC void
dbprog_matchsel_popup(curstat_t *s)
{
	int		i,
			genrestr_len;
	cdinfo_match_t	*mp;
	char		*str,
			*genrestr;
	XmString	xs;

	if (dbp->matchlist == NULL) {
		/* Shouldn't be calling this when matchlist is empty */
		cd_beep();
		return;
	}

	/* Build matchsel list entries */
	mp = dbp->matchlist;
	for (i = 1; mp != NULL; i++, mp = mp->next) {
		if (mp->genre != NULL) {
			genrestr = cdinfo_genre_name(mp->genre);
			genrestr_len = strlen(genrestr);
		}
		else {
			genrestr = NULL;
			genrestr_len = 0;
		}

		str = (char *) MEM_ALLOC("match_str",
			(mp->artist == NULL ?
				strlen(app_data.str_unknartist) :
				strlen(mp->artist)) +
			(mp->title == NULL ?
				strlen(app_data.str_unkndisc) :
				strlen(mp->title)) +
			genrestr_len + 8
		);
		if (str == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		(void) sprintf(str, MATCHLIST_FMT,
			mp->artist == NULL ?
				app_data.str_unknartist : mp->artist,
			mp->title == NULL ?
				app_data.str_unkndisc : mp->title,
			genrestr == NULL ? "" : "(",
			genrestr == NULL ? "" : genrestr,
			genrestr == NULL ? "" : ")"
		);

		xs = create_xmstring(str, NULL, CHSET1, TRUE);

		XmListAddItemUnselected(widgets.matchsel.matchsel_list, xs, i);

		XmStringFree(xs);
		MEM_FREE(str);
	}

	xs = create_xmstring(
		app_data.str_noneofabove,
		NULL,
		XmSTRING_DEFAULT_CHARSET,
		FALSE
	);

	XmListAddItemUnselected(widgets.matchsel.matchsel_list, xs, i);

	XmStringFree(xs);

	match_cnt = i;

	/* Configure the wwwWarp menu */
	wwwwarp_sel_cfg(s);

	XtSetSensitive(widgets.matchsel.ok_btn, False);

	/* The matchsel popup has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason for this
	 * is we want to avoid a screen glitch when we move the window
	 * in cd_dialog_setpos(), so we map the window afterwards.
	 */
	if (!XtIsManaged(widgets.matchsel.form)) {
		XtManageChild(widgets.matchsel.form);

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.matchsel.form));

		XtMapWidget(XtParent(widgets.matchsel.form));
	}

	/* Handle X events locally to prevent from returning to the
	 * caller until a user response has been received.
	 */
	do {
		util_delayms(10);
		event_loop(0);
	} while (XtIsManaged(widgets.matchsel.form));
}


/*
 * dbprog_auth_popup
 *	Pop up the proxy authorization dialog box.  Note that this
 *	function does not return to the caller until a user response
 *	is received.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
dbprog_auth_popup(void)
{
	/* Pop up authorization dialog.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	if (!XtIsManaged(widgets.auth.form)) {
		XtManageChild(widgets.auth.form);

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.auth.form));

		XtMapWidget(XtParent(widgets.auth.form));
	}

	/* Set keyboard focus to the user name field */
	XmProcessTraversal(
		widgets.auth.name_txt,
		XmTRAVERSE_CURRENT
	);

	/* Handle X events locally to prevent from returning to the caller
	 * until we receive a response from the user.
	 */
	do {
		util_delayms(10);
		event_loop(0);
	} while (XtIsManaged(widgets.auth.form));
}


/*
 * dbprog_autoname
 *	Auto-generate the dispname of a fullname from the lastname,
 *	firstname and the components.
 *
 * Args:
 *	Pointer to the fullname structure.
 *
 * Return:
 *	The generated text string.  This string should be freed by the
 *	caller via MEM_FREE when done.  If memory allocation for the
 *	generated string failed, this call will return NULL.
 */
STATIC char *
dbprog_autoname(cdinfo_fname_t *fnp)
{
	char	*str;

	str = (char *) MEM_ALLOC("dispname",
		(fnp->the == NULL ? 0 : (strlen(fnp->the) + 1)) +
		(fnp->lastname == NULL ? 0 : (strlen(fnp->lastname) + 1)) +
		(fnp->firstname == NULL ? 0 : (strlen(fnp->firstname) + 1)) + 1
	);
	if (str == NULL)
		return NULL;

	str[0] = '\0';

	if (fnp->the != NULL)
		(void) sprintf(str, "%s ", fnp->the);
	if (fnp->firstname != NULL)
		(void) strcat(str, fnp->firstname);
	if (fnp->lastname != NULL)
		(void) sprintf(str, "%s%s%s",
			       str,
			       fnp->firstname == NULL ? "" : " ",
			       fnp->lastname);
	return (str);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * dbprog_curfileupd
 *	Update the curr.XXX file to show the current disc status.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_curfileupd(void)
{
	cdinfo_curfileupd();
}


/*
 * dbprog_curtrkupd
 *	Update the track list display to show the current playing
 *	track entry in bold font.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_curtrkupd(curstat_t *s)
{
	int		pos,
			list_pos;
	static int	sav_pos = -1;

	/* Update curfile */
	dbprog_curfileupd();

	if (sav_pos >= 0) {
		/* Update track list entry: un-highlight previous track */
		dbprog_listupd_ent(
			s, sav_pos, 
			(dbp->track[sav_pos].title != NULL) ?
				dbp->track[sav_pos].title : UNDEF_STR,
			FALSE
		);

		if (sel_pos == (sav_pos + 1))
			/* This item is previously selected */
			XmListSelectPos(widgets.dbprog.trk_list,
					sel_pos, False);
	}

	if (s->cur_trk <= 0 || s->mode == MOD_BUSY || s->mode == MOD_NODISC) {
		sav_pos = -1;
		return;
	}

	sav_pos = pos = di_curtrk_pos(s);
	list_pos = pos + 1;

	/* Update track list entry: highlight current track */
	dbprog_listupd_ent(
		s, sav_pos, 
		(dbp->track[sav_pos].title != NULL) ?
			dbp->track[sav_pos].title : UNDEF_STR,
		FALSE
	);

	/* If this item is previously selected, re-select it */
	if (sel_pos == list_pos)
		XmListSelectPos(widgets.dbprog.trk_list, sel_pos, False);
	else if (ind_pos == list_pos)
		XmListSelectPos(widgets.dbprog.trk_list, ind_pos, False);

	/* Auto-scroll the track list if the current track is not visible.
	 * No scrolling is done while a track list entry is selected
	 * or if a track title is being edited.
	 */
	if (sel_pos < 0 && ind_pos < 0) {
		dbprog_list_autoscroll(
			widgets.dbprog.trk_list,
			(int) s->tot_trks,
			list_pos
		);
	}

	dbprog_extt_autotrk_upd(s, list_pos);
}


/*
 * dbprog_progclear
 *	Clear program sequence.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_progclear(curstat_t *s)
{
	cdinfo_ret_t	ret;

	/* Delete track program file */
	if (!s->onetrk_prog && (ret = cdinfo_del_prog()) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_del_prog: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	if (dbp->playorder != NULL) {
		MEM_FREE(dbp->playorder);
		dbp->playorder = NULL;
	}

	s->prog_tot = 0;
	s->prog_cnt = 0;
	s->program = FALSE;

	/* Update display */
	dpy_progmode(s, FALSE);
}


/*
 * dbprog_dbclear
 *	Clear in-core CD information.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	reload - Whether we are going to be re-loading the CD information.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_dbclear(curstat_t *s, bool_t reload)
{
	int		i;
	word16_t	flags;
	bool_t		upd_display = FALSE;
	static bool_t	first_time = TRUE;

	/* Pop down the fullname, track details and credits window (if in
	 * track credit mode) if necessary
	 */
	if (XtIsManaged(widgets.fullname.form)) {
		dbprog_fullname_ok(
			widgets.fullname.ok_btn,
			(XtPointer) s,
			(XtPointer) NULL
		);
	}
	if (XtIsManaged(widgets.dbextt.form)) {
		dbprog_extt_ok(
			widgets.dbextt.ok_btn,
			(XtPointer) s,
			(XtPointer) NULL
		);
	}
	if (credits_mode == CREDITS_TRACK) {
		dbprog_credits_ok(
			widgets.credits.ok_btn,
			(XtPointer) s,
			(XtPointer) NULL
		);
	}

	if (s->flags & STAT_SUBMIT) {
		/* Submit in-core information to CDDB, if so designated */
		(void) dbprog_dbsubmit(s);
	}

	if (first_time || s->mode == MOD_NODISC) {
		/* Pop down the disc details, credits, and segments window */
		if (XtIsManaged(widgets.dbextd.form)) {
			dbprog_extd_ok(
				widgets.dbextd.ok_btn,
				(XtPointer) s,
				(XtPointer) NULL
			);
		}
		if (credits_mode == CREDITS_DISC ||
		    credits_mode == CREDITS_SEG) {
			dbprog_credits_ok(
				widgets.credits.ok_btn,
				(XtPointer) s,
				(XtPointer) NULL
			);
		}
		if (XtIsManaged(widgets.segments.form)) {
			dbprog_segments_ok(
				widgets.segments.ok_btn,
				(XtPointer) s,
				(XtPointer) NULL
			);
		}

		first_time = FALSE;
		upd_display = TRUE;
	}

	/* Save flags */
	flags = s->flags;

	/* Clear album-specific URLs in the wwwWarp menu */
	if (!wwwwarp_cleared)
		wwwwarp_disc_url_clear(s);

	/* Clear CD information structure */
	cdinfo_clear(reload);

	/* Update CD changer list */
	dbprog_chgr_new(s);

	/* Clear flags */
	s->flags &= ~(STAT_SUBMIT | STAT_EJECT | STAT_EXIT | STAT_CHGDISC);

	/* Set qmode flag */
	s->qmode = QMODE_NONE;

	/* Configure the wwwWarp menu */
	if (!wwwwarp_cleared)
		wwwwarp_sel_cfg(s);

	/* Set the wwwwarp_cleared flag so that if this routine gets
	 * called again before we successfully get CDDB data, we don't
	 * go back into wwwwarp_disc_url_clear() or wwwwarp_sel_cfg()
	 * which has the effect of popping down the wwwwarp menu.
	 */
	wwwwarp_cleared = TRUE;

	/* Reset private copy of track list data */
	for (i = 0; i < MAXTRACK; i++) {
		if (trklist_copy[i].title != NULL) {
			MEM_FREE(trklist_copy[i].title);
			trklist_copy[i].title = NULL;
		}
		trklist_copy[i].highlighted = FALSE;
	}

	if (upd_display) {
		/* Update display */
		dbprog_dpytottime(s);
		dpy_dtitle(s);
		dpy_ttitle(s);

		/* Update curfile */
		dbprog_curfileupd();

		/* Update CD info/program display */
		set_text_string(widgets.dbprog.artist_txt, "", FALSE);
		set_text_string(widgets.dbprog.title_txt, "", FALSE);
		XmListDeleteAllItems(widgets.dbprog.trk_list);
		set_text_string(widgets.dbprog.pgmseq_txt, "", FALSE);
		set_text_string(widgets.dbextd.notes_txt, "", FALSE);
		set_text_string(widgets.dbextt.notes_txt, "", FALSE);

		/* Make some buttons insensitive */
		XtSetSensitive(widgets.dbprog.submit_btn, False);
		XtSetSensitive(widgets.dbprog.reload_btn, False);
		XtSetSensitive(widgets.dbprog.fullname_btn, False);
		XtSetSensitive(widgets.dbprog.extd_btn, False);
		XtSetSensitive(widgets.dbprog.dcredits_btn, False);
		XtSetSensitive(widgets.dbprog.segments_btn, False);
		XtSetSensitive(widgets.dbprog.extt_btn, False);
		XtSetSensitive(widgets.dbprog.tcredits_btn, False);

		/* Update button labels */
		dbprog_extt_lblupd(NULL);
		dbprog_tcred_lblupd(NULL);
	}

	/* Clear track title editor field in all cases */
	set_text_string(widgets.dbprog.ttitle_txt, "", FALSE);

	/* Clear changed flag */
	dbp->flags &= ~CDINFO_CHANGED;

	/* Eject the CD, if so specified */
	if (flags & STAT_EJECT) {
		cd_busycurs(TRUE, CURS_ALL);

		di_load_eject(s);

		cd_busycurs(FALSE, CURS_ALL);
	}

	/* Change disc, if so specified */
	if (flags & STAT_CHGDISC) {
		if (!s->chgrscan)
			cd_busycurs(TRUE, CURS_ALL);

		di_chgdisc(s);

		/* Update display */
		dpy_dbmode(s, FALSE);
		dpy_playmode(s, FALSE);

		if (!s->chgrscan)
			cd_busycurs(FALSE, CURS_ALL);
	}

	/* Quit the application, if so specified */
	if (flags & STAT_EXIT)
		cd_quit(s);
}


/*
 * dbprog_progget
 *	Get saved track program from file if available.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_progget(curstat_t *s)
{
	cdinfo_ret_t	ret;

	/* Load user-defined track program, if available */
	if ((ret = cdinfo_load_prog(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_load_prog: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	s->program = dbprog_pgm_active();

	/* Update widgets */
	dbprog_structupd(s);

	/* Just loaded from file, so no need to enable the save button
	 * until the program sequence is changed.
	 */
	XtSetSensitive(widgets.dbprog.savepgm_btn, False);

	/* Parse playorder string */
	dbprog_pgm_parse(s);

	/* Update display */
	dpy_progmode(s, FALSE);
}


/*
 * dbprog_dbget
 *	Look up information about the currently loaded disc, if available.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_dbget(curstat_t *s)
{
	cdinfo_ret_t	ret;
	static bool_t	do_motd = TRUE;

	/* Clear the in-core entry */
	dbprog_dbclear(s, TRUE);

	if (s->onetrk_prog) {
		/* Currently playing a single-track program: clear it first */
		s->onetrk_prog = FALSE;
		dbprog_progclear(s);
	}

	/* Set qmode flag */
	s->qmode = QMODE_WAIT;

	/* Update widgets */
	dbprog_structupd(s);
	dpy_dtitle(s);
	dpy_ttitle(s);

	/* Configure the wwwWarp menu */
	wwwwarp_sel_cfg(s);

	/* Update curfile */
	dbprog_curfileupd();

	XtSetSensitive(widgets.dbprog.reload_btn, False);

	/* If in synchronous mode, change to watch cursor, otherwise,
	 * force normal cursor.
	 */
	if (!s->chgrscan)
		cd_busycurs(cdinfo_issync(), CURS_ALL);

	/* Load CD information */
	if ((ret = cdinfo_load(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_load: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Change to normal cursor */
	if (!s->chgrscan && cdinfo_issync())
		cd_busycurs(FALSE, CURS_ALL);

	if (stopload_active) {
		/* Pop down the stop load dialog */
		cd_confirm_popdown();
		stopload_active = FALSE;
	}

	switch (CDINFO_GET_STAT(ret)) {
	case 0:
		/* Success */
		if (dbp->matchlist != NULL) {
			cdinfo_match_t	*mp;
			int		n = 0;

			for (mp = dbp->matchlist; mp != NULL; mp = mp->next)
				n++;

			if (n == 1 && app_data.single_fuzzy) {
				dbp->match_tag = 1;
				dbprog_matchsel_ok(
					widgets.matchsel.ok_btn,
					(XtPointer) s,
					NULL
				);

				CD_INFO_AUTO(app_data.str_submitcorr);
				return;
			}

			/* Multiple matches found */
			s->qmode = QMODE_NONE;

			/* Pop up dialog */
			dbprog_matchsel_popup(s);
			return;
		}
		else if ((dbp->flags & CDINFO_NEEDREG) != 0) {
			/* User not registered with CDDB */
			s->qmode = QMODE_NONE;

			/* Pop up CDDB user registration dialog */
			userreg_do_popup(s, TRUE);
		}
		else if ((dbp->flags & CDINFO_MATCH) != 0) {
			/* Exact match found */
			s->qmode = QMODE_MATCH;

			/* Got CDDB data, clear the wwwwarp_cleared flag */
			wwwwarp_cleared = FALSE;
		}
		else
			s->qmode = QMODE_NONE;

		break;

	case AUTH_ERR:
		/* Authorization failed */

		/* Set qmode flag */
		s->qmode = QMODE_NONE;

		if (auth_initted) {
			(void) cd_confirm_popup(
				app_data.str_confirm, app_data.str_authfail,
				dbprog_auth_retry, (XtPointer) 1,
				dbprog_auth_retry, NULL 
			);
		}
		else {
			/* Clear out user name and password */
			set_text_string(widgets.auth.name_txt, "", FALSE);
			set_text_string(widgets.auth.pass_txt, "", FALSE);
			if (dbp->proxy_passwd != NULL) {
				(void) memset(dbp->proxy_passwd, 0,
					      strlen(dbp->proxy_passwd));
				MEM_FREE(dbp->proxy_passwd);
				dbp->proxy_passwd = NULL;
			}

			/* Pop up authorization dialog */
			dbprog_auth_popup();
		}
		return;
		/*NOTREACHED*/

	default:
		/* Query error */

		/* Set qmode flag */
		s->qmode = QMODE_ERR;
		break;
	}

	/* Update widgets */
	dbprog_structupd(s);

	/* Configure the wwwWarp menu */
	wwwwarp_sel_cfg(s);

	XtSetSensitive(widgets.dbprog.reload_btn, True);

	/* Update display */
	dpy_dtitle(s);
	dpy_ttitle(s);

	/* Update curfile */
	dbprog_curfileupd();

	/* Add to history and changer lists */
	dbprog_hist_new(s);
	dbprog_chgr_new(s);

	/* Get xmcd MOTD: do this automatically just once if configured */
	if (!app_data.automotd_dsbl && !app_data.cdinfo_inetoffln && do_motd) {
		do_motd = FALSE;
		motd_get(NULL);
	}
}


/*
 * dbprog_chgr_scan_stop
 *	Stop the CD changer scan.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
dbprog_chgr_scan_stop(curstat_t *s)
{
	if (!s->chgrscan)
		return;

	if (scan_id >= 0) {
		cd_untimeout(scan_id);
		scan_id = -1;
	}

	/* Restore original multiplay and reverse modes */
	app_data.multi_play = sav_mplay;
	app_data.reverse = sav_rev;

	s->chgrscan = FALSE;

	if (scan_slot >= 0) {
		/* The user clicked "stop", add the current disc to
		 * the history list.
		 */
		dbprog_hist_new(s);
		scan_slot = -1;
	}
	else if (s->cur_disc != start_slot) {
		/* Go back to original slot */
		if (s->mode != MOD_NODISC && s->mode != MOD_BUSY)
			s->prev_disc = s->cur_disc;
		s->cur_disc = start_slot;
		di_chgdisc(s);

		/* Update display */
		dpy_dbmode(s, FALSE);
		dpy_playmode(s, FALSE);
	}

	start_slot = -1;

	/* Update changer list */
	dbprog_chgr_new(s);

	XtSetSensitive(widgets.dlist.rescan_btn, True);

	/* Pop down the working dialog box */
	cd_working_popdown();

	/* Set to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);
}


/*
 * dbprog_init
 *	Initialize the CD info/program subsystem.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_init(curstat_t *s)
{
	XmString	xs;
	int		i;
	char		*cp;

	/* Check various error message strings to ensure we don't
	 * overflow our error message buffer later.
	 */
	if (((int) (strlen(app_data.str_saverr_fork) + 16) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_saverr_suid) + 32) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_saverr_open) + 1) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_saverr_close) + 1) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_saverr_killed) + 16) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_saverr_write) + 1) >= ERR_BUF_SZ)) {
		CD_FATAL(app_data.str_longpatherr);
		return;
	}

	time_mode = TIME_TRACK;
	dlist_mode = DLIST_HIST;
	reg_mode = REGION_NONE;
	fname_mode = FNAME_NONE;

	(void) memset(&w_cred, 0, sizeof(cdinfo_credit_t));
	(void) memset(&w_seg, 0, sizeof(cdinfo_segment_t));

	/* Set pointer to CD info struct */
	dbp = dbprog_curdb(s);

	/* Seed some numbers */
	for (i = 0, cp = PROGNAME; i < 4 && *cp != '\0'; i++, cp++)
		s->aux[i] = (byte_t) *cp ^ 0xff;

	/* Clear the in-core structure */
	dbprog_dbclear(s, FALSE);

	/* Save disc details and track details labels,
	 * create versions with asterisk
	 */
	XtVaGetValues(widgets.dbprog.extd_btn,
		XmNlabelString, &xs_extd_lbl,
		NULL
	);
	XtVaGetValues(widgets.dbprog.dcredits_btn,
		XmNlabelString, &xs_dcred_lbl,
		NULL
	);
	XtVaGetValues(widgets.dbprog.segments_btn,
		XmNlabelString, &xs_seg_lbl,
		NULL
	);
	XtVaGetValues(widgets.dbprog.extt_btn,
		XmNlabelString, &xs_extt_lbl,
		NULL
	);
	XtVaGetValues(widgets.dbprog.tcredits_btn,
		XmNlabelString, &xs_tcred_lbl,
		NULL
	);
	XtVaGetValues(widgets.segments.credits_btn,
		XmNlabelString, &xs_scred_lbl,
		NULL
	);
	xs = create_xmstring("*", NULL, XmSTRING_DEFAULT_CHARSET, FALSE);
	xs_extd_lblx = XmStringConcat(xs_extd_lbl, xs);
	xs_dcred_lblx = XmStringConcat(xs_dcred_lbl, xs);
	xs_seg_lblx = XmStringConcat(xs_seg_lbl, xs);
	xs_extt_lblx = XmStringConcat(xs_extt_lbl, xs);
	xs_tcred_lblx = XmStringConcat(xs_tcred_lbl, xs);
	xs_scred_lblx = XmStringConcat(xs_scred_lbl, xs);
	XmStringFree(xs);

	/* Initialize the CDDB offline button state */
	XmToggleButtonSetState(
		widgets.dbprog.inetoffln_btn,
		app_data.cdinfo_inetoffln, False
	);

	XtSetSensitive(widgets.dbprog.addpgm_btn, False);
	XtSetSensitive(widgets.dbprog.clrpgm_btn, False);
	XtSetSensitive(widgets.dbprog.savepgm_btn, False);
	XtSetSensitive(widgets.dbprog.submit_btn, False);
	XtSetSensitive(widgets.dbprog.flush_btn,
		       (Boolean) !app_data.cdinfo_inetoffln);
	XtSetSensitive(widgets.dbprog.userreg_btn,
		       (Boolean) (cdinfo_cddb_ver() == 2));

	/* Initialize in-core history and CD changer lists */
	cdinfo_hist_init();
	cdinfo_chgr_init();

	XtVaSetValues(widgets.dlist.type_opt,
		XmNmenuHistory, widgets.dlist.hist_btn,
		NULL
	);

	/* Set disc details and track details window genre selectors */
	for (i = 0; i < 2; i++) {
		XtVaSetValues(widgets.dbextd.genre_opt[i],
			XmNmenuHistory, widgets.dbextd.genre_none_btn[i],
			NULL
		);
		XtVaSetValues(widgets.dbextd.subgenre_opt[i],
			XmNmenuHistory, widgets.dbextd.subgenre_none_btn[i],
			NULL
		);
		XtVaSetValues(widgets.dbextt.genre_opt[i],
			XmNmenuHistory, widgets.dbextt.genre_none_btn[i],
			NULL
		);
		XtVaSetValues(widgets.dbextt.subgenre_opt[i],
			XmNmenuHistory, widgets.dbextt.subgenre_none_btn[i],
			NULL
		);
	}

	/* Set credits window role selectors */
	XtVaSetValues(widgets.credits.prirole_opt,
		XmNmenuHistory, widgets.credits.prirole_none_btn,
		NULL
	);
	XtVaSetValues(widgets.credits.subrole_opt,
		XmNmenuHistory, widgets.credits.subrole_none_btn,
		NULL
	);
}


/*
 * dbprog_chgsubmit
 *	If in-core info has been changed, ask user if it should
 *	be submitted to CDDB.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Whether the application should proceed to shut down
 */
/*ARGSUSED*/
bool_t
dbprog_chgsubmit(curstat_t *s)
{
	if ((dbp->flags & CDINFO_CHANGED) && !app_data.cdinfo_inetoffln) {
		/* We use cd_timeout to schedule a pop-up of the
		 * confirm dialog, because this thread could be
		 * called from a cd_confirm_dialog button callback,
		 * and we cd_confirm_callback is not re-entrant in
		 * this manner
		 */
		(void) cd_timeout(50, dbprog_submit_popup, NULL);
		return FALSE;
	}

	return TRUE;
}


/*
 * dbprog_curartist
 *	Return the current disc artist string.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Disc title text string, or the null string if there
 *	is no title available.
 */
char *
dbprog_curartist(curstat_t *s)
{
	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC ||
	    s->qmode == QMODE_WAIT)
		return NULL;

	return (dbp->disc.artist);
}


/*
 * dbprog_curtitle
 *	Return the current disc title string.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Disc title text string, or the null string if there
 *	is no title available.
 */
char *
dbprog_curtitle(curstat_t *s)
{
	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC ||
	    s->qmode == QMODE_WAIT)
		return NULL;

	return (dbp->disc.title == NULL ?
		app_data.str_unkndisc : dbp->disc.title);
}


/*
 * dbprog_curttitle
 *	Return the current track title string.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Track title text string, or the null string if there
 *	is no title available.
 */
char *
dbprog_curttitle(curstat_t *s)
{
	int	n = di_curtrk_pos(s);

	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC ||
	    (int) s->cur_trk < 0 || s->qmode == QMODE_WAIT)
		return ("");

	if (n < 0 || dbp->track[n].title == NULL)
		return (app_data.str_unkntrk);

	return (dbp->track[n].title);
}


/*
 * dbprog_curdb
 *	Obtain the cdinfo_incore_t structure for the current disc.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Pointer to the cdinfo_incore_t structure.
 */
/*ARGSUSED*/
cdinfo_incore_t *
dbprog_curdb(curstat_t *s)
{
	return (cdinfo_addr());
}


/*
 * dbprog_curseltrk
 *	Obtain the currently selected track number in the track list
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	The selected track number, or -1 if not selected.
 */
/*ARGSUSED*/
int
dbprog_curseltrk(curstat_t *s)
{
	return (sel_pos);
}


/*
 * dbprog_pgm_parse
 *	Parse the program mode play sequence text string, and
 *	update the playorder table in the curstat_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	TRUE=success, FALSE=error.
 */
bool_t
dbprog_pgm_parse(curstat_t *s)
{
	int	i,
		j;
	char	*p,
		*q,
		*tmpbuf;
	bool_t	last = FALSE,
		skipped = FALSE;

	if (dbp->playorder == NULL)
		/* Nothing to do */
		return TRUE;

	tmpbuf = NULL;
	if (!util_newstr(&tmpbuf, dbp->playorder)) {
		CD_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	s->prog_tot = 0;

	for (i = 0, p = q = tmpbuf; i < MAXTRACK; p = ++q) {
		/* Skip p to the next digit */
		for (; !isdigit((int) *p) && *p != '\0'; p++)
			;

		if (*p == '\0')
			/* No more to do */
			break;

		/* Skip q to the next non-digit */
		for (q = p; isdigit((int) *q); q++)
			;

		if (*q == ',')
			*q = '\0';
		else if (*q == '\0')
			last = TRUE;
		else {
			MEM_FREE(tmpbuf);
			CD_WARNING(app_data.str_seqfmterr);
			return FALSE;
		}

		if (q > p) {
			/* Update play sequence */
			for (j = 0; j < MAXTRACK; j++) {
				if (s->trkinfo[j].trkno == atoi(p)) {
					s->trkinfo[i].playorder = j;
					s->prog_tot++;
					i++;
					break;
				}
			}

			if (j >= MAXTRACK)
				skipped = TRUE;
		}

		if (last)
			break;
	}

	if (skipped) {
		/* Delete invalid tracks from list */

		tmpbuf[0] = '\0';
		for (i = 0; i < (int) s->prog_tot; i++) {
			if (i == 0)
				(void) sprintf(tmpbuf, "%u",
				    s->trkinfo[s->trkinfo[i].playorder].trkno);
			else
				(void) sprintf(tmpbuf, "%s,%u", tmpbuf,
				    s->trkinfo[s->trkinfo[i].playorder].trkno);
		}

		set_text_string(widgets.dbprog.pgmseq_txt, tmpbuf, FALSE);

		CD_WARNING(app_data.str_invpgmtrk);
	}

	MEM_FREE(tmpbuf);

	return TRUE;
}


/*
 * dbprog_segments_setmode
 *	Set the "set" button sensitivity and pointer label configuration
 *	in the segments window.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_segments_setmode(curstat_t *s)
{
	int			segptr_mode = 0;
	static XmString		xs_set,
				xs_clear;
	XmListCallbackStruct	cb;
	static int		segptr_prev = -1;
	static bool_t		first = TRUE;

	if (!XtIsManaged(widgets.segments.form))
		return;

	if (first) {
		first = FALSE;
		XtVaGetValues(widgets.segments.set_btn,
			XmNlabelString, &xs_set,
			NULL
		);
		XtVaGetValues(widgets.dbprog.clrpgm_btn,
			XmNlabelString, &xs_clear,
			NULL
		);
	}

	switch (s->mode) {
	case MOD_PLAY:
	case MOD_PAUSE:
	case MOD_STOP:
		if (s->program || s->shuffle)
			segptr_mode = 0;
		else if (s->segplay == SEGP_AB)
			segptr_mode = 3;
		else if (s->mode != MOD_STOP) {
			if (s->segplay == SEGP_NONE)
				segptr_mode = 1;
			else if (s->segplay == SEGP_A)
				segptr_mode = 2;
		}
		else
			segptr_mode = 0;
		break;
	case MOD_SAMPLE:
	default:
		segptr_mode = 0;
		break;
	}

	if (segptr_prev == segptr_mode)
		/* No change */
		return;

	XtVaSetValues(widgets.segments.set_btn,
		XmNlabelString, (segptr_mode == 3) ? xs_clear : xs_set,
		NULL
	);
			
	XtSetSensitive(widgets.segments.set_btn, (Boolean) (segptr_mode > 0));

	switch (segptr_mode) {
	case 1:
		XtMapWidget(widgets.segments.startptr_lbl);
		XtUnmapWidget(widgets.segments.endptr_lbl);
		break;
	case 2:
		XtUnmapWidget(widgets.segments.startptr_lbl);
		XtMapWidget(widgets.segments.endptr_lbl);
		break;
	case 3:
		XtMapWidget(widgets.segments.startptr_lbl);
		XtMapWidget(widgets.segments.endptr_lbl);
		break;
	case 0:
	default:
		XtUnmapWidget(widgets.segments.startptr_lbl);
		XtUnmapWidget(widgets.segments.endptr_lbl);

		/* Put focus on the OK button */
		XmProcessTraversal(widgets.segments.ok_btn,
				   XmTRAVERSE_CURRENT);

		if (seg_pos > 0) {
			/* If dbprog_segments_select is ever changed to use
			 * more fields of the callback struct, this will need
			 * change too.
			 */
			cb.item_position = seg_pos;

			/* Fake a callback */
			dbprog_segments_select(
				widgets.segments.seg_list,
				(XtPointer) s,
				(XtPointer) &cb
			);
		}
		break;
	}

	segptr_prev = segptr_mode;
}


/*
 * dbprog_segments_cancel
 *	Cancel a->b mode in the segments window
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
dbprog_segments_cancel(curstat_t *s)
{
	XmListCallbackStruct	cb;

	if (!XtIsManaged(widgets.segments.form))
		/* Nothing to do */
		return;

	if (seg_pos < 0) {
		/* Simply set some fields to blank */
		set_text_string(widgets.segments.starttrk_txt, "", FALSE);
		set_text_string(widgets.segments.startfrm_txt, "", FALSE);
		set_text_string(widgets.segments.endtrk_txt, "", FALSE);
		set_text_string(widgets.segments.endfrm_txt, "", FALSE);
		return;
	}

	/* If dbprog_segments_select is ever changed to use more
	 * fields of the callback struct, this will need change too.
	 */
	cb.item_position = seg_pos;

	/* Fake a callback */
	dbprog_segments_select(
		widgets.segments.seg_list,
		(XtPointer) s,
		(XtPointer) &cb
	);
}


/*
 * dbprog_stopload_active
 *	Query and/or set the stopload_active boolean flag.
 *
 * Args:
 *	mode - 0 for query only, none-zero to set the stopload_active value
 *	newval - If mode is non-zero, the value to set stopload_active to.
 *
 * Return:
 *	If mode is 0, the current stopload_active value.
 *	If mode is non-zero, the original stopload_active value.
 */
bool_t
dbprog_stopload_active(int mode, bool_t newval)
{
	bool_t val;

	val = stopload_active;
	if (mode != 0)
		stopload_active = newval;

	return (val);
}


/**************** vv Callback routines vv ****************/

/*
 * dbprog_popup
 *	Pop up the CD info/program subsystem window.
 */
/*ARGSUSED*/
void
dbprog_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.dbprog.form)) {
		/* Already popped up: pop it down */
		dbprog_ok(widgets.dbprog.ok_btn, client_data, call_data);
		return;
	}

	/* Pop up the dbprog window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	XtManageChild(widgets.dbprog.form);
	if (first) {
		first = FALSE;
		/* Set window position */
		cd_dialog_setpos(XtParent(widgets.dbprog.form));
	}
	XtMapWidget(XtParent(widgets.dbprog.form));

	/* Put focus on the OK button */
	XmProcessTraversal(widgets.dbprog.ok_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_inetoffln
 *	The Internet Offline button callback.
 */
/*ARGSUSED*/
void
dbprog_inetoffln(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;
	curstat_t			*s = (curstat_t *)(void *) client_data;
	cdinfo_ret_t			ret;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	DBGPRN(DBG_CDI|DBG_UI)(errfp,
		"\n* OFFLINE-CDDB: %s\n", p->set ? "On" : "Off");

	/* If CD information lookup is in progress, cancel it, then
	 * schedule a reload in one second.
	 */
	if (s->qmode == QMODE_WAIT) {
		cdinfo_load_cancel();
		(void) cd_timeout(1000, dbprog_dbget, (byte_t *) s);
	}

	app_data.cdinfo_inetoffln = (bool_t) p->set;

	if ((ret = cdinfo_offline(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_offline: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));

		cd_beep();
		XmToggleButtonSetState(w, (Boolean) !p->set, False);
		app_data.cdinfo_inetoffln = (bool_t) !p->set;
		return;
	}

	if (!app_data.cdinfo_inetoffln && (dbp->flags & CDINFO_CHANGED))
		XtSetSensitive(widgets.dbprog.submit_btn, True);
	else
		XtSetSensitive(widgets.dbprog.submit_btn, False);

	XtSetSensitive(widgets.dbprog.flush_btn, !app_data.cdinfo_inetoffln);

	/* Configure the wwwWarp menu */
	wwwwarp_sel_cfg(s);
}


/*
 * dbprog_text_new
 *	Generic text widget callback function.
 */
/*ARGSUSED*/
void
dbprog_text_new(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;
	char			*str,
				**ent;
	bool_t			credits_handling,
				segments_handling,
				fname_handling;
	cdinfo_fname_t		*fnp = NULL;

	credits_handling = segments_handling = fname_handling = FALSE;

	/* Figure out which widget changed */
	if (w == widgets.dbprog.artist_txt)
		ent = &dbp->disc.artist;
	else if (w == widgets.dbprog.title_txt)
		ent = &dbp->disc.title;
	else if (w == widgets.dbextd.sorttitle_txt)
		ent = &dbp->disc.sorttitle;
	else if (w == widgets.dbextd.the_txt)
		ent = &dbp->disc.title_the;
	else if (w == widgets.dbextd.year_txt)
		ent = &dbp->disc.year;
	else if (w == widgets.dbextd.label_txt)
		ent = &dbp->disc.label;
	else if (w == widgets.dbextd.dnum_txt)
		ent = &dbp->disc.dnum;
	else if (w == widgets.dbextd.tnum_txt)
		ent = &dbp->disc.tnum;
	else if (w == widgets.dbextd.notes_txt)
		ent = &dbp->disc.notes;
	else if (w == widgets.dbextt.sorttitle_txt)
		ent = &dbp->track[extt_pos].sorttitle;
	else if (w == widgets.dbextt.the_txt)
		ent = &dbp->track[extt_pos].title_the;
	else if (w == widgets.dbextt.artist_txt)
		ent = &dbp->track[extt_pos].artist;
	else if (w == widgets.dbextt.year_txt)
		ent = &dbp->track[extt_pos].year;
	else if (w == widgets.dbextt.label_txt)
		ent = &dbp->track[extt_pos].label;
	else if (w == widgets.dbextt.bpm_txt)
		ent = &dbp->track[extt_pos].bpm;
	else if (w == widgets.dbextt.notes_txt)
		ent = &dbp->track[extt_pos].notes;
	else if (w == widgets.credits.name_txt) {
		ent = &w_cred.crinfo.name;
		credits_handling = TRUE;
	}
	else if (w == widgets.credits.notes_txt) {
		ent = &w_cred.notes;
		credits_handling = TRUE;
	}
	else if (w == widgets.segments.name_txt) {
		ent = &w_seg.name;
		segments_handling = TRUE;
	}
	else if (w == widgets.segments.starttrk_txt) {
		ent = &w_seg.start_track;
		segments_handling = TRUE;
	}
	else if (w == widgets.segments.startfrm_txt) {
		ent = &w_seg.start_frame;
		segments_handling = TRUE;
	}
	else if (w == widgets.segments.endtrk_txt) {
		ent = &w_seg.end_track;
		segments_handling = TRUE;
	}
	else if (w == widgets.segments.endfrm_txt) {
		ent = &w_seg.end_frame;
		segments_handling = TRUE;
	}
	else if (w == widgets.segments.notes_txt) {
		ent = &w_seg.notes;
		segments_handling = TRUE;
	}
	else if (w == widgets.fullname.dispname_txt) {
		switch (fname_mode) {
		case FNAME_DISC:
			fnp = &dbp->disc.artistfname;
			break;
		case FNAME_TRACK:
			fnp = &dbp->track[extt_pos].artistfname;
			break;
		case FNAME_CREDITS:
			fnp = &w_cred.crinfo.fullname;
			credits_handling = TRUE;
			break;
		default:
			/* Unsupported fname mode */
			return;
		}
		ent = &fnp->dispname;
		fname_handling = TRUE;
	}
	else if (w == widgets.fullname.lastname_txt) {
		switch (fname_mode) {
		case FNAME_DISC:
			fnp = &dbp->disc.artistfname;
			break;
		case FNAME_TRACK:
			fnp = &dbp->track[extt_pos].artistfname;
			break;
		case FNAME_CREDITS:
			fnp = &w_cred.crinfo.fullname;
			credits_handling = TRUE;
			break;
		default:
			/* Unsupported fname mode */
			return;
		}
		ent = &fnp->lastname;
		fname_handling = TRUE;
	}
	else if (w == widgets.fullname.firstname_txt) {
		switch (fname_mode) {
		case FNAME_DISC:
			fnp = &dbp->disc.artistfname;
			break;
		case FNAME_TRACK:
			fnp = &dbp->track[extt_pos].artistfname;
			break;
		case FNAME_CREDITS:
			fnp = &w_cred.crinfo.fullname;
			credits_handling = TRUE;
			break;
		default:
			/* Unsupported fname mode */
			return;
		}
		ent = &fnp->firstname;
		fname_handling = TRUE;
	}
	else if (w == widgets.fullname.the_txt) {
		switch (fname_mode) {
		case FNAME_DISC:
			fnp = &dbp->disc.artistfname;
			break;
		case FNAME_TRACK:
			fnp = &dbp->track[extt_pos].artistfname;
			break;
		case FNAME_CREDITS:
			fnp = &w_cred.crinfo.fullname;
			credits_handling = TRUE;
			break;
		default:
			/* Unsupported fname mode */
			return;
		}
		ent = &fnp->the;
		fname_handling = TRUE;
	}
	else {
		/* Unsupported by this function */
		return;
	}

	/* Update the appropriate in-core structure entry */
	switch (p->reason) {
	case XmCR_VALUE_CHANGED:
		if ((str = get_text_string(w, TRUE)) == NULL)
			return;

		if (*ent != NULL) {
			if (strcmp(str, *ent) == 0) {
				/* not changed */
				MEM_FREE(str);
				break;
			}
			MEM_FREE(*ent);
			*ent = NULL;
		}

		if (!util_newstr(ent, str)) {
			CD_FATAL(app_data.str_nomemory);
			MEM_FREE(str);
			return;
		}

		if (fname_handling) {
			fname_changed = TRUE;

			if (w != widgets.fullname.dispname_txt &&
			    XmToggleButtonGetState(
				    widgets.fullname.autogen_btn)) {
				char	*cp;

				/* Auto-generate the fullname's dispname */
				if ((cp = dbprog_autoname(fnp)) == NULL) {
					CD_FATAL(app_data.str_nomemory);
					return;
				}

				set_text_string(
					widgets.fullname.dispname_txt,
					cp,
					TRUE
				);
				MEM_FREE(cp);
			}
		}

		/* Special handling for fullname.dispname field:
		 * update the associated fields for album, track,
		 * or credit to match.
		 */
		if (w == widgets.fullname.dispname_txt) {
			switch (fname_mode) {
			case FNAME_DISC:
				set_text_string(
					widgets.dbprog.artist_txt, str, TRUE
				);
				break;
			case FNAME_TRACK:
				set_text_string(
					widgets.dbextt.artist_txt, str, TRUE
				);
				break;
			case FNAME_CREDITS:
				set_text_string(
					widgets.credits.name_txt, str, TRUE
				);
				break;
			}
		}

		/* Update disc details button label if needed */
		if (w == widgets.dbextd.notes_txt)
			dbprog_extd_lblupd();

		/* Update track list entry and track details
		 * button label if needed
		 */
		if (w == widgets.dbextt.notes_txt && extt_pos >= 0) {
			if (sel_pos > 0)
				dbprog_extt_lblupd(&dbp->track[sel_pos-1]);

			dbprog_listupd_ent(
				s,
				extt_pos,
				(dbp->track[extt_pos].title != NULL) ?
					dbp->track[extt_pos].title : UNDEF_STR,
				FALSE
			);

			/* Re-select the list entry if necessary */
			if ((extt_pos + 1) == sel_pos) {
				XmListSelectPos(widgets.dbprog.trk_list,
						sel_pos, False);
			}
			else if ((extt_pos + 1) == ind_pos) {
				XmListSelectPos(widgets.dbprog.trk_list,
						ind_pos, False);
			}
		}

		/* Update credits window title if needed */
		if (w == widgets.segments.name_txt)
			dbprog_set_seg_title();

		/* Update the disc details window and credits window titles
		 * if needed
		 */
		if (w == widgets.dbprog.artist_txt ||
		    w == widgets.dbprog.title_txt) {
			if (XtIsManaged(widgets.dbextd.form)) {
				/* Update disc details disc title */
				dbprog_set_disc_title(
					s->cur_disc,
					dbp->disc.artist,
					dbp->disc.title
				);
			}

			/* Update main window title display */
			dpy_dtitle(s);
		}

		MEM_FREE(str);

		if (credits_handling) {
			if (cred_pos > 0)
				XtSetSensitive(widgets.credits.mod_btn, True);
			else
				XtSetSensitive(widgets.credits.add_btn, True);
		}
		else if (segments_handling) {
			if (seg_pos > 0)
				XtSetSensitive(widgets.segments.mod_btn, True);
			else
				XtSetSensitive(widgets.segments.add_btn, True);
		}
		else {
			dbp->flags |= CDINFO_CHANGED;
			if (!app_data.cdinfo_inetoffln) {
				XtSetSensitive(widgets.dbprog.submit_btn,
					       True);
			}
		}

		break;

	case XmCR_ACTIVATE:
	case XmCR_LOSING_FOCUS:
		if (w == widgets.dbprog.title_txt ||
		    w == widgets.dbextd.sorttitle_txt ||
		    w == widgets.dbextd.year_txt ||
		    w == widgets.dbextd.label_txt ||
		    w == widgets.dbextd.dnum_txt ||
		    w == widgets.dbextd.tnum_txt ||
		    w == widgets.dbextt.sorttitle_txt ||
		    w == widgets.dbextt.artist_txt ||
		    w == widgets.dbextt.year_txt ||
		    w == widgets.dbextt.label_txt ||
		    w == widgets.dbextt.bpm_txt ||
		    w == widgets.fullname.dispname_txt ||
		    w == widgets.fullname.lastname_txt ||
		    w == widgets.fullname.firstname_txt) {

			/* Set cursor to beginning of text */
			XmTextSetInsertionPosition(w, 0);

			if (w == widgets.dbprog.title_txt) {
				/* Update curfile */
				dbprog_curfileupd();
			}
		}
		break;

	default:
		break;
	}
}


/*
 * dbprog_focus_next
 *	Change focus to the next widget on done
 */
/*ARGSUSED*/
void
dbprog_focus_next(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget	nextw;

	if (w == widgets.dbprog.artist_txt)
		nextw = widgets.dbprog.title_txt;
	else if (w == widgets.dbprog.title_txt)
		nextw = widgets.dbprog.ttitle_txt;
	else if (w == widgets.dbextd.sorttitle_txt)
		nextw = widgets.dbextd.the_btn;
	else if (w == widgets.dbextd.the_txt)
		nextw = widgets.dbextd.year_txt;
	else if (w == widgets.dbextd.year_txt)
		nextw = widgets.dbextd.label_txt;
	else if (w == widgets.dbextd.label_txt)
		nextw = widgets.dbextd.comp_btn;
	else if (w == widgets.dbextd.dnum_txt)
		nextw = widgets.dbextd.tnum_txt;
	else if (w == widgets.dbextd.tnum_txt)
		nextw = widgets.dbextd.notes_txt;
	else if (w == widgets.dbextt.sorttitle_txt)
		nextw = widgets.dbextt.the_btn;
	else if (w == widgets.dbextt.the_txt)
		nextw = widgets.dbextt.artist_txt;
	else if (w == widgets.dbextt.artist_txt)
		nextw = widgets.dbextt.fullname_btn;
	else if (w == widgets.dbextt.year_txt)
		nextw = widgets.dbextt.label_txt;
	else if (w == widgets.dbextt.label_txt)
		nextw = widgets.dbextt.bpm_txt;
	else if (w == widgets.credits.name_txt)
		nextw = widgets.credits.fullname_btn;
	else if (w == widgets.segments.name_txt)
		nextw = widgets.segments.starttrk_txt;
	else if (w == widgets.segments.starttrk_txt)
		nextw = widgets.segments.startfrm_txt;
	else if (w == widgets.segments.startfrm_txt)
		nextw = widgets.segments.endtrk_txt;
	else if (w == widgets.segments.endtrk_txt)
		nextw = widgets.segments.endfrm_txt;
	else if (w == widgets.segments.endfrm_txt)
		nextw = widgets.segments.notes_txt;
	else if (w == widgets.fullname.dispname_txt)
		nextw = widgets.fullname.firstname_txt;
	else if (w == widgets.fullname.firstname_txt)
		nextw = widgets.fullname.lastname_txt;
	else if (w == widgets.fullname.lastname_txt)
		nextw = widgets.fullname.the_btn;
	else if (w == widgets.fullname.the_txt)
		nextw = widgets.fullname.ok_btn;
	else if (w == widgets.auth.name_txt)
		nextw = widgets.auth.pass_txt;
	else if (w == widgets.auth.pass_txt)
		nextw = widgets.auth.ok_btn;
	else
		return;

	/* Put the input focus on the next widget */
	XmProcessTraversal(nextw, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_trklist_play
 *	Track list entry selection default action callback.
 */
void
dbprog_trklist_play(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct	*p = (XmListCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;

	if (p->reason != XmCR_DEFAULT_ACTION)
		return;

	/* Stop current playback */
	if (s->mode != MOD_STOP)
		di_stop(s, FALSE);

	sel_pos = p->item_position;

	/* Clear previous program */
	set_text_string(widgets.dbprog.pgmseq_txt, "", FALSE);
	XtSetSensitive(widgets.dbprog.addpgm_btn, False);
	XtSetSensitive(widgets.dbprog.clrpgm_btn, False);
	XtSetSensitive(widgets.dbprog.savepgm_btn, False);
	dbprog_progclear(s);

	/* This is a single-track program as a result of double clicking
	 * on a track (or pressing return).
	 */
	s->onetrk_prog = TRUE;

	/* Add selected track to program */
	dbprog_addpgm(w, client_data, call_data);

	/* Play selected track */
	cd_play_pause(w, client_data, call_data);

	if (sel_pos > 0) {
		XmListDeselectPos(w, sel_pos);
		sel_pos = -1;
	}
	set_text_string(widgets.dbprog.ttitle_txt, "", FALSE);

	XtSetSensitive(widgets.dbprog.addpgm_btn, False);
	XtSetSensitive(widgets.dbprog.extt_btn, False);
	XtSetSensitive(widgets.dbprog.tcredits_btn, False);

	/* Update button labels */
	dbprog_extt_lblupd(NULL);
	dbprog_tcred_lblupd(NULL);
}


/*
 * dbprog_trklist_select
 *	Track list entry selection callback.
 */
void
dbprog_trklist_select(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct	*p = (XmListCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;
	int			pos;
	char			*cp;
	bool_t			resel_list;

	if (p->reason != XmCR_BROWSE_SELECT)
		return;

	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC)
		return;

	pos = p->item_position - 1;

	if (title_edited) {
		title_edited = FALSE;

		cp = get_text_string(widgets.dbprog.ttitle_txt, TRUE);
		if (cp == NULL)
			return;

		/* Update track list entry */
		dbprog_listupd_ent(s, pos, cp, FALSE);

		if (sel_pos > 0) {
			XmListDeselectPos(w, sel_pos);
			sel_pos = -1;
		}
		set_text_string(widgets.dbprog.ttitle_txt, "", FALSE);

		if (!util_newstr(&dbp->track[pos].title, cp)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		MEM_FREE(cp);

		dbp->flags |= CDINFO_CHANGED;
		if (!app_data.cdinfo_inetoffln)
			XtSetSensitive(widgets.dbprog.submit_btn, True);
		XtSetSensitive(widgets.dbprog.addpgm_btn, False);
		XtSetSensitive(widgets.dbprog.extt_btn, False);
		XtSetSensitive(widgets.dbprog.tcredits_btn, False);

		/* Update button labels */
		dbprog_extt_lblupd(NULL);
		dbprog_tcred_lblupd(NULL);

		/* Update the track details window if necessary */
		if (extt_pos == pos &&
		    (XtIsManaged(widgets.dbextt.form) ||
		     XtIsManaged(widgets.credits.form))) {
			dbprog_set_track_title(
				s->trkinfo[pos].trkno,
				dbp->track[pos].title
			);
		}

		/* Update the main window if necessary */
		if (di_curtrk_pos(s) == pos) {
			dpy_ttitle(s);

			/* Update curfile */
			dbprog_curfileupd();
		}

		/* Return the input focus to the track title editor */
		XmProcessTraversal(
			widgets.dbprog.ttitle_txt,
			XmTRAVERSE_CURRENT
		);
	}
	else if (sel_pos == p->item_position) {
		/* This item is already selected: deselect it */

		XmListDeselectPos(w, sel_pos);
		sel_pos = ind_pos = -1;
		set_text_string(widgets.dbprog.ttitle_txt, "", FALSE);

		XtSetSensitive(widgets.dbprog.addpgm_btn, False);
		XtSetSensitive(widgets.dbprog.extt_btn, False);
		XtSetSensitive(widgets.dbprog.tcredits_btn, False);

		/* Update button labels */
		dbprog_extt_lblupd(NULL);
		dbprog_tcred_lblupd(NULL);
	}
	else {
		/* Check full name data and abort if needed */
		if (!dbprog_fullname_ck(s)) {
			cd_beep();
			XmListDeselectPos(w, p->item_position);
			XmListSelectPos(w, sel_pos, False);
			return;
		}

		sel_pos = p->item_position;

		if (dbp->track[pos].title == NULL) {
			set_text_string(widgets.dbprog.ttitle_txt, "", FALSE);
		}
		else {
			char	*str;
			int	n;

			set_text_string(widgets.dbprog.ttitle_txt,
					dbp->track[pos].title,
					TRUE);

			str = XmTextGetString(widgets.dbprog.ttitle_txt);
			n = strlen(str);
			XtFree(str);

			XmTextSetInsertionPosition(
				widgets.dbprog.ttitle_txt,
				n
			);
		}

		XtSetSensitive(widgets.dbprog.addpgm_btn, True);
		XtSetSensitive(widgets.dbprog.extt_btn, True);
		XtSetSensitive(widgets.dbprog.tcredits_btn, True);

		/* Update button labels */
		dbprog_extt_lblupd(&dbp->track[pos]);
		dbprog_tcred_lblupd(&dbp->track[pos]);

		resel_list = FALSE;

		/* Warp the track details window to the new selected track,
		 * if it is popped up.
		 */
		if (XtIsManaged(widgets.dbextt.form)) {
			extt_pos = pos;
			dbprog_extt(w, (XtPointer) FALSE, call_data);
			resel_list = TRUE;
		}

		/* Warp the credits window to the new selected track,
		 * if it is popped up and in track mode.
		 */
		if (credits_mode == CREDITS_TRACK) {
			/* Update credits window if needed */
			extt_pos = pos;
			dbprog_creditupd(s, pos);
			resel_list = TRUE;
		}

		if (resel_list)
			XmListSelectPos(w, sel_pos, False);

		/* Update fullname window if necessary */
		if (fname_mode == FNAME_TRACK) {
			dbprog_fullname(widgets.dbextt.fullname_btn,
					(XtPointer) FALSE, call_data);
		}
		else if (fname_mode == FNAME_CREDITS &&
			 credits_mode == CREDITS_TRACK) {
			dbprog_fullname(widgets.credits.fullname_btn,
					(XtPointer) FALSE, call_data);
		}
	}
}


/*
 * dbprog_ttitle_focuschg
 *	Track title editor text widget keyboard focus change callback.
 */
/*ARGSUSED*/
void
dbprog_ttitle_focuschg(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;
	int			i;

	switch (p->reason) {
	case XmCR_FOCUS:
		if (sel_pos < 0) {
			for (i = 0; i < (int) s->tot_trks; i++) {
				if (dbp->track[i].title == NULL) {
					ind_pos = i + 1;

					XmListSelectPos(
						widgets.dbprog.trk_list,
						ind_pos,
						False
					);
					XmListSetBottomPos(
						widgets.dbprog.trk_list,
						ind_pos
					);
					break;
				}
			}
		}
		break;

	case XmCR_LOSING_FOCUS:
		if (ind_pos > 0) {
			/* Save a copy of the ttitle */
			if (sav_ttitle != NULL)
				MEM_FREE(sav_ttitle);
			sav_ttitle = get_text_string(
				widgets.dbprog.ttitle_txt, TRUE
			);
			if (sav_ttitle != NULL && sav_ttitle[0] == '\0') {
				MEM_FREE(sav_ttitle);
				sav_ttitle = NULL;
			}

			XmListDeselectPos(widgets.dbprog.trk_list, ind_pos);
			ind_pos = -1;
		}
		break;

	default:
		break;
	}
}


/*
 * dbprog_ttitle_new
 *	Track title editor text widget callback function.
 */
void
dbprog_ttitle_new(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;
	char			*cp;
	int			*pos,
				i;
	XmListCallbackStruct	cb;

	if (p->reason == XmCR_VALUE_CHANGED) {
		if ((cp = get_text_string(w, TRUE)) == NULL)
			return;

		if (cp == NULL || *cp == '\0')
			title_edited = FALSE;
		else if (sel_pos < 0)
			title_edited = TRUE;

		MEM_FREE(cp);
		return;
	}
	else if (p->reason == XmCR_ACTIVATE) {
		/* Force w to be the ttitle_txt because this callback
		 * function can also be called as a result of clicking
		 * on the apply_btn.
		 */
		if (w == widgets.dbprog.apply_btn) {
			/* Put focus back on the ttitle_txt widget */
			XmProcessTraversal(
				widgets.dbprog.ttitle_txt,
				XmTRAVERSE_CURRENT
			);
			w = widgets.dbprog.ttitle_txt;

			/* If a saved ttitle string exists, put it back */
			if (sav_ttitle != NULL) {
				set_text_string(w, sav_ttitle, TRUE);
				MEM_FREE(sav_ttitle);
				sav_ttitle = NULL;
			}
		}
	}
	else
		return;

	if (sel_pos > 0 &&
	    XmListGetSelectedPos(widgets.dbprog.trk_list, &pos, &i)) {
		if ((cp = get_text_string(w, TRUE)) == NULL)
			return;

		if (pos == NULL) {
			MEM_FREE(cp);
			return;
		}

		if (i != 1)
			MEM_FREE(cp);
		else {
			/* Update track list entry */
			dbprog_listupd_ent(s, (*pos)-1, cp, FALSE);

			XmListDeselectPos(widgets.dbprog.trk_list, sel_pos);
			sel_pos = -1;

			if (!util_newstr(&dbp->track[(*pos)-1].title, cp)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			MEM_FREE(cp);

			dbp->flags |= CDINFO_CHANGED;
			if (!app_data.cdinfo_inetoffln)
			    XtSetSensitive(widgets.dbprog.submit_btn, True);
			XtSetSensitive(widgets.dbprog.addpgm_btn, False);
			XtSetSensitive(widgets.dbprog.extt_btn, False);
			XtSetSensitive(widgets.dbprog.tcredits_btn, False);

			/* Update button labels */
			dbprog_extt_lblupd(NULL);
			dbprog_tcred_lblupd(NULL);

			/* Update the track details and credits windows if
			 * necessary
			 */
			if (extt_pos == (*pos)-1 &&
			    (XtIsManaged(widgets.dbextt.form) ||
			     XtIsManaged(widgets.credits.form))) {
				dbprog_set_track_title(
					s->trkinfo[extt_pos].trkno,
					dbp->track[extt_pos].title
				);
			}

			/* Update the main window if necessary */
			if (di_curtrk_pos(s) == (*pos)-1) {
				dpy_ttitle(s);

				/* Update curfile */
				dbprog_curfileupd();
			}
		}

		set_text_string(w, "", FALSE);

		XtFree((XtPointer) pos);
	}
	else {
		/* Pressing Return in this case is equivalent to clicking
		 * on the first title-less track on the track list.
		 */
		for (i = 0; i < (int) s->tot_trks; i++) {
			if (dbp->track[i].title == NULL) {
				cb.item_position = i + 1;
				cb.reason = XmCR_BROWSE_SELECT;
				cb.event = p->event;

				dbprog_trklist_select(
					widgets.dbprog.trk_list,
					(XtPointer) s,
					(XtPointer) &cb
				);
				break;
			}
		}
	}

	for (i = 0; i < (int) s->tot_trks; i++) {
		if (dbp->track[i].title == NULL) {
			ind_pos = i + 1;

			XmListSelectPos(
				widgets.dbprog.trk_list,
				ind_pos,
				False
			);
			XmListSetBottomPos(
				widgets.dbprog.trk_list,
				ind_pos
			);
			break;
		}
	}
}


/*
 * dbprog_pgmseq_verify
 *	Play sequence editor text widget user-input verification callback.
 */
/*ARGSUSED*/
void
dbprog_pgmseq_verify(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTextVerifyCallbackStruct
		*p = (XmTextVerifyCallbackStruct *)(void *) call_data;
	int	i;
	char	prev,
		*currstr;

	if (p->reason != XmCR_MODIFYING_TEXT_VALUE)
		return;

	p->doit = True;

	if (p->startPos != p->endPos)
		/* Deleting text, no verification needed */
		return;

	currstr = get_text_string(w, FALSE);

	switch (p->text->format) {
	case XmFMT_8_BIT:
		if (p->currInsert > 0 && currstr != NULL)
			prev = currstr[p->currInsert - 1];
		else
			prev = ',';

		for (i = 0; i < p->text->length; i++) {
			/* Only allowed input is digits, ',' or ' ' */
			if (p->text->ptr[i] == ',' ||
			    p->text->ptr[i] == ' ') {
				if (prev == ',') {
					p->doit = False;
					break;
				}
				/* Substitute ' ' with ',' */
				if (p->text->ptr[i] == ' ')
					p->text->ptr[i] = ',';
			}
			else if (!isdigit((int) p->text->ptr[i])) {
				p->doit = False;
				break;
			}
			prev = p->text->ptr[i];
		}
		break;

	case XmFMT_16_BIT:
	default:
		/* Don't know how to handle other character sets yet */
		p->doit = False;
		break;
	}

	if (currstr != NULL)
		MEM_FREE(currstr);
}


/*
 * dbprog_pgmseq_txtchg
 *	Play sequence editor text widget text changed callback.
 */
/*ARGSUSED*/
void
dbprog_pgmseq_txtchg(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct	*p = (XmAnyCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	if (s->onetrk_prog) {
		/* Currently playing a single-track program: clear it first */
		s->onetrk_prog = FALSE;
		dbprog_progclear(s);
	}

	/* Disable shuffle mode */
	if (s->shuffle) {
		di_shuffle(s, FALSE);
		set_shuffle_btn(FALSE);
	}

	/* Disable a->b mode */
	if (s->segplay != SEGP_NONE)
		s->segplay = SEGP_NONE;

	if (dbp->playorder != NULL)
		MEM_FREE(dbp->playorder);
	dbp->playorder = get_text_string(w, FALSE);

	if ((s->program = dbprog_pgm_active()) == FALSE)
		dbprog_progclear(s);

	/* Update display */
	dpy_progmode(s, FALSE);

	XtSetSensitive(widgets.dbprog.clrpgm_btn, (Boolean) s->program);
	XtSetSensitive(widgets.dbprog.savepgm_btn, (Boolean) s->program);
}


/*
 * dbprog_addpgm
 *	Program Add button callback.
 */
/*ARGSUSED*/
void
dbprog_addpgm(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	char		tmpbuf[8];

	if (sel_pos < 0 || s->mode == MOD_BUSY || s->mode == MOD_NODISC) {
		cd_beep();
		return;
	}

	if (s->onetrk_prog) {
		/* Currently playing a single-track program: clear it first */
		if (w == widgets.dbprog.addpgm_btn)
			s->onetrk_prog = FALSE;
		dbprog_progclear(s);
	}

	/* Disable shuffle mode */
	if (s->shuffle) {
		di_shuffle(s, FALSE);
		set_shuffle_btn(FALSE);
	}

	/* Disable a->b mode */
	if (s->segplay != SEGP_NONE)
		s->segplay = SEGP_NONE;

	if (dbp->playorder != NULL && dbp->playorder[0] == '\0') {
		MEM_FREE(dbp->playorder);
		dbp->playorder = NULL;
	}

	if (dbp->playorder == NULL) {
		(void) sprintf(tmpbuf, "%u", s->trkinfo[sel_pos-1].trkno);
		if (!util_newstr(&dbp->playorder, tmpbuf)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
	}
	else {
		(void) sprintf(tmpbuf, ",%u", s->trkinfo[sel_pos-1].trkno);
		dbp->playorder = (char *) MEM_REALLOC(
			"addpgm",
			dbp->playorder,
			strlen(dbp->playorder) + strlen(tmpbuf) + 1
		);
		if (dbp->playorder == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		(void) strcat(dbp->playorder, tmpbuf);
	}

	if (!s->onetrk_prog) {
		char	*str;
		int	n;

		set_text_string(
			widgets.dbprog.pgmseq_txt, dbp->playorder, FALSE
		);

		str = XmTextGetString(widgets.dbprog.pgmseq_txt);
		n = strlen(str);
		XtFree(str);

		XmTextSetInsertionPosition(widgets.dbprog.pgmseq_txt, n);
	}

	s->program = TRUE;

	/* Update display */
	dpy_progmode(s, FALSE);

	if (!s->onetrk_prog) {
		XtSetSensitive(widgets.dbprog.clrpgm_btn, True);
		XtSetSensitive(widgets.dbprog.savepgm_btn, True);
	}

	XtSetSensitive(widgets.dbprog.addpgm_btn, False);
	XtSetSensitive(widgets.dbprog.extt_btn, False);
	XtSetSensitive(widgets.dbprog.tcredits_btn, False);

	/* Put focus on clear button */
	XmProcessTraversal(widgets.dbprog.clrpgm_btn, XmTRAVERSE_CURRENT);

	/* Update button labels */
	dbprog_extt_lblupd(NULL);
	dbprog_tcred_lblupd(NULL);

	if (sel_pos > 0) {
		XmListDeselectPos(widgets.dbprog.trk_list, sel_pos);
		sel_pos = -1;
	}

	set_text_string(widgets.dbprog.ttitle_txt, "", FALSE);
}


/*
 * dbprog_clrpgm
 *	Program Clear button callback.
 */
/*ARGSUSED*/
void
dbprog_clrpgm(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (s->onetrk_prog) {
		/* Currently playing a single-track program: clear it first */
		s->onetrk_prog = FALSE;
		dbprog_progclear(s);
	}
	else if (sel_pos > 0) {
		XmListDeselectPos(widgets.dbprog.trk_list, sel_pos);
		sel_pos = -1;
		set_text_string(widgets.dbprog.ttitle_txt, "", FALSE);
	}

	set_text_string(widgets.dbprog.pgmseq_txt, "", FALSE);
	XtSetSensitive(widgets.dbprog.addpgm_btn, False);
	XtSetSensitive(widgets.dbprog.clrpgm_btn, False);
	XtSetSensitive(widgets.dbprog.savepgm_btn, False);
	XtSetSensitive(widgets.dbprog.extt_btn, False);
	XtSetSensitive(widgets.dbprog.tcredits_btn, False);

	/* Update button labels */
	dbprog_extt_lblupd(NULL);
	dbprog_tcred_lblupd(NULL);

	/* Put focus on OK button */
	XmProcessTraversal(widgets.dbprog.ok_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_savepgm
 *	Program Save button callback.
 */
/*ARGSUSED*/
void
dbprog_savepgm(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_ret_t	ret;

	if (!s->program)
		return;

	/* Save user-defined track program to file */
	if ((ret = cdinfo_save_prog(s)) != 0) {
		cd_beep();
		DBGPRN(DBG_CDI)(errfp, "cdinfo_save_prog: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
		return;
	}

	XtSetSensitive(widgets.dbprog.savepgm_btn, False);

	/* Put focus on OK button */
	XmProcessTraversal(widgets.dbprog.ok_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_submit
 *	CDDB Submit pushbutton callback.
 */
/*ARGSUSED*/
void
dbprog_submit(Widget w, XtPointer client_data, XtPointer call_data)
{
	(void) cd_confirm_popup(
		app_data.str_confirm, app_data.str_submit,
		(XtCallbackProc) dbprog_submit_yes, client_data,
		(XtCallbackProc) NULL, NULL
	);
}


/*
 * dbprog_submit_popup
 *	CDDB Submit dialog popup timer function
 */
/*ARGSUSED*/
void
dbprog_submit_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	dbprog_do_submit_popup();
}


/*
 * dbprog_submit_yes
 *	CDDB Submit dialog "Yes" pushbutton callback.
 */
/*ARGSUSED*/
void
dbprog_submit_yes(Widget w, XtPointer client_data, XtPointer call_data)
{
	(void) dbprog_dbsubmit((curstat_t *)(void *) client_data);
}


/*
 * dbprog_flush
 *	CD information cache FLUSH button callback.
 */
/*ARGSUSED*/
void
dbprog_flush(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_ret_t	ret;

	/* Change to watch cursor */
	cd_busycurs(TRUE, CURS_ALL);

	if ((ret = cdinfo_flush(s)) != 0) {
		cd_beep();
		DBGPRN(DBG_CDI)(errfp, "cdinfo_flush: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_ALL);
}


/*
 * dbprog_load
 *	CD information RELOAD button callback.
 */
/*ARGSUSED*/
void
dbprog_load(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (s->mode == MOD_BUSY || s->mode == MOD_NODISC) {
		cd_beep();
		return;
	}

	/* Re-load CD information */
	dbprog_dbget(s);
}


/*
 * dbprog_stop_load_yes
 *	CD information STOP LOAD dialog 'yes' button callback.
 */
/*ARGSUSED*/
void
dbprog_stop_load_yes(Widget w, XtPointer client_data, XtPointer call_data)
{
	stopload_active = FALSE;
	cdinfo_load_cancel();
}


/*
 * dbprog_stop_load_no
 *	CD information STOP LOAD dialog 'no' button callback.
 */
/*ARGSUSED*/
void
dbprog_stop_load_no(Widget w, XtPointer client_data, XtPointer call_data)
{
	stopload_active = FALSE;
}


/*
 * dbprog_ok
 *	Pop down CD info/program window.
 */
/*ARGSUSED*/
void
dbprog_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPushButtonCallbackStruct
			*p = (XmPushButtonCallbackStruct *)(void *) call_data;
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (XtIsManaged(widgets.dlist.form)) {
		/* Force a popdown of the dlist window */
		dbprog_dlist_cancel(
			widgets.dlist.cancel_btn,
			(XtPointer) s,
			(XtPointer) p
		);
	}
	if (XtIsManaged(widgets.fullname.form)) {
		/* Force a popdown of the fullname window */
		dbprog_fullname_ok(
			widgets.fullname.ok_btn,
			(XtPointer) s,
			(XtPointer) p
		);
	}
	if (XtIsManaged(widgets.dbextd.form)) {
		/* Force a popdown of the disc details window */
		dbprog_extd_ok(
			widgets.dbextd.ok_btn,
			(XtPointer) s,
			(XtPointer) p
		);
	}
	if (XtIsManaged(widgets.dbextt.form)) {
		/* Force a popdown of the track details window */
		dbprog_extt_ok(
			widgets.dbextt.ok_btn,
			(XtPointer) s,
			(XtPointer) p
		);
	}
	if (XtIsManaged(widgets.credits.form)) {
		/* Force a popdown of the credits window */
		dbprog_credits_popdown(
			widgets.credits.ok_btn,
			(XtPointer) s,
			(XtPointer) p
		);
	}
	if (XtIsManaged(widgets.segments.form)) {
		/* Force a popdown of the segments window */
		dbprog_segments_popdown(
			widgets.segments.ok_btn,
			(XtPointer) s,
			(XtPointer) p
		);
	}

	/* Pop down the CD info/program window */
	XtUnmapWidget(XtParent(widgets.dbprog.form));
	XtUnmanageChild(widgets.dbprog.form);
}


/*
 * dbprog_do_clear
 *	Changed-save dialog box pushbuttons callback.
 */
/*ARGSUSED*/
void
dbprog_do_clear(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = curstat_addr();
	word32_t	flags = (word32_t)(unsigned long) client_data;

	dbp->flags &= ~CDINFO_CHANGED;
	s->flags |= (flags & 0xffff);

	/* Clear in-core CD information */
	dbprog_dbclear(s, FALSE);
}


/*
 * dbprog_timedpy
 *	Toggle the time display mode in the track list.
 */
/*ARGSUSED*/
void
dbprog_timedpy(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmRowColumnCallbackStruct	*p =
		(XmRowColumnCallbackStruct *)(void *) call_data;
	XmToggleButtonCallbackStruct	*q;
	curstat_t			*s = (curstat_t *)(void *) client_data;

	if (p == NULL)
		return;

	q = (XmToggleButtonCallbackStruct *)(void *) p->callbackstruct;

	if (!q->set)
		return;

	/* Overload the function of these buttons to also
	 * dump the contents of the cdinfo_incore_t structure
	 * in debug mode
	 */
	if (app_data.debug & DBG_ALL)
		cdinfo_dump_incore(s);

	if (p->widget == widgets.dbprog.tottime_btn) {
		if (time_mode == TIME_TOTAL)
			return;	/* No change */

		time_mode = TIME_TOTAL;
	}
	else if (p->widget == widgets.dbprog.trktime_btn) {
		if (time_mode == TIME_TRACK)
			return;	/* No change */

		time_mode = TIME_TRACK;
	}
	else
		return;	/* Invalid widget */

	if (s->mode != MOD_BUSY && s->mode != MOD_NODISC)
		/* Update track list with new time display mode */
		dbprog_listupd_all(s, FALSE);
}


/*
 * dbprog_set_changed
 *	Set the flag indicating that the user has made changes to the
 *	in-core CD information.
 */
/*ARGSUSED*/
void
dbprog_set_changed(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmAnyCallbackStruct *p = (XmAnyCallbackStruct *)(void *) call_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	/* Setup of the track details window is not a user change */
	if (!extt_setup) {
		dbp->flags |= CDINFO_CHANGED;
		if (!app_data.cdinfo_inetoffln)
			XtSetSensitive(widgets.dbprog.submit_btn, True);
	}
}


/*
 * dbprog_fullname
 *	Pop up/down the fullname window.
 */
void
dbprog_fullname(Widget w, XtPointer client_data, XtPointer call_data)
{
	bool_t		user_req = (bool_t)(unsigned long) client_data;
	curstat_t	*s = curstat_addr();
	int		new_mode;
	cdinfo_fname_t	*p;
	XmString	xs;
	Widget		nextw;
	char		*str,
			buf2[32],
			buf[STR_BUF_SZ * 2];
	bool_t		fname_err;

	/* Check existing fullname for correctness, if needed */
	fname_err = !dbprog_fullname_ck(s);
	if (fname_err && user_req && fname_mode != FNAME_NONE)
		return;

	/* Set fullname window mode */
	if (w == widgets.dbprog.fullname_btn) {
		if (user_req && fname_mode == FNAME_DISC) {
			/* Pop down the full name window */
			dbprog_fullname_ok(
				widgets.fullname.ok_btn,
				(XtPointer) s, call_data
			);
			return;
		}
		new_mode = FNAME_DISC;
		p = &dbp->disc.artistfname;
	}
	else if (w == widgets.dbextt.fullname_btn) {
		if (user_req && fname_mode == FNAME_TRACK) {
			/* Pop down the full name window */
			dbprog_fullname_ok(
				widgets.fullname.ok_btn,
				(XtPointer) s, call_data
			);
			return;
		}
		new_mode = FNAME_TRACK;
		p = &dbp->track[extt_pos].artistfname;
	}
	else if (w == widgets.credits.fullname_btn) {
		if (user_req && fname_mode == FNAME_CREDITS) {
			/* Pop down the full name window */
			dbprog_fullname_ok(
				widgets.fullname.ok_btn,
				(XtPointer) s, call_data
			);
			return;
		}
		new_mode = FNAME_CREDITS;
		p = &w_cred.crinfo.fullname;
	}
	else
		/* Invalid widget */
		return;

	if (fname_mode != FNAME_NONE && new_mode != fname_mode) {
		/* Pop down the full name window */
		dbprog_fullname_ok(
			widgets.fullname.ok_btn,
			(XtPointer) s, call_data
		);
	}

	fname_mode = new_mode;

	/* Update fullname window head label */
	switch (fname_mode) {
	case FNAME_DISC:
		(void) sprintf(buf, "%.31s", app_data.str_albumartist);
		break;
	case FNAME_TRACK:
		(void) sprintf(buf2, "%.31s", app_data.str_trackartist);
		(void) sprintf(buf, buf2, s->trkinfo[extt_pos].trkno);
		break;
	case FNAME_CREDITS:
		(void) sprintf(buf, "%.31s", app_data.str_credit);
		break;
	}
	(void) sprintf(buf, "%s\n%.90s", buf, app_data.str_fnameguide);

	xs = create_xmstring(buf, NULL, XmSTRING_DEFAULT_CHARSET, TRUE);
	XtVaSetValues(widgets.fullname.head_lbl,
		XmNlabelString, xs,
		NULL
	);
	XmStringFree(xs);

	/* Set autogen button default state */
	if ((str = dbprog_autoname(p)) == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	if (p->dispname != NULL && strcmp(p->dispname, str) != 0) {
		XmToggleButtonSetState(
			widgets.fullname.autogen_btn, False, False
		);
		XtVaSetValues(
			widgets.fullname.dispname_txt,
			XmNeditable, True,
			XmNcursorPositionVisible, True,
			XmNcursorPosition, 0,
			NULL
		);
	}
	else {
		XmToggleButtonSetState(
			widgets.fullname.autogen_btn, True, False
		);
		XtVaSetValues(
			widgets.fullname.dispname_txt,
			XmNeditable, False,
			XmNcursorPositionVisible, False,
			XmNcursorPosition, 0,
			NULL
		);
	}
	MEM_FREE(str);

	/* Update fullname widgets */
	if (p->dispname != NULL)
		set_text_string(
			widgets.fullname.dispname_txt, p->dispname, TRUE
		);
	else
		set_text_string(widgets.fullname.dispname_txt, "", FALSE);

	if (p->lastname != NULL)
		set_text_string(
			widgets.fullname.lastname_txt, p->lastname, TRUE
		);
	else
		set_text_string(widgets.fullname.lastname_txt, "", FALSE);

	if (p->firstname != NULL)
		set_text_string(	
			widgets.fullname.firstname_txt, p->firstname, TRUE
		);
	else
		set_text_string(widgets.fullname.firstname_txt, "", FALSE);

	if (p->the != NULL) {
		XmToggleButtonSetState(widgets.fullname.the_btn, True, False);
		set_text_string(widgets.fullname.the_txt, p->the, TRUE);
		XtSetSensitive(widgets.fullname.the_txt, True);
	}
	else {
		XmToggleButtonSetState(widgets.fullname.the_btn, False, False);
		set_text_string(widgets.fullname.the_txt, "", FALSE);
		XtSetSensitive(widgets.fullname.the_txt, False);
	}

	/* Pop up the fullname window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	if (!XtIsManaged(widgets.fullname.form)) {
		XtManageChild(widgets.fullname.form);

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.fullname.form));

		XtMapWidget(XtParent(widgets.fullname.form));
	}

	/* Put input focus on the appropriate widget */
	if (p->firstname == NULL)
		nextw = widgets.fullname.firstname_txt;
	else if (p->lastname == NULL)
		nextw = widgets.fullname.lastname_txt;
	else
		nextw = widgets.fullname.ok_btn;

	XmProcessTraversal(nextw, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_fullname_autogen
 *	Full name window autogen button callback.
 */
/*ARGSUSED*/
void
dbprog_fullname_autogen(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;
	cdinfo_fname_t	*ent;

	XtVaSetValues(widgets.fullname.dispname_txt,
		XmNeditable, !p->set,
		XmNcursorPositionVisible, !p->set,
		XmNcursorPosition, 0,
		NULL
	);

	switch (fname_mode) {
	case FNAME_DISC:
		ent = &dbp->disc.artistfname;
		break;
	case FNAME_TRACK:
		if (extt_pos < 0)
			return;
		ent = &dbp->track[extt_pos].artistfname;
		break;
	case FNAME_CREDITS:
		ent = &w_cred.crinfo.fullname;
		break;
	default:
		return;
	}

	if (p->set) {
		char	*cp;

		/* Auto-generate the name */
		if ((cp = dbprog_autoname(ent)) == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		if (cp[0] != '\0') {
			set_text_string(
				widgets.fullname.dispname_txt, cp, TRUE
			);
		}
		MEM_FREE(cp);

		/* Put focus on firstname field */
		XmProcessTraversal(
			widgets.fullname.firstname_txt,
			XmTRAVERSE_CURRENT
		);
	}
	else {
		/* The Display name is now editable, so put focus on it */
		XmProcessTraversal(
			widgets.fullname.dispname_txt,
			XmTRAVERSE_CURRENT
		);
	}
}


/*
 * dbprog_fullname_ok
 *	Full name window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_fullname_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	Widget		nextw;

	switch (fname_mode) {
	case FNAME_DISC:
		nextw = NULL;
		break;
	case FNAME_TRACK:
		nextw = widgets.dbextt.year_txt;
		break;
	case FNAME_CREDITS:
		nextw = widgets.credits.notes_txt;
		break;
	default:
		return;
	}

	/* Check fullname for correctness */
	if (!dbprog_fullname_ck(s))
		return;

	/* Pop down the fullname popup */
	XtUnmapWidget(XtParent(widgets.fullname.form));
	XtUnmanageChild(widgets.fullname.form);

	if (nextw != NULL)
		/* Put keyboard focus on next widget */
		XmProcessTraversal(nextw, XmTRAVERSE_CURRENT);

	/* Update curfile if needed */
	if (fname_changed && fname_mode == FNAME_DISC)
		dbprog_curfileupd();

	fname_changed = FALSE;
	fname_mode = FNAME_NONE;
}


/*
 * dbprog_genre_sel
 *	Genre selector callback
 */
/*ARGSUSED*/
void
dbprog_genre_sel(Widget w, XtPointer client_data, XtPointer call_data)
{
	cdinfo_genre_t	*p = (cdinfo_genre_t *)(void *) client_data,
			*gp,
			*genrep,
			*subgenrep,
			*genre2p,
			*subgenre2p;
	char		*newid,
			**ent;
	Widget		pw;
	int		pos;

	if (dbp->genrelist == NULL)
		return;

	pw = XtParent(w);
	if (pw == widgets.dbextd.genre_menu[0]) {
		pos = -1;
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = genrep;
		ent = &dbp->disc.genre;
	}
	else if (pw == widgets.dbextd.genre_menu[1]) {
		pos = -1;
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = genre2p;
		ent = &dbp->disc.genre2;
	}
	else if (pw == widgets.dbextt.genre_menu[0]) {
		if (extt_pos < 0)
			return;
		pos = extt_pos;
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = genrep;
		ent = &dbp->track[pos].genre;
	}
	else if (pw == widgets.dbextt.genre_menu[1]) {
		if (extt_pos < 0)
			return;
		pos = extt_pos;
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = genre2p;
		ent = &dbp->track[pos].genre2;
	}
	else
		return;	/* Invalid widget */

	if (p != gp) {
		dbp->flags |= CDINFO_CHANGED;
		if (!app_data.cdinfo_inetoffln)
			XtSetSensitive(widgets.dbprog.submit_btn, True);
	}

	if (*ent != NULL) {
		MEM_FREE(*ent);
		*ent = NULL;
	}

	if (p != NULL) {
		newid = NULL;

		for (genrep = dbp->genrelist; genrep != NULL;
		     genrep = genrep->next) {
			if (strcmp(genrep->id, p->id) != 0)
				continue;

			if (genrep->child != NULL) {
				/* Look for the first "General xxx" where
				 * xxx is the main genre, and set
				 * the default subgenre to it.
				 */
				for (subgenrep = genrep->child;
				     subgenrep != NULL;
				     subgenrep = subgenrep->next) {
					if (strncmp(subgenrep->name,
						    "General ", 8) == 0 &&
					    strcmp(subgenrep->name + 8,
						    genrep->name) == 0)
						break;
				}

				if (subgenrep != NULL) {
					/* Found it */
					newid = subgenrep->id;
				}
				else {
					/* Not found: just set the default
					 * subgenre to the first one in the
					 * list
					 */
					newid = genrep->child->id;
				}
			}
			else {
				/* This shouldn't happen, but in case a
				 * there is associated no subgenres,
				 * then just set it to the primary genre
				 * itself
				 */
				newid = genrep->id;
			}
		}

		if (!util_newstr(ent, newid)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
	}

	dbprog_genreupd(pos);
}


/*
 * dbprog_subgenre_sel
 *	Subgenre selector callback
 */
/*ARGSUSED*/
void
dbprog_subgenre_sel(Widget w, XtPointer client_data, XtPointer call_data)
{
	cdinfo_genre_t	*p = (cdinfo_genre_t *)(void *) client_data,
			*gp,
			*genrep,
			*subgenrep,
			*genre2p,
			*subgenre2p;
	char		**ent;
	Widget		pw,
			optw,
			nonew;
	int		pos;

	if (dbp->genrelist == NULL)
		return;

	pw = XtParent(w);
	if (pw == widgets.dbextd.subgenre_menu[0]) {
		pos = -1;
		optw = widgets.dbextd.subgenre_opt[0];
		nonew = widgets.dbextd.genre_none_btn[0];
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = subgenrep;
		ent = &dbp->disc.genre;
	}
	else if (pw == widgets.dbextd.subgenre_menu[1]) {
		pos = -1;
		optw = widgets.dbextd.subgenre_opt[1];
		nonew = widgets.dbextd.genre_none_btn[1];
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = subgenre2p;
		ent = &dbp->disc.genre2;
	}
	else if (pw == widgets.dbextt.subgenre_menu[0]) {
		if (extt_pos < 0)
			return;

		pos = extt_pos;
		optw = widgets.dbextt.subgenre_opt[0];
		nonew = widgets.dbextt.genre_none_btn[0];
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = subgenrep;
		ent = &dbp->track[pos].genre;
	}
	else if (pw == widgets.dbextt.subgenre_menu[1]) {
		if (extt_pos < 0)
			return;

		pos = extt_pos;
		optw = widgets.dbextt.subgenre_opt[1];
		nonew = widgets.dbextt.genre_none_btn[1];
		dbprog_curgenre(pos, &genrep, &subgenrep,
				&genre2p, &subgenre2p);
		gp = subgenre2p;
		ent = &dbp->track[pos].genre2;
	}
	else
		return;	/* Invalid widget */

	if (p != gp) {
		dbp->flags |= CDINFO_CHANGED;
		if (!app_data.cdinfo_inetoffln)
			XtSetSensitive(widgets.dbprog.submit_btn, True);
	}

	if (*ent != NULL) {
		MEM_FREE(*ent);
		*ent = NULL;
	}

	if (p == NULL || p->id == NULL) {
		XtVaSetValues(optw, XmNmenuHistory, nonew, NULL);
	}
	else if (!util_newstr(ent, p->id)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	dbprog_genreupd(pos);
}


/*
 * dbprog_role_sel
 *	Role selector callback
 */
/*ARGSUSED*/
void
dbprog_role_sel(Widget w, XtPointer client_data, XtPointer call_data)
{
	cdinfo_role_t		*p = (cdinfo_role_t *)(void *) client_data,
				*q,
				*sr;
	int			i;
	Dimension		x;
	Arg			arg[10];
	static cdinfo_role_t	*save_role = NULL,
				*save_trole = NULL,
				*save_srole = NULL;

	if (dbp->rolelist == NULL)
		return;

	switch (credits_mode) {
	case CREDITS_DISC:
		sr = save_role;
		break;
	case CREDITS_TRACK:
		sr = save_trole;
		break;
	case CREDITS_SEG:
		sr = save_srole;
		break;
	default:
		return;	/* Invalid mode */
	}

	if (cred_pos > 0)
		XtSetSensitive(widgets.credits.mod_btn, True);
	else
		XtSetSensitive(widgets.credits.add_btn, True);

	/* If the primary role changed, re-create the associated
	 * sub-role menu entries
	 */
	if (sr != NULL) {
		for (q = sr->child; q != NULL; q = q->next) {
			/* Delete old sub-role entries */
			if (q->aux != NULL) {
				XtUnmanageChild((Widget) q->aux);
				XtDestroyWidget((Widget) q->aux);
				q->aux = NULL;
			}
		}
	}

	if (p != NULL) {
		for (q = p->child; q != NULL; q = q->next) {
			/* Create new sub-role entries */
			i = 0;
			XtSetArg(arg[i], XmNinitialResourcesPersistent,
				 False); i++;
			XtSetArg(arg[i], XmNshadowThickness, 2); i++;
			q->aux = (void *) XmCreatePushButton(
				widgets.credits.subrole_menu,
				q->name,
				arg,
				i
			);
			XtManageChild((Widget) q->aux);
			register_activate_cb((Widget) q->aux,
					     dbprog_subrole_sel, q);

			if (q == p->child) {
				/* Set default sub-role to the first entry */
				XtVaSetValues(
					widgets.credits.subrole_opt,
					XmNmenuHistory, q->aux,
					NULL
				);
				w_cred.crinfo.role = q;
			}
		}
	}
	else {
		/* Set sub-role selector to "none" */
		XtVaSetValues(
			widgets.credits.subrole_opt,
			XmNmenuHistory, widgets.credits.subrole_none_btn,
			NULL
		);
		w_cred.crinfo.role = NULL;
	}

	switch (credits_mode) {
	case CREDITS_DISC:
		save_role = p;
		break;
	case CREDITS_TRACK:
		save_trole = p;
		break;
	case CREDITS_SEG:
		save_srole = p;
		break;
	default:
		break;
	}

	/* Hack: Force the subrole option menu to resize to match
	 * the selected label.
	 */
	XtVaGetValues(widgets.credits.form, XmNwidth, &x, NULL);
	XtVaSetValues(widgets.credits.form, XmNwidth, x+1, NULL);
	XtVaSetValues(widgets.credits.form, XmNwidth, x, NULL);
}


/*
 * dbprog_subrole_sel
 *	Subrole selector callback
 */
/*ARGSUSED*/
void
dbprog_subrole_sel(Widget w, XtPointer client_data, XtPointer call_data)
{
	cdinfo_role_t	*p = (cdinfo_role_t *)(void *) client_data;

	if (dbp->rolelist == NULL)
		return;

	if (p == NULL) {
		/* "None" selected: set the primary role to "None" too */
		XtVaSetValues(
			widgets.credits.prirole_opt,
			XmNmenuHistory, widgets.credits.prirole_none_btn,
			NULL
		);
		XtCallCallbacks(
			widgets.credits.prirole_none_btn,
			XmNactivateCallback, (XtPointer) NULL
		);
	}

	if (cred_pos > 0)
		XtSetSensitive(widgets.credits.mod_btn, True);
	else
		XtSetSensitive(widgets.credits.add_btn, True);

	w_cred.crinfo.role = p;
}


/*
 * dbprog_extd
 *	Pop up/down the disc details window.
 */
void
dbprog_extd(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.dbextd.form)) {
		/* Pop down the Disc details window */
		dbprog_extd_ok(w, client_data, call_data);
		return;
	}

	/* Update the disc title */
	dbprog_set_disc_title(s->cur_disc, dbp->disc.artist, dbp->disc.title);

	/* Pop up the disc details window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	XtManageChild(widgets.dbextd.form);
	if (first) {
		first = FALSE;
		/* Set up window position */
		cd_dialog_setpos(XtParent(widgets.dbextd.form));
	}
	XtMapWidget(XtParent(widgets.dbextd.form));

	/* Put input focus on the OK button */
	XmProcessTraversal(widgets.dbextd.ok_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_extd_compilation
 *	Disc details compilation toggle button callback
 */
/*ARGSUSED*/
void
dbprog_extd_compilation(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;

	if (dbp->disc.compilation == p->set)
		/* No change */
		return;

	dbp->disc.compilation = p->set;
	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_extd_ok
 *	Disc details window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_extd_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Pop down the Disc details window */
	XtUnmapWidget(XtParent(widgets.dbextd.form));
	XtUnmanageChild(widgets.dbextd.form);
}


/*
 * dbprog_extt
 *	Pop up/down the track details window.
 */
void
dbprog_extt(Widget w, XtPointer client_data, XtPointer call_data)
{
	bool_t		from_main = (bool_t)(unsigned long) client_data,
			managed;
	curstat_t	*s = curstat_addr();
	static bool_t	first = TRUE;

	managed = (bool_t) XtIsManaged(widgets.dbextt.form);
	if (managed) {
		if (from_main) {
			/* Pop down the Track details window */
			dbprog_extt_ok(w, client_data, call_data);
			return;
		}
		else if (credits_mode != CREDITS_TRACK)
			extt_pos = -1;
	}

	if (sel_pos > 0)
		extt_pos = sel_pos - 1;
	else if (extt_pos < 0)
		return;

	/* Enter track details setup mode */
	extt_setup = TRUE;

	/* Update widgets */
	dbprog_exttupd(s, extt_pos);

	if (!managed) {
		/* Pop up the track details popup.
		 * The dialog has mappedWhenManaged set to False,
		 * so we have to map/unmap explicitly.  The reason
		 * for this is we want to avoid a screen glitch when
		 * we move the window in cd_dialog_setpos(), so we
		 * map the window afterwards.
		 */
		XtManageChild(widgets.dbextt.form);
		if (first) {
			first = FALSE;
			/* Set window position */
			cd_dialog_setpos(XtParent(widgets.dbextt.form));
		}
		XtMapWidget(XtParent(widgets.dbextt.form));

		/* Put input focus on the OK button */
		XmProcessTraversal(
			widgets.dbextt.ok_btn,
			XmTRAVERSE_CURRENT
		);
	}

	/* Exit track details setup mode */
	extt_setup = FALSE;
}


/*
 * dbprog_extt_prev
 *	Track details window and credits window previous track button callback.
 */
/*ARGSUSED*/
void
dbprog_extt_prev(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	/* Check full name data and abort if needed */
	if (!dbprog_fullname_ck(s)) {
		cd_beep();
		return;
	}

	/* Put input focus on the OK button */
	if (XtIsManaged(widgets.dbextt.form))
		XmProcessTraversal(widgets.dbextt.ok_btn, XmTRAVERSE_CURRENT);
	if (XtIsManaged(widgets.credits.form))
		XmProcessTraversal(widgets.credits.ok_btn, XmTRAVERSE_CURRENT);

	if (extt_pos <= 0) {
		cd_beep();
		return;
	}

	/* Select the previous track */
	if (sel_pos > 0) {
		XmListDeselectPos(widgets.dbprog.trk_list, sel_pos);
		sel_pos = -1;
	}
	XmListSelectPos(widgets.dbprog.trk_list, extt_pos, True);

	/* Scroll track list if necessary */
	dbprog_list_autoscroll(
		widgets.dbprog.trk_list,
		(int) s->tot_trks,
		extt_pos + 1
	);

	/* Update fullname window if necessary */
	if (fname_mode == FNAME_TRACK) {
		dbprog_fullname(widgets.dbextt.fullname_btn,
				(XtPointer) FALSE, call_data);
	}
	else if (fname_mode == FNAME_CREDITS &&
		 credits_mode == CREDITS_TRACK) {
		dbprog_fullname(widgets.credits.fullname_btn,
				(XtPointer) FALSE, call_data);
	}
}


/*
 * dbprog_extt_next
 *	Track details and credits window next track button callback.
 */
/*ARGSUSED*/
void
dbprog_extt_next(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	int		n;

	/* Check full name data and abort if needed */
	if (!dbprog_fullname_ck(s)) {
		cd_beep();
		return;
	}

	/* Put keyboard focus on the OK button */
	if (XtIsManaged(widgets.dbextt.form))
		XmProcessTraversal(widgets.dbextt.ok_btn, XmTRAVERSE_CURRENT);
	if (XtIsManaged(widgets.credits.form))
		XmProcessTraversal(widgets.credits.ok_btn, XmTRAVERSE_CURRENT);

	if (extt_pos < 0) {
		cd_beep();
		return;
	}

	n = extt_pos + 2;
	if (n > (int) s->tot_trks) {
		cd_beep();
		return;
	}

	/* Select the next track */
	if (sel_pos > 0) {
		XmListDeselectPos(widgets.dbprog.trk_list, sel_pos);
		sel_pos = -1;
	}
	XmListSelectPos(widgets.dbprog.trk_list, n, True);

	/* Scroll track list if necessary */
	dbprog_list_autoscroll(
		widgets.dbprog.trk_list,
		(int) s->tot_trks,
		n
	);

	/* Update fullname window if necessary */
	if (fname_mode == FNAME_TRACK) {
		dbprog_fullname(widgets.dbextt.fullname_btn,
				(XtPointer) FALSE, call_data);
	}
	else if (fname_mode == FNAME_CREDITS &&
		 credits_mode == CREDITS_TRACK) {
		dbprog_fullname(widgets.credits.fullname_btn,
				(XtPointer) FALSE, call_data);
	}
}


/*
 * dbprog_extt_autotrk
 *	Track details window Auto-track button callback.
 */
/*ARGSUSED*/
void
dbprog_extt_autotrk(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;
	curstat_t			*s =
		(curstat_t *)(void *) client_data;

	if (p->reason != XmCR_VALUE_CHANGED)
		return;

	DBGPRN(DBG_UI)(errfp, "\n* AUTO-TRACK: %s\n", p->set ? "On" : "Off");

	auto_trk = (bool_t) p->set;

	/* The auto-track buttons in the trackk details and credits
	 * windows are "ganged" together.
	 */
	if (w == widgets.dbextt.autotrk_btn) {
		XmToggleButtonSetState(
			widgets.credits.autotrk_btn, (Boolean) p->set, False
		);
	}
	else if (w == widgets.credits.autotrk_btn) {
		XmToggleButtonSetState(
			widgets.dbextt.autotrk_btn, (Boolean) p->set, False
		);
	}

	dbprog_extt_autotrk_upd(s, di_curtrk_pos(s) + 1);
}


/*
 * dbprog_extt_ok
 *	Track details window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_extt_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	cdinfo_track_t	*t;

	if (fname_mode == FNAME_TRACK) {
		/* Pop down the full name window */
		dbprog_fullname_ok(
			widgets.fullname.ok_btn,
			client_data, call_data
		);
	}

	if (credits_mode != CREDITS_TRACK)
		extt_pos = -1;

	/* Pop down the Track details window */
	XtUnmapWidget(XtParent(widgets.dbextt.form));
	XtUnmanageChild(widgets.dbextt.form);

	/* Update button labels */
	t = (sel_pos > 0) ? &dbp->track[sel_pos - 1] : NULL;
	dbprog_extt_lblupd(t);
	dbprog_tcred_lblupd(t);
}


/*
 * dbprog_credits_popup
 *	Credits button callback.
 */
/*ARGSUSED*/
void
dbprog_credits_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	static bool_t	first = TRUE;

	if (w == widgets.dbprog.dcredits_btn) {
		if (credits_mode == CREDITS_DISC) {
			/* Already popped up - pop it down */
			dbprog_credits_ok(w, client_data, call_data);
			return;
		}

		credits_mode = CREDITS_DISC;
		dbprog_creditupd(s, -1);

		if (XtIsManaged(widgets.credits.autotrk_btn))
			XtUnmanageChild(widgets.credits.autotrk_btn);
		if (XtIsManaged(widgets.credits.prev_btn))
			XtUnmanageChild(widgets.credits.prev_btn);
		if (XtIsManaged(widgets.credits.next_btn))
			XtUnmanageChild(widgets.credits.next_btn);
	}
	else if (w == widgets.dbprog.tcredits_btn) {
		if (credits_mode == CREDITS_TRACK) {
			/* Already popped up - pop it down */
			dbprog_credits_ok(w, client_data, call_data);
			return;
		}

		if (sel_pos > 0)
			extt_pos = sel_pos - 1;
		else if (extt_pos < 0)
			return;

		credits_mode = CREDITS_TRACK;
		dbprog_creditupd(s, extt_pos);

		if (!XtIsManaged(widgets.credits.autotrk_btn))
			XtManageChild(widgets.credits.autotrk_btn);
		if (!XtIsManaged(widgets.credits.prev_btn))
			XtManageChild(widgets.credits.prev_btn);
		if (!XtIsManaged(widgets.credits.next_btn))
			XtManageChild(widgets.credits.next_btn);
	}
	else if (w == widgets.segments.credits_btn) {
		if (credits_mode == CREDITS_SEG) {
			/* Already popped up - pop it down */
			dbprog_credits_ok(w, client_data, call_data);
			return;
		}

		credits_mode = CREDITS_SEG;
		dbprog_creditupd(s, -1);

		if (XtIsManaged(widgets.credits.autotrk_btn))
			XtUnmanageChild(widgets.credits.autotrk_btn);
		if (XtIsManaged(widgets.credits.prev_btn))
			XtUnmanageChild(widgets.credits.prev_btn);
		if (XtIsManaged(widgets.credits.next_btn))
			XtUnmanageChild(widgets.credits.next_btn);
	}
	else
		return;

	/* Initialize role selectors to "None -> None" */
	XtVaSetValues(widgets.credits.prirole_opt,
		XmNmenuHistory,
		widgets.credits.prirole_none_btn,
		NULL
	);
	XtVaSetValues(widgets.credits.subrole_opt,
		XmNmenuHistory,
		widgets.credits.subrole_none_btn,
		NULL
	);

	/* Pop up credits window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	XtManageChild(widgets.credits.form);
	if (first) {
		first = FALSE;
		/* Set up window position */
		cd_dialog_setpos(XtParent(widgets.credits.form));
	}
	XtMapWidget(XtParent(widgets.credits.form));

	/* Put focus on OK button */
	XmProcessTraversal(widgets.credits.ok_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_credits_popdown
 *	Pop down the credits window and do some cleanup.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
dbprog_credits_popdown(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (fname_mode == FNAME_CREDITS) {
		/* Pop down the full name window
		 * Currently dbprog_fullname_ok doesn't use the call_data
		 * arg, so we pass NULL.  If that changes this needs to
		 * change accordingly.
		 */
		dbprog_fullname_ok(
			widgets.fullname.ok_btn, NULL, client_data
		);
	}

	if (!XtIsManaged(widgets.dbextt.form))
		extt_pos = -1;

	/* Pop down credits window */
	XtUnmapWidget(XtParent(widgets.credits.form));
	XtUnmanageChild(widgets.credits.form);

	if (cred_pos > 0) {
		XmListDeselectPos(widgets.credits.cred_list, cred_pos);
		cred_pos = -1;
	}

	credits_mode = CREDITS_NONE;
}


/*
 * dbprog_credits_select
 *	Credits list selector callback
 */
/*ARGSUSED*/
void
dbprog_credits_select(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct	*p =
		(XmListCallbackStruct *)(void *) call_data;
	cdinfo_credit_t		*list,
				*cp;
	int			i;

	if (cred_pos == p->item_position) {
		/* Already selected: de-select it */
		XmListDeselectPos(w, cred_pos);
		cred_pos = -1;

		/* Set role selectors to "None" */
		XtVaSetValues(
			widgets.credits.prirole_opt,
			XmNmenuHistory, widgets.credits.prirole_none_btn,
			NULL
		);
		XtCallCallbacks(
			widgets.credits.prirole_none_btn,
			XmNactivateCallback, (XtPointer) NULL
		);
		set_text_string(widgets.credits.name_txt, "", FALSE);
		set_text_string(widgets.credits.notes_txt, "", FALSE);

		XtSetSensitive(widgets.credits.add_btn, False);
		XtSetSensitive(widgets.credits.del_btn, False);
		XtSetSensitive(widgets.credits.mod_btn, False);

		if (fname_mode == FNAME_CREDITS) {
			set_text_string(
				widgets.fullname.dispname_txt, "", FALSE
			);
			set_text_string(
				widgets.fullname.lastname_txt, "", FALSE
			);
			set_text_string(
				widgets.fullname.firstname_txt, "", FALSE
			);
			set_text_string(
				widgets.fullname.the_txt, "", FALSE
			);
			fname_changed = FALSE;
		}
	}
	else {
		switch (credits_mode) {
		case CREDITS_DISC:
			list = dbp->disc.credit_list;
			break;
		case CREDITS_TRACK:
			if (extt_pos < 0)
				return;
			list = dbp->track[extt_pos].credit_list;
			break;
		case CREDITS_SEG:
			list = w_seg.credit_list;
			break;
		default:
			return;
		}

		cred_pos = p->item_position;

		/* Look for matching credits list entry */
		i = 0;
		for (cp = list; cp != NULL; cp = cp->next) {
			if (++i == p->item_position)
				break;
		}

		if (cp == NULL || i != p->item_position) {
			/* Error: this shouldn't happen */
			return;
		}

		/* Set role selectors - this causes callbacks to be called,
		 * and thus the w_cred structure will be appropriately
		 * filled.
		 */

		if (cp->crinfo.role != NULL) {
		    if (cp->crinfo.role->parent != NULL &&
			cp->crinfo.role->parent->aux != NULL) {
			XtVaSetValues(
				widgets.credits.prirole_opt,
				XmNmenuHistory, cp->crinfo.role->parent->aux,
				NULL
			);
			XtCallCallbacks(
				cp->crinfo.role->parent->aux,
				XmNactivateCallback,
				(XtPointer) cp->crinfo.role
			);
		    }
		    if (cp->crinfo.role->aux != NULL) {
			XtVaSetValues(
				widgets.credits.subrole_opt,
				XmNmenuHistory, cp->crinfo.role->aux,
				NULL
			);
			XtCallCallbacks(
				cp->crinfo.role->aux,
				XmNactivateCallback,
				(XtPointer) cp->crinfo.role
			);
		    }
		}

		/* Explicitly copy fullname structure data */
		if (!util_newstr(&w_cred.crinfo.fullname.dispname,
				 cp->crinfo.fullname.dispname)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		if (!util_newstr(&w_cred.crinfo.fullname.lastname,
				 cp->crinfo.fullname.lastname)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		if (!util_newstr(&w_cred.crinfo.fullname.firstname,
				 cp->crinfo.fullname.firstname)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		if (!util_newstr(&w_cred.crinfo.fullname.the,
				 cp->crinfo.fullname.the)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		/* Set text fields - this causes callbacks to be called,
		 * and thus the w_cred structure will be appropriately
		 * filled.
		 */

		if (cp->crinfo.name != NULL)
			set_text_string(widgets.credits.name_txt,
					cp->crinfo.name, TRUE);
		else
			set_text_string(widgets.credits.name_txt, "", FALSE);

		if (cp->notes != NULL)
			set_text_string(	
				widgets.credits.notes_txt, cp->notes, TRUE
			);
		else
			set_text_string(widgets.credits.notes_txt, "", FALSE);

		/* Set button sensitivities */
		XtSetSensitive(widgets.credits.add_btn, False);
		XtSetSensitive(widgets.credits.mod_btn, False);
		XtSetSensitive(widgets.credits.del_btn, True);
	}

	if (fname_mode == FNAME_CREDITS) {
		/* Update the full name window */
		dbprog_fullname(
			widgets.credits.fullname_btn,
			(XtPointer) FALSE, call_data
		);
	}
}


/*
 * dbprog_credits_ok
 *	Credits window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_credits_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	if ((w == widgets.credits.ok_btn ||
	     w == widgets.dbprog.dcredits_btn ||
	     w == widgets.dbprog.tcredits_btn ||
	     w == widgets.segments.credits_btn) &&
	    (XtIsSensitive(widgets.credits.add_btn) ||
	     XtIsSensitive(widgets.credits.mod_btn))) {
		(void) cd_confirm_popup(
			app_data.str_confirm,
			app_data.str_discardchg,
			(XtCallbackProc) dbprog_credits_popdown, client_data,
			(XtCallbackProc) NULL, NULL
		);
		return;
	}

	dbprog_credits_popdown(w, client_data, call_data);
}


/*
 * dbprog_credits_add
 *	Credits window Add button callback.
 */
/*ARGSUSED*/
void
dbprog_credits_add(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_credit_t	**list,
			*p,
			*q;
	int		pos,
			n;

	switch (credits_mode) {
	case CREDITS_DISC:
		list = &dbp->disc.credit_list;
		pos = -1;
		break;
	case CREDITS_TRACK:
		if (extt_pos < 0)
			return;
		list = &dbp->track[extt_pos].credit_list;
		pos = extt_pos;
		break;
	case CREDITS_SEG:
		list = &w_seg.credit_list;
		pos = -1;
		break;
	default:
		return;
	}

	/* Check entered data for correctness */
	if (!dbprog_credit_ck(&w_cred, pos, s) || !dbprog_fullname_ck(s))
		return;

	p = (cdinfo_credit_t *)(void *) MEM_ALLOC(
		"cdinfo_credit_t",
		sizeof(cdinfo_credit_t)
	);
	if (p == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memset(p, 0, sizeof(cdinfo_credit_t));

	/* Copy tmp credit structure content to newly allocated struct */
	q = &w_cred;
	p->crinfo.role = q->crinfo.role;
	p->crinfo.name = q->crinfo.name;
	p->crinfo.fullname.dispname = q->crinfo.fullname.dispname;
	p->crinfo.fullname.lastname = q->crinfo.fullname.lastname;
	p->crinfo.fullname.firstname = q->crinfo.fullname.firstname;
	p->crinfo.fullname.the = q->crinfo.fullname.the;
	p->notes = q->notes;

	/* Clean up tmp credit structure */
	q->crinfo.role = NULL;
	q->crinfo.name = NULL;
	q->crinfo.fullname.dispname = NULL;
	q->crinfo.fullname.lastname = NULL;
	q->crinfo.fullname.firstname = NULL;
	q->crinfo.fullname.the = NULL;
	q->notes = NULL;

	/* Add to incore credits list */
	n = 0;
	if (*list == NULL) {
		/* First entry in list */
		*list = p;
	}
	else {
		/* Look for end of credits list and append to it */
		for (n = 1, q = *list; q->next != NULL; q = q->next, n++)
			;
		q->next = p;
		p->prev = q;
	}
	n++;

	dbprog_creditupd(s, pos);
	dbprog_list_autoscroll(widgets.credits.cred_list, n, n);

	if (credits_mode == CREDITS_SEG) {
		if (seg_pos < 0)
			XtSetSensitive(widgets.segments.add_btn, True);
		else
			XtSetSensitive(widgets.segments.mod_btn, True);
	}

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_credits_mod
 *	Credits window Modify button callback.
 */
/*ARGSUSED*/
void
dbprog_credits_mod(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_credit_t	*list,
			*p,
			*q;
	int		pos,
			sav_credpos,
			n;

	if (cred_pos < 0)
		return;	/* Invalid */

	switch (credits_mode) {
	case CREDITS_DISC:
		list = dbp->disc.credit_list;
		pos = -1;
		break;
	case CREDITS_TRACK:
		if (extt_pos < 0)
			return;
		list = dbp->track[extt_pos].credit_list;
		pos = extt_pos;
		break;
	case CREDITS_SEG:
		list = w_seg.credit_list;
		pos = -1;
		break;
	default:
		return;
	}

	/* Check entered data for correctness */
	if (!dbprog_credit_ck(&w_cred, pos, s) || !dbprog_fullname_ck(s))
		return;

	for (n = 0, p = list; p != NULL; p = p->next, n++) {
		if ((n+1) != cred_pos)
			continue;

		/* Update appropriate incore credit entry */
		q = &w_cred;
		if (p->crinfo.name != NULL)
			MEM_FREE(p->crinfo.name);
		if (p->crinfo.fullname.dispname != NULL)
			MEM_FREE(p->crinfo.fullname.dispname);
		if (p->crinfo.fullname.lastname != NULL)
			MEM_FREE(p->crinfo.fullname.lastname);
		if (p->crinfo.fullname.firstname != NULL)
			MEM_FREE(p->crinfo.fullname.firstname);
		if (p->crinfo.fullname.the != NULL)
			MEM_FREE(p->crinfo.fullname.the);
		if (p->notes != NULL)
			MEM_FREE(p->notes);

		p->crinfo.role = q->crinfo.role;
		p->crinfo.name = q->crinfo.name;
		p->crinfo.fullname.dispname = q->crinfo.fullname.dispname;
		p->crinfo.fullname.lastname = q->crinfo.fullname.lastname;
		p->crinfo.fullname.firstname = q->crinfo.fullname.firstname;
		p->crinfo.fullname.the = q->crinfo.fullname.the;
		p->notes = q->notes;

		/* Clean up tmp structure */
		q->crinfo.role = NULL;
		q->crinfo.name = NULL;
		q->crinfo.fullname.dispname = NULL;
		q->crinfo.fullname.lastname = NULL;
		q->crinfo.fullname.firstname = NULL;
		q->crinfo.fullname.the = NULL;
		q->notes = NULL;
	}

	sav_credpos = cred_pos;

	dbprog_creditupd(s, pos);
	dbprog_list_autoscroll(widgets.credits.cred_list, n, sav_credpos);

	if (credits_mode == CREDITS_SEG) {
		if (seg_pos < 0)
			XtSetSensitive(widgets.segments.add_btn, True);
		else
			XtSetSensitive(widgets.segments.mod_btn, True);
	}

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_credits_del
 *	Credits window Delete button callback.
 */
/*ARGSUSED*/
void
dbprog_credits_del(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_credit_t	**list,
			*p;
	int		pos,
			scrollpos,
			i,
			n;

	if (cred_pos < 0)
		return;	/* Invalid */

	switch (credits_mode) {
	case CREDITS_DISC:
		list = &dbp->disc.credit_list;
		pos = -1;
		break;
	case CREDITS_TRACK:
		if (extt_pos < 0)
			return;
		list = &dbp->track[extt_pos].credit_list;
		pos = extt_pos;
		break;
	case CREDITS_SEG:
		list = &w_seg.credit_list;
		pos = -1;
		break;
	default:
		return;
	}

	/* Delete from incore credit list */
	if (cred_pos == 1) {
		p = *list;
		*list = p->next;
		if (p->next != NULL)
			p->next->prev = NULL;
	}
	else {
		i = 0;
		for (p = *list; p != NULL; p = p->next) {
			if (++i < cred_pos)
				continue;

			if (p->next != NULL)
				p->next->prev = p->prev;
			if (p->prev != NULL)
				p->prev->next = p->next;

			break;
		}
	}
	if (p != NULL)
		MEM_FREE(p);

	for (n = 0, p = *list; p != NULL; p = p->next, n++)
		;

	if (n > cred_pos)
		scrollpos = cred_pos;
	else
		scrollpos = n;

	dbprog_creditupd(s, pos);
	dbprog_list_autoscroll(widgets.credits.cred_list, n, scrollpos);

	if (credits_mode == CREDITS_SEG) {
		if (seg_pos < 0)
			XtSetSensitive(widgets.segments.add_btn, True);
		else
			XtSetSensitive(widgets.segments.mod_btn, True);
	}

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_segments_popup
 *	Credits button callback.
 */
/*ARGSUSED*/
void
dbprog_segments_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.segments.form)) {
		/* Already popped up - pop it down */
		dbprog_segments_ok(w, client_data, call_data);
		return;
	}

	/* Pop up segments window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	XtManageChild(widgets.segments.form);

	/* Update segment window */
	dbprog_segmentupd(s);

	if (first) {
		first = FALSE;
		/* Set up window position */
		cd_dialog_setpos(XtParent(widgets.segments.form));
	}
	XtMapWidget(XtParent(widgets.segments.form));

	/* Put focus on OK button */
	XmProcessTraversal(widgets.segments.ok_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_segments_popdown
 *	Pop down the segments window and do some cleanup.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
dbprog_segments_popdown(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (credits_mode == CREDITS_SEG) {
		/* Force a popdown of the credits window */
		dbprog_credits_popdown(
			widgets.credits.ok_btn, client_data, call_data
		);
	}

	/* Pop down segments window */
	XtUnmapWidget(XtParent(widgets.segments.form));
	XtUnmanageChild(widgets.segments.form);

	if (seg_pos > 0) {
		XmListDeselectPos(widgets.segments.seg_list, seg_pos);
		seg_pos = -1;
	}
}


/*
 * dbprog_segments_select
 *	Credits list selector callback
 */
/*ARGSUSED*/
void
dbprog_segments_select(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct	*p =
		(XmListCallbackStruct *)(void *) call_data;
	curstat_t		*s = (curstat_t *)(void *) client_data;
	cdinfo_segment_t	*sp;
	cdinfo_credit_t		*cp,
				*cp2,
				*cp3;
	char			*buf;
	int			i,
				st,
				sf,
				et,
				ef;
	sword32_t		eot;

	if (seg_pos == p->item_position) {
		/* Already selected: de-select it */
		XmListDeselectPos(w, seg_pos);
		seg_pos = -1;

		set_text_string(widgets.segments.name_txt, "", FALSE);
		set_text_string(widgets.segments.starttrk_txt, "", FALSE);
		set_text_string(widgets.segments.startfrm_txt, "", FALSE);
		set_text_string(widgets.segments.endtrk_txt, "", FALSE);
		set_text_string(widgets.segments.endfrm_txt, "", FALSE);
		set_text_string(widgets.segments.notes_txt, "", FALSE);

		for (cp = cp2 = w_seg.credit_list; cp != NULL; cp = cp2) {
			cp2 = cp->next;

			if (cp->crinfo.name != NULL)
				MEM_FREE(cp->crinfo.name);
			if (cp->crinfo.fullname.dispname != NULL)
				MEM_FREE(cp->crinfo.fullname.dispname);
			if (cp->crinfo.fullname.lastname != NULL)
				MEM_FREE(cp->crinfo.fullname.lastname);
			if (cp->crinfo.fullname.firstname != NULL)
				MEM_FREE(cp->crinfo.fullname.firstname);
			if (cp->crinfo.fullname.the != NULL)
				MEM_FREE(cp->crinfo.fullname.the);
			if (cp->notes != NULL)
				MEM_FREE(cp->notes);
			MEM_FREE(cp);
		}
		w_seg.credit_list = NULL;

		XtSetSensitive(widgets.segments.add_btn, False);
		XtSetSensitive(widgets.segments.mod_btn, False);
		XtSetSensitive(widgets.segments.del_btn, False);

		s->segplay = SEGP_NONE;
		dpy_progmode(s, FALSE);
	}
	else {
		seg_pos = p->item_position;

		/* Look for matching segments list entry */
		i = 0;
		for (sp = dbp->disc.segment_list; sp != NULL; sp = sp->next) {
			if (++i == p->item_position)
				break;
		}

		if (sp == NULL || i != p->item_position) {
			/* Error: this shouldn't happen */
			return;
		}

		if (w_seg.name != NULL) {
			MEM_FREE(w_seg.name);
			w_seg.name = NULL;
		}
		if (w_seg.start_track != NULL) {
			MEM_FREE(w_seg.start_track);
			w_seg.start_track = NULL;
		}
		if (w_seg.start_frame != NULL) {
			MEM_FREE(w_seg.start_frame);
			w_seg.start_frame = NULL;
		}
		if (w_seg.end_track != NULL) {
			MEM_FREE(w_seg.end_track);
			w_seg.end_track = NULL;
		}
		if (w_seg.end_frame != NULL) {
			MEM_FREE(w_seg.end_frame);
			w_seg.end_frame = NULL;
		}
		if (w_seg.notes != NULL) {
			MEM_FREE(w_seg.notes);
			w_seg.notes = NULL;
		}
		for (cp = cp2 = w_seg.credit_list; cp != NULL; cp = cp2) {
			cp2 = cp->next;

			if (cp->crinfo.name != NULL)
				MEM_FREE(cp->crinfo.name);
			if (cp->crinfo.fullname.dispname != NULL)
				MEM_FREE(cp->crinfo.fullname.dispname);
			if (cp->crinfo.fullname.lastname != NULL)
				MEM_FREE(cp->crinfo.fullname.lastname);
			if (cp->crinfo.fullname.firstname != NULL)
				MEM_FREE(cp->crinfo.fullname.firstname);
			if (cp->crinfo.fullname.the != NULL)
				MEM_FREE(cp->crinfo.fullname.the);
			if (cp->notes != NULL)
				MEM_FREE(cp->notes);
			MEM_FREE(cp);
		}
		w_seg.credit_list = NULL;

		/* Set text fields - this causes callbacks to be called,
		 * and thus the w_seg structure will be appropriately
		 * filled.
		 */

		if (sp->name != NULL)
			set_text_string(widgets.segments.name_txt,
					sp->name, TRUE);
		else
			set_text_string(widgets.segments.name_txt, "", FALSE);

		if (sp->start_track != NULL)
			set_text_string(widgets.segments.starttrk_txt,
					sp->start_track, FALSE);
		else
			set_text_string(widgets.segments.starttrk_txt,
					"", FALSE);

		if (sp->start_frame != NULL)
			set_text_string(widgets.segments.startfrm_txt,
					sp->start_frame, FALSE);
		else
			set_text_string(widgets.segments.startfrm_txt,
					"", FALSE);

		if (sp->end_track != NULL)
			set_text_string(widgets.segments.endtrk_txt,
					sp->end_track, FALSE);
		else
			set_text_string(widgets.segments.endtrk_txt,
					"", FALSE);

		if (sp->end_frame != NULL)
			set_text_string(widgets.segments.endfrm_txt,
					sp->end_frame, FALSE);
		else
			set_text_string(widgets.segments.endfrm_txt,
					"", FALSE);

		if (sp->notes != NULL)
			set_text_string(widgets.segments.notes_txt,
					sp->notes, TRUE);
		else
			set_text_string(widgets.segments.notes_txt,
					"", FALSE);

		/* Copy credit list to the working seg structure */
		cp3 = NULL;
		for (cp = sp->credit_list; cp != NULL; cp = cp->next) {
			cp2 = (cdinfo_credit_t *)(void *) MEM_ALLOC(
				"cdinfo_credit_t",
				sizeof(cdinfo_credit_t)
			);
			if (cp2 == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			(void) memset(cp2, 0, sizeof(cdinfo_credit_t));

			cp2->crinfo.role = cp->crinfo.role;

			if (!util_newstr(&cp2->crinfo.name, cp->crinfo.name)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			if (!util_newstr(&cp2->crinfo.fullname.dispname,
					 cp->crinfo.fullname.dispname)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			if (!util_newstr(&cp2->crinfo.fullname.lastname,
					 cp->crinfo.fullname.lastname)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			if (!util_newstr(&cp2->crinfo.fullname.firstname,
					 cp->crinfo.fullname.firstname)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			if (!util_newstr(&cp2->crinfo.fullname.the,
					 cp->crinfo.fullname.the)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}
			if (!util_newstr(&cp2->notes, cp->notes)) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			if (w_seg.credit_list == NULL) {
				w_seg.credit_list = cp2;
				cp2->prev = NULL;
			}
			else {
				cp3->next = cp2;
				cp2->prev = cp3;
			}
			cp2->next = NULL;
			cp3 = cp2;
		}

		/* Set button sensitivities */
		XtSetSensitive(widgets.segments.add_btn, False);
		XtSetSensitive(widgets.segments.mod_btn, False);
		XtSetSensitive(widgets.segments.del_btn, True);

		/*
		 * Set a->b mode based on this segment
		 */

		buf = get_text_string(widgets.segments.starttrk_txt, FALSE);
		if (buf != NULL) {
			st = atoi(buf);
			MEM_FREE(buf);
		}
		else
			st = 0;	/* shrug */

		buf = get_text_string(widgets.segments.startfrm_txt, FALSE);
		if (buf != NULL) {
			sf = atoi(buf);
			MEM_FREE(buf);
		}
		else
			sf = 0;	/* shrug */

		buf = get_text_string(widgets.segments.endtrk_txt, FALSE);
		if (buf != NULL) {
			et = atoi(buf);
			MEM_FREE(buf);
		}
		else
			et = 0;	/* shrug */

		buf = get_text_string(widgets.segments.endfrm_txt, FALSE);
		if (buf != NULL) {
			ef = atoi(buf);
			MEM_FREE(buf);
		}
		else
			ef = 0;	/* shrug */

		for (i = 0; i < (int) s->tot_trks; i++) {
			if (s->trkinfo[i].trkno == st)
				break;
		}
		if (i == (int) s->tot_trks)
			i = 0;	/* This shouldn't happen */

		s->bp_startpos_tot.addr = s->trkinfo[i].addr + sf;
		util_blktomsf(
			s->bp_startpos_tot.addr,
			&s->bp_startpos_tot.min,
			&s->bp_startpos_tot.sec,
			&s->bp_startpos_tot.frame,
			MSF_OFFSET
		);

		s->bp_startpos_trk.addr = sf;
		util_blktomsf(
			s->bp_startpos_trk.addr,
			&s->bp_startpos_trk.min,
			&s->bp_startpos_trk.sec,
			&s->bp_startpos_trk.frame,
			MSF_OFFSET
		);

		for (i = 0; i < (int) s->tot_trks; i++) {
			if (s->trkinfo[i].trkno == et)
				break;
		}
		if (i == (int) s->tot_trks) {
			i--;	/* This shouldn't happen */

			eot = s->trkinfo[i+1].addr;

			/* "Enhanced CD" / "CD Extra" hack */
			if (eot > s->discpos_tot.addr)
				eot = s->discpos_tot.addr;

			ef = eot - s->trkinfo[i].addr - 25;
		}

		s->bp_endpos_tot.addr = s->trkinfo[i].addr + ef;
		util_blktomsf(
			s->bp_endpos_tot.addr,
			&s->bp_endpos_tot.min,
			&s->bp_endpos_tot.sec,
			&s->bp_endpos_tot.frame,
			MSF_OFFSET
		);

		s->bp_endpos_trk.addr = ef;
		util_blktomsf(
			s->bp_endpos_trk.addr,
			&s->bp_endpos_trk.min,
			&s->bp_endpos_trk.sec,
			&s->bp_endpos_trk.frame,
			MSF_OFFSET
		);

		/* Clear any play programs */
		set_text_string(widgets.dbprog.pgmseq_txt, "", FALSE);
		XtSetSensitive(widgets.dbprog.addpgm_btn, False);
		XtSetSensitive(widgets.dbprog.clrpgm_btn, False);
		XtSetSensitive(widgets.dbprog.savepgm_btn, False);
		s->onetrk_prog = FALSE;
		dbprog_progclear(s);

		/* Disable shuffle mode */
		if (s->shuffle) {
			di_shuffle(s, FALSE);
			set_shuffle_btn(FALSE);
		}

		/* Stop playback */
		if (s->mode != MOD_STOP)
			di_stop(s, TRUE);

		/* Set a->b mode */
		s->segplay = SEGP_AB;
		dpy_progmode(s, FALSE);
	}

	/* Update credits window */
	dbprog_creditupd(s, -1);
}


/*
 * dbprog_segments_ok
 *	Credits window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_segments_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	if ((w == widgets.segments.ok_btn ||
	     w == widgets.dbprog.segments_btn) &&
	    (XtIsSensitive(widgets.segments.add_btn) ||
	     XtIsSensitive(widgets.segments.mod_btn))) {
		(void) cd_confirm_popup(
			app_data.str_confirm,
			app_data.str_discardchg,
			(XtCallbackProc) dbprog_segments_popdown, client_data,
			(XtCallbackProc) NULL, NULL
		);
		return;
	}

	dbprog_segments_popdown(w, client_data, call_data);
}


/*
 * dbprog_segments_add
 *	Credits window Add button callback.
 */
/*ARGSUSED*/
void
dbprog_segments_add(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t		*s = (curstat_t *)(void *) client_data;
	cdinfo_segment_t	*p,
				*q;
	int			n;

	if (!dbprog_segment_ck(&w_seg, s))
		return;

	p = (cdinfo_segment_t *)(void *) MEM_ALLOC(
		"cdinfo_segment_t",
		sizeof(cdinfo_segment_t)
	);
	if (p == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memset(p, 0, sizeof(cdinfo_segment_t));

	/* Copy tmp segment structure content to newly allocated struct */
	q = &w_seg;
	p->name = q->name;
	p->start_track = q->start_track;
	p->start_frame = q->start_frame;
	p->end_track = q->end_track;
	p->end_frame = q->end_frame;
	p->notes = q->notes;
	p->credit_list = q->credit_list;

	/* Clean up work structure */
	q->name = NULL;
	q->start_track = NULL;
	q->start_frame = NULL;
	q->end_track = NULL;
	q->end_frame = NULL;
	q->notes = NULL;
	q->credit_list = NULL;

	/* Add to incore segments list */
	n = 0;
	if (dbp->disc.segment_list == NULL) {
		/* First entry in list */
		dbp->disc.segment_list = p;
	}
	else {
		/* Look for end of segments list and append to it */
		for (n = 1, q = dbp->disc.segment_list; q->next != NULL;
		     q = q->next, n++)
			;
		q->next = p;
		p->prev = q;
	}
	n++;

	dbprog_segmentupd(s);
	dbprog_list_autoscroll(widgets.segments.seg_list, n, n);

	if (credits_mode == CREDITS_SEG)
		dbprog_creditupd(s, -1);

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_segments_mod
 *	Credits window Modify button callback.
 */
/*ARGSUSED*/
void
dbprog_segments_mod(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t		*s = (curstat_t *)(void *) client_data;
	cdinfo_segment_t	*p,
				*q;
	cdinfo_credit_t		*cp,
				*cp2;
	int			n,
				sav_segpos;

	if (seg_pos < 0)
		return;	/* Invalid */

	if (!dbprog_segment_ck(&w_seg, s))
		return;

	for (n = 0, p = dbp->disc.segment_list; p != NULL; p = p->next, n++) {
		if ((n+1) != seg_pos)
			continue;

		/* Update appropriate incore segment entry */
		q = &w_seg;
		if (p->name != NULL)
			MEM_FREE(p->name);
		if (p->start_track != NULL)
			MEM_FREE(p->start_track);
		if (p->start_frame != NULL)
			MEM_FREE(p->start_frame);
		if (p->end_track != NULL)
			MEM_FREE(p->end_track);
		if (p->end_frame != NULL)
			MEM_FREE(p->end_frame);
		if (p->notes != NULL)
			MEM_FREE(p->notes);
		for (cp = cp2 = p->credit_list; cp != NULL; cp = cp2) {
			cp2 = cp->next;

			if (cp->crinfo.name != NULL)
				MEM_FREE(cp->crinfo.name);
			if (cp->crinfo.fullname.dispname != NULL)
				MEM_FREE(cp->crinfo.fullname.dispname);
			if (cp->crinfo.fullname.lastname != NULL)
				MEM_FREE(cp->crinfo.fullname.lastname);
			if (cp->crinfo.fullname.firstname != NULL)
				MEM_FREE(cp->crinfo.fullname.firstname);
			if (cp->crinfo.fullname.the != NULL)
				MEM_FREE(cp->crinfo.fullname.the);
			if (cp->notes != NULL)
				MEM_FREE(cp->notes);
			MEM_FREE(cp);
		}
		p->credit_list = NULL;

		p->name = q->name;
		p->start_track = q->start_track;
		p->start_frame = q->start_frame;
		p->end_track = q->end_track;
		p->end_frame = q->end_frame;
		p->notes = q->notes;
		p->credit_list = q->credit_list;

		/* Clean up work structure */
		q->name = NULL;
		q->start_track = NULL;
		q->start_frame = NULL;
		q->end_track = NULL;
		q->end_frame = NULL;
		q->notes = NULL;
		q->credit_list = NULL;
	}

	sav_segpos = seg_pos;

	dbprog_segmentupd(s);
	dbprog_list_autoscroll(widgets.segments.seg_list, n, sav_segpos);

	if (credits_mode == CREDITS_SEG)
		dbprog_creditupd(s, -1);

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_segments_del
 *	Credits window Delete button callback.
 */
/*ARGSUSED*/
void
dbprog_segments_del(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t		*s = (curstat_t *)(void *) client_data;
	cdinfo_segment_t	*p;
	cdinfo_credit_t		*cp,
				*cp2;
	int			i,
				n,
				scrollpos;

	if (seg_pos < 0)
		return;	/* Invalid */

	/* Delete from incore segment list */
	if (seg_pos == 1) {
		p = dbp->disc.segment_list;
		dbp->disc.segment_list = p->next;
		if (p->next != NULL)
			p->next->prev = NULL;
	}
	else {
		i = 0;
		for (p = dbp->disc.segment_list; p != NULL; p = p->next) {
			if (++i < seg_pos)
				continue;

			if (p->next != NULL)
				p->next->prev = p->prev;
			if (p->prev != NULL)
				p->prev->next = p->next;

			break;
		}
	}

	if (p != NULL) {
		if (p->name != NULL)
			MEM_FREE(p->name);
		if (p->start_track != NULL)
			MEM_FREE(p->start_track);
		if (p->start_frame != NULL)
			MEM_FREE(p->start_frame);
		if (p->end_track != NULL)
			MEM_FREE(p->end_track);
		if (p->end_frame != NULL)
			MEM_FREE(p->end_frame);
		if (p->notes != NULL)
			MEM_FREE(p->notes);
		for (cp = cp2 = p->credit_list; cp != NULL; cp = cp2) {
			cp2 = cp->next;

			if (cp->crinfo.name != NULL)
				MEM_FREE(cp->crinfo.name);
			if (cp->crinfo.fullname.dispname != NULL)
				MEM_FREE(cp->crinfo.fullname.dispname);
			if (cp->crinfo.fullname.lastname != NULL)
				MEM_FREE(cp->crinfo.fullname.lastname);
			if (cp->crinfo.fullname.firstname != NULL)
				MEM_FREE(cp->crinfo.fullname.firstname);
			if (cp->crinfo.fullname.the != NULL)
				MEM_FREE(cp->crinfo.fullname.the);
			if (cp->notes != NULL)
				MEM_FREE(cp->notes);
			MEM_FREE(cp);
		}

		MEM_FREE(p);
	}

	for (n = 0, p = dbp->disc.segment_list; p != NULL; p = p->next, n++)
		;

	if (n > seg_pos)
		scrollpos = seg_pos;
	else
		scrollpos = n;

	dbprog_segmentupd(s);
	dbprog_list_autoscroll(widgets.segments.seg_list, n, scrollpos);

	if (credits_mode == CREDITS_SEG)
		dbprog_creditupd(s, -1);

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_regionsel_popup
 *	Region change button callback.
 */
/*ARGSUSED*/
void
dbprog_regionsel_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		i;
	cdinfo_region_t	*rp;
	char		*region;

	if (w == widgets.dbextd.region_chg_btn) {
		reg_mode = REGION_DISC;
		region = dbp->disc.region;
	}
	else if (w == widgets.userreg.region_chg_btn) {
		reg_mode = REGION_USERREG;
		region = dbp->userreg.region;
	}
	else
		return;

	/* Pre-select the current region if any */
	if (region != NULL) {
		rp = dbp->regionlist;
		for (i = 1; rp != NULL; i++, rp = rp->next) {
			if (strcmp(region, rp->id) == 0) {
				/* Pre-select the current region */
				XmListSelectPos(widgets.regionsel.region_list,
						i, False);
				dbprog_list_autoscroll(
					widgets.regionsel.region_list,
					reg_cnt, i
				);
				break;
			}
		}
	}

	/* Pop up the region selector window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	if (!XtIsManaged(widgets.regionsel.form)) {
		XtManageChild(widgets.regionsel.form);

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.regionsel.form));

		XtMapWidget(XtParent(widgets.regionsel.form));

		/* Put focus on the region list */
		XmProcessTraversal(
			widgets.regionsel.region_list, XmTRAVERSE_CURRENT
		);
	}
}


/*
 * dbprog_regionsel_select
 *	Region selection list callback.
 */
/*ARGSUSED*/
void
dbprog_regionsel_select(Widget w, XtPointer client_data, XtPointer call_data)
{
	int			i;
	XmListCallbackStruct	*p =
				(XmListCallbackStruct *)(void *) call_data;
	cdinfo_region_t		*rp;
	char			**fptr;

	if (p->reason != XmCR_BROWSE_SELECT)
		return;

	if (reg_mode == REGION_DISC)
		fptr = &dbp->disc.region;
	else if (reg_mode == REGION_USERREG)
		fptr = &dbp->userreg.region;
	else
		return;

	/* Look for matching region list entry */
	i = 0;
	for (rp = dbp->regionlist; rp != NULL; rp = rp->next) {
		if (++i == p->item_position)
			break;
	}

	if (rp == NULL || rp->id == NULL || i != p->item_position) {
		/* Error: this shouldn't happen */
		return;
	}

	if (*fptr != NULL && strcmp(rp->id, *fptr) == 0) {
		/* Not changed - de-select the entry */
		XmListDeselectPos(w, p->item_position);

		MEM_FREE(*fptr);
		*fptr = NULL;

		if (reg_mode == REGION_DISC)
			set_text_string(widgets.dbextd.region_txt, "", FALSE);
		else if (reg_mode == REGION_USERREG)
			set_text_string(widgets.userreg.region_txt, "", FALSE);
		return;
	}

	if (reg_mode == REGION_DISC) {
		dbp->flags |= CDINFO_CHANGED;
		if (!app_data.cdinfo_inetoffln)
			XtSetSensitive(widgets.dbprog.submit_btn, True);
	}

	if (!util_newstr(fptr, rp->id)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	if (reg_mode == REGION_DISC)
		set_text_string(widgets.dbextd.region_txt, rp->name, TRUE);
	else if (reg_mode == REGION_USERREG)
		set_text_string(widgets.userreg.region_txt, rp->name, TRUE);
}


/*
 * dbprog_regionsel_ok
 *	Region selection window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_regionsel_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	int			i;
	XmListCallbackStruct	*p =
				(XmListCallbackStruct *)(void *) call_data;
	cdinfo_region_t		*rp;

	if (w == widgets.regionsel.region_list &&
	    p->reason == XmCR_DEFAULT_ACTION) {
		/* Look for matching region list entry */
		i = 0;
		for (rp = dbp->regionlist; rp != NULL; rp = rp->next) {
			if (++i == p->item_position)
				break;
		}

		if (rp == NULL || rp->id == NULL || i != p->item_position) {
			/* Error: this shouldn't happen */
			return;
		}

		if (reg_mode == REGION_DISC)
			set_text_string(widgets.dbextd.region_txt,
					rp->name, TRUE);
		else if (reg_mode == REGION_USERREG)
			set_text_string(widgets.userreg.region_txt,
					rp->name, TRUE);
	}

	/* Pop down the region selector popup dialog */
	XtUnmapWidget(XtParent(widgets.regionsel.form));
	XtUnmanageChild(widgets.regionsel.form);

	XmListDeselectAllItems(widgets.regionsel.region_list);

	/* Put the input focus on the next widget */
	if (reg_mode == REGION_DISC) {
		XmProcessTraversal(
			widgets.dbextd.lang_chg_btn,
			XmTRAVERSE_CURRENT
		);
	}
	else if (reg_mode == REGION_USERREG) {
		XmProcessTraversal(
			widgets.userreg.postal_txt,
			XmTRAVERSE_CURRENT
		);
	}

	reg_mode = REGION_NONE;
}


/*
 * dbprog_langsel_popup
 *	Language change button callback.
 */
/*ARGSUSED*/
void
dbprog_langsel_popup(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		i;
	cdinfo_lang_t	*lp;

	/* Pre-select the current language if any */
	if (dbp->disc.lang != NULL) {
		lp = dbp->langlist;
		for (i = 1; lp != NULL; i++, lp = lp->next) {
			if (strcmp(dbp->disc.lang, lp->id) == 0) {
				/* Pre-select the current language */
				XmListSelectPos(widgets.langsel.lang_list,
						i, False);
				dbprog_list_autoscroll(
					widgets.langsel.lang_list,
					lang_cnt, i
				);
				break;
			}
		}
	}

	/* Pop up the language selector window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	if (!XtIsManaged(widgets.langsel.form)) {
		XtManageChild(widgets.langsel.form);

		/* Set up dialog box position */
		cd_dialog_setpos(XtParent(widgets.langsel.form));

		XtMapWidget(XtParent(widgets.langsel.form));

		/* Put focus on the language list */
		XmProcessTraversal(
			widgets.langsel.lang_list, XmTRAVERSE_CURRENT
		);
	}
}


/*
 * dbprog_langsel_select
 *	Language selection list callback.
 */
/*ARGSUSED*/
void
dbprog_langsel_select(Widget w, XtPointer client_data, XtPointer call_data)
{
	int			i;
	XmListCallbackStruct	*p =
				(XmListCallbackStruct *)(void *) call_data;
	cdinfo_lang_t		*lp;

	if (p->reason != XmCR_BROWSE_SELECT)
		return;

	/* Look for matching region list entry */
	i = 0;
	for (lp = dbp->langlist; lp != NULL; lp = lp->next) {
		if (++i == p->item_position)
			break;
	}

	if (lp == NULL || lp->id == NULL || i != p->item_position) {
		/* Error: this shouldn't happen */
		return;
	}

	if (dbp->disc.lang != NULL && strcmp(lp->id, dbp->disc.lang) == 0) {
		/* Not changed - de-select the entry */
		XmListDeselectPos(w, p->item_position);

		MEM_FREE(dbp->disc.lang);
		dbp->disc.lang = NULL;

		set_text_string(widgets.dbextd.lang_txt, "", FALSE);
		return;
	}

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);

	if (!util_newstr(&dbp->disc.lang, lp->id)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}

	set_text_string(widgets.dbextd.lang_txt, lp->name, TRUE);
}


/*
 * dbprog_langsel_ok
 *	Language selection window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_langsel_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	int			i;
	XmListCallbackStruct	*p =
				(XmListCallbackStruct *)(void *) call_data;
	cdinfo_lang_t		*lp;

	if (w == widgets.langsel.lang_list &&
	    p->reason == XmCR_DEFAULT_ACTION) {
		/* Look for matching language list entry */
		i = 0;
		for (lp = dbp->langlist; lp != NULL; lp = lp->next) {
			if (++i == p->item_position)
				break;
		}

		if (lp == NULL || lp->id == NULL || i != p->item_position) {
			/* Error: this shouldn't happen */
			return;
		}

		set_text_string(widgets.dbextd.lang_txt, lp->name, TRUE);
	}

	/* Pop down the language selector popup dialog */
	XtUnmapWidget(XtParent(widgets.langsel.form));
	XtUnmanageChild(widgets.langsel.form);

	XmListDeselectAllItems(widgets.langsel.lang_list);

	/* Put the input focus on the next widget */
	XmProcessTraversal(
		widgets.dbextd.dnum_txt,
		XmTRAVERSE_CURRENT
	);
}


/*
 * dbprog_matchsel_select
 *	Match selection list callback.
 */
/*ARGSUSED*/
void
dbprog_matchsel_select(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct	*p =
				(XmListCallbackStruct *)(void *) call_data;

	if (p->reason != XmCR_BROWSE_SELECT)
		return;

	if (p->item_position == match_cnt)
		dbp->match_tag = 0;	/* User chose "none of the above" */
	else
		dbp->match_tag = (long) p->item_position;

	if (!XtIsSensitive(widgets.matchsel.ok_btn))
		XtSetSensitive(widgets.matchsel.ok_btn, True);
}


/*
 * dbprog_matchsel_ok
 *	Match selection window OK button callback.
 */
/*ARGSUSED*/
void
dbprog_matchsel_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	cdinfo_ret_t	ret;

	if (XtIsManaged(widgets.matchsel.form)) {
		/* Pop down the match selector popup dialog */
		XtUnmapWidget(XtParent(widgets.matchsel.form));
		XtUnmanageChild(widgets.matchsel.form);

		/* Remove all matchsel list items */
		XmListDeleteAllItems(widgets.matchsel.matchsel_list);
		match_cnt = 0;
	}

	if (dbp->match_tag == 0)
		s->qmode = QMODE_NONE;
	else
		s->qmode = QMODE_WAIT;

	dpy_dbmode(s, FALSE);

	/* Re-query CDDB for the user selected entry */
	if ((ret = cdinfo_load_matchsel(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_load_matchsel: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));

		/* Set qmode flag */
		s->qmode = QMODE_ERR;
	}
	else if ((dbp->flags & CDINFO_MATCH) != 0) {
		s->qmode = QMODE_MATCH;

		/* Got CDDB data, clear the wwwwarp_cleared flag */
		wwwwarp_cleared = FALSE;
	}
	else
		s->qmode = QMODE_NONE;

	if (stopload_active) {
		/* Pop down the stop load dialog */
		stopload_active = FALSE;
		cd_confirm_popdown();
	}

	/* Update widgets */
	dbprog_structupd(s);

	/* Configure the wwwWarp menu */
	wwwwarp_sel_cfg(s);

	XtSetSensitive(widgets.dbprog.reload_btn, True);

	s->program = dbprog_pgm_active();

	/* Update display */
	dpy_progmode(s, FALSE);
	dpy_dtitle(s);
	dpy_ttitle(s);

	/* Update curfile */
	dbprog_curfileupd();

	/* Add to history and changer lists */
	dbprog_hist_new(s);
	dbprog_chgr_new(s);
}


/*
 * dbprog_the
 *	"The" toggle button callback
 */
/*ARGSUSED*/
void
dbprog_the(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*p =
		(XmToggleButtonCallbackStruct *)(void *) call_data;
	Widget	txtw;

	if (w == widgets.fullname.the_btn)
		txtw = widgets.fullname.the_txt;
	else if (w == widgets.dbextd.the_btn)
		txtw = widgets.dbextd.the_txt;
	else if (w == widgets.dbextt.the_btn)
		txtw = widgets.dbextt.the_txt;
	else {
		cd_beep();
		return;
	}

	if (XtIsSensitive(txtw) == p->set)
		/* No change */
		return;

	XtSetSensitive(txtw, p->set);
	set_text_string(txtw, p->set ? app_data.str_the : "", TRUE);

	/* Put keyboard focus on the The text widget */
	XmProcessTraversal(txtw, XmTRAVERSE_CURRENT);

	dbp->flags |= CDINFO_CHANGED;
	if (!app_data.cdinfo_inetoffln)
		XtSetSensitive(widgets.dbprog.submit_btn, True);
}


/*
 * dbprog_auth_retry
 *	Let the user have the option of retrying the proxy-authorization.
 */
/*ARGSUSED*/
void
dbprog_auth_retry(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = curstat_addr();

	/* Clear out previously entered password */
	if (dbp->proxy_passwd != NULL) {
		(void) memset(dbp->proxy_passwd, 0, strlen(dbp->proxy_passwd));
		MEM_FREE(dbp->proxy_passwd);
		dbp->proxy_passwd = NULL;
	}

	if (client_data != NULL) {
		/* Clear out password field */
		set_text_string(widgets.auth.pass_txt, "", FALSE);

		/* Pop up authorization dialog */
		dbprog_auth_popup();
	}
	else {
		/* Auth failed: cannot get CD info */

		/* Set qmode flag */
		s->qmode = QMODE_NONE;

		/* Update widgets */
		dbprog_structupd(s);

		/* Configure the wwwWarp menu */
		wwwwarp_sel_cfg(s);

		XtSetSensitive(widgets.dbprog.reload_btn, True);

		/* Update display */
		dpy_dtitle(s);
		dpy_ttitle(s);

		/* Update curfile */
		dbprog_curfileupd();

		/* Add to history and changer lists */
		dbprog_hist_new(s);
		dbprog_chgr_new(s);
	}
}


/*
 * dbprog_password_vfy
 *	Verify Callback for password text widgets
 */
/*ARGSUSED*/
void
dbprog_password_vfy(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTextVerifyCallbackStruct
		*p = (XmTextVerifyCallbackStruct *)(void *) call_data;
	char	**fptr,
		*str,
		*tp,
		tc;
	int	i,
		len,
		start;

	if (p->reason != XmCR_MODIFYING_TEXT_VALUE)
		return;

	if (w == widgets.auth.pass_txt)
		fptr = &dbp->proxy_passwd;
	else if (w == widgets.userreg.passwd_txt)
		fptr = &dbp->userreg.passwd;
	else if (w == widgets.userreg.vpasswd_txt)
		fptr = &dbp->userreg.vpasswd;
	else
		return;	/* Invalid widget */

	if (p->text->length >= 1 && p->text->ptr != NULL) {
		if (*fptr == NULL) {
			*fptr = (char *) MEM_ALLOC(
				"passwd",
				p->text->length + 1
			);
			if (*fptr == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			(void) strncpy(*fptr, p->text->ptr, p->text->length);
			(*fptr)[p->text->length] = '\0';
		}
		else {
			len = strlen(*fptr);
			start = (p->startPos < len) ? p->startPos : len;
			tp = *fptr + start;
			tc = *tp;
			*tp = '\0';

			str = (char *) MEM_ALLOC(
				"passwd",
				len + p->text->length + 1
			);
			if (str == NULL) {
				CD_FATAL(app_data.str_nomemory);
				return;
			}

			(void) strcpy(str, *fptr);
			(void) strncat(str, p->text->ptr, p->text->length);
			str[start + p->text->length] = '\0';
			*tp = tc;
			(void) strcat(str, tp);

			(void) memset(*fptr, 0, strlen(*fptr));
			MEM_FREE(*fptr);
			*fptr = str;
		}

		/* Display '*' instead of what the user typed */
		for (i = 0; i < p->text->length; i++)
			p->text->ptr[i] = '*';

		p->doit = True;
	}
	else if (p->text->length == 0) {
		/* backspace */
		if (*fptr != NULL && (*fptr)[0] != '\0') {
			len = strlen(*fptr);

			start = (p->startPos < len) ? p->startPos : (len - 1);
			tp = *fptr + ((p->endPos > len) ? len : p->endPos);

			(*fptr)[start] = '\0';
			(void) strcat(*fptr, tp);
		}

		p->doit = True;
	}
}


/*
 * dbprog_auth_ok
 *	Callback for the proxy-authorization OK button
 */
/*ARGSUSED*/
void
dbprog_auth_ok(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	char		*name;

	if (XtIsManaged(widgets.auth.form)) {
		XtUnmapWidget(XtParent(widgets.auth.form));
		XtUnmanageChild(widgets.auth.form);
	}

	auth_initted = TRUE;

	/* Set proxy auth user name */
	name = get_text_string(widgets.auth.name_txt, TRUE);
	if (!util_newstr(&dbp->proxy_user, name)) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	XtFree(name);

	/* Re-load CD information */
	dbprog_dbget(s);
}


/*
 * dbprog_auth_cancel
 *	Callback for the proxy-authorization Cancel button
 */
/*ARGSUSED*/
void
dbprog_auth_cancel(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (XtIsManaged(widgets.auth.form)) {
		XtUnmapWidget(XtParent(widgets.auth.form));
		XtUnmanageChild(widgets.auth.form);
	}

	dbprog_auth_retry(w, NULL, call_data);
}


/*
 * dbprog_dlist_cancel
 *	Callback for the Disc List Cancel button
 */
/*ARGSUSED*/
void
dbprog_dlist_cancel(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtUnmapWidget(XtParent(widgets.dlist.form));
	XtUnmanageChild(widgets.dlist.form);
}


/*
 * dbprog_dlist
 *	Pop up/down the disc list window.
 */
/*ARGSUSED*/
void
dbprog_dlist(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	static bool_t	first = TRUE;

	if (XtIsManaged(widgets.dlist.form)) {
		/* Pop down the Disc List window */
		dbprog_dlist_cancel(w, client_data, call_data);
		return;
	}

	if (dlist_mode == DLIST_HIST) {
		if (!hist_initted) {
			/* Display the in-core history list */
			dbprog_hist_addall(s);

			hist_initted = TRUE;
		}
	}

	/* Pop up the Disc List window.
	 * The dialog has mappedWhenManaged set to False,
	 * so we have to map/unmap explicitly.  The reason
	 * for this is we want to avoid a screen glitch when
	 * we move the window in cd_dialog_setpos(), so we
	 * map the window afterwards.
	 */
	XtManageChild(widgets.dlist.form);
	if (first) {
		first = FALSE;
		/* Set window position */
		cd_dialog_setpos(XtParent(widgets.dlist.form));
	}
	XtMapWidget(XtParent(widgets.dlist.form));

	/* Put input focus on the cancel button */
	XmProcessTraversal(widgets.dlist.cancel_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_dlist_mode
 *	Disc list type selector menu callback
 */
/*ARGSUSED*/
void
dbprog_dlist_mode(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	int		newmode;

	if (w == widgets.dlist.hist_btn)
		newmode = DLIST_HIST;
	else
		newmode = DLIST_CHGR;

	DBGPRN(DBG_UI)(errfp, "\n* DISC LIST: %s mode selected\n",
		(newmode == DLIST_HIST) ? "History" : "CD Changer");

	if (newmode == dlist_mode)
		/* No change */
		return;

	dlist_mode = newmode;

	/* Clean up the disc list widget */
	XmListDeleteAllItems(widgets.dlist.disc_list);
	dlist_pos = -1;
	hist_cnt = 0;

	XtSetSensitive(widgets.dlist.show_btn, False);
	XtSetSensitive(widgets.dlist.goto_btn, False);
	XtSetSensitive(widgets.dlist.del_btn, False);
	XtSetSensitive(widgets.dlist.delall_btn, False);

	if (dlist_mode == DLIST_HIST) {
		/* Display the in-core history list on the
		 * disc list widget
		 */
		dbprog_hist_addall(s);
	}
	else {
		/* Display the in-core CD changer list on the
		 * disc list widget.
		 */
		dbprog_chgr_addall(s);
	}
}


/*
 * dbprog_dlist_select
 *	Disc list browse selection callback
 */
/*ARGSUSED*/
void
dbprog_dlist_select(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct	*p =
		(XmListCallbackStruct *)(void *) call_data;
	cdinfo_dlist_t		*dp;
	int			i;

	if (dlist_pos == p->item_position) {
		/* Already selected: de-select it */
		XmListDeselectPos(w, dlist_pos);
		dlist_pos = -1;

		XtSetSensitive(widgets.dlist.show_btn, False);
		if (dlist_mode == DLIST_HIST)
			XtSetSensitive(widgets.dlist.del_btn, False);
		else if (dlist_mode == DLIST_CHGR)
			XtSetSensitive(widgets.dlist.goto_btn, False);
	}
	else {
		dlist_pos = p->item_position;

		dp = (dlist_mode == DLIST_HIST) ?
			cdinfo_hist_list() : cdinfo_chgr_list();
		for (i = 1; dp != NULL; dp = dp->next, i++) {
			if (i == dlist_pos)
				break;
		}
		if (dp != NULL && dp->device != NULL)
			XtSetSensitive(widgets.dlist.show_btn, True);

		if (dlist_mode == DLIST_HIST)
			XtSetSensitive(widgets.dlist.del_btn, True);
		else if (dlist_mode == DLIST_CHGR)
			XtSetSensitive(widgets.dlist.goto_btn, True);
	}
}


/*
 * dbprog_dlist_show
 *	Show selected disc list entry
 */
/*ARGSUSED*/
void
dbprog_dlist_show(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		i;
	cdinfo_dlist_t	*dp;
	struct tm	*tm;
	char		typstr[16],
			str[FILE_PATH_SZ * 4];

	if (dlist_pos <= 0) {
		/* User has not selected a show target yet */
		cd_beep();
		return;
	}

	dp = (dlist_mode == DLIST_HIST) ?
		cdinfo_hist_list() : cdinfo_chgr_list();
	for (i = 1; dp != NULL; dp = dp->next, i++) {
		if (i == dlist_pos)
			break;
	}

	XmListDeselectPos(widgets.dlist.disc_list, dlist_pos);
	dlist_pos = -1;
	XtSetSensitive(widgets.dlist.show_btn, False);
	XtSetSensitive(widgets.dlist.goto_btn, False);
	XtSetSensitive(widgets.dlist.del_btn, False);

	/* Put input focus on the cancel button */
	XmProcessTraversal(widgets.dlist.cancel_btn, XmTRAVERSE_CURRENT);

	if (dp != NULL) {
#if !defined(__VMS) || ((__VMS_VER >= 70000000) && (__DECC_VER > 50230003))
		tzset();
#endif
		tm = localtime(&dp->time);

		switch (dp->type) {
		case CDINFO_DLIST_LOCAL:
			(void) sprintf(typstr, " (%.10s):",
					app_data.str_local);
			break;
		case CDINFO_DLIST_REMOTE:
		case CDINFO_DLIST_REMOTE1:
			(void) sprintf(typstr, " (%.10s):",
					app_data.str_cddb);
			break;
		default:
			(void) strcpy(typstr, ":");
			break;
		}

		(void) sprintf(str,
		    "%.127s%s%.127s\n\n%s %.63s\n%s %.255s\n%s %d\n%s%s %s",
			(dp->artist == NULL) ? "" : dp->artist,
			(dp->artist != NULL && dp->title != NULL) ? " / " : "",
			(dp->title == NULL) ?
				app_data.str_unkndisc : dp->title,
			"Genre:",
			dp->genre == NULL ?
				"-" : cdinfo_genre_name(dp->genre),
			"Device:",
			dp->device == NULL ? "-" : dp->device,
			"Disc:", dp->discno,
			"Last loaded", typstr,
			dp->time == 0 ? "-" : asctime(tm)
		);
	}
	else {
		/* Error: shouldn't get here */
		cd_beep();
		return;
	}

	/* Pop up information dialog */
	CD_INFO(str);
}


/*
 * dbprog_dlist_goto
 *	Cange to the selected disc list entry
 */
/*ARGSUSED*/
void
dbprog_dlist_goto(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;
	int		newdisc;

	if (dlist_pos <= 0 || dlist_mode != DLIST_CHGR) {
		/* User has not selected a goto target yet,
		 * or wrong list mode.
		 */
		cd_beep();
		return;
	}

	newdisc = dlist_pos;
	XmListDeselectPos(widgets.dlist.disc_list, dlist_pos);
	dlist_pos = -1;
	XtSetSensitive(widgets.dlist.show_btn, False);
	XtSetSensitive(widgets.dlist.goto_btn, False);

	/* Put input focus on the cancel button */
	XmProcessTraversal(widgets.dlist.cancel_btn, XmTRAVERSE_CURRENT);

	if (newdisc == s->cur_disc)
		return;	/* Nothing to do */

	s->prev_disc = s->cur_disc;
	s->cur_disc = newdisc;

	s->flags |= STAT_CHGDISC;

	/* Ask the user if the changed CD information
	 * should be submitted to CDDB.
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
}


/*
 * dbprog_dlist_delete
 *	Delete selected disc list entry
 */
/*ARGSUSED*/
void
dbprog_dlist_delete(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		i;
	cdinfo_dlist_t	*hp;

	if (dlist_pos <= 0 || dlist_mode != DLIST_HIST) {
		/* User has not selected a delete target yet,
		 * or the list mode is not the history list.
		 */
		cd_beep();
		return;
	}

	/* Delete item from list widget */
	XmListDeletePos(widgets.dlist.disc_list, dlist_pos);

	/* Delete item from in-core history list */
	for (i = 1, hp = cdinfo_hist_list(); hp != NULL; hp = hp->next, i++) {
		if (i == dlist_pos) {
			(void) cdinfo_hist_delent(hp, TRUE);
			break;
		}
	}

	if (hist_cnt > 0)
		hist_cnt--;

	XtSetSensitive(widgets.dlist.show_btn, False);
	XtSetSensitive(widgets.dlist.del_btn, False);
	if (cdinfo_hist_list() == NULL)
		XtSetSensitive(widgets.dlist.delall_btn, False);

	dlist_pos = -1;

	/* Put input focus on the cancel button */
	XmProcessTraversal(widgets.dlist.cancel_btn, XmTRAVERSE_CURRENT);
}


/*
 * dbprog_dlist_delall
 *	Delete all disc list entries
 */
/*ARGSUSED*/
void
dbprog_dlist_delall(Widget w, XtPointer client_data, XtPointer call_data)
{
	char	*str;
	size_t	len;

	if (dlist_mode != DLIST_HIST) {
		/* Delete is supported only for the history list */
		cd_beep();
		return;
	}

	/* Put input focus on the cancel button */
	XmProcessTraversal(widgets.dlist.cancel_btn, XmTRAVERSE_CURRENT);

	len = strlen(app_data.str_dlist_delall) +
	      strlen(app_data.str_askproceed) + 2;
	if ((str = (char *) MEM_ALLOC("msg", len)) == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	(void) sprintf(str, "%s\n%s",
		app_data.str_dlist_delall,
		app_data.str_askproceed
	);

	/* Pop up confirm dialog */
	(void) cd_confirm_popup(
		app_data.str_confirm,
		str,
		(XtCallbackProc) dbprog_dlist_delall_yes, client_data,
		(XtCallbackProc) NULL, NULL
	);

	MEM_FREE(str);
}


/*
 * dbprog_dlist_delall_yes
 *	Delete all confirmation dialog "Yes" button callback.
 */
/*ARGSUSED*/
void
dbprog_dlist_delall_yes(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtSetSensitive(widgets.dlist.show_btn, False);
	XtSetSensitive(widgets.dlist.del_btn, False);
	XtSetSensitive(widgets.dlist.delall_btn, False);

	/* Delete all items in the list widget */
	XmListDeleteAllItems(widgets.dlist.disc_list);

	/* Delete in-core history list and history file */
	cdinfo_hist_delall(TRUE);

	dlist_pos = -1;
	hist_cnt = 0;
}


/*
 * dbprog_dlist_rescan
 *	Re-scan and re-display disc list entries
 */
/*ARGSUSED*/
void
dbprog_dlist_rescan(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	if (dlist_mode == DLIST_HIST) {
		/* Change to watch cursor */
		cd_busycurs(TRUE, CURS_DLIST);

		XtSetSensitive(widgets.dlist.show_btn, False);
		XtSetSensitive(widgets.dlist.goto_btn, False);
		XtSetSensitive(widgets.dlist.del_btn, False);
		XtSetSensitive(widgets.dlist.delall_btn, False);

		/* Delete all items in the list widget */
		XmListDeleteAllItems(widgets.dlist.disc_list);

		/* Delete in-core history list */
		cdinfo_hist_delall(FALSE);

		dlist_pos = -1;
		hist_cnt = 0;

		/* Re-load history file */
		cdinfo_hist_init();

		/* Update list widget */
		dbprog_hist_addall(s);

		/* Change to normal cursor */
		cd_busycurs(FALSE, CURS_DLIST);
	}
	else {
		if (app_data.numdiscs == 1)
			/* Not a changer: nothing to do */
			return;

		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			/* Stop playing first */
			di_stop(s, FALSE);
			break;
		default:
			break;
		}

		/* Make the re-scan button insensitive for now */
		XtSetSensitive(w, False);

		/* Put input focus on the cancel button */
		XmProcessTraversal(
			widgets.dlist.cancel_btn,
			XmTRAVERSE_CURRENT
		);

		/* Change to watch cursor */
		cd_busycurs(TRUE, CURS_ALL);

		if (app_data.numdiscs >= 3) {
			/* Pop up the working dialog */
			cd_working_popup(
				app_data.str_working,
				app_data.str_chgrscan,
				(XtCallbackProc) dbprog_scan_stop_btn,
				(XtPointer) s
			);
		}

		/* Set multiplay mode so we don't block on an empty slot.
		 * Also force reverse mode to be false for scanning.
		 */
		sav_mplay = app_data.multi_play;
		sav_rev = app_data.reverse;
		app_data.multi_play = TRUE;
		app_data.reverse = FALSE;

		/* Start scanning */
		start_slot = s->cur_disc;
		scan_slot = 0;
		s->chgrscan = TRUE;
		dbprog_chgr_scan_next(s);
	}
}


/*
 * dbprog_scan_stop_btn
 *	Rescan working dialog box stop button callback function
 */
/*ARGSUSED*/
void
dbprog_scan_stop_btn(Widget w, XtPointer client_data, XtPointer call_data)
{
	curstat_t	*s = (curstat_t *)(void *) client_data;

	dbprog_chgr_scan_stop(s);
}


/**************** ^^ Callback routines ^^ ****************/


