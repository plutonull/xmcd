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
static char     *_userinfo_c_ident_ = "@(#)userinfo.c	1.14 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbUserInfo_GetAge
 *	Return age
 */
CddbResult
CddbUserInfo_GetAge(CddbUserInfoPtr uinfop, CddbStr *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = (CddbStr) (p->age == NULL ? "" : p->age);
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetAllowEmail
 *	Return allow email flag
 */
CddbResult
CddbUserInfo_GetAllowEmail(CddbUserInfoPtr uinfop, CddbBoolean *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = p->allowemail;
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetAllowStats
 *	Return allow stats flag
 */
CddbResult
CddbUserInfo_GetAllowStats(CddbUserInfoPtr uinfop, CddbBoolean *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = p->allowstats;
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetEmailAddress
 *	Return email address
 */
CddbResult
CddbUserInfo_GetEmailAddress(CddbUserInfoPtr uinfop, CddbStr *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = (CddbStr) (p->emailaddress == NULL ? "" : p->emailaddress);
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetPasswordHint
 *	Return password hint
 */
CddbResult
CddbUserInfo_GetPasswordHint(CddbUserInfoPtr uinfop, CddbStr *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = (CddbStr) (p->passwordhint == NULL ? "" : p->passwordhint);
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetPostalCode
 *	Return postal code
 */
CddbResult
CddbUserInfo_GetPostalCode(CddbUserInfoPtr uinfop, CddbStr *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = (CddbStr) (p->postalcode == NULL ? "" : p->postalcode);
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetRegionId
 *	Return region ID
 */
CddbResult
CddbUserInfo_GetRegionId(CddbUserInfoPtr uinfop, CddbStr *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = (CddbStr) (p->regionid == NULL ? "" : p->regionid);
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetSex
 *	Return gender
 */
CddbResult
CddbUserInfo_GetSex(CddbUserInfoPtr uinfop, CddbStr *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = (CddbStr) (p->sex == NULL ? "" : p->sex);
	return Cddb_OK;
}


/*
 * CddbUserInfo_GetUserHandle
 *	Return user handle
 */
CddbResult
CddbUserInfo_GetUserHandle(CddbUserInfoPtr uinfop, CddbStr *pval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	*pval = (CddbStr) (p->userhandle == NULL ? "" : p->userhandle);
	return Cddb_OK;
}


/*
 * CddbUserInfo_PutAge
 *	Set the age
 */
CddbResult
CddbUserInfo_PutAge(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->age != NULL)
		MEM_FREE(p->age);

	if (newval == NULL || newval[0] == '\0')
		p->age = NULL;
	else
		p->age = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbUserInfo_PutAllowEmail
 *	Set the allow email flag
 */
CddbResult
CddbUserInfo_PutAllowEmail(CddbUserInfoPtr uinfop, CddbBoolean newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	p->allowemail = newval;
	return Cddb_OK;
}


/*
 * CddbUserInfo_PutAllowStats
 *	Set the allow stats flag
 */
CddbResult
CddbUserInfo_PutAllowStats(CddbUserInfoPtr uinfop, CddbBoolean newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	p->allowstats = newval;
	return Cddb_OK;
}


/*
 * CddbUserInfo_PutEmailAddress
 *	Set the email address
 */
CddbResult
CddbUserInfo_PutEmailAddress(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->emailaddress != NULL)
		MEM_FREE(p->emailaddress);

	if (newval == NULL || newval[0] == '\0')
		p->emailaddress = NULL;
	else
		p->emailaddress = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbUserInfo_PutPassword
 *	Set the CDDB password
 */
CddbResult
CddbUserInfo_PutPassword(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->password != NULL)
		MEM_FREE(p->password);

	if (newval == NULL || newval[0] == '\0')
		p->password = NULL;
	else
		p->password = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbUserInfo_PutPasswordHint
 *	Set the password hint
 */
CddbResult
CddbUserInfo_PutPasswordHint(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->passwordhint != NULL)
		MEM_FREE(p->passwordhint);

	if (newval == NULL || newval[0] == '\0')
		p->passwordhint = NULL;
	else
		p->passwordhint = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbUserInfo_PutPostalCode
 *	Set the postal code
 */
CddbResult
CddbUserInfo_PutPostalCode(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->postalcode != NULL)
		MEM_FREE(p->postalcode);

	if (newval == NULL || newval[0] == '\0')
		p->postalcode = NULL;
	else
		p->postalcode = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbUserInfo_PutRegionId
 *	Set the region ID
 */
CddbResult
CddbUserInfo_PutRegionId(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->regionid != NULL)
		MEM_FREE(p->regionid);

	if (newval == NULL || newval[0] == '\0')
		p->regionid = NULL;
	else
		p->regionid = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbUserInfo_PutSex
 *	Set the gender
 */
CddbResult
CddbUserInfo_PutSex(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->sex != NULL)
		MEM_FREE(p->sex);

	if (newval == NULL || newval[0] == '\0')
		p->sex = NULL;
	else
		p->sex = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbUserInfo_PutUserHandle
 *	Set the user handle
 */
CddbResult
CddbUserInfo_PutUserHandle(CddbUserInfoPtr uinfop, CddbConstStr newval)
{
	cddb_userinfo_t	*p = (cddb_userinfo_t *) uinfop;

	if (p->userhandle != NULL)
		MEM_FREE(p->userhandle);

	if (newval == NULL || newval[0] == '\0')
		p->userhandle = NULL;
	else
		p->userhandle = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


