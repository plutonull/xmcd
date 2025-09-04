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
#ifndef __HELP_H__
#define __HELP_H__

#ifndef lint
static char *_help_h_ident_ = "@(#)help.h	6.29 03/12/12";
#endif


#define HELP_XLAT_1	1
#define HELP_XLAT_2	2

/* This structure is used to map widgets to associated help files.
 * Instead of using XtName(), this mechanism allows us to map multiple
 * widgets to a common help file.  Also, we can use arbitrary lengths
 * for the widget name and still have help files with less than 14 chars
 * in its name (necessary for compatibility with some systems).
 */
typedef struct {
	Widget	*widgetp;
	char	*hlpname;
	int	xlat_typ;
} wname_t;


/* Documentation topics list structure */
typedef struct doc_topic {
	char		*name;
	char		*path;
	Widget		actbtn;
	struct doc_topic *next;
} doc_topic_t;


/* Public functions */
extern void	help_init(void);
extern void	help_start(void);
extern void	help_loadfile(char *);
extern void	help_popup(Widget);
extern void	help_popdown(void);
extern bool_t	help_isactive(void);


/* Callback functions */
extern void	help_topic_sel(Widget, XtPointer, XtPointer);

#endif	/* __HELP_H__ */
