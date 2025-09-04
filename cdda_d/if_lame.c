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
static char *_if_lame_c_ident_ = "@(#)if_lame.c	7.12 04/03/11";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"

#if defined(CDDA_SUPPORTED) && defined(HAS_LAME)

#include "cdinfo_d/cdinfo.h"
#include "cdda_d/wr_gen.h"
#include "cdda_d/if_lame.h"


extern appdata_t	app_data;
extern FILE		*errfp;
extern cdda_client_t	*cdda_clinfo;

extern char		*tagcomment;		/* Tag comment */

STATIC unsigned int	lame_ver = 0;		/* LAME version */


#ifdef __VMS
#define MP3_RETURN(x)	return ((bool_t) ((x) == 0))
#else
#define MP3_RETURN(x)	_exit((x))
#endif


/*
 * if_lame_verchk
 *	Check and store LAME encoder program version.  This is used
 *	to deteremine command line options support in the if_lame_gencmd()
 *	function.
 *
 * Args:
 *	path - File path string to the lame executable.
 *	ver_ret - Pointer to location for the lame version.
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
bool_t
if_lame_verchk(char *path, unsigned int *ver_ret)
{
	int		ret;
	size_t		bufsz = (STR_BUF_SZ << 1);
	unsigned int	vmaj,
			vmin,
			vtiny;
	FILE		*fp;
	char		*cmd,
			*tmpbuf;
#ifndef __VMS
	int		pfd[2];
	pid_t		cpid;
	waitret_t	wstat;

	if (PIPE(pfd) < 0) {
		*ver_ret = lame_ver;
		return FALSE;
	}

	switch (cpid = FORK()) {
	case 0:
		/* Child process */

		(void) util_signal(SIGPIPE, SIG_IGN);
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid())
			_exit(1);

		break;

	case -1:
		DBGPRN(DBG_GEN)(errfp,
			"\nif_lame_verchk fork failed (errno=%d).\n",
			errno);
		*ver_ret = lame_ver;
		return FALSE;

	default:
		(void) close(pfd[1]);

		(void) read(pfd[0], &lame_ver, sizeof(lame_ver));
		*ver_ret = lame_ver;

		(void) close(pfd[0]);

		/* Parent: wait for child to finish */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     NULL, 0, FALSE, &wstat);

		if (ret < 0)
			return FALSE;
		else if (WIFEXITED(wstat))
			return ((bool_t) (WEXITSTATUS(wstat) == 0));
		else if (WIFSIGNALED(wstat))
			return FALSE;
	}
#endif

	/* Run the lame program with the --help command line arg,
	 * which will cause it to output a usage help.  Pipe that
	 * into our stdin.  Note that if the lame usage help output
	 * format changes drastically this code may break.
	 */

	/* Allocate temporary buffer */
	cmd = (char *) MEM_ALLOC("cmd", strlen(path) + (STR_BUF_SZ << 1));
	if (cmd == NULL) {
		*ver_ret = lame_ver;
		DBGPRN(DBG_GEN)(errfp, "\nOut of memory.\n");
		MP3_RETURN(2);
	}

#ifndef __VMS
	(void) sprintf(cmd, "%s --help 2>&1", path);
#else
	(void) sprintf(cmd,
		"pipe ( define sys$error sys$output && mcr %s --help )",
		path
	);
#endif

	if ((fp = popen(cmd, "r")) == NULL) {
		*ver_ret = lame_ver;
		MEM_FREE(cmd);
		DBGPRN(DBG_GEN)(errfp, "\npopen [%s] failed.\n", cmd);
		MP3_RETURN(3);
	}

	/* Allocate temporary buffer */
	if ((tmpbuf = (char *) MEM_ALLOC("tmpbuf", bufsz)) == NULL) {
		*ver_ret = lame_ver;
		MEM_FREE(cmd);
		DBGPRN(DBG_GEN)(errfp, "\nOut of memory.\n");
		MP3_RETURN(2);
	}

	/* Check the lame usage output */
	vmaj = vmin = vtiny = 0;
	while (fgets(tmpbuf, bufsz, fp) != NULL) {
		/* Skip blank lines */
		if (tmpbuf[0] == '\n' || tmpbuf[0] == '\0')
			continue;

		if (sscanf(tmpbuf, "LAME version %u.%u.%u ",
			   &vmaj, &vmin, &vtiny) > 1) {
			lame_ver = ENCVER_MK(vmaj, vmin, vtiny);
			break;
		}
	}

	(void) pclose(fp);

	MEM_FREE(tmpbuf);
	MEM_FREE(cmd);

	*ver_ret = lame_ver;
#ifndef __VMS
	(void) write(pfd[1], &lame_ver, sizeof(lame_ver));
	(void) close(pfd[1]);
#endif

	MP3_RETURN(0);
	/*NOTREACHED*/
}


/*
 * if_lame_gencmd
 *	Create a command string to invoke the LAME encoder program.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure.
 *	path - File path string to the lame executable.
 *
 * Return:
 *	The command string.  This is a dynamically allocated buffer.  The
 *	caller should use MEM_FREE to deallocate it when done.  If there
 *	is insufficient memory, NULL is returned.
 */
char *
if_lame_gencmd(gen_desc_t *gdp, char *path)
{
	char		*sq,
			*cmd,
			*cp;
	curstat_t	*s = cdda_clinfo->curstat_addr();
	size_t		extra_len;

#ifdef __VMS
	sq = "\"";
#else
	sq = "'";
#endif

	if (app_data.lame_opts == NULL ||
	    app_data.lameopts_mode == LAMEOPTS_DISABLE)
		extra_len = 0;
	else
		extra_len = strlen(app_data.lame_opts);

	cmd = (char *) MEM_ALLOC("cmd",
		strlen(path) + (strlen(gdp->path) << 1) +
		strlen(tagcomment) + extra_len + 400
	);
	if (cmd == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"if_lame_gencmd: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return NULL;
	}

	(void) sprintf(cmd,
		(lame_ver < ENCVER_MK(3,90,0)) ?
		    "%s%s -r -s 44.1" : "%s%s -r -s 44.1 --bitwidth 16",
		path,
		((app_data.debug & (DBG_GEN|DBG_SND)) == 0) ? " -S" : ""
	);

	if (app_data.lameopts_mode == LAMEOPTS_REPLACE) {
		(void) sprintf(cmd, "%s %s",
			cmd,
			app_data.lame_opts == NULL ? "" : app_data.lame_opts
		);
	}
	else {
		if (app_data.lameopts_mode == LAMEOPTS_INSERT &&
		    app_data.lame_opts != NULL)
			(void) sprintf(cmd, "%s %s", cmd, app_data.lame_opts);

		switch (app_data.comp_mode) {
		case COMPMODE_0:
			(void) sprintf(cmd,
				(lame_ver < ENCVER_MK(3,91,0)) ?
				    "%s -b %d" : "%s --cbr -b %d",
				cmd,
				(app_data.bitrate > 0) ? app_data.bitrate : 128
			);
			break;
		case COMPMODE_1:
			(void) sprintf(cmd,
				(lame_ver < ENCVER_MK(3,84,0)) ?
				    "%s -v -V %d" : "%s --vbr-old -V %d",
				cmd, -(app_data.qual_factor - 10)
			);
			if (app_data.bitrate_min > 0)
				(void) sprintf(cmd, "%s -b %d", cmd,
						app_data.bitrate_min);
			if (app_data.bitrate_max > 0)
				(void) sprintf(cmd, "%s -B %d", cmd,
						app_data.bitrate_max);
			break;
		case COMPMODE_2:
			(void) sprintf(cmd,
				(lame_ver < ENCVER_MK(3,84,0)) ?
				    "%s -v -V %d" : "%s --vbr-new -V %d",
				cmd, -(app_data.qual_factor - 10)
			);
			if (app_data.bitrate_min > 0)
				(void) sprintf(cmd, "%s -b %d", cmd,
						app_data.bitrate_min);
			if (app_data.bitrate_max > 0)
				(void) sprintf(cmd, "%s -B %d", cmd,
						app_data.bitrate_max);
			break;
		case COMPMODE_3:
		default:
			(void) sprintf(cmd,
				(lame_ver < ENCVER_MK(3,84,0)) ?
				    "%s -b %d" : "%s --abr %d",
				cmd,
				(app_data.bitrate > 0) ? app_data.bitrate : 128
			);
			if (app_data.bitrate_min > 0)
				(void) sprintf(cmd, "%s -b %d", cmd,
						app_data.bitrate_min);
			if (app_data.bitrate_max > 0)
				(void) sprintf(cmd, "%s -B %d", cmd,
						app_data.bitrate_max);
			break;
		}

		if (lame_ver > ENCVER_MK(3,88,0)) {
			(void) sprintf(cmd, "%s -q %d",
				cmd,
				-(app_data.comp_algo - 10)
			);
		}

		switch (app_data.chan_mode) {
		case CH_STEREO:
			(void) strcat(cmd, " -m s");
			break;
		case CH_FORCEMS:
			(void) strcat(cmd, " -m f");
			break;
		case CH_MONO:
			(void) strcat(cmd, " -m s -a");
			break;
		case CH_JSTEREO:
		default:
			(void) strcat(cmd, " -m j");
			break;
		}

		if (app_data.lowpass_mode == FILTER_OFF &&
		    app_data.highpass_mode == FILTER_OFF)
			(void) strcat(cmd, " -k");
		else {
			if (app_data.lowpass_mode == FILTER_MANUAL) {
				(void) sprintf(cmd,
				    "%s --lowpass %.1f --lowpass-width %.1f",
				    cmd,
				    (float) app_data.lowpass_freq / 1024.0,
				    (float) app_data.lowpass_width / 1024.0
				);
			}
			if (app_data.highpass_mode == FILTER_MANUAL) {
				(void) sprintf(cmd,
				    "%s --highpass %.1f --highpass-width %.1f",
				    cmd,
				    (float) app_data.highpass_freq / 1024.0,
				    (float) app_data.highpass_width / 1024.0
				);
			}
		}

		if (app_data.copyright)
			(void) strcat(cmd, " -c");
		if (!app_data.original)
			(void) strcat(cmd, " -o");
		if (app_data.checksum)
			(void) strcat(cmd, " -p");
		if (app_data.nores)
			(void) strcat(cmd, " --nores");
		if (app_data.strict_iso)
			(void) strcat(cmd, " --strictly-enforce-ISO");

		if (app_data.lameopts_mode == LAMEOPTS_APPEND &&
		    app_data.lame_opts != NULL)
			(void) sprintf(cmd, "%s %s", cmd, app_data.lame_opts);
	}

	if (app_data.add_tag) {
		cdinfo_incore_t	*cdp = cdinfo_addr();
		chset_conv_t	*up;
		char		*p;
		int		idx = (int) gdp->cdp->i->trk_idx,
				sav_chset_xlat;

		/* Force conversion to ISO-8859 regardless of locale */
		sav_chset_xlat = app_data.chset_xlat;
		app_data.chset_xlat = CHSET_XLAT_ISO8859;

		if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL) {
			app_data.chset_xlat = sav_chset_xlat;
			(void) strcpy(gdp->cdp->i->msgbuf,
					"if_lame_gencmd: Out of memory.");
			DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return NULL;
		}

		if (lame_ver > ENCVER_MK(3,70,0)) {
			switch (app_data.id3tag_mode) {
			case ID3TAG_V1:
				(void) strcat(cmd, " --id3v1-only");
				break;
			case ID3TAG_V2:
				(void) strcat(cmd, " --id3v2-only");
				break;
			case ID3TAG_BOTH:
			default:
				(void) strcat(cmd, " --add-id3v2");
				break;
			}
		}

		if (cdp->disc.title != NULL) {
			p = NULL;
			if (!util_chset_conv(up, cdp->disc.title, &p, FALSE) &&
			    !util_newstr(&p, cdp->disc.title)) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				cp = gen_esc_shellquote(gdp, p);
				if (cp == NULL) {
					util_chset_close(up);
					app_data.chset_xlat = sav_chset_xlat;
					return NULL;
				}
				(void) sprintf(cmd, "%s --tl %s%.80s%s",
						cmd, sq, cp, sq);
				MEM_FREE(p);
				MEM_FREE(cp);
			}
		}

		if (app_data.cdda_trkfile && cdp->track[idx].artist != NULL) {
			p = NULL;
			if (!util_chset_conv(up, cdp->track[idx].artist,
					     &p, FALSE) &&
			    !util_newstr(&p, cdp->track[idx].artist)) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				cp = gen_esc_shellquote(gdp, p);
				if (cp == NULL) {
					util_chset_close(up);
					app_data.chset_xlat = sav_chset_xlat;
					return NULL;
				}
				(void) sprintf(cmd, "%s --ta %s%.80s%s",
						cmd, sq, cp, sq);
				MEM_FREE(p);
				MEM_FREE(cp);
			}
		}
		else if (cdp->disc.artist != NULL) {
			p = NULL;
			if (!util_chset_conv(up, cdp->disc.artist,
					     &p, FALSE) &&
			    !util_newstr(&p, cdp->disc.artist)) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				cp = gen_esc_shellquote(gdp, p);
				if (cp == NULL) {
					util_chset_close(up);
					app_data.chset_xlat = sav_chset_xlat;
					return NULL;
				}
				(void) sprintf(cmd, "%s --ta %s%.80s%s",
						cmd, sq, cp, sq);
				MEM_FREE(p);
				MEM_FREE(cp);
			}
		}

		if (app_data.cdda_trkfile && cdp->track[idx].title != NULL) {
			p = NULL;
			if (!util_chset_conv(up, cdp->track[idx].title,
					     &p, FALSE) &&
			    !util_newstr(&p, cdp->track[idx].title)) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				cp = gen_esc_shellquote(gdp, p);
				if (cp == NULL) {
					util_chset_close(up);
					app_data.chset_xlat = sav_chset_xlat;
					return NULL;
				}
				(void) sprintf(cmd, "%s --tt %s%.80s%s",
						cmd, sq, cp, sq);
				MEM_FREE(p);
				MEM_FREE(cp);
			}
		}
		else if (cdp->disc.title != NULL) {
			p = NULL;
			if (!util_chset_conv(up, cdp->disc.title, &p, FALSE) &&
			    !util_newstr(&p, cdp->disc.title)) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				cp = gen_esc_shellquote(gdp, p);
				if (cp == NULL) {
					util_chset_close(up);
					app_data.chset_xlat = sav_chset_xlat;
					return NULL;
				}
				(void) sprintf(cmd, "%s --tt %s%.80s%s",
						cmd, sq, cp, sq);
				MEM_FREE(p);
				MEM_FREE(cp);
			}
		}

		if (app_data.cdda_trkfile && cdp->track[idx].year != NULL) {
			p = NULL;
			if (!util_chset_conv(up, cdp->track[idx].year,
					     &p, FALSE) &&
			    !util_newstr(&p, cdp->track[idx].year)) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				(void) sprintf(cmd, "%s --ty %s%.4s%s",
						cmd, sq, p, sq);
				MEM_FREE(p);
			}
		}
		else if (cdp->disc.year != NULL) {
			p = NULL;
			if (!util_chset_conv(up, cdp->disc.year, &p, FALSE) &&
			    !util_newstr(&p, cdp->disc.year)) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				(void) sprintf(cmd, "%s --ty %s%.4s%s",
						cmd, sq, p, sq);
				MEM_FREE(p);
			}
		}

		if (app_data.cdda_trkfile && cdp->track[idx].genre != NULL) {
			p = NULL;
			if (!util_chset_conv(up,
				    gen_genre_cddb2id3(cdp->track[idx].genre),
				    &p, FALSE) &&
			    !util_newstr(&p,
				    gen_genre_cddb2id3(
					cdp->track[idx].genre))) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				(void) sprintf(cmd, "%s --tg %s%.4s%s",
						cmd, sq, p, sq);
				MEM_FREE(p);
			}
		}
		else if (cdp->disc.genre != NULL) {
			p = NULL;
			if (!util_chset_conv(up,
				    gen_genre_cddb2id3(cdp->disc.genre),
				    &p, FALSE) &&
			    !util_newstr(&p,
				    gen_genre_cddb2id3(cdp->disc.genre))) {
				util_chset_close(up);
				app_data.chset_xlat = sav_chset_xlat;
				(void) strcpy(gdp->cdp->i->msgbuf,
					    "if_lame_gencmd: Out of memory.");
				DBGPRN(DBG_GEN)(errfp, "%s\n",
						gdp->cdp->i->msgbuf);
				return NULL;
			}
			if (p != NULL) {
				(void) sprintf(cmd, "%s --tg %s%.4s%s",
						cmd, sq, p, sq);
				MEM_FREE(p);
			}
		}

		if (app_data.cdda_trkfile) {
			(void) sprintf(cmd, "%s --tn %s%d%s", cmd,
					sq, (int) s->trkinfo[idx].trkno, sq);
		}

		p = NULL;
		if (!util_chset_conv(up, tagcomment, &p, FALSE) &&
		    !util_newstr(&p, tagcomment)) {
			util_chset_close(up);
			app_data.chset_xlat = sav_chset_xlat;
			(void) strcpy(gdp->cdp->i->msgbuf,
					"if_lame_gencmd: Out of memory.");
			DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return NULL;
		}
		if (p != NULL) {
			(void) sprintf(cmd, "%s --tc %s%.40s%s",
					cmd, sq, p, sq);
			MEM_FREE(p);
		}

		util_chset_close(up);

		/* Restore charset conversion mode */
		app_data.chset_xlat = sav_chset_xlat;
	}

	if ((gdp->flags & GDESC_ISPIPE) != 0) {
#ifdef __VMS
		/* Not possible in this VMS implementation */
		MEM_FREE(cmd);
		cmd = NULL;
		(void) sprintf(gdp->cdp->i->msgbuf, "ERROR: "
			"MP3 format not supported for pipe-to-program.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
#else
		/* gdp->path is the program to pipe to */
		(void) sprintf(cmd,
			"%s - - 2>%s | %s",
			cmd,
			((app_data.debug & (DBG_SND|DBG_GEN)) != 0) ?
				"/tmp/xmcd-lame.$$" : "/dev/null",
			gdp->path
		);
#endif
	}
	else {
		/* gdp->path is the file to write to */
		if ((cp = gen_esc_shellquote(gdp, gdp->path)) == NULL)
			return NULL;

#ifdef __VMS
		(void) sprintf(cmd, "%s - %s", cmd, cp);
#else
		(void) sprintf(cmd,
			"%s - '%s' >%s 2>&1 && chmod %o '%s'",
			cmd,
			cp,
			((app_data.debug & (DBG_SND|DBG_GEN)) != 0) ?
				"/tmp/xmcd-lame.$$" : "/dev/null",
			(unsigned int) gdp->mode,
			cp
		);
#endif

		MEM_FREE(cp);
	}

	return (cmd);
}

#endif	/* CDDA_SUPPORTED HAS_LAME */

