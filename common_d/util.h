/*
 *   util.h - Common utility routines for xmcd, cda and libdi.
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
#ifndef __UTIL_H__
#define __UTIL_H__

#ifndef lint
static char *_util_h_ident_ = "@(#)util.h	6.71 04/03/17";
#endif

/*
 * Default platform feature configurations
 */

#if !defined(USE_SIGACTION) && !defined(USE_SIGSET) && !defined(USE_SIGNAL)
#define USE_SIGNAL      /* Make USE_SIGNAL the default is not specified */
#endif

/* Avoid multiple defines */
#if defined(USE_SIGACTION) && defined(USE_SIGSET)
#undef USE_SIGSET
#endif

/* Avoid multiple defines */
#if defined(USE_SIGPROCMASK) && defined(USE_SIGBLOCK)
#undef USE_SIGBLOCK
#endif

#if defined(USE_SETEUID) || defined(USE_SETREUID) || defined(USE_SETRESUID)
#define HAS_EUID	/* Signify effective uid is supported */

/* Avoid multiple defines */
#if defined(USE_SETEUID) && defined(USE_SETREUID)
#undef USE_SETREUID
#endif
#if defined(USE_SETEUID) && defined(USE_SETRESUID)
#undef USE_SETRESUID
#endif
#if defined(USE_SETREUID) && defined(USE_SETRESUID)
#undef USE_SETRESUID
#endif

#endif

/* Avoid multiple defines */
#ifdef HAS_EUID

#if defined(USE_SETEGID) && defined(USE_SETREGID)
#undef USE_SETREGID
#endif
#if defined(USE_SETEGID) && defined(USE_SETRESGID)
#undef USE_SETRESGID
#endif
#if defined(USE_SETREGID) && defined(USE_SETRESGID)
#undef USE_SETRESGID
#endif

#else

#ifdef USE_SETEGID
#undef USE_SETEGID
#endif
#ifdef USE_SETREGID
#undef USE_SETREGID
#endif
#ifdef USE_SETRESGID
#undef USE_SETRESGID
#endif

#endif

#if !defined(USE_SELECT) && !defined(USE_POLL) && \
    !defined(USE_NAP) && !defined(USE_USLEEP) && \
    !defined(USE_NANOSLEEP) && !defined(USE_PTHREAD_DELAY_NP)
#define USE_SELECT      /* Make USE_SELECT the default if not specified */
#endif


/*
 * Memory allocators
 */
#define _MEM_ALLOC		malloc
#define _MEM_REALLOC		realloc
#define _MEM_CALLOC		calloc
#define _MEM_FREE		free

#ifdef MEM_DEBUG
#define MEM_ALLOC(s,l)		util_dbg_malloc((s),(l))
#define MEM_REALLOC(s,p,l)	util_dbg_realloc((s),(p),(l))
#define MEM_CALLOC(s,n,l)	util_dbg_calloc((s),(n),(l))
#define MEM_FREE(p)		util_dbg_free((p))
#else
#define MEM_ALLOC(s,l)		_MEM_ALLOC((l))
#define MEM_REALLOC(s,p,l)	_MEM_REALLOC((p),(l))
#define MEM_CALLOC(s,n,l)	_MEM_CALLOC((n),(l))
#define MEM_FREE(p)		_MEM_FREE((p))
#endif


/*
 * Directory path name convention
 */
#ifdef __VMS
#define DIR_BEG		'['		/* Directory start */
#define DIR_SEP		'.'		/* Directory separator */
#define DIR_END		']'		/* Directory end */
#define CUR_DIR		"[]"		/* Current directory */
#else
#define DIR_BEG		'/'		/* Directory start */
#define DIR_SEP		'/'		/* Directory separator */
#define DIR_END		'/'		/* Directory end */
#define CUR_DIR		"."		/* Current directory */
#endif

/*
 * Utility macros
 */
#define SQR(x)		((x) * (x))	/* Compute the square of a number */

/*
 * Debugging macros
 */
#define DBG_GEN		0x01		/* General debugging */
#define DBG_DEVIO	0x02		/* Device I/O debugging */
#define DBG_CDI		0x04		/* CD information load debugging */
#define DBG_UI		0x08		/* User interface debugging */
#define DBG_RMT		0x10		/* Remote control debugging */
#define DBG_SND		0x20		/* Sound card debugging */
#define DBG_MOTD	0x40		/* Message of the day debugging */
#define DBG_ALL		0xff		/* All debug levels */

#define DBGPRN(x)	if (app_data.debug & (x)) (void) fprintf
#define DBGDUMP(x)	if (app_data.debug & (x)) util_dbgdump

#define EXE_ERR		0xdeadbeef	/* Magic retcode for util_runcmd() */

/*
 * Return bitmap of util_urlchk
 */
#define IS_LOCAL_URL		0x01
#define IS_REMOTE_URL		0x02
#define NEED_PREPEND_HTTP	0x10
#define NEED_PREPEND_FTP	0x20
#define NEED_PREPEND_MAILTO	0x40

/*
 * Used for the dir parameter of util_vms_urlconv
 */
#define UNIX_2_VMS	1
#define VMS_2_UNIX	2

/*
 * Used by util_chset_open, util_chset_close and util_chset_conv
 */
#define CHSET_UTF8_TO_LOCALE	1	/* Convert from UTF-8 to locale */
#define CHSET_LOCALE_TO_UTF8	2	/* Convert from locale to UTF-8 */


typedef struct chset_conv {
	int		flags;
	void		*lconv_desc;
} chset_conv_t;


/* Public function prototypes */
extern void		util_init(void);
extern void		util_start(void);
extern void		util_onsig(int);
extern void		(*util_signal(int , void (*)(int)))(int);
extern int		util_seteuid(uid_t);
extern int		util_setegid(gid_t);
extern bool_t		util_newstr(char **, char *);
extern uid_t		util_get_ouid(void);
extern gid_t		util_get_ogid(void);
extern bool_t		util_set_ougid(void);
extern struct utsname	*util_get_uname(void);
extern sword32_t	util_ltobcd(sword32_t);
extern sword32_t	util_bcdtol(sword32_t);
extern bool_t		util_stob(char *);
extern char		*util_basename(char *);
extern char		*util_dirname(char *);
extern char		*util_loginname(void);
extern char		*util_homedir(uid_t);
extern char		*util_uhomedir(char *);
extern int		util_dirstat(char *, struct stat *, bool_t);
extern bool_t		util_mkdir(char *, mode_t);
extern void		util_setperm(char *, char *);
extern char		*util_monname(int);
extern bool_t		util_isexecutable(char *);
extern char		*util_findcmd(char *);
extern bool_t		util_checkcmd(char *);
extern int		util_runcmd(char *, void (*)(int), int);
extern int		util_isqrt(int);
extern void		util_blktomsf(sword32_t, byte_t *, byte_t *, byte_t *,
				      sword32_t);
extern void		util_msftoblk(byte_t, byte_t, byte_t, sword32_t *,
				      sword32_t);
extern sword32_t	util_xlate_blk(sword32_t);
extern sword32_t	util_unxlate_blk(sword32_t);
extern int		util_taper_vol(int);
extern int		util_untaper_vol(int);
extern int		util_scale_vol(int);
extern int		util_unscale_vol(int);
extern void		util_delayms(unsigned long);
extern int		util_waitchild(pid_t, time_t, void (*)(int), int,
				       bool_t, waitret_t *);
extern char		*util_strstr(char *s1, char *s2);
extern int		util_strcasecmp(char *, char *);
extern int		util_strncasecmp(char *, char *, int);
extern char		*util_text_reduce(char *);
extern char		*util_cgi_xlate(char *);
extern int		util_urlchk(char *, char **, int *);
extern char		*util_urlencode(char *);
extern void		util_html_fputs(char *, FILE *, bool_t, char *, int);
extern chset_conv_t	*util_chset_open(int);
extern void		util_chset_close(chset_conv_t *);
extern bool_t		util_chset_conv(chset_conv_t *, char *, char **,
					bool_t);
extern word16_t		util_bswap16(word16_t);
extern word32_t		util_bswap24(word32_t);
extern word32_t		util_bswap32(word32_t);
extern word64_t		util_bswap64(word64_t);
extern word16_t		util_lswap16(word16_t);
extern word32_t		util_lswap24(word32_t);
extern word32_t		util_lswap32(word32_t);
extern word64_t		util_lswap64(word64_t);
extern void		util_dbgdump(char *, byte_t *, int);

#ifdef FAKE_64BIT
extern word64_t		util_assign64(sword32_t);
extern word32_t		util_assign32(word64_t);
#endif

#ifdef __VMS
extern DIR		*util_opendir(char *);
extern void		util_closedir(DIR *);
extern struct dirent	*util_readdir(DIR *);
extern pid_t		util_waitpid(pid_t, int *, int);
extern int		util_link(char *, char *);
extern int		util_unlink(char *);
extern char		*util_vms_urlconv(char *, int);
#endif

#ifdef MEM_DEBUG
void			*util_dbg_malloc(char *name, size_t size);
void			*util_dbg_realloc(char *name, void *ptr, size_t size);
void			*util_dbg_calloc(char *name, size_t nelem,
					 size_t elsize);
void			util_dbg_free(void *ptr);
#endif

#endif	/* __UTIL_H__ */


