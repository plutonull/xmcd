/*
 *   libcddb - CDDB Interface Library for xmcd/cda
 *
 *	This library implements an interface to access the "classic"
 *	CDDB1 services.
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
#ifndef lint
static char     *_track_c_ident_ = "@(#)track.c	1.18 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbTrack_AddCredit
 *	Add a credit to the track
 */
/*ARGSUSED*/
CddbResult
CddbTrack_AddCredit(
	CddbTrackPtr	trackp,
	CddbConstStr	id,
	CddbConstStr	name,
	CddbCreditPtr	*pval
)
{
	cddb_track_t	*tp = (cddb_track_t *) trackp;
	cddb_control_t	*cp;
	cddb_rolelist_t	*rlp;
	cddb_role_t	*rp;
	cddb_credit_t	*p;

	if (pval == NULL)
		return Cddb_E_INVALIDARG;

	if (trackp == NULL || id == NULL || id[0] == '\0' ||
	    name == NULL || name[0] == '\0') {
		*pval = NULL;
		return Cddb_E_INVALIDARG;
	}

	p = (cddb_credit_t *) MEM_ALLOC(
		"CddbCredit", sizeof(cddb_credit_t)
	);
	*pval = (CddbCreditPtr) p;

	if (p == NULL)
		return CDDBTRNOutOfMemory;
	
	(void) memset(p, 0, sizeof(cddb_credit_t));
	(void) strcpy(p->objtype, "CddbCredit");

	cp = (cddb_control_t *) tp->control;
	p->role = NULL;
	for (rlp = cp->roletree.rolelists; rlp != NULL; rlp = rlp->next) {
		for (rp = rlp->subroles; rp != NULL; rp = rp->next) {
			if (strcmp(rp->id, id) == 0) {
				p->role = rp;
				break;
			}
		}
		if (p->role != NULL)
			break;
	}
	p->fullname.name = (CddbStr) fcddb_strdup((char *) name);

	/* Add to list */
	p = tp->credits.credits;
	tp->credits.credits = p;
	tp->credits.count++;

	return Cddb_OK;
}


/*
 * CddbTrack_GetArtist
 *	Return the track artist
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetArtist(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetArtistFullName
 *	Return the track artist full name object
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetArtistFullName(CddbTrackPtr trackp, CddbFullNamePtr *pval)
{
	cddb_track_t	*tp = (cddb_track_t *) trackp;

	*pval = (CddbFullNamePtr) &tp->fullname;
	return Cddb_OK;
}


/*
 * CddbTrack_GetBeatsPerMinute
 *	Return the track BPM
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetBeatsPerMinute(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetCredit
 *	Return a track credit
 */
CddbResult
CddbTrack_GetCredit(
	CddbTrackPtr	trackp,
	long		item,
	CddbCreditPtr	*pval
)
{
	cddb_track_t	*tp = (cddb_track_t *) trackp;
	cddb_credit_t	*cp;
	long		i;

	for (i = 1, cp = tp->credits.credits; cp != NULL; i++, cp = cp->next) {
		if (i < item)
			continue;

		*pval = (CddbCreditPtr) cp;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_E_INVALIDARG;
}


/*
 * CddbTrack_GetGenreId
 *	Return track genre ID
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetGenreId(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetISRC
 *	Return track ISRC
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetISRC(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetLabel
 *	Return track recording label
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetLabel(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetNotes
 *	Return track notes
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetNotes(CddbTrackPtr trackp, CddbStr *pval)
{
	cddb_track_t	*tp = (cddb_track_t *) trackp;

	*pval = (CddbStr) (tp->notes == NULL ? "" : tp->notes);
	return Cddb_OK;
}


/*
 * CddbTrack_GetNumCredits
 *	Return number of credits in the track
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetNumCredits(CddbTrackPtr trackp, long *pval)
{
	*pval = 0;		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetSecondaryGenreId
 *	Return the track secondary genre ID
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetSecondaryGenreId(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetTitle
 *	Return the track title
 */
CddbResult
CddbTrack_GetTitle(CddbTrackPtr trackp, CddbStr *pval)
{
	cddb_track_t	*tp = (cddb_track_t *) trackp;

	*pval = (CddbStr) (tp->title == NULL ? "" : tp->title);
	return Cddb_OK;
}


/*
 * CddbTrack_GetTitleSort
 *	Return the track sort title
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetTitleSort(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetTitleThe
 *	Return the track sort title "The" string
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetTitleThe(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_GetYear
 *	Return the year
 */
/*ARGSUSED*/
CddbResult
CddbTrack_GetYear(CddbTrackPtr trackp, CddbStr *pval)
{
	*pval = "";		/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutArtist
 *	Set the track artist
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutArtist(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutArtistFullName
 *	Set the track artist full name information
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutArtistFullName(CddbTrackPtr trackp, const CddbFullNamePtr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutBeatsPerMinute
 *	Set the track BPM
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutBeatsPerMinute(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutGenreId
 *	Set the track genre ID
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutGenreId(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutISRC
 *	Set the track ISRC
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutISRC(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutLabel
 *	Set the track recording label
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutLabel(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutNotes
 *	Set the track notes
 */
CddbResult
CddbTrack_PutNotes(CddbTrackPtr trackp, CddbConstStr newval)
{
	cddb_track_t	*tp = (cddb_track_t *) trackp;

	if (tp->notes != NULL)
		MEM_FREE(tp->notes);

	if (newval == NULL || newval[0] == '\0')
		tp->notes = NULL;
	else
		tp->notes = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbTrack_PutSecondaryGenreId
 *	Set the track secondary genre ID
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutSecondaryGenreId(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutTitle
 *	Set the track title
 */
CddbResult
CddbTrack_PutTitle(CddbTrackPtr trackp, CddbConstStr newval)
{
	cddb_track_t	*tp = (cddb_track_t *) trackp;

	if (tp->title != NULL)
		MEM_FREE(tp->title);

	if (newval == NULL || newval[0] == '\0')
		tp->title = NULL;
	else
		tp->title = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbTrack_PutTitleSort
 *	Set the track sort title
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutTitleSort(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutTitleThe
 *	Set the track sort title "The" string
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutTitleThe(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbTrack_PutYear
 *	Set the year
 */
/*ARGSUSED*/
CddbResult
CddbTrack_PutYear(CddbTrackPtr trackp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


