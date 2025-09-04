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
static char     *_urllist_c_ident_ = "@(#)urllist.c	1.12 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbURLList_GetCount
 *	Return the number of URLs in the list
 */
CddbResult
CddbURLList_GetCount(CddbURLListPtr urllistp, long *pval)
{
	cddb_urllist_t	*ulp = (cddb_urllist_t *) urllistp;

	*pval = ulp->count;
	return Cddb_OK;
}


/*
 * CddbURLList_GetURL
 *	Return a URL in the list
 */
CddbResult
CddbURLList_GetURL(CddbURLListPtr urllistp, long item, CddbURLPtr *pval)
{
	cddb_urllist_t	*ulp = (cddb_urllist_t *) urllistp;
	cddb_url_t	*up;
	long	i;

	for (i = 1, up = ulp->urls; up != NULL; i++, up = up->next) {
		if (i < item)
			continue;

		*pval = (CddbURLPtr) up;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_E_INVALIDARG;
}


