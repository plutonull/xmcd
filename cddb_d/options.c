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
static char     *_options_c_ident_ = "@(#)options.c	1.14 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbOptions_PutLocalCacheFlags
 *	Set the local cache flags
 */
CddbResult
CddbOptions_PutLocalCacheFlags(CddbOptionsPtr optionsp, long newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	op->localcacheflags = newval;
	return Cddb_OK;
}


/*
 * CddbOptions_PutProxyPassword
 *	Set the proxy password
 */
CddbResult
CddbOptions_PutProxyPassword(CddbOptionsPtr optionsp, CddbConstStr newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	if (op->proxypassword != NULL)
		MEM_FREE(op->proxypassword);

	if (newval == NULL || newval[0] == '\0')
		op->proxypassword = NULL;
	else
		op->proxypassword = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbOptions_PutProxyServer
 *	Set the proxy server name
 */
CddbResult
CddbOptions_PutProxyServer(CddbOptionsPtr optionsp, CddbConstStr newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	if (op->proxyserver != NULL)
		MEM_FREE(op->proxyserver);

	if (newval == NULL || newval[0] == '\0')
		op->proxyserver = NULL;
	else
		op->proxyserver = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbOptions_PutProxyServerPort
 *	Set the proxy server port
 */
CddbResult
CddbOptions_PutProxyServerPort(CddbOptionsPtr optionsp, long newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	op->proxyport = newval;
	return Cddb_OK;
}


/*
 * CddbOptions_PutProxyUserName
 *	Set the proxy user name
 */
CddbResult
CddbOptions_PutProxyUserName(CddbOptionsPtr optionsp, CddbConstStr newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	if (op->proxyusername != NULL)
		MEM_FREE(op->proxyusername);

	if (newval == NULL || newval[0] == '\0')
		op->proxyusername = NULL;
	else
		op->proxyusername = (CddbStr) fcddb_strdup((char *) newval);

	return Cddb_OK;
}


/*
 * CddbOptions_PutServerTimeout
 *	Set the server timeout interval
 */
CddbResult
CddbOptions_PutServerTimeout(CddbOptionsPtr optionsp, long newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	op->servertimeout = newval;
	return Cddb_OK;
}


/*
 * CddbOptions_PutTestSubmitMode
 *	Set the test submit mode flag
 */
CddbResult
CddbOptions_PutTestSubmitMode(CddbOptionsPtr optionsp, CddbBoolean newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	op->testsubmitmode = newval;
	return Cddb_OK;
}


/*
 * CddbOptions_PutLocalCacheTimeout
 *	Set the local cache timeout interval
 */
CddbResult
CddbOptions_PutLocalCacheTimeout(CddbOptionsPtr optionsp, long newval)
{
	cddb_options_t	*op = (cddb_options_t *) optionsp;

	op->localcachetimeout = newval;
	return Cddb_OK;
}


