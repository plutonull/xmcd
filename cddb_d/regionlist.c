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
static char     *_regionlist_c_ident_ = "@(#)regionlist.c	1.14 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbRegionList_GetCount
 *	Return the number of regions in the list
 */
CddbResult
CddbRegionList_GetCount(CddbRegionListPtr rlistp, long *pval)
{
	cddb_regionlist_t	*rlp = (cddb_regionlist_t *) rlistp;

	*pval = rlp->count;
	return Cddb_OK;
}


/*
 * CddbRegionList_GetRegion
 *	Return a region in the list
 */
CddbResult
CddbRegionList_GetRegion(
	CddbRegionListPtr	rlistp,
	long			item,
	CddbRegionPtr		*pval
)
{
	cddb_regionlist_t	*rlp = (cddb_regionlist_t *) rlistp;
	cddb_region_t		*rp;
	long			i;

	for (i = 1, rp = rlp->regions; rp != NULL; i++, rp = rp->next) {
		if (i < item)
			continue;

		*pval = (CddbRegionPtr) rp;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_OK;
}


