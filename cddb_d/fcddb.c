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
static char     *_fcddb_c_ident_ = "@(#)fcddb.c	1.110 04/04/20";
#endif

#include "fcddb.h"
#include "common_d/version.h"
#include "cddbp.h"
#include "genretbl.h"
#include "regiontbl.h"
#include "langtbl.h"
#include "roletbl.h"

#ifndef __VMS
/* UNIX */

#ifndef NOREMOTE
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#if defined(_AIX) || defined(__QNX__)
#include <sys/select.h>
#endif
#endif	/* NOREMOTE */

/* Directory path name convention */
#define DIR_BEG			'/'		/* Directory start */
#define DIR_SEP			'/'		/* Directory separator */
#define DIR_END			'/'		/* Directory end */
#define CUR_DIR			"."		/* Current directory */

/* Defines used by fcddb_runcmd */
#define STR_SHPATH		"/bin/sh"	/* Path to shell */
#define STR_SHNAME		"sh"		/* Name of shell */
#define STR_SHARG		"-c"		/* Shell arg */

#else
/* OpenVMS */

#ifndef NOREMOTE
#include <socket.h>
#include <time.h>
#include <in.h>
#include <inet.h>
#include <netdb.h>
#endif	/* NOREMOTE */

/* Directory path name convention */
#define DIR_BEG			'['		/* Directory start */
#define DIR_SEP			'.'		/* Directory separator */
#define DIR_END			']'		/* Directory end */
#define CUR_DIR			"[]"		/* Current directory */

#endif	/* __VMS */

/* Max host name length */
#ifndef HOST_NAM_SZ
#define HOST_NAM_SZ		64
#endif

/* Minimum/maximum macros */
#define FCDDB2_MIN(a,b)		(((a) > (b)) ? (b) : (a))
#define FCDDB2_MAX(a,b)		(((a) > (b)) ? (a) : (b))

/* Skip white space macro */
#define SKIP_SPC(p)		while (*(p) == ' ' || *(p) == '\t') (p)++;

/* CDDBD status code macros */
#define STATCODE_1ST(p)		((p)[0])
#define STATCODE_2ND(p)		((p)[1])
#define STATCODE_3RD(p)		((p)[2])
#define STATCODE_CHECK(p)	\
	(STATCODE_1ST(p) != '\0' && isdigit((int) STATCODE_1ST(p)) && \
	 STATCODE_2ND(p) != '\0' && isdigit((int) STATCODE_2ND(p)) && \
	 STATCODE_3RD(p) != '\0' && isdigit((int) STATCODE_3RD(p)))

/* HTTP status codes */
#define HTTP_PROXYAUTH_FAIL	407

/* Number of seconds per day */
#define SECS_PER_DAY		(24 * 60 * 60)

/* Number of seconds to wait for an external command to complete */
#define CMD_TIMEOUT_SECS	60


bool_t		fcddb_debug;			/* Debug flag */
FILE		*fcddb_errfp;			/* Debug message file stream */
fcddb_gmap_t	*fcddb_gmap_head = NULL;	/* Genre map list head */

STATIC char	fcddb_hellostr[HOST_NAM_SZ + (STR_BUF_SZ * 2)],
		fcddb_extinfo[HOST_NAM_SZ + (STR_BUF_SZ * 2)],
		*fcddb_auth_buf = NULL;

/*
 * fcddb_sum
 *	Compute a checksum number for use by fcddb_discid
 *
 * Args:
 *	n - A numeric value
 *
 * Return:
 *	The checksum of n
 */
STATIC int
fcddb_sum(int n)
{
 	int	ret;

	/* For backward compatibility this algorithm must not change */
	for (ret = 0; n > 0; n /= 10)
		ret += n % 10;

	return (ret);
}


/*
 * fcddb_line_filter
 *	Line filter to prevent multi-line strings from being stored in
 *	single-line fields.
 *
 * Args:
 *	str - The field string
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_line_filter(char *str)
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
 * fcddb_iso8859_to_utf8
 *	Convert the input ISO-8859-1 string to UTF-8 and return the
 *	resultant string.  The output string buffer is dynamically allocated
 *	and should be freed by the caller via MEM_FREE when done.
 *
 * Args:
 *	str - source string (ISO-8859-1)
 *
 * Return:
 *	Return string (UTF-8)
 */
STATIC char *
fcddb_iso8859_to_utf8(char *str)
{
	unsigned char	*p,
			*q,
			*tmpbuf;
	char		*retbuf;

	tmpbuf = (unsigned char *) MEM_ALLOC(
		"utf8_to_buf", strlen(str) * 6 + 1
	);
	if (tmpbuf == NULL)
		return NULL;

	p = (unsigned char *) str;
	q = (unsigned char *) tmpbuf;
	while (*p != '\0') {
		if (*p <= 0x7f) {
			/* Simple US-ASCII character: just copy it */
			*q++ = *p++;
		}
		else {
			/* ISO8859 is byte-oriented, so this needs to
			 * only handle the "extended ASCII" characters
			 * 0xc0 to 0xfd.
			 */
			*q++ = (0xc0 | ((int) (0xc0 & *p) >> 6));
			*q++ = 0x80 | (*p & 0x3f);
			p++;
		}
	}
	*q = '\0';

	retbuf = (char *) MEM_ALLOC(
		"utf8_retbuf", strlen((char *) tmpbuf) + 1
	);
	if (retbuf != NULL)
		(void) strcpy(retbuf, (char *) tmpbuf);

	MEM_FREE(tmpbuf);
	return (retbuf);
}


/*
 * fcddb_utf8_to_iso8859
 *	Convert the input UTF-8 string to ISO8859-1 and return the
 *	resultant string.  The output string buffer is dynamically allocated
 *	and should be freed by the caller via MEM_FREE when done.
 *
 * Args:
 *	str - source string (UTF-8)
 * Return:
 *
 *	Return string (ISO-8859-1)
 */
STATIC char *
fcddb_utf8_to_iso8859(char *str)
{
	unsigned char	*p,
			*q,
			*tmpbuf;
	char		*retbuf;

	tmpbuf = (unsigned char *) MEM_ALLOC(
		"iso8859_to_buf", strlen(str) + 1
	);
	if (tmpbuf == NULL)
		return NULL;
	
	p = (unsigned char *) str;
	q = (unsigned char *) tmpbuf;
	while (*p != '\0') {
		if (*p <= 0x7f) {
			/* Simple US-ASCII character: just copy it */
			*q++ = *p++;
		}
		else if (*p >= 0xc0 && *p <= 0xfd) {
			int	n = 0;

			if ((*p & 0xe0) == 0xc0) {
				n = 2;
				if (*(p+1) >= 0x80 && *(p+1) <= 0xfd) {
				    /* OK */
				    *q = (((*p & 0x03) << 6) |
					   (*(p+1) & 0x3f));

				    if ((*p & 0x1c) != 0) {
					/* Decodes to more than 8 bits */
					*q = '_';
				    }
				    else if (*q <= 0x7f) {
					/* Not the shortest encoding:
					 * illegal.
					 */
					*q = '_';
				    }
				}
				else {
				    /* Malformed UTF-8 character */
				    *q = '_';
				}
				q++;
			}
			else if ((*p & 0xf0) == 0xe0) {
				/* Three-byte sequence */
				n = 3;
			}
			else if ((*p & 0xf8) == 0xf0) {
				/* Four-byte sequence */
				n = 4;
			}
			else if ((*p & 0xfc) == 0xf8) {
				/* Five-byte sequence */
				n = 5;
			}
			else if ((*p & 0xfe) == 0xfc) {
				/* Six-byte sequence */
				n = 6;
			}

			if (n > 2)
				*q++ = '_';

			while (n > 0) {
				if (*(++p) == '\0')
					break;
				n--;
			}
		}
		else {
			/* Malformed UTF-8 sequence: skip */
			p++;
		}
	}
	*q = '\0';

	retbuf = (char *) MEM_ALLOC(
		"iso8859_retbuf", strlen((char *) tmpbuf) + 1
	);
	if (retbuf != NULL)
		(void) strcpy(retbuf, (char *) tmpbuf);

	MEM_FREE(tmpbuf);
	return (retbuf);
}


/*
 * fcddb_strcasecmp
 *	Compare two strings a la strcmp(), except it is case-insensitive.
 *
 * Args:
 *	s1 - The first text string.
 *	s2 - The second text string.
 *
 * Return:
 *	Compare value.  See strcmp(3).
 */
STATIC int
fcddb_strcasecmp(char *s1, char *s2)
{
	char	*buf1,
		*buf2,
		*p;
	int	ret;

	if (s1 == NULL || s2 == NULL)
		return 0;

	/* Allocate tmp buffers */
	buf1 = (char *) MEM_ALLOC("strcasecmp_buf1", strlen(s1)+1);
	buf2 = (char *) MEM_ALLOC("strcasecmp_buf2", strlen(s2)+1);
	if (buf1 == NULL || buf2 == NULL)
		return -1;	/* Shrug */

	/* Convert both strings to lower case and store in tmp buffer */
	for (p = buf1; *s1 != '\0'; s1++, p++)
		*p = (char) ((isupper((int) *s1)) ? tolower((int) *s1) : *s1);
	*p = '\0';
	for (p = buf2; *s2 != '\0'; s2++, p++)
		*p = (char) ((isupper((int) *s2)) ? tolower((int) *s2) : *s2);
	*p = '\0';

	ret = strcmp(buf1, buf2);

	MEM_FREE(buf1);
	MEM_FREE(buf2);

	return (ret);
}


#ifdef __VMS

/*
 * fcddb_vms_dirconv
 *	Convert VMS directory notation from disk:[a.b.c] to disk:[a.b]c.dir
 *	syntax.
 *
 * Args:
 *	path - Input directory path string
 *
 * Return:
 *	The output path string.  The string buffer should be deallocated
 *	by the caller when done.  If an error is encountered, NULL is
 *	returned.
 */
STATIC char *
fcddb_vms_dirconv(char *path)
{
	char	*cp,
		*cp2,
		*cp3,
		*buf,
		tmp[STR_BUF_SZ];

	buf = (char *) MEM_ALLOC("vms_dirconv", strlen(path) + 16);
	if (buf == NULL)
		return NULL;

	(void) strcpy(buf, path);

	if ((cp = strchr(buf, DIR_BEG)) != NULL) {
		if ((cp2 = strrchr(buf, DIR_END)) != NULL) {
			if ((cp3 = strrchr(buf, DIR_SEP)) != NULL) {
				if (fcddb_strcasecmp(cp3, ".dir") == 0) {
					/* Already in the desired form */
					return (buf);
				}
				*cp2 = '\0';
				*cp3 = DIR_END;
				(void) strcat(cp3, ".dir");
				return (buf);
			}
			else {
				if ((cp = strchr(buf, ':')) == NULL)
					cp = buf;
				else
					cp++;

				if (strcmp(cp, "[000000]") == 0)	
					return (buf);

				*cp2 = '\0';
				(void) strcpy(tmp, cp+1);
				(void) sprintf(cp, "[000000]%s.dir", tmp); 
				return (buf);
			}
		}
		else {
			/* Invalid path */
			return NULL;
		}
	}
	else {
		if ((cp2 = strrchr(buf, DIR_SEP)) != NULL) {
			if ((cp3 = strchr(buf, ':')) == NULL)
				cp3 = buf;
			else
				cp3++;

			if (fcddb_strcasecmp(cp2, ".dir") == 0) {
				(void) sprintf(tmp, "[000000]%s", cp3);
				strcpy(cp3, tmp);
				return (buf);
			}
			else {
				/* Invalid path */
				return NULL;
			}
		}
		else {
			if ((cp3 = strchr(buf, ':')) != NULL) {
				(void) strcpy(cp3+1, "[000000]");
				return (buf);
			}

			/* Invalid path */
			return NULL;
		}
	}
	/*NOTREACHED*/
}

#endif	/* __VMS */


/*
 * fcddb_ckmkdir
 *	Check the specified directory path, if it doesn't exist,
 *	attempt to create it.
 *
 * Args:
 *	path - Directory path to check or create
 *	mode - Directory permissions
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_ckmkdir(char *path, mode_t mode)
{
	char		*dpath = NULL;
	struct stat	stbuf;

#ifdef __VMS
	if ((dpath = fcddb_vms_dirconv(path)) == NULL)
		return CDDBTRNCannotCreateFile;
#else
	if ((dpath = fcddb_strdup(path)) == NULL)
		return CDDBTRNOutOfMemory;
#endif

	if (stat(dpath, &stbuf) < 0) {
		if (errno == ENOENT) {
			if (mkdir(path, mode) < 0) {
				FCDDBDBG(fcddb_errfp, "fcddb_ckmkdir: %s "
					"mkdir failed, errno=%d\n",
					path, errno
				);
				MEM_FREE(dpath);
				return CDDBTRNCannotCreateFile;
			}
			(void) chmod(path, mode);
		}
		else {
			FCDDBDBG(fcddb_errfp,
				"fcddb_ckmkdir: %s stat failed, errno=%d\n",
				path, errno
			);
			MEM_FREE(dpath);
			return CDDBTRNCannotCreateFile;
		}
	}
	else if (!S_ISDIR(stbuf.st_mode)) {
		FCDDBDBG(fcddb_errfp, "fcddb_ckmkdir: "
			"%s is not a directory! (mode=0x%x)\n",
			path, (int) stbuf.st_mode
		);
		MEM_FREE(dpath);
		return CDDBTRNCannotCreateFile;
	}

	MEM_FREE(dpath);
	return Cddb_OK;
}


/*
 * fcddb_strcat
 *	Similar to strcat() except this handles special meta characters
 *	in CDDB data.
 *
 * Args:
 *	s1 - target string
 *	s2 - source string
 *
 * Return:
 *	Pointer to target string if successful, or NULL on failure
 */
STATIC char *
fcddb_strcat(char *s1, char *s2)
{
	int	n;
	char	*cp = s1;
	bool_t	proc_slash;

	if (s1 == NULL || s2 == NULL)
		return NULL;

	/* Concatenate two strings, with special handling for newline
	 * and tab characters.
	 */
	proc_slash = FALSE;
	n = strlen(s1);
	s1 += n;

	if (n > 0 && *(s1 - 1) == '\\') {
		proc_slash = TRUE;	/* Handle broken escape sequences */
		s1--;
	}

	for (; *s2 != '\0'; s1++, s2++) {
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
				*s1 = '\n';
				break;
			case 't':
				*s1 = '\t';
				break;
			case '\\':
				*s1 = '\\';
				break;
			case '\0':
				*s1 = '\\';
				s2--;
				break;
			default:
				*s1++ = '\\';
				*s1 = *s2;
				break;
			}
		}
		else
			*s1 = *s2;
	}
	*s1 = '\0';

	return (cp);
}


/*
 * fcddb_http_xlat
 *	String translator that handles HTTP character escape sequences
 *
 * Args:
 *	s1 - source string
 *	s2 - target string
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_http_xlat(char *s1, char *s2)
{
	char	*p,
		*q;

	for (p = s1, q = s2; *p != '\0'; p++) {
		switch (*p) {
		case '?':
		case '=':
		case '+':
		case '&':
		case ' ':
		case '%':
			(void) sprintf(q, "%%%02X", *p);
			q += 3;
			break;
		default:
			*q = *p;
			q++;
			break;
		}
	}
	*q = '\0';
}


/*
 * Data used by fcddb_b64encode
 */
STATIC char	b64map[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '+', '/'
};

#define B64_PAD		'='


/*
 * fcddb_b64encode
 *	Base64 encoding function
 *
 * Args:
 *	ibuf - Input string buffer
 *	len - Number of characters
 *	obuf - Output string buffer
 *	brklines - Whether to break output into multiple lines
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_b64encode(byte_t *ibuf, int len, byte_t *obuf, bool_t brklines)
{
	int	i, j, k, n,
		c[4];
	byte_t	sbuf[4];

	for (i = k = 0; (i + 3) <= len; i += 3, ibuf += 3) {
		c[0] = ((int) ibuf[0] >> 2);
		c[1] = ((((int) ibuf[0] & 0x03) << 4) |
			(((int) ibuf[1] & 0xf0) >> 4));
		c[2] = ((((int) ibuf[1] & 0x0f) << 2) |
			(((int) ibuf[2] & 0xc0) >> 6));
		c[3] = ((int) ibuf[2] & 0x3f);

		for (j = 0; j < 4; j++)
			*obuf++ = b64map[c[j]];

		if (brklines && ++k == 16) {
			k = 0;
			*obuf++ = '\n';
		}
	}

	if (i < len) {
		n = len - i;
		(void) strncpy((char *) sbuf, (char *) ibuf, n);
		for (j = n; j < 3; j++)
			sbuf[j] = (unsigned char) 0;

		n++;
		ibuf = sbuf;
		c[0] = ((int) ibuf[0] >> 2);
		c[1] = ((((int) ibuf[0] & 0x03) << 4) |
			(((int) ibuf[1] & 0xf0) >> 4));
		c[2] = ((((int) ibuf[1] & 0x0f) << 2) |
			(((int) ibuf[2] & 0xc0) >> 6));
		c[3] = ((int) ibuf[2] & 0x3f);

		for (j = 0; j < 4; j++)
			*obuf++ = (j < n) ? b64map[c[j]] : B64_PAD;

		if (brklines && ++k == 16)
			*obuf++ = '\n';
	}

	if (brklines)
		*obuf++ = '\n';

	*obuf = '\0';
}


/*
 * fcddb_genre_addcateg
 *	Add a new category to the genre map table.
 *
 * Args:
 *	categ - The category name string
 *
 * Return:
 *	Nothing.
 */
STATIC void
fcddb_genre_addcateg(char *categ)
{
	fcddb_gmap_t	*gmp,
			*gmp2;

	gmp2 = (fcddb_gmap_t *)(void *) MEM_ALLOC(
		"CddbGenremap", sizeof(fcddb_gmap_t)
	);
	if (gmp2 == NULL)
		return;	/* Can't add: out of memory */

	(void) memset(gmp2, 0, sizeof(fcddb_gmap_t));

	/* Map to unclassifiable */
	gmp2->cddb1name = fcddb_strdup(categ);
	gmp2->meta.id = "220";
	gmp2->meta.name = "Unclassifiable";
	gmp2->sub.id = "221";
	gmp2->sub.name = "General Unclassifiable";
	gmp2->flags = CDDB_GMAP_DUP | CDDB_GMAP_DYN;
	gmp2->next = NULL;

	if (fcddb_gmap_head == NULL) {
		fcddb_gmap_head = gmp2;
	}
	else for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
		if (gmp->next == NULL) {
			gmp->next = gmp2;
			break;
		}
	}
}


/*
 * fcddb_clear_url
 *	Clear a URL structure
 *
 * Args:
 *	up - Pointer to the URL structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_url(cddb_url_t *up)
{
	if (up->type != NULL) {
		MEM_FREE(up->type);
		up->type = NULL;
	}
	if (up->href != NULL) {
		MEM_FREE(up->href);
		up->href = NULL;
	}
	if (up->displaylink != NULL) {
		MEM_FREE(up->displaylink);
		up->displaylink = NULL;
	}
	if (up->displaytext != NULL) {
		MEM_FREE(up->displaytext);
		up->displaytext = NULL;
	}
	if (up->category != NULL) {
		MEM_FREE(up->category);
		up->category = NULL;
	}
	if (up->description != NULL) {
		MEM_FREE(up->description);
		up->description = NULL;
	}
}


/*
 * fcddb_clear_urllist
 *	Clear a URL list structure
 *
 * Args:
 *	up - Pointer to the URL list structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_urllist(cddb_urllist_t *ulp)
{
	cddb_url_t	*up,
			*nextp;


	for (up = ulp->urls; up != NULL; up = nextp) {
		nextp = up->next;
		fcddb_clear_url(up);
		MEM_FREE(up);
	}
	ulp->urls = NULL;
	ulp->count = 0;
}


/*
 * fcddb_clear_urlmanager
 *	Clear a URL manager structure
 *
 * Args:
 *	up - Pointer to the URL manager structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_urlmanager(cddb_urlmanager_t *mp)
{
	cddb_urllist_t	*ulp = &mp->urllist;

	fcddb_clear_urllist(ulp);
	mp->control = NULL;
}


/*
 * fcddb_clear_fullname
 *	Clear a full name structure
 *
 * Args:
 *	up - Pointer to the full name structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_fullname(cddb_fullname_t *fnp)
{
	if (fnp->name != NULL) {
		MEM_FREE(fnp->name);
		fnp->name = NULL;
	}
	if (fnp->lastname != NULL) {
		MEM_FREE(fnp->lastname);
		fnp->lastname = NULL;
	}
	if (fnp->firstname != NULL) {
		MEM_FREE(fnp->firstname);
		fnp->firstname = NULL;
	}
	if (fnp->the != NULL) {
		MEM_FREE(fnp->the);
		fnp->the = NULL;
	}
}


/*
 * fcddb_clear_credit
 *	Clear a credit structure
 *
 * Args:
 *	up - Pointer to the credit structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_credit(cddb_credit_t *cp)
{
	fcddb_clear_fullname(&cp->fullname);
	if (cp->notes != NULL) {
		MEM_FREE(cp->notes);
		cp->notes = NULL;
	}
	cp->role = NULL;
	cp->next = NULL;
}


/*
 * fcddb_clear_credits
 *	Clear a credits structure
 *
 * Args:
 *	up - Pointer to the credits structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_credits(cddb_credits_t *csp)
{
	cddb_credit_t	*cp,
			*nextp;

	for (cp = csp->credits; cp != NULL; cp = nextp) {
		nextp = cp->next;
		fcddb_clear_credit(cp);
		MEM_FREE(cp);
	}
	csp->credits = NULL;
	csp->count = 0;
}


/*
 * fcddb_clear_segment
 *	Clear a segment structure
 *
 * Args:
 *	up - Pointer to the segment structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_segment(cddb_segment_t *sp)
{
	fcddb_clear_credits(&sp->credits);

	if (sp->name != NULL) {
		MEM_FREE(sp->name);
		sp->name = NULL;
	}
	if (sp->notes != NULL) {
		MEM_FREE(sp->notes);
		sp->notes = NULL;
	}
	if (sp->starttrack != NULL) {
		MEM_FREE(sp->starttrack);
		sp->starttrack = NULL;
	}
	if (sp->startframe != NULL) {
		MEM_FREE(sp->startframe);
		sp->startframe = NULL;
	}
	if (sp->endtrack != NULL) {
		MEM_FREE(sp->endtrack);
		sp->endtrack = NULL;
	}
	if (sp->endframe != NULL) {
		MEM_FREE(sp->endframe);
		sp->endframe = NULL;
	}
	sp->control = NULL;
	sp->next = NULL;
}


/*
 * fcddb_clear_segments
 *	Clear a segments structure
 *
 * Args:
 *	up - Pointer to the segments structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_segments(cddb_segments_t *ssp)
{
	cddb_segment_t	*sp,
			*nextp;

	for (sp = ssp->segments; sp != NULL; sp = nextp) {
		nextp = sp->next;
		fcddb_clear_segment(sp);
		MEM_FREE(sp);
	}
	ssp->segments = NULL;
	ssp->count = 0;
}


/*
 * fcddb_clear_genretree
 *	Clear a genre tree structure
 *
 * Args:
 *	up - Pointer to the genre tree structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_genretree(cddb_genretree_t *gp)
{
	cddb_genre_t	*p,
			*q,
			*nextp,
			*nextq;
	fcddb_gmap_t	*gmp,
			*gmp2;

	/* Free genre tree */
	nextp = NULL;
	for (p = gp->genres; p != NULL; p = nextp) {
		nextp = p->nextmeta;

		nextq = NULL;
		for (q = p->next; q != NULL; q = nextq) {
			nextq = q->next;

			if (q->id != NULL)
				MEM_FREE(q->id);
			if (q->name != NULL)
				MEM_FREE(q->name);

			MEM_FREE(q);
		}

		if (p->id != NULL)
			MEM_FREE(p->id);
		if (p->name != NULL)
			MEM_FREE(p->name);

		MEM_FREE(p);
	}
	gp->genres = NULL;
	gp->count = 0;

	/* Deallocate dynamic genre mapping if applicable */
	gmp2 = NULL;
	for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp2) {
		gmp2 = gmp->next;

		if ((gmp->flags & CDDB_GMAP_DYN) != 0) {
			if (gmp->cddb1name != NULL)
				MEM_FREE(gmp->cddb1name);
			if (gmp->meta.id != NULL)
				MEM_FREE(gmp->meta.id);
			if (gmp->meta.name != NULL)
				MEM_FREE(gmp->meta.name);
			if (gmp->sub.id != NULL)
				MEM_FREE(gmp->sub.id);
			if (gmp->sub.name != NULL)
				MEM_FREE(gmp->sub.name);
			MEM_FREE(gmp);
		}
	}
	fcddb_gmap_head = NULL;
}


/*
 * fcddb_clear_roletree
 *	Clear a role tree structure
 *
 * Args:
 *	up - Pointer to the role tree structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_roletree(cddb_roletree_t *rtp)
{
	cddb_rolelist_t	*rlp,
			*nextp;

	nextp = NULL;
	for (rlp = rtp->rolelists; rlp != NULL; rlp = nextp) {
		nextp = rlp->next;
		MEM_FREE(rlp);
	}
	rtp->rolelists = NULL;
	rtp->count = 0;
}


/*
 * fcddb_clear_userinfo
 *	Clear a user info structure
 *
 * Args:
 *	up - Pointer to the user info structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_userinfo(cddb_userinfo_t *up)
{
	if (up->userhandle != NULL) {
		MEM_FREE(up->userhandle);
		up->userhandle = NULL;
	}
	if (up->password != NULL) {
		MEM_FREE(up->password);
		up->password = NULL;
	}
	if (up->passwordhint != NULL) {
		MEM_FREE(up->passwordhint);
		up->passwordhint = NULL;
	}
	if (up->emailaddress != NULL) {
		MEM_FREE(up->emailaddress);
		up->emailaddress = NULL;
	}
	if (up->regionid != NULL) {
		MEM_FREE(up->regionid);
		up->regionid = NULL;
	}
	if (up->postalcode != NULL) {
		MEM_FREE(up->postalcode);
		up->postalcode = NULL;
	}
	if (up->age != NULL) {
		MEM_FREE(up->age);
		up->age = NULL;
	}
	if (up->sex != NULL) {
		MEM_FREE(up->sex);
		up->sex = NULL;
	}
	up->allowemail = 0;
	up->allowstats = 0;
}


/*
 * fcddb_clear_options
 *	Clear a options structure
 *
 * Args:
 *	up - Pointer to the options structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_options(cddb_options_t *op)
{
	if (op->proxyserver != NULL) {
		MEM_FREE(op->proxyserver);
		op->proxyserver = NULL;
	}
	if (op->proxyusername != NULL) {
		MEM_FREE(op->proxyusername);
		op->proxyusername = NULL;
	}
	if (op->proxypassword != NULL) {
		MEM_FREE(op->proxypassword);
		op->proxypassword = NULL;
	}
	if (op->localcachepath != NULL) {
		MEM_FREE(op->localcachepath);
		op->localcachepath = NULL;
	}
	op->proxyport = 0;
	op->servertimeout = 0;
	op->testsubmitmode = 0;
	op->localcachetimeout = 0;
}


/*
 * fcddb_clear_track
 *	Clear a track structure
 *
 * Args:
 *	up - Pointer to the track structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_track(cddb_track_t *tp)
{
	fcddb_clear_credits(&tp->credits);

	if (tp->title != NULL) {
		MEM_FREE(tp->title);
		tp->title = NULL;
	}
	if (tp->notes != NULL) {
		MEM_FREE(tp->notes);
		tp->notes = NULL;
	}
	tp->control = NULL;
}


/*
 * fcddb_clear_tracks
 *	Clear a tracks structure
 *
 * Args:
 *	up - Pointer to the tracks structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_tracks(cddb_tracks_t *tsp)
{
	int	i;

	for (i = 0; i < MAXTRACK; i++)
		fcddb_clear_track(&tsp->track[i]);

	tsp->count = 0;
}


/*
 * fcddb_clear_disc
 *	Clear a disc structure
 *
 * Args:
 *	up - Pointer to the disc structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_disc(cddb_disc_t *dp)
{
	fcddb_idlist_t	*ip,
			*nextp;

	fcddb_clear_credits(&dp->credits);
	fcddb_clear_segments(&dp->segments);
	fcddb_clear_fullname(&dp->fullname);
	fcddb_clear_tracks(&dp->tracks);

	if (dp->category != NULL) {
		MEM_FREE(dp->category);
		dp->category = NULL;
	}
	if (dp->discid != NULL) {
		MEM_FREE(dp->discid);
		dp->discid = NULL;
	}
	if (dp->toc != NULL) {
		MEM_FREE(dp->toc);
		dp->toc = NULL;
	}
	if (dp->title != NULL) {
		MEM_FREE(dp->title);
		dp->title = NULL;
	}
	if (dp->notes != NULL) {
		MEM_FREE(dp->notes);
		dp->notes = NULL;
	}
	if (dp->revision != NULL) {
		MEM_FREE(dp->revision);
		dp->revision = NULL;
	}
	if (dp->submitter != NULL) {
		MEM_FREE(dp->submitter);
		dp->submitter = NULL;
	}

	dp->next = NULL;
	dp->genre = NULL;
	dp->control = NULL;

	nextp = NULL;
	for (ip = dp->idlist; ip != NULL; ip = nextp) {
		nextp = ip->next;
		MEM_FREE(ip);
	}
	dp->idlist = NULL;
}


/*
 * fcddb_clear_discs
 *	Clear a discs structure
 *
 * Args:
 *	up - Pointer to the discs structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_discs(cddb_discs_t *dsp)
{
	cddb_disc_t	*dp,
			*nextp;

	nextp = NULL;
	for (dp = dsp->discs; dp != NULL; dp = nextp) {
		nextp = dp->next;
		fcddb_clear_disc(dp);
		MEM_FREE(dp);
	}

	dsp->discs = NULL;
	dsp->count = 0;
}


/*
 * fcddb_clear_control
 *	Clear a control structure
 *
 * Args:
 *	up - Pointer to the control structure
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_clear_control(cddb_control_t *cp)
{
	if (cp->hostname != NULL) {
		MEM_FREE(cp->hostname);
		cp->hostname = NULL;
	}
	if (cp->clientname != NULL) {
		MEM_FREE(cp->clientname);
		cp->clientname = NULL;
	}
	if (cp->clientid != NULL) {
		MEM_FREE(cp->clientid);
		cp->clientid = NULL;
	}
	if (cp->clientver != NULL) {
		MEM_FREE(cp->clientver);
		cp->clientver = NULL;
	}
	fcddb_clear_userinfo(&cp->userinfo);
	fcddb_clear_options(&cp->options);
	fcddb_clear_disc(&cp->disc);
	fcddb_clear_discs(&cp->discs);
	fcddb_clear_genretree(&cp->genretree);
	fcddb_clear_roletree(&cp->roletree);
	fcddb_clear_urllist(&cp->urllist);
}


/*
 * fcddb_putentry
 *	Write an entry into a CDDB file, used by fcddb_write_cddb()
 *
 * Args:
 *	fp - FILE stream pointer
 *	idstr - The entry identifier
 *	entry - The entry string 
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_putentry(FILE *fp, char *idstr, char *entry)
{
	int	i,
		n;
	char	*cp,
		tmpbuf[STR_BUF_SZ];

	if (entry == NULL) {
		/* Null entry */
		(void) sprintf(tmpbuf, "%s=\r\n", idstr);
		(void) fputs(tmpbuf, fp);
	}
	else {
		/* Write entry to file, splitting into multiple lines
		 * if necessary.  Special handling for newline and tab
		 * characters.
		 */
		cp = entry;

		do {
			(void) sprintf(tmpbuf, "%s=", idstr);
			(void) fputs(tmpbuf, fp);

			n = FCDDB2_MIN((int) strlen(cp), STR_BUF_SZ);

			for (i = 0; i < n; i++, cp++) {
				switch (*cp) {
				case '\n':
					(void) fputs("\\n", fp);
					break;
				case '\t':
					(void) fputs("\\t", fp);
					break;
				case '\\':
					(void) fputs("\\\\", fp);
					break;
				default:
					(void) fputc(*cp, fp);
					break;
				}
			}

			(void) fputs("\r\n", fp);
			if (*cp == '\0')
				break;

		} while (n == STR_BUF_SZ);
	}
}


/*
 * fcddb_write_cddb
 *	Write a CDDB file
 *
 * Args:
 *	path - file path
 *	dp - Pointer to disc structure
 *	issubmit - Whether this function is invoked during a submit
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_write_cddb(char *path, cddb_disc_t *dp, bool_t issubmit)
{
	cddb_control_t	*cp = (cddb_control_t *) dp->control;
	FILE		*fp;
	CddbResult	ret;
	int		i,
			revision,
			ntrks;
	unsigned int	discid_val = 0;
	fcddb_idlist_t	*ip;
	char		*dir,
			*up,
			idstr[12],
			tmpbuf[STR_BUF_SZ * 2];
	unsigned int	addr[MAXTRACK];

	if (cp == NULL)
		return Cddb_E_INVALIDARG;

	(void) sscanf(dp->discid, "%x", (unsigned int *) &discid_val);

	dir = fcddb_dirname(path);
	FCDDBDBG(fcddb_errfp, "fcddb_write_cddb: making %s\n", dir);
	if ((ret = fcddb_mkdir(dir, 0755)) != Cddb_OK)
		return (ret);

	(void) unlink(path);

	FCDDBDBG(fcddb_errfp, "fcddb_write_cddb: writing %s\n", path);
	if ((fp = fopen(path, "w")) == NULL) {
		FCDDBDBG(fcddb_errfp,
			"fcddb_write_cddb: cannot open file for writing: "
			"errno=%d\n", errno);
		return CDDBTRNCannotCreateFile;
	}

	/* File header */
	(void) fputs(
		"# xmcd CD database file\r\n"
		"# Copyright (C) 2001 CDDB, Inc.\r\n#\r\n",
		fp
	);

	ntrks = fcddb_parse_toc((char *) dp->toc, addr);
	if (ntrks == 0 || ntrks != (int) (discid_val & 0xff)) {
		(void) fclose(fp);
		(void) unlink(path);
		return Cddb_E_INVALIDARG;
	}

	/* Track frame offsets */
	(void) fputs("# Track frame offsets:\r\n", fp);
	for (i = 0; i < ntrks; i++) {
		(void) sprintf(tmpbuf, "#\t%u\r\n", addr[i]);
		(void) fputs(tmpbuf, fp);
	}

	/* Disc length */
	(void) sprintf(tmpbuf, "#\r\n# Disc length: %u seconds\r\n#\r\n",
		       addr[ntrks] / FRAME_PER_SEC);
	(void) fputs(tmpbuf, fp);

	/* Revision */
	if (dp->revision == NULL)
		revision = 1;
	else {
		revision = atoi((char *) dp->revision);
		if (revision >= 0 && issubmit)
			revision++;
	}
	(void) sprintf(tmpbuf, "# Revision: %d\r\n", revision);
	(void) fputs(tmpbuf, fp);

	/* Submitter */
	if (issubmit || dp->submitter == NULL)
		(void) sprintf(tmpbuf, "# Submitted via: %s %s\r\n#\r\n",
			       cp->clientname, cp->clientver);
	else
		(void) sprintf(tmpbuf, "# Submitted via: %s\r\n#\r\n",
			       dp->submitter);
	(void) fputs(tmpbuf, fp);

	/* Disc IDs */
	(void) sprintf(tmpbuf, "DISCID=%s", dp->discid);
	i = 1;
	for (ip = dp->idlist; ip != NULL; ip = ip->next) {
		if (ip->discid == discid_val)
			/* This is our own ID, which we already processed */
			continue;

		if (i == 0)
			(void) sprintf(tmpbuf, "DISCID=%08x", ip->discid);
		else
			(void) sprintf(tmpbuf, "%s,%08x", tmpbuf, ip->discid);

		i++;
		if (i == 8) {
			(void) strcat(tmpbuf, "\r\n");
			(void) fputs(tmpbuf, fp);
			i = 0;
		}
	}
	if (i != 0) {
		(void) strcat(tmpbuf, "\r\n");
		(void) fputs(tmpbuf, fp);
	}

	/* Disc artist/title */
	(void) sprintf(tmpbuf, "%s%s%s",
		       (dp->fullname.name == NULL) ?
				"" : dp->fullname.name,
		       (dp->fullname.name != NULL && dp->title != NULL) ?
				" / " : "",
		       (dp->title == NULL) ?
				"" : dp->title);
	if ((up = fcddb_utf8_to_iso8859(tmpbuf)) == NULL) {
		(void) fclose(fp);
		(void) unlink(path);
		return CDDBTRNOutOfMemory;
	}
	fcddb_putentry(fp, "DTITLE", up);
	MEM_FREE(up);

	/* Track titles */
	for (i = 0; i < ntrks; i++) {
		(void) sprintf(idstr, "TTITLE%u", i);
		up = fcddb_utf8_to_iso8859(dp->tracks.track[i].title);
		if (up == NULL) {
			(void) fclose(fp);
			(void) unlink(path);
			return CDDBTRNOutOfMemory;
		}
		fcddb_putentry(fp, idstr, up);
		MEM_FREE(up);
	}

	/* Disc notes */
	if ((up = fcddb_utf8_to_iso8859(dp->notes)) == NULL) {
		(void) fclose(fp);
		(void) unlink(path);
		return CDDBTRNOutOfMemory;
	}
	fcddb_putentry(fp, "EXTD", up);
	MEM_FREE(up);

	/* Track notes */
	for (i = 0; i < ntrks; i++) {
		up = fcddb_utf8_to_iso8859(dp->tracks.track[i].notes);
		if (up == NULL) {
			(void) fclose(fp);
			(void) unlink(path);
			return CDDBTRNOutOfMemory;
		}
		(void) sprintf(idstr, "EXTT%u", i);
		fcddb_putentry(fp, idstr, up);
		MEM_FREE(up);
	}

	/* Track program sequence */
	fcddb_putentry(fp, "PLAYORDER", NULL);

	(void) fclose(fp);

	return Cddb_OK;
}


/*
 * fcddb_parse_cddb_data
 *	Parse CDDB data and fill the disc structure
 *
 * Args:
 *	cp - Pointer to the control structure
 *	buf - CDDB data
 *	discid - The disc ID
 *	category - The category
 *	toc - The TOC
 *	wrcache - Whether to write to cache
 *	dp - Pointer to disc structure
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_parse_cddb_data(
	cddb_control_t	*cp,
	char		*buf,
	char		*discid,
	char		*category,
	char		*toc,
	bool_t		wrcache,
	cddb_disc_t	*dp
)
{
	char		*p,
			*q,
			*r,
			*up,
			filepath[FILE_PATH_SZ];
	fcddb_idlist_t	*ip;
	int		n;
	unsigned int	discid_val = 0;
	CddbResult	ret;

	FCDDBDBG(fcddb_errfp, "fcddb_parse_cddb_data: %s/%s\n",
		 category, discid);

	ret = Cddb_OK;
	p = buf;
	for (;;) {
		SKIP_SPC(p);

		if (*p == '.' || *p == '\0')
			break;			/* Done */

		if ((q = strchr(p, '\n')) != NULL)
			*q = '\0';

		n = strlen(p) - 1;
		if (*(p + n) == '\r')
			*(p + n) = '\0';	/* Zap carriage return */

		if (*p == '#') {		/* Comment */
			if (strncmp(p, "# Revision: ", 12) == 0)
				dp->revision = fcddb_strdup(p + 12);
			else if (strncmp(p, "# Submitted via: ", 17) == 0)
				dp->submitter = fcddb_strdup(p + 17);
		}
		else if (strncmp(p, "DISCID=", 7) == 0) {
			p += 7;
			while ((r = strchr(p, ',')) != NULL) {
				*r = '\0';
				ip = (fcddb_idlist_t *) MEM_ALLOC(
					"idlist",
					sizeof(fcddb_idlist_t)
				);
				if (ip == NULL) {
					ret = CDDBTRNOutOfMemory;
					break;
				}
				(void) sscanf(p, "%x",
					      (unsigned int *) &ip->discid);
				discid_val=ip->discid;	
				ip->next = dp->idlist;
				dp->idlist = ip;
				p = r+1;
			}
			if (ret != Cddb_OK)
				break;

			ip = (fcddb_idlist_t *) MEM_ALLOC(
				"idlist",
				sizeof(fcddb_idlist_t)
			);
			if (ip == NULL) {
				ret = CDDBTRNOutOfMemory;
				break;
			}
			(void) sscanf(p, "%x", (unsigned int *) &ip->discid);
			discid_val=ip->discid;
			ip->next = dp->idlist;
			dp->idlist = ip;
			discid=p;
		}
		else if (strncmp(p, "DTITLE=", 7) == 0) {
			/* Disc artist / title */
			p += 7;

			fcddb_line_filter(p);
			if ((up = fcddb_iso8859_to_utf8(p)) == NULL) {
				ret = CDDBTRNOutOfMemory;
				break;
			}

			/* Put it all in artist field first,
			 * we'll separate artist and title later.
			 */
			if (dp->fullname.name == NULL) {
				dp->fullname.name = (char *) MEM_ALLOC(
					"dp->fullname.name",
					strlen(up) + 1
				);
				if (dp->fullname.name != NULL)
					dp->fullname.name[0] = '\0';
			}
			else {
				dp->fullname.name = (char *) MEM_REALLOC(
					"dp->fullname.name",
					dp->fullname.name,
					strlen(dp->fullname.name) +
						strlen(up) + 1
				);
			}

			if (dp->fullname.name == NULL) {
				MEM_FREE(up);
				ret = CDDBTRNOutOfMemory;
				break;
			}

			(void) fcddb_strcat(dp->fullname.name, up);
			MEM_FREE(up);
		}
		else if (sscanf(p, "TTITLE%d=", &n) > 0) {
			/* Track title */
			p = strchr(p, '=');
			p++;

			if (n >= (int) (discid_val & 0xff))
				continue;

			fcddb_line_filter(p);
			if ((up = fcddb_iso8859_to_utf8(p)) == NULL) {
				ret = CDDBTRNOutOfMemory;
				break;
			}

			if (dp->tracks.track[n].title == NULL) {
				dp->tracks.track[n].title = (char *) MEM_ALLOC(
					"track[n].title",
					strlen(up) + 1
				);
				if (dp->tracks.track[n].title != NULL)
					dp->tracks.track[n].title[0] = '\0';
			}
			else {
				dp->tracks.track[n].title = (char *)
				    MEM_REALLOC(
					"track[n].title",
					dp->tracks.track[n].title,
					strlen(dp->tracks.track[n].title) +
						strlen(up) + 1
				    );
			}

			if (dp->tracks.track[n].title == NULL) {
				MEM_FREE(up);
				ret = CDDBTRNOutOfMemory;
				break;
			}

			(void) fcddb_strcat(dp->tracks.track[n].title, up);
			MEM_FREE(up);
		}
		else if (strncmp(p, "EXTD=", 5) == 0) {
			/* Disc notes */
			p += 5;

			if ((up = fcddb_iso8859_to_utf8(p)) == NULL) {
				ret = CDDBTRNOutOfMemory;
				break;
			}

			if (dp->notes == NULL) {
				dp->notes = (char *) MEM_ALLOC(
					"dp->notes",
					strlen(up) + 1
				);
				if (dp->notes != NULL)
					dp->notes[0] = '\0';
			}
			else {
				dp->notes = (char *) MEM_REALLOC(
					"dp->notes",
					dp->notes,
					strlen(dp->notes) +
						strlen(up) + 1
				);
			}

			if (dp->notes == NULL) {
				MEM_FREE(up);
				ret = CDDBTRNOutOfMemory;
				break;
			}

			(void) fcddb_strcat(dp->notes, up);
			MEM_FREE(up);
		}
		else if (sscanf(p, "EXTT%d=", &n) > 0) {
			/* Track notes */
			p = strchr(p, '=');
			p++;

			if (n >= (int) (discid_val & 0xff))
				continue;

			if ((up = fcddb_iso8859_to_utf8(p)) == NULL) {
				ret = CDDBTRNOutOfMemory;
				break;
			}

			if (dp->tracks.track[n].notes == NULL) {
				dp->tracks.track[n].notes = (char *)
				    MEM_ALLOC(
					"track[n].notes",
					strlen(up) + 1
				    );

				if (dp->tracks.track[n].notes != NULL)
					dp->tracks.track[n].notes[0] = '\0';
			}
			else {
				dp->tracks.track[n].notes = (char *)
				    MEM_REALLOC(
					"track[n].notes",
					dp->tracks.track[n].notes,
					strlen(dp->tracks.track[n].notes) +
						strlen(up) + 1
				    );
			}

			if (dp->tracks.track[n].notes == NULL) {
				MEM_FREE(up);
				ret = CDDBTRNOutOfMemory;
				break;
			}

			(void) fcddb_strcat(dp->tracks.track[n].notes, up);
			MEM_FREE(up);
		}

		p = q + 1;
	}

	if (ret != Cddb_OK)
		return (ret);

	/* Separate into disc artist & title fields */
	if ((p = strchr(dp->fullname.name, '/')) != NULL &&
	     p > (dp->fullname.name + 1) &&
	    *(p-1) == ' ' && *(p+1) == ' ' && *(p+2) != '\0') {
		q = dp->fullname.name;

		/* Artist */
		*(p-1) = '\0';
		dp->fullname.name = (CddbStr) fcddb_strdup(q);

		/* Title */
		dp->title = (CddbStr) fcddb_strdup(p+2);

		MEM_FREE(q);
	}
	else {
		/* Move the whole string to title */
		dp->title = dp->fullname.name;
		dp->fullname.name = NULL;
	}

	/* Set up other fields */
	dp->control = (void *) cp;
	dp->discid = fcddb_strdup(discid);
	dp->toc = (CddbStr) fcddb_strdup(toc);
	dp->tracks.count = (long) (discid_val & 0xff);
	dp->category = fcddb_strdup(category);
	dp->genre = fcddb_genre_categ2gp(cp, category);

	if (wrcache) {
#ifdef __VMS
		(void) sprintf(filepath, "%s.%s]%s.",
			       cp->options.localcachepath, category, discid);
#else
		(void) sprintf(filepath, "%s/%s/%s",
			       cp->options.localcachepath, category, discid);
#endif

		(void) fcddb_write_cddb(filepath, dp, FALSE);
	}

	return Cddb_OK;
}


/*
 * fcddb_connect
 *	Make a network connection to the server
 *
 * Args:
 *	host - Server host name
 *	port - Server port
 *	fd - Return file descriptor
 *
 * Return:
 *	CddbResult status code
 */
#ifdef NOREMOTE
/*ARGSUSED*/
#endif
STATIC CddbResult
fcddb_connect(char *host, unsigned short port, int *fd)
{
#ifndef NOREMOTE
	struct hostent		*hp;
	struct in_addr		ad;
	struct sockaddr_in	sin;

	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;

	FCDDBDBG(fcddb_errfp, "fcddb_connect: %s:%d\n", host, (int) port);
	/* Find server host address */
	if ((hp = gethostbyname(host)) != NULL) {
		(void) memcpy((char *) &sin.sin_addr, hp->h_addr,
			      hp->h_length);
	}
	else {
		if ((ad.s_addr = inet_addr(host)) != (unsigned long) -1)
			(void) memcpy((char *) &sin.sin_addr,
				      (char *) &ad.s_addr, sizeof(ad.s_addr));
		else
			return CDDBTRNHostNotFound;
	}

	/* Open socket */
	if ((*fd = SOCKET(AF_INET, SOCK_STREAM, 0)) < 0) {
		*fd = -1;
		return ((CddbResult) (errno == EINTR ?
			CDDBTRNServerTimeout : CDDBTRNSockCreateErr));
	}

	/* Connect to server */
	if (CONNECT(*fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		(void) close(*fd);
		*fd = -1;
		return ((CddbResult) (errno == EINTR ?
			CDDBTRNServerTimeout : CDDBTRNSockOpenErr));
	}

	return Cddb_OK;
#else
	return CDDBCTLDisabled;
#endif	/* NOREMOTE */
}


/*
 * fcddb_sendcmd
 *	Send a command to the server, and get server response
 *
 * Args:
 *	fd - file descriptor returned by fcddb_connect()
 *	buf - command and response buffer (this function may reallocate
 *	      the buffer depending on response data size)
 *	buflen - The buffer size
 *	isproxy - Whether we're using a proxy server
 *
 * Return:
 *	CddbResult status code
 */
#ifdef NOREMOTE
/*ARGSUSED*/
#endif
STATIC CddbResult
fcddb_sendcmd(int fd, char **buf, size_t *buflen, bool_t isproxy)
{
#ifndef NOREMOTE
	int	len,
		tot,
		ret,
		rbuflen;
	char	*p;

	p = *buf;
	len = strlen(p);
	tot = 0;

	/* Send command to server */
	FCDDBDBG(fcddb_errfp,
		 "fcddb_sendcmd: Sending command:\n------\n%s\n------\n",
		 *buf);

	while (len > 0) {
		if ((ret = send(fd, p, len, 0)) < 0) {
			return ((CddbResult) (errno == EINTR ?
				CDDBTRNServerTimeout : CDDBTRNSendFailed));
		}

		p += ret;
		tot += ret;
		len -= ret;
	}

	if (tot == 0)
		return CDDBTRNSendFailed;

	/* Read server response to command */
	p = *buf;
	rbuflen = *buflen;

	for (;;) {
		/* Check remaining buffer size, enlarge if needed */
		if (rbuflen <= 64) {
			int	offset = p - *buf;

			*buf = (char *) MEM_REALLOC(
				"http_buf",
				*buf,
				*buflen + CDDBP_CMDLEN
			);
			if (*buf == NULL)
				return CDDBTRNOutOfMemory;

			rbuflen += CDDBP_CMDLEN;
			*buflen += CDDBP_CMDLEN;
			p = *buf + offset;
		}

		/* Receive response from server */
		if ((len = recv(fd, p, rbuflen-1, 0)) < 0) {
			return ((CddbResult) (errno == EINTR ?
				CDDBTRNServerTimeout : CDDBTRNRecvFailed));
		}

		*(p + len) = '\0';	/* Mark end of data */
		if (len == 0)
			break;

		rbuflen -= len;
		p += len;
	}

	FCDDBDBG(fcddb_errfp,
		 "fcddb_sendcmd: Server response:\n------\n%s\n------\n",
		 *buf);

	/* Check for proxy authorization failure */
	if (isproxy && strncmp(*buf, "HTTP/", 5) == 0) {
		p = strchr((*buf)+5, ' ');
		if (p != NULL && isdigit((int) *(p+1)) &&
		    (atoi(p+1) == HTTP_PROXYAUTH_FAIL)) {
			/* Need proxy authorization */
			return CDDBTRNHTTPProxyError;
		}
	}

	return Cddb_OK;
#else
	return CDDBCTLDisabled;
#endif	/* NOREMOTE */
}


/*
 * fcddb_disconnect
 *	Disconnect from a server
 *
 * Args:
 *	fd - File descriptor returned from fcddb_connect()
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_disconnect(int fd)
{
	(void) close(fd);
}


/*
 * fcddb_query_cddb
 *	Send a query command to the server
 *
 * Args:
 *	cp - Pointer to the control structure
 *	discid - The disc ID
 *	toc - The TOC
 *	addr - Array of track offset frames
 *	conhost - Server host
 *	conport - Server port
 *	isproxy - Whether we're using a proxy server
 *	category - Return category string
 *	matchcode - Return match code
 *
 * Return:
 *	CddbResult status code
 */
#ifdef NOREMOTE
/*ARGSUSED*/
#endif
STATIC CddbResult
fcddb_query_cddb(
	cddb_control_t		*cp,
	char			*discid,
	char			*toc,
	unsigned int		*addr,
	char			*conhost,
	unsigned short		conport,
	bool_t			isproxy,
	char			*category,
	int			*matchcode
)
{
	int		i;
	unsigned int	discid_val = 0;
	size_t		buflen;
	FILE		*fp;
	fcddb_gmap_t	*gmp;
	cddb_disc_t	*dp;
	char		*buf,
			*p,
			*q,
			*r,
			*s,
			*categp = NULL,
			filepath[FILE_PATH_SZ];
	time_t		t;
	struct stat	stbuf;
#ifndef NOREMOTE
	CddbResult	ret;
	int		fd,
			ntrks;
	char		urlstr[HOST_NAM_SZ + FILE_PATH_SZ + 12];
	bool_t		valid_resp;
#endif

	fcddb_clear_discs(&cp->discs);

	(void) sscanf(discid, "%x", (unsigned int *) &discid_val);

	buflen = CDDBP_CMDLEN;
	if ((buf = (char *) MEM_ALLOC("http_buf", buflen)) == NULL) {
		*matchcode = MATCH_NONE;
		return CDDBTRNOutOfMemory;
	}

	t = time(NULL);

	/* Try to locate the entry in the local cache */
	for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
#ifdef __VMS
		(void) sprintf(filepath, "%s.%s]%s.",
			       cp->options.localcachepath,
			       gmp->cddb1name, discid);
#else
		(void) sprintf(filepath, "%s/%s/%s",
			       cp->options.localcachepath,
			       gmp->cddb1name, discid);
#endif

		if (stat(filepath, &stbuf) < 0 || !S_ISREG(stbuf.st_mode))
			continue;

#ifndef NOREMOTE
		if ((cp->options.localcacheflags & CACHE_DONT_CONNECT) == 0 &&
		    (t - stbuf.st_mtime) >
			(int) (cp->options.localcachetimeout * SECS_PER_DAY)) {

			/* If any potential cached match is expired,
			 * force a server query.
			 */
			fcddb_clear_discs(&cp->discs);
			break;
		}
#endif

		fp = NULL;
		if ((fp = fopen(filepath, "r")) != NULL &&
		    fgets(buf, buflen, fp) != NULL &&
		    strncmp(buf, "# xmcd", 6) == 0) {

			/* Don't use fcddb_obj_alloc here because
			 * we don't want CddbReleaseObject to
			 * free it up.
			 */
			dp = (cddb_disc_t *) MEM_ALLOC(
				"CddbDisc", sizeof(cddb_disc_t)
			);
			if (dp == NULL) {
				MEM_FREE(buf);
				*matchcode = MATCH_NONE;
				return CDDBTRNOutOfMemory;
			}
			(void) memset(dp, 0, sizeof(cddb_disc_t));
			(void) strcpy(dp->objtype, "CddbDisc");

			/* Category: save and convert to genre */
			dp->category = categp = fcddb_strdup(gmp->cddb1name);
			dp->genre = fcddb_genre_categ2gp(cp, gmp->cddb1name);

			/* Disc ID */
			dp->discid = fcddb_strdup(discid);

			/* Artist/title */
			while (fgets(buf, buflen, fp) != NULL) {
				if (buf[0] == '#' ||
				    strncmp(buf, "DTITLE=", 7) != 0)
					continue;

				p = buf + 7;
				fcddb_line_filter(p);

				if ((q = strchr(p, '\r')) != NULL ||
				    (q = strchr(p, '\n')) != NULL)
					*q = '\0';

				/* Put it all in artist first */
				if (dp->fullname.name == NULL) {
					dp->fullname.name = (char *)
					MEM_ALLOC(
						"dp->fullname.name",
						strlen(p) + 1
					);
					if (dp->fullname.name != NULL)
						dp->fullname.name[0] = '\0';
				}
				else {
					dp->fullname.name = (char *)
					MEM_REALLOC(
						"dp->fullname.name",
						dp->fullname.name,
						strlen(dp->fullname.name) +
							strlen(p) + 1
					);
				}

				if (dp->fullname.name == NULL) {
					MEM_FREE(buf);
					*matchcode = MATCH_NONE;
					return CDDBTRNOutOfMemory;
				}

				(void) fcddb_strcat(dp->fullname.name, p);
			}

			/* Separate into disc artist & title fields */
			r = strchr(dp->fullname.name, '/');
			if (r != NULL && r > (dp->fullname.name + 1) &&
			    *(r-1) == ' ' && *(r+1) == ' '
			    && *(r+2) != '\0') {
				s = dp->fullname.name;

				/* Artist */
				*(r-1) = '\0';
				dp->fullname.name = (CddbStr) fcddb_strdup(s);

				/* Title */
				dp->title = (CddbStr) fcddb_strdup(r+2);

				MEM_FREE(s);
			}
			else {
				/* Move the whole string to title */
				dp->title = dp->fullname.name;
				dp->fullname.name = NULL;
			}

			/* TOC */
			dp->toc = (CddbStr) fcddb_strdup(toc);

			/* Add to discs list */
			dp->next = cp->discs.discs;
			cp->discs.discs = dp;
			cp->discs.count++;
		}

		if (fp != NULL)
			(void) fclose(fp);
	}

	if (cp->discs.count == 1) {
		/* Matched one local cache entry */
		*matchcode = MATCH_EXACT;
		(void) strcpy(category, categp);
		fcddb_clear_discs(&cp->discs);
		MEM_FREE(buf);

		FCDDBDBG(fcddb_errfp,
			 "\nfcddb_query_cddb: Exact match in cache %s/%s\n",
			 category, discid);
		return Cddb_OK;
	}
	else if (cp->discs.count > 1) {
		/* Matched multiple local cache entries */
		*matchcode = MATCH_MULTIPLE;
		MEM_FREE(buf);

		FCDDBDBG(fcddb_errfp,
			 "\nfcddb_query_cddb: Multiple matches in cache\n");
		return Cddb_OK;
	}

#ifdef NOREMOTE
	*matchcode = MATCH_NONE;
	MEM_FREE(buf);

	FCDDBDBG(fcddb_errfp, "\nfcddb_query_cddb: No match in cache\n");
	return Cddb_OK;
#else
	if ((cp->options.localcacheflags & CACHE_DONT_CONNECT) != 0) {
		*matchcode = MATCH_NONE;
		MEM_FREE(buf);

		FCDDBDBG(fcddb_errfp,
			"\nfcddb_query_cddb: No match in cache\n");
		return Cddb_OK;
	}

	/*
	 * Set up CDDB query command string
	 */

	FCDDBDBG(fcddb_errfp, "\nfcddb_query_cddb: server query\n");

	ntrks = (int) (discid_val & 0xff);
	urlstr[0] = '\0';

	if (isproxy) {
		(void) sprintf(urlstr, "http://%s:%d",
			       CDDB_SERVER_HOST, HTTP_PORT);
	}

	(void) strcat(urlstr, CDDB_CGI_PATH);

	(void) sprintf(buf, "GET %s?cmd=cddb+query+%s+%d",
		       urlstr, discid, ntrks);

	for (i = 0; i < ntrks; i++)
		(void) sprintf(buf, "%s+%u", buf, addr[i]);

	(void) sprintf(buf, "%s+%u&%s&proto=4 HTTP/1.0\r\n", buf,
		       addr[ntrks] / FRAME_PER_SEC,
		       fcddb_hellostr);

	if (isproxy && fcddb_auth_buf != NULL) {
		(void) sprintf(buf, "%sProxy-Authorization: Basic %s\r\n", buf,
			       fcddb_auth_buf);
	}

	(void) sprintf(buf, "%s%s%s\r\n%s\r\n", buf,
		       "Host: ", CDDB_SERVER_HOST,
		       fcddb_extinfo);

	/* Open network connection to server */
	if ((ret = fcddb_connect(conhost, conport, &fd)) != Cddb_OK) {
		if (buf != NULL)
			MEM_FREE(buf);
		*matchcode = MATCH_NONE;
		return (ret);
	}

	/*
	 * Send the command to the CDDB server and get response code
	 */
	if ((ret = fcddb_sendcmd(fd, &buf, &buflen, isproxy)) != Cddb_OK) {
		if (buf != NULL)
			MEM_FREE(buf);
		*matchcode = MATCH_NONE;
		return (ret);
	}

	/* Close server connection */
	fcddb_disconnect(fd);

	/* Check for CDDB command response */
	valid_resp = FALSE;
	p = buf;
	while ((q = strchr(p, '\n')) != NULL) {
		*q = '\0';
	
		if (STATCODE_CHECK(p)) {
			valid_resp = TRUE;
			*q = '\n';
			break;
		}

		*q = '\n';
		p = q + 1;
		SKIP_SPC(p);
	}

	if (!valid_resp) {
		/* Server error */
		return CDDBTRNBadResponseSyntax;
	}

	/*
	 * Check command status
	 */

	switch (STATCODE_1ST(p)) {
	case STAT_GEN_OK:
	case STAT_GEN_OKCONT:
		switch (STATCODE_3RD(p)) {
		case STAT_QURY_EXACT:
			if (STATCODE_2ND(p) == STAT_SUB_READY) {
				/* Single exact match */
				p += 4;
				SKIP_SPC(p);

				if (*p == '\0' ||
				    (q = strchr(p, ' ')) == NULL) {
					/* Invalid server response */
					ret = CDDBTRNBadResponseSyntax;
					break;
				}
				*q = '\0';

				/* Get category */
				(void) strncpy(category, p,
						FILE_BASE_SZ - 1);
				category[FILE_BASE_SZ - 1] = '\0';

				*q = ' ';

				*matchcode = MATCH_EXACT;
				ret = Cddb_OK;
				break;
			}
			/*FALLTHROUGH*/

		case STAT_QURY_INEXACT:
			if (STATCODE_2ND(p) != STAT_SUB_OUTPUT) {
				/* Invalid server response */
				ret = CDDBTRNBadResponseSyntax;
				break;
			}

			ret = Cddb_OK;

			p += 4;
			SKIP_SPC(p);
			if (*p == '\0' || (q = strchr(p, '\n')) == NULL) {
				/* Invalid server response */
				ret = CDDBTRNBadResponseSyntax;
				break;
			}

			for (;;) {
				p = q + 1;
				SKIP_SPC(p);
				if ((q = strchr(p, '\n')) != NULL)
					*q = '\0';

				if (*p == '.' || *p == '\0')
					/* Done */
					break;

				/* Zap carriage return if needed */
				i = strlen(p) - 1;
				if (*(p+i) == '\r')
					*(p+i) = '\0';

				/* Don't use fcddb_obj_alloc here because
				 * we don't want CddbReleaseObject to
				 * free it up.
				 */
				dp = (cddb_disc_t *) MEM_ALLOC(
					"CddbDisc", sizeof(cddb_disc_t)
				);
				if (dp == NULL) {
					*matchcode = MATCH_NONE;
					return CDDBTRNOutOfMemory;
				}
				(void) memset(dp, 0, sizeof(cddb_disc_t));
				(void) strcpy(dp->objtype, "CddbDisc");

				r = p;
				if ((s = strchr(r, ' ')) == NULL) {
					/* Invalid server response */
					MEM_FREE(dp);
					*matchcode = MATCH_NONE;
					return CDDBTRNBadResponseSyntax;
				}
				*s = '\0';

				/* If the server returned an unrecognized
				 * category, add it to the table.
				 */
				if (fcddb_genre_categ2id(r) == NULL)
					fcddb_genre_addcateg(r);

				/* Category: save and convert to genre */
				dp->category = fcddb_strdup(r);
				dp->genre = fcddb_genre_categ2gp(cp, r);

				*s = ' ';
				r = s + 1;
				SKIP_SPC(r);
				if ((s = strchr(r, ' ')) == NULL) {
					/* Invalid server response */
					MEM_FREE(dp);
					*matchcode = MATCH_NONE;
					return CDDBTRNBadResponseSyntax;
				}
				*s = '\0';

				/* Disc ID */
				dp->discid = fcddb_strdup(r);

				*s = ' ';
				r = s + 1;
				SKIP_SPC(r);
				if (*r == '\0') {
					/* Invalid server response */
					MEM_FREE(dp);
					*matchcode = MATCH_NONE;
					return CDDBTRNBadResponseSyntax;
				}

				/* Artist / Title */

				/* Put it all in artist first */
				dp->fullname.name = (CddbStr) fcddb_strdup(r);

				/* Separate into disc artist & title fields */
				r = strchr(dp->fullname.name, '/');
				if (r != NULL && r > (dp->fullname.name + 1) &&
				    *(r-1) == ' ' && *(r+1) == ' '
				    && *(r+2) != '\0') {
					s = dp->fullname.name;

					/* Artist */
					*(r-1) = '\0';
					dp->fullname.name =
						(CddbStr) fcddb_strdup(s);

					/* Title */
					dp->title =
						(CddbStr) fcddb_strdup(r+2);

					MEM_FREE(s);
				}
				else {
					/* Move the whole string to title */
					dp->title = dp->fullname.name;
					dp->fullname.name = NULL;
				}

				/* TOC */
				dp->toc = (CddbStr) fcddb_strdup(toc);

				if (dp != NULL) {
					/* Add to discs list */
					dp->next = cp->discs.discs;
					cp->discs.discs = dp;
					cp->discs.count++;
				}
			}

			*matchcode = MATCH_MULTIPLE;
			break;

		case STAT_QURY_NONE:
			ret = Cddb_OK;
			break;
		}
		break;

	case STAT_GEN_CLIENT:
		switch (STATCODE_3RD(p)) {
		case STAT_QURY_AUTHFAIL:
			if (STATCODE_2ND(p) != STAT_SUB_CLOSE) {
				/* Invalid server response */
				ret = CDDBTRNBadResponseSyntax;
				break;
			}

			if (isproxy) {
				ret = CDDBTRNHTTPProxyError;
				break;
			}
			break;

		default:
			break;
		}
		/*FALLTHROUGH*/

	case STAT_GEN_OKFAIL:
	case STAT_GEN_INFO:
	case STAT_GEN_ERROR:
	default:
		/* Error */
		ret = CDDBTRNRecordNotFound;
		break;
	}

	if (buf != NULL)
		MEM_FREE(buf);

	if (ret != Cddb_OK)
		*matchcode = MATCH_NONE;

	return (ret);
#endif	/* NOREMOTE */
}


/*
 * fcddb_region_init
 *	Initialize the region list
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_region_init(cddb_control_t *cp)
{
	cddb_region_t	*rp,
			*prevp;
	int		i;

	prevp = NULL;
	for (i = 0; fcddb_region_tbl[i].id != NULL; i++) {
		rp = &fcddb_region_tbl[i];
		(void) strcpy(rp->objtype, "CddbRegion");

		if (i == 0)
			cp->regionlist.regions = rp;
		else
			prevp->next = rp;

		cp->regionlist.count++;
		prevp = rp;
	}

	return Cddb_OK;
}


/*
 * fcddb_language_init
 *	Initialize the language list
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_language_init(cddb_control_t *cp)
{
	cddb_language_t	*lp,
			*prevp;
	int		i;

	prevp = NULL;
	for (i = 0; fcddb_language_tbl[i].id != NULL; i++) {
		lp = &fcddb_language_tbl[i];
		(void) strcpy(lp->objtype, "CddbLanguage");

		if (i == 0)
			cp->languagelist.languages = lp;
		else
			prevp->next = lp;

		cp->languagelist.count++;
		prevp = lp;
	}

	return Cddb_OK;
}


/*
 * fcddb_genre_init
 *	Initialize the genre tree
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_genre_init(cddb_control_t *cp)
{
	fcddb_gmap_t	*gmp = NULL,
			*gmp2 = NULL;
	cddb_genre_t	*gp,
			*gp2,
			*gp3;
	FILE		*fp;
	int		i,
			mapvers = 0;
	unsigned int	flags;
	char		*p,
			*q,
			mapfile[FILE_PATH_SZ],
			buf[STR_BUF_SZ * 2],
			cddb1name[20],
			meta_id[8],
			meta_name[80],
			sub_id[8],
			sub_name[80];
	bool_t		use_builtin = TRUE;

#ifdef __VMS
	(void) sprintf(mapfile, "%s]genre.map", cp->options.localcachepath);
#else
	(void) sprintf(mapfile, "%s/genre.map", cp->options.localcachepath);
#endif

	if ((fp = fopen(mapfile, "r")) != NULL) {
		FCDDBDBG(fcddb_errfp, "fcddb_genre_init: reading %s\n",
			 mapfile);

		for (i = 1; fgets(buf, sizeof(buf)-1, fp) != NULL; i++) {
			/* Make sure string is null terminated */
			buf[sizeof(buf)-1] = '\0';

			/* Zap newline if necessary */
			if (buf[strlen(buf)-1] == '\n')
				buf[strlen(buf)-1] = '\0';

			if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\r'){
				/* Comment or blank line */
				(void) sscanf(buf, "# version=%d", &mapvers);
				continue;
			}

			if (mapvers != GENREMAP_VERSION) {
				(void) fprintf(fcddb_errfp,
					    "Ignored invalid genre map "
					    "table file: %s\n",
					    mapfile
				);
				break;
			}

			use_builtin = FALSE;

			/* Read file entry */
			q = buf;
			if ((p = strchr(q, ':')) == NULL) {
				(void) fprintf(fcddb_errfp,
					"Genre map table file: %s\n"
					"Error in line %d.\n",
					mapfile, i);
				continue;
			}
			*p = '\0';
			strncpy(cddb1name, q, sizeof(cddb1name)-1);
			cddb1name[sizeof(cddb1name)-1] = '\0';

			q = p+1;
			if ((p = strchr(q, ':')) == NULL) {
				(void) fprintf(fcddb_errfp,
					"Genre map table file: %s\n"
					"Error in line %d.\n",
					mapfile, i);
				continue;
			}
			*p = '\0';
			strncpy(meta_id, q, sizeof(meta_id)-1);
			meta_id[sizeof(meta_id)-1] = '\0';

			q = p+1;
			if ((p = strchr(q, ':')) == NULL) {
				(void) fprintf(fcddb_errfp,
					"Genre map table file: %s\n"
					"Error in line %d.\n",
					mapfile, i);
				continue;
			}
			*p = '\0';
			strncpy(meta_name, q, sizeof(meta_name)-1);
			meta_name[sizeof(meta_name)-1] = '\0';

			q = p+1;
			if ((p = strchr(q, ':')) == NULL) {
				(void) fprintf(fcddb_errfp,
					"Genre map table file: %s\n"
					"Error in line %d.\n",
					mapfile, i);
				continue;
			}
			*p = '\0';
			strncpy(sub_id, q, sizeof(sub_id)-1);
			sub_id[sizeof(sub_id)-1] = '\0';

			q = p+1;
			if ((p = strchr(q, ':')) == NULL) {
				(void) fprintf(fcddb_errfp,
					"Genre map table file: %s\n"
					"Error in line %d.\n",
					mapfile, i);
				continue;
			}
			*p = '\0';
			strncpy(sub_name, q, sizeof(sub_name)-1);
			sub_name[sizeof(sub_name)-1] = '\0';

			q = p+1;
			if (sscanf(q, "%x", (unsigned int *) &flags) != 1) {
				(void) fprintf(fcddb_errfp,
					"Genre map table file: %s\n"
					"Error in line %d.\n",
					mapfile, i);
				continue;
			}

			/* Allocate map entry */
			gmp = (fcddb_gmap_t *) MEM_ALLOC(
				"CddbGenremap", sizeof(fcddb_gmap_t)
			);
			if (gmp == NULL) {
				(void) fclose(fp);
				return CDDBTRNOutOfMemory;
			}
			(void) memset(gmp, 0, sizeof(fcddb_gmap_t));

			gmp->cddb1name = fcddb_strdup(cddb1name);
			gmp->meta.id = fcddb_strdup(meta_id);
			gmp->meta.name = fcddb_strdup(meta_name);
			gmp->sub.id = fcddb_strdup(sub_id);
			gmp->sub.name = fcddb_strdup(sub_name);
			gmp->flags = flags | CDDB_GMAP_DYN;
			gmp->next = NULL;

			/* Add to list */
			if (fcddb_gmap_head == NULL)
				fcddb_gmap_head = gmp;
			else
				gmp2->next = gmp;

			gmp2 = gmp;
		}

		(void) fclose(fp);
	}

	if (use_builtin) {
		/* Can't initialize using genre table file, use the
		 * built-in table instead.
		 */
		FCDDBDBG(fcddb_errfp,
			 "fcddb_genre_init: using internal genre map table\n");

		fcddb_gmap_head = &fcddb_genre_map[0];
		for (i = 0; fcddb_genre_map[i].cddb1name != NULL; i++) {
			gmp = &fcddb_genre_map[i];

			/* Turn this table into a linked list */
			gmp->next = &fcddb_genre_map[i+1];
		}
		if (gmp != NULL)
			gmp->next = NULL;

		/* Write out the genre table file */
		if ((fp = fopen(mapfile, "w")) != NULL) {
			FCDDBDBG(fcddb_errfp,
				 "fcddb_genre_init: writing %s\n", mapfile);

			/* Write out header */
			(void) fprintf(fp,
				"# xmcd %s.%s genre mapping table\n# %s\n",
				VERSION_MAJ, VERSION_MIN, COPYRIGHT);
			(void) fprintf(fp, "# version=%d\n", GENREMAP_VERSION);
			(void) fprintf(fp, "#\n# %s:%s:%s:%s:%s:%s\n#\n",
				"cddb1name",
				"meta_id", "meta_name",
				"sub_id", "sub_name",
				"flags"
			);

			/* Write out map entries */
			for (gmp = fcddb_gmap_head; gmp != NULL;
			     gmp = gmp->next) {
				(void) fprintf(fp, "%s:%s:%s:%s:%s:%x\n",
					gmp->cddb1name,
					gmp->meta.id,
					gmp->meta.name,
					gmp->sub.id,
					gmp->sub.name,
					gmp->flags
				);
			}

			(void) fclose(fp);
		}
	}

	/* Set up genre tree */
	gp3 = NULL;
	cp->genretree.count = 0;
	for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
		if ((gmp->flags & CDDB_GMAP_DUP) != 0)
			continue;

		/* Don't use fcddb_obj_alloc() here because we don't want
		 * CddbReleaseObject() to free these.
		 */
		gp = (cddb_genre_t *) MEM_ALLOC(
			"CddbGenre",
			sizeof(cddb_genre_t)
		);
		if (gp == NULL)
			return CDDBTRNOutOfMemory;

		(void) memset(gp, 0, sizeof(cddb_genre_t));
		(void) strcpy(gp->objtype, "CddbGenre");

		gp->id   = (CddbStr) fcddb_strdup(gmp->meta.id);
		gp->name = (CddbStr) fcddb_strdup(gmp->meta.name);

		gp2 = (cddb_genre_t *) MEM_ALLOC(
			"CddbGenre",
			sizeof(cddb_genre_t)
		);
		if (gp2 == NULL)
			return CDDBTRNOutOfMemory;

		(void) memset(gp2, 0, sizeof(cddb_genre_t));
		(void) strcpy(gp2->objtype, "CddbGenre");

		gp2->id   = (CddbStr) fcddb_strdup(gmp->sub.id);
		gp2->name = (CddbStr) fcddb_strdup(gmp->sub.name);
		gp->next  = gp2;

		if (cp->genretree.genres == NULL)
			cp->genretree.genres = gp;
		else
			gp3->nextmeta = gp;

		gp3 = gp;

		cp->genretree.count++;
	}

	return Cddb_OK;
}


/*
 * fcddb_role_init
 *	Initialize the role tree
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_role_init(cddb_control_t *cp)
{
	cddb_role_t	*rp,
			*prevrp;
	cddb_rolelist_t	*rlp,
			*prevlp;
	int		i;

	prevrp = NULL;
	prevlp = NULL;
	rlp = NULL;
	for (i = 0; fcddb_role_tbl[i].id != NULL; i++) {
		rp = &fcddb_role_tbl[i];

		if (rp->roletype == NULL)
			continue;

		if (rp->roletype[0] == 'm') {
			rlp = (cddb_rolelist_t *) MEM_ALLOC(
				"CddbRoleList", sizeof(cddb_rolelist_t)
			);
			if (rlp == NULL)
				break;

			(void) memset(rlp, 0, sizeof(cddb_rolelist_t));
			(void) strcpy(rlp->objtype, "CddbRoleList");

			rlp->metarole = rp;

			if (cp->roletree.rolelists == NULL)
				cp->roletree.rolelists = rlp;
			else
				prevlp->next = rlp;

			cp->roletree.count++;
			prevlp = rlp;
		}
		else if (rp->roletype[0] == 's') {
			if (rlp == NULL)
				break;

			if (rlp->subroles == NULL)
				rlp->subroles = rp;
			else
				prevrp->next = rp;

			rlp->count++;
			prevrp = rp;
		}
		(void) strcpy(rp->objtype, "CddbRole");
	}

	return Cddb_OK;
}


/*
 * fcddb_url_init
 *	Initialize the URL list
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
STATIC CddbResult
fcddb_url_init(cddb_control_t *cp)
{
	cddb_url_t	*up;

	/* Hard-wire the CDDB web site */
	up = (cddb_url_t *) MEM_ALLOC("CddbURL", sizeof(cddb_url_t));
	if (up == NULL)
		return CDDBTRNOutOfMemory;

	(void) memset(up, 0, sizeof(cddb_url_t));
	(void) strcpy(up->objtype, "CddbURL");
	up->type = (CddbStr) fcddb_strdup("menu");
	up->href = (CddbStr) fcddb_strdup("http://www.cddb.com/");
	up->displaytext =
		(CddbStr) fcddb_strdup("CDDB, the #1 Music Info Source");
	up->category = (CddbStr) fcddb_strdup("Music Sites");

	cp->urllist.urls = up;
	cp->urllist.count++;

	return Cddb_OK;
}


/*
 * fcddb_region_halt
 *	Clean up region table
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_region_halt(void)
{
	int		i;
	cddb_region_t	*rp;

	for (i = 0; fcddb_region_tbl[i].id != NULL; i++) {
		rp = &fcddb_region_tbl[i];
		rp->objtype[0] = '\0';
		rp->next = NULL;
	}
}


/*
 * fcddb_language_halt
 *	Clean up language table
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_language_halt(void)
{
	int		i;
	cddb_language_t	*lp;

	for (i = 0; fcddb_language_tbl[i].id != NULL; i++) {
		lp = &fcddb_language_tbl[i];
		lp->objtype[0] = '\0';
		lp->next = NULL;
	}
}


/*
 * fcddb_role_halt
 *	Clean up role table
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
STATIC void
fcddb_role_halt(void)
{
	int		i;
	cddb_role_t	*rp;

	for (i = 0; fcddb_role_tbl[i].id != NULL; i++) {
		rp = &fcddb_role_tbl[i];
		rp->objtype[0] = '\0';
		rp->next = NULL;
	}
}


/*
 * fcddb_onsig
 *	Generic signal handler, intended to just cause an interruption
 *	to a blocked system call and no more.
 *
 * Args:
 *	signo - signal number
 *
 * Return:
 *	Nothing
 */
/*ARGSUSED*/
void
fcddb_onsig(int signo)
{
	/* Do nothing: we just want the side effect of causing the program
	 * to return from a blocked system call with errno set to EINTR.
	 */
	FCDDBDBG(fcddb_errfp, "\nfcddb_onsig: Command timeout\n");

#if !defined(USE_SIGACTION) && !defined(BSDCOMPAT)
	(void) fcddb_signal(signo, fcddb_onsig);
#endif
}


/*
 * fcddb_signal
 *	Wrapper around signal disposition functions to avoid
 *	the scattering of OS-dependent code.
 *
 * Args:
 *	Same as signal(2).
 *	sig - The signal number
 *	disp - The signal disposition
 *
 * Return:
 *	The previous signal disposition.
 */
void
(*fcddb_signal(int sig, void (*disp)(int)))(int)
{
#ifdef USE_SIGACTION
	struct sigaction	sigact,
				oldact;

	sigact.sa_handler = disp;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	if (sigaction(sig, &sigact, &oldact) < 0) {
		perror("util_signal: sigaction failed");
		_exit(1);
	}
	return (oldact.sa_handler);
#endif
#ifdef USE_SIGSET
	return (sigset(sig, disp));
#endif
#ifdef USE_SIGNAL
	return (signal(sig, disp));
#endif
}


/*
 * fcddb_init
 *	Initialize the fcddb module
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
CddbResult
fcddb_init(cddb_control_t *cp)
{
	CddbResult	ret;
	char		tmpuser[STR_BUF_SZ],
			tmphost[HOST_NAM_SZ + STR_BUF_SZ];

	/* Initialize debug message stream */
	fcddb_errfp = stderr;

	/* Initialize regions */
	if ((ret = fcddb_region_init(cp)) != Cddb_OK)
		return (ret);

	/* Initialize languages */
	if ((ret = fcddb_language_init(cp)) != Cddb_OK)
		return (ret);

	/* Initialize genres */
	if ((ret = fcddb_genre_init(cp)) != Cddb_OK)
		return (ret);

	/* Initialize roles */
	if ((ret = fcddb_role_init(cp)) != Cddb_OK)
		return (ret);

	/* Initialize URLs */
	if ((ret = fcddb_url_init(cp)) != Cddb_OK)
		return (ret);

	/* Misc initializations */
	fcddb_http_xlat(cp->userinfo.userhandle, tmpuser),
	fcddb_http_xlat(cp->hostname, tmphost),

	(void) sprintf(fcddb_hellostr, "hello=%.64s+%.64s+%s+%s",
		       tmpuser, tmphost, cp->clientname, cp->clientver);
	(void) sprintf(fcddb_extinfo,
		       /* Believe it or not, this is how MS Internet Explorer
		        * does it, so don't blame me...
		        */
		       "User-Agent: %s (compatible; %s %s)\r\nAccept: %s\r\n",
		       "Mozilla/5.0",
		       cp->clientname,
		       cp->clientver,
		       "text/plain");

	/* For SOCKS support */
	SOCKSINIT(cp->clientname);

	return Cddb_OK;
}


/*
 * fcddb_halt
 *	Shut down the fcddb module
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
/*ARGSUSED*/
CddbResult
fcddb_halt(cddb_control_t *cp)
{
	/* Clean up internal tables */
	fcddb_region_halt();
	fcddb_language_halt();
	fcddb_role_halt();

	if (fcddb_auth_buf != NULL) {
		MEM_FREE(fcddb_auth_buf);
		fcddb_auth_buf = NULL;
	}

	return Cddb_OK;
}


/*
 * fcddb_obj_alloc
 *	Allocate a CDDB object
 *
 * Args:
 *	objtype - The object type to allocate
 *	len - The size of the object
 *
 * Return:
 *	Pointer to allocated memory
 */
void *
fcddb_obj_alloc(char *objtype, size_t len)
{
	void	*retp;
	char	*cp;

	if (objtype == NULL || strlen(objtype) >= len)
		return NULL;

	retp = MEM_ALLOC(objtype, len);
	if (retp != NULL) {
		cp = (char *) retp;
		(void) memset(cp, 0, len);
		(void) sprintf(cp, "+%s", objtype);
	}

	return (retp);
}


/*
 * fcddb_obj_free
 *	Free CDDB object
 *
 * Args:
 *	obj - Pointer to object previously allocated via fcddb_obj_alloc()
 *
 * Return:
 *	Nothing
 */
void
fcddb_obj_free(void *obj)
{
	char	*cp = (char *) obj;

	if (cp == NULL)
		return;

	if (cp[0] == '+') {
		/* Need to add to this list if fcddb_obj_alloc is used
		 * to allocate other types of objects
		 */
		if (strcmp(cp+1, "CddbControl") == 0)
			fcddb_clear_control((cddb_control_t *) obj);
		else if (strcmp(cp+1, "CddbDisc") == 0)
			fcddb_clear_disc((cddb_disc_t *) obj);
		else if (strcmp(cp+1, "CddbDiscs") == 0)
			fcddb_clear_discs((cddb_discs_t *) obj);
		else if (strcmp(cp+1, "CddbOptions") == 0)
			fcddb_clear_options((cddb_options_t *) obj);
		else if (strcmp(cp+1, "CddbUserInfo") == 0)
			fcddb_clear_userinfo((cddb_userinfo_t *) obj);
		else if (strcmp(cp+1, "CddbGenreTree") == 0)
			fcddb_clear_genretree((cddb_genretree_t *) obj);
		else if (strcmp(cp+1, "CddbRoleTree") == 0)
			fcddb_clear_roletree((cddb_roletree_t *) obj);
		else if (strcmp(cp+1, "CddbURLManager") == 0)
			fcddb_clear_urlmanager((cddb_urlmanager_t *) obj);
		else if (strcmp(cp+1, "CddbCredit") == 0)
			fcddb_clear_credit((cddb_credit_t *) obj);
		else if (strcmp(cp+1, "CddbSegment") == 0)
			fcddb_clear_segment((cddb_segment_t *) obj);

		MEM_FREE(obj);
	}
}


/*
 * fcddb_runcmd
 *	Spawn an external command
 *
 * Args:
 *	cmd - The command string
 *
 * Return:
 *	The command exit status
 */
int
fcddb_runcmd(char *cmd)
{
	int		ret;
#ifndef __VMS
	int		fd,
			saverr = 0;
	pid_t		cpid;
	waitret_t	wstat;
	unsigned int	oa;
	void		(*oh)(int);

	FCDDBDBG(fcddb_errfp, "fcddb_runcmd: [%s]\n", cmd);

	/* Fork child to invoke external command */
	switch (cpid = FORK()) {
	case 0:
		break;

	case -1:
		/* Fork failed */
		return EXE_ERR;

	default:
		/* Parent process: wait for child to exit */
		oh = fcddb_signal(SIGALRM, fcddb_onsig);
		oa = alarm(CMD_TIMEOUT_SECS);

		errno = 0;
		ret = WAITPID(cpid, &wstat, 0);
		saverr = errno;

		(void) alarm(oa);
		(void) fcddb_signal(SIGALRM, oh);

		if (ret < 0) {
			if (saverr == EINTR) {
				FCDDBDBG(fcddb_errfp,
					"Timed out waiting for program "
					"to exit (pid=%d)\n", (int) cpid);
			}

			ret = (saverr << 8);
		}
		else if (WIFEXITED(wstat))
			ret = WEXITSTATUS(wstat);
		else
			ret = EXE_ERR;

		FCDDBDBG(fcddb_errfp, "Command exit status %d\n", ret);
		return (ret);
	}

	/* Child process */
	for (fd = 3; fd < 255; fd++) {
		/* Close unneeded file descriptors */
		(void) close(fd);
	}

	/* Restore SIGTERM */
	(void) fcddb_signal(SIGTERM, SIG_DFL);

	/* Ignore SIGALRM */
	(void) fcddb_signal(SIGALRM, SIG_IGN);

	/* Exec a shell to run the command */
	(void) execl(STR_SHPATH, STR_SHNAME, STR_SHARG, cmd, NULL);
	_exit(EXE_ERR);
	/*NOTREACHED*/
#else
	/* Do the command */
	ret = system(cmd);

	FCDDBDBG(fcddb_errfp, "Command exit status %d\n", ret);
	return (ret);
#endif	/* __VMS */
}


/*
 * fcddb_set_auth
 *	Set the HTTP/1.1 proxy-authorization string
 *
 * Args:
 *	name - The proxy user name
 *	passwd - The proxy password
 *
 * Return:
 *	CddbResult status code
 */
CddbResult
fcddb_set_auth(char *name, char *passwd)
{
	int	i,
		j,
		n1,
		n2;
	char	*buf;

	if (name == NULL || passwd == NULL)
		return Cddb_E_INVALIDARG;

	i = strlen(name);
	j = strlen(passwd);
	n1 = i + j + 2;
	n2 = (n1 * 4 / 3) + 8;

	if ((buf = (char *) MEM_ALLOC("strbuf", n1)) == NULL)
		return CDDBTRNOutOfMemory;

	(void) sprintf(buf, "%s:%s", name, passwd);

	(void) memset(name, 0, i);
	(void) memset(passwd, 0, j);

	if (fcddb_auth_buf != NULL)
		MEM_FREE(fcddb_auth_buf);

	fcddb_auth_buf = (char *) MEM_ALLOC("fcddb_auth_buf", n2);
	if (fcddb_auth_buf == NULL)
		return CDDBTRNOutOfMemory;

	/* Base64 encode the name/password pair */
	fcddb_b64encode(
		(byte_t *) buf,
		strlen(buf),
		(byte_t *) fcddb_auth_buf,
		FALSE
	);

	(void) memset(buf, 0, n1);
	MEM_FREE(buf);
	return Cddb_OK;
}


/*
 * fcddb_strdup
 *	fcddb internal version of strdup().
 *
 * Args:
 *	str - source string
 *
 * Return:
 *	Return string
 */
char *
fcddb_strdup(char *str)
{
	char	*buf;

	if (str == NULL)
		return NULL;

	buf = (char *) MEM_ALLOC("strdup_buf", strlen(str) + 1);
	if (buf != NULL)
		(void) strcpy(buf, str); 

	return (buf);
}


/*
 * fcddb_basename
 *	fcddb internal version of basename()
 *
 * Args:
 *	path - File path
 *
 * Return:
 *	Base name of file path
 */
char *
fcddb_basename(char *path)
{
	char		*p;
#ifdef __VMS
	char		*q;
#endif
	static char	buf[FILE_PATH_SZ];

	if (path == NULL)
		return NULL;

	if ((int) strlen(path) >= FILE_PATH_SZ)
		/* Error: path name too long */
		return NULL;

	(void) strcpy(buf, path);

	if ((p = strrchr(buf, DIR_END)) == NULL)
		return (buf);

	p++;

#ifdef __VMS
	if (*p == '\0') {
		/* The supplied path is a directory - special handling */
		*--p = '\0';
		if ((q = strrchr(buf, DIR_SEP)) != NULL)
			p = q + 1;
		else if ((q = strrchr(buf, DIR_BEG)) != NULL)
			p = q + 1;
		else if ((q = strrchr(buf, ':')) != NULL)
			p = q + 1;
		else {
			p = buf;
			*p = '\0';
		}
	}
#endif

	return (p);
}


/*
 * fcddb_dirname
 *	fcddb internal version of dirname()
 *
 * Args:
 *	path - File path
 *
 * Return:
 *	Directory name of file path
 */
char *
fcddb_dirname(char *path)
{
	char		*p;
#ifdef __VMS
	char		*q;
#endif
	static char	buf[FILE_PATH_SZ];

	if (path == NULL)
		return NULL;

	if ((int) strlen(path) >= FILE_PATH_SZ)
		/* Error: path name too long */
		return NULL;

	(void) strcpy(buf, path);

	if ((p = strrchr(buf, DIR_END)) == NULL)
		return (buf);

#ifdef __VMS
	if (*++p == '\0') {
		/* The supplied path is a directory - special handling */
		if ((q = strrchr(buf, DIR_SEP)) != NULL) {
			*q = DIR_END;
			*++q = '\0';
		}
		else if ((q = strrchr(buf, ':')) != NULL)
			*++q = '\0';
		else
			p = buf;
	}
	*p = '\0';
#else
	if (p == buf)
		*++p = '\0';
	else
		*p = '\0';
#endif

	return (buf);
}


/*
 * fcddb_mkdir
 *	Wrapper for the mkdir() call.  Will make all needed parent
 *	directories leading to the target directory if they don't
 *	already exist.
 *
 * Args:
 *	path - The directory path to make
 *	mode - The permissions
 *
 * Return:
 *	CddbResult status code
 */
CddbResult
fcddb_mkdir(char *path, mode_t mode)
{
	CddbResult	ret;
	char		*cp,
			*mypath;
#ifdef __VMS
	char		*cp2,
			dev[STR_BUF_SZ],
			vpath[STR_BUF_SZ],
			chkpath[STR_BUF_SZ];
#endif

	if ((mypath = fcddb_strdup(path)) == NULL)
		return CDDBTRNOutOfMemory;

#ifndef __VMS
/* UNIX */

	/* Make parent directories, if needed */
	cp = mypath;
	while (*(cp+1) == DIR_BEG)
		cp++;	/* Skip extra initial '/'s if absolute path */

	while ((cp = strchr(cp+1, DIR_SEP)) != NULL) {
		*cp = '\0';

		if ((ret = fcddb_ckmkdir(mypath, mode)) != Cddb_OK) {
			MEM_FREE(mypath);
			return (ret);
		}

		*cp = DIR_SEP;
	}

	if ((ret = fcddb_ckmkdir(mypath, mode)) != Cddb_OK) {
		MEM_FREE(mypath);
		return (ret);
	}

	(void) chmod(mypath, mode);

#else
/* VMS */

	dev[0] = vpath[0] = '\0';

	/* Zap out the version number */
	if ((cp = strchr(mypath, ';')) != NULL)
		*cp = '\0';

	/* Separate into disk and path parts */

	cp = mypath;
	if ((cp = strchr(cp+1, ':')) == NULL)
		cp = mypath;
	else {
		*cp = '\0';
		(void) strcpy(dev, mypath);
		*cp++ = ':';
	}

	/* Check path syntax, and convert from
	 * disk:[a.b.c]d.dir syntax to disk:[a.b.c.d]
	 * if needed.
	 */
	cp2 = cp;
	if ((cp = strchr(cp, DIR_BEG)) == NULL) {
		cp = cp2;
		if ((cp2 = strrchr(cp+1, '.')) != NULL) {
			if (fcddb_strcasecmp(cp2, ".dir") == 0) {
				*cp2 = '\0';
				(void) strcpy(vpath, cp);
			}
			else {
				FCDDBDBG(fcddb_errfp, "fcddb_mkdir: "
					"Illegal directory path: %s\n",
					mypath
				);
				return CDDBTRNCannotCreateFile;
			}
		}
	}
	else {
		(void) strcpy(vpath, ++cp);
		if ((cp = strrchr(vpath, DIR_END)) != NULL) {
			*cp ='\0';
			if ((cp2 = strchr(cp+1, '.')) != NULL) {
				if (fcddb_strcasecmp(cp2, ".dir") == 0) {
					*cp2 = '\0';
					*cp = DIR_SEP;
				}
				else {
					FCDDBDBG(fcddb_errfp, "fcddb_mkdir: "
						"Illegal directory path: %s\n",
						mypath
					);
					return CDDBTRNCannotCreateFile;
				}
			}
			else if (*(cp+1) != '\0') {
				FCDDBDBG(fcddb_errfp, "fcddb_mkdir: "
					"Illegal directory path: %s\n",
					mypath
				);
				return CDDBTRNCannotCreateFile;
			}
		}
	}

	cp = vpath;
	while ((cp = strchr(cp, DIR_SEP)) != NULL) {
		*cp = '\0';

		chkpath[0] = '\0';

		(void) sprintf(chkpath, "%s%s[%s]",
			dev, dev[0] == '\0' ? "" : ":", vpath
		);

		*cp++ = DIR_SEP;

		if ((ret = fcddb_ckmkdir(chkpath, mode)) != Cddb_OK) {
			MEM_FREE(mypath);
			return (ret);
		}
	}

	if (vpath[0] == '\0') {
		(void) sprintf(chkpath, "%s%s[000000]",
			dev, dev[0] == '\0' ? "" : ":"
		);

		if ((ret = fcddb_ckmkdir(chkpath, mode)) != Cddb_OK) {
			MEM_FREE(mypath);
			return (ret);
		}

		(void) chmod(chkpath, mode);
	}
	else if (strcmp(vpath, "000000") != 0) {
		(void) sprintf(chkpath, "%s%s[%s]",
			dev, dev[0] == '\0' ? "" : ":", vpath
		);

		if ((ret = fcddb_ckmkdir(chkpath, mode)) != Cddb_OK) {
			MEM_FREE(mypath);
			return (ret);
		}

		(void) chmod(chkpath, mode);
	}

#endif	/* __VMS */

	MEM_FREE(mypath);
	return Cddb_OK;
}


/*
 * fcddb_username
 *	Return user login name
 *
 * Args:
 *	None
 *
 * Return:
 *	The login name
 */
char *
fcddb_username(void)
{
	char		*cp;
#ifdef __VMS
	cp = getlogin();
	if (cp != NULL)
		return (cp);
#else
	struct passwd	*pw;

	/* Get login name from the password file if possible */
	setpwent();
	if ((pw = getpwuid(getuid())) != NULL) {
		endpwent();
		return (pw->pw_name);
	}
	endpwent();

	/* Try the LOGNAME environment variable */
	if ((cp = (char *) getenv("LOGNAME")) != NULL)
		return (cp);

	/* Try the USER environment variable */
	if ((cp = (char *) getenv("USER")) != NULL)
		return (cp);
#endif
	/* If we still can't get the login name, just set it
	 * to "nobody" (shrug).
	 */
	return ("nobody");
}


/*
 * fcddb_homedir
 *	Return the user's home directory
 *
 * Args:
 *	None
 *
 * Return:
 *	The user's home directory
 */
char *
fcddb_homedir(void)
{
#ifndef __VMS
	struct passwd	*pw;
	char		*cp;

	/* Get home directory from the password file if possible */
	setpwent();
	if ((pw = getpwuid(getuid())) != NULL) {
		endpwent();
		return (pw->pw_dir);
	}
	endpwent();

	/* Try the HOME environment variable */
	if ((cp = (char *) getenv("HOME")) != NULL)
		return (cp);

	/* If we still can't get the home directory, just set it to the
	 * current directory (shrug).
	 */
	return (CUR_DIR);
#else
	char		*cp;
	static char	buf[FILE_PATH_SZ];

	if ((cp = (char *) getenv("HOME")) != NULL &&
	    (int) strlen(cp) < sizeof(buf)) {
		(void) strcpy(buf, cp);
		buf[strlen(buf)-1] = '\0';	/* Drop the "]" */
	}
	else
		(void) strcpy(buf, "SYS$DISK:[");

	return (buf);
#endif	/* __VMS */
}


/*
 * fcddb_hostname
 *	Get the host name (with fully qualified domain name if possible)
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
char *
fcddb_hostname(void)
{
#ifndef NOREMOTE
	struct hostent	*he;
	char		**ap,
			hname[HOST_NAM_SZ+1];
	static char	buf[HOST_NAM_SZ+1];

	buf[0] = '\0';

	/* Try to determine host name */
	if (gethostname(hname, HOST_NAM_SZ) < 0 ||
	    (he = gethostbyname(hname)) == NULL) {
		struct utsname	un;

		if (uname(&un) < 0)
			(void) strcpy(buf, "localhost"); /* shrug */
		else
			(void) strncpy(buf, un.nodename, HOST_NAM_SZ);
	}
	else {
		/* Look for a a fully-qualified hostname
		 * (with domain)
		 */
		if (strchr(he->h_name, '.') != NULL)
			(void) strcpy(buf, he->h_name);
		else {
			for (ap = he->h_aliases; *ap != NULL; ap++) {
				if (strchr(*ap, '.') != NULL) {
				    (void) strncpy(buf, *ap, HOST_NAM_SZ);
				    break;
				}
			}
		}

		if (buf[0] == '\0')
			(void) strcpy(buf, hname);
	}

	buf[HOST_NAM_SZ] = '\0';
	return (buf);
#else
	struct utsname	un;
	static char	buf[HOST_NAM_SZ+1];

	if (uname(&un) < 0)
		(void) strcpy(buf, "localhost"); /* shrug */
	else
		(void) strncpy(buf, un.nodename, HOST_NAM_SZ);

	buf[HOST_NAM_SZ] = '\0';
	return (buf);
#endif
}


/*
 * fcddb_genre_id2categ
 *	Convert CDDB2 genre ID to CDDB1 category string.  Since this could
 *	yield multiple matches, only the first matched mapping is returned.
 *
 * Args:
 *	id - The genre ID string
 *
 * Return:
 *	The category string
 */
char *
fcddb_genre_id2categ(char *id)
{
	fcddb_gmap_t	*gmp;

	if (id == NULL)
		return NULL;

	for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
		if (strcmp(gmp->sub.id, id) == 0)
			return (gmp->cddb1name);
	}
	return NULL;
}


/*
 * fcddb_genre_id2gp
 *	Convert CDDB2 genre ID to a genre structure pointer
 *
 * Args:
 *	cp - Pointer to the control structure
 *	id - The genre ID string
 *
 * Return:
 *	Pointer to the genre structure
 */
cddb_genre_t *
fcddb_genre_id2gp(cddb_control_t *cp, char *id)
{
	cddb_genre_t	*p,
			*q;

	if (cp == NULL || id == NULL)
		return NULL;

	for (p = cp->genretree.genres; p != NULL; p = p->nextmeta) {
		if (p->id != NULL && strcmp(p->id, id) == 0)
			return (p);

		for (q = p->next; q != NULL; q = q->next) {
			if (q->id != NULL && strcmp(q->id, id) == 0)
				return (q);
		}
	}

	return NULL;
}


/*
 * fcddb_genre_categ2id
 *	Convert CDDB1 category string to a CDDB2 genre ID
 *
 * Args:
 *	categ - The category name string
 *
 * Return:
 *	The genre ID string
 */
char *
fcddb_genre_categ2id(char *categ)
{
	fcddb_gmap_t	*gmp;

	if (categ == NULL)
		return NULL;

	for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
		if (strcmp(gmp->cddb1name, categ) == 0)
			return (gmp->sub.id);
	}

	return NULL;
}


/*
 * fcddb_genre_categ2gp
 *	Convert CDDB1 category string to a genre structure pointer
 *
 * Args:
 *	cp - Pointer to the control structure
 *	categ - The category string
 *
 * Return:
 *	Pointer to the genre structure
 */
cddb_genre_t *
fcddb_genre_categ2gp(cddb_control_t *cp, char *categ)
{
	fcddb_gmap_t	*gmp;
	char		*id;

	if (cp == NULL || categ == NULL)
		return NULL;

	id = NULL;
	for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
		if (strcmp(gmp->cddb1name, categ) == 0) {
			id = gmp->sub.id;
			break;
		}
	}

	if (id == NULL)
		return NULL;

	return (fcddb_genre_id2gp(cp, id));
}


/*
 * fcddb_parse_toc
 *	Parse a TOC string and populate an array of frame offsets
 *
 * Args:
 *	tocstr - The TOC
 *	addr - Return array of frame offsets
 *
 * Return:
 *	The number of tracks
 */
int
fcddb_parse_toc(char *tocstr, unsigned int *addr)
{
	char	*p,
		*q,
		sav;
	int	i;

	if (tocstr == NULL || addr == NULL)
		return 0;

	i = 0;
	p = tocstr;
	SKIP_SPC(p);

	/* Starting frames for each track */
	while ((q = strchr(p, ' ')) != NULL || (q = strchr(p, '\t')) != NULL) {
		sav = *q;
		*q = '\0';
		addr[i++] = (unsigned int) atoi(p);
		*q = sav;
		p = q+1;
		SKIP_SPC(p);
	}
	addr[i] = (unsigned int) atoi(p);	/* Lead out */

	return (i);
}


/*
 * fcddb_discid
 *	Compute CDDB1 disc ID
 *
 * Args:
 *	ntrks - Number of tracks
 *	addr - Array of frame offsets
 *
 * Return:
 *	The CDDB1 disc ID string
 */
char *
fcddb_discid(int ntrks, unsigned int *addr)
{
	int		i,
			t = 0,
			n = 0;
	unsigned int	discid_val;
	char		discid[12];

	/* For backward compatibility this algorithm must not change */

	for (i = 0; i < ntrks; i++)
		n += fcddb_sum(addr[i] / FRAME_PER_SEC);

	t = (addr[ntrks] / FRAME_PER_SEC) - (addr[0] / FRAME_PER_SEC);

	discid_val = ((n % 0xff) << 24 | t << 8 | ntrks);
	(void) sprintf(discid, "%08x", discid_val);

	return (fcddb_strdup(discid));
}


/*
 * fcddb_read_cddb
 *	Send a read command to the CDDB server
 *
 * Args:
 *	cp - Pointer to the control structure
 *	discid - The disc ID
 *	toc - The TOC
 *	category - The CDDB1 category
 *	conhost - Server host
 *	conport - The server port
 *	isproxy - Whether we're using a proxy server
 *
 * Return:
 *	CddbResult status code
 */
#ifdef NOREMOTE
/*ARGSUSED*/
#endif
CddbResult
fcddb_read_cddb(
	cddb_control_t		*cp,
	char			*discid,
	char			*toc,
	char			*category,
	char			*conhost,
	unsigned short		conport,
	bool_t			isproxy
)
{
	CddbResult	ret;
	int		fd,
			n,
			i;
	size_t		buflen,
			contentlen;
	char		*buf,
			*p,
			filepath[FILE_PATH_SZ];
	time_t		t;
	struct stat	stbuf;
#ifndef NOREMOTE
	char		*q,
			urlstr[HOST_NAM_SZ + FILE_PATH_SZ + 12];
	bool_t		valid_resp;
#endif

#ifdef __VMS
	(void) sprintf(filepath, "%s.%s]%s.",
		       cp->options.localcachepath, category, discid);
#else
	(void) sprintf(filepath, "%s/%s/%s",
		       cp->options.localcachepath, category, discid);
#endif

	t = time(NULL);

	/*
	 * Attempt to read from local cache
	 */
	FCDDBDBG(fcddb_errfp, "\nfcddb_read_cddb: Checking cache\n");

	ret = CDDBTRNRecordNotFound;

	if (stat(filepath, &stbuf) == 0 && S_ISREG(stbuf.st_mode)
#ifndef NOREMOTE
	    &&
	    (((cp->options.localcacheflags & CACHE_DONT_CONNECT) != 0) ||
	     ((t - stbuf.st_mtime) <=
		    (int) (cp->options.localcachetimeout * SECS_PER_DAY)))
#endif
	   ) {
		/*
		 * Use local cache
		 */

		contentlen = stbuf.st_size;
		buflen = contentlen + 1;
		buf = (char *) MEM_ALLOC("cddb_buf", buflen);
		if (buf == NULL)
			return CDDBTRNOutOfMemory;

		ret = Cddb_OK;

		FCDDBDBG(fcddb_errfp, "fcddb_read_cddb: cache read %s/%s\n",
			 category, discid);

		/* Open local cache file */
		if ((fd = open(filepath, O_RDONLY)) < 0) {
			ret = CDDBTRNRecordNotFound;
		}
		else {
			/* Read cache data into buf */

			for (i = contentlen, p = buf; i > 0; i -= n, p += n) {
				if ((n = read(fd, p, i)) < 0) {
					MEM_FREE(buf);
					ret = CDDBTRNRecordNotFound;
					break;
				}
			}
			*p = '\0';

			(void) close(fd);

			if (ret == Cddb_OK) {
				ret = fcddb_parse_cddb_data(
					cp, buf, discid, category, toc, FALSE,
					&cp->disc
				);
			}
		}

		MEM_FREE(buf);
		buf = NULL;

		if (ret == Cddb_OK)
			return (ret);

		/* Fall through */
	}

#ifdef NOREMOTE
	return (ret);
#else
	if ((cp->options.localcacheflags & CACHE_DONT_CONNECT) != 0)
		return (ret);

	/*
	 * Do CDDB server read
	 */

	FCDDBDBG(fcddb_errfp, "fcddb_read_cddb: server read %s/%s\n",
		 category, discid);

	buflen = CDDBP_CMDLEN;
	if ((buf = (char *) MEM_ALLOC("http_buf", buflen)) == NULL)
		return CDDBTRNOutOfMemory;

	/*
	 * Set up CDDB read command string
	 */

	urlstr[0] = '\0';

	if (isproxy) {
		(void) sprintf(urlstr, "http://%s:%d",
			       CDDB_SERVER_HOST, HTTP_PORT);
	}

	(void) strcat(urlstr, CDDB_CGI_PATH);

	(void) sprintf(buf,
		       "GET %s?cmd=cddb+read+%s+%s&%s&proto=4 HTTP/1.0\r\n",
		       urlstr, category, discid, fcddb_hellostr);

	if (isproxy && fcddb_auth_buf != NULL) {
		(void) sprintf(buf, "%sProxy-Authorization: Basic %s\r\n", buf,
			       fcddb_auth_buf);
	}

	(void) sprintf(buf, "%s%s%s\r\n%s\r\n", buf,
		       "Host: ", CDDB_SERVER_HOST,
		       fcddb_extinfo);

	/* Open network connection to server */
	if ((ret = fcddb_connect(conhost, conport, &fd)) != Cddb_OK) {
		if (buf != NULL)
			MEM_FREE(buf);
		return (ret);
	}

	/*
	 * Send the command to the CDDB server and get response code
	 */
	if ((ret = fcddb_sendcmd(fd, &buf, &buflen, isproxy)) != Cddb_OK) {
		if (buf != NULL)
			MEM_FREE(buf);
		return (ret);
	}

	/* Close server connection */
	fcddb_disconnect(fd);

	/* Check for CDDB command response */
	valid_resp = FALSE;
	p = buf;
	while ((q = strchr(p, '\n')) != NULL) {
		*q = '\0';

		if (STATCODE_CHECK(p)) {
			valid_resp = TRUE;
			*q = '\n';
			break;
		}

		*q = '\n';
		p = q + 1;
		SKIP_SPC(p);
	}

	if (!valid_resp) {
		/* Server error */
		return CDDBTRNBadResponseSyntax;
	}
	
	/*
	 * Check command status
	 */
	ret = Cddb_OK;

	switch (STATCODE_1ST(p)) {
	case STAT_GEN_OK:
	case STAT_GEN_OKCONT:
		if (STATCODE_2ND(p) == '1') {
			if ((q = strchr(p, '\n')) == NULL) {
				ret = CDDBTRNBadResponseSyntax;
				break;
			}

			p = q + 1;
			SKIP_SPC(p);
			if (*p == '\0') {
				ret = CDDBTRNBadResponseSyntax;
				break;
			}

			ret = fcddb_parse_cddb_data(
				cp, p, discid, category, toc, TRUE, &cp->disc
			);
			break;
		}

		/*FALLTHROUGH*/

	case STAT_GEN_INFO:
	case STAT_GEN_OKFAIL:
	case STAT_GEN_ERROR:
	case STAT_GEN_CLIENT:
	default:
		/* Error */
		ret = CDDBTRNRecordNotFound;
		break;
	}

	if (buf != NULL)
		MEM_FREE(buf);

	return (ret);
#endif
}


/*
 * fcddb_lookup_cddb
 *	High-level function to perform a CDDB server lookup
 *
 * Args:
 *	cp - Pointer to the control structure
 *	discid - The disc ID
 *	addr - Array of frame offsets
 *	toc - The TOC
 *	conhost - The server host
 *	conport - The server port
 *	isproxy - Whether we're using a proxy server
 *	matchcode - Return matchcode
 *
 * Return:
 *	CddbResult status code
 */
CddbResult
fcddb_lookup_cddb(
	cddb_control_t		*cp,
	char			*discid,
	unsigned int		*addr,
	char			*toc,
	char			*conhost,
	unsigned short		conport,
	bool_t			isproxy,
	int			*matchcode
)
{
	CddbResult	ret;
	char		category[FILE_BASE_SZ];

	*matchcode = MATCH_NONE;
	fcddb_clear_disc(&cp->disc);

	if ((ret = fcddb_query_cddb(cp, discid, toc, addr, conhost, conport,
			       isproxy, category, matchcode)) != Cddb_OK)
		return (ret);

	if (*matchcode == MATCH_EXACT)
		ret = fcddb_read_cddb(cp, discid, toc, category,
				      conhost, conport, isproxy);
	else
		ret = Cddb_OK;

	return (ret);
}


/*
 * fcddb_submit_cddb
 *	Submit a disc entry to CDDB
 *
 * Args:
 *	cp - Pointer to the control structure
 *	dp - Pointer to the disc structure
 *	conhost - The server host
 *	conport - The server port
 *	isproxy - Whether we're using a proxy server
 *
 * Return:
 *	CddbResult status code
 */
#ifdef NOREMOTE
/*ARGSUSED*/
#endif
CddbResult
fcddb_submit_cddb(
	cddb_control_t	*cp,
	cddb_disc_t	*dp,
	char		*conhost,
	unsigned short	conport,
	bool_t		isproxy
)
{
	CddbResult	ret;
	char		filepath[FILE_PATH_SZ];
	struct stat	stbuf;
	fcddb_gmap_t	*gmp;
	char		*p;
#ifndef NOREMOTE
	int		fd,
			n,
			i;
	size_t		contentlen = 0,
			buflen;
	char		*buf,
			*q,
			urlstr[HOST_NAM_SZ + FILE_PATH_SZ + 12];
	bool_t		valid_resp;
#endif

	/* Check if the genre has changed */
	if (dp->category != NULL) {
		if ((p = fcddb_genre_categ2id(dp->category)) != NULL &&
		    strcmp(p, dp->genre->id) != 0) {
			MEM_FREE(dp->category);
			dp->category = NULL;
		}
	}

	if (dp->category == NULL) {
		/* Find the appropriate category: look for an existing
		 * local cache file.
		 */
		for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
			if (strcmp(gmp->sub.id, dp->genre->id) != 0)
				continue;
#ifdef __VMS
			(void) sprintf(filepath, "%s.%s]%s.",
				       cp->options.localcachepath,
				       gmp->cddb1name, dp->discid);
#else
			(void) sprintf(filepath, "%s/%s/%s",
				       cp->options.localcachepath,
				       gmp->cddb1name, dp->discid);
#endif
			if (stat(filepath, &stbuf) < 0)
				continue;

			if (S_ISREG(stbuf.st_mode)) {
				dp->category = fcddb_strdup(gmp->cddb1name);
				break;
			}
		}

		if (dp->category == NULL) {
			/* Try reverse mapping the genre */
			if ((p = fcddb_genre_id2categ(dp->genre->id)) != NULL)
				dp->category = fcddb_strdup(p);
		}

		if (dp->category == NULL) {
			/* Still can't find a matching category: fail */
			return Cddb_E_INVALIDARG;
		}
	}

#ifdef __VMS
	(void) sprintf(filepath, "%s.%s]%s.",
		       cp->options.localcachepath, dp->category, dp->discid);
#else
	(void) sprintf(filepath, "%s/%s/%s",
		       cp->options.localcachepath, dp->category, dp->discid);
#endif

	if ((ret = fcddb_write_cddb(filepath, dp, TRUE)) != Cddb_OK)
		return (ret);

	if (stat(filepath, &stbuf) < 0)
		return CDDBTRNFileWriteError;

#ifdef NOREMOTE
	return Cddb_OK;
#else
	if ((cp->options.localcacheflags & CACHE_SUBMIT_OFFLINE) != 0)
		return Cddb_OK;

	contentlen = (size_t) stbuf.st_size;

	buflen = CDDBP_CMDLEN + contentlen;
	if ((buf = (char *) MEM_ALLOC("http_buf", buflen)) == NULL)
		return CDDBTRNOutOfMemory;

	FCDDBDBG(fcddb_errfp, "\nfcddb_submit_cddb: %s/%s\n",
		 dp->category, dp->discid);

	/*
	 * Set up CDDB submit command string
	 */

	urlstr[0] = '\0';

	if (isproxy) {
		(void) sprintf(urlstr, "http://%s:%d",
			       CDDB_SUBMIT_HOST, HTTP_PORT);
	}

	(void) strcat(urlstr, CDDB_SUBMIT_CGI_PATH);

	(void) sprintf(buf, "POST %s HTTP/1.0\r\n", urlstr);

	if (isproxy && fcddb_auth_buf != NULL) {
		(void) sprintf(buf, "%sProxy-Authorization: Basic %s\r\n", buf,
			       fcddb_auth_buf);
	}

	(void) sprintf(buf,
		       "%s%s%s\r\n%s%s%s\r\n%s%s\r\n"
		       "%s%s@%s\r\n%s%s\r\n%s%u\r\n\r\n", buf,
		       "Host: ", CDDB_SUBMIT_HOST,
		       fcddb_extinfo,
		       "Category: ", dp->category,
		       "Discid: ", dp->discid,
		       "User-Email: ", cp->userinfo.userhandle, cp->hostname,
		       "Submit-Mode: ",
				cp->options.testsubmitmode ? "test" : "submit",
		       "Content-Length: ", (unsigned int) contentlen
	);

	/* Append CDDB file data to buf here */
	if ((fd = open(filepath, O_RDONLY)) < 0) {
		MEM_FREE(buf);
		return CDDBTRNRecordNotFound;
	}

	for (i = contentlen, p = buf + strlen(buf); i > 0; i -= n, p += n) {
		if ((n = read(fd, p, i)) < 0) {
			MEM_FREE(buf);
			return CDDBTRNRecordNotFound;
		}
	}
	*p = '\0';

	(void) close(fd);

	/* Open network connection to server */
	if ((ret = fcddb_connect(conhost, conport, &fd)) != Cddb_OK) {
		if (buf != NULL)
			MEM_FREE(buf);
		return (ret);
	}

	/*
	 * Send the command to the CDDB server and get response code
	 */
	if ((fcddb_sendcmd(fd, &buf, &buflen, isproxy)) != Cddb_OK) {
		if (buf != NULL)
			MEM_FREE(buf);
		return (ret);
	}

	/* Close server connection */
	fcddb_disconnect(fd);

	/* Check for CDDB command response */
	valid_resp = FALSE;
	p = buf;
	while ((q = strchr(p, '\n')) != NULL) {
		*q = '\0';
	
		if (STATCODE_CHECK(p)) {
			valid_resp = TRUE;
			*q = '\n';
			break;
		}

		*q = '\n';
		p = q + 1;
		SKIP_SPC(p);
	}

	if (!valid_resp) {
		/* Server error */
		return CDDBTRNBadResponseSyntax;
	}

	/*
	 * Check command status
	 */
	switch (STATCODE_1ST(p)) {
	case STAT_GEN_OK:
		ret = Cddb_OK;
		break;

	case STAT_GEN_ERROR:
		ret = CDDBSVCMissingField;
		break;

	case STAT_GEN_OKFAIL:
		ret = CDDBSVCServiceError;
		break;

	case STAT_GEN_OKCONT:
	case STAT_GEN_INFO:
	default:
		/* Error */
		ret = CDDBTRNBadResponseSyntax;
		break;
	}

	if (buf != NULL)
		MEM_FREE(buf);

	return (ret);
#endif	/* NOREMOTE */
}


/*
 * fcddb_flush_cddb
 *	Flush local cachestore
 *
 * Args:
 *	cp - Pointer to the control structure
 *
 * Return:
 *	CddbResult status code
 */
#ifdef NOREMOTE
/*ARGSUSED*/
#endif
CddbResult
fcddb_flush_cddb(cddb_control_t *cp, CDDBFlushFlags flags)
{
#ifndef NOREMOTE
	fcddb_gmap_t	*gmp;
	char		cmd[FILE_PATH_SZ + STR_BUF_SZ];

	if (flags == FLUSH_DEFAULT || (flags & FLUSH_CACHE_MEDIA)) {
		for (gmp = fcddb_gmap_head; gmp != NULL; gmp = gmp->next) {
#ifndef __VMS
			(void) sprintf(cmd,
				    "rm -rf %s/%s >/dev/null 2>&1",
				    cp->options.localcachepath,
				    gmp->cddb1name);

			if (fcddb_runcmd(cmd) != 0)
				return CDDBTRNFileDeleteError;
#else
			/* For VMS simply removing the file doesn't
			 * produce the desired effect, so do nothing
			 * for now.
			 */
#endif
		}
	}
#endif
	return Cddb_OK;
}


