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
static char *_cdinfo_i_c_ident_ = "@(#)cdinfo_i.c	7.190 04/04/20";
#endif

#ifdef __VMS
typedef char *	caddr_t;
#endif

#define _CDINFO_INTERN	/* Expose internal function protos in cdinfo.h */
#define XMCD_CDDB	/* Enable correct includes in CDDB2API.h */

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdinfo_d/cdinfo.h"
#include "cddb_d/CDDB2API.h"
#if defined(_AIX) || defined(__QNX__)
#include <sys/select.h>
#endif


/* Time interval to run workproc while waiting for I/O */
#define CDINFO_WAIT_SEC		0
#define CDINFO_WAIT_USEC	200000


extern appdata_t	app_data;
extern FILE		*errfp;

/* Internal data not to be used outside of libcdinfo */
cdinfo_client_t		*cdinfo_clinfo;		/* Client info */
cdinfo_incore_t		*cdinfo_dbp;		/* In core CD info struct */
cdinfo_cddb_t		*cdinfo_cddbp;		/* Opened CDDB handle */
w_ent_t			*cdinfo_discog,		/* Local discog menu ptr */
			*cdinfo_scddb;		/* Search CDDB menu ptr */
bool_t			cdinfo_ischild;		/* Is a child process */

STATIC bool_t		cdinfo_ctrl_initted = FALSE,
						/* CDDB ctrl initialized */
			cdinfo_genurl_initted = FALSE,
						/* Gen URLs list initialized */
			cdinfo_glist_initted = FALSE,
						/* Genre list initialized */
			cdinfo_relist_initted = FALSE,
						/* Region list initialized */
			cdinfo_langlist_initted = FALSE,
						/* Language list initialized */
			cdinfo_rolist_initted = FALSE,
						/* Role list initialized */
			cdinfo_user_regd = FALSE;
						/* User registered */

/* Data structure used for checking wwwwarp.cfg entries */
typedef struct w_entchk {
	w_ent_t		*ent;
	struct w_entchk	*next;
} w_entchk_t;

STATIC w_entchk_t	*cdinfo_wentchk_head = NULL;


/* Data structure for play lists */
typedef struct playls {
	char		*path;		/* File path string */
	struct playls	*prev;		/* List link pointer */
	struct playls	*next;		/* List link pointer */
	struct playls	*prev2;		/* Sorted list link pointer */
	struct playls	*next2;		/* Sorted list link pointer */
} playls_t;


/* CDDB1 to CDDB2 genre mapping table - This assumes that the
 * CDDB2 genre ID is invariant.
 */
struct {
	char	*cddb1_genre;	/* CDDB1 category name */
	char	*cddb2_genre;	/* CDDB2 genre ID */
} cdinfo_genre_map[] = {
	{ "blues",	"32"	/* Blues -> General Blues */		},
	{ "classical",	"46"	/* Classical -> General Classical */	},
	{ "country",	"60"	/* Country -> General Country */	},
	{ "data",	"71"	/* Data -> General Data */		},
	{ "folk",	"96"	/* Folk -> General Folk */		},
	{ "jazz",	"160"	/* Jazz -> General Jazz */		},
	{ "misc",	"221"	/* Unclassifiable -> General Unclass */	},
	{ "newage",	"169"	/* New Age -> General New Age */	},
	{ "reggae",	"246"	/* Reggae -> General Reggae */		},
	{ "rock",	"191"	/* Rock -> General Rock */		},
	{ "soundtrack",	"214"	/* Sountrack -> General Soundtrack */	},
	{ "unclass",	"221"	/* Unclassifiable -> General Unclass */	},
	{ NULL,		NULL						}
};


/***********************
 *  internal routines  *
 ***********************/


/*
 * cdinfo_line_filter
 *	Given a string, process it such that becomes only one
 *	single line (i.e., filter out all text including and
 *	following any newline character).  A newline character
 *	is denotes as the characters '\' and 'n' (NOT '\n').
 *	Used for handling local CD info file data.
 *
 * Arg:
 *	str - The string to be processed
 *
 * Return:
 *	Nothing.  The input string may be modified.
 */
STATIC void
cdinfo_line_filter(char *str)
{
	if (str == NULL)
		return;

	for (; *str != '\0'; str++) {
		if (*str == '\\' && *(str+1) == 'n') {
			*str = '\0';
			break;
		}
	}
}


/*
 * cdinfo_concatstr
 *	Concatenate two text strings with special handling for newline
 *	and tab character translations.  s1 will be dynamically allocated
 *	or reallocated as needed, and must not point to statically-allocated
 *	or stack space.  Used for handling local CD info file data.
 *
 * Args:
 *	s1 - Location of the pointer to the first text string.
 *	s2 - The second text string.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_concatstr(char **s1, char *s2)
{
	int	n;
	char	*str;
	bool_t	proc_slash;

	if (s1 == NULL || s2 == NULL)
		return FALSE;

	if (*s1 == NULL) {
		*s1 = (char *) MEM_ALLOC("s1", strlen(s2) + 1);
		if (*s1 != NULL)
			*s1[0] = '\0';
	}
	else {
		*s1 = (char *) MEM_REALLOC("s1", *s1,
					   strlen(*s1) + strlen(s2) + 1);
	}
	if (*s1 == NULL)
		return FALSE;

	/* Concatenate the two strings, with special handling for newline
	 * and tab characters.
	 */
	str = *s1;
	proc_slash = FALSE;
	n = strlen(str);
	str += n;

	if (n > 0 && *(str - 1) == '\\') {
		proc_slash = TRUE;	/* Handle broken escape sequences */
		str--;
	}

	for (; *s2 != '\0'; str++, s2++) {
		if (*s2 == '\\') {
			if (proc_slash) {
				proc_slash = FALSE;
				continue;
			}
			proc_slash = TRUE;
			s2++;
		}

		if (proc_slash) {
			proc_slash = FALSE;

			switch (*s2) {
			case 'n':
				*str = '\n';
				break;
			case 't':
				*str = '\t';
				break;
			case '\\':
				*str = '\\';
				break;
			case '\0':
				*str = '\\';
				s2--;
				break;
			default:
				*str++ = '\\';
				*str = *s2;
				break;
			}
		}
		else
			*str = *s2;
	}
	*str = '\0';
	
	return TRUE;
}


/*
 * cdinfo_sort_playlist
 *	Given a linked list of playls_t structures, sort them
 *	alphabetically based on the path field.  The sorted list head
 *	is returned.  The original list links are not modified.
 *	To traverse the sorted list, use returned listhead and follow the
 *	next2 pointers of the playls_t structure.
 *
 * Args:
 *	listhead - The original playlist head pointer.
 *
 * Return:
 *	The sorted playlist head pointer.
 */
STATIC playls_t *
cdinfo_sort_playlist(playls_t *listhead)
{
	playls_t	*hp,
			*sp,
			*sp2;

	for (sp = hp = listhead; sp != NULL; sp = sp->next) {
		for (sp2 = hp; sp2 != NULL; sp2 = sp2->next2) {
			if (sp == sp2)
				continue;

			if (strcmp(sp->path, sp2->path) > 0)
				continue;

			sp->prev2 = sp2->prev2;
			sp->next2 = sp2;

			if (sp2 == hp)
				hp = sp;
			else
				sp2->prev2->next2 = sp;

			break;
		}
	}

	return (hp);
}


/*
 * cdinfo_onterm
 *	Signal handler for SIGTERM
 *
 * Args:
 *	signo - The signal number
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_onterm(int signo)
{
	(void) util_signal(signo, SIG_IGN);
	DBGPRN(DBG_CDI)(errfp, "\ncdinfo_onterm: SIGTERM received\n");

	/* Close CDDB connection if necessary */
	if (cdinfo_cddbp != NULL)
		cdinfo_closecddb(cdinfo_cddbp);
	_exit(0);
}


/*
 * cdinfo_sum
 *      Convert an integer to its text string representation, and
 *      compute its checksum.  Used by cdinfo_discid.
 *
 * Args:
 *      n - The integer value.
 *
 * Return:
 *      The integer checksum.
 */
int
cdinfo_sum(int n)
{
 	int	ret;

	/* For backward compatibility this algorithm must not change */
	for (ret = 0; n > 0; n /= 10)
		ret += n % 10;

	return (ret);
}


/*
 * cdinfo_free_glist
 *	Deallocate genre list.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
cdinfo_free_glist(void)
{
	cdinfo_genre_t	*p,
			*q,
			*a,
			*b;

	for (p = q = cdinfo_dbp->genrelist; p != NULL; p = q) {
		q = p->next;

		if (p->id != NULL)
			MEM_FREE(p->id);
		if (p->name != NULL)
			MEM_FREE(p->name);

		for (a = b = p->child; a != NULL; a = b) {
			b = a->next;

			if (a->id != NULL)
				MEM_FREE(a->id);
			if (a->name != NULL)
				MEM_FREE(a->name);

			MEM_FREE(a);
		}

		MEM_FREE(p);
	}

	cdinfo_dbp->genrelist = NULL;
}


/*
 * cdinfo_free_relist
 *	Deallocate region list.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
cdinfo_free_relist(void)
{
	cdinfo_region_t	*p,
			*q;

	for (p = q = cdinfo_dbp->regionlist; p != NULL; p = q) {
		q = p->next;

		if (p->id != NULL)
			MEM_FREE(p->id);
		if (p->name != NULL)
			MEM_FREE(p->name);

		MEM_FREE(p);
	}
	cdinfo_dbp->regionlist = NULL;
}


/*
 * cdinfo_free_langlist
 *	Deallocate language list.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
cdinfo_free_langlist(void)
{
	cdinfo_lang_t	*p,
			*q;

	for (p = q = cdinfo_dbp->langlist; p != NULL; p = q) {
		q = p->next;

		if (p->id != NULL)
			MEM_FREE(p->id);
		if (p->name != NULL)
			MEM_FREE(p->name);

		MEM_FREE(p);
	}
	cdinfo_dbp->langlist = NULL;
}


/*
 * cdinfo_free_rolist
 *	Deallocate role list.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
cdinfo_free_rolist(void)
{
	cdinfo_role_t	*p,
			*q,
			*a,
			*b;

	for (p = q = cdinfo_dbp->rolelist; p != NULL; p = q) {
		q = p->next;

		if (p->id != NULL)
			MEM_FREE(p->id);
		if (p->name != NULL)
			MEM_FREE(p->name);

		for (a = b = p->child; a != NULL; a = b) {
			b = a->next;
			if (a->id != NULL)
				MEM_FREE(a->id);
			if (a->name != NULL)
				MEM_FREE(a->name);
			MEM_FREE(a);
		}

		MEM_FREE(p);
	}

	cdinfo_dbp->rolelist = NULL;
}


/*
 * cdinfo_set_errstr
 *	Set the error message string in the incore structure based on the
 *	CDDB service result code.
 *
 * Args:
 *	code - The CDDB result code
 *
 * Return:
 *	Nothing
 */
/*ARGSUSED*/
STATIC void
cdinfo_set_errstr(CddbResult code)
{
	if (code == Cddb_OK)
		return;

	if (cdinfo_dbp->cddb_errstr != NULL)
		MEM_FREE(cdinfo_dbp->cddb_errstr);

	cdinfo_dbp->cddb_errstr = (char *) MEM_ALLOC(
		"CddbErrorString", ERR_BUF_SZ
	);
	if (cdinfo_dbp->cddb_errstr == NULL)
		return;

	cdinfo_dbp->cddb_errstr[0] = '\0';

	CddbGetErrorString(code, cdinfo_dbp->cddb_errstr, ERR_BUF_SZ);

	if (cdinfo_dbp->cddb_errstr[0] == '\0') {
		MEM_FREE(cdinfo_dbp->cddb_errstr);
		cdinfo_dbp->cddb_errstr = NULL;
	}
	else {
		DBGPRN(DBG_CDI)(errfp, "%s\n", cdinfo_dbp->cddb_errstr);
	}
}


#ifndef SYNCHRONOUS

/*
 * cdinfo_waitio
 *	Check if read data is pending.  If no data and a workproc is
 *	specified, then call workproc.  Keep waiting until there is
 *	data or until timeout.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure
 *	tmout - Timeout interval (in seconds).  If set to 0, no
 *		timeout will occur.
 *
 * Return:
 *	TRUE - There is data pending.
 */
STATIC bool_t
cdinfo_waitio(cdinfo_pipe_t *pp, int tmout)
{
	int		ret;
	fd_set		rfds;
	fd_set		efds;
	struct timeval	to;
	time_t		start = 0,
			now;

	if (tmout > 0)
		start = time(NULL);

	do {
		errno = 0;
		FD_ZERO(&efds);
		FD_ZERO(&rfds);
		FD_SET(pp->r.fd, &rfds);

		to.tv_sec = CDINFO_WAIT_SEC;
		to.tv_usec = CDINFO_WAIT_USEC;
#ifdef __hpux
		ret = select(pp->r.fd+1, (int *) &rfds,
			     NULL, (int *) &efds, &to);
#else
		ret = select(pp->r.fd+1, &rfds, NULL, &efds, &to);
#endif

		if (tmout > 0 && ret == 0) {
			now = time(NULL);
			if ((now - start) > tmout) {
				/* Timeout */
				DBGPRN(DBG_CDI)(errfp,
					"Timed out waiting for data.\n");
				errno = ETIME;
				ret = -1;
			}
		}

		if (ret == 0 && !cdinfo_ischild &&
		    cdinfo_clinfo->workproc != NULL)
			cdinfo_clinfo->workproc(cdinfo_clinfo->arg);

	} while (ret == 0);

	/* Hack: some implementations of select() does not work
	 * on non-socket file descriptors, so just fake a
	 * success status.
	 */
	return TRUE;
}


/*
 * cdinfo_getc
 *	Return a character from the file stream.  Perform buffered
 *	read from file if necessary.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure.
 *
 * Return:
 *	The character, or -1 on EOF or failure.
 */
STATIC int
cdinfo_getc(cdinfo_pipe_t *pp)
{
	static int	i = 0;

	/* Do some work every 10 characters read */
	if (!cdinfo_ischild && (i % 10) == 0 &&
	    cdinfo_clinfo->workproc != NULL)
		cdinfo_clinfo->workproc(cdinfo_clinfo->arg);

	if (++i > 100)
		i = 0;	/* Reset count */

	if (pp->r.cache == NULL) {
		/* Allocate read cache */
		pp->r.cache = (unsigned char *) MEM_ALLOC(
			"read_cache", CDINFO_CACHESZ
		);
		if (pp->r.cache == NULL)
			return -1;
	}

	if (pp->r.pos == pp->r.cnt) {
		/* Wait for data */
		if (!cdinfo_waitio(pp, 0))
			return -1;

		/* Load cache */
		pp->r.cnt = read(pp->r.fd, pp->r.cache, CDINFO_CACHESZ);
		if (pp->r.cnt <= 0) {
			pp->r.cnt = pp->r.pos;
			return -1;
		}
		pp->r.pos = 1;
	}
	else {
		pp->r.pos++;
	}

	return ((int) pp->r.cache[pp->r.pos - 1]);
}


/*
 * cdinfo_gets
 *	Read a line of text from the stream.  The read terminates
 *	when it encounters a newline character, EOF, or if len
 *	characters have been read.
 *
 * Args:
 *	pp - The pipe descriptor obtained via cdinfo_openpipe()
 *	buf - Pointer to return string buffer
 *	len - Maximum number of characters to read
 *
 * Return:
 *	TRUE - Successfully read a line of text
 *	FALSE - Reached EOF or read failure
 */
STATIC bool_t
cdinfo_gets(cdinfo_pipe_t *pp, char *buf, int len)
{
	int	tot = 0,
		c = 0;

	while ((c = cdinfo_getc(pp)) > 0) {
		*buf = (char) c;

		tot++;
		buf++;
		len--;

		if (c == '\n') {
			/* Translate CR-LF into just LF */
			if (*(buf-2) == '\r') {
				*(buf-2) = '\n';
				*(buf-1) = '\0';
				tot--;
				buf--;
			}
			break;
		}

		if (len <= 0)
			break;
	}
	*buf = '\0';
	return ((bool_t) (tot > 0));
}


/*
 * cdinfo_puts
 *	Write a text string into the pipe.
 *
 * Args:
 *	pp - The pipe descriptor obtained via cdinfo_openpipe()
 *	buf - Pointer to string buffer
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_puts(cdinfo_pipe_t *pp, char *buf)
{
	char		prevchar;
	bool_t		done;

	if (buf == NULL)
		return FALSE;

	if (pp->w.cache == NULL) {
		/* Allocate write cache */
		pp->w.cache = (unsigned char *) MEM_ALLOC(
			"write_cache", CDINFO_CACHESZ
		);
		if (pp->w.cache == NULL)
			return FALSE;
	}

	prevchar = '\0';
	done = FALSE;

	while (!done) {
		if (*buf == '\0') {
			/* Insert a newline at end of string if the
			 * string doesn't already have one
			 */
			if (prevchar != '\n')
				buf = "\n";
			else
				break;
		}

		if (pp->w.pos == CDINFO_CACHESZ) {
			unsigned char	*p;
			int		resid,
					n = 0;

			/* Flush write cache */
			p = pp->w.cache;
			for (resid = pp->w.pos; resid > 0; resid -= n) {
				if ((n = write(pp->w.fd, p, resid)) < 0) {
					pp->w.pos = 0;
					return FALSE;
				}
				p += n;
			}
			pp->w.pos = 0;
		}

		pp->w.cache[pp->w.pos] = (unsigned char) *buf;
		pp->w.pos++;
		prevchar = *buf;
		buf++;
	}

	return TRUE;
}


/*
 * cdinfo_getline
 *	Read a line from pipe buffer and allocate data structure element
 *	to store the data.  This version is used for single-line character
 *	string elements.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	name - Name of element
 *	ptr - Incore CD info structure element location
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
STATIC bool_t
cdinfo_getline(cdinfo_pipe_t *pp, char *name, char **ptr)
{
	size_t		n;
	bool_t		ret = FALSE;
	static char	buf[2048];

	while ((ret = cdinfo_gets(pp, buf, sizeof(buf))) == TRUE) {
		if (buf[0] == '\n')
			/* Null string */
			return TRUE;

		n = strlen(buf);

		if (buf[n-1] == '\n') {
			buf[n-1] = '\0';	/* Zap newline */
			ret = TRUE;
		}
		else
			ret = FALSE;

		if (*ptr == NULL) {
			*ptr = (char *) MEM_ALLOC(name, n + 1);
			if (*ptr != NULL)
				*ptr[0] = '\0';
		}
		else {
			*ptr = (char *) MEM_REALLOC(name, *ptr,
						    strlen(*ptr) + n + 1);
		}

		if (*ptr == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) strcat(*ptr, buf);

		if (ret)
			break;
	}

	return (ret);
}


/*
 * cdinfo_getmultiline
 *	Read lines from pipe buffer and allocate data structure
 *	element to store the data.  This version is used for multi-line
 *	character string elements.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	name - Name of element
 *	ptr - Incore CD info structure element location
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
STATIC bool_t
cdinfo_getmultiline(cdinfo_pipe_t *pp, char *name, char **ptr)
{
	bool_t		ret;
	char		*cp;
	static char	buf[2048];

	while ((ret = cdinfo_gets(pp, buf, sizeof(buf))) == TRUE) {
		if (strncmp(buf, XMCD_PIPESIG, strlen(XMCD_PIPESIG)) == 0) {
			/* End of multi-line */
			if (*ptr != NULL) {
				/* Zap the last newline */
				cp = *ptr + strlen(*ptr) - 1;
				if (*cp == '\n')
					*cp = '\0';
			}
			break;
		}

		if (*ptr == NULL) {
			*ptr = (char *) MEM_ALLOC(name, strlen(buf) + 1);
			if (*ptr != NULL)
				*ptr[0] = '\0';
		}
		else {
			*ptr = (char *) MEM_REALLOC(name, *ptr,
						    strlen(*ptr) +
						    strlen(buf) + 1);
		}

		if (*ptr == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) strcat(*ptr, buf);
	}

	return (ret);
}


/*
 * cdinfo_geturl_ents
 *	Read URL elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - match element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_geturl_ents(cdinfo_pipe_t *pp, char *key)
{
	static cdinfo_url_t	*p,
				*q = NULL;

	if (strcmp(key, ".gen.new") == 0) {
		p = (cdinfo_url_t *)(void *) MEM_ALLOC("cdinfo_url_t",
			sizeof(cdinfo_url_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_url_t));

		p->wtype = WTYPE_GEN;

		if (cdinfo_dbp->gen_url_list == NULL)
			cdinfo_dbp->gen_url_list = q = p;
		else {
			q->next = p;
			q = p;
		}

		cdinfo_genurl_initted = TRUE;
	}
	else if (strcmp(key, ".gen.type") == 0) {
		if (!cdinfo_getline(pp, "url.gen.type", &p->type))
			return FALSE;
	}
	else if (strcmp(key, ".gen.href") == 0) {
		if (!cdinfo_getline(pp, "url.gen.href", &p->href))
			return FALSE;
	}
	else if (strcmp(key, ".gen.displink") == 0) {
		if (!cdinfo_getline(pp, "url.gen.displink", &p->displink))
			return FALSE;
	}
	else if (strcmp(key, ".gen.disptext") == 0) {
		if (!cdinfo_getline(pp, "url.gen.disptext", &p->disptext))
			return FALSE;
	}
	else if (strcmp(key, ".gen.categ") == 0) {
		if (!cdinfo_getline(pp, "url.gen.categ", &p->categ))
			return FALSE;
	}
	else if (strcmp(key, ".gen.size") == 0) {
		if (!cdinfo_getline(pp, "url.gen.size", &p->size))
			return FALSE;
	}
	else if (strcmp(key, ".gen.weight") == 0) {
		if (!cdinfo_getline(pp, "url.gen.weight", &p->weight))
			return FALSE;
	}
	else if (strcmp(key, ".disc.new") == 0) {
		p = (cdinfo_url_t *)(void *) MEM_ALLOC("cdinfo_url_t",
			sizeof(cdinfo_url_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_url_t));

		p->wtype = WTYPE_ALBUM;

		if (cdinfo_dbp->disc_url_list == NULL)
			cdinfo_dbp->disc_url_list = q = p;
		else {
			q->next = p;
			q = p;
		}
	}
	else if (strcmp(key, ".disc.type") == 0) {
		if (!cdinfo_getline(pp, "url.disc.type", &p->type))
			return FALSE;
	}
	else if (strcmp(key, ".disc.href") == 0) {
		if (!cdinfo_getline(pp, "url.disc.href", &p->href))
			return FALSE;
	}
	else if (strcmp(key, ".disc.displink") == 0) {
		if (!cdinfo_getline(pp, "url.disc.displink", &p->displink))
			return FALSE;
	}
	else if (strcmp(key, ".disc.disptext") == 0) {
		if (!cdinfo_getline(pp, "url.disc.disptext", &p->disptext))
			return FALSE;
	}
	else if (strcmp(key, ".disc.categ") == 0) {
		if (!cdinfo_getline(pp, "url.disc.categ", &p->categ))
			return FALSE;
	}
	else if (strcmp(key, ".disc.size") == 0) {
		if (!cdinfo_getline(pp, "url.disc.size", &p->size))
			return FALSE;
	}
	else if (strcmp(key, ".disc.weight") == 0) {
		if (!cdinfo_getline(pp, "url.disc.weight", &p->weight))
			return FALSE;
	}

	return TRUE;
}


/*
 * cdinfo_getctrl_ents
 *	Read CDDB control information from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - control element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getctrl_ents(cdinfo_pipe_t *pp, char *key)
{
	if (strcmp(key, ".ver") == 0) {
		if (!cdinfo_getline(pp, "control.ver",
				    &cdinfo_dbp->ctrl_ver))
			return FALSE;
	}

	cdinfo_ctrl_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_getglist_ents
 *	Read genre list elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - genre element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getglist_ents(cdinfo_pipe_t *pp, char *key)
{
	cdinfo_genre_t		*p;
	static cdinfo_genre_t	*gp = NULL,
				*sgp = NULL;

	if (strcmp(key, ".new") == 0) {
		p = (cdinfo_genre_t *)(void *) MEM_ALLOC("genre",
			sizeof(cdinfo_genre_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_genre_t));

		if (cdinfo_dbp->genrelist == NULL)
			cdinfo_dbp->genrelist = gp = p;
		else {
			gp->next = p;
			gp = p;
		}
		sgp = NULL;
	}
	else if (strcmp(key, ".id") == 0) {
		if (!cdinfo_getline(pp, "genre.id", &gp->id)) {
			gp = sgp = NULL;
			cdinfo_free_glist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".name") == 0) {
		if (!cdinfo_getline(pp, "genre.name", &gp->name)) {
			gp = sgp = NULL;
			cdinfo_free_glist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".subgenre.new") == 0) {
		p = (cdinfo_genre_t *)(void *) MEM_ALLOC("genre",
			sizeof(cdinfo_genre_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_genre_t));

		p->parent = gp;
		if (sgp == NULL)
			gp->child = sgp = p;
		else {
			sgp->next = p;
			sgp = p;
		}
	}
	else if (strcmp(key, ".subgenre.id") == 0) {
		if (!cdinfo_getline(pp, "subgenre.id", &sgp->id)) {
			gp = sgp = NULL;
			cdinfo_free_glist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".subgenre.name") == 0) {
		if (!cdinfo_getline(pp, "subgenre.name", &sgp->name)) {
			gp = sgp = NULL;
			cdinfo_free_glist();
			return FALSE;
		}
	}

	cdinfo_glist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_getrelist_ents
 *	Read region list elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - region element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getrelist_ents(cdinfo_pipe_t *pp, char *key)
{
	cdinfo_region_t		*p;
	static cdinfo_region_t	*rp = NULL;

	if (strcmp(key, ".new") == 0) {
		p = (cdinfo_region_t *)(void *) MEM_ALLOC("region",
			sizeof(cdinfo_region_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_region_t));

		if (cdinfo_dbp->regionlist == NULL)
			cdinfo_dbp->regionlist = rp = p;
		else {
			rp->next = p;
			rp = p;
		}
	}
	else if (strcmp(key, ".id") == 0) {
		if (!cdinfo_getline(pp, "region.id", &rp->id)) {
			rp = NULL;
			cdinfo_free_relist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".name") == 0) {
		if (!cdinfo_getline(pp, "region.name", &rp->name)) {
			rp = NULL;
			cdinfo_free_relist();
			return FALSE;
		}
	}

	cdinfo_relist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_getlanglist_ents
 *	Read language list elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - language element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getlanglist_ents(cdinfo_pipe_t *pp, char *key)
{
	cdinfo_lang_t		*p;
	static cdinfo_lang_t	*lp = NULL;

	if (strcmp(key, ".new") == 0) {
		p = (cdinfo_lang_t *)(void *) MEM_ALLOC("language",
			sizeof(cdinfo_lang_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_lang_t));

		if (cdinfo_dbp->langlist == NULL)
			cdinfo_dbp->langlist = lp = p;
		else {
			lp->next = p;
			lp = p;
		}
	}
	else if (strcmp(key, ".id") == 0) {
		if (!cdinfo_getline(pp, "lang.id", &lp->id)) {
			lp = NULL;
			cdinfo_free_langlist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".name") == 0) {
		if (!cdinfo_getline(pp, "lang.name", &lp->name)) {
			lp = NULL;
			cdinfo_free_langlist();
			return FALSE;
		}
	}

	cdinfo_langlist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_getrolist_ents
 *	Read role list elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - role element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getrolist_ents(cdinfo_pipe_t *pp, char *key)
{
	cdinfo_role_t		*p;
	static cdinfo_role_t	*rp = NULL,
				*srp = NULL;

	if (strcmp(key, ".new") == 0) {
		p = (cdinfo_role_t *)(void *) MEM_ALLOC("role",
			sizeof(cdinfo_role_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_role_t));

		if (cdinfo_dbp->rolelist == NULL)
			cdinfo_dbp->rolelist = rp = p;
		else {
			rp->next = p;
			rp = p;
		}
		srp = NULL;
	}
	else if (strcmp(key, ".id") == 0) {
		if (!cdinfo_getline(pp, "role.id", &rp->id)) {
			rp = srp = NULL;
			cdinfo_free_rolist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".name") == 0) {
		if (!cdinfo_getline(pp, "role.name", &rp->name)) {
			rp = srp = NULL;
			cdinfo_free_rolist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".subrole.new") == 0) {
		p = (cdinfo_role_t *)(void *) MEM_ALLOC("role",
			sizeof(cdinfo_role_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_role_t));

		p->parent = rp;
		if (srp == NULL)
			rp->child = srp = p;
		else {
			srp->next = p;
			srp = p;
		}
	}
	else if (strcmp(key, ".subrole.id") == 0) {
		if (!cdinfo_getline(pp, "subrole.id", &srp->id)) {
			rp = srp = NULL;
			cdinfo_free_rolist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".subrole.name") == 0) {
		if (!cdinfo_getline(pp, "subrole.name", &srp->name)) {
			rp = srp = NULL;
			cdinfo_free_rolist();
			return FALSE;
		}
	}
	cdinfo_rolist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_getmatch_ents
 *	Read multiple match elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - match element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getmatch_ents(cdinfo_pipe_t *pp, char *key)
{
	cdinfo_match_t		*p;
	static cdinfo_match_t	*mp = NULL;

	if (strcmp(key, ".new") == 0) {
		p = (cdinfo_match_t *)(void *) MEM_ALLOC("match",
			sizeof(cdinfo_match_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_match_t));

		if (cdinfo_dbp->matchlist == NULL)
			cdinfo_dbp->matchlist = mp = p;
		else {
			mp->next = p;
			mp = p;
		}
	}
	else if (strcmp(key, ".artist") == 0) {
		if (!cdinfo_getline(pp, "match.artist", &mp->artist)) {
			mp = NULL;
			cdinfo_free_matchlist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".title") == 0) {
		if (!cdinfo_getline(pp, "match.title", &mp->title)) {
			mp = NULL;
			cdinfo_free_matchlist();
			return FALSE;
		}
	}
	else if (strcmp(key, ".genre") == 0) {
		if (!cdinfo_getline(pp, "match.genre", &mp->genre)) {
			mp = NULL;
			cdinfo_free_matchlist();
			return FALSE;
		}
	}

	return TRUE;
}


/*
 * cdinfo_getuserreg_ents
 *	Read user registration related elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - userreg element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getuserreg_ents(cdinfo_pipe_t *pp, char *key)
{
	char	*str;

	if (strcmp(key, ".handle") == 0) {
		if (cdinfo_dbp->userreg.handle != NULL) {
			MEM_FREE(cdinfo_dbp->userreg.handle);
			cdinfo_dbp->userreg.handle = NULL;
		}
		if (!cdinfo_getline(pp, "userreg.handle",
				    &cdinfo_dbp->userreg.handle))
			return FALSE;
	}
	else if (strcmp(key, ".hint") == 0) {
		if (cdinfo_dbp->userreg.hint != NULL) {
			MEM_FREE(cdinfo_dbp->userreg.hint);
			cdinfo_dbp->userreg.hint = NULL;
		}
		if (!cdinfo_getline(pp, "userreg.hint",
				    &cdinfo_dbp->userreg.hint))
			return FALSE;
	}
	else if (strcmp(key, ".email") == 0) {
		if (cdinfo_dbp->userreg.email != NULL) {
			MEM_FREE(cdinfo_dbp->userreg.email);
			cdinfo_dbp->userreg.email = NULL;
		}
		if (!cdinfo_getline(pp, "userreg.email",
				    &cdinfo_dbp->userreg.email))
			return FALSE;
	}
	else if (strcmp(key, ".region") == 0) {
		if (cdinfo_dbp->userreg.region != NULL) {
			MEM_FREE(cdinfo_dbp->userreg.region);
			cdinfo_dbp->userreg.region = NULL;
		}
		if (!cdinfo_getline(pp, "userreg.region",
				    &cdinfo_dbp->userreg.region))
			return FALSE;
	}
	else if (strcmp(key, ".postal") == 0) {
		if (cdinfo_dbp->userreg.postal != NULL) {
			MEM_FREE(cdinfo_dbp->userreg.postal);
			cdinfo_dbp->userreg.postal = NULL;
		}
		if (!cdinfo_getline(pp, "userreg.postal",
				    &cdinfo_dbp->userreg.postal))
			return FALSE;
	}
	else if (strcmp(key, ".age") == 0) {
		if (cdinfo_dbp->userreg.age != NULL) {
			MEM_FREE(cdinfo_dbp->userreg.age);
			cdinfo_dbp->userreg.age = NULL;
		}
		if (!cdinfo_getline(pp, "userreg.age",
				    &cdinfo_dbp->userreg.age))
			return FALSE;
	}
	else if (strcmp(key, ".gender") == 0) {
		if (cdinfo_dbp->userreg.gender != NULL) {
			MEM_FREE(cdinfo_dbp->userreg.gender);
			cdinfo_dbp->userreg.gender = NULL;
		}
		if (!cdinfo_getline(pp, "userreg.gender",
				    &cdinfo_dbp->userreg.gender))
			return FALSE;
	}
	else if (strcmp(key, ".allowemail") == 0) {
		str = NULL;
		if (!cdinfo_getline(pp, "userreg.allowemail", &str))
			return FALSE;
		if (str != NULL) {
			if (*str == '1')
				cdinfo_dbp->userreg.allowemail = TRUE;
			MEM_FREE(str);
		}
	}
	else if (strcmp(key, ".allowstats") == 0) {
		str = NULL;
		if (!cdinfo_getline(pp, "userreg.allowstats", &str))
			return FALSE;
		if (str != NULL) {
			if (*str == '1')
				cdinfo_dbp->userreg.allowemail = TRUE;
			MEM_FREE(str);
		}
	}

	cdinfo_user_regd = TRUE;
	return TRUE;
}


/*
 * cdinfo_getdisc_ents
 *	Read disc related elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - disc element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_getdisc_ents(cdinfo_pipe_t *pp, char *key)
{
	char			*str;
	cdinfo_credit_t		*p;
	cdinfo_segment_t	*q;
	static cdinfo_credit_t	*cp = NULL;
	static cdinfo_segment_t	*sp = NULL;

	if (strcmp(key, ".flags") == 0) {
		str = NULL;
		if (!cdinfo_getline(pp, "disc.flags", &str))
			return FALSE;
		if (str != NULL) {
			(void) sscanf(str, "%x", &cdinfo_dbp->flags);
			MEM_FREE(str);
		}
	}
	else if (strcmp(key, ".compilation") == 0) {
		str = NULL;
		if (!cdinfo_getline(pp, "disc.compilation", &str))
			return FALSE;
		if (str != NULL) {
			if (*str == '1')
				cdinfo_dbp->disc.compilation = TRUE;
			MEM_FREE(str);
		}
	}
	else if (strcmp(key, ".artistfname.dispname") == 0) {
		if (!cdinfo_getline(pp, "disc.artistfname.dispname",
				    &cdinfo_dbp->disc.artistfname.dispname))
			return FALSE;
	}
	else if (strcmp(key, ".artistfname.lastname") == 0) {
		if (!cdinfo_getline(pp, "disc.artistfname.lastname",
				    &cdinfo_dbp->disc.artistfname.lastname))
			return FALSE;
	}
	else if (strcmp(key, ".artistfname.firstname") == 0) {
		if (!cdinfo_getline(pp, "disc.artistfname.firstname",
				    &cdinfo_dbp->disc.artistfname.firstname))
			return FALSE;
	}
	else if (strcmp(key, ".artistfname.the") == 0) {
		if (!cdinfo_getline(pp, "disc.artistfname.the",
				    &cdinfo_dbp->disc.artistfname.the))
			return FALSE;
	}
	else if (strcmp(key, ".artist") == 0) {
		if (!cdinfo_getline(pp,
				    "disc.artist", &cdinfo_dbp->disc.artist))
			return FALSE;
	}
	else if (strcmp(key, ".title") == 0) {
		if (!cdinfo_getline(pp, "disc.title", &cdinfo_dbp->disc.title))
			return FALSE;
	}
	else if (strcmp(key, ".sorttitle") == 0) {
		if (!cdinfo_getline(pp, "disc.sorttitle",
				    &cdinfo_dbp->disc.sorttitle))
			return FALSE;
	}
	else if (strcmp(key, ".title_the") == 0) {
		if (!cdinfo_getline(pp, "disc.title_the",
				    &cdinfo_dbp->disc.title_the))
			return FALSE;
	}
	else if (strcmp(key, ".year") == 0) {
		if (!cdinfo_getline(pp, "disc.year", &cdinfo_dbp->disc.year))
			return FALSE;
	}
	else if (strcmp(key, ".label") == 0) {
		if (!cdinfo_getline(pp, "disc.label", &cdinfo_dbp->disc.label))
			return FALSE;
	}
	else if (strcmp(key, ".genre") == 0) {
		if (!cdinfo_getline(pp, "disc.genre", &cdinfo_dbp->disc.genre))
			return FALSE;
	}
	else if (strcmp(key, ".genre2") == 0) {
		if (!cdinfo_getline(pp, "disc.genre2",
				    &cdinfo_dbp->disc.genre2))
			return FALSE;
	}
	else if (strcmp(key, ".dnum") == 0) {
		if (!cdinfo_getline(pp, "disc.dnum", &cdinfo_dbp->disc.dnum))
			return FALSE;
		}
	else if (strcmp(key, ".tnum") == 0) {
		if (!cdinfo_getline(pp, "disc.tnum", &cdinfo_dbp->disc.tnum))
			return FALSE;
	}
	else if (strcmp(key, ".region") == 0) {
		if (!cdinfo_getline(pp,
				    "disc.region", &cdinfo_dbp->disc.region))
			return FALSE;
	}
	else if (strcmp(key, ".lang") == 0) {
		if (!cdinfo_getline(pp, "disc.lang", &cdinfo_dbp->disc.lang))
			return FALSE;
	}
	else if (strcmp(key, ".notes") == 0) {
		if (!cdinfo_getmultiline(pp, "disc.notes",
					 &cdinfo_dbp->disc.notes))
			return FALSE;
	}
	else if (strcmp(key, ".mediaid") == 0) {
		if (!cdinfo_getline(pp, "disc.mediaid",
				    &cdinfo_dbp->disc.mediaid))
			return FALSE;
	}
	else if (strcmp(key, ".muiid") == 0) {
		if (!cdinfo_getline(pp,
				    "disc.muiid", &cdinfo_dbp->disc.muiid))
			return FALSE;
	}
	else if (strcmp(key, ".titleuid") == 0) {
		if (!cdinfo_getline(pp, "disc.titleuid",
				    &cdinfo_dbp->disc.titleuid))
			return FALSE;
	}
	else if (strcmp(key, ".revision") == 0) {
		if (!cdinfo_getline(pp, "disc.revision",
				    &cdinfo_dbp->disc.revision))
			return FALSE;
	}
	else if (strcmp(key, ".revtag") == 0) {
		if (!cdinfo_getline(pp,
				    "disc.revtag", &cdinfo_dbp->disc.revtag))
			return FALSE;
	}
	else if (strcmp(key, ".certifier") == 0) {
		if (!cdinfo_getline(pp, "disc.certifier",
				    &cdinfo_dbp->disc.certifier))
			return FALSE;
	}
	else if (strcmp(key, ".credit.new") == 0) {
		p = (cdinfo_credit_t *)(void *) MEM_ALLOC("cdinfo_credit_t",
			sizeof(cdinfo_credit_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_credit_t));

		if (cdinfo_dbp->disc.credit_list == NULL) {
			cdinfo_dbp->disc.credit_list = p;
			p->prev = NULL;
		}
		else {
			cp->next = p;
			p->prev = cp;
		}
		cp = p;
		p->next = NULL;
	}
	else if (strcmp(key, ".credit.role") == 0) {
		str = NULL;
		if (!cdinfo_getline(pp, "disc.credit.role", &str))
			return FALSE;
		if (str != NULL) {
			cp->crinfo.role = cdinfo_role(str);
			MEM_FREE(str);
		}
	}
	else if (strcmp(key, ".credit.name") == 0) {
		if (!cdinfo_getline(pp, "disc.credit.name",
				    &cp->crinfo.name))
			return FALSE;
	}
	else if (strcmp(key, ".credit.fullname.dispname") == 0) {
		if (!cdinfo_getline(pp, "disc.credit.fullname.dispname",
				    &cp->crinfo.fullname.dispname))
			return FALSE;
	}
	else if (strcmp(key, ".credit.fullname.lastname") == 0) {
		if (!cdinfo_getline(pp, "disc.credit.fullname.lastname",
				    &cp->crinfo.fullname.lastname))
			return FALSE;
	}
	else if (strcmp(key, ".credit.fullname.firstname") == 0) {
		if (!cdinfo_getline(pp, "disc.credit.fullname.firstname",
				    &cp->crinfo.fullname.firstname))
			return FALSE;
	}
	else if (strcmp(key, ".credit.fullname.the") == 0) {
		if (!cdinfo_getline(pp, "disc.credit.fullname.the",
				    &cp->crinfo.fullname.the))
			return FALSE;
	}
	else if (strcmp(key, ".credit.notes") == 0) {
		if (!cdinfo_getmultiline(pp, "disc.credit.notes",
				    &cp->notes))
			return FALSE;
	}
	else if (strcmp(key, ".segment.new") == 0) {
		q = (cdinfo_segment_t *)(void *) MEM_ALLOC("cdinfo_segment_t",
			sizeof(cdinfo_segment_t)
		);
		if (q == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(q, 0, sizeof(cdinfo_segment_t));

		if (cdinfo_dbp->disc.segment_list == NULL) {
			cdinfo_dbp->disc.segment_list = q;
			q->prev = NULL;
		}
		else {
			sp->next = q;
			q->prev = sp;
		}
		sp = q;
		q->next = NULL;
	}
	else if (strcmp(key, ".segment.name") == 0) {
		if (!cdinfo_getline(pp, "disc.segment.name",
				    &sp->name))
			return FALSE;
	}
	else if (strcmp(key, ".segment.notes") == 0) {
		if (!cdinfo_getmultiline(pp, "disc.segment.notes",
				    &sp->notes))
			return FALSE;
	}
	else if (strcmp(key, ".segment.start_track") == 0) {
		if (!cdinfo_getline(pp, "disc.segment.start_track",
				    &sp->start_track))
			return FALSE;
	}
	else if (strcmp(key, ".segment.start_frame") == 0) {
		if (!cdinfo_getline(pp, "disc.segment.start_frame",
				    &sp->start_frame))
			return FALSE;
	}
	else if (strcmp(key, ".segment.end_track") == 0) {
		if (!cdinfo_getline(pp, "disc.segment.end_track",
				    &sp->end_track))
			return FALSE;
	}
	else if (strcmp(key, ".segment.end_frame") == 0) {
		if (!cdinfo_getline(pp, "disc.segment.end_frame",
				    &sp->end_frame))
			return FALSE;
	}
	else if (strcmp(key, ".segment.credit.new") == 0) {
		p = (cdinfo_credit_t *)(void *) MEM_ALLOC("cdinfo_credit_t",
			sizeof(cdinfo_credit_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_credit_t));

		if (sp->credit_list == NULL) {
			sp->credit_list = p;
			p->prev = NULL;
		}
		else {
			cp->next = p;
			p->prev = cp;
		}
		cp = p;
		p->next = NULL;
	}
	else if (strcmp(key, ".segment.credit.role") == 0) {
		str = NULL;
		if (!cdinfo_getline(pp, "disc.segment.credit.role", &str))
			return FALSE;
		if (str != NULL) {
			cp->crinfo.role = cdinfo_role(str);
			MEM_FREE(str);
		}
	}
	else if (strcmp(key, ".segment.credit.name") == 0) {
		if (!cdinfo_getline(pp, "disc.segment.credit.name",
				    &cp->crinfo.name))
			return FALSE;
	}
	else if (strcmp(key, ".segment.credit.fullname.dispname") == 0) {
		if (!cdinfo_getline(pp,
				    "disc.segment.credit.fullname.dispname",
				    &cp->crinfo.fullname.dispname))
			return FALSE;
	}
	else if (strcmp(key, ".segment.credit.fullname.lastname") == 0) {
		if (!cdinfo_getline(pp,
				    "disc.segment.credit.fullname.lastname",
				    &cp->crinfo.fullname.lastname))
			return FALSE;
	}
	else if (strcmp(key, ".segment.credit.fullname.firstname") == 0) {
		if (!cdinfo_getline(pp,
				    "disc.segment.credit.fullname.firstname",
				    &cp->crinfo.fullname.firstname))
			return FALSE;
	}
	else if (strcmp(key, ".segment.credit.fullname.the") == 0) {
		if (!cdinfo_getline(pp, "disc.segment.credit.fullname.the",
				    &cp->crinfo.fullname.the))
			return FALSE;
	}
	else if (strcmp(key, ".segment.credit.notes") == 0) {
		if (!cdinfo_getmultiline(pp, "disc.segment.credit.notes",
				    &cp->notes))
			return FALSE;
	}

	return TRUE;
}


/*
 * cdinfo_gettrack_ents
 *	Read track related elements from pipe, used by the parent.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	key - track element key string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_gettrack_ents(cdinfo_pipe_t *pp, char *key)
{
	int			i;
	char			*str,
				*name;
	cdinfo_credit_t		*p;
	static cdinfo_credit_t	*cp = NULL;

	if (sscanf(key, "%d.", &i) != 1)
		return FALSE;	/* Unexpected data */

	name = strchr(key, '.') + 1;

	if (strcmp(name, "artistfname.dispname") == 0) {
		if (!cdinfo_getline(pp, name,
				&cdinfo_dbp->track[i].artistfname.dispname))
			return FALSE;
	}
	else if (strcmp(name, "artistfname.lastname") == 0) {
		if (!cdinfo_getline(pp, name,
				&cdinfo_dbp->track[i].artistfname.lastname))
			return FALSE;
	}
	else if (strcmp(name, "artistfname.firstname") == 0) {
		if (!cdinfo_getline(pp, name,
				&cdinfo_dbp->track[i].artistfname.firstname))
			return FALSE;
	}
	else if (strcmp(name, "artistfname.the") == 0) {
		if (!cdinfo_getline(pp, name,
				    &cdinfo_dbp->track[i].artistfname.the))
			return FALSE;
	}
	else if (strcmp(name, "artist") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].artist))
			return FALSE;
	}
	else if (strcmp(name, "title") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].title))
			return FALSE;
	}
	else if (strcmp(name, "sorttitle") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].sorttitle))
			return FALSE;
	}
	else if (strcmp(name, "title_the") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].title_the))
			return FALSE;
	}
	else if (strcmp(name, "year") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].year))
			return FALSE;
	}
	else if (strcmp(name, "label") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].label))
			return FALSE;
	}
	else if (strcmp(name, "genre") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].genre))
			return FALSE;
	}
	else if (strcmp(name, "genre2") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].genre2))
			return FALSE;
	}
	else if (strcmp(name, "bpm") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].bpm))
			return FALSE;
	}
	else if (strcmp(name, "notes") == 0) {
		if (!cdinfo_getmultiline(pp, name,
					 &cdinfo_dbp->track[i].notes))
			return FALSE;
	}
	else if (strcmp(name, "isrc") == 0) {
		if (!cdinfo_getline(pp, name, &cdinfo_dbp->track[i].isrc))
			return FALSE;
	}
	else if (strcmp(name, "credit.new") == 0) {
		p = (cdinfo_credit_t *)(void *) MEM_ALLOC("cdinfo_credit_t",
			sizeof(cdinfo_credit_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_credit_t));

		if (cdinfo_dbp->track[i].credit_list == NULL) {
			cdinfo_dbp->track[i].credit_list = p;
			p->prev = NULL;
		}
		else {
			cp->next = p;
			p->prev = cp;
		}
		cp = p;
		p->next = NULL;
	}
	else if (strcmp(name, "credit.role") == 0) {
		str = NULL;
		if (!cdinfo_getline(pp, name, &str))
			return FALSE;
		if (str != NULL) {
			cp->crinfo.role = cdinfo_role(str);
			MEM_FREE(str);
		}
	}
	else if (strcmp(name, "credit.name") == 0) {
		if (!cdinfo_getline(pp, name, &cp->crinfo.name))
			return FALSE;
	}
	else if (strcmp(name, "credit.fullname.dispname") == 0) {
		if (!cdinfo_getline(pp, name, &cp->crinfo.fullname.dispname))
			return FALSE;
	}
	else if (strcmp(name, "credit.fullname.lastname") == 0) {
		if (!cdinfo_getline(pp, name, &cp->crinfo.fullname.lastname))
			return FALSE;
	}
	else if (strcmp(name, "credit.fullname.firstname") == 0) {
		if (!cdinfo_getline(pp, name, &cp->crinfo.fullname.firstname))
			return FALSE;
	}
	else if (strcmp(name, "credit.fullname.the") == 0) {
		if (!cdinfo_getline(pp, name, &cp->crinfo.fullname.the))
			return FALSE;
	}
	else if (strcmp(name, "credit.notes") == 0) {
		if (!cdinfo_getmultiline(pp, name, &cp->notes))
			return FALSE;
	}

	return TRUE;
}


/*
 * cdinfo_puturl_ents
 *	Write URL match elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_puturl_ents(cdinfo_pipe_t *pp)
{
	cdinfo_url_t	*p;

	if (!cdinfo_genurl_initted) {
	    for (p = cdinfo_dbp->gen_url_list; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.new"))
			return FALSE;

		if (p->type != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.type"))
				return FALSE;
			if (!cdinfo_puts(pp, p->type))
				return FALSE;
		}
		if (p->href != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.href"))
				return FALSE;
			if (!cdinfo_puts(pp, p->href))
				return FALSE;
		}
		if (p->displink != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.displink"))
				return FALSE;
			if (!cdinfo_puts(pp, p->displink))
				return FALSE;
		}
		if (p->disptext != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.disptext"))
				return FALSE;
			if (!cdinfo_puts(pp, p->disptext))
				return FALSE;
		}
		if (p->categ != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.categ"))
				return FALSE;
			if (!cdinfo_puts(pp, p->categ))
				return FALSE;
		}
		if (p->size != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.size"))
				return FALSE;
			if (!cdinfo_puts(pp, p->size))
				return FALSE;
		}
		if (p->weight != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.gen.weight"))
				return FALSE;
			if (!cdinfo_puts(pp, p->weight))
				return FALSE;
		}

		cdinfo_genurl_initted = TRUE;
	    }
	}

	for (p = cdinfo_dbp->disc_url_list; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.new"))
			return FALSE;

		if (p->type != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.type"))
				return FALSE;
			if (!cdinfo_puts(pp, p->type))
				return FALSE;
		}
		if (p->href != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.href"))
				return FALSE;
			if (!cdinfo_puts(pp, p->href))
				return FALSE;
		}
		if (p->displink != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.displink"))
				return FALSE;
			if (!cdinfo_puts(pp, p->displink))
				return FALSE;
		}
		if (p->disptext != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.disptext"))
				return FALSE;
			if (!cdinfo_puts(pp, p->disptext))
				return FALSE;
		}
		if (p->categ != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.categ"))
				return FALSE;
			if (!cdinfo_puts(pp, p->categ))
				return FALSE;
		}
		if (p->size != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.size"))
				return FALSE;
			if (!cdinfo_puts(pp, p->size))
				return FALSE;
		}
		if (p->weight != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "url.disc.weight"))
				return FALSE;
			if (!cdinfo_puts(pp, p->weight))
				return FALSE;
		}
	}

	return TRUE;
}


/*
 * cdinfo_putctrl_ents
 *	Write CDDB control information into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putctrl_ents(cdinfo_pipe_t *pp)
{
	if (cdinfo_ctrl_initted)
		return TRUE;

	if (cdinfo_dbp->ctrl_ver != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "control.ver"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->ctrl_ver))
			return FALSE;
	}

	cdinfo_ctrl_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_putglist_ents
 *	Write genre list elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putglist_ents(cdinfo_pipe_t *pp)
{
	cdinfo_genre_t	*p,
			*q;

	if (cdinfo_glist_initted)
		return TRUE;

	for (p = cdinfo_dbp->genrelist; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "genre.new"))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "genre.id"))
			return FALSE;
		if (!cdinfo_puts(pp, p->id))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "genre.name"))
			return FALSE;
		if (!cdinfo_puts(pp, p->name))
			return FALSE;

		for (q = p->child; q != NULL; q = q->next) {
			if (!cdinfo_puts(pp,
					 XMCD_PIPESIG "genre.subgenre.new"))
				return FALSE;

			if (!cdinfo_puts(pp,
					 XMCD_PIPESIG "genre.subgenre.id"))
				return FALSE;
			if (!cdinfo_puts(pp, q->id))
				return FALSE;

			if (!cdinfo_puts(pp,
					 XMCD_PIPESIG "genre.subgenre.name"))
				return FALSE;
			if (!cdinfo_puts(pp, q->name))
				return FALSE;
		}
	}

	cdinfo_glist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_putrelist_ents
 *	Write region list elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putrelist_ents(cdinfo_pipe_t *pp)
{
	cdinfo_region_t	*p;

	if (cdinfo_relist_initted)	
		return TRUE;

	for (p = cdinfo_dbp->regionlist; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "region.new"))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "region.id"))
			return FALSE;
		if (!cdinfo_puts(pp, p->id))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "region.name"))
			return FALSE;
		if (!cdinfo_puts(pp, p->name))
			return FALSE;
	}

	cdinfo_relist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_putlanglist_ents
 *	Write language list elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putlanglist_ents(cdinfo_pipe_t *pp)
{
	cdinfo_lang_t	*p;

	if (cdinfo_langlist_initted)	
		return TRUE;

	for (p = cdinfo_dbp->langlist; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "lang.new"))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "lang.id"))
			return FALSE;
		if (!cdinfo_puts(pp, p->id))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "lang.name"))
			return FALSE;
		if (!cdinfo_puts(pp, p->name))
			return FALSE;
	}

	cdinfo_langlist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_putrolist_ents
 *	Write role list elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putrolist_ents(cdinfo_pipe_t *pp)
{
	cdinfo_role_t	*p,
			*q;

	if (cdinfo_rolist_initted)
		return TRUE;

	for (p = cdinfo_dbp->rolelist; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "role.new"))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "role.id"))
			return FALSE;
		if (!cdinfo_puts(pp, p->id))
			return FALSE;

		if (!cdinfo_puts(pp, XMCD_PIPESIG "role.name"))
			return FALSE;
		if (!cdinfo_puts(pp, p->name))
			return FALSE;

		for (q = p->child; q != NULL; q = q->next) {
			if (!cdinfo_puts(pp,
					 XMCD_PIPESIG "role.subrole.new"))
				return FALSE;

			if (!cdinfo_puts(pp,
					 XMCD_PIPESIG "role.subrole.id"))
				return FALSE;
			if (!cdinfo_puts(pp, q->id))
				return FALSE;

			if (!cdinfo_puts(pp,
					 XMCD_PIPESIG "role.subrole.name"))
				return FALSE;
			if (!cdinfo_puts(pp, q->name))
				return FALSE;
		}
	}

	cdinfo_rolist_initted = TRUE;
	return TRUE;
}


/*
 * cdinfo_putmatch_ents
 *	Write multiple match elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putmatch_ents(cdinfo_pipe_t *pp)
{
	cdinfo_match_t	*p;

	for (p = cdinfo_dbp->matchlist; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "match.new"))
			return FALSE;
		if (!cdinfo_puts(pp, XMCD_PIPESIG "match.artist"))
			return FALSE;
		if (!cdinfo_puts(pp, p->artist))
			return FALSE;
		if (!cdinfo_puts(pp, XMCD_PIPESIG "match.title"))
			return FALSE;
		if (!cdinfo_puts(pp, p->title))
			return FALSE;
		if (!cdinfo_puts(pp, XMCD_PIPESIG "match.genre"))
			return FALSE;
		if (!cdinfo_puts(pp, p->genre))
			return FALSE;
	}

	return TRUE;
}


/*
 * cdinfo_putuserreg_ents
 *	Write user registration related elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putuserreg_ents(cdinfo_pipe_t *pp)
{
	if (cdinfo_user_regd)
		return TRUE;

	if (cdinfo_dbp->userreg.handle != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.handle"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->userreg.handle))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.hint != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.hint"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->userreg.hint))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.email != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.email"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->userreg.email))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.region != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.region"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->userreg.region))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.postal != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.postal"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->userreg.postal))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.age != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.age"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->userreg.age))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.gender != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.gender"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->userreg.gender))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.allowemail) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.allowemail"))
			return FALSE;
		if (!cdinfo_puts(pp, "1\n"))
			return FALSE;
	}
	if (cdinfo_dbp->userreg.allowstats) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "userreg.allowstats"))
			return FALSE;
		if (!cdinfo_puts(pp, "1\n"))
			return FALSE;
	}

	cdinfo_user_regd = TRUE;
	return TRUE;
}


/*
 * cdinfo_putdisc_ents
 *	Write disc related elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_putdisc_ents(cdinfo_pipe_t *pp)
{
	char			buf[32];
	cdinfo_credit_t		*p;
	cdinfo_segment_t	*q;

	if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.flags"))
		return FALSE;
	(void) sprintf(buf, "%x", cdinfo_dbp->flags);
	if (!cdinfo_puts(pp, buf))
		return FALSE;

	if (cdinfo_dbp->disc.compilation) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.compilation"))
			return FALSE;
		if (!cdinfo_puts(pp, "1\n"))
			return FALSE;
	}
	if (cdinfo_dbp->disc.artistfname.dispname != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.artistfname.dispname"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.artistfname.dispname))
			return FALSE;
	}
	if (cdinfo_dbp->disc.artistfname.lastname != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.artistfname.lastname"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.artistfname.lastname))
			return FALSE;
	}
	if (cdinfo_dbp->disc.artistfname.firstname != NULL) {
		if (!cdinfo_puts(pp,
				 XMCD_PIPESIG "disc.artistfname.firstname"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.artistfname.firstname))
			return FALSE;
	}
	if (cdinfo_dbp->disc.artistfname.the != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.artistfname.the"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.artistfname.the))
			return FALSE;
	}
	if (cdinfo_dbp->disc.artist != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.artist"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.artist))
			return FALSE;
	}
	if (cdinfo_dbp->disc.title != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.title"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.title))
			return FALSE;
	}
	if (cdinfo_dbp->disc.sorttitle != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.sorttitle"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.sorttitle))
			return FALSE;
	}
	if (cdinfo_dbp->disc.title_the != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.title_the"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.title_the))
			return FALSE;
	}
	if (cdinfo_dbp->disc.year != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.year"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.year))
			return FALSE;
	}
	if (cdinfo_dbp->disc.label != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.label"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.label))
			return FALSE;
	}
	if (cdinfo_dbp->disc.genre != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.genre"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.genre))
			return FALSE;
	}
	if (cdinfo_dbp->disc.genre2 != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.genre2"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.genre2))
			return FALSE;
	}
	if (cdinfo_dbp->disc.dnum != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.dnum"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.dnum))
			return FALSE;
	}
	if (cdinfo_dbp->disc.tnum != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.tnum"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.tnum))
			return FALSE;
	}
	if (cdinfo_dbp->disc.region != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.region"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.region))
			return FALSE;
	}
	if (cdinfo_dbp->disc.lang != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.lang"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.lang))
			return FALSE;
	}
	if (cdinfo_dbp->disc.notes != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.notes"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.notes))
			return FALSE;
		if (!cdinfo_puts(pp, XMCD_PIPESIG "multi.end"))
			return FALSE;
	}
	if (cdinfo_dbp->disc.mediaid != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.mediaid"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.mediaid))
			return FALSE;
	}
	if (cdinfo_dbp->disc.muiid != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.muiid"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.muiid))
			return FALSE;
	}
	if (cdinfo_dbp->disc.titleuid != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.titleuid"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.titleuid))
			return FALSE;
	}
	if (cdinfo_dbp->disc.revision != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.revision"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.revision))
			return FALSE;
	}
	if (cdinfo_dbp->disc.revtag != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.revtag"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.revtag))
			return FALSE;
	}
	if (cdinfo_dbp->disc.certifier != NULL) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.certifier"))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->disc.certifier))
			return FALSE;
	}
	for (p = cdinfo_dbp->disc.credit_list; p != NULL; p = p->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.credit.new"))
			return FALSE;
		if (p->crinfo.role != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.credit.role"))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.role->id))
				return FALSE;
		}
		if (p->crinfo.name != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.credit.name"))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.name))
				return FALSE;
		}
		if (p->crinfo.fullname.dispname != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.credit.fullname.dispname"))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.dispname))
				return FALSE;
		}
		if (p->crinfo.fullname.lastname != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.credit.fullname.lastname"))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.lastname))
				return FALSE;
		}
		if (p->crinfo.fullname.firstname != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.credit.fullname.firstname"))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.firstname))
				return FALSE;
		}
		if (p->crinfo.fullname.the != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.credit.fullname.the"))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.the))
				return FALSE;
		}
		if (p->notes != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.credit.notes"))
				return FALSE;
			if (!cdinfo_puts(pp, p->notes))
				return FALSE;
			if (!cdinfo_puts(pp, XMCD_PIPESIG "multi.end"))
				return FALSE;
		}
	}
	for (q = cdinfo_dbp->disc.segment_list; q != NULL; q = q->next) {
		if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.segment.new"))
			return FALSE;
		if (q->name != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG "disc.segment.name"))
				return FALSE;
			if (!cdinfo_puts(pp, q->name))
				return FALSE;
		}
		if (q->notes != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.segment.notes"))
				return FALSE;
			if (!cdinfo_puts(pp, q->notes))
				return FALSE;
			if (!cdinfo_puts(pp, XMCD_PIPESIG "multi.end"))
				return FALSE;
		}
		if (q->start_track != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.segment.start_track"))
				return FALSE;
			if (!cdinfo_puts(pp, q->start_track))
				return FALSE;
		}
		if (q->start_frame != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.segment.start_frame"))
				return FALSE;
			if (!cdinfo_puts(pp, q->start_frame))
				return FALSE;
		}
		if (q->end_track != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.segment.end_track"))
				return FALSE;
			if (!cdinfo_puts(pp, q->end_track))
				return FALSE;
		}
		if (q->end_frame != NULL) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.segment.end_frame"))
				return FALSE;
			if (!cdinfo_puts(pp, q->end_frame))
				return FALSE;
		}
		for (p = q->credit_list; p != NULL; p = p->next) {
			if (!cdinfo_puts(pp, XMCD_PIPESIG
					 "disc.segment.credit.new"))
				return FALSE;
			if (p->crinfo.role != NULL) {
				if (!cdinfo_puts(pp, XMCD_PIPESIG
						 "disc.segment.credit.role"))
					return FALSE;
				if (!cdinfo_puts(pp, p->crinfo.role->id))
					return FALSE;
			}
			if (p->crinfo.name != NULL) {
				if (!cdinfo_puts(pp, XMCD_PIPESIG
						 "disc.segment.credit.name"))
					return FALSE;
				if (!cdinfo_puts(pp, p->crinfo.name))
					return FALSE;
			}
			if (p->crinfo.fullname.dispname != NULL) {
				if (!cdinfo_puts(pp, XMCD_PIPESIG
				    "disc.segment.credit.fullname.dispname"))
					return FALSE;
				if (!cdinfo_puts(pp,
						 p->crinfo.fullname.dispname))
					return FALSE;
			}
			if (p->crinfo.fullname.lastname != NULL) {
				if (!cdinfo_puts(pp, XMCD_PIPESIG
				     "disc.segment.credit.fullname.lastname"))
					return FALSE;
				if (!cdinfo_puts(pp,
						 p->crinfo.fullname.lastname))
					return FALSE;
			}
			if (p->crinfo.fullname.firstname != NULL) {
				if (!cdinfo_puts(pp, XMCD_PIPESIG
				     "disc.segment.credit.fullname.firstname"))
					return FALSE;
				if (!cdinfo_puts(pp,
						 p->crinfo.fullname.firstname))
					return FALSE;
			}
			if (p->crinfo.fullname.the != NULL) {
				if (!cdinfo_puts(pp, XMCD_PIPESIG
				     "disc.segment.credit.fullname.the"))
					return FALSE;
				if (!cdinfo_puts(pp, p->crinfo.fullname.the))
					return FALSE;
			}
			if (p->notes != NULL) {
				if (!cdinfo_puts(pp, XMCD_PIPESIG
				     "disc.segment.credit.notes"))
					return FALSE;
				if (!cdinfo_puts(pp, p->notes))
					return FALSE;
				if (!cdinfo_puts(pp, XMCD_PIPESIG "multi.end"))
					return FALSE;
			}
		}
	}

	return TRUE;
}


/*
 * cdinfo_puttrack_ents
 *	Write track related elements into pipe, used by the child.
 *
 * Args:
 *	pp - Pointer to cdinfo_pipe_t
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_puttrack_ents(cdinfo_pipe_t *pp, curstat_t *s)
{
	int		i;
	char		buf[80];
	cdinfo_credit_t	*p;

	for (i = 0; i < (int) s->tot_trks; i++) {
	    if (cdinfo_dbp->track[i].artistfname.dispname != NULL) {
		(void) sprintf(buf, "%strack%d.artistfname.dispname",
				XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp,
				 cdinfo_dbp->track[i].artistfname.dispname))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].artistfname.lastname != NULL) {
		(void) sprintf(buf, "%strack%d.artistfname.lastname",
				XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp,
				 cdinfo_dbp->track[i].artistfname.lastname))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].artistfname.firstname != NULL) {
		(void) sprintf(buf, "%strack%d.artistfname.firstname",
				XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp,
				 cdinfo_dbp->track[i].artistfname.firstname))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].artistfname.the != NULL) {
		(void) sprintf(buf, "%strack%d.artistfname.the",
				XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].artistfname.the))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].artist != NULL) {
		(void) sprintf(buf, "%strack%d.artist", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].artist))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].title != NULL) {
		(void) sprintf(buf, "%strack%d.title", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].title))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].sorttitle != NULL) {
		(void) sprintf(buf, "%strack%d.sorttitle", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].sorttitle))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].title_the != NULL) {
		(void) sprintf(buf, "%strack%d.title_the", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].title_the))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].year != NULL) {
		(void) sprintf(buf, "%strack%d.year", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].year))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].label != NULL) {
		(void) sprintf(buf, "%strack%d.label", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].label))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].genre != NULL) {
		(void) sprintf(buf, "%strack%d.genre", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].genre))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].genre2 != NULL) {
		(void) sprintf(buf, "%strack%d.genre2", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].genre2))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].bpm != NULL) {
		(void) sprintf(buf, "%strack%d.bpm", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].bpm))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].notes != NULL) {
		(void) sprintf(buf, "%strack%d.notes", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].notes))
			return FALSE;
		(void) sprintf(buf, "%smulti.end", XMCD_PIPESIG);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
	    }
	    if (cdinfo_dbp->track[i].isrc != NULL) {
		(void) sprintf(buf, "%strack%d.isrc", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (!cdinfo_puts(pp, cdinfo_dbp->track[i].isrc))
			return FALSE;
	    }

	    for (p = cdinfo_dbp->track[i].credit_list; p != NULL; p = p->next){
		(void) sprintf(buf, "%strack%d.credit.new", XMCD_PIPESIG, i);
		if (!cdinfo_puts(pp, buf))
			return FALSE;
		if (p->crinfo.role != NULL) {
			(void) sprintf(buf, "%strack%d.credit.role",
					XMCD_PIPESIG, i);
			if (!cdinfo_puts(pp, buf))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.role->id))
				return FALSE;
		}
		if (p->crinfo.name != NULL) {
			(void) sprintf(buf, "%strack%d.credit.name",
					XMCD_PIPESIG, i);
			if (!cdinfo_puts(pp, buf))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.name))
				return FALSE;
		}
		if (p->crinfo.fullname.dispname != NULL) {
			(void) sprintf(buf,
					"%strack%d.credit.fullname.dispname",
					XMCD_PIPESIG, i);
			if (!cdinfo_puts(pp, buf))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.dispname))
				return FALSE;
		}
		if (p->crinfo.fullname.lastname != NULL) {
			(void) sprintf(buf,
					"%strack%d.credit.fullname.lastname",
					XMCD_PIPESIG, i);
			if (!cdinfo_puts(pp, buf))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.lastname))
				return FALSE;
		}
		if (p->crinfo.fullname.firstname != NULL) {
			(void) sprintf(buf,
					"%strack%d.credit.fullname.firstname",
					XMCD_PIPESIG, i);
			if (!cdinfo_puts(pp, buf))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.firstname))
				return FALSE;
		}
		if (p->crinfo.fullname.the != NULL) {
			(void) sprintf(buf,
					"%strack%d.credit.fullname.the",
					XMCD_PIPESIG, i);
			if (!cdinfo_puts(pp, buf))
				return FALSE;
			if (!cdinfo_puts(pp, p->crinfo.fullname.the))
				return FALSE;
		}
		if (p->notes != NULL) {
			(void) sprintf(buf, "%strack%d.credit.notes",
					XMCD_PIPESIG, i);
			if (!cdinfo_puts(pp, buf))
				return FALSE;
			if (!cdinfo_puts(pp, p->notes))
				return FALSE;
			if (!cdinfo_puts(pp, XMCD_PIPESIG "multi.end"))
				return FALSE;
		}
	    }
	}

	return TRUE;
}


/*
 * cdinfo_flushpipe
 *	Flush the pipe write cache.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
cdinfo_flushpipe(cdinfo_pipe_t *pp)
{
	if (pp == NULL)
		return FALSE;

	if (pp->w.cache != NULL && pp->w.pos > 0) {
		unsigned char	*p;
		int		resid,
				n = 0;

		/* Flush write cache */
		p = pp->w.cache;
		for (resid = pp->w.pos; resid > 0; resid -= n) {
			if ((n = write(pp->w.fd, p, resid)) < 0) {
				pp->w.pos = 0;
				return FALSE;
			}
			p += n;
		}
		pp->w.pos = 0;
	}

	return TRUE;
}


/*
 * cdinfo_openpipe
 *	Create a one-way pipe and set up for buffered I/O
 *
 * Args:
 *	dir - pipe data direction: CDINFO_DATAIN or CDINFO_DATAOUT
 *	      (from the perspective of parent process)
 *
 * Return:
 *	A pointer to the cdinfo_pipe_t structure which contains pipe
 *	file descriptors and FILE stream pointers.
 */
cdinfo_pipe_t *
cdinfo_openpipe(int dir)
{
	int		pfd[2];
	cdinfo_pipe_t	*pp;

	/* Allocate cdinfo_pipe_t */
	pp = (cdinfo_pipe_t *)(void *) MEM_ALLOC(
		"cdinfo_pipe_t",
		sizeof(cdinfo_pipe_t)
	);
	if (pp == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return NULL;
	}

	/* Create pipe */
	if (PIPE(pfd) < 0) {
		MEM_FREE(pp);
		return NULL;
	}

	pp->dir = dir;
	pp->r.fd = pfd[0];
	pp->w.fd = pfd[1];
	pp->r.rw = O_RDONLY;
	pp->w.rw = O_WRONLY;
	pp->r.pos = pp->w.pos = 0;
	pp->r.cnt = pp->w.cnt = 0;

	/* The caches are allocated on first I/O */
	pp->r.cache = pp->w.cache = NULL;

	return (pp);
}


/*
 * cdinfo_closepipe
 *	Close the pipe created with cdinfo_openpipe.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
cdinfo_closepipe(cdinfo_pipe_t *pp)
{
	int	fd;

	if (pp == NULL)
		return FALSE;

	if (pp->w.cache != NULL && pp->w.pos > 0) {
		unsigned char	*p;
		int		resid,
				n = 0;

		/* Flush write cache */
		p = pp->w.cache;
		for (resid = pp->w.pos; resid > 0; resid -= n) {
			if ((n = write(pp->w.fd, p, resid)) < 0) {
				pp->w.pos = 0;
				return FALSE;
			}
			p += n;
		}
		pp->w.pos = 0;
	}

	/* Close file descriptor */
	if (pp->dir == CDINFO_DATAIN) {
		if (cdinfo_ischild)
			fd = pp->w.fd;
		else
			fd = pp->r.fd;
	}
	else {
		if (cdinfo_ischild)
			fd = pp->r.fd;
		else
			fd = pp->w.fd;
	}
	if (fd != -1)
		(void) close(fd);

	/* Free cache storage */
	if (pp->r.cache != NULL)
		MEM_FREE(pp->r.cache);
	if (pp->w.cache != NULL)
		MEM_FREE(pp->w.cache);

	/* Free cdinfo_pipe_t structure */
	MEM_FREE(pp);

	return TRUE;
}


/*
 * cdinfo_write_datapipe
 *	Write incore CD information into pipe.  Used by child process.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure.
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_write_datapipe(cdinfo_pipe_t *pp, curstat_t *s)
{
	if (!cdinfo_putctrl_ents(pp))
		return FALSE;

	if (!cdinfo_putglist_ents(pp))
		return FALSE;

	if (!cdinfo_putrelist_ents(pp))
		return FALSE;

	if (!cdinfo_putlanglist_ents(pp))
		return FALSE;

	if (!cdinfo_putrolist_ents(pp))
		return FALSE;

	if (!cdinfo_putuserreg_ents(pp))
		return FALSE;

	if (cdinfo_dbp->matchlist != NULL) {
		/* Fuzzy matches */
		if (!cdinfo_putmatch_ents(pp))
			return FALSE;
	}
	else {
		/* Exact match */
		if (!cdinfo_putdisc_ents(pp))
			return FALSE;

		if (!cdinfo_puttrack_ents(pp, s))
			return FALSE;
	}

	/* URLs */
	if (!cdinfo_puturl_ents(pp))
		return FALSE;

	if (!cdinfo_puts(pp, XMCD_PIPESIG "end"))
		return FALSE;

	/* Flush write cache in pipe */
	if (!cdinfo_flushpipe(pp))
		return FALSE;

	return TRUE;
}


/*
 * cdinfo_read_datapipe
 *	Read CDDB data from pipe and update in-core structures.
 *	Used by parent process.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_read_datapipe(cdinfo_pipe_t *pp)
{
	int		n;
	char		*key;
	static char	buf[2048];

	n = strlen(XMCD_PIPESIG);
	key = &buf[n];
	while (cdinfo_gets(pp, buf, sizeof(buf))) {
		if (strncmp(buf, XMCD_PIPESIG, n) == 0) {
			buf[strlen(buf) - 1] = '\0';	/* Zap newline */

			if (strncmp(key, "disc", 4) == 0 &&
			    !cdinfo_getdisc_ents(pp, key + 4)) {
				return FALSE;
			}
			else if (strncmp(key, "track", 5) == 0 &&
				 !cdinfo_gettrack_ents(pp, key + 5)) {
				return FALSE;
			}
			else if (strncmp(key, "url", 3) == 0 &&
				 !cdinfo_geturl_ents(pp, key + 3)) {
				return FALSE;
			}
			else if (strncmp(key, "genre", 5) == 0 &&
				 !cdinfo_getglist_ents(pp, key + 5)) {
				return FALSE;
			}
			else if (strncmp(key, "region", 6) == 0 &&
				 !cdinfo_getrelist_ents(pp, key + 6)) {
				return FALSE;
			}
			else if (strncmp(key, "lang", 4) == 0 &&
				 !cdinfo_getlanglist_ents(pp, key + 4)) {
				return FALSE;
			}
			else if (strncmp(key, "role", 4) == 0 &&
				 !cdinfo_getrolist_ents(pp, key + 4)) {
				return FALSE;
			}
			else if (strncmp(key, "match", 5) == 0 &&
				 !cdinfo_getmatch_ents(pp, key + 5)) {
				return FALSE;
			}
			else if (strncmp(key, "userreg", 7) == 0 &&
				 !cdinfo_getuserreg_ents(pp, key + 7)) {
				return FALSE;
			}
			else if (strncmp(key, "control", 7) == 0 &&
			    !cdinfo_getctrl_ents(pp, key + 7)) {
				return FALSE;
			}
			else if (strcmp(key, "end") == 0) {
				break;
			}
		}
		else {
			/* Unexpected data */
			return FALSE;
		}
	}

	return TRUE;
}


/*
 * cdinfo_write_selpipe
 *	Write multiple match selection to pipe.  Used by parent process.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_write_selpipe(cdinfo_pipe_t *pp)
{
	char	buf[16];

	if (!cdinfo_puts(pp, XMCD_PIPESIG "match.tag"))
		return FALSE;

	(void) sprintf(buf, "%ld", cdinfo_dbp->match_tag);
	if (!cdinfo_puts(pp, buf))
		return FALSE;

	if (!cdinfo_puts(pp, XMCD_PIPESIG "end"))
		return FALSE;

	/* Flush write cache in pipe */
	if (!cdinfo_flushpipe(pp))
		return FALSE;

	return TRUE;
}


/*
 * cdinfo_read_selpipe
 *	Read multiple match selection from pipe.  Used by child process.
 *
 * Args:
 *	pp - Pointer to the cdinfo_pipe_t structure.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_read_selpipe(cdinfo_pipe_t *pp)
{
	int		n;
	char		*key,
			*str;
	static char	buf[80];

	n = strlen(XMCD_PIPESIG);
	key = &buf[n];
	while (cdinfo_gets(pp, buf, sizeof(buf))) {
		if (strncmp(buf, XMCD_PIPESIG, n) == 0) {
			buf[strlen(buf) - 1] = '\0';	/* Zap newline */

			if (strncmp(key, "match.tag", 9) == 0) {
				str = NULL;
				if (!cdinfo_getline(pp, "match.tag", &str))
					return FALSE;

				if (str != NULL) {
					cdinfo_dbp->match_tag = atol(str);
					MEM_FREE(str);
				}
			}
			else if (strcmp(key, "end") == 0) {
				break;
			}
		}
		else {
			/* Unexpected data */
			return FALSE;
		}
	}

	return TRUE;
}

#endif	/* SYNCHRONOUS */


/* 
 * cdinfo_tocstr
 *      Return a text string containing the CD's TOC frame offset
 *	information suitable for use with the CDDB service.
 *
 * Args:
 *      s - Pointer to the curstat_t structure.
 *
 * Return:
 *      The TOC text string.
 */
char *
cdinfo_tocstr(curstat_t *s)
{
	int		i;
	static char	toc[512];

	toc[0] = '\0';

	for (i = 0; i <= (int) s->tot_trks; i++) {
		(void) sprintf(toc, "%s%s%d", toc, (i == 0) ? "" : " ",
				(((s->trkinfo[i].min * 60) +
				   s->trkinfo[i].sec) * FRAME_PER_SEC) +
				   s->trkinfo[i].frame);
	}

	return (toc);
}


/*
 * cdinfo_get_str
 *	CDDB object string property retrieval handler.
 *
 * Args:
 *	funcname - The name of the CDDB service function
 *	func - CDDB service function to call
 *	objp - CDDB object
 *	datap - Return string property
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_get_str(
	char		*funcname,
	CddbResult	(*func)(),
	void		*objp,
	char		**datap
)
{
	CddbResult	ret;
	char		*str;

	DBGPRN(DBG_CDI)(errfp, "%s: ", funcname);
	ret = (*func)(objp, &str);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %s\n", ret, str == NULL ? "-" : str);

	cdinfo_set_errstr(ret);

	return (util_newstr(datap, str));
}

	
/*
 * cdinfo_get_bool
 *	CDDB object boolean property retrieval handler.
 *
 * Args:
 *	funcname - The name of the CDDB service function
 *	func - CDDB service function to call
 *	objp - CDDB object
 *	datap - Return boolean property
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_get_bool(
	char		*funcname,
	CddbResult	(*func)(),
	void		*objp,
	bool_t		*datap
)
{
	CddbResult	ret;
	CddbBoolean	b;

	DBGPRN(DBG_CDI)(errfp, "%s: ", funcname);
	ret = (*func)(objp, &b);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %s\n", ret, b ? "Yes" : "No");

	cdinfo_set_errstr(ret);

	*datap = (bool_t) b;
	return TRUE;
}


/*
 * cdinfo_get_fullname
 *	CDDB object fullname property retrieval handler.
 *
 * Args:
 *	funcname - The name of the CDDB service function
 *	func - CDDB service function to call
 *	objp - CDDB object
 *	dispname - Return name string
 *	lastname - Return last name string
 *	firstname - Return first name string
 *	the - Return "the"
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_get_fullname(
	char		*funcname,
	CddbResult	(*func)(),
	void		*objp,
	char		**dispname,
	char		**lastname,
	char		**firstname,
	char		**the
)
{
	CddbResult	ret;
	CddbFullNamePtr	f;

	DBGPRN(DBG_CDI)(errfp, "%s: ", funcname);
	ret = (*func)(objp, &f);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || f == NULL) {
		*dispname = *lastname = *firstname = *the = NULL;
	}
	else {
		if (!cdinfo_get_str("    CddbFullName_GetName",
				      CddbFullName_GetName, f,
				      dispname)) {
			CddbReleaseObject(f);
			return FALSE;
		}

		if (!cdinfo_get_str("    CddbFullName_GetLastName",
				      CddbFullName_GetLastName, f,
				      lastname)) {
			CddbReleaseObject(f);
			return FALSE;
		}
		if (!cdinfo_get_str("    CddbFullName_GetFirstName",
				      CddbFullName_GetFirstName, f,
				      firstname)) {
			CddbReleaseObject(f);
			return FALSE;
		}
		if (!cdinfo_get_str("    CddbFullName_GetThe",
				      CddbFullName_GetThe, f, the)) {
			CddbReleaseObject(f);
			return FALSE;
		}
		CddbReleaseObject(f);
	}
	return TRUE;
}


/*
 * cdinfo_put_str
 *	CDDB object string property put handler.
 *
 * Args:
 *	funcname - The name of the CDDB service function
 *	func - CDDB service function to call
 *	objp - CDDB object
 *	datap - String property to put
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_put_str(
	char		*funcname,
	CddbResult	(*func)(),
	void		*objp,
	char		*datap
)
{
	CddbResult	ret;

	DBGPRN(DBG_CDI)(errfp, "%s: ", funcname);
	ret = (*func)(objp, (CddbStr) datap);
	DBGPRN(DBG_CDI)(errfp, "0x%lx", ret);

	cdinfo_set_errstr(ret);

	if (util_strstr(funcname, "Password") == NULL) {
		DBGPRN(DBG_CDI)(errfp, " %s\n", datap == NULL ? "-" : datap);
	}
	else {
		/* Don't show any password related strings in debug output */
		DBGPRN(DBG_CDI)(errfp, "\n");
	}

	return (ret == Cddb_OK);
}


/*
 * cdinfo_put_fullname
 *	CDDB object fullname property put handler.
 *
 * Args:
 *	funcname - The name of the CDDB service function
 *	func - CDDB service function to call
 *	objp - CDDB object
 *	dispname - Return name string
 *	lastname - Return last name string
 *	firstname - Return first name string
 *	the - Return "the"
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_put_fullname(
	char		*funcname,
	CddbResult	(*func)(),
	void		*objp,
	char		*dispname,
	char		*lastname,
	char		*firstname,
	char		*the
)
{
	CddbResult	ret;
	CddbFullNamePtr	f;

	if ((f = (CddbFullNamePtr) CddbCreateObject(CddbFullNameType)) == NULL)
		return FALSE;

	if (!cdinfo_put_str("    CddbFullName_PutName",
			      CddbFullName_PutName, f, dispname)) {
		CddbReleaseObject(f);
		return FALSE;
	}

	if (!cdinfo_put_str("    CddbFullName_PutLastName",
			      CddbFullName_PutLastName, f, lastname)) {
		CddbReleaseObject(f);
		return FALSE;
	}
	if (!cdinfo_put_str("    CddbFullName_PutFirstName",
			      CddbFullName_PutFirstName, f, firstname)) {
		CddbReleaseObject(f);
		return FALSE;
	}
	if (!cdinfo_put_str("    CddbFullName_PutThe",
			      CddbFullName_PutThe, f, the)) {
		CddbReleaseObject(f);
		return FALSE;
	}

	DBGPRN(DBG_CDI)(errfp, "%s: ", funcname);
	ret = (*func)(objp, f);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	CddbReleaseObject(f);
	return (ret == Cddb_OK);
}


/*
 * cdinfo_check_userreg
 *	Check CDDB user registration and update structures
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_check_userreg(cdinfo_cddb_t *cp)
{
	CddbUserInfoPtr	uip = NULL;
	CddbResult	ret;
	CddbBoolean	registered = 0;
	bool_t		yn;

	/* Check if user is registered */
	DBGPRN(DBG_CDI)(errfp, "CddbControl_IsRegistered: ");
	ret = CddbControl_IsRegistered(cp->ctrlp, 0, &registered);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %s\n", ret, registered ? "Yes" : "No");

	cdinfo_set_errstr(ret);

	if (!registered) {
		DBGPRN(DBG_CDI)(errfp, "User is not registered with CDDB.\n");
		return FALSE;
	}

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetUserInfo: ");
	ret = CddbControl_GetUserInfo(cp->ctrlp, &uip);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || uip == NULL)
		return FALSE;

	if (!cdinfo_get_str("CddbUserInfo_GetUserHandle",
			      CddbUserInfo_GetUserHandle, uip,
			      &cdinfo_dbp->userreg.handle)) {
		CddbReleaseObject(uip);
		return FALSE;
	}

	DBGPRN(DBG_CDI)(errfp, "User \"%s\" is registered.\n",
			cdinfo_dbp->userreg.handle == NULL ?
			"(null)" : cdinfo_dbp->userreg.handle);

	if (!cdinfo_get_str("CddbUserInfo_GetPasswordHint",
			      CddbUserInfo_GetPasswordHint, uip,
			      &cdinfo_dbp->userreg.hint)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	if (!cdinfo_get_str("CddbUserInfo_GetEmailAddress",
			      CddbUserInfo_GetEmailAddress, uip,
			      &cdinfo_dbp->userreg.email)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	if (!cdinfo_get_str("CddbUserInfo_GetRegionId",
			      CddbUserInfo_GetRegionId, uip,
			      &cdinfo_dbp->userreg.region)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	if (!cdinfo_get_str("CddbUserInfo_GetPostalCode",
			      CddbUserInfo_GetPostalCode, uip,
			      &cdinfo_dbp->userreg.postal)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	if (!cdinfo_get_str("CddbUserInfo_GetAge",
			      CddbUserInfo_GetAge, uip,
			      &cdinfo_dbp->userreg.age)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	if (!cdinfo_get_str("CddbUserInfo_GetSex",
			      CddbUserInfo_GetSex, uip,
			      &cdinfo_dbp->userreg.gender)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	if (!cdinfo_get_bool("CddbUserInfo_GetAllowEmail",
			       CddbUserInfo_GetAllowEmail, uip, &yn)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	cdinfo_dbp->userreg.allowemail = (bool_t) yn;
	if (!cdinfo_get_bool("CddbUserInfo_GetAllowStats",
			       CddbUserInfo_GetAllowStats, uip, &yn)) {
		CddbReleaseObject(uip);
		return FALSE;
	}
	cdinfo_dbp->userreg.allowstats = (bool_t) yn;

	CddbReleaseObject(uip);
	return TRUE;
}


/*
 * cdinfo_allocinit_genre
 *	CDDB genre information initialization routine
 *
 * Args:
 *	genrep - CddbGenre object
 *	ret - cddb_genre_t return structure
 *	sub - Whether a sub-genre is being processed
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_allocinit_genre(CddbGenrePtr genrep, cdinfo_genre_t **ret, bool_t sub)
{
	cdinfo_genre_t	*p;

	*ret = NULL;

	p = (cdinfo_genre_t *)(void *) MEM_ALLOC("genre",
		sizeof(cdinfo_genre_t)
	);
	if (p == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) memset(p, 0, sizeof(cdinfo_genre_t));

	/* Genre ID */
	if (!cdinfo_get_str(sub ? "    CddbGenre_GetId" :
				    "  CddbGenre_GetId",
			      CddbGenre_GetId,
			      genrep, &p->id)) {
		return FALSE;
	}

	/* Genre name */
	if (!cdinfo_get_str(sub ? "    CddbGenre_GetName" :
				    "  CddbGenre_GetName",
			      CddbGenre_GetName,
			      genrep, &p->name)) {
		return FALSE;
	}

	*ret = p;
	return TRUE;
}


/*
 * cdinfo_allocinit_region
 *	CDDB region information initialization routine
 *
 * Args:
 *	regionp - CddbRegion object
 *	ret - cddb_region_t return structure
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_allocinit_region(CddbRegionPtr regionp, cdinfo_region_t **ret)
{
	cdinfo_region_t	*p;

	*ret = NULL;

	p = (cdinfo_region_t *)(void *) MEM_ALLOC("region",
		sizeof(cdinfo_region_t)
	);
	if (p == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) memset(p, 0, sizeof(cdinfo_region_t));

	/* Region ID */
	if (!cdinfo_get_str("  CddbRegion_GetId",
			      CddbRegion_GetId,
			      regionp, &p->id)) {
		return FALSE;
	}

	/* Region name */
	if (!cdinfo_get_str("  CddbRegion_GetName",
			      CddbRegion_GetName,
			      regionp, &p->name)) {
		return FALSE;
	}

	*ret = p;
	return TRUE;
}


/*
 * cdinfo_allocinit_lang
 *	CDDB language information initialization routine
 *
 * Args:
 *	langp - CddbLanguage object
 *	ret - cddb_lang_t return structure
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_allocinit_lang(CddbLanguagePtr langp, cdinfo_lang_t **ret)
{
	cdinfo_lang_t	*p;

	*ret = NULL;

	p = (cdinfo_lang_t *)(void *) MEM_ALLOC("lang",
		sizeof(cdinfo_lang_t)
	);
	if (p == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) memset(p, 0, sizeof(cdinfo_lang_t));

	/* Language ID */
	if (!cdinfo_get_str("  CddbLanguage_GetId",
			      CddbLanguage_GetId,
			      langp, &p->id)) {
		return FALSE;
	}

	/* Language name */
	if (!cdinfo_get_str("  CddbLanguage_GetName",
			      CddbLanguage_GetName,
			      langp, &p->name)) {
		return FALSE;
	}

	*ret = p;
	return TRUE;
}


/*
 * cdinfo_allocinit_role
 *	CDDB role information initialization routine
 *
 * Args:
 *	rolep - CddbRole object
 *	ret - cddb_role_t return structure
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_allocinit_role(CddbRolePtr rolep, cdinfo_role_t **ret)
{
	cdinfo_role_t	*p;

	*ret = NULL;

	p = (cdinfo_role_t *)(void *) MEM_ALLOC("role",
		sizeof(cdinfo_role_t)
	);
	if (p == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) memset(p, 0, sizeof(cdinfo_role_t));

	/* Role ID */
	if (!cdinfo_get_str("    CddbRole_GetId",
			      CddbRole_GetId, rolep, &p->id)) {
		return FALSE;
	}

	/* Role name */
	if (!cdinfo_get_str("    CddbRole_GetName",
			      CddbRole_GetName, rolep, &p->name)) {
		return FALSE;
	}

	*ret = p;
	return TRUE;
}


/*
 * cdinfo_discinfo_query
 *	Query CDDB for album information
 *
 * Args:
 *	discp - CddbDisc object
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_discinfo_query(CddbDiscPtr discp)
{
	CddbResult		ret;
	CddbCreditPtr		credp = NULL;
	CddbSegmentPtr		segp = NULL;
	char			*str;
	long			ncreds,
				nsegs;
	int			i,
				j;
	cdinfo_genre_t		*gp,
				*sgp;
	cdinfo_credit_t		*p,
				*q;
	cdinfo_segment_t	*r,
				*s;
	cdinfo_role_t		*rp;

	/* Disc artist */
	if (!cdinfo_get_str("CddbDisc_GetArtist",
				CddbDisc_GetArtist,
				discp, &cdinfo_dbp->disc.artist)) {
		return FALSE;
	}

	/* Disc title */
	if (!cdinfo_get_str("CddbDisc_GetTitle",
				CddbDisc_GetTitle,
				discp, &cdinfo_dbp->disc.title)) {
		return FALSE;
	}

	/* Artist full name */
	if (!cdinfo_get_fullname("CddbDisc_GetArtistFullName",
				CddbDisc_GetArtistFullName,
				discp,
				&cdinfo_dbp->disc.artistfname.dispname,
				&cdinfo_dbp->disc.artistfname.lastname,
				&cdinfo_dbp->disc.artistfname.firstname,
				&cdinfo_dbp->disc.artistfname.the)) {
		return FALSE;
	}

	/* Disc sort title */
	if (!cdinfo_get_str("CddbDisc_GetTitleSort",
				CddbDisc_GetTitleSort,
				discp, &cdinfo_dbp->disc.sorttitle)) {
		return FALSE;
	}

	/* Disc sort title */
	if (!cdinfo_get_str("CddbDisc_GetTitleThe",
				CddbDisc_GetTitleThe,
				discp, &cdinfo_dbp->disc.title_the)) {
		return FALSE;
	}

	/* Year */
	if (!cdinfo_get_str("CddbDisc_GetYear",
				CddbDisc_GetYear,
				discp, &cdinfo_dbp->disc.year)) {
		return FALSE;
	}

	/* Label */
	if (!cdinfo_get_str("CddbDisc_GetLabel",
				CddbDisc_GetLabel,
				discp, &cdinfo_dbp->disc.label)) {
		return FALSE;
	}

	/* Compilation */
	if (!cdinfo_get_bool("CddbDisc_GetCompilation",
				CddbDisc_GetCompilation,
				discp, &cdinfo_dbp->disc.compilation)) {
		return FALSE;
	}

	/* Genre */
	if (!cdinfo_get_str("CddbDisc_GetGenreId",
				CddbDisc_GetGenreId,
				discp, &cdinfo_dbp->disc.genre)) {
		return FALSE;
	}
	gp = cdinfo_genre(cdinfo_dbp->disc.genre);
	if (gp != NULL && gp->parent == NULL) {
		/* Illegal: the primary genre is set to a genre category.
		 * "Fix" it by setting it to the category's "General"
		 * subgenre.  If not found, then set it to the first
		 * subgenre.
		 */
		for (sgp = gp->child; sgp != NULL; sgp = sgp->next) {
			if (sgp->name != NULL &&
			    strncmp(sgp->name, "General ", 8) == 0) {
				if (!util_newstr(&cdinfo_dbp->disc.genre,
						 sgp->id)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return FALSE;
				}
				break;
			}
		}
		if (sgp == NULL) {
			if (gp->child != NULL) {
				if (!util_newstr(&cdinfo_dbp->disc.genre,
						 gp->child->id)) {
				    CDINFO_FATAL(app_data.str_nomemory);
				    return FALSE;
				}
			}
			else {
				MEM_FREE(cdinfo_dbp->disc.genre);
				cdinfo_dbp->disc.genre = NULL;
			}
		}
	}

	/* Secondary Genre */
	if (!cdinfo_get_str("CddbDisc_GetSecondaryGenreId",
				CddbDisc_GetSecondaryGenreId,
				discp, &cdinfo_dbp->disc.genre2)) {
		return FALSE;
	}
	gp = cdinfo_genre(cdinfo_dbp->disc.genre2);
	if (gp != NULL && gp->parent == NULL) {
		/* Illegal: the secondary genre is set to a genre category.
		 * "Fix" it by setting it to the category's "General"
		 * subgenre.  If not found, then set it to the first
		 * subgenre.
		 */
		for (sgp = gp->child; sgp != NULL; sgp = sgp->next) {
			if (sgp->name != NULL &&
			    strncmp(sgp->name, "General ", 8) == 0) {
				if (!util_newstr(&cdinfo_dbp->disc.genre2,
						 sgp->id)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return FALSE;
				}
				break;
			}
		}
		if (sgp == NULL) {
			if (gp->child != NULL) {
				if (!util_newstr(&cdinfo_dbp->disc.genre2,
						 gp->child->id)) {
				    CDINFO_FATAL(app_data.str_nomemory);
				    return FALSE;
				}
			}
			else {
				MEM_FREE(cdinfo_dbp->disc.genre2);
				cdinfo_dbp->disc.genre2 = NULL;
			}
		}
	}

	/* Disc number in set */
	if (!cdinfo_get_str("CddbDisc_GetNumberInSet",
				CddbDisc_GetNumberInSet,
				discp, &cdinfo_dbp->disc.dnum)) {
		return FALSE;
	}

	/* Total number in set */
	if (!cdinfo_get_str("CddbDisc_GetTotalInSet",
				CddbDisc_GetTotalInSet,
				discp, &cdinfo_dbp->disc.tnum)) {
		return FALSE;
	}

	/* Region */
	if (!cdinfo_get_str("CddbDisc_GetRegionId",
				CddbDisc_GetRegionId,
				discp, &cdinfo_dbp->disc.region)) {
		return FALSE;
	}

	/* Language ID */
	if (!cdinfo_get_str("CddbDisc_GetLanguageId",
				CddbDisc_GetLanguageId,
				discp, &cdinfo_dbp->disc.lang)) {
		return FALSE;
	}

	/* Notes */
	if (!cdinfo_get_str("CddbDisc_GetNotes",
				CddbDisc_GetNotes,
				discp, &cdinfo_dbp->disc.notes)) {
		return FALSE;
	}

	/* Media ID */
	if (!cdinfo_get_str("CddbDisc_GetMediaId",
				CddbDisc_GetMediaId,
				discp, &cdinfo_dbp->disc.mediaid)) {
		return FALSE;
	}

	/* Media unique ID */
	if (!cdinfo_get_str("CddbDisc_GetMuiId",
				CddbDisc_GetMuiId,
				discp, &cdinfo_dbp->disc.muiid)) {
		return FALSE;
	}

	/* Title unique ID */
	if (!cdinfo_get_str("CddbDisc_GetTitleUId",
				CddbDisc_GetTitleUId,
				discp, &cdinfo_dbp->disc.titleuid)) {
		return FALSE;
	}

	/* Revision */
	if (!cdinfo_get_str("CddbDisc_GetRevision",
				CddbDisc_GetRevision,
				discp, &cdinfo_dbp->disc.revision)) {
		return FALSE;
	}

	/* Revision Tag */
	if (!cdinfo_get_str("CddbDisc_GetRevisionTag",
				CddbDisc_GetRevisionTag,
				discp, &cdinfo_dbp->disc.revtag)) {
		return FALSE;
	}

	/* Certifier */
	if (!cdinfo_get_str("CddbDisc_GetCertifier",
				CddbDisc_GetCertifier,
				discp, &cdinfo_dbp->disc.certifier)) {
		return FALSE;
	}

	/* Disc Credits */
	DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetNumCredits: ");
	ret = CddbDisc_GetNumCredits(discp, &ncreds);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld credits\n", ret, ncreds);

	cdinfo_set_errstr(ret);

	q = NULL;
	for (i = 0; i < (int) ncreds; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetCredit[%02d]: ", i+1);
		ret = CddbDisc_GetCredit(discp, (long) (i+1), &credp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);
		
		if (ret != Cddb_OK)
			continue;

		/* Allocate a credit structure */
		p = (cdinfo_credit_t *) MEM_ALLOC(
			"cdinfo_credit_t",
			sizeof(cdinfo_credit_t)
		);
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_credit_t));

		/* Credit role */
		str = NULL;
		if (!cdinfo_get_str("  CddbCredit_GetId",
				      CddbCredit_GetId, credp, &str)) {
			MEM_FREE(p);
			return FALSE;
		}
		rp = cdinfo_role(str);
		if (rp->parent == NULL) {
			/* Illegal: role set to the category role
			 * "Fix" it by setting the role to the
			 * first subrole of that category instead.
			 * If for some reason there are no subroles
			 * then the child would be NULL and thus the
			 * role would be "None -> None".
			 */
			rp = rp->child;
		}
		p->crinfo.role = rp;
		MEM_FREE(str);
		str = NULL;

		/* Credit name */
		if (!cdinfo_get_str("  CddbCredit_GetName",
				      CddbCredit_GetName,
				      credp, &p->crinfo.name)) {
			MEM_FREE(p);
			return FALSE;
		}

		/* Credit full name */
		if (!cdinfo_get_fullname("  CddbCredit_GetFullName",
					CddbCredit_GetFullName,
					credp,
					&p->crinfo.fullname.dispname,
					&p->crinfo.fullname.lastname,
					&p->crinfo.fullname.firstname,
					&p->crinfo.fullname.the)) {
			MEM_FREE(p);
			return FALSE;
		}

		/* Credit notes */
		if (!cdinfo_get_str("  CddbCredit_GetNotes",
				      CddbCredit_GetNotes,
				      credp, &p->notes)) {
			MEM_FREE(p);
			return FALSE;
		}

		CddbReleaseObject(credp);

		if (cdinfo_dbp->disc.credit_list == NULL) {
			cdinfo_dbp->disc.credit_list = p;
			p->prev = NULL;
		}
		else {
			q->next = p;
			p->prev = q;
		}
		q = p;
		p->next = NULL;
	}

	/* Segments */
	DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetNumSegments: ");
	ret = CddbDisc_GetNumSegments(discp, &nsegs);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld segments\n", ret, nsegs);

	cdinfo_set_errstr(ret);

	s = NULL;
	for (i = 0; i < (int) nsegs; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetSegment[%02d]: ", i+1);
		ret = CddbDisc_GetSegment(discp, (long) (i+1), &segp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);
		
		if (ret != Cddb_OK)
			continue;

		/* Allocate a segment structure */
		r = (cdinfo_segment_t *) MEM_ALLOC(
			"cdinfo_segment_t",
			sizeof(cdinfo_segment_t)
		);
		if (r == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(r, 0, sizeof(cdinfo_segment_t));

		/* Segment name */
		if (!cdinfo_get_str("  CddbSegment_GetName",
				      CddbSegment_GetName,
				      segp, &r->name)) {
			MEM_FREE(r);
			return FALSE;
		}

		/* Segment notes */
		if (!cdinfo_get_str("  CddbSegment_GetNotes",
				      CddbSegment_GetNotes,
				      segp, &r->notes)) {
			MEM_FREE(r);
			return FALSE;
		}

		/* Segment start track */
		if (!cdinfo_get_str("  CddbSegment_GetStartTrack",
				      CddbSegment_GetStartTrack,
				      segp, &r->start_track)) {
			MEM_FREE(r);
			return FALSE;
		}

		/* Segment start frame */
		if (!cdinfo_get_str("  CddbSegment_GetStartFrame",
				      CddbSegment_GetStartFrame,
				      segp, &r->start_frame)) {
			MEM_FREE(r);
			return FALSE;
		}

		/* Segment end track */
		if (!cdinfo_get_str("  CddbSegment_GetEndTrack",
				      CddbSegment_GetEndTrack,
				      segp, &r->end_track)) {
			MEM_FREE(r);
			return FALSE;
		}

		/* Segment end frame */
		if (!cdinfo_get_str("  CddbSegment_GetEndFrame",
				      CddbSegment_GetEndFrame,
				      segp, &r->end_frame)) {
			MEM_FREE(r);
			return FALSE;
		}

		/* Segment Credits */
		DBGPRN(DBG_CDI)(errfp, "  CddbSegment_GetNumCredits: ");
		ret = CddbSegment_GetNumCredits(segp, &ncreds);
		DBGPRN(DBG_CDI)(errfp, "0x%lx %ld credits\n", ret, ncreds);

		cdinfo_set_errstr(ret);

		q = NULL;
		for (j = 0; j < (int) ncreds; j++) {
			DBGPRN(DBG_CDI)(errfp,
				"CddbSegment_GetCredit[%02d]: ", j+1);
			ret = CddbSegment_GetCredit(segp, (long) (j+1),
						    &credp);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);
		    
			if (ret != Cddb_OK)
				continue;

			/* Allocate a credit structure */
			p = (cdinfo_credit_t *) MEM_ALLOC(
				"cdinfo_credit_t",
				sizeof(cdinfo_credit_t)
			);
			if (p == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return FALSE;
			}
			(void) memset(p, 0, sizeof(cdinfo_credit_t));

			/* Credit role */
			str = NULL;
			if (!cdinfo_get_str("    CddbCredit_GetId",
					      CddbCredit_GetId, credp,
					      &str)) {
				MEM_FREE(p);
				return FALSE;
			}
			rp = cdinfo_role(str);
			if (rp->parent == NULL) {
				/* Illegal: role set to the category role
				 * "Fix" it by setting the role to the
				 * first subrole of that category instead.
				 * If for some reason there are no subroles
				 * then the child would be NULL and thus the
				 * role would be "None -> None".
				 */
				rp = rp->child;
			}
			p->crinfo.role = rp;
			MEM_FREE(str);
			str = NULL;

			/* Credit name */
			if (!cdinfo_get_str("    CddbCredit_GetName",
					      CddbCredit_GetName,
					      credp, &p->crinfo.name)) {
				MEM_FREE(p);
				return FALSE;
			}

			/* Credit full name */
			if (!cdinfo_get_fullname("    CddbCredit_GetFullName",
					CddbCredit_GetFullName,
					credp,
					&p->crinfo.fullname.dispname,
					&p->crinfo.fullname.lastname,
					&p->crinfo.fullname.firstname,
					&p->crinfo.fullname.the)) {
				MEM_FREE(p);
				return FALSE;
			}

			/* Credit notes */
			if (!cdinfo_get_str("    CddbCredit_GetNotes",
					      CddbCredit_GetNotes,
					      credp, &p->notes)) {
				MEM_FREE(p);
				return FALSE;
			}

			CddbReleaseObject(credp);

			if (r->credit_list == NULL) {
				r->credit_list = p;
				p->prev = NULL;
			}
			else {
				q->next = p;
				p->prev = q;
			}
			q = p;
			p->next = NULL;
		}

		CddbReleaseObject(segp);

		if (cdinfo_dbp->disc.segment_list == NULL) {
			cdinfo_dbp->disc.segment_list = r;
			r->prev = NULL;
		}
		else {
			s->next = r;
			r->prev = s;
		}
		s = r;
		r->next = NULL;
	}

	return TRUE;
}


/*
 * cdinfo_trackinfo_query
 *	Query CDDB for track information
 *
 * Args:
 *	discp - CddbDisc object
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_trackinfo_query(CddbDiscPtr discp)
{
	CddbResult	ret;
	CddbTrackPtr	trkp = NULL;
	CddbCreditPtr	credp = NULL;
	long		ntrks,
			ncreds;
	int		i,
			j;
	char		*str;
	cdinfo_genre_t	*gp,
			*sgp;
	cdinfo_credit_t	*p,
			*q;
	cdinfo_role_t	*rp;

	/* Track information */
	DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetNumTracks: ");
	ret = CddbDisc_GetNumTracks(discp, &ntrks);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld tracks\n", ret, ntrks);

	cdinfo_set_errstr(ret);

	for (i = 0; i < (int) ntrks; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetTrack[%02d]: ", i+1);
		ret = CddbDisc_GetTrack(discp, (long) (i+1), &trkp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK)
			continue;

		/* Track artist */
		if (!cdinfo_get_str("  CddbTrack_GetArtist",
				CddbTrack_GetArtist,
				trkp, &cdinfo_dbp->track[i].artist)) {
			return FALSE;
		}

		/* Track title */
		if (!cdinfo_get_str("  CddbTrack_GetTitle",
				CddbTrack_GetTitle,
				trkp, &cdinfo_dbp->track[i].title)) {
			return FALSE;
		}

		/* Artist full name */
		if (!cdinfo_get_fullname("  CddbTrack_GetArtistFullName",
				CddbTrack_GetArtistFullName,
				trkp,
				&cdinfo_dbp->track[i].artistfname.dispname,
				&cdinfo_dbp->track[i].artistfname.lastname,
				&cdinfo_dbp->track[i].artistfname.firstname,
				&cdinfo_dbp->track[i].artistfname.the)) {
			return FALSE;
		}

		/* Track sort title */
		if (!cdinfo_get_str("  CddbTrack_GetTitleSort",
				CddbTrack_GetTitleSort,
				trkp, &cdinfo_dbp->track[i].sorttitle)) {
			return FALSE;
		}

		/* Track sort title */
		if (!cdinfo_get_str("  CddbTrack_GetTitleThe",
				CddbTrack_GetTitleThe,
				trkp, &cdinfo_dbp->track[i].title_the)) {
			return FALSE;
		}

		/* Year */
		if (!cdinfo_get_str("  CddbTrack_GetYear",
				CddbTrack_GetYear,
				trkp, &cdinfo_dbp->track[i].year)) {
			return FALSE;
		}

		/* Label */
		if (!cdinfo_get_str("  CddbTrack_GetLabel",
				CddbTrack_GetLabel,
				trkp, &cdinfo_dbp->track[i].label)) {
			return FALSE;
		}

		/* Genre */
		if (!cdinfo_get_str("  CddbTrack_GetGenreId",
				CddbTrack_GetGenreId,
				trkp, &cdinfo_dbp->track[i].genre)) {
			return FALSE;
		}
		gp = cdinfo_genre(cdinfo_dbp->track[i].genre);
		if (gp != NULL && gp->parent == NULL) {
			/* Illegal: the primary genre is set to a genre
			 * category.  "Fix" it by setting it to the category's
			 * "General" subgenre.  If not found, then set it to
			 * the first subgenre.
			 */
			for (sgp = gp->child; sgp != NULL; sgp = sgp->next) {
			    if (sgp->name != NULL &&
				strncmp(sgp->name, "General ", 8) == 0) {
				if (!util_newstr(&cdinfo_dbp->track[i].genre,
						 sgp->id)) {
				    CDINFO_FATAL(app_data.str_nomemory);
				    return FALSE;
				}
				break;
			    }
			}
			if (sgp == NULL) {
			    if (gp->child != NULL) {
				if (!util_newstr(&cdinfo_dbp->track[i].genre,
						 gp->child->id)) {
				    CDINFO_FATAL(app_data.str_nomemory);
				    return FALSE;
				}
			    }
			    else {
				MEM_FREE(cdinfo_dbp->track[i].genre);
				cdinfo_dbp->track[i].genre = NULL;
			    }
			}
		}

		/* Secondary Genre */
		if (!cdinfo_get_str("  CddbTrack_GetSecondaryGenreId",
				CddbTrack_GetSecondaryGenreId,
				trkp, &cdinfo_dbp->track[i].genre2)) {
			return FALSE;
		}
		gp = cdinfo_genre(cdinfo_dbp->track[i].genre2);
		if (gp != NULL && gp->parent == NULL) {
			/* Illegal: the secondary genre is set to a genre
			 * category.  "Fix" it by setting it to the category's
			 * "General" subgenre.  If not found, then set it to
			 * the first subgenre.
			 */
			for (sgp = gp->child; sgp != NULL; sgp = sgp->next) {
			    if (sgp->name != NULL &&
				strncmp(sgp->name, "General ", 8) == 0) {
				if (!util_newstr(&cdinfo_dbp->track[i].genre2,
						 sgp->id)) {
				    CDINFO_FATAL(app_data.str_nomemory);
				    return FALSE;
				}
				break;
			    }
			}
			if (sgp == NULL) {
			    if (gp->child != NULL) {
				if (!util_newstr(&cdinfo_dbp->track[i].genre2,
						 gp->child->id)) {
				    CDINFO_FATAL(app_data.str_nomemory);
				    return FALSE;
				}
			    }
			    else {
				MEM_FREE(cdinfo_dbp->track[i].genre2);
				cdinfo_dbp->track[i].genre2 = NULL;
			    }
			}
		}

		/* BPM */
		if (!cdinfo_get_str("  CddbTrack_GetBeatsPerMinute",
				CddbTrack_GetBeatsPerMinute,
				trkp, &cdinfo_dbp->track[i].bpm)) {
			return FALSE;
		}

		/* Notes */
		if (!cdinfo_get_str("  CddbTrack_GetNotes",
				CddbTrack_GetNotes,
				trkp, &cdinfo_dbp->track[i].notes)) {
			return FALSE;
		}

		/* ISRC */
		if (!cdinfo_get_str("  CddbTrack_GetISRC",
				CddbTrack_GetISRC,
				trkp, &cdinfo_dbp->track[i].isrc)) {
			return FALSE;
		}

		/* Track Credits */
		DBGPRN(DBG_CDI)(errfp, "  CddbTrack_GetNumCredits: ");
		ret = CddbTrack_GetNumCredits(trkp, &ncreds);
		DBGPRN(DBG_CDI)(errfp, "0x%lx %ld credits\n", ret, ncreds);

		cdinfo_set_errstr(ret);

		q = NULL;
		for (j = 0; j < (int) ncreds; j++) {
			DBGPRN(DBG_CDI)(errfp,
				"  CddbTrack_GetCredit[%02d]: ", j+1);
			ret = CddbTrack_GetCredit(trkp, (long) (j+1), &credp);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);
			
			if (ret != Cddb_OK)
				continue;

			/* Allocate a credit structure */
			p = (cdinfo_credit_t *) MEM_ALLOC(
				"cdinfo_credit_t",
				sizeof(cdinfo_credit_t)
			);
			if (p == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return FALSE;
			}
			(void) memset(p, 0, sizeof(cdinfo_credit_t));

			/* Credit role */
			str = NULL;
			if (!cdinfo_get_str("    CddbCredit_GetId",
					      CddbCredit_GetId, credp, &str)) {
				MEM_FREE(p);
				return FALSE;
			}
			rp = cdinfo_role(str);
			if (rp->parent == NULL) {
				/* Illegal: role set to the category role
				 * "Fix" it by setting the role to the
				 * first subrole of that category instead.
				 * If for some reason there are no subroles
				 * then the child would be NULL and thus the
				 * role would be "None -> None".
				 */
				rp = rp->child;
			}
			p->crinfo.role = rp;
			MEM_FREE(str);
			str = NULL;

			/* Credit name */
			if (!cdinfo_get_str("    CddbCredit_GetName",
					      CddbCredit_GetName,
					      credp, &p->crinfo.name)) {
				MEM_FREE(p);
				return FALSE;
			}

			/* Credit full name */
			if (!cdinfo_get_fullname("    CddbCredit_GetFullName",
						CddbCredit_GetFullName,
						credp,
						&p->crinfo.fullname.dispname,
						&p->crinfo.fullname.lastname,
						&p->crinfo.fullname.firstname,
						&p->crinfo.fullname.the)) {
				MEM_FREE(p);
				return FALSE;
			}

			/* Credit notes */
			if (!cdinfo_get_str("    CddbCredit_GetNotes",
					      CddbCredit_GetNotes,
					      credp, &p->notes)) {
				MEM_FREE(p);
				return FALSE;
			}

			CddbReleaseObject(credp);

			if (cdinfo_dbp->track[i].credit_list == NULL) {
				cdinfo_dbp->track[i].credit_list = p;
				p->prev = NULL;
			}
			else {
				q->next = p;
				p->prev = q;
			}
			q = p;
			p->next = NULL;
		}

		CddbReleaseObject(trkp);
	}

	return TRUE;
}


/*
 * cdinfo_add_urllist
 *	Given a CddbURLList, add the items in that list to the URL list
 *	specified by *listhead.
 *
 * Args:
 *	listhead - Address of the listhead
 *	urllistp - The CddbURLList
 *	type - WTYPE_ALBUM or WTYPE_GEN
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure (cannot allocate memory)
 */
STATIC bool_t
cdinfo_add_urllist(cdinfo_url_t **listhead, CddbURLListPtr urllistp, int wtype)
{
	CddbURLPtr		urlp;
	CddbResult		ret;
	long			cnt,
				i;
	cdinfo_url_t		*p;
	static cdinfo_url_t	*q = NULL;

	cnt = 0;
	DBGPRN(DBG_CDI)(errfp, "CddbURLList_GetCount: ");
	ret = CddbURLList_GetCount(urllistp, &cnt);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld entries\n", ret, cnt);

	if (ret != Cddb_OK || cnt == 0)
		return TRUE;

	for (i = 1; i <= cnt; i++) {
		urlp = NULL;
		DBGPRN(DBG_CDI)(errfp, "CddbURLList_GetURL: ");
		ret = CddbURLList_GetURL(urllistp, i, &urlp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK || urlp == NULL)
			return TRUE;

		p = (cdinfo_url_t *)(void *) MEM_ALLOC(
			"cdinfo_url_t", sizeof(cdinfo_url_t)
		); 
		if (p == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		(void) memset(p, 0, sizeof(cdinfo_url_t));

		if (!cdinfo_get_str("CddbURL_GetType",
					CddbURL_GetType,
					urlp, &p->type)) {
			MEM_FREE(p);
			return FALSE;
		}
		if (!cdinfo_get_str("CddbURL_GetHref",
					CddbURL_GetHref,
					urlp, &p->href)) {
			MEM_FREE(p);
			return FALSE;
		}
		if (!cdinfo_get_str("CddbURL_GetDisplayLink",
					CddbURL_GetDisplayLink,
					urlp, &p->displink)) {
			MEM_FREE(p);
			return FALSE;
		}
		if (!cdinfo_get_str("CddbURL_GetDisplayText",
					CddbURL_GetDisplayText,
					urlp, &p->disptext)) {
			MEM_FREE(p);
			return FALSE;
		}
		if (!cdinfo_get_str("CddbURL_GetCategory",
					CddbURL_GetCategory,
					urlp, &p->categ)) {
			MEM_FREE(p);
			return FALSE;
		}
		if (!cdinfo_get_str("CddbURL_GetSize",
					CddbURL_GetSize,
					urlp, &p->size)) {
			MEM_FREE(p);
			return FALSE;
		}
		if (!cdinfo_get_str("CddbURL_GetWeight",
					CddbURL_GetWeight,
					urlp, &p->weight)) {
			MEM_FREE(p);
			return FALSE;
		}

		p->wtype = wtype;

		if (*listhead == NULL)
			*listhead = q = p;
		else {
			q->next = p;
			q = p;
		}

		CddbReleaseObject(urlp);
	}

	return TRUE;
}


/*
 * cdinfo_urls_query
 *	Query CDDB for URLs information
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *	discp - CddbDisc object.  If NULL, then general-interest URLs
 *		are queried.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_urls_query(cdinfo_cddb_t *cp, CddbDiscPtr discp)
{
	CddbURLManagerPtr	urlmgrp = NULL;
	CddbURLListPtr		urllistp = NULL;
	CddbResult		ret;
	int			wtype;
	cdinfo_url_t		**listhead;

	/* Get URLS from the service */
	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetURLList: ");
	ret = CddbControl_GetURLList(cp->ctrlp, discp, 0, &urllistp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	if (urllistp != NULL)
		CddbReleaseObject(urllistp);

	if (discp == NULL) {
		listhead = &cdinfo_dbp->gen_url_list;
		wtype = WTYPE_GEN;
	}
	else {
		listhead = &cdinfo_dbp->disc_url_list;
		wtype = WTYPE_ALBUM;
	}

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetURLManager: ");
	ret = CddbControl_GetURLManager(cp->ctrlp, &urlmgrp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	if (ret != Cddb_OK || urlmgrp == NULL)
		return FALSE;

#if 0	/* Not currently used */
	/* Cover URLs */
	DBGPRN(DBG_CDI)(errfp, "CddbURLManager_GetCoverURLs: ");
	ret = CddbURLManager_GetCoverURLs(urlmgrp, discp, &urllistp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);
	if (ret == Cddb_OK && urllistp != NULL) {
		if (!cdinfo_add_urllist(listhead, urllistp, wtype)) {
			CddbReleaseObject(urllistp);
			CddbReleaseObject(urlmgrp);
			return FALSE;
		}
		CddbReleaseObject(urllistp);
	}
#endif

	/* Menu URLs */
	DBGPRN(DBG_CDI)(errfp, "CddbURLManager_GetMenuURLs: ");
	ret = CddbURLManager_GetMenuURLs(urlmgrp, discp, &urllistp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);
	if (ret == Cddb_OK && urllistp != NULL) {
		if (!cdinfo_add_urllist(listhead, urllistp, wtype)) {
			CddbReleaseObject(urllistp);
			CddbReleaseObject(urlmgrp);
			return FALSE;
		}
		CddbReleaseObject(urllistp);
	}

	CddbReleaseObject(urlmgrp);

	return TRUE;
}


/*
 * cdinfo_build_matchlist
 *	Build multiple fuzzy match disc elements list.  This is used
 *	to query the user for a selection.
 *
 * Args:
 *	discs - CddbDiscs object pointer
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_build_matchlist(CddbDiscsPtr discsp)
{
	CddbResult	ret;
	CddbDiscPtr	discp;
	long		cnt,
			i;
	cdinfo_genre_t	*gp,
			*sgp;
	cdinfo_match_t	*mp,
			*mp2;

	DBGPRN(DBG_CDI)(errfp, "CddbDiscs_GetCount: ");
	ret = CddbDiscs_GetCount(discsp, &cnt);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld matches\n", ret, cnt);

	if (ret != Cddb_OK || cnt <= 0)
		return FALSE;

	mp = NULL;
	for (i = 1; i <= cnt; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbDiscs_GetDisc[%ld]: ", i);
		ret = CddbDiscs_GetDisc(discsp, i, &discp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret == Cddb_OK && discp != NULL) {
			mp2 = (cdinfo_match_t *)(void *) MEM_ALLOC(
				"cdinfo_match_t",
				sizeof(cdinfo_match_t)
			);
			if (mp2 == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return FALSE;
			}
			(void) memset(mp2, 0, sizeof(cdinfo_match_t));

			mp2->tag = i;

			if (!cdinfo_get_str("  CddbDisc_GetArtist",
						CddbDisc_GetArtist,
						discp, &mp2->artist)) {
				return FALSE;
			}
			if (!cdinfo_get_str("  CddbDisc_GetTitle",
						CddbDisc_GetTitle,
						discp, &mp2->title)) {
				return FALSE;
			}
			if (!cdinfo_get_str("  CddbDisc_GetGenreId",
						CddbDisc_GetGenreId,
						discp, &mp2->genre)) {
				return FALSE;
			}
			gp = cdinfo_genre(cdinfo_dbp->disc.genre);
			if (gp != NULL && gp->parent == NULL) {
			    /* Illegal: the genre is set to a genre category.
			     * "Fix" it by setting it to the category's
			     * "General" subgenre.  If not found, then set
			     * it to the first subgenre.
			     */
			    for (sgp = gp->child; sgp != NULL;
				 sgp = sgp->next) {
				if (sgp->name != NULL &&
				    strncmp(sgp->name, "General ", 8) == 0) {
				    if (!util_newstr(&cdinfo_dbp->disc.genre,
						     sgp->id)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return FALSE;
				    }
				    break;
				}
			    }
			    if (sgp == NULL) {
				if (gp->child != NULL) {
				    if (!util_newstr(&cdinfo_dbp->disc.genre,
						     gp->child->id)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return FALSE;
				    }
				}
				else {
				    MEM_FREE(cdinfo_dbp->disc.genre);
				    cdinfo_dbp->disc.genre = NULL;
				}
			    }
			}

			if (cdinfo_dbp->matchlist == NULL)
				cdinfo_dbp->matchlist = mp = mp2;
			else {
				mp->next = mp2;
				mp = mp2;
			}

			CddbReleaseObject(discp);
		}
	}
	return TRUE;
}


/*
 * cdinfo_ctrlver_init
 *	Query CDDB control version information initialize the incore struct.
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_ctrlver_init(cdinfo_cddb_t *cp)
{
	return cdinfo_get_str("CddbControl_GetVersion",
				CddbControl_GetVersion, cp->ctrlp,
				&cdinfo_dbp->ctrl_ver);
}


/*
 * cdinfo_chk_service
 *	Check CDDB service for good network connectivity, and that the
 *	server is up and functional.
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cdinfo_chk_service(cdinfo_cddb_t *cp)
{
	CddbResult	ret;
	CddbStr		str;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetServiceStatus: ");
	ret = CddbControl_GetServiceStatus(cp->ctrlp, &str);
	if ((app_data.debug & DBG_CDI) && str != NULL) {
		/* Strip out extra newlines */
		while (str[strlen(str)-1] == '\n')
			str[strlen(str)-1] = '\0';
	}
	DBGPRN(DBG_CDI)(errfp, "0x%lx %s\n", ret, str == NULL ? "" : str);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK)
		return FALSE;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_ServerNoop: ");
	ret = CddbControl_ServerNoop(cp->ctrlp, 0);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	return (ret == Cddb_OK);
}


/*
 * cdinfo_genrelist_init
 *	Query genre tree information from CDDB and initialize the genre list.
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_genrelist_init(cdinfo_cddb_t *cp)
{
	CddbResult		ret;
	CddbGenreTreePtr	gtreep;
	CddbGenreListPtr	glistp;
	CddbGenrePtr		genrep;
	long			cnt,
				subcnt;
	int			i,
				j;
	cdinfo_genre_t		*p,
				*q,
				*r,
				*s;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetGenreTree: ");
	ret = CddbControl_GetGenreTree(cp->ctrlp, 0, &gtreep);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || gtreep == NULL)
		return FALSE;

	DBGPRN(DBG_CDI)(errfp, "CddbGenreTree_GetCount: ");
	ret = CddbGenreTree_GetCount(gtreep, &cnt);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld\n", ret, cnt);

	q = NULL;
	for (i = 1; i <= (int) cnt; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbGenreTree_GetMetaGenre[%d]: ", i);
		ret = CddbGenreTree_GetMetaGenre(gtreep, (long) i, &genrep);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK) {
			CddbReleaseObject(gtreep);
			return FALSE;
		}

		/* Allocate genre entry and fill it with genre info */
		if (!cdinfo_allocinit_genre(genrep, &p, FALSE)) {
			CddbReleaseObject(genrep);
			CddbReleaseObject(gtreep);
			return FALSE;
		}

		CddbReleaseObject(genrep);

		/* Sub-Genre list */
		DBGPRN(DBG_CDI)(errfp, "    CddbGenreTree_GetSubGenreList: ");
		ret = CddbGenreTree_GetSubGenreList(gtreep, p->id, &glistp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK) {
			CddbReleaseObject(gtreep);
			return FALSE;
		}

		DBGPRN(DBG_CDI)(errfp, "    CddbGenreList_GetCount: ");
		ret = CddbGenreList_GetCount(glistp, &subcnt);
		DBGPRN(DBG_CDI)(errfp, "0x%lx %ld\n", ret, subcnt);

		r = NULL;
		for (j = 1; j <= (int) subcnt; j++) {
			DBGPRN(DBG_CDI)(errfp,
				"    CddbGenreList_GetGenre[%d]: ", j);
			ret = CddbGenreList_GetGenre(glistp,
						     (long) j, &genrep);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

			/* Allocate genre entry and fill it with genre info */
			if (!cdinfo_allocinit_genre(genrep, &s, TRUE)) {
				CddbReleaseObject(genrep);
				CddbReleaseObject(glistp);
				CddbReleaseObject(gtreep);
				return FALSE;
			}

			CddbReleaseObject(genrep);

			s->parent = p;
			if (p->child == NULL)
				p->child = r = s;
			else {
				r->next = s;
				r = s;
			}
		}

		CddbReleaseObject(glistp);

		if (cdinfo_dbp->genrelist == NULL)
			cdinfo_dbp->genrelist = q = p;
		else {
			q->next = p;
			q = p;
		}
		p->parent = NULL;
	}

	CddbReleaseObject(gtreep);
	return TRUE;
}


/*
 * cdinfo_regionlist_init
 *	Query region list information from CDDB and initialize the region list.
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_regionlist_init(cdinfo_cddb_t *cp)
{
	CddbResult		ret;
	CddbRegionListPtr	relistp;
	CddbRegionPtr		regionp;
	cdinfo_region_t		*p,
				*q;
	long			cnt;
	int			i;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetRegionList: ");
	ret = CddbControl_GetRegionList(cp->ctrlp, 0, &relistp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK)
		return FALSE;

	DBGPRN(DBG_CDI)(errfp, "CddbRegionList_GetCount: ");
	ret = CddbRegionList_GetCount(relistp, &cnt);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld\n", ret, cnt);

	q = NULL;
	for (i = 1; i <= (int) cnt; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbRegionList_GetRegion[%d]: ", i);
		ret = CddbRegionList_GetRegion(relistp, (long) i, &regionp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK) {
			CddbReleaseObject(relistp);
			return FALSE;
		}

		/* Allocate region entry and fill it with region info */
		if (!cdinfo_allocinit_region(regionp, &p)) {
			CddbReleaseObject(regionp);
			CddbReleaseObject(relistp);
			return FALSE;
		}

		CddbReleaseObject(regionp);

		if (cdinfo_dbp->regionlist == NULL)
			cdinfo_dbp->regionlist = q = p;
		else {
			q->next = p;
			q = p;
		}
	}

	CddbReleaseObject(relistp);
	return TRUE;
}


/*
 * cdinfo_langlist_init
 *	Query language list information from CDDB and initialize the
 *	language list.
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_langlist_init(cdinfo_cddb_t *cp)
{
	CddbResult		ret;
	CddbLanguageListPtr	langlistp;
	CddbLanguagePtr		langp;
	cdinfo_lang_t		*p,
				*q;
	long			cnt;
	int			i;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetLanguageList: ");
	ret = CddbControl_GetLanguageList(cp->ctrlp, 0, &langlistp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK)
		return FALSE;

	DBGPRN(DBG_CDI)(errfp, "CddbLanguageList_GetCount: ");
	ret = CddbLanguageList_GetCount(langlistp, &cnt);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld\n", ret, cnt);

	q = NULL;
	for (i = 1; i <= (int) cnt; i++) {
		DBGPRN(DBG_CDI)(errfp,
			"CddbLanguageList_GetLanguage[%d]: ", i);
		ret = CddbLanguageList_GetLanguage(
			langlistp, (long) i, &langp
		);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK) {
			CddbReleaseObject(langlistp);
			return FALSE;
		}

		/* Allocate language entry and fill it with language info */
		if (!cdinfo_allocinit_lang(langp, &p)) {
			CddbReleaseObject(langp);
			CddbReleaseObject(langlistp);
			return FALSE;
		}

		CddbReleaseObject(langp);

		if (cdinfo_dbp->langlist == NULL)
			cdinfo_dbp->langlist = q = p;
		else {
			q->next = p;
			q = p;
		}
	}

	CddbReleaseObject(langlistp);
	return TRUE;
}


/*
 * cdinfo_rolelist_init
 *	Query role list information from CDDB and initialize the role list.
 *
 * Args:
 *	cp - cdinfo_cddb_t pointer for the CDDB connection
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
STATIC bool_t
cdinfo_rolelist_init(cdinfo_cddb_t *cp)
{
	CddbResult		ret;
	CddbRoleTreePtr		rotreep;
	CddbRoleListPtr		rolistp;
	CddbRolePtr		rolep;
	long			tcnt,
				lcnt;
	int			i,
				j;
	cdinfo_role_t		*p,
				*q,
				*r,
				*s;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetRoleTree: ");
	ret = CddbControl_GetRoleTree(cp->ctrlp, 0, &rotreep);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK)
		return FALSE;

	DBGPRN(DBG_CDI)(errfp, "CddbRoleTree_GetCount: ");
	ret = CddbRoleTree_GetCount(rotreep, &tcnt);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld\n", ret, tcnt);

	q = NULL;
	for (i = 1; i <= (int) tcnt; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbRoleTree_GetRoleList[%d]: ", i);
		ret = CddbRoleTree_GetRoleList(rotreep, (long) i, &rolistp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK) {
			CddbReleaseObject(rotreep);
			return FALSE;
		}

		DBGPRN(DBG_CDI)(errfp, "  CddbRoleList_GetCategoryRole: ");
		ret = CddbRoleList_GetCategoryRole(rolistp, &rolep);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK) {
			CddbReleaseObject(rolistp);
			CddbReleaseObject(rotreep);
			return FALSE;
		}

		/* Allocate role entry and fill it with role info */
		if (!cdinfo_allocinit_role(rolep, &p)) {
			CddbReleaseObject(rolep);
			CddbReleaseObject(rolistp);
			CddbReleaseObject(rotreep);
			return FALSE;
		}

		CddbReleaseObject(rolep);

		DBGPRN(DBG_CDI)(errfp, "  CddbRoleList_GetCount: ");
		ret = CddbRoleList_GetCount(rolistp, &lcnt);
		DBGPRN(DBG_CDI)(errfp, "0x%lx %ld\n", ret, lcnt);

		r = NULL;
		for (j = 1; j <= (int) lcnt; j++) {
			DBGPRN(DBG_CDI)(errfp,
				"  CddbRoleList_GetRole[%d]: ", j);
			ret = CddbRoleList_GetRole(rolistp, (long) j, &rolep);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

			if (ret != Cddb_OK) {
				CddbReleaseObject(rolistp);
				CddbReleaseObject(rotreep);
				return FALSE;
			}

			/* Allocate role entry and fill it with role info */
			if (!cdinfo_allocinit_role(rolep, &s)) {
				CddbReleaseObject(rolep);
				CddbReleaseObject(rolistp);
				CddbReleaseObject(rotreep);
				return FALSE;
			}

			CddbReleaseObject(rolep);

			s->parent = p;
			if (p->child == NULL)
				p->child = r = s;
			else {
				r->next = s;
				r = s;
			}
		}

		CddbReleaseObject(rolistp);

		if (cdinfo_dbp->rolelist == NULL)
			cdinfo_dbp->rolelist = q = p;
		else {
			q->next = p;
			q = p;
		}
		p->parent = NULL;
	}

	CddbReleaseObject(rotreep);
	return TRUE;
}


/* 
 * cdinfo_opencddb
 *	Open a remote CDDB connection
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	query - Whether we're opening CDDB connection for a query operation
 *	retcode - return status code
 *
 * Return:
 *	Open descriptor cdbinfo_file_t
 */
/*ARGSUSED*/
cdinfo_cddb_t *
cdinfo_opencddb(curstat_t *s, bool_t query, int *retcode)
{
	CddbOptionsPtr	optp = NULL;
	cdinfo_cddb_t	*cp;
	CddbResult	ret;
	int		i;
	long		cacheflags;
	char		cache_path[FILE_PATH_SZ];

	/* Default is success */
	*retcode = 0;

	if (cdinfo_cddbp != NULL) {
		/* Already opened - only one open allowed at a time */
		*retcode = OPEN_ERR;
		return NULL;
	}

	/* Allocate cdinfo_cddb_t */
	cp = (cdinfo_cddb_t *)(void *) MEM_ALLOC(
		"cdinfo_cddb_t",
		sizeof(cdinfo_cddb_t)
	);
	if (cp == NULL) {
		*retcode = MEM_ERR;
		CDINFO_FATAL(app_data.str_nomemory);
		return NULL;
	}

	/* Save pointer for signal handler */
	cdinfo_cddbp = cp;

	DBGPRN(DBG_CDI)(errfp, "\nOpening CDDB service...\n");

	DBGPRN(DBG_CDI)(errfp, "CddbInitialize: ");
	ret = CddbInitialize((CddbControlPtr *) &cp->ctrlp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	switch (ret) {
	case Cddb_OK:
		if (cddb_ifver() != 2) {
			*retcode = LIBCDDB_ERR;
			(void) fprintf(errfp,
			    "ERROR: libcddb and libcddbkey mismatch (2:1).\n");
			return NULL;
		}
		break;

	case Cddb_ISCDDB1:
		if (cddb_ifver() != 1) {
			*retcode = LIBCDDB_ERR;
			(void) fprintf(errfp,
			    "ERROR: libcddb and libcddbkey mismatch (1:2).\n");
			return NULL;
		}
		break;

	default:
		cdinfo_set_errstr(ret);
		*retcode = INIT_ERR;
		return NULL;
	}

	if (cp->ctrlp == NULL ||
	    !cddb_setkey(cp, cdinfo_clinfo, &app_data, s, errfp)) {
		*retcode = INIT_ERR;
		return NULL;
	}

	i = 0;
	for (;;) {
		DBGPRN(DBG_CDI)(errfp, "CddbControl_Initialize: ");
		ret = CddbControl_Initialize(
			cp->ctrlp, 0,
			app_data.cdinfo_inetoffln ?
				CACHE_DONT_CONNECT : CACHE_DEFAULT
		);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret == Cddb_OK || ret == Cddb_FALSE)
			break;

		if (++i > 1) {
			cdinfo_set_errstr(ret);
			(void) cdinfo_closecddb(cp);
			*retcode = INIT_ERR;
			return NULL;
		}

		/* Try removing the cache file */
		(void) sprintf(cache_path, "%s/.cddb2/%s/cddb.ds",
				util_homedir(util_get_ouid()),
				XMCD_CLIENT_ID);
		(void) UNLINK(cache_path);
	}

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetOptions: ");
	ret = CddbControl_GetOptions(cp->ctrlp, &optp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	if (ret == Cddb_OK && optp != NULL) {
		if (app_data.use_proxy) {
			char	*p;
			long	port = HTTP_PORT;

			if ((p = strchr(app_data.proxy_server, ':')) != NULL){
				*p = '\0';
				port = atol(p+1);
			}

			/* Proxy server */
			(void) cdinfo_put_str("CddbOptions_PutProxyServer",
				CddbOptions_PutProxyServer, optp,
				app_data.proxy_server
			);

			/* Proxy server port */
			DBGPRN(DBG_CDI)(errfp,
				"CddbOptions_PutProxyServerPort: ");
			ret = CddbOptions_PutProxyServerPort(optp, port);
			DBGPRN(DBG_CDI)(errfp, "0x%lx port %ld\n", ret, port);

			if (p != NULL)
				*p = ':';

			if (app_data.proxy_auth) {
				if (cdinfo_dbp->proxy_user == NULL ||
				    cdinfo_dbp->proxy_passwd == NULL) {
					CddbReleaseObject(optp);

					*retcode = AUTH_ERR;
					(void) cdinfo_closecddb(cp);
					return NULL;
				}

#ifdef __VMS
				/* Hack to work around a memory
				 * corruption problem on VMS.
				 */
				{
				    static char	*sav_user = NULL,
						*sav_pass = NULL;

				    if (cdinfo_dbp->proxy_user[0] == '\0' ||
					cdinfo_dbp->proxy_passwd[0] == '\0') {
					if (sav_user != NULL &&
					    sav_pass != NULL) {
					    (void) strcpy(
						    cdinfo_dbp->proxy_user,
						    sav_user
					    );
					    (void) strcpy(
						    cdinfo_dbp->proxy_passwd,
						    sav_pass
					    );
					}
				    }
				    else {
					if (!util_newstr(&sav_user,
						    cdinfo_dbp->proxy_user) ||
					    !util_newstr(&sav_pass,
						    cdinfo_dbp->proxy_passwd)){
					    *retcode = MEM_ERR;
					    return NULL;
					}
				    }
				}
#endif	/* __VMS */

				/* Proxy user name */
				(void) cdinfo_put_str(
					"CddbOptions_PutProxyUserName",
					CddbOptions_PutProxyUserName, optp,
					cdinfo_dbp->proxy_user
				);

				/* Proxy password */
				(void) cdinfo_put_str(
					"CddbOptions_PutProxyPassword",
					CddbOptions_PutProxyPassword, optp,
					cdinfo_dbp->proxy_passwd
				);
			}
		}
		else {
			/* Clear proxy server settings if any */
			(void) cdinfo_put_str(
				"CddbOptions_PutProxyServer",
				CddbOptions_PutProxyServer, optp,
				NULL
			);

			/* Clear proxy server user name if any */
			(void) cdinfo_put_str(
				"CddbOptions_PutProxyUserName",
				CddbOptions_PutProxyUserName, optp,
				NULL
			);

			/* Clear proxy server password if any */
			(void) cdinfo_put_str(
				"CddbOptions_PutProxyPassword",
				CddbOptions_PutProxyPassword, optp,
				NULL
			);
		}

		/* Set local cache flags */
		cacheflags = app_data.cdinfo_inetoffln ?
			CACHE_DONT_CONNECT : CACHE_DEFAULT;
		DBGPRN(DBG_CDI)(errfp, "CddbOptions_PutLocalCacheFlags: ");
		ret = CddbOptions_PutLocalCacheFlags(optp, cacheflags);
		DBGPRN(DBG_CDI)(errfp, "0x%lx flags=0x%lx\n", ret, cacheflags);

		/* Set local cache timeout value */
		DBGPRN(DBG_CDI)(errfp, "CddbOptions_PutLocalCacheTimeout: ");
		ret = CddbOptions_PutLocalCacheTimeout(
			optp, (long) app_data.cache_timeout
		);
		DBGPRN(DBG_CDI)(errfp,
			"0x%lx %d\n", ret, app_data.cache_timeout);

		/* Set server timeout value */
		DBGPRN(DBG_CDI)(errfp, "CddbOptions_PutServerTimeout: ");
		ret = CddbOptions_PutServerTimeout(
			optp, (long) (app_data.srv_timeout * 1000)
		);
		DBGPRN(DBG_CDI)(errfp,
			"0x%lx %d\n", ret, app_data.srv_timeout * 1000);

		DBGPRN(DBG_CDI)(errfp, "CddbOptions_PutTestSubmitMode: ");
#ifdef CDINFO_PRODUCTION
		ret = CddbOptions_PutTestSubmitMode(optp, 0);
		DBGPRN(DBG_CDI)(errfp, "0x%lx False\n", ret);
#else
		ret = CddbOptions_PutTestSubmitMode(optp, 1);
		DBGPRN(DBG_CDI)(errfp, "0x%lx True\n", ret);
#endif

		/* Write out options */
		DBGPRN(DBG_CDI)(errfp, "CddbControl_SetOptions: ");
		ret = CddbControl_SetOptions(cp->ctrlp, optp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		CddbReleaseObject(optp);
	}

	return (cp);
}


/* 
 * cdinfo_closecddb
 *	Close a remote CDDB connection
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	cdinfo_opencddb.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_closecddb(cdinfo_cddb_t *cp)
{
	CddbResult	ret;

	if (cp == NULL)
		return FALSE;

	if (cp->ctrlp != NULL) {
		DBGPRN(DBG_CDI)(errfp, "CddbControl_Shutdown: ");
		ret = CddbControl_Shutdown(cp->ctrlp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		DBGPRN(DBG_CDI)(errfp, "CddbTerminate: ");
		ret = CddbTerminate(cp->ctrlp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);
	}

	MEM_FREE(cp);
	cdinfo_cddbp = NULL;

	return TRUE;
}


/*
 * cdinfo_initcddb
 *	Perform CDDB initializations
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	     cdinfo_opencddb.
 *	retcode - Pointer to location where a cdinfo_ret_t code is returned.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_initcddb(cdinfo_cddb_t *cp, cdinfo_ret_t *retcode)
{
	*retcode = 0;

	/* Get CDDB control version */
	if (cdinfo_dbp->ctrl_ver == NULL && !cdinfo_ctrlver_init(cp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	/* Set up region list if not done */
	if (cdinfo_dbp->regionlist == NULL && !cdinfo_regionlist_init(cp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	/* Set up language list if not done */
	if (cdinfo_dbp->langlist == NULL && !cdinfo_langlist_init(cp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	if ((app_data.debug & DBG_CDI) != 0 && !app_data.cdinfo_inetoffln &&
	    !cdinfo_chk_service(cp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	/* Check user registration */
	if (!cdinfo_check_userreg(cp)) {
		cdinfo_dbp->flags |= CDINFO_NEEDREG;
		return TRUE;
	}

	/* Set up role list if not done */
	if (cdinfo_dbp->rolelist == NULL && !cdinfo_rolelist_init(cp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	/* Set up genre list if not done */
	if (cdinfo_dbp->genrelist == NULL && !cdinfo_genrelist_init(cp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	/* Query CDDB about general URLs if not done */
	if (cdinfo_dbp->gen_url_list == NULL && !cdinfo_urls_query(cp, NULL)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	return TRUE;
}


/*
 * cdinfo_querycddb
 *	Read CDDB data for the current CD and update incore structures.
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	cdinfo_opencddb.
 *	s - Pointer to curstat_t structure.
 *	retcode - Pointer to location where a cdinfo_ret_t code is returned.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_querycddb(cdinfo_cddb_t *cp, curstat_t *s, cdinfo_ret_t *retcode)
{
	CddbDiscPtr	discp = NULL,
			discp2;
	CddbDiscsPtr	discsp = NULL;
	CddbStr		tocstr;
	CddbResult	ret;
	CDDBMatchCode	matchcode = MATCH_NONE;

	if (!cdinfo_initcddb(cp, retcode))
		return FALSE;

	if ((cdinfo_dbp->flags & CDINFO_NEEDREG) != 0)
		return TRUE;

	if (cdinfo_dbp->match_tag == -1) {
		/* Query by TOC */

		tocstr = (CddbStr) cdinfo_tocstr(s);
		DBGPRN(DBG_CDI)(errfp, "TOC string: %s\n", tocstr);

		DBGPRN(DBG_CDI)(errfp, "CddbControl_LookupMediaByToc: ");
		ret = CddbControl_LookupMediaByToc(
			cp->ctrlp, tocstr, 0, &matchcode
		);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		cdinfo_set_errstr(ret);

		if (ret != Cddb_OK) {
			if (ret == CDDBTRNHTTPProxyError)
				*retcode = AUTH_ERR;
			else
				*retcode = READ_ERR;
			return FALSE;
		}

		switch (matchcode) {
		case MATCH_EXACT:
			DBGPRN(DBG_CDI)(errfp, "Exact CDDB match\n");

			DBGPRN(DBG_CDI)(errfp, "CddbControl_GetMatchedDisc: ");
			ret = CddbControl_GetMatchedDisc(cp->ctrlp, &discp);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

			cdinfo_set_errstr(ret);

			if (ret != Cddb_OK || discp == NULL) {
				*retcode = READ_ERR;
				return FALSE;
			}
			break;

		case MATCH_MULTIPLE:
			DBGPRN(DBG_CDI)(errfp, "Multiple CDDB matches\n");

			/* Get the list of matching discs.
			 * Each one is a partial disc
			 */
			DBGPRN(DBG_CDI)(errfp,
				"CddbControl_GetMatchedDiscs: ");
			ret = CddbControl_GetMatchedDiscs(cp->ctrlp, &discsp);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

			cdinfo_set_errstr(ret);

			if (ret != Cddb_OK || discsp == NULL) {
				*retcode = READ_ERR;
				return FALSE;
			}

			/* Build matchlist */
			if (!cdinfo_build_matchlist(discsp)) {
				/* Failure */
				CddbReleaseObject(discsp);
				*retcode = READ_ERR;
				return FALSE;
			}

			/* Save discs pointer - release later */
			cdinfo_dbp->match_aux = (void *) discsp;
			return TRUE;

		case MATCH_NONE:
			DBGPRN(DBG_CDI)(errfp, "No CDDB match\n");
			return TRUE;

		default:
			DBGPRN(DBG_CDI)(errfp, "Unknown CDDB matchcode 0x%x\n",
				matchcode);
			*retcode = READ_ERR;
			return FALSE;
		}
	}
	else {
		/* Query after fuzzy selection */

		discsp = (CddbDiscsPtr) cdinfo_dbp->match_aux;

		if (cdinfo_dbp->match_tag == 0) {
			/* User chose "none of the above" */
			CddbReleaseObject(discsp);
			return TRUE;
		}

		DBGPRN(DBG_CDI)(errfp,
			"CddbDiscs_GetDisc[%ld]: ", cdinfo_dbp->match_tag);
		ret = CddbDiscs_GetDisc(discsp,
			cdinfo_dbp->match_tag,
			&discp2
		);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		cdinfo_set_errstr(ret);

		if (ret != Cddb_OK || discp2 == NULL) {
			CddbReleaseObject(discsp);
			*retcode = READ_ERR;
			return FALSE;
		}

		DBGPRN(DBG_CDI)(errfp, "CddbControl_GetFullDiscInfo: ");
		ret = CddbControl_GetFullDiscInfo(
			cp->ctrlp, discp2, 0, &discp
		);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		cdinfo_set_errstr(ret);

		if (ret != Cddb_OK) {
			CddbReleaseObject(discp2);
			CddbReleaseObject(discsp);
			*retcode = READ_ERR;
			return FALSE;
		}

		CddbReleaseObject(discp2);
		CddbReleaseObject(discsp);
	}

	/* Query CDDB about the disc */
	if (!cdinfo_discinfo_query(discp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	/* Query CDDB about the tracks */
	if (!cdinfo_trackinfo_query(discp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	/* Query CDDB about the disc-related URLs */
	if (!cdinfo_urls_query(cp, discp)) {
		*retcode = READ_ERR;
		return FALSE;
	}

	CddbReleaseObject(discp);

	cdinfo_dbp->flags |= CDINFO_MATCH;
	return TRUE;
}


/*
 * cdinfo_uregcddb
 *	Register the user with CDDB
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	cdinfo_opencddb.
 *	retcode - return status code
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_uregcddb(cdinfo_cddb_t *cp, cdinfo_ret_t *retcode)
{
	CddbUserInfoPtr	uip = NULL;
	CddbResult	ret;
	CddbBoolean	registered = 0;

	if (cdinfo_dbp->userreg.handle == NULL ||
	    cdinfo_dbp->userreg.passwd == NULL) {
		*retcode = REGI_ERR;
		return FALSE;
	}

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetUserInfo: ");
	ret = CddbControl_GetUserInfo(cp->ctrlp, &uip);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK && uip == NULL) {
		*retcode = REGI_ERR;
		return FALSE;
	}

	(void) cdinfo_put_str("CddbUserInfo_PutUserHandle",
		CddbUserInfo_PutUserHandle, uip,
		cdinfo_dbp->userreg.handle
	);
	(void) cdinfo_put_str("CddbUserInfo_PutPassword",
		CddbUserInfo_PutPassword, uip,
		cdinfo_dbp->userreg.passwd
	);
	(void) cdinfo_put_str("CddbUserInfo_PutPasswordHint",
		CddbUserInfo_PutPasswordHint, uip,
		cdinfo_dbp->userreg.hint == NULL ?
			"" : cdinfo_dbp->userreg.hint
	);
	(void) cdinfo_put_str("CddbUserInfo_PutEmailAddress",
		CddbUserInfo_PutEmailAddress, uip,
		cdinfo_dbp->userreg.email == NULL ?
			"" : cdinfo_dbp->userreg.email
	);
	(void) cdinfo_put_str("CddbUserInfo_PutRegionId",
		CddbUserInfo_PutRegionId, uip,
		cdinfo_dbp->userreg.region == NULL ?
			"" : cdinfo_dbp->userreg.region
	);
	(void) cdinfo_put_str("CddbUserInfo_PutPostalCode",
		CddbUserInfo_PutPostalCode, uip,
		cdinfo_dbp->userreg.postal == NULL ?
			"" : cdinfo_dbp->userreg.postal
	);
	(void) cdinfo_put_str("CddbUserInfo_PutAge",
		CddbUserInfo_PutAge, uip,
		cdinfo_dbp->userreg.age == NULL ?
			"" : cdinfo_dbp->userreg.age
	);
	(void) cdinfo_put_str("CddbUserInfo_PutSex",
		CddbUserInfo_PutSex, uip,
		cdinfo_dbp->userreg.gender == NULL ?
			"" : cdinfo_dbp->userreg.gender
	);

	DBGPRN(DBG_CDI)(errfp, "CddbUserInfo_PutAllowEmail: ");
	ret = CddbUserInfo_PutAllowEmail(uip,
		(CddbBoolean) cdinfo_dbp->userreg.allowemail
	);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %s\n", ret,
		cdinfo_dbp->userreg.allowemail ? "Yes" : "No"
	);

	DBGPRN(DBG_CDI)(errfp, "CddbUserInfo_PutAllowStats: ");
	ret = CddbUserInfo_PutAllowStats(uip,
		(CddbBoolean) cdinfo_dbp->userreg.allowstats
	);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %s\n", ret,
		cdinfo_dbp->userreg.allowstats ? "Yes" : "No"
	);

	/* Set the registration info */
	DBGPRN(DBG_CDI)(errfp, "CddbControl_SetUserInfo: ");
	ret = CddbControl_SetUserInfo(cp->ctrlp, uip);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	CddbReleaseObject(uip);

	switch ((unsigned int) ret) {
	case Cddb_OK:
		/* Success: check if user is now registered */
		DBGPRN(DBG_CDI)(errfp, "CddbControl_IsRegistered: ");
		ret = CddbControl_IsRegistered(cp->ctrlp, 0, &registered);
		DBGPRN(DBG_CDI)(errfp,
			"0x%lx %s\n", ret, registered ? "Yes" : "No");

		if (!registered)
			*retcode = REGI_ERR;

		DBGPRN(DBG_CDI)(errfp,
			"User \"%s\" is %s registered with CDDB.\n",
			cdinfo_dbp->userreg.handle,
			registered ? "now" : "NOT");

		return ((bool_t) registered);

	case (unsigned int) CDDBSVCHandleUsed:
		/* Incorrect password or handle taken */
		*retcode = NAME_ERR;
		return FALSE;

	default:
		/* Other error */
		*retcode = REGI_ERR;
		return FALSE;
	}
	/*NOTREACHED*/
}


/*
 * cdinfo_passhintcddb
 *	Request CDDB to e-mail the password hint.
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	cdinfo_opencddb.
 *	retcode - return status code
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_passhintcddb(cdinfo_cddb_t *cp, cdinfo_ret_t *retcode)
{
	CddbUserInfoPtr	uip = NULL;
	CddbResult	ret;

	if (cdinfo_dbp->userreg.handle == NULL) {
		*retcode = REGI_ERR;
		return FALSE;
	}

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetUserInfo: ");
	ret = CddbControl_GetUserInfo(cp->ctrlp, &uip);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK && uip == NULL) {
		*retcode = REGI_ERR;
		return FALSE;
	}

	(void) cdinfo_put_str("CddbUserInfo_PutUserHandle",
		CddbUserInfo_PutUserHandle, uip,
		cdinfo_dbp->userreg.handle
	);

	/* Set the registration info - request CDDB to mail password hint */
	DBGPRN(DBG_CDI)(errfp, "CddbControl_SetUserInfo: ");
	ret = CddbControl_SetUserInfo(cp->ctrlp, uip);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	CddbReleaseObject(uip);

	switch ((unsigned int) ret) {
	case Cddb_OK:
		/* Success */
		return TRUE;

	case (unsigned int) CDDBSVCUnknownHandle:
		/* Unknown handle */
		*retcode = NAME_ERR;
		return FALSE;

	case (unsigned int) CDDBSVCNoHint:
		/* No hint registered */
		*retcode = HINT_ERR;
		return FALSE;

	case (unsigned int) CDDBSVCNoEmail:
		/* No e-mail address registered */
		*retcode = MAIL_ERR;
		return FALSE;

	default:
		/* Other error */
		*retcode = REGI_ERR;
		return FALSE;
	}
	/*NOTREACHED*/
}


/*
 * cdinfo_submitcddb
 *	Submit current CD info to CDDB
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	     cdinfo_opencddb.
 *	s - Pointer to the curstat_t structure
 *	retcode - Return pointer of status code
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_submitcddb(cdinfo_cddb_t *cp, curstat_t *s, cdinfo_ret_t *retcode)
{
	CddbDiscPtr		discp = NULL;
	CddbTrackPtr		trkp = NULL;
	CddbCreditPtr		credp = NULL;
	CddbSegmentPtr		segp = NULL;
	CddbResult		ret;
	long			ntrks,
				pval;
	int			i;
	CddbStr			tocstr;
	cdinfo_credit_t		*p;
	cdinfo_segment_t	*q;

	if (!cdinfo_initcddb(cp, retcode))
		return FALSE;

	if ((cdinfo_dbp->flags & CDINFO_NEEDREG) != 0)
		return TRUE;

	tocstr = (CddbStr) cdinfo_tocstr(s);

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetSubmitDisc: ");
	ret = CddbControl_GetSubmitDisc(
		cp->ctrlp,
		tocstr,
		cdinfo_dbp->disc.mediaid,
		cdinfo_dbp->disc.muiid,
		&discp
	);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || discp == NULL) {
		if (discp != NULL)
			CddbReleaseObject(discp);
		*retcode = SUBMIT_ERR;
		return FALSE;
	}

	/*
	 * Fill in disc information
	 */
	(void) cdinfo_put_str("CddbDisc_PutToc",
		CddbDisc_PutToc, discp, tocstr
	);

	DBGPRN(DBG_CDI)(errfp, "CddbDisc_PutCompilation: ");
	ret = CddbDisc_PutCompilation(discp,
		(CddbBoolean) cdinfo_dbp->disc.compilation
	);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %s\n", ret,
		cdinfo_dbp->disc.compilation ? "Yes" : "No"
	);

	(void) cdinfo_put_str("CddbDisc_PutArtist",
		CddbDisc_PutArtist, discp, cdinfo_dbp->disc.artist
	);
	(void) cdinfo_put_str("CddbDisc_PutTitle",
		CddbDisc_PutTitle, discp, cdinfo_dbp->disc.title
	);
	(void) cdinfo_put_str("CddbDisc_PutTitleSort",
		CddbDisc_PutTitleSort, discp, cdinfo_dbp->disc.sorttitle
	);
	(void) cdinfo_put_str("CddbDisc_PutTitleThe",
		CddbDisc_PutTitleThe, discp, cdinfo_dbp->disc.title_the
	);
	(void) cdinfo_put_str("CddbDisc_PutYear",
		CddbDisc_PutYear, discp, cdinfo_dbp->disc.year
	);
	(void) cdinfo_put_str("CddbDisc_PutLabel",
		CddbDisc_PutLabel, discp, cdinfo_dbp->disc.label
	);
	(void) cdinfo_put_str("CddbDisc_PutGenreId",
		CddbDisc_PutGenreId, discp, cdinfo_dbp->disc.genre
	);
	(void) cdinfo_put_str("CddbDisc_PutSecondaryGenreId",
		CddbDisc_PutSecondaryGenreId, discp, cdinfo_dbp->disc.genre2
	);
	(void) cdinfo_put_str("CddbDisc_PutNumberInSet",
		CddbDisc_PutNumberInSet, discp, cdinfo_dbp->disc.dnum
	);
	(void) cdinfo_put_str("CddbDisc_PutTotalInSet",
		CddbDisc_PutTotalInSet, discp, cdinfo_dbp->disc.tnum
	);
	(void) cdinfo_put_str("CddbDisc_PutRegionId",
		CddbDisc_PutRegionId, discp, cdinfo_dbp->disc.region
	);
	(void) cdinfo_put_str("CddbDisc_PutLanguageId",
		CddbDisc_PutLanguageId, discp, cdinfo_dbp->disc.lang
	);
	(void) cdinfo_put_str("CddbDisc_PutNotes",
		CddbDisc_PutNotes, discp, cdinfo_dbp->disc.notes
	);
	(void) cdinfo_put_str("CddbDisc_PutRevision",
		CddbDisc_PutRevision, discp, cdinfo_dbp->disc.revision
	);
	(void) cdinfo_put_str("CddbDisc_PutRevisionTag",
		CddbDisc_PutRevisionTag, discp, cdinfo_dbp->disc.revtag
	);

	(void) cdinfo_put_fullname("CddbDisc_PutArtistFullName",
		CddbDisc_PutArtistFullName, discp,
		cdinfo_dbp->disc.artistfname.dispname,
		cdinfo_dbp->disc.artistfname.lastname,
		cdinfo_dbp->disc.artistfname.firstname,
		cdinfo_dbp->disc.artistfname.the
	);

	/* Disc credits */
	for (p = cdinfo_dbp->disc.credit_list; p != NULL; p = p->next) {
		DBGPRN(DBG_CDI)(errfp, "CddbDisc_AddCredit: ");
		ret = CddbDisc_AddCredit(discp,
				p->crinfo.role->id,
				p->crinfo.name,
				&credp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK || credp == NULL)
			continue;

		(void) cdinfo_put_fullname("  CddbCredit_PutFullName",
			CddbCredit_PutFullName, credp,
			p->crinfo.fullname.dispname,
			p->crinfo.fullname.lastname,
			p->crinfo.fullname.firstname,
			p->crinfo.fullname.the
		);

		(void) cdinfo_put_str("  CddbCredit_PutNotes",
			CddbCredit_PutNotes, credp,
			p->notes
		);

		CddbReleaseObject(credp);
	}

	/* Segments */
	for (q = cdinfo_dbp->disc.segment_list; q != NULL; q = q->next) {
		DBGPRN(DBG_CDI)(errfp, "CddbDisc_AddSegment: ");
		ret = CddbDisc_AddSegment(discp, q->name, &segp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK || segp == NULL)
			continue;

		(void) cdinfo_put_str("  CddbSegment_PutNotes",
			CddbSegment_PutNotes, segp,
			q->notes
		);
		(void) cdinfo_put_str("  CddbSegment_PutStartTrack",
			CddbSegment_PutStartTrack, segp,
			q->start_track
		);
		(void) cdinfo_put_str("  CddbSegment_PutStartFrame",
			CddbSegment_PutStartFrame, segp,
			q->start_frame
		);
		(void) cdinfo_put_str("  CddbSegment_PutEndTrack",
			CddbSegment_PutEndTrack, segp,
			q->end_track
		);
		(void) cdinfo_put_str("  CddbSegment_PutEndFrame",
			CddbSegment_PutEndFrame, segp,
			q->end_frame
		);

		/* Segment Credits */
		for (p = q->credit_list; p != NULL; p = p->next) {
			DBGPRN(DBG_CDI)(errfp, "    CddbSegment_AddCredit: ");
			ret = CddbSegment_AddCredit(segp,
					p->crinfo.role->id,
					p->crinfo.name,
					&credp);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

			if (ret != Cddb_OK || credp == NULL)
				continue;

			(void) cdinfo_put_fullname(
				"    CddbCredit_PutFullName",
				CddbCredit_PutFullName, credp,
				p->crinfo.fullname.dispname,
				p->crinfo.fullname.lastname,
				p->crinfo.fullname.firstname,
				p->crinfo.fullname.the
			);

			(void) cdinfo_put_str("    CddbCredit_PutNotes",
				CddbCredit_PutNotes, credp,
				p->notes
			);

			CddbReleaseObject(credp);
		}

		CddbReleaseObject(segp);
	}

	/*
	 * Fill in track information
	 */
	DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetNumTracks: ");
	ret = CddbDisc_GetNumTracks(discp, &ntrks);
	DBGPRN(DBG_CDI)(errfp, "0x%lx %ld tracks\n", ret, ntrks);

	for (i = 0; i < (int) ntrks; i++) {
		DBGPRN(DBG_CDI)(errfp, "CddbDisc_GetTrack[%02d]: ", i+1);
		ret = CddbDisc_GetTrack(discp, (long) (i+1), &trkp);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		if (ret != Cddb_OK || trkp == NULL)
			continue;

		(void) cdinfo_put_str("  CddbTrack_PutArtist",
			CddbTrack_PutArtist, trkp,
			cdinfo_dbp->track[i].artist
		);
		(void) cdinfo_put_str("  CddbTrack_PutTitle",
			CddbTrack_PutTitle, trkp,
			cdinfo_dbp->track[i].title
		);
		(void) cdinfo_put_str("  CddbTrack_PutTitleSort",
			CddbTrack_PutTitleSort, trkp,
			cdinfo_dbp->track[i].sorttitle
		);
		(void) cdinfo_put_str("  CddbTrack_PutTitleThe",
			CddbTrack_PutTitleThe, trkp,
			cdinfo_dbp->track[i].title_the
		);
		(void) cdinfo_put_str("  CddbTrack_PutYear",
			CddbTrack_PutYear, trkp,
			cdinfo_dbp->track[i].year
		);
		(void) cdinfo_put_str("  CddbTrack_PutLabel",
			CddbTrack_PutLabel, trkp,
			cdinfo_dbp->track[i].label
		);
		(void) cdinfo_put_str("  CddbTrack_PutGenreId",
			CddbTrack_PutGenreId, trkp,
			cdinfo_dbp->track[i].genre
		);
		(void) cdinfo_put_str("  CddbTrack_PutSecondaryGenreId",
			CddbTrack_PutSecondaryGenreId, trkp,
			cdinfo_dbp->track[i].genre2
		);
		(void) cdinfo_put_str("  CddbTrack_PutBeatsPerMinute",
			CddbTrack_PutBeatsPerMinute, trkp,
			cdinfo_dbp->track[i].bpm
		);
		(void) cdinfo_put_str("  CddbTrack_PutNotes",
			CddbTrack_PutNotes, trkp,
			cdinfo_dbp->track[i].notes
		);
		(void) cdinfo_put_str("  CddbTrack_PutISRC",
			CddbTrack_PutISRC, trkp,
			cdinfo_dbp->track[i].isrc
		);

		(void) cdinfo_put_fullname("  CddbTrack_PutArtistFullName",
			CddbTrack_PutArtistFullName, trkp,
			cdinfo_dbp->track[i].artistfname.dispname,
			cdinfo_dbp->track[i].artistfname.lastname,
			cdinfo_dbp->track[i].artistfname.firstname,
			cdinfo_dbp->track[i].artistfname.the
		);

		for (p = cdinfo_dbp->track[i].credit_list; p != NULL;
		     p = p->next) {
			DBGPRN(DBG_CDI)(errfp, "  CddbTrack_AddCredit: ");
			ret = CddbTrack_AddCredit(trkp,
					p->crinfo.role->id,
					p->crinfo.name,
					&credp);
			DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

			if (ret != Cddb_OK || credp == NULL)
				continue;

			(void) cdinfo_put_fullname(
				"  CddbCredit_PutFullName",
				CddbCredit_PutFullName, credp,
				p->crinfo.fullname.dispname,
				p->crinfo.fullname.lastname,
				p->crinfo.fullname.firstname,
				p->crinfo.fullname.the
			);

			(void) cdinfo_put_str("  CddbCredit_PutNotes",
				CddbCredit_PutNotes, credp,
				p->notes
			);

			CddbReleaseObject(credp);
		}

		CddbReleaseObject(trkp);
	}

	/* Submit the disc info */
	DBGPRN(DBG_CDI)(errfp, "CddbControl_SubmitDisc: ");
	ret = CddbControl_SubmitDisc(cp->ctrlp, discp, 0, &pval);
	DBGPRN(DBG_CDI)(errfp, "0x%lx pval=%lx\n", ret, pval);

	cdinfo_set_errstr(ret);

	CddbReleaseObject(discp);

	*retcode = (ret == Cddb_OK) ? 0 : SUBMIT_ERR;
	util_delayms(1000);
	return (ret == Cddb_OK);
}


/*
 * cdinfo_submiturlcddb
 *	Submit to CDDB a URL pertaining to the current CD
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	     cdinfo_opencddb.
 *	up - Pointer to the cdinfo_url_t structure
 *	retcode - Return pointer of status code
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_submiturlcddb(cdinfo_cddb_t *cp, cdinfo_url_t *up,
		     cdinfo_ret_t *retcode)
{
	CddbDiscPtr		discp = NULL;
	CddbDiscsPtr		discsp = NULL;
	CddbURLManagerPtr	urlmgrp = NULL;
	CddbURLPtr		urlp = NULL;
	CddbResult		ret;

	if (!cdinfo_initcddb(cp, retcode))
		return FALSE;

	if ((cdinfo_dbp->flags & CDINFO_NEEDREG) != 0)
		return TRUE;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetDiscInfo: ");
	ret = CddbControl_GetDiscInfo(
		cp->ctrlp,
		cdinfo_dbp->disc.mediaid,
		cdinfo_dbp->disc.muiid,
		NULL, NULL, 0,
		&discp, &discsp
	);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || discp == NULL) {
		if (discp != NULL)
			CddbReleaseObject(discp);
		if (discsp != NULL)
			CddbReleaseObject(discsp);
		*retcode = SUBMIT_ERR;
		return FALSE;
	}

	if (discsp != NULL)
		CddbReleaseObject(discsp);

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetURLManager: ");
	ret = CddbControl_GetURLManager(cp->ctrlp, &urlmgrp); 
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || urlmgrp == NULL) {
		CddbReleaseObject(discp);
		*retcode = SUBMIT_ERR;
		return FALSE;
	}

	urlp = (CddbURLPtr) CddbCreateObject(CddbURLType);

	(void) cdinfo_put_str("CddbURL_PutType",
		CddbURL_PutType, urlp, up->type
	);
	(void) cdinfo_put_str("CddbURL_PutHref",
		CddbURL_PutHref, urlp, up->href
	);
	(void) cdinfo_put_str("CddbURL_PutDisplayLink",
		CddbURL_PutDisplayLink, urlp, up->displink
	);
	(void) cdinfo_put_str("CddbURL_PutDisplayText",
		CddbURL_PutDisplayText, urlp, up->disptext
	);
	(void) cdinfo_put_str("CddbURL_PutCategory",
		CddbURL_PutCategory, urlp, up->categ
	);
	(void) cdinfo_put_str("CddbURL_PutDescription",
		CddbURL_PutDescription, urlp, up->desc
	);
	(void) cdinfo_put_str("CddbURL_PutSize",
		CddbURL_PutSize, urlp, up->size
	);
	(void) cdinfo_put_str("CddbURL_PutWeight",
		CddbURL_PutWeight, urlp, up->weight
	);

	DBGPRN(DBG_CDI)(errfp, "CddbURLManager_SubmitURL: ");
	ret = CddbURLManager_SubmitURL(urlmgrp, discp, urlp); 
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	CddbReleaseObject(urlp);
	CddbReleaseObject(urlmgrp);
	CddbReleaseObject(discp);

	util_delayms(1000);
	return (ret == Cddb_OK);
}


/*
 * cdinfo_flushcddb
 *	Flush local CDDB cache.
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	cdinfo_opencddb.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
/*ARGSUSED*/
bool_t
cdinfo_flushcddb(cdinfo_cddb_t *cp)
{
	CddbResult	ret;
	
	DBGPRN(DBG_CDI)(errfp, "CddbControl_FlushLocalCache: ");
	ret = CddbControl_FlushLocalCache(cp->ctrlp, FLUSH_DEFAULT);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	util_delayms(1000);
	return (ret == Cddb_OK ? TRUE : FALSE);
}


/*
 * cdinfo_infobrowsercddb
 *	Invoke CDDB info browser for the current disc
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	cdinfo_opencddb.
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_infobrowsercddb(cdinfo_cddb_t *cp)
{
	CddbDiscPtr	discp = NULL;
	CddbDiscsPtr	discsp = NULL;
	CddbResult	ret;
	int		retcode;

	if (!cdinfo_initcddb(cp, &retcode))
		return FALSE;

	if ((cdinfo_dbp->flags & CDINFO_NEEDREG) != 0)
		return TRUE;

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetDiscInfo: ");
	ret = CddbControl_GetDiscInfo(
		cp->ctrlp,
		cdinfo_dbp->disc.mediaid,
		cdinfo_dbp->disc.muiid,
		NULL, NULL, 0,
		&discp, &discsp
	);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || discp == NULL) {
		if (discp != NULL)
			CddbReleaseObject(discp);
		if (discsp != NULL)
			CddbReleaseObject(discsp);
		return FALSE;
	}

	if (discsp != NULL)
		CddbReleaseObject(discsp);

	DBGPRN(DBG_CDI)(errfp, "CddbControl_InvokeInfoBrowser: ");
	ret = CddbControl_InvokeInfoBrowser(cp->ctrlp, discp, 0, UI_NONE);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	CddbReleaseObject(discp);

	return (ret == Cddb_OK);
}


/*
 * cdinfo_urlcddb
 *	Go to a CDDB-supplied URL
 *
 * Args:
 *	cp - Pointer to the cdinfo_cddb_t structure returned from
 *	cdinfo_opencddb.
 *	wtype - WTYPE_GEN or WTYPE_ALBUM
 *	idx - index number into the URL list
 *
 * Return:
 *	TRUE - Success
 *	FALSE - Failure
 */
bool_t
cdinfo_urlcddb(cdinfo_cddb_t *cp, int wtype, int idx)
{
	CddbDiscPtr		discp = NULL;
	CddbDiscsPtr		discsp = NULL;
	CddbURLManagerPtr	urlmgrp = NULL;
	CddbURLListPtr		urllistp = NULL;
	CddbURLPtr		urlp = NULL;
	CddbResult		ret;
	int			retcode;

	if (!cdinfo_initcddb(cp, &retcode))
		return FALSE;

	if ((cdinfo_dbp->flags & CDINFO_NEEDREG) != 0)
		return TRUE;

	if (wtype == WTYPE_ALBUM) {
		DBGPRN(DBG_CDI)(errfp, "CddbControl_GetDiscInfo: ");
		ret = CddbControl_GetDiscInfo(
			cp->ctrlp,
			cdinfo_dbp->disc.mediaid,
			cdinfo_dbp->disc.muiid,
			NULL, NULL, 0,
			&discp, &discsp
		);
		DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

		cdinfo_set_errstr(ret);

		if (ret != Cddb_OK || discp == NULL) {
			if (discp != NULL)
				CddbReleaseObject(discp);
			if (discsp != NULL)
				CddbReleaseObject(discsp);
			return FALSE;
		}
	}

	if (discsp != NULL)
		CddbReleaseObject(discsp);

	DBGPRN(DBG_CDI)(errfp, "CddbControl_GetURLManager: ");
	ret = CddbControl_GetURLManager(cp->ctrlp, &urlmgrp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || urlmgrp == NULL) {
		if (discp != NULL)
			CddbReleaseObject(discp);
		return FALSE;
	}

	/* Get the appropriate URL list */
	DBGPRN(DBG_CDI)(errfp, "CddbURLManager_GetMenuURLs: ");
	ret = CddbURLManager_GetMenuURLs(urlmgrp, discp, &urllistp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || urllistp == NULL) {
		CddbReleaseObject(urlmgrp);
		if (discp != NULL)
			CddbReleaseObject(discp);
		return FALSE;
	}

	/* Get the appropriate URL */
	DBGPRN(DBG_CDI)(errfp, "CddbURLList_GetURL: ");
	ret = CddbURLList_GetURL(urllistp, (long) idx, &urlp);
	DBGPRN(DBG_CDI)(errfp, "0x%lx idx=%d\n", ret, idx);

	cdinfo_set_errstr(ret);

	if (ret != Cddb_OK || urlp == NULL) {
		CddbReleaseObject(urllistp);
		CddbReleaseObject(urlmgrp);
		if (discp != NULL)
			CddbReleaseObject(discp);
		return FALSE;
	}

	/* Go to the URL */
	DBGPRN(DBG_CDI)(errfp, "CddbURLManager_GotoURL: ");
	ret = CddbURLManager_GotoURL(urlmgrp, urlp, 0);
	DBGPRN(DBG_CDI)(errfp, "0x%lx\n", ret);

	cdinfo_set_errstr(ret);

	CddbReleaseObject(urlp);
	CddbReleaseObject(urllistp);
	CddbReleaseObject(urlmgrp);
	if (discp != NULL)
		CddbReleaseObject(discp);

	return (ret == Cddb_OK);
}


/*
 * cdinfo_free_matchlist
 *	Deallocate the match list.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_free_matchlist(void)
{
	cdinfo_match_t	*p,
			*q;

	for (p = q = cdinfo_dbp->matchlist; p != NULL; p = q) {
		q = p->next;
		if (p->artist != NULL)
			MEM_FREE(p->artist);
		if (p->title != NULL)
			MEM_FREE(p->title);
		if (p->genre != NULL)
			MEM_FREE(p->genre);
		MEM_FREE(p);
	}

	cdinfo_dbp->matchlist = NULL;
	cdinfo_dbp->match_aux = NULL;
	cdinfo_dbp->match_tag = -1;
	cdinfo_dbp->sav_cddbp = NULL;
	cdinfo_dbp->sav_rpp = cdinfo_dbp->sav_spp = NULL;
}


/*
 * cdinfo_skip_whitespace
 *	Given a string, return a pointer to the first non-whitespace
 *	character in it.
 *
 * Args:
 *	str - String pointer
 *
 * Return:
 *	Pointer to first non-whitespace character in the string
 */
char *
cdinfo_skip_whitespace(char *str)
{
	for (; *str != '\0' && (*str == ' ' || *str == '\t'); str++)
		;

	return (str);
}


/*
 * cdinfo_skip_nowhitespace
 *	Given a string, return a pointer to the first whitespace
 *	character in it.
 *
 * Args:
 *	str - String pointer
 *
 * Return:
 *	Pointer to first whitespace character in the string
 */
char *
cdinfo_skip_nowhitespace(char *str)
{
	for (; *str != '\0' && (*str != ' ' && *str != '\t'); str++)
		;

	return (str);
}


/*
 * cdinfo_fgetline
 *	Read a line from the file stream fp, and allocate a dynamic
 *	buffer to hold it.  If a line is terminated with a '\' character,
 *	that is considered a "continuation" marker, and another line
 *	is read and contactenated.  The caller should use MEM_FREE to
 *	deallocate the buffer that is returned when it's done using it.
 *
 * Args:
 *	fp - The stdio file stream
 *
 * Return:
 *	Allocated buffer containing the line read from stream.
 */
char *
cdinfo_fgetline(FILE *fp)
{
	size_t		n;
	char		*buf;
	static char	rbuf[2048];

	buf = NULL;
	for (;;) {
		if (fgets(rbuf, sizeof(rbuf), fp) == NULL)
			break;

		if ((n = strlen(rbuf)) == 0)
			break;

		if (buf == NULL) {
			buf = (char *) MEM_ALLOC("fgetline_buf", n + 1);
			if (buf != NULL)
				buf[0] = '\0';
		}
		else {
			buf = (char *) MEM_REALLOC("fgetline_buf",
				buf, strlen(buf) + n + 1
			);
		}

		if (buf == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return NULL;
		}

		if (rbuf[n-1] == '\n') {
			if (n > 1 && rbuf[n-2] == '\\') {
				rbuf[n-2] = '\0';
				(void) strcat(buf, rbuf);
			}
			else {
				rbuf[n-1] = '\0';
				(void) strcat(buf, rbuf);
				break;
			}
		}
		else
			(void) strcat(buf, rbuf);
	}

	return (buf);
}


/*
 * cdinfo_wwwchk_cleanup
 *	Clean up the check list created in cdinfo_wwwmenu_chk.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_wwwchk_cleanup(void)
{
	w_entchk_t	*c,
			*c1;

	/* Clean up check list */
	for (c = c1 = cdinfo_wentchk_head; c != NULL; c = c1) {
		c1 = c->next;
		MEM_FREE(c);
	}
	cdinfo_wentchk_head = NULL;
}


/*
 * cdinfo_wwwmenu_chk
 *	Given a wwwwarp menu, traverse all its submenus and make sure
 *	there are no circular menu links.
 *
 * Args:
 *	menu - The first entry of a menu
 *	topmenu - boolean indicating whether this is called from the
 *		  top level menu.
 */
bool_t
cdinfo_wwwmenu_chk(w_ent_t *menu, bool_t topmenu)
{
	bool_t			ret;
	w_ent_t			*p;
	w_entchk_t		*c,
				*c1;
	static w_entchk_t	*c2;

	ret = TRUE;

	for (p = menu; p != NULL; p = p->nextent) {
		if (p->type != WTYPE_SUBMENU)
			continue;

		/* Check to see if this submenu points to an ancester menu */
		for (c1 = cdinfo_wentchk_head; c1 != NULL; c1 = c1->next) {
			if (strcmp(c1->ent->name, p->submenu->name) == 0) {
				p->type = WTYPE_NULL;
				p->submenu = NULL;
				ret = FALSE;
				break;
			}
		}
		if (!ret)
			break;

		/* Add this submenu to link */
		c = (w_entchk_t *)(void *) MEM_ALLOC(
			"w_entchk_t", sizeof(w_entchk_t)
		);
		if (c == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			ret = FALSE;
			break;
		}
		c->ent = p->submenu;
		c->next = NULL;

		if (cdinfo_wentchk_head == NULL)
			cdinfo_wentchk_head = c2 = c;
		else {
			c2->next = c;
			c2 = c;
		}

		/* Recurse to the next level */
		if (!cdinfo_wwwmenu_chk(p->submenu, FALSE)) {
			ret = FALSE;
			break;
		}

		if (topmenu)
			cdinfo_wwwchk_cleanup();
	}

	if (topmenu)
		cdinfo_wwwchk_cleanup();
	return (ret);
}


/*
 * cdinfo_scan_url_attrib
 *	Scan a URL template string and record its attributes.
 *
 * Args:
 *	url - The URL template string
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_scan_url_attrib(char *url, url_attrib_t *up)
{
	char	*cp;

	/* Scan URL string to get a count of the number of
	 * substitutions needed.
	 */
	up->xcnt = up->vcnt = 0;
	up->ncnt = up->hcnt = 0;
	up->lcnt = up->ccnt = 0;
	up->icnt = up->acnt = 0;
	up->dcnt = up->tcnt = 0;
	up->rcnt = up->pcnt = 0;
	for (cp = url; *cp != '\0'; cp++) {
		if (*cp == '%') {
			switch ((int) *(++cp)) {
			case 'X':
				up->xcnt++;
				break;
			case 'V':
				up->vcnt++;
				break;
			case 'N':
				up->ncnt++;
				break;
			case 'H':
				up->hcnt++;
				break;
			case 'L':
				up->lcnt++;
				break;
			case 'C':
				up->ccnt++;
				break;
			case 'I':
				up->icnt++;
				break;
			case 'A':
			case 'a':
				up->acnt++;
				break;
			case 'D':
			case 'd':
				up->dcnt++;
				break;
			case 'R':
			case 'r':
				up->rcnt++;
				break;
			case 'T':
			case 't':
				up->tcnt++;
				break;
			case 'B':
			case 'b':
				up->acnt++;
				up->dcnt++;
				break;
			case '#':
				up->pcnt++;
				break;
			default:
				break;
			}
		}
	}
}


/*
 * cdinfo_add_pathent
 *	Add a cdinfo path list component.
 *
 * Args:
 *	path - The path component string
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
cdinfo_add_pathent(char *path)
{
	int			n;
	char			*cp,
				*cp2;
	cdinfo_path_t		*pp;
	STATIC cdinfo_path_t	*pp2 = NULL;

	if (*path == '\0' || *path == '\n')
		return TRUE;
	if (strncmp(path, "cddbp://", 8) == 0 ||
	    strncmp(path, "http://", 7) == 0) {
		(void) fprintf(errfp,
			"NOTICE: Skipped unsupported item in cdinfoPath: %s\n",
			path);
		return TRUE;
	}

	pp = (cdinfo_path_t *)(void *) MEM_ALLOC(
		"cdinfo_path_t",
		sizeof(cdinfo_path_t)
	);
	if (pp == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) memset(pp, 0, sizeof(cdinfo_path_t));

	if (cdinfo_dbp->pathlist == NULL) {
		cdinfo_dbp->pathlist = pp2 = pp;
	}
	else {
		pp2->next = pp;
		pp2 = pp;
	}

	/* Determine path type */
	if (strcmp(path, "CDDB") == 0) {
		/* CDDB service */
		pp->type = CDINFO_RMT;
	}
	else if (strcmp(path, "CDTEXT") == 0) {
		/* CD-TEXT from the disc */
		pp->type = CDINFO_CDTEXT;
	}
	else if (strncmp(path, "file://", 7) == 0) {
		/* Syntax: file://dirpath */
		pp->type = CDINFO_LOC;
		path += 7;
	}
	else {
		/* Syntax: dirpath */
		pp->type = CDINFO_LOC;
	}

	/* Parse the rest of the line */
	switch (pp->type) {
	case CDINFO_RMT:
	case CDINFO_CDTEXT:
		/* Do nothing here */
		break;

	case CDINFO_LOC:
		n = strlen(path);
		if (path[0] == '/') {
			/* Absolute local path name */
			if (!util_newstr(&pp->path, path)) {
				CDINFO_FATAL(app_data.str_nomemory);
				return FALSE;
			}
		}
		else if (path[0] == '~') {
			/* Perform tilde expansion a la [ck]sh */
			if (path[1] == '/') {
				cp2 = util_homedir(util_get_ouid());

				pp->path = (char *) MEM_ALLOC(
					"pp->path",
					n + strlen(cp2)
				);
				if (pp->path == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return FALSE;
				}

				(void) sprintf(pp->path, "%s%s",
					       cp2, &path[1]);
			}
			else if (path[1] == '\0') {
				cp2 = util_homedir(util_get_ouid());

				if (!util_newstr(&pp->path, cp2)) {
					CDINFO_FATAL(app_data.str_nomemory);
					return FALSE;
				}
			}
			else {
				cp = strchr(path, '/');
				if (cp == NULL) {
					cp2 = util_uhomedir(&path[1]);
					if (!util_newstr(&pp->path, cp2)) {
						CDINFO_FATAL(
							app_data.str_nomemory
						);
						return FALSE;
					}
				}
				else {
					*cp = '\0';
					cp2 = util_uhomedir(&path[1]);
					pp->path = (char *) MEM_ALLOC(
						"pp->path",
						n + strlen(cp2)
					);
					if (pp->path == NULL) {
						CDINFO_FATAL(
							app_data.str_nomemory
						);
						return FALSE;
					}

					(void) sprintf(pp->path, CONCAT_PATH,
						       cp2, cp+1);
				}
			}
		}
		else {
			/* Relative local path name */
			pp->path = (char *) MEM_ALLOC(
				"pp->path",
				n + strlen(app_data.libdir) +
					strlen(REL_DBDIR_PATH)
			);
			if (pp->path == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return FALSE;
			}

			(void) sprintf(pp->path, REL_DBDIR_PATH,
				       app_data.libdir, path);

		}

		/* Set category name for CDDB1 to CDDB2 genre mapping */
		cp = util_basename(pp->path);
		if (cp != NULL && !util_newstr(&pp->categ, cp)) {
			MEM_FREE(cp);
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		MEM_FREE(cp);

		/* Make sure that the whole path name + discid can
		 * fit in a FILE_PATH_SZ buffer.  Also, make sure the
		 * category name is less than FILE_BASE_SZ.
		 */
		if ((int) strlen(pp->path) >= (FILE_PATH_SZ - 12)) {
			CDINFO_FATAL(app_data.str_longpatherr);
			return FALSE;
		}

		if ((cp = util_basename(pp->path)) == NULL) {
			CDINFO_FATAL("cdinfoPath component error");
			return FALSE;
		}

		if ((int) strlen(cp) >= FILE_BASE_SZ) {
			MEM_FREE(cp);
			CDINFO_FATAL(app_data.str_longpatherr);
			return FALSE;
		}
		MEM_FREE(cp);

		break;
	}

	return TRUE;
}


/*
 * cdinfo_load_locdb
 *	Attempt to load CD information from the specified local CD info file.
 *
 * Args:
 *	path - File path
 *	categ - CDDB1 category string
 *	s - Pointer to the curstat_t structure
 *	retcode - Return status code
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
/*ARGSUSED*/
bool_t
cdinfo_load_locdb(char *path, char *categ, curstat_t *s, int *retcode)
{
	int	i,
		pos,
		bufsz = STR_BUF_SZ * 3;
	char	*buf,
		*tmpbuf;
	FILE	*fp;

	errno = 0;
	*retcode = 0;

	if ((fp = fopen(path, "r")) == NULL) {
		/* File not found */
		*retcode = 0;
		DBGPRN(DBG_CDI)(errfp, "\n");
		return TRUE;
	}

	if ((buf = (char *) MEM_ALLOC("read_buf", bufsz)) == NULL) {
		*retcode = MEM_ERR;
		DBGPRN(DBG_CDI)(errfp, "\n");
		(void) fclose(fp);
		CDINFO_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	if ((tmpbuf = (char *) MEM_ALLOC("read_tmpbuf", bufsz)) == NULL) {
		*retcode = MEM_ERR;
		DBGPRN(DBG_CDI)(errfp, "\n");
		MEM_FREE(buf);
		(void) fclose(fp);
		CDINFO_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	/* Read first line of database entry */
	if (fgets(buf, bufsz, fp) == NULL) {
		/* Can't read file */
		DBGPRN(DBG_CDI)(errfp, "\n");
		MEM_FREE(buf);
		MEM_FREE(tmpbuf);
		(void) fclose(fp);
		return TRUE;
	}

	/* Database file signature check */
	if (strncmp(buf, "# xmcd", 6) != 0) {
		/* Not a supported database file */
		DBGPRN(DBG_CDI)(errfp, "\n");
		MEM_FREE(buf);
		MEM_FREE(tmpbuf);
		(void) fclose(fp);
		return TRUE;
	}

	/* Read the rest of the database entry */
	while (fgets(buf, bufsz, fp) != NULL) {
		/* Comment line */
		if (buf[0] == '#') {
			/* Concatenated cdinfo file */
			if (strncmp(buf, "# xmcd", 6) == 0)
				break;

			continue;
		}

		buf[strlen(buf)-1] = '\n';

		/* Disc IDs */
		if (sscanf(buf, "DISCID=%[^\n]\n", tmpbuf) > 0) {
			/* Do nothing for this line */
			continue;
		}

		/* Disk title */
		if (sscanf(buf, "DTITLE=%[^\n]\n", tmpbuf) > 0) {
			cdinfo_disc_t	*dp;
			char		*cp;

			cdinfo_line_filter(tmpbuf);

			dp = &cdinfo_dbp->disc;

			/* This assumes that artist and title are separated
			 * " / " (may not be true in all cases; shrug)
			 */
			if ((cp = strchr(tmpbuf, '/')) != NULL &&
			     cp > (tmpbuf + 3) &&
			     *(cp-1) == ' ' && *(cp+1) == ' ') {
				char	*cp2;

				cp2 = cp - 1;
				*cp2 = '\0';
				if (!cdinfo_concatstr(
						    &dp->artistfname.dispname,
						    tmpbuf)) {
					CDINFO_FATAL(app_data.str_nomemory);
					break;
				}
				if (!cdinfo_concatstr(&dp->artist,
						      tmpbuf)) {
					CDINFO_FATAL(app_data.str_nomemory);
					break;
				}

				cp2 = cp + 2;
				if (!cdinfo_concatstr(&dp->title, cp2)) {
					CDINFO_FATAL(app_data.str_nomemory);
					break;
				}
			}
			else if (!cdinfo_concatstr(&dp->title, tmpbuf)) {
				CDINFO_FATAL(app_data.str_nomemory);
				break;
			}
			continue;
		}

		/* Track titles */
		if (sscanf(buf, "TTITLE%u=%[^\n]\n", &pos, tmpbuf) >= 2) {
			cdinfo_track_t	*tp;

			if (pos >= (int) (cdinfo_dbp->discid & 0xff))
				continue;

			cdinfo_line_filter(tmpbuf);

			tp = &cdinfo_dbp->track[pos];

			if (!cdinfo_concatstr(&tp->title, tmpbuf)) {
				CDINFO_FATAL(app_data.str_nomemory);
				break;
			}
			continue;
		}

		/* Disk notes */
		if (sscanf(buf, "EXTD=%[^\n]\n", tmpbuf) > 0) {
			cdinfo_disc_t	*dp;

			dp = &cdinfo_dbp->disc;

			if (!cdinfo_concatstr(&dp->notes, tmpbuf)) {
				CDINFO_FATAL(app_data.str_nomemory);
				break;
			}
			continue;
		}

		/* Track extended info */
		if (sscanf(buf, "EXTT%u=%[^\n]\n", &pos, tmpbuf) >= 2) {
			cdinfo_track_t	*tp;

			if (pos >= (int) (cdinfo_dbp->discid & 0xff))
				continue;

			tp = &cdinfo_dbp->track[pos];

			if (!cdinfo_concatstr(&tp->notes, tmpbuf)) {
				CDINFO_FATAL(app_data.str_nomemory);
				break;
			}
			continue;
		}

#ifdef USE_XMCD2_PLAYORDER
		/* Play order - this is deprecated */
		if (sscanf(buf, "PLAYORDER=%[^\n]\n", tmpbuf) > 0) {
			if (s->program || s->shuffle)
				/* Play program or shuffle already in
				 * progress, do not override it.
				 */
				continue;

			if (!cdinfo_concatstr(&cdinfo_dbp->playorder, tmpbuf)){
				CDINFO_FATAL(app_data.str_nomemory);
				break;
			}
			continue;
		}
#endif
	}

	MEM_FREE(buf);
	MEM_FREE(tmpbuf);

	(void) fclose(fp);

	/* Do CDDB1 -> CDDB2 genre mapping if possible */
	for (i = 0; cdinfo_genre_map[i].cddb1_genre != NULL; i++) {
		if (categ != NULL &&
		    strcmp(categ, cdinfo_genre_map[i].cddb1_genre) == 0) {
			if (!util_newstr(&cdinfo_dbp->disc.genre,
					 cdinfo_genre_map[i].cddb2_genre)) {
				CDINFO_FATAL(app_data.str_nomemory);
			}
			break;
		}
	}

	/* Set the match bit */
	cdinfo_dbp->flags |= (CDINFO_MATCH | CDINFO_FROMLOC);

	DBGPRN(DBG_CDI)(errfp, ": Loaded.\n");
	return TRUE;
}


/*
 * cdinfo_out_discog
 *	Output local discography HTML content for the currently loaded CD
 *
 * Args:
 *	path - File path to the local discography file
 *	s - Pointer to the curstat_t structure
 *	baseurl - The URL to the output file
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
cdinfo_out_discog(char *path, curstat_t *s, char *baseurl)
{
	int		i,
			fmt,
			ncreds,
			nrows,
			ntrkrows;
	unsigned int	dmode,
			fmode;
	char		*p,
			*q,
			*tblparms1,
			*tblparms2,
			*tdparms1,
			*tdparms2,
			*tdparms3,
			*srchact,
			*gname,
			*gpath,
			*dhome,
			*relpath,
			*cmd,
			outdir[FILE_PATH_SZ],
			filepath[FILE_PATH_SZ + 40],
			plspath[FILE_PATH_SZ + 480];
	FILE		*fp,
			*pls_fp;
	DIR		*dp;
	struct dirent	*de;
	cdinfo_credit_t	*cp;
	playls_t	*lheads[MAX_FILEFMTS],
			*sp;
	filefmt_t	*fmp;
	struct stat	stbuf;
	bool_t		first,
			newdiscog;

	(void) sscanf(app_data.cdinfo_filemode, "%o", &fmode);
	/* Make sure file is at least accessible by user */
	fmode |= S_IRUSR | S_IWUSR;
	fmode &= ~(S_ISUID | S_ISGID);

	/* Set directory perm based on file perm */
	dmode = (fmode | S_IXUSR);
	if (fmode & S_IRGRP)
		dmode |= (S_IRGRP | S_IXGRP);
	if (fmode & S_IWGRP)
		dmode |= (S_IWGRP | S_IXGRP);
	if (fmode & S_IROTH)
		dmode |= (S_IROTH | S_IXOTH);
	if (fmode & S_IWOTH)
		dmode |= (S_IWOTH | S_IXOTH);

	if ((p = util_dirname(path)) == NULL) {
		DBGPRN(DBG_CDI)(errfp, "Directory path error: %s\n", path);
		return FALSE;
	}
	(void) strcpy(outdir, p);
	MEM_FREE(p);

	newdiscog = FALSE;
	if (util_dirstat(outdir, &stbuf, FALSE) < 0) {
		if (errno == ENOENT)
			newdiscog = TRUE;
		else {
			DBGPRN(DBG_CDI)(errfp, "Cannot stat %s\n", outdir);
			return FALSE;
		}
	}

	/* Make directories and fix perms */
	if (!util_mkdir(outdir, (mode_t) dmode)) {
		DBGPRN(DBG_CDI)(errfp, "Cannot create directory %s\n", outdir);
		return FALSE;
	}

	/* Remove original file */
	if (UNLINK(path) < 0 && errno != ENOENT) {
		DBGPRN(DBG_CDI)(errfp, "Cannot unlink old %s\n", path);
		return FALSE;
	}

	/* Write new file */
	if ((fp = fopen(path, "w")) == NULL) {
		DBGPRN(DBG_CDI)(errfp,
			"Cannot open file for writing: %s\n", path);
		return FALSE;
	}
	(void) chmod(path, (mode_t) fmode);

	DBGPRN(DBG_CDI)(errfp, "\nWriting local Discography: %s\n", path);

	/* Set up the CDDB search URL string */
	srchact = NULL;
	if (cdinfo_scddb != NULL && cdinfo_scddb->arg != NULL) {
		if (!util_newstr(&srchact, cdinfo_scddb->arg)) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		if ((p = strrchr(srchact, '%')) != NULL)
			*p = '\0';
		else {
			MEM_FREE(srchact);
			srchact = NULL;
		}
	}

	/* Get genre name and path */
	gname = cdinfo_genre_name(cdinfo_dbp->disc.genre);
	if (util_strstr(gname, " -> ") != NULL)
		dhome = "../../..";
	else
		dhome = "../..";

	/* Start HTML output */
	tblparms1 = "CELLSPACING=\"0\" CELLPADDING=\"1\" BORDER=\"0\"";
	tblparms2 = "CELLSPACING=\"0\" CELLPADDING=\"3\" BORDER=\"1\"";
	tdparms1 = "ALIGN=\"center\"";
	tdparms2 = "ALIGN=\"left\"";
	tdparms3 = "ALIGN=\"left\" VALIGN=\"top\"";

	/* Comments */
	(void) fprintf(fp, "<!-- xmcd Local Discography\n");
	(void) fprintf(fp, "     DO NOT EDIT: Generated by %s %s.%s.%s\n",
		cdinfo_clinfo->prog, VERSION_MAJ, VERSION_MIN, VERSION_TEENY);
	(void) fprintf(fp, "     %s\n     URL: %s E-mail: %s -->\n",
		COPYRIGHT, XMCD_URL, EMAIL);
	(void) fputs("<!-- tItLe: ", fp);

	/* Album artist and title info in comments, for sorting by genidx */
	if (cdinfo_dbp->disc.artistfname.lastname != NULL ||
	    cdinfo_dbp->disc.artistfname.firstname != NULL) {
		/* Use sorted artist name if possible */
		if (cdinfo_dbp->disc.artistfname.lastname != NULL) {
			util_html_fputs(cdinfo_dbp->disc.artistfname.lastname,
					fp, FALSE, NULL, 0);
			(void) fputs(", ", fp);
		}
		if (cdinfo_dbp->disc.artistfname.firstname != NULL) {
			util_html_fputs(cdinfo_dbp->disc.artistfname.firstname,
					fp, FALSE, NULL, 0);
		}
		if (cdinfo_dbp->disc.artistfname.the != NULL) {
			(void) fputs(", ", fp);
			util_html_fputs(cdinfo_dbp->disc.artistfname.the,
					fp, FALSE, NULL, 0);
		}
	}
	else if (cdinfo_dbp->disc.artist != NULL) {
		/* Use display name */
		util_html_fputs(cdinfo_dbp->disc.artist, fp, FALSE, NULL, 0);
	}
	else {
		(void) fputs(app_data.str_unknartist, fp);
	}

	if (cdinfo_dbp->disc.sorttitle != NULL) {
		/* Use sort title if possible */
		(void) fputs(" / ", fp);
		util_html_fputs(cdinfo_dbp->disc.sorttitle,
				fp, FALSE, NULL, 0);
		if (cdinfo_dbp->disc.title_the != NULL) {
			(void) fputs(", ", fp);
			util_html_fputs(cdinfo_dbp->disc.title_the,
					fp, FALSE, NULL, 0);
		}
	}
	else if (cdinfo_dbp->disc.title != NULL) {
		/* Use display title */
		(void) fputs(" / ", fp);
		util_html_fputs(cdinfo_dbp->disc.title, fp, FALSE, NULL, 0);
	}
	else {
		(void) fputs(" / ", fp);
		(void) fputs(app_data.str_unkndisc, fp);
	}
	(void) fputs(" -->\n", fp);

	(void) fputs("<HTML>\n<HEAD>\n", fp);

	/* All in-core CD information is encoded in UTF-8 */
	(void) fputs("<META HTTP-EQUIV=\"Content-type\" "
		     "CONTENT=\"text/html; charset=utf-8\">\n", fp);

	(void) fputs("<TITLE>\nxmcd: ", fp);

	if (cdinfo_dbp->disc.artist != NULL)
		util_html_fputs(cdinfo_dbp->disc.artist, fp, FALSE, NULL, 0);
	else
		(void) fputs(app_data.str_unknartist, fp);

	(void) fputs(" / ", fp);

	if (cdinfo_dbp->disc.title != NULL)
		util_html_fputs(cdinfo_dbp->disc.title, fp, FALSE, NULL, 0);
	else
		(void) fputs(app_data.str_unkndisc, fp);

	(void) fputs("\n</TITLE>\n", fp);

#ifdef __VMS
	(void) sprintf(filepath, "%s.discog]bkgnd.gif", app_data.libdir);
	if ((p = util_vms_urlconv(filepath, VMS_2_UNIX)) != NULL) {
		(void) strncpy(filepath, p, sizeof(filepath)-1);
		filepath[sizeof(filepath)-1] = '\0';
		MEM_FREE(p);
	}
#else
	(void) sprintf(filepath, "%s/bkgnd.gif", dhome);
#endif
	(void) fprintf(fp,
		"</HEAD>\n<BODY BGCOLOR=\"%s\" BACKGROUND=\"%s\">\n",
		"#FFFFFF",		/* Background color */
		filepath
	);

	(void) fputs("<DIV ALIGN=\"center\">\n", fp);

	/* xmcd logo */
	(void) fprintf(fp, "<A HREF=\"%s\">\n", XMCD_URL);
#ifdef __VMS
	(void) sprintf(filepath, "%s.discog]xmcdlogo.gif", app_data.libdir);
	if ((p = util_vms_urlconv(filepath, VMS_2_UNIX)) != NULL) {
		(void) strncpy(filepath, p, sizeof(filepath)-1);
		filepath[sizeof(filepath)-1] = '\0';
		MEM_FREE(p);
	}
#else
	(void) sprintf(filepath, "%s/xmcdlogo.gif", dhome);
#endif
	(void) fprintf(fp, "<IMG SRC=\"%s\" ALT=\"xmcd\" BORDER=\"0\">",
			filepath);
	(void) fprintf(fp, "</A><P>\n<H4>Local Discography</H4><P>\n");

	/* Disc artist / title */
	(void) fputs("<H3>\n", fp);
	if (cdinfo_dbp->disc.artist == NULL)
		(void) fprintf(fp, "(%s)", app_data.str_unknartist);
	else {
		if (srchact != NULL) {
			p = cdinfo_txtreduce(cdinfo_dbp->disc.artist, TRUE);
			(void) fprintf(fp, "<A HREF=\"%s%s\">", srchact, p);
			MEM_FREE(p);
		}
		util_html_fputs(cdinfo_dbp->disc.artist, fp, FALSE, NULL, 0);
		if (srchact != NULL)
			(void) fputs("</A>\n", fp);
	}

	(void) fputs(" / ", fp);

	if (cdinfo_dbp->disc.title == NULL)
		(void) fprintf(fp, "(%s)", app_data.str_unkndisc);
	else {
		if (srchact != NULL) {
			p = cdinfo_txtreduce(cdinfo_dbp->disc.title, TRUE);
			(void) fprintf(fp, "<A HREF=\"%s%s\">", srchact, p);
			MEM_FREE(p);
		}
		util_html_fputs(cdinfo_dbp->disc.title, fp, FALSE, NULL, 0);
		if (srchact != NULL)
			(void) fputs("</A>\n", fp);
	}
	(void) fputs("</H3>\n", fp);

	/* Album information */
	(void) fprintf(fp, "<P>\n<TABLE %s>\n", tblparms1);
	(void) fprintf(fp, "<TR><TH %s>Total time:</TH><TD %s>&nbsp;%02d:%02d"
		"</TD></TR>\n",
		tdparms2, tdparms2,
		s->discpos_tot.min, s->discpos_tot.sec
	);
	(void) fprintf(fp, "<TR><TH %s>Primary genre:</TH><TD %s>&nbsp;",
		tdparms2, tdparms2
	);
	util_html_fputs(gname, fp, FALSE, NULL, 0);
	(void) fputs("</TD></TR>\n", fp);
	(void) fprintf(fp, "<TR><TH %s>Secondary genre:</TH><TD %s>&nbsp;",
		tdparms2, tdparms2
	);
	util_html_fputs(cdinfo_genre_name(cdinfo_dbp->disc.genre2),
			fp, FALSE, NULL, 0);
	(void) fputs("</TD></TR>\n", fp);
	(void) fprintf(fp,
		"<TR><TH %s>Xmcd disc ID:</TH><TD %s>&nbsp;%08x</TD></TR>\n",
		tdparms2, tdparms2, cdinfo_dbp->discid
	);
	(void) fprintf(fp, "<TR><TH %s>Artist full name:</TH><TD %s>&nbsp;",
		tdparms2, tdparms2
	);
	if (cdinfo_dbp->disc.artistfname.lastname != NULL) {
		util_html_fputs(cdinfo_dbp->disc.artistfname.lastname,
				fp, FALSE, NULL, 0);
		(void) fputs(", ", fp);
	}
	if (cdinfo_dbp->disc.artistfname.firstname != NULL) {
		util_html_fputs(cdinfo_dbp->disc.artistfname.firstname,
				fp, FALSE, NULL, 0);
	}
	if (cdinfo_dbp->disc.artistfname.the != NULL) {
		(void) fputs(", ", fp);
		util_html_fputs(cdinfo_dbp->disc.artistfname.the,
				fp, FALSE, NULL, 0);
	}
	(void) fputs("</TD></TR>\n", fp);
	(void) fprintf(fp, "<TR><TH %s>Sort title:</TH><TD %s>&nbsp;",
		tdparms2, tdparms2
	);
	if (cdinfo_dbp->disc.sorttitle != NULL) {
		util_html_fputs(cdinfo_dbp->disc.sorttitle,
				fp, FALSE, NULL, 0);
	}
	if (cdinfo_dbp->disc.title_the != NULL) {
		(void) fputs(", ", fp);
		util_html_fputs(cdinfo_dbp->disc.title_the,
				fp, FALSE, NULL, 0);
	}
	(void) fputs("</TD></TR>\n", fp);
	(void) fprintf(fp, "<TR><TH %s>Year:</TH><TD %s>&nbsp;%s</TD></TR>\n",
		tdparms2, tdparms2,
		cdinfo_dbp->disc.year == NULL ? "" : cdinfo_dbp->disc.year
	);
	(void) fprintf(fp, "<TR><TH %s>Record label:</TH><TD %s>&nbsp;",
		tdparms2, tdparms2
	);
	if (cdinfo_dbp->disc.label != NULL)
		util_html_fputs(cdinfo_dbp->disc.label, fp, FALSE, NULL, 0);
	(void) fputs("</TD></TR>\n", fp);
	(void) fprintf(fp,
		"<TR><TH %s>Compilation:</TH><TD %s>&nbsp;%s</TD></TR>\n",
		tdparms2, tdparms2,
		cdinfo_dbp->disc.compilation ? "Yes" : "No"
	);
	(void) fprintf(fp,
		"<TR><TH %s>Disc:</TH><TD %s>&nbsp;%s of %s</TD></TR>\n",
		tdparms2, tdparms2,
		cdinfo_dbp->disc.dnum == NULL ? "?" : cdinfo_dbp->disc.dnum,
		cdinfo_dbp->disc.tnum == NULL ? "?" : cdinfo_dbp->disc.tnum
	);
	(void) fprintf(fp, "<TR><TH %s>Region:</TH><TD %s>&nbsp;",
		tdparms2, tdparms2
	);
	util_html_fputs(cdinfo_region_name(cdinfo_dbp->disc.region),
			fp, FALSE, NULL, 0);
	(void) fputs("</TD></TR>\n", fp);
	(void) fprintf(fp, "<TR><TH %s>Language:</TH><TD %s>&nbsp;",
		tdparms2, tdparms2
	);
	util_html_fputs(cdinfo_lang_name(cdinfo_dbp->disc.lang),
			fp, FALSE, NULL, 0);
	(void) fputs("</TD></TR>\n", fp);
	(void) fputs("</TABLE>\n", fp);

	/* Big info table */
	(void) fprintf(fp, "<P>\n<TABLE %s>\n", tblparms2);
	(void) fputs("<TR>\n", fp);
	(void) fprintf(fp, "<TH %s>Track</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Start</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Length</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Artist</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Title</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Pri genre</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Sec genre</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>BPM</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Year</TH>\n", tdparms1);
	(void) fprintf(fp, "<TH %s>Record label</TH>\n", tdparms1);
	(void) fputs("</TR>\n", fp);

	ntrkrows = 0;
	for (i = 0; i < (int) s->tot_trks; i++) {
		int	min,
			sec,
			secs;

		secs = ((s->trkinfo[i+1].min * 60 + s->trkinfo[i+1].sec) -
			(s->trkinfo[i].min * 60 + s->trkinfo[i].sec));
		min = (byte_t) (secs / 60);
		sec = (byte_t) (secs % 60);

		(void) fprintf(fp, "<TR>\n<TD %s>%d</TD>\n",
			tdparms1,
			s->trkinfo[i].trkno
		);
		(void) fprintf(fp, "<TD %s>%02d:%02d</TD>\n",
			tdparms1, s->trkinfo[i].min, s->trkinfo[i].sec);
		(void) fprintf(fp, "<TD %s>%02d:%02d</TD>\n",
			tdparms1, min, sec);

		if (cdinfo_dbp->track[i].artist == NULL) {
			(void) fprintf(fp, "<TD %s><B>&nbsp;</B></TD>\n",
				tdparms2
			);
		}
		else {
			(void) fprintf(fp, "<TD %s><B>", tdparms2);
			if (srchact != NULL) {
				p = cdinfo_txtreduce(
					cdinfo_dbp->track[i].artist,
					TRUE
				);
				(void) fprintf(fp, "<A HREF=\"%s%s\">",
					srchact, p
				);
				MEM_FREE(p);
			}
			util_html_fputs(
				cdinfo_dbp->track[i].artist,
				fp,
				FALSE,
				NULL,
				0
			);
			if (srchact != NULL)
				(void) fputs("</A>", fp);
			(void) fputs("</B></TD>\n", fp);
		}

		if (cdinfo_dbp->track[i].title == NULL) {
			(void) fprintf(fp, "<TD %s><B>(%s)</B></TD>\n",
				tdparms2,
				app_data.str_unkntrk
			);
		}
		else {
			(void) fprintf(fp, "<TD %s><B>", tdparms2);
			if (srchact != NULL) {
				p = cdinfo_txtreduce(
					cdinfo_dbp->track[i].title,
					TRUE
				);
				(void) fprintf(fp, "<A HREF=\"%s%s\">",
					srchact, p
				);
				MEM_FREE(p);
			}
			util_html_fputs(
				cdinfo_dbp->track[i].title,
				fp,
				FALSE,
				NULL,
				0
			);
			if (srchact != NULL)
				(void) fputs("</A>", fp);
			(void) fputs("</B></TD>\n", fp);
		}

		(void) fprintf(fp, "<TD %s>", tdparms2);
		util_html_fputs(cdinfo_genre_name(cdinfo_dbp->track[i].genre),
				fp, FALSE, NULL, 0);
		fputs("&nbsp;</TD>\n", fp);

		(void) fprintf(fp, "<TD %s>", tdparms2);
		util_html_fputs(cdinfo_genre_name(cdinfo_dbp->track[i].genre2),
				fp, FALSE, NULL, 0);
		fputs("&nbsp;</TD>\n", fp);

		(void) fprintf(fp, "<TD %s>%s</TD>\n",
			tdparms2,
			cdinfo_dbp->track[i].bpm == NULL ? "&nbsp;" :
				cdinfo_dbp->track[i].bpm);

		(void) fprintf(fp, "<TD %s>%s</TD>\n",
			tdparms2,
			cdinfo_dbp->track[i].year == NULL ? "&nbsp;" :
				cdinfo_dbp->track[i].year);

		(void) fprintf(fp, "<TD %s>", tdparms2);
		if (cdinfo_dbp->track[i].label != NULL)
			util_html_fputs(cdinfo_dbp->track[i].label,
					fp, FALSE, NULL, 0);
		else
			(void) fputs("&nbsp;", fp);
		(void) fputs("</TD>\n</TR>\n", fp);

		/* Count the number of rows needed for track credits and
		 * track notes
		 */
		for (cp = cdinfo_dbp->track[i].credit_list; cp != NULL;
		     cp = cp->next)
			ntrkrows++;
		if (cdinfo_dbp->track[i].notes != NULL)
			ntrkrows++;
	}
	(void) fputs("<TR><TD COLSPAN=\"10\">&nbsp;</TD></TR>\n", fp);

	/* Album credits and notes */
	ncreds = 0;
	for (cp = cdinfo_dbp->disc.credit_list; cp != NULL; cp = cp->next)
		ncreds++;

	nrows = ncreds + (cdinfo_dbp->disc.notes == NULL ? 0 : 1);
	if (nrows > 0) {
		first = TRUE;
		(void) fprintf(fp,
			"<TR>\n<TH %s ROWSPAN=\"%d\">Album<BR>"
			"Credits<BR>&amp; Notes</TH>\n",
			tdparms3, nrows
		);

		/* credits */
		for (cp = cdinfo_dbp->disc.credit_list; cp != NULL;
		     cp = cp->next) {
			if (first)
				first = FALSE;
			else
				(void) fputs("<TR>\n", fp);

			(void) fprintf(fp, "<TD %s COLSPAN=\"4\">", tdparms2);
			if (cp->crinfo.name != NULL) {
				if (srchact != NULL) {
					p = cdinfo_txtreduce(cp->crinfo.name,
							     TRUE);
					(void) fprintf(fp, "<A HREF=\"%s%s\">",
						srchact, p
					);
					MEM_FREE(p);
				}
				(void) fputs(cp->crinfo.name, fp);
				if (srchact != NULL)
					(void) fputs("</A>", fp);
			}
			else
				(void) fputs("unknown", fp);

			(void) fputs(" (", fp);
			if (cp->crinfo.role != NULL)
				util_html_fputs(cp->crinfo.role->name,
						fp, FALSE, NULL, 0);
			else
				fputs("unknown", fp);
			(void) fputs(")</TD>\n", fp);

			(void) fprintf(fp, "<TD %s COLSPAN=\"5\">", tdparms2);
			if (cp->notes != NULL)
				util_html_fputs(cp->notes, fp, TRUE,
						"courier", -1);
			else
				(void) fputs("&nbsp;", fp);
			(void) fputs("</TD>\n</TR>\n", fp);
		}

		/* notes */
		if (cdinfo_dbp->disc.notes != NULL) {
			if (!first)
				(void) fputs("<TR>\n", fp);
			(void) fprintf(fp, "<TD %s COLSPAN=\"9\">", tdparms2);
			util_html_fputs(cdinfo_dbp->disc.notes,
					fp, TRUE, "courier", -1);
			(void) fputs("</TD>\n</TR>\n", fp);
		}
	}

	/* Track credits and notes */
	if (ntrkrows > 0) {
		(void) fprintf(fp,
			    "<TH %s ROWSPAN=\"%d\">"
			    "Track<BR>Credits<BR>&amp; Notes</TH>\n",
			    tdparms3, ntrkrows);

		for (i = 0; i < (int) s->tot_trks; i++) {
			if (cdinfo_dbp->track[i].credit_list == NULL &&
			    cdinfo_dbp->track[i].notes == NULL)
				continue;

			ncreds = 0;
			for (cp = cdinfo_dbp->track[i].credit_list; cp != NULL;
			     cp = cp->next)
				ncreds++;

			nrows = ncreds + (cdinfo_dbp->track[i].notes == NULL ?
					  0 : 1);

			(void) fprintf(fp,
				    "<TD %s ROWSPAN=\"%d\">Track %d</TD>\n",
				    tdparms3,
				    nrows,
				    (int) s->trkinfo[i].trkno);

			first = TRUE;
			/* credits */
			for (cp = cdinfo_dbp->track[i].credit_list; cp != NULL;
			     cp = cp->next) {
				if (first)
					first = FALSE;
				else
					(void) fputs("<TR>\n", fp);

				(void) fprintf(fp, "<TD %s COLSPAN=\"3\">",
						tdparms2);

				if (cp->crinfo.name != NULL) {
					if (srchact != NULL) {
						p = cdinfo_txtreduce(
							cp->crinfo.name,
							TRUE
						);
						(void) fprintf(fp,
							"<A HREF=\"%s%s\">",
							srchact, p
						);
						MEM_FREE(p);
					}
					(void) fputs(cp->crinfo.name, fp);
					if (srchact != NULL)
						(void) fputs("</A>", fp);
				}
				else
					(void) fputs("unknown", fp);

				(void) fputs(" (", fp);
				if (cp->crinfo.role != NULL)
					util_html_fputs(cp->crinfo.role->name,
							fp, TRUE, NULL, 0);
				else
					(void) fputs("unknown", fp);
				(void) fputs(")</TD>\n", fp);

				(void) fprintf(fp, "<TD %s COLSPAN=\"5\">",
						tdparms2);
				if (cp->notes == NULL)
					(void) fputs("&nbsp;", fp);
				else
					util_html_fputs(cp->notes, fp, TRUE,
							"courier", -1);
				(void) fputs("</TD>\n</TR>\n", fp);
			}

			/* notes */
			if (cdinfo_dbp->track[i].notes != NULL) {
				if (!first)
					(void) fputs("<TR>\n", fp);

				(void) fprintf(fp, "<TD %s COLSPAN=\"8\">",
						tdparms2);
				util_html_fputs(cdinfo_dbp->track[i].notes,
						fp, TRUE,
						"courier", -1);
				(void) fputs("</TD>\n</TR>\n", fp);
			}
		}
	}

	(void) fputs("</TABLE>\n", fp);

	/* end of <DIV ALIGN="center"> */
	(void) fputs("</DIV>\n<P>\n", fp);

	/* Local discography */
	(void) fputs("<H4>Local Discography</H4>\n<P>\n<UL>\n", fp);

	/* Initialize list heads */
	for (i = 0; i < MAX_FILEFMTS; i++)
		lheads[i] = NULL;

	/* Check directory and add links to files */
	if (((util_urlchk(baseurl, &p, &i) & IS_REMOTE_URL) == 0) &&
	    (dp = OPENDIR(outdir)) != NULL) {
		while ((de = READDIR(dp)) != NULL) {
			if (strcmp(de->d_name, ".") == 0 ||
			    strcmp(de->d_name, "..") == 0)
				/* Skip . and .. */
				continue;

#ifdef __VMS
			/* Discard version number */
			if ((p = strrchr(de->d_name, ';')) != NULL)
				*p = '\0';

			p = util_vms_urlconv(outdir, VMS_2_UNIX);
			if (p != NULL) {
				(void) strncpy(filepath, p,
					       sizeof(filepath)-1);
				filepath[sizeof(filepath)-1] = '\0';
				MEM_FREE(p);
			}
#else
			filepath[0] = '\0';
#endif

			if (util_strcasecmp(de->d_name, "index.html") == 0)
				/* Skip index.html which we're generating */
				continue;

			fmt = -1;
			if ((p = strrchr(de->d_name, '.')) != NULL) {
				if (util_strcasecmp(p, ".m3u") == 0 ||
				    util_strcasecmp(p, ".pls") == 0)
					/* Skip playlist files */
					continue;

				/* Check if the file is an audio track */
				for (i = 0; i < MAX_FILEFMTS; i++) {
					if ((fmp = cdda_filefmt(i)) == NULL)
						continue;

					if (util_strcasecmp(p, fmp->suf) == 0)
						fmt = fmp->fmt;
				}
			}

			if (fmt >= 0) {
				playls_t	**hp;

				/* An audio track: Add to appropriate list */

				hp = &lheads[fmt];

				sp = (playls_t *) MEM_ALLOC(
					"playls_t",
					sizeof(playls_t)
				);
				if (sp != NULL) {
					sp->prev = sp->next = NULL;
					sp->prev2 = sp->next2 = NULL;
					sp->path = NULL;

					if (!util_newstr(&sp->path,
							de->d_name)) {
						MEM_FREE(sp);
					}
					else {
						/* Add to list */
						if (*hp != NULL)
							(*hp)->prev = sp;
						sp->next = *hp;
						*hp = sp;
					}
				}
			}
			else {
				/* Not an audio track */
				(void) fprintf(fp,
				    "<LI><A HREF=\"%s%s\">%s: %s</A></LI>\n",
				    filepath, de->d_name, "File", de->d_name
				);
			}
		}

		(void) CLOSEDIR(dp);
	}

#ifdef __VMS
	p = util_vms_urlconv(outdir, VMS_2_UNIX);
	if (p != NULL) {
		(void) strncpy(filepath, p, sizeof(filepath)-1);
		filepath[sizeof(filepath)-1] = '\0';
		MEM_FREE(p);
	}
#else
	filepath[0] = '\0';
#endif

	relpath = NULL;
	if (app_data.discog_url_pfx != NULL &&
	    (p = util_basename(app_data.discog_url_pfx)) != NULL) {
		if ((q = util_strstr(outdir, p)) != NULL)
			q += strlen(p);
		else
			q = NULL;

		MEM_FREE(p);

		if (q != NULL) {
			if (!util_newstr(&relpath, q)) {
				CDINFO_FATAL(app_data.str_nomemory);
				return FALSE;
			}
#ifdef __VMS
			/* Convert VMS file path name separators into
			 * URL path separators
			 */
			for (p = relpath; *p != '\0'; p++) {
				if (*p == '.')
					*p = '/';
				else if (*p == ']')
					*p = '\0';
			}
#endif
		}
	}

	/* For each file format, generate audio track playlists */
	for (i = 0; i < MAX_FILEFMTS; i++) {
		playls_t	*shead;

		if (lheads[i] == NULL || (fmp = cdda_filefmt(i)) == NULL)
			continue;

#ifdef __VMS
		(void) sprintf(plspath, "%s%s.m3u", outdir, fmp->name);
#else
		(void) sprintf(plspath, "%s%c%s.m3u",
				outdir, DIR_END, fmp->name);
#endif
		(void) UNLINK(plspath);

		/* Sort the track list */
		shead = cdinfo_sort_playlist(lheads[i]);

		/* Write audio track list in discography file */
		(void) fprintf(fp, "<LI>%s audio tracks:</LI>\n", fmp->name);
		(void) fputs("<UL>\n", fp);

		/* Write .m3u format playlist file */
		if ((pls_fp = fopen(plspath, "w")) == NULL) {
			DBGPRN(DBG_CDI)(errfp,
				"Cannot open %s for writing.\n", plspath);
		}
		else {
			(void) chmod(plspath, (mode_t) fmode);

			for (sp = shead; sp != NULL; sp = sp->next2) {
				if (relpath == NULL) {
					(void) fprintf(pls_fp, "%s\n",
						sp->path
					);
				}
				else {
					p = (char *) MEM_ALLOC("plsurl",
					    strlen(relpath) +
					    strlen(sp->path) + 4
					);
					if (p != NULL) {
					    /* This is a URL, so use '/'
					     * even for VMS.
					     */
					    (void) sprintf(p, "%s/%s",
						relpath,
						sp->path
					    );

					    q = util_urlencode(p);
					    if (q != NULL) {
						(void) fprintf(
						    pls_fp, "%s%s\n",
						    app_data.discog_url_pfx,
						    q
						);
						MEM_FREE(q);
					    }

					    MEM_FREE(p);
					}
				}
			}

			(void) fclose(pls_fp);

			/* Add playlist file entry to local discography */
			(void) fprintf(fp,
				"<LI><A HREF=\"%s%s.m3u\">"
				"Playlist</A> (.m3u format)</LI>\n",
				filepath, fmp->name
			);
		}

#ifdef __VMS
		(void) sprintf(plspath, "%s%s.pls", outdir, fmp->name);
#else
		(void) sprintf(plspath, "%s%c%s.pls",
				outdir, DIR_END, fmp->name);
#endif
		(void) UNLINK(plspath);

		/* Write .pls format playlist file */
		if ((pls_fp = fopen(plspath, "w")) == NULL) {
			DBGPRN(DBG_CDI)(errfp,
				"Cannot open %s for writing.\n", plspath);
		}
		else {
			int	n;

			(void) chmod(plspath, (mode_t) fmode);
			(void) fprintf(pls_fp, "[playlist]\n");

			n = 1;
			for (sp = shead; sp != NULL; sp = sp->next2) {
				char	*cp,
					*cp2;
				int	len;

				(void) sprintf(plspath, "%s%c%s",
					       outdir, DIR_END, sp->path);
				if (stat(plspath, &stbuf) < 0 ||
				    !S_ISREG(stbuf.st_mode))
					len = -1;
				else
					len = (int) stbuf.st_size;

				if (relpath == NULL) {
					(void) fprintf(pls_fp, "File%d=%s\n",
						n, sp->path
					);
				}
				else {
					p = (char *) MEM_ALLOC("plsurl",
					    strlen(relpath) +
					    strlen(sp->path) + 4
					);
					/* This is a URL, so use '/'
					 * even for VMS.
					 */
					if (p != NULL) {
					    (void) sprintf(p, "%s/%s",
						relpath,
						sp->path
					    );

					    q = util_urlencode(p);
					    if (q != NULL) {
						(void) fprintf(
						    pls_fp, "File%d=%s%s\n",
						    n,
						    app_data.discog_url_pfx,
						    q
						);
						MEM_FREE(q);
					    }

					    MEM_FREE(p);
					}
				}

				cp = NULL;
				if (!util_newstr(&cp, sp->path)) {
				    cp = sp->path;
				}
				else for (cp2 = cp; *cp2 != '\0'; cp2++) {
				    if (util_strcasecmp(cp2, fmp->suf) == 0) {
					*cp2 = '\0';
					break;
				    }
				    else if (*cp2 == '_' || *cp2 == '-')
					*cp2 = ' ';
				}

				(void) fprintf(pls_fp, "Title%d=%s\n",
						n, cp);
				(void) fprintf(pls_fp, "Length%d=%d\n",
						n, len);

				if (cp != sp->path)
					MEM_FREE(cp);
				n++;
			}

			(void) fprintf(pls_fp, "NumberOfEntries=%d\n", n - 1);
			(void) fprintf(pls_fp, "Version=2\n");
			(void) fclose(pls_fp);

			/* Add playlist file entry to local discography */
			(void) fprintf(fp,
				"<LI><A HREF=\"%s%s.pls\">"
				"Playlist</A> (.pls format)</LI>\n",
				filepath, fmp->name
			);
		}

		/* Add audio track files to local discography */
		for (sp = shead; sp != NULL; sp = sp->next2) {
			(void) fprintf(fp,
				"<LI><A HREF=\"%s%s\">Track: %s</A></LI>\n",
				filepath, sp->path, sp->path
			);
		}
		(void) fputs("</UL>\n", fp);

		/* Deallocate the playlist for the file format */
		sp = lheads[i];
		while (sp != NULL) {
			playls_t	*sp2;

			sp2 = sp->next;
			if (sp->path != NULL)
				MEM_FREE(sp->path);
			MEM_FREE(sp);
			sp = sp2;
		}
		lheads[i] = NULL;
	}

	if (relpath != NULL)
		MEM_FREE(relpath);

#ifdef __VMS
	(void) sprintf(filepath, "%s.discog]index.html", app_data.libdir);
	if ((p = util_vms_urlconv(filepath, VMS_2_UNIX)) != NULL) {
		(void) strncpy(filepath, p, sizeof(filepath)-1);
		filepath[sizeof(filepath)-1] = '\0';
		MEM_FREE(p);
	}
#else
	(void) sprintf(filepath, "%s/index.html", dhome);
#endif
	(void) fprintf(fp, "<LI><A HREF=\"%s\">Main index</A></LI>\n",
			filepath);

#ifdef __VMS
	if ((p = util_dirname(outdir)) == NULL) {
		(void) sprintf(filepath, "index.html");
	}
	else {
		(void) sprintf(filepath, "%sindex.html", p);
		MEM_FREE(p);
	}

	if ((p = util_vms_urlconv(filepath, VMS_2_UNIX)) != NULL) {
		(void) strncpy(filepath, p, sizeof(filepath)-1);
		filepath[sizeof(filepath)-1] = '\0';
		MEM_FREE(p);
	}
#else
	(void) sprintf(filepath, "../index.html");
#endif
	if (cdinfo_dbp->disc.genre != NULL) {
		(void) fprintf(fp, "<LI><A HREF=\"%s\">", filepath);
		util_html_fputs(
			cdinfo_genre_name(cdinfo_dbp->disc.genre),
			fp, TRUE, NULL, 0
		);
		(void) fputs(" index</A></LI>\n", fp);
	}

	if (cdinfo_dbp->disc.genre2 != NULL) {
		gpath = cdinfo_genre_path(cdinfo_dbp->disc.genre2);
#ifdef __VMS
		(void) sprintf(filepath, "%s.discog.%s]index.html",
			       app_data.libdir, gpath);
		if ((p = util_vms_urlconv(filepath, VMS_2_UNIX)) != NULL) {
			(void) strncpy(filepath, p, sizeof(filepath)-1);
			filepath[sizeof(filepath)-1] = '\0';
			MEM_FREE(p);
		}
#else
		(void) sprintf(filepath, "%s/%s/index.html", dhome, gpath);
#endif
		(void) fprintf(fp, "<LI><A HREF=\"%s\">", filepath);
		util_html_fputs(
			cdinfo_genre_name(cdinfo_dbp->disc.genre2),
			fp, TRUE, NULL, 0
		);
		(void) fputs(" index</A></LI>\n", fp);
	}

#ifdef __VMS
	(void) sprintf(filepath, "%s.discog]discog.html", app_data.libdir);
	if ((p = util_vms_urlconv(filepath, VMS_2_UNIX)) != NULL) {
		(void) strncpy(filepath, p, sizeof(filepath)-1);
		filepath[sizeof(filepath)-1] = '\0';
		MEM_FREE(p);
	}
#else
	(void) sprintf(filepath, "%s/discog.html", dhome);
#endif
	(void) fprintf(fp, "<LI><A HREF=\"%s\">%s</A></LI>\n",
			filepath, "How to use Local Discography");

	(void) fputs("</UL>\n<P>\n", fp);

	/* Directory info */
	(void) fprintf(fp, "<HR>\nThis directory: <B>%s</B><BR>\n", outdir);
	(void) fputs("</BODY>\n</HTML>\n", fp);

	if (srchact != NULL)
		MEM_FREE(srchact);

	(void) fclose(fp);

	if (newdiscog) {
		/* Generate local discography index for the primary genre */

		gpath = cdinfo_genre_path(cdinfo_dbp->disc.genre);
		cmd = (char *) MEM_ALLOC("genidx_cmd",
			strlen(gpath) + STR_BUF_SZ
		);
		if (cmd == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return FALSE;
		}
#ifdef __VMS
		(void) sprintf(cmd, "genidx %s", gpath);
#else
		(void) sprintf(cmd, "genidx %s >/dev/null 2>&1 &", gpath);
#endif

		DBGPRN(DBG_CDI)(errfp,
			"\nGenerating local discography index for %s\n",
			gpath);

		(void) util_runcmd(cmd, cdinfo_clinfo->workproc,
				   cdinfo_clinfo->arg);
		MEM_FREE(cmd);
	}

	if (cdinfo_dbp->disc.genre2 != NULL) {
		gpath = cdinfo_genre_path(cdinfo_dbp->disc.genre2);
#ifdef __VMS
		(void) sprintf(filepath, "%s.discog.%s]index.html",
			       app_data.libdir, gpath);
		if ((p = util_vms_urlconv(filepath, VMS_2_UNIX)) != NULL) {
			(void) strncpy(filepath, p, sizeof(filepath)-1);
			filepath[sizeof(filepath)-1] = '\0';
			MEM_FREE(p);
		}
#else
		(void) sprintf(filepath, "%s/%s/%s/index.html",
			       outdir, dhome, gpath);
#endif
		if (stat(filepath, &stbuf) < 0 && errno == ENOENT) {
			/* Generate local discography index for the
		 	 * secondary genre, if applicable
			 */
			cmd = (char *) MEM_ALLOC("genidx_cmd",
				strlen(gpath) + STR_BUF_SZ
			);
			if (cmd == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return FALSE;
			}
#ifdef __VMS
			(void) sprintf(cmd, "genidx %s", gpath);
#else
			(void) sprintf(cmd, "genidx %s >/dev/null 2>&1 &",
				       gpath);
#endif

			DBGPRN(DBG_CDI)(errfp,
			    "\nGenerating local discography index for %s\n",
			    gpath);

			(void) util_runcmd(cmd, cdinfo_clinfo->workproc,
					   cdinfo_clinfo->arg);
			MEM_FREE(cmd);
		}
	}

	return TRUE;
}


/*
 * cdinfo_map_cdtext
 *	Move CD-TEXT data to the incore CD information main structure.
 *
 * Args:
 *	s   - Pointer to the curstat_t structure.
 *	cdt - Pointer to the di_cdtext_t structure.
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_map_cdtext(curstat_t *s, di_cdtext_t *cdt)
{
	int	i;
	char	*cp = NULL;

	if (cdinfo_dbp == NULL || cdt == NULL)
		return;

	/* Disc artist and title */
	cdinfo_dbp->disc.artist = cdt->disc.performer;
	cdinfo_dbp->disc.title = cdt->disc.title;
	cdt->disc.performer = NULL;
	cdt->disc.title = NULL;

	if(cdt->disc.catno != NULL){
		/* Media catalog number (UPC/MCN) */
		(void) strncpy(s->mcn, cdt->disc.catno, sizeof(s->mcn) - 1);
		s->mcn[sizeof(s->mcn) - 1] = '\0';
		MEM_FREE(cdt->disc.catno);
		cdt->disc.catno = NULL;
	} else s->mcn[0]='\0';
	/* Disc identification, songwriter, composer, composer and message:
	 * put in the notes field
	 */
	if (cdt->ident != NULL) {
		cp = (char *) MEM_ALLOC("ident",
			strlen(cdt->ident) + 16
		);
		if (cp == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return;
		}
		(void) sprintf(cp, "Identification:\t%s\n", cdt->ident);
		MEM_FREE(cdt->ident);
		cdt->ident = NULL;
	}
	if (cdt->disc.songwriter != NULL) {
		if (cp == NULL) {
			cp = (char *) MEM_ALLOC("songwriter",
				strlen(cdt->disc.songwriter) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "Songwriter:\t%s\n",
					cdt->disc.songwriter);
		}
		else {
			cp = (char *) MEM_REALLOC("songwriter", cp,
				strlen(cp) + strlen(cdt->disc.songwriter) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "%sSongwriter:\t%s\n",
					cp, cdt->disc.songwriter);
		}
		MEM_FREE(cdt->disc.songwriter);
		cdt->disc.songwriter = NULL;
	}
	if (cdt->disc.composer != NULL) {
		if (cp == NULL) {
			cp = (char *) MEM_ALLOC("composer",
				strlen(cdt->disc.composer) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "Composer:\t%s\n",
					cdt->disc.composer);
		}
		else {
			cp = (char *) MEM_REALLOC("composer", cp,
				strlen(cp) + strlen(cdt->disc.composer) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "%sComposer:\t%s\n",
					cp, cdt->disc.composer);
		}
		MEM_FREE(cdt->disc.composer);
		cdt->disc.composer = NULL;
	}
	if (cdt->disc.arranger != NULL) {
		if (cp == NULL) {
			cp = (char *) MEM_ALLOC("arranger",
				strlen(cdt->disc.arranger) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "Composer:\t%s\n",
					cdt->disc.arranger);
		}
		else {
			cp = (char *) MEM_REALLOC("arranger", cp,
				strlen(cp) + strlen(cdt->disc.arranger) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "%sArranger:\t\t%s\n",
					cp, cdt->disc.arranger);
		}
		MEM_FREE(cdt->disc.arranger);
		cdt->disc.arranger = NULL;
	}
	if (cdt->disc.message != NULL) {
		if (cp == NULL) {
			cp = (char *) MEM_ALLOC("message",
				strlen(cdt->disc.message) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "Composer:\t%s\n",
					cdt->disc.message);
		}
		else {
			cp = (char *) MEM_REALLOC("message", cp,
				strlen(cp) + strlen(cdt->disc.message) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "%s\n%s\n",
					cp, cdt->disc.message);
		}
		MEM_FREE(cdt->disc.message);
		cdt->disc.message = NULL;
	}

	cdinfo_dbp->disc.notes = cp;

	for (i = 0; i < (int) s->tot_trks; i++) {
		/* Track artist, title and ISRC */
		cdinfo_dbp->track[i].artist = cdt->track[i].performer;
		cdinfo_dbp->track[i].title = cdt->track[i].title;
		cdinfo_dbp->track[i].isrc = cdt->track[i].catno;

		cdt->track[i].performer = NULL;
		cdt->track[i].title = NULL;
		cdt->track[i].catno = NULL;

		/* Track songwriter, composer, arranger and message:
		 * put in the notes section.
		 */
		cp = NULL;

		if (cdt->track[i].songwriter != NULL) {
			cp = (char *) MEM_ALLOC("songwriter",
				strlen(cdt->track[i].songwriter) + 16
			);
			if (cp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
			(void) sprintf(cp, "Songwriter:\t%s\n",
					cdt->track[i].songwriter);
			MEM_FREE(cdt->track[i].songwriter);
			cdt->track[i].songwriter = NULL;
		}
		if (cdt->track[i].composer != NULL) {
			if (cp == NULL) {
				cp = (char *) MEM_ALLOC("composer",
					strlen(cdt->track[i].composer) + 16
				);
				if (cp == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(cp, "Composer:\t%s\n",
						cdt->track[i].composer);
			}
			else {
				cp = (char *) MEM_REALLOC("composer", cp,
					strlen(cp) +
					strlen(cdt->track[i].composer) + 16
				);
				if (cp == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(cp, "%sComposer:\t%s\n",
						cp, cdt->track[i].composer);
			}
			MEM_FREE(cdt->track[i].composer);
			cdt->track[i].composer = NULL;
		}
		if (cdt->track[i].arranger != NULL) {
			if (cp == NULL) {
				cp = (char *) MEM_ALLOC("arranger",
					strlen(cdt->track[i].arranger) + 16
				);
				if (cp == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(cp, "Composer:\t%s\n",
						cdt->track[i].arranger);
			}
			else {
				cp = (char *) MEM_REALLOC("arranger", cp,
					strlen(cp) +
					strlen(cdt->track[i].arranger) + 16
				);
				if (cp == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(cp, "%sArranger:\t%s\n",
						cp, cdt->track[i].arranger);
			}
			MEM_FREE(cdt->track[i].arranger);
			cdt->track[i].arranger = NULL;
		}
		if (cdt->track[i].message != NULL) {
			if (cp == NULL) {
				cp = (char *) MEM_ALLOC("message",
					strlen(cdt->track[i].message) + 16
				);
				if (cp == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(cp, "Composer:\t%s\n",
						cdt->track[i].message);
			}
			else {
				cp = (char *) MEM_REALLOC("message", cp,
					strlen(cp) +
					strlen(cdt->track[i].message) + 16
				);
				if (cp == NULL) {
					CDINFO_FATAL(app_data.str_nomemory);
					return;
				}
				(void) sprintf(cp, "%s\n%s\n",
						cp, cdt->track[i].message);
			}
			MEM_FREE(cdt->track[i].message);
			cdt->track[i].message = NULL;
		}

		cdinfo_dbp->track[i].notes = cp;
	}
}


