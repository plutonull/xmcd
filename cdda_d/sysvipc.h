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
#ifndef __SYSVIPC_H__
#define __SYSVIPC_H__

#ifndef lint
static char *_sysvipc_h_ident_ = "@(#)sysvipc.h	7.49 04/03/17";
#endif

#ifdef CDDA_SYSVIPC

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

/* Shared memory keys */
#define	SHMKEY			1111	/* Base key for shared memory */
#define	SEMKEY			2222	/* Base key for semaphores */

#if defined(_SUNOS4) || defined(sgi) || defined(__bsdi__) || \
    defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || \
    (defined(_LINUX) && !defined(_SEM_SEMUN_UNDEFINED))

/* These platforms (incorrectly) pre-defines "union semun" when
 * <sys/sem.h> is included.
 */

#else

/* Semaphore arg */
union semun {
	int		val;
	struct semid_ds	*buf;
	unsigned short	*array;
};

#endif

typedef union semun	semun_t;

/* Shared memory and semaphore permissions */
#ifdef HAS_EUID
#define IPC_PERMS	0600
#else
#define IPC_PERMS	0666
#endif


/* Function prototypes */
extern word32_t	cdda_sysvipc_capab(void);
extern void	cdda_sysvipc_preinit(void);
extern bool_t	cdda_sysvipc_init(curstat_t *s);
extern void	cdda_sysvipc_halt(di_dev_t *, curstat_t *);
extern bool_t	cdda_sysvipc_play(di_dev_t *, curstat_t *,
				  sword32_t, sword32_t);
extern bool_t	cdda_sysvipc_pause_resume(di_dev_t *, curstat_t *, bool_t);
extern bool_t	cdda_sysvipc_stop(di_dev_t *, curstat_t *);
extern int	cdda_sysvipc_vol(di_dev_t *, curstat_t *, int, bool_t);
extern bool_t	cdda_sysvipc_chroute(di_dev_t *, curstat_t *);
extern void	cdda_sysvipc_att(curstat_t *);
extern void	cdda_sysvipc_outport(void);
extern bool_t	cdda_sysvipc_getstatus(di_dev_t *, curstat_t *, cdstat_t *);
extern void	cdda_sysvipc_debug(word32_t);
extern void	cdda_sysvipc_info(char *);
extern int	cdda_sysvipc_initipc(cd_state_t *);
extern void	cdda_sysvipc_waitsem(int, int);
extern void	cdda_sysvipc_postsem(int, int);
extern void	cdda_sysvipc_yield(void);
extern void	cdda_sysvipc_kill(thid_t, int);

#else

#define cdda_sysvipc_capab		NULL
#define cdda_sysvipc_preinit		NULL
#define cdda_sysvipc_init		NULL
#define cdda_sysvipc_halt		NULL
#define cdda_sysvipc_play		NULL
#define cdda_sysvipc_pause_resume	NULL
#define cdda_sysvipc_stop		NULL
#define cdda_sysvipc_vol		NULL
#define cdda_sysvipc_chroute		NULL
#define cdda_sysvipc_att		NULL
#define cdda_sysvipc_outport		NULL
#define cdda_sysvipc_getstatus		NULL
#define cdda_sysvipc_debug		NULL
#define cdda_sysvipc_info		NULL
#define cdda_sysvipc_initipc		NULL
#define cdda_sysvipc_waitsem		NULL
#define cdda_sysvipc_postsem		NULL
#define cdda_sysvipc_yield		NULL
#define cdda_sysvipc_kill		NULL

#endif	/* CDDA_SYSVIPC */

#endif	/* __SYSVIPC_H__ */

