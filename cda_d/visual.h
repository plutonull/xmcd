/*
 *   cda - Command-line CD Audio Player/Ripper
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
#ifndef __VISUAL_H__
#define __VISUAL_H__

#ifndef NOVISUAL

#ifndef lint
static char *_visual_h_ident_ = "@(#)visual.h	6.17 03/12/12";
#endif


#ifndef ACS_HLINE
#define ACS_HLINE	'-'
#endif

#ifdef __QNX__
#ifdef putp
#undef putp
#endif
#define putp(x)			/* Don't use putp under QNX */
#endif	/* __QNX__ */


/*
 * Constant definitions
 */

#define ON_X		2
#define ON_Y		4
#define OFF_X		5
#define OFF_Y		4
#define LOAD_X		13
#define LOAD_Y		4
#define EJECT_X		13
#define EJECT_Y		5
#define PLAY_X		23
#define PLAY_Y		4
#define PAUSE_X		23
#define PAUSE_Y		5
#define STOP_X		33
#define STOP_Y		4
#define LOCK_X		43
#define LOCK_Y		4
#define UNLOCK_X	42
#define UNLOCK_Y	5
#define SHUFFLE_X	52
#define SHUFFLE_Y	4
#define PROGRAM_X	52
#define PROGRAM_Y	5
#define REPEAT_ON_X	62
#define REPEAT_ON_Y	5
#define REPEAT_OFF_X	65
#define REPEAT_OFF_Y	5


#define HELP_INFO "\
                      CDA - Command Line CD Audio Player/Ripper\n\
\n\
F1 (o)  On/Off             D / d                        Prev/Next disc\n\
F2 (j)  Load/Eject         Cursor left/right (C/c)      Prev/Next track\n\
F3 (p)  Play/Pause         < / >                        Prev/Next index\n\
F4 (s)  Stop               Cursor up/down (ctl-^/ctl-v) Scroll screen\n\
F5 (k)  Lock/Unlock        + / -                        Volume up/down\n\
F6 (u)  Shuffle/Program    l / r                        Balance left/right\n\
F7 (e)  Repeat On/Off      Tab                          Stereo/Mono\n\
F8 (q)  Quit               n [mins secs]                Track n [at mins/secs]\n\
                           ^L or ^R                     Refresh screen\n\
\n\
                           ?        Help\n\
\n\
(Alternatives to function and cursor control keys given in parenthesis)\n\
\n\
	             Press space to return to info screen."


#define HELP_SCROLL_LENGTH	16

#define STATUS_LINE0 "\
   F1(o)     F2(j)     F3(p)     F4(s)     F5(k)     F6(u)     F7(e)     F8(q)"

#define STATUS_LINE1 "\
  On/Off     Load/     Play/     Stop      Lock/    Shuffle/  Repeat     Quit"

#define STATUS_LINE2 "\
             Eject     Pause              Unlock    Program   On/Off"


/* Public function prototypes */
extern void	cda_vtidy(void);
extern void	cda_visual(void);


#endif	/* NOVISUAL */

#endif	/* __VISUAL_H__ */

