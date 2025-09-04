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
static char     *_roletree_c_ident_ = "@(#)roletree.c	1.13 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbRoleTree_GetCount
 *	Return the number of role lists in the tree
 */
CddbResult
CddbRoleTree_GetCount(CddbRoleTreePtr rotreep, long *pval)
{
	cddb_roletree_t	*rtp = (cddb_roletree_t *) rotreep;

	*pval = rtp->count;
	return Cddb_OK;
}


/*
 * CddbRoleTree_GetRoleList
 *	Return a role list in the tree
 */
CddbResult
CddbRoleTree_GetRoleList(
	CddbRoleTreePtr rotreep,
	long		item,
	CddbRoleListPtr	*pval)
{
	cddb_roletree_t	*rtp = (cddb_roletree_t *) rotreep;
	cddb_rolelist_t	*rlp;
	long		i;

	for (i = 1, rlp = rtp->rolelists; rlp != NULL; i++, rlp = rlp->next) {
		if (i < item)
			continue;

		*pval = (CddbRoleListPtr) rlp;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_OK;
}


