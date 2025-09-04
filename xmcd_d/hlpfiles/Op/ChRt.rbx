#
# @(#)ChRt.rbx	6.17 04/02/02
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
Channel Routing options

There are five toggle buttons which controls
the audio output channel routing characteristics.

In "Standard playback" mode, some or all of these
selections may not have any effect on CD drives
that do not implement the necessary channel routing
support.

This control affects all CDDA mode outputs (i.e.,
"CDDA playback", "CDDA save to file" and "CDDA
pipe to program").

Only one of these can be selected at a time:

- normal

   Normal stereo: The left channel audio is
   routed to the left output, and the right
   channel audio is routed to the right output.

- reverse

   Reversed stereo: The left channel audio is
   routed to the right output, and the right
   channel audio is routed to the left output.

- mono L

   Monophonic: The left channel audio is routed
   to both outputs.

- mono R

   Monophonic: The right channel audio is routed
   to both outputs.

- mono L+R

   Monophonic: The left and right channels are
   routed to both outputs.

The default start-up setting of these options is
configured via the channelRoute parameter in the
device-specific configuration files.
