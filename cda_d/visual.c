/*
 *   cda - Command-line CD Audio Player/Ripper
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

/*
 *   Visual mode support
 *
 *   Contributing author: Philip Le Riche
 *   E-Mail: pleriche@uk03.bull.co.uk
 */

#ifndef lint
static char *_visual_c_ident_ = "@(#)visual.c	6.125 04/04/05";
#endif

#include "common_d/appenv.h"

#ifndef NOVISUAL

#include "common_d/util.h"
#include "common_d/version.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"

/* TRUE and FALSE is redefined in curses.h. */
#undef TRUE
#undef FALSE

/* curses.h redefines SYSV - handle with care */
#ifdef SYSV
#undef SYSV
#include <curses.h>
#define SYSV
#else
#if defined(_ULTRIX)
#include <cursesX.h>
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__bsdi__)
#include <ncurses.h>
#else
#include <curses.h>
#endif	/* __FreeBSD__ __NetBSD__ __bsdi__ */
#endif	/* ultrix */
#endif	/* SYSV */

#ifdef _SCO_SVR3
#include <sys/stream.h>
#include <sys/ptem.h>
#endif

#ifndef __QNX__
#include <term.h>
#endif

#include "cda_d/cda.h"
#include "cda_d/visual.h"

extern appdata_t	app_data;
extern cdinfo_incore_t	*dbp;
extern char		*emsgp,
			**cda_devlist,
			errmsg[],
			spipe[],
			rpipe[];
extern int		cda_sfd[],
			cda_rfd[],
			exit_status;
extern pid_t		daemon_pid;

STATIC int		scr_lines = 0,		/* number of screen lines */
			scr_cols = 0,		/* number of screen cols */
			scroll_line,		/* 1st line of info window */
			scroll_length,		/* # of scrollable lines */
			route,			/* Stereo, Mono ... */
			old_route = 1,
			track = -2,		/* Current track no. */
			savch;			/* Saved ch for cda_ungetch */
STATIC unsigned int	jc_alarm = 0;		/* Saved alarm for job ctl */
STATIC bool_t		isvisual = FALSE,	/* Currently in visual mode */
			stat_on = FALSE,	/* Visual: cda is "on" */
			ostat_on = TRUE,	/* Previous value */
			refresh_fkeys = TRUE,	/* Refresh funct key labels */
			help = FALSE,		/* Display help in info_win? */
			old_help = TRUE,	/* Previous value */
			refresh_sts = FALSE,	/* Refresh status line */
			win_resize = FALSE,	/* Window resize */
			echflag = FALSE,	/* Own echo flag */
			savch_echo = FALSE,	/* Control echo for savch */
			loaddb = TRUE;		/* Load CD information */
STATIC word32_t		oastat0 = (word32_t)-1,	/* Previous status value */
			oastat1 = (word32_t)-1;
STATIC WINDOW		*info_win,		/* Scrolling window for info */
			*status_win;		/* Window for status */


/* Fatal error macro for visual mode */
#define CDA_V_FATAL(msg, code)	{	\
	emsgp = (msg);			\
	exit_status = (code);		\
	cda_quit(curstat_addr());	\
}

/* Keyboard input to command function mapping table */
typedef struct {
	int	key;
	void	(*keyfunc)(int, word32_t[]);
} keytbl_t;


/***********************
 *  internal routines  *
 ***********************/


#if defined(SIGTSTP) && defined(SIGCONT)
/*
 * ontstp
 *	Handler for job control stop signal
 *
 * Args:
 *	signo - The signal number
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
ontstp(int signo)
{
	/* Cancel alarms and save state */
	jc_alarm = alarm(0);

	/* Put screen in sane state */
	(void) keypad(stdscr, FALSE);
	(void) putp(cursor_normal);
	(void) endwin();

	/* Now stop the process */
	(void) util_signal(SIGTSTP, SIG_DFL);
	(void) kill((pid_t) getpid(), SIGTSTP);
}


/*
 * oncont
 *	Handler for job control continue signal
 *
 * Args:
 *	signo - The signal number
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
oncont(int signo)
{
	/* Use the window resize routine to restore visual attributes */
	win_resize = TRUE;

	if (jc_alarm != 0) {
		/* Get back into the loop */
		(void) kill(getpid(), SIGALRM);
	}
}
#endif	/* SIGTSTP SIGCONT */


#ifdef SIGWINCH
/*
 * onwinch
 *	Handler for window size change signal
 *
 * Args:
 *	signo - The signal number
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
onwinch(int signo)
{
	win_resize = TRUE;
}
#endif


/*
 * cda_makewins
 *	Create the main window areas on the display.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.  Program exits on error.
 */
STATIC void
cda_makewins(void)
{
	WINDOW		*new_info_win,
			*new_status_win;
#ifdef TIOCGWINSZ
	int		fd;
	struct winsize	ws;
#endif

	if (!isvisual) {
		/* Startup */
		if (initscr() == NULL)
		    CDA_FATAL("cda visual Error: Cannot initialize curses");

		isvisual = TRUE;
	}
	else {
		/* Re-initialize due to resize */
		(void) endwin();
#ifdef __hpux
		if (initscr() == NULL)
#else
		if (newterm(NULL, stdout, stdin) == NULL)
#endif
		{
		    CDA_FATAL("cda visual Error: Cannot re-initialize curses");
		}
	}

	scr_cols = scr_lines = 0;

#ifdef TIOCGWINSZ
	fd = fileno(stdout);
	if (isatty(fd) && ioctl(fd, TIOCGWINSZ, &ws) >= 0) {
		scr_lines = (int) ws.ws_row;
		scr_cols = (int) ws.ws_col;
	}
#endif	/* TIOCGWINSZ */

	if (scr_cols == 0 || scr_lines == 0) {
#ifdef getmaxyx
		getmaxyx(stdscr, scr_lines, scr_cols);
#else
		scr_lines = LINES;
		scr_cols = COLS;
#endif
	}

	if (scr_lines < 9 || scr_cols < 40) {
		CDA_V_FATAL(
			"cda visual Error: "
			"The terminal screen size is too small.\n"
			"It must be at least 80 columns x 9 lines\n",
			6
		);
	}

	LINES = scr_lines;
	COLS = scr_cols;

#if defined(NCURSES_VERSION_MAJOR) && (NCURSES_VERSION_MAJOR >= 4)
	(void) resizeterm(LINES, COLS);
#endif

#ifdef SIGWINCH
	/* Handle terminal screen size change */
	(void) util_signal(SIGWINCH, onwinch);
#endif

#if defined(SIGTSTP) && defined(SIGCONT)
	/* Handle job control */
	(void) util_signal(SIGTSTP, ontstp);
	(void) util_signal(SIGCONT, oncont);
#endif

	(void) noecho();
	(void) cbreak();
	(void) clear();
	(void) putp(cursor_invisible);
	(void) scrollok(stdscr, FALSE);

	new_info_win = newpad(132, scr_cols);
	if (new_info_win == NULL) {
		CDA_V_FATAL(
			"cda visual Error: Cannot create information window.",
			6
		);
	}

	new_status_win = newwin(7, scr_cols, scr_lines-8, 0);
	if (new_status_win == NULL) {
		CDA_V_FATAL(
			"cda visual Error: Cannot create status window.",
			6
		);
	}

	if (info_win != NULL)
		(void) delwin(info_win);
	if (status_win != NULL)
		(void) delwin(status_win);

	info_win = new_info_win;
	status_win = new_status_win;

	(void) keypad(info_win, TRUE);
	(void) keypad(status_win, TRUE);

	(void) wclear(info_win);
	(void) wclear(status_win);
	(void) wmove(status_win, 3, 0);
	(void) waddstr(status_win, STATUS_LINE0);
	(void) wmove(status_win, 4, 0);
	(void) waddstr(status_win, STATUS_LINE1);
	(void) wmove(status_win, 5, 0);
	(void) waddstr(status_win, STATUS_LINE2);
	(void) box(status_win, 0, 0);

	(void) refresh();
}


/*
 * cda_wgetch
 *	Own version of curses wgetch. This interworks with cda_ungetch.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Input character or function key token.
 */
STATIC int
cda_wgetch(WINDOW *win)
{
	int	ch;

	if (savch) {
		/* Echo character now if echo on but not yet echoed */
		if (!savch_echo && echflag &&
		    isprint(savch) && !iscntrl(savch)) {
			(void) waddch(win, savch);
			(void) wrefresh(win);
		}
		ch = savch;
		savch = 0;
		return (ch);
	}

	ch = wgetch(win);

	/* Do our own echoing because switching it on and off doesn't
	 * seem to work on some platforms.
	 */
	if (echflag && isprint(ch) && !iscntrl(ch)) {
		(void) waddch(win, ch);
		(void) wrefresh(win);
	}

	return (ch);
}


/*
 * cda_ungetch
 *	Own version of ungetch, because some systems don't have it.
 *	Also, we need to remember the echo status of the ungotten
 *	character.
 *
 * Args:
 *	Char or function key token to return.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_ungetch(int ch)
{
	savch = ch;
	/* Remember whether the character has been echoed */
	savch_echo = echflag;
}


/*
 * cda_wgetstr
 *	Own version of wgetstr, using cda_wgetch and cda_ungetch.
 *
 * Args:
 *	Buffer to be filled with input string.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_wgetstr(WINDOW *win, char *inbuf, int max)
{
	int	ch,
		n,
		x,
		y;
	char	*p;
	bool_t	eos = FALSE;

	p = inbuf;
	n = 0;

	while (!eos) {
		if (n > max) {
			beep();
			break;
		}

		ch = cda_wgetch(win);

		switch (ch) {
		case KEY_BACKSPACE:
		case KEY_LEFT:
		case '\010':
			if (n > 0) {
				p--;
				n--;
				/* Echo the effect of backspace */
				getyx(win, y, x);
				(void) wmove(win, y, x-1);
				(void) waddch(win, ' ');
				(void) wmove(win, y, x-1);
			}
			break;

		case KEY_UP:
		case '\036':	/* ctrl-^ */
		case KEY_DOWN:
		case '\026':	/* ctrl-v */
		case KEY_PPAGE:
		case '\002':	/* ctrl-b */
		case '\025':	/* ctrl-u */
		case KEY_NPAGE:
		case '\006':	/* ctrl-f */
		case '\004':	/* ctrl-d */
			if (n > 0) {
				n--;
				getyx(win, y, x);
				(void) wmove(win, y, x-1);
				(void) waddch(win, ' ');
				(void) wmove(win, y, x-1);
			}

			switch (ch) {
			case KEY_UP:
			case '\036':
				scroll_line--;
				break;
			case KEY_DOWN:
			case '\026':
				scroll_line++;
				break;
			case KEY_PPAGE:
			case '\002':
				scroll_line -= (scr_lines - 9);
				break;
			case '\025':
				scroll_line -= ((scr_lines - 9) / 2);
				break;
			case KEY_NPAGE:
			case '\006':
				scroll_line += (scr_lines - 9);
				break;
			case '\004':
				scroll_line += ((scr_lines - 9) / 2);
				break;
			}

			if (scroll_line < 0)
				scroll_line = 0;
			else if (scroll_line > (scroll_length - 1))
				scroll_line = scroll_length - 1;

			(void) prefresh(info_win, scroll_line, 0, 0, 0,
					scr_lines - 9, scr_cols - 1);
			break;

		case '\n':
		case '\r':
			/* End-of-string */
			eos = TRUE;
			break;

		default:
			if (!isprint(ch) || iscntrl(ch))
				beep();
			else {
				*p++ = (char) ch;
				n++;
			}

			break;
		}

		(void) wrefresh(win);
	}

	*p = '\0';
}


/*
 * cda_v_echo
 *	Own versions of curses echo function.
 *
 * Args:
 *	doecho - Whether to enable echo
 *
 * Return:
 *	Nothing.
 *
 */
STATIC void
cda_v_echo(bool_t doecho)
{
	echflag = doecho;
}


/*
 * cda_clrstatus
 *	Clear the status line and position cursor at the beginning.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_clrstatus(void)
{
	int	i;

	(void) wmove(status_win, 1, 1);
	for (i = 1; i < scr_cols-1; i++)
		(void) waddch(status_win, ' ');
	(void) wmove(status_win, 1, 1);
}


/*
 * cda_v_errhandler
 *	Handler function used when the daemon has exited.
 *
 * Args:
 *	code - status code from cda_sendcmd
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_v_errhandler(int code)
{
	if (code == CDA_OK || code == CDA_PARMERR)
		/* Ignore these codes */
		return;

	/* Daemon is exiting or has exited */
	cda_clrstatus();

	if (code < 0 || code == CDA_DAEMON_EXIT) {
		(void) wprintw(status_win,
			"CD audio daemon pid=%d dev=%s exited.",
			daemon_pid, app_data.device);

		ostat_on = stat_on;
		stat_on = FALSE;

		if (cda_sfd[1] >= 0) {
			(void) close(cda_sfd[1]);
			(void) close(cda_rfd[1]);
			(void) close(cda_rfd[0]);
			cda_sfd[1] = cda_rfd[1] = cda_rfd[0] = -1;
		}

		loaddb = TRUE;	/* Force reload of CDDB */
		track = -2;	/* Force redisplay of version info */
		scroll_line = 0;
	}
	else
		(void) wprintw(status_win, "%s", emsgp);

	(void) wrefresh(status_win);
}


/*
 * cda_v_match_select
 *	Ask the user to select from a list of fuzzy CDDB matches.
 *
 * Args:
 *	matchlist - List of fuzzy matches
 *
 * Return:
 *	User selection number, or 0 for no selection.
 */
STATIC int
cda_v_match_select(cdinfo_match_t *matchlist)
{
	int		i,
			n,
			x;
	cdinfo_match_t	*p;
	chset_conv_t	*up;
	char		*p1,
			*p2,
			*q1,
			*q2,
			input[64];

	(void) wclear(info_win);
	(void) wmove(info_win, 0, 0);

	for (p = matchlist, i = 0; p != NULL; p = p->next, i++)
		;

	if (i == 1 && app_data.single_fuzzy) {
		(void) wprintw(info_win, "\n\n%s\n%s\n\n",
		    "CDDB has found an inexact match.  If this is not the",
		    "correct album, please submit corrections using xmcd.");
		return 1;
	}

	/* Convert CD info from UTF-8 to local charset if possible */
	if ((up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return 0;

	(void) wprintw(info_win, "\n%s\n%s\n\n",
		"CDDB has found the following potential matches for the album",
		"that you have.  Please choose an appropriate entry:");

	for (p = matchlist, i = 1; p != NULL; p = p->next, i++) {
		q1 = (p->artist == NULL) ? "(unknown artist)" : p->artist;
		q2 = (p->title == NULL) ? "(unknown title)" : p->title;
		p1 = p2 = NULL;
		if (!util_chset_conv(up, q1, &p1, FALSE) &&
		    !util_newstr(&p1, q1)) {
			util_chset_close(up);
			return 0;
		}
		if (!util_chset_conv(up, q2, &p2, FALSE) &&
		    !util_newstr(&p2, q2)) {
			util_chset_close(up);
			return 0;
		}
		(void) wprintw(info_win, "  %2d. %.30s / %-40s\n", i,
				p1 == NULL ? "" : p1,
				p2 == NULL ? "" : p2);
		if (p1 != NULL)
			MEM_FREE(p1);
		if (p2 != NULL)
			MEM_FREE(p2);
	}

	(void) wprintw(info_win, "  %2d. None of the above\n\n", i);

	util_chset_close(up);

	scroll_line = 0;
	(void) prefresh(info_win, scroll_line, 0, 0, 0,
			scr_lines - 9, scr_cols - 1);
	getyx(info_win, scroll_length, x);
#ifdef lint
	x = x;	/* Get rid of "set but not used" lint warning */
#endif
	--scroll_length;

	for (;;) {
		cda_clrstatus();
		(void) wprintw(status_win, "Choose one (1-%d): ", i);
		cda_v_echo(TRUE);
		(void) putp(cursor_normal);
		(void) wrefresh(status_win);
		cda_wgetstr(status_win, input, 60);

		n = atoi(input);
		if (n > 0 && n <= i)
			break;

		beep();
	}

	cda_v_echo(FALSE);
	(void) putp(cursor_invisible);

	if (n == i)
		return 0;

	cda_clrstatus();
	(void) wprintw(status_win, "Accessing CDDB...");
	(void) wrefresh(status_win);
	util_delayms(2000);
	return (n);
}


/*
 * cda_v_auth
 *	Ask the user to enter name and password for proxy authorization.
 *
 * Args:
 *	retrycnt - Number of times the user tried
 *
 * Return:
 *	0 - failure
 *	1 - success
 */
STATIC int
cda_v_auth(int retrycnt)
{
	word32_t	arg[CDA_NARGS];
	char		input[64];
	size_t		maxsz = (CDA_NARGS - 1) * sizeof(word32_t);
	int		x,
			retcode;

	(void) wclear(info_win);
	(void) wmove(info_win, 0, 0);

	if (retrycnt == 0)
		(void) wprintw(info_win, "Proxy Authorization is required.\n");
	else {
		(void) wprintw(info_win, "Proxy Authorization failed.");

		if (retrycnt < 3)
			(void) wprintw(info_win, "  Try again.\n");
		else {
			(void) wprintw(info_win, "  Too many tries.\n");
			scroll_line = 0;
			(void) prefresh(info_win, scroll_line, 0, 0, 0,
					scr_lines - 9, scr_cols - 1);
			getyx(info_win, scroll_length, x);
#ifdef lint
			x = x;	/* Get rid of "set but not used" lint warning */
#endif
			--scroll_length;
			util_delayms(2000);
			return 0;
		}
	}

	(void) wprintw(info_win, "%s\n\n",
		"Please enter your proxy user name and password.");

	scroll_line = 0;
	(void) prefresh(info_win, scroll_line, 0, 0, 0,
			scr_lines - 9, scr_cols - 1);
	getyx(info_win, scroll_length, x);
#ifdef lint
	x = x;	/* Get rid of "set but not used" lint warning */
#endif
	--scroll_length;

	/* Get user name */
	input[0] = '\0';
	cda_clrstatus();
	(void) wprintw(status_win, "Username: ");
	cda_v_echo(TRUE);
	(void) putp(cursor_normal);
	(void) wrefresh(status_win);
	cda_wgetstr(status_win, input, 60);
	if (input[0] == '\0')
		return 0;

	if (!util_newstr(&dbp->proxy_user, input))
		CDA_V_FATAL(app_data.str_nomemory, 6);

	/* Get password */
	input[0] = '\0';
	cda_clrstatus();
	(void) wprintw(status_win, "Password: ");
	cda_v_echo(FALSE);
	(void) wrefresh(status_win);
	cda_wgetstr(status_win, input, 60);

	if (!util_newstr(&dbp->proxy_passwd, input))
		CDA_V_FATAL(app_data.str_nomemory, 6);

	/* Pass the proxy user name and password to the daemon */

	(void) memset(arg, 0, CDA_NARGS * sizeof(word32_t));
	(void) strcpy((char *) &arg[0], "user");
	(void) strncpy((char *) &arg[1], dbp->proxy_user, maxsz - 1);
	if (!cda_sendcmd(CDA_AUTHUSER, arg, &retcode)) {
		(void) wprintw(status_win, "%s:\n%s\n"
			"Failed sending proxy user name to the cda daemon.",
			emsgp
		);
		util_delayms(2000);
	}

	(void) memset(arg, 0, CDA_NARGS * sizeof(word32_t));
	(void) strcpy((char *) &arg[0], "pass");
	(void) strncpy((char *) &arg[1], dbp->proxy_passwd, maxsz - 1);
	if (!cda_sendcmd(CDA_AUTHPASS, arg, &retcode)) {
		(void) wprintw(status_win, "%s:\n%s\n"
			"Failed sending proxy password to the cda daemon.",
			emsgp
		);
		util_delayms(2000);
	}

	return 1;
}


/*
 * cda_screen
 *	Paints the screen in visual mode, geting status and extinfo
 *	as required.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_screen(int signo)
{
	word32_t	arg[CDA_NARGS],
			discid,
			astat0 = 0,
			astat1 = 0;
	int		x,
			y,
			i,
			retcode,
			vol,
			bal,
			rptcnt;
	unsigned int	discnum = 0,
			trk,
			idx,
			min,
			sec;
	char		*p,
			*p1,
			*p2,
			*q1,
			*q2,
			*ctrlver;
	curstat_t	*s = curstat_addr();
	bool_t		playing;
	struct stat	stbuf;
	static unsigned int odiscnum;
	static chset_conv_t *up = NULL;

	/* Convert CD info from UTF-8 to local charset if possible */
	if (up == NULL && (up = util_chset_open(CHSET_UTF8_TO_LOCALE)) == NULL)
		return;

	/* Window resize event? */
	if (win_resize) {
		win_resize = FALSE;

		/* Re-init curses */
		cda_makewins();

		/* Force a repaint */
		wclear(info_win);
		wclear(status_win);
		oastat0 = oastat1 = (word32_t) -1;
		ostat_on = !stat_on;
		old_help = !help;
		old_route = !route;
		refresh_fkeys = TRUE;
	}

	/* Need to refresh function key labels? */
	if (refresh_fkeys) {
		refresh_fkeys = FALSE;
		(void) wmove(status_win, 3, 0);
		(void) waddstr(status_win, STATUS_LINE0);
		(void) wmove(status_win, 4, 0);
		(void) waddstr(status_win, STATUS_LINE1);
		(void) wmove(status_win, 5, 0);
		(void) waddstr(status_win, STATUS_LINE2);
		(void) wrefresh(status_win);

		(void) box(status_win, 0, 0);
	}

	/* If daemon running, get status and update display */
	switch ((int) stat_on) {
	case FALSE:
		/* Daemon not running - just update display to "off" */
		if (stat_on != ostat_on) {
			(void) wmove(status_win, ON_Y, ON_X);
			(void) waddstr(status_win, "On");

			(void) wmove(status_win, OFF_Y, OFF_X);
			(void) wattron(status_win, A_STANDOUT);
			(void) waddstr(status_win, "Off");
			(void) wattroff(status_win, A_STANDOUT);

			(void) wmove(status_win, LOAD_Y, LOAD_X);
			(void) waddstr(status_win, "Load");

			(void) wmove(status_win, EJECT_Y, EJECT_X);
			(void) waddstr(status_win, "Eject");

			(void) wmove(status_win, PLAY_Y, PLAY_X);
			(void) waddstr(status_win, "Play");

			(void) wmove(status_win, PAUSE_Y, PAUSE_X);
			(void) waddstr(status_win, "Pause");

			(void) wmove(status_win, STOP_Y, STOP_X);
			(void) waddstr(status_win, "Stop");

			(void) wmove(status_win, LOCK_Y, LOCK_X);
			(void) waddstr(status_win, "Lock");

			(void) wmove(status_win, UNLOCK_Y, UNLOCK_X);
			(void) waddstr(status_win, "Unlock");

			(void) wmove(status_win, SHUFFLE_Y, SHUFFLE_X);
			(void) waddstr(status_win, "Shuffle");

			(void) wmove(status_win, PROGRAM_Y, PROGRAM_X);
			(void) waddstr(status_win, "Program");

			(void) wmove(status_win, REPEAT_ON_Y, REPEAT_ON_X);
			(void) waddstr(status_win, "On");

			(void) wmove(status_win, REPEAT_OFF_Y, REPEAT_OFF_X);
			(void) waddstr(status_win, "Off");
		}

		/* Check to see if daemon started */
		if (cda_daemon_alive()) {
			/* Let the mkfifo complete */
			util_delayms(1000);

			/* Daemon is alive: open FIFOs - command side */
			if (cda_sfd[1] < 0)
				cda_sfd[1] = open(spipe, O_WRONLY
#ifdef O_NOFOLLOW
							 | O_NOFOLLOW
#endif
				);

			if (cda_sfd[1] >= 0) {
				stat_on = TRUE;

				cda_rfd[1] = open(rpipe, O_RDONLY
#ifdef O_NOFOLLOW
							 | O_NOFOLLOW
#endif
				);
				if (cda_rfd[1] < 0) {
				    CDA_V_FATAL(
					"Cannot open recv pipe for reading", 6
				    );
				}
				cda_rfd[0] = open(rpipe, O_WRONLY
#ifdef O_NOFOLLOW
							 | O_NOFOLLOW
#endif
				);
				if (cda_rfd[0] < 0) {
				    CDA_V_FATAL(
					"Cannot open recv pipe for writing", 6
				    );
				}
			}
			else {
				CDA_V_FATAL(
				    "Cannot open send pipe for writing", 6
				);
			}

			/* Check FIFOs */
			if (fstat(cda_sfd[1], &stbuf) < 0 ||
			    !S_ISFIFO(stbuf.st_mode))
				CDA_V_FATAL("Send pipe error: Not a FIFO", 6);

			if (fstat(cda_rfd[1], &stbuf) < 0 ||
			    !S_ISFIFO(stbuf.st_mode))
				CDA_V_FATAL("Recv pipe error: Not a FIFO", 6);

			if (fstat(cda_rfd[0], &stbuf) < 0 ||
			    !S_ISFIFO(stbuf.st_mode))
				CDA_V_FATAL("Recv pipe error: Not a FIFO", 6);
		}

		if (!stat_on)
			break;

		arg[0] = 0;
		if (!cda_sendcmd(CDA_ON, arg, &retcode))
			cda_v_errhandler(retcode);
		else {
			daemon_pid = (pid_t) arg[0];
			cda_clrstatus();
			(void) wprintw(status_win,
				"CD audio daemon pid=%d dev=%s started.",
				daemon_pid, app_data.device);
			(void) wrefresh(status_win);
		}

		/*FALLTHROUGH*/

	case TRUE:
		/* Daemon running - get status and update display */
		(void) memset(arg, 0, CDA_NARGS * sizeof(word32_t));
		if (!cda_sendcmd(CDA_STATUS, arg, &retcode)) {
			cda_v_errhandler(retcode);

			oastat0 = astat0;
			oastat1 = astat1;

			/* Come back in 1 sec */
			(void) util_signal(SIGALRM, cda_screen);
			(void) alarm(1);
			return;
		}

		(void) wmove(status_win, ON_Y, ON_X);
		(void) wattron(status_win, A_STANDOUT);
		(void) waddstr(status_win, "On");
		(void) wattroff(status_win, A_STANDOUT);

		(void) wmove(status_win, OFF_Y, OFF_X);
		(void) waddstr(status_win, "Off");

		astat0 = arg[0];
		astat1 = arg[1];
		rptcnt = (int) arg[2];
		vol = (int) arg[3];
		bal = (int) arg[4];
		route = (int) arg[5];
		discid = arg[6];
		discnum = (unsigned int) arg[7];
		s->cur_disc = arg[7];

		switch (app_data.chg_method) {
		case CHG_SCSI_LUN:
			s->curdev = cda_devlist[s->cur_disc - 1];
			break;
		case CHG_SCSI_MEDCHG:
		case CHG_OS_IOCTL:
		case CHG_NONE:
		default:
			s->curdev = cda_devlist[0];
			break;
		}

		if (RD_ARG_MODE(astat0) == MOD_NODISC || odiscnum != discnum)
			loaddb = TRUE;

		if (discid != 0 && loaddb) {
			/* Load CD information */
			loaddb = FALSE;
			cda_clrstatus();
			(void) wprintw(status_win, "Accessing CDDB...");
			(void) wrefresh(status_win);
			util_delayms(2000);

			cda_dbclear(s, TRUE);
			dbp->discid = discid;
			if (cda_dbload(dbp->discid, cda_v_match_select,
				       cda_v_auth, 0))
				s->qmode = QMODE_MATCH;
			else
				s->qmode = QMODE_NONE;

			cda_clrstatus();
		}

		if (astat0 != oastat0 || astat1 != oastat1) {
			if (RD_ARG_MODE(astat0) == MOD_STOP &&
			    RD_ARG_MODE(oastat0) != MOD_NODISC) {
				/* CD no longer busy or playing:
				 * clear status line.
				 */
				cda_clrstatus();
				refresh_sts = TRUE;
			}

			switch (RD_ARG_MODE(astat0)) {
			case MOD_BUSY:
			case MOD_NODISC:
				(void) wmove(status_win, LOAD_Y, LOAD_X);
				(void) waddstr(status_win, "Load");
				 
				(void) wmove(status_win, EJECT_Y, EJECT_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Eject");
				(void) wattroff(status_win, A_STANDOUT);

				(void) wmove(status_win, PLAY_Y, PLAY_X);
				(void) waddstr(status_win, "Play");

				(void) wmove(status_win, PAUSE_Y, PAUSE_X);
				(void) waddstr(status_win, "Pause");

				(void) wmove(status_win, STOP_Y, STOP_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Stop");
				(void) wattroff(status_win, A_STANDOUT);

				break;

			case MOD_STOP:
				(void) wmove(status_win, LOAD_Y, LOAD_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Load");
				(void) wattroff(status_win, A_STANDOUT);

				(void) wmove(status_win, EJECT_Y, EJECT_X);
				(void) waddstr(status_win, "Eject");

				(void) wmove(status_win, PLAY_Y, PLAY_X);
				(void) waddstr(status_win, "Play");

				(void) wmove(status_win, PAUSE_Y, PAUSE_X);
				(void) waddstr(status_win, "Pause");

				(void) wmove(status_win, STOP_Y, STOP_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Stop");
				(void) wattroff(status_win, A_STANDOUT);

				break;

			case MOD_PLAY:
				(void) wmove(status_win, LOAD_Y, LOAD_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Load");
				(void) wattroff(status_win, A_STANDOUT);

				(void) wmove(status_win, EJECT_Y, EJECT_X);
				(void) waddstr(status_win, "Eject");

				(void) wmove(status_win, PLAY_Y, PLAY_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Play");
				(void) wattroff(status_win, A_STANDOUT);

				(void) wmove(status_win, PAUSE_Y, PAUSE_X);
				(void) waddstr(status_win, "Pause");

				(void) wmove(status_win, STOP_Y, STOP_X);
				(void) waddstr(status_win, "Stop");

				break;

			case MOD_PAUSE:
				(void) wmove(status_win, LOAD_Y, LOAD_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Load");
				(void) wattroff(status_win, A_STANDOUT);

				(void) wmove(status_win, EJECT_Y, EJECT_X);
				(void) waddstr(status_win, "Eject");

				(void) wmove(status_win, PLAY_Y, PLAY_X);
				(void) waddstr(status_win, "Play");

				(void) wmove(status_win, PAUSE_Y, PAUSE_X);
				(void) wattron(status_win, A_STANDOUT);
				(void) waddstr(status_win, "Pause");
				(void) wattroff(status_win, A_STANDOUT);

				(void) wmove(status_win, STOP_Y, STOP_X);
				(void) waddstr(status_win, "Stop");

				break;
			}

			(void) wmove(status_win, LOCK_Y, LOCK_X);
			if (RD_ARG_LOCK(astat0))
				(void) wattron(status_win, A_STANDOUT);
			(void) waddstr(status_win, "Lock");
			(void) wattroff(status_win, A_STANDOUT);

			(void) wmove(status_win, UNLOCK_Y, UNLOCK_X);
			if (!RD_ARG_LOCK(astat0))
				(void) wattron(status_win, A_STANDOUT);
			(void) waddstr(status_win, "Unlock");
			(void) wattroff(status_win, A_STANDOUT);

			(void) wmove(status_win, SHUFFLE_Y, SHUFFLE_X);
			if (RD_ARG_SHUF(astat0))
				(void) wattron(status_win, A_STANDOUT);
			(void) waddstr(status_win, "Shuffle");
			(void) wattroff(status_win, A_STANDOUT);

			(void) wmove(status_win, PROGRAM_Y, PROGRAM_X);
			if (stat_on &&
			    RD_ARG_MODE(astat0) != MOD_BUSY &&
			    RD_ARG_MODE(astat0) != MOD_NODISC &&
			    !RD_ARG_SHUF(astat0)) {
				arg[0] = 1;
				if (!cda_sendcmd(CDA_PROGRAM, arg, &retcode)) {
					cda_v_errhandler(retcode);

					oastat0 = astat0;
					oastat1 = astat1;

					/* Come back in 1 sec */
					(void) util_signal(SIGALRM,
							   cda_screen);
					(void) alarm(1);
					return;
				}

				if (arg[0] != 0)
					wattron(status_win, A_STANDOUT);
			}
			(void) waddstr(status_win, "Program");
			(void) wattroff(status_win, A_STANDOUT);

			(void) wmove(status_win, REPEAT_ON_Y, REPEAT_ON_X);
			if (RD_ARG_REPT(astat0))
				(void) wattron(status_win, A_STANDOUT);
			(void) waddstr(status_win, "On");
			(void) wattroff(status_win, A_STANDOUT);

			(void) wmove(status_win, REPEAT_OFF_Y, REPEAT_OFF_X);
			if (!RD_ARG_REPT(astat0))
				(void) wattron(status_win, A_STANDOUT);
			(void) waddstr(status_win, "Off");
			(void) wattroff(status_win, A_STANDOUT);
		}

		wmove(status_win, 1, 1);
		if (RD_ARG_MODE(astat0) == MOD_BUSY) {
			cda_clrstatus();
			(void) wprintw(status_win, "CD Busy");
		}
		else if (RD_ARG_MODE(astat0) == MOD_PLAY ||
			 RD_ARG_MODE(astat0) == MOD_PAUSE ||
			 RD_ARG_MODE(astat0) == MOD_NODISC) {

			if (RD_ARG_MODE(astat0) == MOD_NODISC) {
				track = -2;	/* Force redisplay of
						 * version/device
						 */
				(void) wprintw(status_win, "No Disc   ");
			}
			else {
				trk = (unsigned int) RD_ARG_TRK(astat1);
				idx = (unsigned int) RD_ARG_IDX(astat1);
				min = (unsigned int) RD_ARG_MIN(astat1);
				sec = (unsigned int) RD_ARG_SEC(astat1);

				if (idx == 0 && app_data.subq_lba) {
					/* In LBA mode the time information
					 * isn't meaningful when in the
					 * lead-in, so just set to zero.
					 */
					min = sec = 0;
				}

				(void) wprintw(status_win,
					"Disc %u Track %02u Index %02u "
					"%s%02u:%02u  ",
					discnum, trk, idx,
					(idx == 0) ? "-" : "+", min, sec);
			}

			(void) wprintw(status_win,
				"Volume %3u%% Balance %3u%% ", vol, bal);

			switch (route) {
			case 0:
				(void) wprintw(status_win, "Stereo    ");
				break;
			case 1:
				(void) wprintw(status_win, "Reverse   ");
				break;
			case 2:
				(void) wprintw(status_win, "Mono-L    ");
				break;
			case 3:
				(void) wprintw(status_win, "Mono-R    ");
				break;
			case 4:
				(void) wprintw(status_win, "Mono-L+R  ");
				break;
			}

			if (rptcnt >= 0)
				(void) wprintw(status_win, "Count %u", rptcnt);

			getyx(status_win, y, x);
#ifdef lint
			y = y;	/* Get rid of "set but not used" lint warning */
#endif
			for (i = x; i < scr_cols-1; i++)
				(void) waddch(status_win, ' ');

			(void) wmove(status_win, 1, 1);
		}
		else if (refresh_sts) {
			refresh_sts = FALSE;
			cda_clrstatus();

			if (stat_on &&
			    RD_ARG_MODE(astat0) != MOD_BUSY &&
			    RD_ARG_MODE(astat0) != MOD_NODISC &&
			    !RD_ARG_SHUF(astat0)) {
				arg[0] = 1;
				if (!cda_sendcmd(CDA_PROGRAM, arg, &retcode)) {
					cda_v_errhandler(retcode);

					oastat0 = astat0;
					oastat1 = astat1;

					/* Come back in 1 sec */
					(void) util_signal(SIGALRM,
							   cda_screen);
					(void) alarm(1);
					return;
				}

				if (arg[0] != 0) {
					(void) wprintw(status_win,
							"Program: ");
					for (i = 1; i <= arg[0]; i++) {
						(void) wprintw(status_win,
							" %02u", arg[i+1]);
					}
				}
			}
		}
		break;
	}

	(void) wrefresh(status_win);

	/* See if we want to display help info */
	if (help) {
		if (!old_help) {
			(void) wclear(info_win);
			(void) wmove(info_win, 0, 0);
			(void) wprintw(info_win, HELP_INFO);
			scroll_line = 0;
			(void) prefresh(info_win, scroll_line, 0, 0, 0,
					scr_lines - 9, scr_cols - 1);
			old_help = help;
		}
		odiscnum = discnum;
		(void) util_signal(SIGALRM, cda_screen);
		(void) alarm(1);
		return;
	}
	else if (old_help) {
		(void) wclear(info_win);
		scroll_line = 0;
		track = -2;	/* Force display of version/device */
	}

	/* Disc changed: redisplay */
	if (odiscnum != discnum) {
		(void) wclear(info_win);
		scroll_line = 0;
		track = -2;
	}

	/* If state is unchanged since last time, no more to do */
	if (stat_on == ostat_on && old_help == help && old_route == route &&
	    RD_ARG_MODE(astat0) == RD_ARG_MODE(oastat0) &&
	    RD_ARG_TRK(astat1) == RD_ARG_TRK(oastat1) &&
	    RD_ARG_IDX(astat1) == RD_ARG_IDX(oastat1) &&
	    odiscnum == discnum) {
		oastat0 = astat0;
		oastat1 = astat1;

		/* Call us again - nothing is too much trouble! */
		(void) util_signal(SIGALRM, cda_screen);
		(void) alarm(1);
		return;
	}

	old_help = help;
	old_route = route;
	odiscnum = discnum;
	ostat_on = stat_on;
	oastat0 = astat0;
	oastat1 = astat1;

	/* Now display data, according to our state: */

	/* Off, busy or no disc, display version and device */
	if (!stat_on ||
	    RD_ARG_MODE(astat0) == MOD_BUSY ||
	    RD_ARG_MODE(astat0) == MOD_NODISC) {
		if (track != -1) {
			track = -1;
			(void) wclear(info_win);
			(void) wmove(info_win, 0,0);
			(void) wprintw(info_win,
				"CDA - Command Line CD Audio Player/Ripper");
			(void) wmove(info_win, 0, scr_cols-18);
			(void) wprintw(info_win, "Press ");
			(void) wattron(info_win, A_STANDOUT);
			(void) wprintw(info_win, "?");
			(void) wattroff(info_win, A_STANDOUT);
			(void) wprintw(info_win, " for help.\n\n");

			(void) wprintw(info_win, "CD audio        %s.%s.%s\n",
				VERSION_MAJ, VERSION_MIN, VERSION_TEENY);

			switch ((int) stat_on) {
			case TRUE:
				if (cda_sendcmd(CDA_VERSION, arg, &retcode)) {
					(void) wprintw(info_win,
						"CD audio daemon %s\n",
						(char *) arg);
					break;
				}
				cda_v_errhandler(retcode);
				/*FALLTHROUGH*/

			case FALSE:
				(void) sprintf((char *) arg, "%s.%s.%s\n%s\n",
					       VERSION_MAJ, VERSION_MIN,
					       VERSION_TEENY, di_methodstr());
				break;
			}
			(void) wprintw(info_win, "\nDevice: %s\n",
					app_data.device);
			if (stat_on) {
				if (!cda_sendcmd(CDA_DEVICE, arg, &retcode)) {
					cda_v_errhandler(retcode);

					oastat0 = astat0;
					oastat1 = astat1;

					/* Come back in 1 sec */
					(void) util_signal(SIGALRM,
							   cda_screen);
					(void) alarm(1);
					return;
				}
				(void) wprintw(info_win, "%s", (char *) arg);
			}
			(void) wprintw(info_win,
				"\n%s\nURL: %s\nE-mail: %s\n\n%s\n\n",
				COPYRIGHT, XMCD_URL, EMAIL, GNU_BANNER);

			ctrlver = cdinfo_cddbctrl_ver();
			(void) wprintw(info_win, "CDDB%s service%s%s\n%s\n",
				(cdinfo_cddb_ver() == 2) ?
					"\262" : " \"classic\"",
				(ctrlver[0] == '\0') ? "" : ": ",
				(ctrlver[0] == '\0') ? "\n" : ctrlver,
				CDDB_BANNER);

			(void) prefresh(info_win, scroll_line, 0, 0, 0,
					scr_lines - 9, scr_cols - 1);
			getyx(info_win, scroll_length, i);
			--scroll_length;
		}
	}
	else if (track != RD_ARG_TRK(astat1)) {
		/* If disc loaded, display extinfo */

		(void) wclear(info_win);
		(void) wmove(info_win, 0, 0);

		/* Get CD information */
		if (RD_ARG_MODE(astat0) == MOD_PLAY ||
		    RD_ARG_MODE(astat0) == MOD_PAUSE) {
			track = RD_ARG_TRK(astat1);
		}
		else
			track = -1;

		arg[0] = 0;
		arg[1] = track;
		if (!cda_sendcmd(CDA_EXTINFO, arg, &retcode)) {
			cda_v_errhandler(retcode);

			oastat0 = astat0;
			oastat1 = astat1;

			/* Come back in 1 sec */
			(void) util_signal(SIGALRM, cda_screen);
			(void) alarm(1);
			return;
		}

		if (s->qmode != QMODE_MATCH ||
		    RD_ARG_MODE(astat0) == MOD_STOP) {
			int	ntrks;

			/* No CD information */
			arg[0] = 0;
			if (!cda_sendcmd(CDA_TOC, arg, &retcode)) {
				cda_v_errhandler(retcode);

				oastat0 = astat0;
				oastat1 = astat1;

				/* Come back in 1 sec */
				(void) util_signal(SIGALRM, cda_screen);
				(void) alarm(1);
				return;
			}

			dbp->discid = arg[0];
			ntrks = (int) (arg[0] & 0xff);

			q1 = (dbp->disc.genre == NULL) ?
				"(unknown genre)" :
				cdinfo_genre_name(dbp->disc.genre);
			p1 = NULL;
			if (!util_chset_conv(up, q1, &p1, FALSE) &&
			    !util_newstr(&p1, q1)) {
				util_chset_close(up);
				return;
			}
			(void) wprintw(info_win, "Disc %u  Genre: %s %s",
				discnum,
				p1 == NULL ? "" : p1,
				(dbp->disc.notes != NULL ||
				 dbp->disc.credit_list != NULL) ?
					 "*" : "");
			if (p1 != NULL)
				MEM_FREE(p1);

			(void) wmove(info_win, 0, scr_cols-18);
			(void) wprintw(info_win, "Press ");
			(void) wattron(info_win, A_STANDOUT);
			(void) wprintw(info_win, "?");
			(void) wattroff(info_win, A_STANDOUT);
			(void) wprintw(info_win, " for help.\n\n");

			q1 = (dbp->disc.artist == NULL) ?
				"" : dbp->disc.artist;
			q2 = (dbp->disc.title == NULL) ?
				"(unknown title)" : dbp->disc.title;
			p1 = p2 = NULL;
			if (!util_chset_conv(up, q1, &p1, FALSE) &&
			    !util_newstr(&p1, q1)) {
				util_chset_close(up);
				return;
			}
			if (!util_chset_conv(up, q2, &p2, FALSE) &&
			    !util_newstr(&p1, q1)) {
				util_chset_close(up);
				return;
			}
			(void) wprintw(info_win, "%s%s%s\n\n",
				p1 == NULL ? "" : p1,
				(dbp->disc.artist != NULL &&
					dbp->disc.title != NULL) ? " / " : "",
				p2 == NULL ? "" : p2);
			if (p1 != NULL)
				MEM_FREE(p1);
			MEM_FREE(p2);

			for (i = 0; i < (int) ntrks; i++) {
				RD_ARG_TOC(arg[i+1], trk, playing, min, sec);
				(void) wprintw(info_win, "%s%02u %02u:%02u  ",
					playing ? ">" : " ",
					trk, min, sec);

				if (dbp->track[i].title != NULL) {
				    q1 = dbp->track[i].title;
				    p1 = NULL;
				    if (!util_chset_conv(up, q1, &p1, FALSE) &&
					!util_newstr(&p1, q1)) {
					    util_chset_close(up);
					    return;
				    }
				    (void) wprintw(info_win, "%s%s\n",
					p1 == NULL ? "" : p1,
					(dbp->track[i].notes != NULL ||
					 dbp->track[i].credit_list != NULL) ?
					    "*" : "");
				    if (p1 != NULL)
					    MEM_FREE(p1);
				}
				else {
				    (void) wprintw(info_win, "??%s\n",
					(dbp->track[i].notes != NULL ||
					 dbp->track[i].credit_list != NULL) ?
					    "*" : "");
				}
			}

			RD_ARG_TOC(arg[i+1], trk, playing, min, sec);
			(void) wprintw(info_win, "\nTotal Time: %02u:%02u\n",
				min, sec);
		}
		else {
			/* Got CD information */
			if (dbp->disc.notes == NULL) {
				(void) wprintw(info_win,
				    "No Disc Notes for this CD.\n");
			}
			else {
				q1 = (dbp->disc.artist == NULL) ?
					"" : dbp->disc.artist;
				q2 = (dbp->disc.title == NULL) ?
					"(unknown title)" : dbp->disc.title;
				p1 = p2 = NULL;
				if (!util_chset_conv(up, q1, &p1, FALSE) &&
				    !util_newstr(&p1, q1)) {
					util_chset_close(up);
					return;
				}
				if (!util_chset_conv(up, q2, &p2, FALSE) &&
				    !util_newstr(&p2, q2)) {
					util_chset_close(up);
					return;
				}
				(void) wprintw(info_win, "%s%s%s\n\n",
				    p1 == NULL ? "" : p1,
				    (dbp->disc.artist != NULL &&
					dbp->disc.title != NULL) ? " / " : "",
				    p2 == NULL ? "" : p2);

				q1 = dbp->disc.notes;
				p1 = NULL;
				if (!util_chset_conv(up, q1, &p1, FALSE) &&
				    !util_newstr(&p1, q1)) {
					util_chset_close(up);
					return;
				}
				/* Not using wprintw here to avoid a bug
				 * with very long strings
				 */
				for (p = p1; p != NULL && *p != '\0'; p++)
				       (void) waddch(info_win, *p);
				(void) waddch(info_win, '\n');
				if (p1 != NULL)
					MEM_FREE(p1);
			}

			for (i = 0; i < scr_cols-1; i++)
				(void) waddch(info_win, ACS_HLINE);
			(void) waddch(info_win, '\n');

			/* If Play or Pause, display track info */
			if (RD_ARG_MODE(astat0) == MOD_PLAY ||
			    RD_ARG_MODE(astat0) == MOD_PAUSE) {
				if (dbp->track[arg[2]].title == NULL) {
					(void) wprintw(info_win,
						"(No title for track %02u.)\n",
						arg[1]);
				}
				else {
					q1 = dbp->track[arg[2]].title;
					p1 = NULL;
					if (!util_chset_conv(up, q1,
							     &p1, FALSE) &&
					    !util_newstr(&p1, q1)) {
						util_chset_close(up);
						return;
					}
					(void) wprintw(info_win, "%s\n",
						       p1 == NULL ? "" : p1);
					if (p1 != NULL)
						MEM_FREE(p1);

					if (dbp->track[arg[2]].notes != NULL) {
						q1 = dbp->track[arg[2]].notes;
						p1 = NULL;
						if (!util_chset_conv(up,
							    q1, &p1, FALSE) &&
						    !util_newstr(&p1, q1)) {
							util_chset_close(up);
							return;
						}
						/* Not using wprintw here
						 * to avoid a bug with very
						 * long strings
						 */
						(void) waddch(info_win, '\n');
						for (p = p1;
						     p != NULL && *p != '\0';
						     p++)
						       (void) waddch(info_win,
								     *p);
						(void) waddch(info_win, '\n');
						if (p1 != NULL)
							MEM_FREE(p1);
					}
				}
			}
		}
		scroll_line = 0;
		getyx(info_win, scroll_length, i);
		(void) prefresh(info_win, scroll_line, 0, 0, 0,
				scr_lines - 9, scr_cols - 1);
	}
	oastat0 = astat0;
	oastat1 = astat1;

	/* Come back in 1 sec */
	(void) util_signal(SIGALRM, cda_screen);
	(void) alarm(1);
}


/*
 * cda_v_on_off
 *	Handler function for the visual mode on/off control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_on_off(int inp, word32_t arg[])
{
	int		retcode;
	curstat_t	*s = curstat_addr();

	cda_clrstatus();

	if (!stat_on) {
		if (cda_daemon_alive()) {
			(void) wprintw(status_win,
				"CD audio daemon already running.");
			(void) wrefresh(status_win);
			beep();
		}
		else {
			/* Start CDA daemon */
			loaddb = TRUE;
			(void) wprintw(status_win, "Initializing...");
			(void) wrefresh(status_win);

			if (!cda_daemon(s)) {
				cda_clrstatus();
				(void) wprintw(status_win,
					"Cannot start CD audio daemon.");
				(void) wrefresh(status_win);
			}
		}
	}
	else {
		if (cda_sendcmd(CDA_OFF, arg, &retcode)) {
			(void) wprintw(status_win,
					"Stopping CD audio daemon...");
			(void) wrefresh(status_win);

			/* Wait for daemon to die */
			do {
				util_delayms(1000);
			} while (cda_daemon_alive());
		}

		cda_v_errhandler(retcode);
	}
}


/*
 * cda_v_load_eject
 *	Handler function for the visual mode load/eject control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_load_eject(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on || RD_ARG_MODE(arg[0]) == MOD_BUSY) {
		beep();
		return;
	}

	arg[0] = (RD_ARG_MODE(arg[0]) == MOD_NODISC) ? 0 : 1;

	if (!cda_sendcmd(CDA_DISC, arg, &retcode))
		cda_v_errhandler(retcode);

	loaddb = TRUE;	/* Force reload of CDDB */
	refresh_sts = TRUE;
}


/*
 * cda_v_play_pause
 *	Handler function for the visual mode play/pause control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_play_pause(int inp, word32_t arg[])
{
	word32_t	cmd;
	int		retcode;

	if (!stat_on ||
	    RD_ARG_MODE(arg[0]) == MOD_BUSY ||
	    RD_ARG_MODE(arg[0]) == MOD_NODISC) {
		beep();
		return;
	}

	if (RD_ARG_MODE(arg[0]) == MOD_PLAY)
		cmd = CDA_PAUSE;
	else
		cmd = CDA_PLAY;

	arg[0] = 0;
	if (!cda_sendcmd(cmd, arg, &retcode))
		cda_v_errhandler(retcode);
}


/*
 * cda_v_stop
 *	Handler function for the visual mode stop control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_stop(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on ||
	    RD_ARG_MODE(arg[0]) == MOD_BUSY ||
	    RD_ARG_MODE(arg[0]) == MOD_NODISC) {
		beep();
		return;
	}

	if (RD_ARG_MODE(arg[0]) != MOD_PLAY &&
	    RD_ARG_MODE(arg[0]) != MOD_PAUSE) {
		beep();
		return;
	}

	if (!cda_sendcmd(CDA_STOP, arg, &retcode))
		cda_v_errhandler(retcode);

	refresh_sts = TRUE;
}


/*
 * cda_v_lock
 *	Handler function for the visual mode lock/unlock control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_lock(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on ||
	    RD_ARG_MODE(arg[0]) == MOD_BUSY ||
	    RD_ARG_MODE(arg[0]) == MOD_NODISC) {
		beep();
		return;
	}

	arg[0] = RD_ARG_LOCK(arg[0]) ? 0 : 1;

	if (!cda_sendcmd(CDA_LOCK, arg, &retcode))
		cda_v_errhandler(retcode);
}


/*
 * cda_v_shufprog
 *	Handler function for the visual mode shuffle/program control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_shufprog(int inp, word32_t arg[])
{
	word32_t	astat0 = arg[0];
	int		i,
			j,
			retcode;
	char		inbuf[80],
			*p;

	if (!stat_on ||
	    RD_ARG_MODE(astat0) == MOD_BUSY ||
	    RD_ARG_MODE(astat0) == MOD_NODISC) {
		beep();
		return;
	}

	/* Not allowed if play or pause */
	if (RD_ARG_MODE(astat0) == MOD_PLAY ||
	    RD_ARG_MODE(astat0) == MOD_PAUSE) {
		beep();
		return;
	}

	/* See if program on */
	arg[0] = 1;
	if (!cda_sendcmd(CDA_PROGRAM, arg, &retcode)) {
		cda_v_errhandler(retcode);
		return;
	}

	/* If neither program nor shuffle, set shuffle */
	if (!RD_ARG_SHUF(astat0) && arg[0] == 0) {
		arg[0] = 1;
		if (!cda_sendcmd(CDA_SHUFFLE, arg, &retcode)) {
			cda_v_errhandler(retcode);
			return;
		}
	}
	else if (RD_ARG_SHUF(astat0)) {
		/* If shuffle, turn it off and prompt for program */
		arg[0] = 0;
		if (!cda_sendcmd(CDA_SHUFFLE, arg, &retcode)) {
			cda_v_errhandler(retcode);
			return;
		}

		(void) wmove(status_win, SHUFFLE_Y, SHUFFLE_X);
		(void) waddstr(status_win, "Shuffle");

		(void) wmove(status_win, PROGRAM_Y, PROGRAM_X);
		(void) wattron(status_win, A_STANDOUT);
		(void) waddstr(status_win, "Program");
		(void) wattroff(status_win, A_STANDOUT);

		cda_clrstatus();
		(void) wprintw(status_win, "Program: ");
		cda_v_echo(TRUE);
		(void) putp(cursor_normal);
		(void) wrefresh(status_win);

		/* Before reading the program list, check for
		 * F6 or "u", and dismiss prog mode if found
		 */
		i = cda_wgetch(status_win);
		if (i != KEY_F(6) && i != 'u') {
			cda_ungetch(i);

			cda_wgetstr(status_win, inbuf, scr_cols-12);

			/* Is the string just read was
			 * terminated by F6, it will
			 * have been ungotten, but must be
			 * thrown away or it will cause
			 * return to shuffle mode.
			 */
			if (savch == KEY_F(6))
				savch = 0;

			j = 0;
			p = inbuf;
			while ((i = strtol(p, &p, 10)) != 0) {
				arg[++j] = i;

				if (p == NULL)
					break;

				while (*p != '\0' && !isdigit((int) *p))
					p++;
			}

			arg[0] = (word32_t) -j;
			if (!cda_sendcmd(CDA_PROGRAM, arg, &retcode)) {
				emsgp = NULL;
				beep();
			}
		}

		cda_v_echo(FALSE);
		(void) putp(cursor_invisible);

		refresh_sts = TRUE;
	}
	else {
		/* Program is on - reset it */
		arg[0] = 0;
		if (!cda_sendcmd(CDA_PROGRAM, arg, &retcode)) {
			cda_v_errhandler(retcode);
			return;
		}
		refresh_sts = TRUE;
	}
}


/*
 * cda_v_repeat
 *	Handler function for the visual mode repeat control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_repeat(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on) {
		beep();
		return;
	}

	arg[0] = RD_ARG_REPT(arg[0]) ? 0 : 1;
	if (!cda_sendcmd(CDA_REPEAT, arg, &retcode))
		cda_v_errhandler(retcode);
}


/*
 * cda_v_disc
 *	Handler function for the visual mode disc change control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_v_disc(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on) {
		beep();
		return;
	}

	arg[0] = (inp == 'd') ? 2 : 3;
	if (!cda_sendcmd(CDA_DISC, arg, &retcode))
		cda_v_errhandler(retcode);

	loaddb = TRUE;
}


/*
 * cda_v_track
 *	Handler function for the visual mode track change control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_v_track(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on ||
	    RD_ARG_MODE(arg[0]) == MOD_BUSY ||
	    RD_ARG_MODE(arg[0]) == MOD_NODISC ||
	    RD_ARG_MODE(arg[0]) != MOD_PLAY) {
		beep();
		return;
	}

	arg[0] = (inp == KEY_LEFT || inp == 'C') ? 0 : 1;
	if (!cda_sendcmd(CDA_TRACK, arg, &retcode))
		cda_v_errhandler(retcode);
}


/*
 * cda_v_index
 *	Handler function for the visual mode index change control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_v_index(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on ||
	    RD_ARG_MODE(arg[0]) == MOD_BUSY ||
	    RD_ARG_MODE(arg[0]) == MOD_NODISC) {
		beep();
		return;
	}

	arg[0] = (inp == '<') ? 0 : 1;
	if (!cda_sendcmd(CDA_INDEX, arg, &retcode))
		cda_v_errhandler(retcode);
}


/*
 * cda_v_volume
 *	Handler function for the visual mode volume control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_v_volume(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on) {
		beep();
		return;
	}

	arg[0] = 0;
	if (!cda_sendcmd(CDA_VOLUME, arg, &retcode)) {
		cda_v_errhandler(retcode);
		return;
	}

	if (inp == '+') {
		if (arg[1] <= 95)
			arg[1] += 5;
		else
			arg[1] = 100;
	}
	else {
		if (arg[1] >= 5)
			arg[1] -= 5;
		else
			arg[1] = 0;
	}

	arg[0] = 1;
	if (!cda_sendcmd(CDA_VOLUME, arg, &retcode)) {
		cda_v_errhandler(retcode);
		return;
	}
}


/*
 * cda_v_balance
 *	Handler function for the visual mode balance control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_v_balance(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on) {
		beep();
		return;
	}

	arg[0] = 0;
	if (!cda_sendcmd(CDA_BALANCE, arg, &retcode)) {
		cda_v_errhandler(retcode);
		return;
	}

	if (inp == 'r' || inp == 'R') {
		if (arg[1] <= 95)
			arg[1] += 5;
		else
			arg[1] = 100;
	}
	else {
		if (arg[1] >= 5)
			arg[1] -= 5;
		else
			arg[1] = 0;
	}

	arg[0] = 1;
	if (!cda_sendcmd(CDA_BALANCE, arg, &retcode))
		cda_v_errhandler(retcode);
}


/*
 * cda_v_route
 *	Handler function for the visual mode channel routing control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_route(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on) {
		beep();
		return;
	}

	if (--route < 0)
		route = 4;

	arg[0] = 1;
	arg[1] = route;
	if (!cda_sendcmd(CDA_ROUTE, arg, &retcode))
		cda_v_errhandler(retcode);
}


/*
 * cda_v_help
 *	Handler function for the visual mode help control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_help(int inp, word32_t arg[])
{
	if (inp == '?') {
		help = TRUE;
		scroll_length = HELP_SCROLL_LENGTH;
	}
	else
		help = FALSE;
}


/*
 * cda_v_repaint
 *	Handler function for the visual mode repaint control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_repaint(int inp, word32_t arg[])
{
	wclear(info_win);
	wclear(status_win);
	oastat0 = oastat1 = (word32_t) -1;
	ostat_on = !stat_on;
	old_help = !help;
	old_route = !route;
	refresh_fkeys = TRUE;
}


/*
 * cda_v_scroll
 *	Handler function for the visual mode scroll control.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_scroll(int inp, word32_t arg[])
{
	switch (inp) {
	case KEY_UP:
	case '^':
	case '\036':	/* ctrl-^ */
		scroll_line--;
		break;
	case KEY_DOWN:
	case 'v':
	case 'V':
	case '\026':	/* ctrl-v */
		scroll_line++;
		break;
	case KEY_PPAGE:
	case '\002':	/* ctrl-b */
		scroll_line -= (scr_lines - 9);
		break;
	case '\025':	/* ctrl-u */
		scroll_line -= ((scr_lines - 9) / 2);
		break;
	case KEY_NPAGE:
	case '\006':	/* ctrl-f */
		scroll_line += (scr_lines - 9);
		break;
	case '\004':	/* ctrl-d */
		scroll_line += ((scr_lines - 9) / 2);
		break;
	default:
		return;
	}

	if (scroll_line < 0)
		scroll_line = 0;
	else if (scroll_line > (scroll_length - 1))
		scroll_line = scroll_length - 1;

	(void) prefresh(info_win, scroll_line, 0, 0, 0,
			scr_lines - 9, scr_cols - 1);
}


/*
 * cda_v_debug
 *	Handler function for the debug command - toggle debug mode.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_debug(int inp, word32_t arg[])
{
	int	retcode;

	if (!stat_on) {
		beep();
		return;
	}

	/* Query current debug mode */
	arg[0] = 0;
	if (!cda_sendcmd(CDA_DEBUG, arg, &retcode)) {
		beep();
		cda_v_errhandler(retcode);
		return;
	}

	/* Toggle debug mode */
	arg[0] = 1;
	if (arg[1] == 0)
		arg[1] = 1;
	else
		arg[1] = 0;

	if (!cda_sendcmd(CDA_DEBUG, arg, &retcode)) {
		beep();
		cda_v_errhandler(retcode);
		return;
	}

	cda_clrstatus();
	(void) wprintw(status_win, "Debug mode is now %s.",
		(arg[1] == 0) ? "off" : "on");
	(void) wrefresh(status_win);
}


/*
 * cda_v_dirtrk
 *	Handler function for the visual mode direct track access.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
STATIC void
cda_v_dirtrk(int inp, word32_t arg[])
{
	int	i,
		mins,
		secs,
		retcode;
	char	inbuf[80],
		*p;

	if (!isdigit(inp))
		return;

	if (!stat_on) {
		beep();
		return;
	}

	cda_ungetch(inp);

	cda_clrstatus();
	(void) wprintw(status_win, "Track n [mins secs] : ");
	cda_v_echo(TRUE);
	(void) putp(cursor_normal);
	(void) wrefresh(status_win);

	cda_wgetstr(status_win, inbuf, 20);
	if ((i = strtol(inbuf, &p, 10)) == 0)
		beep();
	else {
		mins = secs = 0;
		if (p != NULL) {
			while (*p != '\0' && !isdigit((int) *p))
				p++;

			mins = strtol(p, &p, 10);

			if (p != NULL) {
				while (*p != '\0' && !isdigit((int) *p))
					p++;

				secs = strtol(p, &p, 10);
			}
		}

		arg[0] = i;
		arg[1] = mins;
		arg[2] = secs;
		if (!cda_sendcmd(CDA_PLAY, arg, &retcode))
			cda_v_errhandler(retcode);
	}

	cda_v_echo(FALSE);
	(void) putp(cursor_invisible);
	refresh_sts = TRUE;
}


#ifdef KEY_RESIZE
/*
 * cda_v_resize
 *	Handler function when KEY_RESIZE is received.  This is to support
 *	ncurses library configured with its own SIGWINCH handler.
 *
 * Args:
 *	inp - command character
 *	arg - command arguments
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC void
cda_v_resize(int inp, word32_t arg[])
{
	win_resize = TRUE;
}
#endif


/***********************
 *   public routines   *
 ***********************/


/*
 * cda_vtidy
 *	Tidy up and go home for visual mode.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cda_vtidy(void)
{
	if (isvisual) {
		(void) keypad(stdscr, FALSE);
		(void) putp(cursor_normal);
		(void) endwin();
		isvisual = FALSE;
	}

	(void) printf("%s\n", emsgp != NULL ? emsgp : "Goodbye!");
}


/* Keyboard input to command function mapping table */
STATIC keytbl_t	ktbl[] = {
	{ KEY_F(1),	cda_v_on_off		},
	{ 'o',		cda_v_on_off		},
	{ 'O',		cda_v_on_off		},
	{ KEY_F(2),	cda_v_load_eject	},
	{ 'j',		cda_v_load_eject	},
	{ 'J',		cda_v_load_eject	},
	{ KEY_F(3),	cda_v_play_pause	},
	{ 'p',		cda_v_play_pause	},
	{ 'P',		cda_v_play_pause	},
	{ KEY_F(4),	cda_v_stop		},
	{ 's',		cda_v_stop		},
	{ 'S',		cda_v_stop		},
	{ KEY_F(5),	cda_v_lock		},
	{ 'k',		cda_v_lock		},
	{ 'K',		cda_v_lock		},
	{ KEY_F(6),	cda_v_shufprog		},
	{ 'u',		cda_v_shufprog		},
	{ 'U',		cda_v_shufprog		},
	{ KEY_F(7),	cda_v_repeat		},
	{ 'e',		cda_v_repeat		},
	{ 'E',		cda_v_repeat		},
	{ 'd',		cda_v_disc		},
	{ 'D',		cda_v_disc		},
	{ KEY_LEFT,	cda_v_track		},
	{ KEY_RIGHT,	cda_v_track		},
	{ 'C',		cda_v_track		},
	{ 'c',		cda_v_track		},
	{ '<',		cda_v_index		},
	{ '>',		cda_v_index		},
	{ '+',		cda_v_volume		},
	{ '-',		cda_v_volume		},
	{ 'l',		cda_v_balance		},
	{ 'L',		cda_v_balance		},
	{ 'r',		cda_v_balance		},
	{ 'R',		cda_v_balance		},
	{ '\011',	cda_v_route		},
	{ '?',		cda_v_help		},
	{ ' ',		cda_v_help		},
	{ '\014',	cda_v_repaint		},
	{ '\022',	cda_v_repaint		},
	{ KEY_UP,	cda_v_scroll		},
	{ KEY_DOWN,	cda_v_scroll		},
	{ '^',		cda_v_scroll		},
	{ 'v',		cda_v_scroll		},
	{ '\036',	cda_v_scroll		},
	{ '\026',	cda_v_scroll		},
	{ 'V',		cda_v_scroll		},
	{ KEY_PPAGE,	cda_v_scroll		},
	{ KEY_NPAGE,	cda_v_scroll		},
	{ '\002',	cda_v_scroll		},
	{ '\025',	cda_v_scroll		},
	{ '\006',	cda_v_scroll		},
	{ '\004',	cda_v_scroll		},
	{ 'd',		cda_v_debug		},
	{ '\0',		cda_v_dirtrk		},
#ifdef KEY_RESIZE
	{ KEY_RESIZE,	cda_v_resize		},
#endif
	{ '\0',		NULL			}
};


/*
 * cda_visual
 *	Visual (curses mode) interface.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
cda_visual(void)
{
	word32_t	arg[CDA_NARGS];
	int		inp,
			i,
			retcode;
	curstat_t	*s = curstat_addr();

	/* Set up device list */
	cda_parse_devlist(s);

	/* Initialize curses */
	cda_makewins();

	/* Paint the screen every second */
	cda_screen(0);

	/* Main processing loop */
	for (;;) {
		inp = cda_wgetch(status_win);

		if (inp == KEY_F(8) || inp == 'q' || inp == 'Q')
			break;

		/* Cancel alarm so we don't nest */
		(void) alarm(0);

		/* Get current status */
		if (stat_on && !cda_sendcmd(CDA_STATUS, arg, &retcode))
			cda_v_errhandler(retcode);

		/* Execute command function */
		for (i = 0; ktbl[i].keyfunc != NULL; i++) {
			if (ktbl[i].key == inp || ktbl[i].key == '\0') {
				(*ktbl[i].keyfunc)(inp, arg);
				break;
			}
		}

		/* Repaint screen */
		cda_screen(0);
	}

	/* Tidy up and go home */
	(void) alarm(0);
	emsgp = NULL;

	if (cda_daemon_alive()) {
		(void) sprintf(errmsg, "CD audio daemon pid=%d still running.",
			       (int) daemon_pid);
		emsgp = errmsg;
	}

	exit_status = 0;
	cda_quit(s);
	/*NOTREACHED*/
}


#endif	/* NOVISUAL */

