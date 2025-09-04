$! makevms.com, generated from descrip.mms	1.8 03/12/12, at  04/04/20 20:18:07
$ set def [.common_d]
$ cc /stand=rel/extern=strict/include=("../")	/def=(_POSIX_C_SOURCE,has_pthreads) UTIL.C
$ If "''F$Search("LIBUTIL.OLB")'" .EQS. "" Then LIBRARY/Create LIBUTIL.OLB
$ LIBRARY/REPLACE LIBUTIL.OLB UTIL.OBJ
$ ! done 
$ set def [-.cdda_d]
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) CDDA.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB CDDA.OBJ
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) COMMON.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB COMMON.OBJ
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) PTHR.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB PTHR.OBJ
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) RD_SCSI.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB RD_SCSI.OBJ
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) WR_FP.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB WR_FP.OBJ
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) WR_GEN.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB WR_GEN.OBJ
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) WR_OSF1.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB WR_OSF1.OBJ
$ cc /stand=rel/extern=strict/include=("../","mme")	/def=(_POSIX_C_SOURCE=2,has_mme,has_lame,has_pthreads) IF_LAME.C
$ If "''F$Search("LIBCDDA.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDA.OLB
$ LIBRARY/REPLACE LIBCDDA.OLB IF_LAME.OBJ
$ ! done
$ set def [-.libdi_d]
$ cc /stand=rel/extern=strict/include=("../") LIBDI.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB LIBDI.OBJ
$ cc /stand=rel/extern=strict/include=("../") SCSIPT.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB SCSIPT.OBJ
$ cc /stand=rel/extern=strict/include=("../") OS_VMS.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB OS_VMS.OBJ
$ cc /stand=rel/extern=strict/include=("../") VU_CHIN.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB VU_CHIN.OBJ
$ cc /stand=rel/extern=strict/include=("../") VU_HITA.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB VU_HITA.OBJ
$ cc /stand=rel/extern=strict/include=("../") VU_NEC.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB VU_NEC.OBJ
$ cc /stand=rel/extern=strict/include=("../") VU_PANA.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB VU_PANA.OBJ
$ cc /stand=rel/extern=strict/include=("../") VU_PION.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB VU_PION.OBJ
$ cc /stand=rel/extern=strict/include=("../") VU_SONY.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB VU_SONY.OBJ
$ cc /stand=rel/extern=strict/include=("../") VU_TOSH.C
$ If "''F$Search("LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create LIBDI.OLB
$ LIBRARY/REPLACE LIBDI.OLB VU_TOSH.OBJ
$ ! done
$ set def [-.cddb_d]
$ cc /stand=rel/extern=strict/include=("../")/names=shortened CDDBKEY1.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB CDDBKEY1.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened CONTROL.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB CONTROL.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened CREDIT.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB CREDIT.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened DISC.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB DISC.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened DISCS.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB DISCS.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened FCDDB.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB FCDDB.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened FULLNAME.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB FULLNAME.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened GEN.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB GEN.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened GENRE.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB GENRE.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened GENRELIST.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB GENRELIST.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened GENRETREE.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB GENRETREE.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened LANGLIST.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB LANGLIST.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened LANGUAGE.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB LANGUAGE.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened OPTIONS.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB OPTIONS.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened REGION.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB REGION.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened REGIONLIST.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB REGIONLIST.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened ROLE.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB ROLE.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened ROLELIST.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB ROLELIST.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened ROLETREE.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB ROLETREE.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened SEGMENT.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB SEGMENT.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened TRACK.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB TRACK.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened URL.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB URL.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened URLLIST.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB URLLIST.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened URLMANAGER.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB URLMANAGER.OBJ
$ cc /stand=rel/extern=strict/include=("../")/names=shortened USERINFO.C
$ If "''F$Search("LIBCDDB.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDDB.OLB
$ LIBRARY/REPLACE LIBCDDB.OLB USERINFO.OBJ
$ ! done
$ set def [-.cdinfo_d]
$ cc /stand=rel/extern=strict/include=("../")/name=shortened CDINFO_I.C
$ If "''F$Search("LIBCDINFO.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDINFO.OLB
$ LIBRARY/REPLACE LIBCDINFO.OLB CDINFO_I.OBJ
$ cc /stand=rel/extern=strict/include=("../")/name=shortened CDINFO_X.C
$ If "''F$Search("LIBCDINFO.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDINFO.OLB
$ LIBRARY/REPLACE LIBCDINFO.OLB CDINFO_X.OBJ
$ cc /stand=rel/extern=strict/include=("../")/name=shortened HIST.C
$ If "''F$Search("LIBCDINFO.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDINFO.OLB
$ LIBRARY/REPLACE LIBCDINFO.OLB HIST.OBJ
$ cc /stand=rel/extern=strict/include=("../")/name=shortened MOTD.C
$ If "''F$Search("LIBCDINFO.OLB")'" .EQS. "" Then LIBRARY/Create LIBCDINFO.OLB
$ LIBRARY/REPLACE LIBCDINFO.OLB MOTD.OBJ
$ ! done
$ set def [-.labelh_d]
$ cc /stand=rel/extern=strict/include=("../") LABELH.C
$ If "''F$Search("LIBLABELH.OLB")'" .EQS. "" Then LIBRARY/Create LIBLABELH.OLB
$ LIBRARY/REPLACE LIBLABELH.OLB LABELH.OBJ
$ ! done
$ set def [-.xmcd_d]
$ cc /stand=rel/extern=strict/include=("../") CALLBACK.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB CALLBACK.OBJ
$ cc /stand=rel/extern=strict/include=("../") CDFUNC.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB CDFUNC.OBJ
$ cc /stand=rel/extern=strict/include=("../") COMMAND.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB COMMAND.OBJ
$ cc /stand=rel/extern=strict/include=("../") DBPROG.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB DBPROG.OBJ
$ cc /stand=rel/extern=strict/include=("../") GEOM.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB GEOM.OBJ
$ cc /stand=rel/extern=strict/include=("../") HELP.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB HELP.OBJ
$ cc /stand=rel/extern=strict/include=("../") HOTKEY.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB HOTKEY.OBJ
$ cc /stand=rel/extern=strict/include=("../") MAIN.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB MAIN.OBJ
$ cc /stand=rel/extern=strict/include=("../") USERREG.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB USERREG.OBJ
$ cc /stand=rel/extern=strict/include=("../") WIDGET.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB WIDGET.OBJ
$ cc /stand=rel/extern=strict/include=("../") WWWWARP.C
$ If "''F$Search("XMCD.OLB")'" .EQS. "" Then LIBRARY/Create XMCD.OLB
$ LIBRARY/REPLACE XMCD.OLB WWWWARP.OBJ
$ ! done
$ cc /stand=rel/extern=strict/include=("../") [-.LIBDI_D]LIBDI.C
$ If "''F$Search("[-.LIBDI_D]LIBDI.OLB")'" .EQS. "" Then LIBRARY/Create [-.LIBDI_D]LIBDI.OLB
$ LIBRARY/REPLACE [-.LIBDI_D]LIBDI.OLB [-.LIBDI_D]LIBDI.OBJ
$ open/write out XMCD.OPT
$ write out "! mmov (aka mme)
$ write out "sys$sysdevice:[sys0.syscommon.syslib]mmov.olb/lib
$ write out "sys$share:cma$open_rtl.exe/share
$ write out "! others
$ write out "sys$share:decw$xlibshr.exe/share
$ write out "sys$share:decw$xtlibshrr5.exe/share
$ write out "sys$share:decw$xmulibshrr5.exe/share
$ write out "sys$share:decw$xmlibshr12.exe/share
$ write out "sys$share:decw$bkrshr12.exe/share
$ write out "sys$share:decw$dxmlibshr12.exe/share
$ write out "sys$share:decw$mrmlibshr12.exe/share
$ write out "sys$share:ipc$share.exe/share
$ close out
$ link/exe=XMCD/map=XMCD/cross/full/threads_enable=upcalls 		[]main.obj, 		[]xmcd.olb/lib,		[-.labelh_d]liblabelh.olb/lib,		[-.cdinfo_d]libcdinfo.olb/lib,		[-.cddb_d]libcddb.olb/lib,		[-.libdi_d]libdi.olb/lib,		[-.cdda_d]libcdda.olb/lib,		[-.common_d]l-
ibutil.olb/lib,		[]xmcd/opt
$ set def [-]
