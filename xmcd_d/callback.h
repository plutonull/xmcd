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
#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#ifndef lint
static char *_callback_h_ident_ = "@(#)callback.h	6.14 03/12/12";
#endif

/* Macros */
#define register_activate_cb(w, func, arg)				\
	XtAddCallback((w), XmNactivateCallback,				\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_arm_cb(w, func, arg)					\
	XtAddCallback((w), XmNarmCallback,				\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_disarm_cb(w, func, arg)				\
	XtAddCallback((w), XmNdisarmCallback,				\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_valchg_cb(w, func, arg)				\
	XtAddCallback((w), XmNvalueChangedCallback,			\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_drag_cb(w, func, arg)					\
	XtAddCallback((w), XmNdragCallback,				\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_modvfy_cb(w, func, arg)				\
	XtAddCallback((w), XmNmodifyVerifyCallback,			\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_focus_cb(w, func, arg)					\
	XtAddCallback((w), XmNfocusCallback,				\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_losefocus_cb(w, func, arg)				\
	XtAddCallback((w), XmNlosingFocusCallback,			\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_defaction_cb(w, func, arg)				\
	XtAddCallback((w), XmNdefaultActionCallback,			\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_singlesel_cb(w, func, arg)				\
	XtAddCallback((w), XmNsingleSelectionCallback,			\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_browsel_cb(w, func, arg)				\
	XtAddCallback((w), XmNbrowseSelectionCallback,			\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_cascade_cb(w, func, arg)				\
	XtAddCallback((w), XmNcascadingCallback,			\
		(XtCallbackProc) (func),				\
		(XtPointer) (arg))

#define register_btnrel_ev(w, func, arg)				\
	XtAddEventHandler((w), ButtonReleaseMask,			\
			  False,					\
			  (XtEventHandler) (func),			\
			  (XtPointer) (arg))

#define register_focuschg_ev(w, func, arg)				\
	XtAddEventHandler((w), FocusChangeMask,				\
			  False,					\
			  (XtEventHandler) (func),			\
			  (XtPointer) (arg))

#define register_xingchg_ev(w, func, arg)				\
	XtAddEventHandler((w), EnterWindowMask | LeaveWindowMask,	\
			  False,					\
			  (XtEventHandler) (func),			\
			  (XtPointer) (arg))


/* Public function prototypes */
extern void	set_delw_atom(Atom);
extern void	add_delw_callback(Widget, XtCallbackProc, XtPointer);
extern void	rm_delw_callback(Widget, XtCallbackProc, XtPointer);
extern void	register_callbacks(widgets_t *, curstat_t *);

#endif /* __CALLBACK_H__ */

