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
static char     *_gen_c_ident_ = "@(#)gen.c	1.18 04/01/14";
#endif

#include "fcddb.h"


STATIC int	obj_refcnt = 0;
STATIC bool_t	cddb_initted = FALSE;


/*
 * CddbInitialize
 *	Initialize CDDB library
 */
CddbResult
CddbInitialize(CddbControlPtr *cddb)
{
	cddb_control_t	*cp;

	cp = (cddb_control_t *) fcddb_obj_alloc(
		"CddbControl",
		sizeof(cddb_control_t)
	);
	if (cp == NULL)
		return CDDBTRNOutOfMemory;

	cp->magic = (word32_t) CDDB1_MAGIC;

	*cddb = (CddbControlPtr) cp;
	cddb_initted = TRUE;

	return ((CddbResult) cp->magic);
}


/*
 * CddbTerminate
 *	Shut down CDDB library
 */
CddbResult
CddbTerminate(CddbControlPtr cddb)
{
	cddb_control_t	*cp;

	cp = (cddb_control_t *) cddb;
	if (cp == NULL || cp->magic != CDDB1_MAGIC || cddb_initted == FALSE)
		return Cddb_E_INVALIDARG;

	fcddb_obj_free(cp);
	cddb_initted = FALSE;

	return Cddb_OK;
}


/*
 * CddbCreateObject
 *	Create a CDDB object
 */
void *
CddbCreateObject(CddbObjectType type)
{
	void	*p;

	switch (type) {
	case CddbFullNameType:
		p = fcddb_obj_alloc(
			"CddbFullName",
			sizeof(cddb_fullname_t)
		);
		break;
	case CddbURLType:
		p = fcddb_obj_alloc(
			"CddbURL",
			sizeof(cddb_url_t)
		);
		break;
	case CddbID3TagType:	/* Not supported in CDDB1 */
	default:
		p = NULL;
		break;
	}

	return (p);
}


/*
 * CddbReleaseObject
 *	Release a CDDB object
 */
int
CddbReleaseObject(void *objp)
{
	if (objp == NULL)
		return 0;

	fcddb_obj_free(objp);

	return (obj_refcnt);
}


/*
 * CddbGetErrorString
 *	Return an error message string associated with the supplied result
 *	code.
 */
void
CddbGetErrorString(CddbResult code, char *buf, int len)
{
	char	tmpbuf[40];

	if (buf == NULL || len == 0)
		return;

	/* This needs to be improved to return sensible messages */
	(void) sprintf(tmpbuf, "CDDB Error 0x%x", (int) code);
	(void) strncpy(buf, tmpbuf, len-1);
	buf[len-1] = '\0';
}


