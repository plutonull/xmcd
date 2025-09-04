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
 *
 *	This file contains code to support CDDA operations with
 *	parallel reader/writer processes using System V IPC
 *	shared memory and semaphores.
 */
#ifndef lint
static char *_sysvipc_c_ident_ = "@(#)sysvipc.c	7.158 04/03/17";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"


#ifdef CDDA_SYSVIPC

#include "cdda_d/sysvipc.h"

/*
 * Platform-specific configuration for process scheduling control
 */
#if defined(_POSIX_PRIORITY_SCHEDULING)
/* If _POSIX_PRIORITY_SCHEDULING is defined in <unistd.h> then
 * the sched_setscheduler(2) system call is supported.
 */
#include <sched.h>
#define USE_POSIX_SCHED
#endif

#if !defined(USE_POSIX_SCHED) && \
    (defined(_LINUX)  || defined(_SUNOS4) || defined(__hpux)   || \
     defined(_AIX)    || defined(_ULTRIX) || defined(__bsdi__) || \
     defined(FreeBSD) || defined(NetBSD)  || defined(OpenBSD))
#include <sys/resource.h>
#define USE_SETPRIORITY
#endif

#if !defined(USE_POSIX_SCHED) && defined(SVR4) && !defined(sgi)
#include <sys/priocntl.h>
#ifdef _UNIXWARE2
#include <sys/fppriocntl.h>		/* UnixWare 2.x and later */
#else
#include <sys/rtpriocntl.h>		/* Most SVR4 including Solaris */
#endif
#define USE_PRIOCNTL
#endif

#if !defined(USE_POSIX_SCHED) && !defined(USE_SETPRIORITY) && \
    !defined(USE_PRIOCNTL)
#define USE_NICE			/* Fallback to nice(2) */
#endif


extern appdata_t	app_data;
extern FILE		*errfp;
extern cdda_client_t	*cdda_clinfo;

extern cdda_rd_tbl_t	cdda_rd_calltbl[];
extern cdda_wr_tbl_t	cdda_wr_calltbl[];

STATIC void		*sysvipc_shmaddr;	/* Shared memory pointer */
STATIC int		shmid = -1,		/* Shared memory identifier */
			semid = -1,		/* Semaphores identifier */
			skip_hbchk = 0;		/* Skip heartbeat checking */
STATIC cd_state_t	*cd = NULL;		/* CD state pointer */
STATIC char		errbuf[ERR_BUF_SZ];	/* Error message buffer */


#if defined(__GNUC__) && __GNUC__ >= 2 && \
    defined(sgi) && (defined(_ABIN32) || defined(_ABI64))

#define SEMCTL	irix_gcc_semctl

/*
 * gcc_semctl
 *	Wrapper function to re-align the semun_t structure for
 *	SGI IRIX 6.x.  Needed when compiling with gcc due to
 *	differences in calling convention between gcc and IRIX's libc.
 *
 * Args:
 *	Same as semctl(2).
 *
 * Return:
 *	Same as semctl(2).
 */
STATIC int
irix_gcc_semctl(int semid, int semnun, int cmd, semun_t arg)
{
	return semctl(semid, semnun, cmd, (word64_t) arg.val << 32);
}

#else

#define SEMCTL	semctl

#endif


/*
 * cdda_sysvipc_rsig
 *	Reader process signal handler function
 *
 * Args:
 *	signo - The signal number
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cdda_sysvipc_rsig(int signo)
{
	void	(*rdone)(bool_t);

	rdone = cdda_rd_calltbl[app_data.cdda_rdmethod].readdone;

	/* Call cleanup function */
	(*rdone)(FALSE);

	/* End reader process */
	_exit(1);
}


/*
 * cdda_sysvipc_wsig
 *	Writer process signal handler function
 *
 * Args:
 *	signo - The signal number
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cdda_sysvipc_wsig(int signo)
{
	void	(*wdone)(bool_t);

	wdone = cdda_wr_calltbl[app_data.cdda_wrmethod].writedone;

	/* Call cleanup function */
	(*wdone)(FALSE);

	/* End writer process */
	_exit(1);
}


/* 
 * cdda_sysvipc_highpri
 *	Increase the calling process' scheduling priority
 *
 * Args:
 *	name - process name string
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_sysvipc_highpri(char *name)
{
#ifdef HAS_EUID
	uid_t	savuid = geteuid();

	if (util_seteuid(0) < 0 || geteuid() != 0)
#else
	if (getuid() != 0)
#endif
	{
		DBGPRN(DBG_GEN)(errfp,
			"cdda_sysvipc_setpri: Cannot change scheduling: "
			"Insufficient privilege.\n");
#ifdef HAS_EUID
		(void) util_seteuid(savuid);
#endif
		return;
	}

#ifdef USE_NICE
	{
		int	ret;

		if ((ret = nice(-10)) == -1) {
			DBGPRN(DBG_GEN)(errfp,
				"cdda_sysvipc_highpri: "
				"Cannot change scheduling: errno=%d\n",
				errno);
#ifdef HAS_EUID
			(void) util_seteuid(savuid);
#endif
			return;
		}
		DBGPRN(DBG_GEN)(errfp,
				"\n%s running with nice value %d (pid=%d)\n",
				name, ret, (int) getpid());
#ifdef HAS_EUID
		(void) util_seteuid(savuid);
#endif
		return;
	}
#endif	/* USE_NICE */

#ifdef USE_SETPRIORITY
	{
		int	pri = -10;

		if (setpriority(PRIO_PROCESS, 0, pri) < 0) {
			DBGPRN(DBG_GEN)(errfp,
				"cdda_sysvipc_highpri: "
				"Cannot change scheduling: errno=%d\n",
				errno);
#ifdef HAS_EUID
			(void) util_seteuid(savuid);
#endif
			return;
		}
		DBGPRN(DBG_GEN)(errfp,
			    "\n%s running with priority value %d (pid=%d)\n",
			    name, pri, (int) getpid());
#ifdef HAS_EUID
		(void) util_seteuid(savuid);
#endif
		return;
	}
#endif	/* USE_SETPRIORITY */

#ifdef USE_PRIOCNTL
	{
		char		*sched_class;
		pcinfo_t	pci;
		pcparms_t	pcp;

#ifdef _UNIXWARE2
		fpparms_t	*fp;

		/* set fixed priority time class */
		fp = (fpparms_t *) &pcp.pc_clparms;
		fp->fp_pri = FP_NOCHANGE;
		fp->fp_tqsecs = (unsigned long) FP_TQDEF;
		fp->fp_tqnsecs = FP_TQDEF;
		sched_class = "FP";
#else
		rtparms_t	*rt;

		/* set real time class */
		rt = (rtparms_t *) &pcp.pc_clparms;
		rt->rt_pri = RT_NOCHANGE;
		rt->rt_tqsecs = (unsigned long) RT_TQDEF;
		rt->rt_tqnsecs = RT_TQDEF;
		sched_class = "RT";
#endif
		(void) strcpy(pci.pc_clname, sched_class);

		if (priocntl(0, 0, PC_GETCID, (void *) &pci) < 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_sysvipc_highpri: "
				"priocntl PC_GETCID failed: "
				"errno=%d\n", errno);
#ifdef HAS_EUID
			(void) util_seteuid(savuid);
#endif
			return;
		}

		pcp.pc_cid = pci.pc_cid;

		if (priocntl(P_PID, getpid(), PC_SETPARMS, (void *) &pcp) < 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_sysvipc_highpri: "
				"priocntl PC_SETPARMS failed: "
				"errno=%d\n", errno);
			return;
		}
		DBGPRN(DBG_GEN)(errfp,
				"\n%s running SVR4 %s scheduling (pid=%d)\n",
				name, sched_class, (int) getpid());
#ifdef HAS_EUID
		(void) util_seteuid(savuid);
#endif
		return;
	}
#endif	/* USE_PRIOCNTL */

#ifdef USE_POSIX_SCHED
	{
		int			minpri,
					maxpri;
		struct sched_param	scparm;

		/* Set priority to 80% between min and max */
		minpri = sched_get_priority_min(SCHED_RR);
		maxpri = sched_get_priority_max(SCHED_RR);

		if (minpri < 0 || maxpri < 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_sysvipc_highpri: "
				"Failed getting min/max priority values: "
				"errno=%d\n", errno);
#ifdef HAS_EUID
			(void) util_seteuid(savuid);
#endif
			return;
		}

		scparm.sched_priority = ((maxpri - minpri) * 8 / 10) + minpri;

		if (sched_setscheduler(0, SCHED_RR, &scparm) < 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_sysvipc_highpri: "
				"sched_setscheduler failed: "
				"errno=%d\n", errno);
#ifdef HAS_EUID
			(void) util_seteuid(savuid);
#endif
			return;
		}

		DBGPRN(DBG_GEN)(errfp,
			    "\n%s running POSIX SCHED_RR scheduling (pid=%d)\n"
			    "Priority=%d (range: %d,%d)\n",
			    name, (int) getpid(),
			    scparm.sched_priority, minpri, maxpri);

#ifdef HAS_EUID
		(void) util_seteuid(savuid);
#endif
		return;
	}
#endif	/* USE_POSIX_SCHED */
}


/*
 * cdda_sysvipc_getsem
 *	Initialize semaphores, making lock available.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
cdda_sysvipc_getsem(void)
{
	semun_t	arg;

	/* Initialize semaphores */
	if ((semid = semget(SEMKEY + app_data.devnum, 3,
			    IPC_PERMS|IPC_CREAT|IPC_EXCL)) < 0) {
		/* If the semaphores exist already, use them. This
		 * should not happen if the application previously
		 * exited correctly.
		 */
		if (errno == EEXIST) {
			if ((semid = semget(SEMKEY + app_data.devnum, 3,
					    IPC_PERMS|IPC_EXCL)) < 0) {
				(void) sprintf(errbuf, "cdda_sysvipc_getsem: "
					    "semget failed (errno=%d)", errno);
				CDDA_INFO(errbuf);
				DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
				return FALSE;
			}
		}
		else {
			(void) sprintf(errbuf, "cdda_sysvipc_getsem: "
				    "semget failed (errno=%d)", errno);
			CDDA_INFO(errbuf);
			DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
			return FALSE;
		}
	}

	/* Make lock available */
	arg.val = 1;
	if (SEMCTL(semid, LOCK, SETVAL, arg) < 0) {
		(void) sprintf(errbuf, "cdda_sysvipc_getsem: "
			    "semctl SETVAL failed (errno=%d)", errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		return FALSE;
	}

	return TRUE;
}


/*
 * cdda_sysvipc_getshm
 *	Initialize shared memory.  We first work out how much shared
 *	memory we require and fill in a temporary cd_size_t structure
 *	along the way. We then create the shared memory segment and
 *	set the fields of *cd pointing into it.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
cdda_sysvipc_getshm(void)
{
	cd_size_t	*cd_tmp_size;

	/* Store sizes here temporarily */
	cd_tmp_size = (cd_size_t *) MEM_ALLOC(
		"cd_tmp_size", sizeof(cd_size_t)
	);
	if (cd_tmp_size == NULL) {
		CDDA_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	(void) memset(cd_tmp_size, 0, sizeof(cd_size_t));

	/* Sizes */
	cd_tmp_size->str_length = DEF_CDDA_STR_LEN;
	cd_tmp_size->chunk_blocks = app_data.cdda_readchkblks;

	/* Sizes depend on jitter */
	if (app_data.cdda_jitter_corr) {
		cd_tmp_size->olap_blocks = DEF_CDDA_OLAP_BLKS;
		cd_tmp_size->search_blocks = DEF_CDDA_SRCH_BLKS;
	}
	else {
		cd_tmp_size->olap_blocks = 0;
		cd_tmp_size->search_blocks = 0;
	}

	/* Chunk bytes */
	cd_tmp_size->chunk_bytes = cd_tmp_size->chunk_blocks * CDDA_BLKSZ;

	/* Buffer chunks */
	cd_tmp_size->buffer_chunks =
		(FRAME_PER_SEC * DEF_CDDA_BUFFER_SECS) /
		 cd_tmp_size->chunk_blocks;
	/* Check if we need to round up */
	if (((FRAME_PER_SEC * DEF_CDDA_BUFFER_SECS) %
	     cd_tmp_size->chunk_blocks) != 0)
		cd_tmp_size->buffer_chunks++;

	/* Buffer blocks */
	cd_tmp_size->buffer_blocks =
		cd_tmp_size->buffer_chunks * cd_tmp_size->chunk_blocks;

	/* Buffer bytes */
	cd_tmp_size->buffer_bytes =
		cd_tmp_size->buffer_chunks * cd_tmp_size->chunk_bytes;

	/* Overlap bytes */
	cd_tmp_size->olap_bytes = cd_tmp_size->olap_blocks * CDDA_BLKSZ;

	/* Search */
	cd_tmp_size->search_bytes = cd_tmp_size->search_blocks * CDDA_BLKSZ;

	/* How much shared memory do we need? */
	cd_tmp_size->size =
		sizeof(cd_size_t) + sizeof(cd_info_t) + sizeof(cd_buffer_t);

	/* Add memory for overlap */
	cd_tmp_size->size += (cd_tmp_size->search_bytes << 1);

	/* Add memory for data read */
	cd_tmp_size->size +=
		cd_tmp_size->chunk_bytes + (cd_tmp_size->olap_bytes << 1);

	/* Add memory for buffer */
	cd_tmp_size->size += cd_tmp_size->buffer_bytes;

	/* Now create the shared memory segment */
	shmid = shmget(SHMKEY + app_data.devnum, cd_tmp_size->size,
		       IPC_PERMS|IPC_CREAT|IPC_EXCL);
	if (shmid < 0) {
		/*
		 * If shared memory exists already, load it, remove it
		 * and create new one. This should not be necessary
		 * if the application previously exited correctly.
		 */
		if (errno == EEXIST) {
			shmid = shmget(SHMKEY + app_data.devnum,
				       cd_tmp_size->size, IPC_PERMS|IPC_EXCL);
			if (shmid < 0) {
				(void) sprintf(errbuf, "cdda_sysvipc_getshm: "
					    "shmget failed (errno=%d)", errno);
				CDDA_INFO(errbuf);
				DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
				MEM_FREE(cd_tmp_size);
				return FALSE;
			}

			if (shmctl(shmid, IPC_RMID, NULL) != 0) {
				(void) sprintf(errbuf, "cdda_sysvipc_getshm: "
					"shmctl IPC_RMID failed (errno=%d)",
					errno);
				CDDA_INFO(errbuf);
				DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
				MEM_FREE(cd_tmp_size);
				return FALSE;
			}

			shmid = shmget(SHMKEY + app_data.devnum,
				       cd_tmp_size->size,
				       IPC_PERMS|IPC_CREAT|IPC_EXCL);
			if (shmid < 0) {
				(void) sprintf(errbuf, "cdda_sysvipc_getshm: "
					    "shmget failed (errno=%d)", errno);
				CDDA_INFO(errbuf);
				DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
				MEM_FREE(cd_tmp_size);
				return FALSE;
			}
		}
		else {
			(void) sprintf(errbuf, "cdda_sysvipc_getshm: "
					"shmget failed (errno=%d)", errno);
			CDDA_INFO(errbuf);
			DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
			MEM_FREE(cd_tmp_size);
			return FALSE;
		}
	}

	/* Attach */
	if ((sysvipc_shmaddr = (void *) shmat(shmid, NULL, 0)) == NULL) {
		(void) sprintf(errbuf,
				"cdda_sysvipc_getshm: shmat failed (errno=%d)",
				errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		MEM_FREE(cd_tmp_size);
		return FALSE;
	}

	/* Now set our fields pointing into the shared memory */
	cd->cds = (cd_size_t *) sysvipc_shmaddr;

	/* Copy in sizes */
	(void) memcpy(sysvipc_shmaddr, cd_tmp_size, sizeof(cd_size_t));

	/* Info */
	cd->i = (cd_info_t *)(void *)
		((byte_t *) sysvipc_shmaddr + sizeof(cd_size_t));

	/* Buffer */
	cd->cdb = (cd_buffer_t *)(void *)
		((byte_t *) cd->i + sizeof(cd_info_t));

	/* Overlap */
	cd->cdb->olap = (byte_t *) ((byte_t *) cd->cdb + sizeof(cd_buffer_t));

	/* Data */
	cd->cdb->data = (byte_t *)
			((byte_t *) cd->cdb->olap +
				    (cd->cds->search_bytes << 1));

	/* Buffer */
	cd->cdb->b = (byte_t *)
		     ((byte_t *) cd->cdb->data +
				 cd->cds->chunk_bytes +
				 (cd->cds->olap_bytes << 1));

	/* Free temporary cd_size_t */
	MEM_FREE(cd_tmp_size);
	return TRUE;
}


/*
 * cdda_sysvipc_initshm
 *	Initialize fields of the cd_state_t structure. The cd_size_t structure
 *	has already been done in cdda_sysvipc_getshm(). The buffer we
 *	initialize prior to playing.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_sysvipc_initshm(curstat_t *s)
{
	/* General state */
	cd->i->state = CDSTAT_COMPLETED;
	cd->i->chroute = app_data.ch_route;
	cd->i->att = s->cdda_att;
	cd->i->outport = (int) app_data.outport;
	cd->i->vol_taper = app_data.vol_taper;
	cd->i->vol = 0;
	cd->i->vol_left = 100;
	cd->i->vol_right = 100;
	cd->i->jitter = (int) app_data.cdda_jitter_corr;
	cd->i->frm_played = 0;
	cd->i->trk_idx = 0;
	cd->i->trk_len = 0;
	cd->i->debug = app_data.debug;

	/* Start and ending LBAs for tracks */
	cd->i->start_lba = 0;
	cd->i->end_lba = 0;

	/* Initial frames per second statistic */
	cd->i->frm_per_sec = 0;

	/* Process ids */
	cd->i->reader = (thid_t) 0;
	cd->i->writer = (thid_t) 0;

	/* Reader and writer heartbeats */
	cd->i->reader_hb = 0;
	cd->i->writer_hb = 0;

	/* heartbeat timeouts */
	cd->i->reader_hb_timeout = app_data.hb_timeout;
	cd->i->writer_hb_timeout = app_data.hb_timeout * DEF_CDDA_BUFFER_SECS;
}


/*
 * cdda_sysvipc_cleanup
 *	Detaches shared memory, removes it and destroys semaphores.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_sysvipc_cleanup(void)
{
	semun_t	arg;

	if (cd == NULL)
		return;

	DBGPRN(DBG_GEN)(errfp, "\ncdda_sysvipc_cleanup: Cleaning up CDDA.\n");

	/* Detach from shared memory */
	if (cd->cds != NULL && shmdt((char *) cd->cds) < 0) {
		(void) sprintf(errbuf,
			    "cdda_sysvipc_cleanup: shmdt failed (errno=%d)",
			    errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
	}

	/* Deallocate cd_state_t structure */
	MEM_FREE(cd);
	cd = NULL;

	/* Remove shared memory */
	if (shmid >= 0 && shmctl(shmid, IPC_RMID, NULL) < 0) {
		(void) sprintf(errbuf,
			    "cdda_sysvipc_cleanup: shmctl IPC_RMID failed "
			    "(errno=%d)",
			    errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
	}
	shmid = -1;

	/* Destroy semaphores */
	arg.val = 0;
	if (semid >= 0 && SEMCTL(semid, 0, IPC_RMID, arg) < 0) {
		(void) sprintf(errbuf,
			    "cdda_sysvipc_cleanup: semctl IPC_RMID failed "
			    "(errno=%d)",
			    errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
	}
	semid = -1;
}


/*
 * cdda_sysvipc_init2
 *	Initialize shared memory and semaphores.
 *
 * Args:
 *	Pointer to the curstat_t structure.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdda_sysvipc_init2(curstat_t *s)
{
	/* Allocate memory */
	cd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (cd == NULL)
		/* Out of memory */
		return FALSE;

	(void) memset(cd, 0, sizeof(cd_state_t));

	/* Initialize semaphores */
	if (!cdda_sysvipc_getsem()) {
		cdda_sysvipc_cleanup();
		return FALSE;
	}

	/* Initialize shared memory */
	if (!cdda_sysvipc_getshm()) {
		cdda_sysvipc_cleanup();
		return FALSE;
	}

	/* Initialize fields */
	cdda_sysvipc_initshm(s);

	DBGPRN(DBG_GEN)(errfp, "\nSYSVIPC: CDDA initted.\n");
	return TRUE;
}


/*
 * cdda_sysvipc_capab
 *	Return configured CDDA capabilities
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported CDDA read and write features
 */
word32_t
cdda_sysvipc_capab(void)
{
	word32_t	(*rinit)(void),
			(*winit)(void);

	if (app_data.cdda_rdmethod == CDDA_RD_NONE ||
	    app_data.cdda_wrmethod == CDDA_WR_NONE)
		return 0;

	if (app_data.cdda_rdmethod < CDDA_RD_NONE ||
	    app_data.cdda_rdmethod >= CDDA_RD_METHODS ||
	    app_data.cdda_wrmethod < CDDA_WR_NONE ||
	    app_data.cdda_wrmethod >= CDDA_WR_METHODS) {
		CDDA_WARNING(app_data.str_cddainit_fail);
		DBGPRN(DBG_GEN)(errfp, "Warning: %s\n",
				  app_data.str_cddainit_fail);
		return 0;
	}

	/* Call readinit and writeinit functions to determine capability */
	rinit = cdda_rd_calltbl[app_data.cdda_rdmethod].readinit;
	winit = cdda_wr_calltbl[app_data.cdda_wrmethod].writeinit;

	if (rinit == NULL || winit == NULL) {
		CDDA_WARNING(app_data.str_cddainit_fail);
		DBGPRN(DBG_GEN)(errfp, "Warning: %s\n",
				  app_data.str_cddainit_fail);
		return 0;
	}

	return ((*rinit)() | (*winit)());
}


/*
 * cdda_sysvipc_preinit
 *	Early program startup initializations.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_preinit(void)
{
	/* Do nothing */
}


/*
 * cdda_sysvipc_init
 *	Initialize cdda subsystem.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
/*ARGSUSED*/
bool_t
cdda_sysvipc_init(curstat_t *s)
{
	if (app_data.cdda_rdmethod <= CDDA_RD_NONE ||
	    app_data.cdda_rdmethod >= CDDA_RD_METHODS ||
	    app_data.cdda_wrmethod <= CDDA_WR_NONE ||
	    app_data.cdda_wrmethod >= CDDA_WR_METHODS ||
	    cdda_rd_calltbl[app_data.cdda_rdmethod].readfunc == NULL ||
	    cdda_wr_calltbl[app_data.cdda_wrmethod].writefunc == NULL) {
		return FALSE;
	}

	/* Defer the rest of initialization to cdda_sysvipc_init2 */
	return TRUE;
}


/*
 * cdda_sysvipc_halt
 *	Halt cdda subsystem.
 *
 * Args:
 *	devp - Read device descriptor
 *	s    - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_halt(di_dev_t *devp, curstat_t *s)
{
	if (cd == NULL)
		return;

	/* Stop playback, if applicable */
	(void) cdda_sysvipc_stop(devp, s);

	/* Clean up IPC */
	cdda_sysvipc_cleanup();

	DBGPRN(DBG_GEN)(errfp, "\nSYSVIPC: CDDA halted.\n");
}


/*
 * cdda_sysvipc_play
 *	Start cdda play.
 *
 * Args:
 *	devp - Read device descriptor
 *	s -  Pointer to the curstat_t structure
 *	start_lba - Start logical block address
 *	end_lba   - End logical block address
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
cdda_sysvipc_play(di_dev_t *devp, curstat_t *s,
		  sword32_t start_lba, sword32_t end_lba)
{
	int	i;
	pid_t	cpid;
	semun_t	arg;
	bool_t	ret,
		(*rfunc)(di_dev_t *),
		(*wfunc)(curstat_t *);
	void	(*rdone)(bool_t),
		(*wdone)(bool_t);

	if (cd == NULL && !cdda_sysvipc_init2(s))
		return FALSE;

	rfunc = cdda_rd_calltbl[app_data.cdda_rdmethod].readfunc;
	wfunc = cdda_wr_calltbl[app_data.cdda_wrmethod].writefunc;
	rdone = cdda_rd_calltbl[app_data.cdda_rdmethod].readdone;
	wdone = cdda_wr_calltbl[app_data.cdda_wrmethod].writedone;

	if (start_lba >= end_lba) {
		(void) sprintf(errbuf,
			    "cdda_sysvipc_play: "
			    "Start LBA is greater than or equal to end LBA.");
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		return FALSE;
	}

	/* If not stopped, stop first */
	if (cd->i->state != CDSTAT_COMPLETED)
		(void) cdda_sysvipc_stop(devp, s);

	/* Set status */
	cd->i->state = CDSTAT_PLAYING;

	/* Where are we starting */
	cd->i->start_lba = start_lba;
	cd->i->end_lba = end_lba;

	/* Not finished reading */
	cd->i->cdda_done = 0;

	/* Buffer pointers */
	cd->cdb->occupied = 0;
	cd->cdb->nextin = 0;
	cd->cdb->nextout = 0;

	/* Clear message buffer */
	cd->i->msgbuf[0] = '\0';

	/* Room available */
	arg.val = 1;
	if (SEMCTL(semid, ROOM, SETVAL, arg) < 0) {
		(void) sprintf(errbuf,
			    "cdda_sysvipc_play: semctl SETVAL failed "
			    "(errno=%d)",
			    errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		cd->i->state = CDSTAT_COMPLETED;
		return FALSE;
	}

	/* No data available */
	arg.val = 0;
	if (SEMCTL(semid, DATA, SETVAL, arg) < 0) {
		(void) sprintf(errbuf,
			    "cdda_sysvipc_play: semctl SETVAL failed "
			    "(errno=%d)",
			    errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		cd->i->state = CDSTAT_COMPLETED;
		return FALSE;
	}

	/* Fork CDDA reader process */
	switch (cpid = FORK()) {
	case -1:
		(void) sprintf(errbuf,
			    "cdda_sysvipc_play: fork failed (reader) "
			    "(errno=%d)",
			    errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		cd->i->state = CDSTAT_COMPLETED;
		if (cd->i->writer != (thid_t) 0) {
			cdda_sysvipc_kill(cd->i->writer, SIGTERM);
			cd->i->writer = (thid_t) 0;
		}
		return FALSE;

	case 0:
		/* Child */

		/* Close any unneeded file descriptors */
		for (i = 3; i < 10; i++) {
			if (i != devp->fd)
				(void) close(i);
		}

		/* Increase reader process scheduling priority if specified */
		if (app_data.cdda_sched & CDDA_RDPRI)
			cdda_sysvipc_highpri("Reader process");

		/* Store reader pid */
		cd->i->reader = (thid_t)(unsigned long) getpid();

		/* Signal handling */
		(void) util_signal(SIGINT, SIG_IGN);
		(void) util_signal(SIGQUIT, SIG_IGN);
		(void) util_signal(SIGPIPE, SIG_IGN);
		(void) util_signal(SIGTERM, cdda_sysvipc_rsig);

		/* Call read function */
		ret = (*rfunc)(devp);

		/* Call cleanup function */
		(*rdone)((bool_t) !ret);

		_exit(ret ? 0 : 1);
		/*NOTREACHED*/

	default:
		/* Parent */
		DBGPRN(DBG_GEN)(errfp, "\nStarted reader process pid=%d\n",
				(int) cpid);
		break;
	}

	/* Fork CDDA writer process */
	switch (cpid = FORK()) {
	case -1:
		(void) sprintf(errbuf,
			    "cdda_sysvipc_play: fork failed (writer) "
			    "(errno=%d)",
			    errno);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		cd->i->state = CDSTAT_COMPLETED;
		if (cd->i->reader != (thid_t) 0) {
			cdda_sysvipc_kill(cd->i->reader, SIGTERM);
			cd->i->reader = (thid_t) 0;
		}
		return FALSE;

	case 0:
		/* Child */

		/* Close any unneeded file descriptors */
		for (i = 3; i < 10; i++)
			(void) close(i);

		/* Increase writer process scheduling priority if specified */
		if (app_data.cdda_sched & CDDA_WRPRI)
			cdda_sysvipc_highpri("Writer process");

#ifndef HAS_EUID
		/* Set to original uid/gid */
		util_set_ougid();
#endif

		/* Store writer pid */
		cd->i->writer = (thid_t)(unsigned long) getpid();

		/* Signal handling */
		(void) util_signal(SIGINT, SIG_IGN);
		(void) util_signal(SIGQUIT, SIG_IGN);
		(void) util_signal(SIGPIPE, SIG_IGN);
		(void) util_signal(SIGTERM, cdda_sysvipc_wsig);

		/* Call write function */
		ret = (*wfunc)(s);

		/* Call cleanup function */
		(*wdone)((bool_t) !ret);

		_exit(ret ? 0 : 1);
		/*NOTREACHED*/

	default:
		/* Parent */
		DBGPRN(DBG_GEN)(errfp, "\nStarted writer process pid=%d\n",
				(int) cpid);
		break;
	}

	return TRUE;
}


/*
 * cdda_sysvipc_pause_resume
 *	Pause CDDA playback.
 *
 * Args:
 *	devp   - Read device descriptor
 *	s      - Pointer to the curstat_t structure
 *	resume - Whether to resume playback
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
cdda_sysvipc_pause_resume(di_dev_t *devp, curstat_t *s, bool_t resume)
{
	if (cd == NULL && !cdda_sysvipc_init2(s))
		return FALSE;

	if (cd->i->writer != (thid_t) 0) {
		if (resume)
			cd->i->state = CDSTAT_PLAYING;
		else
			cd->i->state = CDSTAT_PAUSED;

		/* Skip heartbeat checking */
		skip_hbchk = CDDA_HB_SKIP;
	}
	return TRUE;
}


/*
 * cdda_sysvipc_stop
 *	Stop CDDA playback.
 *
 * Args:
 *	devp - Read device descriptor
 *	s    - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
cdda_sysvipc_stop(di_dev_t *devp, curstat_t *s)
{
	int		ret;
	pid_t		cpid;
	waitret_t	wstat;

	if (cd == NULL && !cdda_sysvipc_init2(s))
		return FALSE;

	/* Set status */
	cd->i->state = CDSTAT_COMPLETED;

	if (cd->i->writer != (thid_t) 0) {
		cpid = (pid_t)(unsigned long) cd->i->writer;

		/* Wait for writer, blocking */
		DBGPRN(DBG_GEN)(errfp,
			"\ncdda_sysvipc_stop: Waiting for writer pid=%d\n",
			(int) cpid);

		ret = util_waitchild(cpid, app_data.hb_timeout,
				     NULL, 0, FALSE, &wstat);
		if (ret < 0) {
			DBGPRN(DBG_GEN)(errfp,
				"waitpid on writer failed (errno=%d)\n",
				errno
			);
		}
		else if (WIFEXITED(wstat)) {
			ret = WEXITSTATUS(wstat);
			DBGPRN(DBG_GEN)(errfp, "Writer exited, status %d\n",
					ret);

			/* Display message, if any */
			if (ret != 0 && cd->i->msgbuf[0] != '\0')
				CDDA_INFO(cd->i->msgbuf);
		}
		else if (WIFSIGNALED(wstat)) {
			DBGPRN(DBG_GEN)(errfp, "Writer killed, signal %d\n",
					WTERMSIG(wstat));
		}

		cd->i->writer = (thid_t) 0;
	}

	if (cd->i->reader != (thid_t) 0) {
		cpid = (pid_t)(unsigned long) cd->i->reader;

		/* Wait for reader, blocking */
		DBGPRN(DBG_GEN)(errfp,
			"\ncdda_sysvipc_stop: Waiting for reader pid=%d\n",
			(int) cpid);

		ret = util_waitchild(cpid, app_data.hb_timeout,
				     NULL, 0, FALSE, &wstat);
		if (ret < 0) {
			DBGPRN(DBG_GEN)(errfp,
				"waitpid on writer failed (errno=%d)\n",
				errno
			);
		}
		else if (WIFEXITED(wstat)) {
			ret = WEXITSTATUS(wstat);
			DBGPRN(DBG_GEN)(errfp, "Reader exited, status %d\n",
				        ret);

			/* Display message, if any */
			if (ret != 0 && cd->i->msgbuf[0] != '\0')
				CDDA_INFO(cd->i->msgbuf);
		}
		else if (WIFSIGNALED(wstat)) {
			DBGPRN(DBG_GEN)(errfp, "Reader killed, signal %d\n",
				        WTERMSIG(wstat));
		}

		cd->i->reader = (thid_t) 0;
	}

	/* Reset states */
	cdda_sysvipc_initshm(s);

	return TRUE;
}


/*
 * cdda_sysvipc_vol
 *	Change volume setting.
 *
 * Args:
 *	devp  - Read device descriptor
 *	s     - Pointer to the curstat_t structure
 *	vol   - Desired volume level
 *	query - Whether querying or setting the volume
 *
 * Return:
 *	The volume setting, or -1 on failure.
 */
/*ARGSUSED*/
int
cdda_sysvipc_vol(di_dev_t *devp, curstat_t *s, int vol, bool_t query)
{
	if (cd == NULL && !cdda_sysvipc_init2(s))
		return -1;

	if (query) {
		s->level_left = (byte_t) cd->i->vol_left;
		s->level_right = (byte_t) cd->i->vol_right;
		return (cd->i->vol);
	}

	cd->i->vol_taper = app_data.vol_taper;
	cd->i->vol = vol;
	cd->i->vol_left = (int) s->level_left;
	cd->i->vol_right = (int) s->level_right;

	return (vol);
}


/*
 * cdda_sysvipc_chroute
 *	Change channel routing setting.
 *
 * Args:
 *	devp - Read device descriptor
 *	s    - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
cdda_sysvipc_chroute(di_dev_t *devp, curstat_t *s)
{
	if (cd != NULL) {
		cd->i->chroute = app_data.ch_route;
		return TRUE;
	}
	else
		return FALSE;
}


/*
 * cdda_sysvipc_att
 *	Change level attenuator setting.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_att(curstat_t *s)
{
	if (cd != NULL)
		cd->i->att = (int) s->cdda_att;
}


/*
 * cdda_sysvipc_outport
 *	Change output port setting.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_outport(void)
{
	if (cd != NULL)
		cd->i->outport = (int) app_data.outport;
}


/*
 * cdda_sysvipc_getstatus
 *	Get CDDA playback status.
 *
 * Args:
 *	devp - Read device descriptor
 *	s    - Pointer to curstat_t structure
 *	sp - CDDA status return structure
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
cdda_sysvipc_getstatus(di_dev_t *devp, curstat_t *s, cdstat_t *sp)
{
	int		i;
	time_t		now;
	bool_t		reader_fail,
			writer_fail;
	static int	hbint = 0,
			hbcnt = 0;

	if (cd == NULL && !cdda_sysvipc_init2(s))
		return FALSE;

	/* Current playback status */
	sp->status = cd->i->state;
	if (sp->status == CDSTAT_COMPLETED)
		(void) cdda_sysvipc_stop(devp, s);

	/* Current playback location */
	sp->abs_addr.addr = cd->i->start_lba + cd->i->frm_played;

	util_blktomsf(
		sp->abs_addr.addr,
		&sp->abs_addr.min,
		&sp->abs_addr.sec,
		&sp->abs_addr.frame,
		MSF_OFFSET
	);

	i = cd->i->trk_idx;
	if (sp->abs_addr.addr >= s->trkinfo[i].addr &&
	    sp->abs_addr.addr < s->trkinfo[i+1].addr) {
			sp->track = s->trkinfo[i].trkno;

			sp->rel_addr.addr =
				sp->abs_addr.addr - s->trkinfo[i].addr;

			util_blktomsf(
				sp->rel_addr.addr,
				&sp->rel_addr.min,
				&sp->rel_addr.sec,
				&sp->rel_addr.frame,
				0
			);
	}
	else for (i = 0; i < (int) s->tot_trks; i++) {
		if (sp->abs_addr.addr >= s->trkinfo[i].addr &&
		    sp->abs_addr.addr < s->trkinfo[i+1].addr) {
			sp->track = s->trkinfo[i].trkno;

			sp->rel_addr.addr =
				sp->abs_addr.addr - s->trkinfo[i].addr;

			util_blktomsf(
				sp->rel_addr.addr,
				&sp->rel_addr.min,
				&sp->rel_addr.sec,
				&sp->rel_addr.frame,
				0
			);
			break;
		}
	}

	sp->index = 1;	/* Index number not supported in this mode */

	/* Current volume and balance */
	sp->level = (byte_t) cd->i->vol;
	sp->level_left = (byte_t) cd->i->vol_left;
	sp->level_right = (byte_t) cd->i->vol_right;

	sp->tot_frm = cd->i->end_lba - cd->i->start_lba + 1;
	sp->frm_played = cd->i->frm_played;
	sp->frm_per_sec = cd->i->frm_per_sec;

	/* Initialize heartbeat checking interval */
	if (hbint == 0)
		hbcnt = hbint = (1000 / app_data.stat_interval) + 1;

	/* Check reader and writer heartbeats */
	if (sp->status == CDSTAT_PLAYING && --hbcnt == 0 &&
	    cd->i->reader != (thid_t) 0 && cd->i->writer != (thid_t) 0 &&
	    cd->i->reader_hb != 0 && cd->i->writer_hb != 0) {
		hbcnt = hbint;
		now = time(NULL);

		if (skip_hbchk > 0) {
		    /* Skip heartbeat checking for a few iterations after
		     * resuming from pause, to let the reader and writer
		     * threads to catch up.
		     */
		    reader_fail = writer_fail = FALSE;
		    skip_hbchk--;
		}
		else {
		    reader_fail = (bool_t)
			((now - cd->i->reader_hb) > cd->i->reader_hb_timeout);
		    writer_fail = (bool_t)
			((now - cd->i->writer_hb) > cd->i->writer_hb_timeout);
		}

		if (reader_fail || writer_fail) {
			/* Reader or writer is hung or died */
			cdda_sysvipc_kill(cd->i->reader, SIGTERM);
			cdda_sysvipc_kill(cd->i->writer, SIGTERM);

			(void) cdda_sysvipc_stop(devp, s);

			(void) sprintf(errbuf,
				       "CDDA %s thread failure!",
				       writer_fail ? "writer" : "reader");
			CDDA_INFO(errbuf);
			DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		}
	}

	return TRUE;
}


/*
 * cdda_sysvipc_debug
 *	Debug level change notification function
 *
 * Args:
 *	lev - New debug level
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_debug(word32_t lev)
{
	if (cd != NULL)
		cd->i->debug = lev;
}



/*
 * cdda_sysvipc_info
 *	Append CDDA read and write method information to supplied string.
 *
 * Args:
 *	str - The string to append to.
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_info(char *str)
{
	void	(*rinfo)(char *),
		(*winfo)(char *);

	(void) strcat(str, "SYSV IPC\n");

	(void) strcat(str, "    Extract:  ");
	if (app_data.cdda_rdmethod <= CDDA_RD_NONE ||
	    app_data.cdda_rdmethod >= CDDA_RD_METHODS) {
		(void) strcat(str, "not configured\n");	
	}
	else {
		rinfo = cdda_rd_calltbl[app_data.cdda_rdmethod].readinfo;
		if (rinfo == NULL)
			(void) strcat(str, "not configured\n");	
		else
			(*rinfo)(str);
	}

	(void) strcat(str, "    Playback: ");
	if (app_data.cdda_wrmethod <= CDDA_WR_NONE ||
	    app_data.cdda_wrmethod >= CDDA_WR_METHODS) {
		(void) strcat(str, "not configured\n");	
	}
	else {
		winfo = cdda_wr_calltbl[app_data.cdda_wrmethod].writeinfo;
		if (winfo == NULL)
			(void) strcat(str, "not configured\n");	
		else
			(*winfo)(str);
	}
}


/*
 * cdda_sysvipc_initipc
 *	Retrieves shared memory and semaphores. Sets up our pointers
 *	from cd into shared memory.  Used by the reader/writer child
 *	processes.
 *
 * Args:
 *	cd - Pointer to the cd_state_t structure to be filled in
 *
 * Return:
 *	The IPC semaphore ID, or -1 if failed.
 */
int
cdda_sysvipc_initipc(cd_state_t *cdp)
{
	int	id;

	if (sysvipc_shmaddr == NULL) {
		DBGPRN(DBG_GEN)(errfp,
			"cdda_sysvipc_initipc: No shared memory!\n");
		return -1;
	}

	/* Now set our fields pointing into the shared memory */
	cdp->cds = (cd_size_t *) sysvipc_shmaddr;

	/* Info */
	cdp->i = (cd_info_t *)(void *)
		((byte_t *) sysvipc_shmaddr + sizeof(cd_size_t));

	/* Buffer */
	cdp->cdb = (cd_buffer_t *)(void *)
		((byte_t *) cd->i + sizeof(cd_info_t));

	/* Overlap */
	cdp->cdb->olap = (byte_t *) ((byte_t *) cd->cdb + sizeof(cd_buffer_t));

	/* Data */
	cdp->cdb->data = (byte_t *)
			 ((byte_t *) cd->cdb->olap +
				    (cd->cds->search_bytes << 1));

	/* Buffer */
	cdp->cdb->b = (byte_t *)
		      ((byte_t *) cd->cdb->data + cd->cds->chunk_bytes +
				 (cd->cds->olap_bytes << 1));

	/* Semaphores */
	if ((id = semget(SEMKEY + app_data.devnum, 3,
			 IPC_PERMS|IPC_EXCL)) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"cdda_sysvipc_initipc: semget failed: errno=%d\n",
			errno);
	}

	return (id);
}


/*
 * cdda_sysvipc_waitsem
 *	Waits for a semaphore, decrementing its value.
 *
 * Args:
 *	semid - Semaphore id
 *	num   - Semaphore to wait on
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_waitsem(int semid, int num)
{
	struct sembuf	p_buf;

	p_buf.sem_num = (short) num;
	p_buf.sem_op = -1;
	p_buf.sem_flg = 0;

	while (semop(semid, &p_buf, 1) < 0) {
		if (errno == ERANGE) {
			semun_t	arg;

			arg.val = 0;
			arg.buf = NULL;
			arg.array = NULL;

			(void) semctl(semid, num, SETVAL, arg);
		}
		else {
			DBGPRN(DBG_GEN)(errfp, "cdda_sysvipc_waitsem: "
			    "semop failed on %d (errno=%d)\n",
			    num, errno);
			break;
		}
	}
}


/*
 * cdda_sysvipc_postsem
 *	Release a semaphore, incrementing its value.
 *
 * Arguments:
 *	semid - Semaphore id
 *	num   - Semaphore to increment
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_postsem(int semid, int num)
{
	struct sembuf	v_buf;

	v_buf.sem_num = (short) num;
	v_buf.sem_op = 1;
	v_buf.sem_flg = 0;

	while (semop(semid, &v_buf, 1) < 0) {
		if (errno == ERANGE) {
			semun_t	arg;

			arg.val = 0;
			arg.buf = NULL;
			arg.array = NULL;

			(void) semctl(semid, num, SETVAL, arg);
		}
		else {
			DBGPRN(DBG_GEN)(errfp, "cdda_sysvipc_postsem: "
			    "semop failed on %d (errno=%d)\n",
			    num, errno);
			break;
		}
	}
}


/*
 * cdda_sysvipc_yield
 *	Let other processes run.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_yield(void)
{
	/* Just use the delay function go to sleep and let the process
	 * scheduler take care of it.
	 */
	util_delayms(1000);
}


/*
 * cdda_sysvipc_kill
 *	Terminate a specified process.
 *
 * Args:
 *	id - The process or thread id
 *	sig - The signal
 *
 * Return:
 *	Nothing.
 */
void
cdda_sysvipc_kill(thid_t id, int sig)
{
	(void) kill((pid_t)(unsigned long) id, sig);
}


#endif	/* CDDA_SYSVIPC */

