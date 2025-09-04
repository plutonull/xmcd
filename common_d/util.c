/*
 *   util.c - Common utility routines for xmcd, cda and libdi.
 *
 *   xmcd  - Motif(R) CD Audio Player/Ripper
 *   cda   - Command-line CD Audio Player/Ripper
 *   libdi - CD Audio Device Interface Library
 *
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
static char *_util_c_ident_ = "@(#)util.c	6.224 04/03/23";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"

#if defined(USE_SELECT) && defined(_AIX)
#include <sys/select.h>
#endif

#ifdef USE_PTHREAD_DELAY_NP
#include <pthread.h>
#endif

#ifdef HAS_ICONV
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#endif

#ifdef __VMS
#include <fscndef.h>
STATIC int		context;
#else
/* Defines used by util_runcmd */
#define STR_SHPATH	"/bin/sh"	/* Path to shell */
#define STR_SHNAME	"sh"		/* Name of shell */
#define STR_SHARG	"-c"		/* Shell arg */
#endif


/* URL protocols used by util_urlchk */
STATIC struct {
	char		*protocol;
	bool_t		islocal;
} url_protolist[] = {
	{ "http://",	FALSE	},
	{ "https://",	FALSE	},
	{ "ftp://",	FALSE	},
	{ "file:",	TRUE	},
	{ "mailto:",	FALSE	},
	{ "news:",	FALSE	},
	{ "snews:",	FALSE	},
	{ "gopher://",	FALSE	},
	{ "wais://",	FALSE	},
#ifdef _URL_EXTPROTOS		/* Define this if desired */
	{ "nntp:",	FALSE	},
	{ "telnet:",	FALSE	},
	{ "rlogin:",	FALSE	},
	{ "tn3270:",	FALSE	},
	{ "data:",	FALSE	},
	{ "ldap:",	FALSE	},
	{ "ldaps:",	FALSE	},
	{ "castanet:",	FALSE	},
#endif
	{ NULL,		FALSE	}
};


/* Defines used by util_html_fputs() */
#ifdef BUGGY_BROWSER
/* Should always use &nbsp; but some versions of NCSA Mosaic don't grok it */
#define HTML_ESC_SPC	"&#20;"		/* HTML for blank space */
#else
#define HTML_ESC_SPC	"&nbsp;"	/* HTML for blank space */
#endif

#define CHARS_PER_TAB	8		/* # chars in a tab */


/* Signature string to denote an error has occurred while converting
 * a UTF-8 string to ISO8859-1.  Used by util_utf8_to_iso8859().
 */
#define UTF8_ISO8859_ERR_SIG		"[UTF-8 -> ISO8859-1 Error] "


/* Defines used by util_assign64() and util_assign32() */
#if _BYTE_ORDER_ == _B_ENDIAN_
#define LO_WORD	1
#define HI_WORD	0
#else
#define LO_WORD	0
#define HI_WORD	1
#endif


extern appdata_t	app_data;
extern FILE		*errfp;


STATIC uid_t		ouid = 30001;	/* Default to something safe */
STATIC gid_t		ogid = 30001;	/* Default to something safe */
STATIC struct utsname	un;		/* utsname */

/*
 * Data used by util_text_reduce()
 */
STATIC int		excnt,
			delcnt,
			*exlen,
			*dellen;
STATIC char		**exclude_words;
STATIC char		*delete_words[] = {
	"\\n", "\\r", "\\t"
};

/*
 * For util_monname()
 */
STATIC char		*mon_name[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};



/***********************
 *  private routines   *
 ***********************/


/*
 * util_utf8_to_iso8859
 *	UTF-8 to ISO8859 string conversion function.  The output
 *	string buffer will be internally allocated in this function
 *	and should be freed by the caller via MEM_FREE when done.
 *	Note that due to the byte-oriented nature of ISO8859 encoding,
 *	Any malformed UTF-8 character, or a UTF-character that would
 *	evaluate to a value greater than 0xfd are converted into an
 *	underscore '_'.
 *
 * Args:
 *	from - The input UTF-8 string
 *	to   - The output ISO8859 string address pointer
 *	roe  - Revert-on-error boolean flag.  Controls the behavior when
 *	       a conversion error occurs:
 *	       FALSE  - A partial conversion will take place, the
 *			unconvertable characters will be replaced by an
 *			underscore '_' character.
 *	       TRUE   - The output string will be the UTF8_ISO8859_ERR_SIG
 *			signature, followed by a verbatim copy of the
 *			unconverted input UTF-8 string.
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
util_utf8_to_iso8859(char *from, char **to, bool_t roe)
{
	unsigned char	*p,
			*q,
			*tobuf;
	bool_t		err;

	tobuf = (unsigned char *) MEM_ALLOC(
		"iso8859_to_buf",
		strlen(from) + strlen(UTF8_ISO8859_ERR_SIG) + 1
	);
	if (tobuf == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		*to = NULL;
		return FALSE;
	}
	
	p = (unsigned char *) from;
	q = (unsigned char *) tobuf;
	err = FALSE;
	while (*p != '\0') {
		if (*p <= 0x7f) {
			/* Simple US-ASCII character: just copy it */
			*q++ = *p++;
		}
		else if (*p >= 0xc0 && *p <= 0xfd) {
			int	n = 0;

			if ((*p & 0xe0) == 0xc0) {
				/* Two byte sequence */
				n = 2;
				if (*(p+1) >= 0x80 && *(p+1) <= 0xfd) {
				    /* OK */
				    *q = (((*p & 0x03) << 6) |
					   (*(p+1) & 0x3f));

				    if ((*p & 0x1c) != 0) {
					/* Decodes to more than 8 bits */
					*q = '_';
					err = TRUE;
					DBGPRN(DBG_CDI|DBG_GEN)(errfp,
					    "Notice: utf8_to_iso8859: "
					    "Skipping unsupported character.\n"
					);
				    }
				    else if (*q <= 0x7f) {
					/* Not the shortest encoding:
					 * illegal.
					 */
					*q = '_';
					err = TRUE;
					DBGPRN(DBG_CDI|DBG_GEN)(errfp,
					    "Notice: utf8_to_iso8859: "
					    "Skipping illegal character.\n"
					);
				    }
				}
				else {
				    /* Malformed UTF-8 character */
				    *q = '_';
				    err = TRUE;
				    DBGPRN(DBG_CDI|DBG_GEN)(errfp,
					"Notice: utf8_to_iso8859: "
					"Skipping malformed sequence.\n"
				    );
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

			if (n > 2) {
				*q++ = '_';
				err = TRUE;
				DBGPRN(DBG_CDI|DBG_GEN)(errfp,
					"Notice: utf8_to_iso8859: "
					"Skipping unsupported character.\n"
				);
			}

			while (n > 0) {
				if (*(++p) == '\0')
					break;
				n--;
			}
		}
		else {
			/* Malformed UTF-8 sequence: skip */
			p++;
			err = TRUE;
			DBGPRN(DBG_CDI|DBG_GEN)(errfp,
				"Notice: utf8_to_iso8859: "
				"Skipping malformed sequence.\n"
			);
		}
	}
	*q = '\0';

	if (err && (app_data.debug & (DBG_CDI|DBG_GEN)) != 0) {
		util_dbgdump(
			"The string that contains the error",
			(byte_t *) from,
			strlen(from)
		);
		(void) fputs("\n", errfp);
	}

	if (err && roe) {
		/* Revert to input string on error, but prepend
		 * it with an conversion error signature string.
		 */
		(void) sprintf((char *) tobuf, "%s%s",
				UTF8_ISO8859_ERR_SIG, from);
	}

	*to = NULL;
	if (!util_newstr(to, (char *) tobuf)) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		MEM_FREE(tobuf);
		return FALSE;
	}
	MEM_FREE(tobuf);

	return TRUE;
}


/*
 * util_iso8859_to_utf8
 *	ISO8859 to UTF-8 string conversion function.  The output
 *	string buffer will be internally allocated in this function
 *	and should be freed by the caller via MEM_FREE when done.
 *
 * Args:
 *	from - The input ISO8859 string
 *	to   - The output UTF-8 string address pointer
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
util_iso8859_to_utf8(char *from, char **to)
{
	unsigned char	*p,
			*q,
			*tobuf;
	size_t		n;

	/* If the string has the UTF8_ISO8859_ERR_SIG signature at the
	 * beginning we can assume that the rest of the string is already
	 * encoded in UTF-8, so just skip the signature and copy the rest.
	 */
	n = strlen(UTF8_ISO8859_ERR_SIG);
	if (strncmp(from, UTF8_ISO8859_ERR_SIG, n) == 0) {
		*to = NULL;
		if (!util_newstr(to, from + n)) {
			(void) fprintf(errfp, "Error: %s\n",
					app_data.str_nomemory);
			return FALSE;
		}
		return TRUE;
	}

	tobuf = (unsigned char *) MEM_ALLOC(
		"utf8_to_buf", strlen(from) * 6 + 1
	);
	if (tobuf == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		*to = NULL;
		return FALSE;
	}

	p = (unsigned char *) from;
	q = (unsigned char *) tobuf;
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

	*to = NULL;
	if (!util_newstr(to, (char *) tobuf)) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		MEM_FREE(tobuf);
		return FALSE;
	}
	MEM_FREE(tobuf);

	return TRUE;
}


#ifdef __VMS

/*
 * util_vms_dirconv
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
util_vms_dirconv(char *path)
{
	char	*cp,
		*cp2,
		*cp3,
		*buf,
		tmp[STR_BUF_SZ];

	buf = (char *) MEM_ALLOC("vms_dirconv", strlen(path) + 16);
	if (buf == NULL) {
		(void) fprintf(errfp, "Error: Out of memory\n");
		_exit(1);
	}
	(void) strcpy(buf, path);

	if ((cp = strchr(buf, DIR_BEG)) != NULL) {
		if ((cp2 = strrchr(buf, DIR_END)) != NULL) {
			if ((cp3 = strrchr(buf, DIR_SEP)) != NULL) {
				if (util_strcasecmp(cp3, ".dir") == 0) {
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

			if (util_strcasecmp(cp2, ".dir") == 0) {
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
 * util_lconv_open
 *	Open localization for character set conversion.
 *
 * Args:
 *	from - Locale to convert from, or NULL to use the default locale
 *	to - Locale to convert to, or NULL to use the default locale
 *
 * Return:
 *	Locale descriptor to be used with util_do_lconv.
 */
void *
util_lconv_open(char *from, char *to)
{
#ifdef HAS_ICONV
	iconv_t	desc;

	if (from == NULL && to == NULL)
		return NULL;

	if (from == NULL)
		from = nl_langinfo(CODESET);
	if (to == NULL)
		to = nl_langinfo(CODESET);

	if (from == NULL || *from == '\0' || to == NULL || *to == '\0')
		/* Conversion not necessary and not supported */
		return NULL;

	if (strcmp(from, to) == 0)
		/* Conversion not necessary */
		return NULL;

	if ((desc = iconv_open(to, from)) == (iconv_t) -1) {
		DBGPRN(DBG_CDI|DBG_GEN)(errfp,
		    "\"%s\" -> \"%s\": Charset conversion not supported.\n",
		    from, to
		);
		return NULL;
	}

	return ((void *) desc);
#else
	return NULL;
#endif
}


/*
 * util_lconv_close
 *	Close localization for character set conversion
 *
 * Args:
 *	desc - Locale descriptor returned from util_lconv_open.
 *
 * Return:
 *	Nothing.
 */
void
util_lconv_close(void *desc)
{
#ifdef HAS_ICONV
	(void) iconv_close((iconv_t) desc);
#endif
}


/*
 * util_do_lconv
 *	Perform localization for character set conversion.  Note that
 *	this will work only for byte-oriented strings.  Multi-byte
 *	strings will not display correctly.
 *
 * Args:
 *	desc - Locale descriptor returned from util_lconv_open.
 *	from - String to convert.
 *	to - Result string, the storage is internally allocated and should
 *	     be freed via MEM_FREE() when done.
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
util_do_lconv(void *desc, char *from, char **to)
{
#ifdef HAS_ICONV
	int	flen,
		tlen,
		llen,
		ulen;
	char	*tobuf,
		*p;
	bool_t	err = FALSE;

	flen = strlen(from);
	tlen = ((flen + 3) & ~3) + 1;
	llen = tlen - 1;
	ulen = 0;

	if (desc == NULL)
		return FALSE;

	if ((tobuf = p = (char *) MEM_ALLOC("lconv_to", tlen)) == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		return FALSE;
	}

	while (iconv(desc, (void *) &from, (size_t *) &flen,
		     &p, (size_t *) &llen) == (size_t) -1) {

		switch (errno) {
		case E2BIG:
			/* Grow the output buffer and retry */
			ulen = p - tobuf;
			tlen = ((tlen - 1) << 1) + 1;
			tobuf = (char *) MEM_REALLOC("lconv_to", tobuf, tlen);
			if (tobuf == NULL) {
				(void) fprintf(errfp, "Error: %s\n",
					app_data.str_nomemory
				);
				*to = NULL;
				return FALSE;
			}
			llen = tlen - ulen - 1;
			p = tobuf + ulen;
			break;

		case EINVAL:
			DBGPRN(DBG_CDI|DBG_GEN)(errfp,
				"iconv: Charset conversion: "
				"Input string error.\n"
			);
			err = TRUE;
			break;

		case EILSEQ:
			DBGPRN(DBG_CDI|DBG_GEN)(errfp,
				"iconv: Notice: Charset conversion: "
				"Skipping unsupported sequence.\n"
			);
			/* Skip and try to process the rest of the string */
			from++;
			flen = strlen(from);
			break;

		default:
			DBGPRN(DBG_CDI|DBG_GEN)(errfp,
				"iconv: Charset conversion error: "
				"errno=%d\n", errno
			);
			err = TRUE;
			break;
		}

		if (err)
			break;
	}
	*p = '\0';

	if (err && (app_data.debug & (DBG_CDI|DBG_GEN)) != 0) {
		util_dbgdump(
			"The string that contains the error",
			(byte_t *) from,
			strlen(from)
		);
		(void) fputs("\n", errfp);
	}

	*to = NULL;
	if (!util_newstr(to, (char *) tobuf)) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		MEM_FREE(tobuf);
		return FALSE;
	}
	MEM_FREE(tobuf);

	return TRUE;
#else
	return FALSE;
#endif
}


/*
 * util_ckmkdir
 *	Check the specified directory path, if it doesn't exist,
 *	attempt to create it.
 *
 * Args:
 *	path - Directory path to check or create
 *	mode - Directory permissions
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
util_ckmkdir(char *path, mode_t mode)
{
	char		*dpath = NULL;
	struct stat	stbuf;

#ifdef __VMS
	if ((dpath = util_vms_dirconv(path)) == NULL)
		return FALSE;
#else
	if (!util_newstr(&dpath, path))
		return FALSE;
#endif

	if (stat(dpath, &stbuf) < 0) {
		if (errno == ENOENT) {
			if (mkdir(path, mode) < 0) {
				MEM_FREE(dpath);
				return FALSE;
			}
			(void) chmod(path, mode);
		}
		else {
			MEM_FREE(dpath);
			return FALSE;
		}
	}
	else if (!S_ISDIR(stbuf.st_mode)) {
		MEM_FREE(dpath);
		return FALSE;
	}

	MEM_FREE(dpath);
	return TRUE;
}


/***********************
 *   public routines   *
 ***********************/


/*
 * util_init
 *	Initialize the libutil module.  This should be called before
 *	the calling program does a setuid.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
void
util_init(void)
{
	/* Save original uid and gid */
	ouid = getuid();
	ogid = getgid();

	/* Set uname */
	if (uname(&un) < 0) {
		DBGPRN(DBG_GEN)(errfp, "uname(2) failed (errno=%d)\n", errno);
	}

#ifdef _SCO_SVR3
	(void) strcpy(un.sysname, "SCO_SV");
#else
	if (un.sysname[0] == '\0')
		(void) strcpy(un.sysname, "Unknown");
#endif
}


/*
 * util_start
 *	Start up the libutil module.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing
 */
void
util_start(void)
{
	int	n,
		i;
	char	*p,
		*q;
	char	c;

	/* Initializations for util_text_reduce() */
	p = app_data.exclude_words;
	n = 1;
	if (p != NULL) {
		for (; *p != '\0'; p++) {
			if (isspace((int) *p)) {
				n++;
				while (isspace((int) *p))
					p++;
			}
		}
	}
	excnt = n;

	exclude_words = (char **) MEM_ALLOC(
		"exclude_words",
		excnt * sizeof(char *)
	);
	if (exclude_words == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		_exit(1);
	}

	p = app_data.exclude_words;
	if (p == NULL)
		exclude_words[0] = "";
	else {
		n = 0;
		for (q = p; *q != '\0'; q++) {
			if (!isspace((int) *q))
				continue;
			c = *q;
			*q = '\0';
			exclude_words[n] = (char *) MEM_ALLOC(
				"exclude_words[n]",
				strlen(p) + 1
			);
			if (exclude_words[n] == NULL) {
				(void) fprintf(errfp, "Error: %s\n",
					app_data.str_nomemory
				);
				_exit(1);
			}
			(void) strcpy(exclude_words[n], p);
			n++;
			*q = c;
			p = q + 1;
			while (isspace((int) *p))
				p++;
			q = p;
		}
		exclude_words[n] = (char *) MEM_ALLOC(
			"exclude_words[n]",
			strlen(p) + 1
		);
		if (exclude_words[n] == NULL) {
			(void) fprintf(errfp, "Error: %s\n",
				app_data.str_nomemory
			);
			_exit(1);
		}
		(void) strcpy(exclude_words[n], p);
	}

	exlen = (int *) MEM_ALLOC("exlen", sizeof(int) * excnt);
	if (exlen == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		_exit(1);
	}

	for (i = 0; i < excnt; i++)
		exlen[i] = strlen(exclude_words[i]);

	delcnt = sizeof(delete_words) / sizeof(char *);

	dellen = (int *) MEM_ALLOC("dellen", sizeof(int) * delcnt);
	if (dellen == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		_exit(1);
	}

	for (i = 0; i < delcnt; i++)
		dellen[i] = strlen(delete_words[i]);
}


/*
 * util_onsig
 *      Generic signal handler.  This is intended to break a
 *	thread from a blocked system call and no more.
 *
 * Args:
 *      signo - The signal number
 *
 * Return:
 *      Nothing.
 */
/*ARGSUSED*/
void
util_onsig(int signo)
{
	/* Do nothing */

#if !defined(USE_SIGACTION) && !defined(BSDCOMPAT)
	/* Reset handler */
        (void) util_signal(signo, util_onsig);
#endif
}


/*
 * util_signal
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
(*util_signal(int sig, void (*disp)(int)))(int)
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
 * util_seteuid
 *	Wrapper for the seteuid(2) system call, and deal with platform
 *	support variations.
 *
 * Args:
 *	euid - New effective uid
 */
int
util_seteuid(uid_t euid)
{
#ifdef USE_SETEUID
	return (seteuid(euid));
#endif
#ifdef USE_SETREUID
	return (setreuid(-1, euid));
#endif
#ifdef USE_SETRESUID
	return (setresuid(-1, euid, -1));
#endif
#if !defined(USE_SETEUID) && !defined(USE_SETREUID) && !defined(USE_SETRESUID)
	errno = ENOSYS;
	return -1;
#endif
}


/*
 * util_setegid
 *	Wrapper for the setegid(2) system call, and deal with platform
 *	support variations.
 *
 * Args:
 *	egid - New effective gid
 */
int
util_setegid(gid_t egid)
{
#ifdef USE_SETEGID
	return (setegid(egid));
#endif
#ifdef USE_SETREGID
	return (setregid(-1, egid));
#endif
#ifdef USE_SETRESGID
	return (setresgid(-1, egid, -1));
#endif
#if !defined(USE_SETEGID) && !defined(USE_SETREGID) && !defined(USE_SETRESGID)
	errno = ENOSYS;
	return -1;
#endif
}


/*
 * util_newstr
 *	Allocate memory and make a copy of a text string.  Make sure
 *	*ptr is not uninitialized.  It must either be NULL or point to
 *	a memory location previously allocated with MEM_ALLOC (in which
 *	case it will be freed).  *ptr must never point to statically-allocated
 *	or stack space.  Also, if str is the null string, no memory will
 *	be allocated and *ptr will be set to NULL.
 *
 * Args:
 *	ptr - Location of pointer of the new string.
 *	str - The source string.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
util_newstr(char **ptr, char *str)
{
#ifdef MEM_DEBUG
	char	name[24];
#endif
	if (ptr == NULL)
		return FALSE;

	if (*ptr != NULL)
		MEM_FREE(*ptr);

	if (str == NULL || str[0] == '\0') {
		*ptr = NULL;
		return TRUE;
	}

#ifdef MEM_DEBUG
	(void) strncpy(name, str, sizeof(name)-1);
	name[sizeof(name)-1] = '\0';

	if ((*ptr = (char *) MEM_ALLOC(name, strlen(str)+1)) == NULL)
		return FALSE;
#else
	if ((*ptr = (char *) MEM_ALLOC("util_newstr", strlen(str)+1)) == NULL)
		return FALSE;
#endif

	(void) strcpy(*ptr, str);

	return TRUE;
}


/*
 * util_get_ouid
 *	Get original user ID
 *
 * Args:
 *	None
 *
 * Return:
 *	Original uid value.
 */
uid_t
util_get_ouid(void)
{
	return (ouid);
}


/*
 * util_get_ogid
 *	Get original group ID
 *
 * Args:
 *	None
 *
 * Return:
 *	Original gid value.
 */
gid_t
util_get_ogid(void)
{
	return (ogid);
}


/*
 * util_set_ougid
 *	Change user ID and group ID to original setting.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE on success, FALSE on failure.
 */
bool_t
util_set_ougid(void)
{
	/* Force uid and gid to original setting */
	if (setuid(ouid) < 0) {
		DBGPRN(DBG_GEN)(errfp, "Failed to set uid.\n");
		return FALSE;
	}
	if (setgid(ogid) < 0) {
		DBGPRN(DBG_GEN)(errfp, "Failed to set gid.\n");
		return FALSE;
	}
	return (TRUE);
}


/*
 * util_get_uname
 *	Get the utsname structure for the running system
 *	(See uname(2)).
 *
 * Args:
 *	None
 *
 * Return:
 *	Pointer to the utsname structure.
 */
struct utsname *
util_get_uname(void)
{
	return (&un);
}


/*
 * util_ltobcd
 *	32-bit integer to BCD conversion routine
 *
 * Args:
 *	n - 32-bit integer
 *
 * Return:
 *	BCD representation of n
 */
sword32_t
util_ltobcd(sword32_t n)
{
	return ((n % 10) | ((n / 10) << 4));
}


/*
 * util_bcdtol
 *	BCD to 32-bit integer conversion routine
 *
 * Args:
 *	n - BCD value
 *
 * Return:
 *	integer representation of n
 */
sword32_t
util_bcdtol(sword32_t n)
{
	return ((n & 0x0f) + ((n >> 4) * 10));
}


/*
 * util_stob
 *	String to boolean conversion routine
 *
 * Args:
 *	s - text string "True", "true", "False" or "false"
 *
 * Return:
 *	Boolean value representing the string
 */
bool_t
util_stob(char *s)
{
	if (strcmp(s, "True") == 0 || strcmp(s, "true") == 0 ||
	    strcmp(s, "TRUE") == 0)
		return TRUE;

	return FALSE;
}


/*
 * util_basename
 *	Return the basename of a file path
 *
 * Args:
 *	path - The file path string
 *
 * Return:
 *	The basename string.  This string is dynamically allocated in this
 *	this function and should be freed by the caller using MEM_FREE() when
 *	done.  NULL is returned on errors.
 */
char *
util_basename(char *path)
{
	char		*p,
			*q,
			*tmpbuf;
#ifndef __VMS
	int		i;
#endif

	if (path == NULL)
		return NULL;

	tmpbuf = (char *) MEM_ALLOC("util_basename_tmpbuf", strlen(path) + 1);
	if (tmpbuf == NULL)
		return NULL;

	(void) strcpy(tmpbuf, path);

#ifndef __VMS
	i = strlen(tmpbuf)-1;
	if (tmpbuf[i] == DIR_END)
		tmpbuf[i] = '\0';	/* Zap trailing '/' */
#endif

	if ((p = strrchr(tmpbuf, DIR_END)) == NULL)
		return (tmpbuf);

	p++;

#ifdef __VMS
	if (*p == '\0') {
		/* The supplied path is a directory - special handling */
		*--p = '\0';
		if ((q = strrchr(tmpbuf, DIR_SEP)) != NULL)
			p = q + 1;
		else if ((q = strrchr(tmpbuf, DIR_BEG)) != NULL)
			p = q + 1;
		else if ((q = strrchr(tmpbuf, ':')) != NULL)
			p = q + 1;
		else {
			p = tmpbuf;
			*p = '\0';
		}
	}
#endif

	q = NULL;
	if (!util_newstr(&q, p)) {
		MEM_FREE(tmpbuf);
		return NULL;
	}

	MEM_FREE(tmpbuf);
	return (q);
}


/*
 * util_dirname
 *	Return the dirname of a file path
 *
 * Args:
 *	path - The file path string
 *
 * Return:
 *	The dirname string.  This string is dynamically allocated in this
 *	this function and should be freed by the caller using MEM_FREE() when
 *	done.  NULL is returned on errors.
 */
char *
util_dirname(char *path)
{
	char		*p,
			*q,
			*tmpbuf;

	if (path == NULL)
		return NULL;

	tmpbuf = (char *) MEM_ALLOC("util_basename_tmpbuf", strlen(path) + 1);
	if (tmpbuf == NULL)
		return NULL;

	(void) strcpy(tmpbuf, path);

	if ((p = strrchr(tmpbuf, DIR_END)) == NULL) {
		q = NULL;
		if (!util_newstr(&q, CUR_DIR)) {
			MEM_FREE(tmpbuf);
			return NULL;
		}
		return (q);
	}

#ifdef __VMS
	if (*++p == '\0') {
		/* The supplied path is a directory - special handling */
		if ((q = strrchr(tmpbuf, DIR_SEP)) != NULL) {
			*q = DIR_END;
			*++q = '\0';
		}
		else if ((q = strrchr(tmpbuf, ':')) != NULL)
			*++q = '\0';
		else
			p = tmpbuf;
	}
	*p = '\0';
#else
	if (p == tmpbuf)
		*++p = '\0';
	else
		*p = '\0';
#endif

	q = NULL;
	if (!util_newstr(&q, tmpbuf)) {
		MEM_FREE(tmpbuf);
		return NULL;
	}
	MEM_FREE(tmpbuf);
	return (q);
}


/*
 * util_loginname
 *	Return the login name of the current user
 *
 * Args:
 *	None.
 *
 * Return:
 *	The login name string.
 */
char *
util_loginname(void)
{
#if defined(__VMS) || defined(INSECURE)
	char		*cp;
#endif

#ifdef __VMS
	cp = getlogin();
	if (cp != NULL)
		return (cp);
#else
	struct passwd	*pw;

	/* Get login name from the password file if possible */
	setpwent();
	if ((pw = getpwuid(ouid)) != NULL) {
		endpwent();
		return (pw->pw_name);
	}
	endpwent();

#ifdef INSECURE
	/* Try the LOGNAME environment variable */
	if ((cp = (char *) getenv("LOGNAME")) != NULL)
		return (cp);

	/* Try the USER environment variable */
	if ((cp = (char *) getenv("USER")) != NULL)
		return (cp);
#endif	/* INSECURE */

#endif	/* __VMS */

	/* If we still can't get the login name, just set it
	 * to "nobody" (shrug).
	 */
	return ("nobody");
}


/*
 * util_homedir
 *	Return the home directory path of a user given the uid
 *
 * Args:
 *	uid - The uid of the user
 *
 * Return:
 *	The home directory path name string
 */
char *
util_homedir(uid_t uid)
{
#ifndef __VMS
	struct passwd	*pw;
#ifdef INSECURE
	char		*cp;
#endif

	/* Get home directory from the password file if possible */
	setpwent();
	if ((pw = getpwuid(uid)) != NULL) {
		endpwent();
		return (pw->pw_dir);
	}
	endpwent();

#ifdef INSECURE
	/* Try the HOME environment variable */
	if (uid == ouid && (cp = (char *) getenv("HOME")) != NULL)
		return (cp);
#endif	/* INSECURE */

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
 * util_uhomedir
 *	Return the home directory path of a user given the name
 *
 * Args:
 *	name - The name of the user
 *
 * Return:
 *	The home directory path name string
 */
char *
util_uhomedir(char *name)
{
#ifndef __VMS
	struct passwd	*pw;

	/* Get home directory from the password file if possible */
	setpwent();
	if ((pw = getpwnam(name)) != NULL) {
		endpwent();
		return (pw->pw_dir);
	}
	endpwent();

	/* If we still can't get the home directory, just set it to the
	 * current directory (shrug).
	 */
	return (CUR_DIR);
#else
	char		*cp;
	static char	buf[FILE_PATH_SZ];

	if ((cp = (char *) getenv("HOME")) != NULL &&
	    (int) strlen(cp) < FILE_PATH_SZ) {
		(void) strcpy(buf, cp);
		buf[strlen(buf)-1] = '\0';	/* Drop the "]" */
	}
	else
		(void) strcpy(buf, "SYS$DISK:[");

	return (buf);
#endif
}


/*
 * util_dirstat
 *	Wrapper for the stat(2) or lstat(2) system call.  To be used
 *	only for directories.  Use stat() or LSTAT directory for other
 *	file types.  This function takes care of OS platform
 *	idiosyncrasies.
 *
 * Args:
 *	path - The directory path to stat
 *	stbufp - Return buffer
 *	use_lstat - Specifies whether to use lstat(2) if available.
 *
 * Return:
 *	 0 - success
 *	-1 - failure (errno set)
 */
int
util_dirstat(char *path, struct stat *stbufp, bool_t use_lstat)
{
#ifdef __VMS
	int	ret;
	char	*p = util_vms_dirconv(path);

	if (p == NULL) {
		errno = ENOTDIR;
		return -1;
	}

	ret = stat(p, stbufp);

	MEM_FREE(p);
	return (ret);
#else
	if (use_lstat)
		return (LSTAT(path, stbufp));
	else
		return (stat(path, stbufp));
#endif
}


/*
 * util_mkdir
 *	Wrapper for the mkdir() call.  Will make all needed parent
 *	directories leading to the target directory if they don't
 *	already exist.
 *
 * Args:
 *	path - The directory path to make
 *	mode - The permissions
 *
 * Return:
 *	TRUE - mkdir succeeded or directory already exists
 *	FALSE - mkdir failed
 */
bool_t
util_mkdir(char *path, mode_t mode)
{
	char		*cp,
			*mypath;
#ifdef __VMS
	char		*cp2,
			dev[STR_BUF_SZ],
			vpath[STR_BUF_SZ],
			chkpath[STR_BUF_SZ];
#endif

	mypath = NULL;
	if (!util_newstr(&mypath, path)) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		_exit(1);
	}

#ifndef __VMS
/* UNIX */

	/* Make parent directories, if needed */
	cp = mypath;
	while (*(cp+1) == DIR_BEG)
		cp++;	/* Skip extra initial '/'s if absolute path */

	while ((cp = strchr(cp+1, DIR_SEP)) != NULL) {
		*cp = '\0';

		if (!util_ckmkdir(mypath, mode)) {
			MEM_FREE(mypath);
			return FALSE;
		}

		*cp = DIR_SEP;
	}

	if (!util_ckmkdir(mypath, mode)) {
		MEM_FREE(mypath);
		return FALSE;
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
			if (util_strcasecmp(cp2, ".dir") == 0) {
				*cp2 = '\0';
				(void) strcpy(vpath, cp);
			}
			else {
				(void) fprintf(errfp, "util_mkdir: "
					"Illegal directory path: %s\n",
					mypath
				);
				return FALSE;
			}
		}
	}
	else {
		(void) strcpy(vpath, ++cp);
		if ((cp = strrchr(vpath, DIR_END)) != NULL) {
			*cp ='\0';
			if ((cp2 = strchr(cp+1, '.')) != NULL) {
				if (util_strcasecmp(cp2, ".dir") == 0) {
					*cp2 = '\0';
					*cp = DIR_SEP;
				}
				else {
					(void) fprintf(errfp, "util_mkdir: "
						"Illegal directory path: %s\n",
						mypath
					);
					return FALSE;
				}
			}
			else if (*(cp+1) != '\0') {
				(void) fprintf(errfp, "util_mkdir: "
					"Illegal directory path: %s\n",
					mypath
				);
				return FALSE;
			}
		}
	}

	cp = vpath;
	while ((cp = strchr(cp, DIR_SEP)) != NULL) {
		*cp = '\0';

		(void) sprintf(chkpath, "%s%s[%s]",
			dev, dev[0] == '\0' ? "" : ":", vpath
		);

		*cp++ = DIR_SEP;

		if (!util_ckmkdir(chkpath, mode)) {
			MEM_FREE(mypath);
			return FALSE;
		}
	}

	if (vpath[0] == '\0') {
		(void) sprintf(chkpath, "%s%s[000000]",
			dev, dev[0] == '\0' ? "" : ":"
		);

		if (!util_ckmkdir(chkpath, mode)) {
			MEM_FREE(mypath);
			return FALSE;
		}

		(void) chmod(chkpath, mode);
	}
	else if (strcmp(vpath, "000000") != 0) {
		(void) sprintf(chkpath, "%s%s[%s]",
			dev, dev[0] == '\0' ? "" : ":", vpath
		);

		if (!util_ckmkdir(chkpath, mode)) {
			MEM_FREE(mypath);
			return FALSE;
		}

		(void) chmod(chkpath, mode);
	}

#endif	/* __VMS */

	MEM_FREE(mypath);
	return TRUE;
}


/*
 * util_setperm
 *	Set the file permissions of a file.
 *
 * Args:
 *	path - file path name
 *
 * Return:
 *	Nothing
 */
void
util_setperm(char *path, char *modestr)
{
	unsigned int	mode;

	(void) sscanf(modestr, "%o", &mode);

	/* Make sure the file is at least readable to the user just
	 * in case mode is bogus.
	 */
	mode |= S_IRUSR;

	/* Turn off extraneous bits */
	mode &= ~(S_ISUID | S_ISGID | S_IXUSR | S_IXGRP | S_IXOTH);

	/* Set file permission */
	(void) chmod(path, (mode_t) mode);
}


/*
 * util_monname
 *	Convert an interger month to an abbreviated 3-letter month
 *	name string.
 *
 * Args:
 *	mon - The integer month (0 is January, 11 is December).
 *
 * Return:
 *	The month name string
 */
char *
util_monname(int mon)
{
	if (mon < 0 || mon >= 12)
		return ("???");
	return (mon_name[mon]);
}


/*
 * util_isexecutable
 *	Verify executability, given a path to a program
 *
 * Args:
 *	path - the absolute path to the program executable
 *
 * Return:
 *	TRUE - It is executable
 *	FALSE - It is not executable
 */
bool_t
util_isexecutable(char *path)
{
#ifdef __VMS
	return TRUE;	/* shrug */
#else
	char		**cp;
	struct group	*gr;
	struct stat	stbuf;

	if (stat(path, &stbuf) < 0 || !S_ISREG(stbuf.st_mode))
		/* Cannot access file or file is not regular */
		return FALSE;

	if (ouid == 0)
		/* Root can execute any file */
		return TRUE;

	if ((stbuf.st_mode & S_IXUSR) == S_IXUSR && ouid == stbuf.st_uid)
		/* The file is executable, and is owned by the user */
		return TRUE;

	if ((stbuf.st_mode & S_IXGRP) == S_IXGRP) {
		if (ogid == stbuf.st_gid)
			/* The file is group executable, and the
			 * user's current gid matches the group.
			 */
			return TRUE;

		setgrent();
		if ((gr = getgrgid(stbuf.st_gid)) != NULL) {
			for (cp = gr->gr_mem; cp != NULL && *cp != '\0'; cp++) {
				/* The file is group executable, and the
				 * user is a member of that group.
				 */
				if (strcmp(*cp, util_loginname()) == 0)
					return TRUE;
			}
		}
		endgrent();
	}

	if ((stbuf.st_mode & S_IXOTH) == S_IXOTH)
		/* The file is executable by everyone */
		return TRUE;

	return FALSE;
#endif	/* __VMS */
}


/*
 * util_findcmd
 *	Given a command name, return the full path name to it.
 *
 * Args:
 *	cmd - The command name string
 *
 * Return:
 *	The full path name to the command, or NULL if not found.
 *	The string buffer is dynamically allocated in this function
 *	and should be deallocated by the caller using MEM_FREE() when
 *	done.
 */
char *
util_findcmd(char *cmd)
{
#ifdef __VMS
	return NULL;	/* shrug */
#else
	char	*p,
		*q,
		*r,
		*env,
		*path,
		c,
		c2;

	if (cmd == NULL)
		return NULL;

	/* Cut out just the argv[0] portion */
	c = '\0';
	if ((p = strchr(cmd,' ')) != NULL || (p = strchr(cmd,'\t')) != NULL) {
		c = *p;
		*p = '\0';
	}

	if (cmd[0] == '/') {
		/* Absolute path specified */
		if (!util_isexecutable(cmd)) {
			if (p != NULL)
				*p = c;
			return NULL;
		}

		path = NULL;
		if (!util_newstr(&path, cmd)) {
			if (p != NULL)
				*p = c;
			return NULL;
		}

		return (path);
	}

	/* Relative path: walk PATH and look for the executable */
	if ((env = getenv("PATH")) == NULL) {
		if (p != NULL)
			*p = c;
		/* PATH unknown */
		return NULL;
	}

	/* Walk the PATH */
	for (q = env; (r = strchr(q, ':')) != NULL; *r = c2, q = ++r) {
		c2 = *r;
		*r = '\0';

		path = (char *) MEM_ALLOC(
			"findcmd_path",
			strlen(q) + strlen(cmd) + 2
		);
		if (path == NULL)
			return NULL;	/* shrug */

		(void) sprintf(path, "%s/%s", q, cmd);

		if (util_isexecutable(path)) {
			*r = c2;
			if (p != NULL)
				*p = c;
			return (path);
		}

		MEM_FREE(path);
	}
	/* Check last component in PATH */
	path = (char *) MEM_ALLOC(
		"findcmd_path",
		strlen(q) + strlen(cmd) + 2
	);
	if (path == NULL) {
		if (p != NULL)
			*p = c;
		return NULL;	/* shrug */
	}

	(void) sprintf(path, "%s/%s", q, cmd);

	if (util_isexecutable(path)) {
		if (p != NULL)
			*p = c;
		return (path);
	}

	MEM_FREE(path);
	if (p != NULL)
		*p = c;

	return NULL;

#endif	/* __VMS */
}


/*
 * util_checkcmd
 *	Check a command for sanity before running it.
 *
 * Args:
 *	cmd - The command string
 *
 * Return:
 *	TRUE - Command is sane
 *	FALSE - Command is not sane
 */
bool_t
util_checkcmd(char *cmd)
{
#ifdef __VMS
	return TRUE;	/* shrug */
#else
	char	*path;

	if ((path = util_findcmd(cmd)) == NULL)
		return FALSE;

	MEM_FREE(path);
	return TRUE;
#endif	/* __VMS */
}


/*
 * util_runcmd
 *	Set uid and gid to the original user and spawn an external command.
 *
 * Args:
 *	cmd - Command string.
 *	workproc - Function to call when waiting for child process,
 *		   or NULL if no workproc.
 *	workarg - Argument to pass to workproc.
 *
 * Return:
 *	The exit status of the command.
 */
int
util_runcmd(char *cmd, void (*workproc)(int), int workarg)
{
#ifndef __VMS
	int		ret,
			fd;
	pid_t		cpid;
	waitret_t	wstat;

	if (!util_checkcmd(cmd))
		return EXE_ERR;

	/* Fork child to invoke external command */
	switch (cpid = FORK()) {
	case 0:
		break;

	case -1:
		/* Fork failed */
		perror("util_runcmd: fork() failed");
		return EXE_ERR;

	default:
		/* Parent process: wait for child to exit */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     workproc, workarg, TRUE, &wstat);
		if (ret < 0)
			ret = (errno << 8);
		else if (WIFEXITED(wstat))
			ret = WEXITSTATUS(wstat);
		else if (WIFSIGNALED(wstat))
			ret = (WTERMSIG(wstat) << 8) | 0xff;
		else
			ret = EXE_ERR;

		DBGPRN(DBG_GEN)(errfp, "\nCommand exit status %d\n", ret);
		return (ret);
	}

	/* Force uid and gid to original setting */
	if (!util_set_ougid())
		_exit(errno);

	/* Child process */
	for (fd = 3; fd < 255; fd++) {
		/* Close unneeded file descriptors */
		(void) close(fd);
	}

	/* Restore SIGTERM */
	(void) util_signal(SIGTERM, SIG_DFL);

	/* Exec a shell to run the command */
	DBGPRN(DBG_GEN)(errfp, "Command: [%s]\n", cmd);
	(void) execl(STR_SHPATH, STR_SHNAME, STR_SHARG, cmd, NULL);
	_exit(EXE_ERR);
	/*NOTREACHED*/
#else
	int	ret;

	/* Do the command */
	DBGPRN(DBG_GEN)(errfp, "Command: [%s]\n", cmd);
	ret = system(cmd);

	DBGPRN(DBG_GEN)(errfp, "Command exit status %d\n", ret);
	return (ret);
#endif	/* __VMS */
}


/*
 * util_isqrt
 *	Fast integer-based square root routine
 *
 * Args:
 *	n - The integer value whose square-root is to be taken
 *
 * Return:
 *	Resultant square-root integer value
 */
int
util_isqrt(int n)
{
	int	a, b, c, as, bs;

	a = 1;
	b = 1;
	while (a <= n) {
		a = a << 2;
		b = b << 1;
	}
	as = 0;
	bs = 0;
	while (b > 1 && n > 0) {
		a = a >> 2;
		b = b >> 1;
		c = n - (as | a);
		if (c >= 0) {
			n = c;
			as |= (a << 1);
			bs |= b;
		}
		as >>= 1;
	}

	return (bs);
}


/*
 * util_blktomsf
 *	CD logical block to MSF conversion routine
 *
 * Args:
 *	blk - The logical block address
 *	ret_min - Minute (return)
 *	ret_sec - Second (return)
 *	ret_frame - Frame (return)
 *	offset - Additional logical block address offset
 *
 * Return:
 *	Nothing.
 */
void
util_blktomsf(
	sword32_t	blk,
	byte_t		*ret_min,
	byte_t		*ret_sec,
	byte_t		*ret_frame,
	sword32_t	offset)
{
	sword32_t	n = blk + offset;

	*ret_min = (byte_t) ((n / FRAME_PER_SEC) / 60);
	*ret_sec = (byte_t) ((n / FRAME_PER_SEC) % 60);
	*ret_frame = (byte_t) (n % FRAME_PER_SEC);
}


/*
 * util_msftoblk
 *	CD MSF to logical block conversion routine
 *
 * Args:
 *	min - Minute
 *	sec - Second
 *	frame - Frame
 *	ret_blk - The logical block address (return)
 *	offset - Additional logical block address offset
 *
 * Return:
 *	Nothing.
 */
void
util_msftoblk(
	byte_t		min,
	byte_t		sec,
	byte_t		frame,
	sword32_t	*ret_blk,
	sword32_t	offset)
{
	*ret_blk = (FRAME_PER_SEC * (min * 60 + sec) + frame - offset);
}


/*
 * util_xlate_blk
 *	Translate the CD-ROM drive's native LBA/length into an
 *	LBA/length that is based on 2048-byte blocks.  This is
 *	needed to support drives that are configured to a block
 *	size other than 2048 (such as older Sun or SGI).
 *
 * Args:
 *	val - The LBA address or length to translate
 *
 * Return:
 *	The converted LBA.
 */
sword32_t
util_xlate_blk(sword32_t val)
{
	/* If the address is a "signed" negative 3-byte integer, extend
	 * it to 4 bytes.
	 */
	if (val & 0x800000)
		val |= 0xff000000;

	if (app_data.drv_blksz < STD_CDROM_BLKSZ)
		val /= (STD_CDROM_BLKSZ / app_data.drv_blksz);
	else if (app_data.drv_blksz > STD_CDROM_BLKSZ)
		val *= (app_data.drv_blksz / STD_CDROM_BLKSZ);

	return (val);
}


/*
 * util_unxlate_blk
 *	Translate an LBA/length that is based on 2048-byte blocks
 *	into the CD-ROM drive's native LBA/length.  This is needed
 *	to support drives that are configured to a block size other
 *	than 2048 (such as older Sun or SGI).
 *
 * Args:
 *	val - The LBA address or length to translate
 *
 * Return:
 *	The converted LBA
 */
sword32_t
util_unxlate_blk(sword32_t val)
{
	if (app_data.drv_blksz < STD_CDROM_BLKSZ)
		val *= (STD_CDROM_BLKSZ / app_data.drv_blksz);
	else if (app_data.drv_blksz > STD_CDROM_BLKSZ)
		val /= (app_data.drv_blksz / STD_CDROM_BLKSZ);

#if 1
	/* If the address is negative, set it to zero */
	if (val & 0x80000000)
		val = 0;
#else
	/* If the address is negative, reduce to a signed negative
	 * 3-byte integer.
	 */
	if (val & 0x80000000) {
		val &= ~0xff000000;
		val |= 0x800000;
	}
#endif

	return (val);
}


/*
 * util_taper_vol
 *	Translate the volume level based on the configured taper
 *	characteristics.
 *
 * Args:
 *	v - The linear volume value.
 *
 * Return:
 *	The curved volume value.
 */
int
util_taper_vol(int v)
{
	int	a,
		b,
		c;

	c = MAX_VOL >> 1;

	switch (app_data.vol_taper) {
	case 1:
		/* squared taper */
		a = SQR(v);
		b = a / MAX_VOL;
		if ((a % MAX_VOL) >= c)
			b++;
		return (b);
	case 2:
		/* inverse-squared taper */
		a = SQR(MAX_VOL - v);
		b = a / MAX_VOL;
		if ((a % MAX_VOL) >= c)
			b++;
		return (MAX_VOL - b);
	case 0:
	default:
		/* linear taper */
		return (v);
	}
	/*NOTREACHED*/
}


/*
 * util_untaper_vol
 *	Translate the volume level based on the configured taper
 *	characteristics.
 *
 * Args:
 *	v - The curved volume value.
 *
 * Return:
 *	The linear volume value.
 */
int
util_untaper_vol(int v)
{
	switch (app_data.vol_taper) {
	case 1:
		/* squared taper */
		return (util_isqrt(v * 100));
	case 2:
		/* inverse-squared taper */
		return (MAX_VOL - util_isqrt(SQR(MAX_VOL) - (MAX_VOL * v)));
	case 0:
	default:
		/* linear taper */
		return (v);
	}
	/*NOTREACHED*/
}


/*
 * util_scale_vol
 *	Scale logical audio volume value (0-100) to an 8-bit value
 *	(0-0xff) range.
 *
 * Args:
 *	v - The logical volume value
 *
 * Return:
 *	The scaled volume value
 */
int
util_scale_vol(int v)
{
	/* Convert logical audio volume value to 8-bit volume */
	return ((v * (0xff - app_data.base_scsivol) / MAX_VOL) +
	        app_data.base_scsivol);
}


/*
 * util_unscale_vol
 *	Scale an 8-bit audio volume parameter value (0-0xff) to the
 *	logical volume value (0-100).
 *
 * Args:
 *	v - The 8-bit volume value
 *
 * Return:
 *	The logical volume value
 */
int
util_unscale_vol(int v)
{
	int	val;

	/* Convert 8-bit audio volume value to logical volume */
	val = (v - app_data.base_scsivol) * MAX_VOL /
	      (0xff - app_data.base_scsivol);

	return ((val < 0) ? 0 : val);
}


/*
 * util_delayms
 *	Suspend execution for the specified number of milliseconds
 *
 * Args:
 *	msec - The number of milliseconds
 *
 * Return:
 *	Nothing.
 */
void
util_delayms(unsigned long msec)
{
#ifdef USE_SELECT
	struct timeval	to;

	to.tv_sec = (long) msec / 1000;
	to.tv_usec = ((long) msec % 1000) * 1000;

	(void) select(0, NULL, NULL, NULL, &to);
#else
#ifdef USE_POLL
	(void) poll(NULL, 0, (int) msec);
#else
#ifdef USE_NAP
	(void) nap((long) msec);
#else
#ifdef USE_USLEEP
	(void) usleep((long) msec * 1000);
#else
#ifdef USE_NANOSLEEP
	struct timespec	ts;

	ts.tv_sec = (long) msec / 1000;
	ts.tv_nsec = ((long) msec % 1000) * 1000000;

	(void) nanosleep(&ts, NULL);
#else
#ifdef USE_PTHREAD_DELAY_NP
	struct timespec	ts;

	ts.tv_sec = (long) msec / 1000;
	ts.tv_nsec = ((long) msec % 1000) * 1000000;

	(void) pthread_delay_np(&ts);
#else
	/* shrug: Rounded to the nearest second, with a minimum of 1 second */
	if (msec < 1000)
		(void) sleep(1);
	else
		(void) sleep(((unsigned int) msec + 500) / 1000);
#endif	/* USE_PTHREAD_DELAY_NP */
#endif	/* USE_NANOSLEEP */
#endif	/* USE_USLEEP */
#endif	/* USE_NAP */
#endif	/* USE_POLL */
#endif	/* USE_SELECT */
}


/*
 * util_waitchild
 *	Wrapper for waitpid(2), allowing timeouts and running work functions
 *	while we wait for the child to exit.
 *
 * Args:
 *	cpid - The child pid to wait on.
 *	tmout - Timeout in seconds.
 *	workproc - Work function to run while waiting (can be NULL).
 *	workarg - Argument passed to workfunc.
 *	itimer - Whether to use the itimer facility to interrupt the
 *		 wait and run the workproc periodically.  If this is
 *		 FALSE, and a workproc is specified, then alarm() is
 *		 used instead.
 *	wstatp - Pointer to the location where the wait status is returned
 *		 back to the caller.
 *
 * Return:
 *	Return value from the waitpid call.
 */
int
util_waitchild(
	pid_t		cpid,
	time_t		tmout,
	void		(*workproc)(int),
	int		workarg,
	bool_t		itimer,
	waitret_t	*wstatp
)
{
	int			saverr = 0,
				sigcnt,
				ret;
	time_t			begin,
				now;
	unsigned int		oa = 0;
	void			(*oh)(int) = NULL;
#ifdef HAS_ITIMER
	struct itimerval	it,
				oit;
#endif
#ifdef USE_SIGPROCMASK
	sigset_t		sigs;

	(void) sigemptyset(&sigs);
	(void) sigaddset(&sigs, SIGCHLD);
	(void) sigaddset(&sigs, SIGALRM);
	(void) sigaddset(&sigs, SIGPIPE);
#endif	/* USE_SIGPROCMASK */
#ifdef USE_SIGBLOCK
	int		sigs,
			osigs;

	sigs = sigmask(SIGCHLD) | sigmask(SIGALRM) | sigmask(SIGPIPE);
#endif	/* USE_SIGBLOCK */

	if (workproc != NULL) {
		/* Use SIGALRM to interrupt the wait */
		oh = util_signal(SIGALRM, util_onsig);

#ifdef HAS_ITIMER
		if (itimer) {
			it.it_interval.tv_sec = 0;
			it.it_interval.tv_usec = 100000;
			it.it_value.tv_sec = 1;
			it.it_value.tv_usec = 0;

			(void) setitimer(ITIMER_REAL, &it, &oit);
		}
		else
#endif
		{
			oa = alarm(1);
		}
	}

	sigcnt = 0;
	begin = time(NULL);
	while ((ret = WAITPID(cpid, wstatp, 0)) != cpid) {
		if (workproc != NULL) {
#ifdef HAS_ITIMER
			if (!itimer)
#endif
				(void) alarm(0);
		}

		if (ret < 0) {
			/* Allow for a few spurious returns */
			if (errno == ECHILD && ++sigcnt > 5) {
				saverr = errno;
				break;
			}
			if (errno != EINTR) {
				saverr = errno;
				break;
			}
		}
		else if (ret > 0) {
			/* We should not get here */
			DBGPRN(DBG_GEN)(errfp,
			    "\nutil_waitchild: waitpid returned invalid pid "
			    "(expect=%d, got=%d)\n",
			    (int) cpid, ret
			);
			saverr = EINVAL;
			ret = -1;
			break;
		}

		now = time(NULL);
		if ((now - begin) > tmout) {
			/* Timeout: kill child */
			(void) kill(cpid, SIGTERM);
			DBGPRN(DBG_GEN)(errfp,
			    "\nutil_waitchild: timed out waiting for pid=%d\n",
			    (int) cpid
			);
		}

		if (workproc != NULL) {
			/* Block SIGCHLD while we do work */
#ifdef USE_SIGPROCMASK
			(void) sigprocmask(SIG_BLOCK, &sigs, NULL);
#endif
#ifdef USE_SIGBLOCK
			oldsigs = sigblock(sigs);
#endif

			/* Do some work */
			(*workproc)(workarg);

			/* Unblock SIGCHLD */
#ifdef USE_SIGPROCMASK
			(void) sigprocmask(SIG_UNBLOCK, &sigs, NULL);
#endif
#ifdef USE_SIGBLOCK
			(void) sigsetmask(oldsigs);
#endif

#ifdef HAS_ITIMER
			if (!itimer)
#endif
				(void) alarm(1);
		}
	}

	if (workproc != NULL) {
#ifdef HAS_ITIMER
		if (itimer)
			(void) setitimer(ITIMER_REAL, &oit, NULL);
		else
#endif
			(void) alarm(oa);

		(void) util_signal(SIGALRM, oh);
	}

	errno = saverr;
	return ((bool_t) ret == 0);
}


/*
 * util_strstr
 *	If s2 is a substring of s1, return a pointer to s2 in s1.  Otherwise,
 *	return NULL.
 *
 * Args:
 *	s1 - The first text string.
 *	s2 - The second text string.
 *
 * Return:
 *	Pointer to the beginning of the substring, or NULL if not found.
 */
char *
util_strstr(char *s1, char *s2)
{
	int	n;

	if (s1 == NULL || s2 == NULL)
		return NULL;

	n = strlen(s2);
	for (; *s1 != '\0'; s1++) {
		if (strncmp(s1, s2, n) == 0)
			return s1;
	}
	return NULL;
}


/*
 * util_strcasecmp
 *	Compare two strings a la strcmp(), except it is case-insensitive.
 *
 * Args:
 *	s1 - The first text string.
 *	s2 - The second text string.
 *
 * Return:
 *	Compare value.  See strcmp(3).
 */
int
util_strcasecmp(char *s1, char *s2)
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
	if (buf1 == NULL || buf2 == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		_exit(1);
	}

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


/*
 * util_strncasecmp
 *	Compare two strings a la strncmp(), except it is case-insensitive.
 *
 * Args:
 *	s1 - The first text string.
 *	s2 - The second text string.
 *	n - number of characters to compare.
 *
 * Return:
 *	Compare value.  See strncmp(3).
 */
int
util_strncasecmp(char *s1, char *s2, int n)
{
	char	*buf1,
		*buf2,
		*p;
	int	ret;

	if (s1 == NULL || s2 == NULL)
		return 0;

	/* Allocate tmp buffers */
	buf1 = (char *) MEM_ALLOC("strncasecmp_buf1", strlen(s1)+1);
	buf2 = (char *) MEM_ALLOC("strncasecmp_buf2", strlen(s2)+1);
	if (buf1 == NULL || buf2 == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		_exit(1);
	}

	/* Convert both strings to lower case and store in tmp buffer */
	for (p = buf1; *s1 != '\0'; s1++, p++)
		*p = (char) ((isupper((int) *s1)) ? tolower((int) *s1) : *s1);
	*p = '\0';
	for (p = buf2; *s2 != '\0'; s2++, p++)
		*p = (char) ((isupper((int) *s2)) ? tolower((int) *s2) : *s2);
	*p = '\0';

	ret = strncmp(buf1, buf2, n);

	MEM_FREE(buf1);
	MEM_FREE(buf2);

	return (ret);
}


/*
 * util_text_reduce
 *	Reduce a text string to become suitable for use in a keyword search
 *	operation.
 *
 * Args:
 *	str - The input text string
 *
 * Return:
 *	The output text string.  The string buffer is allocated internally
 *	and should be freed by the caller via MEM_FREE().  If an error
 *	occurs, NULL is returned.
 */
char *
util_text_reduce(char *str)
{
	int		i,
			lastex,
			*len;
	char		last,
			next,
			*p1,
			*p2,
			*pr,
			**t,
			*newstr;

	if ((newstr = (char *) MEM_ALLOC("text_reduce_newstr",
					 strlen(str) + 1)) == NULL)
		return NULL;

	p1 = str;
	p2 = newstr;
	pr = newstr;

	last = ' ';
	lastex = -1;

	while (*p1 != '\0') {
		next = *p1;

		for (i = 0, t = delete_words, len = dellen; i < delcnt;
		     i++, t++, len++) {
			if (strncmp(p1, *t, *len) == 0) {
				p1 += *len - 1;
				next = ' ';
				break;
			}
		}

		if (!isalnum((int) last) && !isalnum((int) next)) {
			p1++;
			continue;
		}

		for (i = 0, t = exclude_words, len = exlen; i < excnt;
		    i++, t++, len++) {
			if (lastex != i && !isalnum((int) last) &&
			    util_strncasecmp(p1, *t, *len) == 0 &&
			    !isalnum((int) p1[*len])) {
				p1 += *len;
				lastex = i;
				break;
			}
		}

		if (i < excnt)
			continue;

		if (isalnum((int) next))
			*p2 = next;
		else
			*p2 = ' ';

		last = next;
		p2++;
		p1++;

		if (isalnum((int) next)) {
			lastex = -1;
			pr = p2;
		}
	}

	*pr = '\0';
	return (newstr);
}


/*
 * util_cgi_xlate
 *	Translate a keyword string into CGI form.  It substitutes whitespaces
 *	with the proper separator, conversion to lower case, handles non-
 *	alphanumerdic character translations, etc.
 *
 * Args:
 *	str - The input keywords string, separated by whitespace
 *
 * Return:
 *	The output text string.  The string buffer is allocated internally
 *	and should be freed by the caller via MEM_FREE().  If an error
 *	occurs, NULL is returned.
 */
char *
util_cgi_xlate(char *str)
{
	char	*p,
		*q,
		*new_str;

	if ((q = new_str = (char *) MEM_ALLOC("cgi_xlate_newstr",
					      (strlen(str) * 3) + 5)) == NULL)
		return NULL;

	/* Skip leading whitespaces */
	p = str;
	while (isspace((int) *p))
		p++;

	/* Process the string */
	while (*p != '\0') {
		if (isspace((int) *p)) {
			/* Skip remaining consecutive white spaces */
			while (isspace((int) *(++p)))
				;
			if (*p == '\0')
				break;	/* End of string reached */
			else {
				/* Substitute white spaces with separator */
				*q = '+';
				q++;
			}
		}
		else if ((ispunct((int) *p) &&
			  *p != '_' && *p != '.' &&
			  *p != '*' && *p != '@') ||
			 (*p & 0x80)) {
			/* Need URL-encoding */
			(void) sprintf(q, "%%%02X", (int) (*p));
			q += 3;
			p++;
		}
		else if (isprint((int) *p)) {
			/* Printable character */
			*q = (char) (isupper((int) *p) ?
				tolower((int) *p) : (*p));
			q++;
			p++;
		}
		else
			p++;
	}
	*q = '\0';

	return (new_str);
}


/*
 * util_urlchk
 *	Check a URL for syntax and to see if it's local or remote.
 *	Return a pointer to the actual start of the path name (past the
 *	protocol, hostname and port portion if it's local), and the
 *	length of the URL string.
 *
 * Args:
 *	url - Ths URL string to check
 *	filepath - The character pointer to set to the beginning of the
 *		   file path.
 *	len - The length of the URL string
 *
 * Return:
 *	A bitmap consisting of the following bits:
 *		IS_LOCAL_URL
 *		IS_REMOTE_URL
 *		NEED_PREPEND_HTTP
 *		NEED_PREPEND_FTP
 *		NEED_PREPEND_MAILTO
 */
int
util_urlchk(char *url, char **filepath, int *len)
{
	int	i,
		ret;
	char	*p,
		*q,
		*p1,
		*p2,
		*p3,
		*p4,
		*p5,
		*p6,
		*p7,
		*endstr,
		sav;

	ret = 0;
	*filepath = url;
	endstr = url + strlen(url);

	/* Look for the end of the URL in case we're in the beginning of
	 * a multi-word string
	 */
	if ((p2 = strchr(url, ' ')) == NULL)
		p2 = endstr;
	if ((p3 = strchr(url, '(')) == NULL)
		p3 = endstr;
	if ((p4 = strchr(url, ')')) == NULL)
		p4 = endstr;
	if ((p5 = strchr(url, '\t')) == NULL)
		p5 = endstr;
	if ((p6 = strchr(url, '\r')) == NULL)
		p6 = endstr;
	if ((p7 = strchr(url, '\n')) == NULL)
		p7 = endstr;

	p1 = (p2 < p3) ? p2 : p3;
	p2 = (p4 < p5) ? p4 : p5;
	p3 = (p6 < p7) ? p6 : p7;
	p1 = (p1 < p2) ? p1 : p2;
	p1 = (p1 < p3) ? p1 : p3;

	sav = *p1;
	*p1 = '\0';

	/* Check to see if it's a URL */
	if (util_strncasecmp(url, "www.", 4) == 0) {
		/* Possibly: www.xyz.com */
		p = url + 4;
		if (*p == '\0') {
			/* Failed check: incomplete URL */
			ret = 0;
		}
		else
			ret = (NEED_PREPEND_HTTP | IS_REMOTE_URL);
	}
	else if (util_strncasecmp(url, "ftp.", 4) == 0) {
		/* Possibly: ftp.xyz.com */
		p = url + 4;
		if (*p == '\0') {
			/* Failed check: incomplete URL */
			ret = 0;
		}
		else
			ret = (NEED_PREPEND_FTP | IS_REMOTE_URL);
	}
	else if (util_strncasecmp(url, "mailto:", 7) != 0 &&
		 (p = strchr(url, '@')) != NULL) {
		/* Possibly: user@xyz.com */
		if (p == url || *(++p) == '\0') {
			/* Failed check: @ is at beginning or end of word */
			ret = 0;
		}
		else
			ret = (NEED_PREPEND_MAILTO | IS_REMOTE_URL);
	}
	else {
		/* Check against list of URL protos */
		for (i = 0; url_protolist[i].protocol != NULL; i++) {
			if (util_strncasecmp(url, url_protolist[i].protocol,
				    strlen(url_protolist[i].protocol)) != 0) {
				continue;
			}

			if (url_protolist[i].islocal)
				ret = IS_LOCAL_URL;
			else
				ret = IS_REMOTE_URL;
			break;
		}

		if (ret == 0) {
			/* Didn't match any URL protos: assume to be a
			 * local file path but not a URL.
			 */
			*len = strlen(url);
			*p1 = sav;
			return (ret);
		}

		if (ret == IS_LOCAL_URL) {
			if (util_strncasecmp(url, "file://localhost", 16) == 0)
				p2 = url + 16;
			else if (util_strncasecmp(url, "file://", 7) == 0) {
				p2 = url + 7;
				if ((q = strchr(p2, '/')) != NULL)
					p2 = q + 1;
				else
					p2 += strlen(p2);
			}
			else if (util_strncasecmp(url, "file:", 5) == 0)
				p2 = url + 5;

			*filepath = p2;
		}

		if ((p = strchr(url, ':')) == NULL) {
			/* Failed check: no colon */
			ret = 0;
		}
		if (*(++p) == '\0') {
			/* Failed check: incomplete URL */
			ret = 0;
		}
	}

	/* A general sanity check for remote URLs */
	if ((ret & IS_REMOTE_URL) &&
	    ((p3 = strchr(p, '.')) == NULL || p3 == p || *(p3+1) == '\0' ||
	     !isalnum((int) *(p3-1)) || !isalnum((int) *(p3+1)))) {
		/* Failed check: Expect at least one dot and there
		 * has to be at least a valid character before and after
		 * the dot.
		 */
		ret = 0;
	}

	*len = (ret == 0) ? 0 : (int) strlen(url);
	*p1 = sav;

	return (ret);
}


/*
 * util_urlencode
 *	Fix a string to become a legal URL.  In particular, special character
 *	encodings are performed.
 *
 * Args:
 *	str - Input string
 *
 * Return:
 *	Output string.  This is internally allocated and should be
 *	freed by the caller via MEM_FREE() when done.
 */
char *
util_urlencode(char *str)
{
	char	*p,
		*q,
		*r;

	if (str == NULL || str[0] == '\0')
		return NULL;

	if ((p = (char *) MEM_ALLOC("urlenc", strlen(str) * 3 + 1)) == NULL)
		return NULL;

	for (q = str, r = p; *q != '\0'; q++) {
		switch (*q) {
		case '%':
		case ' ':
		case '\t':
		case '?':
		case '"':
		case '`':
		case '\'':
		case '|':
		case '~':
		case '#':
		case ':':
		case ';':
		case '@':
		case '=':
		case '&':
		case '<':
		case '[':
		case ']':
		case '(':
		case ')':
		case '{':
		case '}':
			/* Encode to hex notation */
			(void) sprintf(r, "%%%02X", *q);
			r += 3;
			break;
		default:
			if ((unsigned int) *q <= 0x1f ||
			    (unsigned int) *q >= 0x7f) {
				/* Control characters or extended ASCII */
				/* Encode to hex notation */
				(void) sprintf(r, "%%%02X", *q);
				r += 3;
				break;
			}
			/* Copy verbatim */
			*r++ = *q;
			break;
		}
	}
	*r = '\0';

	return (p);
}


/*
 * util_html_fputs
 *	Similar to fputs(3), but handles HTML escape sequence, newline and
 *	tab translation.  If specified, it also tries to recognizes URLs
 *	in the string and turns them into links.  In addition, the caller
 *	may specify the font.
 *
 * Args:
 *	str - The string to write
 *	fp - The file stream pointer
 *	urldet - Whether to look for URLs and turn them into links
 *	fontname - The font name to use, or NULL to use default font
 *	fontsize - The font size to use. or 0 to use default size
 *
 * Return:
 *	Nothing.
 */
void
util_html_fputs(
	char	*str,
	FILE	*fp,
	bool_t	urldet,
	char	*fontname,
	int	fontsize
)
{
	int	i,
		j,
		k,
		len,
		urlt;
	char	*cp,
		*p;

	if (fontname != NULL || fontsize != 0) {
		(void) fputs("<FONT", fp);
		if (fontname != NULL)
			(void) fprintf(fp, " FACE=\"%s\"", fontname);
		if (fontsize != 0)
			(void) fprintf(fp, " SIZE=\"%d\"", fontsize);
		(void) fputs(">\n", fp);
	}

	i = k = 0;
	for (cp = str; *cp != '\0'; cp++) {
		switch (*cp) {
		case ' ':
			(void) fprintf(fp, "%s",
				(k > 0) ?
				    HTML_ESC_SPC :
				    ((*(cp + 1) == ' ') ? HTML_ESC_SPC : " ")
			);
			k++;
			break;
		case '\n':
			(void) fputs("<BR>\n", fp);
			i = -1;
			k = 0;
			break;
		case '\t':
			for (j = CHARS_PER_TAB; j > i; j--)
				(void) fputs(HTML_ESC_SPC, fp);
			i = -1;
			k = 0;
			break;
		case '<':
			(void) fputs("&lt;", fp);
			k = 0;
			break;
		case '>':
			(void) fputs("&gt;", fp);
			k = 0;
			break;
		case '&':
			(void) fputs("&amp;", fp);
			k = 0;
			break;
		case '"':
			(void) fputs("&quot;", fp);
			k = 0;
			break;
		default:
			if (!urldet ||
			    (urlt = util_urlchk(cp, &p, &len)) == 0) {
				/* No URL check requested or not a valid URL */
				(void) fputc(*cp, fp);
				k = 0;
				break;
			}

			/* Output URL and add hyperlink */
			if (urlt & NEED_PREPEND_HTTP)
				p = "http://";
			else if (urlt & NEED_PREPEND_FTP)
				p = "ftp://";
			else if (urlt & NEED_PREPEND_MAILTO)
				p = "mailto:";
			else
				p = "";

			(void) fprintf(fp, "<A HREF=\"%s", p);
			(void) fwrite(cp, len, 1, fp);
			(void) fputs("\">", fp);
			(void) fwrite(cp, len, 1, fp);
			(void) fputs("</A>", fp);

			i = ((i + len) % CHARS_PER_TAB) - 1;

			cp += (len - 1);
			k = 0;
			break;
		}
		if (++i == CHARS_PER_TAB)
			i = 0;
	}

	if (fontname != NULL || fontsize != 0)
		(void) fputs("\n</FONT>\n", fp);
}


/*
 * util_chset_open
 *	Initialize character set conversion as specified.
 *
 * Args:
 *	flags - CHSET_UTF8_TO_LOCALE or CHSET_LOCALE_TO_UTF8.
 *
 * Return:
 *	A character set conversion descriptor pointer.
 */
chset_conv_t *
util_chset_open(int flags)
{
	chset_conv_t	*cp;

	cp = (chset_conv_t *)(void *) MEM_ALLOC(
		"chset_conv_t", sizeof(chset_conv_t)
	);
	if (cp == NULL) {
		(void) fprintf(errfp, "Error: %s\n", app_data.str_nomemory);
		return NULL;
	}

	cp->flags = flags;
	cp->lconv_desc = NULL;

#ifdef HAS_ICONV
	if (app_data.chset_xlat == CHSET_XLAT_ICONV) {
		if (flags == CHSET_UTF8_TO_LOCALE) {
			cp->lconv_desc = util_lconv_open(
				app_data.lang_utf8, NULL
			);
		}
		else if (flags == CHSET_LOCALE_TO_UTF8) {
			cp->lconv_desc = util_lconv_open(
				NULL, app_data.lang_utf8
			);
		}
	}
#endif

	return (cp);
}


/*
 * util_chset_close
 *	Shut down character set conversion.
 *
 * Args:
 *	cp - Descriptor pointer previously returned by util_chset_open.
 *
 * Return:
 *	Nothing.
 */
void
util_chset_close(chset_conv_t *cp)
{
	if (cp != NULL) {
		if (cp->lconv_desc != NULL)
			util_lconv_close(cp->lconv_desc);
		MEM_FREE(cp);
	}
}


/*
 * util_chset_conv
 *	Perform character set conversion on a string.  The output string
 *	storage is internally allocated and should be freed by the caller
 *	via MEM_FREE when done.
 *
 * Args:
 *	cp - Descriptor pointer previously returned by util_chset_open.
 *	from - The input string
 *	to - Address pointer to the output string
 *	roe - Revert-on-error boolean flag.  See util_utf8_to_iso8859()
 *	      for details.
 *
 * Returns:
 *	TRUE  - success
 *	FALSE - failure
 */
bool_t
util_chset_conv(chset_conv_t *cp, char *from, char **to, bool_t roe)
{
	if (cp == NULL || to == NULL)
		return FALSE;	/* Invalid arg */

	if (*to != NULL) {
		MEM_FREE(*to);
		*to = NULL;
	}

	if (from == NULL || *from == '\0') {
		/* Null or empty string */
		*to = NULL;
		return TRUE;
	}

	switch (app_data.chset_xlat) {
	case CHSET_XLAT_ICONV:
		/* Use the iconv(3) library to do the conversion */
		if (cp->lconv_desc != NULL &&
		    util_do_lconv(cp->lconv_desc, from, to)) {
			return TRUE;
		}
		/*FALLTHROUGH*/

	case CHSET_XLAT_ISO8859:
		if (cp->flags == CHSET_UTF8_TO_LOCALE) {
			/* Use our own UTF-8 to ISO8859 conversion function */
			return (util_utf8_to_iso8859(from, to, roe));
		}
		else if (cp->flags == CHSET_LOCALE_TO_UTF8) {
			/* Use our own ISO8859 to UTF-8 conversion function */
			return (util_iso8859_to_utf8(from, to));
		}
		/*FALLTHROUGH*/

	case CHSET_XLAT_NONE:
	default:
		/* No conversion specified: Just dup the string */
		return (util_newstr(to, from));
	}
}


/*
 * util_bswap16
 *	16-bit little-endian to big-endian byte-swap routine.
 *	On a big-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word16_t
util_bswap16(word16_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	word16_t	ret;

	ret  = (x & 0x00ff) << 8;
	ret |= (word16_t) (x & 0xff00) >> 8;
	return (ret);
#else
	return (x);
#endif
}


/*
 * util_bswap24
 *	24-bit little-endian to big-endian byte-swap routine.
 *	On a big-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word32_t
util_bswap24(word32_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	word32_t	ret;

	ret  = (x & 0x0000ff) << 16;
	ret |= (x & 0x00ff00);
	ret |= (x & 0xff0000) >> 16;
	return (ret);
#else
	return (x);
#endif
}


/*
 * util_bswap32
 *	32-bit little-endian to big-endian byte-swap routine.
 *	On a big-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word32_t
util_bswap32(word32_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	word32_t	ret;

	ret  = (x & 0x000000ff) << 24;
	ret |= (x & 0x0000ff00) << 8;
	ret |= (x & 0x00ff0000) >> 8;
	ret |= (x & 0xff000000) >> 24;
	return (ret);
#else
	return (x);
#endif
}


/*
 * util_bswap64
 *	64-bit little-endian to big-endian byte-swap routine.
 *	On a big-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word64_t
util_bswap64(word64_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	word64_t	ret;

#ifdef FAKE_64BIT
	ret.v[0] = util_bswap32(x.v[1]);
	ret.v[1] = util_bswap32(x.v[0]);
#else
#if (defined(__GNUC__) && __GNUC__ > 1) || defined(HAS_LONG_LONG)
	ret  = (x & 0x00000000000000ffLL) << 56;
	ret |= (x & 0x000000000000ff00LL) << 40;
	ret |= (x & 0x0000000000ff0000LL) << 24;
	ret |= (x & 0x00000000ff000000LL) << 8;
	ret |= (x & 0x000000ff00000000LL) >> 8;
	ret |= (x & 0x0000ff0000000000LL) >> 24;
	ret |= (x & 0x00ff000000000000LL) >> 40;
	ret |= (x & 0xff00000000000000LL) >> 56;
#else
	ret  = (x & 0x00000000000000ff) << 56;
	ret |= (x & 0x000000000000ff00) << 40;
	ret |= (x & 0x0000000000ff0000) << 24;
	ret |= (x & 0x00000000ff000000) << 8;
	ret |= (x & 0x000000ff00000000) >> 8;
	ret |= (x & 0x0000ff0000000000) >> 24;
	ret |= (x & 0x00ff000000000000) >> 40;
	ret |= (x & 0xff00000000000000) >> 56;
#endif	/* __GNUC__ > 1 || HAS_LONG_LONG */
#endif	/* FAKE_64BIT */
	return (ret);
#else
	return (x);
#endif
}


/*
 * util_lswap16
 *	16-bit big-endian to little-endian byte-swap routine.
 *	On a little-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word16_t
util_lswap16(word16_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	return (x);
#else
	word16_t	ret;

	ret  = (x & 0x00ff) << 8;
	ret |= (word16_t) (x & 0xff00) >> 8;
	return (ret);
#endif
}


/*
 * util_lswap24
 *	24-bit big-endian to little-endian byte-swap routine.
 *	On a little-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word32_t
util_lswap24(word32_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	return (x);
#else
	word32_t	ret;

	ret  = (x & 0x0000ff) << 16;
	ret |= (x & 0x00ff00);
	ret |= (x & 0xff0000) >> 16;
	return (ret);
#endif
}


/*
 * util_lswap32
 *	32-bit big-endian to little-endian byte-swap routine.
 *	On a little-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word32_t
util_lswap32(word32_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	return (x);
#else
	word32_t	ret;

	ret  = (x & 0x000000ff) << 24;
	ret |= (x & 0x0000ff00) << 8;
	ret |= (x & 0x00ff0000) >> 8;
	ret |= (x & 0xff000000) >> 24;
	return (ret);
#endif
}


/*
 * util_lswap64
 *	64-bit big-endian to little-endian byte-swap routine.
 *	On a little-endian system architecture this routine has no effect.
 *
 * Args:
 *	x - The data to be swapped
 *
 * Return:
 *	The swapped data.
 */
word64_t
util_lswap64(word64_t x)
{
#if _BYTE_ORDER_ == _L_ENDIAN_
	return (x);
#else
	word64_t	ret;

#ifdef FAKE_64BIT
	ret.v[0] = util_bswap32(x.v[1]);
	ret.v[1] = util_bswap32(x.v[0]);
#else
	ret  = (x & 0x00000000000000ff) << 56;
	ret |= (x & 0x000000000000ff00) << 40;
	ret |= (x & 0x0000000000ff0000) << 24;
	ret |= (x & 0x00000000ff000000) << 8;
	ret |= (x & 0x000000ff00000000) >> 8;
	ret |= (x & 0x0000ff0000000000) >> 24;
	ret |= (x & 0x00ff000000000000) >> 40;
	ret |= (x & 0xff00000000000000) >> 56;
#endif	/* FAKE_64BIT */
	return (ret);
#endif
}
/*
 * util_dbgdump
 *	Dump a data buffer to screen.
 *
 * Args:
 *	title - Message banner
 *	data - Address of data
 *	len - Number of bytes to dump
 *
 * Return:
 *	Nothing.
 */
void
util_dbgdump(char *title, byte_t *data, int len)
{
	int	i, j, k, n,
		lines;

	if (title == NULL || data == NULL || len <= 0)
		return;

	(void) fprintf(errfp, "\n%s:", title);

	lines = ((len - 1) / 16) + 1;

	for (i = 0, k = 0; i < lines; i++) {
		(void) fprintf(errfp, "\n%04x    ", k);

		for (j = 0, n = k; j < 16; j++, k++) {
			if (k < len)
				(void) fprintf(errfp, "%02x ", *(data + k));
			else
				(void) fprintf(errfp, "-- ");

			if (j == 7)
				(void) fprintf(errfp, " ");
		}

		(void) fprintf(errfp, "   ");

		for (j = 0, k = n; j < 16; j++, k++) {
			if (k < len) {
				(void) fprintf(errfp, "%c",
				    isprint(*(data + k)) ? *(data + k) : '.'
				);
			}
			else
				(void) fprintf(errfp, ".");
		}
	}

	(void) fprintf(errfp, "\n");
}


#ifdef FAKE_64BIT
/*
 * util_assign64
 *	Copy a 32-bit integer into a "fake" 64-bit location.  Sign-extend
 *	the value if necessary.  This assumes a 2's-compliment
 *	representation of integer values.
 *
 * Args:
 *	val - The source 32-bit integer value
 *
 * Return:
 *	The 64-bit quantity.
 */
word64_t
util_assign64(sword32_t val)
{
	word64_t	ret;

	ret.v[LO_WORD] = (word32_t) val;
	if (val < 0)
		ret.v[HI_WORD] = (word32_t) 0xffffffff;
	else
		ret.v[HI_WORD] = 0;

	return (ret);
}


/*
 * util_assign32
 *	Copy a "fake" 64-bit integer into a 32-bit location.
 *
 * Args:
 *	val - The source 64-bit integer value
 *
 * Return:
 *	The 32-bit quantity.
 */
word32_t
util_assign32(word64_t val)
{
	return (val.v[LO_WORD]);
}
#endif


#ifdef __VMS
/*
 * The following section provide UNIX-like functionality for Digital OpenVMS
 */

/* Function prototypes */
extern void	delete();

#ifdef VMS_USE_OWN_DIRENT

extern int	LIB$FIND_FILE();
extern void	LIB$FIND_FILE_END();
extern void	SYS$FILESCAN();

typedef struct {
	short	length;
	short	component;
	int	address;
	int	term;
} item_list;


/*
 * util_opendir
 *	Emulate a UNIX opendir by clearing the context value, and creating
 *	the wild card search by appending *.* to the path name.
 *	(See opendir(2) on UNIX systems)
 *
 * Args:
 *	path - directory path to open
 *
 * Return:
 *	Pointer to the DIR structure descriptor
 */
DIR *
util_opendir(char *path)
{
	static DIR		dir;
	static struct dirent	ent;

	context = 0;
	(void) sprintf(ent.d_name, "%s*.*", path);
	dir.dd_buf = &ent;
 	return (&dir);
}


/*
 * util_closedir
 *	Emulate a UNIX closedir by call LIB$FIND_FILE_END to close 
 *	the file context.  (End the wild card search)
 *	(See closedir(2) on UNIX systems)
 *
 * Args:
 *	dp - pointer to the directory's DIR structure
 *
 * Return:
 *	Nothing.
 */
void
util_closedir(DIR *dp)
{
	LIB$FIND_FILE_END(&context);
}


/*
 * util_readdir
 *	Emulate a UNIX readdir by calling LIB$FIND_FILE, and SYS$FILESCAN
 *	to return the file name back.
 *	(See readdir(2) on UNIX systems)
 *
 * Args:
 *	dp - pointer to the directory's DIR structure
 *
 * Return:
 *	Pointer to the dirent structure pertaining to a directory entry
 */
struct dirent *
util_readdir(DIR *dp)
{
	int			dir_desc[2],
				desc[2],
				i;
	char 			*p,
				*file[FILE_PATH_SZ];
	item_list		list;
	static struct dirent	ent;

	desc[0] = FILE_PATH_SZ;
	desc[1] = (int) file;

	dir_desc[0] = FILE_PATH_SZ;
	dir_desc[1] = (int) dp->dd_buf->d_name;

	if (LIB$FIND_FILE(dir_desc, desc, &context) & 0x01) {
		list.length = 0;
		list.component = FSCN$_NAME;
		list.address = 0;
		list.term = 0;

		SYS$FILESCAN(desc, &list, 0, 0, 0); 

		p = (char *) list.address;
		p[list.length] = '\0';

		for (p = (char *) list.address; *p != '\0'; p++)
			*p = tolower(*p);

		(void) strcpy(ent.d_name, (char *) list.address);
		return (&ent);
	}
	else
		return NULL;
}

#endif	/* VMS_USE_OWN_DIRENT */


/*
 * util_waitpid
 *	Emulate a UNIX waitpid by doing a wait call
 *	(see waitpid(2) on UNIX systems)
 *
 * Args:
 *	pid - process ID to wait for
 *	statloc - pointer to wait status information
 *	options - wait options
 * Return:
 *	The process ID of the process that caused this call to stop
 *	waiting.
 */
pid_t
util_waitpid(pid_t pid, int *statloc, int options)
{
	pid_t	ret;

	ret = wait(statloc);

	/* Under VMS a vfork() call does not create a child process unless
	 * a real process is created.  In the cases where the child does
	 * not follow the vfork with a system() or exec() call to create
	 * a real subprocess, we need to fake things out.
	 */
	if (ret < 0)
		ret = pid;

	/* VMS returns a 1 for success.  Patch it to zero to
	 * make this function compatible with UNIX.
	 */
	if (*statloc == 1)
		*statloc = 0;

	return (ret);
}


/*
 * util_unlink
 *	Emulate a UNIX unlink call
 *	(See unlink(2) on UNIX systems)
 *
 * Args:
 *	file - file path name to unlink
 *
 * Return:
 *	0  - Success
 *	-1 - Failure
 */
int
util_unlink(char *file)
{
	delete(file);
	return 0;
}


/*
 * util_link
 *	Emulate a UNIX link call by copying FILE1 to FILE2
 *	(See link(2) on UNIX systems)
 *
 * Args:
 *	file1 - source file
 *	file2 - destination file
 *
 * Return:
 *	0  - Success
 *	-1 - Failure
 */
int 
util_link(char *file1, char *file2)
{
	FILE	*fp1,
		*fp2;
	char	buf[STR_BUF_SZ * 16];

	fp1 = fopen(file1, "r");
	fp2 = fopen(file2, "w");

	if (fp1 == NULL || fp2 == NULL)
		return -1;

	while (fgets(buf, sizeof(buf), fp1) != NULL)
		(void) fprintf(fp2, "%s", buf);

	(void) fclose(fp1);	
	(void) fclose(fp2);	

	return 0;	
}


/*
 * util_vms_urlconv
 *	URL format translation function between UNIX-style and VMS-style
 *	path name syntax.  This is a hack to work around the inconsistent
 *	URL formats required by the browser under different circumstances.
 *	The caller should MEM_FREE() the return string buffer.
 *
 * Args:
 *	url - The input URL string
 *	dir - Conversion direction: UNIX_2_VMS or VMS_2_UNIX
 *
 * Return:
 *	The converted URL string, or NULL is an error is encountered.
 */
char *
util_vms_urlconv(char *url, int dir)
{
	int	i;
	char	*p1,
		*p2,
		*p3,
		*p4,
		*buf;
	bool_t	first;

	if (util_urlchk(url, &p1, &i) & IS_REMOTE_URL)
		return (url);	/* Remote URL: don't touch it */

	buf = (char *) MEM_ALLOC(
		"util_vms_urlconv",
		(strlen(url) * 2) + FILE_PATH_SZ
	);
	if (buf == NULL)
		return NULL;

	buf[0] = '\0';

	switch (dir) {
	case UNIX_2_VMS:
		first = TRUE;
		if (*p1 == '/')
			p1++;

		for (i = 0; (p2 = strchr(p1, '/')) != NULL; i++) {
			*p2 = '\0';

			if (i == 0 && (strchr(p1, '$') != NULL)) {
				(void) sprintf(buf, "%s:", p1);
			}
			else if (first) {
				first = FALSE;
				(void) sprintf(buf, "%s%c%s",
					       buf, DIR_BEG, p1);
			}
			else {
				(void) sprintf(buf, "%s%c%s",
					       buf, DIR_SEP, p1);
			}

			*p2 = '/';
			p1 = p2 + 1;
		}

		if (first) {
			if (strchr(p1, '$') != NULL)
				(void) sprintf(buf, "%s:", p1);
			else
				(void) sprintf(buf, "%s%s", buf, p1);
		}
		else
			(void) sprintf(buf, "%s%c%s", buf, DIR_END, p1);

		break;

	case VMS_2_UNIX:
		(void) strcpy(buf, "file:");

		if ((p2 = strchr(p1, ':')) != NULL) {
			*p2 = '\0';
			(void) sprintf(buf, "%s//localhost/%s", buf, p1);
			*p2 = ':';
			p1 = p2 + 1;
		}

		if ((p3 = strchr(p1, DIR_BEG)) != NULL) {
			p3++;
			if ((p4 = strchr(p3, DIR_END)) != NULL)
				*p4 = '\0';

			while ((p1 = strchr(p3, DIR_SEP)) != NULL) {
				*p1 = '\0';
				(void) sprintf(buf, "%s/%s", buf, p3);
				*p1 = '.';
				p3 = p1 + 1;
			}
			(void) sprintf(buf, "%s/%s", buf, p3);

			if (p4 != NULL) {
				*p4 = ']';
				p1 = p4 + 1;
			}
		}

		if (p2 == NULL && p3 == NULL)
			(void) sprintf(buf, "%s%s", buf, p1);
		else
			(void) sprintf(buf, "%s/%s", buf, p1);

		break;

	default:
		MEM_FREE(buf);
		buf = NULL;
		break;
	}

	return (buf);
}

#endif	/* __VMS */


#ifdef MEM_DEBUG
/*
 * For memory allocation debugging
 */


/*
 * util_dbg_malloc
 *	Wrapper for malloc(3).
 */
void *
util_dbg_malloc(char *name, size_t size)
{
	void	*ptr;

	if (size == 0) {
		(void) fprintf(stderr, "Malloc(%s, 0)\n", name);
		abort();
	}

	ptr = _MEM_ALLOC(size);
	(void) fprintf(stderr, "Malloc(%s, %d) => 0x%x\n",
			name, (int) size, (int) ptr);
	return (ptr);
}


/*
 * util_dbg_realloc
 *	Wrapper for realloc(3).
 */
void *
util_dbg_realloc(char *name, void *ptr, size_t size)
{
	void	*nptr;

	if (size == 0) {
		(void) fprintf(stderr, "Realloc(%s, 0x%x, 0)\n",
				name, (int) ptr);
		abort();
	}

	nptr = _MEM_REALLOC(ptr, size);
	(void) fprintf(stderr, "Realloc(%s, 0x%x, %d) => 0x%x\n",
			name, (int) ptr, (int) size, (int) nptr);
	return (nptr);
}


/*
 * util_dbg_calloc
 *	Wrapper for calloc(3).
 */
void *
util_dbg_calloc(char *name, size_t nelem, size_t elsize)
{
	void	*ptr;

	if (nelem == 0 || elsize == 0) {
		(void) fprintf(stderr, "Calloc(%s, %d, %d)\n",
				name, (int) nelem, (int) elsize);
		abort();
	}

	ptr = _MEM_CALLOC(nelem, elsize);
	(void) fprintf(stderr, "Calloc(%s, %d, %d) => 0x%x\n",
			name, (int) nelem, (int) elsize, (int) ptr);
	return (ptr);
}


/*
 * util_dbg_free
 *	Wrapper for free(3).
 */
void
util_dbg_free(void *ptr)
{
	(void) fprintf(stderr, "Free(0x%x)\n", ptr);
	_MEM_FREE(ptr);
}

#endif	/* MEM_DEBUG */

