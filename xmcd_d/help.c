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
static char *_help_c_ident_ = "@(#)help.c	7.91 04/03/11";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "common_d/version.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/callback.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/help.h"


extern appdata_t	app_data;
extern widgets_t	widgets;
extern FILE		*errfp;


STATIC wname_t		wname[] = {
    { &widgets.main.mode_btn, 		"Xm%cMode.btn",	    HELP_XLAT_1	},
    { &widgets.main.lock_btn, 		"Xm%cMain.cbx",	    HELP_XLAT_1	},
    { &widgets.main.repeat_btn, 	"Xm%cMain.cbx",	    HELP_XLAT_1	},
    { &widgets.main.shuffle_btn, 	"Xm%cMain.cbx",	    HELP_XLAT_1	},
    { &widgets.main.eject_btn, 		"Xm%cEject.btn",    HELP_XLAT_1	},
    { &widgets.main.quit_btn, 		"Xm%cQuit.btn",	    HELP_XLAT_1	},
    { &widgets.main.dbprog_btn, 	"Xm%cDbProg.btn",   HELP_XLAT_1	},
    { &widgets.main.options_btn, 	"Xm%cOptions.btn",  HELP_XLAT_1	},
    { &widgets.main.ab_btn,	 	"Xm%cAb.btn",	    HELP_XLAT_1	},
    { &widgets.main.sample_btn, 	"Xm%cSample.btn",   HELP_XLAT_1	},
    { &widgets.main.time_btn, 		"Xm%cTime.btn",	    HELP_XLAT_1	},
    { &widgets.main.keypad_btn, 	"Xm%cKeypad.btn",   HELP_XLAT_1	},
    { &widgets.main.wwwwarp_btn, 	"Xm%cWWWwarp.btn",  HELP_XLAT_1	},
    { &widgets.main.level_scale, 	"Xm%cLevel.scl",    HELP_XLAT_1	},
    { &widgets.main.playpause_btn, 	"Xm%cPlayPaus.btn", HELP_XLAT_1	},
    { &widgets.main.stop_btn, 		"Xm%cStop.btn",	    HELP_XLAT_1	},
    { &widgets.main.prevdisc_btn, 	"Xm%cPrevDisc.btn", HELP_XLAT_1	},
    { &widgets.main.nextdisc_btn, 	"Xm%cNextDisc.btn", HELP_XLAT_1	},
    { &widgets.main.prevtrk_btn, 	"Xm%cPrevTrk.btn",  HELP_XLAT_1	},
    { &widgets.main.nexttrk_btn, 	"Xm%cNextTrk.btn",  HELP_XLAT_1	},
    { &widgets.main.previdx_btn, 	"Xm%cPrevIdx.btn",  HELP_XLAT_1	},
    { &widgets.main.nextidx_btn, 	"Xm%cNextIdx.btn",  HELP_XLAT_1	},
    { &widgets.main.rew_btn, 		"Xm%cRew.btn",	    HELP_XLAT_1	},
    { &widgets.main.ff_btn, 		"Xm%cFf.btn",	    HELP_XLAT_1	},
    { &widgets.main.disc_ind, 		"Xm%cDisc.lbl",	    HELP_XLAT_1	},
    { &widgets.main.track_ind, 		"Xm%cTrack.lbl",    HELP_XLAT_1	},
    { &widgets.main.index_ind, 		"Xm%cIndex.lbl",    HELP_XLAT_1	},
    { &widgets.main.time_ind, 		"Xm%cTime.lbl",	    HELP_XLAT_1	},
    { &widgets.main.rptcnt_ind, 	"Xm%cRptCnt.lbl",   HELP_XLAT_1	},
    { &widgets.main.dbmode_ind, 	"Xm%cDbMode.lbl",   HELP_XLAT_1	},
    { &widgets.main.progmode_ind, 	"Xm%cProgMode.lbl", HELP_XLAT_1	},
    { &widgets.main.timemode_ind, 	"Xm%cTimeMode.lbl", HELP_XLAT_1	},
    { &widgets.main.playmode_ind, 	"Xm%cPlayMode.lbl", HELP_XLAT_1	},
    { &widgets.main.dtitle_ind, 	"Xm%cDiscTitl.lbl", HELP_XLAT_1	},
    { &widgets.main.ttitle_ind, 	"Xm%cTrkTitle.lbl", HELP_XLAT_1	},
    { &widgets.keypad.keypad_ind, 	"Kp%cInd.lbl",	    HELP_XLAT_1	},
    { &widgets.keypad.radio_box, 	"Kp%cSel.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[0], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[1], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[2], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[3], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[4], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[5], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[6], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[7], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[8], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.num_btn[9], 	"Kp%cNum.btn",	    HELP_XLAT_1	},
    { &widgets.keypad.clear_btn, 	"Kp%cClear.btn",    HELP_XLAT_1	},
    { &widgets.keypad.enter_btn, 	"Kp%cEnter.btn",    HELP_XLAT_1	},
    { &widgets.keypad.warp_lbl, 	"Kp%cWarp.scl",	    HELP_XLAT_1	},
    { &widgets.keypad.warp_scale, 	"Kp%cWarp.scl",	    HELP_XLAT_1	},
    { &widgets.keypad.cancel_btn, 	"Kp%cCancel.btn",   HELP_XLAT_1	},
    { &widgets.options.categ_list,	"Op%cCateg.lsw",    HELP_XLAT_2	},
    { &widgets.options.mode_chkbox,	"Op%cMode.cbx",	    HELP_XLAT_1	},
    { &widgets.options.mode_jitter_btn,	"Op%cJitter.btn",   HELP_XLAT_1	},
    { &widgets.options.mode_trkfile_btn,"Op%cTrkFile.btn",  HELP_XLAT_1	},
    { &widgets.options.mode_subst_btn,	"Op%cSubst.btn",    HELP_XLAT_1	},
    { &widgets.options.mode_fmt_opt,	"Op%cFileFmt.opt",  HELP_XLAT_1	},
    { &widgets.options.mode_perfmon_btn,"Op%cPerfMon.btn",  HELP_XLAT_1	},
    { &widgets.options.mode_path_txt,	"Op%cPath.txw",	    HELP_XLAT_1	},
    { &widgets.options.mode_pathexp_btn,"Op%cPathExp.btn",  HELP_XLAT_1	},
    { &widgets.options.mode_prog_txt,	"Op%cProg.txw",	    HELP_XLAT_1	},
    { &widgets.options.method_opt,	"Op%cEncode.opt",   HELP_XLAT_1	},
    { &widgets.options.qualval_lbl,	"Op%cQual.scl",	    HELP_XLAT_1	},
    { &widgets.options.qualval1_lbl,	"Op%cQual.scl",	    HELP_XLAT_1	},
    { &widgets.options.qualval2_lbl,	"Op%cQual.scl",	    HELP_XLAT_1	},
    { &widgets.options.qualval_scl,	"Op%cQual.scl",	    HELP_XLAT_1	},
    { &widgets.options.bitrate_opt,	"Op%cBitrate.opt",  HELP_XLAT_1	},
    { &widgets.options.minbrate_opt,	"Op%cBitrate.opt",  HELP_XLAT_1	},
    { &widgets.options.maxbrate_opt,	"Op%cBitrate.opt",  HELP_XLAT_1	},
    { &widgets.options.chmode_opt,	"Op%cChanMode.opt", HELP_XLAT_1	},
    { &widgets.options.compalgo_scl,	"Op%cCompAlgo.scl", HELP_XLAT_1	},
    { &widgets.options.compalgo_lbl,	"Op%cCompAlgo.scl", HELP_XLAT_1	},
    { &widgets.options.compalgo1_lbl,	"Op%cCompAlgo.scl", HELP_XLAT_1	},
    { &widgets.options.compalgo2_lbl,	"Op%cCompAlgo.scl", HELP_XLAT_1	},
    { &widgets.options.lp_opt,		"Op%cFilters.txw",  HELP_XLAT_1	},
    { &widgets.options.lp_freq_txt,	"Op%cFilters.txw",  HELP_XLAT_1	},
    { &widgets.options.lp_width_txt,	"Op%cFilters.txw",  HELP_XLAT_1	},
    { &widgets.options.hp_opt,		"Op%cFilters.txw",  HELP_XLAT_1	},
    { &widgets.options.hp_freq_txt,	"Op%cFilters.txw",  HELP_XLAT_1	},
    { &widgets.options.hp_width_txt,	"Op%cFilters.txw",  HELP_XLAT_1	},
    { &widgets.options.copyrt_btn,	"Op%cFlags.btn",    HELP_XLAT_1	},
    { &widgets.options.orig_btn,	"Op%cFlags.btn",    HELP_XLAT_1	},
    { &widgets.options.nores_btn,	"Op%cFlags.btn",    HELP_XLAT_1	},
    { &widgets.options.chksum_btn,	"Op%cFlags.btn",    HELP_XLAT_1	},
    { &widgets.options.iso_btn,		"Op%cFlags.btn",    HELP_XLAT_1	},
    { &widgets.options.addtag_btn,	"Op%cAddTag.btn",   HELP_XLAT_1	},
    { &widgets.options.id3tag_ver_opt,	"Op%cAddTag.btn",   HELP_XLAT_1	},
    { &widgets.options.lame_radbox,	"Op%cLameOpts.txw", HELP_XLAT_1	},
    { &widgets.options.lame_opts_txt,	"Op%cLameOpts.txw", HELP_XLAT_1	},
    { &widgets.options.sched_rd_radbox,	"Op%cSched.rbx",    HELP_XLAT_1	},
    { &widgets.options.sched_wr_radbox,	"Op%cSched.rbx",    HELP_XLAT_1	},
    { &widgets.options.hb_tout_txt,	"Op%cHbeatTo.txw",  HELP_XLAT_1	},
    { &widgets.options.load_chkbox,	"Op%cLoad.cbx",	    HELP_XLAT_1	},
    { &widgets.options.exit_radbox,	"Op%cExit.rbx",	    HELP_XLAT_1	},
    { &widgets.options.done_chkbox,	"Op%cDone.cbx",	    HELP_XLAT_1	},
    { &widgets.options.eject_chkbox,	"Op%cEject.cbx",    HELP_XLAT_1	},
    { &widgets.options.chg_chkbox, 	"Op%cChgr.cbx",	    HELP_XLAT_1	},
    { &widgets.options.chroute_radbox,	"Op%cChRt.rbx",	    HELP_XLAT_1	},
    { &widgets.options.outport_chkbox,	"Op%cOutPort.cbx",  HELP_XLAT_1	},
    { &widgets.options.vol_radbox, 	"Op%cVolTpr.rbx",   HELP_XLAT_1	},
    { &widgets.options.bal_lbl, 	"Op%cBal.scl",	    HELP_XLAT_1	},
    { &widgets.options.bal_scale, 	"Op%cBal.scl",	    HELP_XLAT_1	},
    { &widgets.options.ball_lbl, 	"Op%cBal.scl",	    HELP_XLAT_1	},
    { &widgets.options.balr_lbl, 	"Op%cBal.scl",	    HELP_XLAT_1	},
    { &widgets.options.balctr_btn, 	"Op%cBalCtr.btn",   HELP_XLAT_1	},
    { &widgets.options.cdda_att_lbl, 	"Op%cCddaAtt.scl",  HELP_XLAT_1	},
    { &widgets.options.cdda_att_scale, 	"Op%cCddaAtt.scl",  HELP_XLAT_1	},
    { &widgets.options.cdda_fadeout_btn,"Op%cCddaAtt.scl",  HELP_XLAT_1	},
    { &widgets.options.cdda_fadein_btn,	"Op%cCddaAtt.scl",  HELP_XLAT_1	},
    { &widgets.options.cdda_fadeout_lbl,"Op%cCddaAtt.scl",  HELP_XLAT_1	},
    { &widgets.options.cdda_fadein_lbl,	"Op%cCddaAtt.scl",  HELP_XLAT_1	},
    { &widgets.options.mbrowser_radbox,	"Op%cMbrowser.rbx", HELP_XLAT_1	},
    { &widgets.options.lookup_radbox,	"Op%cLookup.rbx",   HELP_XLAT_1	},
    { &widgets.options.serv_tout_txt,	"Op%cServTo.txw",   HELP_XLAT_1	},
    { &widgets.options.cache_tout_txt,	"Op%cCacheTo.txw",  HELP_XLAT_1	},
    { &widgets.options.use_proxy_btn,	"Op%cProxy.btn",    HELP_XLAT_1	},
    { &widgets.options.proxy_srvr_txt,	"Op%cProxy.btn",    HELP_XLAT_1	},
    { &widgets.options.proxy_port_txt,	"Op%cProxy.btn",    HELP_XLAT_1	},
    { &widgets.options.proxy_auth_btn,	"Op%cProxy.btn",    HELP_XLAT_1	},
    { &widgets.options.reset_btn, 	"Op%cReset.btn",    HELP_XLAT_1	},
    { &widgets.options.save_btn, 	"Op%cSave.btn",	    HELP_XLAT_1	},
    { &widgets.options.ok_btn,	 	"Op%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.perfmon.speed_lbl, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.speed_ind, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.frames_lbl, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.frames_ind, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.fps_lbl, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.fps_ind, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.ttc_lbl, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.ttc_ind, 	"Pm%cPerfMon.lbl",  HELP_XLAT_1	},
    { &widgets.perfmon.cancel_btn, 	"Pm%cCancel.btn",   HELP_XLAT_1	},
    { &widgets.dbprog.tottime_ind, 	"Dp%cTotTim.lbl",   HELP_XLAT_1	},
    { &widgets.dbprog.inetoffln_btn, 	"Dp%cOffline.btn",  HELP_XLAT_1	},
    { &widgets.dbprog.dlist_btn, 	"Dp%cDList.btn",    HELP_XLAT_1	},
    { &widgets.dbprog.artist_txt, 	"Dp%cArtist.txw",   HELP_XLAT_1	},
    { &widgets.dbprog.title_txt, 	"Dp%cTitle.txw",    HELP_XLAT_1	},
    { &widgets.dbprog.fullname_btn, 	"Dp%cFName.btn",    HELP_XLAT_1	},
    { &widgets.dbprog.extd_btn, 	"Dp%cDExt.btn",	    HELP_XLAT_1	},
    { &widgets.dbprog.dcredits_btn, 	"Dp%cDCredits.btn", HELP_XLAT_1	},
    { &widgets.dbprog.segments_btn, 	"Dp%cSegments.btn", HELP_XLAT_1	},
    { &widgets.dbprog.trk_list, 	"Dp%cTrk.lsw",	    HELP_XLAT_2	},
    { &widgets.dbprog.addpgm_btn, 	"Dp%cAddPgm.btn",   HELP_XLAT_1	},
    { &widgets.dbprog.clrpgm_btn, 	"Dp%cClrPgm.btn",   HELP_XLAT_1	},
    { &widgets.dbprog.savepgm_btn, 	"Dp%cSavePgm.btn",  HELP_XLAT_1	},
    { &widgets.dbprog.radio_box, 	"Dp%cTimSel.btn",   HELP_XLAT_1	},
    { &widgets.dbprog.ttitle_txt, 	"Dp%cTTitle.txw",   HELP_XLAT_1	},
    { &widgets.dbprog.apply_btn, 	"Dp%cApply.btn",    HELP_XLAT_1	},
    { &widgets.dbprog.extt_btn, 	"Dp%cTExt.btn",	    HELP_XLAT_1	},
    { &widgets.dbprog.tcredits_btn, 	"Dp%cTCredits.btn", HELP_XLAT_1	},
    { &widgets.dbprog.pgmseq_txt, 	"Dp%cPgmSeq.txw",   HELP_XLAT_1	},
    { &widgets.dbprog.userreg_btn, 	"Dp%cUserReg.btn",  HELP_XLAT_1	},
    { &widgets.dbprog.submit_btn, 	"Dp%cSubmit.btn",   HELP_XLAT_1	},
    { &widgets.dbprog.flush_btn, 	"Dp%cFlush.btn",    HELP_XLAT_1	},
    { &widgets.dbprog.reload_btn, 	"Dp%cReload.btn",   HELP_XLAT_1	},
    { &widgets.dbprog.ok_btn, 		"Dp%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.dlist.type_opt, 		"Dl%cType.opt",	    HELP_XLAT_1	},
    { &widgets.dlist.disc_list, 	"Dl%cDisc.lsw",	    HELP_XLAT_2	},
    { &widgets.dlist.show_btn, 		"Dl%cShow.btn",	    HELP_XLAT_1	},
    { &widgets.dlist.goto_btn, 		"Dl%cChgTo.btn",    HELP_XLAT_1	},
    { &widgets.dlist.del_btn, 		"Dl%cDel.btn",	    HELP_XLAT_1	},
    { &widgets.dlist.delall_btn, 	"Dl%cDelAll.btn",   HELP_XLAT_1	},
    { &widgets.dlist.rescan_btn, 	"Dl%cReScan.btn",   HELP_XLAT_1	},
    { &widgets.dlist.cancel_btn, 	"Dl%cCancel.btn",   HELP_XLAT_1	},
    { &widgets.fullname.dispname_txt, 	"Df%cName.txw",	    HELP_XLAT_1	},
    { &widgets.fullname.autogen_btn, 	"Df%cName.txw",	    HELP_XLAT_1	},
    { &widgets.fullname.lastname_txt, 	"Df%cName.txw",	    HELP_XLAT_1	},
    { &widgets.fullname.firstname_txt, 	"Df%cName.txw",	    HELP_XLAT_1	},
    { &widgets.fullname.the_btn,	"Df%cName.txw",	    HELP_XLAT_1	},
    { &widgets.fullname.the_txt, 	"Df%cName.txw",	    HELP_XLAT_1	},
    { &widgets.fullname.ok_btn,	 	"Df%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.dbextd.sorttitle_txt, 	"Dd%cSortTt.txw",   HELP_XLAT_1	},
    { &widgets.dbextd.the_btn, 		"Dd%cSortTt.txw",   HELP_XLAT_1	},
    { &widgets.dbextd.the_txt, 		"Dd%cSortTt.txw",   HELP_XLAT_1	},
    { &widgets.dbextd.year_txt, 	"Dd%cYear.txw",	    HELP_XLAT_1	},
    { &widgets.dbextd.label_txt, 	"Dd%cLabel.txw",    HELP_XLAT_1	},
    { &widgets.dbextd.genre_opt[0], 	"Dd%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextd.genre_opt[1], 	"Dd%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextd.subgenre_opt[0], 	"Dd%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextd.subgenre_opt[1], 	"Dd%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextd.region_lbl, 	"Dd%cRegion.txw",   HELP_XLAT_1	},
    { &widgets.dbextd.region_txt, 	"Dd%cRegion.txw",   HELP_XLAT_1	},
    { &widgets.dbextd.region_chg_btn, 	"Dd%cRegion.txw",   HELP_XLAT_1	},
    { &widgets.dbextd.lang_lbl, 	"Dd%cLang.txw",     HELP_XLAT_1	},
    { &widgets.dbextd.lang_txt, 	"Dd%cLang.txw",     HELP_XLAT_1	},
    { &widgets.dbextd.lang_chg_btn, 	"Dd%cLang.txw",     HELP_XLAT_1	},
    { &widgets.dbextd.dnum_txt, 	"Dd%cDnum.txw",	    HELP_XLAT_1	},
    { &widgets.dbextd.tnum_txt, 	"Dd%cTnum.txw",	    HELP_XLAT_1	},
    { &widgets.dbextd.comp_btn, 	"Dd%cComp.btn",	    HELP_XLAT_1	},
    { &widgets.dbextd.notes_txt,	"Dd%cNotes.txw",    HELP_XLAT_1	},
    { &widgets.dbextd.discid_lbl,	"Dd%cInfo.lbl",	    HELP_XLAT_1	},
    { &widgets.dbextd.discid_ind,	"Dd%cInfo.lbl",	    HELP_XLAT_1	},
    { &widgets.dbextd.rev_lbl,		"Dd%cInfo.lbl",	    HELP_XLAT_1	},
    { &widgets.dbextd.rev_ind,		"Dd%cInfo.lbl",	    HELP_XLAT_1	},
    { &widgets.dbextd.cert_lbl,		"Dd%cInfo.lbl",	    HELP_XLAT_1	},
    { &widgets.dbextd.cert_ind,		"Dd%cInfo.lbl",	    HELP_XLAT_1	},
    { &widgets.dbextd.ok_btn,	 	"Dd%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.dbextt.prev_btn, 	"Dt%cDir.btn",	    HELP_XLAT_1	},
    { &widgets.dbextt.next_btn, 	"Dt%cDir.btn",	    HELP_XLAT_1	},
    { &widgets.dbextt.autotrk_btn, 	"Dt%cAutoTr.btn",   HELP_XLAT_1	},
    { &widgets.dbextt.sorttitle_txt,	"Dt%cSortTt.txw",   HELP_XLAT_1	},
    { &widgets.dbextt.the_btn,		"Dt%cSortTt.txw",   HELP_XLAT_1	},
    { &widgets.dbextt.the_txt,		"Dt%cSortTt.txw",   HELP_XLAT_1	},
    { &widgets.dbextt.artist_txt,	"Dt%cArtist.txw",   HELP_XLAT_1	},
    { &widgets.dbextt.fullname_btn,	"Dt%cFName.btn",    HELP_XLAT_1	},
    { &widgets.dbextt.year_txt,		"Dt%cYear.txw",	    HELP_XLAT_1	},
    { &widgets.dbextt.label_txt,	"Dt%cLabel.txw",    HELP_XLAT_1	},
    { &widgets.dbextt.bpm_txt,		"Dt%cBPM.txw",	    HELP_XLAT_1	},
    { &widgets.dbextt.genre_opt[0], 	"Dt%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextt.genre_opt[1], 	"Dt%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextt.subgenre_opt[0], 	"Dt%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextt.subgenre_opt[1], 	"Dt%cGenre.opt",    HELP_XLAT_1	},
    { &widgets.dbextt.notes_txt,	"Dt%cNotes.txw",    HELP_XLAT_1	},
    { &widgets.dbextt.isrc_lbl,		"Dt%cISRC.lbl",	    HELP_XLAT_1	},
    { &widgets.dbextt.ok_btn, 		"Dt%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.credits.autotrk_btn, 	"Cr%cAutoTr.btn",   HELP_XLAT_1	},
    { &widgets.credits.prev_btn, 	"Cr%cDir.btn",	    HELP_XLAT_1	},
    { &widgets.credits.next_btn, 	"Cr%cDir.btn",	    HELP_XLAT_1	},
    { &widgets.credits.cred_list, 	"Cr%cCredit.lsw",   HELP_XLAT_2	},
    { &widgets.credits.prirole_opt, 	"Cr%cPriRole.opt",  HELP_XLAT_1	},
    { &widgets.credits.subrole_opt, 	"Cr%cSubRole.opt",  HELP_XLAT_1	},
    { &widgets.credits.name_txt, 	"Cr%cName.txw",	    HELP_XLAT_1	},
    { &widgets.credits.fullname_btn, 	"Cr%cFName.btn",    HELP_XLAT_1	},
    { &widgets.credits.notes_txt, 	"Cr%cNotes.txw",    HELP_XLAT_1	},
    { &widgets.credits.add_btn, 	"Cr%cAdd.btn",	    HELP_XLAT_1	},
    { &widgets.credits.mod_btn, 	"Cr%cModify.btn",   HELP_XLAT_1	},
    { &widgets.credits.del_btn, 	"Cr%cDelete.btn",   HELP_XLAT_1	},
    { &widgets.credits.ok_btn, 		"Cr%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.segments.seg_list, 	"Sg%cSegment.lsw",  HELP_XLAT_2	},
    { &widgets.segments.name_txt, 	"Sg%cFields.txw",   HELP_XLAT_1	},
    { &widgets.segments.starttrk_txt, 	"Sg%cFields.txw",   HELP_XLAT_1	},
    { &widgets.segments.startfrm_txt, 	"Sg%cFields.txw",   HELP_XLAT_1	},
    { &widgets.segments.endtrk_txt, 	"Sg%cFields.txw",   HELP_XLAT_1	},
    { &widgets.segments.endfrm_txt, 	"Sg%cFields.txw",   HELP_XLAT_1	},
    { &widgets.segments.set_btn, 	"Sg%cSet.btn",	    HELP_XLAT_1	},
    { &widgets.segments.playpaus_btn, 	"Sg%cPlayPaus.btn", HELP_XLAT_1	},
    { &widgets.segments.stop_btn, 	"Sg%cStop.btn",	    HELP_XLAT_1	},
    { &widgets.segments.notes_txt, 	"Sg%cNotes.txw",    HELP_XLAT_1	},
    { &widgets.segments.credits_btn, 	"Sg%cCredits.btn",  HELP_XLAT_1	},
    { &widgets.segments.add_btn, 	"Sg%cAdd.btn",	    HELP_XLAT_1	},
    { &widgets.segments.mod_btn, 	"Sg%cModify.btn",   HELP_XLAT_1	},
    { &widgets.segments.del_btn, 	"Sg%cDelete.btn",   HELP_XLAT_1	},
    { &widgets.segments.ok_btn, 	"Sg%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.submiturl.categ_txt, 	"Su%cSubmURL.txw",  HELP_XLAT_1	},
    { &widgets.submiturl.name_txt, 	"Su%cSubmURL.txw",  HELP_XLAT_1	},
    { &widgets.submiturl.url_txt, 	"Su%cSubmURL.txw",  HELP_XLAT_1	},
    { &widgets.submiturl.desc_txt, 	"Su%cSubmURL.txw",  HELP_XLAT_1	},
    { &widgets.submiturl.submit_btn, 	"Su%cSubmit.btn",   HELP_XLAT_1	},
    { &widgets.submiturl.ok_btn, 	"Su%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.regionsel.region_list, 	"Rs%cRegion.lsw",   HELP_XLAT_2	},
    { &widgets.regionsel.ok_btn, 	"Rs%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.langsel.lang_list, 	"Ls%cLang.lsw",     HELP_XLAT_2	},
    { &widgets.langsel.ok_btn,		"Ls%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.matchsel.matchsel_list, 	"Ms%cMatch.lsw",    HELP_XLAT_2	},
    { &widgets.matchsel.ok_btn, 	"Ms%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.userreg.handle_txt, 	"Ur%cHandle.txw",   HELP_XLAT_1	},
    { &widgets.userreg.passwd_txt, 	"Ur%cPasswd.txw",   HELP_XLAT_1	},
    { &widgets.userreg.vpasswd_txt, 	"Ur%cVpaswd.txw",   HELP_XLAT_1	},
    { &widgets.userreg.hint_txt, 	"Ur%cHint.txw",	    HELP_XLAT_1	},
    { &widgets.userreg.gethint_btn, 	"Ur%cHint.btn",	    HELP_XLAT_1	},
    { &widgets.userreg.email_txt, 	"Ur%cEmail.txw",    HELP_XLAT_1	},
    { &widgets.userreg.region_txt, 	"Ur%cRegion.txw",   HELP_XLAT_1	},
    { &widgets.userreg.region_chg_btn, 	"Ur%cRegion.txw",   HELP_XLAT_1	},
    { &widgets.userreg.postal_txt, 	"Ur%cPostal.txw",   HELP_XLAT_1	},
    { &widgets.userreg.age_txt, 	"Ur%cAge.txw",	    HELP_XLAT_1	},
    { &widgets.userreg.gender_radbox, 	"Ur%cGender.rbx",   HELP_XLAT_1	},
    { &widgets.userreg.allowmail_btn, 	"Ur%cAllowM.btn",   HELP_XLAT_1	},
    { &widgets.userreg.allowstats_btn, 	"Ur%cAllowS.btn",   HELP_XLAT_1	},
    { &widgets.userreg.ok_btn, 		"Ur%cOk.btn",	    HELP_XLAT_1	},
    { &widgets.userreg.priv_btn, 	"Ur%cPrivacy.btn",  HELP_XLAT_1	},
    { &widgets.userreg.cancel_btn,	"Ur%cCancel.btn",   HELP_XLAT_1	},
    { &widgets.help.topic_opt, 		"Hp%cTopic.opt",    HELP_XLAT_1	},
    { &widgets.help.help_txt, 		"Hp%cText.txw",	    HELP_XLAT_1	},
    { &widgets.help.about_btn, 		"Hp%cAbout.btn",    HELP_XLAT_1	},
    { &widgets.help.cancel_btn, 	"Hp%cCancel.btn",   HELP_XLAT_1	},
    { NULL, 				NULL,		    0		},
};

STATIC doc_topic_t	*dochead = NULL,
			*doctail = NULL;


/***********************
 *  internal routines  *
 ***********************/

/*
 * help_docinit
 *	Initialize the documentation topics list.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
help_docinit(void)
{
	int		i;
	Arg		arg[10];
	DIR		*dp;
	struct dirent	*de;
	doc_topic_t	*p;
	struct stat	stbuf;
	char		docdir[FILE_PATH_SZ],
			fname[FILE_PATH_SZ];

	if (app_data.libdir == NULL)
		/* shrug */
		return;

	(void) sprintf(docdir, DOCFILE_PATH, app_data.libdir);
	if ((dp = OPENDIR(docdir)) == NULL) {
		DBGPRN(DBG_GEN)(errfp,
			"Cannot open %s, no topics added.\n", docdir);
		return;
	}

	while ((de = READDIR(dp)) != NULL) {
		/* Skip ".", ".." and dot files */
		if (de->d_name[0] == '.')
			continue;

		(void) sprintf(fname, "%s%s", docdir, de->d_name);
		if (stat(fname, &stbuf) < 0)
			continue;

		/* Skip non-regular files */
		if (!S_ISREG(stbuf.st_mode))
			continue;

		p = (doc_topic_t *) MEM_ALLOC(
			"doc_topic_t",
			sizeof(doc_topic_t)
		);
		if (p == NULL) {
			(void) CLOSEDIR(dp);
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		(void) memset(p, 0, sizeof(doc_topic_t));

		if (!util_newstr(&p->name, de->d_name)) {
			(void) CLOSEDIR(dp);
			CD_FATAL(app_data.str_nomemory);
			return;
		}
		if (!util_newstr(&p->path, fname)) {
			(void) CLOSEDIR(dp);
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		if (dochead == NULL)
			dochead = doctail = p;
		else if (strcmp(p->name, "README") == 0) {
			/* Put README file on top */
			p->next = dochead;
			dochead = p;
		}
		else {
			doctail->next = p;
			doctail = p;
		}
	}

	for (p = dochead; p != NULL; p = p->next) {
		if (p == dochead) {
			i = 0;
			XtSetArg(arg[i], XmNseparatorType,
				 XmSINGLE_DASHED_LINE); i++;
			widgets.help.topic_sep2 = XmCreateSeparator(
				widgets.help.topic_menu,
				"topicSeparator2",
				arg,
				i
			);
			XtManageChild(widgets.help.topic_sep2);
		}

		i = 0;
		XtSetArg(arg[i], XmNshadowThickness, 2); i++;
		p->actbtn = XmCreatePushButton(
			widgets.help.topic_menu,
			p->name,
			arg,
			i
		);
		XtManageChild(p->actbtn);

		register_activate_cb(p->actbtn, help_topic_sel, p);
	}

	(void) CLOSEDIR(dp);
}


/*
 * help_getname
 *	Given a widget, return the associated help file name.
 *
 * Args:
 *	w - The widget
 *
 * Return:
 *	The help file name text string.
 */
STATIC char *
help_getname(Widget w)
{
	int	i;

	for (i = 0; wname[i].widgetp != NULL; i++) {
		if (w == *(wname[i].widgetp))
			return (wname[i].hlpname);
	}
	return NULL;
}


/***********************
 *   public routines   *
 ***********************/


/*
 * help_init
 *	Top level function to set up the help subsystem.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
void
help_init(void)
{
	int		i;
	XtTranslations	xtab1,
			xtab2;

	xtab1 = XtParseTranslationTable(
		"<Btn3Down>,<Btn3Up>: Help()\n"
	);
	xtab2 = XtParseTranslationTable(
		"<Btn3Down>,<Btn3Up>: PrimitiveHelp()\n"
	);

	/* Set up translations */
	for (i = 0; wname[i].widgetp != NULL; i++) {
		XtOverrideTranslations(
			*(wname[i].widgetp),
			(wname[i].xlat_typ == HELP_XLAT_1) ? xtab1 : xtab2
		);
	}

	/* Initialize documentation topics list */
	help_docinit();
}


/*
 * help_start
 *	Start up xmcd help system.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
help_start(void)
{
#ifndef __VMS
	int		ret;
	pid_t		cpid;
	waitret_t	wstat;
	char		*dirpath,
			vfile[16],
			path[FILE_PATH_SZ + 16];
	struct stat	stbuf;

	/* Popup the help window if this version of xmcd is being run
	 * by the user for the first time.
	 */

	dirpath = util_homedir(util_get_ouid());
	if (dirpath == NULL) {
		/* No home directory: shrug */
		return;
	}
	if ((int) strlen(dirpath) >= FILE_PATH_SZ) {
		CD_FATAL(app_data.str_longpatherr);
		return;
	}
	(void) sprintf(vfile, "%s-%s.%s", PROGNAME, VERSION_MAJ, VERSION_MIN);
	(void) sprintf(path, USR_VINIT_PATH, dirpath, vfile);

	switch (cpid = FORK()) {
	case 0:
		/* Child process */

		/* Force uid and gid to original setting */
		if (!util_set_ougid())
			_exit(1);

		if (stat(path, &stbuf) == 0)
			_exit(0);

		/* Create version file */
		if (creat(path, 0666) < 0) {
			DBGPRN(DBG_GEN)(errfp,
				"help_setup: cannot creat %s.\n", path);
			_exit(3);
		}

		_exit(4);
		/*NOTREACHED*/

	case -1:
		/* fork failed */
		break;

	default:
		/* Parent: wait for child to finish */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     NULL, 0, FALSE, &wstat);
		if (ret == cpid && WIFEXITED(wstat) && WEXITSTATUS(wstat) != 0)
			/* Pop up help */
			help_popup(NULL);

		break;
	}
#endif
}


/*
 * help_loadfile
 *	Pop up the help window and display text from the specified file.
 *
 * Args:
 *	path - The file path
 *
 * Return:
 *	Nothing.
 */
void
help_loadfile(char *path)
{
	int		bufsz = STR_BUF_SZ * 2;
	char		*tmpbuf,
			*helptext;
	FILE		*fp;
	static bool_t	first = TRUE;
#ifndef __VMS
	pid_t		cpid;
	waitret_t	wstat;
	int		ret,
			pfd[2];

	if (PIPE(pfd) < 0) {
		DBGPRN(DBG_GEN)(errfp,
			"help_loadfile: pipe failed (errno=%d)\n", errno);
		cd_beep();
		return;
	}

	switch (cpid = FORK()) {
	case 0:
		/* Child */

		(void) util_signal(SIGPIPE, SIG_IGN);
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) close(pfd[1]);
			_exit(1);
		}

		DBGPRN(DBG_GEN)(errfp, "Help: loading %s\n", path);

		if ((fp = fopen(path, "r")) == NULL)
			DBGPRN(DBG_GEN)(errfp, "Cannot open %s\n", path);

		/* Allocate temporary buffer */
		tmpbuf = (char *) MEM_ALLOC("help_loadfile_tmpbuf", bufsz);
		if (tmpbuf == NULL) {
			(void) close(pfd[1]);
			_exit(2);
		}
		
		if (fp != NULL) {
			while (fgets(tmpbuf, bufsz, fp) != NULL)
				(void) write(pfd[1], tmpbuf, strlen(tmpbuf));

			(void) fclose(fp);
		}
		else {
			(void) sprintf(tmpbuf, "[ %s ]\n%s\n",
				       path, app_data.str_nohelp);
			(void) write(pfd[1], tmpbuf, strlen(tmpbuf));
		}
		(void) write(pfd[1], "__hElP_eNd__\n", 13);
		(void) close(pfd[1]);

		MEM_FREE(tmpbuf);

		_exit(0);
		/*NOTREACHED*/

	case -1:
		DBGPRN(DBG_GEN)(errfp,
			"help_loadfile: fork failed (errno=%d)\n", errno);
		(void) close(pfd[0]);
		(void) close(pfd[1]);

		cd_beep();
		return;

	default:
		/* Parent */

		(void) close(pfd[1]);

		if ((fp = fdopen(pfd[0], "r")) == NULL) {
			DBGPRN(DBG_GEN)(errfp,
				"help_loadfile: read pipe fdopen failed\n");
			cd_beep();
			return;
		}
		break;
	}
#else
	DBGPRN(DBG_GEN)(errfp, "Help: loading %s\n", path);

	if ((fp = fopen(path, "r")) == NULL)
		DBGPRN(DBG_GEN)(errfp, "Cannot open %s\n", path);
#endif	/* __VMS */

	helptext = NULL;

	if (fp == NULL) {
		if (!util_newstr(&helptext, app_data.str_nohelp)) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}
	}
	else {
		/* Allocate temporary buffer */
		tmpbuf = (char *) MEM_ALLOC("help_tmpbuf", bufsz);
		if (tmpbuf == NULL) {
			CD_FATAL(app_data.str_nomemory);
			return;
		}

		while (fgets(tmpbuf, bufsz, fp) != NULL) {
			if (strcmp(tmpbuf, "__hElP_eNd__\n") == 0)
				/* Done */
				break;
			if (tmpbuf[0] == '#')
				/* Comment */
				continue;
			if (strncmp(tmpbuf, "@(#)", 4) == 0)
				/* SCCS ident */
				continue;

			if (helptext == NULL) {
				helptext = (char *) MEM_ALLOC(
					"helptext",
					strlen(tmpbuf) + 1
				);

				if (helptext != NULL)
					*helptext = '\0';
			}
			else {
				helptext = (char *) MEM_REALLOC(
					"helptext",
					helptext,
					strlen(helptext) + strlen(tmpbuf) + 1
				);
			}

			if (helptext == NULL) {
				CD_FATAL(app_data.str_nomemory);
				(void) fclose(fp);
				MEM_FREE(tmpbuf);
				return;
			}

			(void) strcat(helptext, tmpbuf);
		}

		(void) fclose(fp);
		MEM_FREE(tmpbuf);
	}

#ifndef __VMS
	/* Wait for child to finish */
	ret = util_waitchild(cpid, app_data.srv_timeout + 5,
			     NULL, 0, FALSE, &wstat);
	if (ret < 0) {
		MEM_FREE(helptext);
		return;
	}
#endif

	/* Set new help text */
	set_text_string(widgets.help.help_txt, helptext, FALSE);
	MEM_FREE(helptext);

	if (!XtIsManaged(widgets.help.form)) {
		/* Pop up help window.
		 * The dialog has mappedWhenManaged set to False,
		 * so we have to map/unmap explicitly.  The reason for this
		 * is we want to avoid a screen glitch when we move the window
		 * in cd_dialog_setpos(), so we map the window afterwards.
		 */
		XtManageChild(widgets.help.form);
		if (first) {
			first = FALSE;
			/* Set window position */
			cd_dialog_setpos(XtParent(widgets.help.form));
		}
		XtMapWidget(XtParent(widgets.help.form));
	}
	else {
		/* Raise the window to the top of stack */
		XRaiseWindow(
			XtDisplay(widgets.help.form),
			XtWindow(XtParent(widgets.help.form))
		);
	}

	/* Set keyboard focus on the Cancel button */
	XmProcessTraversal(
		widgets.help.cancel_btn,
		XmTRAVERSE_CURRENT
	);
}


/*
 * help_popup
 *	Pop up the help window and display help text based on the
 *	specified widget.
 *
 * Args:
 *	w - The widget which the help info is being displayed about.
 *
 * Return:
 *	Nothing.
 */
void
help_popup(Widget w)
{
	char	hlpfile[FILE_PATH_SZ * 2],
		*str,
		*cp;

	if (w == NULL || (cp = help_getname(w)) == NULL)
		/* Generic help */
		cp = "Xm%cHelp.btn";

	if ((str = (char *) MEM_ALLOC("str", strlen(cp))) == NULL) {
		CD_FATAL(app_data.str_nomemory);
		return;
	}
	(void) sprintf(str, cp, DIR_END);
	(void) sprintf(hlpfile, HELPFILE_PATH, app_data.libdir, str);

	MEM_FREE(str);

	XtVaSetValues(widgets.help.topic_opt,
		XmNmenuHistory, widgets.help.online_btn,
		NULL
	);

	/* Change to watch cursor */
	cd_busycurs(TRUE, CURS_HELP);

	/* Load the help file */
	help_loadfile(hlpfile);

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_HELP);
}


/*
 * help_popdown
 *	Pop down the help window.
 *
 * Args:
 *	None
 *
 * Return:
 *	Nothing.
 */
void
help_popdown(void)
{
	if (XtIsManaged(widgets.help.form)) {
		XtUnmapWidget(XtParent(widgets.help.form));
		XtUnmanageChild(widgets.help.form);
	}
}


/* help_isactive
 *	Check if the help window is currently popped up.
 *
 * Args:
 *	None
 *
 * Return:
 *	TRUE: help currently popped up
 *	FALSE: help not currently popped up
 */
bool_t
help_isactive(void)
{
	return ((bool_t) XtIsManaged(widgets.help.form));
}


/**************** vv Callback routines vv ****************/

/*
 * help_topic_sel
 *	Help topic selector callback
 */
/*ARGSUSED*/
void
help_topic_sel(Widget w, XtPointer client_data, XtPointer call_data)
{
	doc_topic_t	*p = (doc_topic_t *)(void *) client_data;

	/* Change to watch cursor */
	cd_busycurs(TRUE, CURS_HELP);

	if (p == NULL) {
		/* Display generic help text */
		help_popup(NULL);
	}
	else if (p->actbtn == w) {
		/* Display appropriate doc file */
		help_loadfile(p->path);
	}

	/* Change to normal cursor */
	cd_busycurs(FALSE, CURS_HELP);

}


