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
#ifndef lint
static char *_cdda_c_ident_ = "@(#)cdda.c	7.99 04/04/20";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"
#include "cdda_d/sysvipc.h"
#include "cdda_d/pthr.h"
#include "cdda_d/rd_scsi.h"
#include "cdda_d/rd_sol.h"
#include "cdda_d/rd_linux.h"
#include "cdda_d/rd_fbsd.h"
#include "cdda_d/rd_aix.h"
#include "cdda_d/wr_oss.h"
#include "cdda_d/wr_sol.h"
#include "cdda_d/wr_irix.h"
#include "cdda_d/wr_hpux.h"
#include "cdda_d/wr_aix.h"
#include "cdda_d/wr_alsa.h"
#include "cdda_d/wr_osf1.h"
#include "cdda_d/wr_fp.h"
#include "cdda_d/wr_gen.h"


extern appdata_t	app_data;
extern FILE		*errfp;


cdda_client_t		*cdda_clinfo;		/* Client information struct */


#ifdef HAS_LAME
#include "cdda_d/if_lame.h"
char	*lameprogs[] = { "lame", "notlame", NULL };
char	*lamepath;
#endif	/* HAS_LAME */


#ifdef HAS_FAAC
#include "cdda_d/if_faac.h"
char	*faacprogs[] = { "faac", NULL };
char	*faacpath;
#endif	/* HAS_FAAC */


/*
 * Cdda method call table
 * This array must correspond to the CDDA_xxx definitions found in the
 * cdda.h file, and the index is specified by the cddaMethod parameter.
 */
struct {
	word32_t (*capab)(void);
	void	 (*preinit)(void);
	bool_t	 (*init)(curstat_t *);
	void	 (*halt)(di_dev_t *, curstat_t *);
	bool_t	 (*play)(di_dev_t *, curstat_t *, int, int);
	bool_t	 (*pause_resume)(di_dev_t *, curstat_t *, bool_t);
	bool_t	 (*stop)(di_dev_t *, curstat_t *);
	int	 (*vol)(di_dev_t *, curstat_t *, int, bool_t);
	bool_t	 (*chroute)(di_dev_t *, curstat_t *);
	void	 (*cdda_att)(curstat_t *);
	void	 (*outport)(void);
	bool_t	 (*getstatus)(di_dev_t *, curstat_t *, cdstat_t *);
	void	 (*debug)(word32_t);
	void	 (*info)(char *);
	int	 (*initipc)(cd_state_t *);
	void	 (*waitsem)(int, int);
	void	 (*postsem)(int, int);
	void	 (*yield)(void);
	void	 (*kill)(thid_t, int);
} cdda_calltbl[] = {
	{ NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL },		/* CDDA_NONE */
	{ cdda_sysvipc_capab,
	  cdda_sysvipc_preinit,
	  cdda_sysvipc_init,
	  cdda_sysvipc_halt,
	  cdda_sysvipc_play,
	  cdda_sysvipc_pause_resume,
	  cdda_sysvipc_stop,
	  cdda_sysvipc_vol,
	  cdda_sysvipc_chroute,
	  cdda_sysvipc_att,
	  cdda_sysvipc_outport,
	  cdda_sysvipc_getstatus,
	  cdda_sysvipc_debug,
	  cdda_sysvipc_info,
	  cdda_sysvipc_initipc,
	  cdda_sysvipc_waitsem,
	  cdda_sysvipc_postsem,
	  cdda_sysvipc_yield,
	  cdda_sysvipc_kill },				/* CDDA_SYSVIPC */
	{ cdda_pthr_capab,
	  cdda_pthr_preinit,
	  cdda_pthr_init,
	  cdda_pthr_halt,
	  cdda_pthr_play,
	  cdda_pthr_pause_resume,
	  cdda_pthr_stop,
	  cdda_pthr_vol,
	  cdda_pthr_chroute,
	  cdda_pthr_att,
	  cdda_pthr_outport,
	  cdda_pthr_getstatus,
	  cdda_pthr_debug,
	  cdda_pthr_info,
	  cdda_pthr_initipc,
	  cdda_pthr_waitsem,
	  cdda_pthr_postsem,
	  cdda_pthr_yield,
	  cdda_pthr_kill }				/* CDDA_PTHREADS */
};


/* Call table to branch into a read-method.  The array index must
 * correspond to the cddaReadMethod parameter.
 */
cdda_rd_tbl_t	cdda_rd_calltbl[] = {
	{ NULL, NULL, NULL },				/* CDDA_RD_NONE */
	{ scsi_rinit,
	  scsi_read,
	  scsi_rdone,
	  scsi_rinfo },					/* CDDA_RD_SCSIPT */
	{ sol_rinit,
	  sol_read,
	  sol_rdone,
	  sol_rinfo },					/* CDDA_RD_SOL */
	{ linux_rinit,
	  linux_read,
	  linux_rdone,
	  linux_rinfo },				/* CDDA_RD_LINUX */
	{ fbsd_rinit,
	  fbsd_read,
	  fbsd_rdone,
	  fbsd_rinfo },					/* CDDA_RD_FBSD */
	{ aix_rinit,
	  aix_read,
	  aix_rdone,
	  aix_rinfo }					/* CDDA_RD_AIX */
};


/* Call table to branch into a write-method.  The array index must
 * correspond to the cddaWriteMethod parameter.
 */
cdda_wr_tbl_t	cdda_wr_calltbl[] = {
	{ NULL, NULL, NULL },				/* CDDA_WR_NONE */
	{ oss_winit,
	  oss_write,
	  oss_wdone,
	  oss_winfo },					/* CDDA_WR_OSS */
	{ sol_winit,
	  sol_write,
	  sol_wdone,
	  sol_winfo },					/* CDDA_WR_SOL */
	{ irix_winit,
	  irix_write,
	  irix_wdone,
	  irix_winfo },					/* CDDA_WR_IRIX */
	{ hpux_winit,
	  hpux_write,
	  hpux_wdone,
	  hpux_winfo },					/* CDDA_WR_HPUX */
	{ aix_winit,
	  aix_write,
	  aix_wdone,
	  aix_winfo },					/* CDDA_WR_AIX */
	{ alsa_winit,
	  alsa_write,
	  alsa_wdone,
	  alsa_winfo },					/* CDDA_WR_ALSA */
	{ osf1_winit,
	  osf1_write,
	  osf1_wdone,
	  osf1_winfo },					/* CDDA_WR_OSF1 */
	{ fp_winit,
	  fp_write,
	  fp_wdone,
	  fp_winfo }					/* CDDA_WR_FP */
};


/* Initialization done flag */
STATIC bool_t		cdda_initted = FALSE;

/* File format description table */
STATIC filefmt_t	cdda_ffmt_tbl[MAX_FILEFMTS] = {
    { FILEFMT_RAW,	"RAW",		".raw",  "CDDA Raw (LE)"             },
    { FILEFMT_AU,	"AU",		".au",   "Sun/Next Audio"            },
    { FILEFMT_WAV,	"WAV",		".wav",  "Wave"                      },
    { FILEFMT_AIFF,	"AIFF",		".aiff", "Apple/SGI Audio"           },
    { FILEFMT_AIFC,	"AIFF-C",	".aifc", "Apple/SGI Audio"           },
    { FILEFMT_MP3,	"MP3",		".mp3",  "MPEG-1 Layer 3"            },
    { FILEFMT_OGG,	"Ogg Vorbis",	".ogg",  "Ogg Vorbis"                },
    { FILEFMT_FLAC,	"FLAC",		".flac", "Free Lossless Audio CODEC" },
    { FILEFMT_AAC,	"AAC",		".aac",  "Advanced Audio Coding"     },
    { FILEFMT_MP4,	"MP4",		".mp4",  "MP4"                       },
};

/* Internal supported bitrates table for MP3 and OggVorbis */
#define NBITRATES	16

STATIC int		_bitrates[NBITRATES] = {
	0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1
};

STATIC int		cdda_nbitrates = 0;


/*********************
 * Private functions *
 *********************/


/*
 * cdda_bitrates_init
 *	Initialize the number of supported bitrates for MP3 and OggVorbis
 *	output.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cdda_bitrates_init(void)
{
	cdda_nbitrates = NBITRATES - 1;
}


/**********************
 *  Public functions  *
 **********************/


/*
 * cdda_capab
 *	Query configured CDDA subsystem capabilities
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
cdda_capab(void)
{
	word32_t	ret,
			(*func)(void);

	if (app_data.cdda_method == CDDA_NONE)
		return 0;

	if (app_data.cdda_method < CDDA_NONE ||
	    app_data.cdda_method >= CDDA_METHODS) {
		CDDA_WARNING(app_data.str_cddainit_fail);
		DBGPRN(DBG_GEN)(errfp, "Warning: %s\n",
				  app_data.str_cddainit_fail);
		return 0;
	}

	func = cdda_calltbl[app_data.cdda_method].capab;
	if (func != NULL)
		ret = (*func)();
	else {
		CDDA_WARNING(app_data.str_cddainit_fail);
		DBGPRN(DBG_GEN)(errfp, "Warning: %s\n",
				  app_data.str_cddainit_fail);
		ret = 0;
	}

	return (ret);
}


/*
 * cdda_preinit
 *	Early program startup initialization function.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_preinit(void)
{
	void	(*func)(void);

	func = cdda_calltbl[app_data.cdda_method].preinit;
	if (func != NULL)
		(*func)();
}


/*
 * cdda_init
 *	Initialize the CDDA subsystem.
 *
 * Args:
 *	s   - Pointer to the curstat_t structure.
 *	clp - Pointer to cdda subsystem client registration info structure.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
cdda_init(curstat_t *s, cdda_client_t *clp)
{
	bool_t	ret,
		(*func)(curstat_t *);

	if (cdda_initted)
		return TRUE;

	cdda_clinfo = (cdda_client_t *)(void *) MEM_ALLOC(
		"cdda_client_t",
		sizeof(cdda_client_t)
	);
	if (cdda_clinfo == NULL)
		return FALSE;

	(void) memcpy(cdda_clinfo, clp, sizeof(cdda_client_t));

	if (app_data.cdda_method <= CDDA_NONE ||
	    app_data.cdda_method >= CDDA_METHODS)
		return FALSE;

	func = cdda_calltbl[app_data.cdda_method].init;
	if (func != NULL)
		ret = (*func)(s);
	else
		ret = FALSE;

	cdda_initted = ret;
	return ret;
}


/*
 * cdda_halt
 *	Shuts down the CDDA subsystem.
 *
 * Args:
 *	devp - Device descriptor
 *	s  - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
cdda_halt(di_dev_t *devp, curstat_t *s)
{
	void	(*func)(di_dev_t *, curstat_t *);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].halt;
	if (func != NULL)
		(*func)(devp, s);

	cdda_initted = FALSE;
}


/*
 * cdda_play
 *	Start playing.  No error checking is done on the output file path
 *	or pipe program strings here.  The caller should pre-check these.
 *
 * Args:
 *	devp - Device descriptor
 *	s  - Pointer to the curstat_t structure
 *	start_lba - Start logical block address
 *	end_lba   - End logical block address
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
cdda_play(di_dev_t *devp, curstat_t *s, sword32_t start_lba, sword32_t end_lba)
{
	bool_t	(*func)(di_dev_t *, curstat_t *, sword32_t, sword32_t);

	if (!cdda_initted)
		return FALSE;

	/* If start == end, just return success */
	if (start_lba == end_lba)
		return TRUE;

	func = cdda_calltbl[app_data.cdda_method].play;
	if (func != NULL)
		return (*func)(devp, s, start_lba, end_lba);

	return FALSE;
}


/*
 * cdda_pause_resume
 *	Pause/resume playback
 *
 * Args:
 *	devp   - Device descriptor
 *	s      - Pointer to the curstat_t structure
 *	resume - Whether to resume playback
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
cdda_pause_resume(di_dev_t *devp, curstat_t *s, bool_t resume)
{
	bool_t	(*func)(di_dev_t *, curstat_t *, bool_t);

	if (!cdda_initted)
		return FALSE;

	func = cdda_calltbl[app_data.cdda_method].pause_resume;
	if (func != NULL)
		return (*func)(devp, s, resume);

	return FALSE;
}


/*
 * cdda_stop
 *	Stop playback
 *
 * Args:
 *	devp - Device descriptor
 *	s    - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
cdda_stop(di_dev_t *devp, curstat_t *s)
{
	bool_t	(*func)(di_dev_t *, curstat_t *);

	if (!cdda_initted)
		return FALSE;

	func = cdda_calltbl[app_data.cdda_method].stop;
	if (func != NULL)
		return (*func)(devp, s);

	return FALSE;
}


/*
 * cdda_vol
 *	Change volume setting
 *
 * Args:
 *	devp  - Device descriptor
 *	s     - Pointer to the curstat_t structure
 *	vol   - Desired volume level
 *	query - Whether querying or setting the volume
 *
 * Return:
 *	The volume level
 */
int
cdda_vol(di_dev_t *devp, curstat_t *s, int vol, bool_t query)
{
	int	(*func)(di_dev_t *, curstat_t *, int, bool_t);

	if (!cdda_initted)
		return FALSE;

	func = cdda_calltbl[app_data.cdda_method].vol;
	if (func != NULL)
		return (*func)(devp, s, vol, query);

	return -1;
}


/*
 * cdda_chroute
 *	Change channel routing setting
 *
 * Args:
 *	devp - Device descriptor
 *	s    - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
cdda_chroute(di_dev_t *devp, curstat_t *s)
{
	bool_t	(*func)(di_dev_t *, curstat_t *);

	if (!cdda_initted)
		return FALSE;

	func = cdda_calltbl[app_data.cdda_method].chroute;
	if (func != NULL)
		return (*func)(devp, s);

	return FALSE;
}


/*
 * cdda_att
 *	Change CDDA attenuator setting
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
cdda_att(curstat_t *s)
{
	void	(*func)(curstat_t *);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].cdda_att;
	if (func != NULL)
		(*func)(s);
}


/*
 * cdda_outport
 *	Change CDDA output port setting
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_outport(void)
{
	void	(*func)(void);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].outport;
	if (func != NULL)
		(*func)();
}


/*
 * cdda_getstatus
 *	Get playback status
 *
 * Args:
 *	devp - Device descriptor
 *	s    - Pointer to the curstat_t structure
 *	sp - cdstat_t return info structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
cdda_getstatus(di_dev_t *devp, curstat_t *s, cdstat_t *sp)
{
	bool_t	(*func)(di_dev_t *, curstat_t *, cdstat_t *);

	if (!cdda_initted)
		return FALSE;

	func = cdda_calltbl[app_data.cdda_method].getstatus;
	if (func != NULL)
		return (*func)(devp, s, sp);

	return FALSE;
}


/*
 * cdda_debug
 *	Debug level change notification function
 *
 * Args:
 *	lev - The new debug level
 *
 * Return:
 *	Nothing.
 */
void
cdda_debug(word32_t lev)
{
	void	(*func)(word32_t);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].debug;
	if (func != NULL)
		(*func)(lev);
}


/*
 * cdda_info
 *	Obtain CDDA configuration information
 *
 * Args:
 *	None.
 *
 * Return:
 *	Pointer to an informational text string buffer.
 */
char *
cdda_info(void)
{
	void		(*func)(char *);
	static bool_t	first = TRUE;
	static char	str[STR_BUF_SZ * 2];

	if (first) {
		/* Set up info string */
		first = FALSE;
		(void) strcpy(str, "CDDA method: ");

		if (app_data.cdda_method < CDDA_NONE ||
		    app_data.cdda_method >= CDDA_METHODS) {
			(void) strcat(str, "not configured\n");
		}
		else {
			func = cdda_calltbl[app_data.cdda_method].info;
			if (func == NULL)
				(void) strcat(str, "not configured\n");
			else
				(*func)(str);
		}
	}

	return (str);
}


/*
 * cdda_initipc
 *	Set up inter-process/thread shared memory and sychronization
 *	based on mode.
 *
 * Args:
 *	Pointer to the cd_state_t structure
 *
 * Return:
 *	Nothing.
 */
int
cdda_initipc(cd_state_t *cdp)
{
	int	(*func)(cd_state_t *);

	if (!cdda_initted)
		return -1;

	func = cdda_calltbl[app_data.cdda_method].initipc;
	if (func != NULL)
		return (*func)(cdp);
	else
		return -1;
}


/*
 * cdda_waitsem
 *	Wait for a semaphore.
 *
 * Args:
 *	id  - Semaphore id
 *	sem - Semaphore to wait on
 *
 * Return:
 *	Nothing.
 */
void
cdda_waitsem(int id, int sem)
{
	void	(*func)(int, int);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].waitsem;
	if (func != NULL)
		(*func)(id, sem);
}


/*
 * cdda_postsem
 *	Release a semaphore.
 *
 * Args:
 *	id  - Semaphore id
 *	sem - Semaphore to release
 *
 * Return:
 *	Nothing.
 */
void
cdda_postsem(int id, int sem)
{
	void	(*func)(int, int);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].postsem;
	if (func != NULL)
		(*func)(id, sem);
}


/*
 * cdda_yield
 *	Let other processes/threads run.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdda_yield(void)
{
	void	(*func)(void);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].yield;
	if (func != NULL)
		(*func)();
}


/*
 * cdda_kill
 *	Terminate a process or thread, depending on the configured mode.
 *
 * Args:
 *	id - The process or thread id
 *	sig - The signal
 *
 * Return:
 *	Nothing.
 */
void
cdda_kill(thid_t id, int sig)
{
	void	(*func)(thid_t, int);

	if (!cdda_initted)
		return;

	func = cdda_calltbl[app_data.cdda_method].kill;
	if (func != NULL)
		(*func)(id, sig);
}


/*
 * cdda_filefmt_supp
 *	Given a CDDA output file format value, return a boolean indicating
 *	whether that format is supported in this compilation.
 *
 * Args:
 *	fmt - The format value
 *
 * Return:
 *	TRUE  - Format supported
 *	FALSE - Format not supported
 */
bool_t
cdda_filefmt_supp(int fmt)
{
	static bool_t	vorb_first = TRUE;
	static bool_t	flac_first = TRUE;
	static bool_t	lame_first = TRUE;
	static bool_t	faac_first = TRUE;
#ifdef HAS_LAME
	unsigned int	lame_ver;
#endif
#ifdef HAS_FAAC
	static bool_t	aacsupp = FALSE,
			mp4supp = FALSE;
	unsigned int	faac_ver;
#endif

	switch (fmt) {
	case FILEFMT_RAW:
	case FILEFMT_AU:
	case FILEFMT_WAV:
	case FILEFMT_AIFF:
	case FILEFMT_AIFC:
		return TRUE;

	case FILEFMT_MP3:
#ifdef HAS_LAME
		if (lame_first) {
			int	i;

			lame_first = FALSE;

			if ((lamepath = getenv("LAME_PATH")) != NULL) {
				/* Check if specified program is executable */
				if (!util_checkcmd(lamepath))
					lamepath = NULL;
			}
			else for (i = 0; lameprogs[i] != NULL; i++) {
				lamepath = util_findcmd(lameprogs[i]);
				if (lamepath != NULL)
					break;
			}

			if (lamepath == NULL) {
				DBGPRN(DBG_GEN|DBG_SND)(errfp,
					"\nLAME encoder:\n"
					"\tProgram:\t\t\tNot found\n"
					"\tMP3 file format disabled\n"
				);
			}
			else if (!if_lame_verchk(lamepath, &lame_ver)) {
				lamepath = NULL;
			}
			else {
				DBGPRN(DBG_GEN|DBG_SND)(errfp,
					"\nLAME encoder:\n"
					"\tProgram:\t\t\t%s\n"
					"\tVersion:\t\t\t%u.%u.%u\n",
					lamepath,
					ENCVER_MAJ(lame_ver),
					ENCVER_MIN(lame_ver),
					ENCVER_TINY(lame_ver)
				);
			}
		}

		return ((bool_t) (lamepath != NULL));
#else
		if (lame_first) {
			lame_first = FALSE;
			DBGPRN(DBG_GEN|DBG_SND)(errfp,
				"\nLAME encoder:\n"
				"\tInterface:\t\t\tDisabled\n"
				"\tMP3 file format disabled\n"
			);
		}
		return FALSE;
#endif	/* HAS_LAME */

	case FILEFMT_OGG:
#ifdef HAS_VORBIS
		if (vorb_first) {
			vorb_first = FALSE;
			DBGPRN(DBG_GEN|DBG_SND)(errfp,
				"\nOgg Vorbis encoder:\n"
				"\tEmbedded:\t\t\tTrue\n"
			);
		}
		return TRUE;
#else
		if (vorb_first) {
			vorb_first = FALSE;
			DBGPRN(DBG_GEN|DBG_SND)(errfp,
				"\nOgg Vorbis encoder:\n"
				"\tEmbedded:\t\t\tFalse\n"
				"\tOgg Vorbis file format disabled\n"
			);
		}
		return FALSE;
#endif	/* HAS_VORBIS */

	case FILEFMT_FLAC:
#ifdef HAS_FLAC
		if (flac_first) {
			flac_first = FALSE;
			DBGPRN(DBG_GEN|DBG_SND)(errfp,
				"\nFLAC encoder:\n"
				"\tEmbedded:\t\t\tTrue\n"
			);
		}
		return TRUE;
#else
		if (flac_first) {
			flac_first = FALSE;
			DBGPRN(DBG_GEN|DBG_SND)(errfp,
				"\nFLAC encoder:\n"
				"\tEmbedded:\t\t\tFalse\n"
				"\tFLAC file format disabled\n"
			);
		}
		return FALSE;
#endif

	case FILEFMT_AAC:
	case FILEFMT_MP4:
#ifdef HAS_FAAC
		if (faac_first) {
			int	i;

			faac_first = FALSE;

			if ((faacpath = getenv("FAAC_PATH")) != NULL) {
				/* Check if specified program is executable */
				if (!util_checkcmd(faacpath))
					faacpath = NULL;
			}
			else for (i = 0; faacprogs[i] != NULL; i++) {
				faacpath = util_findcmd(faacprogs[i]);
				if (faacpath != NULL)
					break;
			}

			if (faacpath == NULL) {
				DBGPRN(DBG_GEN|DBG_SND)(errfp,
					"\nFAAC encoder:\n"
					"\tProgram:\t\t\tNot found\n"
					"\tMPEG-2/4 file formats disabled\n"
				);
			}
			else if (!if_faac_verchk(faacpath, &faac_ver,
						 &mp4supp)) {
				faacpath = NULL;
			}
			else {
				aacsupp = TRUE;

				DBGPRN(DBG_GEN|DBG_SND)(errfp,
					"\nFAAC encoder:\n"
					"\tProgram:\t\t\t%s\n"
					"\tVersion:\t\t\t%u.%u.%u\n"
					"\tAAC support:\t\t\t%s\n"
					"\tMP4 support:\t\t\t%s\n",
					faacpath,
					ENCVER_MAJ(faac_ver),
					ENCVER_MIN(faac_ver),
					ENCVER_TINY(faac_ver),
					aacsupp ? "True" : "False",
					mp4supp ? "True" : "False"
				);
			}
		}

		if (fmt == FILEFMT_MP4)
			return (mp4supp);
		else
			return (aacsupp);
#else
		if (faac_first) {
			faac_first = FALSE;
			DBGPRN(DBG_GEN|DBG_SND)(errfp,
				"\nFAAC encoder:\n"
				"\tInterface:\t\t\tDisabled\n"
				"\tAAC/MP4 file formats disabled\n"
			);
		}
		return FALSE;
#endif	/* HAS_FAAC */

	default:
		return FALSE;
	}
	/*NOTREACHED*/
}


/*
 * cdda_filefmt
 *	Given a CDDA file format code, return a pointer to an
 *	associated filefmt_t structure.
 *
 * Args:
 *	fmt - The file format code
 *
 * Return:
 *	Pointer to the filefmt_t structure, or NULL if fmt is invalid.
 */
filefmt_t *
cdda_filefmt(int fmt)
{
	if (fmt < 0 || fmt >= MAX_FILEFMTS)
		return NULL;

	return (&cdda_ffmt_tbl[fmt]);
}


/*
 * cdda_bitrates
 *	Return the number of supported bitrates for MP3 and OggVorbis
 *
 * Args:
 *	None.
 *
 * Return:
 *	The number of supported bitrates
 */
int
cdda_bitrates(void)
{
	if (cdda_nbitrates == 0)
		cdda_bitrates_init();

	return (cdda_nbitrates);
}


/*
 * cdda_bitrate_val
 *	Given a bitrate index, return the actual bitrate value
 *
 * Args:
 *	idx - The bitrate table index
 *
 * Return:
 *	The bitrate in kb/s
 */
int
cdda_bitrate_val(int idx)
{
	if (cdda_nbitrates == 0)
		cdda_bitrates_init();

	if (idx < 0 || idx >= cdda_nbitrates)
		return -1;

	return (_bitrates[idx]);
}


/*
 * cdda_bitrate_name
 *	Given a bitrate index, return the actual bitrate name string
 *
 * Args:
 *	idx - The bitrate table index
 *
 * Return:
 *	Pointer to the bitrate string.  This points to an internal buffer
 *	that is overwritten on each call.
 */
char *
cdda_bitrate_name(int idx)
{
	static char	name[8];

	if (cdda_nbitrates == 0)
		cdda_bitrates_init();

	if (idx < 0 || idx >= cdda_nbitrates)
		return NULL;

	(void) sprintf(name, "%d", _bitrates[idx]);
	return (name);
}


/*
 * cdda_heartbeat_interval
 *	Compute the number of CDDA read/write loop iterations per
 *	hearbeat.  This is based on the read chunk size.
 *
 * Args:
 *	fps - The current transfer rate in frames per second
 *
 * Return:
 *	A integer loop iteration count.
 */
int
cdda_heartbeat_interval(int fps)
{
	return ((fps / app_data.cdda_readchkblks) + 1);
}


/*
 * cdda_heartbeat
 *	Set a heartbeat timestamp in the location specified.  This is
 *	used to check if either a CDDA read or write thread is hung or
 *	dead.
 *
 * Args:
 *	hbloc  - pointer to location where the timestamp is to be written.
 *	hbtype - CDDA_HB_READER or CDDA_HB_WRITER
 *
 * Return:
 *	Nothing.
 */
void
cdda_heartbeat(time_t *hbloc, int hbtype)
{
	if (hbloc == NULL)
		return;

	*hbloc = time(NULL);

	DBGPRN(DBG_SND)(errfp, "\nCDDA thread heartbeat: %s\n",
		(hbtype == CDDA_HB_READER) ? "reader" : "writer");
}

