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
#ifndef __XMCD_H__
#define __XMCD_H__

#ifndef lint
static char *_xmcd_h_ident_ = "@(#)xmcd.h	6.29 04/01/10";
#endif


/* Program name string */
#define PROGNAME	"xmcd"

/*
 * Include needed X11 and Motif headers
 */
#include <X11/keysym.h>
#include <Xm/Xm.h>
#include <Xm/ArrowB.h>
#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include "labelh_d/LabelH.h"

/* Motif 1.1 compatibility */
#ifndef XmFMT_8_BIT
#define XmFMT_8_BIT	FMT8BIT
#endif
#ifndef XmFMT_16_BIT
#define XmFMT_16_BIT	FMT16BIT
#endif

/* A hack to work around a compile problem on some platforms */
#if defined(offsetof) && defined(XtOffsetOf)
#undef XtOffsetOf
#define XtOffsetOf(s_type,field) XtOffset(s_type*,field)
#endif

/* Character set/font */
#define CHSET1		"chset1"
#define CHSET2		"chset2"
#define CHSET3		"chset3"
#define CHSET4		"chset4"

/* Main display modes */
#define MAIN_NORMAL	0		/* Normal mode */
#define MAIN_BASIC	1		/* Basic mode */


/* Public function prototypes */
extern curstat_t	*curstat_addr(void);
extern void		event_loop(int);
extern void		shutdown_gui(void);

#endif	/* __XMCD_H__ */
