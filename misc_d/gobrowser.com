$!
$! @(#)gobrowser.com	6.18 04/01/14
$!
$! Web browser spawner command procedure for OpenVMS.
$! Currently supports the Mozilla, Mosaic and Netscape browsers.
$! See the INSTALL.VMS file for details.
$! OpenVMS versions: 6.0 and later, tested with 7.2 and 7.3 on VAX(tm) machines
$!                   and 7.2-1 as well as 7.3-1 on Alpha(tm) machines
$!
$!    xmcd  - Motif(R) CD Audio Player/Ripper
$!    cda   - Command-line CD Audio Player/Ripper
$!    libdi - CD Audio Device Interface Library
$!
$!    Copyright (C) 1993-2004  Ti Kan
$!    E-mail: xmcd@amb.org
$!    Contributing author: Michael Monscheuer
$!    E-mail: M.Monscheuer@t-online.de
$!
$!    This program is free software; you can redistribute it and/or modify
$!    it under the terms of the GNU General Public License as published by
$!    the Free Software Foundation; either version 2 of the License, or
$!    (at your option) any later version.
$!
$!    This program is distributed in the hope that it will be useful,
$!    but WITHOUT ANY WARRANTY; without even the implied warranty of
$!    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
$!    GNU General Public License for more details.
$!
$!    You should have received a copy of the GNU General Public License
$!    along with this program; if not, write to the Free Software
$!    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
$!
$!		
$! No way without the "Change Mode Kernel" privilege, CMKRNL is needed for SDA 
$! to trace a process' channels (in this case display and mailbox devices). If 
$! user doesn't have CMKRNL set, check whether it can be set. If so, set it or
$! if not, exit reporting an error to xmcd.
$!
$	IF .NOT. F$PRIVILEGE("CMKRNL")
$	 THEN
$	  PRIVS = F$GETJPI("","AUTHPRIV")
$	  IF F$LOCATE("CMKRNL",PRIVS) .EQ. F$LENGTH(PRIVS) 
$	   THEN
$	    IF F$LOCATE("SETPRV",PRIVS) .EQ. F$LENGTH(PRIVS) 
$	     THEN
$	      EXIT
$	     ELSE
$	      SET PROCESS/PRIVILEGE=SETPRV
$	      SET PROCESS/PRIVILEGE=CMKRNL
$	    ENDIF
$	   ELSE
$	    SET PROCESS/PRIVILEGE=CMKRNL
$	  ENDIF
$	ENDIF
$!
$! Define a default browser if not already done (in user's LOGIN.COM). 
$! Mozilla was chosen as the default browser for the Alpha architecture 
$! because it's the most up-to-date one which can cope with contemporary
$! HTML code much better than Mosaic and Netscape do.
$! Unfortunately, Mozilla is not available for the VAX architecture.
$! Mosaic was chosen as the default browser for the VAX architecture (and in
$! case Mozilla is not installed on Alpha) because it's remarkably
$! faster than Netscape with executing subsequently issued remote control 
$! commands.
$! If it is prefered to only view pictures with XV, XV can be defined as the
$! default viewer for xmcd (in user's LOGIN.COM - see INSTALL.VMS).
$! If a default viewer is not defined in the user's LOGIN.COM, the
$! default browser will be used.
$!
$	IF F$TRNLNM("XMCD_BROWSER") .EQS. "" 
$	 THEN 
$	  IF F$GETSYI("ARCH_NAME") .EQS. "VAX"
$	   THEN
$	    XMCD_BROWSER = "MOSAIC"
$	   ELSE
$	    XMCD_BROWSER = "MOZILLA"
$	  ENDIF
$	 ELSE
$	  XMCD_BROWSER = F$TRNLNM("XMCD_BROWSER") 
$	ENDIF
$
$	IF F$TRNLNM("XMCD_VIEWER") .EQS. ""
$	 THEN
$	  XMCD_VIEWER = F$TRNLNM("XMCD_BROWSER")
$	 ELSE
$	  XMCD_VIEWER = F$TRNLNM("XMCD_VIEWER")
$	ENDIF
$! 
$! Remove quotes from P1 and convert it to lowercase for use with
$! Netscape and Mosaic (some URL's don't work if in uppercase)
$! 
$	P1 = P1 - "'" - "'"
$	P1 = F$EDIT(P1,"LOWERCASE")
$! 
$! 
$! Check whether to browse the 'Net or local discography
$! and set up appropriate command for the browser (or viewer, if any).
$! 
$	IF F$LOCATE(".discog.",P1) .NE. F$LENGTH(P1) .AND. F$LOCATE("xmcd",P1) .NE. F$LENGTH(P1)
$	 THEN
$	  IF XMCD_VIEWER .EQS. "MOSAIC"
$	   THEN 
$	    DEFINE/GROUP/NOLOG XMCD_BROWSER MOSAIC
$	    GOTO NO_XV	  
$	  ENDIF
$	  IF XMCD_VIEWER .EQS. "NETSCAPE" 
$	   THEN 
$	    DEFINE/GROUP/NOLOG XMCD_BROWSER NETSCAPE
$	    GOTO NO_XV	  
$	  ENDIF
$	  IF XMCD_VIEWER .EQS. "MOZILLA" 
$	   THEN 
$	    DEFINE/GROUP/NOLOG XMCD_BROWSER MOZILLA
$	    GOTO NO_XV	  
$	  ENDIF
$	  IF XMCD_VIEWER .EQS. "XV"
$	   THEN
$	    DISK = F$PARSE("''P1'",,,"DEVICE")
$	    DIR  = F$PARSE("''P1'",,,"DIRECTORY")
$	    DIR  = F$ELEMENT(0,".",DIR) + "]"
$	    P1 = P1 - "index.html"
$	    @'DISK''DIR'STARTVIEWER.COM 'P1'
$	    EXIT %X10000000
$	  ENDIF
$	ENDIF
$
$ NO_XV:
$	
$!
$! Branch if not running in detached mode
$!
$	IF F$MODE() .NES. "OTHER"
$	 THEN
$!
$! Branch if trying to control browser remotely and initialize flag
$!
$	  IF XMCD_BROWSER .EQS. "MOSAIC"
$	   THEN
$	    BROWSER_REMOTE = 0
$!
$! Check whether Mosaic perhaps is already running in remote node ...
$!
$	    IF F$TRNLNM("MOSAIC_MAILBOX") .NES. ""
$	     THEN
$	      IF F$GETDVI("''F$TRNLNM("MOSAIC_MAILBOX")'","EXISTS")
$	       THEN
$	        BROWSER_REMOTE = 1
$		MBA = F$TRNLNM("MOSAIC_MAILBOX")
$!
$! ... and if so branch for a quick execution (if MOSAIC still is XMCD_BROWSER, 
$! if not, do not branch and reset flag), the rest was already checked.
$!
$		IF XMCD_BROWSER .EQS. "MOSAIC" THEN GOTO GOT_STATUS
$	        BROWSER_REMOTE = 0
$	      ENDIF
$	    ENDIF
$!
$! Some definitions first
$!
$	    VVER = F$EDIT(F$GETSYI("VERSION"),"COLLAPSE")
$	    I_AM = F$EDIT(F$GETJPI("","UIC"),"COLLAPSE")
$	    NODENAME = F$GETSYI("NODENAME")
$	    MESENV = F$ENVIRONMENT("MESSAGE")
$!
$! Just in case somebody tries to execute this procedure from an unsuitable
$! terminal...
$!
$	    DEFINE/NOLOG SYS$OUTPUT NLA0:
$	    SHOW DISPLAY/SYMBOL
$	    DSPSTAT = $SEVERITY 
$	    DEASSIGN SYS$OUTPUT
$	    IF .NOT. DSPSTAT THEN EXIT
$	    DSPLNODE = DECW$DISPLAY_NODE
$!
$! Now collect information on running processes (necessary to get hold
$! of display and mailbox devices "owned" by user running this procedure)
$!
$	    OPEN/WRITE 0 PID_LIS.TMP
$	    CTX = ""
$ GET_PROCS:
$	    PID = F$PID(CTX)
$	    IF PID .EQS. "" THEN GOTO END_PROCS
$	    OWNER = F$EDIT(F$GETJPI(PID,"UIC"),"COLLAPSE")
$	    IF OWNER .NES. "''I_AM'" THEN GOTO GET_PROCS
$	    IF F$GETJPI(PID,"NODENAME") .NES. "''NODENAME'" THEN GOTO GET_PROCS
$	    IMAGE = F$GETJPI(PID,"IMAGNAME")
$	    IMAGE = F$PARSE(IMAGE,,,"NAME")+F$PARSE(IMAGE,,,"TYPE")
$	    IF IMAGE .NES. "''XMCD_BROWSER'.EXE" THEN GOTO GET_PROCS
$!
$! Found browser running on local node "owned" by user running this procedure
$!
$	    WRITE 0 PID
$	    GOTO GET_PROCS
$ END_PROCS:
$	    CLOSE 0
$	    IF F$FILE_ATTRIBUTES("PID_LIS.TMP","EOF") .EQ. 0
$	     THEN
$!
$! No browser found running on this node
$!
$	      DELETE/NOCONFIRM PID_LIS.TMP;*
$	      GOTO GOT_STATUS
$	    ENDIF
$!
$! Now collect channel info on all processes found running a browser 
$!
$	    OPEN/WRITE 1 GET_MBX.COM
$	    WRITE 1 "$ DEFINE SYS$OUTPUT GET_MBX.DAT"
$	    WRITE 1 "$ ANALYZE/SYSTEM"
$	    OPEN/READ 0 PID_LIS.TMP
$ GET_IMAGE:
$	    READ/ERROR=END_IMAGE 0 PID
$	    WRITE 1 "SET OUTPUT ''PID'.SDA_OUT"
$	    WRITE 1 "SHOW PROCESS/CHANNEL/ID=''PID'"
$	    GOTO GET_IMAGE
$ END_IMAGE:
$	    WRITE 1 "EXIT"
$	    WRITE 1 "$ DEASSIGN SYS$OUTPUT"
$	    CLOSE 0
$	    CLOSE 1
$	    @GET_MBX
$ GET_DEVICE:
$	    SEARCH_FILE = F$SEARCH("*.SDA_OUT;*")
$	    IF XMCD_BROWSER .EQS. "MOSAIC"
$!
$! Check SDA process channel info for MBA (mosaic mailbox) devices
$! (only in case Mosaic is the browser)
$!
$	     THEN
$	      IF SEARCH_FILE .EQS. "" THEN GOTO END_DEVICE
$	      IF VVER .GES. "V6.2"
$	       THEN
$	        SEARCH/OUTPUT=FND.TMP/NOWARNINGS 'SEARCH_FILE' MBA
$	        SEARCH_SEVER = $SEVERITY
$	       ELSE
$	        SET MESSAGE/NOTEXT/NOFACILITY/NOIDENTIFICATION/NOSEVERITY
$	        SEARCH/OUTPUT=FND.TMP 'SEARCH_FILE' MBA
$	        SEARCH_SEVER = $SEVERITY
$	        SET MESSAGE 'MESENV'
$	      ENDIF
$	      IF SEARCH_SEVER .NE. 1
$	       THEN
$	        DELETE/NOCONFIRM 'SEARCH_FILE',FND.TMP;*
$!
$! Nothing appropriate found
$!
$	        GOTO GET_DEVICE
$	      ENDIF
$	    ENDIF
$!
$! Check SDA process channel info for MBA and/or WSA (display) devices
$!
$	    IF VVER .GES. "V6.2"
$	     THEN
$	      SEARCH/OUTPUT=FND.TMP/NOWARN 'SEARCH_FILE' MBA,WSA
$	     ELSE
$	      SET MESSAGE/NOTEXT/NOFACILITY/NOIDENTIFICATION/NOSEVERITY
$	      SEARCH/OUTPUT=FND.TMP 'SEARCH_FILE' MBA,WSA
$	      SET MESSAGE 'MESENV'
$	    ENDIF
$!
$! Now exactly determine the MBA (if any) and WSA device names found
$!
$	    OPEN/READ 2 FND.TMP
$ SDA_READ:
$	    READ/ERROR=END_SDA 2 STR
$	    IF F$LOCATE("WSA",STR) .NE. F$LENGTH(STR) THEN WSA = F$ELEMENT(3," ",F$EDIT(STR,"COMPRESS"))
$	    IF F$LOCATE("MBA",STR) .NE. F$LENGTH(STR) THEN MBA = F$ELEMENT(4," ",F$EDIT(STR,"COMPRESS"))
$	    GOTO SDA_READ
$ END_SDA:
$	    CLOSE 2
$	    DELETE/NOCONFIRM 'SEARCH_FILE',FND.TMP;*
$!
$! Check whether the display device found matches the current display ...
$!
$	    DEFINE/NOLOG SYS$OUTPUT NLA0:
$	    SHOW DISPLAY/SYMBOL 'WSA'
$	    DEASSIGN SYS$OUTPUT
$!
$! ... if it doesn't, check next one found
$!
$	    IF DSPLNODE .NES. DECW$DISPLAY_NODE THEN GOTO GET_DEVICE
$!
$! Found a display owned by current user where a browser is running on
$! in remote mode
$!
$	    BROWSER_REMOTE = 1
$ END_DEVICE:
$	    DELETE/NOCONFIRM PID_LIS.TMP;*,GET_MBX.*;*
$ GOT_STATUS:
$	    IF BROWSER_REMOTE 
$	     THEN 
$!
$! Now setup browser specific command syntax and issue remote control command
$!
$	      IF XMCD_BROWSER .EQS. "MOSAIC"
$	       THEN
$	        URL = "''P1'"
$	        OPEN/WRITE BROWSER 'MBA'
$	        IF F$LOCATE(".discog",URL) .NE. F$LENGTH(URL) .AND. F$LOCATE("xmcd",URL) .NE. F$LENGTH(URL)
$		 THEN
$		  DISK = F$PARSE("''URL'",,,"DEVICE") - ":"
$		  DIRE = F$PARSE("''URL'",,,"DIRECTORY") - "[" - "]"
$		  FILN = F$PARSE("''URL'",,,"NAME")
$		  FILT = F$PARSE("''URL'",,,"TYPE")
$		  DIR1 = F$ELEMENT(0,".",DIRE)
$		  DIR2 = F$ELEMENT(1,".",DIRE)
$		  DIR3 = F$ELEMENT(2,".",DIRE)
$		  DIR4 = F$ELEMENT(3,".",DIRE)
$		  DIR5 = F$ELEMENT(4,".",DIRE)
$		  DIR6 = F$ELEMENT(5,".",DIRE)
$		  DIR7 = F$ELEMENT(6,".",DIRE)
$		  DIR8 = F$ELEMENT(7,".",DIRE)
$	   
$		  URL = ""
$		  IF DISK .NES. "." THEN URL = URL + "/''DISK'"
$		  IF DIR1 .NES. "." THEN URL = URL + "/''DIR1'"
$		  IF DIR2 .NES. "." THEN URL = URL + "/''DIR2'"
$		  IF DIR3 .NES. "." THEN URL = URL + "/''DIR3'"
$		  IF DIR4 .NES. "." THEN URL = URL + "/''DIR4'"
$		  IF DIR5 .NES. "." THEN URL = URL + "/''DIR5'"
$		  IF DIR6 .NES. "." THEN URL = URL + "/''DIR6'"
$		  IF DIR7 .NES. "." THEN URL = URL + "/''DIR7'"
$		  IF DIR8 .NES. "." THEN URL = URL + "/''DIR8'"
$		  URL = "file://localhost''URL'" + "/''FILN'''FILT'"
$		ENDIF
$	        WRITE BROWSER "goto|''URL'"
$	        CLOSE BROWSER
$	        DEFINE/NOLOG/JOB MOSAIC_MAILBOX "''MBA'"
$	      ENDIF
$!
$! Report success to xmcd
$!
$	      EXIT "%X10000000"
$	     ELSE
$!
$! No browser found controlable remotely, 
$! implies BrowserDirect command in common.cfg)
$!
$!	      EXIT
$	      GOTO FORCE_DIRECT
$	    ENDIF
$	  ENDIF
$!
$! Prepare to fire up browser (not controlled remotely).
$! Determine display environment and save it as well as the requested URL
$! to a file
$!
$ FORCE_DIRECT:
$	  DEFINE SYS$OUTPUT NLA0:
$	  SHOW DISPLAY/SYMBOL
$	  DEASSIGN SYS$OUTPUT
$	  OPEN/WRITE 0 SYS$LOGIN:XMCD_BROWSER.PAR
$	  WRITE 0 "''P1'"
$	  WRITE 0 "''DECW$DISPLAY_NODE'"
$	  WRITE 0 "''DECW$DISPLAY_TRANSPORT'"
$	  CLOSE 0
$!
$! Now fire up this procedure as a detached process (so that xmcd does
$! not need to wait for the browser to terminate)
$!
$	  SET MESSAGE/NOTEXT/NOFACILITY/NOIDENTIFICATION/NOSEVERITY
$	  RUN/DETACHED SYS$SYSTEM:LOGINOUT-
             /INPUT='F$ENVI("PROCEDURE")'/AUTHORIZE-
             /OUTPUT=NLA0:/ERROR=NLA0:/PRIO=4
$         IF $SEVERITY .EQ. 1 
$	   THEN 
$	    EXIT %X10000000
$	  ENDIF
$	  EXIT
$	ENDIF
$!
$! Running in detached mode now
$! First read the info necessary for firing up the browser (see the
$! last but one section above)
$!
$	SET DEFAULT SYS$LOGIN
$	OPEN/READ 0 XMCD_BROWSER.PAR
$	READ 0 URL
$	READ 0 NODE
$	READ 0 TRANS
$	CLOSE 0
$	DELETE/NOCONFIRM  XMCD_BROWSER.PAR;*
$!
$! Now create the display, determine the browser and setup the command
$! it is expected to execute ...
$!
$	SET DISPLAY/CREATE/NODE='NODE'/TRANSPORT='TRANS'/EXECUTIVE
$	BROWSER :== $SYS$SYSTEM:'XMCD_BROWSER'
$	IF XMCD_BROWSER .EQS. "MOZILLA" THEN BROWSER :== 'XMCD_MOZISTART'
$	IF XMCD_BROWSER .EQS. "MOSAIC" THEN COMMAND = "/REMOTE/MAILBOX=XMCD ''URL'"
$	IF XMCD_BROWSER .EQS. "NETSCAPE" .OR. XMCD_BROWSER .EQS. "MOZILLA" 
$	 THEN 
$	  IF F$LOCATE(".discog",URL) .NE. F$LENGTH(URL) .AND. F$LOCATE("xmcd",URL) .NE. F$LENGTH(URL)
$	   THEN
$	    DISK = F$PARSE("''URL'",,,"DEVICE") - ":"
$	    DIRE = F$PARSE("''URL'",,,"DIRECTORY") - "[" - "]"
$	    FILN = F$PARSE("''URL'",,,"NAME")
$	    FILT = F$PARSE("''URL'",,,"TYPE")
$	    DIR1 = F$ELEMENT(0,".",DIRE)
$	    DIR2 = F$ELEMENT(1,".",DIRE)
$	    DIR3 = F$ELEMENT(2,".",DIRE)
$	    DIR4 = F$ELEMENT(3,".",DIRE)
$	    DIR5 = F$ELEMENT(4,".",DIRE)
$	    DIR6 = F$ELEMENT(5,".",DIRE)
$	    DIR7 = F$ELEMENT(6,".",DIRE)
$	    DIR8 = F$ELEMENT(7,".",DIRE)
$	   
$	    URL = ""
$	    IF DISK .NES. "." THEN URL = URL + "/''DISK'"
$	    IF DIR1 .NES. "." THEN URL = URL + "/''DIR1'"
$	    IF DIR2 .NES. "." THEN URL = URL + "/''DIR2'"
$	    IF DIR3 .NES. "." THEN URL = URL + "/''DIR3'"
$	    IF DIR4 .NES. "." THEN URL = URL + "/''DIR4'"
$	    IF DIR5 .NES. "." THEN URL = URL + "/''DIR5'"
$	    IF DIR6 .NES. "." THEN URL = URL + "/''DIR6'"
$	    IF DIR7 .NES. "." THEN URL = URL + "/''DIR7'"
$	    IF DIR8 .NES. "." THEN URL = URL + "/''DIR8'"
$	    URL = URL + "/''FILN'''FILT'"
$	  ENDIF
$	  COMMAND = "''URL'"
$	  IF F$SEARCH("SYS$LOGIN:''XMCD_BROWSER'_REMOTE.FLAG") .NES. ""
$	   THEN 
$	    IF F$LOCATE("http://",COMMAND) .NE. F$LENGTH(COMMAND)
$	     THEN
$	      COMMAND = "-remote openURL(''COMMAND')"
$	     ELSE
$	      COMMAND = "-remote openfile(''COMMAND')"
$	    ENDIF
$	   ELSE
$	    IF XMCD_BROWSER .EQS. "MOZILLA" 
$	     THEN
$	      IF F$LOCATE("http://",COMMAND) .EQ. F$LENGTH(COMMAND)
$	       THEN
$		COMMAND = "file://''COMMAND'"
$	      ENDIF
$	    ENDIF
$	  OPEN/WRITE 100 SYS$LOGIN:'XMCD_BROWSER'_REMOTE.FLAG
$	  CLOSE 100
$	  ENDIF
$	ENDIF
$	
$!
$! ... and finally fire up the browser
$!
$	BROWSER 'COMMAND'
$	IF F$LOCATE("-remote",COMMAND) .EQ. F$LENGTH(COMMAND) -
	 THEN IF F$SEARCH("SYS$LOGIN:''XMCD_BROWSER'_REMOTE.FLAG") .NES. "" -
	  THEN DELETE/NOCONFIRM SYS$LOGIN:'XMCD_BROWSER'_REMOTE.FLAG;*
$	
