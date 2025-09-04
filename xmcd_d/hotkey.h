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
#ifndef __HOTKEY_H__
#define __HOTKEY_H__

#ifndef lint
static char *_hotkey_h_ident_ = "@(#)hotkey.h	6.18 03/12/12";
#endif

/* Public functions */
extern void	hotkey_init(void);
extern void	hotkey_grabkeys(Widget);
extern void	hotkey_ungrabkeys(Widget);
extern void	hotkey_tooltip_mnemonic(Widget);
extern bool_t	hotkey_ckkey(char *, char *);
extern void	hotkey(Widget, XEvent *, String *, Cardinal *);

#endif	/* __HOTKEY_H__ */
