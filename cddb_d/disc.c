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
static char     *_disc_c_ident_ = "@(#)disc.c	1.21 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbDisc_AddCredit
 *	Add a credit to a disc object
 */
/*ARGSUSED*/
CddbResult
CddbDisc_AddCredit(
	CddbDiscPtr	discp,
	CddbConstStr	id,
	CddbConstStr	name,
	CddbCreditPtr	*pval
)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;
	cddb_control_t	*cp;
	cddb_rolelist_t	*rlp;
	cddb_role_t	*rp;
	cddb_credit_t	*p;

	if (pval == NULL)
		return Cddb_E_INVALIDARG;

	if (discp == NULL || id == NULL || id[0] == '\0' ||
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

	cp = (cddb_control_t *) dp->control;
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
	p = dp->credits.credits;
	dp->credits.credits = p;
	dp->credits.count++;

	return Cddb_OK;
}


/*
 * CddbDisc_AddSegment
 *	Add a segment to a disc object
 */
/*ARGSUSED*/
CddbResult
CddbDisc_AddSegment(CddbDiscPtr discp, CddbConstStr name, CddbSegmentPtr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;
	cddb_segment_t	*p;

	if (pval == NULL)
		return Cddb_E_INVALIDARG;

	if (discp == NULL || name == NULL || name[0] == '\0') {
		*pval = NULL;
		return Cddb_E_INVALIDARG;
	}

	p = (cddb_segment_t *) MEM_ALLOC(
		"CddbSegment", sizeof(cddb_segment_t)
	);
	*pval = (CddbSegmentPtr) p;

	if (p == NULL)
		return CDDBTRNOutOfMemory;

	(void) memset(p, 0, sizeof(cddb_segment_t));
	(void) strcpy(p->objtype, "CddbSegment");

	p->name = (CddbStr) fcddb_strdup((char *) name);

	/* Add to list */
	p = dp->segments.segments;
	dp->segments.segments = p;
	dp->segments.count++;

	return Cddb_OK;
}


/*
 * CddbDisc_GetArtist
 *	Return disc artist
 */
CddbResult
CddbDisc_GetArtist(CddbDiscPtr discp, CddbStr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	*pval = (CddbStr) (dp->fullname.name == NULL ? "" : dp->fullname.name);
	return Cddb_OK;
}


/*
 * CddbDisc_GetArtistFullName
 *	Return disc artist full name object
 */
CddbResult
CddbDisc_GetArtistFullName(CddbDiscPtr discp, CddbFullNamePtr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	*pval = (CddbFullNamePtr) &dp->fullname;
	return Cddb_OK;
}


/*
 * CddbDisc_GetCertifier
 *	Return disc information certifier
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetCertifier(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetCompilation
 *	Return whether disc is a compilation
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetCompilation(CddbDiscPtr discp, CddbBoolean *pval)
{
	*pval = (CddbBoolean) 0;	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetCredit
 *	Return a disc credit
 */
CddbResult
CddbDisc_GetCredit(CddbDiscPtr discp, long item, CddbCreditPtr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;
	cddb_credit_t	*cp;
	long		i;

	for (i = 1, cp = dp->credits.credits; cp != NULL; i++, cp = cp->next) {
		if (i < item)
			continue;

		*pval = (CddbCreditPtr) cp;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_E_INVALIDARG;
}


/*
 * CddbDisc_GetGenreId
 *	Return disc genre ID
 */
CddbResult
CddbDisc_GetGenreId(CddbDiscPtr discp, CddbStr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	if (dp->genre != NULL)
		*pval = dp->genre->id;
	else
		*pval = "";	/* Shrug */

	return Cddb_OK;
}


/*
 * CddbDisc_GetLabel
 *	Return disc recording label
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetLabel(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetLanguageId
 *	Return disc primary language ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetLanguageId(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetLanguages
 *	Return alternative disc languages
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetLanguages(CddbDiscPtr discp, CddbLanguagesPtr *pval)
{
	*pval = NULL;			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetMediaId
 *	Return disc media ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetMediaId(CddbDiscPtr discp, CddbStr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	/* For CDDB1 we store the disc ID in the MediaId */
	if (dp->discid != NULL)
		*pval = (CddbStr) dp->discid;
	else
		*pval = "";	/* Shrug */

	return Cddb_OK;
}


/*
 * CddbDisc_GetMuiId
 *	Return disc MUI ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetMuiId(CddbDiscPtr discp, CddbStr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	/* For CDDB1 we store the category in the MuiId */
	if (dp->category != NULL)
		*pval = (CddbStr) dp->category;
	else
		*pval = "";	/* Shrug */

	return Cddb_OK;
}


/*
 * CddbDisc_GetNotes
 *	Return disc notes
 */
CddbResult
CddbDisc_GetNotes(CddbDiscPtr discp, CddbStr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	*pval = (CddbStr) (dp->notes == NULL ? "" : dp->notes);
	return Cddb_OK;
}


/*
 * CddbDisc_GetNumberInSet
 *	Return disc number in set
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetNumberInSet(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetNumCredits
 *	Return number of credits
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetNumCredits(CddbDiscPtr discp, long *pval)
{
	*pval = 0;
	return Cddb_OK;
}


/*
 * CddbDisc_GetNumSegments
 *	Return number of segments
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetNumSegments(CddbDiscPtr discp, long *pval)
{
	*pval = 0;
	return Cddb_OK;
}


/*
 * CddbDisc_GetNumTracks
 *	Return number of tracks
 */
CddbResult
CddbDisc_GetNumTracks(CddbDiscPtr discp, long *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	*pval = dp->tracks.count;
	return Cddb_OK;
}


/*
 * CddbDisc_GetRegionId
 *	Return disc region ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetRegionId(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetRevision
 *	Return disc info revision
 */
CddbResult
CddbDisc_GetRevision(CddbDiscPtr discp, CddbStr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	*pval = (CddbStr) (dp->revision == NULL ? "" : dp->revision);
	return Cddb_OK;
}


/*
 * CddbDisc_GetRevisionTag
 *	Return disc info revision tag
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetRevisionTag(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetSecondaryGenreId
 *	Return disc secondary genre ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetSecondaryGenreId(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetSegment
 *	Return a disc segment
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetSegment(CddbDiscPtr discp, long item, CddbSegmentPtr *pval)
{
	*pval = NULL;			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetTitle
 *	Return disc title
 */
CddbResult
CddbDisc_GetTitle(CddbDiscPtr discp, CddbStr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	*pval = (CddbStr) (dp->title == NULL ? "" : dp->title);
	return Cddb_OK;
}


/*
 * CddbDisc_GetTitleSort
 *	Return disc sort title
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetTitleSort(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetTitleThe
 *	Return disc sort title "The" string
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetTitleThe(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetTitleUId
 *	Return title unique ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetTitleUId(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetTotalInSet
 *	Return total discs in set
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetTotalInSet(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_GetTrack
 *	Return a disc track object
 */
CddbResult
CddbDisc_GetTrack(CddbDiscPtr discp, long item, CddbTrackPtr *pval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	*pval = (CddbTrackPtr) &dp->tracks.track[(int) (item - 1)];
	return Cddb_OK;
}


/*
 * CddbDisc_GetYear
 *	Return year object
 */
/*ARGSUSED*/
CddbResult
CddbDisc_GetYear(CddbDiscPtr discp, CddbStr *pval)
{
	*pval = "";			/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutArtist
 *	Set the disc artist name
 */
CddbResult
CddbDisc_PutArtist(CddbDiscPtr discp, CddbConstStr newval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	if (dp->fullname.name != NULL)
		MEM_FREE(dp->fullname.name);

	if (newval == NULL || newval[0] == '\0')
		dp->fullname.name = NULL;
	else
		dp->fullname.name = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbDisc_PutArtistFullName
 *	Set the disc artist full name information
 */
CddbResult
CddbDisc_PutArtistFullName(CddbDiscPtr discp, const CddbFullNamePtr newval)
{
	cddb_disc_t		*dp = (cddb_disc_t *) discp;
	cddb_fullname_t	*fnp = (cddb_fullname_t *) newval;

	if (newval == NULL)
		return Cddb_E_INVALIDARG;

	if (dp->fullname.name != NULL)
		MEM_FREE(dp->fullname.name);
	if (dp->fullname.lastname != NULL)
		MEM_FREE(dp->fullname.lastname);
	if (dp->fullname.firstname != NULL)
		MEM_FREE(dp->fullname.firstname);
	if (dp->fullname.the != NULL)
		MEM_FREE(dp->fullname.the);

	dp->fullname.name = (CddbStr) fcddb_strdup(fnp->name);
	dp->fullname.lastname = (CddbStr) fcddb_strdup(fnp->lastname);
	dp->fullname.firstname = (CddbStr) fcddb_strdup(fnp->firstname);
	dp->fullname.the = (CddbStr) fcddb_strdup(fnp->the);

	return Cddb_OK;
}


/*
 * CddbDisc_PutCompilation
 *	Set the disc compilation flag
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutCompilation(CddbDiscPtr discp, CddbBoolean newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutGenreId
 *	Set the disc genre ID
 */
CddbResult
CddbDisc_PutGenreId(CddbDiscPtr discp, CddbConstStr newval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	dp->genre = fcddb_genre_id2gp((cddb_control_t *) dp->control,
				       (char *) newval);
	return Cddb_OK;
}


/*
 * CddbDisc_PutLabel
 *	Set the disc recording label
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutLabel(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutLanguageId
 *	Set the disc language ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutLanguageId(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutNotes
 *	Set the disc notes
 */
CddbResult
CddbDisc_PutNotes(CddbDiscPtr discp, CddbConstStr newval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	if (dp->notes != NULL)
		MEM_FREE(dp->notes);

	if (newval == NULL || newval[0] == '\0')
		dp->notes = NULL;
	else
		dp->notes = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbDisc_PutNumberInSet
 *	Set the disc number in set
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutNumberInSet(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutRegionId
 *	Set the disc region ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutRegionId(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutRevision
 *	Set the disc information revision number
 */
CddbResult
CddbDisc_PutRevision(CddbDiscPtr discp, CddbConstStr newval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	if (dp->revision != NULL)
		MEM_FREE(dp->revision);

	if (newval == NULL || newval[0] == '\0')
		dp->revision = NULL;
	else
		dp->revision = fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbDisc_PutRevisionTag
 *	Set the disc information revision tag
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutRevisionTag(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutSecondaryGenreId
 *	Set the disc secondary genre ID
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutSecondaryGenreId(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutTitle
 *	Set the disc title
 */
CddbResult
CddbDisc_PutTitle(CddbDiscPtr discp, CddbConstStr newval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	if (dp->title != NULL)
		MEM_FREE(dp->title);

	if (newval == NULL || newval[0] == '\0')
		dp->title = NULL;
	else
		dp->title = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbDisc_PutTitleSort
 *	Set the disc sort title
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutTitleSort(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutTitleThe
 *	Set the disc sort title "The" string
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutTitleThe(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutToc
 *	Set the disc TOC
 */
CddbResult
CddbDisc_PutToc(CddbDiscPtr discp, CddbConstStr newval)
{
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	if (dp->toc != NULL)
		MEM_FREE(dp->toc);

	if (newval == NULL || newval[0] == '\0')
		dp->toc = NULL;
	else
		dp->toc = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbDisc_PutTotalInSet
 *	Set the total number of discs in set
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutTotalInSet(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbDisc_PutYear
 *	Set the year
 */
/*ARGSUSED*/
CddbResult
CddbDisc_PutYear(CddbDiscPtr discp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


