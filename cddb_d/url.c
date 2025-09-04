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
static char     *_url_c_ident_ = "@(#)url.c	1.14 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbURL_GetCategory
 *	Return the URL category
 */
CddbResult
CddbURL_GetCategory(CddbURLPtr urlp, CddbStr *pval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	*pval = (CddbStr) (up->category == NULL ? "" : up->category);
	return Cddb_OK;
}


/*
 * CddbURL_GetDescription
 *	Return the URL description
 */
CddbResult
CddbURL_GetDescription(CddbURLPtr urlp, CddbStr *pval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	*pval = (CddbStr) (up->description == NULL ? "" : up->description);
	return Cddb_OK;
}


/*
 * CddbURL_GetDisplayLink
 *	Return the URL display link
 */
CddbResult
CddbURL_GetDisplayLink(CddbURLPtr urlp, CddbStr *pval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	*pval = (CddbStr) (up->displaylink == NULL ? "" : up->displaylink);
	return Cddb_OK;
}


/*
 * CddbURL_GetDisplayText
 *	Return the URL display text
 */
CddbResult
CddbURL_GetDisplayText(CddbURLPtr urlp, CddbStr *pval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	*pval = (CddbStr) (up->displaytext == NULL ? "" : up->displaytext);
	return Cddb_OK;
}


/*
 * CddbURL_GetHref
 *	Return the URL HREF
 */
CddbResult
CddbURL_GetHref(CddbURLPtr urlp, CddbStr *pval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	*pval = (CddbStr) (up->href == NULL ? "" : up->href);
	return Cddb_OK;
}


/*
 * CddbURL_GetSize
 *	Return the display size
 */
/*ARGSUSED*/
CddbResult
CddbURL_GetSize(CddbURLPtr urlp, CddbStr *pval)
{
	*pval = "";	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbURL_GetType
 *	Return the URL type
 */
CddbResult
CddbURL_GetType(CddbURLPtr urlp, CddbStr *pval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	*pval = (CddbStr) (up->type == NULL ? "" : up->type);
	return Cddb_OK;
}


/*
 * CddbURL_GetWeight
 *	Return the display weight
 */
/*ARGSUSED*/
CddbResult
CddbURL_GetWeight(CddbURLPtr urlp, CddbStr *pval)
{
	*pval = "";	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbURL_PutCategory
 *	Set the URL category
 */
CddbResult
CddbURL_PutCategory(CddbURLPtr urlp, CddbConstStr newval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	if (up->category != NULL)
		MEM_FREE(up->category);

	if (newval == NULL || newval[0] == '\0')
		up->category = NULL;
	else
		up->category = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbURL_PutDescription
 *	Set the URL description
 */
CddbResult
CddbURL_PutDescription(CddbURLPtr urlp, CddbConstStr newval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	if (up->description != NULL)
		MEM_FREE(up->description);

	if (newval == NULL || newval[0] == '\0')
		up->description = NULL;
	else
		up->description = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbURL_PutDisplayLink
 *	Set the URL display link
 */
CddbResult
CddbURL_PutDisplayLink(CddbURLPtr urlp, CddbConstStr newval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	if (up->displaylink != NULL)
		MEM_FREE(up->displaylink);

	if (newval == NULL || newval[0] == '\0')
		up->displaylink = NULL;
	else
		up->displaylink = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbURL_PutDisplayText
 *	Set the URL display text
 */
CddbResult
CddbURL_PutDisplayText(CddbURLPtr urlp, CddbConstStr newval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	if (up->displaytext != NULL)
		MEM_FREE(up->displaytext);

	if (newval == NULL || newval[0] == '\0')
		up->displaytext = NULL;
	else
		up->displaytext = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbURL_PutHref
 *	Set the URL HREF
 */
CddbResult
CddbURL_PutHref(CddbURLPtr urlp, CddbConstStr newval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	if (up->href != NULL)
		MEM_FREE(up->href);

	if (newval == NULL || newval[0] == '\0')
		up->href = NULL;
	else
		up->href = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbURL_PutSize
 *	Set the display size
 */
/*ARGSUSED*/
CddbResult
CddbURL_PutSize(CddbURLPtr urlp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbURL_PutType
 *	Set the URL type
 */
CddbResult
CddbURL_PutType(CddbURLPtr urlp, CddbConstStr newval)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;

	if (up->type != NULL)
		MEM_FREE(up->type);

	if (newval == NULL || newval[0] == '\0')
		up->type = NULL;
	else
		up->type = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbURL_PutWeight
 *	Set the display weight
 */
/*ARGSUSED*/
CddbResult
CddbURL_PutWeight(CddbURLPtr urlp, CddbConstStr newval)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


