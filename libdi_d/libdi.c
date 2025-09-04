/*
 *   libdi - CD Audio Device Interface Library
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
static char *_libdi_c_ident_ = "@(#)libdi.c	7.170 04/04/20";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "libdi_d/scsipt.h"
#include "libdi_d/slioc.h"
#include "libdi_d/fbioc.h"
#include "libdi_d/aixioc.h"
#include "cdda_d/cdda.h"


#define PARM_BUF_SZ	(STR_BUF_SZ * 16)	/* Temporary buffer size */


extern appdata_t	app_data;
extern FILE		*errfp;


/* libdi module init routines */
diinit_tbl_t		diinit[] = {
	{ scsipt_init },		/* SCSI pass-through method */
	{ slioc_init },			/* SunOS/Solaris/Linux ioctl method */
	{ fbioc_init },			/* FreeBSD ioctl method */
	{ aixioc_init },		/* AIX IDE ioctl method */
	{ NULL }			/* List terminator */
};


/* libdi interface calling table */
di_tbl_t		ditbl[MAX_METHODS];

/* Application callbacks */
di_client_t		*di_clinfo;

/* Device list table */
char			**di_devlist;

/* Clip frames */
sword32_t		di_clip_frames;

/* Flag to indicate whether CD information has been loaded */
STATIC bool_t		di_cdinfo_loaded = FALSE;

/* Device lock file */
STATIC char		lockfile[FILE_PATH_SZ] = { '\0' };


/***********************
 *  internal routines  *
 ***********************/


/*
 * di_boolstr
 *	Return the string "False" or "True" depending upon the
 *	passed in boolean parameter.
 *
 * Args:
 *	parm - boolean parameter
 *
 * Return:
 *	"False" if parm is FALSE.
 *	"True" is parm is TRUE.
 */
STATIC char *
di_boolstr(bool_t parm)
{
	return ((parm == FALSE) ? "False" : "True");
}


/*
 * di_prncfg
 *	Display configuration information
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
di_prncfg(void)
{
	(void) fprintf(errfp, "\nBasic parameters:\n");

	(void) fprintf(errfp, "\tlibdir:\t\t\t\t%s\n",
		       app_data.libdir);

	(void) fprintf(errfp, "\nX resources:\n");

	(void) fprintf(errfp, "\tversion:\t\t\t%s\n",
		       app_data.version == NULL ?
				"(undef)" : app_data.version);
	(void) fprintf(errfp, "\tmainWindowMode:\t\t\t%d\n",
		       app_data.main_mode);
	(void) fprintf(errfp, "\tmodeChangeGravity:\t\t%d\n",
		       app_data.modechg_grav);
	(void) fprintf(errfp, "\tnormalMainWidth:\t\t%d\n",
		       app_data.normal_width);
	(void) fprintf(errfp, "\tnormalMainHeight:\t\t%d\n",
		       app_data.normal_height);
	(void) fprintf(errfp, "\tbasicMainWidth:\t\t\t%d\n",
		       app_data.basic_width);
	(void) fprintf(errfp, "\tbasicMainHeight:\t\t%d\n",
		       app_data.basic_height);
	(void) fprintf(errfp, "\tdisplayBlinkOnInterval:\t\t%d\n",
		       app_data.blinkon_interval);
	(void) fprintf(errfp, "\tdisplayBlinkOffInterval:\t%d\n",
		       app_data.blinkoff_interval);
	(void) fprintf(errfp, "\tmainShowFocus:\t\t\t%s\n",
		       di_boolstr(app_data.main_showfocus));
	(void) fprintf(errfp, "\tinstallColormap:\t\t%s\n",
		       di_boolstr(app_data.instcmap));

	(void) fprintf(errfp, "\nCommon parameters:\n");

	(void) fprintf(errfp, "\tdevice:\t\t\t\t%s\n",
		       app_data.device);
	(void) fprintf(errfp, "\toutputPort:\t\t\t0x%x\n",
		       (int) app_data.outport);
	(void) fprintf(errfp, "\tcdinfoPath:\t\t\t%s\n",
		       app_data.cdinfo_path);
	(void) fprintf(errfp, "\tcharsetConvMode:\t\t%d\n",
		       app_data.chset_xlat);
	(void) fprintf(errfp, "\tlangUtf8:\t\t\t%s\n",
		       app_data.lang_utf8);
	(void) fprintf(errfp, "\tacceptFuzzyDefault:\t\t%s\n",
		       di_boolstr(app_data.single_fuzzy));
	(void) fprintf(errfp, "\tcdinfoFileMode:\t\t\t%s\n",
		       app_data.cdinfo_filemode);
	(void) fprintf(errfp, "\tproxyServer:\t\t\t%s\n",
		       app_data.proxy_server == NULL ?
		           "(undef)" : app_data.proxy_server);
	(void) fprintf(errfp, "\thistoryFileMode:\t\t%s\n",
		       app_data.hist_filemode);
	(void) fprintf(errfp, "\thistoryFileDisable:\t\t%s\n",
		       di_boolstr(app_data.histfile_dsbl));
	(void) fprintf(errfp, "\tcddbCacheTimeout:\t\t%d\n",
		       app_data.cache_timeout);
	(void) fprintf(errfp, "\tserviceTimeout:\t\t\t%d\n",
		       app_data.srv_timeout);
	(void) fprintf(errfp, "\tlocalDiscographyMode:\t\t%d\n",
		       app_data.discog_mode);
	(void) fprintf(errfp, "\tmaximumHistory:\t\t\t%d\n",
		       app_data.cdinfo_maxhist);
	(void) fprintf(errfp, "\tstatusPollInterval:\t\t%d\n",
		       app_data.stat_interval);
	(void) fprintf(errfp, "\tinsertPollInterval:\t\t%d\n",
		       app_data.ins_interval);
	(void) fprintf(errfp, "\ttimeDisplayMode:\t\t%d\n",
		       app_data.timedpy_mode);
	(void) fprintf(errfp, "\ttooltipDelayInterval:\t\t%d\n",
		       app_data.tooltip_delay);
	(void) fprintf(errfp, "\ttooltipActiveInterval:\t\t%d\n",
		       app_data.tooltip_time);
	(void) fprintf(errfp, "\tinsertPollDisable:\t\t%s\n",
		       di_boolstr(app_data.ins_disable));
	(void) fprintf(errfp, "\tpreviousThreshold:\t\t%d\n",
		       app_data.prev_threshold);
	(void) fprintf(errfp, "\tsearchSkipBlocks:\t\t%d\n",
		       app_data.skip_blks);
	(void) fprintf(errfp, "\tsearchPauseInterval:\t\t%d\n",
		       app_data.skip_pause);
	(void) fprintf(errfp, "\tsearchSpeedUpCount:\t\t%d\n",
		       app_data.skip_spdup);
	(void) fprintf(errfp, "\tsearchVolumePercent:\t\t%d\n",
		       app_data.skip_vol);
	(void) fprintf(errfp, "\tsearchMinVolume:\t\t%d\n",
		       app_data.skip_minvol);
	(void) fprintf(errfp, "\tsampleBlocks:\t\t\t%d\n",
		       app_data.sample_blks);
	(void) fprintf(errfp, "\tinternetOffline:\t\t%s\n",
		       di_boolstr(app_data.cdinfo_inetoffln));
	(void) fprintf(errfp, "\tcddbUseProxy:\t\t\t%s\n",
		       di_boolstr(app_data.use_proxy));
	(void) fprintf(errfp, "\tproxyAuthorization:\t\t%s\n",
		       di_boolstr(app_data.proxy_auth));
	(void) fprintf(errfp, "\tautoMusicBrowser:\t\t%s\n",
		       di_boolstr(app_data.auto_musicbrowser));
	(void) fprintf(errfp, "\tshowScsiErrMsg:\t\t\t%s\n",
		       di_boolstr(app_data.scsierr_msg));
	(void) fprintf(errfp, "\tsolaris2VolumeManager:\t\t%s\n",
		       di_boolstr(app_data.sol2_volmgt));
	(void) fprintf(errfp, "\ttooltipEnable:\t\t\t%s\n",
		       di_boolstr(app_data.tooltip_enable));
	(void) fprintf(errfp, "\tremoteControlEnable:\t\t%s\n",
		       di_boolstr(app_data.remote_enb));
	(void) fprintf(errfp, "\tremoteControlLog:\t\t%s\n",
		       di_boolstr(app_data.remote_log));
	(void) fprintf(errfp, "\tcddaFilePerTrack:\t\t%s\n",
		       di_boolstr(app_data.cdda_trkfile));
	(void) fprintf(errfp, "\tcddaSpaceToUnderscore:\t\t%s\n",
		       di_boolstr(app_data.subst_underscore));
	(void) fprintf(errfp, "\tcddaFileFormat:\t\t\t%d\n",
		       app_data.cdda_filefmt);
	(void) fprintf(errfp, "\tcddaFileTemplate:\t\t%s\n",
		       app_data.cdda_tmpl == NULL ? "-" : app_data.cdda_tmpl);
	(void) fprintf(errfp, "\tcddaPipeProgram:\t\t%s\n",
		       app_data.pipeprog == NULL ? "-" : app_data.pipeprog);
	(void) fprintf(errfp, "\tcddaSchedOptions:\t\t%d\n",
		       app_data.cdda_sched);
	(void) fprintf(errfp, "\tcddaHeartbeatTimeout:\t\t%d\n",
		       app_data.hb_timeout);
	(void) fprintf(errfp, "\tcompressionMode:\t\t%d\n",
		       app_data.comp_mode);
	(void) fprintf(errfp, "\tcompressionBitrate:\t\t%d\n",
		       app_data.bitrate);
	(void) fprintf(errfp, "\tminimumBitrate:\t\t\t%d\n",
		       app_data.bitrate_min);
	(void) fprintf(errfp, "\tmaximumBitrate:\t\t\t%d\n",
		       app_data.bitrate_max);
	(void) fprintf(errfp, "\tcompressionQuality:\t\t%d\n",
		       app_data.qual_factor);
	(void) fprintf(errfp, "\tchannelMode:\t\t\t%d\n",
		       app_data.chan_mode);
	(void) fprintf(errfp, "\tcompressionAlgorithm:\t\t%d\n",
		       app_data.comp_algo);
	(void) fprintf(errfp, "\tlowpassMode:\t\t\t%d\n",
		       app_data.lowpass_mode);
	(void) fprintf(errfp, "\tlowpassFrequency:\t\t%d\n",
		       app_data.lowpass_freq);
	(void) fprintf(errfp, "\tlowpassWidth:\t\t\t%d\n",
		       app_data.lowpass_width);
	(void) fprintf(errfp, "\thighpassMode:\t\t\t%d\n",
		       app_data.highpass_mode);
	(void) fprintf(errfp, "\thighpassFrequency:\t\t%d\n",
		       app_data.highpass_freq);
	(void) fprintf(errfp, "\thighpassWidth:\t\t\t%d\n",
		       app_data.highpass_width);
	(void) fprintf(errfp, "\tcopyrightFlag:\t\t\t%s\n",
		       di_boolstr(app_data.copyright));
	(void) fprintf(errfp, "\toriginalFlag:\t\t\t%s\n",
		       di_boolstr(app_data.original));
	(void) fprintf(errfp, "\tnoBitReservoirFlag:\t\t%s\n",
		       di_boolstr(app_data.nores));
	(void) fprintf(errfp, "\tchecksumFlag:\t\t\t%s\n",
		       di_boolstr(app_data.checksum));
	(void) fprintf(errfp, "\tstrictISO:\t\t\t%s\n",
		       di_boolstr(app_data.strict_iso));
	(void) fprintf(errfp, "\taddInfoTag:\t\t\t%s\n",
		       di_boolstr(app_data.add_tag));
	(void) fprintf(errfp, "\tlameOptionsMode:\t\t%d\n",
		       app_data.lameopts_mode);
	(void) fprintf(errfp, "\tlameOptions:\t\t\t%s\n",
		       app_data.lame_opts == NULL ? "-" : app_data.lame_opts);
	(void) fprintf(errfp, "\tid3TagMode:\t\t\t%d\n",
		       app_data.id3tag_mode);
	(void) fprintf(errfp, "\tspinDownOnLoad:\t\t\t%s\n",
		       di_boolstr(app_data.load_spindown));
	(void) fprintf(errfp, "\tplayOnLoad:\t\t\t%s\n",
		       di_boolstr(app_data.load_play));
	(void) fprintf(errfp, "\tejectOnDone:\t\t\t%s\n",
		       di_boolstr(app_data.done_eject));
	(void) fprintf(errfp, "\texitOnExit:\t\t\t%s\n",
		       di_boolstr(app_data.done_exit));
	(void) fprintf(errfp, "\tejectOnExit:\t\t\t%s\n",
		       di_boolstr(app_data.exit_eject));
	(void) fprintf(errfp, "\tstopOnExit:\t\t\t%s\n",
		       di_boolstr(app_data.exit_stop));
	(void) fprintf(errfp, "\texitOnEject:\t\t\t%s\n",
		       di_boolstr(app_data.eject_exit));
	(void) fprintf(errfp, "\trepeatMode:\t\t\t%s\n",
		       di_boolstr(app_data.repeat_mode));
	(void) fprintf(errfp, "\tshuffleMode:\t\t\t%s\n",
		       di_boolstr(app_data.shuffle_mode));
	(void) fprintf(errfp, "\tdiscogURLPrefix:\t\t%s\n",
		       app_data.discog_url_pfx == NULL ?
			   "-" : app_data.discog_url_pfx);
	(void) fprintf(errfp, "\tautoMotdDisable:\t\t%s\n",
		       di_boolstr(app_data.automotd_dsbl));
	(void) fprintf(errfp, "\tdebugLevel:\t\t\t0x%x\n",
		       (int) app_data.debug);
	(void) fprintf(errfp, "\texcludeWords:\t\t\t%s\n",
		       app_data.exclude_words == NULL ?
		           "(undef)" : app_data.exclude_words);

	(void) fprintf(errfp, "\nDevice-specific (privileged) parameters:\n");

	(void) fprintf(errfp, "\tdevnum:\t\t\t\t%d\n",
		       app_data.devnum);
	(void) fprintf(errfp, "\tdeviceList:\t\t\t%s\n",
		       app_data.devlist);
	(void) fprintf(errfp, "\tdeviceInterfaceMethod:\t\t%d\n",
		       app_data.di_method);
	(void) fprintf(errfp, "\tcddaMethod:\t\t\t%d\n",
		       app_data.cdda_method);
	(void) fprintf(errfp, "\tcddaReadMethod:\t\t\t%d\n",
		       app_data.cdda_rdmethod);
	(void) fprintf(errfp, "\tcddaWriteMethod:\t\t%d\n",
		       app_data.cdda_wrmethod);
	(void) fprintf(errfp, "\tcddaScsiModeSelect:\t\t%s\n",
		       di_boolstr(app_data.cdda_modesel));
	(void) fprintf(errfp, "\tcddaScsiDensity:\t\t%d\n",
		       app_data.cdda_scsidensity);
	(void) fprintf(errfp, "\tcddaScsiReadCommand:\t\t%d\n",
		       app_data.cdda_scsireadcmd);
	(void) fprintf(errfp, "\tcddaReadChunkBlocks:\t\t%d\n",
		       app_data.cdda_readchkblks);
	(void) fprintf(errfp, "\tcddaDataBigEndian:\t\t%s\n",
		       di_boolstr(app_data.cdda_bigendian));
	(void) fprintf(errfp, "\tdriveVendorCode:\t\t%d\n",
		       app_data.vendor_code);
	(void) fprintf(errfp, "\tscsiVersionCheck:\t\t%s\n",
		       di_boolstr(app_data.scsiverck));
	(void) fprintf(errfp, "\tnumDiscs:\t\t\t%d\n",
		       app_data.numdiscs);
	(void) fprintf(errfp, "\tmediumChangeMethod:\t\t%d\n",
		       app_data.chg_method);
	(void) fprintf(errfp, "\tscsiAudioVolumeBase:\t\t%d\n",
		       app_data.base_scsivol);
	(void) fprintf(errfp, "\tminimumPlayBlocks:\t\t%d\n",
		       app_data.min_playblks);
	(void) fprintf(errfp, "\tplayAudio10Support:\t\t%s\n",
		       di_boolstr(app_data.play10_supp));
	(void) fprintf(errfp, "\tplayAudio12Support:\t\t%s\n",
		       di_boolstr(app_data.play12_supp));
	(void) fprintf(errfp, "\tplayAudioMSFSupport:\t\t%s\n",
		       di_boolstr(app_data.playmsf_supp));
	(void) fprintf(errfp, "\tplayAudioTISupport:\t\t%s\n",
		       di_boolstr(app_data.playti_supp));
	(void) fprintf(errfp, "\tloadSupport:\t\t\t%s\n",
		       di_boolstr(app_data.load_supp));
	(void) fprintf(errfp, "\tejectSupport:\t\t\t%s\n",
		       di_boolstr(app_data.eject_supp));
	(void) fprintf(errfp, "\tmodeSenseSetDBD:\t\t%s\n",
		       di_boolstr(app_data.msen_dbd));
	(void) fprintf(errfp, "\tmodeSenseUse10Byte:\t\t%s\n",
		       di_boolstr(app_data.msen_10));
	(void) fprintf(errfp, "\tvolumeControlSupport:\t\t%s\n",
		       di_boolstr(app_data.mselvol_supp));
	(void) fprintf(errfp, "\tbalanceControlSupport:\t\t%s\n",
		       di_boolstr(app_data.balance_supp));
	(void) fprintf(errfp, "\tchannelRouteSupport:\t\t%s\n",
		       di_boolstr(app_data.chroute_supp));
	(void) fprintf(errfp, "\tpauseResumeSupport:\t\t%s\n",
		       di_boolstr(app_data.pause_supp));
	(void) fprintf(errfp, "\tstrictPauseResume:\t\t%s\n",
		       di_boolstr(app_data.strict_pause_resume));
	(void) fprintf(errfp, "\tplayPausePlay:\t\t\t%s\n",
		       di_boolstr(app_data.play_pause_play));
	(void) fprintf(errfp, "\tcaddyLockSupport:\t\t%s\n",
		       di_boolstr(app_data.caddylock_supp));
	(void) fprintf(errfp, "\tcurposFormat:\t\t\t%s\n",
		       di_boolstr(app_data.curpos_fmt));
	(void) fprintf(errfp, "\tnoTURWhenPlaying:\t\t%s\n",
		       di_boolstr(app_data.play_notur));
	(void) fprintf(errfp, "\ttocLBA:\t\t\t\t%s\n",
		       di_boolstr(app_data.toc_lba));
	(void) fprintf(errfp, "\tsubChannelLBA:\t\t\t%s\n",
		       di_boolstr(app_data.subq_lba));
	(void) fprintf(errfp, "\tdriveBlockSize:\t\t\t%d\n",
		       app_data.drv_blksz);
	(void) fprintf(errfp, "\tspinUpInterval:\t\t\t%d\n",
		       app_data.spinup_interval);
	(void) fprintf(errfp, "\tmcnDisable:\t\t\t%s\n",
		       di_boolstr(app_data.mcn_dsbl));
	(void) fprintf(errfp, "\tisrcDisable:\t\t\t%s\n",
		       di_boolstr(app_data.isrc_dsbl));
	(void) fprintf(errfp, "\tcdTextDisable:\t\t\t%s\n",
		       di_boolstr(app_data.cdtext_dsbl));

	(void) fprintf(errfp,
		       "\nDevice-specific (user-modifiable) parameters:\n");

	(void) fprintf(errfp, "\tplayMode:\t\t\t%d\n",
		       app_data.play_mode);
	(void) fprintf(errfp, "\tvolumeControlTaper:\t\t%d\n",
		       app_data.vol_taper);
	(void) fprintf(errfp, "\tstartupVolume:\t\t\t%d\n",
		       app_data.startup_vol);
	(void) fprintf(errfp, "\tchannelRoute:\t\t\t%d\n",
		       app_data.ch_route);
	(void) fprintf(errfp, "\tcloseOnEject:\t\t\t%s\n",
		       di_boolstr(app_data.eject_close));
	(void) fprintf(errfp, "\tcaddyLock:\t\t\t%s\n",
		       di_boolstr(app_data.caddy_lock));
	(void) fprintf(errfp, "\tmultiPlay:\t\t\t%s\n",
		       di_boolstr(app_data.multi_play));
	(void) fprintf(errfp, "\treversePlay:\t\t\t%s\n",
		       di_boolstr(app_data.reverse));
	(void) fprintf(errfp, "\tcddaJitterCorrection:\t\t%s\n",
		       di_boolstr(app_data.cdda_jitter_corr));

	(void) fprintf(errfp, "\n");
}


/*
 * di_parse_devlist
 *	Parse the app_data.devlist string and create the di_devlist array.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
di_parse_devlist(void)
{
	char		*p,
			*q;
	int		i,
			n,
			listsz;
	curstat_t	*s = di_clinfo->curstat_addr();

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

		di_devlist = (char **) MEM_ALLOC("di_devlist", listsz);
		if (di_devlist == NULL) {
			DI_FATAL(app_data.str_nomemory);
			return;
		}
		(void) memset(di_devlist, 0, listsz);

		p = q = app_data.devlist;
		if (p == NULL || *p == '\0') {
			DI_FATAL(app_data.str_devlist_undef);
			return;
		}

		for (i = 0; i < n; i++) {
			q = strchr(p, ';');

			if (q == NULL && i < (n - 1)) {
				DI_FATAL(app_data.str_devlist_count);
				return;
			}

			if (q != NULL)
				*q = '\0';

			if (!util_newstr(&di_devlist[i], p)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}

			if (q != NULL)
				*q = ';';

			p = q + 1;
		}

		/* In this mode, closeOnEject must be True */
		app_data.eject_close = TRUE;
		break;

	case CHG_OS_IOCTL:
	case CHG_NONE:
	default:
		if (app_data.chg_method == CHG_OS_IOCTL) {
			/* In this mode, closeOnEject must be True */
			app_data.eject_close = TRUE;
		}
		else {
			/* Some fix-ups in case of mis-configuration */
			app_data.numdiscs = 1;
		}

		if (app_data.devlist == NULL) {
			if (!util_newstr(&app_data.devlist, app_data.device)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
		}
		else if (strcmp(app_data.devlist, app_data.device) != 0) {
			MEM_FREE(app_data.devlist);
			app_data.devlist = NULL;
			if (!util_newstr(&app_data.devlist, app_data.device)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
		}

		di_devlist = (char **) MEM_ALLOC("di_devlist", sizeof(char *));
		if (di_devlist == NULL) {
			DI_FATAL(app_data.str_nomemory);
			return;
		}
		di_devlist[0] = NULL;
		if (!util_newstr(&di_devlist[0], app_data.devlist)) {
			DI_FATAL(app_data.str_nomemory);
			return;
		}
		break;
	}

	/* Initialize to the first device */
	s->curdev = di_devlist[0];
}


/*
 * di_chktmpl
 *	Check the CDDA save-to-file path template for the specified token.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	tok - The token string to look for
 *
 * Return:
 *	TRUE  - Token is found
 *	FALSE - Token not found
 */
STATIC bool_t
di_chktmpl(curstat_t *s, char *tok)
{
	char	*p;

	if ((p = util_strstr(s->outf_tmpl, tok)) != NULL) {
		if (p == s->outf_tmpl) {
			/* The token is found at the beginning
			 * of the template string.
			 */
			return TRUE;
		}
		else if (p == (s->outf_tmpl + 1)) {
			if (*(p-1) != '%') {
				/* The token is found and is not a character
				 * after a "%%" token.  This is a special
				 * case check for cases where the token
				 * is 1 character offset into the template
				 * string.
				 */
				return TRUE;
			}
		}
		else if (*(p-1) != '%' || *(p-2) == '%') {
			/* The token is found and is not a character after
			 * a "%%" token.  This is a "general" check for
			 * tokens that appear at 2 character offset into
			 * the template string or more.
			 */
			return TRUE;
		}
	}

	/* The token is not found in the template string */
	return FALSE;
}


/***********************
 *   public routines   *
 ***********************/


/*
 * di_init
 *	Top-level function to initialize the libdi modules.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_init(di_client_t *clp)
{
	int		i;
	curstat_t	*s = clp->curstat_addr();

	di_clinfo = (di_client_t *)(void *) MEM_ALLOC(
		"di_client_t",
		sizeof(di_client_t)
	);
	if (di_clinfo == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return;
	}
	(void) memcpy(di_clinfo, clp, sizeof(di_client_t));

#if defined(DI_SCSIPT) && defined(DEMO_ONLY)
	/* Hardwire some params for demo mode.  This overrides the
	 * parameters from the config files.
	 */
	app_data.di_method = DI_SCSIPT;
	app_data.vendor_code = VENDOR_SCSI2;
	app_data.play10_supp = TRUE;
	app_data.play12_supp = TRUE;
	app_data.playmsf_supp = TRUE;
	app_data.playti_supp = TRUE;
	app_data.load_supp = TRUE;
	app_data.eject_supp = TRUE;
	app_data.msen_dbd = FALSE;
	app_data.mselvol_supp = TRUE;
	app_data.balance_supp = TRUE;
	app_data.chroute_supp = TRUE;
	app_data.pause_supp = TRUE;
	app_data.caddylock_supp = TRUE;
	app_data.curpos_fmt = TRUE;
	app_data.cdda_scsidensity = 0;
	app_data.cdda_scsireadcmd = 0;	/* MMC Read CD */
	app_data.cdda_readchkblks = 4;	/* Max xfer length of cdsim */
	app_data.cdda_method = 0;       /* None */
	app_data.cdda_rdmethod = 1;     /* SCSI pass-through */
	app_data.cdda_wrmethod = 8;     /* File and pipe only */
	app_data.cdda_jitter_corr = FALSE;
#else
	/* Sanity check */
	if (app_data.di_method < 0 || app_data.di_method >= MAX_METHODS) {
		DI_FATAL(app_data.str_nomethod);
		return;
	}
#endif

	/* Parse the device list and set up structure */
	di_parse_devlist();

	/* Check string lengths.  This is to avoid subsequent
	 * string buffer overflows.
	 */
	if (((int) (strlen(app_data.str_notrom) +
		    strlen(app_data.device)) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_notscsi2) +
		    strlen(app_data.device)) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_staterr) +
		    strlen(app_data.device)) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_noderr) +
		    strlen(app_data.device)) >= ERR_BUF_SZ) ||
	    ((int) (strlen(app_data.str_nocfg) + 12 +
		    strlen(app_data.libdir)) >= ERR_BUF_SZ)) {
		DI_FATAL(app_data.str_longpatherr);
		return;
	}

	lockfile[0] = '\0';

	/* Initialize the libdi modules */
	for (i = 0; i < MAX_METHODS; i++) {
		if (diinit[i].init != NULL)
			diinit[i].init(s, &ditbl[i]);
	}

	/* Sanity check again */
	if (ditbl[app_data.di_method].methodstr == NULL) {
		DI_FATAL(app_data.str_nomethod);
		return;
	}

	/* Fill in device capabilities bitmask return */
	clp->capab = (cdda_capab() | CAPAB_PLAYAUDIO);
}


/*
 * di_bitrate_valid
 *	Check if a bitrate is valid.
 *
 * Args:
 *	bitrate - The bitrate in kb/s to check
 *
 * Returns:
 *	TRUE  - valid
 *	FALSE - invalid
 */
bool_t
di_bitrate_valid(int bitrate)
{
	int	i,
		n = cdda_bitrates();

	if (bitrate == 0)
		return TRUE;	/* Special case for "default" bitrate */
		
	/* Check against list of valid bitrates */
	for (i = 0; i < n; i++) {
		int	br = cdda_bitrate_val(i);

		if (br >= 32 && br == bitrate)
			break;
	}
	if (i == n)
		return FALSE;

	return TRUE;
}


/*
 * di_load_cdtext
 *	Read CD-TEXT information from the CD if available, parse the raw
 *	data and fill the di_cdtext_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	t - Pointer to the di_cdtext_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_load_cdtext(curstat_t *s, di_cdtext_t *t)
{
	if (ditbl[app_data.di_method].load_cdtext != NULL)
		ditbl[app_data.di_method].load_cdtext(s, t);
	else
		t->cdtext_valid = FALSE;
}


/*
 * di_clear_cdtext
 *	Clear the contents of the CD-TEXT information structure.
 *
 * Args:
 *	t - Pointer to the di_cdtext_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_clear_cdtext(di_cdtext_t *t)
{
	int	i;

	t->cdtext_valid = FALSE;
	if (t->disc.title != NULL) {
		MEM_FREE(t->disc.title);
		t->disc.title = NULL;
	}
	if (t->disc.performer != NULL) {
		MEM_FREE(t->disc.performer);
		t->disc.performer = NULL;
	}
	if (t->disc.songwriter != NULL) {
		MEM_FREE(t->disc.songwriter);
		t->disc.songwriter = NULL;
	}
	if (t->disc.composer != NULL) {
		MEM_FREE(t->disc.composer);
		t->disc.composer = NULL;
	}
	if (t->disc.arranger != NULL) {
		MEM_FREE(t->disc.arranger);
		t->disc.arranger = NULL;
	}
	if (t->disc.message != NULL) {
		MEM_FREE(t->disc.message);
		t->disc.message = NULL;
	}
	if (t->disc.catno != NULL) {
		MEM_FREE(t->disc.catno);
		t->disc.catno = NULL;
	}
	for (i = 0; i < MAXTRACK; i++) {
		if (t->track[i].title != NULL) {
			MEM_FREE(t->track[i].title);
			t->track[i].title = NULL;
		}
		if (t->track[i].performer != NULL) {
			MEM_FREE(t->track[i].performer);
			t->track[i].performer = NULL;
		}
		if (t->track[i].songwriter != NULL) {
			MEM_FREE(t->track[i].songwriter);
			t->track[i].songwriter = NULL;
		}
		if (t->track[i].composer != NULL) {
			MEM_FREE(t->track[i].composer);
			t->track[i].composer = NULL;
		}
		if (t->track[i].arranger != NULL) {
			MEM_FREE(t->track[i].arranger);
			t->track[i].arranger = NULL;
		}
		if (t->track[i].message != NULL) {
			MEM_FREE(t->track[i].message);
			t->track[i].message = NULL;
		}
		if (t->track[i].catno != NULL) {
			MEM_FREE(t->track[i].catno);
			t->track[i].catno = NULL;
		}
	}
}


/*
 * di_playmode
 *	Init/halt the CDDA mode
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
di_playmode(curstat_t *s)
{
	switch (s->mode) {
	case MOD_STOP:
	case MOD_NODISC:
	case MOD_BUSY:
		break;
	default:
		/* Stop playback first before changing mode */
		di_stop(s, TRUE);
		break;
	}

	/* Set clip frames */
	if (PLAYMODE_IS_STD(app_data.play_mode))
		di_clip_frames = CLIP_FRAMES;
	else
		di_clip_frames = 0;

	if (ditbl[app_data.di_method].playmode != NULL)
		return (ditbl[app_data.di_method].playmode(s));

	return FALSE;
}


/*
 * di_check_disc
 *	Check if disc is ready for use
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
bool_t
di_check_disc(curstat_t *s)
{
	if (ditbl[app_data.di_method].check_disc != NULL)
		return (ditbl[app_data.di_method].check_disc(s));

	return FALSE;
}


/*
 * di_status_upd
 *	Force update of playback status
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_status_upd(curstat_t *s)
{
	if (ditbl[app_data.di_method].status_upd != NULL)
		ditbl[app_data.di_method].status_upd(s);
}


/*
 * di_lock
 *	Caddy lock function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable caddy lock
 *
 * Return:
 *	Nothing.
 */
void
di_lock(curstat_t *s, bool_t enable)
{
	if (ditbl[app_data.di_method].lock != NULL)
		ditbl[app_data.di_method].lock(s, enable);
}


/*
 * di_repeat
 *	Repeat mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable repeat mode
 *
 * Return:
 *	Nothing.
 */
void
di_repeat(curstat_t *s, bool_t enable)
{
	if (ditbl[app_data.di_method].repeat != NULL)
		ditbl[app_data.di_method].repeat(s, enable);
}


/*
 * di_shuffle
 *	Shuffle mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	enable - whether to enable/disable shuffle mode
 *
 * Return:
 *	Nothing.
 */
void
di_shuffle(curstat_t *s, bool_t enable)
{
	if (ditbl[app_data.di_method].shuffle != NULL)
		ditbl[app_data.di_method].shuffle(s, enable);
}


/*
 * di_load_eject
 *	CD caddy load and eject function.  If disc caddy is not
 *	loaded, it will attempt to load it.  Otherwise, it will be
 *	ejected.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_load_eject(curstat_t *s)
{
	if (ditbl[app_data.di_method].load_eject != NULL)
		ditbl[app_data.di_method].load_eject(s);
}


/*
 * di_ab
 *	A->B segment play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_ab(curstat_t *s)
{
	if (ditbl[app_data.di_method].ab != NULL)
		ditbl[app_data.di_method].ab(s);
}


/*
 * di_sample
 *	Sample play mode function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_sample(curstat_t *s)
{
	if (ditbl[app_data.di_method].sample != NULL)
		ditbl[app_data.di_method].sample(s);
}


/*
 * di_level
 *	Audio volume control function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	level - The volume level to set to
 *	drag - Whether this is an update due to the user dragging the
 *		volume control slider thumb.  If this is FALSE, then
 *		a final volume setting has been found.
 *
 * Return:
 *	Nothing.
 */
void
di_level(curstat_t *s, byte_t level, bool_t drag)
{
	if (ditbl[app_data.di_method].level != NULL)
		ditbl[app_data.di_method].level(s, level, drag);
}


/*
 * di_play_pause
 *	Audio playback and pause function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_play_pause(curstat_t *s)
{
	if (ditbl[app_data.di_method].play_pause != NULL)
		ditbl[app_data.di_method].play_pause(s);
}


/*
 * di_stop
 *	Stop function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	stop_disc - Whether to actually spin down the disc or just
 *		update status.
 *
 * Return:
 *	Nothing.
 */
void
di_stop(curstat_t *s, bool_t stop_disc)
{
	if (ditbl[app_data.di_method].stop != NULL)
		ditbl[app_data.di_method].stop(s, stop_disc);
}


/*
 * di_chgdisc
 *	Change disc function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_chgdisc(curstat_t *s)
{
	if (ditbl[app_data.di_method].chgdisc != NULL)
		ditbl[app_data.di_method].chgdisc(s);
}


/*
 * di_prevtrk
 *	Previous track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_prevtrk(curstat_t *s)
{
	if (ditbl[app_data.di_method].prevtrk != NULL)
		ditbl[app_data.di_method].prevtrk(s);
}


/*
 * di_nexttrk
 *	Next track function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_nexttrk(curstat_t *s)
{
	if (ditbl[app_data.di_method].nexttrk != NULL)
		ditbl[app_data.di_method].nexttrk(s);
}


/*
 * di_previdx
 *	Previous index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_previdx(curstat_t *s)
{
	if (ditbl[app_data.di_method].previdx != NULL)
		ditbl[app_data.di_method].previdx(s);
}


/*
 * di_nextidx
 *	Next index function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_nextidx(curstat_t *s)
{
	if (ditbl[app_data.di_method].nextidx != NULL)
		ditbl[app_data.di_method].nextidx(s);
}


/*
 * di_rew
 *	Search-rewind function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_rew(curstat_t *s, bool_t start)
{
	if (ditbl[app_data.di_method].rew != NULL)
		ditbl[app_data.di_method].rew(s, start);
}


/*
 * di_ff
 *	Search-fast-forward function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_ff(curstat_t *s, bool_t start)
{
	if (ditbl[app_data.di_method].ff != NULL)
		ditbl[app_data.di_method].ff(s, start);
}


/*
 * di_warp
 *	Track warp function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_warp(curstat_t *s)
{
	if (ditbl[app_data.di_method].warp != NULL)
		ditbl[app_data.di_method].warp(s);
}


/*
 * di_route
 *	Channel routing function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_route(curstat_t *s)
{
	if (ditbl[app_data.di_method].route != NULL)
		ditbl[app_data.di_method].route(s);
}


/*
 * di_mute_on
 *	Mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_mute_on(curstat_t *s)
{
	if (ditbl[app_data.di_method].mute_on != NULL)
		ditbl[app_data.di_method].mute_on(s);
}


/*
 * di_mute_off
 *	Un-mute audio function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_mute_off(curstat_t *s)
{
	if (ditbl[app_data.di_method].mute_off != NULL)
		ditbl[app_data.di_method].mute_off(s);
}


/*
 * di_cddajitter
 *	CDDA jitter correction setting change notification function
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_cddajitter(curstat_t *s)
{
	if (ditbl[app_data.di_method].cddajitter != NULL)
		ditbl[app_data.di_method].cddajitter(s);
}


/*
 * di_debug
 *	Debug level change notification function
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
di_debug(void)
{
	if (ditbl[app_data.di_method].debug != NULL)
		ditbl[app_data.di_method].debug();
}


/*
 * di_start
 *	Start the SCSI pass-through module.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_start(curstat_t *s)
{
	if (app_data.debug & DBG_ALL)
		di_prncfg();	/* Print debug information */

	if (ditbl[app_data.di_method].start != NULL)
		ditbl[app_data.di_method].start(s);
}


/*
 * di_icon
 *	Handler for main window iconification/de-iconification
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	iconified - Whether the main window is iconified
 *
 * Return:
 *	Nothing.
 */
void
di_icon(curstat_t *s, bool_t iconified)
{
	if (ditbl[app_data.di_method].icon != NULL)
		ditbl[app_data.di_method].icon(s, iconified);
}


/*
 * di_halt
 *	Shut down the SCSI pass-through and vendor-unique modules.
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
di_halt(curstat_t *s)
{
	if (ditbl[app_data.di_method].halt != NULL)
		ditbl[app_data.di_method].halt(s);

	di_devunlock(s);
}


/*
 * di_dump_curstat
 *	Display contents of the the curstat_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
di_dump_curstat(curstat_t *s)
{
	int		i;

	(void) fprintf(errfp,
		       "\nDumping contents of curstat_t structure at 0x%lx:\n",
		       (unsigned long) s);
	(void) fprintf(errfp,
		       "curdev=%s\nfirst_disc=%d last_disc=%d\n",
		       s->curdev, s->first_disc, s->last_disc);
	(void) fprintf(errfp,
		       "cur_disc=%d prev_disc=%d\n",
		       s->cur_disc, s->prev_disc);
	(void) fprintf(errfp,
		       "mode=%d time_dpy=%d flags=0x%x\n",
		       s->mode, s->time_dpy, s->flags);
	(void) fprintf(errfp,
		       "first_trk=%d last_trk=%d tot_trks=%d mcn=\"%s\"\n",
		       s->first_trk, s->last_trk, s->tot_trks, s->mcn);
	(void) fprintf(errfp, "cur_trk=%d cur_idx=%d\n",
		       s->cur_trk, s->cur_idx);
	(void) fprintf(errfp,
		      "discpos_tot.msf=%02u:%02u:%02u discpos_tot.addr=0x%x\n",
		       s->discpos_tot.min, s->discpos_tot.sec,
		       s->discpos_tot.frame, s->discpos_tot.addr);
	(void) fprintf(errfp,
		       "curpos_tot.msf=%02u:%02u:%02u curpos_tot.addr=0x%x\n",
		       s->curpos_tot.min, s->curpos_tot.sec,
		       s->curpos_tot.frame, s->curpos_tot.addr);
	(void) fprintf(errfp,
		       "curpos_trk.msf=%02u:%02u:%02u curpos_trk.addr=0x%x\n",
		       s->curpos_tot.min, s->curpos_tot.sec,
		       s->curpos_tot.frame, s->curpos_tot.addr);

	for (i = 0; i < MAXTRACK; i++) {
		(void) fprintf(errfp,
			       "[%2d] trkno=%03d addr=%06d (0x%05x) "
			       "msf=%02d:%02d:%02d type=%d isrc=\"%s\"\n",
			       i, s->trkinfo[i].trkno,
			       s->trkinfo[i].addr, s->trkinfo[i].addr,
			       s->trkinfo[i].min, s->trkinfo[i].sec,
			       s->trkinfo[i].frame, s->trkinfo[i].type,
			       s->trkinfo[i].isrc);
		if (s->trkinfo[i].trkno == LEAD_OUT_TRACK)
			break;
	}

	(void) fprintf(errfp, "playorder=");
	for (i = 0; i < (int) s->prog_tot; i++)
		(void) fprintf(errfp, "%d ",
			s->trkinfo[s->trkinfo[i].playorder].trkno);
	(void) fprintf(errfp, "\n");

	(void) fprintf(errfp,
		"sav_iaddr=0x%x rptcnt=%d repeat=%d shuffle=%d program=%d\n",
		s->sav_iaddr, s->rptcnt, s->repeat, s->shuffle, s->program);
	(void) fprintf(errfp,
		"onetrk_prog=%d caddy_lk=%d prog_tot=%d prog_cnt=%d\n",
		s->onetrk_prog, s->caddy_lock,
		s->prog_tot, s->prog_cnt);
	(void) fprintf(errfp,
		"chgrscan=%d level=%d "
		"level_left=%d level_right=%d cdda_att=%d\n",
		s->chgrscan, s->level, s->level_left, s->level_right,
		s->cdda_att);
	(void) fprintf(errfp,
		"vendor=\"%-8s\" prod=\"%-16s\" revnum=\"%-4s\"\n",
		s->vendor, s->prod, s->revnum);
	(void) fprintf(errfp, "outf_tmpl=%s\n",
		s->outf_tmpl == NULL ? "NULL" : s->outf_tmpl);
}


/*
 * di_common_parmload
 *	Load the common configuration file and initialize parameters.
 *
 * Args:
 *	path   - Path name to the file to load.
 *	priv   - Whether the privileged keywords are to be recognized.
 *	reload - This is a reload operation
 *
 * Return:
 *	Nothing.
 */
void
di_common_parmload(char *path, bool_t priv, bool_t reload)
{
	FILE		*fp;
	char		*buf,
			*parm,
			errstr[ERR_BUF_SZ],
			trypath[FILE_PATH_SZ];
	struct utsname	*un;
	bool_t		notry2;
	static bool_t	force_debug;
#ifndef __VMS
	pid_t		cpid;
	waitret_t	wstat;
	int		ret,
			pfd[2];

	un = util_get_uname();

	if (priv && di_isdemo())
		app_data.device = "(sim1)";

	if (PIPE(pfd) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"di_common_parmload: pipe failed (errno=%d)\n",
			errno);
		if (priv && !di_isdemo()) {
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_FATAL(errstr);
		}
		return;
	}
	
	switch (cpid = FORK()) {
	case 0:
		/* Child */

		/* Close un-needed pipe descriptor */
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) close(pfd[1]);
			_exit(1);
		}

		notry2 = FALSE;
		if (((int) strlen(path) + (int) strlen(un->nodename) + 2)
		    > FILE_PATH_SZ) {
			DBGPRN(DBG_GEN)(errfp, "NOTICE: %s: %s\n",
				"Host-specific config files not used",
				"Name too long");
			(void) strcpy(trypath, path);
			notry2 = TRUE;
		}
		else
			/* Try host-specific config file first */
			(void) sprintf(trypath, "%s-%s", path, un->nodename);

		DBGPRN(DBG_GEN)(errfp,
			"Loading common parameters: %s\n", trypath);
		fp = fopen(trypath, "r");

		if (fp == NULL && !notry2) {
			DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n",
					trypath);

			/* Try generic config file */
			(void) strcpy(trypath, path);

			DBGPRN(DBG_GEN)(errfp,
				"Loading common parameters: %s\n",
				trypath);
			fp = fopen(trypath, "r");
		}

		if (fp == NULL) {
			DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n",
					trypath);
			(void) close(pfd[1]);

			if (priv && !di_isdemo())
				_exit(3);
			_exit(0);
		}

		/* Allocate temporary buffer */
		if ((buf = (char *) MEM_ALLOC("buf", PARM_BUF_SZ)) == NULL) {
			(void) close(pfd[1]);
			(void) fclose(fp);
			_exit(2);
		}

		while (fgets(buf, PARM_BUF_SZ, fp) != NULL) {
			/* Skip comments and blank lines */
			if (buf[0] == '#' || buf[0] == '!' || buf[0] == '\n')
				continue;
			(void) write(pfd[1], buf, strlen(buf));
		}
		(void) write(pfd[1], ".\n", 2);

		(void) close(pfd[1]);
		(void) fclose(fp);

		MEM_FREE(buf);
		_exit(0);
		/*NOTREACHED*/

	case -1:
		DBGPRN(DBG_GEN)(errfp,
			"di_common_parmload: fork failed (errno=%d)\n",
			errno);
		(void) close(pfd[0]);
		(void) close(pfd[1]);

		if (priv && !di_isdemo()) {
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_FATAL(errstr);
		}
		return;

	default:
		/* Parent */

		/* Close un-needed pipe descriptor */
		(void) close(pfd[1]);

		if ((fp = fdopen(pfd[0], "r")) == NULL) {
			DBGPRN(DBG_GEN)(errfp,
			    "di_common_parmload: read pipe fdopen failed\n");
			if (priv && !di_isdemo()) {
				/* Cannot open pipe */
				(void) sprintf(errstr, app_data.str_nocfg,
					       path);
				DI_FATAL(errstr);
			}
			return;
		}
		break;
	}
#else
	un = util_get_uname();

	notry2 = FALSE;
	if (((int) strlen(path) + (int) strlen(un->nodename) + 2)
	    > FILE_PATH_SZ) {
		DBGPRN(DBG_GEN)(errfp, "NOTICE: %s: %s\n",
			"Host-specific config files not used",
			"Name too long");
		(void) strcpy(trypath, path);
		notry2 = TRUE;
	}
	else
		/* Try host-specific config file first */
		(void) sprintf(trypath, "%s-%s", path, un->nodename);

	DBGPRN(DBG_GEN)(errfp, "Loading common parameters: %s\n", trypath);
	fp = fopen(trypath, "r");

	if (fp == NULL && !notry2) {
		DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n", trypath);

		/* Try generic config file */
		(void) strcpy(trypath, path);

		DBGPRN(DBG_GEN)(errfp,
			"Loading common parameters: %s\n", trypath);
		fp = fopen(trypath, "r");
	}

	if (fp == NULL) {
		DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n", trypath);
		if (priv && !di_isdemo()) {
			/* Cannot open system config file. */
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_FATAL(errstr);
		}
		return;
	}
#endif	/* __VMS */

	/* Allocate temporary buffer */
	buf = (char *) MEM_ALLOC("buf", PARM_BUF_SZ);
	parm = (char *) MEM_ALLOC("parmbuf", PARM_BUF_SZ);
	if (buf == NULL || parm == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return;
	}

	if ((priv && (app_data.debug & DBG_ALL) != 0) || reload)
		force_debug = TRUE;
	else
		force_debug = FALSE;

	/* Read in common parameters */
	while (fgets(buf, PARM_BUF_SZ, fp) != NULL) {
		/* Skip comments and blank lines */
		if (buf[0] == '#' || buf[0] == '!' || buf[0] == '\n')
			continue;

		/* Done */
		if (buf[0] == '.' && buf[1] == '\n')
			break;

		if (priv && sscanf(buf, "device: %[^\n]\n", parm) > 0) {
			if (di_isdemo())
				continue;

			/* If app_data.device is not NULL, then it means
			 * the user had specified a device on the command
			 * line, and that should take precedence.
			 */
			if (app_data.device == NULL &&
			    !util_newstr(&app_data.device, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "outputPort: %s\n", parm) > 0) {
			app_data.outport = (word32_t) atoi(parm);
			continue;
		}
		if (sscanf(buf, "cdinfoPath: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.cdinfo_path, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "charsetConvMode: %s\n", parm) > 0) {
			app_data.chset_xlat = atoi(parm);
			continue;
		}
		if (sscanf(buf, "langUtf8: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.lang_utf8, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "acceptFuzzyDefault: %s\n", parm) > 0) {
			app_data.single_fuzzy = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "proxyServer: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.proxy_server, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "cddbCacheTimeout: %s\n", parm) > 0) {
			app_data.cache_timeout = atoi(parm);
			continue;
		}
		if (sscanf(buf, "serviceTimeout: %s\n", parm) > 0) {
			app_data.srv_timeout = atoi(parm);
			continue;
		}
		if (sscanf(buf, "localDiscographyMode: %s\n", parm) > 0) {
			app_data.discog_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "maximumHistory: %s\n", parm) > 0) {
			app_data.cdinfo_maxhist = atoi(parm);
			continue;
		}
		if (sscanf(buf, "internetOffline: %s\n", parm) > 0) {
			app_data.cdinfo_inetoffln = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "cddbUseProxy: %s\n", parm) > 0) {
			app_data.use_proxy = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "proxyAuthorization: %s\n", parm) > 0) {
			app_data.proxy_auth = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "autoMusicBrowser: %s\n", parm) > 0) {
			app_data.auto_musicbrowser = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "cdinfoFileMode: %s\n", parm) > 0) {
			if (!util_newstr(&app_data.cdinfo_filemode, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "historyFileMode: %s\n", parm) > 0) {
			if (!util_newstr(&app_data.hist_filemode, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "statusPollInterval: %s\n", parm) > 0) {
			app_data.stat_interval = atoi(parm);
			continue;
		}
		if (sscanf(buf, "insertPollInterval: %s\n", parm) > 0) {
			app_data.ins_interval = atoi(parm);
			continue;
		}
		if (sscanf(buf, "insertPollDisable: %s\n", parm) > 0) {
			app_data.ins_disable = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "previousThreshold: %s\n", parm) > 0) {
			app_data.prev_threshold = atoi(parm);
			continue;
		}
		if (sscanf(buf, "sampleBlocks: %s\n", parm) > 0) {
			app_data.sample_blks = atoi(parm);
			continue;
		}
		if (sscanf(buf, "timeDisplayMode: %s\n", parm) > 0) {
			app_data.timedpy_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "solaris2VolumeManager: %s\n", parm) > 0) {
			app_data.sol2_volmgt = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "showScsiErrMsg: %s\n", parm) > 0) {
			app_data.scsierr_msg = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "curfileEnable: %s\n", parm) > 0) {
			app_data.write_curfile = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "tooltipEnable: %s\n", parm) > 0) {
			app_data.tooltip_enable = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "tooltipDelayInterval: %s\n", parm) > 0) {
			app_data.tooltip_delay = atoi(parm);
			continue;
		}
		if (sscanf(buf, "tooltipActiveInterval: %s\n", parm) > 0) {
			app_data.tooltip_time = atoi(parm);
			continue;
		}
		if (sscanf(buf, "historyFileDisable: %s\n", parm) > 0) {
			app_data.histfile_dsbl = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "remoteControlEnable: %s\n", parm) > 0) {
			app_data.remote_enb = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "remoteControlLog: %s\n", parm) > 0) {
			app_data.remote_log = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "cddaFilePerTrack: %s\n", parm) > 0) {
			app_data.cdda_trkfile = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "cddaSpaceToUnderscore: %s\n", parm) > 0) {
			app_data.subst_underscore = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "cddaFileFormat: %s\n", parm) > 0) {
			app_data.cdda_filefmt = atoi(parm);
			continue;
		}
		if (sscanf(buf, "cddaFileTemplate: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.cdda_tmpl, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "cddaPipeProgram: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.pipeprog, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "cddaSchedOptions: %s\n", parm) > 0) {
			app_data.cdda_sched = atoi(parm);
			continue;
		}
		if (sscanf(buf, "cddaHeartbeatTimeout: %s\n", parm) > 0) {
			app_data.hb_timeout = atoi(parm);
			continue;
		}
		if (sscanf(buf, "compressionMode: %s\n", parm) > 0) {
			app_data.comp_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "compressionBitrate: %s\n", parm) > 0) {
			app_data.bitrate = atoi(parm);
			continue;
		}
		if (sscanf(buf, "minimumBitrate: %s\n", parm) > 0) {
			app_data.bitrate_min = atoi(parm);
			continue;
		}
		if (sscanf(buf, "maximumBitrate: %s\n", parm) > 0) {
			app_data.bitrate_max = atoi(parm);
			continue;
		}
		if (sscanf(buf, "compressionQuality: %s\n", parm) > 0) {
			app_data.qual_factor = atoi(parm);
			continue;
		}
		if (sscanf(buf, "channelMode: %s\n", parm) > 0) {
			app_data.chan_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "compressionAlgorithm: %s\n", parm) > 0) {
			app_data.comp_algo = atoi(parm);
			continue;
		}
		if (sscanf(buf, "lowpassMode: %s\n", parm) > 0) {
			app_data.lowpass_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "lowpassFrequency: %s\n", parm) > 0) {
			app_data.lowpass_freq = atoi(parm);
			continue;
		}
		if (sscanf(buf, "lowpassWidth: %s\n", parm) > 0) {
			app_data.lowpass_width = atoi(parm);
			continue;
		}
		if (sscanf(buf, "highpassMode: %s\n", parm) > 0) {
			app_data.highpass_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "highpassFrequency: %s\n", parm) > 0) {
			app_data.highpass_freq = atoi(parm);
			continue;
		}
		if (sscanf(buf, "highpassWidth: %s\n", parm) > 0) {
			app_data.highpass_width = atoi(parm);
			continue;
		}
		if (sscanf(buf, "copyrightFlag: %s\n", parm) > 0) {
			app_data.copyright = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "originalFlag: %s\n", parm) > 0) {
			app_data.original = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "noBitReservoirFlag: %s\n", parm) > 0) {
			app_data.nores = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "checksumFlag: %s\n", parm) > 0) {
			app_data.checksum = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "strictISO: %s\n", parm) > 0) {
			app_data.strict_iso = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "lameOptionsMode: %s\n", parm) > 0) {
			app_data.lameopts_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "lameOptions: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.lame_opts, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "addInfoTag: %s\n", parm) > 0) {
			app_data.add_tag = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "id3TagMode: %s\n", parm) > 0) {
			app_data.id3tag_mode = atoi(parm);
			continue;
		}
		if (sscanf(buf, "spinDownOnLoad: %s\n", parm) > 0) {
			app_data.load_spindown = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "playOnLoad: %s\n", parm) > 0) {
			app_data.load_play = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "ejectOnDone: %s\n", parm) > 0) {
			app_data.done_eject = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "exitOnDone: %s\n", parm) > 0) {
			app_data.done_exit = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "ejectOnExit: %s\n", parm) > 0) {
			app_data.exit_eject = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "stopOnExit: %s\n", parm) > 0) {
			app_data.exit_stop = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "exitOnEject: %s\n", parm) > 0) {
			app_data.eject_exit = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "repeatMode: %s\n", parm) > 0) {
			app_data.repeat_mode = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "shuffleMode: %s\n", parm) > 0) {
			app_data.shuffle_mode = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "discogURLPrefix: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.discog_url_pfx, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
		if (sscanf(buf, "autoMotdDisable: %s\n", parm) > 0) {
			app_data.automotd_dsbl = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "debugLevel: %s\n", parm) > 0) {
			if (!force_debug)
				app_data.debug = (word32_t) atoi(parm);
			continue;
		}
		if (sscanf(buf, "excludeWords: %[^\n]\n", parm) > 0) {
			if (strcmp(parm, "-") == 0)
				parm[0] = '\0';
			if (!util_newstr(&app_data.exclude_words, parm)) {
				DI_FATAL(app_data.str_nomemory);
				return;
			}
			continue;
		}
	}

	MEM_FREE(buf);
	MEM_FREE(parm);

	(void) fclose(fp);

	/* In case of error */
	if (app_data.device == NULL || app_data.device[0] == '\0')
		app_data.device = "/dev/cdrom";
	if (app_data.lang_utf8 == NULL || app_data.lang_utf8[0] == '\0')
		app_data.lang_utf8 = "UTF-8";
	if (app_data.timedpy_mode < 0 ||
	    app_data.timedpy_mode >= TIMEDPY_MAX_MODES)
		app_data.timedpy_mode = 0;
	if (app_data.tooltip_delay < 0)
		app_data.tooltip_delay = 1000;
	if (app_data.tooltip_time < 0)
		app_data.tooltip_delay = 3000;
	if (app_data.stat_interval <= 0)
		app_data.stat_interval = 260;
	if (app_data.cdinfo_maxhist < 0)
		app_data.cdinfo_maxhist = 0;
	if (app_data.cache_timeout <= 0)
		app_data.cache_timeout = DEF_CACHE_TIMEOUT;
	if (app_data.srv_timeout <= 0)
		app_data.srv_timeout = DEF_SRV_TIMEOUT;
	if (app_data.cdda_filefmt < 0 || app_data.cdda_filefmt >= MAX_FILEFMTS)
		app_data.cdda_filefmt = FILEFMT_RAW;
	if (app_data.comp_mode < 0 || app_data.comp_mode > 3)
		app_data.comp_mode = 0;
	if (app_data.qual_factor < 1 || app_data.qual_factor > 10)
		app_data.qual_factor = 3;
	if (app_data.chan_mode < 0 || app_data.chan_mode > 3)
		app_data.chan_mode = 0;
	if (app_data.comp_algo < 1 || app_data.comp_algo > 10)
		app_data.comp_algo = 4;
	if (app_data.lowpass_mode < 0 || app_data.lowpass_mode > 2)
		app_data.lowpass_mode = 0;
	if (app_data.highpass_mode < 0 || app_data.highpass_mode > 2)
		app_data.highpass_mode = 0;
	if (app_data.lowpass_freq < MIN_LOWPASS_FREQ ||
	    app_data.lowpass_freq > MAX_LOWPASS_FREQ)
		app_data.lowpass_freq = MAX_LOWPASS_FREQ;
	if (app_data.lowpass_width < 0)
		app_data.lowpass_width = 0;
	if (app_data.highpass_freq < MIN_HIGHPASS_FREQ ||
	    app_data.highpass_freq > MAX_HIGHPASS_FREQ)
		app_data.highpass_freq = MIN_HIGHPASS_FREQ;
	if (app_data.highpass_width < 0)
		app_data.highpass_width = 0;
	if (app_data.lameopts_mode < 0 || app_data.lameopts_mode > 3)
		app_data.lameopts_mode = 0;
	if (app_data.id3tag_mode < 1 || app_data.id3tag_mode > 3)
		app_data.id3tag_mode = 3;

	if (app_data.cdda_sched < 0 || app_data.cdda_sched > 3)
		app_data.cdda_sched = 0;
	if (app_data.hb_timeout < MIN_HB_TIMEOUT)
		app_data.hb_timeout = DEF_HB_TIMEOUT;

	/* playOnLoad overrides spinDownOnLoad */
	if (app_data.load_play)
		app_data.load_spindown = FALSE;

	/* Check validity of bitrates */
	if (!di_bitrate_valid(app_data.bitrate))
		app_data.bitrate = 0;
	if (!di_bitrate_valid(app_data.bitrate_min))
		app_data.bitrate_min = 0;
	if (!di_bitrate_valid(app_data.bitrate_max))
		app_data.bitrate_min = 0;
	if (app_data.bitrate_min > 0 && app_data.bitrate > 0 &&
	    (app_data.bitrate_min > app_data.bitrate ||
	     app_data.bitrate_min > app_data.bitrate_max))
		app_data.bitrate_min = 0;
	if (app_data.bitrate_max > 0 && app_data.bitrate > 0 &&
	    (app_data.bitrate_max < app_data.bitrate ||
	     app_data.bitrate_max < app_data.bitrate_min))
		app_data.bitrate_min = 0;

#ifdef __VMS
	/* Force space/tab substitution to underscores on VMS */
	app_data.subst_underscore = TRUE;
#else
	/* Wait for child to exit */
	ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			     NULL, 0, FALSE, &wstat);
	if (ret < 0) {
		DBGPRN(DBG_GEN)(errfp, "di_common_parmload: "
				"waitpid failed (errno=%d)\n", errno);
	}
	else if (WIFEXITED(wstat)) {
		if (WEXITSTATUS(wstat) != 0) {
			DBGPRN(DBG_GEN)(errfp, "di_common_parmload: "
					"child exited (status=%d)\n",
					WEXITSTATUS(wstat));
			(void) sprintf(errstr, app_data.str_nocfg, path);
			if (priv && !di_isdemo()) {
				DI_FATAL(errstr);
			}
			else {
				DI_WARNING(errstr);
			}
		}
	}
	else if (WIFSIGNALED(wstat)) {
		DBGPRN(DBG_GEN)(errfp, "di_common_parmload: "
				"child killed (signal=%d)\n",
				WTERMSIG(wstat));
		(void) sprintf(errstr, app_data.str_nocfg, path);
		if (priv && !di_isdemo()) {
			DI_FATAL(errstr);
		}
		else {
			DI_WARNING(errstr);
		}
	}
#endif
}


/*
 * di_devspec_parmload
 *	Load the specified device-specific configuration file and
 *	initialize parameters.
 *
 * Args:
 *	path   - Path name to the file to load.
 *	priv   - Whether the privileged keywords are to be recognized.
 *	reload - This is a reload operation.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
di_devspec_parmload(char *path, bool_t priv, bool_t reload)
{
	FILE		*fp;
	char		*buf,
			*parm,
			errstr[ERR_BUF_SZ],
			trypath[FILE_PATH_SZ];
	struct utsname	*un;
	curstat_t	*s = NULL;
	bool_t		notry2;
#ifndef __VMS
	pid_t		cpid;
	waitret_t	wstat;
	int		ret,
			pfd[2];

	un = util_get_uname();

	if (di_clinfo != NULL)
		s = di_clinfo->curstat_addr();

	if (priv && di_isdemo()) {
		char	*cp;

		cp = "(sim1);(sim2);(sim3);(sim4);(sim5);(sim6);(sim7);(sim8)";
		if (!util_newstr(&app_data.devlist, cp)) {
			DI_FATAL(app_data.str_nomemory);
			return;
		}
		app_data.numdiscs = 8;
		app_data.chg_method = CHG_SCSI_LUN;
		app_data.multi_play = TRUE;
	}

	if (PIPE(pfd) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"di_devspec_parmload: pipe failed (errno=%d)\n",
			errno);
		if (priv && !di_isdemo()) {
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_FATAL(errstr);
		}
		return;
	}

	switch (cpid = FORK()) {
	case 0:
		/* Child */

		/* Close un-needed pipe descriptor */
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) close(pfd[1]);
			_exit(1);
		}

		notry2 = FALSE;
		if (((int) strlen(path) + (int) strlen(un->nodename) + 2)
		    > FILE_PATH_SZ) {
			DBGPRN(DBG_GEN)(errfp, "NOTICE: %s: %s\n",
				"Host-specific config files not used",
				"Name too long");
			(void) strcpy(trypath, path);
			notry2 = TRUE;
		}
		else
			/* Try host-specific config file first */
			(void) sprintf(trypath, "%s-%s", path, un->nodename);

		DBGPRN(DBG_GEN)(errfp,
			"Loading device-specific parameters: %s\n", trypath);

		fp = fopen(trypath, "r");

		if (fp == NULL && !notry2) {
			DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n",
					trypath);

			/* Try generic config file */
			(void) strcpy(trypath, path);

			DBGPRN(DBG_GEN)(errfp,
				"Loading device-specific parameters: %s\n",
				trypath);
			fp = fopen(trypath, "r");
		}

		if (fp == NULL) {
			DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n",
					trypath);
			(void) close(pfd[1]);

			if (priv && !di_isdemo())
				_exit(3);
			_exit(0);
		}

		/* Allocate temporary buffer */
		if ((buf = (char *) MEM_ALLOC("buf", PARM_BUF_SZ)) == NULL) {
			(void) close(pfd[1]);
			(void) fclose(fp);
			_exit(2);
		}

		while (fgets(buf, PARM_BUF_SZ, fp) != NULL) {
			/* Skip comments and blank lines */
			if (buf[0] == '#' || buf[0] == '!' || buf[0] == '\n')
				continue;
			(void) write(pfd[1], buf, strlen(buf));
		}
		(void) write(pfd[1], ".\n", 2);

		(void) close(pfd[1]);
		(void) fclose(fp);

		MEM_FREE(buf);
		_exit(0);
		/*NOTREACHED*/

	case -1:
		DBGPRN(DBG_GEN)(errfp,
			"di_devspec_parmload: fork failed (errno=%d)\n",
			errno);
		(void) close(pfd[0]);
		(void) close(pfd[1]);

		if (priv && !di_isdemo()) {
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_FATAL(errstr);
		}
		return;

	default:
		/* Parent */

		/* Close un-needed pipe descriptor */
		(void) close(pfd[1]);

		if ((fp = fdopen(pfd[0], "r")) == NULL) {
			DBGPRN(DBG_GEN)(errfp,
			    "di_devspec_parmload: read pipe fdopen failed\n");
			if (priv && !di_isdemo()) {
				/* Cannot open pipe */
				(void) sprintf(errstr, app_data.str_nocfg,
					       path);
				DI_FATAL(errstr);
			}
			return;
		}
		break;
	}
#else
	un = util_get_uname();

	notry2 = FALSE;
	if (((int) strlen(path) + (int) strlen(un->nodename) + 2)
	    > FILE_PATH_SZ) {
		DBGPRN(DBG_GEN)(errfp, "NOTICE: %s: %s\n",
			"Host-specific config files not used",
			"Name too long");
		(void) strcpy(trypath, path);
		notry2 = TRUE;
	}
	else
		/* Try host-specific config file first */
		(void) sprintf(trypath, "%s-%s", path, un->nodename);

	DBGPRN(DBG_GEN)(errfp,
		"Loading device-specific parameters: %s\n", trypath);
	fp = fopen(trypath, "r");

	if (fp == NULL && !notry2) {
		DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n", trypath);

		/* Try generic config file */
		(void) strcpy(trypath, path);

		DBGPRN(DBG_GEN)(errfp,
			"Loading device-specific parameters: %s\n", trypath);
		fp = fopen(trypath, "r");
	}

	if (fp == NULL) {
		DBGPRN(DBG_GEN)(errfp, "    Cannot open %s\n", trypath);
		if (priv && !di_isdemo()) {
			/* Cannot open master device-specific
			 * config file.
			 */
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_FATAL(errstr);
		}
		return;
	}
#endif	/* __VMS */

	/* Allocate temporary buffer */
	buf = (char *) MEM_ALLOC("buf", PARM_BUF_SZ);
	parm = (char *) MEM_ALLOC("parmbuf", PARM_BUF_SZ);
	if (buf == NULL || parm == NULL) {
		DI_FATAL(app_data.str_nomemory);
		return;
	}

	/* Read in device-specific parameters */
	while (fgets(buf, PARM_BUF_SZ, fp) != NULL) {
		/* Skip comments and blank lines */
		if (buf[0] == '#' || buf[0] == '!' || buf[0] == '\n')
			continue;

		/* Done */
		if (buf[0] == '.' && buf[1] == '\n')
			break;

		/* These are privileged parameters and users
		 * cannot overide them in their .xmcdcfg file.
		 */
		if (priv) {
			if (sscanf(buf, "logicalDriveNumber: %s\n",
				   parm) > 0) {
				app_data.devnum = atoi(parm);
				continue;
			}
			if (sscanf(buf, "deviceList: %[^\n]\n", parm) > 0) {
				if (!util_newstr(&app_data.devlist, parm)) {
					DI_FATAL(app_data.str_nomemory);
					return;
				}
				continue;
			}
			if (sscanf(buf, "deviceInterfaceMethod: %s\n",
				   parm) > 0) {
				app_data.di_method = atoi(parm);
				continue;
			}
			if (sscanf(buf, "cddaMethod: %s\n", parm) > 0) {
				app_data.cdda_method = atoi(parm);
				continue;
			}
			if (sscanf(buf, "cddaReadMethod: %s\n", parm) > 0) {
				app_data.cdda_rdmethod = atoi(parm);
				continue;
			}
			if (sscanf(buf, "cddaWriteMethod: %s\n", parm) > 0) {
				app_data.cdda_wrmethod = atoi(parm);
				continue;
			}
			if (sscanf(buf, "cddaScsiModeSelect: %s\n",
				   parm) > 0) {
				app_data.cdda_modesel = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "cddaScsiDensity: %s\n", parm) > 0) {
				app_data.cdda_scsidensity = atoi(parm);
				continue;
			}
			if (sscanf(buf, "cddaScsiReadCommand: %s\n",
				   parm) > 0) {
				app_data.cdda_scsireadcmd = atoi(parm);
				continue;
			}
			if (sscanf(buf, "cddaReadChunkBlocks: %s\n",
				   parm) > 0) {
				app_data.cdda_readchkblks = atoi(parm);
				continue;
			}
			if (sscanf(buf, "cddaDataBigEndian: %s\n", parm) > 0) {
				app_data.cdda_bigendian = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "driveVendorCode: %s\n",
				   parm) > 0) {
				app_data.vendor_code = atoi(parm);
				continue;
			}
			if (sscanf(buf, "scsiVersionCheck: %s\n",
				   parm) > 0) {
				app_data.scsiverck = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "numDiscs: %s\n", parm) > 0) {
				app_data.numdiscs = atoi(parm);
				continue;
			}
			if (sscanf(buf, "mediumChangeMethod: %s\n",
				   parm) > 0) {
				app_data.chg_method = atoi(parm);
				continue;
			}
			if (sscanf(buf, "scsiAudioVolumeBase: %s\n",
				   parm) > 0) {
				app_data.base_scsivol = atoi(parm);
				continue;
			}
			if (sscanf(buf, "minimumPlayBlocks: %s\n", parm) > 0) {
				app_data.min_playblks = atoi(parm);
				continue;
			}
			if (sscanf(buf, "playAudio10Support: %s\n",
				   parm) > 0) {
				app_data.play10_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "playAudio12Support: %s\n",
				   parm) > 0) {
				app_data.play12_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "playAudioMSFSupport: %s\n",
				   parm) > 0) {
				app_data.playmsf_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "playAudioTISupport: %s\n",
				   parm) > 0) {
				app_data.playti_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "loadSupport: %s\n", parm) > 0) {
				app_data.load_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "ejectSupport: %s\n", parm) > 0) {
				app_data.eject_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "modeSenseSetDBD: %s\n", parm) > 0) {
				app_data.msen_dbd = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "modeSenseUse10Byte: %s\n",
				   parm) > 0) {
				app_data.msen_10 = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "volumeControlSupport: %s\n",
				   parm) > 0) {
				app_data.mselvol_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "balanceControlSupport: %s\n",
				   parm) > 0) {
				app_data.balance_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "channelRouteSupport: %s\n",
				   parm) > 0) {
				app_data.chroute_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "pauseResumeSupport: %s\n",
				   parm) > 0) {
				app_data.pause_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "strictPauseResume: %s\n", parm) > 0) {
				app_data.strict_pause_resume = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "playPausePlay: %s\n", parm) > 0) {
				app_data.play_pause_play = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "caddyLockSupport: %s\n", parm) > 0) {
				app_data.caddylock_supp = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "curposFormat: %s\n", parm) > 0) {
				app_data.curpos_fmt = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "noTURWhenPlaying: %s\n", parm) > 0) {
				app_data.play_notur = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "tocLBA: %s\n", parm) > 0) {
				app_data.toc_lba = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "subChannelLBA: %s\n", parm) > 0) {
				app_data.subq_lba = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "driveBlockSize: %s\n", parm) > 0) {
				app_data.drv_blksz = atoi(parm);
				continue;
			}
			if (sscanf(buf, "spinUpInterval: %s\n", parm) > 0) {
				app_data.spinup_interval = atoi(parm);
				continue;
			}
			if (sscanf(buf, "mcnDisable: %s\n", parm) > 0) {
				app_data.mcn_dsbl = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "isrcDisable: %s\n", parm) > 0) {
				app_data.isrc_dsbl = util_stob(parm);
				continue;
			}
			if (sscanf(buf, "cdTextDisable: %s\n", parm) > 0) {
				app_data.cdtext_dsbl = util_stob(parm);
				continue;
			}
		}

		/* These are general parameters that can be
		 * changed by the user.
		 */
		if (sscanf(buf, "playMode: %s\n", parm) > 0) {
			/* Only allow playmode to change while not playing */
			if (s == NULL ||
			    (s->mode != MOD_PLAY && s->mode != MOD_PAUSE &&
			     s->mode != MOD_SAMPLE)) {
				app_data.play_mode = atoi(parm);
			}
			continue;
		}
		if (sscanf(buf, "volumeControlTaper: %s\n", parm) > 0) {
			app_data.vol_taper = atoi(parm);
			continue;
		}
		if (sscanf(buf, "startupVolume: %s\n", parm) > 0) {
			app_data.startup_vol = atoi(parm);
			continue;
		}
		if (sscanf(buf, "channelRoute: %s\n", parm) > 0) {
			app_data.ch_route = atoi(parm);
			continue;
		}
		if (sscanf(buf, "searchSkipBlocks: %s\n", parm) > 0) {
			app_data.skip_blks = atoi(parm);
			continue;
		}
		if (sscanf(buf, "searchPauseInterval: %s\n", parm) > 0) {
			app_data.skip_pause = atoi(parm);
			continue;
		}
		if (sscanf(buf, "searchSpeedUpCount: %s\n", parm) > 0) {
			app_data.skip_spdup = atoi(parm);
			continue;
		}
		if (sscanf(buf, "searchVolumePercent: %s\n", parm) > 0) {
			app_data.skip_vol = atoi(parm);
			continue;
		}
		if (sscanf(buf, "searchMinVolume: %s\n", parm) > 0) {
			app_data.skip_minvol = atoi(parm);
			continue;
		}
		if (sscanf(buf, "closeOnEject: %s\n", parm) > 0) {
			app_data.eject_close = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "caddyLock: %s\n", parm) > 0) {
			app_data.caddy_lock = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "multiPlay: %s\n", parm) > 0) {
			app_data.multi_play = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "reversePlay: %s\n", parm) > 0) {
			app_data.reverse = util_stob(parm);
			continue;
		}
		if (sscanf(buf, "cddaJitterCorrection: %s\n", parm) > 0) {
			app_data.cdda_jitter_corr = util_stob(parm);
			continue;
		}
	}

	MEM_FREE(buf);
	MEM_FREE(parm);

	(void) fclose(fp);

#ifndef __VMS
	/* Wait for child to exit */
	ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			     NULL, 0, FALSE, &wstat);
	if (ret < 0) {
		DBGPRN(DBG_GEN)(errfp, "di_devspec_parmload: "
				"waitpid failed (errno=%d)\n", errno);
	}
	else if (WIFEXITED(wstat)) {
		if (WEXITSTATUS(wstat) != 0) {
			DBGPRN(DBG_GEN)(errfp, "di_devspec_parmload: "
					"child exited (status=%d)\n",
					WEXITSTATUS(wstat));
			(void) sprintf(errstr, app_data.str_nocfg, path);
			if (priv && !di_isdemo()) {
				DI_FATAL(errstr);
			}
			else {
				DI_WARNING(errstr);
			}
		}
	}
	else if (WIFSIGNALED(wstat)) {
		DBGPRN(DBG_GEN)(errfp, "di_devspec_parmload: "
				"child killed (signal=%d)\n",
				WTERMSIG(wstat));
		(void) sprintf(errstr, app_data.str_nocfg, path);
		if (priv && !di_isdemo()) {
			DI_FATAL(errstr);
		}
		else {
			DI_WARNING(errstr);
		}
	}
#endif

	if (!priv) {
		/* If the drive does not support software eject, then we
		 * can't lock the caddy.
		 */
		if (!app_data.eject_supp) {
			app_data.caddylock_supp = FALSE;
			app_data.done_eject = FALSE;
			app_data.exit_eject = FALSE;
		}

		/* If the drive does not support locking the caddy, don't
		 * attempt to lock it.
		 */
		if (!app_data.caddylock_supp)
			app_data.caddy_lock = FALSE;

		/* If the drive does not support software volume
		 * control, then it can't support the balance
		 * control either.  Also, force the volume control
		 * taper selector to the linear position.
		 */
		if (!app_data.mselvol_supp) {
			app_data.balance_supp = FALSE;
			app_data.vol_taper = VOLTAPER_LINEAR;
		}

		/* If the drive does not support channel routing,
		 * force the channel routing setting to normal.
		 */
		if (!app_data.chroute_supp)
			app_data.ch_route = 0;

		/* Other fix-ups as needed */
		if (app_data.numdiscs <= 0)
			app_data.numdiscs = 1;

		if (app_data.numdiscs == 1) {
			app_data.multi_play = app_data.reverse = FALSE;
		}

		if (app_data.startup_vol > 100)
			app_data.startup_vol = 100;
		else if (app_data.startup_vol < -1)
			app_data.startup_vol = -1;

		if (app_data.drv_blksz == 0 ||
		    (app_data.drv_blksz % 512) != 0)
			app_data.drv_blksz = STD_CDROM_BLKSZ;

		if (app_data.cdda_readchkblks < MIN_CDDA_CHUNK_BLKS ||
		    app_data.cdda_readchkblks > MAX_CDDA_CHUNK_BLKS)
			app_data.cdda_readchkblks = DEF_CDDA_CHUNK_BLKS;
	}
}


/*
 * di_common_parmsave
 *	Save the common configuration parameters to file.
 *
 * Args:
 *	path - Path name to the file to save to.
 *
 * Return:
 *	Nothing.
 */
void
di_common_parmsave(char *path)
{
	FILE		*fp;
	char		*truestr = "True",
			*falsestr = "False",
			*cp,
			filetmpl[FILE_PATH_SZ * 2];
	bool_t		user_tmpl;
#ifndef __VMS
	int		ret;
	pid_t		cpid;
	waitret_t	wstat;
	char		*dirpath,
			errstr[ERR_BUF_SZ];

	/* Fork child to perform actual I/O */
	switch (cpid = FORK()) {
	case 0:
		/* Child process */
		break;

	case -1:
		DBGPRN(DBG_GEN)(errfp,
			"di_common_parmsave: fork failed (errno=%d)\n",
			errno);
		return;

	default:
		/* Parent process: wait for child to exit */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     NULL, 0, FALSE, &wstat);
		if (ret < 0) {
			DBGPRN(DBG_GEN)(errfp, "di_common_parmsave: "
					"waitpid failed (errno=%d)\n", errno);
		}
		else if (WIFEXITED(wstat)) {
			if (WEXITSTATUS(wstat) != 0) {
				DBGPRN(DBG_GEN)(errfp, "di_common_parmsave: "
						"child exited (status=%d)\n",
						WEXITSTATUS(wstat));
				(void) sprintf(errstr, app_data.str_nocfg,
					       path);
				DI_WARNING(errstr);
			}
		}
		else if (WIFSIGNALED(wstat)) {
			DBGPRN(DBG_GEN)(errfp, "di_common_parmsave: "
					"child killed (signal=%d)\n",
					WTERMSIG(wstat));
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_WARNING(errstr);
		}
		return;
	}

	/* Force uid and gid to original setting */
	if (!util_set_ougid())
		_exit(1);

	if ((dirpath = util_dirname(path)) == NULL) {
		DI_FATAL("File dirname error");
		return;
	}
	if (!util_mkdir(dirpath, 0755)) {
		DBGPRN(DBG_GEN)(errfp,
			"di_common_parmsave: cannot mkdir %s.\n", dirpath);
		MEM_FREE(dirpath);
		_exit(2);
	}
	MEM_FREE(dirpath);
#endif	/* __VMS */

	if ((cp = strrchr(app_data.cdda_tmpl, '.')) != NULL)
		*cp = '\0';

	(void) sprintf(filetmpl,
#ifdef __VMS
		       "%%S.%%C.%%I]%s",
#else
		       "%%S/%%C/%%I/%s",
#endif
		       app_data.cdda_trkfile ? FILEPATH_TRACK : FILEPATH_DISC);
	user_tmpl = (bool_t) (strcmp(app_data.cdda_tmpl, filetmpl) != 0);

	if (cp != NULL)
		*cp = '.';

	DBGPRN(DBG_GEN)(errfp, "Writing device-specific file %s\n", path);

	/* Open file for writing */
	if ((fp = fopen(path, "w")) == NULL) {
		DBGPRN(DBG_GEN)(errfp,
			"di_common_parmsave: cannot open %s.\n", path);
#ifdef __VMS
		return;
#else
		_exit(2);
#endif
	}

	/* Write banner */
	(void) fprintf(fp, "# xmcd %s.%s Common Configuration File\n",
		       VERSION_MAJ, VERSION_MIN);
	(void) fprintf(fp, "# %s\n# %s\n#\n# %s %s%s\n# %s\n#\n",
		       COPYRIGHT, "Automatically generated -- DO NOT EDIT!",
		       "See the", app_data.libdir, "/config/common.cfg",
		       "file for details about the parameters.");

	/* Write only parameters configurable via the user interface */
	(void) fprintf(fp, "outputPort:\t\t%d\n", (int) app_data.outport);
	(void) fprintf(fp, "cddaFilePerTrack:\t%s\n",
		       app_data.cdda_trkfile ? truestr : falsestr);
	(void) fprintf(fp, "cddaSpaceToUnderscore:\t%s\n",
		       app_data.subst_underscore ? truestr : falsestr);
	(void) fprintf(fp, "cddaFileFormat:\t\t%d\n", app_data.cdda_filefmt);
	(void) fprintf(fp, "cddaFileTemplate:\t%.1000s\n",
		       user_tmpl ? app_data.cdda_tmpl : "-");
	(void) fprintf(fp, "cddaPipeProgram:\t%.1000s\n",
		       (app_data.pipeprog != NULL) ? app_data.pipeprog : "-");
	(void) fprintf(fp, "cddaSchedOptions:\t%d\n", app_data.cdda_sched);
	(void) fprintf(fp, "cddaHeartbeatTimeout:\t%d\n", app_data.hb_timeout);
	(void) fprintf(fp, "compressionMode:\t%d\n", app_data.comp_mode);
	(void) fprintf(fp, "compressionBitrate:\t%d\n", app_data.bitrate);
	(void) fprintf(fp, "minimumBitrate:\t\t%d\n", app_data.bitrate_min);
	(void) fprintf(fp, "maximumBitrate:\t\t%d\n", app_data.bitrate_max);
	(void) fprintf(fp, "compressionQuality:\t%d\n", app_data.qual_factor);
	(void) fprintf(fp, "channelMode:\t\t%d\n", app_data.chan_mode);
	(void) fprintf(fp, "compressionAlgorithm:\t%d\n", app_data.comp_algo);
	(void) fprintf(fp, "lowpassMode:\t\t%d\n", app_data.lowpass_mode);
	(void) fprintf(fp, "lowpassFrequency:\t%d\n", app_data.lowpass_freq);
	(void) fprintf(fp, "lowpassWidth:\t\t%d\n", app_data.lowpass_width);
	(void) fprintf(fp, "highpassMode:\t\t%d\n", app_data.highpass_mode);
	(void) fprintf(fp, "highpassFrequency:\t%d\n", app_data.highpass_freq);
	(void) fprintf(fp, "highpassWidth:\t\t%d\n", app_data.highpass_width);
	(void) fprintf(fp, "copyrightFlag:\t\t%s\n",
		       app_data.copyright ? truestr : falsestr);
	(void) fprintf(fp, "originalFlag:\t\t%s\n",
		       app_data.original ? truestr : falsestr);
	(void) fprintf(fp, "noBitReservoirFlag:\t%s\n",
		       app_data.nores ? truestr : falsestr);
	(void) fprintf(fp, "checksumFlag:\t\t%s\n",
		       app_data.checksum ? truestr : falsestr);
	(void) fprintf(fp, "strictISO:\t\t%s\n",
		       app_data.strict_iso ? truestr : falsestr);
	(void) fprintf(fp, "lameOptionsMode:\t%d\n", app_data.lameopts_mode);
	(void) fprintf(fp, "lameOptions:\t\t%.1000s\n",
		       (app_data.lame_opts != NULL) ?
		        app_data.lame_opts : "-");
	(void) fprintf(fp, "id3TagMode:\t\t%d\n", app_data.id3tag_mode);
	(void) fprintf(fp, "addInfoTag:\t\t%s\n",
		       app_data.add_tag ? truestr : falsestr);
	(void) fprintf(fp, "spinDownOnLoad:\t\t%s\n",
		       app_data.load_spindown ? truestr : falsestr);
	(void) fprintf(fp, "playOnLoad:\t\t%s\n",
		       app_data.load_play ? truestr : falsestr);
	(void) fprintf(fp, "ejectOnDone:\t\t%s\n",
		       app_data.done_eject ? truestr : falsestr);
	(void) fprintf(fp, "exitOnDone:\t\t%s\n",
		       app_data.done_exit ? truestr : falsestr);
	(void) fprintf(fp, "ejectOnExit:\t\t%s\n",
		       app_data.exit_eject ? truestr : falsestr);
	(void) fprintf(fp, "stopOnExit:\t\t%s\n",
		       app_data.exit_stop ? truestr : falsestr);
	(void) fprintf(fp, "exitOnEject:\t\t%s\n",
		       app_data.eject_exit ? truestr : falsestr);
	(void) fprintf(fp, "repeatMode:\t\t%s\n",
		       app_data.repeat_mode ? truestr : falsestr);
	(void) fprintf(fp, "shuffleMode:\t\t%s\n",
		       app_data.shuffle_mode ? truestr : falsestr);
	(void) fprintf(fp, "cdinfoPath:\t\t%s\n", app_data.cdinfo_path);
	(void) fprintf(fp, "charsetConvMode:\t%d\n",
		       app_data.chset_xlat);
	(void) fprintf(fp, "langUtf8:\t\t%s\n", app_data.lang_utf8);
	(void) fprintf(fp, "acceptFuzzyDefault:\t%s\n",
		       app_data.single_fuzzy ? truestr : falsestr);
	(void) fprintf(fp, "cddbUseProxy:\t\t%s\n",
		       app_data.use_proxy ? truestr : falsestr);
	(void) fprintf(fp, "proxyServer:\t\t%s\n", app_data.proxy_server);
	(void) fprintf(fp, "proxyAuthorization:\t%s\n",
		       app_data.proxy_auth ? truestr : falsestr);
	(void) fprintf(fp, "autoMusicBrowser:\t%s\n",
		       app_data.auto_musicbrowser ? truestr : falsestr);
	(void) fprintf(fp, "cddbCacheTimeout:\t%d\n", app_data.cache_timeout);
	(void) fprintf(fp, "serviceTimeout:\t\t%d\n", app_data.srv_timeout);
	(void) fprintf(fp, "autoMotdDisable:\t%s\n",
		       app_data.automotd_dsbl ? truestr : falsestr);

	if (fclose(fp) != 0) {
#ifdef __VMS
		return;
#else
		_exit(1);
#endif
	}

	(void) chmod(path, 0644);

	/* Child exits here */
#ifndef __VMS
	_exit(0);
#endif
	/*NOTREACHED*/
}


/*
 * di_devspec_parmsave
 *	Save the device-specific configuration parameters to file.
 *
 * Args:
 *	path - Path name to the file to save to.
 *
 * Return:
 *	Nothing.
 */
void
di_devspec_parmsave(char *path)
{
	FILE		*fp;
	char		*truestr = "True",
			*falsestr = "False";
#ifndef __VMS
	int		ret;
	pid_t		cpid;
	waitret_t	wstat;
	char		*dirpath,
			errstr[ERR_BUF_SZ];

	/* Fork child to perform actual I/O */
	switch (cpid = FORK()) {
	case 0:
		/* Child process */
		break;

	case -1:
		DBGPRN(DBG_GEN)(errfp,
			"di_devspec_parmsave: fork failed (errno=%d)\n",
			errno);
		return;

	default:
		/* Parent process: wait for child to exit */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     NULL, 0, FALSE, &wstat);
		if (ret < 0) {
			DBGPRN(DBG_GEN)(errfp, "di_devspec_parmsave: "
					"waitpid failed (errno=%d)\n", errno);
		}
		else if (WIFEXITED(wstat)) {
			if (WEXITSTATUS(wstat) != 0) {
				DBGPRN(DBG_GEN)(errfp, "di_devspec_parmsave: "
						"child exited (status=%d)\n",
						WEXITSTATUS(wstat));
				(void) sprintf(errstr, app_data.str_nocfg,
					       path);
				DI_WARNING(errstr);
			}
		}
		else if (WIFSIGNALED(wstat)) {
			DBGPRN(DBG_GEN)(errfp, "di_devspec_parmsave: "
					"child killed (signal=%d)\n",
					WTERMSIG(wstat));
			(void) sprintf(errstr, app_data.str_nocfg, path);
			DI_WARNING(errstr);
		}
		return;
	}

	/* Force uid and gid to original setting */
	if (!util_set_ougid())
		_exit(1);

	if ((dirpath = util_dirname(path)) == NULL) {
		DI_FATAL("File dirname error");
		return;
	}
	if (!util_mkdir(dirpath, 0755)) {
		DBGPRN(DBG_GEN)(errfp,
			"di_devspec_parmsave: cannot mkdir %s.\n", dirpath);
		MEM_FREE(dirpath);
		_exit(2);
	}
	MEM_FREE(dirpath);
#endif	/* __VMS */

	DBGPRN(DBG_GEN)(errfp, "Writing device-specific file %s\n", path);

	/* Open file for writing */
	if ((fp = fopen(path, "w")) == NULL) {
		DBGPRN(DBG_GEN)(errfp,
			"di_devspec_parmsave: cannot open %s.\n", path);
#ifdef __VMS
		return;
#else
		_exit(2);
#endif
	}

	/* Write banner */
	(void) fprintf(fp, "# xmcd %s.%s Device-Specific Configuration File\n",
		       VERSION_MAJ, VERSION_MIN);
	(void) fprintf(fp, "# %s\n# %s\n#\n# %s %s%s\n# %s\n#\n",
		       COPYRIGHT, "Automatically generated -- DO NOT EDIT!",
		       "See the", app_data.libdir, "/config/device.cfg",
		       "file for details about the parameters.");

	/* Write only user-changeable parameters */
	(void) fprintf(fp, "playMode:\t\t%d\n", app_data.play_mode);
	(void) fprintf(fp, "cddaJitterCorrection:\t%s\n",
		       app_data.cdda_jitter_corr ? truestr : falsestr);
	(void) fprintf(fp, "volumeControlTaper:\t%d\n", app_data.vol_taper);
	(void) fprintf(fp, "channelRoute:\t\t%d\n", app_data.ch_route);
	(void) fprintf(fp, "caddyLock:\t\t%s\n",
		       app_data.caddy_lock ? truestr : falsestr);
	(void) fprintf(fp, "multiPlay:\t\t%s\n",
		       app_data.multi_play ? truestr : falsestr);
	(void) fprintf(fp, "reversePlay:\t\t%s\n",
		       app_data.reverse ? truestr : falsestr);

	if (fclose(fp) != 0) {
#ifdef __VMS
		return;
#else
		_exit(1);
#endif
	}

	(void) chmod(path, 0644);

	/* Child exits here */
#ifndef __VMS
	_exit(0);
#endif
	/*NOTREACHED*/
}


/*
 * di_devlock
 *	Create a lock to prevent another cooperating CD audio process
 *	from accessing the same CD-ROM device.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	path - The CD-ROM device node path name.
 *
 * Return:
 *	TRUE if the lock was successful.  If FALSE, then it indicates
 *	that another xmcd process currently has the lock.
 */
/*ARGSUSED*/
bool_t
di_devlock(curstat_t *s, char *path)
{
#ifndef __VMS
/* UNIX */
	int		fd;
	pid_t		pid,
			mypid;
	char		buf[32];
	struct stat	stbuf;

	if (di_isdemo())
		return TRUE;	/* No locking needed in demo mode */

	if (stat(path, &stbuf) < 0) {
		if (errno == ENOENT)
			/* If the device node is missing it is probably
			 * due to dynamic node creation/removal
			 * on some platforms based on presence of media.
			 * In this case, let the lock call succeed without
			 * actually setting a lock.  Otherwise the status
			 * would erroneously become "cd busy".
			 */
			return TRUE;
		else
			return FALSE;
	}

	(void) sprintf(lockfile, "%s/lock.%x", TEMP_DIR, (int) stbuf.st_rdev);

	DBGPRN(DBG_GEN)(errfp, "\nLock file: %s\n", lockfile);

	mypid = getpid();

	for (;;) {
		fd = open(lockfile, O_CREAT | O_EXCL | O_WRONLY);
		if (fd < 0) {
			if (errno == EEXIST) {
				fd = open(lockfile, O_RDONLY
#ifdef O_NOFOLLOW
						    | O_NOFOLLOW
#endif
				);
				if (fd < 0)
					return FALSE;

				if (read(fd, buf, 32) > 0) {
					if (strncmp(buf, "cdlk ", 5) != 0){
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
					(void) UNLINK(lockfile);
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
			(void) sprintf(buf, "cdlk %d\n", (int) mypid);
			(void) write(fd, buf, strlen(buf));

#ifdef NO_FCHMOD
			(void) close(fd);
			(void) chmod(lockfile, 0644);
#else
			(void) fchmod(fd, 0644);
			(void) close(fd);
#endif

			return TRUE;
		}
	}
#else
/* OpenVMS */
	extern int		SYS$GETDVIW();
	static char		ref_cnt;
	int			status;

	struct {
		short		buflen;
		short		code;
		char		*bufadr;
		int		*length;
		int		terminator;
	} itemlist = {
		4,
		DVI$_REFCNT,
		&ref_cnt,
		NULL,
		0
	};

        struct dsc$descriptor	dev_nam_desc;

	if (path == NULL)
		return TRUE;

	ref_cnt = 1;

	dev_nam_desc.dsc$b_class = DSC$K_CLASS_S;
	dev_nam_desc.dsc$b_dtype = DSC$K_DTYPE_T;
	dev_nam_desc.dsc$a_pointer = path;
	dev_nam_desc.dsc$w_length = strlen(path);

	status = SYS$GETDVIW(0, 0, &dev_nam_desc, &itemlist, 0, 0, 0, 0);

	if (status != 1) { 
		(void) fprintf(errfp,
		       "CD audio: Cannot get information on device: %s\n",
			    path);
		return FALSE;
	}
	
	return ((bool_t) (ref_cnt == 0));
#endif	/* __VMS */
}


/*
 * di_devunlock
 *	Unlock the lock that was created with di_devlock().
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
void
di_devunlock(curstat_t *s)
{
#ifndef __VMS
	if (lockfile[0] != '\0' && s->devlocked)
		(void) UNLINK(lockfile);
#endif
}


/*
 * di_devalloc
 *	Allocate a device descriptor structure.
 *
 * Args:
 *	path - The device path name (required)
 *
 * Return:
 *	Pointer to the allocated structure, or NULL on failure.
 */
di_dev_t *
di_devalloc(char *path)
{
	di_dev_t	*devp;

	if (path == NULL || *path == '\0')
		return FALSE;

	devp = (di_dev_t *)(void *) MEM_ALLOC(
		"di_dev_t",
		sizeof(di_dev_t)
	);
	if (devp == NULL)
		return NULL;

	(void) memset(devp, 0, sizeof(di_dev_t));

	devp->fd = -1;
	devp->path = NULL;
	if (!util_newstr(&devp->path, path))
		return NULL;

	return (devp);
}


/*
 * di_devfree
 *	De-allocate a device descriptor structure.
 *
 * Args:
 *	devp - Pointer to the device descriptor.
 *
 * Return:
 *	Nothing.
 */
void
di_devfree(di_dev_t *devp)
{
	MEM_FREE(devp->path);
	MEM_FREE(devp);
}


/*
 * di_methodstr
 *	Return a text string indicating the current operating mode.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Mode text string.
 */
char *
di_methodstr(void)
{
	char		*p;
	static char	str[STR_BUF_SZ * 6];

	if (ditbl[app_data.di_method].methodstr != NULL)
		p = (ditbl[app_data.di_method].methodstr());
	else
		p = "";

	(void) sprintf(str, "Device interface method: %s\n%s",
			p, cdda_info());
	return (str);
}


/*
 * di_isdemo
 *	Query if this is a demo-only version of the CD player.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	TRUE - demo-only version.
 *	FALSE - real version.
 */
bool_t
di_isdemo(void)
{
#ifdef DEMO_ONLY
	return TRUE;
#else
	return FALSE;
#endif
}


/*
 * di_curtrk_pos
 *	Return the trkinfo table offset location of the current playing
 *	CD track.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Integer offset into the trkinfo table, or -1 if not currently
 *	playing audio.
 */
int
di_curtrk_pos(curstat_t *s)
{
	int	i;

	if ((int) s->cur_trk <= 0)
		return -1;

	i = (int) s->cur_trk - 1;

	if (s->trkinfo[i].trkno == s->cur_trk)
		return (i);

	for (i = 0; i < MAXTRACK; i++) {
		if (s->trkinfo[i].trkno == s->cur_trk)
			return (i);
	}
	return -1;
}


/*
 * di_curprog_pos
 *	Return an integer representing the position of the current
 *	program or shuffle mode playing order (0 = first, 1 = second, ...).
 *	This routine should be used only when in program or shuffle play
 *	mode.
 *
 * Arg:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	An integer representing the position of the current program
 *	or shuffle mode playing order, or -1 if not in the appropriate mode.
 */
int
di_curprog_pos(curstat_t *s)
{
	return ((int) s->trkinfo[s->prog_cnt].playorder);
}


/*
 * di_clear_cdinfo
 *	Clear in-core CD information
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *	reload - Whether we're going to be reloading the CD information
 *
 * Return:
 *	Nothing.
 */
void
di_clear_cdinfo(curstat_t *s, bool_t reload)
{
	DBCLEAR(s, reload);
	di_cdinfo_loaded = FALSE;
}


/*
 * di_get_cdinfo
 *	Look up CD information
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	CD information lookup status (QMODE_xxx from appenv.h)
 */
byte_t
di_get_cdinfo(curstat_t *s)
{
	if (di_cdinfo_loaded)
		return TRUE;

	/* Call into the client to do the actual lookup */
	DBGET(s);
	di_cdinfo_loaded = TRUE;

	return (s->qmode);
}


/*
 * di_prepare_cdda
 *	Prepare for CDDA operation
 *
 * Args:
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
bool_t
di_prepare_cdda(curstat_t *s)
{
	if (PLAYMODE_IS_STD(app_data.play_mode))
		return TRUE;

	/* Expand CDDA output file pathnames if necessary */
	if ((app_data.play_mode & PLAYMODE_FILE) != 0 &&
	    di_clinfo->mkoutpath != NULL) {
		if (di_chktmpl(s, "%C") ||
		    di_chktmpl(s, "%A") || di_chktmpl(s, "%a") ||
		    di_chktmpl(s, "%D") || di_chktmpl(s, "%d") ||
		    di_chktmpl(s, "%R") || di_chktmpl(s, "%r") ||
		    di_chktmpl(s, "%T") || di_chktmpl(s, "%t") ||
		    di_chktmpl(s, "%B") || di_chktmpl(s, "%b"))
			(void) di_get_cdinfo(s);

		if (!di_clinfo->mkoutpath(s))
			return FALSE;
	}

	/* Check CDDA pipe program if necessary */
	if ((app_data.play_mode & PLAYMODE_PIPE) != 0 &&
	    di_clinfo->ckpipeprog != NULL && !di_clinfo->ckpipeprog(s))
		return FALSE;

	return TRUE;
}


/*
 * di_reset_curstat
 *	Reset the curstat_t structure to initial defaults.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *	clear_toc - Whether the trkinfo CD table-of-contents 
 *		should be cleared.
 *	eject - Whether the medium is being ejected
 *
 * Return:
 *	Nothing.
 */
void
di_reset_curstat(curstat_t *s, bool_t clear_toc, bool_t eject)
{
	sword32_t	i;
	static bool_t	first_time = TRUE;

	s->cur_trk = s->cur_idx = -1;
	s->curpos_tot.min = s->curpos_tot.sec = s->curpos_tot.frame = 0;
	s->curpos_trk.min = s->curpos_trk.sec = s->curpos_trk.frame = 0;
	s->curpos_tot.addr = s->curpos_trk.addr = 0;
	s->sav_iaddr = 0;
	s->prog_cnt = 0;
	s->frm_played = s->tot_frm = 0;
	s->frm_per_sec = 0;

	if (s->onetrk_prog) {
		s->onetrk_prog = FALSE;
		PROGCLEAR(s);
	}

	if (clear_toc) {
		s->flags = 0;
		s->segplay = SEGP_NONE;
		s->first_trk = s->last_trk = -1;
		s->discpos_tot.min = s->discpos_tot.sec = 0;
		s->tot_trks = 0;
		s->discpos_tot.addr = 0;
		s->prog_tot = 0;
		s->program = FALSE;

		(void) memset(s->mcn, 0, sizeof(s->mcn));

		for (i = 0; i < MAXTRACK; i++) {
			s->trkinfo[i].trkno = -1;
			s->trkinfo[i].min = 0;
			s->trkinfo[i].sec = 0;
			s->trkinfo[i].frame = 0;
			s->trkinfo[i].addr = 0;
			s->trkinfo[i].type = TYP_AUDIO;
			s->trkinfo[i].playorder = -1;
			if (s->trkinfo[i].outfile != NULL) {
				MEM_FREE(s->trkinfo[i].outfile);
				s->trkinfo[i].outfile = NULL;
			}

			(void) memset(s->trkinfo[i].isrc, 0,
				      sizeof(s->trkinfo[i].isrc));
		}
	}

	if (eject) {
		s->mode = MOD_NODISC;
		s->rptcnt = 0;
	}

	if (first_time) {
		/* These are to be initialized only once */
		first_time = FALSE;

		s->first_disc = 1;
		s->last_disc = app_data.numdiscs;
		s->cur_disc = app_data.reverse ? s->last_disc : s->first_disc;
		s->prev_disc = -1;
		s->time_dpy = T_ELAPSED_TRACK;
		s->repeat = s->shuffle = FALSE;
		s->rptcnt = 0;
		s->level = 0;
		s->level_left = s->level_right = 100;
		s->cdda_att = 100;
		s->caddy_lock = FALSE;
		s->vendor[0] = '\0';
		s->prod[0] = '\0';
		s->revnum[0] = '\0';
	}
}


/*
 * di_reset_shuffle
 *	Recompute a new shuffle play sequence.  Updates the playorder
 *	table in the curstat_t structure.
 *
 * Args:
 *	s - Pointer to the curstat_t structure.
 *
 * Return:
 *	Nothing.
 */
void
di_reset_shuffle(curstat_t *s)
{
	sword32_t	i,
			j,
			n,
			x;

	srand((unsigned) time(NULL));
	s->prog_cnt = 0;

	/* Initialize the play-list with only audio tracks */
	n = 0;
	for (i = 0; i < (int) s->tot_trks; i++) {
		if (s->trkinfo[i].type == TYP_AUDIO)
			s->trkinfo[n++].playorder = i;
	}

	s->prog_tot = (byte_t) n;

	/* Initialize rest of list */
	for (; n < MAXTRACK; n++)
		s->trkinfo[n].playorder = -1;

	/*
	 * Now shuffle the playlist by swapping random pairs:
	 * this is the most efficient alogorithm for a linear
	 * distribution.
	 */
	for (i = (int) s->prog_tot - 1; i > 0; i--) {
		j = rand() % (i + 1);
		if (j != i) {
			x = s->trkinfo[i].playorder;
			s->trkinfo[i].playorder = s->trkinfo[j].playorder;
			s->trkinfo[j].playorder = x;
		}
	}

	if (app_data.debug & DBG_GEN) {
		(void) fprintf(errfp, "\nShuffle tracks: ");

		for (i = 0; i < (int) s->prog_tot; i++)
			(void) fprintf(errfp, "%d ",
				   s->trkinfo[s->trkinfo[i].playorder].trkno);

		(void) fprintf(errfp, "\n");
	}
}


