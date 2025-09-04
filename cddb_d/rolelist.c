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
static char     *_rolelist_c_ident_ = "@(#)rolelist.c	1.13 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbRoleList_GetCategoryRole
 *	Return the category role of a role list
 */
CddbResult
CddbRoleList_GetCategoryRole(CddbRoleListPtr rolistp, CddbRolePtr *pval)
{
	cddb_rolelist_t	*rlp = (cddb_rolelist_t *) rolistp;

	*pval = (CddbRolePtr) rlp->metarole;
	return Cddb_OK;
}


/*
 * CddbRoleList_GetCount
 *	Return the number of sub-roles in the list
 */
CddbResult
CddbRoleList_GetCount(CddbRoleListPtr rolistp, long *pval)
{
	cddb_rolelist_t	*rlp = (cddb_rolelist_t *) rolistp;

	*pval = rlp->count;
	return Cddb_OK;
}


/*
 * CddbRoleList_GetRole
 *	Return a role in the list
 */
CddbResult
CddbRoleList_GetRole(CddbRoleListPtr rolistp, long item, CddbRolePtr *pval)
{
	cddb_rolelist_t	*rlp = (cddb_rolelist_t *) rolistp;
	cddb_role_t	*rp;
	long		i;

	for (i = 1, rp = rlp->subroles; rp != NULL; i++, rp = rp->next) {
		if (i < item)
			continue;

		*pval = (CddbRolePtr) rp;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_OK;
}


