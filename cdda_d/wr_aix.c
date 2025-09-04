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

/*
 *   IBM AIX Ultimedia Services sound driver support
 */
#ifndef lint
static char *_wr_aix_c_ident_ = "@(#)wr_aix.c	7.65 04/03/24";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#if defined(CDDA_WR_AIX) && defined(CDDA_SUPPORTED)

#include <sys/audio.h>
#include "cdda_d/wr_gen.h"
#include "cdda_d/wr_aix.h"


extern appdata_t	app_data;
extern FILE		*errfp;

STATIC int		aix_wsemid,		/* Semaphores identifier */
			aix_outport = 0;	/* Output port */
STATIC pid_t		aix_pipe_pid = -1;	/* Pipe to program pid */
STATIC cd_state_t	*aix_wcd;		/* Shared memory pointer */
STATIC bool_t		aix_write_dev,		/* Write to audio device */
			aix_write_file,		/* Write to output file */
			aix_write_pipe,		/* Pipe to program */
			aix_file_be = FALSE,	/* Big endian output file */
			aix_vol_supp = FALSE;	/* Volume control supported */
STATIC long		aix_curr_vol;		/* Previous volume */
STATIC gen_desc_t	*aix_audio_desc = NULL,	/* Audio device desc */
			*aix_file_desc = NULL,	/* Output file desc */
			*aix_pipe_desc = NULL;	/* Pipe to program desc */
STATIC unsigned int	aix_file_mode;		/* Output file mode */


/*
 * aixaud_scale_vol
 *	Scale volume from xmcd 0-100 to AIX audio device values
 *
 * Args:
 *	vol - xmcd volume 0-100
 *
 * Return:
 *	AIX volume value
 */
STATIC int
aixaud_scale_vol(int vol)
{
	int	n;

	n = ((vol * AIX_MAX_VOL) / 100);
	if (((vol * AIX_MAX_VOL) % 100) >= 50)
		n++;

	/* Range checking */
	if (n > AIX_MAX_VOL)
		n = AIX_MAX_VOL;

	return (n << 16);
}


/*
 * aixaud_unscale_vol
 *	Scale volume from AIX audio device values to xmcd 0-100
 *
 * Args:
 *	vol - AIX volume value
 *
 * Return:
 *	xmcd volume 0-100
 */
STATIC int
aixaud_unscale_vol(int vol)
{
	int	n;

	n = ((vol >> 16) * 100) / AIX_MAX_VOL;
	if (((vol * 100) % AIX_MAX_VOL) >= AIX_HALF_VOL)
		n++;

	/* Range checking */
	if (n < 0)
		n = 0;
	else if (n > 100)
		n = 100;

	return (n);
}


/*
 * aixaud_open_dev
 *	Opens the audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
aixaud_open_dev(void)
{
	char		*cp,
			*trydev[] = {
				DEFAULT_PCIDEV,
				DEFAULT_MCADEV,
				NULL
			};
	int		i = 0;
	struct stat	stbuf;

	if (!gen_set_eid(aix_wcd))
		return FALSE;

	/* Use environment variable if set */
	if ((cp = getenv("AUDIODEV")) != NULL) {
		/* Check to make sure it's a device */
		if (stat(trydev[i], &stbuf) < 0 || !S_ISCHR(stbuf.st_mode)) {
			(void) sprintf(aix_wcd->i->msgbuf,
					"aixaud_open_dev: "
					"%s is not a character device",
					cp);
			DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
			(void) gen_reset_eid(aix_wcd);
			return FALSE;
		}
	}
	else {
		for (i = 0; trydev[i] != NULL; i++) {
			/* Check to make sure it's a device */
			if (stat(trydev[i], &stbuf) < 0 ||
			    !S_ISCHR(stbuf.st_mode))
				continue;

			cp = trydev[i];
			break;
		}

		if (cp == NULL) {
			(void) sprintf(aix_wcd->i->msgbuf,
					"aixaud_open_dev: "
					"No audio device found");
			DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
			(void) gen_reset_eid(aix_wcd);
			return FALSE;
		}
	}

	/* Open audio device */
	aix_audio_desc = gen_open_file(
		aix_wcd,
		cp,
		O_WRONLY,
		0,
		FILEFMT_RAW,
		0
	);
	if (aix_audio_desc == NULL) {
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_open_dev: open of %s failed:\n%s",
			    cp, strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
		(void) gen_reset_eid(aix_wcd);
		return FALSE;
	}

	(void) gen_reset_eid(aix_wcd);
	return TRUE;
}


/*
 * aixaud_close_dev
 *	Closes the audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
aixaud_close_dev(void)
{
	int		i;
	audio_control	acontrol;
	audio_buffer	abuffer;

	if (!gen_set_eid(aix_wcd))
		return FALSE;

	/* Wait for the playback buffer to drain */

	(void) memset(&acontrol, 0, sizeof(acontrol));
	acontrol.request_info = &abuffer;
	acontrol.position = 0;

	for (i = 0; i < 50; i++) {
		if (!gen_ioctl(aix_audio_desc, AUDIO_BUFFER, &acontrol)) {
			(void) sprintf(aix_wcd->i->msgbuf,
				    "aixaud_close_dev: "
				    "AUDIO_BUFFER failed:\n%s",
				    strerror(errno));
			DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
			break;
		}
		else if (abuffer.flags <= 0)
			break;

		util_delayms(200);
	}

	(void) memset(&acontrol, 0, sizeof(acontrol));
	acontrol.ioctl_request = AUDIO_STOP;
	acontrol.request_info = NULL;
	acontrol.position = 0;

	if (!gen_ioctl(aix_audio_desc, AUDIO_CONTROL, &acontrol)) {
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_close_dev: "
			    "AUDIO_CONTROL failed:\n%s",
			    strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
	}

	/* Close audio device */
	(void) gen_close_file(aix_audio_desc);
	aix_audio_desc = NULL;

	(void) gen_reset_eid(aix_wcd);
	return TRUE;
}


/*
 * aixaud_set_outport
 *	Set CDDA playback output port.
 *
 * Args:
 *	outport - New output port selector bitmap CDDA_OUT_XXX
 *		  Note that:
 *		  "speaker" is mapped to "internal speaker",
 *		  "headphone" is mapped to "external speaker",
 *		  "line-out" is mapped to "output 1".
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixaud_set_outport(int outport)
{
	audio_change	achange;
	audio_control	acontrol;

	if (outport != 0 && outport != aix_outport) {
		(void) memset(&achange, 0, sizeof(achange));
		achange.volume = AUDIO_IGNORE;
		achange.volume_delay = 0;
		achange.balance = AUDIO_IGNORE;
		achange.balance_delay = 0;
		achange.input = AUDIO_IGNORE;
		achange.treble = AUDIO_IGNORE;
		achange.bass = AUDIO_IGNORE;
		achange.pitch = AUDIO_IGNORE;
		achange.monitor = AUDIO_IGNORE;
		achange.dev_info = NULL;

		if ((outport & CDDA_OUT_SPEAKER) != 0)
			achange.output |= INTERNAL_SPEAKER;
		if ((outport & CDDA_OUT_HEADPHONE) != 0)
			achange.output |= EXTERNAL_SPEAKER;
		if ((outport & CDDA_OUT_LINEOUT) != 0)
			achange.output |= OUTPUT_1;

		(void) memset(&acontrol, 0, sizeof(acontrol));
		acontrol.ioctl_request = AUDIO_CHANGE;
		acontrol.request_info = (char *) &achange;
		acontrol.position = 0;

		if (!gen_ioctl(aix_audio_desc, AUDIO_CONTROL, &acontrol)) {
			(void) sprintf(aix_wcd->i->msgbuf,
				    "aixaud_set_outport: "
				    "AUDIO_CHANGE failed:\n%s",
				    strerror(errno));
			DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
		}

		aix_outport = outport;
	}
}


/*
 * aixaud_config
 *	Sets up audio device for playing CD quality audio.
 *
 * Args:
 *	None.
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
aixaud_config(void)
{
	audio_init	ainit;
	audio_control	acontrol;
	audio_status	astatus;

	(void) memset(&ainit, 0, sizeof(ainit));
	ainit.srate = 44100;
	ainit.channels = 2;
	ainit.mode = PCM;
	ainit.bits_per_sample = 16;
	ainit.flags = BIG_ENDIAN | TWOS_COMPLEMENT;
	ainit.operation = PLAY;
	ainit.bsize = AIX_AUDIO_BSIZE;

	if (!gen_ioctl(aix_audio_desc, AUDIO_INIT, &ainit)) {
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_config: AUDIO_INIT failed:\n%s",
			    strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
		return FALSE;
	}

	/* Check audio init return code */
	switch (ainit.rc) {
	case 0:
	case NO_RECORD:
		/* OK */
		break;

	case NO_PLAY:
	case INVALID_REQUEST:
	case CONFLICT:
	case OVERLOADED:
	default:
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_config: AUDIO_INIT failed (rc=%d)",
			    ainit.rc);
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
		return FALSE;
	}

	/* Check volume support */
	if (ainit.flags & VOLUME)
		aix_vol_supp = TRUE;

	/* Select audio output port */
	aixaud_set_outport(aix_wcd->i->outport);

	/* Start audio playback operation */
	(void) memset(&acontrol, 0, sizeof(acontrol));
	acontrol.ioctl_request = AUDIO_START;
	acontrol.request_info = NULL;
	acontrol.position = 0;

	if (!gen_ioctl(aix_audio_desc, AUDIO_CONTROL, &acontrol)) {
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_config: AUDIO_START failed:\n%s",
			    strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
		return FALSE;
	}

	/* Retrieve current settings */
	(void) memset(&astatus, 0, sizeof(astatus));
	if (!gen_ioctl(aix_audio_desc, AUDIO_STATUS, &astatus)) {
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_set_info: AUDIO_STATUS failed:\n%s",
			    strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
		return FALSE;
	}

	aix_curr_vol = -1;

	if (aix_vol_supp) {
		/* Update volume */
		aix_curr_vol = astatus.change.volume;
		aix_wcd->i->vol = util_untaper_vol(
			aixaud_unscale_vol((int) astatus.change.volume)
		);
	}
	else {
		aix_wcd->i->vol = 100;
	}

	/* Balance control not supported for now */
	aix_wcd->i->vol_left = 100;
	aix_wcd->i->vol_right = 100;

	return TRUE;
}


/*
 * aixaud_update
 *	Updates hardware volume and balance settings from GUI settings.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixaud_update(void)
{
	audio_change		achange;
	audio_control		acontrol;

	/* Select audio output port */
	aixaud_set_outport(aix_wcd->i->outport);

	if (!aix_vol_supp)
		return;

	app_data.vol_taper = aix_wcd->i->vol_taper;

	(void) memset(&achange, 0, sizeof(achange));
	achange.volume = (long) aixaud_scale_vol(
		util_taper_vol(aix_wcd->i->vol)
	);

	if (achange.volume == aix_curr_vol)
		/* No change */
		return;

	achange.volume_delay = 0;
	achange.balance = AUDIO_IGNORE;
	achange.balance_delay = 0;
	achange.input = AUDIO_IGNORE;
	achange.output = AUDIO_IGNORE;
	achange.treble = AUDIO_IGNORE;
	achange.bass = AUDIO_IGNORE;
	achange.pitch = AUDIO_IGNORE;
	achange.monitor = AUDIO_IGNORE;
	achange.dev_info = NULL;

	(void) memset(&acontrol, 0, sizeof(acontrol));
	acontrol.ioctl_request = AUDIO_CHANGE;
	acontrol.request_info = (char *) &achange;
	acontrol.position = 0;

	aix_curr_vol = achange.volume;

	/* Apply new settings */
	if (!gen_ioctl(aix_audio_desc, AUDIO_CONTROL, &acontrol)) {
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_set_info: AUDIO_CHANGE failed\n%s",
			    strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
	}
}


/*
 * aixaud_status
 *	Updates volume and balance settings from audio device.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
aixaud_status(void)
{
	audio_status	astatus;

	/* Retrieve current settings */
	(void) memset(&astatus, 0, sizeof(astatus));
	if (!gen_ioctl(aix_audio_desc, AUDIO_STATUS, &astatus)) {
		(void) sprintf(aix_wcd->i->msgbuf,
			    "aixaud_status: AUDIO_STATUS failed\n%s",
			    strerror(errno));
		DBGPRN(DBG_SND)(errfp, "%s\n", aix_wcd->i->msgbuf);
		return;
	}

	if (aix_vol_supp) {
		/* Update volume */
		aix_curr_vol = astatus.change.volume;
		aix_wcd->i->vol = util_untaper_vol(
			aixaud_unscale_vol((int) astatus.change.volume)
		);
	}
	else {
		aix_wcd->i->vol = 100;
	}

	/* Balance control not supported for now */
	aix_wcd->i->vol_left = 100;
	aix_wcd->i->vol_right = 100;
}


/*
 * aixaud_write
 *	Function responsible for writing from the buffer to the audio
 *	device (and file if saving).
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
aixaud_write(curstat_t *s)
{
	byte_t	*in_data,
		*sw_data,
		*le_data,
		*be_data,
		*dev_data,
		*file_data,
		*pipe_data;
	char	*fname = NULL;
	int	trk_idx = -1,
		hbcnt = 1;
	time_t	start_time,
		tot_time;
	bool_t	ret;

	/* Get memory for audio device write buffer */
	le_data = (byte_t *) MEM_ALLOC("le_data", aix_wcd->cds->chunk_bytes);
	if (le_data == NULL) {
		(void) sprintf(aix_wcd->i->msgbuf,
				"aixaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", aix_wcd->i->msgbuf);
		return FALSE;
	}

	/* Get memory for audio file write buffer */
	be_data = (byte_t *) MEM_ALLOC("be_data", aix_wcd->cds->chunk_bytes);
	if (be_data == NULL) {
		MEM_FREE(le_data);
		(void) sprintf(aix_wcd->i->msgbuf,
				"aixaud_write: out of memory");
		DBGPRN(DBG_GEN)(errfp, "%s\n", aix_wcd->i->msgbuf);
		return FALSE;
	}

	(void) memset(le_data, 0, aix_wcd->cds->chunk_bytes);
	(void) memset(be_data, 0, aix_wcd->cds->chunk_bytes);

	in_data = app_data.cdda_bigendian ? be_data : le_data;
	sw_data = app_data.cdda_bigendian ? le_data : be_data;

	/* Start time */
	start_time = time(NULL);

	/* While not stopped */
	ret = TRUE;
	while (aix_wcd->i->state != CDSTAT_COMPLETED) {
		/* Get lock */
		cdda_waitsem(aix_wsemid, LOCK);

		/* End if finished and nothing in the buffer */
		if (aix_wcd->cdb->occupied == 0 && aix_wcd->i->cdda_done) {
			aix_wcd->i->state = CDSTAT_COMPLETED;
			cdda_postsem(aix_wsemid, LOCK);
			break;
		}

		/* Wait until there is something there */
		while (aix_wcd->cdb->occupied <= 0 &&
		       aix_wcd->i->state != CDSTAT_COMPLETED) {
			cdda_postsem(aix_wsemid, LOCK);
			cdda_waitsem(aix_wsemid, DATA);
			cdda_waitsem(aix_wsemid, LOCK);
		}

		/* Break if stopped */
		if (aix_wcd->i->state == CDSTAT_COMPLETED) {
			cdda_postsem(aix_wsemid, LOCK);
			break;
		}

		/* Set the current track and track length info */
		cdda_wr_setcurtrk(aix_wcd);

		/* Copy a chunk from the nextout slot into the data buffers */
		(void) memcpy(
			in_data,
			&aix_wcd->cdb->b[
			    aix_wcd->cds->chunk_bytes * aix_wcd->cdb->nextout
			],
			aix_wcd->cds->chunk_bytes
		);

		/* Update pointers */
		aix_wcd->cdb->nextout++;
		aix_wcd->cdb->nextout %= aix_wcd->cds->buffer_chunks;
		aix_wcd->cdb->occupied--;

		/* Set heartbeat and interval, update stats */
		if (--hbcnt == 0) {
			cdda_heartbeat(&aix_wcd->i->writer_hb, CDDA_HB_WRITER);

			tot_time = time(NULL) - start_time;
			if (tot_time > 0 && aix_wcd->i->frm_played > 0) {
				aix_wcd->i->frm_per_sec = (int)
					((float) aix_wcd->i->frm_played /
					 (float) tot_time) + 0.5;
			}

			hbcnt = cdda_heartbeat_interval(
				aix_wcd->i->frm_per_sec
			);
		}

		/* Signal room available */
		cdda_postsem(aix_wsemid, ROOM);

		/* Release lock */
		cdda_postsem(aix_wsemid, LOCK);

		/* Change channel routing and attenuation if necessary */
		gen_chroute_att(aix_wcd->i->chroute,
				aix_wcd->i->att,
				(sword16_t *)(void *) in_data,
				(size_t) aix_wcd->cds->chunk_bytes);

		/* Prepare byte-swapped version of the data */
		gen_byteswap(in_data, sw_data,
			     (size_t) aix_wcd->cds->chunk_bytes);

		/* Write to device */
		if (aix_write_dev) {
			/* Always feed AIX sound driver big endian data */
			dev_data = be_data;

			if (!gen_write_chunk(aix_audio_desc, dev_data,
					(size_t) aix_wcd->cds->chunk_bytes)) {
				aix_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(aix_wsemid, LOCK);
				ret = FALSE;
				break;
			}

			/* Update volume and balance settings */
			aixaud_update();

			if ((aix_wcd->i->frm_played % FRAME_PER_SEC) == 0)
				aixaud_status();
		}

		/* Write to file */
		if (aix_write_file) {
			file_data = aix_file_be ? be_data : le_data;

			/* Open new track file if necessary */
			if (!app_data.cdda_trkfile) {
				fname = s->trkinfo[0].outfile;
			}
			else if (trk_idx != aix_wcd->i->trk_idx) {
				trk_idx = aix_wcd->i->trk_idx;

				if (aix_file_desc != NULL &&
				    !gen_close_file(aix_file_desc)) {
					aix_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(aix_wsemid, LOCK);
					ret = FALSE;
					break;
				}

				fname = s->trkinfo[trk_idx].outfile;
				aix_file_desc = gen_open_file(
					aix_wcd,
					fname,
					O_WRONLY | O_TRUNC | O_CREAT,
					(mode_t) aix_file_mode,
					app_data.cdda_filefmt,
					aix_wcd->i->trk_len
				);
				if (aix_file_desc == NULL) {
					aix_wcd->i->state = CDSTAT_COMPLETED;
					cdda_postsem(aix_wsemid, LOCK);
					ret = FALSE;
					break;
				}
			}

			if (!gen_write_chunk(aix_file_desc, file_data,
					(size_t) aix_wcd->cds->chunk_bytes)) {
				aix_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(aix_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		/* Pipe to program */
		if (aix_write_pipe) {
			pipe_data = aix_file_be ? be_data : le_data;

			if (!gen_write_chunk(aix_pipe_desc, pipe_data,
					(size_t) aix_wcd->cds->chunk_bytes)) {
				aix_wcd->i->state = CDSTAT_COMPLETED;
				cdda_postsem(aix_wsemid, LOCK);
				ret = FALSE;
				break;
			}
		}

		aix_wcd->i->frm_played += aix_wcd->cds->chunk_blocks;

		while (aix_wcd->i->state == CDSTAT_PAUSED)
			util_delayms(400);

		/* Update debug level */
		app_data.debug = aix_wcd->i->debug;
	}

	/* Wake up reader */
	cdda_postsem(aix_wsemid, ROOM);

	/* Free memory */
	MEM_FREE(le_data);
	MEM_FREE(be_data);

	return (ret);
}


/*
 * aix_winit
 *	Pre-playback support check function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Bitmask of supported features
 */
word32_t
aix_winit(void)
{
	return (CDDA_WRITEDEV | CDDA_WRITEFILE | CDDA_WRITEPIPE);
}


/*
 * aix_write
 *	Opens audio device/file, attaches shared memory and semaphores.
 *	Continuously reads data from shared memory and writes it
 *	to the audio device/file.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
aix_write(curstat_t *s)
{
	size_t	estlen;

	if (!PLAYMODE_IS_CDDA(app_data.play_mode)) {
		(void) fprintf(errfp, "aix_write: Nothing to do.\n");
		return FALSE;
	}

	aix_write_dev  = (bool_t) ((app_data.play_mode & PLAYMODE_CDDA) != 0);
	aix_write_file = (bool_t) ((app_data.play_mode & PLAYMODE_FILE) != 0);
	aix_write_pipe = (bool_t) ((app_data.play_mode & PLAYMODE_PIPE) != 0);

	/* Generic services initialization */
	gen_write_init();

	/* Allocate memory */
	aix_wcd = (cd_state_t *) MEM_ALLOC("cd_state_t", sizeof(cd_state_t));
	if (aix_wcd == NULL) {
		(void) fprintf(errfp, "aix_write: out of memory\n");
		return FALSE;
	}
	(void) memset(aix_wcd, 0, sizeof(cd_state_t));

	/* Initialize shared memory and semaphores */
	if ((aix_wsemid = cdda_initipc(aix_wcd)) < 0)
		return FALSE;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &aix_file_mode);
	/* Make sure file is at least accessible by user */
	aix_file_mode |= S_IRUSR | S_IWUSR;
	aix_file_mode &= ~(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);

	estlen = (aix_wcd->i->end_lba - aix_wcd->i->start_lba + 1) *
		 CDDA_BLKSZ;

	/* Set heartbeat */
	cdda_heartbeat(&aix_wcd->i->writer_hb, CDDA_HB_WRITER);

	if (aix_write_file || aix_write_pipe) {
		/* Open file and/or pipe */
		if (!app_data.cdda_trkfile && aix_write_file) {
			aix_file_desc = gen_open_file(
				aix_wcd,
				s->trkinfo[0].outfile,
				O_WRONLY | O_TRUNC | O_CREAT,
				(mode_t) aix_file_mode,
				app_data.cdda_filefmt,
				estlen
			);
			if (aix_file_desc == NULL)
				return FALSE;
		}

		if (aix_write_pipe) {
			aix_pipe_desc = gen_open_pipe(
				aix_wcd,
				app_data.pipeprog,
				&aix_pipe_pid,
				app_data.cdda_filefmt,
				estlen
			);
			if (aix_pipe_desc == NULL)
				return FALSE;

			DBGPRN(DBG_SND)(errfp,
					  "\naixaud_write: Pipe to [%s]: "
					  "chunk_blks=%d\n",
					  app_data.pipeprog,
					  aix_wcd->cds->chunk_blocks);
		}

		/* Set endian */
		aix_file_be = gen_filefmt_be(app_data.cdda_filefmt);
	}

	if (aix_write_dev) {
		/* Open audio device */
		if (!aixaud_open_dev())
			return FALSE;

		/* Configure audio */
		if (!aixaud_config())
			return FALSE;

		/* Initial settings */
		aixaud_update();
	}

	/* Do the writing */
	if (!aixaud_write(s))
		return FALSE;

	return TRUE;
}


/*
 * aix_wdone
 *	Post-playback cleanup function
 *
 * Args:
 *	killreader - whether to terminate the reader thread or process
 *
 * Return:
 *	Nothing.
 */
void
aix_wdone(bool_t killreader)
{
	DBGPRN(DBG_GEN)(errfp, "\naix_wdone: Cleaning up writer\n");

	if (aix_audio_desc != NULL)
		(void) aixaud_close_dev();

	if (aix_file_desc != NULL) {
		(void) gen_close_file(aix_file_desc);
		aix_file_desc = NULL;
	}

	if (aix_pipe_desc != NULL) {
		(void) gen_close_pipe(aix_pipe_desc);
		aix_pipe_desc = NULL;
	}

	cdda_yield();

	if (aix_pipe_pid > 0) {
		(void) kill(aix_pipe_pid, SIGTERM);
		aix_pipe_pid = -1;
	}

	if (aix_wcd != NULL) {
		if (aix_wcd->i != NULL) {
			if (killreader && aix_wcd->i->reader != (thid_t) 0) {
				cdda_kill(aix_wcd->i->reader, SIGTERM);
				aix_wcd->i->state = CDSTAT_COMPLETED;
			}

			/* Reset frames played counter */
			aix_wcd->i->frm_played = 0;
		}

		MEM_FREE(aix_wcd);
		aix_wcd = NULL;
	}

	aix_write_dev = aix_write_file = aix_write_pipe = FALSE;
}


/*
 * aix_winfo
 *	Append method identification to informational string.
 *
 * Args:
 *	str - The informational string
 *
 * Return:
 *	Nothing.
 */
void
aix_winfo(char *str)
{
	(void) strcat(str, "AIX audio driver\n");
}

#endif	/* CDDA_WR_AIX CDDA_SUPPORTED */

