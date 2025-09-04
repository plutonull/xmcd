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
static char     *_fullname_c_ident_ = "@(#)fullname.c	1.12 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbFullName_GetFirstName
 *	Return first name of the full name object
 */
CddbResult
CddbFullName_GetFirstName(CddbFullNamePtr fnamep, CddbStr *pval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	*pval = (fnp->firstname == NULL) ? "" : fnp->firstname;
	return Cddb_OK;
}


/*
 * CddbFullName_GetLastName
 *	Return last name of the full name object
 */
CddbResult
CddbFullName_GetLastName(CddbFullNamePtr fnamep, CddbStr *pval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	*pval = (fnp->lastname == NULL) ? "" : fnp->lastname;
	return Cddb_OK;
}


/*
 * CddbFullName_GetName
 *	Return display name of the full name object
 */
CddbResult
CddbFullName_GetName(CddbFullNamePtr fnamep, CddbStr *pval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	*pval = (fnp->name == NULL) ? "" : fnp->name;
	return Cddb_OK;
}


/*
 * CddbFullName_GetThe
 *	Return "The" string of the full name object
 */
CddbResult
CddbFullName_GetThe(CddbFullNamePtr fnamep, CddbStr *pval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	*pval = (fnp->the == NULL) ? "" : fnp->the;
	return Cddb_OK;
}


/*
 * CddbFullName_PutFirstName
 *	Set the first name of the full name object
 */
CddbResult
CddbFullName_PutFirstName(CddbFullNamePtr fnamep, CddbConstStr newval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	if (fnp->firstname != NULL)
		MEM_FREE(fnp->firstname);

	if (newval == NULL || newval[0] == '\0')
		fnp->firstname = NULL;
	else
		fnp->firstname = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbFullName_PutLastName
 *	Set the last name of the full name object
 */
CddbResult
CddbFullName_PutLastName(CddbFullNamePtr fnamep, CddbConstStr newval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	if (fnp->lastname != NULL)
		MEM_FREE(fnp->lastname);

	if (newval == NULL || newval[0] == '\0')
		fnp->lastname = NULL;
	else
		fnp->lastname = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbFullName_PutName
 *	Set the display name of the full name object
 */
CddbResult
CddbFullName_PutName(CddbFullNamePtr fnamep, CddbConstStr newval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	if (fnp->name != NULL)
		MEM_FREE(fnp->name);

	if (newval == NULL || newval[0] == '\0')
		fnp->name = NULL;
	else
		fnp->name = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbFullName_PutThe
 *	Set the "The" string of the full name object
 */
CddbResult
CddbFullName_PutThe(CddbFullNamePtr fnamep, CddbConstStr newval)
{
	cddb_fullname_t	*fnp = (cddb_fullname_t *) fnamep;

	if (fnp->the != NULL)
		MEM_FREE(fnp->the);

	if (newval == NULL || newval[0] == '\0')
		fnp->the = NULL;
	else
		fnp->the = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


