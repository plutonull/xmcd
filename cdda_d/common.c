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
static char *_common_c_ident_ = "@(#)common.c	7.4 03/12/12";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"
#include "cdda_d/sysvipc.h"
#include "cdda_d/pthr.h"


extern appdata_t	app_data;
extern FILE		*errfp;
extern cdda_client_t	*cdda_clinfo;


/*
 * cdda_rd_corrjitter
 *	Correct for jitter. The olap array contains cdp->cds->search_bytes * 2
 *	of data and the last data written to the device is situated at
 *	olap[cdp->cds->search_bytes]. We try to match the last data read,
 *	which starts at data[cdp->cds->olap_bytes], around the center of olap.
 *	Once matched we may have to copy some data from olap into data
 *	(overshoot) or skip some of the audio in data (undershoot). The
 *	function returns a pointer into data indicating from where we should
 *	start writing.
 *	Used by the reader child process only.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Pointer into data
 */
byte_t *
cdda_rd_corrjitter(cd_state_t *cdp)
{
	int		idx,
			d;
	byte_t		*i,
			*j,
			*l;

	/* If jitter correction off, return */
	if (!cdp->i->jitter)
		return (cdp->cdb->data);

	/* Find a match in olap, if we can */
	i = cdp->cdb->olap;
	l = &cdp->cdb->olap[(cdp->cds->search_bytes << 1) - 1];
	j = &cdp->cdb->data[cdp->cds->olap_bytes];

	do {
		i = memchr(i, (int) *j, l - i);
		if (i != NULL) {
			if ((i - cdp->cdb->olap) % 2 == 0) {
				d = memcmp(j, i, cdp->cds->str_length);
				if (d == 0)
					break;
			}
			i++;
		}
	} while (i != NULL && i < l);

	/* Did we find a match? */
	if (i == NULL || i >= l) {
		DBGPRN(DBG_GEN)(errfp,
		    "cdda_rd_corrjitter: Could not correct for jitter.\n");
		idx = 0;
	}
	else {
		idx = i - cdp->cdb->olap - cdp->cds->search_bytes;
	}

	/* Update pointer */
	j -= idx;

	/* Match on RHS, copy some olap into data */
	if (idx > 0)
		(void) memcpy(j, cdp->cdb->olap + cdp->cds->search_bytes, idx);

	return (j);
}


/*
 * cdda_wr_setcurtrk
 *	Set the track index and track length based on the current playing
 *	location.
 *
 * Args:
 *	cdp - Pointer to the cd_state_t structure
 *
 * Return:
 *	Nothing.
 */
void
cdda_wr_setcurtrk(cd_state_t *cdp)
{
	int		i;
	sword32_t	lba;
	curstat_t	*s = cdda_clinfo->curstat_addr();
	static int	prev = -1;

	if (cdp->i->state != CDSTAT_PLAYING && cdp->i->state != CDSTAT_PAUSED)
		return;

	lba = cdp->i->start_lba + cdp->i->frm_played;

	i = cdp->i->trk_idx;
	if (i >= 0 &&
	    lba >= s->trkinfo[i].addr && lba < s->trkinfo[i+1].addr) {
		cdp->i->trk_idx = i;
		cdp->i->trk_len = CDDA_BLKSZ *
			(s->trkinfo[i+1].addr - s->trkinfo[i].addr + 1);
	}
	else for (i = 0; i < (int) s->tot_trks; i++) {
		if (lba >= s->trkinfo[i].addr &&
		    lba < s->trkinfo[i+1].addr) {
			cdp->i->trk_idx = i;
			cdp->i->trk_len = CDDA_BLKSZ *
			    (s->trkinfo[i+1].addr - s->trkinfo[i].addr + 1);
			break;
		}
	}

	if (i != prev && s->trkinfo[i].trkno != LEAD_OUT_TRACK) {
		int	shf;
		float	shfsec;

		shf = (int) (lba - s->trkinfo[i].addr);
		shfsec = (float) abs(shf) / (float) FRAME_PER_SEC;

		DBGPRN(DBG_GEN)(errfp,
			"\nCDDA: Playing track %d, start lba=0x%x "
			"shift=%d (%.3f sec)\n",
			(int) s->trkinfo[i].trkno, (int) lba, shf, shfsec);
		prev = i;
	}
}


