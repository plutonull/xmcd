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
static char     *_credit_c_ident_ = "@(#)credit.c	1.15 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbCredit_GetFullName
 *	Get full name object within a credit
 */
CddbResult
CddbCredit_GetFullName(CddbCreditPtr credp, CddbFullNamePtr *pval)
{
	cddb_credit_t	*cp = (cddb_credit_t *) credp;

	*pval = (CddbFullNamePtr) &cp->fullname;
	return Cddb_OK;
}


/*
 * CddbCredit_GetId
 *	Get credit role ID
 */
CddbResult
CddbCredit_GetId(CddbCreditPtr credp, CddbStr *pval)
{
	cddb_credit_t	*cp = (cddb_credit_t *) credp;

	if (cp->role == NULL)
		return Cddb_E_INVALIDARG;

	*pval = (CddbStr) (cp->role->id == NULL ? "" : cp->role->id);
	return Cddb_OK;
}


/*
 * CddbCredit_GetName
 *	Get credit name
 */
CddbResult
CddbCredit_GetName(CddbCreditPtr credp, CddbStr *pval)
{
	cddb_credit_t	*cp = (cddb_credit_t *) credp;

	*pval = (CddbStr) (cp->fullname.name == NULL ? "" : cp->fullname.name);
	return Cddb_OK;
}


/*
 * CddbCredit_GetNotes
 *	Get credit notes
 */
CddbResult
CddbCredit_GetNotes(CddbCreditPtr credp, CddbStr *pval)
{
	cddb_credit_t	*cp = (cddb_credit_t *) credp;

	*pval = (CddbStr) (cp->notes == NULL ? "" : cp->notes);
	return Cddb_OK;
}


/*
 * CddbCredit_PutFullName
 *	Set the full name information within a credit
 */
CddbResult
CddbCredit_PutFullName(CddbCreditPtr credp, const CddbFullNamePtr newval)
{
	cddb_credit_t	*cp = (cddb_credit_t *) credp;
	cddb_fullname_t	*fnp = (cddb_fullname_t *) newval;

	if (newval == NULL)
		return Cddb_E_INVALIDARG;

	if (cp->fullname.name != NULL)
		MEM_FREE(cp->fullname.name);
	if (cp->fullname.lastname != NULL)
		MEM_FREE(cp->fullname.lastname);
	if (cp->fullname.firstname != NULL)
		MEM_FREE(cp->fullname.firstname);
	if (cp->fullname.the != NULL)
		MEM_FREE(cp->fullname.the);

	cp->fullname.name = (CddbStr) fcddb_strdup(fnp->name);
	cp->fullname.lastname = (CddbStr) fcddb_strdup(fnp->lastname);
	cp->fullname.firstname = (CddbStr) fcddb_strdup(fnp->firstname);
	cp->fullname.the = (CddbStr) fcddb_strdup(fnp->the);

	return Cddb_OK;
}


/*
 * CddbCredit_PutNotes
 *	Set credit notes
 */
CddbResult
CddbCredit_PutNotes(CddbCreditPtr credp, CddbConstStr newval)
{
	cddb_credit_t	*cp = (cddb_credit_t *) credp;

	if (cp->notes != NULL)
		MEM_FREE(cp->notes);

	cp->notes = (CddbStr) fcddb_strdup((char *) newval);
	return Cddb_OK;
}


