/*
 *   cdda - CD Digital Audio support
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
 */
#ifndef lint
static char *_wr_gen_c_ident_ = "@(#)wr_gen.c	7.164 04/03/17";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "cdda_d/common.h"

#ifdef CDDA_SUPPORTED

#include "cdinfo_d/cdinfo.h"
#include "cdda_d/wr_gen.h"
#include "cdda_d/au.h"
#include "cdda_d/wav.h"
#include "cdda_d/aiff.h"
#include "cdda_d/if_lame.h"
#include "cdda_d/if_vorb.h"
#include "cdda_d/if_flac.h"
#include "cdda_d/if_faac.h"
#include "cdda_d/id3map.h"


#ifdef __VMS
#include <unixlib.h>
/* The C headers exclude fsync for _POSIX_C_SOURCE and _ANSI_C_SOURCE */
extern int	fsync(int);
/* The unixlib.h header file has these only on the later releases
 * of Compaq C, but the functions were available earlier.
 */
extern int	decc$set_child_standard_streams(int, int, int);
extern int	decc$write_eof_to_mbx(int);
#endif


/* Defines used by gen_open_pipe() */
#define STR_SHPATH	"/bin/sh"		/* Path to shell */
#define STR_SHNAME	"sh"			/* Name of shell */
#define STR_SHARG	"-c"			/* Shell arg */


extern appdata_t	app_data;
extern FILE		*errfp;
extern cdda_client_t	*cdda_clinfo;

#ifdef HAS_LAME
extern char		*lamepath;
#endif

#ifdef HAS_FAAC
extern char		*faacpath;
#endif


STATIC uid_t		ruid,			/* Real user id */
			euid;			/* Effective user id */
STATIC gid_t		rgid,			/* Real group id */
			egid;			/* Effective group id */

char			*tagcomment =
			"xmcd-" VERSION_MAJ "." VERSION_MIN "." VERSION_TEENY;
						/* Tag comment string */

/*********************
 * Private functions *
 *********************/


#ifdef __VMS
/*
 * gen_exec_vms
 *	Parse a command string, create an argv array from it, and pass it
 *	to execvp() for execution.
 *
 * Args:
 *	cmd - The command string.
 *
 * Return:
 *	On success, this function does not return.
 */
STATIC void
gen_exec_vms(char *cmd)
{
	char	**largv = NULL,
		*cmd_copy;
	int	largc = 0,
		i,
		j,
		k,
		l,
		c,
		saverr = 0;
	bool_t	white = FALSE,
		string = FALSE,
		quote = FALSE;

	l = strlen(cmd);
	if ((cmd_copy = (char *) MEM_ALLOC("cmd_copy", l + 1)) == NULL) {
		(void) fprintf(errfp, "gen_exec_vms: Out of memory.\n");
		errno = ENOMEM;
		return;
	}

	for (i = j = k = 0; (c = cmd[i]) != '\0'; i++) {
		switch (c) {
		case '"':
			if (quote) {
				cmd_copy[k++] = c;
				quote = FALSE;
			}
			else
				string = !string;
			break;
		case '\\':
			if (quote) {
				cmd_copy[k++] = c;
				quote = FALSE;
			}
			else
				quote = TRUE;
			break;
		case ' ':
		case '\t':
			if (string || quote) {
				cmd_copy[k++] = c;

				if (quote)
					quote = FALSE;
			}
			else {
				if (white)
					continue;

				cmd_copy[k++] = 0;

				largv = (char **) MEM_REALLOC("largv",
					largv, (largc+2) * sizeof(*largv)
				);

				largv[largc] = &cmd_copy[j];
				largc++;
				white = TRUE;
			}
			break;
		default:
			if (white) {
				j = k;
				white = FALSE;
			}

			quote = FALSE;
			cmd_copy[k++] = c;
			break;
		}
	}
	cmd_copy[k] = '\0';

	if (cmd_copy[0] != '\0') {
		if (!white) {
			largv = (char **) MEM_REALLOC("largv",
				largv, (largc + 2) * sizeof(*largv)
			);
			largv[largc] = &cmd_copy[j];
			largc++;
		}
		largv[largc] = NULL;

		if (app_data.debug & DBG_GEN) {
			(void) fprintf(errfp, "gen_vms_exec: cmd='%s'\n", cmd);
			for (i = 0; largv[i]; i++) {
				(void) fprintf(errfp, "    largv[%d]='%s'\n",
						i, largv[i]);
			}
		}

		/* Exec the program */
		(void) execvp(largv[0], largv);
		saverr = errno;
	}

	MEM_FREE(largv);
	MEM_FREE(cmd_copy);
	errno = saverr;
}
#endif	/* __VMS */


/*
 * gen_write_au_hdr
 *	Writes an au file header to the output file.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
gen_write_au_hdr(gen_desc_t *gdp)
{
	au_filehdr_t	hdr;

	if (gdp->datalen == 0) {
		struct stat	stbuf;

		/* Get file information */
#ifndef NO_FSYNC
		(void) fsync(gdp->fd);
#endif
		if (fstat(gdp->fd, &stbuf) < 0) {
			(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_write_au_hdr: fstat failed (errno=%d)",
				errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}

		/* Set actual data length */
		gdp->datalen = stbuf.st_size - sizeof(hdr) -
			       strlen(tagcomment);

		/* Rewind to beginning of file */
		if (!gen_seek(gdp, (off_t) 0, SEEK_SET))
			return FALSE;
	}

	(void) memset(&hdr, 0, sizeof(hdr));

	/* Initialize an au file header. */
	hdr.au_magic = util_bswap32(AUDIO_AU_FILE_MAGIC);
	hdr.au_offset = util_bswap32(sizeof(hdr) + strlen(tagcomment));
	hdr.au_data_size = util_bswap32((word32_t) gdp->datalen);
	hdr.au_encoding = util_bswap32(AUDIO_AU_ENCODING_LINEAR_16);
	hdr.au_sample_rate = util_bswap32(44100);
	hdr.au_channels = util_bswap32(2);

	/* Write header */
	if (!gen_write_chunk(gdp, (byte_t *) &hdr, sizeof(hdr)))
		return FALSE;

	/* Write comment */
	if (!gen_write_chunk(gdp, (byte_t *) tagcomment, strlen(tagcomment)))
		return FALSE;

	return TRUE;
}


/*
 * gen_write_wav_hdr
 *	Writes an wav file header to the output file.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
gen_write_wav_hdr(gen_desc_t *gdp)
{
	wav_filehdr_t	hdr;

	if (gdp->datalen == 0) {
		struct stat	stbuf;

		/* Get file information */
#ifndef NO_FSYNC
		(void) fsync(gdp->fd);
#endif
		if (fstat(gdp->fd, &stbuf) < 0) {
			(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_write_wav_hdr: fstat failed (errno=%d)",
				errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}

		/* Set actual data length */
		gdp->datalen = stbuf.st_size - sizeof(hdr);

		/* Rewind to beginning of file */
		if (!gen_seek(gdp, (off_t) 0, SEEK_SET))
			return FALSE;
	}

	(void) memset(&hdr, 0, sizeof(hdr));

	/* RIFF chunk */
	(void) strncpy(hdr.r_riff, "RIFF", 4);
	hdr.r_length = util_lswap32((word32_t) gdp->datalen + 36);
	(void) strncpy(hdr.r_wave, "WAVE", 4);

	/* FORMAT chunk */
	(void) strncpy(hdr.f_format, "fmt ", 4);
	hdr.f_length = util_lswap32(16);
	hdr.f_const = util_lswap16(1);
	hdr.f_channels = util_lswap16(2);
	hdr.f_sample_rate = util_lswap32(44100);
	hdr.f_bytes_per_s = util_lswap32(44100 * 2 * 2);
	hdr.f_block_align = util_lswap16(4);
	hdr.f_bits_per_sample = util_lswap16(16);

	/* DATA chunk */
	(void) strncpy(hdr.d_data, "data", 4);
	hdr.d_length = util_lswap32((word32_t) gdp->datalen);

	/* Write header */
	if (!gen_write_chunk(gdp, (byte_t *) &hdr, sizeof(hdr)))
		return FALSE;

	return TRUE;
}


/*
 * gen_write_aiff_hdr
 *	Writes an aiff file header to the output file.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
gen_write_aiff_hdr(gen_desc_t *gdp)
{
	word32_t	tmp;
	aiff_filehdr_t	hdr;

	if (gdp->datalen == 0) {
		struct stat	stbuf;

		/* Get file information */
#ifndef NO_FSYNC
		(void) fsync(gdp->fd);
#endif
		if (fstat(gdp->fd, &stbuf) < 0) {
			(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_write_aiff_hdr: fstat failed (errno=%d)",
				errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}

		/* Set actual data length */
		gdp->datalen = stbuf.st_size - AIFF_HDRSZ;

		/* Rewind to beginning of file */
		if (!gen_seek(gdp, (off_t) 0, SEEK_SET))
			return FALSE;
	}

	(void) memset(&hdr, 0, sizeof(hdr));

	/* FORM chunk */
	(void) strncpy(hdr.a_form, "FORM", 4);
	tmp = (word32_t) gdp->datalen + AIFF_HDRSZ - 8;
	hdr.a_length[0] = (tmp & 0xff000000) >> 24;
	hdr.a_length[1] = (tmp & 0x00ff0000) >> 16;
	hdr.a_length[2] = (tmp & 0x0000ff00) >> 8;
	hdr.a_length[3] = (tmp & 0x000000ff);
	(void) strncpy(hdr.a_aiff, "AIFF", 4);

	/* COMM chunk */
	(void) strncpy(hdr.c_comm, "COMM", 4);
	tmp = 18;
	hdr.c_length[0] = (tmp & 0xff000000) >> 24;
	hdr.c_length[1] = (tmp & 0x00ff0000) >> 16;
	hdr.c_length[2] = (tmp & 0x0000ff00) >> 8;
	hdr.c_length[3] = (tmp & 0x000000ff);
	tmp = 2;
	hdr.c_channels[0] = (tmp & 0xff00) >> 8;
	hdr.c_channels[1] = (tmp & 0x00ff);
	tmp = (word32_t) gdp->datalen >> 2;
	hdr.c_frames[0] = (tmp & 0xff000000) >> 24;
	hdr.c_frames[1] = (tmp & 0x00ff0000) >> 16;
	hdr.c_frames[2] = (tmp & 0x0000ff00) >> 8;
	hdr.c_frames[3] = (tmp & 0x000000ff);
	tmp = 16;
	hdr.c_sample_size[0] = (tmp & 0xff00) >> 8;
	hdr.c_sample_size[1] = (tmp & 0x00ff);
	(void) strncpy(hdr.c_sample_rate, "@\016\254D\0\0\0\0\0\0", 10);
				/* 44100 in 80-bit IEEE floating point */

	/* SSND chunk */
	(void) strncpy(hdr.s_ssnd, "SSND", 4);
	tmp = (word32_t) gdp->datalen + 8;
	hdr.s_length[0] = (tmp & 0xff000000) >> 24;
	hdr.s_length[1] = (tmp & 0x00ff0000) >> 16;
	hdr.s_length[2] = (tmp & 0x0000ff00) >> 8;
	hdr.s_length[3] = (tmp & 0x000000ff);

	/* Write header */
	if (!gen_write_chunk(gdp, (byte_t *) &hdr, AIFF_HDRSZ))
		return FALSE;

	return TRUE;
}


/*
 * gen_write_aifc_hdr
 *	Writes an aifc file header to the output file.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
gen_write_aifc_hdr(gen_desc_t *gdp)
{
	word32_t	tmp;
	aifc_filehdr_t	hdr;

	if (gdp->datalen == 0) {
		struct stat	stbuf;

		/* Get file information */
#ifndef NO_FSYNC
		(void) fsync(gdp->fd);
#endif
		if (fstat(gdp->fd, &stbuf) < 0) {
			(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_write_aifc_hdr: fstat failed (errno=%d)",
				errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}

		/* Set actual data length */
		gdp->datalen = stbuf.st_size - AIFC_HDRSZ;

		/* Rewind to beginning of file */
		if (!gen_seek(gdp, (off_t) 0, SEEK_SET))
			return FALSE;
	}

	(void) memset(&hdr, 0, sizeof(hdr));

	/* FORM chunk */
	(void) strncpy(hdr.a_form, "FORM", 4);
	tmp = (word32_t) gdp->datalen + AIFC_HDRSZ - 8;
	hdr.a_length[0] = (tmp & 0xff000000) >> 24;
	hdr.a_length[1] = (tmp & 0x00ff0000) >> 16;
	hdr.a_length[2] = (tmp & 0x0000ff00) >> 8;
	hdr.a_length[3] = (tmp & 0x000000ff);
	(void) strncpy(hdr.a_aifc, "AIFC", 4);

	/* FVER chunk */
	(void) strncpy(hdr.f_fver, "FVER", 4);
	tmp = 4;
	hdr.f_length[0] = (tmp & 0xff000000) >> 24;
	hdr.f_length[1] = (tmp & 0x00ff0000) >> 16;
	hdr.f_length[2] = (tmp & 0x0000ff00) >> 8;
	hdr.f_length[3] = (tmp & 0x000000ff);
	tmp = 2726318400UL;
	hdr.f_version[0] = (tmp & 0xff000000) >> 24;
	hdr.f_version[1] = (tmp & 0x00ff0000) >> 16;
	hdr.f_version[2] = (tmp & 0x0000ff00) >> 8;
	hdr.f_version[3] = (tmp & 0x000000ff);

	/* COMM chunk */
	(void) strncpy(hdr.c_comm, "COMM", 4);
	tmp = 18;
	hdr.c_length[0] = (tmp & 0xff000000) >> 24;
	hdr.c_length[1] = (tmp & 0x00ff0000) >> 16;
	hdr.c_length[2] = (tmp & 0x0000ff00) >> 8;
	hdr.c_length[3] = (tmp & 0x000000ff);
	tmp = 2;
	hdr.c_channels[0] = (tmp & 0xff00) >> 8;
	hdr.c_channels[1] = (tmp & 0x00ff);
	tmp = (word32_t) gdp->datalen >> 2;
	hdr.c_frames[0] = (tmp & 0xff000000) >> 24;
	hdr.c_frames[1] = (tmp & 0x00ff0000) >> 16;
	hdr.c_frames[2] = (tmp & 0x0000ff00) >> 8;
	hdr.c_frames[3] = (tmp & 0x000000ff);
	tmp = 16;
	hdr.c_sample_size[0] = (tmp & 0xff00) >> 8;
	hdr.c_sample_size[1] = (tmp & 0x00ff);
	(void) strncpy(hdr.c_sample_rate, "@\016\254D\0\0\0\0\0\0", 10);
				/* 44100 in 80-bit IEEE floating point */
	(void) strncpy(hdr.c_comptype, "NONE", 4);
	hdr.c_complength = 14;
	(void) strncpy(hdr.c_compstr, "not compressed", 14);

	/* SSND chunk */
	(void) strncpy(hdr.s_ssnd, "SSND", 4);
	tmp = (word32_t) gdp->datalen + 8;
	hdr.s_length[0] = (tmp & 0xff000000) >> 24;
	hdr.s_length[1] = (tmp & 0x00ff0000) >> 16;
	hdr.s_length[2] = (tmp & 0x0000ff00) >> 8;
	hdr.s_length[3] = (tmp & 0x000000ff);

	/* Write header */
	if (!gen_write_chunk(gdp, (byte_t *) &hdr, AIFC_HDRSZ))
		return FALSE;

	return TRUE;
}


/********************
 * Public functions *
 ********************/


/*
 * gen_esc_shellquote
 *	Handle quote characters in a quoted shell command line string.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *	str - Input string
 *
 * Return:
 *	Return string.  This is a dynamically allocated string.  The caller
 *	should deallocate it via MEM_FREE when done.  A NULL is returned
 *	if the input string is NULL, or if the output string buffer cannot
 *	be allocated.
 */
char *
gen_esc_shellquote(gen_desc_t *gdp, char *str)
{
	char	*ret,
		*cp;

	if (str == NULL)
		return NULL;

#ifdef __VMS
	/* For VMS, just dup the string.  All character validity
	 * checks has been done prior to getting here.
	 */
	ret = NULL;
	if (!util_newstr(&ret, str)) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"gen_esc_shellquote: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return NULL;
	}
#else
	if ((ret = (char *) MEM_ALLOC("ret", strlen(str) + 1)) == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"gen_esc_shellquote: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return NULL;
	}

	for (cp = ret; *str != '\0'; cp++, str++) {
		if (*str == '\047')
			*cp = '`';	/* Change single quote to backquote */
		else
			*cp = *str;
	}
	*cp = '\0';
#endif

	return (ret);
}


/*
 * gen_genre_cddb2id3
 *	Map CDDB2 genre to ID3tag genre.  This function now does a sparse
 *	mapping because the two genre systems are not one-to-one mappable.
 *	Moreover this has to work for both classic CDDB and CDDB2.
 * 
 * Args:
 *	genreid - CDDB genre ID string
 *
 * Return:
 *	ID3tag genre ID string.
 */
char *
gen_genre_cddb2id3(char	*genreid)
{
	cdinfo_genre_t	*gp;
	char		*id;
	int		i;

	if ((gp = cdinfo_genre(genreid)) == NULL)
		return ("12");	/* "Other" */

	if (gp->parent == NULL)
		id = gp->id;		/* CDDB Genre */
	else
		id = gp->parent->id;	/* CDDB Subgenre */

	/* Look for mapping in the genre mapping table */
	for (i = 0; id3_gmap[i].cddbgenre != NULL; i++) {
		if (strcmp(id, id3_gmap[i].cddbgenre) == 0)
			return (id3_gmap[i].id3genre);
	}

	return ("12");	/* Other */
}


/*
 * gen_set_eid
 *	Set effective uid/gid to that of the invoking user.  This is
 *	called before accessing any device or files for security reasons.
 *
 * Args:
 *	cdp - Pointer to the cd_state_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_set_eid(cd_state_t *cdp)
{
#ifdef HAS_EUID
	if (util_seteuid(ruid) < 0 || geteuid() != ruid) {
		(void) strcpy(cdp->i->msgbuf, "gen_set_eid: Cannot set uid");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		return FALSE;
	}
	if (util_setegid(rgid) < 0 || getegid() != rgid) {
		(void) strcpy(cdp->i->msgbuf, "gen_set_eid: Cannot set gid");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		return FALSE;
	}
#endif
	return TRUE;
}


/*
 * gen_reset_eid
 *	Restore saved uid/gid after the use of gen_set_eid()..
 *
 * Args:
 *	cdp - Pointer to the cd_state_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_reset_eid(cd_state_t *cdp)
{
#ifdef HAS_EUID
	if (util_seteuid(euid) < 0 || geteuid() != euid) {
		(void) strcpy(cdp->i->msgbuf, "gen_reset_eid: Cannot set uid");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		return FALSE;
	}
	if (util_setegid(egid) < 0 || getegid() != egid) {
		(void) strcpy(cdp->i->msgbuf, "gen_reset_eid: Cannot set gid");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		return FALSE;
	}
#endif

	return TRUE;
}


/*
 * gen_open_file
 *	Open an output file.
 *
 * Args:
 *	cdp	- Pointer to the cd_state_t structure
 *	path	- File path name
 *	oflag	- Open flags
 *	mode	- Open mode, or 0 if the mode is not to be set
 *	fmt	- File header format to be written
 *	datalen	- Audio data length in bytes
 *
 * Return:
 *	Pointer to a gen_desc_t structure associated with this open,
 *	or NULL on failure.
 */
gen_desc_t *
gen_open_file(
	cd_state_t	*cdp,
	char		*path,
	int		oflag,
	mode_t		mode,
	int		fmt,
	size_t		datalen
)
{
	gen_desc_t	*gdp;
	int		fd;
	char		*oflag_str;
	struct stat	stbuf;

	if (!cdda_filefmt_supp(fmt)) {
		(void) sprintf(cdp->i->msgbuf,
			    "gen_open_file: Support for the selected file "
			    "format (%d) is not compiled-in.",
			    fmt);
		DBGPRN(DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
		return NULL;
	}

	if (!gen_set_eid(cdp))
		return NULL;

	if ((oflag & 0x03) == O_WRONLY && (oflag & O_CREAT) != 0) {
		char	*dpath;
		mode_t	dmode;

		if ((dpath = util_dirname(path)) == NULL) {
			(void) sprintf(cdp->i->msgbuf,
				"gen_open_file: Invalid directory path:\n%s",
				dpath);
			DBGPRN(DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
			(void) gen_reset_eid(cdp);
			return NULL;
		}

		if (util_dirstat(dpath, &stbuf, FALSE) < 0 ) {
			if (errno == ENOENT) {
				/* Set directory perm based on file perm */
				dmode = (mode | S_IXUSR);
				if (mode & S_IRGRP)
					dmode |= (S_IRGRP | S_IXGRP);
				if (mode & S_IWGRP)
					dmode |= (S_IWGRP | S_IXGRP);
				if (mode & S_IROTH)
					dmode |= (S_IROTH | S_IXOTH);
				if (mode & S_IWOTH)
					dmode |= (S_IWOTH | S_IXOTH);

				/* Make directory if necessary */
				if (!util_mkdir(dpath, dmode)) {
					(void) sprintf(cdp->i->msgbuf,
						"gen_open_file: "
						"Cannot create directory:\n%s",
						dpath
					);
					DBGPRN(DBG_SND)(errfp, "%s\n",
							cdp->i->msgbuf);
					MEM_FREE(dpath);
					(void) gen_reset_eid(cdp);
					return NULL;
				}
			}
			else {
				(void) sprintf(cdp->i->msgbuf,
					"gen_open_file: "
					"Cannot stat %s (errno=%d)",
					dpath,
					errno
				);
				DBGPRN(DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
				MEM_FREE(dpath);
				(void) gen_reset_eid(cdp);
				return NULL;
			}
		}
		else if (!S_ISDIR(stbuf.st_mode)) {
			(void) sprintf(cdp->i->msgbuf,
				    "gen_open_file: %s:\nNot a directory.",
				    dpath);
			DBGPRN(DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
			MEM_FREE(dpath);
			(void) gen_reset_eid(cdp);
			return NULL;
		}

		MEM_FREE(dpath);
	}

#if defined(HAS_LAME) || defined(HAS_FAAC)
	switch (fmt) {
	case FILEFMT_MP3:
	case FILEFMT_AAC:
	case FILEFMT_MP4:
		{
			char	*cmd;

			/* Use the external encoder program, pipe audio
			 * data to it.
			 */
#ifdef HAS_LAME
			if (fmt == FILEFMT_MP3 && lamepath == NULL) {
				(void) strcpy(cdp->i->msgbuf,
					    "LAME encoder program not found.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}
#endif

#ifdef HAS_FAAC
			if ((fmt == FILEFMT_AAC || fmt == FILEFMT_MP4) &&
			    faacpath == NULL) {
				(void) strcpy(cdp->i->msgbuf,
					    "FAAC encoder program not found.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}
#endif

			/* Create temporary gen_desc_t structure */
			gdp = (gen_desc_t *) MEM_ALLOC(
				"gen_desc_t", sizeof(gen_desc_t)
			);
			if (gdp == NULL) {
				(void) strcpy(cdp->i->msgbuf,
					    "gen_open_file: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}
			(void) memset(gdp, 0, sizeof(gen_desc_t));

			gdp->fd = -1;
			gdp->mode = mode;
			gdp->fmt = fmt;
			gdp->cdp = cdp;
			gdp->datalen = datalen;
			gdp->fp = NULL;

			if (!util_newstr(&gdp->path, path)) {
				MEM_FREE(gdp);
				(void) strcpy(cdp->i->msgbuf,
					    "gen_open_file: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}

			switch (fmt) {
#ifdef HAS_LAME
			case FILEFMT_MP3:
				cmd = if_lame_gencmd(gdp, lamepath);
				break;
#endif

#ifdef HAS_FAAC
			case FILEFMT_AAC:
			case FILEFMT_MP4:
				cmd = if_faac_gencmd(gdp, faacpath);
				break;
#endif

			default:
				cmd = NULL;
				break;
			}

			if (cmd == NULL) {
				MEM_FREE(gdp->path);
				MEM_FREE(gdp);
				(void) gen_reset_eid(cdp);
				return NULL;
			}

			MEM_FREE(gdp->path);
			MEM_FREE(gdp);

			/* Spawn external encoder and open pipe to it */
			gdp = gen_open_pipe(cdp, cmd, NULL, fmt, datalen);

			MEM_FREE(cmd);

			(void) gen_reset_eid(cdp);
			return (gdp);
		}
		/*NOTREACHED*/

	default:
		break;
	}
#endif	/* HAS_LAME HAS_FAAC */

	/* Open file */
	if ((fd = open(path, oflag, mode)) < 0) {
		(void) sprintf(cdp->i->msgbuf,
			    "gen_open_file: open of %s failed (errno=%d)",
			    path, errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
		(void) gen_reset_eid(cdp);
		return NULL;
	}

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nOpened [%s]\n", path);

	gdp = (gen_desc_t *) MEM_ALLOC("gen_desc_t", sizeof(gen_desc_t));
	if (gdp == NULL) {
		(void) close(fd);
		(void) strcpy(cdp->i->msgbuf, "gen_open_file: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		(void) gen_reset_eid(cdp);
		return NULL;
	}
	(void) memset(gdp, 0, sizeof(gen_desc_t));

	switch ((int) (oflag & 0x03)) {
	case O_WRONLY:
		oflag_str = "w";
		if (mode != (mode_t) 0)
			(void) chmod(path, mode);
		break;
	case O_RDWR:
		oflag_str = "r+";
		if (mode != (mode_t) 0)
			(void) chmod(path, mode);
		break;
	case O_RDONLY:
	default:
		oflag_str = "r";
		break;
	}

	gdp->fd = fd;
	gdp->mode = mode;
	gdp->fmt = fmt;
	gdp->cdp = cdp;
	gdp->datalen = datalen;

#ifdef __VMS
	gdp->fp = NULL;
#else
	if (fstat(gdp->fd, &stbuf) == 0 &&
	    (S_ISCHR(stbuf.st_mode) || S_ISBLK(stbuf.st_mode))) {
		gdp->fp = NULL;	/* Use non-stdio on devices special files */
	}
	else {
		if ((gdp->fp = fdopen(gdp->fd, oflag_str)) == NULL) {
			DBGPRN(DBG_SND)(errfp,
					"%s: fdopen failed.\n"
					"Using non-buffered file I/O.\n",
					path);
		}
		else {
			gdp->buf = (char *) MEM_ALLOC("gdp_buf", GDESC_BUFSZ);
			if (gdp->buf != NULL)
				setbuf(gdp->fp, gdp->buf);
		}
	}
#endif

	if (!util_newstr(&gdp->path, path)) {
		if (gdp->fp != NULL)
			(void) fclose(gdp->fp);
		else
			(void) close(gdp->fd);
		MEM_FREE(gdp);
		(void) strcpy(cdp->i->msgbuf, "gen_open_file: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		(void) gen_reset_eid(cdp);
		return NULL;
	}

	/* Update file header */
	switch (fmt) {
	case FILEFMT_AU:
		if (!gen_write_au_hdr(gdp)) {
			(void) close(fd);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

	case FILEFMT_WAV:
		if (!gen_write_wav_hdr(gdp)) {
			(void) close(fd);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

	case FILEFMT_AIFF:
		if (!gen_write_aiff_hdr(gdp)) {
			(void) close(fd);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

	case FILEFMT_AIFC:
		if (!gen_write_aifc_hdr(gdp)) {
			(void) close(fd);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

#ifdef HAS_VORBIS
	case FILEFMT_OGG:
		if (!if_vorbis_init(gdp)) {
			(void) close(fd);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;
#endif	/* HAS_VORBIS */

#ifdef HAS_FLAC
	case FILEFMT_FLAC:
		if (!if_flac_init(gdp)) {
			(void) close(fd);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;
#endif	/* HAS_FLAC */

	default:
		break;
	}

	(void) gen_reset_eid(cdp);
	return (gdp);
}


/*
 * gen_close_file
 *	Close a file opened with gen_open_file().
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_close_file(gen_desc_t *gdp)
{
	cd_state_t	*cdp = gdp->cdp;
	bool_t		ret;

	if (!gen_set_eid(cdp))
		return FALSE;

	gdp->datalen = 0;
	if (gdp->fp != NULL)
		(void) fflush(gdp->fp);

	switch (gdp->fmt) {
	case FILEFMT_AU:
		(void) gen_write_au_hdr(gdp);
		break;

	case FILEFMT_WAV:
		(void) gen_write_wav_hdr(gdp);
		break;

	case FILEFMT_AIFF:
		(void) gen_write_aiff_hdr(gdp);
		break;

	case FILEFMT_AIFC:
		(void) gen_write_aifc_hdr(gdp);
		break;

#ifdef HAS_LAME
	case FILEFMT_MP3:
		ret = gen_close_pipe(gdp);
		(void) gen_reset_eid(cdp);
		return (ret);
#endif	/* HAS_LAME */

#ifdef HAS_VORBIS
	case FILEFMT_OGG:
		if_vorbis_halt(gdp);
		break;
#endif	/* HAS_VORBIS */

#ifdef HAS_FLAC
	case FILEFMT_FLAC:
		if_flac_halt(gdp);
		break;
#endif	/* HAS_FLAC */

#ifdef HAS_FAAC
	case FILEFMT_AAC:
	case FILEFMT_MP4:
		ret = gen_close_pipe(gdp);
		(void) gen_reset_eid(cdp);
		return (ret);
#endif	/* HAS_FAAC */

	default:
		break;
	}

	/* Close file */
	if (gdp->fp != NULL)
		ret = (fclose(gdp->fp) == 0);
	else
		ret = (close(gdp->fd) == 0);

	if (ret) {
		DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nClosed [%s]\n", gdp->path);
	}
	else {
		DBGPRN(DBG_GEN|DBG_SND)(errfp,
			"\nClose [%s] failed: errno=%d\n",
			gdp->path, errno);
	}

	MEM_FREE(gdp->path);
	MEM_FREE(gdp);

	(void) gen_reset_eid(cdp);
	return (ret);
}


/*
 * gen_open_pipe
 *	Spawn an external program and connect a pipe to its stdin
 *
 * Args:
 *	cdp	 - Pointer to the cd_state_t structure
 *	progpath - Command line of program to spawn
 *	ret_pid	 - Child process ID return
 *	fmt	 - File header format to be written
 *	datalen	 - Audio data length in bytes
 *
 * Return:
 *	Pointer to a gen_desc_t structure associated with this open,
 *	or NULL on failure.
 */
gen_desc_t *
gen_open_pipe(
	cd_state_t	*cdp,
	char		*progpath,
	pid_t		*ret_pid,
	int		fmt,
	size_t		datalen
)
{
	gen_desc_t	*gdp;
	char		*cmd;
	pid_t		cpid;
	int		i,
			pfd[2];

	if (!cdda_filefmt_supp(fmt)) {
		(void) sprintf(cdp->i->msgbuf,
			    "gen_open_pipe: Support for the selected file "
			    "format (%d) is not compiled-in.",
			    fmt);
		DBGPRN(DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
		return NULL;
	}

	if (!gen_set_eid(cdp))
		return NULL;

	if (PIPE(pfd) < 0) {
		(void) sprintf(cdp->i->msgbuf,
				"gen_open_pipe: output pipe failed (errno=%d)",
				errno);
		DBGPRN(DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
		(void) gen_reset_eid(cdp);
		return NULL;
	}

	cmd = NULL;
	if (!util_newstr(&cmd, progpath)) {
		(void) strcpy(cdp->i->msgbuf, "gen_open_pipe: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		(void) gen_reset_eid(cdp);
		return NULL;
	}

#if defined(HAS_LAME) || defined(HAS_FAAC)
	if (ret_pid != NULL) {
		switch (fmt) {
		case FILEFMT_MP3:
		case FILEFMT_AAC:
		case FILEFMT_MP4:
			/* Use the external encoder program, pipe audio
			 * data to it.
			 */
#ifdef HAS_LAME
			if (fmt == FILEFMT_MP3 && lamepath == NULL) {
				(void) strcpy(cdp->i->msgbuf,
					    "LAME encoder program not found.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}
#endif
#ifdef HAS_FAAC
			if ((fmt == FILEFMT_AAC || fmt == FILEFMT_MP4) &&
			    faacpath == NULL) {
				(void) strcpy(cdp->i->msgbuf,
					    "FAAC encoder program not found.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}
#endif

			/* Create temporary gen_desc_t structure */
			gdp = (gen_desc_t *) MEM_ALLOC(
				"gen_desc_t", sizeof(gen_desc_t)
			);
			if (gdp == NULL) {
				(void) strcpy(cdp->i->msgbuf,
					    "gen_open_file: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}
			(void) memset(gdp, 0, sizeof(gen_desc_t));

			gdp->fd = -1;
			gdp->fmt = fmt;
			gdp->cdp = cdp;
			gdp->datalen = datalen;
			gdp->fp = NULL;
			gdp->flags |= GDESC_ISPIPE;

			if (!util_newstr(&gdp->path, progpath)) {
				MEM_FREE(gdp);
				(void) strcpy(cdp->i->msgbuf,
					    "gen_open_pipe: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
				(void) gen_reset_eid(cdp);
				return NULL;
			}

			if (cmd != NULL)
				MEM_FREE(cmd);

			switch (fmt) {
#ifdef HAS_LAME
			case FILEFMT_MP3:
				cmd = if_lame_gencmd(gdp, lamepath);
				break;
#endif

#ifdef HAS_FAAC
			case FILEFMT_AAC:
			case FILEFMT_MP4:
				cmd = if_faac_gencmd(gdp, faacpath);
				break;
#endif

			default:
				cmd = NULL;
			}

			if (cmd == NULL) {
				MEM_FREE(gdp->path);
				MEM_FREE(gdp);
				(void) gen_reset_eid(cdp);
				return NULL;
			}

			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			break;

		default:
			break;
		}
	}
#endif	/* HAS_LAME HAS_FAAC */

	DBGPRN(DBG_GEN|DBG_SND)(errfp, "\nInvoking [%s]\n", cmd);

	switch (cpid = FORK()) {
	case -1:
		(void) sprintf(cdp->i->msgbuf,
			    "gen_open_pipe: fork failed (errno=%d)",
			    errno);
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		(void) gen_reset_eid(cdp);
		return NULL;

	case 0:
		/* Child */

		(void) util_signal(SIGTERM, SIG_DFL);
		(void) util_signal(SIGALRM, SIG_DFL);
		(void) util_signal(SIGPIPE, SIG_DFL);

#ifdef __VMS
		/* Special handling to work with VMS vfork */

		/* Close on exec for unneeded pipe/file descriptors */
		(void) fcntl(pfd[1], F_SETFD, FD_CLOEXEC);

		for (i = 3; i < 255; i++) {
			if (i != pfd[0])
				(void) fcntl(i, F_SETFD, FD_CLOEXEC);
		}

		/* Connect stdin to pipe, leave stdout and stderr as is */
		if (decc$set_child_standard_streams(pfd[0], -1, -1) < 0) {
			(void) fprintf(errfp,
				"gen_open_pipe: "
				"decc$set_child_standard_streams failed "
				"(errno=%d)\n",
				errno
			);
			_exit(1);
		}

		gen_exec_vms(cmd);
#else
		/* Close unneeded pipe descriptors */
		(void) close(pfd[1]);

		for (i = 3; i < 255; i++) {
			/* Close unneeded fds */
			if (i != pfd[0])
				(void) close(i);
		}

		(void) close(0);	/* Close stdin */

		if (dup(pfd[0]) < 0) {	/* Connect stdin to pipe */
			(void) fprintf(errfp,
				    "gen_open_pipe: dup failed (errno=%d)\n",
				    errno);
			_exit(1);
		}

		/* Exec the pipe program */
		(void) execl(STR_SHPATH, STR_SHNAME, STR_SHARG, cmd, NULL);
#endif

		/* An error has occurred */
		_exit(errno);

		/*NOTREACHED*/

	default:
		/* Parent */
		if (ret_pid != NULL)
			*ret_pid = cpid;

		/* Close unneeded pipe descriptor */
		(void) close(pfd[0]);
		break;
	}

	gdp = (gen_desc_t *) MEM_ALLOC("gen_desc_t", sizeof(gen_desc_t));
	if (gdp == NULL) {
		(void) close(pfd[1]);
		(void) strcpy(cdp->i->msgbuf, "gen_open_pipe: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", cdp->i->msgbuf);
		(void) gen_reset_eid(cdp);
		return NULL;
	}
	(void) memset(gdp, 0, sizeof(gen_desc_t));

	gdp->fd = pfd[1];
	gdp->fmt = fmt;
	gdp->cdp = cdp;
	gdp->datalen = datalen;
	gdp->flags |= GDESC_ISPIPE;
	gdp->path = cmd;
	gdp->cpid = cpid;

#ifdef __VMS
	gdp->fp = NULL;
#else
	if ((gdp->fp = fdopen(gdp->fd, "w")) == NULL) {
		DBGPRN(DBG_SND)(errfp,
				"%s: fdopen failed.\n"
				"Using non-buffered file I/O.\n",
				gdp->path);
	}
	else {
		gdp->buf = (char *) MEM_ALLOC("gdp_buf", GDESC_BUFSZ);
		if (gdp->buf != NULL)
			setbuf(gdp->fp, gdp->buf);
	}
#endif

	switch (fmt) {
	case FILEFMT_AU:
		if (!gen_write_au_hdr(gdp)) {
			(void) close(pfd[1]);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

	case FILEFMT_WAV:
		if (!gen_write_wav_hdr(gdp)) {
			(void) close(pfd[1]);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

	case FILEFMT_AIFF:
		if (!gen_write_aiff_hdr(gdp)) {
			(void) close(pfd[1]);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

	case FILEFMT_AIFC:
		if (!gen_write_aifc_hdr(gdp)) {
			(void) close(pfd[1]);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;

#ifdef HAS_VORBIS
	case FILEFMT_OGG:
		if (!if_vorbis_init(gdp)) {
			(void) close(pfd[1]);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;
#endif	/* HAS_VORBIS */

#ifdef HAS_FLAC
	case FILEFMT_FLAC:
		if (!if_flac_init(gdp)) {
			(void) close(pfd[1]);
			MEM_FREE(gdp->path);
			MEM_FREE(gdp);
			(void) gen_reset_eid(cdp);
			return NULL;
		}
		break;
#endif	/* HAS_FLAC */

	default:
		break;
	}

	(void) gen_reset_eid(cdp);
	return (gdp);
}


/*
 * gen_close_pipe
 *	Close the program pipe that was opened via gen_open_pipe()
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_close_pipe(gen_desc_t *gdp)
{
	cd_state_t	*cdp = gdp->cdp;
	char		*cp;
	int		ret,
			wstat;

	if (!gen_set_eid(cdp))
		return FALSE;

	switch (gdp->fmt) {
#ifdef HAS_VORBIS
	case FILEFMT_OGG:
		if_vorbis_halt(gdp);
		break;
#endif	/* HAS_VORBIS */

#ifdef HAS_FLAC
	case FILEFMT_FLAC:
		if_flac_halt(gdp);
		break;
#endif	/* HAS_FLAC */

	default:
		break;
	}

	/* Close pipe */
	if (gdp->fp != NULL)
		(void) fclose(gdp->fp);
	else {
#ifdef __VMS
		decc$write_eof_to_mbx(gdp->fd);
#endif
		(void) close(gdp->fd);
	}

	/* Cut off anything beyond argv[0] */
	if ((cp = strchr(gdp->path, ' ')) != NULL)
		*cp = '\0';

	/* Wait for pipe program and get exit status */
	ret = util_waitchild(gdp->cpid, app_data.hb_timeout,
			     NULL, 0, TRUE, &wstat);
	if (ret < 0) {
		DBGPRN(DBG_GEN|DBG_SND)(errfp,
			    "%s: waitpid failed (pid=%d errno=%d)\n",
			    gdp->path, (int) gdp->cpid, errno);
		ret = -1;
	}
	else if (WIFEXITED(wstat)) {
		ret = WEXITSTATUS(wstat);
		if (ret == 0) {
			DBGPRN(DBG_GEN|DBG_SND)(errfp,
				"%s: Program exited (pid=%d status=%d)\n",
				gdp->path, (int) gdp->cpid, ret);
		}
		else {
			(void) sprintf(cdp->i->msgbuf,
				"NOTICE: %s\nProgram exited abnormally "
				"(pid=%d status=%d)",
				gdp->path, (int) gdp->cpid, ret
			);
			DBGPRN(DBG_GEN|DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
		}
	}
	else if (WIFSIGNALED(wstat)) {
		ret = WTERMSIG(wstat);
		(void) sprintf(cdp->i->msgbuf,
			"NOTICE: %s\nProgram killed (pid=%d status=%d)",
			gdp->path, (int) gdp->cpid, ret
		);
		DBGPRN(DBG_GEN|DBG_SND)(errfp, "%s\n", cdp->i->msgbuf);
	}

	MEM_FREE(gdp->path);
	MEM_FREE(gdp);

	(void) gen_reset_eid(cdp);
	return (ret == 0);
}


/*
 * gen_read_chunk
 *	Reads data from the opened file descriptor, catering for possible
 *	interrupts.
 *
 * Args:
 *	gdp	- Pointer to the gen_desc_t structure
 *	data	- Data buffer
 *	len	- Data length
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_read_chunk(gen_desc_t *gdp, byte_t *data, size_t len)
{
	int	offset = 0,
		req,
		n;

	if (data == NULL || len == 0)
		/* Nothing to do */
		return TRUE;

	if (!gen_set_eid(gdp->cdp))
		return FALSE;

	if ((app_data.debug & DBG_SND) != 0) {
		if ((gdp->flags & GDESC_ISPIPE) != 0) {
			char	*cp,
				sav = '\0';

			if ((cp = strchr(gdp->path, ' ')) != NULL) {
				sav = *cp;
				*cp = '\0';
			}

			(void) fprintf(errfp,
					"\nReading pipe [%s]: %d bytes\n",
					gdp->path, (int) len);

			if (cp != NULL)
				*cp = sav;
		}
		else
			(void) fprintf(errfp, "\nReading [%s]: %d bytes\n",
					gdp->path, (int) len);
	}

	/* Read in */
	do {
		errno = 0;
		req = len - offset;

		if (gdp->fp != NULL) {
			n = fread(&data[offset], 1, req, gdp->fp);
			if (n < req) {
				if (feof(gdp->fp)) {
					(void) gen_reset_eid(gdp->cdp);
					return TRUE;
				}
				else if (ferror(gdp->fp))
					break;
			}
		}
		else {
			if ((n = read(gdp->fd, &data[offset], req)) < 0)
				break;
		}

		/* Have we finished? */
		if (n == req || n == 0) {
			(void) gen_reset_eid(gdp->cdp);
			return TRUE;
		}

		offset += n;

	} while (offset < len);

	if (n >= 0) {
		(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_read_chunk: read failed (ret=%d)", n);
	}
	else {
		(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_read_chunk: read failed (errno=%d)",
				errno);
	}
	DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);

	(void) gen_reset_eid(gdp->cdp);
	return FALSE;
}


/*
 * gen_write_chunk
 *	Writes data to the opened file descriptor, catering for possible
 *	interrupts.
 *
 * Args:
 *	gdp	- Pointer to the gen_desc_t structure
 *	data	- Data buffer
 *	len	- Data length
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_write_chunk(gen_desc_t *gdp, byte_t *data, size_t len)
{
	int	offset = 0,
		req,
		n;
	bool_t	ret;

	if (!gen_set_eid(gdp->cdp))
		return FALSE;

	if (data == NULL && len == 0) {
		/* Flush buffered I/O */
		if (gdp->fp != NULL)
			(void) fflush(gdp->fp);

		/* Write EOF */
		ret = !(write(gdp->fd, NULL, 0) < 0);

		(void) gen_reset_eid(gdp->cdp);
		return (ret);
	}

	if (len == 0) {
		/* Nothing to do */
		(void) gen_reset_eid(gdp->cdp);
		return TRUE;
	}

	if ((gdp->flags & GDESC_WRITEOUT) == 0) {
		switch (gdp->fmt) {
#ifdef HAS_VORBIS
		case FILEFMT_OGG:
			ret = if_vorbis_encode_chunk(gdp, data, len);
			(void) gen_reset_eid(gdp->cdp);
			return (ret);
#endif	/* HAS_VORBIS */

#ifdef HAS_FLAC
		case FILEFMT_FLAC:
			ret = if_flac_encode_chunk(gdp, data, len);
			(void) gen_reset_eid(gdp->cdp);
			return (ret);
#endif	/* HAS_FLAC */

		default:
			break;
		}
	}

	gdp->flags &= ~GDESC_WRITEOUT;

	if ((app_data.debug & DBG_SND) != 0) {
		if ((gdp->flags & GDESC_ISPIPE) != 0) {
			char	*cp,
				sav = '\0';

			if ((cp = strchr(gdp->path, ' ')) != NULL) {
				sav = *cp;
				*cp = '\0';
			}

			(void) fprintf(errfp,
					"\nWriting pipe [%s]: %d bytes\n",
					gdp->path, (int) len);

			if (cp != NULL)
				*cp = sav;
		}
		else
			(void) fprintf(errfp, "\nWriting [%s]: %d bytes\n",
					gdp->path, (int) len);
	}

	/* Write out */
	do {
		req = len - offset;
		errno = 0;

		if (gdp->fp != NULL) {
			n = fwrite(&data[offset], 1, req, gdp->fp);
			if (n < req && ferror(gdp->fp))
				break;
		}
		else {
			if ((n = write(gdp->fd, &data[offset], req)) <= 0)
				break;
		}

		/* Have we finished? */
		if (n == req) {
			(void) gen_reset_eid(gdp->cdp);
			return TRUE;
		}

		offset += n;

	} while (offset < len);

	if (n >= 0) {
		(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_write_chunk: write failed (ret=%d)", n);
	}
	else {
		(void) sprintf(gdp->cdp->i->msgbuf,
				"gen_write_chunk: write failed (errno=%d)",
				errno);
	}
	DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);

	(void) gen_reset_eid(gdp->cdp);
	return FALSE;
}


/*
 * gen_seek
 *	Wrapper function for fseek(3) and lseek(2).  It uses the appropriate
 *	function based on whether a buffered stdio stream is opened.
 *
 * Args:
 *	gdp    - Pointer to the gen_desc_t structure
 *	offset - Seek offset
 *	whence - Seek type (SEEK_SET, SEEK_CUR or SEEK_END)
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_seek(gen_desc_t *gdp, off_t offset, int whence)
{
	off_t	ret;

	if (gdp->fp != NULL) {
		if ((ret = fseek(gdp->fp, (long) offset, whence)) < 0) {
			(void) sprintf(gdp->cdp->i->msgbuf,
				    "gen_seek: %s: fseek failed (errno=%d)",
				    gdp->path, errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		}
	}
	else {
		if ((ret = lseek(gdp->fd, offset, whence)) < 0) {
			(void) sprintf(gdp->cdp->i->msgbuf,
				    "gen_seek: %s: lseek failed (errno=%d)",
				    gdp->path, errno);
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		}
	}

	return ((bool_t) (ret >= 0));
}


#ifndef __VMS
/*
 * gen_ioctl
 *	Wrapper for ioctl(2) function.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *	req - ioctl request
 *	arg - ioctl argument
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
gen_ioctl(gen_desc_t *gdp, int req, void *arg)
{
	/* Flush buffered I/O first */
	if (gdp->fp != NULL)
		(void) fflush(gdp->fp);

	/* Do the ioctl */
	return (!(ioctl(gdp->fd, req, arg) < 0));
}
#endif


/*
 * gen_byteswap
 *	Carry out byte swapping.
 *
 * Args:
 *	srcbuf - audio data source buffer
 *	tgtbuf - audio data target buffer
 *	len    - data length in bytes
 *
 * Return:
 *	Nothing.
 */
void
gen_byteswap(byte_t *srcbuf, byte_t *tgtbuf, size_t len)
{
	int	i;

	/* Run through samples */
	for (i = 0; i < (int) len; i += 2) {
		/* Byte swapping */
		tgtbuf[i] = srcbuf[i+1];
		tgtbuf[i+1] = srcbuf[i];;
	}
}


/*
 * gen_filefmt_be
 *	Determine whether the audio data should be in big endian for the
 *	format of file and platform we're running on.
 *
 * Args:
 *	filefmt - File format code
 *
 * Return:
 *	FALSE - should be little endian
 *	TRUE  - should be big endian
 */
bool_t
gen_filefmt_be(int filefmt)
{
	switch (filefmt) {
	case FILEFMT_FLAC:
		/* Always native endian */
#if _BYTE_ORDER_ == _B_ENDIAN_
		return TRUE;
#else
		return FALSE;
#endif

	case FILEFMT_AU:
	case FILEFMT_AIFF:
	case FILEFMT_AIFC:
	case FILEFMT_MP3:
	case FILEFMT_AAC:
	case FILEFMT_MP4:
		/* Always big endian */
		return TRUE;

	case FILEFMT_WAV:
	case FILEFMT_OGG:
	default:
		/* Always little endian */
		return FALSE;
	}
}


/*
 * gen_chroute_att
 *	Other optional processing may need be carried out on the audio
 *	prior to sending it to the audio device, file and/or pipe stream.
 *
 * Args:
 *	chroute - Channel routing value:
 *	    CHROUTE_NORMAL	Leave as is
 *	    CHROUTE_REVERSE	Swaps left and right
 *	    CHROUTE_L_MONO	Feed left to both channels
 *	    CHROUTE_R_MONO	Feed right to both channels
 *	    CHROUTE_MONO	Feed left and right avrg to both channels
 *	att - Attenuation level (0-100)
 *	data - Data to transform
 *	len  - Data length in bytes
 *
 * Return:
 *	Nothing.
 */
void
gen_chroute_att(int chroute, int att, sword16_t *data, size_t len)
{
	int		nsamples,
			i,
			l,
			r;
	sword16_t	temp1,
			temp2,
			temp3;

	nsamples = (int) (len / (sizeof(sword16_t) << 1));

	switch (chroute) {
	case CHROUTE_NORMAL:
		if (att >= 100)
			break;

		for (i = 0; i < nsamples; i++) {
			l = i << 1;
			r = l + 1;

			/* Set attenuation */
			temp1 = util_lswap16(data[l]);
			temp2 = util_lswap16(data[r]);
			if (att == 0) {
				temp1 = temp2 = 0;
			}
			else {
				temp1 = (sword16_t) ((temp1 * att) / 100);
				temp2 = (sword16_t) ((temp2 * att) / 100);
			}

			data[l] = util_lswap16(temp1);
			data[r] = util_lswap16(temp2);
		}
		break;

	case CHROUTE_REVERSE:
		for (i = 0; i < nsamples; i++) {
			l = i << 1;
			r = l + 1;

			/* Set attenuation */
			temp1 = util_lswap16(data[l]);
			temp2 = util_lswap16(data[r]);
			if (att == 0) {
				temp1 = temp2 = 0;
			}
			else if (att < 100) {
				temp1 = (sword16_t) ((temp1 * att) / 100);
				temp2 = (sword16_t) ((temp2 * att) / 100);
			}

			/* Swap left and right samples */
			data[l] = util_lswap16(temp2);
			data[r] = util_lswap16(temp1);
		}
		break;

	case CHROUTE_L_MONO:
		for (i = 0; i < nsamples; i++) {
			l = i << 1;
			r = l + 1;

			/* Set attenuation */
			temp1 = util_lswap16(data[l]);
			if (att == 0)
				temp1 = 0;
			else if (att < 100)
				temp1 = (sword16_t) ((temp1 * att) / 100);

			/* Make right same as left */
			data[l] = data[r] = util_lswap16(temp1);
		}
		break;

	case CHROUTE_R_MONO:
		for (i = 0; i < nsamples; i++) {
			l = i << 1;
			r = l + 1;

			/* Set attenuation */
			temp2 = util_lswap16(data[r]);
			if (att == 0)
				temp2 = 0;
			else if (att < 100)
				temp2 = (sword16_t) ((temp2 * att) / 100);

			/* Make left same as right */
			data[l] = data[r] = util_lswap16(temp2);
		}
		break;

	case CHROUTE_MONO:
		for (i = 0; i < nsamples; i++) {
			l = i << 1;
			r = l + 1;

			/* Set attenuation */
			temp1 = util_lswap16(data[l]);
			temp2 = util_lswap16(data[r]);
			if (att == 0) {
				temp1 = temp2 = 0;
			}
			else if (att < 100) {
				temp1 = (sword16_t) ((temp1 * att) / 100);
				temp2 = (sword16_t) ((temp2 * att) / 100);
			}

			/* Average left and right */
			temp3 = util_lswap16((temp1 + temp2) >> 1);
			data[l] = data[r] = temp3;
		}
		break;

	default:
		break;
	}
}


/*
 * gen_write_init
 *	Generic write services initialization
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
gen_write_init(void)
{
	static bool_t	gen_write_initted = FALSE;

	if (!gen_write_initted) {
		gen_write_initted = TRUE;

		ruid = util_get_ouid();
		rgid = util_get_ogid();
		euid = geteuid();
		egid = getegid();
	}
}

#endif	/* CDDA_SUPPORTED */


