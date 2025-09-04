#
# @(#)VolTpr.rbx	6.16 03/12/12
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
Volume Control Taper options

There are three toggle buttons which controls the
taper characteristics of the Xmcd main window
volume control slider.  This is intended to
complement the characteristics of the software
volume control in the CD drive to achieve the
smoothest range of control possible.

Only one of these can be selected at a time:

- linear

   A linear taper is selected.

- square

   A square taper is selected.  This causes more
   resolution in low control settings, and more
   dramatic changes at high settings.

- inverse square

   An inverse square taper is selected.  This
   causes dramatic changes in the low control
   settings, and more resolution in the high
   settings.

The default start-up setting of these options is
configured via the volumeControlTaper parameter
in the device-specific configuration files.
