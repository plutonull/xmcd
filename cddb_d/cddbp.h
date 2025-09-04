/*
 *   cddbp - CDDB1 Protocol
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
#ifndef __CDDBP_H__
#define __CDDBP_H__

#ifndef lint
static char *_cddbp_h_ident_ = "@(#)cddbp.h	1.6 03/12/12";
#endif


/*
 * Server response code (three-digit code)
 *
 * First digit (general-status):
 * 1xx	Informative message
 * 2xx	Command OK
 * 3xx	Command OK so far, continue
 * 4xx	Command OK, but cannot be performed for some specified reasons
 * 5xx	Command unimplemented, incorrect, or program error
 * 6xx	Client-specific internal status
 *
 * Second digit (sub-status):
 * x0x     Ready for further commands
 * x1x     More server-to-client output follows (until terminating marker)
 * x2x     More client-to-server input follows (until terminating marker)
 * x3x     Connection will close
 *
 * Third digit:
 * xx[0-9] Command-specific code
 */


/*
 * Definitions for the first digit
 */
#define STAT_GEN_INFO		'1'	/* Informative message */
#define STAT_GEN_OK		'2'	/* Command OK */
#define STAT_GEN_OKCONT		'3'	/* Command OK so far, continue */
#define STAT_GEN_OKFAIL		'4'	/* Command OK, but cannot be performed
					 * for some specified reasons
					 */
#define STAT_GEN_ERROR		'5'	/* Command unimplemented, incorrect,
					 * or program error
					 */
#define STAT_GEN_CLIENT		'6'	/* Client internal status codes */

/*
 * Definitions for the second digit
 */
#define STAT_SUB_READY		'0'	/* Ready for further commands */
#define STAT_SUB_OUTPUT		'1'	/* More server-to-client output
					 * follows (until terminating marker)
					 */
#define STAT_SUB_INPUT		'2'	/* More client-to-server input follows
					 * (until terminating marker)
					 */
#define STAT_SUB_CLOSE		'3'	/* Connection will close */

/*
 * Definitions for the third digit
 */

/* Sign on banner */
#define STAT_BANR_RDWR		'0'	/* OK, read/write allowed */
#define STAT_BANR_RDONLY	'1'	/* OK, read only */
#define STAT_BANR_PERM		'2'	/* No connection: permission denied */
#define STAT_BANR_USERS		'3'	/* No connection: too many users */
#define STAT_BANR_LOAD		'4'	/* No connection: load too high */

/* Set proto level */
#define STAT_PROTO_OK		'0'	/* OK, display curr and max levels */
#define STAT_PROTO_SET		'1'	/* OK, set to specified level */
#define STAT_PROTO_ERR		'1'	/* Illegal level specified */
#define STAT_PROTO_NOCHG	'2'	/* Already at specified level */

/* Hello handshake */
#define STAT_HELO_OK		'0'	/* Handshake successful */
#define STAT_HELO_FAIL		'1'	/* Handshake failed */

/* Query */
#define STAT_QURY_EXACT		'0'	/* Found exact match */
#define STAT_QURY_INEXACT	'1'	/* Found inexact match */
#define STAT_QURY_NONE		'2'	/* No match found */
#define STAT_QURY_AUTHFAIL	'3'	/* Proxy authorization failure */

/* Read */
#define STAT_READ_OK		'0'	/* OK, CDDB data follows (until
					 * terminating marker)
					 */
#define STAT_READ_FAIL		'1'	/* Specified CDDB entry not found */

/* Write: pre-data */
#define STAT_WRIT_OK		'0'	/* OK, CDDB data follows (until
					 * terminating marker)
					 */
#define STAT_WRIT_PERM		'1'	/* Permission denied */
#define STAT_WRIT_FSFULL	'2'	/* Server file system full */

/* Write: post-data */
#define STAT_WRIT_ACCEPT	'0'	/* CDDB entry accepted */
#define STAT_WRIT_REJECT	'1'	/* CDDB entry rejected */

/* Help */
#define STAT_HELP_OK		'0'	/* OK, help info follows (until
					 * terminating marker)
					 */
#define STAT_HELP_FAIL		'1'	/* No help information available */

/* Quit */
#define STAT_QUIT_OK		'0'	/* OK, closing connection */


#define CDDBP_CMDLEN		2048	/* Command buffer length */

#endif	/* __CDDBP_H__ */

