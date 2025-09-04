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
static char     *_genretree_c_ident_ = "@(#)genretree.c	1.11 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbGenreTree_GetCount
 *	Return the number of genre lists in the tree
 */
CddbResult
CddbGenreTree_GetCount(CddbGenreTreePtr gtreep, long *pval)
{
	cddb_genretree_t	*p = (cddb_genretree_t *) gtreep;

	*pval = p->count;
	return Cddb_OK;
}


/*
 * CddbGenreTree_GetMetaGenre
 *	Return a meta genre in the tree
 */
CddbResult
CddbGenreTree_GetMetaGenre(
	CddbGenreTreePtr	gtreep,
	long			item,
	CddbGenrePtr		*pval
)
{
	cddb_genretree_t	*p = (cddb_genretree_t *) gtreep;
	cddb_genre_t		*gp;
	long			i;

	for (i = 1, gp = p->genres; p != NULL; i++, gp = gp->nextmeta) {
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


/*
 * CddbGenreTree_GetSubGenreList
 *	Return a genre list in the tree
 */
CddbResult
CddbGenreTree_GetSubGenreList(
	CddbGenreTreePtr	gtreep,
	CddbConstStr		id,
	CddbGenreListPtr	*pval
)
{
	cddb_genretree_t	*p = (cddb_genretree_t *) gtreep;
	cddb_genrelist_t	*glp;
	cddb_genre_t		*gp;

	*pval = NULL;
	for (gp = p->genres; gp != NULL; gp = gp->nextmeta) {
		if (gp->id != NULL && strcmp(gp->id, id) == 0) {
			glp = (cddb_genrelist_t *) fcddb_obj_alloc(
				"CddbGenreList",
				sizeof(cddb_genrelist_t)
			);
			if (glp == NULL)
				return CDDBTRNOutOfMemory;

			glp->genres = gp->next;
			*pval = (CddbGenreListPtr) glp;
			break;
		}
	}
	return Cddb_OK;
}


