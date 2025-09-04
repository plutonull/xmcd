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
static char     *_segment_c_ident_ = "@(#)segment.c	1.16 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbSegment_AddCredit
 *	Add a credit to the segment
 */
CddbResult
CddbSegment_AddCredit(
	CddbSegmentPtr	segp,
	CddbConstStr	id,
	CddbConstStr	name,
	CddbCreditPtr	*pval
)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;
	cddb_control_t	*cp;
	cddb_rolelist_t	*rlp;
	cddb_role_t	*rp;
	cddb_credit_t	*p;

	if (pval == NULL)
		return Cddb_E_INVALIDARG;

	if (segp == NULL || id == NULL || id[0] == '\0' ||
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

	cp = (cddb_control_t *) sp->control;
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
	p = sp->credits.credits;
	sp->credits.credits = p;
	sp->credits.count++;

	return Cddb_OK;
}


/*
 * CddbSegment_GetCredit
 *	Return a credit of the segment
 */
CddbResult
CddbSegment_GetCredit(
	CddbSegmentPtr	segp,
	long		item,
	CddbCreditPtr	*pval
)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;
	cddb_credit_t	*cp;
	long		i;

	for (i = 1, cp = sp->credits.credits; cp != NULL; i++, cp = cp->next) {
		if (i < item)
			continue;

		*pval = (CddbCreditPtr) cp;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_E_INVALIDARG;
}


/*
 * CddbSegment_GetEndFrame
 *	Return the segment end frame
 */
CddbResult
CddbSegment_GetEndFrame(CddbSegmentPtr segp, CddbStr *pval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	*pval = (CddbStr) (sp->endframe == NULL ? "" : sp->endframe);
	return Cddb_OK;
}


/*
 * CddbSegment_GetEndTrack
 *	Return the segment end track
 */
CddbResult
CddbSegment_GetEndTrack(CddbSegmentPtr segp, CddbStr *pval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	*pval = (CddbStr) (sp->endtrack == NULL ? "" : sp->endtrack);
	return Cddb_OK;
}


/*
 * CddbSegment_GetName
 *	Return the segment name
 */
CddbResult
CddbSegment_GetName(CddbSegmentPtr segp, CddbStr *pval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	*pval = (CddbStr) (sp->name == NULL ? "" : sp->name);
	return Cddb_OK;
}


/*
 * CddbSegment_GetNotes
 *	Return the segment notes
 */
CddbResult
CddbSegment_GetNotes(CddbSegmentPtr segp, CddbStr *pval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	*pval = (CddbStr) (sp->notes == NULL ? "" : sp->notes);
	return Cddb_OK;
}


/*
 * CddbSegment_GetNumCredits
 *	Return the number of segment credits
 */
/*ARGSUSED*/
CddbResult
CddbSegment_GetNumCredits(CddbSegmentPtr segp, long *pval)
{
	*pval = 0;	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbSegment_GetStartFrame
 *	Return the segment start frame
 */
CddbResult
CddbSegment_GetStartFrame(CddbSegmentPtr segp, CddbStr *pval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	*pval = (CddbStr) (sp->startframe == NULL ? "" : sp->startframe);
	return Cddb_OK;
}


/*
 * CddbSegment_GetStartTrack
 *	Return the segment start track
 */
CddbResult
CddbSegment_GetStartTrack(CddbSegmentPtr segp, CddbStr *pval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	*pval = (CddbStr) (sp->starttrack == NULL ? "" : sp->starttrack);
	return Cddb_OK;
}


/*
 * CddbSegment_PutEndFrame
 *	Set the segment end frame
 */
CddbResult
CddbSegment_PutEndFrame(CddbSegmentPtr segp, CddbConstStr newval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	if (sp->endframe != NULL)
		MEM_FREE(sp->endframe);

	if (newval == NULL || newval[0] == '\0')
		sp->endframe = NULL;
	else
		sp->endframe = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbSegment_PutEndTrack
 *	Set the segment end track
 */
CddbResult
CddbSegment_PutEndTrack(CddbSegmentPtr segp, CddbConstStr newval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	if (sp->endtrack != NULL)
		MEM_FREE(sp->endtrack);

	if (newval == NULL || newval[0] == '\0')
		sp->endtrack = NULL;
	else
		sp->endtrack = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbSegment_PutNotes
 *	Set the segment notes
 */
CddbResult
CddbSegment_PutNotes(CddbSegmentPtr segp, CddbConstStr newval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	if (sp->notes != NULL)
		MEM_FREE(sp->notes);

	if (newval == NULL || newval[0] == '\0')
		sp->notes = NULL;
	else
		sp->notes = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbSegment_PutStartFrame
 *	Set the segment start frame
 */
CddbResult
CddbSegment_PutStartFrame(CddbSegmentPtr segp, CddbConstStr newval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	if (sp->startframe != NULL)
		MEM_FREE(sp->startframe);

	if (newval == NULL || newval[0] == '\0')
		sp->startframe = NULL;
	else
		sp->startframe = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbSegment_PutStartTrack
 *	Set the segment start track
 */
CddbResult
CddbSegment_PutStartTrack(CddbSegmentPtr segp, CddbConstStr newval)
{
	cddb_segment_t	*sp = (cddb_segment_t *) segp;

	if (sp->starttrack != NULL)
		MEM_FREE(sp->starttrack);

	if (newval == NULL || newval[0] == '\0')
		sp->starttrack = NULL;
	else
		sp->starttrack = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


