$!
$! @(#)seldrive.com	7.9 04/01/14
$!
$! CD drive device settings command procedure for OpenVMS
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
$!   This procedure will on behalf of the user apply the modifications needed
$!   in XMCD's configuration file DEVICE.CFG for making use of the CD drive's
$!   capabilities.
$!   It usually is being executed from inside INSTALLVMS.COM at the time XMCD 
$!   is being installed on your system.
$!   However, this procedure also can be run from the command prompt. 
$!   This for example may become necessary if the CD drive was replaced with
$!   a different model than used previously.
$!   If running this procedure from the command prompt and you don't know what
$!   CD drive model you are using with XMCD, you may pass the CD drive's VMS 
$!   device name to this procedure as P1 (f.i. @SELDRIVE $1$DKA400). VMS will
$!   tell you your drive's model name then if it is willing and able to reveal
$!   it.
$!   
$!   This procedure makes use of the drive configuration tables in the
$!   [.LIBDI_D.CFGTBL] subdirectory which contain vendor and drive specific 
$!   configuration data on numerous CD drives. Two menues are presented
$!   to chose the vendor and model name from.
$!   
$!   Parameters:
$!   ------------
$!   P1 - Optional, VMS device name of CD drive used with XMCD, f.i. $1$DKA400 
$!        Only used when procedure is run from the command prompt
$!   P2 - Reserved for use with INSTALLVMS.COM only.
$!   
$!   Restrictions:
$!   --------------
$!   
$!   This procedure cannot be used with pre-V7.0 VMS versions.
$!   Also, it will discard unsuitable versions of xmcd configuration
$!   tables and resultingly not apply any modification to DEVICE.CFG
$!   
$!   
$! Some basics first...
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
$	
$	MATCHTBLVERS = 6
$	
$	IF F$GETSYI("VERSION") .LES. "V7" 
$	 THEN 
$	  WRITE SYS$OUTPUT "''CR'''LF'%DRVCFG-F-VVMM, VMS version mismatch, version is not 7.0 or later''CR'''LF'"
$	  EXIT
$	ENDIF
$	IF P2 .EQS. "NO_ENTRY_TEXT" THEN GOTO NO_P1
$!
$! Prepare user having to specify the CD drive model.
$!
$	WRITE SYS$OUTPUT "''ESC'[0;0H''ESC'[2J''CR'''LF'  Configuration of device settings"
$	WRITE SYS$OUTPUT "  ---------------------------------"
$	WRITE SYS$OUTPUT "''CR'''LF'  You now need to specify your CD drive model."
$	WRITE SYS$OUTPUT "  This information will be used to apply changes to xmcd's device"
$	WRITE SYS$OUTPUT "  configuration file which will be necessary to make use of all your"
$	WRITE SYS$OUTPUT "  CD drive's capabilities."
$	WRITE SYS$OUTPUT ""
$	
$	IF P1 .EQS. "" THEN GOTO ENTRY_TEXT_DONE
$	IF .NOT. F$GETDVI(P1,"EXISTS")
$	 THEN 
$	  WRITE SYS$OUTPUT "''CR'''LF'%SYSTEM-W-NOSUCHDEV, no such device available - ''P1'"
$	  EXIT
$	ENDIF
$	DRVMDL = F$GETDVI(P1,"DEVICE_TYPE_NAME")
$	
$	WRITE SYS$OUTPUT "  Your CD drive ''P1' is considered to be a ''ESC'[7m ''DRVMDL' ''ESC'[0m ."
$	WRITE SYS$OUTPUT ""
$	WRITE SYS$OUTPUT "  Remember this model type when next being prompted with the drive"
$	WRITE SYS$OUTPUT "  manufacturer and model type menues.''CR'''LF'"
$
$ ENTRY_TEXT_DONE:
$
$	INQUIRE/NOPUNCTATION DUMMY "''ESC'[28C''ESC'[7mPress Return to continue''ESC'[0m"
$	WRITE SYS$OUTPUT "''ESC'[1A''ESC'[2K"
$
$ NO_P1:
$	
$!	
$! Check whether procedure is run in correct environment.
$!	
$	IF F$SEARCH("DEVICE.CFG") .EQS. ""
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  %DRVCFG-F-NOCFGFIL, No device configuration file DEVICE.CFG found!"
$	  WRITE SYS$OUTPUT "  Make sure you are running this procedure from your xmcd top-level directory!''CR'''LF'"
$	  GOTO ERR_EXIT
$	ENDIF
$!	
$! Determine available vendor specific configuration files.
$!	
$	WRITE SYS$OUTPUT "''CR'''LF'  %DRVCFG-S-GACD, Gathering available drive configuration data."
$	WRITE SYS$OUTPUT "  This may take a few seconds.''CR'''LF'''LF'"
$	VCNT = 0
$	DCNT = 0
$ GET_VENDOR:
$	VEND = F$SEARCH("[.LIBDI_D.CFGTBL]*.")
$	IF VEND .EQS. "" THEN GOTO GOT_VENDORS
$	SEARCH/NOWARNING/NOOUT 'VEND' "tblver=''MATCHTBLVERS'"
$	IF $SEVERITY .NE. 1 THEN GOTO GET_VENDOR
$	VCNT = VCNT + 1
$	VEND'VCNT' = F$PARSE(VEND,,,"NAME")
$       PIPE SEARCH 'VEND' "alias" | (READ SYS$INPUT RESULT ; DEFINE/JOB ALINAM &RESULT)
$       VENDS'VCNT' = F$EDIT(F$ELEMENT(1,"=",F$TRNLNM("ALINAM")),"TRIM")
$       DEASS/JOB ALINAM
$	FNDSTR = VENDS'VCNT'
$	WRITE SYS$OUTPUT "''ESC'[1A''ESC'[0K  Found vendor: ''FNDSTR'"
$	GOTO GET_VENDOR
$ GOT_VENDORS:
$	IF VCNT .EQ. 0
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  %DRVCFG-F-NOCFGFIL, No drive configuration files found!"
$	  GOTO ERR_EXIT
$	ENDIF
$!
$! Display vendor name menu
$!
$	WRITE SYS$OUTPUT "''ESC'[0;0H''ESC'[2J"
$	WRITE SYS$OUTPUT "''ESC'#6Drive Manufacturer Menu"
$	WRITE SYS$OUTPUT "-------------------------------------------------------------------------------"
$	RCNT = 1
$	LCNT = 4
$	FILL = " "
$ DISP_VENDORS:
$	IF LCNT .EQ. 21 
$	 THEN 
$	  LCNT = 4
$	  RCNT = RCNT + 33
$	ENDIF
$	DCNT = DCNT + 1
$	IF DCNT .GT. VCNT THEN GOTO DISP_VEND_DONE
$	LCNT = LCNT + 1
$	IF DCNT .GE. 10 THEN FILL = ""
$	VENDSTR = VENDS'DCNT'
$	WRITE SYS$OUTPUT "''ESC'[''LCNT';''RCNT'H''FILL'''DCNT') ''VENDSTR'"
$	GOTO DISP_VENDORS
$	
$ DISP_VEND_DONE:
$	
$	OPEN/READ 0 SYS$COMMAND
$	READ/END=ERR_EXIT/PROMPT="''ESC'[23;0H''ESC'[0KEnter your CD drive's manufacturer number (or press ^Z to exit): " 0 DCHOICE
$	CLOSE 0
$	IF DCHOICE .LT. 1 .OR. DCHOICE .GT. DCNT-1 THEN GOTO DISP_VEND_DONE
$	
$	FILE = VEND'DCHOICE'
$	OPEN/READ 1 [.LIBDI_D.CFGTBL]'FILE'.
$!
$! Determine available drive models of selected vendor
$!
$	DRCNT = 0
$ READDEVFIL:	
$	READ/ERROR=ENDREADDEV 1 LINE
$	STR = F$EDIT(F$EXTRACT(0,1,LINE),"COLLAPSE")
$	IF STR .EQS. "#" .OR. STR .EQS. "" THEN GOTO READDEVFIL
$	DRCNT = DRCNT + 1
$	DRIVE'DRCNT' = F$ELEMENT(0,":",LINE)
$	GOTO READDEVFIL
$ ENDREADDEV:
$	CLOSE 1
$	VCNT = 0
$	IF DRCNT .EQ. 0
$	 THEN
$	  WRITE SYS$OUTPUT "''CR'''LF'  %DRVCFG-F-NODRVPAR, No ''vendstr' drives found!"
$	  GOTO ERR_EXIT
$	ENDIF
$!
$! Display model name menue
$!
$	MODELMANU = VENDS'DCHOICE'
$	WRITE SYS$OUTPUT "''ESC'[0;0H''ESC'[2J"
$	WRITE SYS$OUTPUT "  ''MODELMANU' Drive Model Menu"
$	WRITE SYS$OUTPUT "  -----------------------------------------------------------------------------"
$	RCNT = 3
$	LCNT = 4
$	FILL = " "
$ DISP_MODELS:
$	IF LCNT .EQ. 21 
$	 THEN 
$	  LCNT = 4
$	  RCNT = RCNT + 18
$	ENDIF
$	VCNT = VCNT + 1
$	IF VCNT .GT. DRCNT THEN GOTO DISP_MODEL_DONE
$	LCNT = LCNT + 1
$	IF VCNT .GE. 10 THEN FILL = ""
$	MODELSTR = DRIVE'VCNT'
$	WRITE SYS$OUTPUT "''ESC'[''LCNT';''RCNT'H''FILL'''VCNT') ''MODELSTR'"
$	GOTO DISP_MODELS
$	
$ DISP_MODEL_DONE:
$	
$	OPEN/READ 2 SYS$COMMAND
$	READ/END=ERR_EXIT/PROMPT="''ESC'[23;3H''ESC'[0KEnter your CD drive's model number (or press ^Z to exit): " 2 MCHOICE
$	CLOSE 2
$	IF MCHOICE .LT. 1 .OR. MCHOICE .GT. VCNT-1 THEN GOTO DISP_MODEL_DONE
$	MODEL = DRIVE'MCHOICE'
$!
$! Setup params to be updated in DEVICE.CFG
$!
$	OPEN/READ 3 [.LIBDI_D.CFGTBL]'FILE'.
$ READDEVCAP:	
$	READ/ERROR=ENDREADCAP 3 LINE
$	STR = F$EDIT(F$EXTRACT(0,1,LINE),"COLLAPSE")
$	IF STR .EQS. "#" .OR. STR .EQS. "" THEN GOTO READDEVCAP
$	DRIVE = F$ELEMENT(0,":",LINE)
$	IF DRIVE .EQS. MODEL THEN GOTO ENDREADCAP
$	GOTO READDEVCAP
$ ENDREADCAP:
$	CLOSE 3
$	
$	WRITE SYS$OUTPUT "''CR'''LF'  %DRVCFG-I-CHKCHG, Checking required modifications to DEVICE.CFG"
$	
$	mode = F$ELEMENT(1,":",LINE)
$! 
$! For non-SCSI drives (where the cfgtbl mode field is 0), the
$! deviceInterfaceMethod parameter is set in a platform-dependent manner
$! (See the comments of this parameter in device.cfg).  For VMS, the
$! deviceInterfaceMethod should be set to 0 even in this case because
$! non-SCSI (IDE) drives are supported via SCSI-emulation in VMS's DQdriver.
$!	
$	OPEN/WRITE 101 CHANGE.1
$	WRITE 101 "deviceInterfaceMethod:  0"
$	CLOSE 101
$       OPEN/WRITE 201 MOD1.EDT	
$	WRITE 201 "FIND 'deviceInterfaceMethod:'"
$	WRITE 201 "DELETE"
$	WRITE 201 "INCLUDE CHANGE.1"
$	WRITE 201 "EXIT"
$	CLOSE 201
$
$	vcode = F$ELEMENT(2,":",LINE)
$	OPEN/WRITE 102 CHANGE.2
$	WRITE 102 "driveVendorCode:        ''vcode'"
$	CLOSE 102
$       OPEN/WRITE 202 MOD2.EDT	
$	WRITE 202 "FIND 'driveVendorCode:'"
$	WRITE 202 "DELETE"
$	WRITE 202 "INCLUDE CHANGE.2"
$	WRITE 202 "EXIT"
$	CLOSE 202
$
$	pmsf = F$ELEMENT(3,":",LINE)
$	IF pmsf
$	 THEN 
$	  pmsf = "True"
$	 ELSE
$	  pmsf = "False"
$	ENDIF
$	OPEN/WRITE 103 CHANGE.3
$	WRITE 103 "playAudioMSFSupport:    ''pmsf'"
$	CLOSE 103
$       OPEN/WRITE 203 MOD3.EDT	
$	WRITE 203 "FIND 'playAudioMSFSupport:'"
$	WRITE 203 "DELETE"
$	WRITE 203 "INCLUDE CHANGE.3"
$	WRITE 203 "EXIT"
$	CLOSE 203
$
$	p12 = F$ELEMENT(4,":",LINE)
$       IF p12
$        THEN
$         p12 = "True"
$        ELSE
$         p12 = "False"
$       ENDIF
$	OPEN/WRITE 104 CHANGE.4
$	WRITE 104 "playAudio12Support:     ''p12'"
$	CLOSE 104
$       OPEN/WRITE 204 MOD4.EDT	
$	WRITE 204 "FIND 'playAudio12Support:'"
$	WRITE 204 "DELETE"
$	WRITE 204 "INCLUDE CHANGE.4"
$	WRITE 204 "EXIT"
$	CLOSE 204
$
$	p10 = F$ELEMENT(5,":",LINE)
$       IF p10
$        THEN
$         p10 = "True"
$        ELSE
$         p10 = "False"
$       ENDIF
$	OPEN/WRITE 105 CHANGE.5
$	WRITE 105 "playAudio10Support:     ''p10'"
$	CLOSE 105
$       OPEN/WRITE 205 MOD5.EDT	
$	WRITE 205 "FIND 'playAudio10Support:'"
$	WRITE 205 "DELETE"
$	WRITE 205 "INCLUDE CHANGE.5"
$	WRITE 205 "EXIT"
$	CLOSE 205
$
$	pti = F$ELEMENT(6,":",LINE)
$       IF pti
$        THEN
$         pti = "True"
$        ELSE
$         pti = "False"
$       ENDIF
$	OPEN/WRITE 106 CHANGE.6
$	WRITE 106 "playAudioTISupport:     ''pti'"
$	CLOSE 106
$       OPEN/WRITE 206 MOD6.EDT	
$	WRITE 206 "FIND 'playAudioTISupport:'"
$	WRITE 206 "DELETE"
$	WRITE 206 "INCLUDE CHANGE.6"
$	WRITE 206 "EXIT"
$	CLOSE 206
$
$	ld = F$ELEMENT(7,":",LINE)
$       IF ld
$        THEN
$         ld = "True"
$        ELSE
$         ld = "False"
$       ENDIF
$	OPEN/WRITE 107 CHANGE.7
$	WRITE 107 "loadSupport:            ''ld'"
$	CLOSE 107
$       OPEN/WRITE 207 MOD7.EDT	
$	WRITE 207 "FIND 'loadSupport:'"
$	WRITE 207 "DELETE"
$	WRITE 207 "INCLUDE CHANGE.7"
$	WRITE 207 "EXIT"
$	CLOSE 207
$
$	ej = F$ELEMENT(8,":",LINE)
$       IF ej
$        THEN
$         ej = "True"
$        ELSE
$         ej = "False"
$       ENDIF
$	OPEN/WRITE 108 CHANGE.8
$	WRITE 108 "ejectSupport:           ''ej'"
$	CLOSE 108
$       OPEN/WRITE 208 MOD8.EDT	
$	WRITE 208 "FIND 'ejectSupport:'"
$	WRITE 208 "DELETE"
$	WRITE 208 "INCLUDE CHANGE.8"
$	WRITE 208 "EXIT"
$	CLOSE 208
$
$	mdbd = F$ELEMENT(9,":",LINE)
$       IF mdbd 
$        THEN
$         mdbd = "True"
$        ELSE
$         mdbd = "False"
$       ENDIF
$	OPEN/WRITE 109 CHANGE.9
$	WRITE 109 "modeSenseSetDBD:        ''mdbd'"
$	CLOSE 109
$       OPEN/WRITE 209 MOD9.EDT	
$	WRITE 209 "FIND 'modeSenseSetDBD:'"
$	WRITE 209 "DELETE"
$	WRITE 209 "INCLUDE CHANGE.9"
$	WRITE 209 "EXIT"
$	CLOSE 209
$
$	m10 = F$ELEMENT(10,":",LINE)
$       IF m10
$        THEN
$         m10 = "True"
$        ELSE
$         m10 = "False"
$       ENDIF
$	OPEN/WRITE 110 CHANGE.10
$	WRITE 110 "modeSenseUse10Byte:     ''m10'"
$	CLOSE 110
$       OPEN/WRITE 210 MOD10.EDT	
$	WRITE 210 "FIND 'modeSenseUse10Byte:'"
$	WRITE 210 "DELETE"
$	WRITE 210 "INCLUDE CHANGE.10"
$	WRITE 210 "EXIT"
$	CLOSE 210
$
$	vctl = F$ELEMENT(11,":",LINE)
$	VOLCTL = "False"
$	IF vctl .EQ. 1 .OR. vctl .EQ. 3 .OR. vctl .EQ. 5 .OR. vctl .EQ. 7 THEN VOLCTL = "True"
$	OPEN/WRITE 111A CHANGE.11A
$	WRITE 111A "volumeControlSupport:   ''VOLCTL'"
$	CLOSE 111A
$       OPEN/WRITE 211A MOD11A.EDT	
$	WRITE 211A "FIND 'volumeControlSupport:'"
$	WRITE 211A "DELETE"
$	WRITE 211A "INCLUDE CHANGE.11A"
$	WRITE 211A "EXIT"
$	CLOSE 211A
$
$	BALCTL = "False"
$	IF vctl .EQ. 2 .OR. vctl .EQ. 3 .OR. vctl .EQ. 6 .OR. vctl .EQ. 7 THEN BALCTL = "True"
$	OPEN/WRITE 111B CHANGE.11B
$	WRITE 111B "balanceControlSupport:  ''BALCTL'"
$	CLOSE 111B
$       OPEN/WRITE 211B MOD11B.EDT	
$	WRITE 211B "FIND 'balanceControlSupport:'"
$	WRITE 211B "DELETE"
$	WRITE 211B "INCLUDE CHANGE.11B"
$	WRITE 211B "EXIT"
$	CLOSE 211B
$
$	ROUCTL = "False"
$	IF vctl .EQ. 4 .OR. vctl .EQ. 5 .OR. vctl .EQ. 6 .OR. vctl .EQ. 7 THEN ROUCTL = "True"
$	OPEN/WRITE 111C CHANGE.11C
$	WRITE 111C "channelRouteSupport:    ''ROUCTL'"
$	CLOSE 111C
$       OPEN/WRITE 211C MOD11C.EDT	
$	WRITE 211C "FIND 'channelRouteSupport:'"
$	WRITE 211C "DELETE"
$	WRITE 211C "INCLUDE CHANGE.11C"
$	WRITE 211C "EXIT"
$	CLOSE 211C
$
$	vbase = F$ELEMENT(12,":",LINE)
$	OPEN/WRITE 112 CHANGE.12
$	WRITE 112 "scsiAudioVolumeBase:    ''vbase'"
$	CLOSE 112
$       OPEN/WRITE 212 MOD12.EDT	
$	WRITE 212 "FIND 'scsiAudioVolumeBase:'"
$	WRITE 212 "DELETE"
$	WRITE 212 "INCLUDE CHANGE.12"
$	WRITE 212 "EXIT"
$	CLOSE 212
$
$	vtaper = F$ELEMENT(13,":",LINE)
$	OPEN/WRITE 113 CHANGE.13
$	WRITE 113 "volumeControlTaper:     ''vtaper'"
$	CLOSE 113
$       OPEN/WRITE 213 MOD13.EDT	
$	WRITE 213 "FIND 'volumeControlTaper:'"
$	WRITE 213 "DELETE"
$	WRITE 213 "INCLUDE CHANGE.13"
$	WRITE 213 "EXIT"
$	CLOSE 213
$
$	paus = F$ELEMENT(14,":",LINE)
$       IF paus
$        THEN
$         paus = "True"
$        ELSE
$         paus = "False"
$       ENDIF
$	OPEN/WRITE 114 CHANGE.14
$	WRITE 114 "pauseResumeSupport:     ''paus'"
$	CLOSE 114
$       OPEN/WRITE 214 MOD14.EDT	
$	WRITE 214 "FIND 'pauseResumeSupport:'"
$	WRITE 214 "DELETE"
$	WRITE 214 "INCLUDE CHANGE.14"
$	WRITE 214 "EXIT"
$	CLOSE 214
$
$	strict = F$ELEMENT(15,":",LINE)
$       IF strict
$        THEN
$         strict = "True"
$        ELSE
$         strict = "False"
$       ENDIF
$	OPEN/WRITE 115 CHANGE.15
$	WRITE 115 "strictPauseResume:      ''strict'"
$	CLOSE 115
$       OPEN/WRITE 215 MOD15.EDT	
$	WRITE 215 "FIND 'strictPauseResume:'"
$	WRITE 215 "DELETE"
$	WRITE 215 "INCLUDE CHANGE.15"
$	WRITE 215 "EXIT"
$	CLOSE 215
$
$	ppp = F$ELEMENT(16,":",LINE)
$       IF ppp
$        THEN
$         ppp = "True"
$        ELSE
$         ppp = "False"
$       ENDIF
$	OPEN/WRITE 116 CHANGE.16
$	WRITE 116 "playPausePlay:          ''ppp'"
$	CLOSE 116
$       OPEN/WRITE 216 MOD16.EDT	
$	WRITE 216 "FIND 'playPausePlay:'"
$	WRITE 216 "DELETE"
$	WRITE 216 "INCLUDE CHANGE.16"
$	WRITE 216 "EXIT"
$	CLOSE 216
$
$	lk = F$ELEMENT(17,":",LINE)
$       IF lk
$        THEN
$         lk = "True"
$        ELSE
$         lk = "False"
$       ENDIF
$	OPEN/WRITE 117 CHANGE.17
$	WRITE 117 "caddyLockSupport:       ''lk'"
$	CLOSE 117
$       OPEN/WRITE 217 MOD17.EDT	
$	WRITE 217 "FIND 'caddyLockSupport:'"
$	WRITE 217 "DELETE"
$	WRITE 217 "INCLUDE CHANGE.17"
$	WRITE 217 "EXIT"
$	CLOSE 217
$
$	cp = F$ELEMENT(18,":",LINE)
$       IF cp
$        THEN
$         cp = "True"
$        ELSE
$         cp = "False"
$       ENDIF
$	OPEN/WRITE 118 CHANGE.18
$	WRITE 118 "curposFormat:           ''cp'"
$	CLOSE 118
$       OPEN/WRITE 218 MOD18.EDT	
$	WRITE 218 "FIND 'curposFormat:'"
$	WRITE 218 "DELETE"
$	WRITE 218 "INCLUDE CHANGE.18"
$	WRITE 218 "EXIT"
$	CLOSE 218
$
$	tur = F$ELEMENT(19,":",LINE)
$       IF tur
$        THEN
$         tur = "True"
$        ELSE
$         tur = "False"
$       ENDIF
$	OPEN/WRITE 119 CHANGE.19
$	WRITE 119 "noTURWhenPlaying:       ''tur'"
$	CLOSE 119
$       OPEN/WRITE 219 MOD19.EDT	
$	WRITE 219 "FIND 'noTURWhenPlaying:'"
$	WRITE 219 "DELETE"
$	WRITE 219 "INCLUDE CHANGE.19"
$	WRITE 219 "EXIT"
$	CLOSE 219
$
$	chg = F$ELEMENT(20,":",LINE)
$	OPEN/WRITE 120 CHANGE.20
$	WRITE 120 "mediumChangeMethod:     ''chg'"
$	CLOSE 120
$       OPEN/WRITE 220 MOD20.EDT	
$	WRITE 220 "FIND 'mediumChangeMethod:'"
$	WRITE 220 "DELETE"
$	WRITE 220 "INCLUDE CHANGE.20"
$	WRITE 220 "EXIT"
$	CLOSE 220
$
$	ndisc = F$ELEMENT(21,":",LINE)
$	OPEN/WRITE 121 CHANGE.21
$	WRITE 121 "numDiscs:               ''ndisc'"
$	CLOSE 121
$       OPEN/WRITE 221 MOD21.EDT	
$	WRITE 221 "FIND 'numDiscs:'"
$	WRITE 221 "DELETE"
$	WRITE 221 "INCLUDE CHANGE.21"
$	WRITE 221 "EXIT"
$	CLOSE 221
$
$	sp = F$ELEMENT(22,":",LINE)
$	OPEN/WRITE 122 CHANGE.22
$	WRITE 122 "spinUpInterval:         ''sp'"
$	CLOSE 122
$       OPEN/WRITE 222 MOD22.EDT	
$	WRITE 222 "FIND 'spinUpInterval:'"
$	WRITE 222 "DELETE"
$	WRITE 222 "INCLUDE CHANGE.22"
$	WRITE 222 "EXIT"
$	CLOSE 222
$
$	msel = F$ELEMENT(23,":",LINE)
$       IF msel
$        THEN
$         msel = "True"
$        ELSE
$         msel = "False"
$       ENDIF
$	OPEN/WRITE 123 CHANGE.23
$	WRITE 123 "cddaScsiModeSelect:     ''msel'"
$	CLOSE 123
$       OPEN/WRITE 223 MOD23.EDT	
$	WRITE 223 "FIND 'cddaScsiModeSelect:'"
$	WRITE 223 "DELETE"
$	WRITE 223 "INCLUDE CHANGE.23"
$	WRITE 223 "EXIT"
$	CLOSE 223
$
$	dens = F$ELEMENT(24,":",LINE)
$	OPEN/WRITE 124 CHANGE.24
$	WRITE 124 "cddaScsiDensity:        ''dens'"
$	CLOSE 124
$       OPEN/WRITE 224 MOD24.EDT	
$	WRITE 224 "FIND 'cddaScsiDensity:'"
$	WRITE 224 "DELETE"
$	WRITE 224 "INCLUDE CHANGE.24"
$	WRITE 224 "EXIT"
$	CLOSE 224
$
$	rdcmd = F$ELEMENT(25,":",LINE)
$	OPEN/WRITE 125 CHANGE.25
$	WRITE 125 "cddaScsiReadCommand:    ''rdcmd'"
$	CLOSE 125
$       OPEN/WRITE 225 MOD25.EDT	
$	WRITE 225 "FIND 'cddaScsiReadCommand:'"
$	WRITE 225 "DELETE"
$	WRITE 225 "INCLUDE CHANGE.25"
$	WRITE 225 "EXIT"
$	CLOSE 225
$
$	be = F$ELEMENT(26,":",LINE)
$       IF be
$        THEN
$         be = "True"
$        ELSE
$         be = "False"
$       ENDIF
$	OPEN/WRITE 126 CHANGE.26
$	WRITE 126 "cddaDataBigEndian:      ''be'"
$	CLOSE 126
$       OPEN/WRITE 226 MOD26.EDT	
$	WRITE 226 "FIND 'cddaDataBigEndian:'"
$	WRITE 226 "DELETE"
$	WRITE 226 "INCLUDE CHANGE.26"
$	WRITE 226 "EXIT"
$	CLOSE 226
$!
$! OpenVMS CDDA defaults
$!
$	OPEN/WRITE 1000 CHANGE.1000
$	WRITE 1000 "cddaMethod:             2"
$	CLOSE 1000
$       OPEN/WRITE 2000 MOD2000.EDT	
$	WRITE 2000 "FIND 'cddaMethod:'"
$	WRITE 2000 "DELETE"
$	WRITE 2000 "INCLUDE CHANGE.1000"
$	WRITE 2000 "EXIT"
$	CLOSE 2000
$	
$	OPEN/WRITE 1001 CHANGE.1001
$	WRITE 1001 "cddaReadMethod:         1"
$	CLOSE 1001
$       OPEN/WRITE 2001 MOD2001.EDT	
$	WRITE 2001 "FIND 'cddaReadMethod:'"
$	WRITE 2001 "DELETE"
$	WRITE 2001 "INCLUDE CHANGE.1001"
$	WRITE 2001 "EXIT"
$	CLOSE 2001
$	
$	OPEN/WRITE 1002 CHANGE.1002
$	WRITE 1002 "cddaWriteMethod:        7"
$	CLOSE 1002
$       OPEN/WRITE 2002 MOD2002.EDT	
$	WRITE 2002 "FIND 'cddaWriteMethod:'"
$	WRITE 2002 "DELETE"
$	WRITE 2002 "INCLUDE CHANGE.1002"
$	WRITE 2002 "EXIT"
$	CLOSE 2002
$	
$!
$! Display some informational messages and apply changes to DEVICE.CFG
$!
$	
$	WRITE SYS$OUTPUT "  %DRVCFG-I-COPY, Saving default DEVICE.CFG as DEVICE_ORIG.CFG"
$	COPYY DEVICE.CFG DEVICE_ORIG.CFG
$	WRITE SYS$OUTPUT "  %DRVCFG-I-APPCHG, Applying changes to DEVICE.CFG"
$       DEFINE/NOLOG SYS$INPUT SYS$COMMAND
$       DEFINE/NOLOG SYS$OUTPUT NLA0:
$       EDITTT/EDT/COMMAND=MOD1.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD2.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD3.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD4.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD5.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD6.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD7.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD8.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD9.EDT   DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD10.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD11A.EDT DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD11B.EDT DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD11C.EDT DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD12.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD13.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD14.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD15.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD16.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD17.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD18.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD19.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD20.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD21.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD22.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD23.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD24.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD25.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD26.EDT  DEVICE.CFG
$	
$       EDITTT/EDT/COMMAND=MOD2000.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD2001.EDT  DEVICE.CFG
$       EDITTT/EDT/COMMAND=MOD2002.EDT  DEVICE.CFG
$	
$       DEASSIGN SYS$OUTPUT
$       DEASSIGN SYS$INPUT
$	WRITE SYS$OUTPUT "  %DRVCFG-I-CLNUP, Cleaning up temporary files..."
$       DELETE/NOCONFIRM MOD*.EDT;*,CHANGE.*;*
$	PURGE DEVICE.CFG
$	RENAME DEVICE.CFG DEVICE.CFG;1
$	
$	WRITE SYS$OUTPUT "''CR'''LF'  %DRVCFG-S-SUCUPD, DEVICE.CFG successfully updated."
$	GOTO EXIT
$	
$ ERR_EXIT:
$	WRITE SYS$OUTPUT "''CR'''LF'  Automatic configuration of DEVICE.CFG _not_ done.''CR'''LF'"
$	WRITE SYS$OUTPUT "  You need to either re-run this procedure or edit your DEVICE.CFG file "
$	WRITE SYS$OUTPUT "  manually to make use of all your CD drive's capabilities.''CR'''LF'"
$	IF P2 .NES. "NO_ENTRY_TEXT" THEN GOTO EXIT
$	INQUIRE/NOPUNCTATION DUMMY "''ESC'[28C''ESC'[7mPress Return to continue''ESC'[0m"
$
$ EXIT:
$	EXIT
$	
