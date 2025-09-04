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
static char *_hist_c_ident_ = "@(#)hist.c	7.31 04/03/11";
#endif

#define _CDINFO_INTERN	/* Expose internal function protos in cdinfo.h */

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"

#define HIST_HYST	10
#define HIST_BANNER	\
"# xmcd %s.%s history file\n# %s\n#\n# \
device disc time type discid categ disc_title\n#\n"

extern appdata_t	app_data;
extern FILE		*errfp;
extern cdinfo_client_t	*cdinfo_clinfo;
extern bool_t		cdinfo_ischild;                

STATIC cdinfo_dlist_t	*cdinfo_hist_head = NULL,
						/* History list head */
			*cdinfo_hist_tail = NULL;
						/* History list tail */
STATIC cdinfo_dlist_t	*cdinfo_chgr_array = NULL;
						/* CD changer list array */
STATIC int		cdinfo_hist_mcnt = 0,	/* History list mem cnt */
			cdinfo_hist_fcnt = 0;	/* History list file cnt */


/***********************
 *  internal routines  *
 ***********************/

/*
 * cdinfo_hist_writefile
 *	Save the history file as history.bak, and then create a new
 *	history file based on the old history file's contents.
 *
 * Args:
 *	hp1 - Pointer to a cdinfo_dlist_t structure whose entry is
 *	      to be deleted from the history file.  If NULL, then
 *	      no designated entry will be deleted.
 *	hp2 - Pointer to a cdinfo_dlist_t structure whose entry is
 *	      to be appended to the history file.  If NULL, then
 *	      no designated entry will be appended.
 *	skip - Truncate the history file by deleting this number of
 *	       old entries.
 *
 * Return:
 *	Error code, or 0 for success.
 */
STATIC int
cdinfo_hist_writefile(cdinfo_dlist_t *hp1, cdinfo_dlist_t *hp2, int skip)
{
	int		i,
			n,
			ret;
	FILE		*ifp,
			*ofp;
	char		*homep,
			*histbuf,
			device[FILE_PATH_SZ],
			genre[DLIST_BUF_SZ],
			artist[DLIST_BUF_SZ],
			title[DLIST_BUF_SZ],
			path[FILE_PATH_SZ + 32],
			bakpath[FILE_PATH_SZ + 32];
	cdinfo_dlist_t	h;

	homep = util_homedir(util_get_ouid());
	if ((int) strlen(homep) >= FILE_PATH_SZ) {
		/* Path name too long */
		return OPEN_ERR;
	}

	(void) sprintf(path, USR_HIST_PATH, homep);
	(void) sprintf(bakpath, "%s.bak", path);

	/* Allocate temporary buffer */
	if ((histbuf = (char *) MEM_ALLOC("histbuf", DLIST_BUF_SZ)) == NULL)
		return MEM_ERR;

	DBGPRN(DBG_GEN)(errfp, "Writing history file %s\n", path);

	/* Rename old history file */
	(void) UNLINK(bakpath);

#if defined(USE_RENAME)

	if (rename(path, bakpath) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"    Cannot rename %s -> %s: (errno=%d)\n",
			path, bakpath, errno);
		MEM_FREE(histbuf);
		return LINK_ERR;
	}

#else	/* USE_RENAME */

	if (LINK(path, bakpath) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"    Cannot link %s -> %s: (errno=%d)\n",
			path, bakpath, errno);
		MEM_FREE(histbuf);
		return LINK_ERR;
	}

	(void) UNLINK(path);

#endif	/* USE_RENAME */

	/* Get history file handles */
	if ((ifp = fopen(bakpath, "r")) == NULL) {
		DBGPRN(DBG_GEN)(errfp,
			"    File open error: %s (errno=%d)\n",
			bakpath, errno);
		MEM_FREE(histbuf);
		return OPEN_ERR;
	}
	if ((ofp = fopen(path, "w")) == NULL) {
		DBGPRN(DBG_GEN)(errfp,
			"    File open error: %s (errno=%d)\n",
			path, errno);
		MEM_FREE(histbuf);
		return OPEN_ERR;
	}

	/* Set file permissions */
	util_setperm(path, app_data.hist_filemode);

	/* Write history file header */
	(void) fprintf(ofp, HIST_BANNER, VERSION_MAJ, VERSION_MIN, COPYRIGHT);

	/* Read from old history file */
	for (i = 1; fgets(histbuf, DLIST_BUF_SZ, ifp) != NULL; i++) {
		/* Skip comments and blank lines */
		if (histbuf[0] == '#' || histbuf[0] == '!' ||
		    histbuf[0] == '\n')
			continue;

		/* Truncate old entries if necessary */
		if (--skip > 0)
			continue;

		if ((n = sscanf(histbuf,
			   "%255s %x %x %x %x %127s %127[^\n/]/ %127[^\n]\n",
			   device,
			   &h.discno,
			   (int *) &h.time,
			   &h.type,
			   &h.discid,
			   genre,
			   artist,
			   title)) != 8) {
			if (n == 7 && artist[0] != '\0') {
				/* Old style line */
				(void) strcpy(title, artist);
				(void) strcpy(artist, "-");
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
				    "Old history file syntax error: line %d\n",
				    i);
				continue;
			}
		}
		n = strlen(artist) - 1;
		if (artist[n] == ' ')
			artist[n] = '\0';

		/* Skip deleted entry */
		if (hp1 != NULL &&
		    h.time == hp1->time && h.discid == hp1->discid)
			continue;

		/* Write to history file */
		if (fprintf(ofp, "%s", histbuf) < 0) {
			ret = errno;
			(void) fclose(ifp);
			(void) fclose(ofp);
			errno = ret;

			DBGPRN(DBG_GEN)(errfp,
				"File write error: %s (errno=%d)\n",
				path, errno);
			MEM_FREE(histbuf);
			return WRITE_ERR;
		}
	}

	/* Append new entry */
	if (hp2 != NULL) {
		ret = fprintf(
			ofp,
			"%.255s %x %x %x %08x %.63s %.127s / %.127s\n",
			hp2->device == NULL ? "-" : hp2->device,
			hp2->discno,
			(int) hp2->time,
			hp2->type,
			(int) hp2->discid,
			hp2->genre == NULL ? "-" : hp2->genre,
			hp2->artist == NULL ? "-" : hp2->artist,
			hp2->title == NULL ? "-" : hp2->title
		);
		if (ret < 0) {
			ret = errno;
			(void) fclose(ifp);
			(void) fclose(ofp);
			errno = ret;

			DBGPRN(DBG_GEN)(errfp,
				"File write error: %s (errno=%d)\n",
				path, errno);
			MEM_FREE(histbuf);
			return WRITE_ERR;
		}
	}

	/* Close file */
	if (fclose(ifp) != 0) {
		DBGPRN(DBG_GEN)(errfp, "File close error: %s (errno=%d)\n",
			bakpath, errno);
		MEM_FREE(histbuf);
		return CLOSE_ERR;
	}
	if (fclose(ofp) != 0) {
		DBGPRN(DBG_GEN)(errfp, "File close error: %s (errno=%d)\n",
			path, errno);
		MEM_FREE(histbuf);
		return CLOSE_ERR;
	}

	MEM_FREE(histbuf);
	return 0;
}


/***********************
 *   public routines   *
 ***********************/


/* cdinfo_hist_init
 *	Initialize history mechanism.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cdinfo_hist_init(void)
{
	int		i,
			n;
	FILE		*fp;
	char		*histbuf,
			*homep,
			*p,
			device[FILE_PATH_SZ],
			genre[DLIST_BUF_SZ],
			artist[DLIST_BUF_SZ],
			title[DLIST_BUF_SZ],
			path[FILE_PATH_SZ + 32];
	cdinfo_dlist_t	h,
			*hp,
			*nhp;
#ifndef __VMS
	pid_t		cpid;
	waitret_t	wstat;
	int		ret,
			saverr,
			pfd[2];
	size_t		cnt;

	if (app_data.histfile_dsbl)
		return;

	if (PIPE(pfd) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"cdinfo_hist_init: pipe failed (errno=%d)\n",
			errno);
		return;
	}

	homep = util_homedir(util_get_ouid());
	if ((int) strlen(homep) >= FILE_PATH_SZ)
		/* Path name too long */
		return;

	(void) sprintf(path, USR_HIST_PATH, homep);

	switch (cpid = FORK()) {
	case 0:
		/* Child */
		cdinfo_ischild = TRUE;

		(void) util_signal(SIGPIPE, SIG_IGN);

		/* Close un-needed pipe descriptor */
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) close(pfd[1]);
			_exit(1);
		}

		/* Allocate temporary buffer */
		if ((histbuf = (char *) MEM_ALLOC("histbuf",
					      DLIST_BUF_SZ)) == NULL) {
			(void) close(pfd[1]);
			_exit(2);
		}

		DBGPRN(DBG_GEN)(errfp, "Loading history: %s\n", path);
		if ((fp = fopen(path, "r")) == NULL) {
			DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n", path);
			(void) close(pfd[1]);
			_exit(0);
		}
		
		n = 0;
		while (fgets(histbuf, DLIST_BUF_SZ, fp) != NULL) {
			cnt = strlen(histbuf);
			p = histbuf;
			while (cnt != 0) {
				if ((n = write(pfd[1], p, cnt)) < 0)
					break;
				cnt -= n;
				p += n;
			}
		}
		if (n >= 0)
			n = write(pfd[1], ".\n", 2);

		if (n < 0)
			saverr = errno;

		(void) fclose(fp);
		(void) close(pfd[1]);

		if (n < 0) {
			DBGPRN(DBG_GEN)(errfp,
				"cdinfo_hist_init: "
				"pipe write error (errno=%d)\n",
				saverr
			);
		}
		_exit((n > 0) ? 0 : 1);
		/*NOTREACHED*/

	case -1:
		DBGPRN(DBG_GEN)(errfp,
			"cdinfo_hist_init: fork failed (errno=%d)\n", errno);
		(void) close(pfd[0]);
		(void) close(pfd[1]);
		return;

	default:
		/* Parent */

		/* Close un-needed pipe descriptor */
		(void) close(pfd[1]);

		if ((fp = fdopen(pfd[0], "r")) == NULL) {
			DBGPRN(DBG_GEN)(errfp,
			    "cdinfo_hist_init: read pipe fdopen failed\n");
			return;
		}
		break;
	}
#else
	homep = util_homedir(util_get_ouid());
	if (strlen(homep) >= FILE_PATH_SZ)
		/* Path name too long */
		return;

	(void) sprintf(path, USR_HIST_PATH, homep);
	DBGPRN(DBG_GEN)(errfp, "Loading history: %s\n", path);
	if ((fp = fopen(path, "r")) == NULL) {
		DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n", path);
		return;
	}
#endif	/* __VMS */

	/* Allocate temporary buffer */
	if ((histbuf = (char *) MEM_ALLOC("histbuf", DLIST_BUF_SZ)) == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return;
	}

	/* Free old list, if any */
	for (hp = nhp = cdinfo_hist_head; hp != NULL; hp = nhp) {
		if (hp->device != NULL)
			MEM_FREE(hp->device);
		if (hp->genre != NULL)
			MEM_FREE(hp->genre);
		if (hp->artist != NULL)
			MEM_FREE(hp->artist);
		if (hp->title != NULL)
			MEM_FREE(hp->title);
		nhp = hp->next;
		MEM_FREE(hp);
	}
	cdinfo_hist_head = NULL;
	cdinfo_hist_mcnt = cdinfo_hist_fcnt = 0;

	h.device = device;
	h.genre = genre;
	h.artist = artist;
	h.title = title;

	/* Read in history file */
	for (i = 1; fgets(histbuf, DLIST_BUF_SZ, fp) != NULL; i++) {
		/* Skip comments and blank lines */
		if (histbuf[0] == '#' || histbuf[0] == '!' ||
		    histbuf[0] == '\n')
			continue;
		/* Done */
		if (histbuf[0] == '.' && histbuf[1] == '\n')
			break;

		if ((n = sscanf(histbuf,
			   "%255s %x %x %x %x %127s %127[^\n/]/ %127[^\n]\n",
			   h.device,
			   &h.discno,
			   (int *) &h.time,
			   &h.type,
			   &h.discid,
			   h.genre,
			   h.artist,
			   h.title)) != 8) {
			if (n == 7 && h.artist[0] != '\0') {
				/* Old style line */
				(void) strcpy(h.title, h.artist);
				(void) strcpy(h.artist, "-");
			}
			else {
				DBGPRN(DBG_GEN)(errfp,
					"History file syntax error: line %d\n",
					i);
				continue;
			}
		}
		h.device[FILE_PATH_SZ - 1] = '\0';
		h.genre[DLIST_BUF_SZ - 1] = '\0';
		h.artist[DLIST_BUF_SZ - 1] = '\0';
		h.title[DLIST_BUF_SZ - 1] = '\0';
		n = strlen(h.artist) - 1;
		if (h.artist[n] == ' ')
			h.artist[n] = '\0';

		cdinfo_hist_fcnt++;

		if (++cdinfo_hist_mcnt > app_data.cdinfo_maxhist) {
			if (cdinfo_hist_tail == NULL)
				break;	/* Empty list: Cannot continue */

			/* Max entries exceeded: circulate around and
			 * re-use an existing structure.
			 */
			hp = cdinfo_hist_tail;
			cdinfo_hist_tail = hp->prev;
			cdinfo_hist_tail->next = NULL;

			if (hp->device != NULL)
				MEM_FREE(hp->device);
			if (hp->genre != NULL)
				MEM_FREE(hp->genre);
			if (hp->artist != NULL)
				MEM_FREE(hp->artist);
			if (hp->title != NULL)
				MEM_FREE(hp->title);

			if (cdinfo_hist_mcnt > 0)
				cdinfo_hist_mcnt--;
		}
		else {
			/* Allocate new entry */
			hp = (cdinfo_dlist_t *) MEM_ALLOC(
				"cdinfo_hist",
				sizeof(cdinfo_dlist_t)
			);
			if (hp == NULL) {
				CDINFO_FATAL(app_data.str_nomemory);
				return;
			}
		}

		*hp = h;	/* Structure copy */
		hp->device = hp->genre = hp->artist = hp->title = NULL;

		p = (strcmp(h.device, "-") == 0) ? NULL : h.device;
		if (!util_newstr(&hp->device, p)) {
			CDINFO_FATAL(app_data.str_nomemory);
			return;
		}

		p = (strcmp(h.genre, "-") == 0) ? NULL : h.genre;
		if (!util_newstr(&hp->genre, p)) {
			CDINFO_FATAL(app_data.str_nomemory);
			return;
		}

		p = (strcmp(h.artist, "-") == 0) ? NULL : h.artist;
		if (!util_newstr(&hp->artist, p)) {
			CDINFO_FATAL(app_data.str_nomemory);
			return;
		}

		p = (strcmp(h.title, "-") == 0) ? NULL : h.title;
		if (!util_newstr(&hp->title, p)) {
			CDINFO_FATAL(app_data.str_nomemory);
			return;
		}

		hp->next = cdinfo_hist_head;
		hp->prev = NULL;
		if (cdinfo_hist_head != NULL)
			cdinfo_hist_head->prev = hp;
		else
			cdinfo_hist_tail = hp;
		cdinfo_hist_head = hp;
	}

	(void) fclose(fp);
	MEM_FREE(histbuf);

#ifndef __VMS
	ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			     NULL, 0, FALSE, &wstat);
	if (ret < 0) {
		DBGPRN(DBG_GEN)(errfp, "cdinfo_hist_init: "
				"waitpid failed (errno=%d)\n", errno);
	}
	else if (WIFEXITED(wstat)) {
		if (WEXITSTATUS(wstat) != 0) {
			DBGPRN(DBG_GEN)(errfp, "cdinfo_hist_init: "
					"child exited (status=%d)\n",
					WEXITSTATUS(wstat));
		}
	}
	else if (WIFSIGNALED(wstat)) {
		DBGPRN(DBG_GEN)(errfp, "cdinfo_hist_init: "
				"child killed (signal=%d)\n",
				WTERMSIG(wstat));
	}
#endif
}


/*
 * cdinfo_hist_addent
 *	Add one entry to the in-core history list and to the user's
 *	history file.
 *
 * Args:
 *	hp - Pointer to the filled cdinfo_dlist_t structure containing
 *	     information to save.
 *	updfile - Whether to update the history file
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
cdinfo_ret_t
cdinfo_hist_addent(cdinfo_dlist_t *hp, bool_t updfile)
{
	int		ret,
			skip;
	cdinfo_dlist_t	*hp2;
	FILE		*fp;
	char		*homep,
			path[FILE_PATH_SZ + 32];
	cdinfo_ret_t	retcode;
	bool_t		newfile,
			wholefile;
	struct stat	stbuf;

	if (cdinfo_hist_head != NULL &&
	    cdinfo_hist_head->discid == hp->discid) {
		/* Same disc as the most recent item on history list:
		 * no need to add.
		 */
		return 0;
	}

	/*
	 * Add to in-core list
	 */

	if (++cdinfo_hist_mcnt > app_data.cdinfo_maxhist) {
		if (cdinfo_hist_tail == NULL)
			return 0;	/* Empty list: Cannot continue */

		/* Max entries exceeded: circulate around and
		 * re-use an existing structure.
		 */
		hp2 = cdinfo_hist_tail;
		cdinfo_hist_tail = hp2->prev;
		cdinfo_hist_tail->next = NULL;

		if (hp2->device != NULL)
			MEM_FREE(hp2->device);
		if (hp2->genre != NULL)
			MEM_FREE(hp2->genre);
		if (hp2->artist != NULL)
			MEM_FREE(hp2->artist);
		if (hp2->title != NULL)
			MEM_FREE(hp2->title);

		if (cdinfo_hist_mcnt > 0)
			cdinfo_hist_mcnt--;
	}
	else {
		/* Allocate new entry */
		hp2 = (cdinfo_dlist_t *) MEM_ALLOC(
			"cdinfo_hist",
			sizeof(cdinfo_dlist_t)
		);
		if (hp2 == NULL) {
			CDINFO_FATAL(app_data.str_nomemory);
			return 0;
		}
	}

	*hp2 = *hp;	/* Structure copy */
	hp2->device = hp2->genre = hp2->artist = hp2->title = NULL;

	if (!util_newstr(&hp2->device, hp->device)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	if (!util_newstr(&hp2->genre, hp->genre)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	if (!util_newstr(&hp2->artist, hp->artist)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	if (!util_newstr(&hp2->title, hp->title)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	hp2->next = cdinfo_hist_head;
	hp2->prev = NULL;
	if (cdinfo_hist_head != NULL)
		cdinfo_hist_head->prev = hp2;
	else
		cdinfo_hist_tail = hp2;
	cdinfo_hist_head = hp2;

	if (app_data.histfile_dsbl || !updfile)
		return 0;

	cdinfo_hist_fcnt++;

	skip = 0;
	if ((cdinfo_hist_fcnt - cdinfo_hist_mcnt) > HIST_HYST) {
		/* Write out the whole file */
		skip = cdinfo_hist_fcnt - cdinfo_hist_mcnt + 1;
		cdinfo_hist_fcnt = cdinfo_hist_mcnt;
		wholefile = TRUE;
	}
	else
		wholefile = FALSE;

	/*
	 * Write out one entry to file
	 */

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	if (wholefile) {
		/* Write out the whole file */
		ret = cdinfo_hist_writefile(NULL, hp, skip);
		CH_RET(ret);
	}

	homep = util_homedir(util_get_ouid());
	if ((int) strlen(homep) >= FILE_PATH_SZ)
		/* Path name too long */
		CH_RET(OPEN_ERR);

	(void) sprintf(path, USR_HIST_PATH, homep);

	DBGPRN(DBG_GEN)(errfp, "Writing history file %s\n", path);

	/* Check the path */
	newfile = FALSE;
	if (stat(path, &stbuf) < 0) {
		if (errno == ENOENT)
			newfile = TRUE;
		else {
			DBGPRN(DBG_GEN)(errfp,
				"    Cannot stat %s (errno=%d)\n",
				path, errno);
			CH_RET(OPEN_ERR);
		}
	}
	else {
		if (!S_ISREG(stbuf.st_mode)) {
			DBGPRN(DBG_GEN)(errfp,
				"    Not a regular file error: %s\n", path);
			CH_RET(OPEN_ERR);
		}

		if (stbuf.st_size == 0)
			newfile = TRUE;
	}

	/* Get a history file handle */
	if ((fp = fopen(path, "a")) == NULL) {
		DBGPRN(DBG_GEN)(errfp, "    Cannot open file: %s\n", path);
		CH_RET(OPEN_ERR);
	}

	/* Set file permissions */
	util_setperm(path, app_data.hist_filemode);

	if (newfile)
		/* Write history file header */
		(void) fprintf(fp, HIST_BANNER,
			       VERSION_MAJ, VERSION_MIN, COPYRIGHT);

	/* Write to history file */
	ret = fprintf(
		fp,
		"%.255s %x %x %x %08x %.63s %.127s / %.127s\n",
		hp->device == NULL ? "-" : hp->device,
		hp->discno,
		(int) hp->time,
		hp->type,
		(int) hp->discid,
		hp->genre == NULL ? "-" : hp->genre,
		hp->artist == NULL ? "-" : hp->artist,
		hp->title == NULL ? "-" : hp->title
	);

	if (ret < 0) {
		ret = errno;
		(void) fclose(fp);
		errno = ret;

		DBGPRN(DBG_GEN)(errfp, "File write error: %s (errno=%d)\n",
			path, errno);
		CH_RET(WRITE_ERR);
	}

	/* Close file */
	if (fclose(fp) != 0) {
		DBGPRN(DBG_GEN)(errfp, "File close error: %s (errno=%d)\n",
			path, errno);
		CH_RET(CLOSE_ERR);
	}

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_hist_delent
 *	Delete one entry from the in-core history list and from the user's
 *	history file.
 *
 * Args:
 *	hp - Pointer to the filled cdinfo_dlist_t structure containing
 *	     information to delete.
 *	updfile - Whether to update the history file.
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
cdinfo_ret_t
cdinfo_hist_delent(cdinfo_dlist_t *hp, bool_t updfile)
{
	int		ret,
			skip;
	cdinfo_dlist_t	*hp2,
			h;
	cdinfo_ret_t	retcode;

	h = *hp;	/* Structure copy */

	/*
	 * Delete from in-core list
	 */
	for (hp2 = cdinfo_hist_head; hp2 != NULL; hp2 = hp2->next) {
		if (hp2->time == hp->time && hp2->discid == hp->discid) {
			if (hp2 == cdinfo_hist_head)
				cdinfo_hist_head = hp2->next;
			else
				hp2->prev->next = hp2->next;

			if (hp2->next != NULL)
				hp2->next->prev = hp2->prev;

			if (hp2->device != NULL)
				MEM_FREE(hp2->device);
			if (hp2->genre != NULL)
				MEM_FREE(hp2->genre);
			if (hp2->artist != NULL)
				MEM_FREE(hp2->artist);
			if (hp2->title != NULL)
				MEM_FREE(hp2->title);
			MEM_FREE(hp2);

			if (cdinfo_hist_mcnt > 0)
				cdinfo_hist_mcnt--;
			break;
		}
	}

	if (app_data.histfile_dsbl || !updfile)
		return 0;

	/*
	 * Delete from history file
	 */

	skip = cdinfo_hist_fcnt - cdinfo_hist_mcnt;
	cdinfo_hist_fcnt = cdinfo_hist_mcnt;

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	/* Write the history file */
	ret = cdinfo_hist_writefile(&h, NULL, skip);

	/* Child exits here */
	CH_RET(ret);
	/*NOTREACHED*/
}


/*
 * cdinfo_hist_delall
 *	Delete the user's entire xmcd history from the in-core history list
 *	and the user's history file.
 *
 * Args:
 *	updfile - Whether to update the history file.
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
cdinfo_ret_t
cdinfo_hist_delall(bool_t updfile)
{
	cdinfo_dlist_t	*hp,
			*nhp;
	char		*homep,
			path[FILE_PATH_SZ + 32],
			bakpath[FILE_PATH_SZ + 32];
	cdinfo_ret_t	retcode;

	/* Delete in-core list */
	for (hp = nhp = cdinfo_hist_head; hp != NULL; hp = nhp) {
		nhp = hp->next;
		if (hp->device != NULL)
			MEM_FREE(hp->device);
		if (hp->genre != NULL)
			MEM_FREE(hp->genre);
		if (hp->artist != NULL)
			MEM_FREE(hp->artist);
		if (hp->title != NULL)
			MEM_FREE(hp->title);
		MEM_FREE(hp);
	}
	cdinfo_hist_head = NULL;
	cdinfo_hist_mcnt = cdinfo_hist_fcnt = 0;

	if (app_data.histfile_dsbl || !updfile)
		return 0;

	/* Clear history file */
	homep = util_homedir(util_get_ouid());
	if ((int) strlen(homep) >= FILE_PATH_SZ)
		/* Path name too long */
		return CDINFO_SET_CODE(OPEN_ERR, ENAMETOOLONG);

	(void) sprintf(path, USR_HIST_PATH, homep);
	(void) sprintf(bakpath, "%s.bak", path);

	if (cdinfo_forkwait(&retcode))
		return (retcode);

	DBGPRN(DBG_GEN)(errfp, "Deleting history file %s\n", path);

	/* Rename old history file */
	(void) UNLINK(bakpath);

#if defined(USE_RENAME)

	if (rename(path, bakpath) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"Cannot rename file %s -> %s (errno=%d)\n",
			path, bakpath, errno);
		CH_RET(LINK_ERR);
	}

#else	/* USE_RENAME */

	if (LINK(path, bakpath) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"Cannot link file %s -> %s (errno=%d)\n",
			path, bakpath, errno);
		CH_RET(LINK_ERR);
	}

	(void) UNLINK(path);

#endif	/* USE_RENAME */

	/* Child exits here */
	CH_RET(0);
	/*NOTREACHED*/
}


/*
 * cdinfo_hist_list
 *	Return the head of the xmcd history list.
 *
 * Args:
 *	None.
 *
 * Return:
 *	A pointer to the head of the xmcd history list, or NULL if the list
 *	is empty.
 */
cdinfo_dlist_t *
cdinfo_hist_list(void)
{
	return (cdinfo_hist_head);
}


/*
 * cdinfo_chgr_init
 */
void
cdinfo_chgr_init(void)
{
	int		i,
			lastdisc,
			allocsz;

	/* Allocate array of cdinfo_dlist_t pointers */
	if ((allocsz = sizeof(cdinfo_dlist_t) * app_data.numdiscs) <= 0)
		return;

	cdinfo_chgr_array = (cdinfo_dlist_t *) MEM_ALLOC(
		"cdinfo_chgr_array", allocsz
	);
	if (cdinfo_chgr_array == NULL) {
		CDINFO_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memset(cdinfo_chgr_array, 0, allocsz);

	lastdisc = app_data.numdiscs - 1;
	for (i = 0; i < app_data.numdiscs; i++) {
		cdinfo_chgr_array[i].discno = i+1;

		if (i == lastdisc)
			cdinfo_chgr_array[i].next = NULL;
		else
			cdinfo_chgr_array[i].next = &cdinfo_chgr_array[i+1];

		if (i == 0)
			cdinfo_chgr_array[i].prev = NULL;
		else
			cdinfo_chgr_array[i].prev = &cdinfo_chgr_array[i-1];
	}
}


/*
 * cdinfo_chgr_addent
 *	Add one entry to the CD changer list
 *
 * Args:
 *	hp - Pointer to the filled cdinfo_dlist_t structure containing
 *	     information to add.
 *
 * Return:
 *	return code as defined by cdinfo_ret_t
 */
cdinfo_ret_t
cdinfo_chgr_addent(cdinfo_dlist_t *cp)
{
	int	i;

	if (cdinfo_chgr_array == NULL)
		/* Not yet initialized */
		return 0;

	if (cp->discno <= 0 || cp->discno > app_data.numdiscs)
		return CDINFO_SET_CODE(CMD_ERR, EINVAL);

	i = cp->discno - 1;

	/* Don't do a structure copy here, or we'd mess up the list links */
	cdinfo_chgr_array[i].discno = cp->discno;
	cdinfo_chgr_array[i].type = cp->type;
	cdinfo_chgr_array[i].discid = cp->discid;
	cdinfo_chgr_array[i].time = cp->time;

	if (!util_newstr(&cdinfo_chgr_array[i].device, cp->device)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	if (!util_newstr(&cdinfo_chgr_array[i].genre, cp->genre)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	if (!util_newstr(&cdinfo_chgr_array[i].artist, cp->artist)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	if (!util_newstr(&cdinfo_chgr_array[i].title, cp->title)) {
		CDINFO_FATAL(app_data.str_nomemory);
		return CDINFO_SET_CODE(MEM_ERR, 0);
	}

	return 0;
}


/*
 * cdinfo_chgr_list
 *	Return the head of the CD changer list.
 *
 * Args:
 *	None.
 *
 * Return:
 *	A pointer to the head of the CD changer list, or NULL if the list
 *	is empty.
 */
cdinfo_dlist_t *
cdinfo_chgr_list(void)
{
	return (&cdinfo_chgr_array[0]);
}


