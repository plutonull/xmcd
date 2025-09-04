/*
 *   xmcd - Motif(R) CD Audio Player/Ripper
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
#ifndef __COMMAND_H__
#define __COMMAND_H__

#ifndef lint
static char *_command_h_ident_ = "@(#)command.h	6.17 04/03/05";
#endif


/* Property names */
#define CMD_IDENT		"XMCD_CMD_IDENT"
#define CMD_ATT			"XMCD_CMD_ATT"
#define CMD_REQ			"XMCD_CMD_REQ"
#define CMD_ACK			"XMCD_CMD_ACK"

/* Max property data lengths */
#define MAX_IDENT_LEN		(FILE_PATH_SZ * 2)
#define MAX_ATT_LEN		(STR_BUF_SZ * 2)
#define MAX_REQ_LEN		(STR_BUF_SZ * 2)
#define MAX_ACK_LEN		1		/* 32-bit multiple */

/* Status codes */
#define CMD_STAT_SUCCESS	0	/* Request completed successfully */
#ifdef __VMS
/* VMS program exit codes must map to certain system-defined values
 * otherwise we would get inappropriate error messages.
 */
#define CMD_STAT_NOCLIENT	2312	/* No remote client window found */
#define CMD_STAT_ATTFAIL	231616	/* Cannot attach to remote window */
#define CMD_STAT_PROCESSING	4976	/* Client is processing request */
#define CMD_STAT_DESTROYED	1704	/* Remote window was destroyed */
#define CMD_STAT_ACKFAIL	2642	/* Request acknowledgement error */
#define CMD_STAT_UNSUPP		9224	/* Unsupported request */
#define CMD_STAT_BADARG		4040	/* Invalid argument */
#define CMD_STAT_INVAL		229976	/* Invalid operation */
#define CMD_STAT_DISABLED	848	/* Remote control is disabled */
#define CMD_STAT_NOCMD		229992	/* No command specified */
#else
/* UNIX */
#define CMD_STAT_NOCLIENT	61	/* No remote client window found */
#define CMD_STAT_ATTFAIL	62	/* Cannot attach to remote window */
#define CMD_STAT_PROCESSING	63	/* Client is processing request */
#define CMD_STAT_DESTROYED	64	/* Remote window was destroyed */
#define CMD_STAT_ACKFAIL	65	/* Request acknowledgement error */
#define CMD_STAT_UNSUPP		66	/* Unsupported request */
#define CMD_STAT_BADARG		67	/* Invalid argument */
#define CMD_STAT_INVAL		68	/* Invalid operation */
#define CMD_STAT_DISABLED	69	/* Remote control is disabled */
#define CMD_STAT_NOCMD		70	/* No command specified */
#endif

#ifdef __VMS
#define RL_RET(code) 		return
#else
#define RL_RET(code)		_exit((code))
#endif

/* Public functions */
extern void	cmd_init(curstat_t *, Display *, bool_t);
extern void	cmd_startup(byte_t *);
extern int	cmd_sendrmt(Display *, char *);
extern void	cmd_usage(void);

/* Callback functions */
extern void	cmd_propchg(Widget, XtPointer, XEvent *, Boolean *);

#endif	/* __COMMAND_H__ */
