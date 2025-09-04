$!
$! @(#)installvms.com	7.31 04/04/20
$!
$! Top-level installation command procedure for OpenVMS
$! See the INSTALL.VMS file for details.
$! OpenVMS versions: 6.0 and later, tested with 7.2 and 7.3 on VAX(tm) machines
$!                   and 7.2-1 as well as 7.3-1 on Alpha(tm) machines
$!
$!      xmcd   - Motif(R) CD Audio Player/Ripper
$!
$!   Copyright (C) 1993-2004  Ti Kan
$!   E-mail: xmcd@amb.org
$!   Contributing author: Michael Monscheuer
$!   E-mail: M.Monscheuer@t-online.de
$!
$!   This program is free software; you can redistribute it and/or modify
$!   it under the terms of the GNU General Public License as published by
$!   the Free Software Foundation; either version 2 of the License, or
$!   (at your option) any later version.
$!
$!   This program is distributed in the hope that it will be useful,
$!   but WITHOUT ANY WARRANTY; without even the implied warranty of
$!   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
$!   GNU General Public License for more details.
$!
$!   You should have received a copy of the GNU General Public License
$!   along with this program; if not, write to the Free Software
$!   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
$!
$!=============================================================================
$! 
$! 
$! If xmcd.exe already exists in the xmcd_d directory and a further 
$! compilation of the sources is NOT desired, you may pass the text
$! string "install" as P1 to this procedure to only install xmcd.
$!
$! You need to specify "install" as P1 if you intend to install
$! a precompiled binary kit.
$! 
$! In case you want to only compile and link the image, pass the
$! the text string "compile" as P1 to this procedure. 
$! 
$! Passing no parameters to this procedure will result in compiling,
$! linking, and installing xmcd. P1 is not case sensitive.
$! 
$! This procedure also will generate Local Discography category 
$! index files in HTML format. Furthermore it will convert pre-v3.x
$! CD related HTML index files to the xmcd v3.x standard.
$! 
$! Running this procedure again after some failure or interruption
$! is supported. It is expected to recognize what it is meant to do.
$!
$!
$	SET NOON
$	P1 = F$EXTRACT(0,1,F$EDIT(P1,"UPCASE,COLLAPSE"))
$	IF F$TRNLNM("LMF$DEC_C","LMF$LICENSE_TABLE") .EQS. "" .AND. P1 .NES. "I"
$	 THEN 
$	  WRITE SYS$OUTPUT ""
$	  WRITE SYS$OUTPUT "  This node does not have a C compiler license loaded."
$	  WRITE SYS$OUTPUT "  Building the xmcd executable is not possible."
$	  WRITE SYS$OUTPUT ""
$	  EXIT
$	ENDIF
$	ENVI = F$ENVIRONMENT("DEFAULT")
$	USEN = F$ENVIRONMENT("DEFAULT")
$	IF F$LOCATE("<",ENVI) .NE. F$LENGTH(ENVI) THEN SET DEFAULT []
$	ENVI = F$ENVIRONMENT("DEFAULT")
$!
$! Resolve concealed logical, if necessary. 
$!
$	ENVD = F$PARSE(ENVI,,,"DEVICE") - ":"
$	IF F$TRNLNM(ENVD,,,,,"CONCEALED")
$	 THEN
$	  CCLD = F$TRNLNM(ENVD)
$	  IF F$LOCATE("[",CCLD) .EQ. F$LENGTH(CCLD)
$	   THEN
$	    ENVI = F$TRNLNM(ENVD) + F$PARSE(ENVI,,,"DIRECTORY")
$	   ELSE
$	    ENVI = F$TRNLNM(ENVD) - "]" + (F$PARSE(ENVI,,,"DIRECTORY") - "[")
$	  ENDIF
$	ENDIF
$!
$	IF F$VERIFY()
$	 THEN
$	  ESC = "<ESC>"
$	  BEL = "<BEL>"
$	  CR  = "<CR>"
$	  LF  = "<LF>"
$	 ELSE
$	  BEL[0,8]=7
$	  LF[0,8]=10
$	  CR[0,8]=13
$	  ESC[0,8]=27
$	ENDIF
$	WRITE SYS$OUTPUT ""
$	WRITE SYS$OUTPUT "  This is free software and comes with no warranty."
$	WRITE SYS$OUTPUT "  See the GNU General Public License in the COPYING file for details.''CR'''LF'"
$	WRITE SYS$OUTPUT "  This software contains support for the Gracenote CDDB(R) Disc"
$	WRITE SYS$OUTPUT "  Recognition Service.  See the CDDB file in the DOCS area for information.''CR'''LF'"
$	INQUIRE/NOPUNCTATION DUMMY "''ESC'[28C''ESC'[7mPress Return to continue''ESC'[0m"
$	WRITE SYS$OUTPUT "''ESC'[0;0H''ESC'[2J''CR'''LF'  Beginning INSTALLVMS.COM at ''F$CVTIME(,"ABSOLUTE",)'"
$	WRITE SYS$OUTPUT " ----------------------------------------------------''CR'''LF'"
$	IF P1 .EQS. "" 
$	 THEN 
$	  WRITE SYS$OUTPUT "  This procedure will first compile and link the sources."
$	  WRITE SYS$OUTPUT "  Then it will install xmcd on your system."
$	 ELSE
$	  IF P1 .EQS. "C"
$	   THEN
$	    WRITE SYS$OUTPUT "  This procedure will compile and link the sources."
$	    WRITE SYS$OUTPUT "  An installation of xmcd on your system will not be done."
$	   ELSE
$	    WRITE SYS$OUTPUT "  This procedure will install xmcd on your system."
$	    WRITE SYS$OUTPUT "  A new xmcd image file will not be created."
$	  ENDIF
$	ENDIF
$	WRITE SYS$OUTPUT ""
$	WRITE SYS$OUTPUT "  Checking environment for presence of CDDA components ..."
$!
$! Check for presence of MMOV (MultiMedia services for OpenVms, aka mme on 
$! unix) if running on an Alpha machine.
$!
$	MMOVRTL = 0
$	MMOVRTF = 0
$	MMOVDVL = 0
$	MMOVDVF = 0
$	IF F$GETSYI("ARCH_NAME") .EQS. "Alpha"
$	 THEN
$	  IF F$TRNLNM("LMF$DEC_MMOV-RT","LMF$LICENSE_TABLE") .NES. "" THEN MMOVRTL = 1
$	  IF F$SEARCH("SYS$SYSTEM:MMOV$AUDIO*.EXE")          .NES. "" THEN MMOVRTF = 1
$	  IF F$TRNLNM("LMF$DEC_MMOV-DV","LMF$LICENSE_TABLE") .NES. "" THEN MMOVDVL = 1
$	  IF F$SEARCH("SYS$COMMON:[MMOV_INCLUDES]*.*")       .NES. "" THEN MMOVDVF = 1
$	ENDIF
$!
$! Check for presence of LAME MP3 encoder
$!
$	LAME = 0
$	LAMELOCA = F$SEARCH("[000000...]LAME.EXE")
$	IF LAMELOCA .EQS. "" THEN LAMELOCA = F$SEARCH("SYS$SYSTEM:LAME.EXE")
$	IF LAMELOCA .NES. ""
$	 THEN
$	  MESENV = F$ENVIRONMENT("MESSAGE")
$	  SET MESSAGE/NOTEXT/NOSEVERITY/NOFACILITY/NOIDENTIFICATION
$	  DEFINE SYS$ERROR LAMEOUT.TMP
$	  MC 'LAMELOCA'
$	  DEASSIGN SYS$ERROR
$	  SET MESSAGE 'MESENV'
$	  IF F$SEARCH("LAMEOUT.TMP") .NES. ""
$	   THEN
$	    OPEN/READ 99 LAMEOUT.TMP
$	    READ  99 LINE
$	    CLOSE 99
$	    DELETE/NOCONFIRM LAMEOUT.TMP;*
$	    IF F$EXTRACT(0,4,LINE) .EQS. "LAME" THEN LAME = 1
$	  ENDIF
$	ENDIF
$	WRITE SYS$OUTPUT "''ESC'[2A''ESC'[0J"
$ 	
$	IF P1 .NES. "C"
$	 THEN
$	  WRITE SYS$OUTPUT "  Prerequisite for a successful completion of this installation"
$	  WRITE SYS$OUTPUT "  is write access to SYS$SYSTEM:. If write access to SYS$SYTEM:"
$	  WRITE SYS$OUTPUT "  is not available, a personal installation for user"
$	  WRITE SYS$OUTPUT "  ''F$EDIT(F$GETJPI("","USERNAME"),"COLLAPSE")' will be created.''CR'''LF'"
$	  WRITE SYS$OUTPUT "  To run xmcd you need the DIAGNOSE and PHY_IO privileges."
$	  WRITE SYS$OUTPUT "  To use xmcd's WWWwarp features you need the CMKRNL privilege."
$	  WRITE SYS$OUTPUT "  For further details regarding privileges see the INSTALL.VMS file.''CR'''LF'"
$	  WRITE SYS$OUTPUT "  Please be prepared to answer a few simple questions regarding''CR'''LF'"
$	  WRITE SYS$OUTPUT "   - device names"
$	  IF LAME
$	   THEN
$	    WRITE SYS$OUTPUT "   - previously installed versions of xmcd and"
$	    WRITE SYS$OUTPUT "   - web browser(s) installed on your system''CR'''LF'"
$	   ELSE
$	    WRITE SYS$OUTPUT "   - previously installed versions of xmcd"
$	    WRITE SYS$OUTPUT "   - the presence of the LAME MP3 encoder and"
$	    WRITE SYS$OUTPUT "   - web browser(s) installed on your system''CR'''LF'"
$	  ENDIF
$	ENDIF
$	INQUIRE/NOPUNCTATION DUMMY "''ESC'[20C''ESC'[7mPress Return to continue or ^Y to abort''ESC'[0m"
$	WRITE SYS$OUTPUT "''ESC'[1A''ESC'[2K"
$! 
$! Check whether this procedure is run from the correct directory.
$! 
$	IF F$SEARCH("DOCS_D.DIR") .EQS. "" .AND. F$SEARCH("DOCS.DIR") .EQS. ""
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'''BEL'  It seems your current default directory is not the"
$	  WRITE SYS$OUTPUT "  one it should be.''CR'''LF'''BEL'"
$	  WRITE SYS$OUTPUT "  Please SET DEFAULT to the xmcd top-level directory"
$	  WRITE SYS$OUTPUT "  and run this procedure again.''CR'''LF'''BEL'"
$	  GOTO ENDEX
$	ENDIF
$	IF P1 .EQS. "I" THEN GOTO NO_CC
$	IF P1 .EQS. "C" THEN GOTO INQSSP
$! 
$! Check whether user knows what he's going to do.
$! 
$ INQRNR:
$	INQUIRE RNR "  Did you read the installation notes in INSTALL.VMS ? (Y/N)"
$	RNR = F$EXTRACT(0,1,F$EDIT(RNR,"UPCASE,COLLAPSE"))
$	IF RNR .NES. "Y" .AND. RNR .NES. "N" THEN GOTO INQRNR
$	IF .NOT. RNR 
$	 THEN
$ INQRTN:
$	  INQUIRE RTN "  Do you want to read them now ? (Y/N)"
$	  RTN = F$EXTRACT(0,1,F$EDIT(RTN,"UPCASE,COLLAPSE"))
$	  IF RTN .NES. "Y" .AND. RTN .NES. "N" THEN GOTO INQRTN
$	  IF .NOT. RTN THEN GOTO INQBKP
$	  INSTNOTES = F$SEARCH("[...]INSTALL.VMS")
$	  TYPE/PAGE 'INSTNOTES'
$	ENDIF
$ INQBKP:
$	WRITE SYS$OUTPUT "''CR'''LF'"
$	WRITE SYS$OUTPUT "  This procedure may move directory trees."
$	INQUIRE BKP "  Are you satisfied with the backups of your disks ? (Y/N)"
$	BKP = F$EXTRACT(0,1,F$EDIT(BKP,"UPCASE,COLLAPSE"))
$	IF BKP .NES. "Y" .AND. BKP .NES. "N" THEN GOTO INQBKP
$	IF .NOT. BKP
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  Please correct the situation and backup your disks first before"
$	  WRITE SYS$OUTPUT "  installing xmcd. ''CR'''LF'''CR'''LF'"
$	  GOTO ENDEX
$	ENDIF
$ INQSSP:
$	MAKEVMS_MOD = 0
$	WRITE SYS$OUTPUT "''CR'''LF'"
$	WRITE SYS$OUTPUT "  If you want the Gracenote CDDB(R) Disc Recognition Service"
$	WRITE SYS$OUTPUT "  (remote CD database) functionality being disabled, answer NO to"
$	WRITE SYS$OUTPUT "  the following question."
$	WRITE SYS$OUTPUT "  You need to answer with NO in case your system has no TCP/IP and"
$	WRITE SYS$OUTPUT "  socket support.''CR'''LF'"
$	INQUIRE SSP "  Do you want the remote CD database functionality being enabled ? (Y/N)"
$	SSP = F$EXTRACT(0,1,F$EDIT(SSP,"UPCASE,COLLAPSE"))
$	IF SSP .NES. "Y" .AND. SSP .NES. "N" THEN GOTO INQSSP
$	IF SSP .EQS. "N"
$	 THEN
$	  OPEN/WRITE 97 NOCDDB.EDT
$	  WRITE 97 "SUBSTITUTE/def=(/def=(noremote,/WHOLE"
$	  WRITE 97 "EXIT"
$	  CLOSE 97
$	  DEFINE/NOLOG SYS$INPUT SYS$COMMAND
$	  DEFINE/NOLOG SYS$OUTPUT NLA0:
$	  EDITTT/EDT/COMMAND=NOCDDB.EDT MAKEVMS.COM
$	  DEASSIGN SYS$OUTPUT
$	  DEASSIGN SYS$INPUT
$	  DELETE/NOCONFIRM NOCDDB.EDT;*
$	  MAKEVMS_MOD = MAKEVMS_MOD + 1
$	ENDIF
$! 
$! Cleanup XMCD_D directory first, then compile and link.
$! If compiling or linking failed, revert cleanup if applicable.
$! 
$	IF F$SEARCH("[.XMCD_D]XMCD.EXE") .NES. "" THEN RENAME [.XMCD_D]XMCD.EXE;* [.XMCD_D]XMCD_OLD.EXE
$
$! 
$! If running on Alpha, determine how to compile in regard to MMOV kits and licenses.
$! 
$	IF F$GETSYI("ARCH_NAME") .EQS. "Alpha"
$	 THEN
$	  IF MMOVRTL + MMOVRTF + MMOVDVL + MMOVDVF .NE. 4
$	   THEN
$	    WRITE SYS$OUTPUT "''CR'''LF'"
$	    WRITE SYS$OUTPUT "  Playing CDDA audio on your machine's audio device requires the following"
$	    WRITE SYS$OUTPUT "  MMOV (MultiMedia for OpenVMS) prerequisites:"
$	    WRITE SYS$OUTPUT ""
$	    IF MMOVRTF THEN WRITE SYS$OUTPUT "   - The MMOV runtime kit"
$	    IF .NOT. MMOVRTF THEN WRITE SYS$OUTPUT "   - The MMOV runtime kit         *"
$	    IF MMOVRTL THEN WRITE SYS$OUTPUT "   - The MMOV runtime license"
$	    IF .NOT. MMOVRTL THEN WRITE SYS$OUTPUT "   - The MMOV runtime license     *"
$	    IF MMOVDVF THEN WRITE SYS$OUTPUT "   - The MMOV development kit"
$	    IF .NOT. MMOVDVF THEN WRITE SYS$OUTPUT "   - The MMOV development kit     *"
$	    IF MMOVDVL THEN WRITE SYS$OUTPUT "   - The MMOV development license"
$	    IF .NOT. MMOVDVL THEN WRITE SYS$OUTPUT "   - The MMOV development license *"
$	    WRITE SYS$OUTPUT ""
$	    WRITE SYS$OUTPUT "  The items marked with an asterisk (*) don't seem to be present on your system."
$	    WRITE SYS$OUTPUT "  A xmcd executable with CDDA play audio support cannot be compiled."
$	    WRITE SYS$OUTPUT "  You may now continue to compile a xmcd version which only supports saving"
$	    WRITE SYS$OUTPUT "  CDDA audio to files or you may exit this procedure to install the required"
$	    WRITE SYS$OUTPUT "  kits and licenses."
$	    WRITE SYS$OUTPUT ""
$ INQCOWOMO:
$	    INQUIRE COWOMO "  Do you want to continue without CDDA play audio support ? (Y/N)
$	    COWOMO = F$EXTRACT(0,1,F$EDIT(COWOMO,"UPCASE,COLLAPSE"))
$	    IF COWOMO .NES. "Y" .AND. COWOMO .NES. "N" THEN GOTO INQCOWOMO
$	    IF COWOMO .EQS. "N" 
$	     THEN
$	      WRITE SYS$OUTPUT ""
$	      WRITE SYS$OUTPUT "  Please refer to the file INSTALL.VMS in the [.docs] directory regarding"
$	      WRITE SYS$OUTPUT "  a correct installation of MMOV."
$	      WRITE SYS$OUTPUT ""
$	      GOTO ENDEX
$	    ENDIF
$	    OPEN/WRITE 98 NOMMOV.EDT
$	    WRITE 98 "SUBSTITUTE/has_mme,//WHOLE"
$	    WRITE 98 "FIND '$ write out ""! mmov (aka mme)'"
$	    WRITE 98 "DELETE"
$	    WRITE 98 "FIND '$ write out ""sys$sysdevice:[sys0.syscommon.syslib]mmov.olb'"
$	    WRITE 98 "DELETE"
$	    WRITE 98 "EXIT"
$	    CLOSE 98
$	    DEFINE/NOLOG SYS$INPUT SYS$COMMAND
$	    DEFINE/NOLOG SYS$OUTPUT NLA0:
$	    EDITTT/EDT/COMMAND=NOMMOV.EDT MAKEVMS.COM
$	    DEASSIGN SYS$OUTPUT
$	    DEASSIGN SYS$INPUT
$	    DELETE/NOCONFIRM NOMMOV.EDT;*
$	    MAKEVMS_MOD = MAKEVMS_MOD + 1
$	   ELSE
$	    IF F$TRNLNM("MME") .EQS. ""
$	     THEN
$	      DEFINE/SYSTEM/EXEC/NOLOG MME MMOV$INCLUDE
$	      WRITE SYS$OUTPUT ""
$	      WRITE SYS$OUTPUT "  The logical name MME was not found to be defined on your system."
$	      WRITE SYS$OUTPUT "  It is required for the MMOV development software and has now been defined"
$	      WRITE SYS$OUTPUT "  temporarily."
$	      WRITE SYS$OUTPUT "  Please add the following command to SYS$COMMON:[SYS$STARTUP]MMOV$STARTUP.COM:"
$	      WRITE SYS$OUTPUT ""
$	      WRITE SYS$OUTPUT "  $DEFINE/SYSTEM/EXEC/NOLOG MME MMOV$INCLUDE"
$	      WRITE SYS$OUTPUT ""
$	      INQUIRE/NOPUNCTATION DUMMY "''ESC'[20C''ESC'[7mPress Return to continue or ^Y to abort''ESC'[0m"
$	      WRITE SYS$OUTPUT "''ESC'[1A''ESC'[2K"
$	    ENDIF
$	  ENDIF
$	ENDIF
$!
$! If running on VAX, MMOV is not available.
$!
$	IF F$GETSYI("ARCH_NAME") .EQS. "VAX"
$	 THEN
$	  OPEN/WRITE 98 NOMMOV.EDT
$	  WRITE 98 "SUBSTITUTE/has_mme,//WHOLE"
$	  WRITE 98 "FIND '$ write out ""! mmov (aka mme)'"
$	  WRITE 98 "DELETE"
$	  WRITE 98 "FIND '$ write out ""sys$sysdevice:[sys0.syscommon.syslib]mmov.olb'"
$	  WRITE 98 "DELETE"
$	  WRITE 98 "EXIT"
$	  CLOSE 98
$	  DEFINE/NOLOG SYS$INPUT SYS$COMMAND
$	  DEFINE/NOLOG SYS$OUTPUT NLA0:
$	  EDITTT/EDT/COMMAND=NOMMOV.EDT MAKEVMS.COM
$	  DEASSIGN SYS$OUTPUT
$	  DEASSIGN SYS$INPUT
$	  DELETE/NOCONFIRM NOMMOV.EDT;*
$	  MAKEVMS_MOD = MAKEVMS_MOD + 1
$	ENDIF
$!
$! Check for VMS version. Modify compilation commands if pthreads not available 
$!
$	IF F$GETSYI("VERSION") .LES. "V7.0"
$	 THEN
$	  OPEN/WRITE 96 NOPTHR.EDT
$	  WRITE 96 "SUBSTITUTE/,use_pthread_delay_np//WHOLE"
$	  WRITE 96 "SUBSTITUTE/has_pthreads/no_pthreads/WHOLE"
$	  WRITE 96 "EXIT"
$	  CLOSE 96
$	  DEFINE/NOLOG SYS$INPUT SYS$COMMAND
$	  DEFINE/NOLOG SYS$OUTPUT NLA0:
$	  EDITTT/EDT/COMMAND=NOPTHR.EDT MAKEVMS.COM
$	  DEASSIGN SYS$OUTPUT
$	  DEASSIGN SYS$INPUT
$	  DELETE/NOCONFIRM NOPTHR.EDT;*
$	  MAKEVMS_MOD = MAKEVMS_MOD + 1
$	ENDIF
$
$	WRITE SYS$OUTPUT "''ESC'[0;0H''ESC'[2J''CR'''LF'  Starting compilation of source code...''CR'''LF'"
$	@MAKEVMS
$	SET DEFAULT 'ENVI'
$ RMMK:
$	IF MAKEVMS_MOD .NE. 0 THEN DELETE/NOCONFIRM MAKEVMS.COM;0
$	MAKEVMS_MOD = MAKEVMS_MOD - 1
$	IF MAKEVMS_MOD .GE. 0 THEN GOTO RMMK
$	IF F$SEARCH("[.XMCD_D]XMCD.EXE") .EQS. ""
$	 THEN
$	  IF F$SEARCH("[.XMCD_D]XMCD_OLD.EXE") .NES. "" THEN RENAME [.XMCD_D]XMCD_OLD.EXE;* [.XMCD_D]XMCD.EXE
$	  WRITE SYS$OUTPUT "''CR'''LF'  Compiling and linking the xmcd executable failed."
$	  WRITE SYS$OUTPUT "  Installation of xmcd aborted.''CR'''LF'''CR'''LF'"
$	  GOTO ENDEX
$	ENDIF
$	WRITE SYS$OUTPUT "''CR'''LF'  Compiling and linking the xmcd executable completed successfully"
$	IF P1 .EQS. "C"
$	 THEN
$	  WRITE SYS$OUTPUT "  at ''F$CVTIME(,"ABSOLUTE",)'''CR'''LF'"
$	  GOTO ENDEX
$	ENDIF
$ NO_CC:
$	IF F$SEARCH("[.XMCD_D]XMCD.EXE") .NES. "" THEN RENAME [.XMCD_D]XMCD.EXE []
$	CMDDEF = "XMCD :== $''ENVI'XMCD.EXE"
$! 
$! Determine XMCD_LIBDIR logical and add definition of logicals to 
$! user's LOGIN.COM
$! 
$	LIBDIR = ENVI  -"]"
$	SET DEFAULT SYS$LOGIN
$	IF F$SEARCH("LOGIN.COM") .EQS. ""
$	 THEN
$	  WRITE SYS$OUTPUT ""
$	  WRITE SYS$OUTPUT "  You don't seem to have SYS$LOGIN:LOGIN.COM executed as the default"
$	  WRITE SYS$OUTPUT "  procedure when logging into the system."
$	  WRITE SYS$OUTPUT "  Please include the LOGIN.COM now being created into your default login"
$	  WRITE SYS$OUTPUT "  procedure after completion of this procedure."
$	  WRITE SYS$OUTPUT ""
$	ENDIF
$	IF .NOT. LAME
$	 THEN
$	  WRITE SYS$OUTPUT ""
$	  WRITE SYS$OUTPUT "  The LAME MP3 encoder was not found searching the default directories."
$ INQLMPR:
$	  INQUIRE LAMEPRES "  Do you have the LAME MP3 encoder installed on your sytem ? (Y/N)"
$	  LAMEPRES = F$EXTRACT(0,1,F$EDIT(LAMEPRES,"UPCASE,COLLAPSE"))
$	  IF LAMEPRES .NES. "Y" .AND. LAMEPRES .NES. "N" THEN GOTO INQLMPR
$	  IF LAMEPRES 
$	   THEN
$	    WRITE SYS$OUTPUT ""
$	    WRITE SYS$OUTPUT "  Please enter the complete path including the device name for the LAME"
$	    WRITE SYS$OUTPUT "  executable now (device:[directory]name.extension)''CR'''LF'"
$	    INQUIRE LAMELOCA "  The LAME path is"
$	    IF F$SEARCH(LAMELOCA) .EQS. ""
$	     THEN
$	      WRITE SYS$OUTPUT ""
$	      WRITE SYS$OUTPUT "  Sorry, but the LAME executable is NOT ''LAMELOCA'"
$	      WRITE SYS$OUTPUT "  Please review your related answers.''CR'''LF'"
$	      WRITE SYS$OUTPUT ""
$	      GOTO INQLMPR
$	     ELSE
$	      MESENV = F$ENVIRONMENT("MESSAGE")
$	      SET MESSAGE/NOTEXT/NOSEVERITY/NOFACILITY/NOIDENTIFICATION
$	      DEFINE SYS$ERROR LAMEOUT.TMP
$	      MC 'LAMELOCA'
$	      DEASSIGN SYS$ERROR
$	      SET MESSAGE 'MESENV'
$	      IF F$SEARCH("LAMEOUT.TMP") .NES. ""
$	       THEN
$	        OPEN/READ 99 LAMEOUT.TMP
$	        READ  99 LINE
$	        CLOSE 99
$	        DELETE/NOCONFIRM LAMEOUT.TMP;*
$	        IF F$EXTRACT(0,4,LINE) .EQS. "LAME" THEN LAME = 1
$	      ENDIF
$	      IF .NOT. LAME
$	       THEN
$	        WRITE SYS$OUTPUT ""
$	        WRITE SYS$OUTPUT "  Sorry, but the LAME executable is NOT ''LAMELOCA'"
$	        WRITE SYS$OUTPUT "  Please review your related answers.''CR'''LF'"
$	        WRITE SYS$OUTPUT ""
$	        GOTO INQLMPR
$	      ENDIF
$	    ENDIF
$	  ENDIF
$	ENDIF
$	LIBDIR_D = LIBDIR
$	IF F$TRNLNM(F$ELEMENT(0,":",LIBDIR)) .NES. "" THEN LIBDIR_D = F$TRNLNM(F$ELEMENT(0,":",LIBDIR)) + F$ELEMENT(1,":",LIBDIR)
$	OPEN/WRITE 100 LOGIN.ADD
$	WRITE 100 "$	"
$	WRITE 100 "$	''CMDDEF'"
$	WRITE 100 "$	DEFINE/JOB XMCD_LIBDIR ""''LIBDIR'"""
$	WRITE 100 "$	DEFINE/JOB $DISCOGDIR ''LIBDIR_D'.DISCOG.] /TRANSLATION_ATTRIBUTES=(TERMINAL,CONCEALED)"
$	IF LAME THEN WRITE 100 "$	LAME_PATH :== ''LAMELOCA'"
$	WRITE 100 "$	"
$	CLOSE 100
$	WRITE SYS$OUTPUT "''CR'''LF'  Next you will automatically enter your LOGIN.COM."
$	WRITE SYS$OUTPUT "  Then move your cursor to the line where you want the setup commands for"
$	WRITE SYS$OUTPUT "  the xmcd library and discography directories to be.''CR'''LF'"
$	WRITE SYS$OUTPUT "  Simply press the key ""PF1"" and then ""I"" to insert the line(s) needed."
$	WRITE SYS$OUTPUT "  To exit press ""PF1"" and ""E"" then.''CR'''LF'"
$	WRITE SYS$OUTPUT "  If you are re-running this procedure and already have the correct"
$	WRITE SYS$OUTPUT "  definitions in your LOGIN.COM, only press ""PF1"" and ""Q"" to quit.''CR'''LF'"
$	INQUIRE/NOPUNCTATION DUMMY "''ESC'[28C''ESC'[7mPress Return to continue''ESC'[0m"
$	OPEN/WRITE 116 E.E
$	WRITE 116 "SET TEXT END ""[EOF]"""
$	WRITE 116 "SET MODE CHANGE"
$	WRITE 116 "DEFINE KEY GOLD E AS 'EXT EXIT.'"
$	WRITE 116 "DEFINE KEY GOLD Q AS 'EXT QUIT.'"
$	WRITE 116 "DEFINE KEY GOLD I AS ""EXT INCLUDE LOGIN.ADD."""
$	CLOSE 116
$	DEFINE/USER/NOLOG SYS$INPUT SYS$COMMAND
$	EDITTT/EDT/COMMAND=E.E LOGIN.COM
$	DELETE/NOCONFIRM LOGIN.ADD;*,E.E;*
$	SET DEFAULT 'ENVI'
$!
$! Move basic config files to target directory.
$! Prompt user to specify a CD drive.
$!
$	IF F$SEARCH("[.LIBDI_D]COMMON.CFG") .NES. "" THEN RENAME [.LIBDI_D]COMMON.CFG []
$	IF F$SEARCH("[.LIBDI_D]DEVICE.CFG") .NES. "" THEN RENAME [.LIBDI_D]DEVICE.CFG []
$	WRITE SYS$OUTPUT "''CR'''LF'  You now need to specify the CD drive you want to use with xmcd."
$	WRITE SYS$OUTPUT "  Please enter its name at the following prompt."
$	WRITE SYS$OUTPUT "  If you are in a VMS cluster environment, you need to"
$	WRITE SYS$OUTPUT "  specify the full device name, f.i. MYBOX$DKB400:''CR'''LF'"
$ DEV_NAM:
$	CDR = ""
$	INQUIRE CDR "  Please enter the CD drive's device name"
$	CDR = F$EDIT(CDR,"UPCASE,COLLAPSE")
$	IF CDR .EQS. "" THEN GOTO DEV_NAM
$	IF CDR .EQS. "EXIT" THEN GOTO ENDEX
$	IF .NOT. F$GETDVI(CDR,"EXISTS") 
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  ''BEL'''CDR' is not a valid device name.''BEL'"
$	  GOTO DEV_NAM
$	ENDIF  
$!
$! Apply required changes to config files.
$!
$	OPEN/WRITE 120 COMM.MOD
$	OPEN/WRITE 121 DEVI.MOD
$	WRITE 120 "device:			''CDR'"
$	WRITE 121 "devicelist:		''CDR'"
$	CLOSE 120
$	CLOSE 121
$	OPEN/WRITE 130 COMM.EDT
$	OPEN/WRITE 131 DEVI.EDT
$	WRITE 130 "FIND 'device:'"
$	WRITE 131 "FIND 'devicelist:'"
$	WRITE 130 "DELETE"
$	WRITE 131 "DELETE"
$	WRITE 130 "INCLUDE COMM.MOD"
$	WRITE 131 "INCLUDE DEVI.MOD"
$	WRITE 130 "EXIT"
$	WRITE 131 "EXIT"
$	CLOSE 130
$	CLOSE 131
$	WRITE SYS$OUTPUT "''CR'''LF'  Updating basic config files to use ''CDR'..."
$	DEFINE/NOLOG SYS$INPUT SYS$COMMAND
$	DEFINE/NOLOG SYS$OUTPUT NLA0:
$	EDITTT/EDT/COMMAND=COMM.EDT COMMON.CFG
$	EDITTT/EDT/COMMAND=DEVI.EDT DEVICE.CFG
$	DEASSIGN SYS$OUTPUT
$	DEASSIGN SYS$INPUT
$	DELETE/NOCONFIRM *.EDT;*,*.MOD;*
$	IF F$GETSYI("VERSION") .LES. "V7" THEN GOTO SETUP_HELP
$!
$! Prepare user having to specify the CD drive model.
$!
$	DRVMDL = F$GETDVI(CDR,"DEVICE_TYPE_NAME")
$	
$	WRITE SYS$OUTPUT "''CR'''LF'  You now need to specify your CD drive model."
$	WRITE SYS$OUTPUT "  This information will be used to apply changes to xmcd's device"
$	WRITE SYS$OUTPUT "  configuration file which will be necessary to make use of all your"
$	WRITE SYS$OUTPUT "  CD drive's capabilities."
$	WRITE SYS$OUTPUT ""
$	WRITE SYS$OUTPUT "  Your CD drive ''CDR' is considered to be a ''ESC'[7m ''DRVMDL' ''ESC'[0m."
$	WRITE SYS$OUTPUT ""
$	WRITE SYS$OUTPUT "  Remember this model type when next being prompted with the drive"
$	WRITE SYS$OUTPUT "  manufacturer and model type menus.''CR'''LF'"
$	INQUIRE/NOPUNCTATION DUMMY "''ESC'[28C''ESC'[7mPress Return to continue''ESC'[0m"
$!
$! Start the device.cfg configuration procedure
$!
$	IF F$SEARCH("[.MISC_D]SELDRIVE.COM") .NES. "" THEN RENAME [.MISC_D]SELDRIVE.COM []
$	@SELDRIVE.COM "" "NO_ENTRY_TEXT"
$	
$	WRITE SYS$OUTPUT "''CR'''LF'"
$
$ SETUP_HELP:
$!
$! Move help and doc files as well as the X resource file to their target directories.
$!
$	WRITE SYS$OUTPUT "''CR'''LF'  Setting up help and documentation files..."
$	IF F$SEARCH("HELP.DIR") .EQS. "" THEN CREATE/DIRECTORY [.HELP]
$	IF F$SEARCH("[.XMCD_D.HLPFILES]*.*") .NES. "" THEN RENAME [.XMCD_D.HLPFILES]*.* [.HELP]
$	IF F$SEARCH("DOCS_D.DIR") .NES. "" THEN RENAME DOCS_D.DIR DOCS.DIR
$	WRITE SYS$OUTPUT "''CR'''LF'  Moving X resources file to target directory..."
$	COPY/NOLOG [.XMCD_D]XMCD.AD SYS$COMMON:[DECW$DEFAULTS.USER]XMCD.DAT
$	IF $SEVERITY .NE. 1
$	 THEN 
$	  WRITE SYS$OUTPUT "''CR'''LF'  Copying the X resources file to DECW$SYSTEM_DEFAULTS failed."
$	  WRITE SYS$OUTPUT "  Please have your system manager copy XMCD.AD from the XMCD_D area to"
$	  WRITE SYS$OUTPUT "  SYS$COMMON:[DECW$DEFAULTS.USER]XMCD.DAT on your behalf for a"
$	  WRITE SYS$OUTPUT "  correct installation."
$	  WRITE SYS$OUTPUT "  To make xmcd working for only you, the X resources file is copied to"
$	  WRITE SYS$OUTPUT "  your SYS$LOGIN: directory.''CR''LF'"
$	  INQUIRE/NOPUNCTATION DUMMY "''BEL'''ESC'[28C''ESC'[7mPress Return to continue''ESC'[0m''BEL'"
$	  COPY/NOLOG [.XMCD_D]XMCD.AD SYS$LOGIN:XMCD.DAT
$	  SET FILE/PROT=W:RE SYS$LOGIN:XMCD.DAT
$	 ELSE
$	  SET FILE/PROT=W:RE SYS$COMMON:[DECW$DEFAULTS.USER]XMCD.DAT
$	  IF F$SEARCH("SYS$LOGIN:XMCD.DAT") .NES. ""
$	   THEN
$	    WRITE SYS$OUTPUT "''CR'''LF'  Please review your private X resources file in SYS$LOGIN"
$	    WRITE SYS$OUTPUT "  to match the current version number and also check it for current 
$	    WRITE SYS$OUTPUT "  changes you may want to apply."
$	  ENDIF
$	ENDIF
$!
$! If no CDINFO directory exists or if there are no files in it, 
$! prompt user to indicate whether there is a 2.x version CDDB.
$! If there is, let the user specify the directory where the old CDDB 
$! is resident. 
$!
$	XMCD2 = 0
$	XMCD3 = 0
$	CDEX  = 0
$	CDDB  = 0
$	IF F$SEARCH("CDINFO.DIR") .NES. "" 
$	 THEN 
$	  CDEX = 1
$	  IF F$SEARCH("[.CDINFO]*.DIR") .NES. "" 
$	   THEN 
$	    GOTO NO_CDDB
$	  ENDIF
$	ENDIF
$	WRITE SYS$OUTPUT "''CR'''LF'  If you have xmcd version 2.x previously installed on your system,"
$	WRITE SYS$OUTPUT "  there is a CDDB.DIR directory under your xmcd library.  This has"
$	WRITE SYS$OUTPUT "  been deprecated, but this release of xmcd will continue to read files"
$	WRITE SYS$OUTPUT "  from that directory, if it is renamed or backed up to CDINFO.DIR."
$	WRITE SYS$OUTPUT "  Please indicate now if you have xmcd version 2.x previously installed"
$	WRITE SYS$OUTPUT "  on your system. If so, you'll be prompted to enter the complete path"
$	WRITE SYS$OUTPUT "  including the device name for xmcd 2.x's CDDB.DIR''CR'''LF'"
$ INQDB:
$	INQUIRE CDDB "  Do you have xmcd version 2.x previously installed ? (Y/N)"
$	CDDB = F$EXTRACT(0,1,F$EDIT(CDDB,"UPCASE,COLLAPSE"))
$	IF CDDB .NES. "Y" .AND. CDDB .NES. "N" THEN GOTO INQDB
$	IF .NOT. CDDB THEN GOTO NO_CDDB_2X
$	WRITE SYS$OUTPUT "''CR'''LF'  Please enter the complete path of your xmcd 2.x's CDDB.DIR"
$	WRITE SYS$OUTPUT "  For example MYBOX$DKA400:[XMCD-2_6]''CR'''LF'"
$	VRB  = "Specify"
$	EVRB = ""
$ INQDBP:
$	INQUIRE CDDB_LOC "  ''VRB' complete path''EVRB'"
$	IF F$EDIT(CDDB_LOC,"COLLAPSE,UPCASE") .EQS. "EXIT" THEN GOTO NO_CDDB_2X
$	IF F$SEARCH("''CDDB_LOC'CDDB.DIR") .EQS. ""
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  ''CDDB_LOC' does not include CDDB.DIR''CR'''LF'"
$	  WRITE SYS$OUTPUT "  If you erroneously indicated to have a 2.x CDDB"
$	  WRITE SYS$OUTPUT "  please type EXIT at the path prompt.''CR'''LF'"
$	  VRB  = "Respecify"
$	  EVRB = " or type EXIT"
$	  GOTO INQDBP 
$	ENDIF
$!
$! Determine whether the old CDDB is located on this disk device or on a
$! different one. If the latter, usage of BACKUP is required, otherwise
$! a rename operation is sufficient.
$!
$	DEVI_OLD = F$PARSE(CDDB_LOC,,,"DEVICE")
$	DEVI_CUR = F$PARSE(ENVI,,,"DEVICE")
$	PATH_OLD = F$PARSE(CDDB_LOC,,,"DIRECTORY")
$	PATH_CUR = F$PARSE(ENVI,,,"DIRECTORY")
$	WRITE SYS$OUTPUT "''CR'''LF'  Moving local xmcd 2.x CD database to target environment."
$	IF CDEX THEN DELETE/NOCONFIRM CDINFO.DIR;*
$	IF DEVI_OLD .EQS. DEVI_CUR
$	 THEN
$	  RENAME 'PATH_OLD'CDDB.DIR []CDINFO.DIR
$	 ELSE
$	  WRITE SYS$OUTPUT "  This may take some time..."
$	  PATH_OLD = PATH_OLD - "]"
$	  PATH_CUR = PATH_CUR - "]"
$	  BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.CDDB...]*.* 'DEVI_CUR''PATH_CUR'.CDINFO...]*
$	ENDIF
$	WRITE SYS$OUTPUT ""
$	XMCD2 = 1
$	GOTO NO_CDDB
$ NO_CDDB_2X:
$	WRITE SYS$OUTPUT "''CR'''LF'  If you have xmcd version 3.x previously installed on your system,"
$	WRITE SYS$OUTPUT "  there may be a CDINFO.DIR directory under your xmcd library."
$	WRITE SYS$OUTPUT "  Please indicate now if you have xmcd version 3.x previously installed"
$	WRITE SYS$OUTPUT "  on your system. If so, you'll be prompted to enter the complete path"
$	WRITE SYS$OUTPUT "  including the device name for xmcd 3.x's CDINFO.DIR''CR'''LF'"
$ INQDB3:
$	INQUIRE CDDB "  Do you have xmcd version 3.x previously installed ? (Y/N)"
$	CDDB = F$EXTRACT(0,1,F$EDIT(CDDB,"UPCASE,COLLAPSE"))
$	IF CDDB .NES. "Y" .AND. CDDB .NES. "N" THEN GOTO INQDB3
$	IF .NOT. CDDB THEN GOTO NO_CDDB
$	WRITE SYS$OUTPUT "''CR'''LF'  Please enter the complete path of your xmcd 3.x's CDINFO.DIR"
$	WRITE SYS$OUTPUT "  For example MYBOX$DKA400:[XMCD-3_1]''CR'''LF'"
$	VRB  = "Specify"
$	EVRB = ""
$ INQDBP3:
$	INQUIRE CDDB_LOC "  ''VRB' complete path''EVRB'"
$	IF F$EDIT(CDDB_LOC,"COLLAPSE,UPCASE") .EQS. "EXIT"
$	 THEN 
$	  CDDB = "N"
$	  GOTO NO_CDDB
$	ENDIF
$	IF F$SEARCH("''CDDB_LOC'CDINFO.DIR") .EQS. ""
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  ''CDDB_LOC' does not include CDINFO.DIR''CR'''LF'"
$	  WRITE SYS$OUTPUT "  If you erroneously indicated to have a 3.x CDINFO structure"
$	  WRITE SYS$OUTPUT "  please type EXIT at the path prompt.''CR'''LF'"
$	  VRB  = "Respecify"
$	  EVRB = " or type EXIT"
$	  GOTO INQDBP3
$	ENDIF
$!
$! Determine whether the old CDDB is located on this disk device or on a
$! different one. If the latter, usage of BACKUP is required, otherwise
$! a rename operation is sufficient.
$!
$	DEVI_OLD = F$PARSE(CDDB_LOC,,,"DEVICE")
$	DEVI_CUR = F$PARSE(ENVI,,,"DEVICE")
$	PATH_OLD = F$PARSE(CDDB_LOC,,,"DIRECTORY")
$	PATH_CUR = F$PARSE(ENVI,,,"DIRECTORY")
$	WRITE SYS$OUTPUT "''CR'''LF'  Moving local xmcd 3.x CD database to target environment."
$	IF CDEX THEN DELETE/NOCONFIRM CDINFO.DIR;*
$	IF DEVI_OLD .EQS. DEVI_CUR
$	 THEN
$	  RENAME 'PATH_OLD'CDINFO.DIR []CDINFO.DIR
$	 ELSE
$	  WRITE SYS$OUTPUT "  This may take some time..."
$	  PATH_OLD = PATH_OLD - "]"
$	  PATH_CUR = PATH_CUR - "]"
$	  BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.CDINFO...]*.* 'DEVI_CUR''PATH_CUR'.CDINFO...]*
$	ENDIF
$	WRITE SYS$OUTPUT ""
$	XMCD3 = 1
$ NO_CDDB:
$! 
$! Apply modifications to enable wwwwarp functions. 
$! First move files to target directory. 
$! 
$	WRITE SYS$OUTPUT "''CR'''LF'  Preparing WWWwarp setup...''CR'''LF'"
$	IF F$SEARCH("[.MISC_D]STARTVIEWER.COM")	.NES. "" THEN RENAME [.MISC_D]STARTVIEWER.COM []
$	IF F$SEARCH("[.MISC_D]INSTWEB.COM")	.NES. "" THEN RENAME [.MISC_D]INSTWEB.COM []
$	IF F$SEARCH("[.CDINFO_D]WWWWARP.CFG")	.NES. "" THEN RENAME [.CDINFO_D]WWWWARP.CFG []
$	@INSTWEB.COM
$	SET DEFAULT 'ENVI'
$! 
$! Setup local Discography. Create required subdir and move required files
$! to the discog area, then create genre and subgenre directories.
$! 
$	WRITE SYS$OUTPUT "''CR'''LF'  Setting up Local Discography...''CR'''LF'"
$	IF F$SEARCH("DISCOG.DIR") .EQS. "" THEN CREATE/DIRECTORY [.DISCOG]
$	IF F$SEARCH("[.MISC_D]DISCOGVMS.HTM")	.NES. "" THEN RENAME [.MISC_D]DISCOGVMS.HTM [.DISCOG]DISCOG.HTML
$	IF F$SEARCH("[.MISC_D]BKGND.GIF")	.NES. "" THEN RENAME [.MISC_D]BKGND.GIF	    [.DISCOG]
$	IF F$SEARCH("[.MISC_D]XMCDLOGO.GIF")	.NES. "" THEN RENAME [.MISC_D]XMCDLOGO.GIF  [.DISCOG]
$	IF F$SEARCH("[.XMCD_D]XMCD.HTM")	.NES. "" THEN RENAME [.XMCD_D]XMCD.HTM      [.DISCOG]XMCD.HTML	
$	IF F$SEARCH("[.CDA_D]CDA.HTM")		.NES. "" THEN RENAME [.CDA_D]CDA.HTM      [.DISCOG]CDA.HTML	
$	WRITE SYS$OUTPUT "  Creating Local Discography genre directories.''CR'''LF'"
$	IF F$SEARCH("[.DISCOG.BLUES]GENERAL_BLUES.DIR")		  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.BLUES.GENERAL_BLUES]
$	IF F$SEARCH("[.DISCOG.CLASSICAL]GENERAL_CLASSICAL.DIR")	  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.CLASSICAL.GENERAL_CLASSICAL]
$	IF F$SEARCH("[.DISCOG.COUNTRY]GENERAL_COUNTRY.DIR")	  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.COUNTRY.GENERAL_COUNTRY]
$	IF F$SEARCH("[.DISCOG.DATA]GENERAL_DATA.DIR")		  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.DATA.GENERAL_DATA]
$	IF F$SEARCH("[.DISCOG.FOLK]GENERAL_FOLK.DIR")		  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.FOLK.GENERAL_FOLK]
$	IF F$SEARCH("[.DISCOG.JAZZ]GENERAL_JAZZ.DIR")		  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.JAZZ.GENERAL_JAZZ]
$	IF F$SEARCH("[.DISCOG.NEW_AGE]GENERAL_NEW_AGE.DIR")	  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.NEW_AGE.GENERAL_NEW_AGE]
$	IF F$SEARCH("[.DISCOG.ROCK]GENERAL_ROCK.DIR")		  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.ROCK.GENERAL_ROCK]
$	IF F$SEARCH("[.DISCOG.SOUNDTRACK]GENERAL_SOUNDTRACK.DIR") .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.SOUNDTRACK.GENERAL_SOUNDTRACK]
$	IF F$SEARCH("[.DISCOG.UNCLASSIFIABLE]GENERAL_UNCLASSIFIABLE.DIR") .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.UNCLASSIFIABLE.GENERAL_UNCLASSIFIABLE]
$	IF F$SEARCH("[.DISCOG.WORLD]REGGAE.DIR")		  .EQS. "" THEN CREATE/DIRECTORY [.DISCOG.WORLD.REGGAE]
$	IF .NOT. CDDB THEN GOTO EXIT
$	VERVAR = ""
$	IF XMCD2 THEN VERVAR = "2.x"
$	IF XMCD3 THEN VERVAR = "3.x"
$	IF F$SEARCH("''PATHOLD'DISCOG.DIR") .EQS. ""
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  No xmcd ''VERVAR' Local Discography files found."
$	  GOTO EXIT
$	ENDIF
$	WRITE SYS$OUTPUT "''CR'''LF'  Moving xmcd ''VERVAR' Local Discography files to target environment."
$	WRITE SYS$OUTPUT "  This may take a few minutes...''CR'''LF'"
$	PATH_OLD = PATH_OLD - "]"
$	PATH_CUR = PATH_CUR - "]"
$	WRITE SYS$OUTPUT "  Moving Blues genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_BLUES."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.BLUES...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.BLUES...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.BLUES.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving Classical genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_CLASSICAL."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.CLASSICAL...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.CLASSICAL...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.CLASSICAL.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving Country genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_COUNTRY."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.COUNTRY...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.COUNTRY...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.COUNTRY.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving Data genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_DATA."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.DATA...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.DATA...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.DATA.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving Folk genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_FOLK."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.FOLK...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.FOLK...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.FOLK.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving Jazz genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_JAZZ."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.JAZZ...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.JAZZ...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.JAZZ.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving miscellaneous genre files..."
$	IF XMCD2 
$	 THEN
$	  IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.MISC...]*.*") .NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.MISC...]*.* 'DEVI_CUR''PATH_CUR'.DISCOG.UNCLASSIFIABLE.GENERAL_UNCLASSIFIABLE...]
$	 ELSE
$	  IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.UNCLASSIFIABLE...]*.*") .NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.UNCLASSIFIABLE...]*.* 'DEVI_CUR''PATH_CUR'.DISCOG.UNCLASSIFIABLE...]
$	ENDIF
$	WRITE SYS$OUTPUT "  Moving Newage genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_NEW_AGE."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.NEWAGE...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.NEWAGE...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.NEW_AGE.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving Reggae genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "WORLD."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.REGGAE...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.REGGAE...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.'SBGRSD'REGGAE...]
$	WRITE SYS$OUTPUT "  Moving Rock genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_ROCK."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.ROCK...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.ROCK...]*.*	 'DEVI_CUR''PATH_CUR'.DISCOG.ROCK.'SBGRSD'..]
$	WRITE SYS$OUTPUT "  Moving Soundtrack genre files..."
$	SBGRSD = ""
$	IF XMCD2 THEN SBGRSD = "GENERAL_SOUNDTRACK."
$	IF F$SEARCH("''DEVI_OLD'''PATH_OLD'.DISCOG.SOUNDTRACK...]*.*")	.NES. "" THEN BACKUP/NOLOG 'DEVI_OLD''PATH_OLD'.DISCOG.SOUNDTRACK...]*.* 'DEVI_CUR''PATH_CUR'.DISCOG.SOUNDTRACK.'SBGRSD'..]
$	
$	WRITE SYS$OUTPUT "''CR'''LF'  Creating main index and category HTML index files..."
$	DEFINE/JOB/NOLOG $DISCOGDIR 'LIBDIR_D'.DISCOG.] /TRANSLATION_ATTRIBUTES=(TERMINAL,CONCEALED)
$	@GENIDX "" RUN_SILENT
$	IF XMCD3 THEN GOTO EXIT
$	WRITE SYS$OUTPUT "''CR'''LF'  Starting conversion of xmcd 2.x Local Discography index files"
$	WRITE SYS$OUTPUT "  This may take some time, depending on the number of your files.''CR'''LF'"
$!	
$! No warning messages from the SEARCH command wanted. If running a VMS
$! pre-7.x version, SET MESSAGE is required to have the box shutup. The 
$! user's message environment is restored after completion of this part.
$!	
$	VSCOM = ""
$	IF F$GETSYI("VERSION") .GES. "V6.2"
$	 THEN
$	  VSCOM = "/NOWARNING"
$	 ELSE
$	  MESENV = F$ENVIRONMENT("MESSAGE")
$	  SET MESSAGE/NOTEXT/NOSEVERITY/NOFACILITY/NOIDENTIFICATION
$	ENDIF
$!	
$! Now start searching for INDEX.HTML files in the discog area.
$! If it's not a disk's INDEX file or if it already has been converted, 
$! search for next file. 
$!	
$	FILES_CVT = 0
$ CONVOLDIDX:
$	WFILE    = F$SEARCH("[.DISCOG.*...]INDEX.HTML;0")
$	IF WFILE .EQS. "" THEN GOTO CONV_DONE
$	SEARCH/NOOUT'VSCOM' 'WFILE' "Total:","Total time:"
$	IF $SEVERITY .NE. 1 THEN GOTO CONVOLDIDX
$	SEARCH/NOOUT'VSCOM' 'WFILE' "xmcd 3.0"
$	IF $SEVERITY .EQ. 1 THEN GOTO CONVOLDIDX
$	THIS_DIR = F$PARSE(WFILE,,,"DEVICE") + F$PARSE(WFILE,,,"DIRECTORY")
$	FILES_CVT = FILES_CVT + 1
$!	
$! Found file to convert. First save info which can't be replaced by some
$! default stuff.
$!	
$	SEARCH/EXACT'VSCOM'/OUT=FND.TMP 'WFILE' "Disc ID:","Disc ID</B>:"
$	OPEN/READ 180 FND.TMP
$	READ      180 LINE
$	CLOSE     180
$	DELETE/NOCONFIRM FND.TMP;0
$	PRE_26 = 0
$	IF F$LOCATE("ID</B>:",LINE) .NE.F$LENGTH(LINE) THEN PRE_26 = 1
$!
$! Setup changes for xmcd 3.x INDEX.HTML format
$!
$	LP       = F$EXTRACT(0,F$LOCATE("<TD ALIGN=",LINE)+17,LINE)
$	LINEX    = LINE - LP
$	MP       = F$EXTRACT(0,F$LOCATE("<BR>",LINEX),LINEX)
$	RP       = LINEX - MP
$	DDSK     = F$PARSE(WFILE,,,"DEVICE") - ":"
$	WDIR     = F$PARSE(WFILE,,,"DIRECTORY")
$	DDIR     = "''WDIR'" - "[" - "]"
$	GENRE    = DDIR - F$EXTRACT(0,F$LOCATE("DISCOG.",DDIR)+7,DDIR)
$	SUBGENRE = F$ELEMENT(1,".",GENRE)
$	ID	 = F$ELEMENT(2,".",GENRE)	
$	GENRE    = F$ELEMENT(0,".",GENRE)
$	DDIR	 = DDIR - ".DISCOG.''GENRE'.''SUBGENRE'.''ID'"
$ EDITDDIR:
$	IF F$LOCATE(".",DDIR) .EQ. F$LENGTH(DDIR) THEN GOTO NOEDITDDIR
$	DDIR['F$LOCATE(".",DDIR),1] := "/"
$	IF F$LOCATE(".",DDIR) .LT. F$LENGTH(DDIR) THEN GOTO EDITDDIR
$	DDIR = F$EDIT(DDIR,"TRIM")
$ NOEDITDDIR:
$	GENRESTR = "_" + F$EDIT(GENRE,"LOWERCASE")
$ GENREEDIT:
$	UPC = F$EDIT(F$EXTRACT(F$LOCATE("_",GENRESTR)+1,1,GENRESTR),"UPCASE")
$	GENRESTR['F$LOCATE("_",GENRESTR)+1,1] := "''UPC'"
$	GENRESTR['F$LOCATE("_",GENRESTR),1] := " "
$	IF F$LOCATE("_",GENRESTR) .LT. F$LENGTH(GENRESTR) THEN GOTO GENREEDIT
$	GENRESTR = F$EDIT(GENRESTR,"TRIM")
$	SUBGENRESTR = "_" + F$EDIT(SUBGENRE,"LOWERCASE")
$ SUBGENREEDIT:
$	UPC = F$EDIT(F$EXTRACT(F$LOCATE("_",SUBGENRESTR)+1,1,SUBGENRESTR),"UPCASE")
$	SUBGENRESTR['F$LOCATE("_",SUBGENRESTR)+1,1] := "''UPC'"
$	SUBGENRESTR['F$LOCATE("_",SUBGENRESTR),1] := " "
$	IF F$LOCATE("_",SUBGENRESTR) .LT. F$LENGTH(SUBGENRESTR) THEN GOTO SUBGENREEDIT
$	SUBGENRESTR = F$EDIT(SUBGENRESTR,"TRIM")
$	IF PRE_26
$	 THEN
$	  LP = "<TABLE CELLSPACING=""0"" CELLPADDING=""1"" BORDER=""0""><TR><TH ALIGN=""left"">Disc ID:</TH><TD ALIGN=""left"">"
$	  RP = "<BR></TD></TR></TABLE>"
$	ENDIF
$	GENRELINE = LP + GENRESTR + " -> " + SUBGENRESTR + " ''ID'" + RP
$	WRITE SYS$OUTPUT "  Converting ''ID' in ''SUBGENRESTR' genre."
$!
$! Setup command procedure with the info collected above for use by text editor
$!	
$	OPEN/WRITE 300 CIDX.EDT
$	WRITE 300 "FIND '     DO NOT EDIT: Generated by xmcd 2.'"
$	WRITE 300 "DELETE"
$	WRITE 300 "DELETE"
$	WRITE 300 "DELETE"
$	OPEN/WRITE 201 CIDX1.TMP
$	WRITE 201 "DO NOT EDIT: Generated by xmcd 3.3.2"
$	WRITE 201 "Copyright (C) 1993-2004  Ti Kan"
$	WRITE 201 "URL: http://www.amb.org/xmcd/ E-mail: xmcd@amb.org -->"
$	CLOSE 201
$	WRITE 300 "INCLUDE CIDX1.TMP"
$	WRITE 300 "FIND 'bkgnd.gif'
$	WRITE 300 "DELETE"
$	OPEN/WRITE 202 CIDX2.TMP
$	WRITE 202 "<BODY BGCOLOR=""#FFFFFF"" BACKGROUND=""file://localhost/''DDSK'/''DDIR'/discog/bkgnd.gif"">"
$	CLOSE 202
$	WRITE 300 "INCLUDE CIDX2.TMP"
$	WRITE 300 "FIND 'metalab.unc.edu'"
$	WRITE 300 "DELETE"
$	OPEN/WRITE 203 CIDX3.TMP
$	WRITE 203 "<A HREF=""http://www.amb.org/xmcd/"">"
$	CLOSE 203
$	WRITE 300 "INCLUDE CIDX3.TMP"
$	WRITE 300 "FIND 'xmcdlogo.gif'
$	WRITE 300 "DELETE"
$	OPEN/WRITE 204 CIDX4.TMP
$	WRITE 204 "<IMG SRC=""file://localhost/''DDSK'/''DDIR'/discog/xmcdlogo.gif"" ALT=""xmcd"" BORDER=""0""></A><P>"
$	CLOSE 204
$	WRITE 300 "INCLUDE CIDX4.TMP"
$	WRITE 300 "FIND 'Disc ID'"
$	WRITE 300 "DELETE"
$	OPEN/WRITE 205 CIDX5.TMP
$	WRITE 205 GENRELINE
$	CLOSE 205
$	WRITE 300 "INCLUDE CIDX5.TMP"
$	WRITE 300 "FIND '<H4>Local Discography'"
$	WRITE 300 "FIND '<LI>'"
$	WRITE 300 "DELETE REST"
$	OPEN/WRITE 206 CIDX6.TMP
$ LOCFIL:
$	LOCFIL = F$SEARCH("''THIS_DIR'*.*",1)
$	IF LOCFIL .EQS. "" THEN GOTO LFEND
$	LOCFIL = F$PARSE(LOCFIL,,,"NAME") + F$PARSE(LOCFIL,,,"TYPE")
$	WRITE 206 "<LI><A HREF=""file://localhost/''DDSK'/''DDIR'/discog/''GENRE'/''SUBGENRE'/''ID'/''LOCFIL'"">File: ''LOCFIL'</A></LI>"
$	GOTO LOCFIL
$ LFEND:
$	WRITE 206 "<LI><A HREF=""file://localhost/''DDSK'/''DDIR'/discog/index.html"">Main index</A>"
$	WRITE 206 "<LI><A HREF=""file://localhost/''DDSK'/''DDIR'/discog/''GENRE'/''SUBGENRE'/index.html"">''GENRESTR' -> ''SUBGENRESTR' index</A>"
$	WRITE 206 "<LI><A HREF=""file://localhost/''DDSK'/''DDIR'/discog/discog.html"">How to use Local Discography</A></LI>"
$	WRITE 206 "</UL>"
$	WRITE 206 "<H4>Links</H4>"
$	WRITE 206 "<P>"
$	WRITE 206 "<UL>"
$	WRITE 206 "<LI><A HREF=""http://www.amb.org/xmcd/"">Xmcd official web site</A></LI>"
$	WRITE 206 "<LI><A HREF=""http://www.cddb.com/"">CDDB - The #1 music info source</A></LI>"
$	WRITE 206 "<LI><A HREF=""http://www.yahoo.com/Entertainment/Music/Reviews/"">Yahoo! music reviews</A></LI>"
$	WRITE 206 "</UL>"
$	WRITE 206 "<HR>"
$	WRITE 206 "This directory: <B>''THIS_DIR'</B>"
$	WRITE 206 "</BODY>"
$	WRITE 206 "</HTML>"
$	CLOSE 206
$	WRITE 300 "INCLUDE CIDX6.TMP"
$	WRITE 300 "EXIT"
$	CLOSE 300
$!
$! And finally apply the changes to complete conversion to 3.x format
$!	
$	DEFINE/NOLOG SYS$INPUT SYS$COMMAND
$	DEFINE/NOLOG SYS$OUTPUT NLA0:
$	EDITTT/EDT/COMMAND=CIDX.EDT 'WFILE'
$	DELETE/NOCONFIRM CIDX*.TMP;0,CIDX.EDT;0
$	PFILE = WFILE - F$PARSE(WFILE,,,"VERSION")
$	PURGE/NOLOG 'PFILE'
$	DEASSIGN SYS$OUTPUT
$	GOTO CONVOLDIDX
$ CONV_DONE:
$	SET MESSAGE 'MESENV'
$	IF FILES_CVT .NE. 0
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  ''FILES_CVT' INDEX.HTML files been have converted to xmcd 3.x format."
$	 ELSE
$	  WRITE SYS$OUTPUT "''CR'''LF'  No index files found to convert to xmcd 3.x format"
$	ENDIF
$ EXIT:
$	WRITE SYS$OUTPUT "''BEL'''CR'''LF'  Installation of XMCD completed at ''F$CVTIME(,"ABSOLUTE",)'''CR'''LF'''CR'''LF'"
$	WRITE SYS$OUTPUT "  For information on advanced configuration please see the documentation files.''CR'''LF'"
$	WRITE SYS$OUTPUT "  Restart your X-session to have all changes take effect,"
$	WRITE SYS$OUTPUT "  then type XMCD at the command prompt to start the application.''CR'''LF'''CR'''LF'"
$ ENDEX:
$	SET DEF 'USEN'
