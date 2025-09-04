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
#ifndef __DBPROG_H__
#define __DBPROG_H__

#ifndef lint
static char *_dbprog_h_ident_ = "@(#)dbprog.h	7.41 03/12/12";
#endif


#define TRKLIST_FMT	" %02d  %02u:%02u  %s%s "
#define CREDITLIST_FMT	"%s (%s)%s"
#define SEGLIST_FMT	"%s (%s/%s -> %s/%s)%s"
#define MATCHLIST_FMT	"%s / %s %s%s%s"
#define HISTLIST_FMT	"%.3s %02d %02d:%02d  %.127s%s%.127s"
#define CHGRLIST_FMT	"Disc %-4d  %.127s%s%.127s"
#define UNDEF_STR	"??"
#define ASTERISK_STR	"*"

/* Track list time modes */
#define TIME_TOTAL	1
#define TIME_TRACK	2

/* Disc list window modes */
#define DLIST_HIST	1
#define DLIST_CHGR	2

/* Credit list window modes */
#define CREDITS_NONE	0
#define CREDITS_DISC	1
#define CREDITS_TRACK	2
#define CREDITS_SEG	3

/* Fullname window modes */
#define FNAME_NONE	0
#define FNAME_DISC	1
#define FNAME_TRACK	2
#define FNAME_CREDITS	3

/* Region selector window modes */
#define REGION_NONE	0
#define REGION_DISC	1
#define REGION_USERREG	2


/* Public functions */
extern void	dbprog_curfileupd(void);
extern void	dbprog_curtrkupd(curstat_t *);
extern void	dbprog_progclear(curstat_t *);
extern void	dbprog_dbclear(curstat_t *, bool_t);
extern void	dbprog_progget(curstat_t *);
extern void	dbprog_dbget(curstat_t *);
extern void	dbprog_chgr_scan_stop(curstat_t *);
extern void	dbprog_init(curstat_t *);
extern bool_t	dbprog_chgsubmit(curstat_t *);
extern char	*dbprog_curartist(curstat_t *);
extern char	*dbprog_curtitle(curstat_t *);
extern char	*dbprog_curttitle(curstat_t *);
extern cdinfo_incore_t
		*dbprog_curdb(curstat_t *);
extern int	dbprog_curseltrk(curstat_t *);
extern bool_t	dbprog_pgm_parse(curstat_t *);
extern void	dbprog_segments_setmode(curstat_t *);
extern void	dbprog_segments_cancel(curstat_t *);
extern bool_t	dbprog_stopload_active(int, bool_t);

/* Callback functions */
extern void	dbprog_popup(Widget, XtPointer, XtPointer);
extern void	dbprog_inetoffln(Widget, XtPointer, XtPointer);
extern void	dbprog_text_new(Widget, XtPointer, XtPointer);
extern void	dbprog_focus_next(Widget, XtPointer, XtPointer);
extern void	dbprog_trklist_play(Widget, XtPointer, XtPointer);
extern void	dbprog_trklist_select(Widget, XtPointer, XtPointer);
extern void	dbprog_ttitle_focuschg(Widget, XtPointer, XtPointer);
extern void	dbprog_ttitle_new(Widget, XtPointer, XtPointer);
extern void	dbprog_pgmseq_verify(Widget, XtPointer, XtPointer);
extern void	dbprog_pgmseq_txtchg(Widget, XtPointer, XtPointer);
extern void	dbprog_addpgm(Widget, XtPointer, XtPointer);
extern void	dbprog_clrpgm(Widget, XtPointer, XtPointer);
extern void	dbprog_savepgm(Widget, XtPointer, XtPointer);
extern void	dbprog_submit(Widget, XtPointer, XtPointer);
extern void	dbprog_submit_popup(Widget, XtPointer, XtPointer);
extern void	dbprog_submit_yes(Widget, XtPointer, XtPointer);
extern void	dbprog_submit_url(Widget, XtPointer, XtPointer);
extern void	dbprog_submit_url_chg(Widget, XtPointer, XtPointer);
extern void	dbprog_submit_url_submit(Widget, XtPointer, XtPointer);
extern void	dbprog_submit_url_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_flush(Widget, XtPointer, XtPointer);
extern void	dbprog_load(Widget, XtPointer, XtPointer);
extern void	dbprog_stop_load_yes(Widget, XtPointer, XtPointer);
extern void	dbprog_stop_load_no(Widget, XtPointer, XtPointer);
extern void	dbprog_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_do_clear(Widget, XtPointer, XtPointer);
extern void	dbprog_timedpy(Widget, XtPointer, XtPointer);
extern void	dbprog_set_changed(Widget, XtPointer, XtPointer);
extern void	dbprog_fullname(Widget, XtPointer, XtPointer);
extern void	dbprog_fullname_autogen(Widget, XtPointer, XtPointer);
extern void	dbprog_fullname_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_genre_sel(Widget, XtPointer, XtPointer);
extern void	dbprog_subgenre_sel(Widget, XtPointer, XtPointer);
extern void	dbprog_role_sel(Widget, XtPointer, XtPointer);
extern void	dbprog_subrole_sel(Widget, XtPointer, XtPointer);
extern void	dbprog_extd(Widget, XtPointer, XtPointer);
extern void	dbprog_extd_compilation(Widget, XtPointer, XtPointer);
extern void	dbprog_extd_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_extt(Widget, XtPointer, XtPointer);
extern void	dbprog_extt_prev(Widget, XtPointer, XtPointer);
extern void	dbprog_extt_next(Widget, XtPointer, XtPointer);
extern void	dbprog_extt_autotrk(Widget, XtPointer, XtPointer);
extern void	dbprog_extt_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_credits_popup(Widget, XtPointer, XtPointer);
extern void	dbprog_credits_popdown(Widget, XtPointer, XtPointer);
extern void	dbprog_credits_select(Widget, XtPointer, XtPointer);
extern void	dbprog_credits_add(Widget, XtPointer, XtPointer);
extern void	dbprog_credits_mod(Widget, XtPointer, XtPointer);
extern void	dbprog_credits_del(Widget, XtPointer, XtPointer);
extern void	dbprog_credits_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_segments_popup(Widget, XtPointer, XtPointer);
extern void	dbprog_segments_popdown(Widget, XtPointer, XtPointer);
extern void	dbprog_segments_select(Widget, XtPointer, XtPointer);
extern void	dbprog_segments_add(Widget, XtPointer, XtPointer);
extern void	dbprog_segments_mod(Widget, XtPointer, XtPointer);
extern void	dbprog_segments_del(Widget, XtPointer, XtPointer);
extern void	dbprog_segments_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_regionsel_popup(Widget, XtPointer, XtPointer);
extern void	dbprog_regionsel_select(Widget, XtPointer, XtPointer);
extern void	dbprog_regionsel_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_langsel_popup(Widget, XtPointer, XtPointer);
extern void	dbprog_langsel_select(Widget, XtPointer, XtPointer);
extern void	dbprog_langsel_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_matchsel_select(Widget, XtPointer, XtPointer);
extern void	dbprog_matchsel_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_the(Widget, XtPointer, XtPointer);
extern void	dbprog_password_vfy(Widget, XtPointer, XtPointer);
extern void	dbprog_auth_retry(Widget, XtPointer, XtPointer);
extern void	dbprog_auth_ok(Widget, XtPointer, XtPointer);
extern void	dbprog_auth_cancel(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_cancel(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_mode(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_select(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_show(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_goto(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_delete(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_delall(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_delall_yes(Widget, XtPointer, XtPointer);
extern void	dbprog_dlist_rescan(Widget, XtPointer, XtPointer);
extern void	dbprog_scan_stop_btn(Widget, XtPointer, XtPointer);

#endif	/* __DBPROG_H__ */
