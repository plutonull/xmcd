/*
 *   cda - Command-line CD Audio Player/Ripper
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
static char *_cda_c_ident_ = "@(#)cda.c	6.352 04/04/20";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "common_d/version.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdinfo_d/cdinfo.h"
#include "cdinfo_d/motd.h"
#include "cda_d/cda.h"
#include "cda_d/visual.h"
#include "cda_d/userreg.h"

#ifdef HAS_ICONV
#include <iconv.h>
#include <locale.h>
#endif


#define PGM_SEPCHAR	','			/* Program seq separator */
#define MAX_PKT_RETRIES	200			/* Max retries reading pkt */
#define PIPE_TIMEOUT	5			/* 5 seconds to time out */


extern char		*ttyname();

appdata_t		app_data;		/* Option settings */
cdinfo_incore_t		*dbp;			/* CD information pointer */
curstat_t		status;			/* Current CD player status */
char			*emsgp,			/* Error msg for use on exit */
			**cda_devlist,		/* Device list table */
			errmsg[ERR_BUF_SZ],	/* Error message buffer */
			spipe[FILE_PATH_SZ],	/* Send pipe path */
			rpipe[FILE_PATH_SZ];	/* Receive pipe path */
int			cda_sfd[2] = {-1,-1},	/* Send pipe file desc */
			cda_rfd[2] = {-1,-1},	/* Receive pipe file desc */
			exit_status;		/* Exit status */
pid_t			daemon_pid;		/* CDA daemon pid */
FILE			*errfp;			/* Error message stream */

STATIC int		cont_delay = 1,		/* Status display interval */
			inetoffln = 0;		/* Inet Offline cmd line arg */
STATIC dev_t		cd_rdev;		/* CD device number */
STATIC bool_t		isdaemon = FALSE,	/* Am I the daemon process */
			stat_cont = FALSE,	/* Continuous display status */
			batch = FALSE;		/* Non-interactive */
STATIC FILE		*ttyfp;			/* /dev/tty */
STATIC cdinfo_client_t	cdinfo_cldata;		/* Client info for libcdinfo */
STATIC di_client_t	di_cldata;		/* Client info for libdi */
STATIC char		dlock[FILE_PATH_SZ];	/* Daemon lock file path */
STATIC cdapkt_t		*curr_pkt;		/* Current received packet */
#ifndef NOVISUAL
STATIC bool_t		visual = FALSE;		/* Visual (curses) mode */
#endif


/***********************
 *  internal routines  *
 ***********************/


/*
 * onsig0
 *	Signal handler to shut down application.
 *
 * Args:
 *	signo - The signal number.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
onsig0(int signo)
{
	exit_status = 3;
	cda_quit(&status);
	/*NOTREACHED*/
}


/*
 * onsig1
 *	SIGPIPE signal handler.
 *
 * Args:
 *	signo - The signal number.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
onsig1(int signo)
{
	/* Do nothing */
#if !defined(USE_SIGACTION) && !defined(BSDCOMPAT)
	/* Re-arm the handler */
	(void) util_signal(signo, onsig1);
#endif
}


/*
 * onintr
 *	Signal handler to stop continuous status display.
 *
 * Args:
 *	signo - The signal number.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
onintr(int signo)
{
	stat_cont = FALSE;
	(void) printf("\r");
#if !defined(USE_SIGACTION) && !defined(BSDCOMPAT)
	(void) util_signal(signo, SIG_IGN);
#endif
}


/*
 * cda_pgm_parse
 *	Parse the shuffle/program mode play sequence text string, and
 *	update the playorder table in the curstat_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	TRUE  = success,
 *	FALSE = error.
 */
STATIC bool_t
cda_pgm_parse(curstat_t *s)
{
	int	i,
		j;
	char	*p,
		*q,
		*tmpbuf;
	bool_t	last = FALSE,
		skipped = FALSE;

	if (dbp->playorder == NULL)
		/* Nothing to do */
		return TRUE;

	tmpbuf = NULL;
	if (!util_newstr(&tmpbuf, dbp->playorder)) {
		CDA_FATAL(app_data.str_nomemory);
		return FALSE;
	}

	s->prog_tot = 0;

	for (i = 0, p = q = tmpbuf; i < MAXTRACK; p = ++q) {
		/* Skip p to the next digit */
		for (; !isdigit((int) *p) && *p != '\0'; p++)
			;

		if (*p == '\0')
			/* No more to do */
			break;

		/* Skip q to the next non-digit */
		for (q = p; isdigit((int) *q); q++)
			;

		if (*q == ',')
			*q = '\0';
		else if (*q == '\0')
			last = TRUE;
		else {
			MEM_FREE(tmpbuf);
			CDA_WARNING(app_data.str_seqfmterr);
			return FALSE;
		}

		if (q > p) {
			/* Update play sequence */
			for (j = 0; j < MAXTRACK; j++) {
				if (s->trkinfo[j].trkno == atoi(p)) {
					s->trkinfo[i].playorder = j;
					s->prog_tot++;
					i++;
					break;
				}
			}

			if (j >= MAXTRACK)
				skipped = TRUE;
		}

		if (last)
			break;
	}

	if (skipped) {
		/* Delete invalid tracks from list */

		tmpbuf[0] = '\0';
		for (i = 0; i < (int) s->prog_tot; i++) {
			if (i == 0)
				(void) sprintf(tmpbuf, "%u",
				    s->trkinfo[s->trkinfo[i].playorder].trkno);
			else
				(void) sprintf(tmpbuf, "%s,%u", tmpbuf,
				    s->trkinfo[s->trkinfo[i].playorder].trkno);
		}

		CDA_WARNING(app_data.str_invpgmtrk);
	}

	MEM_FREE(tmpbuf);

	return TRUE;
}


/*
 * cda_mkdirs
 *	Called at startup time to create some needed directories, if
 *	they aren't already there.
 *	Currently these are:
 *		$HOME/.xmcdcfg
 *		$HOME/.xmcdcfg/prog
 *		/tmp/.cdaudio
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_mkdirs(void)
{
	pid_t		cpid;
	waitret_t	wstat;
	char		*errmsg,
			*homepath,
			path[FILE_PATH_SZ + 16];
	struct stat	stbuf;

#ifndef NOMKTMPDIR
	errmsg = (char *) MEM_ALLOC(
		"errmsg",
		strlen(app_data.str_tmpdirerr) + strlen(TEMP_DIR)
	);
	if (errmsg == NULL) {
		CDA_FATAL(app_data.str_nomemory);
		return;
	}

	/* Make temporary directory, if needed */
	(void) sprintf(errmsg, app_data.str_tmpdirerr, TEMP_DIR);
	if (LSTAT(TEMP_DIR, &stbuf) < 0) {
		if (!util_mkdir(TEMP_DIR, 0777)) {
			CDA_FATAL(errmsg);
			return;
		}
	}
	else if (!S_ISDIR(stbuf.st_mode)) {
		CDA_FATAL(errmsg);
		return;
	}

	MEM_FREE(errmsg);
#endif	/* NOMKTMPDIR */

	homepath = util_homedir(util_get_ouid());
	if ((int) strlen(homepath) >= FILE_PATH_SZ) {
		CDA_FATAL(app_data.str_longpatherr);
		return;
	}

	switch (cpid = FORK()) {
	case 0:
		/* Child process */

		/* Force uid and gid to original setting */
		if (!util_set_ougid())
			_exit(1);

		/* Create the per-user config directory */
		(void) sprintf(path, USR_CFG_PATH, homepath);
		if (LSTAT(path, &stbuf) < 0) {
			if (errno == ENOENT && !util_mkdir(path, 0755)) {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: cannot mkdir %s.\n", path);
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: cannot stat %s.\n", path);
			}
		}
		else if (!S_ISDIR(stbuf.st_mode)) {
			DBGPRN(DBG_GEN)(errfp,
				"cd_mkdirs: %s is not a directory.\n", path);
		}

		/* Create the per-user track program directory */
		(void) sprintf(path, USR_PROG_PATH, homepath);
		if (LSTAT(path, &stbuf) < 0) {
			if (errno == ENOENT && !util_mkdir(path, 0755)) {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: cannot mkdir %s.\n", path);
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
					"cd_mkdirs: cannot stat %s.\n", path);
			}
		}
		else if (!S_ISDIR(stbuf.st_mode)) {
			DBGPRN(DBG_GEN)(errfp,
				"cd_mkdirs: %s is not a directory.\n", path);
		}

		_exit(0);
		/*NOTREACHED*/

	case -1:
		/* fork failed */
		break;

	default:
		/* Parent: wait for child to finish */
		(void) util_waitchild(cpid, app_data.srv_timeout + 5,
				      NULL, 0, FALSE, &wstat);
		break;
	}
}


/*
 * cda_strcpy
 *	Similar to strcpy(3), but skips some punctuation characters.
 *
 * Args:
 *	tgt - Target string buffer
 *	src - Source string buffer
 *
 * Return:
 *	The number of characters copied
 */
STATIC size_t
cda_strcpy(char *tgt, char *src)
{
	size_t	n = 0;
	bool_t	prev_space = FALSE;

	if (src == NULL)
		return 0;

	for (; *src != '\0'; src++) {
		switch (*src) {
		case '\'':
		case '"':
		case ',':
		case ':':
		case ';':
		case '|':
			/* Skip some punctuation characters */
			break;

		case ' ':
		case '\t':
		case '/':
		case '\\':
			/* Substitute these with underscores */
			if (!prev_space) {
				*tgt++ = app_data.subst_underscore ? '_' : ' ';
				n++;
				prev_space = TRUE;
			}
			break;

		default:
			if (!isprint((int) *src))
				/* Skip unprintable characters */
				break;

			*tgt++ = *src;
			n++;
			prev_space = FALSE;
			break;
		}
	}
	*tgt = '\0';
	return (n);
}


/*
 * cda_unlink
 *	Unlink the specified file
 *
 * Args:
 *	path - The file path to unlink
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
cda_unlink(char *path)
{
	pid_t	cpid;
	int	ret,
		wstat;
	bool_t	success;

	if (path == NULL)
		return FALSE;

	switch (cpid = FORK()) {
	case -1:
		DBGPRN(DBG_GEN)(errfp,
				"cda_unlink: fork failed (errno=%d)\n",
				errno);
		success = FALSE;
		break;

	case 0:
		util_set_ougid();

		DBGPRN(DBG_GEN)(errfp, "Unlinking [%s]\n", path);

		if ((ret = UNLINK(path)) != 0 && errno == ENOENT)
			ret = 0;

		if (ret != 0) {
			DBGPRN(DBG_GEN)(errfp,
					"unlink failed (errno=%d)\n",
					errno);
		}
		_exit((int) (ret != 0));
		/*NOTREACHED*/

	default:
		/* Parent: wait for child to finish */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     NULL, 0, FALSE, &wstat);
		if (ret < 0) {
			DBGPRN(DBG_GEN)(errfp,
					"waitpid failed (errno=%d)\n", errno);
			success = FALSE;
		}
		else if (WIFEXITED(wstat)) {
			if ((ret = WEXITSTATUS(wstat)) != 0) {
				DBGPRN(DBG_GEN)(errfp,
					"unlink child exited (status=%d)\n",
					ret);
			}
			success = (bool_t) (ret == 0);
		}
		else if (WIFSIGNALED(wstat)) {
			DBGPRN(DBG_GEN)(errfp,
					"unlink child killed (signal=%d)\n",
					WTERMSIG(wstat));
			success = FALSE;
		}
		else {
			DBGPRN(DBG_GEN)(errfp, "unlink child error\n");
			success = FALSE;
		}
		break;
	}

	return (success);
}


/*
 * cda_mkfname
 *	Construct a file name based on the supplied template, and append
 *	it to the target string.
 *
 * Args:
 *	s      - Pointer to the curstat_t structure
 *	trkidx - Track index number (0-based), or -1 if not applicable
 *	tgt    - The target string
 *	tmpl   - The template string
 *	tgtlen - Target string buffer size
 *
 * Returns:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
cda_mkfname(curstat_t *s, int trkidx, char *tgt, char *tmpl, int tgtlen)
{
	int		len,
			n,
			remain;
	char		*cp,
			*cp2,
			*p,
			tmp[(STR_BUF_SZ * 4) + 4];
	bool_t		err;

	err = FALSE;
	n = 0;
	len = strlen(tgt);
	cp = tgt + len;

	for (cp2 = tmpl; *cp2 != '\0'; cp2++, cp += n, len += n) {
		if ((remain = (tgtlen - len)) <= 0)
			break;

		switch (*cp2) {
		case '%':
			switch (*(++cp2)) {
			case 'X':
				n = strlen(PROGNAME);
				if (n < remain)
					(void) strcpy(cp, PROGNAME);
				else
					err = TRUE;
				break;

			case 'V':
				n = strlen(VERSION_MAJ) +
				    strlen(VERSION_MIN) + 1;
				if (n < remain)
					(void) sprintf(cp, "%s.%s",
							VERSION_MAJ,
							VERSION_MIN);
				else
					err = TRUE;
				break;

			case 'N':
				p = util_loginname();
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case '~':
				p = util_homedir(util_get_ouid());
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'H':
				p = util_get_uname()->nodename;
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'L':
				p = app_data.libdir;
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'S':
				p = app_data.libdir;
				n = strlen(p) + strlen("discog") + 1;
				if (n < remain) {
					(void) strcpy(cp, p);
					(void) strcat(cp, "/discog");
				}
				else
					err = TRUE;
				break;

			case 'C':
				p = cdinfo_genre_path(dbp->disc.genre);
				n = strlen(p);
				if (n < remain)
					(void) strcpy(cp, p);
				else
					err = TRUE;
				break;

			case 'I':
				n = 8;
				if (n < remain)
					(void) sprintf(cp, "%08x",
							dbp->discid);
				else
					err = TRUE;
				break;

			case 'A':
			case 'a':
				p = dbp->disc.artist;
				if (p == NULL)
					p = "artist";
				if (*cp2 == 'a') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CDA_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cda_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'a')
					MEM_FREE(p);
				break;

			case 'D':
			case 'd':
				p = dbp->disc.title;
				if (p == NULL)
					p = "title";
				if (*cp2 == 'd') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CDA_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cda_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'd')
					MEM_FREE(p);
				break;

			case 'R':
			case 'r':
				p = dbp->track[trkidx].artist;
				if (p == NULL)
					p = "trackartist";
				if (*cp2 == 'r') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CDA_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cda_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'r')
					MEM_FREE(p);
				break;

			case 'T':
			case 't':
				p = dbp->track[trkidx].title;
				if (p == NULL)
					p = "track";
				if (*cp2 == 't') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CDA_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cda_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 't')
					MEM_FREE(p);
				break;

			case 'B':
			case 'b':
				(void) sprintf(tmp, "%.127s%s%.127s",
					(dbp->disc.artist == NULL) ?
						"artist" : dbp->disc.artist,
					(dbp->disc.artist != NULL &&
					 dbp->disc.title != NULL) ?
						"-" : "",
					(dbp->disc.title == NULL) ?
						"title" : dbp->disc.title);
				p = tmp;
				if (*cp2 == 'b') {
					p = util_text_reduce(p);
					if (p == NULL) {
					    CDA_FATAL(app_data.str_nomemory);
					    return FALSE;
					}
				}

				n = strlen(p);
				if (n < remain)
					n = cda_strcpy(cp, p);
				else
					err = TRUE;

				if (*cp2 == 'b')
					MEM_FREE(p);
				break;

			case '#':
				n = 2;
				if (n < remain)
					(void) sprintf(cp, "%02d",
						s->trkinfo[trkidx].trkno);
				else
					err = TRUE;
				break;

			case '%':
			case ' ':
			case '\t':
			case '\'':
			case '"':
			case ',':
			case ';':
			case ':':
			case '|':
			case '\\':
				/* Skip some punctuation characters */
				n = 1;
				if (n < remain)
					*cp = '%';
				else
					err = TRUE;
				break;

			default:
				n = 2;
				if (n < remain)
					*cp = '%';
				else
					err = TRUE;
				break;
			}
			break;

		case ' ':
		case '\t':
			n = 1;
			if (n < remain) {
				if (app_data.subst_underscore)
					*cp = '_';
				else
					*cp = ' ';
			}
			else
				err = TRUE;
			break;

		case '\'':
		case '"':
		case ',':
		case ';':
		case ':':
		case '|':
		case '\\':
			/* Skip some punctuation characters */
			n = 0;
			break;

		default:
			if (!isprint((int) *cp2)) {
				/* Skip unprintable characters */
				n = 0;
				break;
			}

			n = 1;
			if (n < remain)
				*cp = *cp2;
			else
				err = TRUE;
			break;
		}

		if (err)
			break;
	}
	*cp = '\0';

	if (err) {
		(void) fprintf(errfp, "%s\n", app_data.str_longpatherr);
		return FALSE;
	}

	return TRUE;
}


/*
 * cda_mkoutpath
 *	Construct audio output file names and check for collision
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return
 *	TRUE  - passed
 *	FALSE - failed
 */
STATIC bool_t
cda_mkoutpath(curstat_t *s)
{
	char		*dpath = NULL,
			*path;
	int		i,
			j;
	struct stat	stbuf;

	if (s->outf_tmpl == NULL) {
		(void) fprintf(errfp, "%s\n", app_data.str_noaudiopath);
		return FALSE;
	}

	if (app_data.cdda_trkfile) {
		/* Normal play, program, shuffle and sample modes */
		for (i = 0; i < (int) s->tot_trks; i++) {
			if (s->program) {
				bool_t	inprog;

				inprog = FALSE;
				for (j = 0; j < (int) s->prog_tot; j++) {
					if (i == s->trkinfo[j].playorder) {
						inprog = TRUE;
						break;
					}
				}

				if (!inprog)
					/* This track is not in the program */
					continue;
			}
			else if (!s->shuffle &&
				 s->cur_trk > s->trkinfo[i].trkno) {
				continue;
			}

			path = (char *) MEM_ALLOC("path", FILE_PATH_SZ);
			if (path == NULL) {
				CDA_FATAL(app_data.str_nomemory);
				return FALSE;
			}
			path[0] = '\0';

			if (!cda_mkfname(s, i, path, s->outf_tmpl,
					 FILE_PATH_SZ)) {
				MEM_FREE(path);
				return FALSE;
			}

			if (!util_newstr(&s->trkinfo[i].outfile, path)) {
				MEM_FREE(path);
				CDA_FATAL(app_data.str_nomemory);
				return FALSE;
			}

			if (LSTAT(path, &stbuf) == 0 &&
			    (!S_ISREG(stbuf.st_mode) || !cda_unlink(path))) {
				(void) fprintf(errfp, "%s: %s\n",
						path,
						app_data.str_audiopathexists);
				MEM_FREE(path);
				return FALSE;
			}

			if (dpath == NULL &&
			    (dpath = util_dirname(path)) == NULL) {
				MEM_FREE(path);
				CDA_FATAL("File dirname error");
				return FALSE;
			}

			MEM_FREE(path);
		}

		if (dpath != NULL) {
			/* Show output directory path */
			(void) printf("%s: %s\n",
				"CDDA save-to-file output directory",
				dpath
			);
			(void) fflush(stdout);
			util_delayms(2000);

			MEM_FREE(dpath);
		}
	}
	else {
		path = (char *) MEM_ALLOC("path", FILE_PATH_SZ);
		if (path == NULL) {
			CDA_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		path[0] = '\0';

		if (!cda_mkfname(s, -1, path, s->outf_tmpl, FILE_PATH_SZ)) {
			MEM_FREE(path);
			return FALSE;
		}

		if (!util_newstr(&s->trkinfo[0].outfile, path)) {
			MEM_FREE(path);
			CDA_FATAL(app_data.str_nomemory);
			return FALSE;
		}

		if (LSTAT(path, &stbuf) == 0 &&
		    (!S_ISREG(stbuf.st_mode) || !cda_unlink(path))) {
			(void) fprintf(errfp, "%s: %s\n",
					path, app_data.str_audiopathexists);
			MEM_FREE(path);
			return FALSE;
		}

		/* Show output directory path */
		if ((dpath = util_dirname(path)) == NULL) {
			MEM_FREE(path);
			CDA_FATAL("File dirname error");
			return FALSE;
		}

		(void) printf("%s: %s\n",
			"CDDA save-to-file output directory",
			dpath
		);
		(void) fflush(stdout);

		MEM_FREE(dpath);
		MEM_FREE(path);

		util_delayms(2000);
	}

	return TRUE;
}


/*
 * cda_ckpipeprog
 *	Check pipe to program path
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return
 *	TRUE  - passed
 *	FALSE - failed
 */
/*ARGSUSED*/
STATIC bool_t
cda_ckpipeprog(curstat_t *s)
{
	char	*str;

	if (app_data.pipeprog == NULL) {
		(void) fprintf(errfp, "%s\n", app_data.str_noprog);
		return FALSE;
	}

	if (!util_checkcmd(app_data.pipeprog)) {
		str = (char *) MEM_ALLOC("str",
			strlen(app_data.pipeprog) +
			strlen(app_data.str_cannotinvoke) + 8
		);
		if (str == NULL) {
			CDA_FATAL(app_data.str_nomemory);
			return FALSE;
		}

		(void) sprintf(str, app_data.str_cannotinvoke,
				app_data.pipeprog);
		(void) fprintf(errfp, "%s\n", str);

		MEM_FREE(str);
		return FALSE;
	}

	return TRUE;
}


/*
 * cda_fix_outfile_path
 *	Fix the CDDA audio output file path to make sure there is a proper
 *	suffix based on the file format.  Also, replace white spaces in
 *	the file path string with underscores.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
cda_fix_outfile_path(curstat_t *s)
{
	filefmt_t	*fmp;
	char		*suf,
			*cp,
			tmp[FILE_PATH_SZ * 2 + 8];

	/* File suffix */
	if ((fmp = cdda_filefmt(app_data.cdda_filefmt)) == NULL)
		suf = "";
	else
		suf = fmp->suf;

	if (s->outf_tmpl == NULL) {
		/* No default outfile, set it */
		(void) sprintf(tmp,
			"%%S/%%C/%%I/%s%s",
			app_data.cdda_trkfile ? FILEPATH_TRACK : FILEPATH_DISC,
			suf
		);

		if (!util_newstr(&s->outf_tmpl, tmp)) {
			CDA_FATAL(app_data.str_nomemory);
			return;
		}
	}
	else if ((cp = strrchr(s->outf_tmpl, '.')) != NULL) {
		char	*cp2 = strrchr(s->outf_tmpl, DIR_END);

		if (cp2 == NULL || cp > cp2) {
			/* Change suffix if necessary */
			if (strcmp(cp, suf) != 0) {
				*cp = '\0';
				(void) strncpy(tmp, s->outf_tmpl,
					    FILE_PATH_SZ - strlen(suf) - 1);
				(void) strcat(tmp, suf);

				if (!util_newstr(&s->outf_tmpl, tmp)) {
					CDA_FATAL(app_data.str_nomemory);
					return;
				}
			}
		}
		else {
			/* No suffix, add one */
			(void) strncpy(tmp, s->outf_tmpl,
					FILE_PATH_SZ - strlen(suf) - 1);
			(void) strcat(tmp, suf);

			if (!util_newstr(&s->outf_tmpl, tmp)) {
				CDA_FATAL(app_data.str_nomemory);
				return;
			}
		}
	}
	else {
		/* No suffix, add one */
		(void) strncpy(tmp, s->outf_tmpl,
				FILE_PATH_SZ - strlen(suf) - 1);
		(void) strcat(tmp, suf);

		if (!util_newstr(&s->outf_tmpl, tmp)) {
			CDA_FATAL(app_data.str_nomemory);
			return;
		}
	}
}


/*
 * cda_hist_new
 *	Add current CD to the history list.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_hist_new(curstat_t *s)
{
	cdinfo_dlist_t	h;

	if (dbp->discid == 0)
		/* Don't add to history if no disc */
		return;

	h.device = s->curdev;
	h.discno = (int) s->cur_disc;
	h.type = ((dbp->flags & CDINFO_MATCH) == 0) ? 0 :
		((dbp->flags & (CDINFO_FROMLOC | CDINFO_FROMCDT)) ?
			CDINFO_DLIST_LOCAL : CDINFO_DLIST_REMOTE);
	h.discid = dbp->discid;
	h.genre = dbp->disc.genre;
	h.artist = dbp->disc.artist;
	h.title = dbp->disc.title;
	h.time = time(NULL);

	/* Add to in-core history list */
	(void) cdinfo_hist_addent(&h, TRUE);
}


/*
 * cda_dbget
 *	Load the CD information pertaining to the current disc, if available.
 *	This is for use by the cda daemon, and only as a callback from the
 *	libdi module.  Its sole purpose is to allow the daemon to properly
 *	expand the CDDA save-to-file output path names, and to do the
 *	initial connection to the MOTD service.  Otherwise the daemon
 *	doesn't really need the CD information to operate.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_dbget(curstat_t *s)
{
	cdinfo_ret_t	ret;
	static bool_t	do_motd = TRUE;

	/* Clear the in-core entry */
	cda_dbclear(s, TRUE);

	/* Load user-defined track program, if available */
	if ((ret = cdinfo_load_prog(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_load_prog: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}
	else
		s->program = TRUE;

	/* Parse play order string and set the play order */
	(void) cda_pgm_parse(s);

	/* Load CD information */
	if ((ret = cdinfo_load(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_load: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	switch (CDINFO_GET_STAT(ret)) {
	case 0:
		/* Success */
		if (dbp->matchlist != NULL) {
			cdinfo_match_t	*mp;
			int		n = 0;

			for (mp = dbp->matchlist; mp != NULL; mp = mp->next)
				n++;

			if (n != 1) {
				/* Note: The daemon cannot invoke a dialog
				 * with the user to select from multiple
				 * matches, so we do the next best thing
				 * and just select the first one.  There is
				 * a chance that it's wrong, but it is better
				 * than just failing every time.  We display
				 * a warning to notify the user.
				 */
				CDA_WARNING("CDDB has located multiple "
					"matches for this CD.  The first "
					"match is being used for the CDDA "
					"output file path name expansion.");
			}

			dbp->match_tag = (long) 1;

			/* Load CD information */
			if ((ret = cdinfo_load_matchsel(s)) != 0) {
				/* Failed to load */
				DBGPRN(DBG_CDI)(errfp,
					"\ncdinfo_load_matchsel (daemon): "
					"status=%d arg=%d\n",
					CDINFO_GET_STAT(ret),
					CDINFO_GET_ARG(ret));
				s->qmode = QMODE_NONE;
			}
			else if ((dbp->flags & CDINFO_MATCH) != 0)
				/* Exact match now */
				s->qmode = QMODE_MATCH;
			else
				s->qmode = QMODE_NONE;
		}
		else if ((dbp->flags & CDINFO_NEEDREG) != 0)
			/* User not registered with CDDB.
			 * Note: The daemon cannot invoke a dialog to let
			 * the user register with CDDB, but this should have
			 * been done on the client side already.  If it's
			 * not done, just fail.
			 */
			s->qmode = QMODE_NONE;
		else if ((dbp->flags & CDINFO_MATCH) != 0)
			/* Exact match found */
			s->qmode = QMODE_MATCH;
		else
			/* No match found */
			s->qmode = QMODE_NONE;

		/* Get MOTD: do this automatically just once if configured */
		if (!app_data.automotd_dsbl &&
		    !app_data.cdinfo_inetoffln && do_motd) {
			do_motd = FALSE;
			motd_get(NULL);
		}

		break;

	case AUTH_ERR:
		/* Authorization failed.
		 * Note: The daemon cannot invoke a dialog with the user
		 * to get the login and password, so all we could do is
		 * fail here.  A client should have passed the info
		 * to us if it had asked the user once, but it's possible
		 * that the client hasn't done a CDDB lookup yet.
		 */
		s->qmode = QMODE_NONE;
		break;

	default:
		/* Query error */
		s->qmode = QMODE_NONE;
		break;
	}

	/* Update CD history */
	cda_hist_new(s);
}


/*
 * cda_init
 *	Initialize the CD interface subsystem.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_init(curstat_t *s)
{
	char	*bdevname,
		path[FILE_PATH_SZ * 2];

	/* Set address to cdinfo incore structure */
	dbp = cdinfo_addr();

	if ((bdevname = util_basename(app_data.device)) == NULL)
		CDA_FATAL("Device path basename error: Aborting.");

	/* Get system-wide device-specific configuration parameters */
	(void) sprintf(path, SYS_DSCFG_PATH, app_data.libdir, bdevname);
	di_devspec_parmload(path, TRUE, FALSE);

	/* Get user device-specific configuration parameters */
	(void) sprintf(path, USR_DSCFG_PATH, util_homedir(util_get_ouid()),
		       bdevname);
	di_devspec_parmload(path, FALSE, FALSE);

	MEM_FREE(bdevname);

	/* Initialize CD info */
	cda_dbclear(s, FALSE);

	if (isdaemon) {
		/* Initialize CD history mechanism */
		cdinfo_hist_init();

		/* Initialize the CD hardware */
		di_cldata.curstat_addr = curstat_addr;
		di_cldata.mkoutpath = cda_mkoutpath;
		di_cldata.ckpipeprog = cda_ckpipeprog;
		di_cldata.quit = cda_quit;
		di_cldata.dbclear = cda_dbclear;
		di_cldata.dbget = cda_dbget;
		di_cldata.fatal_msg = cda_fatal_msg;
		di_cldata.warning_msg = cda_warning_msg;
		di_cldata.info_msg = cda_info_msg;
		di_init(&di_cldata);

		/* Set default modes */
		di_repeat(s, app_data.repeat_mode);
		di_shuffle(s, app_data.shuffle_mode);

		/* Set default output file format and path */
		if (!cdda_filefmt_supp(app_data.cdda_filefmt)) {
			/* Specified format not supported: force to RAW */
			(void) fprintf(errfp,
				"The cddaFileFormat (%d) is not supported: "
				"reset to RAW\n", app_data.cdda_filefmt);
			app_data.cdda_filefmt = FILEFMT_RAW;
		}
		if (app_data.cdda_tmpl != NULL &&
		    !util_newstr(&s->outf_tmpl, app_data.cdda_tmpl)) {
			(void) fprintf(errfp,
				"%s: Aborting.\n", app_data.str_nomemory);
			exit_status = 1;
			cda_quit(s);
			/*NOTREACHED*/
		}
		cda_fix_outfile_path(s);

		/* Play mode and capabilities */
		if ((di_cldata.capab & CAPAB_PLAYAUDIO) == 0) {
			(void) fprintf(errfp,
				"No CD playback capability.  Aborting.\n");
			exit_status = 1;
			cda_quit(s);
			/*NOTREACHED*/
		}

		if (PLAYMODE_IS_CDDA(app_data.play_mode)) {
			if ((di_cldata.capab & CAPAB_RDCDDA) == 0) {
				app_data.play_mode = PLAYMODE_STD;
			}
			else {
				if ((di_cldata.capab & CAPAB_WRDEV) == 0)
					app_data.play_mode &= ~PLAYMODE_CDDA;
				if ((di_cldata.capab & CAPAB_WRFILE) == 0)
					app_data.play_mode &= ~PLAYMODE_FILE;
				if ((di_cldata.capab & CAPAB_WRPIPE) == 0)
					app_data.play_mode &= ~PLAYMODE_PIPE;
			}

			/* Set the default play mode */
			if (PLAYMODE_IS_CDDA(app_data.play_mode) &&
			    !di_playmode(s)) {
				app_data.play_mode = PLAYMODE_STD;
			}

			if (PLAYMODE_IS_STD(app_data.play_mode)) {
				(void) fprintf(errfp,
				    "Reverted to standard playback mode.\n");
			}
		}
	}
	else
		di_reset_curstat(s, TRUE, TRUE);

	/* Force CD-TEXT config to False if it doesn't appear in the
	 * CD info path list.
	 */
	if (!cdinfo_cdtext_iscfg())
		app_data.cdtext_dsbl = TRUE;
}


/*
 * cda_start
 *	Secondary startup functions.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_start(curstat_t *s)
{
	struct stat	stbuf;

	/* Start up libutil */
	util_start();

	/* Start up I/O interface */
	di_start(s);

	/* Open FIFOs - daemon side */
	cda_sfd[0] = open(spipe, O_RDONLY
#ifdef O_NOFOLLOW
		| O_NOFOLLOW
#endif
	);
	if (cda_sfd[0] < 0) {
		perror("CD audio daemon: cannot open send pipe");
		exit_status = 4;
		cda_quit(s);
		/*NOTREACHED*/
	}
	cda_rfd[0] = open(rpipe, O_WRONLY
#ifdef O_NOFOLLOW
		| O_NOFOLLOW
#endif
	);
	if (cda_rfd[0] < 0) {
		perror("CD audio daemon: cannot open recv pipe");
		exit_status = 4;
		cda_quit(s);
		/*NOTREACHED*/
	}

	/* Check FIFOs */
	if (fstat(cda_sfd[0], &stbuf) < 0 || !S_ISFIFO(stbuf.st_mode)) {
		(void) fprintf(errfp,
			    "CD audio daemon: Send pipe error: Not a FIFO\n");
		di_halt(s);
		/* Remove lock */
		if (dlock[0] != '\0')
			(void) UNLINK(dlock);
		_exit(4);
	}
	if (fstat(cda_rfd[0], &stbuf) < 0 || !S_ISFIFO(stbuf.st_mode)) {
		(void) fprintf(errfp,
			    "CD audio daemon: Recv pipe error: Not a FIFO\n");
		di_halt(s);
		/* Remove lock */
		if (dlock[0] != '\0')
			(void) UNLINK(dlock);
		_exit(4);
	}
}


/*
 * cda_daemonlock
 *	Check and set a lock to prevent multiple CD audio daemon
 *	processes.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE - Lock successful
 *	FALSE - Daemon already running
 */
STATIC bool_t
cda_daemonlock(void)
{
	int		fd;
	pid_t		pid,
			mypid;
	char		buf[32];

	(void) sprintf(dlock, "%s/cdad.%x", TEMP_DIR, (int) cd_rdev);
	mypid = getpid();

	for (;;) {
		fd = open(dlock, O_CREAT | O_EXCL | O_WRONLY);
		if (fd < 0) {
			if (errno == EEXIST) {
				fd = open(dlock, O_RDONLY
#ifdef O_NOFOLLOW
						 | O_NOFOLLOW
#endif
				);
				if (fd < 0)
					return FALSE;

				if (read(fd, buf, 32) > 0) {
					if (strncmp(buf, "cdad ", 5) != 0) {
						(void) close(fd);
						return FALSE;
					}
					pid = (pid_t) atoi(buf + 5);
				}
				else {
					(void) close(fd);
					return FALSE;
				}

				(void) close(fd);

				if (pid == mypid)
					/* Our own lock */
					return TRUE;

				if (pid <= 0 ||
				    (kill(pid, 0) < 0 && errno == ESRCH)) {
					/* Pid died, steal its lockfile */
					(void) UNLINK(dlock);
				}
				else {
					/* Pid still running: clash */
					return FALSE;
				}
			}
			else
				return FALSE;
		}
		else {
			(void) sprintf(buf, "cdad %d\n", (int) mypid);
			(void) write(fd, buf, strlen(buf));

#ifdef NO_FCHMOD
			(void) close(fd);
			(void) chmod(dlock, 0644);
#else
			(void) fchmod(fd, 0644);
			(void) close(fd);
#endif

			return TRUE;
		}
	}
}


/*
 * cda_poll
 *	Periodic polling function - used to manage program, shuffle,
 *	and repeat modes.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_poll(curstat_t *s)
{
	static int	n = 0;

	if (++n > 100)
		n = 0;

	if (n % 3)
		return;

	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY ||
	    (s->mode == MOD_STOP && app_data.load_play))
		(void) di_check_disc(s);
	else if (s->mode != MOD_STOP && s->mode != MOD_PAUSE &&
		 s->mode != MOD_NODISC)
		di_status_upd(s);
}


/*
 * cda_sendpkt
 *	Write a CDA packet down the pipe.
 *
 * Args:
 *	name - The text string describing the caller module
 *	fd - Pipe file descriptor
 *	s - Pointer to the packet data
 *
 * Return:
 *	TRUE - pipe write successful
 *	FALSE - pipe write failed
 */
STATIC bool_t
cda_sendpkt(char *name, int fd, cdapkt_t *s)
{
	byte_t	*p = (byte_t *) s;
	int	i,
		ret;

	if (fd < 0)
		return FALSE;

	/* Brand packet with magic number */
	s->magic = CDA_MAGIC;

	/* Set timeout */
	(void) util_signal(SIGALRM, onsig1);
	(void) alarm(PIPE_TIMEOUT);

	/* Send a packet */
	i = sizeof(cdapkt_t);
	while ((ret = write(fd, p, i)) < i) {
		if (ret < 0) {
			switch (errno) {
			case EINTR:
			case EPIPE:
			case EBADF:
				(void) alarm(0);

				if (isdaemon)
					return FALSE;

				if (!cda_daemon_alive())
					return FALSE;
				
				(void) util_signal(SIGALRM, onsig1);
				(void) alarm(PIPE_TIMEOUT);
				break;
			default:
				(void) alarm(0);
				(void) sprintf(errmsg,
					"%s: packet write error (errno=%d)\n",
					name, errno);
				CDA_WARNING(errmsg);
				return FALSE;
			}
		}
		else if (ret == 0) {
			(void) alarm(0);
			util_delayms(1000);
			(void) util_signal(SIGALRM, onsig1);
			(void) alarm(PIPE_TIMEOUT);
		}
		else {
			i -= ret;
			p += ret;
		}
	}

	(void) alarm(0);
	return TRUE;
}


/*
 * cda_getpkt
 *	Read a CDA packet from the pipe.
 *
 * Args:
 *	name - The text string describing the caller module
 *	fd - Pipe file descriptor
 *	s - Pointer to the packet data
 *
 * Return:
 *	TRUE - pipe read successful
 *	FALSE - pipe read failed
 */
STATIC bool_t
cda_getpkt(char *name, int fd, cdapkt_t *r)
{
	byte_t	*p = (byte_t *) r;
	int	i,
		ret;

	if (fd < 0)
		return FALSE;

	/* Get a packet */
	i = sizeof(cdapkt_t);
	while ((ret = read(fd, p, i)) < i) {
		if (ret < 0) {
			switch (errno) {
			case EINTR:
			case EPIPE:
			case EBADF:
				if (!isdaemon)
					return FALSE;

				/* Use this occasion to perform
				 * polling function
				 */
				cda_poll(&status);

				util_delayms(1000);
				break;
			default:
				(void) sprintf(errmsg,
					"%s: packet read error (errno=%d)\n",
					name, errno);
				CDA_WARNING(errmsg);
				return FALSE;
			}
		}
		else if (ret == 0) {
			if (isdaemon) {
				/* Use this occasion to
				 * perform polling function
				 */
				cda_poll(&status);
			}

			util_delayms(1000);
		}
		else {
			i -= ret;
			p += ret;
		}
	}

	/* Check packet for magic number */
	if (r->magic != CDA_MAGIC) {
		(void) sprintf(errmsg, "%s: bad packet magic number.", name);
		CDA_WARNING(errmsg);
		return FALSE;
	}

	return TRUE;
}


/*
 * cda_do_onoff
 *	Perform the "on" and "off" commands.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_onoff(curstat_t *s, cdapkt_t *p)
{
	p->arg[0] = getpid();
}


/*
 * cda_do_disc
 *	Perform the "disc" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_disc(curstat_t *s, cdapkt_t *p)
{
	int	i;

	if (s->mode == MOD_BUSY)
		p->retcode = CDA_INVALID;
	else if (p->arg[0] == 0) {
		/* Load */
		if (s->mode == MOD_NODISC)
			di_load_eject(s);
		else
			p->retcode = CDA_INVALID;
	}
	else if (p->arg[0] == 1) {
		/* Eject */
		if (s->mode != MOD_NODISC)
			di_load_eject(s);
		else
			p->retcode = CDA_INVALID;
	}
	else if (p->arg[0] == 2) {
		/* Next */
		if (s->cur_disc < s->last_disc) {
			s->prev_disc = s->cur_disc;
			s->cur_disc++;
			di_chgdisc(s);
		}
		else
			p->retcode = CDA_INVALID;
	}
	else if (p->arg[0] == 3) {
		/* Prev */
		if (s->cur_disc > s->first_disc) {
			s->prev_disc = s->cur_disc;
			s->cur_disc--;
			di_chgdisc(s);
		}
		else
			p->retcode = CDA_INVALID;
	}
	else if (p->arg[0] == 4) {
		/* disc# */
		i = p->arg[1];
		if (i >= s->first_disc && i <= s->last_disc) {
			if (s->cur_disc != i) {
				s->prev_disc = s->cur_disc;
				s->cur_disc = i;
				di_chgdisc(s);
			}
		}
		else
			p->retcode = CDA_INVALID;
	}
}


/*
 * cda_do_lock
 *	Perform the "lock" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_lock(curstat_t *s, cdapkt_t *p)
{
	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY)
		p->retcode = CDA_INVALID;
	else if (p->arg[0] == 0) {
		/* Unlock */
		if (s->caddy_lock) {
			s->caddy_lock = FALSE;
			di_lock(s, FALSE);
		}
		else
			p->retcode = CDA_INVALID;
	}
	else if (p->arg[0] == 1) {
		/* Lock */
		if (!s->caddy_lock) {
			s->caddy_lock = TRUE;
			di_lock(s, TRUE);
		}
		else
			p->retcode = CDA_INVALID;
	}
	else
		p->retcode = CDA_INVALID;
}


/*
 * cda_do_play
 *	Perform the "play" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_play(curstat_t *s, cdapkt_t *p)
{
	int		i;
	sword32_t	blkno;
	bool_t		stopfirst;

	switch (s->mode) {
	case MOD_PLAY:
		stopfirst = TRUE;
		break;
	case MOD_BUSY:
	case MOD_NODISC:
		p->retcode = CDA_INVALID;
		return;
	case MOD_STOP:
	default:
		stopfirst = FALSE;

		if (p->arg[0] != 0)
			s->cur_trk = p->arg[0];

		break;
	}

	/* Starting track number */
	i = -1;
	if (p->arg[0] != 0) {
		if (s->shuffle || s->prog_cnt > 0) {
			p->retcode = CDA_INVALID;
			return;
		}

		for (i = 0; i < (int) s->tot_trks; i++) {
			if (s->trkinfo[i].trkno == p->arg[0])
				break;
		}

		if (i >= (int) s->tot_trks) {
			/* Invalid track specified */
			p->retcode = CDA_PARMERR;
			return;
		}

		s->cur_trk = p->arg[0];
	}

	if (stopfirst) {
		/* Stop current playback first */
		di_stop(s, FALSE);

		/*
		 * Restore s->cur_trk value because di_stop() zaps it
		 */
		if (p->arg[0] != 0)
			s->cur_trk = p->arg[0];
	}

	if (p->arg[0] != 0 && (int) p->arg[1] >= 0 && (int) p->arg[2] >= 0) {
		sword32_t	eot;

		/* Track offset specified */
		if (p->arg[2] > 59) {
			p->retcode = CDA_PARMERR;
			return;
		}

		util_msftoblk(
			(byte_t) p->arg[1],
			(byte_t) p->arg[2],
			0,
			&blkno,
			0
		);

		eot = s->trkinfo[i+1].addr;

		/* "Enhanced CD" / "CD Extra" hack */
		if (eot > s->discpos_tot.addr)
			eot = s->discpos_tot.addr;

		if (blkno >= (eot - s->trkinfo[i].addr)) {
			p->retcode = CDA_PARMERR;
			return;
		}

		s->curpos_trk.addr = blkno;
		util_blktomsf(
			s->curpos_trk.addr,
			&s->curpos_trk.min,
			&s->curpos_trk.sec,
			&s->curpos_trk.frame,
			0
		);

		s->curpos_tot.addr = s->trkinfo[i].addr + s->curpos_trk.addr;
		util_blktomsf(
			s->curpos_tot.addr,
			&s->curpos_tot.min,
			&s->curpos_tot.sec,
			&s->curpos_tot.frame,
			MSF_OFFSET
		);
	}

	/* Start playback */
	di_play_pause(s);
}


/*
 * cda_do_pause
 *	Perform the "pause" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_pause(curstat_t *s, cdapkt_t *p)
{
	if (s->mode == MOD_PLAY)
		di_play_pause(s);
	else
		p->retcode = CDA_INVALID;
}


/*
 * cda_do_stop
 *	Perform the "stop" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_stop(curstat_t *s, cdapkt_t *p)
{
	if (s->mode == MOD_BUSY)
		p->retcode = CDA_INVALID;
	else
		di_stop(s, TRUE);
}


/*
 * cda_do_track
 *	Perform the "track" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_track(curstat_t *s, cdapkt_t *p)
{
	int	i;

	if (p->arg[0] == 0) {
		/* Previous track */
		if (s->mode == MOD_PLAY) {
			if ((i = di_curtrk_pos(s)) > 0)
				s->curpos_tot.addr = s->trkinfo[i].addr;
			di_prevtrk(s);
		}
		else
			p->retcode = CDA_INVALID;
	}
	else {
		/* Next track */
		if (s->mode == MOD_PLAY)
			di_nexttrk(s);
		else
			p->retcode = CDA_INVALID;
	}
}


/*
 * cda_do_index
 *	Perform the "index" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_index(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Previous index */
		if (s->mode == MOD_PLAY && s->prog_tot <= 0) {
			if (s->cur_idx > 1)
				s->curpos_tot.addr = s->sav_iaddr;
			di_previdx(s);
		}
		else
			p->retcode = CDA_INVALID;
	}
	else {
		/* Next index */
		if (s->mode == MOD_PLAY && s->prog_tot <= 0)
			di_nextidx(s);
		else
			p->retcode = CDA_INVALID;
	}
}


/*
 * cda_do_program
 *	Perform the "program" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_program(curstat_t *s, cdapkt_t *p)
{
	int		i,
			j;
	cdinfo_ret_t	ret;

	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY)
		p->retcode = CDA_INVALID;
	else if ((int) p->arg[0] == 1) {
		/* Query */
		if ((int) s->prog_tot > (CDA_NARGS - 2)) {
			p->retcode = CDA_INVALID;
			return;
		}
		p->arg[0] = (word32_t) s->prog_tot;
		p->arg[1] = (word32_t) s->prog_cnt;
		p->arg[2] = (word32_t) -1;

		for (i = 0; i < (int) s->prog_tot; i++) {
			p->arg[i+2] = (word32_t)
				s->trkinfo[s->trkinfo[i].playorder].trkno;
		}
	}
	else if ((int) p->arg[0] == 2) {
		/* Save */
		if (s->shuffle) {
			p->retcode = CDA_INVALID;
			return;
		}

		p->arg[0] = (word32_t) 0;
		p->arg[2] = (word32_t) -2;
		if (!s->program) {
			p->arg[2] = (word32_t) -1;
			return;
		}

		if ((ret = cdinfo_save_prog(s)) != 0) {
			DBGPRN(DBG_CDI)(errfp,
				"cdinfo_save_prog: status=%d arg=%d\n",
				CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
		}
	}
	else if ((int) p->arg[0] == 0) {
		/* Clear */
		if (s->shuffle) {
			/* program and shuffle modes are mutually-exclusive */
			p->retcode = CDA_INVALID;
			return;
		}

		p->arg[1] = (word32_t) 0;
		p->arg[2] = (word32_t) -3;
		s->prog_tot = 0;
		s->program = FALSE;

		if ((ret = cdinfo_del_prog()) != 0) {
			DBGPRN(DBG_CDI)(errfp,
				"cdinfo_del_prog: status=%d arg=%d\n",
				CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
		}

		if (dbp->playorder != NULL) {
			MEM_FREE(dbp->playorder);
			dbp->playorder = NULL;
		}
	}
	else if ((int) p->arg[0] < 0) {
		/* Define */
		if (s->shuffle) {
			/* program and shuffle modes are mutually-exclusive */
			p->retcode = CDA_INVALID;
			return;
		}

		s->prog_tot = -(p->arg[0]);
		s->program = TRUE;

		if (dbp->playorder != NULL) {
			MEM_FREE(dbp->playorder);
			dbp->playorder = NULL;
		}

		for (i = 0; i < (int) s->prog_tot; i++) {
			char	num[8];

			for (j = 0; j < (int) s->tot_trks; j++) {
				if (s->trkinfo[j].trkno == p->arg[i+1])
					break;
			}

			if (j >= (int) s->tot_trks) {
				s->prog_tot = 0;
				s->program = FALSE;
				p->retcode = CDA_PARMERR;
				return;
			}

			s->trkinfo[i].playorder = j;

			(void) sprintf(num, "%d", (int) s->trkinfo[j].trkno);
			if (dbp->playorder == NULL) {
				if (!util_newstr(&dbp->playorder, num)) {
					p->retcode = CDA_FAILED;
					return;
				}
			}
			else {
				dbp->playorder = (char *) MEM_REALLOC(
				    "playorder",
				    dbp->playorder,
				    strlen(dbp->playorder) + strlen(num) + 2
				);
				if (dbp->playorder == NULL) {
					p->retcode = CDA_FAILED;
					return;
				}
				(void) sprintf(dbp->playorder, "%s,%s",
					       dbp->playorder, num);
			}
		}
	}
	else
		p->retcode = CDA_PARMERR;
}


/*
 * cda_do_shuffle
 *	Perform the "shuffle" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_shuffle(curstat_t *s, cdapkt_t *p)
{
	if (s->program) {
		p->retcode = CDA_INVALID;
		return;
	}

	if (s->mode != MOD_NODISC && s->mode != MOD_STOP) {
		p->retcode = CDA_INVALID;
		return;
	}

	di_shuffle(s, (bool_t) (p->arg[0] == 1));
}


/*
 * cda_do_repeat
 *	Perform the "repeat" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_repeat(curstat_t *s, cdapkt_t *p)
{
	di_repeat(s, (bool_t) (p->arg[0] == 1));
}


/*
 * cda_do_volume
 *	Perform the "volume" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_volume(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) s->level;
		p->arg[2] = (word32_t) app_data.vol_taper;
	}
	else if (p->arg[0] == 1) {
		/* Set volume level */
		if ((int) p->arg[1] >= 0 && (int) p->arg[1] <= 100)
			di_level(s, (byte_t) p->arg[1], FALSE);
	}
	else if (p->arg[0] == 2) {
		/* Set volume taper */
		if ((int) p->arg[2] >= 0 && (int) p->arg[2] <= 100)
			app_data.vol_taper = (int) p->arg[2];
	}
	else
		p->retcode = CDA_PARMERR;
}


/*
 * cda_do_balance
 *	Perform the "balance" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_balance(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t)
		    ((int) (s->level_right - s->level_left) / 2) + 50;
	}
	else if ((int) p->arg[1] == 50) {
		/* Center setting */
		s->level_left = s->level_right = 100;
		di_level(s, (byte_t) s->level, FALSE);
	}
	else if ((int) p->arg[1] < 50 && (int) p->arg[1] >= 0) {
		/* Attenuate the right channel */
		s->level_left = 100;
		s->level_right = 100 + (((int) p->arg[1] - 50) * 2);
		di_level(s, (byte_t) s->level, FALSE);
	}
	else if ((int) p->arg[1] > 50 && (int) p->arg[1] <= 100) {
		/* Attenuate the left channel */
		s->level_left = 100 - (((int) p->arg[1] - 50) * 2);
		s->level_right = 100;
		di_level(s, (byte_t) s->level, FALSE);
	}
	else
		p->retcode = CDA_PARMERR;
}


/*
 * cda_do_route
 *	Perform the "route" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_route(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.ch_route;
	}
	else if ((int) p->arg[1] >= 0 && (int) p->arg[1] <= 4) {
		/* Set */
		app_data.ch_route = (int) p->arg[1];
		di_route(s);
	}
	else
		p->retcode = CDA_PARMERR;
}


/*
 * cda_do_outport
 *	Perform the "outport" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_outport(curstat_t *s, cdapkt_t *p)
{
	switch ((int) p->arg[0]) {
	case 0:
		/* Query */
		p->arg[1] = (word32_t) app_data.outport;
		break;
	case 1:
		/* Toggle */
		app_data.outport ^= (word32_t) p->arg[1];
		break;
	case 2:
		/* Set */
		app_data.outport = (word32_t) p->arg[1];
		break;
	default:
		p->retcode = CDA_PARMERR;
		return;
	}

	cdda_outport();
}


/*
 * cda_do_cdda_att
 *	Perform the "cdda-att" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_cdda_att(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) s->cdda_att;
	}
	else if (p->arg[0] == 1) {
		/* Set CDDA attenuator level */
		if ((int) p->arg[1] >= 0 && (int) p->arg[1] <= 100) {
			s->cdda_att = (byte_t) p->arg[1];
			cdda_att(s);
		}
	}
	else
		p->retcode = CDA_PARMERR;
}


/*
 * cda_do_status
 *	Perform the "status" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_status(curstat_t *s, cdapkt_t *p)
{
	word32_t	discid;

	/* Initialize */
	(void) memset(p->arg, 0, CDA_NARGS * sizeof(word32_t));

	WR_ARG_MODE(p->arg[0], s->mode);

	if (s->caddy_lock)
		WR_ARG_LOCK(p->arg[0]);
	if (s->shuffle)
		WR_ARG_SHUF(p->arg[0]);
	if (s->repeat)
		WR_ARG_REPT(p->arg[0]);
	if (s->program)
		WR_ARG_PROG(p->arg[0]);

	WR_ARG_TRK(p->arg[1], s->cur_trk);
	WR_ARG_IDX(p->arg[1], s->cur_idx);
	WR_ARG_MIN(p->arg[1], s->curpos_trk.min);
	WR_ARG_SEC(p->arg[1], s->curpos_trk.sec);

	if (s->repeat && s->mode == MOD_PLAY)
		p->arg[2] = (word32_t) s->rptcnt;
	else
		p->arg[2] = (word32_t) -1;

	p->arg[3] = (word32_t) s->level;
	p->arg[4] = (word32_t)
		    ((int) (s->level_right - s->level_left) / 2) + 50;
	p->arg[5] = (word32_t) app_data.ch_route;
	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY)
		discid = 0;
	else
		discid = cdinfo_discid(s);

	dbp->discid = discid;
	p->arg[7] = s->cur_disc;
	p->arg[6] = discid;
}


/*
 * cda_do_toc
 *	Perform the "toc" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_toc(curstat_t *s, cdapkt_t *p)
{
	int		i,
			j;
	word32_t	min,
			sec;
	bool_t		offsets;

	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY) {
		p->retcode = CDA_INVALID;
		return;
	}

	offsets = (p->arg[0] == 1);

	/* Initialize */
	(void) memset(p->arg, 0, CDA_NARGS * sizeof(word32_t));

	p->arg[0] = cdinfo_discid(s);

	if (offsets) {
		for (i = 0; i < (int) s->tot_trks; i++) {
			WR_ARG_TOC(p->arg[i+1], s->trkinfo[i].trkno,
				   s->cur_trk,
				   s->trkinfo[i].min,
				   s->trkinfo[i].sec);
		}
	}
	else for (i = 0; i < (int) s->tot_trks; i++) {
		j = ((s->trkinfo[i+1].min * 60 + s->trkinfo[i+1].sec) - 
		     (s->trkinfo[i].min * 60 + s->trkinfo[i].sec));

		/* "Enhanced CD" / "CD Extra" hack */
		if (s->trkinfo[i].type == TYP_AUDIO &&
		    s->trkinfo[i+1].addr > s->discpos_tot.addr) {
		    j -= ((s->trkinfo[i+1].addr - s->discpos_tot.addr) /
			   FRAME_PER_SEC);
		}
		min = j / 60;
		sec = j % 60;

		WR_ARG_TOC(p->arg[i+1], s->trkinfo[i].trkno,
			   s->cur_trk, min, sec);
	}

	/* Lead-out track */
	WR_ARG_TOC(p->arg[i+1], s->trkinfo[i].trkno,
		   0, s->trkinfo[i].min, s->trkinfo[i].sec);
}


/*
 * cda_do_toc2
 *	Perform the "toc2" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_toc2(curstat_t *s, cdapkt_t *p)
{
	int	i;

	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY) {
		p->retcode = CDA_INVALID;
		return;
	}

	/* Initialize */
	(void) memset(p->arg, 0, CDA_NARGS * sizeof(word32_t));

	p->arg[0] = cdinfo_discid(s);

	for (i = 0; i < (int) s->tot_trks; i++) {
		WR_ARG_TOC2(p->arg[i+1], s->trkinfo[i].trkno,
			   s->trkinfo[i].min,
			   s->trkinfo[i].sec,
			   s->trkinfo[i].frame
		);
	}

	/* Lead-out track */
	WR_ARG_TOC2(p->arg[i+1], s->trkinfo[i].trkno,
		   s->trkinfo[i].min,
		   s->trkinfo[i].sec,
		   s->trkinfo[i].frame
	);
}


/*
 * cda_do_extinfo
 *	Perform the "extinfo" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_extinfo(curstat_t *s, cdapkt_t *p)
{
	int	i,
		j;

	if (s->mode == MOD_NODISC || s->mode == MOD_BUSY) {
		p->retcode = CDA_INVALID;
		return;
	}

	p->arg[0] = cdinfo_discid(s);
	p->arg[2] = (word32_t) -1;

	if ((int) p->arg[1] == -1) {
		if (s->mode != MOD_PLAY) {
			p->arg[1] = p->arg[2] = (word32_t) -1;
			return;
		}

		p->arg[1] = (word32_t) s->cur_trk;
		j = (int) s->cur_trk;
	}
	else
		j = (int) p->arg[1];

	for (i = 0; i < (int) s->tot_trks; i++) {
		if ((int) s->trkinfo[i].trkno == j)
			break;
	}
	if (i < (int) s->tot_trks)
		p->arg[2] = i;
	else
		p->retcode = CDA_PARMERR;
}


/*
 * cda_doc_on_load
 *	Perform the "on-load" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_on_load(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.caddy_lock;
		p->arg[2] = (word32_t) app_data.load_spindown;
		p->arg[3] = (word32_t) app_data.load_play;
	}
	else {
		switch (p->arg[1]) {
		case 0: /* none */
			app_data.load_spindown = FALSE;
			app_data.load_play = FALSE;
			break;
		case 1: /* spindown */
			app_data.load_spindown = TRUE;
			app_data.load_play = FALSE;
			break;
		case 2: /* autoplay */
			app_data.load_spindown = FALSE;
			app_data.load_play = TRUE;
			break;
		case 3:	/* noautolock */
			app_data.caddy_lock = FALSE;
			break;
		case 4: /* autolock */
			app_data.caddy_lock = TRUE;
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_on_exit
 *	Perform the "on-exit" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_on_exit(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.exit_stop;
		p->arg[2] = (word32_t) app_data.exit_eject;
	}
	else {
		switch (p->arg[1]) {
		case 0: /* none */
			app_data.exit_stop = FALSE;
			app_data.exit_eject = FALSE;
			break;
		case 1: /* autostop */
			app_data.exit_stop = TRUE;
			app_data.exit_eject = FALSE;
			break;
		case 2: /* autoeject */
			app_data.exit_stop = FALSE;
			app_data.exit_eject = TRUE;
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_on_done
 *	Perform the "on-done" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_on_done(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.done_eject;
		p->arg[2] = (word32_t) app_data.done_exit;
	}
	else {
		switch (p->arg[1]) {
		case 0:	/* noautoeject */
			app_data.done_eject = FALSE;
			break;
		case 1: /* autoeject */
			app_data.done_eject = TRUE;
			break;
		case 2: /* noautoexit */
			app_data.done_exit = FALSE;
			break;
		case 3: /* autoexit */
			app_data.done_exit = TRUE;
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_on_eject
 *	Perform the "on-eject" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_on_eject(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.eject_exit;
	}
	else {
		/* set [no]autoeject */
		app_data.eject_exit = (p->arg[1] == 0) ? FALSE : TRUE;
	}
}


/*
 * cda_do_chgr
 *	Perform the "changer" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_chgr(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.multi_play;
		p->arg[2] = (word32_t) app_data.reverse;
	}
	else {
		switch (p->arg[1]) {
		case 0:	/* nomultiplay */
			app_data.multi_play = FALSE;
			app_data.reverse = FALSE;
			break;
		case 1: /* multiplay */
			app_data.multi_play = TRUE;
			break;
		case 2: /* noreverse */
			app_data.reverse = FALSE;
			break;
		case 3: /* reverse */
			app_data.multi_play = TRUE;
			app_data.reverse = TRUE;
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_mode
 *	Perform the "mode" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_mode(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.play_mode;
	}
	else {
		switch (s->mode) {
		case MOD_PLAY:
		case MOD_PAUSE:
		case MOD_SAMPLE:
			/* Disallow changing the mode while playing */
			p->retcode = CDA_INVALID;
			return;

		default:
			break;
		}

		if (p->arg[1] == 0)
			app_data.play_mode = PLAYMODE_STD;
		else {
			if ((app_data.play_mode & p->arg[1]) == 0)
				app_data.play_mode |= (int) p->arg[1];
			else
				app_data.play_mode &= (int) ~(p->arg[1]);
		}

		if (!di_playmode(s)) {
			app_data.play_mode = PLAYMODE_STD;
			p->retcode = CDA_REFUSED;
		}

		p->arg[1] = (word32_t) app_data.play_mode;

		if (PLAYMODE_IS_STD(app_data.play_mode))
			app_data.vol_taper = VOLTAPER_LINEAR;
	}
}


/*
 * cda_do_jitter
 *	Perform the "jittercorr" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_jitter(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.cdda_jitter_corr;
	}
	else if (app_data.cdda_jitter_corr != (bool_t) p->arg[1]) {
		/* Set */

		if (di_isdemo())
			return;	/* Don't actually do anything in demo mode */

		app_data.cdda_jitter_corr = (bool_t) p->arg[1];

		/* Notify device interface of the change */
		di_cddajitter(s);
	}
}


/*
 * cda_do_trkfile
 *	Perform the "trackfile" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_trkfile(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.cdda_trkfile;
	}
	else if (app_data.cdda_trkfile != (bool_t) p->arg[1]) {
		/* Set */
		app_data.cdda_trkfile = (bool_t) p->arg[1];

		/* Set new output file path template */
		cda_fix_outfile_path(s);

		(void) fprintf(errfp,
			"Notice: %s\n",
			"The output file path template has been changed."
		);
	}
}


/*
 * cda_do_subst
 *	Perform the "subst" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_subst(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.subst_underscore;
	}
	else if (app_data.subst_underscore != (bool_t) p->arg[1]) {
		/* Set */
		app_data.subst_underscore = (bool_t) p->arg[1];
	}
}


/*
 * cda_do_filefmt
 *	Perform the "filefmt" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_filefmt(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.cdda_filefmt;
	}
	else if (app_data.cdda_filefmt != (int) p->arg[1]) {
		if (!cdda_filefmt_supp((int) p->arg[1])) {
			/* The requested file format is not supported */
			(void) fprintf(errfp, "Error: %s\n",
				"Support for the requested file format "
				"is not installed.\n"
			);
			return;
		}

		/* Set */
		app_data.cdda_filefmt = (int) p->arg[1];

		/* Set new output file path template */
		cda_fix_outfile_path(s);

		(void) fprintf(errfp,
			"Notice: %s\n",
			"The output file path template has been changed."
		);
	}
}


/*
 * cda_do_file
 *	Update the output file path template
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_file(curstat_t *s, cdapkt_t *p)
{
	int		i;
	filefmt_t	*fmp,
			*fmp2;
	char		*cp,
			*cp2,
			*cp3,
			*suf;
	size_t		maxsz = (CDA_NARGS - 1) * sizeof(word32_t);
	int		newfmt;

	cp = (char *) &p->arg[1];

	if (p->arg[0] == 0) {
		/* Query */
		if (s->outf_tmpl == NULL)
			*cp = '\0';
		else {
			(void) strncpy(cp, s->outf_tmpl, maxsz - 1);
			*(cp + maxsz - 1) = '\0';
		}
	}
	else {
		/* Set */
		if (s->outf_tmpl != NULL && strcmp(s->outf_tmpl, cp) == 0) {
			/* No change */
			return;
		}

		/* File suffix */
		if ((fmp = cdda_filefmt(app_data.cdda_filefmt)) == NULL)
			suf = "";
		else
			suf = fmp->suf;

		/* Check if file suffix indicates a change in file format */
		newfmt = app_data.cdda_filefmt;
		if ((cp2 = strrchr(cp, '.')) != NULL) {
			if ((cp3 = strrchr(cp, DIR_END)) != NULL &&
			    cp2 > cp3) {
			    if (strcmp(suf, cp2) != 0) {
				for (i = 0; i < MAX_FILEFMTS; i++) {
				    if ((fmp2 = cdda_filefmt(i)) == NULL)
					continue;

				    if (util_strcasecmp(cp2, fmp2->suf) == 0)
					newfmt = fmp2->fmt;
				}
			    }
			}
			else if (strcmp(suf, cp2) != 0) {
			    for (i = 0; i < MAX_FILEFMTS; i++) {
				if ((fmp2 = cdda_filefmt(i)) == NULL)
				    continue;

				if (util_strcasecmp(cp2, fmp2->suf) == 0)
				    newfmt = fmp2->fmt;
			    }
			}
		}

		if (!cdda_filefmt_supp(newfmt)) {
			/* The requested file format is not supported */
			(void) fprintf(errfp, "Error: %s\n",
				"Support for the requested file format "
				"is not installed.\n"
			);
			return;
		}

		if (!util_newstr(&s->outf_tmpl, cp))
			CDA_FATAL(app_data.str_nomemory);

		/* File format changed */
		if (newfmt != app_data.cdda_filefmt) {
			app_data.cdda_filefmt = newfmt;
			(void) fprintf(errfp,
				"Notice: %s\n",
				"The output file format has been changed."
			);
		}

		/* Fix output file suffix to match the file type */
		cda_fix_outfile_path(s);

		if (util_strstr(s->outf_tmpl, "%T") != NULL ||
		    util_strstr(s->outf_tmpl, "%t") != NULL ||
		    util_strstr(s->outf_tmpl, "%R") != NULL ||
		    util_strstr(s->outf_tmpl, "%r") != NULL ||
		    util_strstr(s->outf_tmpl, "%#") != NULL) {
			if (!app_data.cdda_trkfile) {
				app_data.cdda_trkfile = TRUE;
				(void) fprintf(errfp,
					"Notice: %s\n",
					"File-per-track has been enabled."
				);
			}
		}
		else if (app_data.cdda_trkfile) {
			app_data.cdda_trkfile = FALSE;
			(void) fprintf(errfp,
				"Notice: %s\n",
				"File-per-track has been disabled."
			);
		}
	}
}


/*
 * cda_do_pipeprog
 *	Update the pipe-to-program path
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_pipeprog(curstat_t *s, cdapkt_t *p)
{
	char	*cp;
	size_t	maxsz = (CDA_NARGS - 1) * sizeof(word32_t);

	cp = (char *) &p->arg[1];

	if (p->arg[0] == 0) {
		/* Query */
		if (app_data.pipeprog == NULL)
			*cp = '\0';
		else {
			(void) strncpy(cp, app_data.pipeprog, maxsz - 1);
			*(cp + maxsz - 1) = '\0';
		}
	}
	else {
		/* Set */
		if (!util_newstr(&app_data.pipeprog, cp))
			CDA_FATAL(app_data.str_nomemory);
	}
}


/*
 * cda_do_compress
 *	Perform the "compress" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_compress(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.comp_mode;
		p->arg[3] = (word32_t) app_data.cdda_filefmt;
		switch (app_data.comp_mode) {
		case COMPMODE_0:
		case COMPMODE_3:
			p->arg[2] = (word32_t) app_data.bitrate;
			break;

		case COMPMODE_1:
		case COMPMODE_2:
			p->arg[2] = (word32_t) app_data.qual_factor;
			break;

		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
	else {
		/* Set */
		int	val = (int) p->arg[2];

		switch ((int) p->arg[1]) {
		case COMPMODE_0:
		case COMPMODE_3:
			if (di_bitrate_valid(val)) {
				app_data.comp_mode = (int) p->arg[1];
				app_data.bitrate = val;
			}
			else
				p->retcode = CDA_PARMERR;
			break;

		case COMPMODE_1:
		case COMPMODE_2:
			if (val <= 0 || val > 10) {
				p->retcode = CDA_PARMERR;
				break;
			}
			app_data.comp_mode = (int) p->arg[1];
			app_data.qual_factor = val;
			break;

		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_brate
 *	Perform the "min-brate" and "max-brate" commands.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_brate(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		if (p->arg[1] == 1)
			p->arg[2] = (word32_t) app_data.bitrate_min;
		else if (p->arg[1] == 2)
			p->arg[2] = (word32_t) app_data.bitrate_max;
		else
			p->retcode = CDA_PARMERR;
	}
	else {
		/* Set */
		int	val = (int) p->arg[2];

		if (!di_bitrate_valid(val))
			p->retcode = CDA_PARMERR;
		else if (p->arg[1] == 1)
			app_data.bitrate_min = val;
		else if (p->arg[1] == 2)
			app_data.bitrate_max = val;
		else
			p->retcode = CDA_PARMERR;
	}
}


/*
 * cda_do_coding
 *	Perform the "coding" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_coding(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.chan_mode;
		p->arg[2] = (word32_t) app_data.comp_algo;
	}
	else if (p->arg[0] == 1) {
		/* Set MP3 mode */
		switch ((int) p->arg[1]) {
		case CH_STEREO:
		case CH_JSTEREO:
		case CH_FORCEMS:
		case CH_MONO:
			app_data.chan_mode = (int) p->arg[1];
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
	else {
		/* Set compression algorithm */
		int	val;

		val = (int) p->arg[2];
		if (val <= 0 || val > 10)
			p->retcode = CDA_PARMERR;
		else
			app_data.comp_algo = val;
	}
}


/*
 * cda_do_filter
 *	Perform the "lowpass" and "highpass" commands.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_filter(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		if (p->arg[2] == 1) {
			p->arg[1] = (word32_t) app_data.lowpass_mode;
			p->arg[3] = (word32_t) app_data.lowpass_freq;
			p->arg[4] = (word32_t) app_data.lowpass_width;
		}
		else if (p->arg[2] == 2) {
			p->arg[1] = (word32_t) app_data.highpass_mode;
			p->arg[3] = (word32_t) app_data.highpass_freq;
			p->arg[4] = (word32_t) app_data.highpass_width;
		}
		else
			p->retcode = CDA_PARMERR;
	}
	else {
		/* Set */
		int	freq = (int) p->arg[3],
			width = (int) p->arg[4];

		switch ((int) p->arg[2]) {
		case 1:
			app_data.lowpass_mode = (int) p->arg[1];
			if (app_data.lowpass_mode != FILTER_MANUAL)
				break;

			if (freq < MIN_LOWPASS_FREQ ||
			    freq > MAX_LOWPASS_FREQ) {
				p->retcode = CDA_PARMERR;
				break;
			}

			app_data.lowpass_freq = freq;
			if (width >= 0)
				app_data.lowpass_width = width;
			break;
		case 2:
			app_data.highpass_mode = (int) p->arg[1];
			if (app_data.highpass_mode != FILTER_MANUAL)
				break;

			if (freq < MIN_HIGHPASS_FREQ ||
			    freq > MAX_HIGHPASS_FREQ) {
				p->retcode = CDA_PARMERR;
				break;
			}

			app_data.highpass_freq = freq;
			if (width >= 0)
				app_data.highpass_width = width;
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_flags
 *	Perform the "flags" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_flags(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.copyright;
		p->arg[2] = (word32_t) app_data.original;
		p->arg[3] = (word32_t) app_data.nores;
		p->arg[4] = (word32_t) app_data.checksum;
		p->arg[5] = (word32_t) app_data.strict_iso;
	}
	else {
		/* Set */
		if ((int) p->arg[1] >= 0)
			app_data.copyright = (bool_t) p->arg[1];
		if ((int) p->arg[2] >= 0)
			app_data.original = (bool_t) p->arg[2];
		if ((int) p->arg[3] >= 0)
			app_data.nores = (bool_t) p->arg[3];
		if ((int) p->arg[4] >= 0)
			app_data.checksum = (bool_t) p->arg[4];
		if ((int) p->arg[5] >= 0)
			app_data.strict_iso = (bool_t) p->arg[5];
	}
}


/*
 * cda_do_lameopts
 *	Perform the "lameopts" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_lameopts(curstat_t *s, cdapkt_t *p)
{
	char	*cp;
	size_t	maxsz = (CDA_NARGS - 2) * sizeof(word32_t);

	cp = (char *) &p->arg[2];

	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.lameopts_mode;
		if (app_data.lame_opts == NULL)
			*cp = '\0';
		else {
			(void) strncpy(cp, app_data.lame_opts, maxsz - 1);
			*(cp + maxsz - 1) = '\0';
		}
	}
	else {
		/* Set */
		switch ((int) p->arg[1]) {
		case LAMEOPTS_DISABLE:
		case LAMEOPTS_INSERT:
		case LAMEOPTS_APPEND:
		case LAMEOPTS_REPLACE:
			app_data.lameopts_mode = (int) p->arg[1];

			if (strcmp(cp, ".") != 0 &&
			    !util_newstr(&app_data.lame_opts, cp)) {
				CDA_FATAL(app_data.str_nomemory);
			}
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_tag
 *	Perform the "tag" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_tag(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		if (app_data.add_tag)
			p->arg[1] = (word32_t) app_data.id3tag_mode;
		else
			p->arg[1] = 0;
	}
	else {
		/* Set */
		switch ((int) p->arg[1]) {
		case 0:
			app_data.add_tag = FALSE;
			break;
		case ID3TAG_V1:
		case ID3TAG_V2:
		case ID3TAG_BOTH:
			app_data.add_tag = TRUE;
			app_data.id3tag_mode = (int) p->arg[1];
			break;
		default:
			p->retcode = CDA_PARMERR;
			break;
		}
	}
}


/*
 * cda_do_authuser
 *	Pass the proxy authorization username from client to daemon.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_authuser(curstat_t *s, cdapkt_t *p)
{
	if (strncmp((char *) p->arg, "user", 4) != 0) {
		p->retcode = CDA_PARMERR;
		return;
	}

	if (!util_newstr(&dbp->proxy_user, (char *) &p->arg[1]))
		CDA_FATAL(app_data.str_nomemory);
}


/*
 * cda_do_authpass
 *	Pass the proxy authorization password from client to daemon.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_authpass(curstat_t *s, cdapkt_t *p)
{
	if (strncmp((char *) p->arg, "pass", 4) != 0) {
		p->retcode = CDA_PARMERR;
		return;
	}

	if (!util_newstr(&dbp->proxy_passwd, (char *) &p->arg[1]))
		CDA_FATAL(app_data.str_nomemory);
}


/*
 * cda_do_motd
 *	Perform the "motd" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_motd(curstat_t *s, cdapkt_t *p)
{
	(void) printf("Retrieving messages...\n");
	(void) fflush(stdout);
	motd_get((byte_t *) 1);
}


/*
 * cda_do_device
 *	Perform the "device" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_do_device(curstat_t *s, cdapkt_t *p)
{
	(void) sprintf((char *) p->arg, "Drive: %s %s (%s)\n\n%s",
		    s->vendor, s->prod, s->revnum, di_methodstr());
}


/*
 * cda_do_version
 *	Perform the "version" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_version(curstat_t *s, cdapkt_t *p)
{
	(void) sprintf((char *) p->arg, "%s.%s.%s",
		    VERSION_MAJ, VERSION_MIN, VERSION_TEENY);
}


/*
 * cda_do_debug
 *	Perform the "debug" command.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_do_debug(curstat_t *s, cdapkt_t *p)
{
	if (p->arg[0] == 0) {
		/* Query */
		p->arg[1] = (word32_t) app_data.debug;
	}
	else {
		/* Set level */
		app_data.debug = (word32_t) p->arg[1];

		/* Debug level change notification */
		di_debug();
	}
}


/*
 * prn_program
 *	Print current program sequence, if any.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_program(word32_t arg[])
{
	int	i;

	if ((int) arg[0] > 0) {
		(void) printf("Current program:");
		for (i = 0; i < arg[0]; i++)
			(void) printf(" %d", (int) arg[i+2]);
		(void) printf("\n");

		if ((int) arg[1] > 0) {
			(void) printf("Playing:        ");
			for (i = 0; i < (int) (arg[1] - 1); i++) {
				(void) printf("  %s",
					((int) arg[i+2] > 9) ? " " : "");
			}
			(void) printf(" %s^\n",
				((int) arg[i+2] > 9) ? " " : "");
		}
	}
	else if ((int) arg[0] == 0) {
		if ((int) arg[2] == -1)
			(void) printf("No program sequence defined.\n");
		else if ((int) arg[2] == -2)
			(void) printf("Program sequence saved.\n");
		else if ((int) arg[2] == -3)
			(void) printf("Program sequence cleared.\n");
	}
}


/*
 * prn_volume
 *	Print current volume setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_volume(word32_t arg[])
{
	char	*tprstr;

	if ((int) arg[0] == 0) {
		switch ((int) arg[2]) {
		case VOLTAPER_LINEAR:
			tprstr = "linear";
			break;
		case VOLTAPER_SQR:
			tprstr = "square";
			break;
		case VOLTAPER_INVSQR:
			tprstr = "invsqr";
			break;
		default:
			tprstr = "unknown";
			break;
		}
		(void) printf("Current volume: %d (range 0-100) %s taper\n",
			      (int) arg[1], tprstr);
	}
}


/*
 * prn_balance
 *	Print current balance setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_balance(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Current balance: %d (range 0-100, center:50)\n",
			      (int) arg[1]);
	}
}


/*
 * prn_route
 *	Print current channel routing setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_route(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Current routing: %d ", (int) arg[1]);

		switch ((int) arg[1]) {
		case CHROUTE_NORMAL:
			(void) printf("(normal stereo)\n");
			break;
		case CHROUTE_REVERSE:
			(void) printf("(reverse stereo)\n");
			break;
		case CHROUTE_L_MONO:
			(void) printf("(mono-L)\n");
			break;
		case CHROUTE_R_MONO:
			(void) printf("(mono-R)\n");
			break;
		case CHROUTE_MONO:
			(void) printf("(mono-L+R)\n");
			break;
		}
	}
}


/*
 * prn_outport
 *	Print current CDDA output port setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_outport(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Current CDDA output port: %d ", (int) arg[1]);

		if ((int) arg[1] != 0) {
			bool_t	sep = FALSE;

			(void) printf("(");
			if ((arg[1] & CDDA_OUT_SPEAKER) != 0) {
				(void) printf("speaker");
				sep = TRUE;
			}
			if ((arg[1] & CDDA_OUT_HEADPHONE) != 0) {
				(void) printf("%sheadphone", sep ? ", " : "");
				sep = TRUE;
			}
			if ((arg[1] & CDDA_OUT_LINEOUT) != 0) {
				(void) printf("%sline-out", sep ? ", " : "");
			}
			(void) printf(")");
		}

		(void) printf("\n");
	}
}


/*
 * prn_cdda_att
 *	Print current CDDA attenuator setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_cdda_att(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Current CDDA attenuator: %d%% gain\n",
			      (int) arg[1]);
	}
}


/*
 * prn_stat
 *	Print current CD status.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_stat(word32_t arg[])
{
	unsigned int	disc,
			trk,
			idx,
			min,
			sec;

	if (stat_cont)
		(void) printf("\r");

	switch (RD_ARG_MODE(arg[0])) {
	case MOD_BUSY:
		(void) printf("CD_Busy    disc:-  -- --  --:--");
		break;
	case MOD_NODISC:
		(void) printf("No_Disc    disc:-  -- --  --:--");
		break;
	case MOD_STOP:
		(void) printf("CD_Stopped disc:%u  -- --  --:--",
			      (unsigned int) arg[7]);
		break;
	case MOD_PLAY:
		disc = (unsigned int) arg[7];
		trk = (unsigned int) RD_ARG_TRK(arg[1]);
		idx = (unsigned int) RD_ARG_IDX(arg[1]);
		min = (unsigned int) RD_ARG_MIN(arg[1]);
		sec = (unsigned int) RD_ARG_SEC(arg[1]);

		if (idx == 0 && app_data.subq_lba) {
			/* In LBA mode the time display is not meaningful
			 * while in the lead-in, so just set to 0
			 */
			min = sec = 0;
		}

		(void) printf("CD_Playing disc:%u  %02u %02u %s%02u:%02u",
			      disc, trk, idx,
			      (idx == 0) ? "-" : "+", min, sec);
		break;
	case MOD_PAUSE:
		disc = (unsigned int) arg[7];
		trk = (unsigned int) RD_ARG_TRK(arg[1]);
		idx = (unsigned int) RD_ARG_IDX(arg[1]);
		min = (unsigned int) RD_ARG_MIN(arg[1]);
		sec = (unsigned int) RD_ARG_SEC(arg[1]);

		if (idx == 0 && app_data.subq_lba) {
			/* In LBA mode the time display is not meaningful
			 * while in the lead-in, so just set to 0
			 */
			min = sec = 0;
		}

		(void) printf("CD_Paused  disc:%u  %02u %02u %s%02u:%02u",
			      disc, trk, idx,
			      (idx == 0) ? "-" : "+", min, sec);
		break;
	default:
		(void) printf("Inv_status disc:-  -- --  --:--");
		break;
	}

	(void) printf(" %slock", RD_ARG_LOCK(arg[0]) ? "+" : "-");
	(void) printf(" %sshuf", RD_ARG_SHUF(arg[0]) ? "+" : "-");
	(void) printf(" %sprog", RD_ARG_PROG(arg[0]) ? "+" : "-");
	(void) printf(" %srept", RD_ARG_REPT(arg[0]) ? "+" : "-");

	if ((int) arg[2] >= 0)
		(void) printf(" %d", (int) arg[2]);
	else
		(void) printf(" -");

	if (!stat_cont)
		(void) printf("\n");
}


/*
 * prn_toc
 *	Print current CD Table Of Contents.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_toc(word32_t arg[])
{
	int		i;
	byte_t		ntrks,
			trkno,
			min,
			sec;
	bool_t		playing;
	curstat_t	*s = curstat_addr();
	chset_conv_t	*up;
	char		*p1,
			*p2,
			*q1,
			*q2;

	/* Convert CD info from UTF-8 to local charset if possible */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return;

	ntrks = arg[0] & 0xff;

	q1 = (dbp->disc.genre == NULL) ?
		"(unknown genre)" : cdinfo_genre_name(dbp->disc.genre);
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Genre: %s %s\n", p1 == NULL ? "" : p1,
		      (dbp->disc.notes != NULL ||
		       dbp->disc.credit_list != NULL) ? "*" : "");
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = (dbp->disc.artist == NULL) ? "" : dbp->disc.artist;
	q2 = (dbp->disc.title == NULL) ? "(unknown title)" : dbp->disc.title;
	p1 = p2 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q2, &p2, FALSE) && !util_newstr(&p2, q2)) {
		util_chset_close(up);
		return;
	}
	(void) printf("%s%s%s\n\n",
		      p1 == NULL ? "" : p1,
		      (dbp->disc.artist != NULL && dbp->disc.title != NULL) ?
			    " / " : "",
		      p2 == NULL ? "" : p2);
	if (p1 != NULL)
		MEM_FREE(p1);
	if (p2 != NULL)
		MEM_FREE(p2);

	for (i = 0; i < (int) ntrks; i++) {
		RD_ARG_TOC(arg[i+1], trkno, playing, min, sec);
		(void) printf("%s%02u %02u:%02u  ",
			      playing ? ">" : " ", trkno, min, sec);
		if (s->qmode == QMODE_MATCH && dbp->track[i].title != NULL) {
			p1 = NULL;
			if (!util_chset_conv(up, dbp->track[i].title,
					     &p1, FALSE) &&
			    !util_newstr(&p1, dbp->track[i].title)) {
				util_chset_close(up);
				return;
			}
			(void) printf("%s%s\n", p1 == NULL ? "" : p1,
				      (dbp->track[i].notes != NULL ||
				       dbp->track[i].credit_list != NULL) ?
					"*" : "");
			if (p1 != NULL)
				MEM_FREE(p1);
		}
		else {
			(void) printf("??%s\n",
				      (dbp->track[i].notes != NULL ||
				       dbp->track[i].credit_list != NULL) ?
					"*" : "");
		}
	}

	util_chset_close(up);

	RD_ARG_TOC(arg[i+1], trkno, playing, min, sec);
	(void) printf("\nTotal Time: %02u:%02u\n", min, sec);
}


/*
 * prn_extinfo
 *	Print current Disc and Track extended information.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_extinfo(word32_t arg[])
{
	curstat_t	*s = curstat_addr();
	int		i;
	cdinfo_credit_t	*p;
	chset_conv_t	*up;
	char		*p1,
			*p2,
			*p3,
			*q1,
			*q2,
			*q3;

	if (s->qmode != QMODE_MATCH) {
		(void) printf("No information was found for this CD\n");
		return;
	}

	/* Convert CD info from UTF-8 to local charset if possible */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return;

	(void) printf("------- Album Information -------\n");

	(void) printf("Xmcd disc ID: %08x\n", dbp->discid);

	q1 = (dbp->disc.artist == NULL) ? "" : dbp->disc.artist;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Artist: %s\n", p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = (dbp->disc.title == NULL) ? "" : dbp->disc.title;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Title: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = (dbp->disc.artistfname.lastname == NULL) ?
		"" : dbp->disc.artistfname.lastname;
	q2 = (dbp->disc.artistfname.firstname == NULL) ?
		"" : dbp->disc.artistfname.firstname;
	q3 = (dbp->disc.artistfname.the == NULL) ?
		"" : dbp->disc.artistfname.the;
	p1 = p2 = p3 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q2, &p2, FALSE) && !util_newstr(&p2, q2)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q3, &p3, FALSE) && !util_newstr(&p3, q3)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Artist full name: %s%s%s%s%s\n",
		      p1 == NULL ? "" : p1,
		      dbp->disc.artistfname.lastname == NULL ? "" : ", ",
		      p2 == NULL ? "" : p2,
		      dbp->disc.artistfname.the == NULL ? "" : ", ",
		      p3 == NULL ? "" : p3);
	if (p1 != NULL)
		MEM_FREE(p1);
	if (p2 != NULL)
		MEM_FREE(p2);
	if (p3 != NULL)
		MEM_FREE(p3);

	q1 = (dbp->disc.sorttitle == NULL) ? "" : dbp->disc.sorttitle;
	q2 = (dbp->disc.title_the == NULL) ? "" : dbp->disc.title_the;
	p1 = p2 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q2, &p2, FALSE) && !util_newstr(&p2, q2)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Sort title: %s%s%s\n",
		      p1 == NULL ? "" : p1,
		      dbp->disc.title_the == NULL ? "" : ", ",
		      p2 == NULL ? "" : p2);
	if (p1 != NULL)
		MEM_FREE(p1);
	if (p2 != NULL)
		MEM_FREE(p2);

	q1 = (dbp->disc.year == NULL) ? "" : dbp->disc.year;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Year: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = (dbp->disc.label == NULL) ? "" : dbp->disc.label;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Record label: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	(void) printf("Compilation: %s\n",
		      dbp->disc.compilation ? "Yes" : "No");

	q1 = cdinfo_genre_name(dbp->disc.genre);
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Genre 1: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = cdinfo_genre_name(dbp->disc.genre2);
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Genre 2: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = dbp->disc.dnum == NULL ? "?" : dbp->disc.dnum;
	q2 = dbp->disc.tnum == NULL ? "?" : dbp->disc.tnum;
	p1 = p2 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q2, &p2, FALSE) && !util_newstr(&p2, q2)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Disc %s of %s\n",
		      p1 == NULL ? "" : p1,
		      p2 == NULL ? "" : p2);
	if (p1 != NULL)
		MEM_FREE(p1);
	if (p2 != NULL)
		MEM_FREE(p2);

	(void) printf("Credits:\n");
	for (p = dbp->disc.credit_list; p != NULL; p = p->next) {
		q1 = p->crinfo.name == NULL ? "unknown" : p->crinfo.name;
		q2 = p->crinfo.role == NULL ?
			"unknown" : cdinfo_role_name(p->crinfo.role->id);
		p1 = p2 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return;
		}
		if (!util_chset_conv(up, q2, &p2, FALSE) &&
		    !util_newstr(&p2, q2)) {
			util_chset_close(up);
			return;
		}
		(void) printf("\t%s (%s)\n",
			      p1 == NULL ? "" : p1,
			      p2 == NULL ? "" : p2);
		if (p1 != NULL)
			MEM_FREE(p1);
		if (p2 != NULL)
			MEM_FREE(p2);
	}

	q1 = dbp->disc.region == NULL ?
		"" : cdinfo_region_name(dbp->disc.region);
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Region: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = dbp->disc.lang == NULL ? "" : cdinfo_lang_name(dbp->disc.lang);
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Language: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = dbp->disc.revision == NULL ? "" : dbp->disc.revision;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Revision: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = dbp->disc.certifier == NULL ? "" : dbp->disc.certifier;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Certifier: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	if ((int) arg[1] < 0) {
		util_chset_close(up);
		return;
	}

	(void) printf("\n------ Track %02d Information ------\n",
		      (int) arg[1]);

	i = (int) arg[2];

	q1 = dbp->track[i].title == NULL ? "" : dbp->track[i].title;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Title: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = dbp->track[i].sorttitle == NULL ? "" : dbp->track[i].sorttitle;
	q2 = dbp->track[i].title_the == NULL ? "" : dbp->track[i].title_the;
	p1 = p2 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q2, &p2, FALSE) && !util_newstr(&p2, q2)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Sort Title: %s%s%s\n",
		      p1 == NULL ? "" : p1,
		      dbp->track[i].title_the == NULL ? "" : ", ",
		      p2 == NULL ? "" : p2);
	if (p1 != NULL)
		MEM_FREE(p1);
	if (p2 != NULL)
		MEM_FREE(p2);

	q1 = dbp->track[i].artist == NULL ? "" : dbp->track[i].artist;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Artist: %s\n", p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = (dbp->track[i].artistfname.lastname == NULL) ?
		"" : dbp->track[i].artistfname.lastname;
	q2 = (dbp->track[i].artistfname.firstname == NULL) ?
		"" : dbp->track[i].artistfname.firstname;
	q3 = (dbp->track[i].artistfname.the == NULL) ?
		"" : dbp->track[i].artistfname.the;
	p1 = p2 = p3 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q2, &p2, FALSE) && !util_newstr(&p2, q2)) {
		util_chset_close(up);
		return;
	}
	if (!util_chset_conv(up, q3, &p3, FALSE) && !util_newstr(&p3, q3)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Artist full name: %s%s%s%s%s\n",
		      p1 == NULL ? "" : p1,
		      dbp->track[i].artistfname.lastname == NULL ? "" : ", ",
		      p2 == NULL ? "" : p2,
		      dbp->track[i].artistfname.the == NULL ? "" : ", ",
		      p3 == NULL ? "" : p3);
	if (p1 != NULL)
		MEM_FREE(p1);
	if (p2 != NULL)
		MEM_FREE(p2);
	if (p3 != NULL)
		MEM_FREE(p3);

	q1 = dbp->track[i].year == NULL ? "" : dbp->track[i].year;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Year: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = dbp->track[i].label == NULL ? "" : dbp->track[i].label;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Record Label: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = dbp->track[i].bpm == NULL ? "" : dbp->track[i].bpm;
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("BPM: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = cdinfo_genre_name(dbp->track[i].genre);
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Genre 1: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	q1 = cdinfo_genre_name(dbp->track[i].genre2);
	p1 = NULL;
	if (!util_chset_conv(up, q1, &p1, FALSE) && !util_newstr(&p1, q1)) {
		util_chset_close(up);
		return;
	}
	(void) printf("Genre 2: %s\n", p1 == NULL ? "" : p1);
	if (p1 != NULL)
		MEM_FREE(p1);

	(void) printf("Credits:\n");
	for (p = dbp->track[i].credit_list; p != NULL; p = p->next) {
		q1 = p->crinfo.name == NULL ? "unknown" : p->crinfo.name;
		q2 = p->crinfo.role == NULL ?
			"unknown" : cdinfo_role_name(p->crinfo.role->id);
		p1 = p2 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return;
		}
		if (!util_chset_conv(up, q2, &p2, FALSE) &&
		    !util_newstr(&p2, q2)) {
			util_chset_close(up);
			return;
		}
		(void) printf("\t%s (%s)\n",
			      p1 == NULL ? "" : p1,
			      p2 == NULL ? "" : p2);
		if (p1 != NULL)
			MEM_FREE(p1);
		if (p2 != NULL)
			MEM_FREE(p2);
	}

	util_chset_close(up);
}


/*
 * prn_notes
 *	Print current Disc and Track Notes.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_notes(word32_t arg[])
{
	int		i;
	curstat_t	*s = curstat_addr();
	chset_conv_t	*up;
	char		*p1,
			*q1;

	if (s->qmode != QMODE_MATCH) {
		(void) printf("No information was found for this CD\n");
		return;
	}

	/* Convert CD info from UTF-8 to local charset if possible */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return;

	(void) printf("------- Album Notes -------\n");

	if (dbp->disc.notes == NULL)
		(void) printf("(none)\n");
	else {
		q1 = (dbp->disc.title == NULL) ?
			app_data.str_unkndisc : dbp->disc.title;
		p1 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return;
		}
		(void) printf("%s%s%s\n\n",
				dbp->disc.title == NULL ? "(" : "",
				p1 == NULL ? "" : p1,
				dbp->disc.title == NULL ? ")" : "");
		if (p1 != NULL)
			MEM_FREE(p1);

		q1 = dbp->disc.notes;
		p1 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return;
		}
		(void) printf("%s\n", p1 == NULL ? "" : p1);
		if (p1 != NULL)
			MEM_FREE(p1);
	}

	if ((int) arg[1] < 0) {
		util_chset_close(up);
		return;
	}

	(void) printf("\n------ Track %02d Notes ------\n",
		      (int) arg[1]);

	i = (int) arg[2];
	if (dbp->track[i].notes == NULL)
		(void) printf("(none)\n");
	else {
		q1 = (dbp->track[i].title == NULL) ?
			app_data.str_unkndisc : dbp->track[i].title;
		p1 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return;
		}
		(void) printf("%s%s%s\n\n",
				dbp->track[i].title == NULL ? "(" : "",
				p1 == NULL ? "" : p1,
				dbp->track[i].title == NULL ? ")" : "");
		if (p1 != NULL)
			MEM_FREE(p1);

		q1 = dbp->track[i].notes;
		p1 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return;
		}
		(void) printf("%s\n", p1 == NULL ? "" : p1);
		if (p1 != NULL)
			MEM_FREE(p1);
	}

	util_chset_close(up);
}


 /*
 * prn_on_load
 *	Print on-load mode.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_on_load(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("On load: %s%s%s\n",
			((int) arg[1] == 0) ? "noautolock" : "autolock",
			((int) arg[2] == 0) ? "" : " spindown",
			((int) arg[3] == 0) ? "" : " autoplay");
	}
}


/*
 * prn_on_exit
 *	Print on-exit mode.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_on_exit(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("On exit: %s\n",
			((int) arg[1] == 0 && (int) arg[2] == 0) ? "none" : 
			((int) arg[1] == 0) ? "autoeject" : "autostop");
	}
}


/*
 * prn_on_done
 *	Print on-done mode.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_on_done(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("On done:%s%s%s\n",
			((int) arg[1] == 0 && (int) arg[2] == 0) ?
				" none" : "",
			((int) arg[1] == 0) ? "" : " autoeject",
			((int) arg[2] == 0) ? "" : " autoexit");
	}
}


/*
 * prn_on_eject
 *	Print on-eject mode.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_on_eject(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("On eject: %s\n",
			((int) arg[1] == 0) ? "none" : "autoexit");
	}
}


/*
 * prn_chgr
 *	Print changer mode.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_chgr(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Changer:%s%s%s\n",
			((int) arg[1] == 0 && (int) arg[2] == 0) ?
				" none" : "",
			((int) arg[1] == 0) ? "" : " multiplay",
			((int) arg[2] == 0) ? "" : " reverse");
	}
}


/*
 * prn_mode
 *	Print playback mode.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_mode(word32_t arg[])
{
	char	modestr[STR_BUF_SZ];

	if (PLAYMODE_IS_STD((byte_t) arg[1])) {
		(void) strcat(modestr, "standard");
	}
	else {
		(void) strcpy(modestr, "CDDA ");
		if ((arg[1] & PLAYMODE_CDDA) != 0) {
			(void) strcat(modestr, "cdda-play");
		}
		if ((arg[1] & PLAYMODE_FILE) != 0) {
			if ((arg[1] & PLAYMODE_CDDA) != 0)
				(void) strcat(modestr, "/cdda-save");
			else
				(void) strcat(modestr, "cdda-save");
		}
		if ((arg[1] & PLAYMODE_PIPE) != 0) {
			if ((arg[1] & PLAYMODE_CDDA) != 0 ||
			    (arg[1] & PLAYMODE_FILE) != 0)
				(void) strcat(modestr, "/cdda-pipe");
			else
				(void) strcat(modestr, "cdda-pipe");
		}
	}
	(void) printf("Playback mode: %s\n", modestr);
}


/*
 * prn_jitter
 *	Print jitter correction setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_jitter(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Jitter correction is %s\n",
			((int) arg[1]) ? "on" : "off");
	}
}


/*
 * prn_trkfile
 *	Print trackfile setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_trkfile(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Trackfile is %s\n",
			((int) arg[1]) ? "on" : "off");
	}
}


/*
 * prn_subst
 *	Print subst setting.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_subst(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("Underscore substitution is %s\n",
			((int) arg[1]) ? "on" : "off");
	}
}


/*
 * prn_filefmt
 *	Print output file format.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_filefmt(word32_t arg[])
{
	filefmt_t	*fmp;

	if ((int) arg[0] == 0) {
		if ((fmp = cdda_filefmt((int) arg[1])) != NULL)
			(void) printf("Output file format: %s (%s)\n",
				      fmp->desc, fmp->suf);
		else
			(void) printf("Unknown file format.\n");
	}
}


/*
 * prn_file
 *	Print output file.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_file(word32_t arg[])
{
	char	*cp;

	if ((int) arg[0] == 0) {
		cp = (char *) &arg[1];
		if (*cp == '\0')
			(void) printf("No output file template defined.\n");
		else
			(void) printf("Output file template: %s\n", cp);
	}
}


/*
 * prn_pipeprog
 *	Print pipe-to-program path.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_pipeprog(word32_t arg[])
{
	char	*cp;

	if ((int) arg[0] == 0) {
		cp = (char *) &arg[1];
		if (*cp == '\0')
			(void) printf("No pipe-to program defined.\n");
		else
			(void) printf("Pipe-to program: %s\n", cp);
	}
}


/*
 * prn_compress
 *	Print MP3/OggVorbis compression settings.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_compress(word32_t arg[])
{
	if ((int) arg[0] != 0)
		return;

	switch ((int) arg[3]) {
	case FILEFMT_MP3:
		switch ((int) arg[1]) {
		case COMPMODE_0:
			(void) printf("CBR, bitrate %d kb/s\n",
					(int) arg[2]);
			break;
		case COMPMODE_1:
			(void) printf("VBR, quality %d\n", (int) arg[2]);
			break;
		case COMPMODE_2:
			(void) printf("VBR-2, quality %d\n", (int) arg[2]);
			break;
		case COMPMODE_3:
			(void) printf("ABR, average bitrate %d kb/s\n",
					(int) arg[2]);
			break;
		default:
			break;
		}
		break;

	case FILEFMT_OGG:
	case FILEFMT_MP4:
		switch ((int) arg[1]) {
		case COMPMODE_0:
		case COMPMODE_3:
			(void) printf("VBR, average bitrate %d kb/s\n",
					(int) arg[2]);
			break;
		case COMPMODE_1:
		case COMPMODE_2:
			(void) printf("VBR, quality %d\n", (int) arg[2]);
			break;
		default:
			break;
		}
		break;

	case FILEFMT_FLAC:
		/* TIKAN - not yet implemented */
		break;

	case FILEFMT_AAC:
		switch ((int) arg[1]) {
		case COMPMODE_0:
			(void) printf("MPEG-2, VBR, average bitrate %d kb/s\n",
					(int) arg[2]);
			break;
		case COMPMODE_1:
			(void) printf("MPEG-2, VBR, quality %d\n",
					(int) arg[2]);
			break;
		case COMPMODE_2:
			(void) printf("MPEG-4, VBR, quality %d\n",
					(int) arg[2]);
			break;
		case COMPMODE_3:
			(void) printf("MPEG-4, VBR, average bitrate %d kb/s\n",
					(int) arg[2]);
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}


/*
 * prn_brate
 *	Print min and max bitrate settings.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_brate(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("%s bitrate: %d kb/s\n",
			    (arg[1] == 1) ? "Minimum" : "Maximum",
			    (int) arg[2]);
	}
}


/*
 * prn_coding
 *	Print coding mode and algorithm settings.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_coding(word32_t arg[])
{
	char	*modestr;

	if ((int) arg[0] == 0) {
		switch ((int) arg[1]) {
		case CH_STEREO:
			modestr = "stereo";
			break;
		case CH_JSTEREO:
			modestr = "j-stereo";
			break;
		case CH_FORCEMS:
			modestr = "force-ms";
			break;
		case CH_MONO:
			modestr = "mono";
			break;
		default:
			modestr = "unknown";
			break;
		}

		(void) printf("%s mode, algorithm %d\n",
				modestr, (int) arg[2]);
	}
}


/*
 * prn_filter
 *	Print lowpass and highpass filter settings.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_filter(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		char *fstr = (arg[2] == 1) ? "Lowpass" : "Highpass";

		switch ((int) arg[1]) {
		case FILTER_OFF:
			(void) printf("%s filter: off\n", fstr);
			break;
		case FILTER_AUTO:
			(void) printf("%s filter: auto\n", fstr);
			break;
		case FILTER_MANUAL:
			(void) printf("%s filter: freq %d Hz, width %d Hz\n",
				    fstr, (int) arg[3], (int) arg[4]);
			break;
		default:
			break;
		}
	}
}


/*
 * prn_flags
 *	Print flags settings.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_flags(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("%s\t%d\n%s\t%d\n%s\t%d\n%s\t%d\n%s\t%d\n",
			    "Copyright:", arg[1],
			    "Original:", arg[2],
			    "No res:", arg[3],
			    "Checksum:", arg[4],
			    "Strict-ISO:", arg[5]);
	}
}


/*
 * prn_lameopts
 *	Print direct LAME command line options.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_lameopts(word32_t arg[])
{
	char	*cp,
		*str1,
		*str2;

	if ((int) arg[0] == 0) {
		cp = (char *) &arg[2];
		(void) printf("Direct LAME command line options:%s%s\n",
			      (cp[0] == '\0') ? " " : "\n",
			      (cp[0] == '\0') ? "(none)" : cp);

		str2 = " the standard settings";

		switch ((int) arg[1]) {
		case LAMEOPTS_INSERT:
			str1 = "inserted before";
			break;
		case LAMEOPTS_APPEND:
			str1 = "appended after";
			break;
		case LAMEOPTS_REPLACE:
			str1 = "used instead of";
			break;
		case LAMEOPTS_DISABLE:
		default:
			str1 = "currently disabled";
			str2 = "";
			break;
		}

		(void) printf("These options are %s%s.\n", str1, str2);
	}
}


/*
 * prn_tag
 *	Print ID3 tag/comment tag settings.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_tag(word32_t arg[])
{
	if ((int) arg[0] == 0) {
		(void) printf("CD info tag: ");

		switch ((int) arg[1]) {
		case 0:
			(void) printf("disabled.\n");
			break;
		case ID3TAG_V1:
			(void) printf("enabled, v1 only\n");
			break;
		case ID3TAG_V2:
			(void) printf("enabled, v2 only\n");
			break;
		case ID3TAG_BOTH:
			(void) printf("enabled, v1 and v2\n");
			break;
		default:
			(void) printf("invalid setting\n");
			break;
		}
	}
}


/*
 * prn_device
 *	Print device information.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_device(word32_t arg[])
{
	(void) printf("Device: %s\n", app_data.device);
	(void) printf("%s\n", (char *) arg);
}


/*
 * prn_version
 *	Print version number and other information.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_version(word32_t arg[])
{
	char	*ctrlver;

	ctrlver = cdinfo_cddbctrl_ver();
	(void) printf("CDA - Command Line CD Audio Player/Ripper\n\n");
	(void) printf("CD audio        %s.%s.%s\n",
		      VERSION_MAJ, VERSION_MIN, VERSION_TEENY);
	(void) printf("CD audio daemon %s\n", (char *) arg);
	(void) printf("\n%s\nURL: %s\nE-mail: %s\n\n%s\n\n",
		      COPYRIGHT, XMCD_URL, EMAIL, GNU_BANNER);
	(void) printf("CDDB%s service%s%s\n%s\n",
		(cdinfo_cddb_ver() == 2) ? "\262" : " \"classic\"",
		(ctrlver[0] == '\0') ? "" : ": ",
		(ctrlver[0] == '\0') ? "\n" : ctrlver,
		CDDB_BANNER);
}


/*
 * prn_debug
 *	Print debug mode.
 *
 * Args:
 *	arg - Argument array from CD audio daemon response packet.
 *
 * Return:
 *	Nothing.
 */
STATIC void
prn_debug(word32_t arg[])
{
	(void) printf("Debug level is %s%u\n",
		((int) arg[0] == 0) ? "" : "now ", arg[1]);
}


/* Service function mapping table */
struct {
	word32_t	cmd;
	void		(*srvfunc)(curstat_t *, cdapkt_t *);
	void		(*prnfunc)(word32_t *);
} cmd_fmtab[] = {
	{ CDA_ON,		cda_do_onoff,		NULL		},
	{ CDA_OFF,		cda_do_onoff,		NULL		},
	{ CDA_DISC,		cda_do_disc,		NULL		},
	{ CDA_LOCK,		cda_do_lock,		NULL		},
	{ CDA_PLAY,		cda_do_play,		NULL		},
	{ CDA_PAUSE,		cda_do_pause,		NULL		},
	{ CDA_STOP,		cda_do_stop,		NULL		},
	{ CDA_TRACK,		cda_do_track,		NULL		},
	{ CDA_INDEX,		cda_do_index,		NULL		},
	{ CDA_PROGRAM,		cda_do_program,		prn_program	},
	{ CDA_SHUFFLE,		cda_do_shuffle,		NULL		},
	{ CDA_REPEAT,		cda_do_repeat,		NULL		},
	{ CDA_VOLUME,		cda_do_volume,		prn_volume	},
	{ CDA_BALANCE,		cda_do_balance,		prn_balance	},
	{ CDA_ROUTE,		cda_do_route,		prn_route	},
	{ CDA_OUTPORT,		cda_do_outport,		prn_outport	},
	{ CDA_CDDA_ATT,		cda_do_cdda_att,	prn_cdda_att	},
	{ CDA_STATUS,		cda_do_status,		NULL		},
	{ CDA_TOC,		cda_do_toc,		prn_toc		},
	{ CDA_TOC2,		cda_do_toc2,		NULL		},
	{ CDA_EXTINFO,		cda_do_extinfo,		prn_extinfo	},
	{ CDA_ON_LOAD,		cda_do_on_load,		prn_on_load	},
	{ CDA_ON_EXIT,		cda_do_on_exit,		prn_on_exit	},
	{ CDA_ON_DONE,		cda_do_on_done,		prn_on_done	},
	{ CDA_ON_EJECT,		cda_do_on_eject,	prn_on_eject	},
	{ CDA_CHGR,		cda_do_chgr,		prn_chgr	},
	{ CDA_DEVICE,		cda_do_device,		prn_device	},
	{ CDA_VERSION,		cda_do_version,		prn_version	},
	{ CDA_DEBUG,		cda_do_debug,		prn_debug	},
	{ CDA_NOTES,		cda_do_extinfo,		prn_notes	},
	{ CDA_CDDBREG,		NULL,			NULL		},
	{ CDA_CDDBHINT,		NULL,			NULL		},
	{ CDA_MODE,		cda_do_mode,		prn_mode	},
	{ CDA_JITTER,		cda_do_jitter,		prn_jitter	},
	{ CDA_TRKFILE,		cda_do_trkfile,		prn_trkfile	},
	{ CDA_SUBST,		cda_do_subst,		prn_subst	},
	{ CDA_FILEFMT,		cda_do_filefmt,		prn_filefmt	},
	{ CDA_FILE,		cda_do_file,		prn_file	},
	{ CDA_PIPEPROG,		cda_do_pipeprog,	prn_pipeprog	},
	{ CDA_COMPRESS,		cda_do_compress,	prn_compress	},
	{ CDA_BRATE,		cda_do_brate,		prn_brate	},
	{ CDA_CODING,		cda_do_coding,		prn_coding	},
	{ CDA_FILTER,		cda_do_filter,		prn_filter	},
	{ CDA_FLAGS,		cda_do_flags,		prn_flags	},
	{ CDA_LAMEOPTS,		cda_do_lameopts,	prn_lameopts	},
	{ CDA_TAG,		cda_do_tag,		prn_tag		},
	{ CDA_AUTHUSER,		cda_do_authuser,	NULL		},
	{ CDA_AUTHPASS,		cda_do_authpass,	NULL		},
	{ CDA_MOTD,		cda_do_motd,		NULL		},
	{ 0,			NULL,			NULL		}
};


/*
 * cda_docmd
 *	Perform the command received.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	p - Pointer to the CDA packet structure.
 *
 * Return:
 *	TRUE - Received a CDA_OFF command: daemon should shut down
 *	FALSE - Received normal command.
 */
STATIC bool_t
cda_docmd(curstat_t *s, cdapkt_t *p)
{
	int		i;
	static time_t	cur_time,
			prev_time;

	/* Default status */
	p->retcode = CDA_OK;

	/* Update CD status */
	if (p->cmd != CDA_OFF) {
		if (s->mode == MOD_NODISC || s->mode == MOD_BUSY ||
		    (s->mode == MOD_STOP && app_data.load_play))
			(void) di_check_disc(s);
		else if (s->mode != MOD_STOP && s->mode != MOD_PAUSE) {
			prev_time = cur_time;
			cur_time = time(NULL);

			if (cur_time != prev_time)
				di_status_upd(s);
		}
	}

	p->retcode = CDA_INVALID;

	/* Map the command to its service function and call it */
	for (i = 0; cmd_fmtab[i].cmd != 0; i++) {
		if (p->cmd == cmd_fmtab[i].cmd) {
			p->retcode = CDA_OK;
			(*cmd_fmtab[i].srvfunc)(s, p);
			break;
		}
	}

	return (p->cmd == CDA_OFF);
}


/*
 * usage
 *	Display command line usage syntax
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
usage(void)
{
	(void) fprintf(errfp,
		"%s %s.%s.%s Command-line CD player/ripper\n%s\n%s\n\n",
		PROGNAME, VERSION_MAJ, VERSION_MIN, VERSION_TEENY,
		COPYRIGHT, XMCD_URL
	);

	(void) fprintf(errfp, "Usage: %s %s\n",
		PROGNAME,
		"[-dev device] [-batch] [-debug n#]\n"
		"           [-online | -offline] command"
	);

	(void) fprintf(errfp,
	    "\nValid commands are:\n"
	    "\ton\n"
	    "\toff\n"
	    "\tdisc <load | eject | next | prev | disc#>\n"
	    "\tlock <on | off>\n"
	    "\tplay [track# [min:sec]]\n"
	    "\tpause\n"
	    "\tstop\n"
	    "\ttrack <prev | next>\n"
	    "\tindex <prev | next>\n"
	    "\tprogram [clear | save | track# ...]\n"
	    "\tshuffle <on | off>\n"
	    "\trepeat <on | off>\n"
	    "\tvolume [value# | linear | square | invsqr]   (value 0-100)\n"
	    "\tbalance [value#]   (value 0-100, center:50)\n"
	    "\troute [stereo | reverse | mono-l | mono-r | mono | value#]\n"
	    "\toutport [speaker | headphone | line-out | value#]\n"
	    "\tcdda-att [value#]  (value 0-100)\n"
	    "\tstatus [cont [secs#]]\n"
	    "\ttoc [offsets]\n"
	    "\textinfo [track#]\n"
	    "\tnotes [track#]\n"
	    "\ton-load [none | spindown | autoplay | autolock | noautolock]\n"
	    "\ton-exit [none | autostop | autoeject]\n"
	    "\ton-done [autoeject | noautoeject | autoexit | noautoexit]\n"
	    "\ton-eject [autoexit | noautoexit]\n"
	    "\tchanger [multiplay | nomultiplay | reverse | noreverse]\n"
	    "\tmode [standard | cdda-play | cdda-save | cdda-pipe]\n"
	    "\tjittercorr [on | off]\n"
	    "\ttrackfile [on | off]\n"
	    "\tsubst [on | off]\n"
	    "\tfilefmt [raw | au | wav | aiff | aiff-c | "
			"mp3 | ogg | flac | aac | mp4]\n"
	    "\toutfile [\"template\"]\n"
	    "\tpipeprog [\"path [arg ...]\"]\n"
	    "\tcompress [<0 | 3> [bitrate#] | <1 | 2> [qual#]]\n"
	    "\tmin-brate [bitrate#]\n"
	    "\tmax-brate [bitrate#]\n"
	    "\tcoding [stereo | j-stereo | force-ms | mono | algo#]\n"
	    "\tlowpass [off | auto | freq# [width#]]\n"
	    "\thighpass [off | auto | freq# [width#]]\n"
	    "\tflags [C|c][O|o][N|n][E|e][I|i]\n"
	    "\tlameopts [<disable | insert | append | replace> "
			"[\"options\"]]\n"
	    "\ttag [off | v1 | v2 | both]\n"
	    "\tmotd\n"
	    "\tdevice\n"
	    "\tversion\n"
	    "\tcddbreg\n"
	    "\tcddbhint\n"
	    "\tdebug [level#]\n"
	    "\tvisual\n"
	);
}


/*
 * parse_time
 *	Parse a string of the form "min:sec" and convert to integer
 *	minute and second values.
 *
 * Args:
 *	str - Pointer to the "min:sec" string.
 *	min - pointer to where the minute value is to be written.
 *	sec - pointer to where the second value is to be written.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
parse_time(char *str, int *min, int *sec)
{
	char	*p;

	if ((p = strchr(str, ':')) == NULL)
		return FALSE;
	
	if (!isdigit((int) *str) || !isdigit((int) *(p+1)))
		return FALSE;

	*p = '\0';
	*min = atoi(str);
	*sec = atoi(p+1);
	*p = ':';

	return TRUE;
}


/*
 * cda_parse_args
 *	Parse CDA command line arguments.
 *
 * Args:
 *	argc, argv
 *	cmd - Pointer to the command code.
 *	arg - Command argument array.
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cda_parse_args(int argc, char **argv, word32_t *cmd, word32_t arg[])
{
	int	i,
		j,
		min,
		sec;

	/* Default values */
	*cmd = 0;
	(void) memset(arg, 0, CDA_NARGS * sizeof(word32_t));

	/* Command line args handling */
	for (i = 1; i < argc; i++) {
		if (*cmd != 0) {
			/* Multiple commands specified */
			usage();
			return FALSE;
		}

		if (strcmp(argv[i], "-dev") == 0) {
			if (++i < argc) {
				if (!di_isdemo())
					app_data.device = argv[i];
			}
			else {
				usage();
				return FALSE;
			}
		}
		else if (strcmp(argv[i], "-debug") == 0) {
			if (++i < argc && isdigit((int) argv[i][0]))
				app_data.debug = (word32_t) atoi(argv[i]);
			else {
				usage();
				return FALSE;
			}
		}
		else if (strcmp(argv[i], "-batch") == 0) {
			batch = TRUE;
		}
		else if (strcmp(argv[i], "-online") == 0) {
			inetoffln = 1;
		}
		else if (strcmp(argv[i], "-offline") == 0) {
			inetoffln = 2;
		}
		else if (strcmp(argv[i], "on") == 0) {
			*cmd = CDA_ON;
		}
		else if (strcmp(argv[i], "off") == 0) {
			*cmd = CDA_OFF;
		}
		else if (strcmp(argv[i], "disc") == 0) {
			/* <load | eject | next | prev | disc#> */
			if (++i < argc) {
				if (strcmp(argv[i], "load") == 0)
					arg[0] = 0;
				else if (strcmp(argv[i], "eject") == 0)
					arg[0] = 1;
				else if (strcmp(argv[i], "next") == 0)
					arg[0] = 2;
				else if (strcmp(argv[i], "prev") == 0)
					arg[0] = 3;
				else if (isdigit((int) argv[i][0])) {
					arg[0] = 4;
					arg[1] = atoi(argv[i]);
				}
				else {
					/* Wrong arg */
					usage();
					return FALSE;
				}
			}
			else {
				/* Missing arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_DISC;
		}
		else if (strcmp(argv[i], "lock") == 0) {
			/* <on | off> */
			if (++i < argc) {
				if (strcmp(argv[i], "off") == 0)
					arg[0] = 0;
				else if (strcmp(argv[i], "on") == 0)
					arg[0] = 1;
				else {
					/* Wrong arg */
					usage();
					return FALSE;
				}
			}
			else {
				/* Missing arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_LOCK;
		}
		else if (strcmp(argv[i], "play") == 0) {
			/* [track# [min:sec]] */
			if ((i+1) < argc && isdigit((int) argv[i+1][0])) {
				/* The user specified the track number */
				if ((arg[0] = atoi(argv[++i])) == 0) {
					/* Wrong arg */
					usage();
					return FALSE;
				}

				if ((i+1) < argc &&
				    parse_time(argv[i+1], &min, &sec)) {
					/* The user specified a time offset */
					arg[1] = min;
					arg[2] = sec;
					i++;
				}
				else {
					arg[1] = arg[2] = (word32_t) -1;
				}
			}
			*cmd = CDA_PLAY;
		}
		else if (strcmp(argv[i], "pause") == 0) {
			*cmd = CDA_PAUSE;
		}
		else if (strcmp(argv[i], "stop") == 0) {
			*cmd = CDA_STOP;
		}
		else if (strcmp(argv[i], "track") == 0) {
			/* <prev | next> */
			if (++i < argc) {
				if (strcmp(argv[i], "prev") == 0)
					arg[0] = 0;
				else if (strcmp(argv[i], "next") == 0)
					arg[0] = 1;
				else {
					/* Wrong arg */
					usage();
					return FALSE;
				}
			}
			else {
				/* Missing arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_TRACK;
		}
		else if (strcmp(argv[i], "index") == 0) {
			/* <prev | next> */
			if (++i < argc) {
				if (strcmp(argv[i], "prev") == 0)
					arg[0] = 0;
				else if (strcmp(argv[i], "next") == 0)
					arg[0] = 1;
				else {
					/* Wrong arg */
					usage();
					return FALSE;
				}
			}
			else {
				/* Missing arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_INDEX;
		}
		else if (strcmp(argv[i], "program") == 0) {
			/* [clear | save | track# ...] */
			arg[0] = 1;

			if ((i+1) < argc) {
				if (strcmp(argv[i+1], "clear") == 0) {
					i++;
					arg[0] = 0;
				}
				else if (strcmp(argv[i+1], "save") == 0) {
					i++;
					arg[0] = 2;
				}
				else {
					j = 0;
					while ((i+1) < argc &&
					       isdigit((int) argv[i+1][0]) &&
					       j < (CDA_NARGS-2)) {
						arg[++j] = atoi(argv[++i]);
					}
					if (j > 0)
						arg[0] = (word32_t) -j;
				}
			}
			*cmd = CDA_PROGRAM;
		}
		else if (strcmp(argv[i], "shuffle") == 0) {
			/* <on | off> */
			if (++i < argc) {
				if (strcmp(argv[i], "off") == 0)
					arg[0] = 0;
				else if (strcmp(argv[i], "on") == 0)
					arg[0] = 1;
				else {
					/* Wrong arg */
					usage();
					return FALSE;
				}
			}
			else {
				/* Missing arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_SHUFFLE;
		}
		else if (strcmp(argv[i], "repeat") == 0) {
			/* <on | off> */
			if (++i < argc) {
				if (strcmp(argv[i], "off") == 0)
					arg[0] = 0;
				else if (strcmp(argv[i], "on") == 0)
					arg[0] = 1;
				else {
					/* Wrong arg */
					usage();
					return FALSE;
				}
			}
			else {
				/* Missing arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_REPEAT;
		}
		else if (strcmp(argv[i], "volume") == 0) {
			/* [value# | linear | square | invsqr] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "linear") == 0) {
				arg[0] = 2;
				arg[2] = VOLTAPER_LINEAR;
				i++;
			}
			else if (strcmp(argv[i+1], "square") == 0) {
				arg[0] = 2;
				arg[2] = VOLTAPER_SQR;
				i++;
			}
			else if (strcmp(argv[i+1], "invsqr") == 0) {
				arg[0] = 2;
				arg[2] = VOLTAPER_INVSQR;
				i++;
			}
			else if (isdigit((int) argv[i+1][0])) {
				/* Set volume level */
				arg[0] = 1;
				arg[1] = (word32_t) atoi(argv[++i]);
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_VOLUME;
		}
		else if (strcmp(argv[i], "balance") == 0) {
			/* [value#] */
			if ((i+1) >= argc || !isdigit((int) argv[i+1][0]))
				/* Query */
				arg[0] = 0;
			else {
				/* Set */
				arg[0] = 1;
				arg[1] = (word32_t) atoi(argv[++i]);
			}
			*cmd = CDA_BALANCE;
		}
		else if (strcmp(argv[i], "route") == 0) {
			/* [stereo | reverse | mono-l | mono-r | 
			 *  mono | value#] 
			 */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "stereo") == 0) {
				arg[0] = 1;
				arg[1] = CHROUTE_NORMAL;
				i++;
			}
			else if (strcmp(argv[i+1], "reverse") == 0) {
				arg[0] = 1;
				arg[1] = CHROUTE_REVERSE;
				i++;
			}
			else if (strcmp(argv[i+1], "mono-l") == 0) {
				arg[0] = 1;
				arg[1] = CHROUTE_L_MONO;
				i++;
			}
			else if (strcmp(argv[i+1], "mono-r") == 0) {
				arg[0] = 1;
				arg[1] = CHROUTE_R_MONO;
				i++;
			}
			else if (strcmp(argv[i+1], "mono") == 0) {
				arg[0] = 1;
				arg[1] = CHROUTE_MONO;
				i++;
			}
			else if (isdigit((int) argv[i+1][0])) {
				/* value# */
				arg[0] = 1;
				arg[1] = (word32_t) atoi(argv[++i]);
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_ROUTE;
		}
		else if (strcmp(argv[i], "outport") == 0) {
			/* [speaker | headphone | line-out | value#] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "speaker") == 0) {
				arg[0] = 1;
				arg[1] = CDDA_OUT_SPEAKER;
				i++;
			}
			else if (strcmp(argv[i+1], "headphone") == 0) {
				arg[0] = 1;
				arg[1] = CDDA_OUT_HEADPHONE;
				i++;
			}
			else if (strcmp(argv[i+1], "line-out") == 0) {
				arg[0] = 1;
				arg[1] = CDDA_OUT_LINEOUT;
				i++;
			}
			else if (isdigit((int) argv[i+1][0])) {
				/* value# */
				arg[0] = 2;
				arg[1] = (word32_t) atoi(argv[++i]);
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_OUTPORT;
		}
		else if (strcmp(argv[i], "cdda-att") == 0) {
			/* [value#] */
			if ((i+1) >= argc || !isdigit((int) argv[i+1][0]))
				/* Query */
				arg[0] = 0;
			else {
				/* Set */
				arg[0] = 1;
				arg[1] = (word32_t) atoi(argv[++i]);
			}
			*cmd = CDA_CDDA_ATT;
		}
		else if (strcmp(argv[i], "status") == 0) {
			/* [cont [secs#]] */
			if ((i+1) >= argc || strcmp(argv[i+1], "cont") != 0)
				stat_cont = FALSE;
			else {
				i++;
				stat_cont = TRUE;
				if ((i+1) < argc &&
				    isdigit((int) argv[i+1][0]))
					cont_delay = atoi(argv[++i]);
			}
			*cmd = CDA_STATUS;
		}
		else if (strcmp(argv[i], "toc") == 0) {
			/* [offsets] */
			if ((i+1) >= argc || strcmp(argv[i+1], "offsets") != 0)
				arg[0] = 0;
			else {
				i++;
				arg[0] = 1;
			}
			*cmd = CDA_TOC;
		}
		else if (strcmp(argv[i], "extinfo") == 0) {
			/* [track#] */
			arg[0] = 0;
			if ((i+1) >= argc || !isdigit((int) argv[i+1][0]))
				arg[1] = (word32_t) -1;
			else
				arg[1] = atoi(argv[++i]);

			*cmd = CDA_EXTINFO;
		}
		else if (strcmp(argv[i], "notes") == 0) {
			/* [track#] */
			arg[0] = 0;
			if ((i+1) >= argc || !isdigit((int) argv[i+1][0]))
				arg[1] = (word32_t) -1;
			else
				arg[1] = atoi(argv[++i]);

			*cmd = CDA_NOTES;
		}
		else if (strcmp(argv[i], "on-load") == 0) {
			/* [none | spindown | autoplay | autolock | noautolock] 
			 */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "none") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else if (strcmp(argv[i+1], "spindown") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else if (strcmp(argv[i+1], "autoplay") == 0) {
				arg[0] = 1;
				arg[1] = 2;
				i++;
			}
			else if (strcmp(argv[i+1], "noautolock") == 0) {
				arg[0] = 1;
				arg[1] = 3;
				i++;
			}
			else if (strcmp(argv[i+1], "autolock") == 0) {
				arg[0] = 1;
				arg[1] = 4;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_ON_LOAD;
		}
		else if (strcmp(argv[i], "on-exit") == 0) {
			/* [none | autostop | autoeject] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "none") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else if (strcmp(argv[i+1], "autostop") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else if (strcmp(argv[i+1], "autoeject") == 0) {
				arg[0] = 1;
				arg[1] = 2;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_ON_EXIT;
		}
		else if (strcmp(argv[i], "on-done") == 0) {
			/* [autoeject | noautoeject | autoexit | noautoexit] 
			 */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "noautoeject") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else if (strcmp(argv[i+1], "autoeject") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else if (strcmp(argv[i+1], "noautoexit") == 0) {
				arg[0] = 1;
				arg[1] = 2;
				i++;
			}
			else if (strcmp(argv[i+1], "autoexit") == 0) {
				arg[0] = 1;
				arg[1] = 3;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_ON_DONE;
		}
		else if (strcmp(argv[i], "on-eject") == 0) {
			/* [autoexit | noautoexit] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "noautoexit") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else if (strcmp(argv[i+1], "autoexit") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_ON_EJECT;
		}
		else if (strcmp(argv[i], "changer") == 0) {
			/* [multiplay | nomultiplay | reverse | noreverse] 
			 */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "nomultiplay") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else if (strcmp(argv[i+1], "multiplay") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else if (strcmp(argv[i+1], "noreverse") == 0) {
				arg[0] = 1;
				arg[1] = 2;
				i++;
			}
			else if (strcmp(argv[i+1], "reverse") == 0) {
				arg[0] = 1;
				arg[1] = 3;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_CHGR;
		}
		else if (strcmp(argv[i], "mode") == 0) {
			/* [standard | cdda-play | cdda-save | cdda-save-play] 
			 */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "standard") == 0) {
				arg[0] = 1;
				arg[1] = PLAYMODE_STD;
				i++;
			}
			else if (strcmp(argv[i+1], "cdda-play") == 0) {
				arg[0] = 1;
				arg[1] = PLAYMODE_CDDA;
				i++;
			}
			else if (strcmp(argv[i+1], "cdda-save") == 0) {
				arg[0] = 1;
				arg[1] = PLAYMODE_FILE;
				i++;
			}
			else if (strcmp(argv[i+1], "cdda-pipe") == 0) {
				arg[0] = 1;
				arg[1] = PLAYMODE_PIPE;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_MODE;
		}
		else if (strcmp(argv[i], "jittercorr") == 0) {
			/* [on | off] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "on") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else if (strcmp(argv[i+1], "off") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_JITTER;
		}
		else if (strcmp(argv[i], "trackfile") == 0) {
			/* [on | off] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "on") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else if (strcmp(argv[i+1], "off") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_TRKFILE;
		}
		else if (strcmp(argv[i], "subst") == 0) {
			/* [on | off] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "on") == 0) {
				arg[0] = 1;
				arg[1] = 1;
				i++;
			}
			else if (strcmp(argv[i+1], "off") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_SUBST;
		}
		else if (strcmp(argv[i], "filefmt") == 0) {
			/* [ raw | au | wav | aiff | aiff-c |
			 *   mp3 | ogg | flac | aac | mp4 ]
			 */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (util_strcasecmp(argv[i+1], "raw") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_RAW;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "au") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_AU;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "wav") == 0 ||
				 util_strcasecmp(argv[i+1], "wave") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_WAV;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "aiff") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_AIFF;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "aiff-c") == 0 ||
				 util_strcasecmp(argv[i+1], "aifc") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_AIFC;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "mp3") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_MP3;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "ogg") == 0 ||
				 util_strcasecmp(argv[i+1], "vorbis") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_OGG;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "flac") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_FLAC;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "aac") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_AAC;
				i++;
			}
			else if (util_strcasecmp(argv[i+1], "mp4") == 0) {
				arg[0] = 1;
				arg[1] = FILEFMT_MP4;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_FILEFMT;
		}
		else if (strcmp(argv[i], "outfile") == 0) {
			if (++i < argc) {
				/* Set */
				struct stat	stbuf;

				arg[0] = 1;
				if (strlen(argv[i]) >=
				    ((CDA_NARGS - 1) * sizeof(word32_t))) {
					(void) fprintf(errfp,
						"%s: file name too long.\n",
						argv[i]);
					return FALSE;
				}
				if (LSTAT(argv[i], &stbuf) == 0 &&
				    !S_ISREG(stbuf.st_mode)) {
					(void) fprintf(errfp, "%s: %s\n",
						argv[i],
						app_data.str_notregfile);
					return FALSE;
				}
				(void) strcpy((char *) &arg[1], argv[i]);
			}
			else {
				/* Query */
				arg[0] = 0;
			}
			*cmd = CDA_FILE;
		}
		else if (strcmp(argv[i], "pipeprog") == 0) {
			if (++i < argc) {
				/* Set */
				arg[0] = 1;
				if (strlen(argv[i]) >=
				    ((CDA_NARGS - 1) * sizeof(word32_t))) {
					(void) fprintf(errfp,
						"%s: file name too long.\n",
						argv[i]);
					return FALSE;
				}
				(void) strcpy((char *) &arg[1], argv[i]);
			}
			else {
				/* Query */
				arg[0] = 0;
			}
			*cmd = CDA_PIPEPROG;
		}
		else if (strcmp(argv[i], "compress") == 0) {
			/* [<0|3> [bitrate#] | <1|2> [qual#]] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "0") == 0 ||
				 strcmp(argv[i+1], "cbr") == 0) {
				arg[0] = 1;
				arg[1] = COMPMODE_0;
				i++;
				if ((i+1) >= argc ||
				    !isdigit((int) argv[i+1][0]))
					arg[2] = (word32_t) -1;
				else
					arg[2] = atoi(argv[++i]);
			}
			else if (strcmp(argv[i+1], "3") == 0 ||
				 strcmp(argv[i+1], "abr") == 0) {
				arg[0] = 1;
				arg[1] = COMPMODE_3;
				i++;
				if ((i+1) >= argc ||
				    !isdigit((int) argv[i+1][0]))
					arg[2] = (word32_t) -1;
				else
					arg[2] = atoi(argv[++i]);
			}
			else if (strcmp(argv[i+1], "1") == 0 ||
				 strcmp(argv[i+1], "vbr") == 0) {
				arg[0] = 1;
				arg[1] = COMPMODE_1;
				i++;
				if ((i+1) >= argc ||
				    !isdigit((int) argv[i+1][0]))
					arg[2] = (word32_t) -1;
				else
					arg[2] = atoi(argv[++i]);
			}
			else if (strcmp(argv[i+1], "2") == 0 ||
				 strcmp(argv[i+1], "vbr2") == 0) {
				arg[0] = 1;
				arg[1] = COMPMODE_2;
				i++;
				if ((i+1) >= argc ||
				    !isdigit((int) argv[i+1][0]))
					arg[2] = (word32_t) -1;
				else
					arg[2] = atoi(argv[++i]);
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_COMPRESS;
		}
		else if (strcmp(argv[i], "min-brate") == 0) {
			/* [brate#] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
				arg[1] = 1;
			}
			else if (isdigit((int) argv[i+1][0])) {
				arg[0] = 1;
				arg[2] = atoi(argv[i+1]);
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_BRATE;
		}
		else if (strcmp(argv[i], "max-brate") == 0) {
			/* [brate#] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
				arg[1] = 2;
			}
			else if (isdigit((int) argv[i+1][0])) {
				arg[0] = 1;
				arg[2] = atoi(argv[i+1]);
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_BRATE;
		}
		else if (strcmp(argv[i], "lowpass") == 0) {
			/* [off | auto | freq# [width#]] */
			arg[2] = 1;
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "off") == 0) {
				arg[0] = 1;
				arg[1] = FILTER_OFF;
				i++;
			}
			else if (strcmp(argv[i+1], "auto") == 0) {
				arg[0] = 1;
				arg[1] = FILTER_AUTO;
				i++;
			}
			else if (isdigit((int) argv[i+1][0])) {
				arg[0] = 1;
				arg[1] = FILTER_MANUAL;
				arg[3] = (word32_t) atoi(argv[i+1]);
				i++;
				if ((i+1) >= argc ||
				    !isdigit((int) argv[i+1][0]))
					arg[4] = (word32_t) -1;
				else {
					if ((j = atoi(argv[++i])) < 0) {
						usage();
						return FALSE;
					}
					arg[4] = (word32_t) j;
				}
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_FILTER;
		}
		else if (strcmp(argv[i], "highpass") == 0) {
			/* [off | auto | freq# [width#]] */
			arg[2] = 2;
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "off") == 0) {
				arg[0] = 1;
				arg[1] = FILTER_OFF;
				i++;
			}
			else if (strcmp(argv[i+1], "auto") == 0) {
				arg[0] = 1;
				arg[1] = FILTER_AUTO;
				i++;
			}
			else if (isdigit((int) argv[i+1][0])) {
				arg[0] = 1;
				arg[1] = FILTER_MANUAL;
				arg[3] = (word32_t) atoi(argv[i+1]);
				i++;
				if ((i+1) >= argc ||
				    !isdigit((int) argv[i+1][0]))
					arg[4] = (word32_t) -1;
				else {
					if ((j = atoi(argv[++i])) < 0) {
						usage();
						return FALSE;
					}
					arg[4] = (word32_t) j;
				}
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_FILTER;
		}
		else if (strcmp(argv[i], "coding") == 0) {
			/* [stereo | j-stereo | force-ms | mono | algo#] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "stereo") == 0) {
				arg[0] = 1;
				arg[1] = CH_STEREO;
				i++;
			}
			else if (strcmp(argv[i+1], "j-stereo") == 0) {
				arg[0] = 1;
				arg[1] = CH_JSTEREO;
				i++;
			}
			else if (strcmp(argv[i+1], "force-ms") == 0) {
				arg[0] = 1;
				arg[1] = CH_FORCEMS;
				i++;
			}
			else if (strcmp(argv[i+1], "mono") == 0) {
				arg[0] = 1;
				arg[1] = CH_MONO;
				i++;
			}
			else if (isdigit((int) argv[i+1][0])) {
				arg[0] = 2;
				arg[2] = (word32_t) atoi(argv[i+1]);
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_CODING;
		}
		else if (strcmp(argv[i], "flags") == 0) {
			/* [C|c][O|o][N|n][E|e][I|i] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else {
				arg[0] = 1;
				arg[1] = (word32_t) -1;
				arg[2] = (word32_t) -1;
				arg[3] = (word32_t) -1;
				arg[4] = (word32_t) -1;
				arg[5] = (word32_t) -1;

				if (strchr(argv[i+1], 'C') != 0)
					arg[1] = 1;
				else if (strchr(argv[i+1], 'c') != 0)
					arg[1] = 0;

				if (strchr(argv[i+1], 'O') != 0)
					arg[2] = 1;
				else if (strchr(argv[i+1], 'o') != 0)
					arg[2] = 0;

				if (strchr(argv[i+1], 'N') != 0)
					arg[3] = 1;
				else if (strchr(argv[i+1], 'n') != 0)
					arg[3] = 0;

				if (strchr(argv[i+1], 'E') != 0)
					arg[4] = 1;
				else if (strchr(argv[i+1], 'e') != 0)
					arg[4] = 0;

				if (strchr(argv[i+1], 'I') != 0)
					arg[5] = 1;
				else if (strchr(argv[i+1], 'i') != 0)
					arg[5] = 0;
			}
			*cmd = CDA_FLAGS;
		}
		else if (strcmp(argv[i], "lameopts") == 0) {
			/* [<disable | insert | append | replace> options] */
			if (++i < argc) {
				/* Set */
				arg[0] = 1;
				if (strcmp(argv[i], "disable") == 0)
					arg[1] = LAMEOPTS_DISABLE;
				else if (strcmp(argv[i], "insert") == 0)
					arg[1] = LAMEOPTS_INSERT;
				else if (strcmp(argv[i], "append") == 0)
					arg[1] = LAMEOPTS_APPEND;
				else if (strcmp(argv[i], "replace") == 0)
					arg[1] = LAMEOPTS_REPLACE;
				else
					return FALSE;

				if (++i >= argc) {
				    (void) strcpy((char *) &arg[2], ".");
				}
				else {
				    if (strlen(argv[i]) >=
					((CDA_NARGS - 2) * sizeof(word32_t))) {
					    (void) fprintf(errfp,
						"%s: lame options too long.\n",
						argv[i]);
					    return FALSE;
				    }
				    (void) strcpy((char *) &arg[2], argv[i]);
				}
			}
			else {
				/* Query */
				arg[0] = 0;
			}
			*cmd = CDA_LAMEOPTS;
		}
		else if (strcmp(argv[i], "tag") == 0) {
			/* [off | v1 | v2 | both] */
			if ((i+1) >= argc) {
				/* Query */
				arg[0] = 0;
			}
			else if (strcmp(argv[i+1], "off") == 0) {
				arg[0] = 1;
				arg[1] = 0;
				i++;
			}
			else if (strcmp(argv[i+1], "v1") == 0) {
				arg[0] = 1;
				arg[1] = ID3TAG_V1;
				i++;
			}
			else if (strcmp(argv[i+1], "v2") == 0) {
				arg[0] = 1;
				arg[1] = ID3TAG_V2;
				i++;
			}
			else if (strcmp(argv[i+1], "both") == 0) {
				arg[0] = 1;
				arg[1] = ID3TAG_BOTH;
				i++;
			}
			else {
				/* Wrong arg */
				usage();
				return FALSE;
			}
			*cmd = CDA_TAG;
		}
		else if (strcmp(argv[i], "motd") == 0) {
			*cmd = CDA_MOTD;
		}
		else if (strcmp(argv[i], "device") == 0) {
			*cmd = CDA_DEVICE;
		}
		else if (strcmp(argv[i], "version") == 0) {
			*cmd = CDA_VERSION;
		}
		else if (strcmp(argv[i], "cddbreg") == 0) {
			*cmd = CDA_CDDBREG;
		}
		else if (strcmp(argv[i], "cddbhint") == 0) {
			*cmd = CDA_CDDBHINT;
		}
		else if (strcmp(argv[i], "debug") == 0) {
			/* [level] */
			if ((i+1) < argc && isdigit((int) argv[i+1][0])) {
				/* Set debug level */
				arg[0] = 1;
				arg[1] = (word32_t) atoi(argv[++i]);
			}
			else {
				/* Query */
				arg[0] = 0;
			}
			*cmd = CDA_DEBUG;
		}
		else if (strcmp(argv[i], "visual") == 0) {
#ifdef NOVISUAL
			(void) fprintf(errfp, "%s %s\n",
				       "Cannot start visual mode:",
				       "curses support is not compiled in!");
			return FALSE;
#else
			visual = TRUE;
			*cmd = CDA_STATUS;
			/* Make sure simulator/debug output is redirectable */
			ttyfp = stderr;
#endif
		}
		else {
			usage();
			return FALSE;
		}
	}

	if (*cmd == 0) {
		/* User did not specify a command */
		usage();
		return FALSE;
	}

	return TRUE;
}


/*
 * cda_match_select
 *	Ask the user to select from a list of fuzzy CDDB matches.
 *
 * Args:
 *	matchlist - List of fuzzy matches
 *
 * Return:
 *	User selection number, or 0 for no selection.
 */
STATIC int
cda_match_select(cdinfo_match_t *matchlist)
{
	int		i,
			n;
	cdinfo_match_t	*p;
	chset_conv_t	*up;
	char		*p1,
			*p2,
			*q1,
			*q2,
			input[64];

	for (p = matchlist, i = 0; p != NULL; p = p->next, i++)
		;

	if (batch || (i == 1 && app_data.single_fuzzy)) {
		(void) fprintf(ttyfp, "\n\n%s\n%s\n\n",
		    "CDDB has found an inexact match.  If this is not the",
		    "correct album, please submit corrections using xmcd.");
		(void) fflush(ttyfp);
		return 1;
	}

	/* Convert CD info from UTF-8 to local charset if possible */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return 0;

	(void) fprintf(ttyfp, "\n\n%s\n%s\n\n",
		"CDDB has found the following potential matches for the album",
		"that you have.  Please choose an appropriate entry:");

	/* Display list */
	for (p = matchlist, i = 1; p != NULL; p = p->next, i++) {
		q1 = (p->artist == NULL) ? "(unknown artist)" : p->artist;
		q2 = (p->title == NULL) ? "(unknown title)" : p->title;
		p1 = p2 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return 0;
		}
		if (!util_chset_conv(up, q2, &p2, FALSE) &&
		    !util_newstr(&p2, q2)) {
			util_chset_close(up);
			return 0;
		}
		(void) fprintf(ttyfp, "  %2d. %.30s / %-40s\n", i,
				p1 == NULL ? "" : p1,
				p2 == NULL ? "" : p2);
		if (p1 != NULL)
			MEM_FREE(p1);
		if (p2 != NULL)
			MEM_FREE(p2);
	}

	(void) fprintf(ttyfp, "  %2d. None of the above\n\n", i);

	util_chset_close(up);

	for (;;) {
		(void) fprintf(ttyfp, "Choose one (1-%d): ", i);
		if (fgets(input, 63, stdin) == NULL) {
			(void) fprintf(ttyfp, "\n");
			(void) fflush(ttyfp);
			return 0;
		}

		input[strlen(input)-1] = '\0';

		n = atoi(input);
		if (n > 0 && n <= i)
			break;
	}

	(void) fprintf(ttyfp, "\n");
	(void) fflush(ttyfp);
	return ((n == i) ? 0 : n);
}


/*
 * cda_auth
 *	Ask the user to enter user and password for proxy authorization.
 *
 * Args:
 *	retrycnt - How many times has the user retried this
 *
 * Return:
 *	0 for failure
 *	1 for success
 */
STATIC int
cda_auth(int retrycnt)
{
	word32_t	arg[CDA_NARGS];
	char		input[64];
	size_t		maxsz = (CDA_NARGS - 1) * sizeof(word32_t);
	int		retcode;

	if (batch)
		return 0;

	if (retrycnt == 0)
		(void) fprintf(ttyfp, "Proxy Authorization is required.\n");
	else {
		(void) fprintf(ttyfp, "Proxy Authorization failed.");

		if (retrycnt < 3)
			(void) fprintf(ttyfp, "  Try again.\n");
		else {
			(void) fprintf(ttyfp, "  Too many tries.\n\n");
			(void) fflush(ttyfp);
			return 0;
		}
	}

	(void) fprintf(ttyfp, "%s\n\n",
		"Please enter your proxy user name and password.");

	/* Get user name */
	(void) fprintf(ttyfp, "Username: ");
	(void) fflush(ttyfp);
	if (fgets(input, 63, stdin) == NULL) {
		(void) fprintf(ttyfp, "\n");
		(void) fflush(ttyfp);
		return 0;
	}
	input[strlen(input)-1] = '\0';
	if (input[0] == '\0')
		return 0;

	if (!util_newstr(&dbp->proxy_user, input)) {
		CDA_FATAL(app_data.str_nomemory);
		return 0;
	}

	/* Get password */
	(void) fprintf(ttyfp, "Password: ");
	(void) fflush(ttyfp);
	cda_echo(ttyfp, FALSE);
	if (fgets(input, 63, stdin) == NULL) {
		cda_echo(ttyfp, TRUE);
		(void) fprintf(ttyfp, "\n");
		(void) fflush(ttyfp);
		return 0;
	}
	cda_echo(ttyfp, TRUE);
	input[strlen(input)-1] = '\0';

	if (!util_newstr(&dbp->proxy_passwd, input)) {
		CDA_FATAL(app_data.str_nomemory);
		return 0;
	}

	(void) fprintf(ttyfp, "\n");

	/* Pass the proxy user name and password to the daemon */

	(void) memset(arg, 0, CDA_NARGS * sizeof(word32_t));
	(void) strcpy((char *) &arg[0], "user");
	(void) strncpy((char *) &arg[1], dbp->proxy_user, maxsz - 1);
	if (!cda_sendcmd(CDA_AUTHUSER, arg, &retcode)) {
		(void) fprintf(ttyfp, "%s:\n%s\n",
			"Failed sending proxy user name to the cda daemon",
			emsgp
		);
		util_delayms(2000);
	}

	(void) memset(arg, 0, CDA_NARGS * sizeof(word32_t));
	(void) strcpy((char *) &arg[0], "pass");
	(void) strncpy((char *) &arg[1], dbp->proxy_passwd, maxsz - 1);
	if (!cda_sendcmd(CDA_AUTHPASS, arg, &retcode)) {
		(void) fprintf(ttyfp, "%s:\n%s\n",
			"Failed sending proxy password to the cda daemon",
			emsgp
		);
		util_delayms(2000);
	}

	(void) fflush(ttyfp);
	return 1;
}


/***********************
 *   public routines   *
 ***********************/


/*
 * cda_echo
 *	Turn off/on echo mode.  It is assumed that the program starts
 *	with echo mode enabled.
 *
 * Args:
 *	fp - Opened file stream to the user's terminal
 *	doecho - Whether echo should be enabled
 *
 * Return:
 *	Nothing.
 */
void
cda_echo(FILE *fp, bool_t doecho)
{
	int			fd = fileno(fp);
#ifdef USE_TERMIOS
	struct termios		tio;

	(void) tcgetattr(fd, &tio);

	if (doecho)
		tio.c_lflag |= ECHO;
	else
		tio.c_lflag &= ~ECHO;

	(void) tcsetattr(fd, TCSADRAIN, &tio);
#endif	/* USE_TERMIOS */

#ifdef USE_TERMIO
	struct termio		tio;

	(void) ioctl(fd, TCGETA, &tio);

	if (doecho)
		tio.c_lflag |= ECHO;
	else
		tio.c_lflag &= ~ECHO;

	(void) ioctl(fd, TCSETAW, &tio);
#endif	/* USE_TERMIO */

#ifdef USE_SGTTY
	struct sgttyb		tio;

	(void) ioctl(fd, TIOCGETP, &tio);

	if (doecho)
		tio.sg_flags |= ECHO;
	else
		tio.sg_flags &= ~ECHO;

	(void) ioctl(fd, TIOCSETP, &tio);
#endif	/* USE_SGTTY */
}


/*
 * curstat_addr
 *	Return the address of the curstat_t structure.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
curstat_t *
curstat_addr(void)
{
	return (&status);
}


/*
 * cda_quit
 *      Shut down and exit
 *
 * Args:
 *      s - Pointer to the curstat_t structure.
 *
 * Return:
 *      No return: program exits at end of function
 */
void
cda_quit(curstat_t *s)
{
	if (isdaemon) {
		/* Send response packet to prevent client hang */
		if (curr_pkt != NULL) {
			curr_pkt->retcode = CDA_DAEMON_EXIT;
			(void) cda_sendpkt("CD audio daemon",
					   cda_rfd[0], curr_pkt);
		}

		/* Remove lock */
		if (dlock[0] != '\0')
			(void) UNLINK(dlock);

		/* Close FIFOs - daemon side */
		if (cda_sfd[0] >= 0)
			(void) close(cda_sfd[0]);
		if (cda_rfd[0] >= 0)
			(void) close(cda_rfd[0]);

		/* Remove FIFOs */
		if (spipe[0] != '\0')
			(void) UNLINK(spipe);
		if (rpipe[0] != '\0')
			(void) UNLINK(rpipe);

		/* Shut down CD interface subsystem */
		di_halt(s);
	}
#ifndef NOVISUAL
	else {
		cda_vtidy();
	}
#endif

	exit(exit_status);
}


/*
 * cda_warning_msg
 *	Print warning message.
 *
 * Args:
 *	title - Not used.
 *	msg - The warning message text string.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
cda_warning_msg(char *title, char *msg)
{
	(void) fprintf(errfp, "CD audio Warning: %s\n", msg);
}


/*
 * cda_info_msg
 *	Print info message.
 *
 * Args:
 *	title - Not used.
 *	msg - The info message text string.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
cda_info_msg(char *title, char *msg)
{
	(void) fprintf(errfp, "%s\n", msg);
}


/*
 * cda_fatal_msg
 *	Print fatal error message.
 *
 * Args:
 *	title - Not used..
 *	msg - The fatal error message text string.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
cda_fatal_msg(char *title, char *msg)
{
	(void) fprintf(errfp, "CD audio Fatal Error:\n%s\n", msg);
	exit_status = 6;
	cda_quit(&status);
	/*NOTREACHED*/
}


/*
 * cda_parse_devlist
 *	Parse the app_data.devlist string and create the cda_devlist array.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
cda_parse_devlist(curstat_t *s)
{
	char		*p,
			*q;
	int		i,
			n,
			listsz;

	if (app_data.chg_method < 0 || app_data.chg_method >= MAX_CHG_METHODS)
		/* Fix-up in case of mis-configuration */
		app_data.chg_method = CHG_NONE;

	n = app_data.numdiscs;

	switch (app_data.chg_method) {
	case CHG_SCSI_MEDCHG:
		n = 2;
		/*FALLTHROUGH*/

	case CHG_SCSI_LUN:
		/* SCSI LUN addressing method */
		listsz = n * sizeof(char *);

		cda_devlist = (char **) MEM_ALLOC("cda_devlist", listsz);
		if (cda_devlist == NULL) {
			CDA_FATAL(app_data.str_nomemory);
			return;
		}
		(void) memset(cda_devlist, 0, listsz);

		p = q = app_data.devlist;
		if (p == NULL || *p == '\0') {
			CDA_FATAL(app_data.str_devlist_undef);
			return;
		}

		for (i = 0; i < n; i++) {
			q = strchr(p, ';');

			if (q == NULL && i < (n - 1)) {
				CDA_FATAL(app_data.str_devlist_count);
				return;
			}

			if (q != NULL)
				*q = '\0';

			cda_devlist[i] = NULL;
			if (!util_newstr(&cda_devlist[i], p)) {
				CDA_FATAL(app_data.str_nomemory);
				return;
			}

			if (q != NULL)
				*q = ';';

			p = q + 1;
		}
		break;

	case CHG_OS_IOCTL:
	case CHG_NONE:
	default:
		if (app_data.devlist == NULL) {
			if (!util_newstr(&app_data.devlist, app_data.device)) {
				CDA_FATAL(app_data.str_nomemory);
				return;
			}
		}
		else if (strcmp(app_data.devlist, app_data.device) != 0) {
			MEM_FREE(app_data.devlist);
			if (!util_newstr(&app_data.devlist, app_data.device)) {
				CDA_FATAL(app_data.str_nomemory);
				return;
			}
		}

		cda_devlist = (char **) MEM_ALLOC(
			"cda_devlist",
			sizeof(char *)
		);
		if (cda_devlist == NULL) {
			CDA_FATAL(app_data.str_nomemory);
			return;
		}

		cda_devlist[0] = NULL;
		if (!util_newstr(&cda_devlist[0], app_data.devlist)) {
			CDA_FATAL(app_data.str_nomemory);
			return;
		}
		break;
	}

	/* Initialize to the first device */
	s->curdev = cda_devlist[0];
}


/*
 * cda_dbclear
 *	Clear in-core CD information.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	reload - Whether we are going to be re-loading the CD info entry.
 *
 * Return:
 *	Nothing.
 */
void
cda_dbclear(curstat_t *s, bool_t reload)
{
	/* Clear in-core CD information.
	 * Note that in cda, QMODE_ERR means CD info is not loaded.
	 * This is different than in xmcd.
	 */
	cdinfo_clear(reload);
	s->qmode = QMODE_ERR;
}


/*
 * cda_dbload
 *	Load the CD information pertaining to the current disc, if available.
 *	This is for use by the cda client.
 *
 * Args:
 *	id - The xmcd disc ID
 *	ifunc - Function pointer to a fuzzy CD info match handling function.
 *	pfunc - Function pointer to a proxy authorization handling function.
 *	depth - recursion depth
 *
 * Return:
 *	TRUE = success
 *	FALSE = failure
 */
bool_t
cda_dbload(
	word32_t	id,
	int		(*ifunc)(cdinfo_match_t *),
	int		(*pfunc)(int),
	int		depth
)
{
	int		i,
			n,
			retcode,
			ret;
	curstat_t	*s;
	word32_t	arg[CDA_NARGS];

	s = &status;
	(void) memset(arg, 0, CDA_NARGS * sizeof(word32_t));

	if (!cda_sendcmd(CDA_TOC2, arg, &retcode)) {
		(void) printf("\n%s\n", emsgp);
		return FALSE;
	}

	s->tot_trks = id & 0xff;

	/* Update curstat with essential info */
	for (i = 0; i < (int) s->tot_trks; i++) {
		RD_ARG_TOC2(arg[i+1], s->trkinfo[i].trkno,
			s->trkinfo[i].min,
			s->trkinfo[i].sec,
			s->trkinfo[i].frame
		);

		util_msftoblk(
			s->trkinfo[i].min,
			s->trkinfo[i].sec,
			s->trkinfo[i].frame,
			&s->trkinfo[i].addr,
			MSF_OFFSET
		);
	}

	/* Lead-out track */
	RD_ARG_TOC2(arg[i+1], s->trkinfo[i].trkno,
		s->trkinfo[i].min,
		s->trkinfo[i].sec,
		s->trkinfo[i].frame
	);
	util_msftoblk(
		s->trkinfo[i].min,
		s->trkinfo[i].sec,
		s->trkinfo[i].frame,
		&s->trkinfo[i].addr,
		MSF_OFFSET
	);

	/* Set appropriate offline mode */
	if ((ret = cdinfo_offline(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_offline: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	/* Load CD information */
	if ((ret = cdinfo_load(s)) != 0) {
		/* Failed to load */
		DBGPRN(DBG_CDI)(errfp, "\ncdinfo_load: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));

		switch (CDINFO_GET_STAT(ret)) {
		case AUTH_ERR:
			if (pfunc == NULL || (n = (*pfunc)(depth)) == 0)
				return FALSE;

			/* Recursion */
			return (cda_dbload(id, ifunc, pfunc, depth + 1));

		default:
			return FALSE;
		}
	}

	if (dbp->matchlist != NULL) {
		/* Got a list of "fuzzy matches": Prompt user
		 * for a selection.
		 */
		if (ifunc == NULL || (n = (*ifunc)(dbp->matchlist)) == 0)
			return TRUE;

		dbp->match_tag = (long) n;

		/* Load CD information */
		if ((ret = cdinfo_load_matchsel(s)) != 0) {
			/* Failed to load */
			DBGPRN(DBG_CDI)(errfp,
				"\ncdinfo_load_matchsel: status=%d arg=%d\n",
				CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
			return FALSE;
		}
		else if ((dbp->flags & CDINFO_MATCH) != 0) {
			/* Exact match */
			if (app_data.discog_mode & DISCOG_GEN_CDINFO) {
				/* Generate new local discography */
				if ((ret = cdinfo_gen_discog(s)) != 0) {
				    DBGPRN(DBG_CDI)(errfp,
					"cdinfo_gen_discog: status=%d arg=%d\n",
					CDINFO_GET_STAT(ret),
					CDINFO_GET_ARG(ret));
				}
			}

			return TRUE;
		}
		else
			return FALSE;
	}
	else if ((dbp->flags & CDINFO_NEEDREG) != 0) {
		/* User registration required */
		(void) fprintf(ttyfp,
		    "\n\nYou must register with CDDB to access the CDDB2 "
		    "service.\nTo do that, run the \"cda cddbreg\" command.");
		return FALSE;
	}
	else if ((dbp->flags & CDINFO_MATCH) != 0) {
		/* Exact match */
		if (app_data.discog_mode & DISCOG_GEN_CDINFO) {
			/* Generate new local discography */
			if ((ret = cdinfo_gen_discog(s)) != 0) {
				DBGPRN(DBG_CDI)(errfp,
					"cdinfo_gen_discog: status=%d arg=%d\n",
					CDINFO_GET_STAT(ret),
					CDINFO_GET_ARG(ret));
			}
		}

		return TRUE;
	}

	return TRUE;
}


/*
 * cda_sendcmd
 *	Send command down the pipe and handle response.
 *
 * Args:
 *	cmd - The command code
 *	arg - Command arguments
 *	retcode - Return code
 *
 * Return:
 *	Status code
 */
bool_t
cda_sendcmd(word32_t cmd, word32_t arg[], int *retcode)
{
	int		i;
	cdapkt_t	p,
			r;

	/* Fill in command packet */
	(void) memset(&p, 0, sizeof(cdapkt_t));
	p.pktid = getpid();
	p.cmd = cmd;
	(void) memcpy(p.arg, arg, CDA_NARGS * sizeof(word32_t));

	/* Send command packet */
	if (!cda_sendpkt("CD audio", cda_sfd[1], &p)) {
		emsgp = "\nCannot send packet to CD audio daemon.";
		*retcode = -1;
		return FALSE;
	}

	for (i = 0; ; i++) {
		/* Get response packet */
		if (!cda_getpkt("CD audio", cda_rfd[1], &r)) {
			emsgp = "Cannot get packet from CD audio daemon.";
			*retcode = -1;
			return FALSE;
		}

		/* Sanity check */
		if (p.pktid == r.pktid)
			break;

		/* This is not our packet */

		if (i >= MAX_PKT_RETRIES) {
			emsgp =
			"Retry overflow reading packet from CD audio daemon.";
			*retcode = -1;
			return FALSE;
		}

		/* Increment packet reject count */
		r.rejcnt++;

		/* If packet has not reached reject limit, put it back
		 * into the pipe.
		 */
		if (r.rejcnt < MAX_PKT_RETRIES &&
		    !cda_sendpkt("CD audio", cda_rfd[0], &r)) {
			emsgp = "Cannot send packet to CD audio daemon.";
			*retcode = -1;
			return FALSE;
		}

		/* Introduce some randomness to shuffle
		 * the order of packets in the pipe
		 */
		if ((rand() & 0x80) != 0)
			util_delayms(100);
	}

	/* Return args */
	(void) memcpy(arg, r.arg, CDA_NARGS * sizeof(word32_t));

	*retcode = r.retcode;

	/* Check return code */
	switch (r.retcode) {
	case CDA_OK:
		return TRUE;
	case CDA_INVALID:
		emsgp = "This command is not valid in the current state.";
		return FALSE;
	case CDA_PARMERR:
		emsgp = "Command argument error.";
		return FALSE;
	case CDA_FAILED:
		emsgp = "This command failed.";
		return FALSE;
	case CDA_REFUSED:
		emsgp = "The CD audio daemon does not accept this command.";
		return FALSE;
	case CDA_DAEMON_EXIT:
		emsgp = "CD audio daemon exited.";
		return FALSE;
	default:
		emsgp = "The CD audio daemon returned an invalid status.";
		return FALSE;
	}
	/*NOTREACHED*/
}


/*
 * cda_daemon_alive
 *	Check if the cda daemon is running.
 *
 * Args:
 *	None.
 *
 * Return:
 *	TRUE - daemon is alive
 *	FALSE - daemon is dead
 */
bool_t
cda_daemon_alive(void)
{
	int		fd;
	pid_t		pid;
	char		dlock[FILE_PATH_SZ],
			buf[32];

	(void) sprintf(dlock, "%s/cdad.%x", TEMP_DIR, (int) cd_rdev);

	fd = open(dlock, O_RDONLY
#ifdef O_NOFOLLOW
			 | O_NOFOLLOW
#endif
	);
	if (fd < 0)
		return FALSE;

	if (read(fd, buf, 32) < 0) {
		(void) close(fd);
		return FALSE;
	}
	(void) close(fd);

	if (strncmp(buf, "cdad ", 5) != 0)
		return FALSE;

	pid = (pid_t) atoi(buf + 5);
	
	if (kill(pid, 0) < 0)
		return FALSE;

	daemon_pid = pid;

	return TRUE;
}


/*
 * cda_daemon
 *	CD audio daemon main loop function.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Visual mode: FALSE = failure, TRUE = success
 *	Command line mode: No return
 */
bool_t
cda_daemon(curstat_t *s)
{
	int		i;
	cdapkt_t	p;
	bool_t		done = FALSE;

	if (cda_daemon_alive()) {
		/* Daemon already running */
#ifndef NOVISUAL
		if (visual)
			return TRUE;
#endif
		cda_quit(s);
		/*NOTREACHED*/
	}

	/* Make some needed directories */
	cda_mkdirs();

	/* Become a daemon process */
	switch (FORK()) {
	case -1:
#ifndef NOVISUAL
		if (visual)
			return FALSE;
#endif
		perror("Cannot fork CD audio daemon");
		cda_quit(s);
		/*NOTREACHED*/
	case 0:
		/* Child process: the daemon */
		break;
	default:
		/* Parent process */
		/* Make sure the daemon is running */
		for (i = 0; i < 5; i++) {
			if (cda_daemon_alive())
				break;
			util_delayms(1000);
		}
		if (i >= 5) {
			CDA_FATAL("CD audio daemon not started.");
		}

#ifndef NOVISUAL
		if (visual)
			return TRUE;
#endif
		exit(0);
	}

	/* Check for duplicate daemon processes */
	if (!cda_daemonlock()) {
		(void) fprintf(errfp, "CD audio daemon already running.\n");
		exit(1);
	}

	/* Set daemon flag */
	isdaemon = TRUE;

	/* Create FIFOs */
	if (MKFIFO(spipe, 0600) < 0) {
		if (errno != EEXIST) {
			perror("CD audio: Cannot create send pipe");
			exit_status = 4;
			cda_quit(s);
			/*NOTREACHED*/
		}

		/* Try removing and re-creating the FIFO */
		if (UNLINK(spipe) < 0) {
			perror("CD audio: Cannot unlink old send pipe");
			exit_status = 4;
			cda_quit(s);
			/*NOTREACHED*/
		}
		if (MKFIFO(spipe, 0600) < 0) {
			perror("CD audio: Cannot create send pipe");
			exit_status = 4;
			cda_quit(s);
			/*NOTREACHED*/
		}
	}
	if (MKFIFO(rpipe, 0600) < 0) {
		if (errno != EEXIST) {
			perror("CD audio: Cannot create recv pipe");
			exit_status = 4;
			cda_quit(s);
			/*NOTREACHED*/
		}

		/* Try removing and re-creating the FIFO */
		if (UNLINK(rpipe) < 0) {
			perror("CD audio: Cannot unlink old recv pipe");
			exit_status = 4;
			cda_quit(s);
			/*NOTREACHED*/
		}
		if (MKFIFO(rpipe, 0600) < 0) {
			perror("CD audio: Cannot create recv pipe");
			exit_status = 4;
			cda_quit(s);
			/*NOTREACHED*/
		}
	}

	/* Initialize and start drive interfaces */
	cda_init(s);
	cda_start(s);

	/* Handle some signals */
	if (util_signal(SIGHUP, onsig0) == SIG_IGN)
		(void) util_signal(SIGHUP, SIG_IGN);
	(void) util_signal(SIGTERM, SIG_IGN);
	(void) util_signal(SIGINT, SIG_IGN);
	(void) util_signal(SIGQUIT, SIG_IGN);
#if defined(SIGTSTP) && defined(SIGCONT)
	(void) util_signal(SIGTSTP, SIG_IGN);
	(void) util_signal(SIGCONT, SIG_IGN);
#endif

	/* Main command handling loop */
	while (!done) {
		/* Get command packet */
		if (!cda_getpkt("CD audio daemon", cda_sfd[0], &p)) {
			exit_status = 5;
			cda_quit(s);
			/*NOTREACHED*/
		}

		/* HACK: Set current packet pointer.  In case
		 * cda_docmd() leads to cda_quit() due to exitOnEject
		 * or exitOnDone, cda_quit() can send a response
		 * packet back to the client before exiting.
		 */
		curr_pkt = &p;

		/* Interpret and carry out command */
		done = cda_docmd(s, &p);

		/* Clear current packet pointer */
		curr_pkt = NULL;

		/* Send response packet */
		if (!cda_sendpkt("CD audio daemon", cda_rfd[0], &p)) {
			exit_status = 5;
			cda_quit(s);
			/*NOTREACHED*/
		}
	}

	/* Stop the drive */
	exit_status = 0;
	cda_quit(s);

	/* This is to appease gcc -Wall even though we can't get here */
	return TRUE;
}


/*
 * main
 *	The main function
 */
int
main(int argc, char **argv)
{
	int		i,
			retcode,
			ret;
	word32_t	cmd,
			*arg1,
			*arg2;
	struct stat	stbuf;
	char		*ttypath,
			*cp,
			*hd,
			path[FILE_PATH_SZ * 2];
	bool_t		loaddb = TRUE;
	curstat_t	*s;

	/* Program startup multi-threading initializations */
	cdda_preinit();

	errfp = stderr;
	s = &status;
	(void) memset(s, 0, sizeof(curstat_t));

	/* Hack: Aside from stdin, stdout, stderr, there shouldn't
	 * be any other files open, so force-close them.  This is
	 * necessary in case xmcd was inheriting any open file
	 * descriptors from a parent process which is for the
	 * CD-ROM device, and violating exclusive-open requirements
	 * on some platforms.
	 */
	for (i = 3; i < 10; i++)
		(void) close(i);

#ifdef HAS_ICONV
	/* Set locale to be based on environment */
	(void) setlocale(LC_ALL, "");
#endif

	(void) util_signal(SIGCHLD, SIG_DFL);

	/* Initialize */
	if ((ttypath = ttyname(2)) == NULL)
		ttypath = "/dev/tty";
	if ((ttyfp = fopen(ttypath, "w")) != NULL)
		setbuf(ttyfp, NULL);
	else
		ttyfp = stderr;

	/* Initialize error messages */
	app_data.str_nomethod = STR_NOMETHOD;
	app_data.str_novu = STR_NOVU;
	app_data.str_unknartist = STR_UNKNARTIST;
	app_data.str_unkndisc = STR_UNKNDISC;
	app_data.str_unkntrk = STR_UNKNTRK;
	app_data.str_fatal = STR_FATAL;
	app_data.str_warning = STR_WARNING;
	app_data.str_info = STR_INFO;
	app_data.str_nomemory = STR_NOMEMORY;
	app_data.str_nocfg = STR_NOCFG;
	app_data.str_notrom = STR_NOTROM;
	app_data.str_notscsi2 = STR_NOTSCSI2;
	app_data.str_moderr = STR_MODERR;
	app_data.str_staterr = STR_STATERR;
	app_data.str_noderr = STR_NODERR;
	app_data.str_recoverr = STR_RECOVERR;
	app_data.str_maxerr = STR_MAXERR;
	app_data.str_tmpdirerr = STR_TMPDIRERR;
	app_data.str_libdirerr = STR_LIBDIRERR;
	app_data.str_seqfmterr = STR_SEQFMTERR;
	app_data.str_invpgmtrk = STR_INVPGMTRK;
	app_data.str_longpatherr = STR_LONGPATHERR;
	app_data.str_devlist_undef = STR_DEVLIST_UNDEF;
	app_data.str_devlist_count = STR_DEVLIST_COUNT;
	app_data.str_medchg_noinit = STR_MEDCHG_NOINIT;
	app_data.str_noaudiopath = STR_NOAUDIOPATH;
	app_data.str_audiopathexists = STR_AUDIOPATHEXISTS;
	app_data.str_cddainit_fail = STR_CDDAINIT_FAIL;
	app_data.str_notregfile = STR_NOTREGFILE;
	app_data.str_noprog = STR_NOPROG;
	app_data.str_overwrite = STR_OVERWRITE;
	app_data.str_nomsg = STR_NOMSG;

	/* Initialize other app_data parameters */
	app_data.stat_interval = 260;
	app_data.ins_interval = 4000;
	app_data.startup_vol = -1;
	app_data.aux = (void *) &status;

	/* Allocate arg arrays */
	arg1 = (word32_t *) MEM_ALLOC("arg1", CDA_NARGS * sizeof(word32_t));
	arg2 = (word32_t *) MEM_ALLOC("arg2", CDA_NARGS * sizeof(word32_t));
	if (arg1 == NULL || arg2 == NULL)
		CDA_FATAL(app_data.str_nomemory);

	/* Parse command line args */
	if (!cda_parse_args(argc, argv, &cmd, arg1))
		exit(1);

	/* Seed random number generator */
	srand((unsigned) time(NULL));
	for (i = 0, cp = PROGNAME; i < 4 && *cp != '\0'; i++, cp++)
		s->aux[i] = (byte_t) *cp ^ 0xff;

	/* Debug mode start-up banner */
	DBGPRN(DBG_ALL)(errfp, "CDA %s.%s.%s DEBUG MODE\n\n",
		VERSION_MAJ, VERSION_MIN, VERSION_TEENY);

	/* Initialize libutil */
	util_init();

	/* Set library directory path */
	if ((cp = (char *) getenv("XMCD_LIBDIR")) == NULL)
		CDA_FATAL(app_data.str_libdirerr);

	if (!util_newstr(&app_data.libdir, cp))
		CDA_FATAL(app_data.str_nomemory);

	/* Paranoia: avoid overflowing buffers */
	if ((int) strlen(app_data.libdir) >= FILE_PATH_SZ)
		CDA_FATAL(app_data.str_longpatherr);

	hd = util_homedir(util_get_ouid());
	if ((int) strlen(hd) >= FILE_PATH_SZ)
		CDA_FATAL(app_data.str_longpatherr);

	/* Set up defaults */
	app_data.cdinfo_maxhist = 100;

	/* Get system common configuration parameters */
	(void) sprintf(path, SYS_CMCFG_PATH, app_data.libdir);
	di_common_parmload(path, TRUE, FALSE);

	/* Get user common configuration parameters */
	(void) sprintf(path, USR_CMCFG_PATH, hd);
	di_common_parmload(path, FALSE, FALSE);

	if (app_data.device != NULL &&
	    (int) strlen(app_data.device) >= FILE_PATH_SZ)
		CDA_FATAL(app_data.str_longpatherr);

	if ((cp = util_basename(app_data.device)) == NULL)
		CDA_FATAL("Device path basename error");

	if ((int) strlen(cp) >= FILE_BASE_SZ)
		CDA_FATAL(app_data.str_longpatherr);

	MEM_FREE(cp);

	/* Check validity of device */
	if (di_isdemo())
		cd_rdev = 0;
	else {
		if (stat(app_data.device, &stbuf) < 0) {
			(void) sprintf(errmsg, "Cannot stat %s.",
				       app_data.device);
			CDA_FATAL(errmsg);
		}
		cd_rdev = stbuf.st_rdev;
	}

	/* FIFO paths */
	(void) sprintf(spipe, "%s/send.%x", TEMP_DIR, (int) cd_rdev);
	(void) sprintf(rpipe, "%s/recv.%x", TEMP_DIR, (int) cd_rdev);

	/* Initialize CD information services */
	if (inetoffln > 0)
		app_data.cdinfo_inetoffln = (inetoffln == 1) ? FALSE : TRUE;
	(void) strcpy(cdinfo_cldata.prog, PROGNAME);
	(void) strcpy(cdinfo_cldata.user, util_loginname());
	cdinfo_cldata.isdemo = di_isdemo;
	cdinfo_cldata.curstat_addr = curstat_addr;
	cdinfo_cldata.fatal_msg = cda_fatal_msg;
	cdinfo_cldata.warning_msg = cda_warning_msg;
	cdinfo_cldata.info_msg = cda_info_msg;
	cdinfo_init(&cdinfo_cldata);

	/* Set up basic wwwWarp structure */
	cdinfo_wwwwarp_parmload();

#ifndef NOVISUAL
	if (visual) {
		/* Handle some signals */
		if (util_signal(SIGINT, onsig0) == SIG_IGN)
			(void) util_signal(SIGINT, SIG_IGN);
		if (util_signal(SIGQUIT, onsig0) == SIG_IGN)
			(void) util_signal(SIGQUIT, SIG_IGN);
		if (util_signal(SIGTERM, onsig0) == SIG_IGN)
			(void) util_signal(SIGTERM, SIG_IGN);
		(void) util_signal(SIGPIPE, onsig1);

		/* Initialize subsystems */
		cda_init(s);

		/* Start visual mode */
		cda_visual();
	}
#endif

	if (cmd == CDA_ON) {
		/* Start CDA daemon */
		(void) printf("Initializing...\n");
		if (!cda_daemon(&status))
			exit(1);
	}

	/* Initialize subsystems */
	cda_init(s);

	/* Create device list */
	cda_parse_devlist(s);

	/* Open FIFOs - command side */
	cda_sfd[1] = open(spipe, O_WRONLY
#ifdef O_NOFOLLOW
				 | O_NOFOLLOW
#endif
	);
	if (cda_sfd[1] < 0) {
		perror("CD audio: cannot open send pipe");
		CDA_FATAL(
			"Run \"cda on\" first to initialize CD audio daemon."
		);
	}
	cda_rfd[1] = open(rpipe, O_RDONLY
#ifdef O_NOFOLLOW
				 | O_NOFOLLOW
#endif
	);
	if (cda_rfd[1] < 0) {
		perror("CD audio: cannot open recv pipe for reading");
		CDA_FATAL(
			"Run \"cda on\" first to initialize CD audio daemon."
		);
	}
	cda_rfd[0] = open(rpipe, O_WRONLY
#ifdef O_NOFOLLOW
				 | O_NOFOLLOW
#endif
	);
	if (cda_rfd[0] < 0) {
		perror("CD audio: cannot open recv pipe for writing");
		CDA_FATAL(
			"Run \"cda on\" first to initialize CD audio daemon."
		);
	}

	/* Check FIFOs */
	if (fstat(cda_sfd[1], &stbuf) < 0 || !S_ISFIFO(stbuf.st_mode))
		CDA_FATAL("Send pipe error: Not a FIFO");
	if (fstat(cda_rfd[1], &stbuf) < 0 || !S_ISFIFO(stbuf.st_mode))
		CDA_FATAL("Recv pipe error: Not a FIFO");
	if (fstat(cda_rfd[0], &stbuf) < 0 || !S_ISFIFO(stbuf.st_mode))
		CDA_FATAL("Recv pipe error: Not a FIFO");

	/* Handle some signals */
	(void) util_signal(SIGINT, onintr);
	(void) util_signal(SIGQUIT, onintr);
	(void) util_signal(SIGTERM, onintr);

	ret = 0;
	for (;;) {
		/* Send the command */
		switch (cmd) {
		case CDA_ON:
			/* Send command to cda daemon */
			if (!cda_sendcmd(cmd, arg1, &retcode)) {
				(void) printf("%s\n", emsgp);
				ret = 2;
				break;
			}

			daemon_pid = (pid_t) arg1[0];

			(void) fprintf(errfp,
				    "CD audio daemon pid=%d dev=%s started.\n",
				    (int) daemon_pid, app_data.device);
			break;

		case CDA_OFF:
			/* Send command to cda daemon */
			if (!cda_sendcmd(cmd, arg1, &retcode)) {
				(void) printf("%s\n", emsgp);
				ret = 2;
				break;
			}

			daemon_pid = (pid_t) arg1[0];

			(void) fprintf(errfp,
				    "CD audio daemon pid=%d dev=%s exited.\n",
				    (int) daemon_pid, app_data.device);
			break;

		case CDA_STATUS:
			/* Send command to cda daemon */
			if (!cda_sendcmd(cmd, arg1, &retcode)) {
				(void) printf("%s\n", emsgp);
				ret = 2;
				break;
			}

			if (RD_ARG_MODE(arg1[0]) == MOD_NODISC)
				loaddb = TRUE;

			dbp->discid = arg1[6];
			s->cur_disc = arg1[7];

			switch (app_data.chg_method) {
			case CHG_SCSI_LUN:
				s->curdev = cda_devlist[s->cur_disc - 1];
				break;
			case CHG_SCSI_MEDCHG:
			case CHG_OS_IOCTL:
			case CHG_NONE:
			default:
				s->curdev = cda_devlist[0];
				break;
			}

			/* Load CD information */
			if (loaddb &&
			    RD_ARG_MODE(arg1[0]) != MOD_NODISC &&
			    dbp->discid != 0) {
				if (cda_dbload(dbp->discid, NULL, NULL, 0))
					s->qmode = QMODE_MATCH;
				else
					s->qmode = QMODE_NONE;
			}

			/* Print status */
			prn_stat(arg1);
			break;

		default:	/* All other commands */
			/* Check current status */
			if (!cda_sendcmd(CDA_STATUS, arg2, &retcode)) {
				(void) printf("%s\n", emsgp);
				ret = 2;
				break;
			}

			if (RD_ARG_MODE(arg2[0]) == MOD_NODISC)
				loaddb = TRUE;

			dbp->discid = arg2[6];
			s->cur_disc = arg2[7];

			switch (app_data.chg_method) {
			case CHG_SCSI_LUN:
				s->curdev = cda_devlist[s->cur_disc - 1];
				break;
			case CHG_SCSI_MEDCHG:
			case CHG_OS_IOCTL:
			case CHG_NONE:
			default:
				s->curdev = cda_devlist[0];
				break;
			}

			/* Load CD information */
			if (loaddb && RD_ARG_MODE(arg2[0]) != MOD_NODISC &&
			    dbp->discid != 0) {
				if ((cmd == CDA_TOC || cmd == CDA_EXTINFO ||
				     cmd == CDA_NOTES)) {
					(void) printf("Accessing CDDB... ");
					(void) fflush(stdout);

					if (cda_dbload(dbp->discid,
						       cda_match_select,
						       cda_auth, 0))
						s->qmode = QMODE_MATCH;
					else
						s->qmode = QMODE_NONE;

					(void) printf("\n\n");
				}
				else if (cmd != CDA_PLAY) {
					if (cda_dbload(dbp->discid,
						       NULL, NULL, 0))
						s->qmode = QMODE_MATCH;
					else
						s->qmode = QMODE_NONE;
				}
			}

			if (cmd == CDA_CDDBREG) {
				if (!cda_userreg(ttyfp))
					ret = 2;
				break;
			}
			else if (cmd == CDA_CDDBHINT) {
				if (!cda_cddbhint(ttyfp))
					ret = 2;
				break;
			}

			/* Send command to cda daemon */
			if (!cda_sendcmd(cmd, arg1, &retcode) &&
			    retcode != CDA_DAEMON_EXIT) {
				(void) printf("%s\n", emsgp);
				ret = 2;
				break;
			}

			/* Map the command to its print function and
			 * call it
			 */
			for (i = 0; cmd_fmtab[i].cmd != 0; i++) {
				if (cmd == cmd_fmtab[i].cmd) {
					if (cmd_fmtab[i].prnfunc != NULL)
						(*cmd_fmtab[i].prnfunc)(arg1);
					break;
				}
			}
			break;
		}

		(void) fflush(stdout);

		if (!stat_cont)
			break;

		if (cont_delay > 0)
			util_delayms(cont_delay * 1000);
	}

	/* Close FIFOs - command side */
	if (cda_sfd[1] >= 0)
		(void) close(cda_sfd[1]);
	if (cda_rfd[1] >= 0)
		(void) close(cda_rfd[1]);
	if (cda_rfd[0] >= 0)
		(void) close(cda_rfd[0]);

	exit(ret);

	/*NOTREACHED*/
}


