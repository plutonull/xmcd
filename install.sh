#!/bin/sh
#
# @(#)install.sh	7.94 04/04/20
#
# Script to install the software binary and support files on
# the target system.
#
# Command line options:
#
# -n	Do not clean up distribution files after installing
# -b	Run in non-interactive mode, use default answers for all prompts.
#	The following environment variables can be used to override the
#	default answers:
#	BATCH_BINDIR		Binary executable installation directory
#	BATCH_XMCDLIB		Xmcd support files top level directory
#	BATCH_DISCOGDIR		Local discographies top level directory
#	BATCH_MANDIR		Manual page top level directory
#	BATCH_MANSUFFIX		Manual page file suffix
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

OPATH="$PATH"
PATH=/bin:/usr/bin:/sbin:/usr/sbin:/etc:/usr/freeware/bin:/usr/local/bin
export PATH

XMCD_VER=3.3.2
XMCD_URL=http://www.amb.org/xmcd/
DIRPERM=755
SCRPERM=755
FILEPERM=444
XBINPERM=4711
BINPERM=711
GDIRPERM=777
OWNER=bin
GROUP=bin
XBINOWNER=root
XBINGROUP=bin
ERRFILE=/tmp/xmcd.err
TMPFILE=/tmp/xmcdinst.$$
STARTUP_SCRIPT=.xmcd_start

#
# Utility functions
#

do_exit()
{
	if [ $1 -ne 0 ]
	then
	    $ECHO "\nERROR(s) have occurred in the installation."
	    if [ $ERRFILE != /dev/null ]
	    then
		$ECHO "See $ERRFILE for details."
	    fi
	    exit $1
	fi

	$ECHO "\nInstallation of xmcd is now complete."

	if [ -n "$SHLIBEXISTS" ]
	then
	    $ECHO "\nNOTICE: A CDDB shared library is already installed on"
	    $ECHO "your system ($SHLIBEXISTS) and install did not overwrite"
	    $ECHO "it.  If your existing CDDB shared library is old,"
	    $ECHO "please remove it and re-install xmcd again."
	fi

	if [ "$LDSO_NEEDCONF" -eq 1 ]
	then
	    $ECHO "\nNOTICE: You need to do the following to allow xmcd and"
	    $ECHO "cda to find the CDDB shared library.  Xmcd/cda may not"
	    $ECHO "work until this is done."
	    if [ -n "$LDSO_CONF" ]
	    then
		$ECHO "  - Make sure that $SHLIBDIR is in your"
		$ECHO "    $LDSO_CONF file."
	    fi
	    if [ -n "$LDCONFIG" ]
	    then
		$ECHO "  - Login as root and run $LDCONFIG to activate"
		$ECHO "    the CDDB shared library."
	    fi
	fi

	CKWEBSITE=0
	LAME_FOUND=0
	FAAC_FOUND=0
	pathdirs=`echo "$PATH" | sed 's/:/ /g'`
	for i in $pathdirs $BINTRYDIRS
	do
		if [ -x ${i}/lame -o -x ${i}/notlame ]
		then
			LAME_FOUND=1
		fi
		if [ -x ${i}/faac ]
		then
			FAAC_FOUND=1
		fi
	done
	if [ $LAME_FOUND -eq 0 ]
	then
	    $ECHO "\nNOTICE: If you want to use xmcd/cda to rip CDs to MP3"
	    $ECHO "format, you must install the LAME encoder software."
	    CKWEBSITE=1
	fi
	if [ $FAAC_FOUND -eq 0 ]
	then
	    $ECHO "\nNOTICE: If you want to use xmcd/cda to rip CDs to AAC and"
	    $ECHO "MP4 formats, you must install the FAAC encoder software."
	    CKWEBSITE=1
	fi
	if [ $CKWEBSITE -eq 1 ]
	then
	    $ECHO "\nPlease see the \"Downloads\" area on the xmcd web site"
	    $ECHO "for further information."
	fi

	if [ -n "$BATCH" ]
	then
	    $ECHO "\nNOTICE: Before using xmcd/cda for the first time, you"
	    $ECHO "must set up the software by running the following program:"
	    $ECHO "\n\t${XMCDLIB}/config/config.sh\n"
	    $ECHO "Xmcd/cda will not work until that is done."
	fi

	$ECHO "\nPlease read the $XMCDLIB/docs/README file.\n\n"
	exit 0
}

log_err()
{
	if [ "$1" = "-p" ]
	then
		$ECHO "Error: $2"
	fi
	$ECHO "$2" >>$ERRFILE
	ERROR=1
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

	if [ $RETSTAT -ne 0 -a -f $source ]
	then
		# Use hard link if a regular file
		ln $source $target 2>/dev/null
		RETSTAT=$?
	fi

	if [ $RETSTAT -ne 0 ]
	then
		log_err -n "Cannot link $source -> $target"
	fi

	return $RETSTAT
}

do_chown()
{
	if [ $1 != _default_ ]
	then
		chown $1 $2 2>/dev/null
	fi
}

do_chgrp()
{
	if [ $1 != _default_ ]
	then
		chgrp $1 $2 2>/dev/null
	fi
}

do_chmod()
{
	if [ $1 != _default_ ]
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
	do_chown $owner $dir
	do_chgrp $group $dir
	do_chmod $perm $dir
	return 0
}

inst_file()
{
	source=$1
	target=$2
	perm=$3
	owner=$4
	group=$5
	tdir=`dirname $target`

	if [ -n "$tdir" -a -d "$tdir" -a -w "$tdir" ]
	then
		if [ ! -f $source ]
		then
			$ECHO "\t$target NOT installed"
			log_err -n "Cannot install ${target}: file missing."
			return 1
		fi

		$ECHO "\t$target"
		rm -f $target
		cp $source $target
		if [ $? != 0 ]
		then
			log_err -n "Cannot install $target: file copy error."
			return 1
		fi

		if [ -f $target ]
		then
			do_chown $owner $target 2>/dev/null
			do_chgrp $group $target 2>/dev/null
			do_chmod $perm $target 2>/dev/null
		fi
	else
		$ECHO "\t$target NOT installed"
		log_err -n \
		    "Cannot install $target: target directory not writable."
		return 1
	fi
	return 0
}

link_prompt()
{
	$ECHO "\nFor security reasons, setuid programs (such as xmcd) search"
	$ECHO "only /usr/lib and/or /usr/ccs/lib for dynamic libraries."
	$ECHO "Some of the dynamic libraries that xmcd needs may not be in"
	$ECHO "the standard locations, thus xmcd may not be able to find"
	$ECHO "them."

	$ECHO "\nSymbolic links can be created now to correct this problem."

	$ECHO "\nDo you want this installation procedure to search your"
	$ECHO "system for needed dynamic libraries, and create symbolic links"
	$ECHO "of these libraries to /usr/lib\c"
	YNDEF=y
	get_yn " "
	if [ $? != 0 ]
	then
		$ECHO "\nNo links will be created.\n"
		$ECHO "If you encounter difficulty starting xmcd, see the FAQ"
		$ECHO "file in the xmcd distribution for further information."
		return 1
	fi
	$ECHO ""
	return 0
}

link_dynlibs()
{
	LINKFOUND=False

	#
	# Set LD_LIBRARY_PATH to point to all conceivable places where
	# dynamic libraries can hide
	#
	LD_LIBRARY_PATH=$LIBDIR:/usr/X/lib:/usr/X11/lib:/usr/X386/lib:/usr/X11R5/lib:/usr/X11R6/lib:/usr/openwin/lib:/usr/X/desktop:/usr/Motif/lib:/usr/Motif1.1/lib:/usr/Motif1.2/lib:/usr/Motif2.0/lib:/usr/dt/lib:/usr/lib/X11:/usr/ccs/lib:/usr/lib:/lib:/usr/freeware/lib32:/usr/local/lib
	export LD_LIBRARY_PATH

	# Find the ldd program
	for i in /bin /usr/bin /usr/ccs/bin
	do
		if [ -x $i/ldd ]
		then
			LDD=$i/ldd
		fi
	done

	if [ -z "$LDD" ]
	then
		# Can't locate ldd
		return
	fi

	if [ ! -r xmcd_d/xmcd ]
	then
		# Can't read xmcd binary
		return
	fi

	# Run ldd to determine its dynamic library configuration
	$LDD xmcd_d/xmcd >$TMPFILE 2>/dev/null

	if fgrep '=>' $TMPFILE >/dev/null 2>&1
	then
		# BSD/SunOS/SVR5 style ldd output
		DYNLIBS="`fgrep -v 'not found' $TMPFILE | \
			fgrep -v 'xmcd needs:' | \
			sed -e 's/^.*=> *//' -e 's/(.*)//' | tr '\015' ' '`"
		ERRLIBS="`fgrep 'not found' $TMPFILE | $AWK '{ print $1 }' | \
			tr '\015' ' '`"
	else
		# SVR4 style ldd output
		DYNLIBS="`fgrep 'loaded:' $TMPFILE | sed 's/^.*: //' | \
			tr '\015' ' '`"
		ERRLIBS="`fgrep 'error opening' $TMPFILE | \
			sed 's/^.*opening //' | tr '\015' ' '`"
	fi

	# Remove temp files
	rm -f $TMPFILE

	for i in $ERRLIBS _xoxo_
	do
		if [ "$i" = _xoxo_ ]
		then
			break
		fi

		# A needed library is not found in LD_LIBRARY_PATH
		log_err -p "\nNeeded library $i not found!  See the xmcd FAQ."
	done

	for i in $DYNLIBS _xoxo_
	do
		if [ "$i" = _xoxo_ ]
		then
			# Done processing
			break
		fi

		LIBNAME=`basename $i`
		DIRNAME=`dirname $i`

		if [ "$DIRNAME" = /usr/lib -o "$DIRNAME" = /usr/ccs/lib ]
		then
			# This is the standard library location
			continue
		fi

		if [ -f /usr/lib/$LIBNAME -o -f /usr/ccs/lib/$LIBNAME ]
		then
			# Link already there
			continue
		fi

		if [ $LINKFOUND = False ]
		then
			LINKFOUND=True

			if [ -z "$BATCH" ]
			then
				link_prompt
				if [ $? != 0 ]
				then
					return
				fi
			fi
		fi

		YNDEF=y
		if [ -n "$BATCH" ] || \
			get_yn "Link $DIRNAME/$LIBNAME to /usr/lib/$LIBNAME"
		then
			do_link $DIRNAME/$LIBNAME /usr/lib/$LIBNAME
		else
			$ECHO "$DIRNAME/$LIBNAME not linked."
		fi

	done
}

fix_discog()
{
	ogenre=$1
	genre=$2
	id=$3
	file=$DISCOGDIR/$genre/$id/index.html
	gn=`echo $genre | sed -e 's/_/ /g' -e 's/\// -> /g'`
	gn_s=`echo $gn | sed 's/\//\\\\\//g'`

	if [ ! -f $file ]
	then
		return;
	fi

	out=1
	cat $file | while read line
	do
	    if [ "$line" = '<LI>Browse category:' ]
	    then
		echo "<LI><A HREF=\"../../index.html\">Main index</A>"
		echo "<LI><A HREF=\"../index.html\">$gn index</A>"
		out=0
		continue
	    elif [ "$line" = '</LI>' -a out -eq 0 ]
	    then
		out=1
		continue
	    fi

	    if [ $out -eq 1 ]
	    then
		if [ "$line" = '</BODY>' ]
		then
			echo "<HR>"
			echo "This directory: <B>$DISCOGDIR/$genre/$id</B>"
		fi
		echo "$line"
	    fi
	done | \
	sed -e "s/Generated by xmcd 2\..*$/Generated by xmcd $XMCD_VER/" \
	    -e 's/Copyright .* Ti Kan/Copyright (C) 1993-2004  Ti Kan/' \
	    -e 's/Browse .* Xmcd web site/Xmcd official web site/' \
	    -e 's/Browse .* CDDB web site/CDDB - The #1 music info source/' \
	    -e 's/Browse music reviews/Yahoo! music reviews/' \
	    -e 's/How to set up Local/How to use Local/' \
	    -e "s/$ogenre $id/$gn_s $id/" \
	    -e "s/$ows_s/$XMCDURL_S/g" \
		>${file}.new 2>/dev/null

	sed 's/\.\.\/\.\./\.\.\/\.\.\/\.\./g' <${file}.new >$file
	chmod 666 $file 2>/dev/null
	rm -f ${file}.new
}


#
# Main execution starts here
#

# Catch some signals
trap "rm -f $TMPFILE; exit 1" 1 2 3 5 15

#
# Get platform information
#
OS_SYS=`(uname -s) 2>/dev/null`
OS_REL=`(uname -r) 2>/dev/null`
if [ "$OS_SYS" = IRIX ]
then
	OS_MACH=mips
else
	OS_MACH=`(uname -m) 2>/dev/null`
fi

if [ -z "$OS_SYS" ]
then
	OS_SYS=unknown
fi
if [ -z "$OS_REL" ]
then
	OS_REL=unknown
fi
if [ -z "$OS_MACH" ]
then
	OS_MACH=unknown
fi

OS_SYS_T=`echo "$OS_SYS" | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`
OS_MACH_T=`echo "$OS_MACH" | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`
OS_REL_T=`echo "$OS_REL" | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`

# Use Sysv echo if possible
if [ -x /usr/5bin/echo ]
then
	ECHO=/usr/5bin/echo				# SunOS SysV echo
elif [ -z "`(echo -e a) 2>/dev/null | fgrep e`" ]
then
	ECHO="echo -e"					# GNU bash, etc.
else
	ECHO=echo					# generic SysV
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

# Remove old error log file
ERROR=0
rm -f $ERRFILE
if [ -f $ERRFILE ]
then
	$ECHO "Cannot remove old $ERRFILE: error logging not enabled."
	ERRFILE=/dev/null
fi

# Parse command line args
NOREMOVE=0
BATCH=
if [ $# -gt 1 ]
then
	for i in $*
	do
		case "$i" in
		-n)
			# Whether to remove distribution files after
			# installation
			NOREMOVE=1
			;;
		-b)
			# Specify batch mode
			BATCH=$i
			;;
		*)
			log_err -p "Unknown option: $i"
			do_exit 1
			;;
		esac
	done
elif [ -f docs_d/INSTALL -a -f docs_d/PORTING ]
then
	# Do not remove if running from source code directory
	NOREMOVE=1
fi


# Implement platform-specific features and deal with OS quirks
LINKLIBS=False
SHELL=/bin/sh
SCO=False
LDSO_NEEDCONF=0
SHLIBEXISTS=""

if [ "$OS_SYS" = A/UX ]
then
	# Apple A/UX
	SHELL=/bin/ksh
elif [ "$OS_SYS" = FreeBSD ]
then
	# FreeBSD
	OS_REL_T=`echo "$OS_REL_T" | sed 's/\..*$//'`
	OS_SYS_T=${OS_SYS_T}_${OS_REL_T}
elif [ "$OS_SYS" = HP-UX ]
then
	# HP-UX
	SHELL=/bin/ksh
elif [ "$OS_SYS" = Linux ]
then
	# Linux
	LDSO_CONF=/etc/ld.so.conf
	LDCONFIG=/sbin/ldconfig
elif [ "$OS_SYS" = SunOS -o "$OS_SYS" = Solaris ]
then
	case "$OS_REL" in
	4.*)
		# SunOS 4.x
		LDCONFIG=/usr/etc/ldconfig
		;;
	5.*)
		# SunOS 5.x
		LINKLIBS=True
		OS_SYS_T=Solaris
		;;
	*)
		;;
	esac
elif [ "$OS_SYS" = ULTRIX ]
then
	# Digital Ultrix
	SHELL=/bin/sh5
elif [ -x /bin/ftx ] && /bin/ftx
then
	case "$OS_REL" in
	4.*)
		# Stratus FTX SVR4
		LINKLIBS=True
		;;
	*)
		;;
	esac
elif [ -x /bin/i386 -o -x /sbin/i386 ] && i386
then
	case "$OS_REL" in
	3.*)
		if (uname -X | fgrep "Release = 3.2") >/dev/null 2>&1
		then
			# SCO UNIX/Open Desktop/Open Server
			SCO=True
			OS_REL_T=`uname -X 2>/dev/null | \
				fgrep "Version = " | \
				sed -e 's/Version = //' -e 's/\..*$//'`
			if [ -z "$OS_REL_T" ]
			then
				OS_REL_T=`uname -v | sed 's/\..*$//'`
			fi
			OS_SYS_T=SCO_3.2v${OS_REL_T}
		fi
		;;
	4.*)
		# UNIX SVR4.x
		LINKLIBS=True
		OS_SYS_T=${OS_SYS_T}_${OS_REL_T}
		;;
	4*MP)
		# UNIX SVR4 MP
		LINKLIBS=True
		OS_SYS_T=${OS_SYS_T}_${OS_REL_T}
		;;
	5)
		# UNIX SVR5
		LINKLIBS=True
		OS_SYS_T=${OS_SYS_T}_${OS_REL_T}
		;;
	5.*)
		# UNIX SVR5.x
		LINKLIBS=True
		OS_SYS_T=${OS_SYS_T}_${OS_REL_T}
		;;
	*)
		;;
	esac
elif [ -x /bin/m88k ] && m88k
then
	case "$OS_REL" in
	4.*)
		# Motorola SVR4/m88k
		LINKLIBS=True
		;;
	*)
		;;
	esac
fi


$ECHO "\nInstalling \"xmcd\" Motif CD Player/Ripper version $XMCD_VER by Ti Kan"
$ECHO "----------------------------------------------------------------"
$ECHO "\nThis is free software and comes with no warranty."
$ECHO "See the GNU General Public License in the COPYING file"
$ECHO "for details."
$ECHO "\nThis software contains support for the Gracenote"
$ECHO "CDDB(R) Disc Recognition Service.  See the CDDB file"
$ECHO "for information."

# Check privilege
(id | fgrep 'uid=0(root)') >/dev/null 2>&1
if [ $? != 0 ]
then
	$ECHO "\n\nYou should be the super user to install xmcd."

	YNDEF=n
	if [ -z "$BATCH" ] && get_yn "\n  Proceed with installation anyway"
	then
		$ECHO "\nWARNING: Without super-user privilege, some files may"
		$ECHO "not be properly installed, or they may be installed"
		$ECHO "with incorrect permissions."

		XBINPERM=711
		XBINOWNER=_default_
		OWNER=_default_
		GROUP=_default_
	else
		log_err -p "Not super user: installation aborted by user."
		do_exit 1
	fi
fi


# Check existence of binaries

MISSING=
for i in xmcd_d/xmcd cda_d/cda util_d/gobrowser cdda_d/has_alsa
do
	if [ ! -f $i ]
	then
		MISSING="$MISSING $i"
	fi
done

if [ -n "$MISSING" ]
then
	$ECHO "\n\nThe following executable binaries are missing:\n"
	for i in $MISSING
	do
		$ECHO "\t$i"
	done
	$ECHO "\nIf you have the xmcd source code distribution, make sure"
	$ECHO "you compile the source code to generate the binaries first."
	$ECHO "See the INSTALL file for details."
	$ECHO "\nIf you have the xmcd binary distribution, it is probably"
	$ECHO "corrupt."

	YNDEF=n
	if [ -z "$BATCH" ] && get_yn "\n  Proceed with installation anyway"
	then
		$ECHO "\nThe missing files will not be installed."
	else
		log_err -p "Missing binaries: installation aborted by user."
		do_exit 1
	fi
fi


# Determine BINDIR

BINTRYDIRS="\
	/usr/bin/X11 \
	/usr/X/bin \
	/usr/X11/bin \
	/usr/X11R6/bin \
	/usr/X11R5/bin \
	/usr/X386/bin \
	/usr/openwin/bin \
	/usr/freeware/bin/X11 \
	/usr/freeware/bin \
	/usr/local/bin/X11 \
	/usr/local/bin \
	/usr/lbin \
"

if [ -z "$BINDIR" ]
then
	for i in $BINTRYDIRS
	do
		if [ -d $i ]
		then
			BINDIR=$i
			break
		fi
	done

	if [ -z "$BINDIR" ]
	then
		BINDIR=/usr/bin/X11
	fi
else
	BINDIR=`echo $BINDIR | sed 's/\/\//\//g'`
fi


$ECHO "\n\nThe X binary directory is where the xmcd and cda executable files"
$ECHO "will be installed.  It should be in each xmcd user's PATH environment."

if [ -n "$BATCH" ]
then
	if [ -n "$BATCH_BINDIR" ]
	then
		BINDIR=$BATCH_BINDIR
	fi
	$ECHO "\nUsing $BINDIR"
else
	while :
	do
		if get_str "\n  Enter X binary directory\n  [${BINDIR}]:"
		then
			UDIR="$ANS"
			if echo "$UDIR" | grep "^/" >/dev/null 2>&1
			then
			    if [ -d "$UDIR" ]
			    then
				BINDIR="$UDIR"
				break
			    else
				YNDEF=y
				if get_yn \
				"  Directory $UDIR does not exist.  Create it"
				then
				    BINDIR="$UDIR"
				    break
				else
				    $ECHO \
				    "  Error: Cannot install in ${UDIR}."
				fi
			    fi
			else
			    $ECHO "  Error: You must use an absolute path."
			fi
		else
			break
		fi
	done
fi


# Determine LIBDIR

LIBTRYDIRS="\
	/usr/lib/X11 \
	/usr/X/lib \
	/usr/X11/lib \
	/usr/X11R6/lib/X11 \
	/usr/X11R5/lib/X11 \
	/usr/X386/lib \
	/usr/openwin/lib \
	/usr/freeware/lib/X11 \
	/usr/freeware/lib \
	/usr/local/lib/X11 \
	/usr/local/lib \
"

if [ -z "$LIBDIR" ]
then
	for i in $LIBTRYDIRS
	do
		if [ -d $i ]
		then
			LIBDIR=$i
			break
		fi
	done

	if [ -z "$LIBDIR" ]
	then
		LIBDIR=/usr/lib/X11
	fi
else
	LIBDIR=`echo $LIBDIR | sed 's/\/\//\//g'`
fi


# Determine xmcd libdir

$ECHO "\n\nThe xmcd library directory is where xmcd/cda support files"
$ECHO "will be installed."

XMCDLIB=$LIBDIR/xmcd

if [ -n "$BATCH" ]
then
	if [ -n "$BATCH_XMCDLIB" ]
	then
		XMCDLIB=$BATCH_XMCDLIB
	fi
	$ECHO "\nUsing $XMCDLIB"
else
	while :
	do
		if get_str "\n  Enter xmcd library directory\n  [${XMCDLIB}]:"
		then
			UDIR="$ANS"
			if echo "$UDIR" | grep "^/" >/dev/null 2>&1
			then
			    if [ -d "$UDIR" ]
			    then
				XMCDLIB="$UDIR"
				break
			    else
				if get_yn \
				"  Directory $UDIR does not exist.  Create it"
				then
					XMCDLIB="$UDIR"
					break
				else
					$ECHO \
					"  Error: Cannot install in ${UDIR}."
				fi
			    fi
			else
			    $ECHO "  Error: You must use an absolute path."
			fi
		else
			break
		fi
	done
fi


# Determine DISCOGDIR

DISCOGDIR=$XMCDLIB/discog

if [ ! -d $DISCOGDIR ]
then
	$ECHO "\n\nThe Local Discographies directory is where documents,"
	$ECHO "images, sound clips and other files related to each CD may"
	$ECHO "be stored.  These can then be viewed or played using the"
	$ECHO "wwwWarp feature of xmcd."

	if [ -n "$BATCH" ]
	then
		if [ -n "$BATCH_DISCOGDIR" ]
		then
			DISCOGDIR=$BATCH_DISCOGDIR
		fi
		$ECHO "\nUsing $DISCOGDIR"
	else
		if get_str \
		"\n  Enter Local Discographies directory\n  [${DISCOGDIR}]:"
		then
			DISCOGDIR=$ANS
		fi
	fi
fi


# Determine MANDIR

if [ -z "$MANDIR" ]
then
	for i in	/usr/man/man.LOCAL \
			/usr/share/man/man1 \
			/usr/X11/man/man1 \
			/usr/X/man/man1 \
			/usr/X11R6/man/man1 \
			/usr/X11R5/man/man1 \
			/usr/X386/man/man1 \
			/usr/freeware/catman/u_man/cat1 \
			/usr/local/man/man1
	do
		if [ -d $i ]
		then
			MANDIR=$i
			break
		fi
	done

	if [ -z "$MANDIR" ]
	then
		MANDIR=/usr/man/man1
	fi

else
	MANDIR=`echo $MANDIR | sed 's/\/\//\//g'`
fi

$ECHO "\n\nThe on-line manual directory is where the man pages in"
$ECHO "in the xmcd package will be installed."

if [ -n "$BATCH" ]
then
	if [ -n "$BATCH_MANDIR" ]
	then
		MANDIR=$BATCH_MANDIR
	fi
	$ECHO "\nUsing $MANDIR"
else
	if get_str "\n  Enter on-line manual directory\n  [${MANDIR}]:"
	then
		MANDIR=$ANS
	fi
fi

if [ ! -d $MANDIR -a -z "$BATCH" ]
then
	YNDEF=y
	get_yn "  Directory $MANDIR does not exist.  Create it"
	if [ $? -ne 0 ]
	then
		$ECHO "  The xmcd on-line manual will not be installed."
		MANDIR=
	fi
fi

# Determine MANSUFFIX

if [ -n "$MANDIR" ]
then
	if [ -z "$MANSUFFIX" ]
	then
		MANSUFFIX=1
	fi

	if [ -n "$BATCH" ]
	then
		if [ -n "$BATCH_MANSUFFIX" ]
		then
			MANSUFFIX=$BATCH_MANSUFFIX
		fi
		$ECHO "Using ManSuffix $MANSUFFIX"
	else
		if get_str \
		"\n  Enter on-line manual file name suffix\n  [${MANSUFFIX}]:"
		then
			MANSUFFIX=$ANS
		fi
	fi
fi


# Remove old xmcd components

$ECHO "\n\nChecking for old xmcd components..."

# Old binaries
dirs=`echo "$OPATH" | $AWK -F: '{ for (i = 1; i <= NF; i++) print $i }'`
for i in $BINTRYDIRS
do
	dirs=`$ECHO "$dirs\n$i"`
done
dirs=`($ECHO "$dirs" | \
       sed -e 's/^[ 	]*//' -e '/^$/d' | \
       sort | uniq) 2>/dev/null`

if [ -n "$dirs" ]
then
	for i in $dirs
	do
	    if [ "$i" = "$BINDIR" -o "$i" = "." ]
	    then
		    continue
	    fi

	    for j in xmcd cda cddbcmd wm2xmcd dp2xmcd $STARTUP_SCRIPT
	    do
		tryfile=${i}/${j}
		if [ -f $tryfile -a -x $tryfile ]
		then
		    if [ -z "$BATCH" ]
		    then
			YNDEF=y
			if get_yn "Remove old executable $tryfile"
			then
			    rm -f $tryfile
			    if [ $? -ne 0 ]
			    then
				$ECHO "Cannot remove $tryfile."
			    fi
			fi
		    else
			rm -f $tryfile
		    fi
		fi
	    done
	done
fi

# Old xmcd app-defaults files
dirs=`for i in $LIBTRYDIRS $LIBDIR; do echo "$i"; done | sort | uniq`
for i in $dirs
do
	tryfile=${i}/app-defaults/XMcd
	if [ -f "$tryfile" ]
	then
		if [ -z "$BATCH" ]
		then
		    YNDEF=y
		    if get_yn "Remove old xmcd resource file $tryfile"
		    then
			rm -f $tryfile
			if [ $? -ne 0 ]
			then
			    $ECHO "Cannot remove ${tryfile}."
			fi
		    fi
		else
		    rm -f $tryfile
		fi
	fi
done


# Set architecture-specific binary and library directory
ARCHBIN="${XMCDLIB}/bin-${OS_SYS_T}-${OS_MACH_T}"
ARCHLIB="${XMCDLIB}/lib-${OS_SYS_T}-${OS_MACH_T}"


# Make all necessary directories
$ECHO "\n\nMaking directories..."

if [ ! -d "$BINDIR" ]
then
	make_dir $BINDIR $DIRPERM $OWNER $GROUP
fi
make_dir $XMCDLIB $DIRPERM $OWNER $GROUP
make_dir $XMCDLIB/app-defaults $DIRPERM $OWNER $GROUP
make_dir $XMCDLIB/config $DIRPERM $OWNER $GROUP
make_dir $XMCDLIB/config/.tbl $DIRPERM $OWNER $GROUP
make_dir $XMCDLIB/docs $DIRPERM $OWNER $GROUP
make_dir $XMCDLIB/help $DIRPERM $OWNER $GROUP
for i in xmcd_d/hlpfiles/[A-Z][a-z]
do
	if [ -d $i ]
	then
		j=`echo $i | sed 's/xmcd\_d\/hlpfiles\///'`
		make_dir $XMCDLIB/help/$j $DIRPERM $OWNER $GROUP
	fi
done
make_dir $XMCDLIB/pixmaps $DIRPERM $OWNER $GROUP
make_dir $XMCDLIB/scripts $DIRPERM $OWNER $GROUP
make_dir $ARCHBIN $DIRPERM $OWNER $GROUP
make_dir $ARCHLIB $DIRPERM $OWNER $GROUP
make_dir $DISCOGDIR $GDIRPERM $OWNER $GROUP

if [ -n "$MANDIR" ]
then
	make_dir $MANDIR $DIRPERM $OWNER $GROUP
fi

if [ "$DISCOGDIR" != "$XMCDLIB/discog" ]
then
	do_link "$DISCOGDIR" "$XMCDLIB/discog"
	$ECHO "\t$XMCDLIB/discog"
fi

# Rename the cddb directory if it exists
if [ ! -d $XMCDLIB/cdinfo -a -d $XMCDLIB/cddb ]
then
	mv $XMCDLIB/cddb $XMCDLIB/cdinfo
fi


# Install files

SHELL_S=`echo $SHELL | sed 's/\//\\\\\//g'`
BINDIR_S=`echo $BINDIR | sed 's/\//\\\\\//g'`
XMCDLIB_S=`echo $XMCDLIB | sed 's/\//\\\\\//g'`
DISCOGDIR_S=`echo $DISCOGDIR | sed 's/\//\\\\\//g'`
XMCDURL_S=`echo $XMCD_URL | sed 's/\//\\\\\//g'`

$ECHO "\nInstalling xmcd files..."

# Private binaries
for i in cddbcmd dp2xmcd wm2xmcd
do
	# Remove deprecated executables
	rm -f $ARCHBIN/$i $BINDIR/$i
done
inst_file xmcd_d/xmcd $ARCHBIN/xmcd $XBINPERM $XBINOWNER $GROUP
inst_file cda_d/cda $ARCHBIN/cda $XBINPERM $XBINOWNER $GROUP
inst_file util_d/gobrowser $ARCHBIN/gobrowser $BINPERM $OWNER $GROUP
inst_file cdda_d/has_alsa $ARCHBIN/has_alsa $BINPERM $OWNER $GROUP

rm -f $ARCHBIN/README
echo "The executable files in this directory are not intended to be
run directly.  Please use the startup wrapper scripts installed in
$BINDIR." >$ARCHBIN/README
do_chown $OWNER $ARCHBIN/README
do_chgrp $GROUP $ARCHBIN/README
do_chmod $FILEPERM $ARCHBIN/README
$ECHO "\t$ARCHBIN/README"

# X resource defaults file
inst_file xmcd_d/XMcd.ad $XMCDLIB/app-defaults/XMcd $FILEPERM $OWNER $GROUP
inst_file xmcd_d/XMcd_sgi.ad $XMCDLIB/app-defaults/XMcd.sgi \
	$FILEPERM $OWNER $GROUP

# XKeysymDB file
inst_file xmcd_d/XKeysymDB $XMCDLIB/app-defaults/XKeysymDB \
	$FILEPERM $OWNER $GROUP

# Documentation
rm -f $XMCDLIB/docs/*
for i in docs_d/*
do
	case $i in
	*SCCS)
		;;
	*RCS)
		;;
	*CVS)
		;;
	*)
		inst_file $i $XMCDLIB/docs/`basename $i` \
			$FILEPERM $OWNER $GROUP\
		;;
	esac
done

# Help files
rm -f $XMCDLIB/help/[A-Z]*.??? $XMCDLIB/help/[A-Z][a-z]/[A-Z]*.???
for i in xmcd_d/hlpfiles/[A-Z][a-z]/*.???
do
	j=`echo $i | sed 's/xmcd\_d\/hlpfiles\///'`
	inst_file $i $XMCDLIB/help/$j $FILEPERM $OWNER $GROUP
done

# Icon/pixmap files
for i in xmcd.icon \
	 xmcd_a.px \
	 xmcd_b.px \
	 xmcd.xpm \
	 xmcd.open.fti \
	 xmcd.closed.fti \
	 xmcd.ftr \
	 XMcd_sgi.icon
do
	inst_file misc_d/$i $XMCDLIB/pixmaps/$i $FILEPERM $OWNER $GROUP
done

# Configuration files
if [ -f $XMCDLIB/config/common.cfg ]
then
	diff $XMCDLIB/config/common.cfg libdi_d/common.cfg >/dev/null 2>&1
	if [ $? -ne 0 ]
	then
		# Save old config file for use by config.sh
		mv $XMCDLIB/config/common.cfg $XMCDLIB/config/common.cfg.old
	fi
fi
inst_file libdi_d/common.cfg $XMCDLIB/config/common.cfg \
	$FILEPERM $OWNER $GROUP
inst_file libdi_d/device.cfg $XMCDLIB/config/device.cfg \
	$FILEPERM $OWNER $GROUP
inst_file cdinfo_d/wwwwarp.cfg $XMCDLIB/config/wwwwarp.cfg \
	$FILEPERM $OWNER $GROUP

rm -f $XMCDLIB/config/.tbl/*
ENTRIES=`(cd libdi_d/cfgtbl; echo * | \
	sed -e 's/Imakefile//' -e 's/Makefile//' -e 's/SCCS//' -e 's/RCS//')`
for i in $ENTRIES
do
	if (fgrep "tblver=" libdi_d/cfgtbl/$i) >/dev/null 2>&1
	then
		inst_file libdi_d/cfgtbl/$i $XMCDLIB/config/.tbl/$i \
			$FILEPERM $OWNER $GROUP
	fi
done

# Configuration script
sed -e "s/^#!\/bin\/sh.*/#!$SHELL_S/" \
    -e "s/^BINDIR=.*/BINDIR=$BINDIR_S/" \
    -e "s/^XMCDLIB=.*/XMCDLIB=$XMCDLIB_S/" \
    -e "s/^DISCOGDIR=.*/DISCOGDIR=$DISCOGDIR_S/" \
    -e "s/^XMCD_URL=.*/XMCD_URL=$XMCDURL_S/" \
    -e "s/^OWNER=.*/OWNER=$OWNER/" \
    -e "s/^GROUP=.*/GROUP=$GROUP/" \
    <libdi_d/config.sh >/tmp/xmcdcfg.$$

rm -f $XMCDLIB/config/configure.sh
inst_file /tmp/xmcdcfg.$$ $XMCDLIB/config/config.sh $SCRPERM $OWNER $GROUP
rm -f /tmp/xmcdcfg.$$

# Convenience link to config.sh
if [ "$SCO" = True ]
then
	if [ -w /usr/lib/mkdev ]
	then
		$ECHO "\t/usr/lib/mkdev/xmcd"
		rm -f /usr/lib/mkdev/xmcd
		do_link $XMCDLIB/config/config.sh /usr/lib/mkdev/xmcd
	fi
fi

# Shell scripts
for i in ncsarmt ncsawrap nswrap gobrowser
do
	# Remove deprecated scripts
	rm -f $XMCDLIB/scripts/$i
done

sed -e "s/^#!\/bin\/sh.*/#!$SHELL_S/" \
    -e "s/^BINDIR=.*/BINDIR=$BINDIR_S/" \
    -e "s/^XMCDLIB=.*/XMCDLIB=$XMCDLIB_S/" \
    -e "s/^DISCOGDIR=.*/DISCOGDIR=$DISCOGDIR_S/" \
    -e "s/^XMCD_URL=.*/XMCD_URL=$XMCDURL_S/" \
    -e "s/^OWNER=.*/OWNER=$OWNER/" \
    -e "s/^GROUP=.*/GROUP=$GROUP/" \
    <misc_d/genidx.sh >$XMCDLIB/scripts/genidx
do_chown $OWNER $XMCDLIB/scripts/genidx
do_chgrp $GROUP $XMCDLIB/scripts/genidx
do_chmod $SCRPERM $XMCDLIB/scripts/genidx
$ECHO "\t$XMCDLIB/scripts/genidx"

# Local discographies
inst_file misc_d/bkgnd.gif $DISCOGDIR/bkgnd.gif $FILEPERM $OWNER $GROUP
inst_file misc_d/xmcdlogo.gif $DISCOGDIR/xmcdlogo.gif $FILEPERM $OWNER $GROUP
rm -f $DISCOGDIR/discog.html
sed -e "s/\$SHELL/$SHELL_S/g" \
    -e "s/\$BINDIR/$BINDIR_S/g" \
    -e "s/\$XMCDLIB/$XMCDLIB_S/g" \
    -e "s/file:\/\/localhost\/\$DISCOGDIR/file:\/\/localhost$DISCOGDIR_S/g" \
    -e "s/\$DISCOGDIR/$DISCOGDIR_S/g" \
    -e "s/\$XMCD_URL/$XMCDURL_S/g" \
    <misc_d/discog.htm >$DISCOGDIR/discog.html
do_chown $OWNER $DISCOGDIR/discog.html
do_chgrp $GROUP $DISCOGDIR/discog.html
do_chmod $FILEPERM $DISCOGDIR/discog.html
$ECHO "\t$DISCOGDIR/discog.html"
for i in xmcd cda
do
	# HTML man pages
	rm -f $DISCOGDIR/${i}.html
	inst_file ${i}_d/${i}.htm $DISCOGDIR/${i}.html $FILEPERM $OWNER $GROUP
done

# Startup wrapper script
rm -f $BINDIR/$STARTUP_SCRIPT
sed -e "s/^#!\/bin\/sh.*/#!$SHELL_S/" \
    -e "s/XMCD_LIBDIR=.*/XMCD_LIBDIR=$XMCDLIB_S/" \
	misc_d/start.sh >$BINDIR/$STARTUP_SCRIPT
do_chown $OWNER $BINDIR/$STARTUP_SCRIPT
do_chgrp $GROUP $BINDIR/$STARTUP_SCRIPT
do_chmod $SCRPERM $BINDIR/$STARTUP_SCRIPT
$ECHO "\t$BINDIR/$STARTUP_SCRIPT"

# Use startup wrapper script to invoke all supported programs
for i in xmcd cda
do
	rm -f $BINDIR/$i
	do_link $BINDIR/$STARTUP_SCRIPT $BINDIR/$i
	$ECHO "\t$BINDIR/$i"
done

# Manual page files
if [ -n "$MANDIR" -a -n "$MANSUFFIX" ]
then
	for i in xmcd cda
	do
		inst_file ${i}_d/${i}.man $MANDIR/${i}.$MANSUFFIX \
			$FILEPERM $OWNER $GROUP
	done
fi

# CDDB shared library if bundled
notfatal="This may not be a fatal problem."

f=`echo cddb_d/libcddb.s*`

if [ -f "$f" ]
then
	n=`basename $f`
	inst_file $f $ARCHLIB/$n $SCRPERM $OWNER $GROUP

	# Look for existing libcddb installation
	dlist="/usr/lib /lib"
	if [ -n "$LDSO_CONF" ]
	then
		dlist="$dlist `cat $LDSO_CONF 2>/dev/null`"
	fi

	for d in $dlist
	do
		if [ ! -d $d ]
		then
			continue
		fi

		# [ -L file ] is not universally supported hence this
		# weird syntax in a subshell with stderr directed to /dev/null
		# Try both sh and ksh
		if ( test -L $d/$n ) 2>/dev/null || \
		   ( ksh -c "test -L $d/$n" ) 2>/dev/null
		then
			if [ ! -w $d ]
			then
				# Existing CDDB shared library symlink
				# directory not writable
				$ECHO "\t$d/$n NOT linked"
				SHLIBEXISTS=$d/$n
			fi
			SHLIBDIR=$d
			break
		elif [ -f $d/$n ]
		then
			if [ ! -w $d/$n ]
			then
				# Existing CDDB shared library not writable
				$ECHO "\t$d/$n NOT linked"
				SHLIBEXISTS=$d/$n
			fi
			SHLIBDIR=$d
			break
		fi
	done

	if [ -z "$SHLIBEXISTS" ]
	then
		if [ -z "$SHLIBDIR" ]
		then
			if [ "$OS_SYS" = Linux ]
			then
				if [ -d /usr/local/lib ]
				then
					SHLIBDIR=/usr/local/lib
				else
					SHLIBDIR=/usr/lib
				fi
			else
				SHLIBDIR=/usr/lib
			fi
		fi

		# Make symlink
		do_link $ARCHLIB/$n $SHLIBDIR/$n
		if [ $? -eq 0 ]
		then
			$ECHO "\t$SHLIBDIR/$n"
		else
			$ECHO "\t$SHLIBDIR/$n NOT linked"
			if [ $OWNER = _default_ -a $LINKLIBS = False ]
			then
				log_err -n "$notfatal"
			fi
		fi

		if [ "$OS_SYS" = Linux ]
		then
			# Update ld.so.conf if needed
			fgrep $SHLIBDIR $LDSO_CONF >/dev/null 2>&1
			if [ $? -ne 0 ]
			then
				echo $SHLIBDIR >>$LDSO_CONF 2>/dev/null
				if [ $? -ne 0 ]
				then
					LDSO_NEEDCONF=1
				fi
			fi

			# Run ldconfig
			$LDCONFIG 2>/dev/null
			if [ $? -ne 0 ]
			then
				LDSO_NEEDCONF=1
			fi
		elif [ "$OS_SYS" = SunOS ]
		then
			case "$OS_REL" in
			4.*)
				# SunOS 4.x
				# Run ldconfig
				$LDCONFIG 2>/dev/null
				if [ $? -ne 0 ]
				then
					LDSO_NEEDCONF=1
				fi
				;;
			*)
				;;
			esac
		fi
	fi

	# Make symlink
	n2="$n"
	while :
	do
		x=`echo "$n2" | sed 's/\.[0-9]*$//'`
		if [ "$x" = "$n2" ]
		then
			break
		fi
		n2=$x

		(cd $ARCHLIB; do_link $n $n2) >/dev/null 2>&1

		if [ -z "$SHLIBEXISTS" ]
		then
			(cd $SHLIBDIR; do_link $n $n2) >/dev/null 2>&1
			if [ $? -eq 0 ]
			then
				$ECHO "\t$SHLIBDIR/$n2"
			else
				$ECHO "\t$SHLIBDIR/$n2 NOT linked"
				if [ $OWNER = _default_ -a $LINKLIBS = False ]
				then
					log_err -n "$notfatal"
				fi
			fi
		fi
	done
fi

# Set up local discography directories and files
if [ -d $DISCOGDIR ]
then
	$ECHO "\nSetting up xmcd Local Discography..."

	ows_s=`echo "http://metalab.unc.edu/tkan/xmcd/" | sed 's/\//\\\\\//g'`

	for d in		\
		blues		\
		classical	\
		country		\
		data		\
		folk		\
		jazz		\
		misc		\
		newage		\
		reggae		\
		rock		\
		soundtrack	\
		unclass
	do
	    case $d in
	    blues)
		newgenre=Blues/General_Blues
		;;
	    classical)
		newgenre=Classical/General_Classical
		;;
	    country)
		newgenre=Country/General_Country
		;;
	    data)
		newgenre=Data/General_Data
		;;
	    folk)
		newgenre=Folk/General_Folk
		;;
	    jazz)
		newgenre=Jazz/General_Jazz
		;;
	    misc)
		newgenre=Unclassifiable/General_Unclassifiable
		;;
	    newage)
		newgenre=New_Age/General_New_Age
		;;
	    reggae)
		newgenre=Reggae/General_Reggae
		;;
	    rock)
		newgenre=Rock/General_Rock
		;;
	    soundtrack)
		newgenre=Soundtrack/General_Soundtrack
		;;
	    unclass)
		newgenre=Unclassifiable/General_Unclassifiable
		;;
	    esac

	    # Make a basic set of xmcd-3.x compatible genre directories
	    make_dir $DISCOGDIR/$newgenre $GDIRPERM $OWNER $GROUP

	    pdir=`dirname $DISCOGDIR/$newgenre`
	    do_chown $OWNER $pdir
	    do_chgrp $GROUP $pdir
	    do_chmod $GDIRPERM $pdir

	    # Move old xmcd-2.x discography files into new location
	    if [ -d $DISCOGDIR/$d ]
	    then
		for e in $DISCOGDIR/$d/*
		do
		    if [ ! -d $e ]
		    then
			continue
		    fi

		    id=`basename $e`

		    if [ `expr "$id" : '[0-9,a-f][0-9,a-f][0-9,a-f][0-9,a-f][0-9,a-f][0-9,a-f][0-9,a-f][0-9,a-f]'` -ne 8 ]
		    then
			continue
		    fi

		    mkdir $DISCOGDIR/$newgenre/$id 2>/dev/null
		    chmod 777 $DISCOGDIR/$newgenre/$id 2>/dev/null

		    mv $e/* $e/.??* $DISCOGDIR/$newgenre/$id >/dev/null 2>&1

		    # Fix-up up local discography file
		    fix_discog $d $newgenre $id
		done
		rm -rf $DISCOGDIR/$d
	    fi
	done
fi

# Generate local discography index files
if [ -x $XMCDLIB/scripts/genidx ]
then
	$XMCDLIB/scripts/genidx
fi

if [ -z "$BATCH" ]
then
	# Run device-dependent config script
	if [ -r $XMCDLIB/config/config.sh ]
	then
		FROM_INSTALL_SH=1; export FROM_INSTALL_SH
		$SHELL $XMCDLIB/config/config.sh
		if [ $? -ne 0 ]
		then
			log_err -n "$XMCDLIB/config/config.sh failed."
		fi
	else
		log_err -p "Cannot execute $XMCDLIB/config/config.sh"
	fi
fi

# Set up dynamic library links
if [ $LINKLIBS = True ]
then
	link_dynlibs
fi

# Clean up
if [ $NOREMOVE = 0 ]
then
	rm -rf  common_d libdi_d misc_d xmcd_d cda_d \
		cdinfo_d docs_d cdda_d cddb_d util_d install.sh \
		xmcdbin.tar xmcd.tar
fi

do_exit $ERROR

