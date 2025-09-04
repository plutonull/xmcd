$!
$! @(#)startviewer.com	6.13 04/01/14
$!
$! Image viewer startup procedure.  See the INSTALL.VMS file for details.
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
$	IF F$MODE() .NES. "OTHER"
$	 THEN
$	  DEFINE SYS$OUTPUT NLA0:
$	  SHOW DISPLAY/SYMBOL
$	  DEASSIGN SYS$OUTPUT
$	  OPEN/WRITE 0 SYS$LOGIN:XMCD_VIEWER.PAR
$	  WRITE 0 "''P1'"
$	  WRITE 0 "''DECW$DISPLAY_NODE'"
$	  WRITE 0 "''DECW$DISPLAY_TRANSPORT'"
$	  CLOSE 0
$	  SET MESSAGE/NOTEXT/NOFACILITY/NOIDENTIFICATION/NOSEVERITY
$	  RUN/DETACHED SYS$SYSTEM:LOGINOUT-
             /INPUT='F$ENVI("PROCEDURE")'/AUTHORIZE-
             /OUTPUT=NLA0:/ERROR=NLA0:/PRIO=4
$!
$! Report success to xmcd
$! 
$         IF $SEVERITY .EQ. 1 THEN EXIT %X10000000
$	  EXIT
$	ENDIF
$!
$	SET DEFAULT SYS$LOGIN
$	OPEN/READ 0 XMCD_VIEWER.PAR
$	READ 0 PIXFIL
$	READ 0 NODE
$	READ 0 TRANS
$	CLOSE 0
$	DELETE/NOCONFIRM  XMCD_VIEWER.PAR;*
$	SET DISPLAY/CREATE/NODE='NODE'/TRANSPORT='TRANS'/EXECUTIVE
$!
$! Replace the directory in the next line with the directory your copy 
$! of XV is resident in.
$!
$	XV :== $KIDDY$DKA0:[XV-3_10A]XV
$	XV 'PIXFIL'COVER.GIF -fixed -ge 300x300+24-590
$	
