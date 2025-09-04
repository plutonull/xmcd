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
 *	parallel reader/writer threads implemented using POSIX 1003.1c
 *	threads.
 */
#ifndef lint
static char *_pthr_c_ident_ = "@(#)pthr.c	7.54 04/03/17";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"


#ifdef CDDA_PTHREADS

#include "cdda_d/pthr.h"

/*
 * Platform-specific configuration for thread scheduling control
 */
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && \
    (defined(_POSIX_PRIORITY_SCHEDULING) || \
     defined(PTHREAD_SCHED_MAX_PRIORITY))
/* If these are defined in <unistd.h> and <pthread.h> then the
 * system fully supports POSIX priority scheduling control for
 * threads.
 */
#define USE_POSIX_THREAD_SCHED
#endif


extern appdata_t	app_data;
extern FILE		*errfp;
extern cdda_client_t	*cdda_clinfo;

extern cdda_rd_tbl_t	cdda_rd_calltbl[];
extern cdda_wr_tbl_t	cdda_wr_calltbl[];

/* Semaphore structure */
typedef struct {
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	int		v;
} pthr_sem_t;

#define PTHR_NSEMS	3			/* Number of semaphores */

STATIC void		*pthr_shmaddr;		/* Shared memory pointer */
STATIC int		skip_hbchk = 0;		/* Skip heartbeat checking */
STATIC cd_state_t	*cd = NULL;		/* CD state pointer */
STATIC char		errbuf[ERR_BUF_SZ];	/* Error message buffer */
STATIC pthr_sem_t	pthr_sem[PTHR_NSEMS];	/* LOCK, ROOM and DATA */
STATIC void		(*opipe)(int);		/* Saved SIGPIPE handler */


/* 
 * cdda_pthr_highpri
 *	Increase the calling thread's scheduling priority
 *
 * Args:
 *	name - thread name string
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_pthr_highpri(char *name)
{
#ifdef USE_POSIX_THREAD_SCHED
	int			ret,
				minpri,
				maxpri;
	pthread_t		tid;
	struct sched_param	scparm;

#ifdef HAS_EUID
	uid_t			savuid = geteuid();

	if (util_seteuid(0) < 0 || geteuid() != 0) {
		DBGPRN(DBG_GEN)(errfp,
			"cdda_pthr_setpri: Cannot change scheduling: "
			"Insufficient privilege.\n");
		(void) util_seteuid(savuid);
		return;
	}
#endif

	tid = pthread_self();

	/* Set priority to 80% between min and max */
#ifdef PTHREAD_SCHED_MAX_PRIORITY
	minpri = PTHREAD_SCHED_MIN_PRIORITY;
	maxpri = PTHREAD_SCHED_MAX_PRIORITY;
#else
	minpri = sched_get_priority_min(SCHED_RR);
	maxpri = sched_get_priority_max(SCHED_RR);
#endif
	if (minpri < 0 || maxpri < 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_highpri: "
				"Failed getting min/max priority values: "
				"errno=%d\n", errno);
#ifdef HAS_EUID
		(void) util_seteuid(savuid);
#endif
		return;
	}

	scparm.sched_priority = ((maxpri - minpri) * 8 / 10) + minpri;

	if ((ret = pthread_setschedparam(tid, SCHED_RR, &scparm)) < 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_highpri: "
				"pthread_setschedparam failed: "
				"ret=%d\n", ret);
#ifdef HAS_EUID
		(void) util_seteuid(savuid);
#endif
		return;
	}

	DBGPRN(DBG_GEN)(errfp,
		    "\n%s running POSIX SCHED_RR scheduling (thread id=%d)\n"
		    "Priority=%d (range: %d,%d)\n",
		    name, (int) tid, scparm.sched_priority, minpri, maxpri);

#ifdef HAS_EUID
	(void) util_seteuid(savuid);
#endif
#endif	/* USE_POSIX_THREAD_SCHED */
}


/*
 * cdda_pthr_atfork
 *	Child process handler function for fork(2), registered and called
 *	from pthread_atfork(3), so that the child process can run without
 *	being blocked in mutexes.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_pthr_atfork(void)
{
	int	i,
		ret;

	/* Initialize all mutexes to unlocked state */
	for (i = 0; i < PTHR_NSEMS; i++) {
		ret = pthread_mutex_init(&pthr_sem[i].mutex, NULL);
		if (ret != 0) {
			(void) sprintf(errbuf,
				"cdda_pthr_atfork: pthread_mutex_init failed "
				"(sem %d, ret=%d)",
				i, ret);
			CDDA_INFO(errbuf);
			DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		}
	}
}


/*
 * cdda_pthr_getsem
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
cdda_pthr_getsem(void)
{
	int	i,
		ret;

	for (i = 0; i < PTHR_NSEMS; i++) {
		pthr_sem[i].v = 0;

		ret = pthread_mutex_init(&pthr_sem[i].mutex, NULL);
		if (ret != 0) {
			(void) sprintf(errbuf,
				"cdda_pthr_getsem: pthread_mutex_init failed "
				"(sem %d, ret=%d)",
				i, ret);
			CDDA_INFO(errbuf);
			DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
			return FALSE;
		}

		ret = pthread_cond_init(&pthr_sem[i].cond, NULL);
		if (ret != 0) {
			(void) sprintf(errbuf,
				"cdda_pthr_getsem: pthread_cond_init failed "
				"(sem %d, ret=%d)",
				i, ret);
			CDDA_INFO(errbuf);
			DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
			return FALSE;
		}
	}

	/* Make lock available */
	pthr_sem[LOCK].v = 1;

	return TRUE;
}


/*
 * cdda_pthr_getshm
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
cdda_pthr_getshm(void)
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

	/* Now create the pthread shared memory: just malloc it */
	pthr_shmaddr = (byte_t *) MEM_ALLOC("cdda_shm", cd_tmp_size->size);
	if (pthr_shmaddr == NULL) {
		(void) sprintf(errbuf, "cdda_pthr_getshm: Out of memory.");
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		MEM_FREE(cd_tmp_size);
		return FALSE;
	}

	/* Now set our fields pointing into the shared memory */
	cd->cds = (cd_size_t *) pthr_shmaddr;

	/* Copy in sizes */
	(void) memcpy(pthr_shmaddr, cd_tmp_size, sizeof(cd_size_t));

	/* Info */
	cd->i = (cd_info_t *)(void *)
		((byte_t *) pthr_shmaddr + sizeof(cd_size_t));

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
 * cdda_pthr_initshm
 *	Initialize fields of the cd_state_t structure. The cd_size_t structure
 *	has already been done in cdda_pthr_getshm(). The buffer we initialize
 *	prior to playing.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_pthr_initshm(curstat_t *s)
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
 * cdda_pthr_wdone
 *	Write thread cleanup.  This is called when the write thread is
 *	forcibly canceled.
 *
 * Args:
 *	p - Unused
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cdda_pthr_wdone(void *p)
{
	void	(*wdone)(bool_t);

	wdone = cdda_wr_calltbl[app_data.cdda_wrmethod].writedone;

	/* Call cleanup function */
	(*wdone)(FALSE);
}


/*
 * cdda_pthr_rdone
 *	Read thread cleanup.  This is called when the read thread is
 *	forcibly canceled.
 *
 * Args:
 *	p - Unused
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cdda_pthr_rdone(void *p)
{
	void	(*rdone)(bool_t);

	rdone = cdda_rd_calltbl[app_data.cdda_rdmethod].readdone;

	/* Call cleanup function */
	(*rdone)(FALSE);
}


/*
 * cdda_pthr_unblock
 *	Unblock the other threads.
 *
 * Args:
 *	p - Unused
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cdda_pthr_unblock(void *p)
{
	int	i;

	/*
	 * There was a problem, so we came here. This thread
	 * will no longer be part of the game, it was the only competing
	 * thread for the resources, which it still may have locked, so
	 * unconditionally give them up.
	 */
	for (i = 0; i < PTHR_NSEMS; i++)
		cdda_pthr_postsem(0, i);
}


/*
 * cdda_pthr_writer
 *	Writer thread function
 *
 * Args:
 *	arg - Pointer to the curstat_t structure.
 *
 * Return:
 *	Always returns NULL.
 */
STATIC void *
cdda_pthr_writer(void *arg)
{
	curstat_t	*s = (curstat_t *) arg;
	int		ret;
	bool_t		status,
			(*wfunc)(curstat_t *);
	void		(*wdone)(bool_t);
#ifndef __VMS
	sigset_t	sigs;
#endif

	/* Store writer thread id */
	cd->i->writer = (thid_t) pthread_self();

	DBGPRN(DBG_GEN)(errfp, "\nStarted writer thread id=%d\n",
			(int) cd->i->writer);

#ifndef __VMS
	/* Block some signals in this thread */
	(void) sigemptyset(&sigs);
	(void) sigaddset(&sigs, SIGPIPE);
	(void) sigaddset(&sigs, SIGALRM);
#ifdef USE_SIGTHREADMASK
	if ((ret = sigthreadmask(SIG_BLOCK, &sigs, NULL)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "sigthreadmask failed: "
				"(writer ret=%d)\n", ret);
	}
#else
	if ((ret = pthread_sigmask(SIG_BLOCK, &sigs, NULL)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "pthread_sigmask failed: "
				"(writer ret=%d)\n", ret);
	}
#endif
#endif	/* __VMS */

	wfunc = cdda_wr_calltbl[app_data.cdda_wrmethod].writefunc;
	wdone = cdda_wr_calltbl[app_data.cdda_wrmethod].writedone;

	/* Increase writer thread scheduling priority if specified */
	if (app_data.cdda_sched & CDDA_WRPRI)
		cdda_pthr_highpri("Writer thread");

	pthread_cleanup_push(cdda_pthr_wdone, NULL);
	pthread_cleanup_push(cdda_pthr_unblock, NULL);

	/* Call write function */
	status = (*wfunc)(s);

	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);

	/* Call cleanup function */
	(*wdone)((bool_t) !status);

	/* End of write thread */
	pthread_exit((void *) (status ? 0 : 1));

	/* Should never get here */
	return NULL;
}


/*
 * cdda_pthr_reader
 *	Reader thread function
 *
 * Args:
 *	arg - Pointer to the di_dev_t structure.
 *
 * Return:
 *	Always returns NULL.
 */
STATIC void *
cdda_pthr_reader(void *arg)
{
	di_dev_t	*devp = (di_dev_t *) arg;
	int		ret;
	bool_t		status,
			(*rfunc)(di_dev_t *);
	void		(*rdone)(bool_t);
#ifndef __VMS
	sigset_t	sigs;
#endif

	/* Store reader thread id */
	cd->i->reader = (thid_t) pthread_self();

	DBGPRN(DBG_GEN)(errfp, "\nStarted reader thread id=%d\n",
			(int) cd->i->reader);

#ifndef __VMS
	/* Block some signals in this thread */
	(void) sigemptyset(&sigs);
	(void) sigaddset(&sigs, SIGPIPE);
	(void) sigaddset(&sigs, SIGALRM);
#ifdef USE_SIGTHREADMASK
	if ((ret = sigthreadmask(SIG_BLOCK, &sigs, NULL)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "sigthreadmask failed: "
				"(reader ret=%d)\n", ret);
	}
#else
	if ((ret = pthread_sigmask(SIG_BLOCK, &sigs, NULL)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "pthread_sigmask failed: "
				"(reader ret=%d)\n", ret);
	}
#endif
#endif	/* __VMS */

	rfunc = cdda_rd_calltbl[app_data.cdda_rdmethod].readfunc;
	rdone = cdda_rd_calltbl[app_data.cdda_rdmethod].readdone;

	/* Increase reader thread scheduling priority if specified */
	if (app_data.cdda_sched & CDDA_RDPRI)
		cdda_pthr_highpri("Reader thread");

	pthread_cleanup_push(cdda_pthr_rdone, NULL);
	pthread_cleanup_push(cdda_pthr_unblock, NULL);

	/* Call read function */
	status = (*rfunc)(devp);

	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);

	/* Call cleanup function */
	(*rdone)((bool_t) !status);

	/* End of read thread */
	pthread_exit((void *) (status ? 0 : 1));

	/* Should never get here */
	return NULL;
}


/*
 * cdda_pthr_cleanup
 *	Detaches shared memory, removes it and destroys semaphores.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_pthr_cleanup(void)
{
	int	i,
		ret;

	if (cd == NULL)
		return;

	DBGPRN(DBG_GEN)(errfp, "\ncdda_pthr_cleanup: Cleaning up CDDA.\n");

	/* Deallocate cd_state_t structure */
	MEM_FREE(cd);
	cd = NULL;

	/* Deallocate shared memory */
	if (pthr_shmaddr != NULL) {
		MEM_FREE(pthr_shmaddr);
		pthr_shmaddr = NULL;
	}

	/* Destroy semaphores */
	for (i = 0; i < PTHR_NSEMS; i++) {
		if ((ret = pthread_mutex_destroy(&pthr_sem[i].mutex)) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_cleanup: "
			    "pthread_mutex_destroy failed "
			    "(sem %d, ret=%d)\n",
			    i, ret);
		}
		if ((ret = pthread_cond_destroy(&pthr_sem[i].cond)) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_cleanup: "
			    "pthread_cond_destroy failed "
			    "(sem %d, ret=%d)\n",
			    i, ret);
		}
	}
}


/*
 * cdda_pthr_init2
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
cdda_pthr_init2(curstat_t *s)
{
	/* Allocate memory */
	cd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (cd == NULL)
		/* Out of memory */
		return FALSE;

	(void) memset(cd, 0, sizeof(cd_state_t));

	/* Initialize semaphores */
	if (!cdda_pthr_getsem()) {
		cdda_pthr_cleanup();
		return FALSE;
	}

	/* Initialize shared memory */
	if (!cdda_pthr_getshm()) {
		cdda_pthr_cleanup();
		return FALSE;
	}

	/* Initialize fields */
	cdda_pthr_initshm(s);

	DBGPRN(DBG_GEN)(errfp, "\nPTHREADS: CDDA initted.\n");
	return TRUE;
}


/*
 * cdda_pthr_capab
 *	Return configured CDDA capabilities
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported CDDA read and write features
 */
word32_t
cdda_pthr_capab(void)
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
 * cdda_pthr_init
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
cdda_pthr_init(curstat_t *s)
{
	if (app_data.cdda_rdmethod <= CDDA_RD_NONE ||
	    app_data.cdda_rdmethod >= CDDA_RD_METHODS ||
	    app_data.cdda_wrmethod <= CDDA_WR_NONE ||
	    app_data.cdda_wrmethod >= CDDA_WR_METHODS ||
	    cdda_rd_calltbl[app_data.cdda_rdmethod].readfunc == NULL ||
	    cdda_wr_calltbl[app_data.cdda_wrmethod].writefunc == NULL) {
		return FALSE;
	}

	/* Defer the rest of initialization to cdda_pthr_init2 */
	return TRUE;
}


/*
 * cdda_pthr_halt
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
cdda_pthr_halt(di_dev_t *devp, curstat_t *s)
{
	if (cd == NULL)
		return;

	/* Stop playback, if applicable */
	(void) cdda_pthr_stop(devp, s);

	/* Clean up IPC */
	cdda_pthr_cleanup();

	DBGPRN(DBG_GEN)(errfp, "\nPTHREADS: CDDA halted.\n");
}


/*
 * cdda_pthr_play
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
cdda_pthr_play(di_dev_t *devp, curstat_t *s,
		  sword32_t start_lba, sword32_t end_lba)
{
	int		ret;
	pthread_t	tid;

	if (cd == NULL && !cdda_pthr_init2(s))
		return FALSE;

	if (start_lba >= end_lba) {
		(void) sprintf(errbuf,
			    "cdda_pthr_play: "
			    "Start LBA is greater than or equal to end LBA.");
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		return FALSE;
	}

	/* If not stopped, stop first */
	if (cd->i->state != CDSTAT_COMPLETED)
		(void) cdda_pthr_stop(devp, s);

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
	pthr_sem[ROOM].v = 1;

	/* No data available */
	pthr_sem[DATA].v = 0;

#ifndef __VMS	/* VMS has no real fork, so this does not apply */
	/* Register fork handler */
	if ((ret = pthread_atfork(NULL, NULL, cdda_pthr_atfork)) != 0) {
		(void) sprintf(errbuf,
			    "cdda_pthr_play: pthread_atfork failed (ret=%d)",
			    ret);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		cd->i->state = CDSTAT_COMPLETED;
		return FALSE;
	}
#endif

	/* Ignore SIGPIPE */
	opipe = util_signal(SIGPIPE, SIG_IGN);

	/* Start CDDA reader thread */
	if ((ret = pthread_create(&tid, NULL,
				  cdda_pthr_reader, (void *) devp)) != 0) {
		(void) sprintf(errbuf,
			    "cdda_pthr_play: pthread_create failed "
			    "(reader ret=%d)",
			    ret);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		cd->i->state = CDSTAT_COMPLETED;
		if (cd->i->writer != (thid_t) 0) {
			cdda_pthr_kill(cd->i->writer, SIGTERM);
			cd->i->writer = (thid_t) 0;
		}

		/* Restore saved SIGPIPE handler */
		(void) util_signal(SIGPIPE, opipe);
		return FALSE;
	}

	/* Start CDDA writer thread */
	if ((ret = pthread_create(&tid, NULL,
				  cdda_pthr_writer, (void *) s)) != 0) {
		(void) sprintf(errbuf,
			    "cdda_pthr_play: pthread_create failed "
			    "(writer ret=%d)",
			    ret);
		CDDA_INFO(errbuf);
		DBGPRN(DBG_GEN)(errfp, "%s\n", errbuf);
		cd->i->state = CDSTAT_COMPLETED;
		if (cd->i->reader != (thid_t) 0) {
			cdda_pthr_kill(cd->i->reader, SIGTERM);
			cd->i->reader = (thid_t) 0;
		}

		/* Restore saved SIGPIPE handler */
		(void) util_signal(SIGPIPE, opipe);
		return FALSE;
	}

	return TRUE;
}


/*
 * cdda_pthr_pause_resume
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
cdda_pthr_pause_resume(di_dev_t *devp, curstat_t *s, bool_t resume)
{
	if (cd == NULL && !cdda_pthr_init2(s))
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
 * cdda_pthr_stop
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
cdda_pthr_stop(di_dev_t *devp, curstat_t *s)
{
	void		*status;
	int		ret;
	pthread_t	tid;

	if (cd == NULL && !cdda_pthr_init2(s))
		return FALSE;

	/* Set status */
	cd->i->state = CDSTAT_COMPLETED;

	if (cd->i->writer != (thid_t) 0) {
		tid = (pthread_t) cd->i->writer;

		/* Wait for writer, blocking */
		DBGPRN(DBG_GEN)(errfp,
			"\ncdda_pthr_stop: Waiting for writer thread id=%d\n",
			(int) tid);

		if ((ret = pthread_join(tid, &status)) != 0) {
			DBGPRN(DBG_GEN)(errfp,
				"pthread_join failed (writer ret=%d)\n", ret);
		}
		else {
			ret = (int) status;
			if (ret == (int) PTHREAD_CANCELED) {
				DBGPRN(DBG_GEN)(errfp, "Writer canceled\n");
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
					"Writer exited, status %d\n", ret);
			}

			/* Display message, if any */
			if (ret != 0 && cd->i->msgbuf[0] != '\0')
				CDDA_INFO(cd->i->msgbuf);
		}

		cd->i->writer = (thid_t) 0;
	}

	if (cd->i->reader != (thid_t) 0) {
		tid = (pthread_t) cd->i->reader;

		/* Wait for reader, blocking */
		DBGPRN(DBG_GEN)(errfp,
			"\ncdda_pthr_stop: Waiting for reader thread id=%d\n",
			(int) tid);

		if ((ret = pthread_join(tid, &status)) != 0) {
			DBGPRN(DBG_GEN)(errfp,
				"pthread_join failed (reader ret=%d)\n", ret);
		}
		else {
			ret = (int) status;
			if (ret == (int) PTHREAD_CANCELED) {
				DBGPRN(DBG_GEN)(errfp, "Reader canceled\n");
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
					"Reader exited, status %d\n", ret);
			}

			/* Display message, if any */
			if (ret != 0 && cd->i->msgbuf[0] != '\0')
				CDDA_INFO(cd->i->msgbuf);
		}

		cd->i->reader = (thid_t) 0;
	}

	/* Reset states */
	cdda_pthr_initshm(s);

	/* Restore saved SIGPIPE handler */
	(void) util_signal(SIGPIPE, opipe);

	return TRUE;
}


/*
 * cdda_pthr_vol
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
cdda_pthr_vol(di_dev_t *devp, curstat_t *s, int vol, bool_t query)
{
	if (cd == NULL && !cdda_pthr_init2(s))
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
 * cdda_pthr_chroute
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
cdda_pthr_chroute(di_dev_t *devp, curstat_t *s)
{
	if (cd != NULL) {
		cd->i->chroute = app_data.ch_route;
		return TRUE;
	}
	else
		return FALSE;
}


/*
 * cdda_pthr_att
 *	Change CDDA level attentuator setting.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
cdda_pthr_att(curstat_t *s)
{
	if (cd != NULL)
		cd->i->att = (int) s->cdda_att;
}


/*
 * cdda_pthr_outport
 *	Change CDDA output port setting.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_pthr_outport(void)
{
	if (cd != NULL)
		cd->i->outport = (int) app_data.outport;
}


/*
 * cdda_pthr_getstatus
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
cdda_pthr_getstatus(di_dev_t *devp, curstat_t *s, cdstat_t *sp)
{
	int		i;
	time_t		now;
	bool_t		reader_fail,
			writer_fail;
	static int	hbint = 0,
			hbcnt = 0;

	if (cd == NULL && !cdda_pthr_init2(s))
		return FALSE;

	/* Current playback status */
	sp->status = cd->i->state;
	if (sp->status == CDSTAT_COMPLETED)
		(void) cdda_pthr_stop(devp, s);

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
			cdda_pthr_kill(cd->i->reader, SIGTERM);
			cdda_pthr_kill(cd->i->writer, SIGTERM);

			(void) cdda_pthr_stop(devp, s);

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
 * cdda_pthr_debug
 *	Debug level change notification function
 *
 * Args:
 *	lev - New debug level
 *
 * Return:
 *	Nothing.
 */
void
cdda_pthr_debug(word32_t lev)
{
	if (cd != NULL)
		cd->i->debug = lev;
}



/*
 * cdda_pthr_info
 *	Append CDDA read and write method information to supplied string.
 *
 * Args:
 *	str - The string to append to.
 *
 * Return:
 *	Nothing.
 */
void
cdda_pthr_info(char *str)
{
	void	(*rinfo)(char *),
		(*winfo)(char *);

	(void) strcat(str, "POSIX Threads\n");

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
 * cdda_pthr_initipc
 *	Retrieves shared memory and semaphores. Sets up our pointers
 *	from cd into shared memory.  Used by the reader/writer threads.
 *
 * Args:
 *	cd - Pointer to the cd_state_t structure to be filled in
 *
 * Return:
 *	The IPC semaphore ID, or -1 if failed.
 */
int
cdda_pthr_initipc(cd_state_t *cdp)
{
	if (pthr_shmaddr == NULL) {
		DBGPRN(DBG_GEN)(errfp,
			"cdda_pthr_initipc: No shared memory!\n");
		return -1;
	}

	/* Now set our fields pointing into the shared memory */
	cdp->cds = (cd_size_t *) pthr_shmaddr;

	/* Info */
	cdp->i = (cd_info_t *)(void *)
		((byte_t *) pthr_shmaddr + sizeof(cd_size_t));

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

	return (1);
}


/*
 * cdda_pthr_waitsem
 *	Waits for a semaphore
 *
 * Args:
 *	semid - Unused
 *	num   - Semaphore to wait on
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
cdda_pthr_waitsem(int semid, int num)
{
	int		ret;
	pthread_t	tid;


	tid = pthread_self();

	if ((ret = pthread_mutex_lock(&pthr_sem[num].mutex)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_waitsem: "
			"pthread_mutex_lock failed (sem %d ret=%d)\n",
			num, ret);
		if ((ret = pthread_cancel(tid)) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_waitsem: "
				"pthread_cancel failed (ret=%d)\n", ret);
		}
		return;
	}

	while (pthr_sem[num].v == 0) {
		ret = pthread_cond_wait(&pthr_sem[num].cond,
					&pthr_sem[num].mutex);
		if (ret != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_waitsem: "
				"pthread_cond_wait failed (sem %d ret=%d)\n",
				num, ret);
			if ((ret = pthread_cancel(tid)) != 0) {
				DBGPRN(DBG_GEN)(errfp, "cdda_pthr_waitsem: "
				    "pthread_cancel failed (ret=%d)\n", ret);
			}
			return;
		}
	}

	pthr_sem[num].v = 0;

	if ((ret = pthread_mutex_unlock(&pthr_sem[num].mutex)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_waitsem: "
			"pthread_mutex_unlock failed (sem %d ret=%d)\n",
			num, ret);
		if ((ret = pthread_cancel(tid)) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_waitsem: "
				"pthread_cancel failed (ret=%d)\n", ret);
		}
	}
}


/*
 * cdda_pthr_postsem
 *	Releases a semaphore
 *
 * Args:
 *	semid - Unused
 *	num   - Semaphore to wait on
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
cdda_pthr_postsem(int semid, int num)
{
	int		ret;
	pthread_t	tid;

	tid = pthread_self();

	if ((ret = pthread_mutex_lock(&pthr_sem[num].mutex)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_postsem: "
			"pthread_mutex_lock failed (sem %d ret=%d)\n",
			num, ret);
		if ((ret = pthread_cancel(tid)) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_postsem: "
				"pthread_cancel failed (ret=%d)\n", ret);
		}
		return;
	}

	pthr_sem[num].v = 1;

	if ((ret = pthread_mutex_unlock(&pthr_sem[num].mutex)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_postsem: "
			"pthread_mutex_unlock failed (sem %d ret=%d)\n",
			num, ret);
		if ((ret = pthread_cancel(tid)) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_postsem: "
				"pthread_cancel failed (ret=%d)\n", ret);
		}
		return;
	}

	if ((ret = pthread_cond_signal(&pthr_sem[num].cond)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_postsem: "
			"pthread_mutex_unlock failed (sem %d ret=%d)\n",
			num, ret);
		if ((ret = pthread_cancel(tid)) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdda_pthr_postsem: "
				"pthread_cancel failed (ret=%d)\n", ret);
		}
	}
}


/*
 * cdda_pthr_preinit
 *	Early program startup initializations.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_pthr_preinit(void)
{
#ifdef NEED_PTHREAD_INIT
	pthread_init();
#endif
}


/*
 * cdda_pthr_yield
 *	Let other threads run.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_pthr_yield(void)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
	sched_yield();
#endif
	util_delayms(100);
}


/*
 * cdda_pthr_kill
 *	Terminate a thread.
 *
 * Args:
 *	id - The thread id
 *	sig - The signal
 *
 * Return:
 *	Nothing.
 */
void
cdda_pthr_kill(thid_t id, int sig)
{
	int	ret;

	if (sig != SIGTERM) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_kill: "
			"signal %d not implemented for pthreads\n", sig);
		return;
	}
	if ((ret = pthread_cancel((pthread_t) id)) != 0) {
		DBGPRN(DBG_GEN)(errfp, "cdda_pthr_kill: "
			"pthread_cancel failed (ret=%d)\n", ret);
	}
}


#endif	/* CDDA_PTHREADS */

