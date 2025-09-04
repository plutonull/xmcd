/*
 *   cdda - CD Digital Audio support
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
 */
#ifndef __PTHR_H__
#define __PTHR_H__

#ifndef lint
static char *_pthr_h_ident_ = "@(#)pthr.h	7.9 04/02/13";
#endif

#ifdef CDDA_PTHREADS

#include <pthread.h>


/* Function prototypes */
extern word32_t	cdda_pthr_capab(void);
extern void	cdda_pthr_preinit(void);
extern bool_t	cdda_pthr_init(curstat_t *s);
extern void	cdda_pthr_halt(di_dev_t *, curstat_t *);
extern bool_t	cdda_pthr_play(di_dev_t *, curstat_t *, sword32_t, sword32_t);
extern bool_t	cdda_pthr_pause_resume(di_dev_t *, curstat_t *, bool_t);
extern bool_t	cdda_pthr_stop(di_dev_t *, curstat_t *);
extern int	cdda_pthr_vol(di_dev_t *, curstat_t *, int, bool_t);
extern bool_t	cdda_pthr_chroute(di_dev_t *, curstat_t *);
extern void	cdda_pthr_att(curstat_t *);
extern void	cdda_pthr_outport(void);
extern bool_t	cdda_pthr_getstatus(di_dev_t *, curstat_t *, cdstat_t *);
extern void	cdda_pthr_debug(word32_t);
extern void	cdda_pthr_info(char *);
extern int	cdda_pthr_initipc(cd_state_t *);
extern void	cdda_pthr_waitsem(int, int);
extern void	cdda_pthr_postsem(int, int);
extern void	cdda_pthr_yield(void);
extern void	cdda_pthr_kill(thid_t, int);

#else

#define cdda_pthr_capab		NULL
#define cdda_pthr_preinit	NULL
#define cdda_pthr_init		NULL
#define cdda_pthr_halt		NULL
#define cdda_pthr_play		NULL
#define cdda_pthr_pause_resume	NULL
#define cdda_pthr_stop		NULL
#define cdda_pthr_vol		NULL
#define cdda_pthr_chroute	NULL
#define cdda_pthr_att		NULL
#define cdda_pthr_outport	NULL
#define cdda_pthr_getstatus	NULL
#define cdda_pthr_debug		NULL
#define cdda_pthr_info		NULL
#define cdda_pthr_initipc	NULL
#define cdda_pthr_waitsem	NULL
#define cdda_pthr_postsem	NULL
#define cdda_pthr_yield		NULL
#define cdda_pthr_kill		NULL

#endif	/* CDDA_PTHREADS */

#endif	/* __PTHR_H__ */

