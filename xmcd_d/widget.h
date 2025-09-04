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
#ifndef __WIDGET_H__
#define __WIDGET_H__

#ifndef lint
static char *_widget_h_ident_ = "@(#)widget.h	7.79 04/02/13";
#endif

/* Holder for all widgets */
typedef struct {
	/* Top-level shell */
	Widget		toplevel;

	/* Main window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	mode_btn;		/* Normal/Basic mode button */
		Widget	chkbox_frm;		/* Frame for Lock toggle */
		Widget	check_box;		/* Toggle button checkbox */
		Widget	lock_btn;		/* Caddy lock button */
		Widget	repeat_btn;		/* Repeat button */
		Widget	shuffle_btn;		/* Shuffle button */
		Widget	eject_btn;		/* Eject button */
		Widget	quit_btn;		/* Quit button */
		Widget	disc_ind;		/* Disc number indicator */
		Widget	track_ind;		/* Track number indicator */
		Widget	index_ind;		/* Index number indicator */
		Widget	time_ind;		/* Time indicator */
		Widget	rptcnt_ind;		/* Repeat count indicator */
		Widget	dbmode_ind;		/* CD information indicator */
		Widget	progmode_ind;		/* Program mode indicator */
		Widget	timemode_ind;		/* Time mode indicator */
		Widget	playmode_ind;		/* Play mode indicator */
		Widget	dtitle_ind;		/* Disc title indicator */
		Widget	ttitle_ind;		/* Track title indicator */
		Widget	dbprog_btn;		/* CDDB/Program button */
		Widget	options_btn;		/* Options button */
		Widget	ab_btn;			/* A->B mode button */
		Widget	sample_btn;		/* Sample mode button */
		Widget	time_btn;		/* Time display button */
		Widget	keypad_btn;		/* Keypad button */
		Widget	wwwwarp_bar;		/* wwwWarp menu bar */
		Widget	wwwwarp_btn;		/* wwwWarp button */
		Widget	level_scale;		/* Volume control slider */
		Widget	playpause_btn;		/* Play/Pause button */
		Widget	stop_btn;		/* Stop button */
		Widget	prevdisc_btn;		/* Previous Disc button */
		Widget	nextdisc_btn;		/* Previous Disc button */
		Widget	prevtrk_btn;		/* Prev Track button */
		Widget	nexttrk_btn;		/* Next Track button */
		Widget	previdx_btn;		/* Prev Index button */
		Widget	nextidx_btn;		/* Next Index button */
		Widget	rew_btn;		/* Search REW button */
		Widget	ff_btn;			/* Search FF button */
	} main;

	/* Keypad window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	keypad_lbl;		/* Keypad label */
		Widget	keypad_ind;		/* Keypad indicator */
		Widget	radio_frm;		/* Keypad mode Frame */
		Widget	radio_box;		/* Keypad mode radio box */
		Widget	track_btn;		/* Track button */
		Widget	disc_btn;		/* Disc button */
		Widget	num_btn[10];		/* Number keys */
		Widget	clear_btn;		/* Clear button */
		Widget	enter_btn;		/* Enter button */
		Widget	warp_lbl;		/* Track warp label */
		Widget	warp_scale;		/* Track warp slider */
		Widget	keypad_sep;		/* Separator bar */
		Widget	cancel_btn;		/* Cancel button */
	} keypad;

	/* Options window widgets */
	struct {
		Widget	form;			/* Form container */

		Widget	categ_lbl;		/* Category label */
		Widget	categ_list;		/* Category list */
		Widget	categ_sep;		/* Category separator */

		Widget	mode_lbl;		/* Play mode label */
		Widget	mode_chkbox_frm;	/* Play mode chkbox frame */
		Widget	mode_jitter_btn;	/* CDDA jitter corr button */
		Widget	mode_trkfile_btn;	/* One file per track button */
		Widget	mode_subst_btn;		/* Underscore subst button */
		Widget	mode_fmt_opt;		/* File format options menu */
		Widget	mode_perfmon_btn;	/* CDDA Perf monitor button */
		Widget	mode_path_lbl;		/* File path label */
		Widget	mode_path_txt;		/* File path tmpl text */
		Widget	mode_pathexp_btn;	/* File path expand button */
		Widget	mode_prog_lbl;		/* Program path label */
		Widget	mode_prog_txt;		/* Program path text */
		Widget	mode_nullmarker;
		Widget	mode_chkbox;		/* Play mode check box */
		Widget	mode_std_btn;		/* Standard button */
		Widget	mode_cdda_btn;		/* CDDA button */
		Widget	mode_file_btn;		/* Save file button */
		Widget	mode_pipe_btn;		/* Pipe to program button */
		Widget	mode_fmt_menu_lbl;	/* File format menu label */
		Widget	mode_fmt_menu_sep;	/* File format menu sep */
		Widget	mode_fmt_raw_btn;	/* CDR format button */
		Widget	mode_fmt_au_btn;	/* AU format button */
		Widget	mode_fmt_wav_btn;	/* WAV format button */
		Widget	mode_fmt_aiff_btn;	/* AIFF format button */
		Widget	mode_fmt_aifc_btn;	/* AIFF-C format button */
		Widget	mode_fmt_mp3_btn;	/* MP3 format button */
		Widget	mode_fmt_ogg_btn;	/* Ogg Vorbis format button */
		Widget	mode_fmt_flac_btn;	/* FLAC format button */
		Widget	mode_fmt_aac_btn;	/* AAC format button */
		Widget	mode_fmt_mp4_btn;	/* MP4 format button */
		Widget	mode_fmt_menu;		/* File format pulldown menu */

		Widget	method_opt;		/* Method options menu */
		Widget	qualval_lbl;		/* Quality factor label */
		Widget	qualval1_lbl;		/* Quality factor label */
		Widget	qualval2_lbl;		/* Quality factor label */
		Widget	qualval_scl;		/* Quality factor scale */
		Widget	bitrate_opt;		/* Bitrate options menu */
		Widget	minbrate_opt;		/* Min bitrate options menu */
		Widget	maxbrate_opt;		/* Max bitrate options menu */
		Widget	chmode_opt;		/* Channel mode options menu */
		Widget	compalgo_lbl;		/* Comp algorithm label */
		Widget	compalgo1_lbl;		/* Comp algorithm label */
		Widget	compalgo2_lbl;		/* Comp algorithm label */
		Widget	compalgo_scl;		/* Comp algorithm scale */
		Widget	encode_sep;		/* Separator */
		Widget	filters_lbl;		/* Filters label */
		Widget	freq_lbl;		/* Frequency label */
		Widget	width_lbl;		/* Width label */
		Widget	lp_opt;			/* Lopass option menu */
		Widget	lp_freq_txt;		/* Lopass frequency text */
		Widget	lp_width_txt;		/* Lopass width text */
		Widget	hp_opt;			/* Hipass option menu */
		Widget	hp_freq_txt;		/* Hipass frequency text */
		Widget	hp_width_txt;		/* Hipass width text */
		Widget	encode_sep2;		/* Separator */
		Widget	copyrt_btn;		/* Copyright button */
		Widget	orig_btn;		/* Original button */
		Widget	nores_btn;		/* No bit reservoir button */
		Widget	chksum_btn;		/* Checksum button */
		Widget	iso_btn;		/* Strict ISO button */
		Widget	addtag_btn;		/* Add tag button */
		Widget	id3tag_ver_opt;		/* ID3tag version option mnu */
		Widget	comp_nullmarker;
		Widget	method_menu_lbl;	/* Method menu label */
		Widget	method_menu_sep;	/* Method menu separator */
		Widget	method_menu;		/* Method menu */
		Widget	mode0_btn;		/* Mode0 button */
		Widget	mode1_btn;		/* Mode1 button */
		Widget	mode2_btn;		/* Mode2 button */
		Widget	mode3_btn;		/* Mode3 button */
		Widget	bitrate_menu_lbl;	/* Bitrate menu label */
		Widget	bitrate_menu_sep;	/* Bitrate menu separator */
		Widget	bitrate_def_btn;	/* Default bitrate button */
		Widget	bitrate_menu;		/* Bitrate menu */
		Widget	minbrate_menu_lbl;	/* Min bitrate menu label */
		Widget	minbrate_menu_sep;	/* Min bitrate menu sep */
		Widget	minbrate_def_btn;	/* Min bitrate default btn */
		Widget	minbrate_menu;		/* Min bitrate menu */
		Widget	maxbrate_menu_lbl;	/* Max bitrate menu label */
		Widget	maxbrate_menu_sep;	/* Max bitrate menu sep */
		Widget	maxbrate_def_btn;	/* Max bitrate default btn */
		Widget	maxbrate_menu;		/* Max bitrate menu */
		Widget	chmode_menu_lbl;	/* Chan mode menu label */
		Widget	chmode_menu_sep;	/* Chan mode menu separator */
		Widget	chmode_stereo_btn;	/* Chan mode stereo btn */
		Widget	chmode_jstereo_btn;	/* Chan mode j-stereo btn */
		Widget	chmode_forcems_btn;	/* Chan mode force m/s btn */
		Widget	chmode_mono_btn;	/* Chan mode mono btn */
		Widget	chmode_menu;		/* Chan mode menu */
		Widget	lp_menu_lbl;		/* Lopass menu label */
		Widget	lp_menu_sep;		/* Lopass menu separator */
		Widget	lp_off_btn;		/* Lopass off button */
		Widget	lp_auto_btn;		/* Lopass auto button */
		Widget	lp_manual_btn;		/* Lopass manual button */
		Widget	lp_menu;		/* Lopass menu */
		Widget	hp_menu_lbl;		/* Hipass menu label */
		Widget	hp_menu_sep;		/* Hipass menu separator */
		Widget	hp_off_btn;		/* Hipass off button */
		Widget	hp_auto_btn;		/* Hipass auto button */
		Widget	hp_manual_btn;		/* Hipass manual button */
		Widget	hp_menu;		/* Hipass menu */
		Widget	id3tag_ver_menu_lbl;	/* ID3tag version menu label */
		Widget	id3tag_ver_menu_sep;	/* ID3tag version menu sep */
		Widget	id3tag_v1_btn;		/* ID3tag version 1 button */
		Widget	id3tag_v2_btn;		/* ID3tag version 2 button */
		Widget	id3tag_both_btn;	/* ID3tag both button */
		Widget	id3tag_ver_menu;	/* ID3tag version menu */

		Widget	lame_lbl;		/* Advanced LAME opts label */
		Widget	lame_mode_lbl;		/* LAME opts mode label */
		Widget	lame_radbox_frm;	/* LAME opts radbox frame */
		Widget	lame_opts_lbl;		/* LAME opts string label */
		Widget	lame_opts_txt;		/* LAME opts string label */
		Widget	lame_nullmarker;
		Widget	lame_radbox;		/* LAME opts radio box */
		Widget	lame_disable_btn;	/* LAME opts disable button */
		Widget	lame_insert_btn;	/* LAME opts insert button */
		Widget	lame_append_btn;	/* LAME opts append button */
		Widget	lame_replace_btn;	/* LAME opts replace button */

		Widget	sched_lbl;		/* Sched options label */
		Widget	sched_rd_lbl;		/* Read sched label */
		Widget	sched_rd_radbox_frm;	/* Read sched radio box frm */
		Widget	sched_wr_lbl;		/* Write sched label */
		Widget	sched_wr_radbox_frm;	/* Write sched radio box frm */
		Widget	hb_tout_lbl;		/* heartbeat timeout label */
		Widget	hb_tout_txt;		/* heartbeat timeout text */
		Widget	sched_nullmarker;
		Widget	sched_rd_radbox;	/* Read sched radio box */
		Widget	sched_wr_radbox;	/* Write sched radio box */
		Widget	sched_rd_norm_btn;	/* Read sched normal button */
		Widget	sched_rd_hipri_btn;	/* Read sched hipri button */
		Widget	sched_wr_norm_btn;	/* Write sched normal button */
		Widget	sched_wr_hipri_btn;	/* Write sched hipri button */

		Widget	load_lbl;		/* Load options label */
		Widget	load_chkbox_frm;	/* Load options chkbox frame */
		Widget	exit_lbl;		/* Exit options label */
		Widget	exit_radbox_frm;	/* Exit options frame */
		Widget	done_lbl;		/* Done options label */
		Widget	done_chkbox_frm;	/* Done options frame */
		Widget	eject_lbl;		/* Eject options label */
		Widget	eject_chkbox_frm;	/* Eject options frame */
		Widget	automation_nullmarker;
		Widget	load_chkbox;		/* Load options check box */
		Widget	exit_radbox;		/* Exit options radio box */
		Widget	done_chkbox;		/* Done options check box */
		Widget	eject_chkbox;		/* Eject options check box */
		Widget	load_none_btn;		/* None button */
		Widget	load_spdn_btn;		/* Spin Down button */
		Widget	load_play_btn;		/* Auto Play button */
		Widget	load_lock_btn;		/* Auto Lock button */
		Widget	exit_none_btn;		/* None button */
		Widget	exit_stop_btn;		/* Auto Stop button */
		Widget	exit_eject_btn;		/* Auto Eject button */
		Widget	done_eject_btn;		/* Auto Eject button */
		Widget	done_exit_btn;		/* Auto Exit button */
		Widget	eject_exit_btn;		/* Auto Exit button */

		Widget	chg_lbl;		/* Changer options label */
		Widget	chg_chkbox_frm;		/* Changer options frame */
		Widget	changer_nullmarker;
		Widget	chg_chkbox;		/* Changer options chk box */
		Widget	chg_multiplay_btn;	/* Changer multi-play button */
		Widget	chg_reverse_btn;	/* Changer reverse button */

		Widget	chroute_lbl;		/* Channel route label */
		Widget	chroute_radbox_frm;	/* Channel route frame */
		Widget	outport_lbl;		/* Output port label */
		Widget	outport_chkbox_frm;	/* Output port frame */
		Widget	chroute_nullmarker;
		Widget	chroute_radbox;		/* Channel route radio box */
		Widget	chroute_stereo_btn;	/* Stereo button */
		Widget	chroute_rev_btn;	/* Stereo Reverse button */
		Widget	chroute_left_btn;	/* Mono Left-only button */
		Widget	chroute_right_btn;	/* Mono Right-only button */
		Widget	chroute_mono_btn;	/* Mono Left+Right button */
		Widget	outport_chkbox;		/* Outport select check box */
		Widget	outport_spkr_btn;	/* Speaker */
		Widget	outport_phone_btn;	/* Headphone */
		Widget	outport_line_btn;	/* Line-out */

		Widget	vol_lbl;		/* Vol ctrl taper label */
		Widget	vol_radbox_frm;		/* Vol ctrl taper frame */
		Widget	bal_lbl;		/* Balance ctrl label */
		Widget	ball_lbl;		/* Balance ctrl left label */
		Widget	balr_lbl;		/* Balance ctrl right label */
		Widget	bal_scale;		/* Balance ctrl scale */
		Widget	balctr_btn;		/* Balance center button */
		Widget	cdda_att_lbl;		/* CDDA attenuator label */
		Widget	cdda_att_scale;		/* CDDA attentuator scale */
		Widget	cdda_fadeout_btn;	/* CDDA fade-out button */
		Widget	cdda_fadein_btn;	/* CDDA fade-in button */
		Widget	cdda_fadeout_lbl;	/* CDDA fade-out label */
		Widget	cdda_fadein_lbl;	/* CDDA fade-in label */
		Widget	volbal_nullmarker;
		Widget	vol_radbox;		/* Vol ctrl taper radio box */
		Widget	vol_linear_btn;		/* Linear button */
		Widget	vol_square_btn;		/* Square button */
		Widget	vol_invsqr_btn;		/* Inverse Square button */

		Widget	cddb_lbl;		/* CDDB options label */
		Widget	mbrowser_lbl;		/* Music browser label */
		Widget	mbrowser_frm;		/* Music browser frame */
		Widget	lookup_lbl;		/* Lookup priority label */
		Widget	lookup_frm;		/* Lookup priority frame */
		Widget	cddb_sep1;		/* Separator bar */
		Widget	serv_tout_lbl;		/* Service timeout label */
		Widget	serv_tout_txt;		/* Service timeout text */
		Widget	cache_tout_lbl;		/* Cache timeout label */
		Widget	cache_tout_txt;		/* Cache timeout text */
		Widget	cddb_sep2;		/* Separator bar */
		Widget	use_proxy_btn;		/* Use HTTP proxy server btn */
		Widget	proxy_srvr_lbl;		/* Proxy server label */
		Widget	proxy_srvr_txt;		/* Proxy server text */
		Widget	proxy_port_lbl;		/* Proxy port label */
		Widget	proxy_port_txt;		/* Proxy port text */
		Widget	proxy_auth_btn;		/* Proxy authorization btn */
		Widget	cddb_nullmarker;
		Widget	mbrowser_radbox;	/* Music browser radio box */
		Widget	autobr_btn;		/* Auto music browser */
		Widget	manbr_btn;		/* Manual music browser */
		Widget	lookup_radbox;		/* Lookup priority radio box */
		Widget	cddb_pri_btn;		/* CDDB priority button */
		Widget	cdtext_pri_btn;		/* CD-TEXT priority button */

		Widget	options_sep;		/* Separator bar */
		Widget	reset_btn;		/* Reset button */
		Widget	save_btn;		/* Save button */
		Widget	ok_btn;			/* OK button */
	} options;

	/* CDDA Performance Monitor window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	speed_lbl;		/* Speed factor label */
		Widget	speed_ind;		/* Speed factor indicator */
		Widget	frames_lbl;		/* Frames xfer'd label */
		Widget	frames_ind;		/* Frames xfer'd indicator */
		Widget	fps_lbl;		/* Frames per sec label */
		Widget	fps_ind;		/* Frames per sec indicator */
		Widget	ttc_lbl;		/* Time-to-completion label */
		Widget	ttc_ind;		/* Time-to-completion ind */
		Widget	perfmon_sep;		/* Separator bar */
		Widget	cancel_btn;		/* Cancel button */
	} perfmon;

	/* CD Information/Program window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	tottime_ind;		/* Total time indicator */
		Widget	inetoffln_btn;		/* Internet Offline button */
		Widget	logo_lbl;		/* Logo label */
		Widget	dlist_btn;		/* Disc list button */
		Widget	artist_lbl;		/* Disc artist label */
		Widget	artist_txt;		/* Disc artist text */
		Widget	title_lbl;		/* Disc title label */
		Widget	title_txt;		/* Disc title text */
		Widget	fullname_btn;		/* Artist fullname button */
		Widget	extd_lbl;		/* Disc label */
		Widget	extd_btn;		/* Disc details button */
		Widget	dcredits_btn;		/* Disc credits button */
		Widget	segments_btn;		/* Segments button */
		Widget	dbprog_sep1;		/* Separator bar */
		Widget	trklist_lbl;		/* Track list label */
		Widget	trklist_sw;		/* Track list scrolled window */
		Widget	trk_list;		/* Track list */
		Widget	radio_lbl;		/* Time dpy radio box label */
		Widget	radio_frm;		/* Time dpy radio box Frame */
		Widget	radio_box;		/* Time dpy radio box */
		Widget	tottime_btn;		/* Total time button */
		Widget	trktime_btn;		/* Track time button */
		Widget	ttitle_lbl;		/* Track title label */
		Widget	ttitle_txt;		/* Track title text */
		Widget	apply_btn;		/* Track title apply button */
		Widget	extt_lbl;		/* Track label */
		Widget	extt_btn;		/* Track details button */
		Widget	tcredits_btn;		/* Track credits button */
		Widget	pgm_lbl;		/* Program label */
		Widget	addpgm_btn;		/* Add program button */
		Widget	clrpgm_btn;		/* Clear program button */
		Widget	savepgm_btn;		/* Save program button */
		Widget	pgmseq_lbl;		/* Program sequence label */
		Widget	pgmseq_txt;		/* Program sequence text */
		Widget	userreg_btn;		/* User registration button */
		Widget	dbprog_sep2;		/* Separator bar */
		Widget	submit_btn;		/* CDDB submit button */
		Widget	flush_btn;		/* CDDB flush cache button */
		Widget	reload_btn;		/* Reload CD info button */
		Widget	ok_btn;			/* OK button */
	} dbprog;

	/* Disc list window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	type_opt;		/* List type option menu */
		Widget	type_menu;		/* List type pulldown menu */
		Widget	menu_lbl;		/* Menu title label */
		Widget	menu_sep;		/* Menu separator bar */
		Widget	hist_btn;		/* History menu button */
		Widget	chgr_btn;		/* Changer menu button */
		Widget	disclist_lbl;		/* Disc list label */
		Widget	disclist_sw;		/* Disc list scrolled window */
		Widget	disc_list;		/* Disc list */
		Widget	show_btn;		/* Show button */
		Widget	goto_btn;		/* Change to button */
		Widget	del_btn;		/* Delete button */
		Widget	delall_btn;		/* Delete all button */
		Widget	rescan_btn;		/* Re-scan button */
		Widget	dlist_sep;		/* Separator bar */
		Widget	cancel_btn;		/* Cancel button */
	} dlist;

	/* Full name window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	head_lbl;		/* Head label */
		Widget	dispname_lbl;		/* display name label */
		Widget	dispname_txt;		/* display name label */
		Widget	autogen_btn;		/* auto-gen button */
		Widget	lastname_lbl;		/* last name label */
		Widget	lastname_txt;		/* last name label */
		Widget	firstname_lbl;		/* first name label */
		Widget	firstname_txt;		/* first name label */
		Widget	the_btn;		/* "The" togglebutton */
		Widget	the_txt;		/* "The" text */
		Widget	fullname_sep;		/* Separator bar */
		Widget	ok_btn;			/* OK button */
	} fullname;

	/* Disc details window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	discno_lbl;		/* Disc number label */
		Widget	disc_lbl;		/* Disc artist/title label */
		Widget	dbextd_sep;		/* Separator bar */
		Widget	album_lbl;		/* Album label */
		Widget	sorttitle_lbl;		/* Sort title label */
		Widget	sorttitle_txt;		/* Sort title text */
		Widget	the_btn;		/* The togglebutton */
		Widget	the_txt;		/* The text */
		Widget	year_lbl;		/* Year label */
		Widget	year_txt;		/* Year text */
		Widget	label_lbl;		/* Label label */
		Widget	label_txt;		/* Label text */
		Widget	comp_btn;		/* Compilation button */
		Widget	genre_opt[2];		/* Genre option menu */
		Widget	genre_menu[2];		/* Genre pulldown menu */
		Widget	genre_lbl[2];		/* Genre menu label */
		Widget	genre_sep[2];		/* Genre menu separator */
		Widget	genre_none_btn[2];	/* Genre "none" button */
		Widget	subgenre_opt[2];	/* Subgenre option menu */
		Widget	subgenre_menu[2];	/* Subgenre pulldown menu */
		Widget	subgenre_lbl[2];	/* Subgenre menu label */
		Widget	subgenre_sep[2];	/* Subgenre menu separator */
		Widget	subgenre_none_btn[2];	/* Subgenre "none" button */
		Widget	dnum_lbl;		/* Disc number in set label */
		Widget	dnum_txt;		/* Disc number in set text */
		Widget	of_lbl;			/* "of" label */
		Widget	tnum_txt;		/* Total number in set text */
		Widget	region_lbl;		/* Region label */
		Widget	region_txt;		/* Region text */
		Widget	region_chg_btn;		/* Region change button */
		Widget	lang_lbl;		/* Language label */
		Widget	lang_txt;		/* Language text */
		Widget	lang_chg_btn;		/* Language change button */
		Widget	dbextd_sep2;		/* Separator bar */
		Widget	notes_lbl;		/* Disc notes label */
		Widget	notes_txt;		/* Disc notes text */
		Widget	discid_lbl;		/* Xmcd disc ID label */
		Widget	discid_ind;		/* Xmcd disc ID ind label */
		Widget	mcn_lbl;		/* Media catalog # label */
		Widget	mcn_ind;		/* Media catalog # ind label */
		Widget	rev_lbl;		/* Revision label */
		Widget	rev_ind;		/* Revision ind label */
		Widget	cert_lbl;		/* Certifier label */
		Widget	cert_ind;		/* Certifier ind label */
		Widget	dbextd_sep3;		/* Separator bar */
		Widget	ok_btn;			/* OK button */
	} dbextd;

	/* Track details window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	autotrk_btn;		/* Auto-track button */
		Widget	prev_btn;		/* Prev track arrow button */
		Widget	next_btn;		/* Next track arrow button */
		Widget	trkno_lbl;		/* Track number label */
		Widget	trk_lbl;		/* Track title label */
		Widget	dbextt_sep;		/* Separator bar */
		Widget	sorttitle_lbl;		/* Sort title label */
		Widget	sorttitle_txt;		/* Sort title text */
		Widget	the_btn;		/* The togglebutton */
		Widget	the_txt;		/* The text */
		Widget	artist_lbl;		/* Artist label */
		Widget	artist_txt;		/* Artist text */
		Widget	fullname_btn;		/* Artist full name button */
		Widget	year_lbl;		/* Year label */
		Widget	year_txt;		/* Year text */
		Widget	label_lbl;		/* Label label */
		Widget	label_txt;		/* Label text */
		Widget	bpm_lbl;		/* BPM label */
		Widget	bpm_txt;		/* BPM text */
		Widget	genre_opt[2];		/* Genre option menu */
		Widget	genre_menu[2];		/* Genre pulldown menu */
		Widget	genre_lbl[2];		/* Genre menu label */
		Widget	genre_sep[2];		/* Genre menu separator */
		Widget	genre_none_btn[2];	/* Genre "none" button */
		Widget	subgenre_opt[2];	/* Subgenre option menu */
		Widget	subgenre_menu[2];	/* Subgenre pulldown menu */
		Widget	subgenre_lbl[2];	/* Subgenre menu label */
		Widget	subgenre_sep[2];	/* Subgenre menu separator */
		Widget	subgenre_none_btn[2];	/* Subgenre "none" button */
		Widget	dbextt_sep2;		/* Separator bar */
		Widget	notes_lbl;		/* Track notes label */
		Widget	notes_txt;		/* Track notes text */
		Widget	isrc_lbl;		/* ISRC label */
		Widget	isrc_ind;		/* ISRC indicator label */
		Widget	dbextt_sep3;		/* Separator bar */
		Widget	ok_btn;			/* OK button */
	} dbextt;

	/* Credits list/editor window */
	struct {
		Widget	form;			/* Form container */
		Widget	autotrk_btn;		/* Auto-track button */
		Widget	prev_btn;		/* Prev track arrow button */
		Widget	next_btn;		/* Next track arrow button */
		Widget	disctrk_lbl;		/* Disc/track number label */
		Widget	title_lbl;		/* Title label */
		Widget	credlist_lbl;		/* Credit list label */
		Widget	cred_list;		/* Credit list */
		Widget	cred_sep1;		/* Separator */
		Widget	crededit_lbl;		/* Credit editor label */
		Widget	prirole_opt;		/* Role option menu */
		Widget	prirole_menu;		/* Role pulldown menu */
		Widget	prirole_lbl;		/* Role menu label */
		Widget	prirole_sep;		/* Role menu separator */
		Widget	prirole_none_btn;	/* Role "none" button */
		Widget	subrole_opt;		/* Subrole option menu */
		Widget	subrole_menu;		/* Subrole pulldown menu */
		Widget	subrole_lbl;		/* Subrole menu label */
		Widget	subrole_sep;		/* Subrole menu separator */
		Widget	subrole_none_btn;	/* Subrole "none" button */
		Widget	name_lbl;		/* Name label */
		Widget	name_txt;		/* Name text */
		Widget	fullname_btn;		/* Full name button */
		Widget	notes_lbl;		/* Notes label */
		Widget	notes_txt;		/* Notes text */
		Widget	cred_sep2;		/* Separator */
		Widget	add_btn;		/* Add button */
		Widget	mod_btn;		/* Modify button */
		Widget	del_btn;		/* Delete button */
		Widget	ok_btn;			/* OK button */
	} credits;

	/* Segments list/editor window */
	struct {
		Widget	form;			/* Form container */
		Widget	discno_lbl;		/* Disc number label */
		Widget	disc_lbl;		/* Disc artist/title label */
		Widget	seglist_lbl;		/* Segment list label */
		Widget	seg_list;		/* Segment list */
		Widget	seg_sep1;		/* Separator bar */
		Widget	segedit_lbl;		/* Segment editor label */
		Widget	name_lbl;		/* Segment name label */
		Widget	name_txt;		/* Segment name text */
		Widget	start_lbl;		/* Start label */
		Widget	end_lbl;		/* End label */
		Widget	track_lbl;		/* Track label */
		Widget	frame_lbl;		/* Frame label */
		Widget	starttrk_txt;		/* Start track text */
		Widget	startfrm_txt;		/* Start frame text */
		Widget	endtrk_txt;		/* End track text */
		Widget	endfrm_txt;		/* End frame text */
		Widget	startptr_lbl;		/* Start pointer label */
		Widget	endptr_lbl;		/* End pointer label */
		Widget	playset_lbl;		/* Play-set label */
		Widget	set_btn;		/* Set button */
		Widget	playpaus_btn;		/* Play pause button */
		Widget	stop_btn;		/* Stop button */
		Widget	notes_lbl;		/* Segment notes label */
		Widget	notes_txt;		/* Segment notes text */
		Widget	segment_lbl;		/* Segment label */
		Widget	credits_btn;		/* Credits button */
		Widget	seg_sep2;		/* Separator bar */
		Widget	add_btn;		/* Add button */
		Widget	mod_btn;		/* Modify button */
		Widget	del_btn;		/* Delete button */
		Widget	ok_btn;			/* OK button */
	} segments;

	/* Submit URL window */
	struct {
		Widget	form;			/* Form container */
		Widget	logo_lbl;		/* Logo label */
		Widget	heading_lbl;		/* Heading label */
		Widget	categ_lbl;		/* Category label */
		Widget	categ_txt;		/* Category text */
		Widget	name_lbl;		/* Name label */
		Widget	name_txt;		/* Name text */
		Widget	url_lbl;		/* URL label */
		Widget	url_txt;		/* URL text */
		Widget	desc_lbl;		/* Description label */
		Widget	desc_txt;		/* Description text */
		Widget	submiturl_sep;		/* Separator bar */
		Widget	submit_btn;		/* Submit button */
		Widget	ok_btn;			/* OK button */
	} submiturl;

	/* Region selector window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	region_lbl;		/* Region list label */
		Widget	region_list;		/* Region list */
		Widget	region_sep;		/* Separator bar */
		Widget	ok_btn;			/* OK button */
	} regionsel;

	/* Language selector window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	lang_lbl;		/* Language list label */
		Widget	lang_list;		/* Language list */
		Widget	lang_sep;		/* Separator bar */
		Widget	ok_btn;			/* OK button */
	} langsel;

	/* Multiple match selector window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	logo_lbl;		/* Match list logo label */
		Widget	matchsel_lbl;		/* Match list label */
		Widget	matchsel_list;		/* Match list */
		Widget	matchsel_sep;		/* Separator bar */
		Widget	ok_btn;			/* OK button */
	} matchsel;

	/* wwwWarp popup menu widgets */
	struct {
		Widget	menu;			/* Main popup menu */
		Widget	genurls_menu;		/* General URLs menu */
		Widget	discurls_menu;		/* Album-specific URLs menu */
	} wwwwarp;

	/* User registration window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	logo_lbl;		/* Logo label */
		Widget	caption_lbl;		/* Caption label */
		Widget	handle_lbl;		/* Handle label */
		Widget	handle_txt;		/* Handle text */
		Widget	passwd_lbl;		/* Password label */
		Widget	passwd_txt;		/* Password text */
		Widget	vpasswd_lbl;		/* Verify password label */
		Widget	vpasswd_txt;		/* Verify password text */
		Widget	hint_lbl;		/* Password hint label */
		Widget	hint_txt;		/* Password hint text */
		Widget	gethint_btn;		/* Get hint button */
		Widget	email_lbl;		/* Email addr label */
		Widget	email_txt;		/* Email addr text */
		Widget	region_lbl;		/* Region label */
		Widget	region_txt;		/* Region text */
		Widget	region_chg_btn;		/* Region change button */
		Widget	postal_lbl;		/* Postal code label */
		Widget	postal_txt;		/* Postal code text */
		Widget	age_lbl;		/* Age label */
		Widget	age_txt;		/* Age text */
		Widget	gender_lbl;		/* Gender label */
		Widget	gender_frm;		/* Gender frame */
		Widget	gender_radbox;		/* Gender radio box */
		Widget	male_btn;		/* Male button */
		Widget	female_btn;		/* Female button */
		Widget	unspec_btn;		/* Unspecified button */
		Widget	allowmail_btn;		/* Allow mail toggle button */
		Widget	allowstats_btn;		/* Allow stats toggle button */
		Widget	userreg_sep;		/* separator bar */
		Widget	ok_btn;			/* OK button */
		Widget	priv_btn;		/* Privacy info button */
		Widget	cancel_btn;		/* Cancel button */
	} userreg;

	/* Help window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	topic_opt;		/* Topic option menu */
		Widget	topic_menu;		/* Topic pulldown menu */
		Widget	topic_lbl;		/* Topic menu label */
		Widget	topic_sep;		/* Topic menu separator */
		Widget	online_btn;		/* Online Help button */
		Widget	topic_sep2;		/* Topic menu separator2 */
		Widget	help_txt;		/* Help text */
		Widget	help_sep;		/* Separator bar */
		Widget	about_btn;		/* About button */
		Widget	cancel_btn;		/* Cancel button */
	} help;

	/* Authorization window widgets */
	struct {
		Widget	form;			/* Form container */
		Widget	auth_lbl;		/* Authorization label */
		Widget	name_lbl;		/* Name label */
		Widget	name_txt;		/* Name text */
		Widget	pass_lbl;		/* Password label */
		Widget	pass_txt;		/* Password text */
		Widget	auth_sep;		/* Separator bar */
		Widget	ok_btn;			/* OK button */
		Widget	cancel_btn;		/* Cancel button */
	} auth;

	/* Dialog windows widgets */
	struct {
		Widget	info;			/* Info popup */
		Widget	info2;			/* Info2 popup */
		Widget	info2_txt;		/* Info2 text widget */
		Widget	warning;		/* Warning popup */
		Widget	fatal;			/* Fatal error popup */
		Widget	confirm;		/* Confirm popup */
		Widget	working;		/* Working popup */
		Widget	about;			/* About popup */
	} dialog;

	/* Tooltip shell widgets */
	struct {
		Widget	shell;			/* Shell */
		Widget	tooltip_lbl;		/* Tool-tip label */
	} tooltip;
} widgets_t;


/* Holder for all button-face pixmaps */
typedef struct {
	/* Main window pixmaps */
	struct {
		Pixmap	icon_pixmap;		/* Icon logo */
		Pixmap	mode_pixmap;		/* Mode */
		Pixmap	mode_hlpixmap;		/* Mode */
		Pixmap	lock_pixmap;		/* Prevent caddy removal */
		Pixmap	repeat_pixmap;		/* Repeat */
		Pixmap	shuffle_pixmap;		/* Shuffle */
		Pixmap	eject_pixmap;		/* Eject */
		Pixmap	eject_hlpixmap;		/* Eject */
		Pixmap	quit_pixmap;		/* Quit */
		Pixmap	quit_hlpixmap;		/* Quit */
		Pixmap	dbprog_pixmap;		/* CDDB/Prog */
		Pixmap	dbprog_hlpixmap;	/* CDDB/Prog */
		Pixmap	options_pixmap;		/* Options */
		Pixmap	options_hlpixmap;	/* Options */
		Pixmap	ab_pixmap;		/* A->B */
		Pixmap	ab_hlpixmap;		/* A->B */
		Pixmap	sample_pixmap;		/* Sample */
		Pixmap	sample_hlpixmap;	/* Sample */
		Pixmap	time_pixmap;		/* Time */
		Pixmap	time_hlpixmap;		/* Time */
		Pixmap	keypad_pixmap;		/* Keypad */
		Pixmap	keypad_hlpixmap;	/* Keypad */
		Pixmap	world_pixmap;		/* wwwWarp */
		Pixmap	world_hlpixmap;		/* wwwWarp */
		Pixmap	playpause_pixmap;	/* Play/Pause */
		Pixmap	playpause_hlpixmap;	/* Play/Pause */
		Pixmap	stop_pixmap;		/* Stop */
		Pixmap	stop_hlpixmap;		/* Stop */
		Pixmap	prevdisc_pixmap;	/* Prev Disc */
		Pixmap	prevdisc_hlpixmap;	/* Prev Disc */
		Pixmap	nextdisc_pixmap;	/* Next Disc */
		Pixmap	nextdisc_hlpixmap;	/* Next Disc */
		Pixmap	prevtrk_pixmap;		/* Prev Track */
		Pixmap	prevtrk_hlpixmap;	/* Prev Track */
		Pixmap	nexttrk_pixmap;		/* Next Track */
		Pixmap	nexttrk_hlpixmap;	/* Next Track */
		Pixmap	previdx_pixmap;		/* Prev Index */
		Pixmap	previdx_hlpixmap;	/* Prev Index */
		Pixmap	nextidx_pixmap;		/* Next Index */
		Pixmap	nextidx_hlpixmap;	/* Next Index */
		Pixmap	rew_pixmap;		/* Search REW */
		Pixmap	rew_hlpixmap;		/* Search REW */
		Pixmap	ff_pixmap;		/* Search FF */
		Pixmap	ff_hlpixmap;		/* Search FF */
	} main;

	/* CD Information/Program window pixmaps */
	struct {
		Pixmap	logo_pixmap;		/* CD Logo */
	} dbprog;

	/* CD Match window pixmaps */
	struct {
		Pixmap	logo_pixmap;		/* CDDB Logo */
	} matchsel;

	/* User registration window pixmaps */
	struct {
		Pixmap	logo_pixmap;		/* CDDB Logo */
	} userreg;

	/* Segments window pixmaps */
	struct {
		Pixmap	playpause_pixmap;	/* Play/Pause */
		Pixmap	playpause_hlpixmap;	/* Play/Pause */
		Pixmap	stop_pixmap;		/* Stop */
		Pixmap	stop_hlpixmap;		/* Stop */
	} segments;

	/* SubmitURL window pixmaps */
	struct {
		Pixmap	logo_pixmap;		/* CDDB Logo */
	} submiturl;

	/* Dialog windows pixmaps */
	struct {
		Pixmap	about_pixmap;		/* Program logo */
	} dialog;
} pixmaps_t;


/* Mode flags for bm_to_px(): used to set foreground/background colors */
#define BM_PX_NORMAL	0		/* normal/normal */
#define BM_PX_REV	1		/* reverse/reverse */
#define BM_PX_HIGHLIGHT	2		/* highlight/normal */
#define BM_PX_WHITE	3		/* white/normal */
#define BM_PX_BLACK	4		/* black/normal */
#define BM_PX_BW	5		/* black/white */
#define BM_PX_BWREV	6		/* white/black */


/* Public function prototypes */
extern Pixmap	bm_to_px(Widget, void *, int, int, int, int);
extern void	create_widgets(widgets_t *);
extern void	pre_realize_config(widgets_t *);
extern void	post_realize_config(widgets_t *, pixmaps_t *);

#endif /* __WIDGET_H__ */

