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
static char     *_urlmanager_c_ident_ = "@(#)urlmanager.c	1.20 03/12/12";
#endif

#include "fcddb.h"


/*
 * CddbURLManager_GetMenuURLs
 *	Return a list of menu URLS
 */
CddbResult
CddbURLManager_GetMenuURLs(
	CddbURLManagerPtr	urlmgrp,
	CddbDiscPtr		discp,
	CddbURLListPtr		*pval
)
{
	cddb_urlmanager_t	*mp = (cddb_urlmanager_t *) urlmgrp;
	cddb_disc_t		*dp = (cddb_disc_t *) discp;
	cddb_control_t		*cp;
	cddb_url_t		*up,
				*up2;

	cp = mp->control;
	if (dp == NULL)
		up = cp->urllist.urls;
	else
		up = dp->urllist.urls;

	for (; up != NULL; up = up->next) {
		if (up->type != NULL && strcmp(up->type, "menu") != 0)
			continue;

		if (up->href == NULL || up->displaytext == NULL)
			continue;	/* No HREF or display text */

		/* Don't use fcddb_obj_alloc here because we don't
		 * want CddbReleaseObject to free it.
		 */
		up2 = (cddb_url_t *) MEM_ALLOC("CddbURL", sizeof(cddb_url_t));
		if (up2 == NULL)
			break;
		(void) memset(up2, 0, sizeof(cddb_url_t));
		(void) strcpy(up2->objtype, "CddbURL");

		/* Copy contents */
		up2->type = (CddbStr) fcddb_strdup((char *) up->type);
		up2->href = (CddbStr) fcddb_strdup((char *) up->href);
		up2->displaytext =
			(CddbStr) fcddb_strdup((char *) up->displaytext);
		if (up->displaylink != NULL)
			up2->displaylink =
			    (CddbStr) fcddb_strdup((char *) up->displaylink);
		if (up->category != NULL)
			up2->category =
			    (CddbStr) fcddb_strdup((char *) up->category);
		if (up->description != NULL)
			up2->description =
			    (CddbStr) fcddb_strdup((char *) up->description);

		/* Add to list */
		up2->next = mp->urllist.urls;
		mp->urllist.urls = up2;
		mp->urllist.count++;
	}

	*pval = (CddbURLListPtr) &mp->urllist;
	return Cddb_OK;
}


/*
 * CddbURLManager_GotoURL
 *	Spawn browser to the specified URL
 */
/*ARGSUSED*/
CddbResult
CddbURLManager_GotoURL(
	CddbURLManagerPtr	urlmgrp,
	CddbURLPtr		urlp,
	CddbConstStr		rawurl
)
{
	cddb_url_t	*up = (cddb_url_t *) urlp;
	char		*cmd;
	int		ret;

	if (rawurl == NULL)
		rawurl = up->href;

	/* Invoke web browser to the specified URL */
	cmd = (char *) MEM_ALLOC("cmd", strlen(rawurl) + FILE_PATH_SZ + 4);
	if (cmd == NULL)
		return CDDBTRNOutOfMemory;

	(void) sprintf(cmd, "%s '%s'", CDDB_GOBROWSER, rawurl);

	ret = fcddb_runcmd(cmd);
	MEM_FREE(cmd);

#ifdef __VMS
	return ((ret == 0 || (ret & 0x1) == 1) ? Cddb_OK : Cddb_E_FAIL);
#else
	return ((ret == 0) ? Cddb_OK : Cddb_E_FAIL);
#endif
}


/*
 * CddbURLManager_SubmitURL
 *	Submit a URL to CDDB
 */
/*ARGSUSED*/
CddbResult
CddbURLManager_SubmitURL(
	CddbURLManagerPtr	urlmgrp,
	CddbDiscPtr		discp,
	CddbURLPtr		urlp
)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


