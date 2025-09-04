#
# @(#)Sched.rbx	7.7 03/12/12
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
CDDA thread scheduling

In CDDA playback modes, Xmcd reads from the CD
and writes to the audio device, file and pipe in
separate parallel threads.  The read thread fills
an internal buffer with audio data while the write
thread drains that buffer.  The buffer helps to
avoid a data underrun condition where a momentary
increase in system load might cause a glitch in
the audio.

On systems with unpredictable loads or those with
marginal performance, you may experience smoother
CDDA playback if the read and/or the write threads
are run with increased scheduling priority.

This section allows you to select the scheduling
priorities of the read and write threads.

"Normal priority" means that the thread is given
the same UNIX process scheduling order as other
normal processes or threads on the system.  The
kernel employs a "fair" time-shared scheduling
algorithm for these threads.

The meaning of "High priority" is somewhat platform
dependent.  On most systems this causes the kernel
to increase the thread's opportunity to run at the
expense of other proceses.  On some platforms this
may be a pseudo real-time or fixed-priority scheduling
algorithm, which guarantees that the thread will run
even under high system loads.

Note that under some circumstances, running both the
read and write threads in high priority mode may
"starve" other processes on the system, making them
sluggish or unresponsive.  Since xmcd's graphical
windowing interface is itself a separate thread with
normal priority, it may become slow to respond too.

If you change the settings of the read or write
scheduling priorities, it will not affect a CDDA
playback operation in progress.  It will take effect
on the next CDDA play session.

If xmcd is not run with root privilege it will not
be able to increase the scheduling priority of its
threads.

These settings have no effect when running in the
Standard (non-CDDA) playback mode.

