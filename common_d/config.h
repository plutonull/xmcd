/*
 *   config.h - Platform or OS specific feature configuration
 *
 *   xmcd  - Motif(R) CD Audio Player/Ripper
 *   cda   - Command-line CD Audio Player/Ripper
 *
 *   Copyright (C) 1993-2004  Ti Kan
 *   E-mail: xmcd@amb.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifndef lint
static char *_config_h_ident_ = "@(#)config.h	7.14 04/04/05";
#endif

/*
 * The following defines are configured here:
 *
 * BSDCOMPAT
 *	The system is a BSD-derived UNIX flavor.
 *
 * USE_TERMIOS
 * USE_TERMIO
 * USE_SGTTY
 *	One of these should be defined for tty mode control.
 *
 * USE_SIGACTION
 * USE_SIGSET
 * USE_SIGNAL
 *	One of these should be defined for signal disposition control.
 *
 * USE_SELECT
 * USE_POLL
 * USE_NAP
 * USE_USLEEP
 * USE_NANOSLEEP
 * USE_PTHREAD_DELAY_NP
 *	One of the above should be defined to determine which method
 *	is used for high-resolution timing delay.
 *
 * USE_SIGPROCMASK
 * USE_SIGBLOCK
 *	One of these should be defined to determine which method is
 *	used to block signals.
 *
 * USE_SETEUID
 * USE_SETREUID
 * USE_SETRESUID
 *	One of these should be defined to determine which method is
 *	used to set the effective uid.
 *
 * USE_SETEGID
 * USE_SETREGID
 * USE_SETRESGID
 *	One of these should be defined to determine which method is
 *	used to set the effective gid.
 *
 * USE_RENAME
 *	Specify that the rename(2) system call shall be used to change the
 *	name of a file.  Otherwise the link(2) and unlink(2) systems calls
 *	will be used instead.
 *
 * USE_GETWD
 *	Use the getwd(3) function to determine the current working directory
 *	instead of getcwd(3).  This is only used if BSDCOMPAT is also
 *	defined.
 *
 * HAS_LONG_LONG
 *	The compiler supports the "long long" type for a 64-bit integer.
 *	For gcc this capability is auto-detected and this definition is
 *	not necessary.
 *
 * HAS_ITIMER
 *	The system supports the setitimer(3) and getitimer(3) high
 *	resolution timer facility.
 *
 * HAS_ICONV
 *	The system supports the open_iconv(3), close_iconv(3) and iconv(3)
 *	functions (for character set conversion).
 *
 * HAS_PTHREADS
 *	The system supports POSIX 1003.1c threads (i.e., has <pthread.h>
 *	and a functional preemptive pthread library).  This is normally
 *	automatically detected, but you may need to force this on some
 *	platforms.
 *
 * NO_PTHREADS
 *	Force POSIX threads support to be disabled.  If both NO_PTHREADS
 *	and HAS_PTHREADS are defined, the latter takes precedence.
 *
 * NEED_PTHREAD_INIT
 *      The system requires a call to pthread_init() upon entry into
 *      main() in a multi-threaded program.
 *
 * USE_SIGTHREADMASK
 *	The system supports the sigthreadmask(3) function instead of
 *	pthread_sigmask(3) for blocking signals in a thread.
 *
 * USE_SYS_SOUNDCARD_H
 *	For OSS sound driver support, use the <sys/soundcard.h> header file
 *	instead of the one supplied with the xmcd source code package.
 *
 * NO_FCHMOD
 *	The system does not support the fchmod(2) system call.
 *
 * NO_FSYNC
 *	The system does not support the fsync(2) system call.
 *
 * AIX_IDE (AIX only)
 *	Enable the AIX IDE ioctl method.  This option must be used for
 *	AIX 4.x users that are using IDE CD-ROM drives who wants to use
 *	any of the CDDA modes.
 *
 * SOL2_RSENSE (Solaris only)
 *	Whether to include code that makes use of the auto request-sense
 *	feature in Solaris 2.2 or later.
 *
 * NETBSD_OLDIOC (NetBSD only)
 *	Whether to enable the old-NetBSD ioctl method.  This option must be
 *	used for users of older NetBSD systems (prior to v1.2G or so).
 *
 * OEM_CDROM
 *	Define this if you have one of those strange OEM SCSI CD-ROM
 *	drives that identify themselves as a hard disk (see the FAQ file).
 *
 * NOVISUAL
 *	Build cda without the visual (curses) mode support.
 *
 */

/* AIX */
#if defined(_AIX)
#define USE_TERMIO
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SIGTHREADMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#define HAS_ICONV
#define NEED_PTHREAD_INIT
#define AIX_IDE
#endif

/* A/UX */
#if defined(macII)
#define USE_TERMIO
#ifndef USG
#define USG
#endif
#endif

/* BSD/OS */
#if defined(__bsdi__)
#define BSDCOMPAT
#define USE_TERMIOS
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#if !defined(HAS_NCURSES_H)
#define NOVISUAL
#endif
#endif

/* FreeBSD */
#if defined(__FreeBSD__)
#define BSDCOMPAT
#define USE_TERMIOS
#define USE_SIGACTION
#define USE_SELECT
#define USE_SETEUID
#define USE_SETEGID
#define USE_SIGPROCMASK
#endif

/* HP-UX */
#if defined(__hpux)
#define USE_TERMIO
#define USE_SIGACTION
#define USE_SELECT
#define USE_SETRESUID
#define USE_SETRESGID
#define USE_SIGPROCMASK
#define HAS_ITIMER
#if defined(_INCLUDE_POSIX1C_SOURCE)
#define HAS_PTHREADS
#endif
#endif

/* IRIX */
#if defined(sgi)
#define USE_TERMIO
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#if defined(_IRIX6)
#define HAS_ICONV
#define HAS_LONG_LONG
#endif
#endif

/* Linux */
#if defined(_LINUX)
#define USE_TERMIO
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#if defined(HAS_ICONV_H)
#define HAS_ICONV
#endif
#endif

/* NetBSD */
#if defined(__NetBSD__)
#define BSDCOMPAT
#define USE_TERMIOS
#define USE_SIGACTION
#define USE_SELECT
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#define USE_SIGPROCMASK
#define NOVISUAL
#endif

/* OpenBSD */
#if defined(__OpenBSD__)
#define BSDCOMPAT
#define USE_SIGACTION
#define USE_TERMIOS
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#endif

/* OpenVMS */
#if defined(__VMS)
#if defined(HAS_PTHREADS) && !defined(NO_PTHREADS)
#define USE_PTHREAD_DELAY_NP
#else
#define USE_USLEEP
#endif
#endif

/* OSF1/Digital UNIX/Tru64 UNIX */
#if defined(_OSF1)
#define BSDCOMPAT
#define USE_TERMIOS
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#define HAS_ICONV
#endif

/* SCO UNIX/Open Desktop/Open Server */
#if defined(_SCO_SVR3)
#define USE_TERMIO
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#if defined(_SCO5)
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#define HAS_ICONV
#else
#define NO_FCHMOD
#define NO_FSYNC
#endif
#endif

/* Sony NEWS */
#if defined(sony_news)
#define BSDCOMPAT
#define USE_SGTTY
#define NOVISUAL
#endif

/* Solaris */
#if defined(_SOLARIS)
#define USE_TERMIO
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#define HAS_ICONV
#define SOL2_RSENSE
#endif

/* SunOS 4.x */
#if defined(_SUN4)
#define BSDCOMPAT
#define USE_TERMIOS
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#endif

/* Ultrix */
#if defined(_ULTRIX)
#define BSDCOMPAT
#define USE_TERMIO
#define USE_SELECT
#define USE_SETEUID
#define USE_SETEGID
#define USE_SIGBLOCK
#endif

/* SVR4.2 and UnixWare */
#if defined(_UNIXWARE)
#define USE_TERMIO
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#if defined(_UNIXWARE2)
#define HAS_ICONV
#endif
#endif

/* Generic SVR4 */
#if (defined(SVR4) || defined(__svr4__)) && \
    !defined(_SOLARIS) && !defined(sgi) && !defined(_UNIXWARE)
#define USE_TERMIO
#define USE_SIGACTION
#define USE_SELECT
#define USE_SIGPROCMASK
#define USE_SETEUID
#define USE_SETEGID
#define HAS_ITIMER
#endif

#endif	/* __CONFIG_H__ */


