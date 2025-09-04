/*
 *   cddbkey1 - CDDB Interface Library for xmcd/cda
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
static char *_cddbkey1_c_ident_ = "@(#)cddbkey1.c	7.26 03/12/12";
#endif

#define XMCD_CDDB	/* Enable the correct include file in CDDB2API.h */

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"
#include "cddb_d/CDDB2API.h"


/*
 * cddb_ifver
 *	Return the CDDB interface version number
 *
 * Args:
 *	None.
 *
 * Return:
 *	The CDDB interface version (1 = CDDB1, 2 = CDDB2)
 */
int
cddb_ifver(void)
{
	return 1;
}


/*
 * cddb_setkey
 *	Register the client information with the CDDB library
 *
 * Args:
 *	cp - CDDB pointer from cdinfo_opencddb()
 *	clp - cdinfo client info structure pointer
 *	adp - Pointer to the app_data structure
 *	s - Pointer to the curstat_t structure
 *	errfp - FILE stream to debugging output
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
cddb_setkey(
	cdinfo_cddb_t	*cp,
	cdinfo_client_t	*clp,
	appdata_t	*adp,
	curstat_t	*s,
	FILE		*errfp
)
{
	CddbResult	ret;

	if (adp->debug & DBG_CDI)
		(void) fprintf(errfp, "CddbControl_SetClientInfo: ");
	ret = CddbControl_SetClientInfo(
		cp->ctrlp, XMCD_CLIENT_ID,
		(adp->debug & DBG_CDI) ? "debug" : "",
		VERSION_MAJ "." VERSION_MIN, clp->prog
	);
	if (adp->debug & DBG_CDI)
		(void) fprintf(errfp, "0x%lx\n", ret);

	return (ret == Cddb_OK || ret == Cddb_FALSE);
}


