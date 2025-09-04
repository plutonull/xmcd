#!/bin/sh
#
# @(#)makerel.sh	6.28 03/12/12
#
# Make software binary release
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

# The file containing a list of binary release files
BINLIST=misc_d/BINLIST
TMPDIR=/tmp/_makerel_d
TOPDIR=xmcdbin
ZFILE=xmcdbin.tar.gz
UUEFILE=xmcdbin.uue
COMPRESS=gzip
UNCOMPRESS=gunzip
ENCODE=uuencode
DECODE=uudecode
TAR=/usr/local/bin/scotar

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

# Select tar program
if [ ! -x $TAR ]
then
	TAR=tar
fi

CURDIR=`pwd`
if [ `basename "$CURDIR"` = misc_d ]
then
	cd ..
elif [ ! -f install.sh ]
then
	$ECHO "You must run the makerel.sh script while in the xmcd"
	$ECHO "source code distribution top-level directory or in the"
	$ECHO "misc_d subdirectory."
	exit 1
fi

if [ ! -r $BINLIST ]
then
	$ECHO "Error: Cannot open $BINLIST"
	exit 2
fi

VERS=`grep "VERSION=" $BINLIST | sed 's/^.*VERSION=//'`

$ECHO "Creating xmcd/cda version $VERS binary release...\n"

trap "$ECHO 'Interrupted!'; rm -rf $TMPDIR $ZFILE $UUEFILE; exit 1" 1 2 3 5 15

# Make temp directory and copy release files to it
rm -rf $TMPDIR
mkdir -p $TMPDIR
for i in `awk '!/^#/ { print $1 }' $BINLIST`
do
	$ECHO "\t$i"
	DEST=`dirname $TMPDIR/$TOPDIR/$i`
	mkdir -p $DEST >/dev/null 2>&1
	cp $i $DEST
done

# Copy CDDB2 shared library into binary bundle
case `uname -s` in
HP-UX)
	shlib=libcddb.sl
	;;
SunOS)
	case `uname -r` in
	4*)
		shlib=libcddb.so.1.0
		;;
	5*)
		shlib=libcddb.so.1
		;;
	*)
		shlib=libcddb.so.1
		;;
	esac
	;;
*)
	shlib=libcddb.so.1
	;;
esac

for d in /usr/lib /usr/local/lib /lib /usr/ccs/lib
do
	if [ -f $d/$shlib ]
	then
		DEST=$TMPDIR/$TOPDIR/cddb_d
		mkdir -p $DEST >/dev/null 2>&1
		cp $d/$shlib $DEST
		$ECHO "\tcddb_d/$shlib"
		break
	fi
done

if [ ! -d $TMPDIR/$TOPDIR/cddb_d ]
then
	$ECHO "\nNOTICE: CDDB shared library not included."
fi

# Strip the binary symbol tables
strip $TMPDIR/$TOPDIR/xmcd_d/xmcd >/dev/null 2>&1
strip $TMPDIR/$TOPDIR/cda_d/cda >/dev/null 2>&1
strip $TMPDIR/$TOPDIR/util_d/gobrowser >/dev/null 2>&1

# Remove comment section of binaries if possible
(mcs -da "@(#)xmcd $VERS (C) Ti Kan 1993-2004" \
	$TMPDIR/$TOPDIR/xmcd_d/xmcd) >/dev/null 2>&1
(mcs -da "@(#)cda $VERS (C) Ti Kan 1993-2004" \
	$TMPDIR/$TOPDIR/cda_d/cda) >/dev/null 2>&1
(mcs -da "@(#)gobrowser $VERS" \
	$TMPDIR/$TOPDIR/util_d/gobrowser) >/dev/null 2>&1

$ECHO "\nFixing permissions..."
(cd $TMPDIR; find * -type d -print | xargs chmod 755)
(cd $TMPDIR; find * -type f -print | xargs chmod 444)

$ECHO "\nCreating \"$COMPRESS\"ed tar archive..."
# Create tar archive and compress it
(cd $TMPDIR; $TAR cf - *) | $COMPRESS >$ZFILE

$ECHO "\n\"$ENCODE\"ing..."

$ECHO '
Instructions to unpack xmcd v_VERS_ binary and other info
------------------------------------------------------

At the end of this message is a "_COMPRESS_"ed and "_ENCODE_"ed
tar file containing the xmcd and cda executables as well as
their supporting files.  To extract, save this message in a
file "_UUEFILE_", login as root and do the following:

	_DECODE_ _UUEFILE_
	_UNCOMPRESS_ < _ZFILE_ | tar xvf -
	cd _TOPDIR_
	sh ./install.sh

Be sure to run the "install.sh" script to install and configure
the software.  Otherwise, it will not work properly.

For further information, see the XMCDLIB/docs/README file after
installation.

' | sed	-e "s/_VERS_/$VERS/g" \
	-e "s/_UUEFILE_/$UUEFILE/g" \
	-e "s/_ZFILE_/$ZFILE/g" \
	-e "s/_TOPDIR_/$TOPDIR/g" \
	-e "s/_COMPRESS_/$COMPRESS/g" \
	-e "s/_UNCOMPRESS_/$UNCOMPRESS/g" \
	-e "s/_ENCODE_/$ENCODE/g" \
	-e "s/_DECODE_/$DECODE/g" >$UUEFILE

# Uuencode
$ENCODE $ZFILE $ZFILE >>$UUEFILE
$ECHO "\n\n\n" >>$UUEFILE

rm -rf $TMPDIR

$ECHO ""
ls -l $ZFILE $UUEFILE

exit 0

