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
static char	*_control_c_ident_ = "@(#)control.c	1.39 04/03/11";
#endif

#include "fcddb.h"
#include "common_d/version.h"


extern bool_t		fcddb_debug;
extern FILE		*fcddb_errfp;


/* Scratch array for the starting frame offset of each track */
STATIC unsigned int	trkframes[MAXTRACK];


/*
 * CddbControl_Initialize
 *	Initialize CDDB control
 */
/*ARGSUSED*/
CddbResult
CddbControl_Initialize(CddbControlPtr ctrlp, long unused, CDDBCacheFlags flags)
{
	int		i;
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	char		cacheroot[FILE_PATH_SZ];

	if (cp->magic != CDDB1_MAGIC)
		return Cddb_E_INVALIDARG;
	if (cp->clientid == NULL)
		return CDDBTRNNoUserInfo;

	/* Initialize structures */
	(void) strcpy(cp->options.objtype, "CddbOptions");
	(void) strcpy(cp->userinfo.objtype, "CddbUserInfo");
	(void) strcpy(cp->regionlist.objtype, "CddbRegionList");
	(void) strcpy(cp->languagelist.objtype, "CddbLanguageList");
	(void) strcpy(cp->genretree.objtype, "CddbGenreTree");
	(void) strcpy(cp->roletree.objtype, "CddbRoleTree");
	(void) strcpy(cp->urllist.objtype, "CddbURLList");
	(void) strcpy(cp->discs.objtype, "CddbDiscs");
	(void) strcpy(cp->disc.objtype, "CddbDisc");
	(void) strcpy(cp->disc.fullname.objtype, "CddbFullName");
	(void) strcpy(cp->disc.tracks.objtype, "CddbTracks");

	/* Set up back pointers */
	cp->disc.control = (void *) cp;
	for (i = 0; i < MAXTRACK; i++)
		cp->disc.tracks.track[i].control = (void *) cp;

	/* Set up some defaults */
	cp->options.proxyport = 80;
	cp->options.servertimeout = 60000;
#ifdef __VMS
	(void) sprintf(cacheroot, "%s.cddb2.%s",
		       fcddb_homedir(), cp->clientid);
#else
	(void) sprintf(cacheroot, "%s/.cddb2/%s",
		       fcddb_homedir(), cp->clientid);
#endif
	cp->options.localcachepath = (CddbStr) fcddb_strdup(cacheroot);
	cp->options.localcachetimeout = 7;
	cp->options.localcacheflags = (long) flags;

	/* Get user name and host name */
	cp->userinfo.userhandle = (CddbStr) fcddb_strdup(fcddb_username());
	cp->hostname = (CddbStr) fcddb_strdup(fcddb_hostname());

	/* General library initializations */
	return (fcddb_init(cp));
}


/*
 * CddbControl_Shutdown
 *	Shutdown CDDB control
 */
CddbResult
CddbControl_Shutdown(CddbControlPtr ctrlp)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	/* General library shutdown */
	return (fcddb_halt(cp));
}


/*
 * CddbControl_FlushLocalCache
 *	Flush local cache
 */
CddbResult
CddbControl_FlushLocalCache(CddbControlPtr ctrlp, CDDBFlushFlags flags)
{
	return fcddb_flush_cddb((cddb_control_t *) ctrlp, flags);
}


/*
 * CddbControl_GetDiscInfo
 *	Return disc object given the mediaid and muiid
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetDiscInfo(
	CddbControlPtr	ctrlp,
	CddbConstStr	mediaid,
	CddbConstStr	muiid,
	CddbConstStr	revid,
	CddbConstStr	revtag,
	CddbBoolean	unused,
	CddbDiscPtr	*disc,
	CddbDiscsPtr	*discs
)
{
	*disc = NULL;
	*discs = NULL;
	return Cddb_E_NOTIMPL;
}


/*
 * CddbControl_GetFullDiscInfo
 *	Return fully populated disc object after a fuzzy match selection
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetFullDiscInfo(	
	CddbControlPtr	ctrlp,
	CddbDiscPtr	discp,
	CddbBoolean	unused,
        CddbDiscPtr	*pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	cddb_disc_t	*dp1 = (cddb_disc_t *) discp,
			*dp2 = &cp->disc;
	char		*conhost;
	unsigned int	oa;
	unsigned short	conport;
	CddbResult	ret;
	bool_t		isproxy;
	void		(*oh)(int);

	if (dp1 == NULL || pval == NULL || dp1->discid == NULL ||
	    dp1->category == NULL || dp1->genre == NULL || dp1->toc == NULL) {
		*pval = NULL;
		return Cddb_E_INVALIDARG;
	}

	if (cp->options.proxyserver == NULL) {
		conhost = CDDB_SERVER_HOST;
		conport = HTTP_PORT;
		isproxy = FALSE;
	}
	else {
		conhost = cp->options.proxyserver;
		conport = (unsigned short) cp->options.proxyport;
		isproxy = TRUE;
	}

	/* Set timeout alarm */
	oh = fcddb_signal(SIGALRM, fcddb_onsig);
	oa = alarm(cp->options.servertimeout / 1000);

	ret = fcddb_read_cddb(cp, dp1->discid, dp1->toc, dp1->category,
			       conhost, conport, isproxy);

	(void) alarm(oa);
	(void) fcddb_signal(SIGALRM, oh);

	if (ret == Cddb_OK)
		*pval = (CddbDiscPtr) dp2;
	else
		*pval = NULL;

	return (ret);
}


/*
 * CddbControl_GetGenreTree
 *	Return genre tree
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetGenreTree(
	CddbControlPtr	ctrlp,
	CddbBoolean	unused,
	CddbGenreTreePtr *pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	*pval = (CddbGenreTreePtr) &cp->genretree;
	return Cddb_OK;
}


/*
 * CddbControl_GetMatchedDisc
 *	Return disc object after exact match
 */
CddbResult
CddbControl_GetMatchedDisc(CddbControlPtr ctrlp, CddbDiscPtr *disc)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	*disc = (CddbDiscPtr) &cp->disc;
	return Cddb_OK;
}


/*
 * CddbControl_GetMatchedDiscs
 *	Return discs object after fuzzy match
 */
CddbResult
CddbControl_GetMatchedDiscs(CddbControlPtr ctrlp, CddbDiscsPtr *discs)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	*discs = (CddbDiscsPtr) &cp->discs;
	return Cddb_OK;
}


/*
 * CddbControl_GetOptions
 *	Return options object
 */
CddbResult
CddbControl_GetOptions(CddbControlPtr ctrlp, CddbOptionsPtr *pval)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	cddb_options_t	*op;

	op = (cddb_options_t *) fcddb_obj_alloc(
		"CddbOptions", sizeof(cddb_options_t)
	);
	if (op == NULL)
		return CDDBTRNOutOfMemory;

	if (cp->options.proxyserver != NULL) {
		op->proxyserver =
		    (CddbStr) fcddb_strdup((char *) cp->options.proxyserver);
	}
	if (cp->options.proxyusername != NULL) {
		op->proxyusername =
		    (CddbStr) fcddb_strdup((char *) cp->options.proxyusername);
	}
	if (cp->options.proxypassword != NULL) {
		op->proxypassword =
		    (CddbStr) fcddb_strdup((char *) cp->options.proxypassword);
	}
	op->proxyport = cp->options.proxyport;
	op->servertimeout = cp->options.servertimeout;
	op->testsubmitmode = cp->options.testsubmitmode;

	*pval = (CddbOptionsPtr) op;
	return Cddb_OK;
}


/*
 * CddbControl_GetRegionList
 *	Return region list object
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetRegionList(
	CddbControlPtr	ctrlp,
	CddbBoolean	unused,
	CddbRegionListPtr *pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	*pval = (CddbRegionListPtr) &cp->regionlist;
	return Cddb_OK;
}


/*
 * CddbControl_GetLanguageList
 *	Return language list object
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetLanguageList(
	CddbControlPtr	ctrlp,
	CddbBoolean	unused,
	CddbLanguageListPtr *pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	*pval = (CddbLanguageListPtr) &cp->languagelist;
	return Cddb_OK;
}


/*
 * CddbControl_GetRoleTree
 *	Return role tree object
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetRoleTree(
	CddbControlPtr	ctrlp,
	CddbBoolean 	unused,
	CddbRoleTreePtr	*pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	*pval = (CddbRoleTreePtr) &cp->roletree;
	return Cddb_OK;
}


/*
 * CddbControl_GetServiceStatus
 *	Return CDDB service status string
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetServiceStatus(CddbControlPtr ctrlp, CddbStr *pval)
{
	*pval = "GetServiceStatus not implemented.";
	return Cddb_OK;
}


/*
 * CddbControl_GetSubmitDisc
 *	Return a skeleton disc object to be filled for submission
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetSubmitDisc(
	CddbControlPtr	ctrlp,
	CddbConstStr	toc,
	CddbConstStr	mediaid,
	CddbConstStr	muiid,
	CddbDiscPtr	*pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	cddb_disc_t	*dp;

	if (toc == NULL) {
		*pval = NULL;
		return Cddb_E_INVALIDARG;
	}

	dp = (cddb_disc_t *) fcddb_obj_alloc(
		"CddbDisc",
		sizeof(cddb_disc_t)
	);
	if (dp == NULL) {
		*pval = NULL;
		return CDDBTRNOutOfMemory;
	}

	dp->tracks.count = fcddb_parse_toc((char *) toc, trkframes);
	if (dp->tracks.count == 0) {
		*pval = NULL;
		return Cddb_E_INVALIDARG;
	}

	dp->discid = fcddb_discid(dp->tracks.count, trkframes);

	if (mediaid != NULL && strcmp(dp->discid, mediaid) != 0) {
		/* Sanity check */
		*pval = NULL;
		return Cddb_E_INVALIDARG;
	}

	if (muiid != NULL)
		dp->category = fcddb_strdup((char *) muiid);
	else
		dp->category = NULL;

	dp->toc = (CddbStr) fcddb_strdup((char *) toc);
	dp->control = cp;

	*pval = (CddbDiscPtr) dp;
	return Cddb_OK;
}


/*
 * CddbControl_GetURLList
 *	Return URL list object
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetURLList(
	CddbControlPtr	ctrlp,
	CddbDiscPtr	discp,
	CddbBoolean	unused,
	CddbURLListPtr	*pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	cddb_disc_t	*dp = (cddb_disc_t *) discp;

	if (discp == NULL)
		*pval = (CddbURLListPtr) &cp->urllist;
	else
		*pval = (CddbURLListPtr) &dp->urllist;

	return Cddb_OK;
}


/*
 * CddbControl_GetURLManager
 *	Return URL manager object
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetURLManager(CddbControlPtr ctrlp, CddbURLManagerPtr *pval)
{
	cddb_control_t		*cp = (cddb_control_t *) ctrlp;
	cddb_urlmanager_t	*mp;

	mp = (cddb_urlmanager_t *) fcddb_obj_alloc(
		"CddbURLManager",
		sizeof(cddb_urlmanager_t)
	);
	if (mp == NULL) {
		*pval = NULL;
		return CDDBTRNOutOfMemory;
	}

	mp->control = cp;

	*pval = (CddbURLManagerPtr) mp;
	return Cddb_OK;
}


/*
 * CddbControl_GetUserInfo
 *	Return user info object
 */
CddbResult
CddbControl_GetUserInfo(CddbControlPtr ctrlp, CddbUserInfoPtr *pval)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	*pval = (CddbUserInfoPtr) &cp->userinfo;
	return Cddb_OK;
}


/*
 * CddbControl_GetVersion
 *	Return CDDB control version string
 */
/*ARGSUSED*/
CddbResult
CddbControl_GetVersion(CddbControlPtr ctrlp, CddbStr *pval)
{
	static char	buf[STR_BUF_SZ];

	(void) sprintf(buf, "CDDBControl xmcd %s.%s.%s",
		       VERSION_MAJ, VERSION_MIN, VERSION_TEENY);
	*pval = buf;
	return Cddb_OK;
}


/*
 * CddbControl_InvokeInfoBrowser
 *	Spawn CDDB Music Browser
 */
/*ARGSUSED*/
CddbResult
CddbControl_InvokeInfoBrowser(
	CddbControlPtr	ctrlp,
	CddbDiscPtr	discp,
	CddbURLPtr	urlp,
	CDDBUIFlags	flags
)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbControl_IsRegistered
 *	Check user registration status
 */
/*ARGSUSED*/
CddbResult
CddbControl_IsRegistered(
	CddbControlPtr	ctrlp,
	CddbBoolean	unused,
	CddbBoolean	*ret
)
{
	*ret = 1;		/* Hard code to TRUE */
	return Cddb_OK;
}


/*
 * CddbControl_LookupMediaByToc
 *	Do CDDB query based on a CD's TOC
 */
/*ARGSUSED*/
CddbResult
CddbControl_LookupMediaByToc(
	CddbControlPtr	ctrlp,
	CddbConstStr	toc,
	CddbBoolean	unused,
	CDDBMatchCode	*matchcode
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	char		*conhost,
			*discid;
	int		ntrks;
	unsigned int	oa;
	CddbResult	ret;
	unsigned short	conport;
	bool_t		isproxy;
	void		(*oh)(int);

	if ((ntrks = fcddb_parse_toc((char *) toc, trkframes)) == 0) {
		*matchcode = 0;
		return Cddb_E_INVALIDARG;
	}

	discid = fcddb_discid(ntrks, trkframes);

	if (cp->options.proxyserver == NULL) {
		conhost = CDDB_SERVER_HOST;
		conport = HTTP_PORT;
		isproxy = FALSE;
	}
	else {
		conhost = cp->options.proxyserver;
		conport = (unsigned short) cp->options.proxyport;
		isproxy = TRUE;
	}

	/* Set timeout alarm */
	oh = fcddb_signal(SIGALRM, fcddb_onsig);
	oa = alarm(cp->options.servertimeout / 1000);

	ret = fcddb_lookup_cddb(cp, discid, trkframes, (char *) toc,
				 conhost, conport, isproxy,
				 (int *) matchcode);

	(void) alarm(oa);
	(void) fcddb_signal(SIGALRM, oh);

	MEM_FREE(discid);
	return (ret);
}


/*
 * CddbControl_ServerNoop
 *	Contact the CDDB service and perform a non-operation
 */
/*ARGSUSED*/
CddbResult
CddbControl_ServerNoop(CddbControlPtr ctrlp, CddbBoolean unused)
{
	/* Not supported in CDDB1 */
	return Cddb_OK;
}


/*
 * CddbControl_SetClientInfo
 *	Set CDDB client information
 */
CddbResult
CddbControl_SetClientInfo(
	CddbControlPtr	ctrlp,
	CddbConstStr	clid,
	CddbConstStr	cltag,
	CddbConstStr	clver,
	CddbConstStr	clregstr
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;

	if (ctrlp == NULL || clid == NULL || cltag == NULL || clver == NULL ||
	    clregstr == NULL)
		return Cddb_E_INVALIDARG;

	cp->clientname = (CddbStr) fcddb_strdup((char *) clregstr);
	cp->clientid = (CddbStr) fcddb_strdup((char *) clid);
	cp->clientver = (CddbStr) fcddb_strdup((char *) clver);

	if (strcmp((char *) cltag, "debug") == 0)
		fcddb_debug = TRUE;

	return Cddb_OK;
}


/*
 * CddbControl_SetOptions
 *	Set options information
 */
CddbResult
CddbControl_SetOptions(CddbControlPtr ctrlp, CddbOptionsPtr optionsp)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	cddb_options_t	*op = (cddb_options_t *) optionsp;
	CddbResult	ret;

	ret = Cddb_OK;

	if (cp->options.proxyserver != NULL)
		MEM_FREE(cp->options.proxyserver);
	if (cp->options.proxyusername != NULL)
		MEM_FREE(cp->options.proxyusername);
	if (cp->options.proxypassword != NULL)
		MEM_FREE(cp->options.proxypassword);

	if (op->proxyserver == NULL || op->proxyserver[0] == '\0')
		cp->options.proxyserver = NULL;
	else
		cp->options.proxyserver =
		    (CddbStr) fcddb_strdup((char *) op->proxyserver);

	if (op->proxyusername == NULL || op->proxyusername[0] == '\0')
		cp->options.proxyusername = NULL;
	else
		cp->options.proxyusername =
		    (CddbStr) fcddb_strdup((char *) op->proxyusername);

	if (op->proxypassword == NULL || op->proxypassword[0] == '\0')
		cp->options.proxypassword = NULL;
	else
		cp->options.proxypassword =
		    (CddbStr) fcddb_strdup((char *) op->proxypassword);

	cp->options.proxyport = op->proxyport;
	cp->options.servertimeout = op->servertimeout;
	cp->options.testsubmitmode = op->testsubmitmode;

	if (cp->options.proxyusername != NULL &&
	    cp->options.proxypassword != NULL) {
		ret = fcddb_set_auth(cp->options.proxyusername,
				      cp->options.proxypassword);
	}

	return (ret);
}


/*
 * CddbControl_SetUserInfo
 *	Set user information
 */
/*ARGSUSED*/
CddbResult
CddbControl_SetUserInfo(CddbControlPtr ctrlp, CddbUserInfoPtr uinfop)
{
	/* Do nothing */
	return Cddb_OK;
}


/*
 * CddbControl_SubmitDisc
 *	Submit disc information to CDDB
 */
/*ARGSUSED*/
CddbResult
CddbControl_SubmitDisc(
	CddbControlPtr	ctrlp,
	CddbDiscPtr	discp,
	CddbBoolean	unused,
	long		*pval
)
{
	cddb_control_t	*cp = (cddb_control_t *) ctrlp;
	cddb_disc_t	*dp = (cddb_disc_t *) discp;
	char		*conhost;
	int		i,
			ntrks;
	unsigned int	discid_val = 0,
			oa;
	CddbResult	ret;
	unsigned short	conport;
	bool_t		isproxy;
	void		(*oh)(int);

	if (ctrlp == NULL || discp == NULL || pval == NULL)
		return Cddb_E_INVALIDARG;

	*pval = 0;

	/* Do some sanity checking */
	if (dp->title == NULL || dp->genre == NULL)
		return CDDBCTLMissingField;

	if (dp->discid == NULL)
		return Cddb_E_INVALIDARG;

	(void) sscanf(dp->discid, "%x", (unsigned int *) &discid_val);

	ntrks = (discid_val & 0xff);
	if (ntrks == 0)
		return Cddb_E_INVALIDARG;

	for (i = 0; i < ntrks; i++) {
		if (dp->tracks.track[i].title == NULL)
			return CDDBCTLMissingField;
	}

	if (cp->options.proxyserver == NULL) {
		conhost = CDDB_SUBMIT_HOST;
		conport = HTTP_PORT;
		isproxy = FALSE;
	}
	else {
		conhost = cp->options.proxyserver;
		conport = (unsigned short) cp->options.proxyport;
		isproxy = TRUE;
	}

	/* Set timeout alarm */
	oh = fcddb_signal(SIGALRM, fcddb_onsig);
	oa = alarm(cp->options.servertimeout / 1000);

	ret = fcddb_submit_cddb(cp, dp, conhost, conport, isproxy);

	(void) alarm(oa);
	(void) fcddb_signal(SIGALRM, oh);

	return (ret);
}


