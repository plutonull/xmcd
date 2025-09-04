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
static char *_if_faac_c_ident_ = "@(#)if_faac.c	7.12 04/04/21";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"

#if defined(CDDA_SUPPORTED) && defined(HAS_FAAC)

#include "cdinfo_d/cdinfo.h"
#include "cdda_d/wr_gen.h"
#include "cdda_d/if_faac.h"


extern appdata_t	app_data;
extern FILE		*errfp;
extern cdda_client_t	*cdda_clinfo;

extern char		*tagcomment;		/* Tag comment */

/* FAAC version, capability and command line configuration */
typedef struct {
	unsigned int	version;		/* FAAC version */
	bool_t		outfile_oflag;		/* Use -o to specify outfile */
	bool_t		abr_use_b;		/* Use -b for ABR */
	bool_t		def_no_tns;		/* TNS is off by default */
	bool_t		new_long_notns;		/* Use --no-tns */
	bool_t		long_no_midside;	/* Use --no-midside */
	bool_t		long_mpeg_vers;		/* Use --mpeg-vers */
	bool_t		mp4_supp;		/* Has MP4 support */
	bool_t		tag_supp;		/* Has MP4 tagging support */
	bool_t		rsvd[4];		/* Reserved */
} faac_cfg_t;

STATIC faac_cfg_t	faac_cfg;


#ifdef __VMS
#define MP4_RETURN(x)   return ((bool_t) ((x) == 0))
#else
#define MP4_RETURN(x)   _exit((x))
#endif


/*
 * if_faac_verchk
 *	Check the faac executable for its version and whether it has
 *	MP4 output file format support.
 *
 * Args:
 *	path - File path string to the faac executable.
 *	ver_ret - Pointer to location for the faac version.
 *	mp4_ret - Pointer to location for the mp4 support flag.
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
bool_t
if_faac_verchk(char *path, unsigned int *ver_ret, bool_t *mp4_ret)
{
	int		ret;
	unsigned int	vmaj,
			vmin,
			vtiny;
	size_t		bufsz = (STR_BUF_SZ << 1);
	FILE		*fp;
	char		*cmd,
			*tmpbuf;
#ifndef __VMS
	int		pfd[2];
	pid_t		cpid;
	waitret_t	wstat;

	(void) memset(&faac_cfg, 0, sizeof(faac_cfg_t));

	if (PIPE(pfd) < 0) {
		*ver_ret = faac_cfg.version;
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
			"\nif_faac_verchk fork failed (errno=%d).\n",
			errno);
		*ver_ret = faac_cfg.version;
		return FALSE;

	default:
		(void) close(pfd[1]);

		(void) read(pfd[0], &faac_cfg, sizeof(faac_cfg));
		*ver_ret = faac_cfg.version;
		*mp4_ret = faac_cfg.mp4_supp;

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

	/* Run the faac program without any command line args,
	 * which will cause it to output a usage help.  Pipe that
	 * into our stdin.  Note that if the faac usage help output
	 * format changes drastically this code may break.
	 */
	cmd = (char *) MEM_ALLOC("faac_cmd", strlen(path) + (STR_BUF_SZ << 1));
	if (cmd == NULL) {
		*ver_ret = faac_cfg.version;
		DBGPRN(DBG_GEN)(errfp, "\nOut of memory.\n");
		MP4_RETURN(3);
	}

#ifndef __VMS
	(void) sprintf(cmd, "%s --long-help 2>&1", path);
#else
	(void) sprintf(cmd,
		"pipe ( define sys$error sys$output && mcr %s --long-help )",
		path
	);
#endif

	if ((fp = popen(cmd, "r")) == NULL) {
		*ver_ret = faac_cfg.version;
		MEM_FREE(cmd);
		DBGPRN(DBG_GEN)(errfp, "\npopen [%s] failed.\n", cmd);
		MP4_RETURN(2);
	}

	/* Allocate temporary buffer */
	if ((tmpbuf = (char *) MEM_ALLOC("tmpbuf", bufsz)) == NULL) {
		*ver_ret = faac_cfg.version;
		MEM_FREE(cmd);
		DBGPRN(DBG_GEN)(errfp, "\nOut of memory.\n");
		MP4_RETURN(3);
	}

	/* Check the faac usage output */
	vmaj = vmin = vtiny = 0;
	while (fgets(tmpbuf, bufsz, fp) != NULL) {
		/* Skip blank lines */
		if (tmpbuf[0] == '\n' || tmpbuf[0] == '\0')
			continue;

		/* Check faac version */
		if (sscanf(tmpbuf, "libfaac version %u.%u.%u",
			   &vmaj, &vmin, &vtiny) >= 2 ||
		    sscanf(tmpbuf, "FAAC %u.%u.%u",
			   &vmaj, &vmin, &vtiny) >= 2) {
			faac_cfg.version = ENCVER_MK(vmaj, vmin, vtiny);
			continue;
		}

		/* Check if we need to use -o to specify the output file */
		if (strncmp(tmpbuf, "  -o X", 6) == 0) {
			char	*p;

			for (p = tmpbuf + 6; *p == ' ' || *p == '\t'; p++)
				;

			if (strncmp(p, "Set output file", 15) == 0) {
				/* Found it */
				faac_cfg.outfile_oflag = TRUE;
				continue;
			}
		}

		/* Check if we should use -b for ABR (else use -a) */
		if (strncmp(tmpbuf, "  -b <bitrate>", 14) == 0) {
			/* Found it */
			faac_cfg.abr_use_b = TRUE;
			continue;
		}

		/* Check if TNS is enabled by default, and whether to use
		 * --no-tns or --notns to disable TNS.
		 */
		if (strncmp(tmpbuf, "  --tns", 7) == 0) {
			/* Found it */
			faac_cfg.def_no_tns = TRUE;
			continue;
		}
		if (strncmp(tmpbuf, "  --no-tns", 9) == 0) {
			/* Found it */
			faac_cfg.new_long_notns = TRUE;
			continue;
		}

		/* Check if we should use --no-midside to disable mid-side
		 * coding (else use -n)
		 */
		if (strncmp(tmpbuf, "  --no-midside", 14) == 0) {
			/* Found it */
			faac_cfg.long_no_midside = TRUE;
			continue;
		}

		/* Check if we should use --mpeg-vers to select MPEG version
		 * (else use -m)
		 */
		if (strncmp(tmpbuf, "  --mpeg-vers", 13) == 0) {
			/* Found it */
			faac_cfg.long_mpeg_vers = TRUE;
			continue;
		}

		/* Check if "-w" is in there */
		if (strncmp(tmpbuf, "  -w", 4) == 0) {
			/* Found it */
			faac_cfg.mp4_supp = TRUE;
			continue;
		}

		/* Check if this faac version supports tagging */
		if (strncmp(tmpbuf, "  --artist", 10) == 0) {
			/* Found it */
			faac_cfg.tag_supp = TRUE;
			continue;
		}
	}

	(void) pclose(fp);

	MEM_FREE(tmpbuf);
	MEM_FREE(cmd);

	*ver_ret = faac_cfg.version;
	*mp4_ret = faac_cfg.mp4_supp;

#ifndef __VMS
	(void) write(pfd[1], &faac_cfg, sizeof(faac_cfg));
	(void) close(pfd[1]);
#endif

	MP4_RETURN(0);
	/*NOTREACHED*/
}


/*
 * if_faac_gencmd
 *	Create a command string to invoke the FAAC codec program.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure.
 *	path - File path string to the faac executable.
 *
 * Return:
 *	The command string.  This is a dynamically allocated buffer.  The
 *	caller should use MEM_FREE to deallocate it when done.  If there
 *	is insufficient memory, NULL is returned.
 */
char *
if_faac_gencmd(gen_desc_t *gdp, char *path)
{
	char		*sq,
			*cmd,
			*cp;
	curstat_t	*s = cdda_clinfo->curstat_addr();

#ifdef __VMS
	sq = "\"";
#else
	sq = "'";
#endif

	if (gdp->fmt == FILEFMT_MP4 && !faac_cfg.mp4_supp)
		return NULL;

	cmd = (char *) MEM_ALLOC("cmd",
		strlen(path) + (strlen(gdp->path) << 1) +
		strlen(tagcomment) + 400
	);
	if (cmd == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"if_faac_gencmd: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return NULL;
	}

	(void) sprintf(cmd, "%s -P -R 44100 -B 16 -C 2", path);

	switch (gdp->fmt) {
	case FILEFMT_MP4:
		switch (app_data.comp_mode) {
		case COMPMODE_0:
		case COMPMODE_3:
			/* Set the average bitrate, MP4.
			 * The bitrate is per-channel, so divide by 2.
			 */
			(void) sprintf(cmd, "%s -w %s %d",
				cmd,
				faac_cfg.abr_use_b ? "-b" : "-a",
				(app_data.bitrate > 0) ?
					(app_data.bitrate / 2) : 64
			);
			break;
		case COMPMODE_1:
		case COMPMODE_2:
		default:
			/* Scale the quality to between 65 and 200, MP4 */
			(void) sprintf(cmd, "%s -w -q %d",
				cmd,
				app_data.qual_factor * 15 + 50
			);
			break;
		}
		break;

	case FILEFMT_AAC:
	default:
		switch (app_data.comp_mode) {
		case COMPMODE_0:
			/* Set the average bitrate, MPEG-2.
			 * The bitrate is per-channel, so divide by 2.
			 */
			(void) sprintf(cmd, "%s %s 2 %s %d",
				cmd,
				faac_cfg.long_mpeg_vers ? "--mpeg-vers" : "-m",
				faac_cfg.abr_use_b ? "-b" : "-a",
				(app_data.bitrate > 0) ?
					(app_data.bitrate / 2) : 64
			);
			break;
		case COMPMODE_2:
			/* Scale the quality to between 65 and 200, MPEG-4 */
			(void) sprintf(cmd, "%s %s 4 -q %d",
				cmd,
				faac_cfg.long_mpeg_vers ? "--mpeg-vers" : "-m",
				app_data.qual_factor * 15 + 50
			);
			break;
		case COMPMODE_3:
			/* Set the average bitrate, MPEG-4.
			 * The bitrate is per-channel, so divide by 2.
			 */
			(void) sprintf(cmd, "%s %s 4 %s %d",
				cmd,
				faac_cfg.long_mpeg_vers ? "--mpeg-vers" : "-m",
				faac_cfg.abr_use_b ? "-b" : "-a",
				(app_data.bitrate > 0) ?
					(app_data.bitrate / 2) : 64
			);
			break;
		case COMPMODE_1:
		default:
			/* Scale the quality to between 65 and 200, MPEG-2 */
			(void) sprintf(cmd, "%s %s 2 -q %d",
				cmd,
				faac_cfg.long_mpeg_vers ? "--mpeg-vers" : "-m",
				app_data.qual_factor * 15 + 50
			);
			break;
		}
		break;
	}

	/* Bandwidth limiting */
	if (app_data.lowpass_mode == FILTER_MANUAL &&
	    app_data.lowpass_freq < 22050) {
		(void) sprintf(cmd, "%s -c %d", cmd, app_data.lowpass_freq);
	}

	/* Temporal noise shaping */
	if (app_data.comp_algo == 10) {
		(void) strcat(cmd, " --tns");
	}
	else if (!faac_cfg.def_no_tns) {
		(void) strcat(
			cmd,
			faac_cfg.new_long_notns ? " --no-tns" : " --notns"
		);
	}

	switch (app_data.chan_mode) {
	case CH_STEREO:
		/* Disable mid-side coding */
		(void) strcat(
			cmd,
			faac_cfg.long_no_midside ? " --no-midside" : " -n"
		);
		break;
	case CH_JSTEREO:
	case CH_FORCEMS:
		break;
	case CH_MONO:
	default:
		DBGPRN(DBG_GEN)(errfp,
			"NOTICE: Mono mode not supported by faac: Ignored.\n");
		break;
	}

	/* File tagging */
	if (gdp->fmt == FILEFMT_MP4 && app_data.add_tag && faac_cfg.tag_supp) {
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
					"if_faac_gencmd: Out of memory.");
			DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return NULL;
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
				(void) sprintf(cmd, "%s --album %s%.80s%s",
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
				(void) sprintf(cmd, "%s --artist %s%.80s%s",
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
				(void) sprintf(cmd, "%s --artist %s%.80s%s",
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
				(void) sprintf(cmd, "%s --title %s%.80s%s",
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
				(void) sprintf(cmd, "%s --title %s%.80s%s",
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
				(void) sprintf(cmd, "%s --year %s%.4s%s",
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
				(void) sprintf(cmd, "%s --year %s%.4s%s",
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
				(void) sprintf(cmd, "%s --genre %s%.4s%s",
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
				(void) sprintf(cmd, "%s --genre %s%.4s%s",
						cmd, sq, p, sq);
				MEM_FREE(p);
			}
		}

		if (app_data.cdda_trkfile) {
			(void) sprintf(cmd, "%s --track %s%d/%d%s", cmd,
					sq,
					(int) s->trkinfo[idx].trkno,
					(int) s->tot_trks,
					sq);
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
			(void) sprintf(cmd, "%s --comment %s%.40s%s",
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
			"AAC and MP4 formats not supported "
			"for pipe-to-program.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
#else
		/* gdp->path is the program to pipe to */
		(void) sprintf(cmd,
			"%s - %s- 2>%s | %s",
			cmd,
			faac_cfg.outfile_oflag ? "-o " : "",
			((app_data.debug & (DBG_SND|DBG_GEN)) != 0) ?
				"/tmp/xmcd-faac.$$" : "/dev/null",
			gdp->path
		);
#endif
	}
	else {
		/* gdp->path is the file to write to */
		if ((cp = gen_esc_shellquote(gdp, gdp->path)) == NULL)
			return NULL;

#ifdef __VMS
		(void) sprintf(cmd, "%s - %s%s",
			       cmd, faac_cfg.outfile_oflag ? "-o " : "", cp);
#else
		(void) sprintf(cmd,
			"%s - %s'%s' >%s 2>&1 && chmod %o '%s'",
			cmd,
			faac_cfg.outfile_oflag ? "-o " : "",
			cp,
			((app_data.debug & (DBG_SND|DBG_GEN)) != 0) ?
				"/tmp/xmcd-faac.$$" : "/dev/null",
			(unsigned int) gdp->mode,
			cp
		);
#endif

		MEM_FREE(cp);
	}

	return (cmd);
}

#endif	/* CDDA_SUPPORTED HAS_FAAC */

