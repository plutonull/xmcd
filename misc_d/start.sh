#!/bin/sh
#
# @(#)start.sh	6.25 03/12/12
# Startup wrapper script for xmcd, cda and related programs.
# This script is used to setup the basic startup environment,
# and allows network sharing of xmcd files under different
# platforms and architectures.
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
PATH=${PATH}:/sbin:/usr/sbin:/bin:/usr/bin:/etc
export PATH


proc_env()
{
	if [ -z "$XMCD_LIBDIR" ]
	then
		XMCD_LIBDIR=/usr/lib/X11/xmcd
		export XMCD_LIBDIR
	fi

	SYS_T="$SYS"
	MACH_T="$MACH"
	REL_T="$REL"

	XFILESEARCHPATH=$XMCD_LIBDIR/%T/%N%S:$XFILESEARCHPATH
	XUSERFILESEARCHPATH=$HOME/.xmcdcfg/%N%S:$XUSERFILESEARCHPATH
	XKEYSYMDB=$XMCD_LIBDIR/app-defaults/XKeysymDB
	export XFILESEARCHPATH XUSERFILESEARCHPATH XKEYSYMDB

	# Platform-specific handling
	if [ "$SYS_T" = FreeBSD ]
	then
		# Differentiate between FreeBSD versions
		REL_T=`echo "$REL_T" | sed 's/\..*$//'`
		SYS_T=${SYS_T}_${REL_T}
	elif [ "$SYS_T" = SunOS ]
	then
		# Differentiate between SunOS 4.x and Solaris
		case "$REL_T" in
		4.*)
			;;
		5.*)
			SYS_T=Solaris
			;;
		*)
			SYS_T=Solaris
			;;
		esac
	elif [ -x /bin/i386 -o -x /sbin/i386 ] 
	then
		case "$REL_T" in
		3.2)
			# SCO UNIX/Open Desktop/Open Server
			if (uname -X | fgrep "Release = 3.2") >/dev/null 2>&1
			then
				REL_T=`uname -X 2>/dev/null | \
					fgrep "Version = " | \
					sed -e 's/Version = //' -e 's/\..*$//'`
				if [ -z "$REL_T" ]
				then
					REL_T=`uname -v | sed 's/\..*$//'`
				fi
				SYS_T=SCO_3.2v${REL_T}
			fi
			;;
		4.*)
			# UNIX SVR4.x
			SYS_T=${SYS_T}_${REL_T}
			;;
		5)
			# UNIX SVR5
			SYS_T=${SYS_T}_${REL_T}
			;;
		5.*)
			# UNIX SVR5.x
			SYS_T=${SYS_T}_${REL_T}
			;;
		*)
			;;
		esac
	fi

	PATH=$XMCD_LIBDIR/bin-${SYS_T}-${MACH_T}:$XMCD_LIBDIR/scripts:$PATH
	export PATH

	# Set shared lib search path to include the xmcd arch-dependent
	# lib directory.
	# Note that with setuid xmcd/cda executables this has no effect
	# on some platforms.
	LD_LIBRARY_PATH=$XMCD_LIBDIR/lib-${SYS_T}-${MACH_T}:$LD_LIBRARY_PATH
	export LD_LIBRARY_PATH
	SHLIB_PATH=$XMCD_LIBDIR/lib-${SYS_T}-${MACH_T}:$SHLIB_PATH
	export SHLIB_PATH

	# Set arch-dependent executable
	EXEPATH=$XMCD_LIBDIR/bin-${SYS_T}-${MACH_T}/$PROG
}


#
# Main starts here
#

PROG=`(basename $0) 2>/dev/null`
SYS=`(uname -s) 2>/dev/null | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`
if [ "$SYS" = IRIX ]
then
	MACH=mips
else
	MACH=`(uname -m) 2>/dev/null | \
		sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`
fi
REL=`(uname -r) 2>/dev/null | sed -e 's/\//_/g' -e 's/-/_/g' -e 's/[ 	]/_/g'`

o_path=$PATH
o_ldlibpath=$LD_LIBRARY_PATH
o_shlibpath=$SHLIB_PATH

proc_env
if [ -x "$EXEPATH" ]
then
	exec "$EXEPATH" ${1+"$@"}
else
	# The user may have defined a bad XMCD_LIBDIR environment variable.
	# Fall back to default and re-try.
	unset XMCD_LIBDIR
	PATH=$o_path; export PATH
	LD_LIBRARY_PATH=$o_ldlibpath; export LD_LIBRARY_PATH
	SHLIB_PATH=$o_shlibpath; export SHLIB_PATH

	proc_env
	if [ -x "$EXEPATH" ]
	then
		exec "$EXEPATH" ${1+"$@"}
	else
		echo "Cannot invoke $EXEPATH" 1>&2
		echo "Check your installation of ${PROG}." 1>&2
		exit 1
	fi
fi

