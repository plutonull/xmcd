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
static char     *_langlist_c_ident_ = "@(#)langlist.c	1.3 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbLanguageList_GetCount
 *	Return the number of languages in the list
 */
CddbResult
CddbLanguageList_GetCount(CddbLanguageListPtr llistp, long *pval)
{
	cddb_languagelist_t	*rlp = (cddb_languagelist_t *) llistp;

	*pval = rlp->count;
	return Cddb_OK;
}


/*
 * CddbLanguageList_GetLanguage
 *	Return a language in the list
 */
CddbResult
CddbLanguageList_GetLanguage(
	CddbLanguageListPtr	llistp,
	long			item,
	CddbLanguagePtr		*pval
)
{
	cddb_languagelist_t	*llp = (cddb_languagelist_t *) llistp;
	cddb_language_t		*lp;
	long			i;

	for (i = 1, lp = llp->languages; lp != NULL; i++, lp = lp->next) {
		if (i < item)
			continue;

		*pval = (CddbLanguagePtr) lp;
		return Cddb_OK;
	}

	*pval = NULL;
	return Cddb_OK;
}


