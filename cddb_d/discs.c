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
static char     *_discs_c_ident_ = "@(#)discs.c	1.10 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbDiscs_GetCount
 *	Return the number of discs in the object
 */
CddbResult
CddbDiscs_GetCount(CddbDiscsPtr discsp, long *pval)
{
	cddb_discs_t	*dsp = (cddb_discs_t *) discsp;

	*pval = dsp->count;
	return Cddb_OK;
}


/*
 * CddbDiscs_GetDisc
 *	Return a disc object
 */
CddbResult
CddbDiscs_GetDisc(CddbDiscsPtr discsp, long item, CddbDiscPtr *pval)
{
	cddb_discs_t	*dsp = (cddb_discs_t *) discsp;
	cddb_disc_t	*dp;
	int		i;

	if (item > dsp->count)
		return Cddb_E_INVALIDARG;

	for (i = 1, dp = dsp->discs; dp != NULL; i++, dp = dp->next) {
		if (i == item) {
			*pval = (CddbDiscPtr) dp;
			return Cddb_OK;
		}
	}

	*pval = NULL;
	return Cddb_E_INVALIDARG;
}


