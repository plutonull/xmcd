#!/bin/sh
#
# @(#)config.sh	7.110 04/04/20
#
# Script to set up the device-dependent configuration files.
#
#    xmcd  - Motif(R) CD Audio Player/Ripper
#    cda   - Command-line CD Audio Player/Ripper
#
#    Copyright (C) 1993-2004  Ti Kan
#    E-mail: xmcd@amb.org
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin:/etc:/usr/local/bin:/usr/ucb
export PATH

VER=3.3.2

# Change the following directory to fit your local configuration
BINDIR=/usr/bin/X11
XMCDLIB=/usr/lib/X11/xmcd
DISCOGDIR=/usr/lib/X11/xmcd/discog
XMCD_URL=http://www.amb.org/xmcd/

ERRFILE=/tmp/xmcd.err
BTMPFILE=/tmp/.xmcdcfg.b$$
MTMPFILE=/tmp/.xmcdcfg.m$$

CFGDIR=$XMCDLIB/config
SCRDIR=$XMCDLIB/scripts
CDDB1PATH="rock;jazz;blues;newage;classical;reggae;folk;country;soundtrack;misc;unclass;data"
OWNER=bin
GROUP=bin
CDIRPERM=777

# Utility functions

do_exit()
{
	if [ $1 -eq 0 ]
	then
		rm -f $CFGDIR/common.cfg.old
		$ECHO "\n\nXmcd set-up is now complete.\n"
		$ECHO "Please read the DRIVES file supplied with the xmcd"
		$ECHO "distribution for hardware configuration information"
		$ECHO "about specific drives.\n"
	else
		$ECHO "\n\nErrors have occurred configuring xmcd."
		if [ $ERRFILE != /dev/null ]
		then
			$ECHO "See $ERRFILE for an error log."
		fi
	fi
	exit $1
}

log_err()
{
	if [ "$1" = "-p" ]
	then
		$ECHO "Error: $2" >&2
	fi
	$ECHO "$2" >>$ERRFILE
}

get_str()
{
	$ECHO "$* \c"
	read ANS
	if [ -n "$ANS" ]
	then
		return 0
	else
		return 1
	fi
}

get_yn()
{
	if [ -z "$YNDEF" ]
	then
		YNDEF=y
	fi

	while :
	do
		$ECHO "$*? [${YNDEF}] \c"
		read ANS
		if [ -n "$ANS" ]
		then
			case $ANS in
			[yY])
				RET=0
				break
				;;
			[nN])
				RET=1
				break
				;;
			*)
				$ECHO "Please answer y or n"
				;;
			esac
		else
			if [ $YNDEF = y ]
			then
				RET=0
			else
				RET=1
			fi
			break
		fi
	done

	YNDEF=
	return $RET
}


do_link()
{
	source=$1
	target=$2

	rm -f $target

	# Try symlink first
	ln -s $source $target 2>/dev/null
	RETSTAT=$?

	if [ $RETSTAT != 0 -a -f $source ]
	then
		# Use hard link if a regular file
		ln $source $target 2>/dev/null
		RETSTAT=$?
	fi

	if [ $RETSTAT != 0 ]
	then
		log_err -p "Cannot link $source -> $target"
	fi
	return $RETSTAT
}


do_chown()
{
	if [ $2 != _default_ ]
	then
		chown $1 $2 2>/dev/null
	fi
}


do_chgrp()
{
	if [ $2 != _default_ ]
	then
		chgrp $1 $2 2>/dev/null
	fi
}


do_chmod()
{
	if [ $2 != _default_ ]
	then
		chmod $1 $2 2>/dev/null
	fi
}


make_dir()
{
	dir=$1
	perm=$2
	owner=$3
	group=$4
	$ECHO "\t$dir"
	if [ ! -d $dir ]
	then
		mkdir -p $dir
	fi
	do_chown $owner $dir 2>/dev/null
	do_chgrp $group $dir 2>/dev/null
	do_chmod $perm $dir 2>/dev/null
	return 0
}


get_old_cparm()
{
	if [ -r $CFGDIR/common.cfg.old ]
	then
		cparmfile=$CFGDIR/common.cfg.old
	elif [ -z "$FROM_INSTALL_SH" ]
	then
		cparmfile=$CFGDIR/common.cfg
	else
		return 1
	fi

	PARMVAL=`grep "^${1}:" $cparmfile 2>/dev/null | $AWK '{ print $2 }'`
	return $?
}


get_proxy()
{
	YNDEF=n
	if get_old_cparm "cddbUseProxy" || get_old_cparm "cddbUseHttpProxy"
	then
		if echo "$PARMVAL" | fgrep -i true >/dev/null 2>&1
		then
			YNDEF=y
		fi
	fi
	$ECHO "\n  Is this system within a firewall that requires the use of"
	get_yn "  an HTTP proxy server to access the Internet"
	if [ $? -ne 0 ]
	then
		USEPROXY=False
		PROXYSERVER="localhost:80"
		return
	fi

	USEPROXY=True

	PROXYSERVER=
	PROXYPORT=
	if get_old_cparm "proxyServer"
	then
		PROXYSERVER=`echo ${PARMVAL} | $AWK -F: '{ print $1 }'`
		PROXYPORT=`echo ${PARMVAL} | $AWK -F: '{ print $2 }'`
	fi
	if [ -z "$PROXYSERVER" ]
	then
		PROXYSERVER=localhost
	fi
	if [ -z "$PROXYPORT" ]
	then
		PROXYPORT=80
	fi

	while :
	do
		if get_str \
		"  Enter the proxy server host name or IP number: [${PROXYSERVER}]"
		then
			if ($ECHO "$ANS" | grep "[ 	]") >/dev/null 2>&1
			then
				$ECHO "  Invalid input.  Try again."
				continue
			else
				PROXYSERVER="$ANS"
			fi
		fi
		break
	done

	while :
	do
		if get_str \
		"  Enter the proxy server port number: [${PROXYPORT}]"
		then
			if [ `expr "$ANS" : '^[0-9]*$'` -eq 0 ]
			then
				$ECHO "  Invalid input.  Try again."
				continue
			else
				PROXYPORT="$ANS"
			fi
		fi
		break
	done

	PROXYSERVER="${PROXYSERVER}:${PROXYPORT}"

	YNDEF=n
	if get_old_cparm "proxyAuthorization" && \
		echo "$PARMVAL" | fgrep -i true >/dev/null 2>&1
	then
		YNDEF=y
	fi
	get_yn "  Does the proxy server require password authorization"
	if [ $? -ne 0 ]
	then
		PROXYAUTH=False
	else
		PROXYAUTH=True
	fi
}


cddb_config()
{
	# Set up cdinfoPath

	$ECHO "\nIf your system has Internet connectivity and functional"
	$ECHO "domain name service (DNS), you should answer 'y' to the next"
	$ECHO "question.  If this system is not linked to the Internet at all,"
	$ECHO "then answer 'n'."

	YNDEF=y
	if get_old_cparm "internetOffline" || get_old_cparm "disableCddb" || \
	   get_old_cparm "cddbRemoteDisable"
	then
		if echo "$PARMVAL" | fgrep -i true >/dev/null 2>&1
		then
			YNDEF=n
		fi
	fi

	$ECHO "\n  Would you like to use the free Internet CDDB(R) service"
	get_yn "  for album/track information"
	if [ $? -ne 0 ]
	then
		$ECHO "\nInternet CDDB server access is disabled."
		$ECHO "To enable it later, run the $CFGDIR/config.sh"
		$ECHO "script again."
		INETOFFLN=True
		CDINFOPATH="CDTEXT;${CDDB1PATH}"
		return
	fi

	INETOFFLN=False
	CDINFOPATH="CDDB;CDTEXT;${CDDB1PATH}"

	get_proxy
}


ask_scsi_config()
{
	$ECHO "\n  Since you have an unlisted drive, I will assume"
	$ECHO "  that it is SCSI-2 compliant.  If this is not true then"
	$ECHO "  xmcd will probably not work."

	YNDEF=n
	if get_yn "\n  Do you want to continue"
	then
		METHOD=0
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYNOTUR=False
	else
		return 1
	fi

	$ECHO "\n  You will now be asked several technical questions about"
	$ECHO "  your drive.  If you don't know the answer, try accepting"
	$ECHO "  the default values, and if problems occur when using"
	$ECHO "  xmcd, reconfigure the settings by running this script"
	$ECHO "  again, or editing the $CFGDIR/$CONFIGFILE"
	$ECHO "  file."
	$ECHO "\n  Please refer to the \"xmcd Configuration Guide\" for help."
	$ECHO "  It can be found at the xmcd web site:\n  ${XMCD_URL}"
	$ECHO "\n  If you get an unlisted drive working with xmcd in this"
	$ECHO "  manner, the author of xmcd would like to hear from you"
	$ECHO "  and incorporate the settings into the next xmcd release."
	$ECHO "  Please send e-mail to \"xmcd@amb.org\"."

	while :
	do
	    YNDEF=n
	    if get_yn "\n  Is your drive on $XMCD_DEV a multi-disc changer"
	    then
		$ECHO "\n  Select one of the following changer methods:\n"
		$ECHO "  1.\tSCSI LUN addressing method"
		$ECHO "  2.\tSCSI medium changer method"
		$ECHO "  3.\tother"
		$ECHO "  q.\tquit (abort configuration)"

		while :
		do
		    if get_str "\n  Enter choice: [1]"
		    then
			case "$ANS" in
			[1-3])
			    break
			    ;;
			q)
			    return 1
			    ;;
			*)
			    $ECHO "  Please answer 1 to 3."
			    ;;
			esac
		    else
			ANS=1
			break
		    fi
		done	

		if [ "$ANS" -eq 3 ]
		then
		    #
		    # Unsupported changer method: just treat it
		    # as a single-disc drive
		    #
		    $ECHO "\n  Your drive will be treated as a single-disc unit."
		    CHGMETHOD=0
		    NUMDISCS=1
		    MULTIPLAY=False
		else
		    CHGMETHOD="$ANS"
		    while :
		    do
			if get_str \
			    "\n  How many discs does the drive support?:"
			then
			    case "$ANS" in
			    [1-9]*)
				NUMDISCS="$ANS"
				break
				;;
			    *)
				$ECHO "  Invalid input.  Try again."
				;;
			    esac
			else
			    $ECHO "  Invalid input.  Try again."
			fi
		    done

		    if [ "$NUMDISCS" -gt 1 ]
		    then
			MULTIPLAY=True
		    else
			CHGMETHOD=0
			MULTIPLAY=False
		    fi
		fi
	    else
		CHGMETHOD=0
		NUMDISCS=1
		MULTIPLAY=False
	    fi

	    $ECHO "\n  Does your drive on $XMCD_DEV:\n"

	    YNDEF=y
	    if get_yn "  - Support the Play_Audio_MSF command"
	    then
		    PLAYMSF=True
	    else
		    PLAYMSF=False
	    fi

	    YNDEF=n
	    if get_yn "  - Support the Play_Audio(12) command"
	    then
		    PLAY12=True
	    else
		    PLAY12=False
	    fi

	    YNDEF=y
	    if get_yn "  - Support the Play_Audio(10) command"
	    then
		    PLAY10=True
	    else
		    PLAY10=False
	    fi

	    YNDEF=y
	    if get_yn "  - Support the Play_Audio_Track/Index command"
	    then
		    PLAYTI=True
	    else
		    PLAYTI=False
	    fi

	    YNDEF=n
	    if get_yn \
		"  - Support caddy/tray load via the Start_Stop_Unit command"
	    then
		    LOAD=True
	    else
		    LOAD=False
	    fi

	    YNDEF=y
	    if get_yn \
		"  - Support caddy/tray eject via the Start_Stop_Unit command"
	    then
		    EJECT=True
	    else
		    EJECT=False
	    fi

	    YNDEF=y
	    if get_yn \
		"  - Support the DBD bit in the Mode_Sense command"
	    then
		    MODEDBD=True
	    else
		    MODEDBD=False
	    fi

	    YNDEF=n
	    if get_yn \
		"  - Need the 10-byte version of the Mode_Sense command"
	    then
		    MODE10BYTE=True
	    else
		    MODE10BYTE=False
	    fi

	    YNDEF=y
	    if get_yn \
		"  - Support audio volume control via the Mode_Select command"
	    then
		    YNDEF=y
		    if get_yn \
		    "  - Support independent volume control for each channel"
		    then
			    VOLSUPP=True
			    BALSUPP=True
		    else
			    VOLSUPP=True
			    BALSUPP=False
		    fi

		    YNDEF=y
		    if get_yn \
			"  - Support audio channel routing via Mode_Select"
		    then
			    CHRSUPP=True
		    else
			    CHRSUPP=False
		    fi
	    else
		    VOLCTL=0
		    VOLSUPP=False
		    BALSUPP=False
		    CHRSUPP=False
	    fi

	    YNDEF=y
	    if get_yn "  - Support the Pause/Resume command"
	    then
		    PAUSE=True
	    else
		    PAUSE=False
	    fi

	    YNDEF=y
	    if get_yn "  - Support the Prevent/Allow_Medium_Removal command"
	    then
		    CADDYLOCK=True
	    else
		    CADDYLOCK=False
	    fi

	    YNDEF=n
	    if get_yn \
		"  - Support Data Format 1 of the Read_Subchannel command"
	    then
		    CURPOSFMT=True
	    else
		    CURPOSFMT=False
	    fi

	    YNDEF=n
	    if get_yn "  - Return CDDA data in big-endian byte order"
	    then
		    CDDADATABE=True
	    else
		    CDDADATABE=False
	    fi

	    YNDEF=y
	    if get_yn "  - Need a Mode_Select operation to enable CDDA reading"
	    then
		    CDDASCSIMODESEL=True
	    else
		    CDDASCSIMODESEL=False
	    fi

	    if [ $CDDASCSIMODESEL = True ]
	    then
		while :
		do
		    if get_str \
			"  \n  What is the Mode_Select density code to enable CDDA reading?: [0]"
		    then
			case "$ANS" in
			[0-9]*)
			    CDDASCSIDENSITY="$ANS"
			    break
			    ;;
			*)
			    $ECHO "  Invalid input.  Try again."
			    ;;
			esac
		    else
			CDDASCSIDENSITY=0
			break
		    fi
		done
	    else
		CDDASCSIDENSITY=0
	    fi

	    while :
	    do
		$ECHO "  \n  Which command should be used for CDDA reads?"
		$ECHO "    0	MMC standard Read_CD"
		$ECHO "    1	SCSI standard Read"
		$ECHO "    2	NEC Read_CDDA"
		$ECHO "    3	Sony Read_CDDA"
		if get_str "  Select one: [2]"
		then
		    case "$ANS" in
		    [0-3])
			CDDASCSIREADCMD="$ANS"
			break
			;;
		    *)
			$ECHO "  Invalid input.  Try again."
			;;
		    esac
		else
		    CDDASCSIREADCMD=2
		    break
		fi
	    done

	    while :
	    do
		if get_str \
		    "  \n  What is the required wait time (seconds) for the drive to spin up?: [3]"
		then
		    case "$ANS" in
		    [0-9]*)
			SPINUPINTVL="$ANS"
			break
			;;
		    *)
			$ECHO "  Invalid input.  Try again."
			;;
		    esac
		else
		    SPINUPINTVL=3
		    break
		fi
	    done

	    if [ "$PAUSE" = True ]
	    then
		YNDEF=n
		if get_yn \
		"\n  Does the drive require strict pause/resume symmetry"
		then
			STRICTPR=True
		else
			STRICTPR=False
		fi

		YNDEF=n
		if get_yn \
		"\n  Does the drive require a pause before reissuing play"
		then
			PLAYPAUSEPLAY=True
		else
			PLAYPAUSEPLAY=False
		fi
	    else
		    STRICTPR=False
		    PLAYPAUSEPLAY=False
	    fi

	    $ECHO "\n  This is the configuration for ${XMCD_DEV}:\n"
	    $ECHO "  logicalDriverNumber:   $DRVNO"
	    $ECHO "  mediumChangeMethod:    $CHGMETHOD"
	    $ECHO "  numDiscs:              $NUMDISCS"
	    $ECHO "  playAudio12Support:    $PLAY12"
	    $ECHO "  playAudioMSFSupport:   $PLAYMSF"
	    $ECHO "  playAudio10Support:    $PLAY10"
	    $ECHO "  playAudioTISupport:    $PLAYTI"
	    $ECHO "  loadSupport:           $LOAD"
	    $ECHO "  ejectSupport:          $EJECT"
	    $ECHO "  modeSenseSetDBD:       $MODEDBD"
	    $ECHO "  modeSenseUse10Byte:    $MODE10BYTE"
	    $ECHO "  volumeControlSupport:  $VOLSUPP"
	    $ECHO "  balanceControlSupport: $BALSUPP"
	    $ECHO "  pauseResumeSupport:    $PAUSE"
	    $ECHO "  strictPauseResume:     $STRICTPR"
	    $ECHO "  playPausePlay:         $PLAYPAUSEPLAY"
	    $ECHO "  caddyLockSupport:      $CADDYLOCK"
	    $ECHO "  curposFormat:          $CURPOSFMT"
	    $ECHO "  multiPlay:             $MULTIPLAY"
	    $ECHO "  spinUpInterval:        $SPINUPINTVL"
	    $ECHO "  cddaScsiModeSelect:    $CDDASCSIMODESEL"
	    $ECHO "  cddaScsiReadCommand:   $CDDASCSIREADCMD"
	    $ECHO "  cddaScsiDensity:       $CDDASCSIDENSITY"
	    $ECHO "  cddaDataBigEndian:     $CDDADATABE"

	    YNDEF=y
	    if get_yn "\n  Is this acceptable"
	    then
		    break
	    fi

	    $ECHO "  Try again..."
	done

	return 0
}


ask_nonscsi_config()
{
	#
	# Select the ioctl method
	#
	$ECHO "\n  Please select a Device Interface Method:\n"
	$ECHO "  1.\tSunOS/Solaris/Linux/QNX ioctl method"
	$ECHO "  2.\tFreeBSD/NetBSD/OpenBSD ioctl method"
	$ECHO "  3.\tIBM AIX IDE ioctl method"
	$ECHO "  4.\tBSDI/WindRiver BSD/OS ATAPI"
	$ECHO "  5.\tSCO Open Server ATAPI BTLD"
	$ECHO "  6.\tCompaq/HP Tru64 UNIX, Digital UNIX ATAPI"
	$ECHO "  7.\tHP-UX ATAPI"
	$ECHO "  8.\tLinux SCSI Emulation for ATAPI drives"
	$ECHO "  9.\tFreeBSD ATAPI-CAM for ATAPI drives"
	$ECHO "  q.\tquit (abort configuration)"

	if [ -z "$IOCDEF" -o "$IOCDEF" = 0 ]
	then
		IOCDEF=1
	fi

	while :
	do
		if get_str "\n  Enter choice: [$IOCDEF]"
		then
			case "$ANS" in
			[1-9])
				break
				;;
			q)
				return 1
				;;
			*)
				$ECHO "  Please answer 1 to 9."
				;;
			esac
		else
			ANS=$IOCDEF
			break
		fi
	done

	IOCMETHOD="$ANS"

	if [ -z "$NUMDISCS" ]
	then
		YNDEF=n
		if get_yn "\n  Is your drive on $XMCD_DEV a multi-disc changer"
		then
			while :
			do
				if get_str \
				"\n  How many discs does the drive support?:"
				then
				    case "$ANS" in
				    [1-9]*)
					NUMDISCS="$ANS"
					break
					;;
				    *)
					$ECHO "  Invalid input.  Try again."
					;;
				    esac
				else
				    $ECHO "  Invalid input.  Try again."
				fi
			done

			if [ "$NUMDISCS" -gt 1 ]
			then
				CHGMETHOD=3
				MULTIPLAY=True
			else
				CHGMETHOD=0
				MULTIPLAY=False
			fi
		else
			CHGMETHOD=0
			NUMDISCS=1
			MULTIPLAY=False
		fi
	fi

	# Set the rest of the parameters
	case "$IOCMETHOD" in
	1)	# SunOS/Solaris/Linux/QNX ioctl method
		METHOD=1
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=True
		LOAD=True
		EJECT=True
		MODEDBD=False
		MODE10BYTE=False
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=True
		VOLCTL=3
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=False
		CURPOSFMT=False
		PLAYNOTUR=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=0
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	2)	# FreeBSD/NetBSD ioctl method
		METHOD=2
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=True
		LOAD=True
		EJECT=True
		MODEDBD=False
		MODE10BYTE=False
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=False
		VOLCTL=3
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=True
		CURPOSFMT=False
		PLAYNOTUR=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=0
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	3)	# IBM AIX IDE ioctl method
		METHOD=3
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=True
		LOAD=False
		EJECT=True
		MODEDBD=False
		MODE10BYTE=False
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=False
		VOLCTL=3
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=True
		CURPOSFMT=False
		PLAYNOTUR=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=0
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	4)	# BSDI/WindRiver BSD/OS ATAPI (SCSI emulation)
		# Set up as a "generic" SCSI-2 drive
		METHOD=0
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=False
		LOAD=True
		EJECT=True
		MODEDBD=False
		MODE10BYTE=False
		VOLSUPP=False
		BALSUPP=False
		CHRSUPP=False
		VOLCTL=0
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=False
		CURPOSFMT=True
		PLAYNOTUR=False
		CHGMETHOD=0
		NUMDISCS=1
		MULTIPLAY=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=1
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	5)	# SCO Open Server ATAPI BTLD (SCSI emulation)
		# Set up as a "generic" SCSI-2 drive
		METHOD=0
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=False
		LOAD=True
		EJECT=True
		MODEDBD=False
		MODE10BYTE=False
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=False
		VOLCTL=3
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=False
		CURPOSFMT=False
		PLAYNOTUR=False
		CHGMETHOD=0
		NUMDISCS=1
		MULTIPLAY=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=1
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	6)	# Compaq/HP Tru64 UNIX / Digital UNIX ATAPI (SCSI emulation)
		# Set up as a "generic" SCSI-2 drive
		METHOD=0
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=False
		LOAD=True
		EJECT=True
		MODEDBD=False
		MODE10BYTE=False
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=True
		VOLCTL=7
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=True
		CURPOSFMT=True
		PLAYNOTUR=False
		CHGMETHOD=0
		NUMDISCS=1
		MULTIPLAY=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=1
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	7)	# HP-UX ATAPI (SCSI emulation)
		# Set up as a "generic" SCSI-2 drive
		METHOD=0
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=False
		LOAD=True
		EJECT=True
		MODEDBD=True
		MODE10BYTE=False
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=True
		VOLCTL=7
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=True
		CURPOSFMT=True
		PLAYNOTUR=False
		CHGMETHOD=0
		NUMDISCS=1
		MULTIPLAY=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=1
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	8)	# Linux SCSI Emulation for ATAPI
		# Set up as a "generic" SCSI-2 drive
		METHOD=0
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=False
		LOAD=True
		EJECT=True
		MODEDBD=True
		MODE10BYTE=True
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=True
		VOLCTL=7
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=True
		CURPOSFMT=True
		PLAYNOTUR=False
		CHGMETHOD=0
		NUMDISCS=1
		MULTIPLAY=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=1
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	9)	# FreeBSD ATAPI-CAM SCSI Emulation
		# Set up as a "generic" SCSI-2 drive
		METHOD=0
		VENDOR=0
		VOLBASE=0
		VOLTAPER=0
		PLAYMSF=True
		PLAY12=False
		PLAY10=False
		PLAYTI=False
		LOAD=True
		EJECT=True
		MODEDBD=True
		MODE10BYTE=True
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=True
		VOLCTL=7
		PAUSE=True
		STRICTPR=False
		PLAYPAUSEPLAY=False
		CADDYLOCK=True
		CURPOSFMT=True
		PLAYNOTUR=False
		CHGMETHOD=0
		NUMDISCS=1
		MULTIPLAY=False
		SPINUPINTVL=3
		CDDASCSIMODESEL=1
		CDDASCSIREADCMD=0
		CDDASCSIDENSITY=0
		CDDADATABE=0
		;;
	*)	# Should not get here
		$ECHO "  Error: unsupported ioctl method."
		return 1
		;;
	esac

	return 0
}


drive_sel()
{
	if [ $IS_SCSI -eq 0 -o "$OS_SYS" = QNX ]
	then
		ask_nonscsi_config
		return $?
	fi

	eval `\
	(
		$ECHO "ENTRIES=\""
		cd $CFGDIR/.tbl
		for i in *
		do
			if [ -f $i ]
			then
				if fgrep tblver=6 $CFGDIR/.tbl/$i \
			   		>/dev/null 2>&1
				then
					$ECHO "$i \c"
				else
					log_err -p \
					"$CFGDIR/.tbl/$i version mismatch"
				fi
			fi
		done
		$ECHO "\""
	)`

	j=1
	>$BTMPFILE
	if [ -n "$ENTRIES" ]
	then
		for i in $ENTRIES
		do
			brand=`fgrep tblalias= $CFGDIR/.tbl/$i 2>/dev/null | \
				sed 's/^.*tblalias=//'`
			if [ -z "$brand" ]
			then
				brand=$i
			fi
			$ECHO "  $j\t$brand" >>$BTMPFILE
			j=`expr $j + 1`
		done
	fi
	if [ $IS_SCSI -eq 0 ]
	then
		$ECHO "  $j\tother non-SCSI" >>$BTMPFILE
	elif [ $IS_SCSI -eq 1 ]
	then
		$ECHO "  $j\tother SCSI" >>$BTMPFILE
	fi
	$ECHO "  ..\tStart over with drive $DRVNO configuration" >>$BTMPFILE
	$ECHO "  q\tquit (abort configuration)" >>$BTMPFILE

	showmenu=1
	while :
	do
		if [ $showmenu = 1 ]
		then
			$ECHO "\n  Device ($XMCD_DEV) configuration"
			$ECHO "  Please select the drive brand:\n"

			PGLEN=`wc -l $BTMPFILE | $AWK '{ print $1 + 2 }'`
			if [ $PGLEN -gt 18 ]
			then
				PGLEN=`expr $PGLEN / 2`
				pr -t -2 -w78 -l$PGLEN $BTMPFILE
			else
				cat $BTMPFILE
			fi
			showmenu=0
		fi

		if get_str "\n  Enter choice:"
		then
			if [ "$ANS" = q ]
			then
				rm -f $BTMPFILE
				return 1
			elif [ "$ANS" = ".." ]
			then
				rm -f $BTMPFILE
				return 2
			elif [ "$ANS" -lt 1 -o "$ANS" -gt $j ]
			then
				$ECHO "  Please answer 1 to $j."
			elif [ $IS_SCSI -eq 0 ] && [ "$ANS" = "$j" ]
			then
				ask_nonscsi_config
				rm -f $BTMPFILE
				return $?
			elif [ $IS_SCSI -eq 1 ] && [ "$ANS" = "$j" ]
			then
				ask_scsi_config
				rm -f $BTMPFILE
				return $?
			else
				k=1
				for i in $ENTRIES
				do
					if [ $k = $ANS ]
					then
						model_sel $i $CFGDIR/.tbl/$i
						ret=$?
						if [ $ret -eq 2 ]
						then
							showmenu=1
							break
						else
							rm -f $BTMPFILE
							return $ret
						fi
					fi
					k=`expr $k + 1`
				done
			fi
		else
			$ECHO "  Please answer 1 to $j."
		fi
	done

	# Should not get here.
	rm -f $BTMPFILE
	return 1
}


model_sel()
{
	BRAND=$1
	CFGFILE=$2

	$ECHO "\n  Device ($XMCD_DEV) configuration"
	$ECHO "  Please select the $BRAND drive model:\n"

	$AWK -F: '
	BEGIN		{ n = 1 }
	/^#/		{ next }
	/^$/		{ next }
	/^[ 	]*$/	{ next }
	{
		if ($2 == 0) {
			if ($3 == 1)
				mode = "OS driver ioctl"
			else
				mode = "other"
		}
		else if ($2 == 1)
			mode = "SCSI-1"
		else if ($2 >= 2)
			mode = "SCSI-2"

		if (is_scsi == "-1") {
			printf("  %d\t%-12s%s\n", n, $1, mode)
			n++
		}
		else if (is_scsi == "0" && $2 == 0) {
			printf("  %d\t%-12s%s\n", n, $1, mode)
			n++
		}
		else if (is_scsi == "1" && $2 >= 1) {
			printf("  %d\t%-12s%s\n", n, $1, mode)
			n++
		}
	}
	END {
		if (is_scsi == "0")
			printf("  %d\t%-12s%s\n", n, "other", "non-SCSI")
		else if (is_scsi == "1")
			printf("  %d\t%-12s%s\n", n, "other", "SCSI")
		printf("  ..\tGo back to drive brand menu\n")
		printf("  q\tquit (abort configuration)\n")
	}
	' is_scsi=$IS_SCSI $CFGFILE >$MTMPFILE

	k=`wc -l $MTMPFILE | $AWK '{ print $1 - 2 }'`
	PGLEN=`expr $k + 4`

	if [ $PGLEN -gt 16 ]
	then
		$ECHO "\tModel       Mode\c"
		$ECHO "                       Model       Mode\n"
		PGLEN=`expr $PGLEN / 2`
		pr -t -2 -w78 -l$PGLEN $MTMPFILE
	else
		$ECHO "\tModel       Mode\n"
		cat $MTMPFILE
	fi

	rm -f $MTMPFILE

	while :
	do
		if get_str "\n  Enter choice:"
		then
			if [ "$ANS" = "q" ]
			then
				return 1
			elif [ "$ANS" = ".." ]
			then
				return 2
			elif [ "$ANS" -lt 1 -o "$ANS" -gt $k ]
			then
				$ECHO "  Please answer 1 to $k."
			elif [ $IS_SCSI -eq 0 ] && [ "$ANS" = "$k" ]
			then
				ask_nonscsi_config
				return $?
			elif [ $IS_SCSI -eq 1 ] && [ "$ANS" = "$k" ]
			then
				ask_scsi_config
				return $?
			else
				read_config $CFGFILE $ANS
				if [ $IS_SCSI -eq 0 ]
				then
					ask_nonscsi_config
				fi
				return $?
			fi
		else
			$ECHO "  Please answer 1 to $k."
		fi
	done

	# Should not get here.
	return 1
}


read_config()
{
	eval `$AWK -F: '
	BEGIN {
		n = 0
	}
	!/^#/ {
	    if ((is_scsi && $2 > 0) || (!is_scsi && $2 == 0)) {
		n++
		if (n == sel) {
		    if ($2 > 0) {
			print "METHOD=0"
			printf("VENDOR=%d\n", $3)
		    }
		    else {
			printf("METHOD=%d\n", $3)
			print "VENDOR=0"
		    }

		    printf("PLAYMSF=%s\n", $4 == 0 ? "False" : "True")
		    printf("PLAY12=%s\n", $5 == 0 ? "False" : "True")
		    printf("PLAY10=%s\n", $6 == 0 ? "False" : "True")
		    printf("PLAYTI=%s\n", $7 == 0 ? "False" : "True")
		    printf("LOAD=%s\n", $8 == 0 ? "False" : "True")
		    printf("EJECT=%s\n", $9 == 0 ? "False" : "True")
		    printf("MODEDBD=%s\n", $10 == 0 ? "False" : "True")
		    printf("MODE10BYTE=%s\n", $11 == 0 ? "False" : "True")
		    printf("VOLCTL=%d\n", $12)
		    printf("VOLBASE=%d\n", $13)
		    printf("VOLTAPER=%d\n", $14)
		    printf("PAUSE=%s\n", $15 == 0 ? "False" : "True")
		    printf("STRICTPR=%s\n", $16 == 0 ? "False" : "True")
		    printf("PLAYPAUSEPLAY=%s\n", $17 == 0 ? "False" : "True")
		    printf("CADDYLOCK=%s\n", $18 == 0 ? "False" : "True")
		    printf("CURPOSFMT=%s\n", $19 == 0 ? "False" : "True")
		    printf("PLAYNOTUR=%s\n", $20 == 0 ? "False" : "True")
		    printf("CHGMETHOD=%d\n", $21)
		    printf("MULTIPLAY=%s\n", $21 == 0 ? "False" : "True")
		    printf("NUMDISCS=%d\n", $22)
		    printf("SPINUPINTVL=%d\n", $23)
		    printf("CDDASCSIMODESEL=%s\n", $24 == 0 ? "False" : "True")
		    printf("CDDASCSIDENSITY=%d\n", $25)
		    printf("CDDASCSIREADCMD=%d\n", $26)
		    printf("CDDADATABE=%s\n", $27 == 0 ? "False" : "True")
		}
	    }
	}
	' is_scsi=$IS_SCSI sel=$2 $1`

	return $?
}


set_platform_parms()
{
	# Set platform-specific parameters

	HAS_OSS=0

	if [ -x /bin/ipcs -o -x /usr/bin/ipcs ]
	then
		# System has SysV IPC
		CDDAMETHOD=1
		CDDAREADMETHOD=1

		if [ -c /dev/dsp ]
		then
			HAS_OSS=1
			CDDAWRITEMETHOD=1
		else
			CDDAWRITEMETHOD=8
		fi
	else
		# No SysV IPC: disable CDDA
		CDDAMETHOD=0
		CDDAREADMETHOD=0
		CDDAWRITEMETHOD=0
	fi
	CDDAREADCHUNKBLOCKS=4
	IOCDEF=0

	if [ "$OS_SYS" = AIX ]
	then
		# IBM AIX
		DEVPREF=/dev/rcd
		DEVSUFF=
		FIRST=0
		IOCDEF=3
		CDDAMETHOD=1
		if [ $IS_SCSI -eq 1 ]
		then
			CDDAREADMETHOD=1
		else
			CDDAREADMETHOD=5
		fi
		if [ "$HAS_OSS" -ne 1 ]
		then
			CDDAWRITEMETHOD=5
		fi
	elif [ "$OS_SYS" = A/UX ]
	then
		# Apple A/UX
		DEVPREF=/dev/scsi/
		DEVSUFF=
		FIRST=3
	elif [ "$OS_SYS" = BSD/OS ]
	then
		# BSDI/WindRiver BSD/OS
		DEVPREF=/dev/rsd
		DEVSUFF=c
		FIRST=2
		IOCDEF=4
		SCSI_EMUL=1
		CDDAMETHOD=1
		CDDAREADMETHOD=1
		CDDAWRITEMETHOD=1
	elif [ "$OS_SYS" = dgux ]
	then
		# Data General DG/UX
		DEVPREF="/dev/scsi/scsi(ncsc@7(FFFB0000,7),"
		DEVSUFF=",0)"
		FIRST=2
	elif [ "$OS_SYS" = FreeBSD ]
	then
		# FreeBSD
		if [ $IS_SCSI -eq 1 -o $SCSI_EMUL -eq 1 ]
		then
			if [ -c /dev/rcd0 ]
			then
				DEVPREF=/dev/rcd
				DEVSUFF=c
			else
				DEVPREF=/dev/cd
				DEVSUFF=
			fi
		else
			if [ -c /dev/racd0 ]
			then
				DEVPREF=/dev/racd
				DEVSUFF=c
			else
				DEVPREF=/dev/acd
				DEVSUFF=
			fi
		fi
		FIRST=0
		if [ $SCSI_EMUL -eq 1 ]
		then
			IOCDEF=9
		else
			IOCDEF=1
		fi
		CDDAMETHOD=1
		if [ $IS_SCSI -eq 1 -o $SCSI_EMUL -eq 1 ]
		then
			CDDAREADMETHOD=1
		else
			CDDAREADMETHOD=4
		fi
		CDDAWRITEMETHOD=1
	elif [ "$OS_SYS" = HP-UX ]
	then
		case "$OS_REL" in
		[AB].09*)	# HP-UX 9.x
			DEVPREF=/dev/rdsk/c201d
			DEVSUFF=s0
			FIRST=4
			;;
		B.10*)		# HP-UX 10.x
			DEVPREF=/dev/rdsk/c0t
			DEVSUFF=s0
			FIRST=4
		;;
		B.11*)		# HP-UX 11.x
			DEVPREF=/dev/rdsk/c0t
			DEVSUFF=s0
			FIRST=4
			;;
		*)
			OS_REL=unknown
			;;
		esac
		IOCDEF=7
		SCSI_EMUL=1
		CDDAMETHOD=1
		CDDAREADMETHOD=1
		if [ "$HAS_OSS" -eq 0 ]
		then
			CDDAWRITEMETHOD=4
		fi
	elif [ "$OS_SYS" = IRIX -o "$OS_SYS" = IRIX64 ]
	then
		# SGI IRIX
		DEVPREF=`hinv | grep CDROM | line | \
		    sed 's/^.*controller \([0-9]*\).*$/\/dev\/scsi\/sc\1d/'`
		DEVSUFF=l0
		FIRST=`hinv | grep CDROM | line | \
			sed 's/^.*unit \([0-9]*\).*$/\1/'`
		CDDAMETHOD=1
		CDDAREADMETHOD=1
		if [ "$HAS_OSS" -eq 0 ]
		then
			CDDAWRITEMETHOD=3
		fi
	elif [ "$OS_SYS" = Linux ]
	then
		# Linux
		ARCHBIN="${XMCDLIB}/bin-${OS_SYS_T}-${OS_MACH_T}"

		if [ $IS_SCSI -eq 1 -o $SCSI_EMUL -eq 1 ]
		then
			DEVPREF=/dev/scd
			FIRST=0
		else
			DEVPREF=/dev/cdrom
			FIRST=
		fi
		DEVSUFF=
		BLKDEV=1
		if [ $SCSI_EMUL -eq 1 ]
		then
			IOCDEF=8
		else
			IOCDEF=1
		fi
		CDDAMETHOD=1
		if [ $IS_SCSI -eq 1 -o $SCSI_EMUL -eq 1 ]
		then
			CDDAREADMETHOD=1
		else
			CDDAREADMETHOD=3
		fi
		if [ -d /proc/asound ] && [ -x ${ARCHBIN}/has_alsa ] && \
		   ${ARCHBIN}/has_alsa
		then
			# System is running ALSA and xmcd/cda has
			# native ALSA support compiled in
			$ECHO
			$ECHO "  Your system is running the ALSA sound driver."
			$ECHO "  You can have xmcd operate in native ALSA mode"
			$ECHO "  or use the OSS emulation capability in ALSA."
			YNDEF=y
			get_yn "\n  Do you want to use the native ALSA mode"
			if [ $? -eq 0 ]
			then
				CDDAWRITEMETHOD=6
			fi
		fi
		CDDAREADCHUNKBLOCKS=8
	elif [ "$OS_SYS" = NetBSD -o "$OS_SYS" = OpenBSD ]
	then
		# NetBSD/OpenBSD
		if [ $IS_SCSI -eq 1 ]
		then
			DEVPREF=/dev/rcd
		else
			DEVPREF=/dev/rwcd
		fi
		case "$OS_MACH" in
		*86)
			DEVSUFF=d
			;;
		*)
			DEVSUFF=c
			;;
		esac
		FIRST=0
		IOCDEF=2
		CDDAMETHOD=1
		if [ $IS_SCSI -eq 1 ]
		then
			CDDAREADMETHOD=1
		else
			CDDAREADMETHOD=4
		fi
		CDDAWRITEMETHOD=1
	elif [ "$OS_SYS" = OSF1 ]
	then
		case "$OS_MACH" in
		alpha)	# Digital OSF/1
			if [ -d /devices/rdisk ]
			then
				DEVPREF=/devices/rdisk/cdrom
				DEVSUFF=c
				FIRST=2
			else
				DEVPREF=/dev/rrz
				DEVSUFF=c
				FIRST=4
			fi
			IOCDEF=6
			SCSI_EMUL=1
			CDDAMETHOD=1
			CDDAREADMETHOD=1
			if [ "$HAS_OSS" -eq 0 ]
			then
				CDDAWRITEMETHOD=7
			fi
			;;
		*)
			OS_REL=unknown
			;;
		esac
	elif [ "$OS_SYS" = QNX ]
	then
		# QNX
		DEVPREF=/dev/cd
		DEVSUFF=
		FIRST=0
		BLKDEV=1
		IOCDEF=1
	elif [ "$OS_SYS" = SINIX-N ]
	then
		# SNI SINIX-N
		DEVPREF=/dev/ios0/rsdisk
		DEVSUFF=s0
		FIRST=005
	elif [ "$OS_SYS" = SINIX-P ]
	then
		# SNI SINIX-P
		DEVPREF=/dev/ios0/rsdisk
		DEVSUFF=s0
		FIRST=006
	elif [ "$OS_SYS" = SunOS -o "$OS_SYS" = Solaris ]
	then
		case "$OS_REL" in
		4.*)	# SunOS 4.x
			case `arch -k` in
			sun4[cmu])
				DEVPREF=/dev/rsr
				DEVSUFF=
				FIRST=0
				;;
			*)
				OS_REL=unknown
				;;
			esac
			CDDAREADCHUNKBLOCKS=8
			;;
		5.*)	# SunOS 5.x
			YNDEF=y
			if get_old_cparm "solaris2VolumeManager"
			then
				if echo "$PARMVAL" | fgrep -i false >/dev/null
				then
					YNDEF=n
				fi
			fi
			if get_yn \
			"  Does your system use the Volume Manager (/usr/sbin/vold)"
			then
				DEVPREF=/vol/dev/aliases/cdrom
				DEVSUFF=
				FIRST=0
				VOLMGT=True
				CLOSEONEJECT=True
			else
				DEVPREF=/dev/rdsk/c0t
				DEVSUFF=d0s2
				FIRST=6
			fi
			IOCDEF=1
			CDDAMETHOD=1
			if [ $IS_SCSI -eq 1 ]
			then
				CDDAREADMETHOD=1
			else
				CDDAREADMETHOD=2
			fi
			if [ "$HAS_OSS" -eq 0 ]
			then
				CDDAWRITEMETHOD=2
			fi
			CDDAREADCHUNKBLOCKS=8
			;;
		*)
			OS_REL=unknown
			;;
		esac
	elif [ "$OS_SYS" = ULTRIX ]
	then
		case "$OS_MACH" in
		RISC)	# Digital Ultrix
			DEVPREF=/dev/rrz
			DEVSUFF=c
			FIRST=4
			;;
		*)
			OS_REL=unknown
			;;
		esac
	elif [ -x /bin/ftx ] && ftx
	then
		case "$OS_REL" in
		4.*)	
			if [ -x /bin/hppa ] && hppa
			then
				# Stratus FTX SVR4/PA-RISC
				DEVPREF=/dev/rcdrom/c0a2d
				DEVSUFF=l0
				FIRST=0
			else
				# On non-supported FTX variants
				OS_REL=unknown
			fi
			;;
		*)
			OS_REL=unknown
			;;
		esac
	elif [ -x /bin/i386 -o -x /sbin/i386 ] && i386
	then
		case "$OS_REL" in
		3.2)	# SCO UNIX/Open Desktop/Open Server
			DEVPREF=/dev/rcd
			DEVSUFF=
			FIRST=0
			IOCDEF=5
			SCSI_EMUL=1
			CDDAMETHOD=1
			CDDAREADMETHOD=1
			CDDAWRITEMETHOD=1
			CDDAREADCHUNKBLOCKS=8
			;;
		4.0)	# UNIX SVR4.0/x86
			DEVPREF=/dev/rcdrom/cd
			DEVSUFF=
			FIRST=0
			;;
		4.1)	# UNIX SVR4.1/x86
			DEVPREF=/dev/rcdrom/cdrom
			DEVSUFF=
			FIRST=1
			;;
		4.2)	# UNIX SVR4.2/x86 (UnixWare 1.x)
			DEVPREF=/dev/rcdrom/cdrom
			DEVSUFF=
			FIRST=1
			;;
		4*MP)	# UNIX SVR4.2MP/x86 (UnixWare 2.x)
			DEVPREF=/dev/rcdrom/cdrom
			DEVSUFF=
			FIRST=1
			CDDAMETHOD=1
			CDDAREADMETHOD=1
			CDDAWRITEMETHOD=1
			;;
		5)	# UNIX SVR5/x86 (UnixWare 7, Caldera Open UNIX 8)
			DEVPREF=/dev/rcdrom/cdrom
			DEVSUFF=
			FIRST=1
			CDDAMETHOD=1
			CDDAREADMETHOD=1
			CDDAWRITEMETHOD=1
			CDDAREADCHUNKBLOCKS=8
			;;
		5.*)	# UNIX SVR5.*/x86 (UnixWare 7.x, Caldera Open UNIX 8)
			DEVPREF=/dev/rcdrom/cdrom
			DEVSUFF=
			FIRST=1
			CDDAMETHOD=1
			CDDAREADMETHOD=1
			CDDAWRITEMETHOD=1
			CDDAREADCHUNKBLOCKS=8
			;;
		*)
			OS_REL=unknown
			;;
		esac
	elif [ -x /bin/m88k ] && m88k
	then
		case "$OS_REL" in
		4.0)	# UNIX SVR4.0/88k
			DEVPREF=/dev/rdsk/m187_c0d
			DEVSUFF=s7
			FIRST=3
			;;
		*)
			OS_REL=unknown
			;;
		esac
	elif [ -r /vmunix ] && (strings /vmunix | fgrep NEWS-OS) \
		>/dev/null 2>&1
	then
		# Sony NEWS-OS
		DEVPREF=/dev/rsd
		DEVSUFF=c
		FIRST=06
	else
		OS_REL=unknown
	fi


	if [ "$OS_REL" = unknown ]
	then
		$ECHO "\nError: This is not a supported operating system."
		YNDEF=n
		get_yn "Would you like to proceed anyway"
		if [ $? -ne 0 ]
		then
			$ECHO "\nConfiguration aborted." >&2
			log_err -n "Configuration aborted by user"
			do_exit 3
		fi
	fi
}


config_drv_blksz()
{
	while :
	do
	    YNDEF=n
	    $ECHO "\n  Standard CD drives have a ${STD_BLKSZ}-byte block size."
	    if get_yn "  Does this drive have a non-standard block size"
	    then
		if get_str "  Enter the block size: [${NONSTD_BLKSZ}]"
		then
		    if [ `expr "$ANS" : '^[0-9]*$'` -eq 0 ]
		    then
			$ECHO "  Invalid input.  Try again."
			continue
		    elif [ `expr "$ANS" % 512` -ne 0 ]
		    then
			$ECHO "  The block size must be a multiple of 512."
			$ECHO "  Try again."
			continue
		    else
			DRVBLKSZ="$ANS"
			break
		    fi
		else
		    DRVBLKSZ=$NONSTD_BLKSZ
		    break
		fi
	    else
		DRVBLKSZ=$STD_BLKSZ
		break
	    fi
	done
}


show_cdda_config()
{
	if [ "$CDDAMETHOD" -eq 0 ]
	then
		$ECHO "\n  The CDDA modes are disabled."
		return
	fi

	$ECHO "\n  The CDDA configuration has been set as follows:"

	$ECHO "  - Extraction: \c"

	case "$CDDAREADMETHOD" in
	0)	$ECHO "disabled"
		;;
	1)	$ECHO "SCSI pass-through"
		;;
	2)	$ECHO "Solaris ioctl"
		;;
	3)	$ECHO "Linux ioctl"
		;;
	4)	$ECHO "FreeBSD ioctl"
		;;
	5)	$ECHO "AIX IDE ioctl"
		;;
	*)	$ECHO "unsupported method"
		;;
	esac

	$ECHO "  - Playback:   \c"

	case "$CDDAWRITEMETHOD" in
	0)	$ECHO "disabled"
		;;
	1)	$ECHO "Open Sound System (OSS)"
		;;
	2)	$ECHO "Solaris audio driver"
		;;
	3)	$ECHO "IRIX audio driver"
		;;
	4)	$ECHO "HP-UX audio driver"
		;;
	5)	$ECHO "AIX audio driver"
		;;
	6)	$ECHO "ALSA sound driver"
		;;
	7)	$ECHO "OSF1 sound driver"
		;;
	8)	$ECHO "File & Pipe only"
		;;
	*)	$ECHO "unsupported method"
		;;
	esac
}


drive_config()
{
	EXITSTAT=0
	DRVNO=0
	SEDLINE=

	case "$OS_SYS" in
	Linux|FreeBSD|NetBSD|OpenBSD)
		SCSIDEF=n
		;;
	*)
		SCSIDEF=y
		;;
	esac

	while :
	do
	    $ECHO "\nConfiguring drive $DRVNO..."
	    NUMDISCS=
	    SCSI_EMUL=0

	    #
	    # Set default device path name
	    #

	    YNDEF=$SCSIDEF
	    if get_yn "\n  Does this drive use a SCSI interface"
	    then
		IS_SCSI=1	
	    else
		IS_SCSI=0

		$ECHO "\n  Non-SCSI drives are currently supported only on the"
		$ECHO "  following platforms:\n"
		$ECHO "\tBSDI/WindRiver BSD/OS"
		$ECHO "\tCompaq/HP Tru64 UNIX, Digital UNIX"
		$ECHO "\tFreeBSD"
		$ECHO "\tHP-UX"
		$ECHO "\tIBM AIX"
		$ECHO "\tLinux"
		$ECHO "\tNetBSD"
		$ECHO "\tOpenBSD"
		$ECHO "\tQNX"
		$ECHO "\tSCO Open Server"
		$ECHO "\tSun Solaris"
		$ECHO "\n  You must have kernel driver support for your CD"
		$ECHO "  drive type."

		YNDEF=y
		get_yn "\n  Do you want to continue"
		if [ $? -ne 0 ]
		then
		    $ECHO "\nConfiguration aborted." >&2
		    log_err -n "Configuration aborted by user."
		    do_exit 1
		fi
	    fi

	    if [ $IS_SCSI -eq 0 ]
	    then
		EMUL_NAME="SCSI Emulation"

		case "$OS_SYS" in
		Linux)
		    EMUL_IF="ide-scsi"
		    ;;
		FreeBSD)
		    EMUL_IF="ATAPI-CAM"
		    ;;
		*)
		    ;;
		esac

		if [ -n "$EMUL_IF" ]
		then
		    YNDEF=n
		    get_yn \
		    "\n  Are you using the $EMUL_NAME (${EMUL_IF}) interface"
		    if [ $? -eq 0 ]
		    then
			SCSI_EMUL=1
		    fi
		fi
	    fi

	    # Set platform specific parameters
	    set_platform_parms

	    if [ $DRVNO -eq 0 ]
	    then
		DEVNO=$FIRST
	    elif [ -z "$DEVNO" ]
	    then
		DEVNO=1
	    fi

	    if [ $DRVNO -eq 0 ] && get_old_cparm "device" && [ -n "$PARMVAL" ]
	    then
		# If the old device path has the expected prefix for the
		# type of device, then use the old device name as the
		# default
		n="`expr length "$DEVPREF" 2>/dev/null`"
		m="`expr $PARMVAL : "$DEVPREF" 2>/dev/null`"
		if [ -n "$n" -a "$n" != 0 -a "$n" = "$m" ]
		then
		    DEFAULT_DEV=$PARMVAL
		else
		    DEFAULT_DEV="${DEVPREF}${DEVNO}${DEVSUFF}"
		fi
	    else
		DEFAULT_DEV="${DEVPREF}${DEVNO}${DEVSUFF}"
	    fi

	    while :
	    do
		if get_str "\n  Enter device path: [$DEFAULT_DEV]"
		then
		    XMCD_DEV=$ANS
		else
		    XMCD_DEV=$DEFAULT_DEV
		fi

		if [ $VOLMGT = True ]
		then
		    break
		fi
		if [ $BLKDEV = 0 -a -c $XMCD_DEV ]
		then
		    break
		fi
		if [ $BLKDEV = 1 -a -b $XMCD_DEV ]
		then
		    break
		fi

		$ECHO "  $XMCD_DEV is an invalid device."
	    done

	    CONFIGFILE=`basename $XMCD_DEV`
	    if [ $DRVNO -eq 0 ]
	    then
		XMCD_DEV0="$XMCD_DEV"
	    fi

	    drive_sel
	    CFGSTAT=$?
	    if [ $CFGSTAT -eq 2 ]
	    then
		continue
	    elif [ $CFGSTAT -ne 0 ]
	    then
		$ECHO "\nConfiguration aborted." >&2
		log_err -n "Configuration aborted by user."
		do_exit $CFGSTAT
	    fi

	    if [ $NUMDISCS -gt 1 ]
	    then
		$ECHO "\n  This drive is a ${NUMDISCS}-disc changer."

		case $CHGMETHOD in
		1)
		    # SCSI LUN addressing method
		    $ECHO "\n  In order to change discs, your system must support separate device"
		    $ECHO "  nodes for each LUN of the multi-disc changer."

		    YNDEF=y
		    if get_yn "  Does your OS platforms support this?"
		    then
			$ECHO "\n  Please enter the device nodes now."

			n=1
			DEVLIST=
			LUNDEV=$XMCD_DEV
			while [ $n -le $NUMDISCS ]
			do
			    #
			    # Construct a default device
			    #
			    if [ "$OS_SYS" != HP-UX ] && \
			       (echo $LUNDEV | grep "s[0-9]*$") >/dev/null 2>&1
			    then
				LUNPREF=`echo $LUNDEV | \
				    sed 's/\(.*\)[0-9]s[0-9]*/\1/'`
				LUNNUM=`echo $LUNDEV | \
				    sed 's/.*\([0-9]\)s[0-9]*/\1/'`
				LUNSUFF=`echo $LUNDEV | \
				    sed 's/.*[0-9]\(s[0-9]*\)/\1/'`
			    else
				LUNPREF=`echo $LUNDEV | \
				    sed 's/\(.*\)[0-9][^0-9]*/\1/'`
				LUNNUM=`echo $LUNDEV | \
				    sed 's/.*\([0-9]\)[^0-9]*/\1/'`
				LUNSUFF=`echo $LUNDEV | \
				    sed 's/.*[0-9]\([^0-9]*\)/\1/'`
			    fi

			    if [ -n "$LUNNUM" ]
			    then
				if [ $n -gt 1 ]
				then
				    LUNNUM=`expr $LUNNUM + 1`
				fi
			    else
				LUNNUM=`expr $n - 1`
			    fi

			    SAVDEV=$LUNDEV
			    LUNDEV="${LUNPREF}${LUNNUM}${LUNSUFF}"

			    if get_str "  Disc $n: [$LUNDEV]"
			    then
				LUNDEV=$ANS
			    fi

			    if [ $VOLMGT = False ]
			    then
				if [ $BLKDEV = 0 -a ! -c $LUNDEV ]
				then
				    $ECHO "  $LUNDEV is invalid.  Try again."
				    LUNDEV=$SAVDEV
				    continue
				fi
				if [ $BLKDEV = 1 -a ! -b $LUNDEV ]
				then
				    $ECHO "  $LUNDEV is invalid.  Try again."
				    LUNDEV=$SAVDEV
				    continue
				fi
			    fi

			    if [ -z "$DEVLIST" ]
			    then
				DEVLIST=$LUNDEV
			    else
				DEVLIST="${DEVLIST};${LUNDEV}"
			    fi

			    n=`expr $n + 1`
			done
		    else
			$ECHO "  It will be treated as a single disc player."
			CHGMETHOD=0
			NUMDISCS=1
			MULTIPLAY=False
			DEVLIST=$XMCD_DEV
		    fi
		    ;;
		2)
		    # SCSI medium changer method
		    $ECHO "\n  In order to change discs, your system must support a separate device"
		    $ECHO "  node for the medium changer mechanism."

		    DEVLIST=$XMCD_DEV

		    YNDEF=y
		    if get_yn "  Does your OS platforms support this?"
		    then
			while :
			do
			    LUNDEV=/dev/changer
			    if get_str \
			    "\n  Enter the medium changer device: [$LUNDEV]"
			    then
				LUNDEV=$ANS
			    fi

			    if [ ! -c $LUNDEV ]
			    then
				$ECHO "  $LUNDEV is invalid.  Try again."
				continue
			    fi

			    DEVLIST="${DEVLIST};${LUNDEV}"
			    break
			done
		    else
			$ECHO \
			"\n  The drive will be treated as a single disc player."
			CHGMETHOD=0
			NUMDISCS=1
			MULTIPLAY=False
			DEVLIST=$XMCD_DEV
		    fi
		    ;;
		3)
		    # OS ioctl method
		    DEVLIST=$XMCD_DEV
		    ;;
		*)
		    # Unsupported changer method
		    DEVLIST=$XMCD_DEV
		    ;;
		esac
	    else
		DEVLIST=$XMCD_DEV
	    fi

	    # Config drive blocksize
	    config_drv_blksz

	    DRVNOTICE="\n  The configuration disables these features:"

	    if [ $PLAYTI = False ]
	    then
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The Previous Index and Next Index buttons."
	    fi

	    if [ $CADDYLOCK = False ]
	    then
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The caddy/tray lock."
	    fi

	    if [ $LOAD = False ]
	    then
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - Software-controlled caddy/tray load."
	    fi

	    if [ $PAUSE = False -a $VENDOR = 0 ]
	    then
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - Audio pause/resume function."
	    fi

	    case "$VOLCTL" in
	    0)
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The volume, balance and channel routing controls."
		VOLSUPP=False
		BALSUPP=False
		CHRSUPP=False
		;;
	    1)
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The balance and channel routing controls."
		VOLSUPP=True
		BALSUPP=False
		CHRSUPP=False
		;;
	    2)
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The volume and channel routing controls."
		VOLSUPP=False
		BALSUPP=True
		CHRSUPP=False
		;;
	    3)
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The channel routing control."
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=False
		;;
	    4)
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The volume and balance controls."
		VOLSUPP=False
		BALSUPP=False
		CHRSUPP=True
		;;
	    5)
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The balance control."
		VOLSUPP=True
		BALSUPP=False
		CHRSUPP=True
		;;
	    6)
		$ECHO "$DRVNOTICE"
		DRVNOTICE="\c"
		$ECHO "  - The volume control."
		VOLSUPP=False
		BALSUPP=True
		CHRSUPP=True
		;;
	    7)
		VOLSUPP=True
		BALSUPP=True
		CHRSUPP=True
		;;
	    *)
		;;
	    esac

	    # Display CDDA configuration
	    show_cdda_config

	    $ECHO "\n  Creating the $CFGDIR/$CONFIGFILE file..."
	    GDEVLIST=`echo $DEVLIST | sed 's/\//\\\\\//g'`

	    sed \
	    -e "s/^!.*DO NOT MODIFY.*$/! DEVICE CONFIGURATION FILE/" \
	    -e "s/^logicalDriveNumber:.*/logicalDriveNumber:	$DRVNO/" \
	    -e "s/^cddaMethod:.*/cddaMethod:		$CDDAMETHOD/" \
	    -e "s/^cddaReadMethod:.*/cddaReadMethod:		$CDDAREADMETHOD/" \
	    -e "s/^cddaWriteMethod:.*/cddaWriteMethod:	$CDDAWRITEMETHOD/" \
	    -e "s/^cddaScsiModeSelect:.*/cddaScsiModeSelect:	$CDDASCSIMODESEL/" \
	    -e "s/^cddaScsiDensity:.*/cddaScsiDensity:	$CDDASCSIDENSITY/" \
	    -e "s/^cddaScsiReadCommand:.*/cddaScsiReadCommand:	$CDDASCSIREADCMD/" \
	    -e "s/^cddaReadChunkBlocks:.*/cddaReadChunkBlocks:	$CDDAREADCHUNKBLOCKS/" \
	    -e "s/^cddaDataBigEndian:.*/cddaDataBigEndian:	$CDDADATABE/" \
	    -e "s/^deviceInterfaceMethod:.*/deviceInterfaceMethod:	$METHOD/" \
	    -e "s/^mediumChangeMethod:.*/mediumChangeMethod:	$CHGMETHOD/" \
	    -e "s/^numDiscs:.*/numDiscs:		$NUMDISCS/" \
	    -e "s/^deviceList:.*/deviceList:	$GDEVLIST/" \
	    -e "s/^driveVendorCode:.*/driveVendorCode:	$VENDOR/" \
	    -e "s/^playAudio12Support:.*/playAudio12Support:	$PLAY12/" \
	    -e "s/^playAudioMSFSupport:.*/playAudioMSFSupport:	$PLAYMSF/" \
	    -e "s/^playAudio10Support:.*/playAudio10Support:	$PLAY10/" \
	    -e "s/^playAudioTISupport:.*/playAudioTISupport:	$PLAYTI/" \
	    -e "s/^loadSupport:.*/loadSupport:		$LOAD/" \
	    -e "s/^ejectSupport:.*/ejectSupport:		$EJECT/" \
	    -e "s/^modeSenseSetDBD:.*/modeSenseSetDBD:	$MODEDBD/" \
	    -e "s/^modeSenseUse10Byte:.*/modeSenseUse10Byte:	$MODE10BYTE/" \
	    -e "s/^volumeControlSupport:.*/volumeControlSupport:	$VOLSUPP/" \
	    -e "s/^balanceControlSupport:.*/balanceControlSupport:	$BALSUPP/" \
	    -e "s/^channelRouteSupport:.*/channelRouteSupport:	$CHRSUPP/" \
	    -e "s/^volumeControlTaper:.*/volumeControlTaper:	$VOLTAPER/" \
	    -e "s/^scsiAudioVolumeBase:.*/scsiAudioVolumeBase:	$VOLBASE/" \
	    -e "s/^pauseResumeSupport:.*/pauseResumeSupport:	$PAUSE/" \
	    -e "s/^strictPauseResume:.*/strictPauseResume:	$STRICTPR/" \
	    -e "s/^playPausePlay:.*/playPausePlay:		$PLAYPAUSEPLAY/" \
	    -e "s/^caddyLockSupport:.*/caddyLockSupport:	$CADDYLOCK/" \
	    -e "s/^curposFormat:.*/curposFormat:		$CURPOSFMT/" \
	    -e "s/^noTURWhenPlaying:.*/noTURWhenPlaying:	$PLAYNOTUR/" \
	    -e "s/^driveBlockSize:.*/driveBlockSize:		$DRVBLKSZ/" \
	    -e "s/^spinUpInterval:.*/spinUpInterval:		$SPINUPINTVL/" \
	    -e "s/^closeOnEject:.*/closeOnEject:		$CLOSEONEJECT/" \
	    -e "s/^multiPlay:.*/multiPlay:		$MULTIPLAY/" \
		< $CFGDIR/device.cfg > $CFGDIR/$CONFIGFILE
	    do_chmod 644 $CFGDIR/$CONFIGFILE 2>/dev/null
	    do_chown $OWNER $CFGDIR/$CONFIGFILE 2>/dev/null
	    do_chgrp $GROUP $CFGDIR/$CONFIGFILE 2>/dev/null
	    (cd $CFGDIR; rm -f ${CONFIGFILE}-${OS_NODE} ;\
	     do_link $CONFIGFILE ${CONFIGFILE}-${OS_NODE})

	    if [ $SCSI_EMUL -eq 1 ]
	    then
		$ECHO "\n  A set of default parameters have been configured for"
		$ECHO "  your drive.  If any function does not work, please refer"
		$ECHO "  to the \"xmcd Configuration Guide\" for help."
		$ECHO "  It can be found at the xmcd web site:\n  ${XMCD_URL}"
	    fi

	    YNDEF=n
	    if get_yn \
		    "\n  Do you have more CD drives on your system"
	    then
		DRVNO=`expr $DRVNO + 1`

		if [ -n "$DEVNO" ]
		then
		    case `expr $DEVNO : '.*'` in
		    0)
			;;
		    1)
			DEVNO=`expr $DEVNO + 1`
			;;
		    2)
			DEVNO=`echo $DEVNO | \
			    $AWK '{ printf("%02d\n", $1 + 1) }'`
			;;
		    3)
			DEVNO=`echo $DEVNO | \
			    $AWK '{ printf("%03d\n", $1 + 1) }'`
			;;
		    4)
			DEVNO=`echo $DEVNO | \
			    $AWK '{ printf("%04d\n", $1 + 1) }'`
			;;
		    *)
			DEVNO=`expr $DEVNO + 1`
			;;
		    esac
		fi
	    else
		break
	    fi
	done

	# Display SCSI emulation reminder
	if [ -n "$EMUL_IF" -a $SCSI_EMUL -eq 1 ]
	then
	    $ECHO "\n  Since you have configured your drive for ${EMUL_NAME},"
	    $ECHO "  Please be sure that your kernel is configured with the"
	    $ECHO "  ${EMUL_IF} interface."
	fi
}


common_config()
{
	if get_old_cparm "discogURLPrefix" && [ -n "$PARMVAL" ]
	then
		DISCOGURL="$PARMVAL"
	else
		DISCOGURL="-"
	fi

	#
	# Configure common.cfg file
	#
	chmod 644 $CFGDIR/common.cfg 2>/dev/null
	if [ -w $CFGDIR/common.cfg ]
	then
		$AWK '
		/^device:/	{
			printf("device:\t\t\t%s\n", device)
			next
		}
		/^cdinfoPath:/	{
			printf("cdinfoPath:\t\t%s\n", cdinfopath)
			next
		}
		/^internetOffline:/	{
			printf("internetOffline:\t%s\n", inetoffln)
			next
		}
		/^cddbUseProxy:/ {
			printf("cddbUseProxy:\t\t%s\n", useproxy)
			next
		}
		/^proxyServer:/ {
			printf("proxyServer:\t\t%s\n", proxyserver)
			next
		}
		/^proxyAuthorization:/ {
			printf("proxyAuthorization:\t%s\n", proxyauth)
			next
		}
		/^solaris2VolumeManager:/ {
			printf("solaris2VolumeManager:\t%s\n", volmgt)
			next
		}
		/^discogURLPrefix:/ {
			printf("discogURLPrefix:\t%s\n", discogurl)
			next
		}
		{
			print $0
		}' \
			device="$XMCD_DEV0" \
			cdinfopath="$CDINFOPATH" \
			inetoffln="$INETOFFLN" \
			useproxy="$USEPROXY" \
			proxyserver="$PROXYSERVER" \
			proxyauth="$PROXYAUTH" \
			volmgt="$VOLMGT" \
			discogurl="$DISCOGURL" \
			$CFGDIR/common.cfg > /tmp/xmcd.$$

		cp /tmp/xmcd.$$ $CFGDIR/common.cfg
		rm -f /tmp/xmcd.$$
		(cd $CFGDIR; rm -f common.cfg-${OS_NODE} ;\
			 do_link common.cfg common.cfg-${OS_NODE})
	else
		log_err -p "Cannot configure $CFGDIR/common.cfg"
		EXITSTAT=1 
	fi
}


#
# Main starts here
#

# Catch some signals
trap "rm -f $BTMPFILE $MTMPFILE; exit 1" 1 2 3 5 15

# Get platform information
OS_SYS=`(uname -s) 2>/dev/null`
OS_REL=`(uname -r) 2>/dev/null`
OS_MACH=`(uname -m) 2>/dev/null`
OS_NODE=`(uname -n) 2>/dev/null`

if [ -z "$OS_SYS" ]
then
	OS_SYS=unkn
fi
if [ -z "$OS_REL" ]
then
	OS_REL=unkn
fi
if [ -z "$OS_MACH" ]
then
	OS_MACH=unkn
fi
if [ -z "$OS_NODE" ]
then
	OS_NODE=unkn
fi

OS_SYS_T=`echo "$OS_SYS" | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`
OS_MACH_T=`echo "$OS_MACH" | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`
OS_REL_T=`echo "$OS_REL" | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`

# Use Sysv echo if possible
if [ -x /usr/5bin/echo ]				# SunOS SysV echo
then
	ECHO=/usr/5bin/echo
elif [ -z "`(echo -e a) 2>/dev/null | fgrep e`" ]	# GNU bash, etc.
then
	ECHO="echo -e"
else							# generic SysV
	ECHO=echo
fi
if [ "$OS_SYS" = QNX ]
then
	ECHO=echo
fi

# Use nawk if available
if [ -x /bin/nawk -o -x /usr/bin/nawk ]
then
	AWK=nawk
else
	# If awk doesn't work well on your system and you have gawk,
	# try changing the following to gawk.
	AWK=awk
fi

# Error log file handling
if [ -f $ERRFILE -a ! -w $ERRFILE ]
then
	ERRFILE=/dev/null
fi

$ECHO "\nXmcd version $VER Configuration Program"
$ECHO "Setting up for host: ${OS_NODE}"
$ECHO "----------------------------------------"

# Sanity check

if [ ! -w $CFGDIR ]
then
	log_err -p "No write permission in $CFGDIR"
	do_exit 1
fi

if [ ! -r $CFGDIR/device.cfg ]
then
	log_err -p "Cannot find $CFGDIR/device.cfg"
	do_exit 2
fi

fgrep cfgver=1 $CFGDIR/device.cfg >/dev/null 2>&1
if [ $? -ne 0 ]
then
	log_err -p "$CFGDIR/device.cfg version mismatch"
	do_exit 2
fi

if [ ! -d $CFGDIR/.tbl ]
then
	log_err -p "The directory $CFGDIR/.tbl is missing"
	do_exit 2
fi

#
# Set some defaults
#
DEVPREF=/dev/rcdrom
DEVSUFF=
FIRST=0
BLKDEV=0
STD_BLKSZ=2048
NONSTD_BLKSZ=512
DRVBLKSZ=${STD_BLKSZ}
VOLMGT=False
CLOSEONEJECT=False
INETOFFLN=False
USEPROXY=False
PROXYSERVER="localhost:80"
PROXYAUTH=False
CDINFOPATH=


#
# Configure CDDB server access
#
$ECHO "\n*** CDDB(R) ACCESS CONFIGURATION ***"
cddb_config

#
# Device and common configuration
#
$ECHO "\n\n*** DRIVE CONFIGURATION ***"
drive_config
common_config

do_exit $EXITSTAT

