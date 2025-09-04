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
static char     *_region_c_ident_ = "@(#)region.c	1.14 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbRegion_GetId
 *	Return the region ID
 */
CddbResult
CddbRegion_GetId(CddbRegionPtr regionp, CddbStr *pval)
{
	cddb_region_t	*rp = (cddb_region_t *) regionp;

	*pval = (CddbStr) (rp->id == NULL ? "" : rp->id);
	return Cddb_OK;
}



/*
 * CddbRegion_GetName
 *	Return the region name
 */
CddbResult
CddbRegion_GetName(CddbRegionPtr regionp, CddbStr *pval)
{
	cddb_region_t	*rp = (cddb_region_t *) regionp;

	*pval = (CddbStr) (rp->name == NULL ? "" : rp->name);
	return Cddb_OK;
}


