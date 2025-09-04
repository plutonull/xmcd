#
# @(#)Lookup.rbx	7.6 03/12/12
#
#   xmcd - Motif(R) CD Audio Player/Ripper
#
#   Copyright (C) 1993-2004  Ti Kan
#   E-mail: ti@amb.org
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
Lookup Priority selection

Only one of these toggle buttons can be selected
at a time:

- CDDB

   Xmcd will try the Gracenote CDDB service first,
   and use CD-Text as a secondary source when looking
   up CD information.

- CD-Text

   Xmcd will try reading CD-Text first, and use
   Gracenote CDDB as a secondary source when
   looking up CD information.

CD-Text is data recorded on the CD itself, containing
album/track artist, title and other information.
Note that not all CD drives support reading CD-Text
data, and only some CDs contain CD-Text information.

The CDDB and CD-Text toggle buttons will be selectable
only if the cdinfoPath parameter contains both types of
lookup source, and the cdTextDisable parameter is not
set to True.

