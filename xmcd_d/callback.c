/*
 *   xmcd - Motif(R) CD Audio Player/Ripper
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
static char *_callback_c_ident_ = "@(#)callback.c	7.83 04/02/02";
#endif

#include "common_d/appenv.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/callback.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"
#include "xmcd_d/dbprog.h"
#include "xmcd_d/wwwwarp.h"
#include "xmcd_d/userreg.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/help.h"
#include "xmcd_d/hotkey.h"


extern appdata_t	app_data;
extern widgets_t	widgets;

STATIC Atom		delw;


/***********************
 *  internal routines  *
 ***********************/


/*
 * register_main_callbacks
 *	Register all callback routines for widgets in the main window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_main_callbacks(widgets_t *m, curstat_t *s)
{
	/* Main window callbacks */
	register_focus_cb(m->main.form, cd_shell_focus_chg, m->main.form);

	register_arm_cb(m->main.mode_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.mode_btn, cd_mode, s);
	register_focuschg_ev(m->main.mode_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.mode_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.lock_btn, cd_tooltip_cancel, s);
	register_xingchg_ev(m->main.lock_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.repeat_btn, cd_tooltip_cancel, s);
	register_xingchg_ev(m->main.repeat_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.shuffle_btn, cd_tooltip_cancel, s);
	register_xingchg_ev(m->main.shuffle_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.eject_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.eject_btn, cd_load_eject, s);
	register_focuschg_ev(m->main.eject_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.eject_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.quit_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.quit_btn, cd_quit_btn, s);
	register_focuschg_ev(m->main.quit_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.quit_btn, cd_xing_chg, NULL);

	register_xingchg_ev(m->main.disc_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.track_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.index_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.time_ind, cd_xing_chg, NULL);

	register_xingchg_ev(m->main.rptcnt_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.dbmode_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.progmode_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.timemode_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.playmode_ind, cd_xing_chg, NULL);

	register_xingchg_ev(m->main.dtitle_ind, cd_xing_chg, NULL);
	register_xingchg_ev(m->main.ttitle_ind, cd_xing_chg, NULL);

	register_arm_cb(m->main.dbprog_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.dbprog_btn, dbprog_popup, s);
	register_focuschg_ev(m->main.dbprog_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.dbprog_btn, cd_xing_chg, NULL);

	register_cascade_cb(m->main.wwwwarp_btn, cd_tooltip_cancel, s);

	register_arm_cb(m->main.options_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.options_btn, cd_options_popup, s);
	register_focuschg_ev(m->main.options_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.options_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.ab_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.ab_btn, cd_ab, s);
	register_focuschg_ev(m->main.ab_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.ab_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.sample_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.sample_btn, cd_sample, s);
	register_focuschg_ev(m->main.sample_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.sample_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.time_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.time_btn, cd_time, s);
	register_focuschg_ev(m->main.time_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.time_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.keypad_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.keypad_btn, cd_keypad_popup, s);
	register_focuschg_ev(m->main.keypad_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.keypad_btn, cd_xing_chg, NULL);

	register_focuschg_ev(m->main.wwwwarp_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.wwwwarp_btn, cd_xing_chg, NULL);

	register_valchg_cb(m->main.level_scale, cd_tooltip_cancel, s);
	register_valchg_cb(m->main.level_scale, cd_level, s);
	register_drag_cb(m->main.level_scale, cd_tooltip_cancel, s);
	register_drag_cb(m->main.level_scale, cd_level, s);
	register_xingchg_ev(m->main.level_scale, cd_xing_chg, NULL);

	register_arm_cb(m->main.playpause_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.playpause_btn, cd_play_pause, s);
	register_focuschg_ev(m->main.playpause_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.playpause_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.stop_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.stop_btn, cd_stop, s);
	register_focuschg_ev(m->main.stop_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.stop_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.prevdisc_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.prevdisc_btn, cd_chgdisc, s);
	register_focuschg_ev(m->main.prevdisc_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.prevdisc_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.nextdisc_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.nextdisc_btn, cd_chgdisc, s);
	register_focuschg_ev(m->main.nextdisc_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.nextdisc_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.prevtrk_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.prevtrk_btn, cd_prevtrk, s);
	register_focuschg_ev(m->main.prevtrk_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.prevtrk_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.nexttrk_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.nexttrk_btn, cd_nexttrk, s);
	register_focuschg_ev(m->main.nexttrk_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.nexttrk_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.previdx_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.previdx_btn, cd_previdx, s);
	register_focuschg_ev(m->main.previdx_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.previdx_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.nextidx_btn, cd_tooltip_cancel, s);
	register_activate_cb(m->main.nextidx_btn, cd_nextidx, s);
	register_focuschg_ev(m->main.nextidx_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.nextidx_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.rew_btn, cd_tooltip_cancel, s);
	register_arm_cb(m->main.rew_btn, cd_rew, s);
	register_disarm_cb(m->main.rew_btn, cd_rew, s);
	register_activate_cb(m->main.rew_btn, cd_rew, s);
	register_focuschg_ev(m->main.rew_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.rew_btn, cd_xing_chg, NULL);

	register_arm_cb(m->main.ff_btn, cd_tooltip_cancel, s);
	register_arm_cb(m->main.ff_btn, cd_ff, s);
	register_disarm_cb(m->main.ff_btn, cd_ff, s);
	register_activate_cb(m->main.ff_btn, cd_ff, s);
	register_focuschg_ev(m->main.ff_btn, cd_focus_chg, NULL);
	register_xingchg_ev(m->main.ff_btn, cd_xing_chg, NULL);

	register_btnrel_ev(m->main.dbmode_ind, cd_dbmode_ind, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		m->toplevel,
		(XtCallbackProc) cd_exit,
		(XtPointer) s
	);
}


/*
 * register_keypad_callbacks
 *	Register all callback routines for widgets in the keypad window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_keypad_callbacks(widgets_t *m, curstat_t *s)
{
	int	i;

	register_focus_cb(m->keypad.form, cd_shell_focus_chg, m->keypad.form);

	/* Keypad popup callbacks */
	for (i = 0; i < 10; i++)
		register_activate_cb(m->keypad.num_btn[i], cd_keypad_num,
				     (long) i);

	register_activate_cb(m->keypad.clear_btn, cd_keypad_clear, s);
	register_activate_cb(m->keypad.enter_btn, cd_keypad_enter, s);

	register_valchg_cb(m->keypad.warp_scale, cd_warp, s);
	register_drag_cb(m->keypad.warp_scale, cd_warp, s);

	register_activate_cb(m->keypad.cancel_btn, cd_keypad_popdown, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->keypad.form),
		(XtCallbackProc) cd_keypad_popdown,
		(XtPointer) s
	);
}


/*
 * register_options_callbacks
 *	Register all callback routines for widgets in the options window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_options_callbacks(widgets_t *m, curstat_t *s)
{
	register_focus_cb(m->options.form, cd_shell_focus_chg,
			  m->options.form);

	register_browsel_cb(m->options.categ_list, cd_options_categsel, s);
	register_defaction_cb(m->options.categ_list, cd_options_categsel, s);

	register_valchg_cb(m->options.mode_jitter_btn, cd_jitter_corr, s);
	register_valchg_cb(m->options.mode_trkfile_btn, cd_file_per_trk, s);
	register_valchg_cb(m->options.mode_subst_btn, cd_subst, s);

	register_activate_cb(m->options.mode_fmt_raw_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_au_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_wav_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_aiff_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_aifc_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_mp3_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_ogg_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_flac_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_aac_btn, cd_filefmt_mode, s);
	register_activate_cb(m->options.mode_fmt_mp4_btn, cd_filefmt_mode, s);

	register_activate_cb(m->options.mode_perfmon_btn, cd_perfmon, s);

	register_valchg_cb(m->options.mode_path_txt, cd_filepath_new, s);
	register_activate_cb(m->options.mode_path_txt, cd_filepath_new, s);
	register_losefocus_cb(m->options.mode_path_txt, cd_filepath_new, s);

	register_activate_cb(m->options.mode_pathexp_btn, cd_filepath_exp, s);

	register_valchg_cb(m->options.mode_prog_txt, cd_pipeprog_new, s);
	register_activate_cb(m->options.mode_prog_txt, cd_pipeprog_new, s);
	register_losefocus_cb(m->options.mode_prog_txt, cd_pipeprog_new, s);

	register_activate_cb(m->options.mode0_btn, cd_comp_mode, s);
	register_activate_cb(m->options.mode1_btn, cd_comp_mode, s);
	register_activate_cb(m->options.mode2_btn, cd_comp_mode, s);
	register_activate_cb(m->options.mode3_btn, cd_comp_mode, s);

	register_activate_cb(m->options.bitrate_def_btn, cd_bitrate, s);
	register_activate_cb(m->options.minbrate_def_btn, cd_min_bitrate, s);
	register_activate_cb(m->options.maxbrate_def_btn, cd_max_bitrate, s);

	register_valchg_cb(m->options.qualval_scl, cd_qualfactor, s);
	register_drag_cb(m->options.qualval_scl, cd_qualfactor, s);

	register_activate_cb(m->options.chmode_stereo_btn, cd_chanmode, s);
	register_activate_cb(m->options.chmode_jstereo_btn, cd_chanmode, s);
	register_activate_cb(m->options.chmode_forcems_btn, cd_chanmode, s);
	register_activate_cb(m->options.chmode_mono_btn, cd_chanmode, s);

	register_valchg_cb(m->options.compalgo_scl, cd_compalgo, s);
	register_drag_cb(m->options.compalgo_scl, cd_compalgo, s);

	register_activate_cb(m->options.lp_off_btn, cd_lowpass_mode, s);
	register_activate_cb(m->options.lp_auto_btn, cd_lowpass_mode, s);
	register_activate_cb(m->options.lp_manual_btn, cd_lowpass_mode, s);
	register_activate_cb(m->options.hp_off_btn, cd_highpass_mode, s);
	register_activate_cb(m->options.hp_auto_btn, cd_highpass_mode, s);
	register_activate_cb(m->options.hp_manual_btn, cd_highpass_mode, s);

	register_modvfy_cb(m->options.lp_freq_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.lp_freq_txt, cd_filter_freq, s);
	register_activate_cb(m->options.lp_freq_txt, cd_filter_freq, s);
	register_losefocus_cb(m->options.lp_freq_txt, cd_filter_freq, s);
	register_modvfy_cb(m->options.hp_freq_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.hp_freq_txt, cd_filter_freq, s);
	register_activate_cb(m->options.hp_freq_txt, cd_filter_freq, s);
	register_losefocus_cb(m->options.hp_freq_txt, cd_filter_freq, s);
	register_modvfy_cb(m->options.lp_width_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.lp_width_txt, cd_filter_width, s);
	register_activate_cb(m->options.lp_width_txt, cd_filter_width, s);
	register_losefocus_cb(m->options.lp_width_txt, cd_filter_width, s);
	register_modvfy_cb(m->options.hp_width_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.hp_width_txt, cd_filter_width, s);
	register_activate_cb(m->options.hp_width_txt, cd_filter_width, s);
	register_losefocus_cb(m->options.hp_width_txt, cd_filter_width, s);

	register_valchg_cb(m->options.copyrt_btn, cd_addflag, s);
	register_valchg_cb(m->options.orig_btn, cd_addflag, s);
	register_valchg_cb(m->options.nores_btn, cd_addflag, s);
	register_valchg_cb(m->options.chksum_btn, cd_addflag, s);
	register_valchg_cb(m->options.iso_btn, cd_addflag, s);

	register_valchg_cb(m->options.addtag_btn, cd_addtag, s);
	register_activate_cb(m->options.id3tag_v1_btn, cd_id3tag_mode, s);
	register_activate_cb(m->options.id3tag_v2_btn, cd_id3tag_mode, s);
	register_activate_cb(m->options.id3tag_both_btn, cd_id3tag_mode, s);

	register_modvfy_cb(m->options.lame_opts_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->options.lame_opts_txt, cd_set_lameopts, s);
	register_activate_cb(m->options.lame_opts_txt, cd_set_lameopts, s);
	register_losefocus_cb(m->options.lame_opts_txt, cd_set_lameopts, s);

	register_valchg_cb(m->options.bal_scale, cd_balance, s);
	register_drag_cb(m->options.bal_scale, cd_balance, s);

	register_activate_cb(m->options.balctr_btn, cd_balance_center, s);

	register_valchg_cb(m->options.cdda_att_scale, cd_cdda_att, s);
	register_drag_cb(m->options.cdda_att_scale, cd_cdda_att, s);

	register_activate_cb(m->options.cdda_fadeout_btn, cd_cdda_fade, s);
	register_activate_cb(m->options.cdda_fadein_btn, cd_cdda_fade, s);

	register_modvfy_cb(m->options.hb_tout_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.hb_tout_txt, cd_set_timeouts, s);
	register_activate_cb(m->options.hb_tout_txt, cd_set_timeouts, s);
	register_losefocus_cb(m->options.hb_tout_txt, cd_set_timeouts, s);

	register_modvfy_cb(m->options.serv_tout_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.serv_tout_txt, cd_set_timeouts, s);
	register_activate_cb(m->options.serv_tout_txt, cd_set_timeouts, s);
	register_losefocus_cb(m->options.serv_tout_txt, cd_set_timeouts, s);

	register_modvfy_cb(m->options.cache_tout_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.cache_tout_txt, cd_set_timeouts, s);
	register_activate_cb(m->options.cache_tout_txt, cd_set_timeouts, s);
	register_losefocus_cb(m->options.cache_tout_txt, cd_set_timeouts, s);

	register_modvfy_cb(m->options.cache_tout_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->options.cache_tout_txt, cd_set_servers, s);
	register_activate_cb(m->options.proxy_srvr_txt, cd_set_servers, s);
	register_losefocus_cb(m->options.proxy_srvr_txt, cd_set_servers, s);

	register_modvfy_cb(m->options.proxy_port_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->options.proxy_port_txt, cd_set_servers, s);
	register_activate_cb(m->options.proxy_port_txt, cd_set_servers, s);
	register_losefocus_cb(m->options.proxy_port_txt, cd_set_servers, s);

	register_valchg_cb(m->options.use_proxy_btn, cd_set_proxy, s);
	register_valchg_cb(m->options.proxy_auth_btn, cd_set_proxy, s);

	register_activate_cb(m->options.reset_btn, cd_options_reset, s);
	register_activate_cb(m->options.save_btn, cd_options_save, s);
	register_activate_cb(m->options.ok_btn, cd_options_popdown, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->options.form),
		(XtCallbackProc) cd_options_popdown,
		(XtPointer) s
	);
}


/*
 * register_perfmon_callbacks
 *	Register all callback routines for widgets in the
 *	CDDA performance monitor window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_perfmon_callbacks(widgets_t *m, curstat_t *s)
{
	/* CDDA perf monitor popup callbacks */
	register_focus_cb(m->perfmon.form, cd_shell_focus_chg,
			  m->perfmon.form);

	register_activate_cb(m->perfmon.cancel_btn, cd_perfmon_popdown, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->perfmon.form),
		(XtCallbackProc) cd_perfmon_popdown,
		(XtPointer) s
	);
}


/*
 * register_dbprog_callbacks
 *	Register all callback routines for widgets in the
 *	CD Information/program window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_dbprog_callbacks(widgets_t *m, curstat_t *s)
{
	/* CD info/program popup callbacks */
	register_focus_cb(m->dbprog.form, cd_shell_focus_chg, m->dbprog.form);

	register_valchg_cb(m->dbprog.inetoffln_btn, dbprog_inetoffln, s);

	register_activate_cb(m->dbprog.dlist_btn, dbprog_dlist, s);

	register_modvfy_cb(m->dbprog.artist_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbprog.artist_txt, dbprog_text_new, s);

	register_modvfy_cb(m->dbprog.title_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbprog.title_txt, dbprog_text_new, s);
	register_activate_cb(m->dbprog.title_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbprog.title_txt, dbprog_text_new, s);

	register_activate_cb(m->dbprog.fullname_btn, dbprog_fullname,
			     (XtPointer) TRUE);

	register_activate_cb(m->dbprog.extd_btn, dbprog_extd, s);
	register_activate_cb(m->dbprog.dcredits_btn, dbprog_credits_popup, s);
	register_activate_cb(m->dbprog.segments_btn, dbprog_segments_popup, s);

	register_defaction_cb(m->dbprog.trk_list, dbprog_trklist_play, s);
	register_browsel_cb(m->dbprog.trk_list, dbprog_trklist_select, s);

	register_modvfy_cb(m->dbprog.ttitle_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbprog.ttitle_txt, dbprog_ttitle_new, s);
	register_activate_cb(m->dbprog.ttitle_txt, dbprog_ttitle_new, s);
	register_focus_cb(m->dbprog.ttitle_txt, dbprog_ttitle_focuschg, s);
	register_losefocus_cb(m->dbprog.ttitle_txt, dbprog_ttitle_focuschg, s);

	register_activate_cb(m->dbprog.apply_btn, dbprog_ttitle_new, s);

	register_activate_cb(m->dbprog.extt_btn, dbprog_extt, TRUE);

	register_activate_cb(m->dbprog.tcredits_btn, dbprog_credits_popup, s);

	register_modvfy_cb(m->dbprog.pgmseq_txt, dbprog_pgmseq_verify, s);
	register_valchg_cb(m->dbprog.pgmseq_txt, dbprog_pgmseq_txtchg, s);

	register_activate_cb(m->dbprog.addpgm_btn, dbprog_addpgm, s);
	register_activate_cb(m->dbprog.clrpgm_btn, dbprog_clrpgm, s);
	register_activate_cb(m->dbprog.savepgm_btn, dbprog_savepgm, s);

	register_activate_cb(m->dbprog.userreg_btn, userreg_popup, s);

	register_activate_cb(m->dbprog.submit_btn, dbprog_submit, s);
	register_activate_cb(m->dbprog.flush_btn, dbprog_flush, s);
	register_activate_cb(m->dbprog.reload_btn, dbprog_load, s);
	register_activate_cb(m->dbprog.ok_btn, dbprog_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->dbprog.form),
		(XtCallbackProc) dbprog_ok,
		(XtPointer) s
	);
}


/*
 * register_dlist_callbacks
 *	Register all callback routines for widgets in the disc list window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_dlist_callbacks(widgets_t *m, curstat_t *s)
{
	/* Disc list window callbacks */
	register_focus_cb(m->dlist.form, cd_shell_focus_chg, m->dlist.form);

	register_activate_cb(m->dlist.hist_btn, dbprog_dlist_mode, s);
	register_activate_cb(m->dlist.chgr_btn, dbprog_dlist_mode, s);

	register_defaction_cb(m->dlist.disc_list, dbprog_dlist_show, s);
	register_browsel_cb(m->dlist.disc_list, dbprog_dlist_select, s);

	register_activate_cb(m->dlist.show_btn, dbprog_dlist_show, s);
	register_activate_cb(m->dlist.goto_btn, dbprog_dlist_goto, s);
	register_activate_cb(m->dlist.del_btn, dbprog_dlist_delete, s);
	register_activate_cb(m->dlist.delall_btn, dbprog_dlist_delall, s);
	register_activate_cb(m->dlist.rescan_btn, dbprog_dlist_rescan, s);

	register_activate_cb(m->dlist.cancel_btn, dbprog_dlist_cancel, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->dlist.form),
		(XtCallbackProc) dbprog_dlist_cancel,
		(XtPointer) s
	);
}


/*
 * register_fullname_callbacks
 *	Register all callback routines for widgets in the full name window.
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_fullname_callbacks(widgets_t *m, curstat_t *s)
{
	/* fullname window callbacks */
	register_focus_cb(m->fullname.form, cd_shell_focus_chg,
			  m->fullname.form);

	register_modvfy_cb(m->fullname.dispname_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->fullname.dispname_txt, dbprog_text_new, s);
	register_activate_cb(m->fullname.dispname_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->fullname.dispname_txt, dbprog_text_new, s);

	register_valchg_cb(m->fullname.autogen_btn,
			   dbprog_fullname_autogen, s);

	register_modvfy_cb(m->fullname.lastname_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->fullname.lastname_txt, dbprog_text_new, s);
	register_activate_cb(m->fullname.lastname_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->fullname.lastname_txt, dbprog_text_new, s);

	register_modvfy_cb(m->fullname.firstname_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->fullname.firstname_txt, dbprog_text_new, s);
	register_activate_cb(m->fullname.firstname_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->fullname.firstname_txt, dbprog_text_new, s);

	register_valchg_cb(m->fullname.the_btn, dbprog_the, s);
	register_modvfy_cb(m->fullname.the_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->fullname.the_txt, dbprog_text_new, s);
	register_activate_cb(m->fullname.the_txt, dbprog_focus_next, s);

	register_activate_cb(m->fullname.ok_btn, dbprog_fullname_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->fullname.form),
		(XtCallbackProc) dbprog_fullname_ok,
		(XtPointer) s
	);
}


/*
 * register_extd_callbacks
 *	Register all callback routines for widgets in the disc details window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_extd_callbacks(widgets_t *m, curstat_t *s)
{
	/* Disc details window callbacks */
	register_focus_cb(m->dbextd.form, cd_shell_focus_chg, m->dbextd.form);

	register_modvfy_cb(m->dbextd.sorttitle_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbextd.sorttitle_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextd.sorttitle_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextd.sorttitle_txt, dbprog_text_new, s);

	register_valchg_cb(m->dbextd.the_btn, dbprog_the, s);
	register_modvfy_cb(m->dbextd.the_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbextd.the_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextd.the_txt, dbprog_focus_next, s);

	register_modvfy_cb(m->dbextd.year_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->dbextd.year_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextd.year_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextd.year_txt, dbprog_text_new, s);

	register_modvfy_cb(m->dbextd.label_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbextd.label_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextd.label_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextd.label_txt, dbprog_text_new, s);

	register_valchg_cb(m->dbextd.comp_btn, dbprog_extd_compilation, s);

	register_modvfy_cb(m->dbextd.dnum_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->dbextd.dnum_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextd.dnum_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextd.dnum_txt, dbprog_text_new, s);

	register_modvfy_cb(m->dbextd.tnum_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->dbextd.tnum_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextd.tnum_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextd.tnum_txt, dbprog_text_new, s);

	register_activate_cb(m->dbextd.genre_none_btn[0],
			     dbprog_genre_sel, NULL);
	register_activate_cb(m->dbextd.subgenre_none_btn[0],
			     dbprog_subgenre_sel, NULL);
	register_activate_cb(m->dbextd.genre_none_btn[1],
			     dbprog_genre_sel, NULL);
	register_activate_cb(m->dbextd.subgenre_none_btn[1],
			     dbprog_subgenre_sel, NULL);

	register_activate_cb(m->dbextd.region_chg_btn,
			     dbprog_regionsel_popup, s);

	register_activate_cb(m->dbextd.lang_chg_btn,
			     dbprog_langsel_popup, s);

	register_valchg_cb(m->dbextd.notes_txt, dbprog_text_new, s);

	register_activate_cb(m->dbextd.ok_btn, dbprog_extd_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->dbextd.form),
		(XtCallbackProc) dbprog_extd_ok,
		(XtPointer) s
	);
}


/*
 * register_extt_callbacks
 *	Register all callback routines for widgets in the Track details
 *	window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_extt_callbacks(widgets_t *m, curstat_t *s)
{
	/* Track details window callbacks */
	register_focus_cb(m->dbextt.form, cd_shell_focus_chg, m->dbextt.form);

	register_activate_cb(m->dbextt.prev_btn, dbprog_extt_prev, s);
	register_activate_cb(m->dbextt.next_btn, dbprog_extt_next, s);
	register_valchg_cb(m->dbextt.autotrk_btn, dbprog_extt_autotrk, s);

	register_modvfy_cb(m->dbextt.sorttitle_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbextt.sorttitle_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextt.sorttitle_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextt.sorttitle_txt, dbprog_text_new, s);

	register_valchg_cb(m->dbextt.the_btn, dbprog_the, s);
	register_modvfy_cb(m->dbextt.the_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbextt.the_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextt.the_txt, dbprog_focus_next, s);

	register_modvfy_cb(m->dbextt.artist_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbextt.artist_txt, dbprog_text_new, s);
	register_losefocus_cb(m->dbextt.artist_txt, dbprog_text_new, s);

	register_activate_cb(m->dbextt.fullname_btn, dbprog_fullname,
			     (XtPointer) TRUE);

	register_modvfy_cb(m->dbextt.year_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->dbextt.year_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextt.year_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextt.year_txt, dbprog_text_new, s);

	register_modvfy_cb(m->dbextt.label_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->dbextt.label_txt, dbprog_text_new, s);
	register_activate_cb(m->dbextt.label_txt, dbprog_focus_next, s);
	register_losefocus_cb(m->dbextt.label_txt, dbprog_text_new, s);

	register_modvfy_cb(m->dbextt.bpm_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->dbextt.bpm_txt, dbprog_text_new, s);
	register_losefocus_cb(m->dbextt.bpm_txt, dbprog_text_new, s);

	register_activate_cb(m->dbextt.genre_none_btn[0],
			     dbprog_genre_sel, NULL);
	register_activate_cb(m->dbextt.subgenre_none_btn[0],
			     dbprog_subgenre_sel, NULL);
	register_activate_cb(m->dbextt.genre_none_btn[1],
			     dbprog_genre_sel, NULL);
	register_activate_cb(m->dbextt.subgenre_none_btn[1],
			     dbprog_subgenre_sel, NULL);

	register_valchg_cb(m->dbextt.notes_txt, dbprog_text_new, s);

	register_activate_cb(m->dbextt.ok_btn, dbprog_extt_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->dbextt.form),
		(XtCallbackProc) dbprog_extt_ok,
		(XtPointer) s
	);
}


/*
 * register_credits_callbacks
 *	Register all callback routines for widgets in the credits window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_credits_callbacks(widgets_t *m, curstat_t *s)
{
	/* Credits window callbacks */
	register_focus_cb(m->credits.form, cd_shell_focus_chg,
			  m->credits.form);

	register_activate_cb(m->credits.prev_btn, dbprog_extt_prev, s);
	register_activate_cb(m->credits.next_btn, dbprog_extt_next, s);
	register_valchg_cb(m->credits.autotrk_btn, dbprog_extt_autotrk, s);

	register_browsel_cb(m->credits.cred_list, dbprog_credits_select, s);

	register_activate_cb(m->credits.prirole_none_btn,
			     dbprog_role_sel, NULL);
	register_activate_cb(m->credits.subrole_none_btn,
			     dbprog_subrole_sel, NULL);

	register_modvfy_cb(m->credits.name_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->credits.name_txt, dbprog_text_new, s);

	register_activate_cb(m->credits.fullname_btn, dbprog_fullname,
			     (XtPointer) TRUE);

	register_valchg_cb(m->credits.notes_txt, dbprog_text_new, s);

	register_activate_cb(m->credits.add_btn, dbprog_credits_add, s);
	register_activate_cb(m->credits.mod_btn, dbprog_credits_mod, s);
	register_activate_cb(m->credits.del_btn, dbprog_credits_del, s);
	register_activate_cb(m->credits.ok_btn, dbprog_credits_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->credits.form),
		(XtCallbackProc) dbprog_credits_ok,
		(XtPointer) s
	);
}


/*
 * register_segments_callbacks
 *	Register all callback routines for widgets in the segments window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_segments_callbacks(widgets_t *m, curstat_t *s)
{
	/* Credits window callbacks */
	register_focus_cb(m->segments.form, cd_shell_focus_chg,
			  m->segments.form);

	register_browsel_cb(m->segments.seg_list, dbprog_segments_select, s);
	register_defaction_cb(m->segments.seg_list, cd_play_pause, s);

	register_modvfy_cb(m->segments.name_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->segments.name_txt, dbprog_text_new, s);
	register_activate_cb(m->segments.name_txt, dbprog_focus_next, s);

	register_modvfy_cb(m->segments.starttrk_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->segments.starttrk_txt, dbprog_text_new, s);
	register_activate_cb(m->segments.starttrk_txt, dbprog_focus_next, s);

	register_modvfy_cb(m->segments.startfrm_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->segments.startfrm_txt, dbprog_text_new, s);
	register_activate_cb(m->segments.startfrm_txt, dbprog_focus_next, s);

	register_modvfy_cb(m->segments.endtrk_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->segments.endtrk_txt, dbprog_text_new, s);
	register_activate_cb(m->segments.endtrk_txt, dbprog_focus_next, s);

	register_modvfy_cb(m->segments.endfrm_txt, cd_txtnline_vfy, s);
	register_valchg_cb(m->segments.endfrm_txt, dbprog_text_new, s);
	register_activate_cb(m->segments.endfrm_txt, dbprog_focus_next, s);

	register_activate_cb(m->segments.set_btn, cd_ab, s);

	register_valchg_cb(m->segments.notes_txt, dbprog_text_new, s);

	register_activate_cb(m->segments.credits_btn, dbprog_credits_popup, s);

	register_activate_cb(m->segments.playpaus_btn, cd_play_pause, s);
	register_activate_cb(m->segments.stop_btn, cd_stop, s);

	register_activate_cb(m->segments.add_btn, dbprog_segments_add, s);
	register_activate_cb(m->segments.mod_btn, dbprog_segments_mod, s);
	register_activate_cb(m->segments.del_btn, dbprog_segments_del, s);
	register_activate_cb(m->segments.ok_btn, dbprog_segments_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->segments.form),
		(XtCallbackProc) dbprog_segments_ok,
		(XtPointer) s
	);
}


/*
 * register_submiturl_callbacks
 *	Register all callback routines for widgets in the submit URL window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_submiturl_callbacks(widgets_t *m, curstat_t *s)
{
	/* SubmitURL window callbacks */
	register_focus_cb(m->submiturl.form, cd_shell_focus_chg,
			  m->submiturl.form);

	register_modvfy_cb(m->submiturl.categ_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->submiturl.categ_txt, wwwwarp_submit_url_chg, s);
	register_activate_cb(m->submiturl.categ_txt, wwwwarp_focus_next, s);

	register_modvfy_cb(m->submiturl.name_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->submiturl.name_txt, wwwwarp_submit_url_chg, s);
	register_activate_cb(m->submiturl.name_txt, wwwwarp_focus_next, s);

	register_modvfy_cb(m->submiturl.url_txt, cd_txtline_vfy, s);
	register_valchg_cb(m->submiturl.url_txt, wwwwarp_submit_url_chg, s);
	register_activate_cb(m->submiturl.url_txt, wwwwarp_focus_next, s);

	register_valchg_cb(m->submiturl.desc_txt, wwwwarp_submit_url_chg, s);

	register_activate_cb(m->submiturl.submit_btn,
			     wwwwarp_submit_url_submit, s);
	register_activate_cb(m->submiturl.ok_btn, wwwwarp_submit_url_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->submiturl.form),
		(XtCallbackProc) wwwwarp_submit_url_ok,
		(XtPointer) s
	);
}


/*
 * register_regionsel_callbacks
 *	Register all callback routines for widgets in the regionsel window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_regionsel_callbacks(widgets_t *m, curstat_t *s)
{
	/* Region selector popup callbacks */
	register_focus_cb(m->regionsel.form, cd_shell_focus_chg,
			  m->regionsel.form);

	register_defaction_cb(m->regionsel.region_list,
			      dbprog_regionsel_ok, s);
	register_browsel_cb(m->regionsel.region_list,
			    dbprog_regionsel_select, s);

	register_activate_cb(m->regionsel.ok_btn, dbprog_regionsel_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->regionsel.form),
		(XtCallbackProc) dbprog_regionsel_ok,
		(XtPointer) s
	);
}


/*
 * register_langsel_callbacks
 *	Register all callback routines for widgets in the langsel window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_langsel_callbacks(widgets_t *m, curstat_t *s)
{
	/* Language selector popup callbacks */
	register_focus_cb(m->langsel.form, cd_shell_focus_chg,
			  m->langsel.form);

	register_defaction_cb(m->langsel.lang_list, dbprog_langsel_ok, s);
	register_browsel_cb(m->langsel.lang_list, dbprog_langsel_select, s);

	register_activate_cb(m->langsel.ok_btn, dbprog_langsel_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->langsel.form),
		(XtCallbackProc) dbprog_langsel_ok,
		(XtPointer) s
	);
}


/*
 * register_matchsel_callbacks
 *	Register all callback routines for widgets in the matchsel window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_matchsel_callbacks(widgets_t *m, curstat_t *s)
{
	/* Match selector popup callbacks */
	register_focus_cb(m->matchsel.form, cd_shell_focus_chg,
			  m->matchsel.form);

	register_defaction_cb(m->matchsel.matchsel_list,
			      dbprog_matchsel_ok, s);
	register_browsel_cb(m->matchsel.matchsel_list,
			    dbprog_matchsel_select, s);

	register_activate_cb(m->matchsel.ok_btn, dbprog_matchsel_ok, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->matchsel.form),
		(XtCallbackProc) dbprog_matchsel_ok,
		(XtPointer) s
	);
}


/*
 * register_userreg_callbacks
 *	Register all callback routines for widgets in the userreg window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_userreg_callbacks(widgets_t *m, curstat_t *s)
{
	/* wwwWarp window callbacks */
	register_focus_cb(m->userreg.form, cd_shell_focus_chg,
			  m->userreg.form);

	register_modvfy_cb(m->userreg.handle_txt, cd_txtline_vfy, s);
	register_activate_cb(m->userreg.handle_txt, userreg_focus_next, s);

	register_modvfy_cb(m->userreg.passwd_txt, dbprog_password_vfy, s);
	register_activate_cb(m->userreg.passwd_txt, userreg_focus_next, s);

	register_modvfy_cb(m->userreg.vpasswd_txt, dbprog_password_vfy, s);
	register_activate_cb(m->userreg.vpasswd_txt, userreg_focus_next, s);

	register_modvfy_cb(m->userreg.hint_txt, cd_txtline_vfy, s);
	register_activate_cb(m->userreg.hint_txt, userreg_focus_next, s);

	register_activate_cb(m->userreg.gethint_btn, userreg_gethint, s);

	register_modvfy_cb(m->userreg.email_txt, cd_txtline_vfy, s);
	register_activate_cb(m->userreg.email_txt, userreg_focus_next, s);

	register_activate_cb(m->userreg.region_chg_btn,
			     dbprog_regionsel_popup, s);

	register_modvfy_cb(m->userreg.postal_txt, cd_txtline_vfy, s);
	register_activate_cb(m->userreg.postal_txt, userreg_focus_next, s);

	register_modvfy_cb(m->userreg.age_txt, cd_txtnline_vfy, s);
	register_activate_cb(m->userreg.age_txt, userreg_focus_next, s);

	register_activate_cb(m->userreg.ok_btn, userreg_ok, s);
	register_activate_cb(m->userreg.priv_btn, userreg_privacy, s);
	register_activate_cb(m->userreg.cancel_btn, userreg_cancel, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->userreg.form),
		(XtCallbackProc) userreg_cancel,
		(XtPointer) s
	);
}


/*
 * register_help_callbacks
 *	Register all callback routines for widgets in the help text
 *	display window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_help_callbacks(widgets_t *m, curstat_t *s)
{
	/* Help popup window callbacks */
	register_focus_cb(m->help.form, cd_shell_focus_chg, m->help.form);

	register_activate_cb(m->help.online_btn, help_topic_sel, NULL);
	register_activate_cb(m->help.about_btn, cd_about, s);
	register_activate_cb(m->help.cancel_btn, cd_help_cancel, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->help.form),
		(XtCallbackProc) cd_help_cancel,
		(XtPointer) s
	);
}


/*
 * register_auth_callbacks
 *	Register all callback routines for widgets in the authorization
 *	window
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_auth_callbacks(widgets_t *m, curstat_t *s)
{
	/* Authorization window callbacks */
	register_focus_cb(m->auth.form, cd_shell_focus_chg, m->auth.form);

	register_modvfy_cb(m->auth.name_txt, cd_txtline_vfy, s);
	register_activate_cb(m->auth.name_txt, dbprog_focus_next, s);

	register_modvfy_cb(m->auth.pass_txt, dbprog_password_vfy, s);
	register_activate_cb(m->auth.pass_txt, dbprog_auth_ok, s);

	register_activate_cb(m->auth.ok_btn, dbprog_auth_ok, s);
	register_activate_cb(m->auth.cancel_btn, dbprog_auth_cancel, s);

	/* Install WM_DELETE_WINDOW handler */
	add_delw_callback(
		XtParent(m->auth.form),
		(XtCallbackProc) dbprog_auth_cancel,
		(XtPointer) s
	);
}


/*
 * register_dialog_callbacks
 *	Register all callback routines for widgets in the dialog
 *	box windows
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
register_dialog_callbacks(widgets_t *m, curstat_t *s)
{
	Widget	btn;

	/* Info, Warning, Fatal and DB Match dialog window callbacks */
	register_focus_cb(m->dialog.info, cd_shell_focus_chg,
			  m->dialog.info);
	register_focus_cb(m->dialog.info2, cd_shell_focus_chg,
			  m->dialog.info2);
	register_focus_cb(m->dialog.warning, cd_shell_focus_chg,
			  m->dialog.warning);
	register_focus_cb(m->dialog.fatal, cd_shell_focus_chg,
			  m->dialog.fatal);

	btn = XmMessageBoxGetChild(m->dialog.info, XmDIALOG_OK_BUTTON);
	register_activate_cb(btn, cd_info_ok, s);

#if XmVersion > 1001
	btn = XmMessageBoxGetChild(m->dialog.info2, XmDIALOG_OK_BUTTON);
#else
	btn = XmSelectionBoxGetChild(m->dialog.info2, XmDIALOG_OK_BUTTON);
#endif
	register_activate_cb(btn, cd_info2_ok, s);

	btn = XmMessageBoxGetChild(m->dialog.warning, XmDIALOG_OK_BUTTON);
	register_activate_cb(btn, cd_warning_ok, s);

	btn = XmMessageBoxGetChild(m->dialog.fatal, XmDIALOG_OK_BUTTON);
	register_activate_cb(btn, cd_fatal_ok, s);

	/* Install WM_DELETE_WINDOW handlers */
	add_delw_callback(
		XtParent(m->dialog.info),
		(XtCallbackProc) cd_info_ok,
		(XtPointer) s
	);
	add_delw_callback(
		XtParent(m->dialog.info2),
		(XtCallbackProc) cd_info2_ok,
		(XtPointer) s
	);
	add_delw_callback(
		XtParent(m->dialog.warning),
		(XtCallbackProc) cd_warning_ok,
		(XtPointer) s
	);
	add_delw_callback(
		XtParent(m->dialog.fatal),
		(XtCallbackProc) cd_fatal_ok,
		(XtPointer) s
	);
}


/***********************
 *   public routines   *
 ***********************/


/*
 * set_delw_atom
 *	Set the WM_DELETE_WINDOW atom - called from post_realize_config()
 *
 * Args:
 *	a - The atom
 *
 * Return:
 *	Nothing.
 */
void
set_delw_atom(Atom a)
{
	delw = a;
}


/*
 * add_delw_callback
 *	Add a callback function for WM_DELETE_WINDOW.
 *
 * Args:
 *	w - Widget
 *	func - Callback function
 *	arg - Callback function argument
 *
 * Return:
 *	Nothing.
 */
void
add_delw_callback(Widget w, XtCallbackProc func, XtPointer arg)
{
	XmAddWMProtocolCallback(w, delw, func, arg);
}


/*
 * rm_delw_callback
 *	Remove a callback function for WM_DELETE_WINDOW.
 *
 * Args:
 *	w - Widget
 *	func - Callback function
 *	arg - Callback function argument
 *
 * Return:
 *	Nothing.
 */
void
rm_delw_callback(Widget w, XtCallbackProc func, XtPointer arg)
{
	XmRemoveWMProtocolCallback(w, delw, func, arg);
}


/*
 * register_callbacks
 *	Top-level function to register all callback routines
 *
 * Args:
 *	m - Pointer to the main widgets structure
 *	s - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
void
register_callbacks(widgets_t *m, curstat_t *s)
{
	register_main_callbacks(m, s);
	register_keypad_callbacks(m, s);
	register_options_callbacks(m, s);
	register_perfmon_callbacks(m, s);
	register_dbprog_callbacks(m, s);
	register_dlist_callbacks(m, s);
	register_fullname_callbacks(m, s);
	register_extd_callbacks(m, s);
	register_extt_callbacks(m, s);
	register_credits_callbacks(m, s);
	register_segments_callbacks(m, s);
	register_submiturl_callbacks(m, s);
	register_regionsel_callbacks(m, s);
	register_langsel_callbacks(m, s);
	register_matchsel_callbacks(m, s);
	register_userreg_callbacks(m, s);
	register_help_callbacks(m, s);
	register_auth_callbacks(m, s);
	register_dialog_callbacks(m, s);
}


