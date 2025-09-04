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
#ifndef __USERREG_H__
#define __USERREG_H__

#ifndef lint
static char *_userreg_h_ident_ = "@(#)userreg.h	7.13 03/12/12";
#endif


/* Public functions */
extern void	userreg_structupd(curstat_t *);
extern void	userreg_do_popup(curstat_t *s, bool_t);

/* Callback functions */
extern void	userreg_popup(Widget, XtPointer, XtPointer);
extern void	userreg_focus_next(Widget, XtPointer, XtPointer);
extern void	userreg_gender_sel(Widget, XtPointer, XtPointer);
extern void	userreg_gethint(Widget, XtPointer, XtPointer);
extern void	userreg_ok(Widget, XtPointer, XtPointer);
extern void	userreg_privacy(Widget, XtPointer, XtPointer);
extern void	userreg_cancel(Widget, XtPointer, XtPointer);

#endif	/* __DBPROG_H__ */
