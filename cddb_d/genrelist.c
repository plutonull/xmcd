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
static char     *_genrelist_c_ident_ = "@(#)genrelist.c	1.10 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbGenreList_GetCount
 *	Return the number of genres in list
 */
CddbResult
CddbGenreList_GetCount(CddbGenreListPtr glistp, long *pval)
{
	cddb_genrelist_t	*p = (cddb_genrelist_t *) glistp;
	cddb_genre_t		*gp;
	long			i;

	for (i = 0, gp = p->genres; gp != NULL; i++, gp = gp->next)
		;

	*pval = i;
	return Cddb_OK;
}


/*
 * CddbGenreList_GetGenre
 *	Return a genre in the list
 */
CddbResult
CddbGenreList_GetGenre(CddbGenreListPtr glistp, long item, CddbGenrePtr *pval)
{
	cddb_genrelist_t	*p = (cddb_genrelist_t *) glistp;
	cddb_genre_t		*gp;
	long			i;

	for (i = 1, gp = p->genres; gp != NULL; i++, gp = gp->next) {
		if (i == item)
			break;
	}
	if (i > item) {
		*pval = NULL;
		return Cddb_E_INVALIDARG;
	}

	*pval = (CddbGenrePtr) gp;
	return Cddb_OK;
}


