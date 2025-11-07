/*
 *   cdinfo - CD Information Management Library
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
static char *_cdinfo_x_c_ident_ = "@(#)cdinfo_x.c	7.176 04/04/20";
#endif

#ifdef __VMS
typedef char *	caddr_t;
#endif

#define _CDINFO_INTERN	/* Expose internal function protos in cdinfo.h */

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"


#define MAX_ENV_LEN	(STR_BUF_SZ * 16)

extern appdata_t	app_data;
extern FILE		*errfp;
extern cdinfo_client_t	*cdinfo_clinfo;		/* Client info */
extern cdinfo_incore_t	*cdinfo_dbp;		/* Incore CD info structure */
extern cdinfo_cddb_t	*cdinfo_cddbp;		/* Opened CDDB handle */
extern w_ent_t		*cdinfo_discog,		/* Local discog menu ptr */
			*cdinfo_scddb;		/* Search CDDB menu ptr */
extern void		*cdinfo_lconv_desc;	/* Charset conv descriptor */
extern bool_t		cdinfo_ischild;		/* Is a child process */

STATIC pid_t		child_pid = 0;		/* Child pid */
STATIC char		curfile[FILE_PATH_SZ] = { '\0' };
						/* Current disc info file */


/***********************
 *   private routines  *
 ***********************/


/*
 * cdinfo_mkpath
 *	Convert a string to replace all occurrences of non-alpha and
 *	non-digit characters with an underscore.  Multiple such characters
 *	are only substituted with one underscore.  This is used to create
 *	names suitable for use as a path name.  This function allocates
 *	memory which must be freed by the caller via MEM_FREE when done.
 *
 * Args:
 *	str - Input string
 *	len - Max output string length including terminating null character
 *	
 * Return:
 *	Converted string, or NULL on failure.
 */
STATIC char *
cdinfo_mkpath(char *str, int len)
{
	int	i;
	char	*buf,
		*cp,
		*cp2;

	if ((buf = (char *) MEM_ALLOC("cdinfo_mkpath", len)) == NULL)
		return NULL;

	cp = buf;
	cp2 = str;
	for (i = 0; *cp2 != '\0' && i < (len - 1); i++) {
		/* Replace non-alpha and non-digit chars with an underscore */

		if (isalnum((int) *cp2))
			*cp++ = *cp2;
		else if (cp > buf && *(cp-1) != '_')
			*cp++ = '_';
		else
			i--;

		cp2++;
	}
	*cp = '\0';

	return (buf);
}


/*
 * cdinfo_forkwait
 *	Function that forks a child and sets the uid and gid to the original
 *	invoking user.  The parent then waits for the child to do some
 *	work and exit, and return an exit code in the location pointed to
 *	by retcode.  The caller can check the return of this function to
 *	determine whether it's the parent or child.  If SYNCHRONOUS is
 *	defined, this function always returns FALSE and no child is forked.
 *
 * Args:
 *	retcode - Location that the parent is to return an status code of
 *		  the child.
 *
 * Return:
 *	TRUE - Parent process: child has exited and the status code is
 *	       returned in retcode.
 *	FALSE - Child process: Now running with original uid/gid.
 */
bool_t
cdinfo_forkwait(cdinfo_ret_t *retcode)
{
#ifdef SYNCHRONOUS
	*retcode = 0;
	return FALSE;
#else
	int		ret;
	pid_t		cpid;
	waitret_t	wstat;

	/* Fork child and make child do actual work */
	switch (cpid = FORK()) {
	case 0:
		/* Child process */
		cdinfo_ischild = TRUE;

		(void) util_signal(SIGTERM, cdinfo_onterm);
		(void) util_signal(SIGPIPE, SIG_IGN);

		/* Force uid and gid to original setting */
		if (!util_set_ougid())
			_exit(SETUID_ERR);

		return FALSE;

	case -1:
		*retcode = CDINFO_SET_CODE(FORK_ERR, errno);
		return TRUE;

	default:
		/* Parent process: wait for child to exit */
		child_pid = cpid;

		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     cdinfo_clinfo->workproc,
				     cdinfo_clinfo->arg,
				     TRUE, &wstat);
		child_pid = 0;

		if (ret < 0) {
			*retcode = CDINFO_SET_CODE(WAIT_ERR, errno);
		}
		else if (WIFEXITED(wstat)) {
			if (WEXITSTATUS(wstat) == 0)
				*retcode = 0;
			else
				*retcode = CDINFO_SET_CODE(
					WEXITSTATUS(wstat), 0
				);
		}
		else if (WIFSIGNALED(wstat)) {
			*retcode = CDINFO_SET_CODE(
				KILLED_ERR, WTERMSIG(wstat)
			);
		}
		else
			*retcode = 0;

		return TRUE;
	}

	/*NOTREACHED*/
#endif	/* SYNCHRONOUS */
}


/***********************
 *   public routines   *
 ***********************/


/*
 * cdinfo_addr
 *	Return a pointer to the incore CD information structure.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Pointer to the incore CD info structure.
 */
cdinfo_incore_t *
cdinfo_addr(void)
{
	return (cdinfo_dbp);
}


/*
 * cdinfo_discid
 *      Compute a quasi-unique integer disc identifier based on the
 *	number of tracks, the length of each track, and a checksum of
 *	the string that represents the offset of each track.
 *
 * Args:
 *      s - Pointer to the curstat_t structure.
 *
 * Return:
 *      The integer disc ID.
 */
word32_t
cdinfo_discid(curstat_t *s)
{
	int	i,
		a,
		t = 0,
		n = 0;

	/* For backward compatibility this algorithm must not change */
	a = (int) s->tot_trks;

	for (i = 0; i < a; i++)
		n += cdinfo_sum((s->trkinfo[i].min * 60) + s->trkinfo[i].sec);

	t = ((s->trkinfo[a].min * 60) + s->trkinfo[a].sec) -
	     ((s->trkinfo[0].min * 60) + s->trkinfo[0].sec);

	return ((n % 0xff) << 24 | t << 8 | s->tot_trks);
}


/*
 * cdinfo_txtreduce
 *	Transform a text string into a CGI search string
 *
 * Args:
 *	text - Input text string
 *      reduce - Whether to reduce text (remove punctuations,
 *		 exclude words  from the excludeWords list).
 *
 * Return:
 *	Output string.  The storage is internally allocated per call,
 *	and should be freed by the caller with MEM_FREE() after the
 *	buffer is no longer needed.  If there is no sufficient memory
 *	to allocate the buffer, NULL is returned.
 */
char *
cdinfo_txtreduce(char *text, bool_t reduce)
{
	char	*p,
		*q;

	if (reduce) {
		if ((p = util_text_reduce(text)) == NULL)
		return NULL;
	}
	else
		p = text;

	if ((q = util_cgi_xlate(p)) == NULL)
		return NULL;

	if (reduce)
		MEM_FREE(p);

	return (q);
}


/*
 * cdinfo_tmpl_to_url
 *	Make a URL string from a template.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	tmpl - Template string
 *	url - Buffer to store resultant URL string
 *	trkpos - The track position for %T or %t
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_tmpl_to_url(curstat_t *s, char *tmpl, char *url, int trkpos)
{
	char	*q,
		*r,
		*a;
	char	dtitle[(STR_BUF_SZ * 4) + 4];

	r = url;
	for (a = tmpl; *a != '\0'; a++) {
		switch (*a) {
		case '%':
			switch (*(++a)) {
			case 'X':
				(void) strcpy(r, cdinfo_clinfo->prog);
				r += strlen(cdinfo_clinfo->prog);
				break;

			case 'V':
				(void) sprintf(r, "%s.%s",
					       VERSION_MAJ, VERSION_MIN);
				r += (strlen(VERSION_MAJ) +
				      strlen(VERSION_MIN) + 1);
				break;

			case 'N':
				q = util_loginname();
				(void) strcpy(r, q);
				r += strlen(q);
				break;

			case '~':
				q = util_homedir(util_get_ouid());
				(void) strcpy(r, q);
				r += strlen(q);
				break;

			case 'H':
				(void) strcpy(r, cdinfo_clinfo->host);
				r += strlen(cdinfo_clinfo->host);
				break;

			case 'L':
				(void) strcpy(r, app_data.libdir);
				r += strlen(app_data.libdir);
				break;

			case 'S':
				(void) strcpy(r, app_data.libdir);
#ifdef __VMS
				(void) strcat(r, ".discog");
#else
				(void) strcat(r, "/discog");
#endif
				r += strlen(app_data.libdir) +
				     strlen("discog") + 1;
				break;

			case 'C':
				q = cdinfo_genre_path(cdinfo_dbp->disc.genre);
				(void) strcpy(r, q);
				r += strlen(q);
				break;

			case 'I':
				(void) sprintf(r, "%08x", cdinfo_dbp->discid);
				r += 8;
				break;

			case 'A':
			case 'a':
				/* Translate disk artist into a
				 * keyword string in CGI form
				 */
				if (cdinfo_dbp->disc.artist == NULL)
					break;

				q = cdinfo_txtreduce(
					cdinfo_dbp->disc.artist,
					(bool_t) (*a == 'a')
				);
				if (q == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}

				(void) strcpy(r, q);
				r += strlen(q);
				MEM_FREE(q);
				break;

			case 'R':
			case 'r':
				/* Translate track artist into a
				 * keyword string in CGI form
				 */
				if (trkpos >= 0 &&
				    cdinfo_dbp->track[trkpos].artist != NULL) {
					q = cdinfo_txtreduce(
					    cdinfo_dbp->track[trkpos].artist,
					    (bool_t) (*a == 'r')
					);
					if (q == NULL) {
						CDINFO_FATAL(
							app_data.str_nomemory
						);
						return;
					}
					strcpy(r, q);
					r += strlen(q);
					MEM_FREE(q);
					break;
				}

				if (cdinfo_dbp->disc.artist == NULL)
					break;

				/* Can't do track, Use disc artist instead */
				q = cdinfo_txtreduce(cdinfo_dbp->disc.artist,
						      (bool_t) (*a == 'r'));
				if (q == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}

				(void) strcpy(r, q);
				r += strlen(q);
				MEM_FREE(q);
				break;

			case 'T':
			case 't':
				/* Translate track title into a
				 * keyword string in CGI form
				 */
				if (trkpos >= 0 &&
				    cdinfo_dbp->track[trkpos].title != NULL) {
					q = cdinfo_txtreduce(
						cdinfo_dbp->track[trkpos].title,
						(bool_t) (*a == 't')
					);
					if (q == NULL) {
						CDINFO_FATAL(
							app_data.str_nomemory
						);
						return;
					}
					strcpy(r, q);
					r += strlen(q);
					MEM_FREE(q);
					break;
				}

				if (cdinfo_dbp->disc.title == NULL)
					break;

				/* Can't do track, Use disc title instead */
				q = cdinfo_txtreduce(cdinfo_dbp->disc.title,
						     (bool_t) (*a == 't'));
				if (q == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}

				(void) strcpy(r, q);
				r += strlen(q);
				MEM_FREE(q);
				break;

			case 'D':
			case 'd':
				/* Translate disc title into a
				 * keyword string in CGI form
				 */
				if (cdinfo_dbp->disc.title == NULL)
					break;

				q = cdinfo_txtreduce(cdinfo_dbp->disc.title,
						      (bool_t) (*a == 'd'));
				if (q == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}

				(void) strcpy(r, q);
				r += strlen(q);
				MEM_FREE(q);
				break;

			case 'B':
			case 'b':
				/* Translate artist and disc title into a
				 * keyword string in CGI form
				 */
				if (cdinfo_dbp->disc.artist == NULL &&
				    cdinfo_dbp->disc.title == NULL)
					break;

				(void) sprintf(dtitle, "%.127s%s%.127s",
					(cdinfo_dbp->disc.artist == NULL) ?
						"" : cdinfo_dbp->disc.artist,
					(cdinfo_dbp->disc.artist != NULL &&
					 cdinfo_dbp->disc.title != NULL) ?
						" / " : "",
					(cdinfo_dbp->disc.title == NULL) ?
						"" : cdinfo_dbp->disc.title);

				q = cdinfo_txtreduce(
					dtitle,
					(bool_t) (*a == 'b')
				);
				if (q == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}

				(void) strcpy(r, q);
				r += strlen(q);
				MEM_FREE(q);
				break;

			case '#':
				if (trkpos >= 0) {
					(void) sprintf(r, "%02d",
						    s->trkinfo[trkpos].trkno);
					r += 2;
				}
				break;

			case '%':
				*r++ = '%';
				break;

			default:
				*r++ = '%';
				*r++ = *a;
				break;
			}
			break;

		default:
			*r++ = *a;
			break;
		}
	}
	*r = '\0';
}


/*
 * cdinfo_url_len
 *	Determine a string buffer size large enough to hold a
 *	URL after it is expanded from its template.
 *
 * Args:
 *	tmpl - Template string
 *	up - Pointer to the associated URL attributes structure
 *	trkpos - Pointer to the track position for %T or %t.
 *
 * Return:
 *	The buffer length.
 */
int
cdinfo_url_len(char *tmpl, url_attrib_t *up, int *trkpos)
{
	int	n,
		artist_len,
		title_len,
		dtitle_len;

	artist_len = (cdinfo_dbp->disc.artist != NULL) ?
		strlen(cdinfo_dbp->disc.artist) : STR_BUF_SZ;
	title_len = (cdinfo_dbp->disc.title != NULL) ?
		strlen(cdinfo_dbp->disc.title) : STR_BUF_SZ;
	dtitle_len = artist_len + title_len;

	n = 16 + strlen(tmpl) + ((up->acnt + up->dcnt) * dtitle_len);

	if (*trkpos < 0 || cdinfo_dbp->track[*trkpos].title == NULL) {
	    *trkpos = -1;
	    n += ((up->rcnt * dtitle_len) + (up->tcnt * dtitle_len));
	}
	else {
	    n += (up->tcnt * strlen(cdinfo_dbp->track[*trkpos].title));

	    if (cdinfo_dbp->track[*trkpos].artist != NULL)
		n += (up->rcnt * strlen(cdinfo_dbp->track[*trkpos].artist));
	    else
		n += (up->rcnt * artist_len);
	}

	n += (up->xcnt * strlen(cdinfo_clinfo->prog));
	n += (up->vcnt * (strlen(VERSION_MAJ) + strlen(VERSION_MIN) + 1));
	n += (up->ncnt * STR_BUF_SZ);
	n += (up->hcnt * HOST_NAM_SZ);
	n += (up->lcnt * strlen(app_data.libdir));
	n += (up->pcnt * 2);
	if (cdinfo_dbp->disc.genre != NULL) {
		n += (up->ccnt *
		      strlen(cdinfo_genre_path(cdinfo_dbp->disc.genre)));
	}
	n += (up->icnt * 8);
	n++;

	return (n);
}


/*
 * cdinfo_init
 *	Initialize CD information management services
 *
 * Args:
 *	progname - The client program name string
 *	username - The client user login name string
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_init(cdinfo_client_t *clp)
{
	char	*cp;

	cdinfo_clinfo = (cdinfo_client_t *)(void *) MEM_ALLOC(
		"cdinfo_client_t", sizeof(cdinfo_client_t)
	);
	if (cdinfo_clinfo == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memcpy(cdinfo_clinfo, clp, sizeof(cdinfo_client_t));

	/* Sanity check */
	if (clp->prog[0] == '\0')
		(void) strcpy(clp->prog, "unknown");

	/* Hard wire file modes in case these are missing from the
	 * common.cfg file
	 */
	if (app_data.cdinfo_filemode == NULL)
		app_data.cdinfo_filemode = "0666";
	if (app_data.hist_filemode == NULL)
		app_data.hist_filemode = "0644";

	/* Load XMCD_CDINFOPATH environment variable, if specified */
	if ((cp = (char *) getenv("XMCD_CDINFOPATH")) != NULL &&
	    !util_newstr(&app_data.cdinfo_path, cp)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return;
	}

	if (app_data.cdinfo_path == NULL &&
	    !util_newstr(&app_data.cdinfo_path, "CDDB;CDTEXT")) {
		CDINFO_FATAL(app_data.str_nomemory);
		return;
	}

	if ((int) strlen(app_data.cdinfo_path) >= MAX_ENV_LEN) {
		CDINFO_FATAL(app_data.str_longpatherr);
		return;
	}

	/* Allocate CD info incore structure */
	cdinfo_dbp = (cdinfo_incore_t *)(void *) MEM_ALLOC(
		"cdinfo_incore_t",
		sizeof(cdinfo_incore_t)
	);
	if (cdinfo_dbp == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memset(cdinfo_dbp, 0, sizeof(cdinfo_incore_t));

	/* Set up CD info path list */
	cdinfo_reinit();
}


/*
 * cdinfo_reinit
 *	Re-initialize CD information management services.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_reinit(void)
{
	char		*cp,
			*path;
	cdinfo_path_t	*pp,
			*pp_next;

	/* Deallocate CD info path list, if present */
	pp = cdinfo_dbp->pathlist;
	while (pp != NULL) {
		pp_next = pp->next;

		if (pp->categ != NULL)
			MEM_FREE(pp->categ);
		if (pp->path != NULL)
			MEM_FREE(pp->path);
		MEM_FREE(pp);

		pp = pp_next;
	}
	cdinfo_dbp->pathlist = NULL;

	/* Create new CD info path list */
	path = app_data.cdinfo_path;
	while ((cp = strchr(path, CDINFOPATH_SEPCHAR)) != NULL) {
		*cp = '\0';

		if (!cdinfo_add_pathent(path))
			return;

		*cp = CDINFOPATH_SEPCHAR;
		path = cp + 1;
	}
	(void) cdinfo_add_pathent(path);
}


/*
 * cdinfo_halt
 *	Shut down cdinfo subsystem.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing
 */
void
cdinfo_halt(curstat_t *s)
{
	if (cdinfo_cddbp != NULL)
		(void) cdinfo_closecddb(cdinfo_cddbp);

	if (curfile[0] != '\0' && s->devlocked)
		(void) UNLINK(curfile);
}


/*
 * cdinfo_submit
 *	Submit current CD information to CDDB server
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
/*ARGSUSED*/
cdinfo_ret_t
cdinfo_submit(curstat_t *s)
{
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL) {
		/* CDDB service failure */
		CH_RET(retcode);
	}

	/* Submit disc info to CDDB */
	if (!cdinfo_submitcddb(cp, s, &retcode)) {
		/* Submit failed */
		(void) cdinfo_closecddb(cp);
		CH_RET(retcode);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_submit_url
 *	Submit to CDDB a URL pertaining to the current CD
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	up - Pointer to the cdinfo_url_t structure
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
/*ARGSUSED*/
cdinfo_ret_t
cdinfo_submit_url(curstat_t *s, cdinfo_url_t *up)
{
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	if (up == NULL ||
	    up->categ == NULL || up->href == NULL || up->disptext == NULL)
		return CDINFO_SET_CODE(ARG_ERR, 0);

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL) {
		/* CDDB service failure */
		CH_RET(retcode);
	}

	/* Submit disc info to CDDB */
	if (!cdinfo_submiturlcddb(cp, up, &retcode)) {
		/* Submit failed */
		(void) cdinfo_closecddb(cp);
		CH_RET(retcode);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_flush
 *	Flush CD information cache
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
/*ARGSUSED*/
cdinfo_ret_t
cdinfo_flush(curstat_t *s)
{
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL) {
		/* CDDB service failure */
		CH_RET(retcode);
	}

	/* Flush CDDB local cache */
	if (!cdinfo_flushcddb(cp)) {
		/* Cache flush failed */
		(void) cdinfo_closecddb(cp);
		CH_RET(FLUSH_ERR);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_offline
 *	Set the offline mode to what is specified by app_data.cdinfo_inetoffln
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
/*ARGSUSED*/
cdinfo_ret_t
cdinfo_offline(curstat_t *s)
{
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL) {
		/* CDDB service failure */
		CH_RET(retcode);
	}

	/* The cdinfo_opencddb call implicitly does the work, so
	 * we don't need to do anything else here.
	 */

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_load
 *	Load CD database entry for the currently inserted CD.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Return value will be 0 for a successful connection, and
 *	a successful query will cause the CDINFO_MATCH bit to be
 *	set in the flags field of the cdinfo_incore_t structure.
 *	If an error occurred, then the return code is as defined
 *	by cdinfo_ret_t.
 */
cdinfo_ret_t
cdinfo_load(curstat_t *s)
{
#ifdef SYNCHRONOUS
	cdinfo_path_t		*pp;
	cdinfo_cddb_t		*cp;
	cdinfo_credit_t		*p;
	cdinfo_segment_t	*q;
	di_cdtext_t		*cdt;
	char			*path;
	int			i,
				retcode;

	cdinfo_dbp->flags &= ~(CDINFO_MATCH | CDINFO_FROMLOC | CDINFO_FROMCDT);

	if ((cdinfo_dbp->discid = cdinfo_discid(s)) == 0)
		return CDINFO_SET_CODE(ARG_ERR, 0);

	if (cdinfo_cddb_iscfg() &&
	    (cdinfo_dbp->regionlist == NULL ||
	     cdinfo_dbp->langlist == NULL ||
	     cdinfo_dbp->rolelist == NULL ||
	     cdinfo_dbp->genrelist == NULL)) {
		DBGPRN(DBG_CDI)(errfp, "Initializing CDDB service...\n");

		/* Open CDDB connection */
		if ((cp = cdinfo_opencddb(s, TRUE, &retcode)) != NULL) {
			/* Check user registration, set up region, lang, role
			 * and genre lists
			 */
			(void) cdinfo_initcddb(cp, &retcode);
			(void) cdinfo_closecddb(cp);
		}
		else if (retcode == AUTH_ERR) {
			/* Special case handling for proxy auth error */
			return CDINFO_SET_CODE(retcode, 0);
		}
	}

	for (pp = cdinfo_dbp->pathlist; pp != NULL; pp = pp->next) {
		DBGPRN(DBG_CDI)(errfp, "*** CD info query: ");
		retcode = 0;

		switch (pp->type) {
		case CDINFO_CDTEXT:
			/* Use CD-TEXT information from the CD */
			DBGPRN(DBG_CDI)(errfp, "Trying CD-TEXT...\n");
			cdt = &cdinfo_dbp->cdtext;
			di_load_cdtext(s, cdt);

			if (cdt->cdtext_valid) {
				cdinfo_map_cdtext(s, cdt);
				cdinfo_dbp->flags |=
					(CDINFO_MATCH | CDINFO_FROMCDT);
			}
			break;

		case CDINFO_RMT:
			/* Remote: Use CDDB service */
			DBGPRN(DBG_CDI)(errfp, "Trying CDDB service...\n");

			/* Open CDDB connection */
			if ((cp = cdinfo_opencddb(s, TRUE, &retcode)) == NULL)
			{
				/* CDDB service failure */
				if (retcode == AUTH_ERR) {
					/* Special case handling for
					 * proxy auth error
					 */
					return CDINFO_SET_CODE(retcode, 0);
				}
				break;
			}

			/* Read CD information into incore structure */
			if (!cdinfo_querycddb(cp, s, &retcode)) {
				/* Query failed */
				(void) cdinfo_closecddb(cp);
				if (retcode == AUTH_ERR) {
					/* Special case handling for
					 * proxy auth error
					 */
					return CDINFO_SET_CODE(retcode, 0);
				}
				break;
			}

			if (cdinfo_dbp->matchlist != NULL) {
				/* Inexact "fuzzy matches" found.
				 * Query user to choose one
				 */
				cdinfo_dbp->match_tag = 0;

				/* Don't close CDDB connection here,
				 * but save pointer.
				 */
				cdinfo_dbp->sav_cddbp = cp;

				/* We return here, and continue to
				 * service this query via the callback
				 * cdinfo_load_matchsel().
				 */
				return 0;
			}

			/* Close CDDB connection */
			(void) cdinfo_closecddb(cp);

			break;

		case CDINFO_LOC:
			/* Local: Open local CD info file */

			path = (char *) MEM_ALLOC(
				"path", strlen(pp->path) + 12
			);
			if (path == NULL)
				return CDINFO_SET_CODE(MEM_ERR, errno);;

			(void) sprintf(path, CDINFOFILE_PATH,
				       pp->path, cdinfo_dbp->discid);

			DBGPRN(DBG_CDI)(errfp, "Trying %s", path);
			(void) cdinfo_load_locdb(path, pp->categ, s, &retcode);

			MEM_FREE(path);
			break;
		}

		if ((cdinfo_dbp->flags & CDINFO_MATCH) != 0 ||
		    (cdinfo_dbp->flags & CDINFO_NEEDREG) != 0) {
			/* Found a match or need user registration */
			break;
		}
	}

	/* If the display name field of the fullname structures are
	 * NULL, * fill them with the appropriate text.  Conversely,
	 * if the display name field is non NULL and the matching
	 * name fields are NULL, back fill the data. 
	 */

	if (cdinfo_dbp->disc.artistfname.dispname == NULL &&
	    cdinfo_dbp->disc.artist != NULL &&
	    !util_newstr(&cdinfo_dbp->disc.artistfname.dispname,
			 cdinfo_dbp->disc.artist)) {
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}
	else if (cdinfo_dbp->disc.artist == NULL &&
		 cdinfo_dbp->disc.artistfname.dispname != NULL &&
		 !util_newstr(&cdinfo_dbp->disc.artist,
			      cdinfo_dbp->disc.artistfname.dispname)) {
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	for (p = cdinfo_dbp->disc.credit_list; p != NULL; p = p->next) {
		if (p->crinfo.fullname.dispname == NULL &&
		    p->crinfo.name != NULL &&
		    !util_newstr(&p->crinfo.fullname.dispname,
				 p->crinfo.name)) {
			return CDINFO_SET_CODE(MEM_ERR, 0);
		}
		else if (p->crinfo.name == NULL &&
			 p->crinfo.fullname.dispname != NULL &&
			 !util_newstr(&p->crinfo.name,
				      p->crinfo.fullname.dispname)) {
			return CDINFO_SET_CODE(MEM_ERR, 0);
		}
	}

	for (q = cdinfo_dbp->disc.segment_list; q != NULL; q = q->next) {
		for (p = q->credit_list; p != NULL; p = p->next) {
			if (p->crinfo.fullname.dispname == NULL &&
			    p->crinfo.name != NULL &&
			    !util_newstr(&p->crinfo.fullname.dispname,
					 p->crinfo.name)) {
				return CDINFO_SET_CODE(MEM_ERR, 0);
			}
			else if (p->crinfo.name == NULL &&
				 p->crinfo.fullname.dispname != NULL &&
				 !util_newstr(&p->crinfo.name,
					      p->crinfo.fullname.dispname)) {
				return CDINFO_SET_CODE(MEM_ERR, 0);
			}
		}
	}

	for (i = 0; i < (int) s->tot_trks; i++) {
		if (cdinfo_dbp->track[i].artistfname.dispname == NULL &&
		    cdinfo_dbp->track[i].artist != NULL &&
		    !util_newstr(&cdinfo_dbp->track[i].artistfname.dispname,
				 cdinfo_dbp->track[i].artist)) {
			return CDINFO_SET_CODE(MEM_ERR, 0);
		}
		else if (cdinfo_dbp->track[i].artist == NULL &&
			 cdinfo_dbp->track[i].artistfname.dispname != NULL &&
			 !util_newstr(&cdinfo_dbp->track[i].artist,
				cdinfo_dbp->track[i].artistfname.dispname)) {
			return CDINFO_SET_CODE(MEM_ERR, 0);
		}

		for (p = cdinfo_dbp->track[i].credit_list; p != NULL;
		     p = p->next){
			if (p->crinfo.fullname.dispname == NULL &&
			    p->crinfo.name != NULL &&
			    !util_newstr(&p->crinfo.fullname.dispname,
					 p->crinfo.name)) {
				return CDINFO_SET_CODE(MEM_ERR, 0);
			}
			else if (p->crinfo.name == NULL &&
				 p->crinfo.fullname.dispname != NULL &&
				 !util_newstr(&p->crinfo.name,
					      p->crinfo.fullname.dispname)) {
				return CDINFO_SET_CODE(MEM_ERR, 0);
			}
		}

		/* If we have ISRC data that was read from the CD itself,
		 * use these instead of the ones from CDDB.
		 */
		if (s->trkinfo[i].isrc[0] != 0 &&
		    !util_newstr(&cdinfo_dbp->track[i].isrc,
				 s->trkinfo[i].isrc)) {
			return CDINFO_SET_CODE(MEM_ERR, 0);
		}
	}

	return 0;

#else	/* SYNCHRONOUS */
	int			i,
				ret,
				retcode;
	pid_t			cpid;
	waitret_t		wstat;
	cdinfo_path_t		*pp;
	cdinfo_cddb_t		*cp;
	cdinfo_credit_t		*p;
	cdinfo_segment_t	*q;
	cdinfo_pipe_t		*spp,
				*rpp;
	di_cdtext_t		*cdt;
	char			*path;
	bool_t			done;

	cdinfo_dbp->flags &= ~(CDINFO_MATCH | CDINFO_FROMLOC | CDINFO_FROMCDT);

	if ((cdinfo_dbp->discid = cdinfo_discid(s)) == 0)
		return CDINFO_SET_CODE(ARG_ERR, 0);

	/* Open pipes for IPC */
	if ((rpp = cdinfo_openpipe(CDINFO_DATAIN)) == NULL)
		return CDINFO_SET_CODE(OPEN_ERR, errno);
	if ((spp = cdinfo_openpipe(CDINFO_DATAOUT)) == NULL) {
		(void) cdinfo_closepipe(rpp);
		return CDINFO_SET_CODE(OPEN_ERR, errno);
	}

	/* Fork child to performs actual I/O */
	switch (cpid = FORK()) {
	case 0:
		/* Child process */
		cdinfo_ischild = TRUE;

		(void) util_signal(SIGTERM, cdinfo_onterm);
		(void) util_signal(SIGPIPE, SIG_IGN);

		/* Close unneeded descriptors */
		(void) close(rpp->r.fd);
		(void) close(spp->w.fd);
		rpp->r.fd = spp->w.fd = -1;

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) cdinfo_closepipe(rpp);
			(void) cdinfo_closepipe(spp);
			CH_RET(SETUID_ERR);
		}

		break;

	case -1:
		(void) cdinfo_closepipe(rpp);
		(void) cdinfo_closepipe(spp);
		return CDINFO_SET_CODE(FORK_ERR, errno);

	default:
		/* Parent process */
		child_pid = cpid;

		/* Close unneeded descriptors */
		(void) close(rpp->w.fd);
		(void) close(spp->r.fd);
		rpp->w.fd = spp->r.fd = -1;

		/* Read CD info content from pipe into in-core structure */
		(void) cdinfo_read_datapipe(rpp);

		if (cdinfo_dbp->matchlist != NULL) {
			/* Inexact "fuzzy matches" found.  Query user
			 * to choose one.
			 */
			cdinfo_dbp->match_tag = 0;

			/* Don't close pipes here, but save pointer */
			cdinfo_dbp->sav_rpp = rpp;
			cdinfo_dbp->sav_spp = spp;

			/* We return with child process still running,
			 * and continue to service this query via
			 * the callback cdinfo_load_matchsel().
			 */
			return 0;
		}

		/* Close pipes */
		(void) cdinfo_closepipe(rpp);
		(void) cdinfo_closepipe(spp);
		int tom=0;
		while(tom < s->tot_trks) {
			DBGPRN(DBG_CDI)(errfp, "Dump2: %s\n", cdinfo_dbp->cdtext.track[tom++].title);
		}

		/* Parent process: wait for child to exit */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     cdinfo_clinfo->workproc,
				     cdinfo_clinfo->arg,
				     TRUE, &wstat);
		child_pid = 0;

		if (ret < 0) {
			return CDINFO_SET_CODE(WAIT_ERR, errno);
		}
		if (WIFEXITED(wstat)) {
			if (WEXITSTATUS(wstat) == 0)
				return 0;
			else
				return CDINFO_SET_CODE(WEXITSTATUS(wstat), 0);
		}
		else if (WIFSIGNALED(wstat)) {
			return CDINFO_SET_CODE(KILLED_ERR, WTERMSIG(wstat));
		}
		else
			return 0;

		/*NOTREACHED*/
	}

	if (cdinfo_cddb_iscfg() &&
	    (cdinfo_dbp->regionlist == NULL ||
	     cdinfo_dbp->langlist == NULL ||
	     cdinfo_dbp->rolelist == NULL ||
	     cdinfo_dbp->genrelist == NULL)) {
		DBGPRN(DBG_CDI)(errfp, "Initializing CDDB service...\n");

		/* Open CDDB connection */
		if ((cp = cdinfo_opencddb(s, TRUE, &retcode)) != NULL) {
			/* Check user registration, set up region, lang, role
			 * and genre lists
			 */
			(void) cdinfo_initcddb(cp, &retcode);
			(void) cdinfo_closecddb(cp);
		}
		else if (retcode == AUTH_ERR) {
			/* Special case handling for proxy auth error */
			(void) cdinfo_closepipe(rpp);
			(void) cdinfo_closepipe(spp);
			CH_RET(retcode);
		}
	}

	done = FALSE;
	for (pp = cdinfo_dbp->pathlist; pp != NULL; pp = pp->next) {
		DBGPRN(DBG_CDI)(errfp, "*** CD info query: ");
		retcode = 0;

		switch (pp->type) {
		case CDINFO_CDTEXT:
			/* Use CD-TEXT information from the CD */
			DBGPRN(DBG_CDI)(errfp, "Trying CD-TEXT...\n");
			cdt = &cdinfo_dbp->cdtext;
			di_load_cdtext(s, cdt);
			DBGPRN(DBG_CDI)(errfp, "Have Text?: %d\n", cdt->cdtext_valid);
			int tom=0;
			while(tom < s->tot_trks) {
				DBGPRN(DBG_CDI)(errfp, "Dump: %s\n", cdt->track[tom++].title);
			}
			if (cdt->cdtext_valid) {
				cdinfo_map_cdtext(s, cdt);
				cdinfo_dbp->flags |=
					(CDINFO_MATCH | CDINFO_FROMCDT);
				done = TRUE;
			}
			break;

		case CDINFO_RMT:
			/* Remote: Use CDDB service */
			DBGPRN(DBG_CDI)(errfp, "Trying CDDB service...\n");

			/* Open CDDB connection */
			if ((cp = cdinfo_opencddb(s, TRUE, &retcode)) == NULL)
			{
				/* CDDB service Failure */
				if (retcode == AUTH_ERR) {
					/* Special case handling for
					 * proxy auth error
					 */
					(void) cdinfo_closepipe(rpp);
					(void) cdinfo_closepipe(spp);
					CH_RET(retcode);
				}
				break;
			}

			/* Read CD info content and update incore structures */
			if (!cdinfo_querycddb(cp, s, &retcode)) {
				/* Failure */
				(void) cdinfo_closecddb(cp);
				if (retcode == AUTH_ERR) {
					/* Special case handling for
					 * proxy auth error
					 */
					(void) cdinfo_closepipe(rpp);
					(void) cdinfo_closepipe(spp);
					CH_RET(retcode);
				}
				break;
			}

			if (cdinfo_dbp->matchlist != NULL) {
				/* Inexact "fuzzy matches" found.
				 * Transmit the list to parent process
				 * for processing.
				 */
				cdinfo_dbp->match_tag = 0;

				if (!cdinfo_write_datapipe(rpp, s)) {
					/* Failure */
					(void) cdinfo_closecddb(cp);
					(void) cdinfo_closepipe(rpp);
					(void) cdinfo_closepipe(spp);
					cdinfo_free_matchlist();
					CH_RET(WRITE_ERR);
				}

				util_delayms(2000);

				/* Read user selection */
				if (!cdinfo_read_selpipe(spp)) {
					/* Failure */
					(void) cdinfo_closecddb(cp);
					(void) cdinfo_closepipe(rpp);
					(void) cdinfo_closepipe(spp);
					cdinfo_free_matchlist();
					CH_RET(READ_ERR);
				}

				/* Read CD info content and update incore
				 * structures
				 */
				if (!cdinfo_querycddb(cp, s, &retcode)) {
					/* Failure */
					(void) cdinfo_closecddb(cp);
					if (retcode == AUTH_ERR) {
						/* Special case handling for
						 * proxy auth error
						 */
						(void) cdinfo_closepipe(rpp);
						(void) cdinfo_closepipe(spp);
						cdinfo_free_matchlist();
						CH_RET(retcode);
					}
					break;
				}

				/* Deallocate the matchlist */
				cdinfo_free_matchlist();
			}

			/* Close CDDB connection */
			(void) cdinfo_closecddb(cp);

			if ((cdinfo_dbp->flags & CDINFO_MATCH) != 0 ||
			    (cdinfo_dbp->flags & CDINFO_NEEDREG) != 0) {
				/* Found a match or needs user reg */
				done = TRUE;
			}
			break;

		case CDINFO_LOC:
			/* Local: Open local CD info file */
			path = (char *) MEM_ALLOC(
				"path", strlen(pp->path) + 12
			);
			if (path == NULL) {
				(void) cdinfo_closepipe(rpp);
				(void) cdinfo_closepipe(spp);
				CH_RET(MEM_ERR);
			}

			(void) sprintf(path, CDINFOFILE_PATH,
				       pp->path, cdinfo_dbp->discid);

			DBGPRN(DBG_CDI)(errfp, "Trying %s", path);
			(void) cdinfo_load_locdb(path, pp->categ, s, &retcode);

			MEM_FREE(path);

			if ((cdinfo_dbp->flags & CDINFO_MATCH) != 0) {
				/* Found a match */
				done = TRUE;
			}
			break;
		}

		if (done)
			break;
	}

	/* If the display name field of the fullname structures are
	 * NULL, fill them with the appropriate text.  Conversely,
	 * if the display name field is non NULL and the matching
	 * name fields are NULL, back fill the data. 
	 */

	if (cdinfo_dbp->disc.artistfname.dispname == NULL &&
	    cdinfo_dbp->disc.artist != NULL &&
	    !util_newstr(&cdinfo_dbp->disc.artistfname.dispname,
			 cdinfo_dbp->disc.artist)) {
		(void) cdinfo_closepipe(rpp);
		(void) cdinfo_closepipe(spp);
		CH_RET(MEM_ERR);
	}
	else if (cdinfo_dbp->disc.artist == NULL &&
		 cdinfo_dbp->disc.artistfname.dispname != NULL &&
		 !util_newstr(&cdinfo_dbp->disc.artist,
			      cdinfo_dbp->disc.artistfname.dispname)) {
		(void) cdinfo_closepipe(rpp);
		(void) cdinfo_closepipe(spp);
		CH_RET(MEM_ERR);
	}

	for (p = cdinfo_dbp->disc.credit_list; p != NULL; p = p->next) {
		if (p->crinfo.fullname.dispname == NULL &&
		    p->crinfo.name != NULL &&
		    !util_newstr(&p->crinfo.fullname.dispname,
				 p->crinfo.name)) {
			(void) cdinfo_closepipe(rpp);
			(void) cdinfo_closepipe(spp);
			CH_RET(MEM_ERR);
		}
		else if (p->crinfo.name == NULL &&
			 p->crinfo.fullname.dispname != NULL &&
			 !util_newstr(&p->crinfo.name,
				      p->crinfo.fullname.dispname)) {
			(void) cdinfo_closepipe(rpp);
			(void) cdinfo_closepipe(spp);
			CH_RET(MEM_ERR);
		}
	}

	for (q = cdinfo_dbp->disc.segment_list; q != NULL; q = q->next) {
		for (p = q->credit_list; p != NULL; p = p->next) {
			if (p->crinfo.fullname.dispname == NULL &&
			    p->crinfo.name != NULL &&
			    !util_newstr(&p->crinfo.fullname.dispname,
					 p->crinfo.name)) {
				(void) cdinfo_closepipe(rpp);
				(void) cdinfo_closepipe(spp);
				CH_RET(MEM_ERR);
			}
			else if (p->crinfo.name == NULL &&
				 p->crinfo.fullname.dispname != NULL &&
				 !util_newstr(&p->crinfo.name,
					      p->crinfo.fullname.dispname)) {
				(void) cdinfo_closepipe(rpp);
				(void) cdinfo_closepipe(spp);
				CH_RET(MEM_ERR);
			}
		}
	}

	for (i = 0; i < (int) s->tot_trks; i++) {
		if (cdinfo_dbp->track[i].artistfname.dispname == NULL &&
		    cdinfo_dbp->track[i].artist != NULL &&
		    !util_newstr(&cdinfo_dbp->track[i].artistfname.dispname,
				 cdinfo_dbp->track[i].artist)) {
			CH_RET(MEM_ERR);
		}
		else if (cdinfo_dbp->track[i].artist == NULL &&
			 cdinfo_dbp->track[i].artistfname.dispname != NULL &&
			 !util_newstr(&cdinfo_dbp->track[i].artist,
				cdinfo_dbp->track[i].artistfname.dispname)) {
			(void) cdinfo_closepipe(rpp);
			(void) cdinfo_closepipe(spp);
			CH_RET(MEM_ERR);
		}

		for (p = cdinfo_dbp->track[i].credit_list; p != NULL;
		     p = p->next) {
			if (p->crinfo.fullname.dispname == NULL &&
			    p->crinfo.name != NULL &&
			    !util_newstr(&p->crinfo.fullname.dispname,
					 p->crinfo.name)) {
				(void) cdinfo_closepipe(rpp);
				(void) cdinfo_closepipe(spp);
				CH_RET(MEM_ERR);
			}
			else if (p->crinfo.name == NULL &&
				 p->crinfo.fullname.dispname != NULL &&
				 !util_newstr(&p->crinfo.name,
					      p->crinfo.fullname.dispname)) {
				(void) cdinfo_closepipe(rpp);
				(void) cdinfo_closepipe(spp);
				CH_RET(MEM_ERR);
			}
		}

		/* If we have ISRC data that was read from the CD itself,
		 * use these instead of the ones from CDDB.
		 */
		if (s->trkinfo[i].isrc[0] != 0 &&
		    !util_newstr(&cdinfo_dbp->track[i].isrc,
				 s->trkinfo[i].isrc)) {
			(void) cdinfo_closepipe(rpp);
			(void) cdinfo_closepipe(spp);
			CH_RET(MEM_ERR);
		}
	}

	/* Write incore CD info into pipe */
	if (!cdinfo_write_datapipe(rpp, s))
		retcode = WRITE_ERR;
	else
		retcode = 0;

	(void) cdinfo_closepipe(rpp);
	(void) cdinfo_closepipe(spp);
	CH_RET(retcode);

	/*NOTREACHED*/
#endif	/* SYNCHRONOUS */
}


/*
 * cdinfo_load_matchsel
 *	Load CD database entry after user chooses a fuzzy match
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Return value will be 0 for a successful connection, and
 *	a successful query will cause the CDINFO_MATCH bit to be
 *	set in the flags field of the cdinfo_incore_t structure.
 *	If an error occurred, then the return code is as defined
 *	by cdinfo_ret_t.
 */
/*ARGSUSED*/
cdinfo_ret_t
cdinfo_load_matchsel(curstat_t *s)
{
#ifdef SYNCHRONOUS
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	cp = cdinfo_dbp->sav_cddbp;
	if (cp == NULL)
		return CDINFO_SET_CODE(READ_ERR, 0);

	/* Read CD information into incore structure */
	if (!cdinfo_querycddb(cp, s, &retcode)) {
		/* Query failed */
		(void) cdinfo_closecddb(cp);

		/* Deallocate the matchlist */
		cdinfo_free_matchlist();

		return CDINFO_SET_CODE(retcode, 0);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Deallocate the matchlist */
	cdinfo_free_matchlist();

	return 0;

#else	/* SYNCHRONOUS */
	int		ret;
	pid_t		cpid;
	waitret_t	wstat;
	cdinfo_pipe_t	*spp,
			*rpp;
	void		(*oh)(int);

	/* Restore pointers */
	rpp = cdinfo_dbp->sav_rpp;
	spp = cdinfo_dbp->sav_spp;

	/* Child pid */
	cpid = child_pid;

	if (rpp == NULL || spp == NULL || cpid == 0) {
		/* Error */
		return CDINFO_SET_CODE(READ_ERR, 0);
	}

	/* Child process should still be running.  Communicate user
	 * selection to it.
	 */
	oh = util_signal(SIGPIPE, SIG_IGN);
	if (!cdinfo_write_selpipe(spp)) {
		/* Failure */
		(void) cdinfo_closepipe(rpp);
		(void) cdinfo_closepipe(spp);
		cdinfo_load_cancel();
		(void) util_signal(SIGPIPE, oh);

		/* Deallocate the matchlist */
		cdinfo_free_matchlist();

		return CDINFO_SET_CODE(READ_ERR, 0);
	}
	(void) util_signal(SIGPIPE, oh);

	/* Deallocate the matchlist */
	cdinfo_free_matchlist();

	/* Read CD info content from pipe into in-core structure */
	(void) cdinfo_read_datapipe(rpp);

	/* Close pipes */
	(void) cdinfo_closepipe(rpp);
	(void) cdinfo_closepipe(spp);

	/* Parent process: wait for child to exit */
	ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			     cdinfo_clinfo->workproc, cdinfo_clinfo->arg,
			     TRUE, &wstat);
	child_pid = 0;

	if (ret < 0) {
		return CDINFO_SET_CODE(WAIT_ERR, errno);
	}
	if (WIFEXITED(wstat)) {
		if (WEXITSTATUS(wstat) == 0)
			return 0;
		else
			return CDINFO_SET_CODE(WEXITSTATUS(wstat), 0);
	}
	else if (WIFSIGNALED(wstat)) {
		return CDINFO_SET_CODE(KILLED_ERR, WTERMSIG(wstat));
	}
	else
		return 0;
#endif	/* SYNCHRONOUS */
}


/*
 * cdinfo_load_prog
 *	Load user-saved track program for the currently inserted CD.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Return value will be 0 for a successful load, or an error value
 *	on failure.
 */
cdinfo_ret_t
cdinfo_load_prog(curstat_t *s)
{
	FILE		*fp;
	char		*homepath,
			trk[8],
			buf[STR_BUF_SZ * 2],
			junk[STR_BUF_SZ * 2],
			path[FILE_PATH_SZ];
#ifndef SYNCHRONOUS
	pid_t		cpid;
	waitret_t	wstat;
	int		ret,
			pfd[2];

	if ((cdinfo_dbp->discid = cdinfo_discid(s)) == 0)
		return CDINFO_SET_CODE(ARG_ERR, 0);

	if (s->program || s->shuffle)
		/* Program or shuffle already in progress */
		return CDINFO_SET_CODE(BUSY_ERR, 0);

	if (PIPE(pfd) < 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_load_prog: pipe failed (errno=%d)\n", errno);
		return CDINFO_SET_CODE(OPEN_ERR, 0);
	}
	
	switch (cpid = FORK()) {
	case 0:
		/* Child */
		cdinfo_ischild = TRUE;

		(void) util_signal(SIGTERM, cdinfo_onterm);
		(void) util_signal(SIGPIPE, SIG_IGN);

		/* Close un-needed pipe descriptor */
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) close(pfd[1]);
			_exit(SETUID_ERR);
		}

		homepath = util_homedir(util_get_ouid());

		(void) sprintf(path, USR_PROG_PATH, homepath);
		(void) sprintf(path, "%s%c%08x%s",
			path, DIR_END, cdinfo_dbp->discid,
#ifdef __VMS
			"."
#else
			""
#endif
		);

		DBGPRN(DBG_CDI)(errfp, "Loading track program: %s\n", path);

		if ((fp = fopen(path, "r")) == NULL) {
			(void) close(pfd[1]);
			_exit(OPEN_ERR);
		}

		while (fgets(buf, sizeof(buf), fp) != NULL) {
			/* Skip comments */
			if (buf[0] == '#' || buf[0] == '!' || buf[0] == '\n')
				continue;
			(void) write(pfd[1], buf, strlen(buf));
		}
		(void) write(pfd[1], ".\n", 2);

		(void) close(pfd[1]);
		(void) fclose(fp);

		_exit(0);
		/*NOTREACHED*/

	case -1:
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_load_prog: fork failed (errno=%d)\n", errno);
		(void) close(pfd[0]);
		(void) close(pfd[1]);
		return CDINFO_SET_CODE(FORK_ERR, 0);
		/*NOTREACHED*/

	default:
		/* Parent */
		child_pid = cpid;

		/* Close un-needed pipe descriptor */
		(void) close(pfd[1]);

		if ((fp = fdopen(pfd[0], "r")) == NULL) {
			DBGPRN(DBG_CDI)(errfp,
				"cdinfo_load_prog: read pipe fdopen failed\n");
			return CDINFO_SET_CODE(READ_ERR, 0);
		}
		break;
	}
#else
	if ((cdinfo_dbp->discid = cdinfo_discid(s)) == 0)
		return CDINFO_SET_CODE(ARG_ERR, 0);

	if (s->program || s->shuffle)
		/* Program or shuffle already in progress */
		return CDINFO_SET_CODE(BUSY_ERR, 0);

	homepath = util_homedir(util_get_ouid());

	(void) sprintf(path, USR_PROG_PATH, homepath);
	(void) sprintf(path, "%s%c%08x%s",
		path, DIR_END, cdinfo_dbp->discid,
#ifdef __VMS
		"."
#else
		""
#endif
	);

	DBGPRN(DBG_CDI)(errfp, "Loading track program: %s\n", path);

	if ((fp = fopen(path, "r")) == NULL)
		return CDINFO_SET_CODE(OPEN_ERR, 0);
#endif	/* SYNCHRONOUS */

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		int	n;

		if (buf[0] == '.' && buf[1] == '\n')
			/* Done */
			break;

		if (buf[0] == '#' || buf[0] == '!' || buf[0] == '\n')
			/* Skip comments */
			continue;

		buf[strlen(buf)-1] = '\0';	/* Zap newline */

		/* Parse the line and check for error */
		if (sscanf(buf, "%2s %80s", trk, junk) < 1 ||
			   (n = atoi(trk)) <= 0)
			continue;

		(void) sprintf(trk, "%d", n);
		if (cdinfo_dbp->playorder == NULL) {
			if (!util_newstr(&cdinfo_dbp->playorder, trk)) {
				(void) fclose(fp);
				return CDINFO_SET_CODE(MEM_ERR, errno);
			}
		}
		else {
			cdinfo_dbp->playorder = (char *) MEM_REALLOC(
				"playorder",
				cdinfo_dbp->playorder,
				strlen(cdinfo_dbp->playorder) + strlen(trk) + 2
			);
			if (cdinfo_dbp->playorder == NULL) {
				(void) fclose(fp);
				return CDINFO_SET_CODE(MEM_ERR, errno);
			}
			(void) sprintf(cdinfo_dbp->playorder, "%s,%s",
				       cdinfo_dbp->playorder, trk);
		}
	}

	(void) fclose(fp);

#ifndef SYNCHRONOUS
	/* Parent process: wait for child to exit */
	ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			     cdinfo_clinfo->workproc, cdinfo_clinfo->arg,
			     TRUE, &wstat);
	child_pid = 0;

	if (ret < 0) {
		return CDINFO_SET_CODE(WAIT_ERR, errno);
	}
	if (WIFEXITED(wstat)) {
		if (WEXITSTATUS(wstat) == 0)
			return 0;
		else
			return CDINFO_SET_CODE(WEXITSTATUS(wstat), 0);
	}
	else if (WIFSIGNALED(wstat)) {
		return CDINFO_SET_CODE(KILLED_ERR, WTERMSIG(wstat));
	}
	else
		return 0;
#else
	return 0;
#endif
}


/*
 * cdinfo_save_prog
 *	Write user-defined track program into file for future retrieval
 *
 * Arg:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Return value will be 0 for a successful write, or an error value
 *	on failure.
 */
cdinfo_ret_t
cdinfo_save_prog(curstat_t *s)
{
	FILE		*fp;
	char		*p,
			*q,
			*tartist,
			*ttitle,
			*homepath,
			path[FILE_PATH_SZ];
	cdinfo_ret_t	retcode;
	int		i,
			err;

	if (cdinfo_dbp->discid == 0 ||
	    cdinfo_dbp->playorder == NULL ||
	    !isdigit((int) cdinfo_dbp->playorder[0]))
		return (ARG_ERR);

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	homepath = util_homedir(util_get_ouid());

	(void) sprintf(path, USR_PROG_PATH, homepath);
	(void) sprintf(path, "%s%c%08x%s",
		path, DIR_END, cdinfo_dbp->discid,
#ifdef __VMS
		"."
#else
		""
#endif
	);

	/* Remove original file */
	if (UNLINK(path) < 0 && errno != ENOENT) {
		err = errno;
		DBGPRN(DBG_CDI)(errfp, "Cannot unlink old %s\n", path);
		CH_RET(err);
	}

	/* Write new file */
	if ((fp = fopen(path, "w")) == NULL) {
		err = errno;
		DBGPRN(DBG_CDI)(errfp,
			"Cannot open file for writing: %s\n", path);
		CH_RET(err);
	}

	DBGPRN(DBG_CDI)(errfp, "Writing track program: %s\n", path);

	(void) fprintf(fp, "# xmcd %s.%s track program file\n# %s\n#\n",
		       VERSION_MAJ, VERSION_MIN, COPYRIGHT);
	(void) fprintf(fp, "# ID:     %08x\n", cdinfo_dbp->discid);
	(void) fprintf(fp, "# Artist: %.65s\n",
			(cdinfo_dbp->disc.artist != NULL) ?
				cdinfo_dbp->disc.artist : "<unknown>");
	(void) fprintf(fp, "# Title:  %.65s\n",
			(cdinfo_dbp->disc.title != NULL) ?
				cdinfo_dbp->disc.title : "<unknown>");
	(void) fprintf(fp, "# Genre:  %.65s\n#\n",
			cdinfo_genre_name(cdinfo_dbp->disc.genre));

	p = q = cdinfo_dbp->playorder;
	do {
		if ((q = strchr(p, ',')) != NULL)
			*q = '\0';
		
		for (i = 0; i < (int) s->tot_trks; i++) {
			if (s->trkinfo[i].trkno == atoi(p))
				break;
		}

		if (i < (int) s->tot_trks) {
			tartist = cdinfo_dbp->track[i].artist;
			ttitle = cdinfo_dbp->track[i].title;

			(void) fprintf(fp, "%02d  %.25s%s%.45s\n",
				s->trkinfo[i].trkno,
				(tartist == NULL) ? "" : tartist,
				(tartist != NULL && ttitle != NULL) ?
					" / " : "",
				(ttitle == NULL) ? "" : ttitle
			);
		}

		if (q != NULL)
			*q = ',';

		p = q + 1;
	} while (q != NULL);

	(void) fclose(fp);

	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_del_prog
 *	Delete the user-defined track program file for the loaded CD.
 *
 * Arg:
 *	None
 *
 * Return:
 *	Return value will be 0 for a successful delete, or an error value
 *	on failure.
 */
cdinfo_ret_t
cdinfo_del_prog(void)
{
	char		*homepath,
			path[FILE_PATH_SZ];
	cdinfo_ret_t	retcode;

	if (cdinfo_dbp->discid == 0)
		return (ARG_ERR);

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	homepath = util_homedir(util_get_ouid());

	(void) sprintf(path, USR_PROG_PATH, homepath);
	(void) sprintf(path, "%s%c%08x%s",
		path, DIR_END, cdinfo_dbp->discid,
#ifdef __VMS
		"."
#else
		""
#endif
	);

	DBGPRN(DBG_CDI)(errfp, "Deleting track program: %s\n", path);

	if (UNLINK(path) < 0)
		CH_RET(errno);

	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_reguser
 *	Register the user with CDDB
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Return value will be 0 for a successful connection, otherwise
 *	the return status will be as defined for cdinfo_ret_t.
 */
cdinfo_ret_t
cdinfo_reguser(curstat_t *s)
{
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	if (cdinfo_forkwait(&retcode)) {
		if (retcode == 0) {
			/* Clear flag */
			cdinfo_dbp->flags &= ~CDINFO_NEEDREG;
		}
		return (retcode);
	}

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL)
		CH_RET(retcode);

	/* Register user with CDDB */
	if (!cdinfo_uregcddb(cp, &retcode)) {
		(void) cdinfo_closecddb(cp);
		CH_RET(retcode);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Clear flag */
	cdinfo_dbp->flags &= ~CDINFO_NEEDREG;

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_getpasshint
 *	Request CDDB to send password hint via e-mail
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Return value will be 0 for a successful connection, otherwise
 *	the return status will be as defined for cdinfo_ret_t.
 */
cdinfo_ret_t
cdinfo_getpasshint(curstat_t *s)
{
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL)
		CH_RET(retcode);

	/* Ask CDDB to send password hint */
	if (!cdinfo_passhintcddb(cp, &retcode)) {
		(void) cdinfo_closecddb(cp);
		CH_RET(retcode);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_go_musicbrowser
 *	Invoke CDDB music browser.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Return value will be 0 for a successful connection, otherwise
 *	the return status will be as defined for cdinfo_ret_t.
 */
cdinfo_ret_t
cdinfo_go_musicbrowser(curstat_t *s)
{
	cdinfo_ret_t	retcode;
	cdinfo_cddb_t	*cp;

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL) {
		/* Failure */
		CH_RET(retcode);
	}

	/* Invoke CDDB info browser */
	if (!cdinfo_infobrowsercddb(cp)) {
		/* Failure */
		(void) cdinfo_closecddb(cp);
		CH_RET(CMD_ERR);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_go_url
 *	Invoke web browser and go to the specified URL.
 *
 * Args:
 *	url - The URL string.
 *
 * Return:
 *	Return value will be 0 for a successful connection, otherwise
 *	the return status will be as defined for cdinfo_ret_t.
 */
cdinfo_ret_t
cdinfo_go_url(char *url)
{
	int	ret;
	char	*cmd;

	if (url == NULL || url[0] == '\0')
		return CDINFO_SET_CODE(CMD_ERR, 0);

	/* Invoke web browser to the specified URL */
	cmd = (char *) MEM_ALLOC("urlstr", strlen(url) + FILE_PATH_SZ + 4);
	if (cmd == NULL)
		return CDINFO_SET_CODE(MEM_ERR, 0);

	(void) sprintf(cmd, "%s '%s'", CDINFO_GOBROWSER, url);

	ret = util_runcmd(cmd, cdinfo_clinfo->workproc, cdinfo_clinfo->arg);

	MEM_FREE(cmd);

#ifdef __VMS
	return CDINFO_SET_CODE(
		(ret == 0 || (ret & 0x1) == 1) ? 0 : CMD_ERR, 0
	);
#else
	return CDINFO_SET_CODE((ret == 0) ? 0 : CMD_ERR, 0);
#endif
}


/*
 * cdinfo_go_cddburl
 *	Invoke web browser to go to a URL supplied by CDDB
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	type - WTYPE_GEN or WTYPE_ALBUM
 *	idx - URL index as found in the cdinfo_dbp->gen_url_list
 *
 * Return:
 *	Return value will be 0 for a successful connection, otherwise
 *	the return status will be as defined for cdinfo_ret_t.
 */
cdinfo_ret_t
cdinfo_go_cddburl(curstat_t *s, int type, int idx)
{
	cdinfo_cddb_t	*cp;
	cdinfo_ret_t	retcode;

	if (type != WTYPE_GEN && type != WTYPE_ALBUM)
		return CDINFO_SET_CODE(ARG_ERR, 0);

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Open CDDB connection */
	if ((cp = cdinfo_opencddb(s, FALSE, &retcode)) == NULL) {
		/* Failure */
		CH_RET(retcode);
	}

	/* Invoke web browser to go to the specified URL */
	if (!cdinfo_urlcddb(cp, type, idx)) {
		/* Failure */
		(void) cdinfo_closecddb(cp);
		CH_RET(CMD_ERR);
	}

	/* Close CDDB connection */
	(void) cdinfo_closecddb(cp);

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_gen_discog
 *	Write out local discography file containing the currently loaded
 *	CD information.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	baseurl - The URL to the output file
 *
 * Return:
 *	Return value will be 0 for a successful connection, otherwise
 *	the return status will be as defined for cdinfo_ret_t.
 */
cdinfo_ret_t
cdinfo_gen_discog(curstat_t *s)
{
	int		i,
			n;
	char		*path,
			*url;
	bool_t		err;
	cdinfo_ret_t	retcode;

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	if (cdinfo_discog == NULL) {
		/* Local discography not defined */
		DBGPRN(DBG_CDI)(errfp,
			"Local discography not defined in %s.\n", WWWWARP_CFG);
		CH_RET(ENOENT);
	}

	i = -1;
	n = cdinfo_url_len(cdinfo_discog->arg, &cdinfo_discog->attrib, &i);
	if ((url = (char *) MEM_ALLOC("cdinfo_genurl", n)) == NULL) {
		DBGPRN(DBG_CDI)(errfp, "Out of memory.\n");
		CH_RET(ENOMEM);
	}

	/* Make the URL from template */
	cdinfo_tmpl_to_url(s, cdinfo_discog->arg, url, i);

	if (util_urlchk(url, &path, &n) & IS_REMOTE_URL) {
		/* Error: local discography cannot be remote */
		DBGPRN(DBG_CDI)(errfp,
			"Local discography URL error: %s\n", url);
		MEM_FREE(url);
		CH_RET(EINVAL);
	}

	/* Generate local discography */
	err = !cdinfo_out_discog(path, s, url);

	MEM_FREE(url);
	CH_RET(err ? EIO : 0);
	/*NOTREACHED*/
}


/*
 * cdinfo_clear
 *	Clear the in-core cdinfo_incore_t structure
 *
 * Args:
 *	reload - Whether this operation is due to a reload of the CD info
 *		(We don't want to clear a play sequence in this case).
 *
 * Return:
 *	Nothing
 */
void
cdinfo_clear(bool_t reload)
{
	int			i;
	cdinfo_url_t		*u,
				*v;
	cdinfo_credit_t		*p,
				*q;
	cdinfo_segment_t	*r,
				*s;

	if (cdinfo_dbp == NULL)
		return;

	/* Clear CD-TEXT information */
	di_clear_cdtext(&cdinfo_dbp->cdtext);

	/* Cancel pending queries */
	cdinfo_load_cancel();

	/* Disc */
	cdinfo_dbp->disc.compilation = FALSE;

	if (cdinfo_dbp->disc.artistfname.dispname != NULL) {
		MEM_FREE(cdinfo_dbp->disc.artistfname.dispname);
		cdinfo_dbp->disc.artistfname.dispname = NULL;
	}
	if (cdinfo_dbp->disc.artistfname.lastname != NULL) {
		MEM_FREE(cdinfo_dbp->disc.artistfname.lastname);
		cdinfo_dbp->disc.artistfname.lastname = NULL;
	}
	if (cdinfo_dbp->disc.artistfname.firstname != NULL) {
		MEM_FREE(cdinfo_dbp->disc.artistfname.firstname);
		cdinfo_dbp->disc.artistfname.firstname = NULL;
	}
	if (cdinfo_dbp->disc.artistfname.the != NULL) {
		MEM_FREE(cdinfo_dbp->disc.artistfname.the);
		cdinfo_dbp->disc.artistfname.the = NULL;
	}
	if (cdinfo_dbp->disc.artist != NULL) {
		MEM_FREE(cdinfo_dbp->disc.artist);
		cdinfo_dbp->disc.artist = NULL;
	}
	if (cdinfo_dbp->disc.title != NULL) {
		MEM_FREE(cdinfo_dbp->disc.title);
		cdinfo_dbp->disc.title = NULL;
	}
	if (cdinfo_dbp->disc.sorttitle != NULL) {
		MEM_FREE(cdinfo_dbp->disc.sorttitle);
		cdinfo_dbp->disc.sorttitle = NULL;
	}
	if (cdinfo_dbp->disc.title_the != NULL) {
		MEM_FREE(cdinfo_dbp->disc.title_the);
		cdinfo_dbp->disc.title_the = NULL;
	}
	if (cdinfo_dbp->disc.year != NULL) {
		MEM_FREE(cdinfo_dbp->disc.year);
		cdinfo_dbp->disc.year = NULL;
	}
	if (cdinfo_dbp->disc.label != NULL) {
		MEM_FREE(cdinfo_dbp->disc.label);
		cdinfo_dbp->disc.label = NULL;
	}
	if (cdinfo_dbp->disc.genre != NULL) {
		MEM_FREE(cdinfo_dbp->disc.genre);
		cdinfo_dbp->disc.genre = NULL;
	}
	if (cdinfo_dbp->disc.genre2 != NULL) {
		MEM_FREE(cdinfo_dbp->disc.genre2);
		cdinfo_dbp->disc.genre2 = NULL;
	}
	if (cdinfo_dbp->disc.dnum != NULL) {
		MEM_FREE(cdinfo_dbp->disc.dnum);
		cdinfo_dbp->disc.dnum = NULL;
	}
	if (cdinfo_dbp->disc.tnum != NULL) {
		MEM_FREE(cdinfo_dbp->disc.tnum);
		cdinfo_dbp->disc.tnum = NULL;
	}
	if (cdinfo_dbp->disc.region != NULL) {
		MEM_FREE(cdinfo_dbp->disc.region);
		cdinfo_dbp->disc.region = NULL;
	}
	if (cdinfo_dbp->disc.lang != NULL) {
		MEM_FREE(cdinfo_dbp->disc.lang);
		cdinfo_dbp->disc.lang = NULL;
	}
	if (cdinfo_dbp->disc.notes != NULL) {
		MEM_FREE(cdinfo_dbp->disc.notes);
		cdinfo_dbp->disc.notes = NULL;
	}
	if (cdinfo_dbp->disc.mediaid != NULL) {
		MEM_FREE(cdinfo_dbp->disc.mediaid);
		cdinfo_dbp->disc.mediaid = NULL;
	}
	if (cdinfo_dbp->disc.muiid != NULL) {
		MEM_FREE(cdinfo_dbp->disc.muiid);
		cdinfo_dbp->disc.muiid = NULL;
	}
	if (cdinfo_dbp->disc.titleuid != NULL) {
		MEM_FREE(cdinfo_dbp->disc.titleuid);
		cdinfo_dbp->disc.titleuid = NULL;
	}
	if (cdinfo_dbp->disc.revision != NULL) {
		MEM_FREE(cdinfo_dbp->disc.revision);
		cdinfo_dbp->disc.revision = NULL;
	}
	if (cdinfo_dbp->disc.revtag != NULL) {
		MEM_FREE(cdinfo_dbp->disc.revtag);
		cdinfo_dbp->disc.revtag = NULL;
	}
	if (cdinfo_dbp->disc.certifier != NULL) {
		MEM_FREE(cdinfo_dbp->disc.certifier);
		cdinfo_dbp->disc.certifier = NULL;
	}
	if (cdinfo_dbp->disc.lang != NULL) {
		MEM_FREE(cdinfo_dbp->disc.lang);
		cdinfo_dbp->disc.lang = NULL;
	}
	for (p = cdinfo_dbp->disc.credit_list; p != NULL; p = q) {
		q = p->next;
		if (p->crinfo.name != NULL)
			MEM_FREE(p->crinfo.name);
		if (p->crinfo.fullname.dispname != NULL)
			MEM_FREE(p->crinfo.fullname.dispname);
		if (p->crinfo.fullname.lastname != NULL)
			MEM_FREE(p->crinfo.fullname.lastname);
		if (p->crinfo.fullname.firstname != NULL)
			MEM_FREE(p->crinfo.fullname.firstname);
		if (p->crinfo.fullname.the != NULL)
			MEM_FREE(p->crinfo.fullname.the);
		if (p->notes != NULL)
			MEM_FREE(p->notes);
		MEM_FREE(p);
	}
	cdinfo_dbp->disc.credit_list = NULL;

	for (r = cdinfo_dbp->disc.segment_list; r != NULL; r = s) {
		s = r->next;
		if (r->name != NULL)
			MEM_FREE(r->name);
		if (r->notes != NULL)
			MEM_FREE(r->notes);
		if (r->start_track != NULL)
			MEM_FREE(r->start_track);
		if (r->start_frame != NULL)
			MEM_FREE(r->start_frame);
		if (r->end_track != NULL)
			MEM_FREE(r->end_track);
		if (r->end_frame != NULL)
			MEM_FREE(r->end_frame);
		for (p = r->credit_list; p != NULL; p = q) {
			q = p->next;
			if (p->crinfo.name != NULL)
				MEM_FREE(p->crinfo.name);
			if (p->crinfo.fullname.dispname != NULL)
				MEM_FREE(p->crinfo.fullname.dispname);
			if (p->crinfo.fullname.lastname != NULL)
				MEM_FREE(p->crinfo.fullname.lastname);
			if (p->crinfo.fullname.firstname != NULL)
				MEM_FREE(p->crinfo.fullname.firstname);
			if (p->crinfo.fullname.the != NULL)
				MEM_FREE(p->crinfo.fullname.the);
			if (p->notes != NULL)
				MEM_FREE(p->notes);
			MEM_FREE(p);
		}
		MEM_FREE(r);
	}
	cdinfo_dbp->disc.segment_list = NULL;

	/* Tracks */
	for (i = MAXTRACK-1; i >= 0; i--) {
		if (cdinfo_dbp->track[i].artistfname.dispname != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].artistfname.dispname);
			cdinfo_dbp->track[i].artistfname.dispname = NULL;
		}
		if (cdinfo_dbp->track[i].artistfname.lastname != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].artistfname.lastname);
			cdinfo_dbp->track[i].artistfname.lastname = NULL;
		}
		if (cdinfo_dbp->track[i].artistfname.firstname != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].artistfname.firstname);
			cdinfo_dbp->track[i].artistfname.firstname = NULL;
		}
		if (cdinfo_dbp->track[i].artistfname.the != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].artistfname.the);
			cdinfo_dbp->track[i].artistfname.the = NULL;
		}
		if (cdinfo_dbp->track[i].artist != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].artist);
			cdinfo_dbp->track[i].artist = NULL;
		}
		if (cdinfo_dbp->track[i].title != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].title);
			cdinfo_dbp->track[i].title = NULL;
		}
		if (cdinfo_dbp->track[i].sorttitle != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].sorttitle);
			cdinfo_dbp->track[i].sorttitle = NULL;
		}
		if (cdinfo_dbp->track[i].title_the != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].title_the);
			cdinfo_dbp->track[i].title_the = NULL;
		}
		if (cdinfo_dbp->track[i].year != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].year);
			cdinfo_dbp->track[i].year = NULL;
		}
		if (cdinfo_dbp->track[i].label != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].label);
			cdinfo_dbp->track[i].label = NULL;
		}
		if (cdinfo_dbp->track[i].genre != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].genre);
			cdinfo_dbp->track[i].genre = NULL;
		}
		if (cdinfo_dbp->track[i].genre2 != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].genre2);
			cdinfo_dbp->track[i].genre2 = NULL;
		}
		if (cdinfo_dbp->track[i].bpm != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].bpm);
			cdinfo_dbp->track[i].bpm = NULL;
		}
		if (cdinfo_dbp->track[i].notes != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].notes);
			cdinfo_dbp->track[i].notes = NULL;
		}
		if (cdinfo_dbp->track[i].isrc != NULL) {
			MEM_FREE(cdinfo_dbp->track[i].isrc);
			cdinfo_dbp->track[i].isrc = NULL;
		}
		for (p = cdinfo_dbp->track[i].credit_list; p != NULL; p = q) {
			q = p->next;
			if (p->crinfo.name != NULL)
				MEM_FREE(p->crinfo.name);
			if (p->crinfo.fullname.dispname != NULL)
				MEM_FREE(p->crinfo.fullname.dispname);
			if (p->crinfo.fullname.lastname != NULL)
				MEM_FREE(p->crinfo.fullname.lastname);
			if (p->crinfo.fullname.firstname != NULL)
				MEM_FREE(p->crinfo.fullname.firstname);
			if (p->crinfo.fullname.the != NULL)
				MEM_FREE(p->crinfo.fullname.the);
			if (p->notes != NULL)
				MEM_FREE(p->notes);
			MEM_FREE(p);
		}
		cdinfo_dbp->track[i].credit_list = NULL;
	}

	/* Album-related URLs */
	for (u = v = cdinfo_dbp->disc_url_list; u != NULL; u = v) {
		v = u->next;
		if (u->disptext != NULL)
			MEM_FREE(u->disptext);
		if (u->modifier != NULL)
			MEM_FREE(u->modifier);
		if (u->keyname != NULL)
			MEM_FREE(u->keyname);
		MEM_FREE(u);
	}
	cdinfo_dbp->disc_url_list = NULL;

	/* Misc */
	if (!reload && cdinfo_dbp->playorder != NULL) {
		MEM_FREE(cdinfo_dbp->playorder);
		cdinfo_dbp->playorder = NULL;
	}

	cdinfo_dbp->discid = 0;
	cdinfo_dbp->flags = 0;

	/* Deallocate the matchlist */
	cdinfo_free_matchlist();
}


/*
 * cdinfo_genre
 *	Given a genre ID, return a pointer to the associated genre structure.
 *
 * Args:
 *	id - The genre ID string.
 *
 * Return:
 *	Pointer to the genre structure.
 */
cdinfo_genre_t *
cdinfo_genre(char *id)
{
	cdinfo_genre_t	*gp,
			*sgp;

	if (id == NULL)
		return NULL;

	for (gp = cdinfo_dbp->genrelist; gp != NULL; gp = gp->next) {
		if (gp->id != NULL && strcmp(gp->id, id) == 0)
			return (gp);

		for (sgp = gp->child; sgp != NULL; sgp = sgp->next) {
			if (sgp->id != NULL && strcmp(sgp->id, id) == 0)
				return (sgp);
		}
	}

	/* Can't find genre entry */
	return NULL;
}


/*
 * cdinfo_genre_name
 *	Given a genre ID, return the genre name string.
 *
 * Args:
 *	id - The genre ID string.
 *
 * Return:
 *	Pointer to the genre name string.
 */
char *
cdinfo_genre_name(char *id)
{
	cdinfo_genre_t	*gp;
	static char	name[STR_BUF_SZ * 2 + 8];

	if (id == NULL)
		return ("");

	if ((gp = cdinfo_genre(id)) == NULL) {
		/* If genre entry is not found, just use the ID */
		return (id);
	}

	if (gp->parent == NULL) {
		/* Genre */
		(void) sprintf(name, "%.63s",
				gp->name == NULL ? id : gp->name);
		return (name);
	}
	else {
		/* Subgenre */
		(void) sprintf(name, "%.63s -> %.63s",
				gp->parent->name == NULL ?
					gp->parent->id : gp->parent->name,
				gp->name == NULL ? gp->id : gp->name);
		return (name);
	}

	/*NOTREACHED*/
}


/*
 * cdinfo_genre_path
 *	Return a relative file path name for a given genre ID.
 *
 * Args:
 *	id - The genre ID string
 *
 * Return:
 *	Pointer to the path name string.
 */
char *
cdinfo_genre_path(char *id)
{
	char		*cp,
			*cp2;
	cdinfo_genre_t	*gp;
	static char	buf[STR_BUF_SZ * 2 + 2];

	if (id == NULL)
#ifdef __VMS
		return ("Unknown.Unknown");
#else
		return ("Unknown/Unknown");
#endif

	if ((gp = cdinfo_genre(id)) == NULL) {
		/* If genre entry is not found, just use the ID */
		return (id);
	}

	if (gp->parent == NULL) {
		/* Genre */
		cp = cdinfo_mkpath(
			gp->name == NULL ? gp->id : gp->name,
			STR_BUF_SZ * 2
		);
		if (cp == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return ("");
		}

		(void) strcpy(buf, cp);
		MEM_FREE(cp);

		return (buf);
	}
	else {
		/* Subgenre */
		cp = cdinfo_mkpath(
			gp->parent->name == NULL ?
				gp->parent->id : gp->parent->name,
			STR_BUF_SZ
		);
		cp2 = cdinfo_mkpath(
			gp->name == NULL ? gp->id : gp->name,
			STR_BUF_SZ
		);
		if (cp == NULL || cp2 == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return ("");
		}

		(void) sprintf(buf, "%s%c%s", cp, DIR_SEP, cp2);
		MEM_FREE(cp);
		MEM_FREE(cp2);

		return (buf);
	}

	/*NOTREACHED*/
}


/*
 * cdinfo_region_name
 *	Given a region ID, return the region name.
 *
 * Args:
 *	id - The region ID string.
 *
 * Return:
 *	Pointer to the region name string.
 */
char *
cdinfo_region_name(char *id)
{
	cdinfo_region_t	*rp;

	if (id == NULL)
		return ("");

	for (rp = cdinfo_dbp->regionlist; rp != NULL; rp = rp->next) {
		if (rp->id != NULL && strcmp(rp->id, id) == 0)
			return (rp->name);
	}
	return ("");
}


/*
 * cdinfo_lang_name
 *	Given a language ID, return the language name.
 *
 * Args:
 *	id - The language ID string.
 *
 * Return:
 *	Pointer to the language name string.
 */
char *
cdinfo_lang_name(char *id)
{
	cdinfo_lang_t	*lp;

	if (id == NULL)
		return ("");

	for (lp = cdinfo_dbp->langlist; lp != NULL; lp = lp->next) {
		if (lp->id != NULL && strcmp(lp->id, id) == 0)
			return (lp->name);
	}
	return ("");
}


/*
 * cdinfo_role
 *	Given a role ID, return a pointer to the role list element.
 *
 * Args:
 *	id - The role ID string.
 *
 * Return:
 *	Pointer to the role list element.
 */
cdinfo_role_t *
cdinfo_role(char *id)
{
	cdinfo_role_t	*rp,
			*srp;

	if (id == NULL)
		return NULL;

	for (rp = cdinfo_dbp->rolelist; rp != NULL; rp = rp->next) {
		if (rp->id != NULL && strcmp(rp->id, id) == 0)
			return (rp);

		for (srp = rp->child; srp != NULL; srp = srp->next) {
			if (srp->id != NULL && strcmp(srp->id, id) == 0)
				return (srp);
		}
	}

	return NULL;
}


/*
 * cdinfo_role_name
 *	Given a role ID, return the role name.
 *
 * Args:
 *	id - The role ID string.
 *
 * Return:
 *	Pointer to the role name string.
 */
char *
cdinfo_role_name(char *id)
{
	cdinfo_role_t	*rp;

	if ((rp = cdinfo_role(id)) == NULL)
		return ("unknown role");

	return (rp->name);
}


/*
 * cdinfo_load_cancel
 *	Cancel asynchronous CD info load operation, if active.
 *
 * Args:
 *	None.
 *
 * Return:
 *	nothing.
 */
void
cdinfo_load_cancel(void)
{
	if (child_pid > 0) {
		/* Kill child process */
		(void) kill(child_pid, SIGTERM);
		child_pid = 0;
	}
}


/*
 * cdinfo_cddb_ver
 *	Returns the version string of the CDDB interface
 *
 * Args:
 *	None.
 *
 * Return:
 *	The CDDB interface version number
 */
int
cdinfo_cddb_ver(void)
{
	return (cddb_ifver());
}


/*
 * cdinfo_cddbctrl_ver
 *	Returns the version string of the CDDB control
 *
 * Args:
 *	None.
 *
 * Return:
 *	The version string.
 */
char *
cdinfo_cddbctrl_ver(void)
{
	static char	buf[STR_BUF_SZ];

	if (cdinfo_dbp->ctrl_ver == NULL)
		return ("");

	(void) sprintf(buf, "%.63s\n", cdinfo_dbp->ctrl_ver);
	return (buf);
}


/*
 * cdinfo_cddb_iscfg
 *	Returns whether CDDB is configured in the cdinfoPath parameter.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE - CDDB is configured
 *	FALSE - CDDB not configured
 */
bool_t
cdinfo_cddb_iscfg(void)
{
	cdinfo_path_t	*pp;

	for (pp = cdinfo_dbp->pathlist; pp != NULL; pp = pp->next) {
		if (pp->type == CDINFO_RMT)
			return TRUE;
	}
	return FALSE;
}


/*
 * cdinfo_cdtext_iscfg
 *	Returns whether CD-TEXT is configured in the cdinfoPath parameter.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE - CDDB is configured
 *	FALSE - CDDB not configured
 */
bool_t
cdinfo_cdtext_iscfg(void)
{
	cdinfo_path_t	*pp;

	for (pp = cdinfo_dbp->pathlist; pp != NULL; pp = pp->next) {
		if (pp->type == CDINFO_CDTEXT)
			return TRUE;
	}
	return FALSE;
}


/*
 * cdinfo_wwwwarp_parmload
 *	Load the wwwwarp configuration file and initialize structures.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_wwwwarp_parmload(void)
{
	FILE		*fp;
	char		*buf,
			*name,
			*p,
			*q,
			trypath[FILE_PATH_SZ];
	w_ent_t		*wp,
			*wp1,
			*wp2,
			*wp3;
	bool_t		newmenu;
#ifndef SYNCHRONOUS
	pid_t		cpid;
	waitret_t	wstat;
	int		ret,
			pfd[2];

	if (PIPE(pfd) < 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_wwwwarp_parmload: pipe failed (errno=%d)\n",
			errno);
		return;
	}
	
	switch (cpid = FORK()) {
	case 0:
		/* Child */
		cdinfo_ischild = TRUE;

		(void) util_signal(SIGTERM, cdinfo_onterm);
		(void) util_signal(SIGPIPE, SIG_IGN);

		/* Close un-needed pipe descriptor */
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) close(pfd[1]);
			_exit(1);
		}

		/* Try the user's own wwwwarp.cfg, if it exists */
		(void) sprintf(trypath, USR_DSCFG_PATH,
				util_homedir(util_get_ouid()), WWWWARP_CFG);
		DBGPRN(DBG_GEN|DBG_CDI)(errfp,
			"Loading wwwwarp parameters: %s\n", trypath);
		fp = fopen(trypath, "r");

		if (fp == NULL) {
			DBGPRN(DBG_GEN|DBG_CDI)(errfp, "    Cannot open %s\n",
					trypath);

			/* Try the system-wide wwwwarp.cfg, if it exists */
			(void) sprintf(trypath, SYS_DSCFG_PATH,
					app_data.libdir, WWWWARP_CFG);
			DBGPRN(DBG_GEN|DBG_CDI)(errfp,
				"Loading wwwwarp parameters: %s\n", trypath);
			fp = fopen(trypath, "r");
		}

		if (fp == NULL) {
			DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"    Cannot open %s\n", trypath);

			(void) close(pfd[1]);
			_exit(0);
		}
		
		while ((buf = cdinfo_fgetline(fp)) != NULL) {
			(void) write(pfd[1], buf, strlen(buf));
			(void) write(pfd[1], "\n", 1);
			MEM_FREE(buf);
		}
		(void) write(pfd[1], ".\n", 2);

		(void) fclose(fp);
		(void) close(pfd[1]);

		_exit(0);
		/*NOTREACHED*/

	case -1:
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_wwwwarp_parmload: fork failed (errno=%d)\n",
			errno);
		(void) close(pfd[0]);
		(void) close(pfd[1]);
		return;

	default:
		/* Parent */
		child_pid = cpid;

		/* Close un-needed pipe descriptor */
		(void) close(pfd[1]);

		if ((fp = fdopen(pfd[0], "r")) == NULL) {
			DBGPRN(DBG_CDI)(errfp,
			"cdinfo_common_parmload: read pipe fdopen failed\n");
			return;
		}
		break;
	}
#else
	/* Try the user's own wwwwarp.cfg, if it exists */
	(void) sprintf(trypath, USR_DSCFG_PATH,
			util_homedir(util_get_ouid()), WWWWARP_CFG);
	DBGPRN(DBG_GEN|DBG_CDI)(errfp, "Loading wwwwarp parameters: %s\n",
				trypath);
	fp = fopen(trypath, "r");

	if (fp == NULL) {
		DBGPRN(DBG_GEN|DBG_CDI)(errfp, "    Cannot open %s\n",
					trypath);

		/* Try the system-wide wwwwarp.cfg, if it exists */
		(void) sprintf(trypath, SYS_DSCFG_PATH,
				app_data.libdir, WWWWARP_CFG);
		DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"Loading wwwwarp parameters: %s\n",
					trypath);
		fp = fopen(trypath, "r");
	}

	if (fp == NULL) {
		DBGPRN(DBG_GEN|DBG_CDI)(errfp, "    Cannot open %s\n",
					trypath);
		return;
	}

#endif	/* SYNCHRONOUS */

	newmenu = FALSE;
	wp1 = wp2 = NULL;
	name = NULL;

	/* Read in common parameters */
	while ((buf = cdinfo_fgetline(fp)) != NULL) {
		if (strcmp(buf, ".") == 0) {
			/* Done */
			MEM_FREE(buf);
			break;
		}

		p = q = cdinfo_skip_whitespace(buf);

		if (*p == '#' || *p == '!' || *p == '\0') {
			/* Skip comments and blank lines */
			MEM_FREE(buf);
			continue;
		}

		if (*p == '{' || *p == '}') {
			/* Do nothing.  These are only visual aids for
			 * the user.
			 */
			MEM_FREE(buf);
			continue;
		}

		if (strncmp(p, "Menu", 4) == 0) {
			/* Start of new menu */
			p = cdinfo_skip_whitespace(p + 4);
			if (*p == '\0') {
				DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"    Error in %s file:\nline: %s\n",
					WWWWARP_CFG, buf);
				MEM_FREE(buf);
				break;
			}
			p = cdinfo_skip_whitespace(p);
			q = cdinfo_skip_nowhitespace(p);
			*q = '\0';

			if (!util_newstr(&name, p)) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}

			newmenu = TRUE;
		}
		else {
			/* Menu entry */
			wp = (w_ent_t *)(void *) MEM_ALLOC(
				"w_ent_t", sizeof(w_ent_t)
			);
			if (wp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) memset(wp, 0, sizeof(w_ent_t));

			if (newmenu) {
				if (cdinfo_dbp->wwwwarp_list == NULL)
					cdinfo_dbp->wwwwarp_list = wp1 = wp;
				else {
					wp1->nextmenu = wp;
					wp1 = wp;
				}
			}
			else if (wp2 == NULL) {
				DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"    Error in %s file:\nline: %s\n",
					WWWWARP_CFG, buf);
				MEM_FREE(buf);
				break;
			}
			else
				wp2->nextent = wp;

			wp2 = wp;

			if (!util_newstr(&wp->name, name)) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}

			/* Description */
			if (*p != '"') {
				DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"    Error in %s file:\nline: %s\n",
					WWWWARP_CFG, buf);
				wp->type = WTYPE_NULL;
				MEM_FREE(buf);
				break;
			}
			p++;

			if ((q = strchr(p, '"')) == NULL) {
				DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"    Error in %s file:\nline: %s\n",
					WWWWARP_CFG, buf);
				wp->type = WTYPE_NULL;
				MEM_FREE(buf);
				break;
			}
			*q = '\0';

			if (!util_newstr(&wp->desc, p)) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}

			p = cdinfo_skip_whitespace(q+1);

			/* Shortcut key */
			if (strncmp(p, "f.", 2) != 0) {
				char	*cp;

				q = cdinfo_skip_nowhitespace(p);
				if (*q == '\0') {
					DBGPRN(DBG_GEN|DBG_CDI)(errfp,
						"    Error in %s file:\n"
						"line: %s\n",
						WWWWARP_CFG, buf);
					MEM_FREE(buf);
					break;
				}
				*q = '\0';

				if ((cp = strchr(p, '-')) == NULL) {
					DBGPRN(DBG_GEN|DBG_CDI)(errfp,
						"    Error in %s file:\n"
						"line: %s\n",
						WWWWARP_CFG, buf);
					MEM_FREE(buf);
					break;
				}
				*cp = '\0';

				if (!util_newstr(&wp->modifier, p)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				if (!util_newstr(&wp->keyname, cp+1)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}

				p = cdinfo_skip_whitespace(q+1);
			}
			else {
				wp->modifier = NULL;
				wp->keyname = NULL;
			}

			/* Type */
			if (strncmp(p, "f.title", 7) == 0) {
				q = p + 7;
				wp->type = WTYPE_TITLE;
			}
			else if (strncmp(p, "f.separator2", 12) == 0) {
				q = p + 12;
				wp->type = WTYPE_SEP2;
			}
			else if (strncmp(p, "f.separator", 11) == 0) {
				q = p + 11;
				wp->type = WTYPE_SEP;
			}
			else if (strncmp(p, "f.about", 7) == 0) {
				q = p + 7;
				wp->type = WTYPE_ABOUT;
			}
			else if (strncmp(p, "f.help", 6) == 0) {
				q = p + 6;
				wp->type = WTYPE_HELP;
			}
			else if (strncmp(p, "f.motd", 6) == 0) {
				q = p + 6;
				wp->type = WTYPE_MOTD;
			}
			else if (strncmp(p, "f.goto", 6) == 0) {
				q = p + 6;
				wp->type = WTYPE_GOTO;
			}
			else if (strncmp(p, "f.menu", 6) == 0) {
				q = p + 6;
				wp->type = WTYPE_SUBMENU;
			}
			else if (strncmp(p, "f.infoBrowser", 13) == 0) {
				q = p + 13;
				wp->type = WTYPE_BROWSER;
			}
			else if (strncmp(p, "f.discog", 8) == 0) {
				q = p + 8;
				wp->type = WTYPE_DISCOG;
			}
			else if (strncmp(p, "f.generalSites", 14) == 0) {
				q = p + 14;
				wp->type = WTYPE_GEN;
			}
			else if (strncmp(p, "f.albumSites", 12) == 0) {
				q = p + 12;
				wp->type = WTYPE_ALBUM;
			}
			else if (strncmp(p, "f.submitURL", 11) == 0) {
				q = p + 11;
				wp->type = WTYPE_SUBMITURL;
			}
			else {
				DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"    Error in %s file:\nline: %s\n",
					WWWWARP_CFG, buf);
				wp->type = WTYPE_NULL;
				MEM_FREE(buf);
				break;
			}

			p = cdinfo_skip_whitespace(q);

			/* Args */
			switch(wp->type) {
			case WTYPE_SUBMENU:
				q = cdinfo_skip_nowhitespace(p);
				*q = '\0';

				if (!util_newstr(&wp->arg, p)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				break;

			case WTYPE_GOTO:
			case WTYPE_DISCOG:
				if (*p == '"')
					p++;

				q = cdinfo_skip_nowhitespace(p);
				if (*(q-1) == '"')
					*(q-1) = '\0';
				else
					*q = '\0';

				if (!util_newstr(&wp->arg, p)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}

				/* Scan URL and record its attributes */
				cdinfo_scan_url_attrib(wp->arg, &wp->attrib);

				/* Save menu pointer for later use */
				if (wp->type == WTYPE_DISCOG)
					cdinfo_discog = wp;
				else if (strcmp(wp->name,
						"searchMenu") == 0 &&
					 util_strncasecmp(wp->desc,
							  "CDDB", 4) == 0) {
					cdinfo_scddb = wp;
				}

				break;

			case WTYPE_BROWSER:
			case WTYPE_ALBUM:
				wp->attrib.acnt++;
				wp->attrib.dcnt++;
				wp->attrib.tcnt++;
				wp->attrib.icnt++;
				wp->attrib.ccnt++;
				break;

			default:
				break;
			}

			newmenu = FALSE;
		}

		MEM_FREE(buf);
	}
	if (name != NULL)
		MEM_FREE(name);

	(void) fclose(fp);

	/* Bind submenus */
	for (wp = cdinfo_dbp->wwwwarp_list; wp != NULL; wp = wp->nextmenu) {
	    for (wp2 = wp; wp2 != NULL; wp2 = wp2->nextent) {
		if (wp2->type != WTYPE_SUBMENU)
			continue;

		for (wp3 = cdinfo_dbp->wwwwarp_list; wp3 != NULL;
		     wp3 = wp3->nextmenu) {
			if (strcmp(wp2->arg, wp3->name) == 0) {
				wp2->submenu = wp3;
				break;
			}
		}
		if (wp3 == NULL) {
			DBGPRN(DBG_GEN|DBG_CDI)(errfp,
				"    Error in %s file: %s not found\n",
				WWWWARP_CFG, wp2->arg);
		}
	    }
	}

	/* Check for circular menu links */
	for (wp = cdinfo_dbp->wwwwarp_list; wp != NULL; wp = wp->nextmenu) {
		if (!cdinfo_wwwmenu_chk(wp, TRUE)) {
			DBGPRN(DBG_GEN|DBG_CDI)(errfp,
				"\n    Error in %s file: "
				"circular menu link.\n",
				WWWWARP_CFG);
		}
	}

#ifndef SYNCHRONOUS
	ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			     NULL, 0, FALSE, &wstat);
	child_pid = 0;
	if (ret < 0) {
		DBGPRN(DBG_GEN|DBG_CDI)(errfp, "cdinfo_wwwwarp_parmload: "
				"waitpid failed (errno=%d)\n",
				errno);
	}
	else if (WIFEXITED(wstat)) {
		if (WEXITSTATUS(wstat) != 0) {
			DBGPRN(DBG_GEN|DBG_CDI)(errfp,
					"cdinfo_wwwwarp_parmload: "
					"child exited (status=%d)\n",
					WEXITSTATUS(wstat));
		}
	}
	else if (WIFSIGNALED(wstat)) {
		DBGPRN(DBG_GEN|DBG_CDI)(errfp, "cdinfo_wwwwarp_parmload: "
				"child killed (signal=%d)\n",
				WTERMSIG(wstat));
	}
#endif
}


/*
 * cdinfo_wwwwarp_ckkey
 *	Given a shortcut key, check to see if it is unique amongst
 *	existing wwwwarp menu shortcuts.
 *
 * Args:
 *	keyname - The key name to check
 *
 * Return:
 *	TRUE  - is unique
 *	FALSE - not unique
 */
bool_t
cdinfo_wwwwarp_ckkey(char *keyname)
{
	w_ent_t	*p,
		*q;

	if (keyname == NULL)
		return FALSE;

	for (p = cdinfo_dbp->wwwwarp_list; p != NULL; p = p->nextmenu) {
		for (q = p; q != NULL; q = q->nextent) {
			if (q->keyname != NULL &&
			    util_strcasecmp(q->keyname, keyname) == 0)
				return FALSE;
		}
	}
	return TRUE;
}


/*
 * cdinfo_curfileupd
 *	Update the current disc info file.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing
 */
void
cdinfo_curfileupd(void)
{
#ifndef __VMS
	FILE		*fp;
	struct stat	stbuf;
	char		*artist,
			*title,
			*ttitle,
			str[FILE_PATH_SZ * 3];
	static char	prev[FILE_PATH_SZ * 3],
			prev_artist[STR_BUF_SZ],
			prev_title[STR_BUF_SZ],
			prev_ttitle[STR_BUF_SZ];
	curstat_t	*s;
	bool_t		playing,
			changed;

	if (!app_data.write_curfile ||			/* disabled */
	    strncmp(app_data.device, "(sim", 4) == 0)	/* demo */
		return;

	if (cdinfo_clinfo->curstat_addr == NULL)
		return;

	s = cdinfo_clinfo->curstat_addr();

	if (!s->devlocked)
		/* Don't write curfile if we don't own the device */
		return;

	playing = FALSE;
	artist = title = ttitle = NULL;

	if (s->mode != MOD_NODISC) {
		char	modestr[24];

		artist = cdinfo_dbp->disc.artist;
		title = cdinfo_dbp->disc.title;

		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			playing = TRUE;

			(void) sprintf(modestr, "Playing track %d",
				       s->cur_trk);

			if (s->cur_trk > 0) {
				int	n;

				n = (int) s->cur_trk - 1;

				if (s->trkinfo[n].trkno != s->cur_trk) {
					for (n = 0; n < MAXTRACK; n++) {
						if (s->trkinfo[n].trkno ==
						    s->cur_trk)
							break;
					}
				}

				ttitle = cdinfo_dbp->track[n].title;
			}
			break;

		case MOD_STOP:
			(void) strcpy(modestr, "Stopped");
			break;

		default:
			modestr[0] = '\0';
			break;
		}

		(void) sprintf(str,
			       "%s\t\t%s\n%s\t\t%s\n%s\t\t%s\n",
			       "Device:", s->curdev == NULL ? "-" : s->curdev,
			       "Genre:",
			       cdinfo_genre_name(cdinfo_dbp->disc.genre),
			       "Status:", modestr);
	}
	else {
		(void) sprintf(str,
			       "%s\t\t%s\n%s\t\t%s\n%s\t\t%s\n",
			       "Device:", s->curdev == NULL ? "-" : s->curdev,
			       "Genre:", "-",
			       "Status:", "No_Disc");
	}

	changed = FALSE;

	if (playing) {
		if (ttitle == NULL && prev_ttitle[0] != '\0')
			changed = TRUE;
		else if (ttitle != NULL &&
			 strncmp(ttitle, prev_ttitle, STR_BUF_SZ - 1) != 0)
			changed = TRUE;
	}

	if (s->mode != MOD_NODISC) {
		if (artist == NULL && prev_artist[0] != '\0')
			changed = TRUE;
		else if (title == NULL && prev_title[0] != '\0')
			changed = TRUE;
		else if (artist != NULL && 
			 strncmp(artist, prev_artist, STR_BUF_SZ - 1) != 0)
			changed = TRUE;
		else if (title != NULL && 
			 strncmp(title, prev_title, STR_BUF_SZ - 1) != 0)
			changed = TRUE;
	}

	if (strcmp(str, prev) != 0)
		changed = TRUE;

	if (!changed)
		return;

	/* Write to file */
	if (curfile[0] == '\0') {
		if (stat(app_data.device, &stbuf) < 0) {
			DBGPRN(DBG_CDI)(errfp,
				"\nCannot stat %s\n", app_data.device);
			return;
		}

		(void) sprintf(curfile, "%s/curr.%x",
			       TEMP_DIR, (int) stbuf.st_rdev);
	}

	/* Remove original file */
	if (UNLINK(curfile) < 0 && errno != ENOENT) {
		DBGPRN(DBG_CDI)(errfp, "\nCannot unlink old %s\n", curfile);
		return;
	}

	/* Write new file */
	if ((fp = fopen(curfile, "w")) == NULL) {
		DBGPRN(DBG_CDI)(errfp,
			"\nCannot open %s for writing\n", curfile);
		return;
	}

	DBGPRN(DBG_CDI)(errfp,
		"\nWriting current disc info file: %s\n", curfile);

	(void) fprintf(fp, "#\n# Xmcd current CD information\n#\n%s", str);

	if (s->mode != MOD_NODISC) {
		(void) fprintf(fp, "Disc:\t\t%s / %s\n",
			       artist == NULL ?
			       "<unknown artist>" : artist,
			       title == NULL ?
			       "<unknown disc title>" : title);
	}
	else {
		(void) fprintf(fp, "Disc:\t\t-\n");
	}

	if (playing) {
		(void) fprintf(fp, "Track:\t\t%s\n",
			       ttitle == NULL ?
			       "<unknown track title>" : ttitle);
	}
	else {
		(void) fprintf(fp, "Track:\t\t-\n");
	}

	(void) fclose(fp);
	(void) chmod(curfile, 0644);
	
	if (ttitle == NULL)
		prev_ttitle[0] = '\0';
	else {
		(void) strncpy(prev_ttitle, ttitle, STR_BUF_SZ - 1);
		prev_ttitle[STR_BUF_SZ - 1] = '\0';
	}

	if (artist == NULL)
		prev_artist[0] = '\0';
	else {
		(void) strncpy(prev_artist, artist, STR_BUF_SZ - 1);
		prev_artist[STR_BUF_SZ - 1] = '\0';
	}

	if (title == NULL)
		prev_title[0] = '\0';
	else {
		(void) strncpy(prev_title, title, STR_BUF_SZ - 1);
		prev_title[STR_BUF_SZ - 1] = '\0';
	}

	(void) strcpy(prev, str);
#endif	/* __VMS */
}


/*
 * cdinfo_issync
 *	Return a boolean value indicating whether we are running in
 *	synchronous mode.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE - synchronous mode
 *	FALSE - asynchronous mode
 */
bool_t
cdinfo_issync(void)
{
#ifdef SYNCHRONOUS
	return TRUE;
#else
	return FALSE;
#endif
}


/*
 * cdinfo_dump_incore
 *	Displays the contents of the cdinfo_incore_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Returns:
 *	Nothing.
 */
void
cdinfo_dump_incore(curstat_t *s)
{
	int		i,
			j;

	if (cdinfo_dbp == NULL)
		return;

	(void) fprintf(errfp,
		       "\nDumping the cdinfo_incore_t structure at 0x%lx:\n",
		       (unsigned long) cdinfo_dbp);
	(void) fprintf(errfp, "ctrl_ver=%s\n",
			cdinfo_dbp->ctrl_ver == NULL ?
				"NULL" : cdinfo_dbp->ctrl_ver);
	(void) fprintf(errfp, "discid=%08x flags=0x%x\n",
		       cdinfo_dbp->discid, cdinfo_dbp->flags);
	(void) fprintf(errfp, "disc: ----------------------\n");
	(void) fprintf(errfp, "  revision=%s revtag=%s region=%s lang=%s\n",
		       cdinfo_dbp->disc.revision == NULL ?
				"NULL" : cdinfo_dbp->disc.revision,
		       cdinfo_dbp->disc.revtag == NULL ?
				"NULL" : cdinfo_dbp->disc.revtag,
		       cdinfo_dbp->disc.region == NULL ?
				"NULL" : cdinfo_dbp->disc.region,
		       cdinfo_dbp->disc.lang == NULL ?
				"NULL" : cdinfo_dbp->disc.lang);
	(void) fprintf(errfp, "  compilation=%d dnum=%s tnum=%s\n",
		       (int) cdinfo_dbp->disc.compilation,
		       cdinfo_dbp->disc.dnum == NULL ?
				"NULL" : cdinfo_dbp->disc.dnum,
		       cdinfo_dbp->disc.tnum == NULL ?
				"NULL" : cdinfo_dbp->disc.tnum);
	(void) fprintf(errfp,
		       "  fullname: dispname=%s "
		       "lastname=%s firstname=%s the=%s\n",
		       cdinfo_dbp->disc.artistfname.dispname == NULL ?
				"NULL" : cdinfo_dbp->disc.artistfname.dispname,
		       cdinfo_dbp->disc.artistfname.lastname == NULL ?
				"NULL" : cdinfo_dbp->disc.artistfname.lastname,
		       cdinfo_dbp->disc.artistfname.firstname == NULL ?
				"NULL" : cdinfo_dbp->disc.artistfname.firstname,
		       cdinfo_dbp->disc.artistfname.the == NULL ?
				"NULL" : cdinfo_dbp->disc.artistfname.the);
	(void) fprintf(errfp,
		       "  artist=%s title=%s\n  sorttitle=%s title_the=%s\n",
		       cdinfo_dbp->disc.artist == NULL ?
				"NULL" : cdinfo_dbp->disc.artist,
		       cdinfo_dbp->disc.title == NULL ?
				"NULL" : cdinfo_dbp->disc.title,
		       cdinfo_dbp->disc.sorttitle == NULL ?
				"NULL" : cdinfo_dbp->disc.sorttitle,
		       cdinfo_dbp->disc.title_the == NULL ?
				"NULL" : cdinfo_dbp->disc.title_the);
	(void) fprintf(errfp, "  year=%s label=%s genre=%s genre2=%s\n",
		       cdinfo_dbp->disc.year == NULL ?
				"NULL" : cdinfo_dbp->disc.year,
		       cdinfo_dbp->disc.label == NULL ?
				"NULL" : cdinfo_dbp->disc.label,
		       cdinfo_dbp->disc.genre == NULL ?
				"NULL" : cdinfo_dbp->disc.genre,
		       cdinfo_dbp->disc.genre2 == NULL ?
				"NULL" : cdinfo_dbp->disc.genre2);
	(void) fprintf(errfp, "  mediaid=%s\n  muiid=%s titleuid=%s\n",
		       cdinfo_dbp->disc.mediaid == NULL ?
				"NULL" : cdinfo_dbp->disc.mediaid,
		       cdinfo_dbp->disc.muiid == NULL ?
				"NULL" : cdinfo_dbp->disc.muiid,
		       cdinfo_dbp->disc.titleuid == NULL ?
				"NULL" : cdinfo_dbp->disc.titleuid);
	(void) fprintf(errfp, "  certifier=%s\n",
		       cdinfo_dbp->disc.certifier == NULL ?
				"NULL" : cdinfo_dbp->disc.certifier);
	if (cdinfo_dbp->disc.notes == NULL)
		(void) fprintf(errfp, "  notes=NULL\n");
	else
		(void) fprintf(errfp, "  notes=\n%s\n", cdinfo_dbp->disc.notes);

	if (cdinfo_dbp->disc.credit_list == NULL)
		(void) fprintf(errfp, "  credit_list=NULL\n");
	else {
		cdinfo_credit_t	*p;

		(void) fprintf(errfp, "credit_list=\n");
		for (i = 0, p = cdinfo_dbp->disc.credit_list; p != NULL;
		     p = p->next, i++) {
			(void) fprintf(errfp, "  credit[%d]\n", i);
			(void) fprintf(errfp, "    role=%s name=%s\n",
				    p->crinfo.role == NULL ?
					"NULL" : p->crinfo.role->name,
				    p->crinfo.name == NULL ?
					"NULL" : p->crinfo.name);
			(void) fprintf(errfp,
			       "    fullname: dispname=%s lastname=%s "
			       "firstname=%s the=%s\n",
			       p->crinfo.fullname.dispname == NULL ?
				    "NULL" :
				    p->crinfo.fullname.dispname,
			       p->crinfo.fullname.lastname == NULL ?
				    "NULL" :
				    p->crinfo.fullname.lastname,
			       p->crinfo.fullname.firstname == NULL ?
				    "NULL" :
				    p->crinfo.fullname.firstname,
			       p->crinfo.fullname.the == NULL ?
				    "NULL" :
				    p->crinfo.fullname.the);
			if (p->notes == NULL)
				(void) fprintf(errfp,
				    "    notes=NULL\n");
			else
				(void) fprintf(errfp,
				    "    notes=\n%s\n", p->notes);
		}
	}

	if (cdinfo_dbp->disc.segment_list == NULL)
		(void) fprintf(errfp, "  segment_list=NULL\n");
	else {
		cdinfo_segment_t	*q;

		(void) fprintf(errfp, "segment_list=\n");
		for (i = 0, q = cdinfo_dbp->disc.segment_list; q != NULL;
		     q = q->next, i++) {
			(void) fprintf(errfp, "  segment[%d]\n", i);
			(void) fprintf(errfp, "    name=%s start=%s/%s ",
				    q->name == NULL ? "NULL" : q->name,
				    q->start_track == NULL ?
					    "?" : q->start_track,
				    q->start_frame == NULL ?
					    "?" : q->start_frame);
			(void) fprintf(errfp, "end=%s/%s\n",
				    q->end_track == NULL ?
					    "?" : q->end_track,
				    q->end_frame == NULL ?
					    "?" : q->end_frame);
			if (q->notes == NULL)
				(void) fprintf(errfp, "    notes=NULL\n");
			else
				(void) fprintf(errfp,
					       "    notes=\n%s\n", q->notes);

			if (q->credit_list == NULL)
			    (void) fprintf(errfp, "    credit_list=NULL\n");
			else {
			    cdinfo_credit_t	*p;

			    for (j = 0, p = q->credit_list; p != NULL;
				 p = p->next, j++) {
				(void) fprintf(errfp, "    credit[%d]\n", j);
				(void) fprintf(errfp,
					    "      role=%s name=%s\n",
					    p->crinfo.role == NULL ?
						"NULL" : p->crinfo.role->name,
					    p->crinfo.name == NULL ?
						"NULL" : p->crinfo.name);
				(void) fprintf(errfp,
				       "      fullname: dispname=%s "
				       "lastname=%s firstname=%s the=%s\n",
				       p->crinfo.fullname.dispname == NULL ?
					    "NULL" :
					    p->crinfo.fullname.dispname,
				       p->crinfo.fullname.lastname == NULL ?
					    "NULL" :
					    p->crinfo.fullname.lastname,
				       p->crinfo.fullname.firstname == NULL ?
					    "NULL" :
					    p->crinfo.fullname.firstname,
				       p->crinfo.fullname.the == NULL ?
					    "NULL" :
					    p->crinfo.fullname.the);
					if (p->notes == NULL)
					    (void) fprintf(errfp,
						    "      notes=NULL\n");
					else
					    (void) fprintf(errfp,
						    "      notes=\n%s\n",
						    p->notes);
			    }
			}
		}
	}

	for (i = 0; i < (int) s->tot_trks; i++) {
		(void) fprintf(errfp, "track[%d] ----------------------\n", i);

		(void) fprintf(errfp,
			       "  fullname: dispname=%s lastname=%s "
			       "firstname=%s the=%s\n",
			   cdinfo_dbp->track[i].artistfname.dispname == NULL ?
				    "NULL" :
				    cdinfo_dbp->track[i].artistfname.dispname,
			   cdinfo_dbp->track[i].artistfname.lastname == NULL ?
				    "NULL" :
				    cdinfo_dbp->track[i].artistfname.lastname,
			   cdinfo_dbp->track[i].artistfname.firstname == NULL ?
				    "NULL" :
				    cdinfo_dbp->track[i].artistfname.firstname,
			   cdinfo_dbp->track[i].artistfname.the == NULL ?
				    "NULL" :
				    cdinfo_dbp->track[i].artistfname.the);
		(void) fprintf(errfp, "  artist=%s title=%s\n",
			       cdinfo_dbp->track[i].artist == NULL ?
					"NULL" : cdinfo_dbp->track[i].artist,
			       cdinfo_dbp->track[i].title == NULL ?
					"NULL" : cdinfo_dbp->track[i].title);
		(void) fprintf(errfp, "  sorttitle=%s title_the=%s\n",
			       cdinfo_dbp->track[i].sorttitle == NULL ?
					"NULL" :
					cdinfo_dbp->track[i].sorttitle,
			       cdinfo_dbp->track[i].title_the == NULL ?
					"NULL" :
					cdinfo_dbp->track[i].title_the);
		(void) fprintf(errfp,
			       "  year=%s label=%s genre=%s genre2=%s\n",
			       cdinfo_dbp->track[i].year == NULL ?
					"NULL" : cdinfo_dbp->track[i].year,
			       cdinfo_dbp->track[i].label == NULL ?
					"NULL" : cdinfo_dbp->track[i].label,
			       cdinfo_dbp->track[i].genre == NULL ?
					"NULL" : cdinfo_dbp->track[i].genre,
			       cdinfo_dbp->track[i].genre2 == NULL ?
					"NULL" :
					cdinfo_dbp->track[i].genre2);
		(void) fprintf(errfp, "  isrc=%s\n",
			       cdinfo_dbp->track[i].isrc == NULL ?
					"NULL" : cdinfo_dbp->track[i].isrc);
		if (cdinfo_dbp->track[i].notes == NULL)
			(void) fprintf(errfp, "  notes=NULL\n");
		else
			(void) fprintf(errfp, "  notes=\n%s\n",
				       cdinfo_dbp->track[i].notes);

		if (cdinfo_dbp->track[i].credit_list == NULL)
			(void) fprintf(errfp, "  credit_list=NULL\n");
		else {
			cdinfo_credit_t	*p;

			(void) fprintf(errfp, "  credit_list=\n");
			for (j = 0, p = cdinfo_dbp->track[j].credit_list;
			     p != NULL; p = p->next, j++) {
				(void) fprintf(errfp, "  credit[%d]\n", j);
				(void) fprintf(errfp, "    role=%s name=%s\n",
					    p->crinfo.role == NULL ?
						"NULL" : p->crinfo.role->name,
					    p->crinfo.name == NULL ?
						"NULL" : p->crinfo.name);
				(void) fprintf(errfp,
				       "    fullname: dispname=%s lastname=%s "
				       "firstname=%s the=%s\n",
				       p->crinfo.fullname.dispname == NULL ?
					    "NULL" :
					    p->crinfo.fullname.dispname,
				       p->crinfo.fullname.lastname == NULL ?
					    "NULL" :
					    p->crinfo.fullname.lastname,
				       p->crinfo.fullname.firstname == NULL ?
					    "NULL" :
					    p->crinfo.fullname.firstname,
				       p->crinfo.fullname.the == NULL ?
					    "NULL" :
					    p->crinfo.fullname.the);
				if (p->notes == NULL)
					(void) fprintf(errfp,
					    "    notes=NULL\n");
				else
					(void) fprintf(errfp,
					    "    notes=\n%s\n", p->notes);
			}
		}
	}

	(void) fprintf(errfp, "----------------------\n");

	if (cdinfo_dbp->genrelist == NULL)
		(void) fprintf(errfp, "genrelist=NULL\n");
	else {
		cdinfo_genre_t	*p,
				*q;

		(void) fprintf(errfp, "genrelist:\n");
		for (p = cdinfo_dbp->genrelist, i = 0; p != NULL;
		     p = p->next, i++) {
		    (void) fprintf(errfp, "genre[%d]: %s (%s)\n",
		    		   i, p->name, p->id);

		    for (q = p->child, j = 0; q != NULL; q = q->next, j++) {
			(void) fprintf(errfp, "  subgenre[%d]: %s (%s)\n",
					j, q->name, q->id);
		    }
		}
	}

	if (cdinfo_dbp->regionlist == NULL)
		(void) fprintf(errfp, "regionlist=NULL\n");
	else {
		cdinfo_region_t	*p;

		(void) fprintf(errfp, "regionlist:\n");
		for (p = cdinfo_dbp->regionlist, i = 0; p != NULL;
		     p = p->next, i++) {
			(void) fprintf(errfp, "region[%d]: %s (%s)\n",
				       i, p->name, p->id);
		}
	}

	if (cdinfo_dbp->langlist == NULL)
		(void) fprintf(errfp, "langlist=NULL\n");
	else {
		cdinfo_lang_t	*p;

		(void) fprintf(errfp, "langlist:\n");
		for (p = cdinfo_dbp->langlist, i = 0; p != NULL;
		     p = p->next, i++) {
			(void) fprintf(errfp, "lang[%d]: %s (%s)\n",
				       i, p->name, p->id);
		}
	}

	if (cdinfo_dbp->rolelist == NULL)
		(void) fprintf(errfp, "rolelist=NULL\n");
	else {
		cdinfo_role_t	*p,
				*q;

		(void) fprintf(errfp, "rolelist:\n");
		for (p = cdinfo_dbp->rolelist, i = 0; p != NULL;
		     p = p->next, i++) {
			(void) fprintf(errfp, "role[%d]: %s (%s)\n",
				       i, p->name, p->id);
			for (q = p->child, j = 0; q != NULL; q = q->next, j++){
				(void) fprintf(errfp,
					       "  subrole[%d]: %s (%s)\n",
					       j, q->name, q->id);
			}
		}
	}

	if (cdinfo_dbp->matchlist == NULL)
		(void) fprintf(errfp, "matchlist=NULL\n");
	else {
		cdinfo_match_t	*p;

		for (p = cdinfo_dbp->matchlist, i = 0; p != NULL;
		     p = p->next, i++) {
			(void) fprintf(errfp,
				       "matchlist[%d] ptr=0x%lx genre=%s ",
				       i, (unsigned long) p,
				       p->genre == NULL ? "NULL" : p->genre);
			(void) fprintf(errfp, "artist=%s title=%s\n",
				       p->artist == NULL ? "NULL" : p->artist,
				       p->title == NULL ? "NULL" : p->title);
		}
	}
	if (cdinfo_dbp->match_aux == NULL)
		(void) fprintf(errfp, "match_aux=NULL\n");
	else
		(void) fprintf(errfp, "match_aux=0x%lx\n",
			       (unsigned long) cdinfo_dbp->match_aux);
	(void) fprintf(errfp, "match_tag=%ld\n", cdinfo_dbp->match_tag);

	if (cdinfo_dbp->gen_url_list == NULL)
		(void) fprintf(errfp, "gen_url_list=NULL\n");
	else {
		cdinfo_url_t	*u;

		for (u = cdinfo_dbp->gen_url_list, i = 0; u != NULL;
		     u = u->next, i++) {
			(void) fprintf(errfp,
			       "gen_url_list[%d] ptr=0x%lx key=%s-%s "
			       "type=%s size=%s weight=%s categ=%s\n",
			       i, (unsigned long) u,
			       u->modifier == NULL ? "NULL" : u->modifier,
			       u->keyname == NULL ? "NULL" : u->keyname,
			       u->type == NULL ? "NULL" : u->type,
			       u->size == NULL ? "NULL" : u->size,
			       u->weight == NULL ? "NULL" : u->weight,
			       u->categ == NULL ? "NULL" : u->categ);

			(void) fprintf(errfp,
			       "                 href=%s\n",
			       u->href == NULL ? "NULL" : u->href);
			(void) fprintf(errfp,
			       "                 displink=%s\n",
			       u->displink == NULL ? "NULL" : u->displink);
			(void) fprintf(errfp,
			       "                 disptext=%s\n",
			       u->disptext == NULL ? "NULL" : u->disptext);
		}
	}
	if (cdinfo_dbp->disc_url_list == NULL)
		(void) fprintf(errfp, "disc_url_list=NULL\n");
	else {
		cdinfo_url_t	*u;

		for (u = cdinfo_dbp->disc_url_list, i = 0; u != NULL;
		     u = u->next, i++) {
			(void) fprintf(errfp,
			       "gen_disc_list[%d] ptr=0x%lx key=%s-%s "
			       "type=%s size=%s weight=%s categ=%s\n",
			       i, (unsigned long) u,
			       u->modifier == NULL ? "NULL" : u->modifier,
			       u->keyname == NULL ? "NULL" : u->keyname,
			       u->type == NULL ? "NULL" : u->type,
			       u->size == NULL ? "NULL" : u->size,
			       u->weight == NULL ? "NULL" : u->weight,
			       u->categ == NULL ? "NULL" : u->categ);

			(void) fprintf(errfp,
			       "                  href=%s\n",
			       u->href == NULL ? "NULL" : u->href);
			(void) fprintf(errfp,
			       "                  displink=%s\n",
			       u->displink == NULL ? "NULL" : u->displink);
			(void) fprintf(errfp,
			       "                  disptext=%s\n",
			       u->disptext == NULL ? "NULL" : u->disptext);
		}
	}

	(void) fprintf(errfp, "playorder=%s\n",
		       cdinfo_dbp->playorder == NULL ?
				"NULL" : cdinfo_dbp->playorder);
}


