/*
 * @(#)gobrowser.c	7.29 04/03/18
 * 
 * Usage: gobrowser url_string
 *
 *	This program will attempt to remote control a running web browser
 *	session on the user's display to the specified URL.  If none is
 *	found, it will then search for a supported browser executable on
 *	the system, and start a new instance of Netscape to go to the URL.
 *
 *	This version of gobrowser supports the Netscape, Mozilla, Galeon
 *	and Opera web browsers.
 *
 *	If compiled with -DDO_POPUP, when spawning a new browser, a dialog
 *	box is popped up to inform the user that the browser is being started.
 *
 *	If the BROWSER_PATH environment is set and points to a browser
 *	executable, it will be used instead of searching the system.
 *	This is useful if there are multiple browsers are installed
 *	and the user wants to use a specific version, or if the browser
 *	executable is installed in an unusual location or has a non-
 *	standard name.
 *
 *	If the GBDEBUG environment variable is set to a non-zero value,
 *	this program will output verbose diagnostics.
 *
 * Compile:
 *	cc -O -o gobrowser gobrowser.c -lX11
 *	cc -DDO_POPUP -O -o gobrowser gobrowser.c -lXm -lXt -lXext -lX11
 *	X11R6 systems also require -lSM -lICE
 *	On SVR4 systems (Solaris/UnixWare) add -lsocket -lnsl
 *
 * Copyright (C) 1993-2004  Ti Kan
 * Email: ti@amb.org
 *
 * Portions of this code was adapted from 'remote.c' supplied by Netscape:
 * Copyright (C) 1996 Netscape Communications Corporation, all rights reserved.
 * Created: Jamie Zawinski <jwz@netscape.com>, 24-Dec-94.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include <stdio.h>
#ifndef NO_STDLIB_H
#include <stdlib.h>
#endif
#ifndef NO_UNISTD_H
#include <unistd.h>
#endif
#ifdef BSDCOMPAT
#include <strings.h>
#ifndef strchr
#define strchr			index
#endif
#ifndef strrchr
#define strrchr			rindex
#endif
#else
#include <string.h>
#endif
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef DO_POPUP
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/MessageB.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xatom.h>


#ifdef sony_news
typedef int			pid_t;
typedef int			mode_t;
#define WAITPID(pid, svp, op)	wait4(pid, svp, 0, 0)
#else
#define WAITPID			waitpid
#endif  /* sony_news */

#if defined(macII) || defined(sony_news)
#ifndef WEXITSTATUS
#define WEXITSTATUS(x)	(((x.w_status) >> 8) & 0377)
#endif
#ifndef WTERMSIG
#define WTERMSIG(x)	((x.w_status) & 0177)
#endif
typedef union wait	waitret_t;
#else   /* macII sony_news */
typedef int		waitret_t;
#endif  /* macII sony_news */

#define EXE_ERR			255
#define CMD_TIMEOUT_SECS	60


/* Executable names to match */
char	*browser_names[] = {
	"netscape",
	"Netscape",
	"netscape_communicator",
	"netscape_navigator",
	"netscape-communicator",
	"netscape-navigator",
	"communicator",
	"navigator",
	"mozilla",
	"Mozilla",
	"mozilla-viewer.sh",
	"run-mozilla.sh",
	"galeon",
	"opera",
	NULL
};


/* Directories to look for netscape executable
 * In addition to these, gobrowser will also walk the user's PATH
 * environment looking there too.
 */
char	*search_paths[] = {
	"/usr/bin/X11",
	"/usr/X/bin",
	"/usr/X11/bin",
	"/usr/X11R6/bin",
	"/usr/X11R5/bin",
	"/usr/X386/bin",
	"/usr/openwin/bin",
	"/usr/local/bin/X11",
	"/usr/local/bin",
	"/usr/lbin",
	"/usr/local/netscape",
	"/usr/freeware/bin/X11",
	"/usr/freeware/bin",
	"/opt/netscape/bin",
	"/opt/netscape",
	"/opt/netscape6",
	"/opt/netscape4",
	"/opt/communicator/bin",
	"/opt/communicator",
	"/usr/local/mozilla",
	"/opt/mozilla/bin",
	"/opt/mozilla",
	"/usr/local/src/mozilla",
	NULL
};


#define NS_VERSION_PROP		"_MOZILLA_VERSION"
#define NS_LOCK_PROP		"_MOZILLA_LOCK"
#define NS_COMMAND_PROP		"_MOZILLA_COMMAND"
#define NS_RESPONSE_PROP	"_MOZILLA_RESPONSE"


/* Global data */
Atom	xa_nsver;
Atom	xa_nslock;
Atom	xa_nscmd;
Atom	xa_nsresp;
char	*lock_data;
int	exit_stat;
int	gbdebug;


/* Defines used by run_cmd */
#define STR_SHPATH	"/bin/sh"	/* Path to shell */
#define STR_SHNAME	"sh"		/* Name of shell */
#define STR_SHARG	"-c"		/* Shell arg */


#ifdef DO_POPUP

#define WM_FUDGE_X	10		/* Window manager decoration */
#define WM_FUDGE_Y	25		/* fudge factors */

/* Some resources for the popup dialog box */
char	*resources[] = {
	"GObrowser*foreground: black",
	"GObrowser*background: grey65",
	"GObrowser*shadowThickness: 2",
	NULL
};

#endif	/* DO_POPUP */


/*
 * onalrm
 *	Signal handler for SIGALRM.  This is normally to break a
 *	thread from a blocked system call and no more.
 *
 * Args:
 *	signo - The signal number
 *
 * Return:
 *	Nothing.
 */
void
onalrm(int signo)
{
	(void) signal(signo, onalrm);
}


/*
 * path_basename
 *	Return the basename of a file path.  Similar to basename(1).
 *
 * Args:
 *	path - input file path string
 *
 * Return:
 *	Pointer to the beginning of the base name portion of the input
 *	path string, or the null string if an error occurred.
 */
char *
path_basename(char *path)
{
	char	*cp;

	if (path == NULL || *path == '\0')
		return ("");		/* Input error */

	if ((cp = strrchr(path, '/')) != NULL)
		return (cp + 1);	/* Found basename */

	return (path);			/* Relative path name */
}


/*
 * run_cmd
 *	Spawn a shell child and execute a command
 *
 * Args:
 *	cmd - The shell command string
 *
 * Return:
 *	The exit status of the command
 */
int
run_cmd(char *cmd)
{
	int		i,
			ret,
			saverr = 0,
			fd;
	unsigned int	oa;
	pid_t		cpid;
	waitret_t	wstat;
	void		(*oh)(int);

	/* Fork child to invoke external command */
	switch (cpid = fork()) {
	case 0:
		break;

	case -1:
		/* Fork failed */
		perror("run_cmd: fork() failed");
		return EXE_ERR;

	default:
		/* Parent process: wait for child to exit */
		oh = signal(SIGALRM, onalrm);
		oa = alarm(1);
		for (i = 0; (ret = WAITPID(cpid, &wstat, 0)) != cpid; i++) {
			(void) alarm(0);

			if (ret < 0 && errno != EINTR) {
				saverr = errno;
				break;
			}

			if (i >= CMD_TIMEOUT_SECS) {
				saverr = errno;
				(void) fprintf(stderr,
					"run_cmd: Timed out waiting for "
					"command to exit (pid=%d)\n",
					(int) cpid
				);
				break;
			}

			(void) alarm(1);
		}
		(void) alarm(oa);
		(void) signal(SIGALRM, oh);

		if (ret < 0)
			ret = (saverr << 8);
		else if (WIFEXITED(wstat))
			ret = WEXITSTATUS(wstat);
		else
			ret = EXE_ERR;
		return (ret);
	}

	/* Child process */
	for (fd = 3; fd < 255; fd++) {
		/* Close unneeded file descriptors */
		(void) close(fd);
	}

	/* Exec a shell to run the command */
	(void) execl(STR_SHPATH, STR_SHNAME, STR_SHARG, cmd, NULL);
	_exit(EXE_ERR);
	/*NOTREACHED*/
}


#ifdef DO_POPUP

/*
 * do_exit
 *	Callback function for the OK button: Just exit
 */
/*ARGSUSED*/
void
do_exit(Widget w, XtPointer client_data, XtPointer call_data)
{
	exit(exit_stat);
}


/*
 * popup_dialog
 *	Pop up a message dialog box with the specified string.  It will
 *	auto pop-down in 5 seconds, or if the user clicks the OK button.
 *
 * Args:
 *	str - The text string to display
 *	argc - Pointer to the command line argument count
 *	argv - Pointer to the command line arg array
 *
 * Return:
 *	Nothing
 */
void
popup_dialog(char *str, int *argc, char **argv)
{
	XtAppContext		app_context;
	XtWidgetGeometry	geom;
	Display			*dpy;
	Window			rootwin,
				childwin;
	Widget			toplevel,
				w;
	XmString		xs;
	Arg			arg[10];
	int			i,
				screen,
				ptr_x,
				ptr_y,
				win_x,
				win_y,
				swidth,
				sheight,
				junk;
	unsigned int		keybtn;

#ifdef USE_SGI_DESKTOP
	SgiUseSchemes("all");
#endif

	toplevel = XtVaAppInitialize(
		&app_context,
		"GObrowser",
		NULL, 0,
		argc, argv,
		resources,
		NULL
	);

	xs = XmStringCreateLtoR(str, XmSTRING_DEFAULT_CHARSET);

	/* Create message box dialog box */
	i = 0;
	XtSetArg(arg[i], XmNmessageString, xs); i++;
	XtSetArg(arg[i], XmNdialogType, XmDIALOG_WORKING); i++;
	w = XmCreateMessageBox(toplevel, str, arg, i);
	XtManageChild(w);

	XtUnmanageChild(XmMessageBoxGetChild(w, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(w, XmDIALOG_HELP_BUTTON));
	XtAddCallback(XmMessageBoxGetChild(w, XmDIALOG_OK_BUTTON),
		      XmNactivateCallback,
		      (XtCallbackProc) do_exit, (XtPointer) NULL);

	XmStringFree(xs);

	XtRealizeWidget(toplevel);

	/* Position dialog box under mouse cursor */
	dpy = XtDisplay(toplevel);
	screen = DefaultScreen(dpy);
	swidth = XDisplayWidth(dpy, screen);
	sheight = XDisplayHeight(dpy, screen);
	
	/* Get current pointer location */
	XQueryPointer(dpy, XtWindow(toplevel), &rootwin, &childwin,
		      &ptr_x, &ptr_y, &junk, &junk, &keybtn);

	/* Get dialog window dimensions */
	(void) XtQueryGeometry(w, NULL, &geom);

	win_x = ptr_x - (geom.width / 2);
	if (win_x < 0)
		win_x = 0;
	else if ((int) (win_x + geom.width + WM_FUDGE_X) > swidth)
		win_x = swidth - geom.width - WM_FUDGE_X;

	win_y = ptr_y - (geom.height / 2);
	if (win_y < 0)
		win_y = 0;
	else if ((int) (win_y + geom.height + WM_FUDGE_Y) > sheight)
		win_y = sheight - geom.height - WM_FUDGE_Y;

	XtVaSetValues(w, XmNx, win_x, XmNy, win_y, NULL);

	/* Make dialog auto-popdown in 5 seconds */
	XtAppAddTimeOut(app_context, 5000,
			(XtTimerCallbackProc) do_exit, (XtPointer) NULL);

	/* Handle X events */
	XtAppMainLoop(app_context);
}

#endif	/* DO_POPUP */


/*
 * clientwin_r
 *	Traverse a window hierarchy looking for a client's
 *	top-level window, and return its window ID.  Used by
 *	clientwin() to descend the window tree.
 *
 * Args:
 *	dpy - The X display
 *	clwin - The top level client window ID
 *	wm_state - property atom
 *
 * Return:
 *	The window ID, or 0 if not found.
 */
Window
clientwin_r(Display *dpy, Window clwin, Atom wm_state)
{
	Window		root,
			parent,
			*ch;
	Atom		type = None;
	int		i,
			fmt;
	unsigned int	nch;
	unsigned long	n,
			resid;
	unsigned char	*data;
	Window		retwin;

	if (!XQueryTree(dpy, clwin, &root, &parent, &ch, &nch) ||
	    ch == NULL || nch == 0)
		return (Window) 0;

	for (retwin = 0, i = 0; i < (int) nch; i++) {
		if (XGetWindowProperty(dpy, ch[i], wm_state, 0, 0,
				   False, AnyPropertyType, &type, &fmt,
				   &n, &resid, &data) != Success || type == 0)
			continue;

		retwin = ch[i];

		if (retwin != 0)
			break;
	}

	for (i = 0; retwin == 0 && i < (int) nch; i++) {
		if ((retwin = clientwin_r(dpy, ch[i], wm_state)) != 0)
			break;
	}

	if (ch != NULL)
		XFree((char *) ch);

	return (retwin);
}


/*
 * clientwin
 *	Find a top-level client window within the window hierarchy
 *	whose root is clwin.
 *
 * Args:
 *	dpy - The X display
 *	clwin - The top level client window ID
 *
 * Return
 *	The window ID, or 0 if not found.
 */
Window
clientwin(Display *dpy, Window clwin)
{
	Atom		wm_state,
			type;
	int		fmt;
	unsigned long	n,
			resid;
	unsigned char	*data;
	Window		retwin;

	/* ICCCM says that windows with the WM_STATE property are
	 * top level windows.
	 */
	if ((wm_state = XInternAtom(dpy, "WM_STATE", True)) == 0)
		return (clwin);

	if (XGetWindowProperty(dpy, clwin, wm_state,
			0, 0, False, AnyPropertyType, &type,
			&fmt, &n, &resid, &data) != Success || type != 0)
		/* clwin is the top-level client window */
		return (clwin);

	/* Traverse clwin's children */
	if ((retwin = clientwin_r(dpy, clwin, wm_state)) == 0)
		retwin = clwin;

	return (retwin);
}


/*
 * ns_findwin
 *	Locate a netscape top level window running on the specified display.
 *
 * Args:
 *	progname - The name of this program
 *	dpy - The X display
 *
 * Return:
 *	The window ID, or 0 if not found.
 */
Window
ns_findwin(char *progname, Display *dpy)
{
	int		i;
	Window		root = RootWindowOfScreen(DefaultScreenOfDisplay(dpy)),
			root2,
			parent,
			*kids;
	unsigned int	nkids;
	Window		win = 0;

	if (!XQueryTree(dpy, root, &root2, &parent, &kids, &nkids)) {
		(void) fprintf(stderr,
			"%s: XQueryTree failed on display %s\n",
			progname,
			DisplayString(dpy)
		);
		return (Window) 0;
	}

	for (i = nkids - 1; i >= 0; i--) {
		Atom		type;
		int		format;
		unsigned long	nitems,
				bytesafter;
		unsigned char	*version = 0;
		Window		w;
		int		status;

		w = clientwin(dpy, kids[i]);

		status = XGetWindowProperty(
			dpy, w, xa_nsver,
			0, (65536 / sizeof (long)),
			False, XA_STRING,
			&type, &format, &nitems, &bytesafter,
			&version
		);

		if (version == NULL)
			continue;

		if (status == Success && type != None) {
			win = w;
			break;
		}
	}

	return (win);
}


/*
 * ns_getlock
 *	Lock the netscape window for exclusive remote control
 *
 * Args:
 *	progname - The name of this program
 *	dpy - The X display
 *	win - The window ID of the netscape window
 *
 * Return:
 *	0 - failure
 *	1 - success
 */
int
ns_getlock(char *progname, Display *dpy, Window win)
{
	int	locked = 0,
		waited = 0;

	XSelectInput(dpy, win, PropertyChangeMask | StructureNotifyMask);

	if ((lock_data = (char *) malloc(255)) == NULL)
		return 0;
	(void) sprintf(lock_data, "pid%d@", (int) getpid());
	if (gethostname(lock_data + strlen(lock_data), 100) < 0) {
		perror("gethostname");
		return 0;
	}

	do {
		int		result;
		Atom		actual_type;
		int		actual_format;
		unsigned long	nitems,
				bytes_after;
		unsigned char	*data = NULL;

		XGrabServer(dpy);

		result = XGetWindowProperty(
			dpy, win, xa_nslock,
			0, (65536 / sizeof (long)),
			False, /* don't delete */
			XA_STRING,
			&actual_type, &actual_format,
			&nitems, &bytes_after,
			&data
		);
		if (result != Success || actual_type == None) {
			/* It's not now locked - lock it. */
			XChangeProperty(
				dpy, win, xa_nslock, XA_STRING, 8,
				PropModeReplace, (unsigned char *) lock_data,
				strlen(lock_data)
			);
			locked = 1;
		}

		XUngrabServer(dpy);
		XSync(dpy, False);

		if (!locked) {
			/* We tried to grab the lock this time, and failed
			 * because someone else is holding it already.
			 * So, wait for a PropertyDelete event to come in,
			 * and try again.
			 */
			(void) fprintf(stderr,
				"%s: window 0x%x is locked by %s; waiting...\n",
				progname, (unsigned int) win, data);
			waited = 1;

			for (;;) {
				XEvent	event;

				XNextEvent(dpy, &event);

				if (event.xany.type == DestroyNotify &&
				    event.xdestroywindow.window == win) {
					(void) fprintf(stderr,
					"%s: window 0x%x destroyed.\n",
					progname, (unsigned int) win);
					return 0;
				}
				else if (event.xany.type == PropertyNotify &&
				    event.xproperty.state == PropertyDelete &&
				    event.xproperty.window == win &&
				    event.xproperty.atom == xa_nslock) {
					/* Someone deleted their lock,
					 * so now we can try again.
					 */
					break;
				}
			}
		}
		if (data != NULL)
			XFree(data);
	}  while (!locked);

	if (waited)
		(void) fprintf(stderr, "%s: obtained lock.\n", progname);

	return 1;
}


/*
 * ns_freelock
 *	Unlock the netscape window
 *
 * Args:
 *	progname - The name of this program
 *	dpy - The X display
 *	win - The netscape window
 *
 * Return:
 *	Nothing
 */
void
ns_freelock(char *progname, Display *dpy, Window win)
{
	int		result;
	Atom		actual_type;
	int		actual_format;
	unsigned long	nitems,
			bytes_after;
	unsigned char	*data = NULL;

	result = XGetWindowProperty(
		dpy, win, xa_nslock,
		0, (65536 / sizeof (long)),
		True, /* atomic delete after */
		XA_STRING,
		&actual_type, &actual_format,
		&nitems, &bytes_after,
		&data
	);
	if (result != Success) {
		(void) fprintf(stderr,
			"%s: unable to read and delete "
			NS_LOCK_PROP
			" property\n",
			progname
		);
		return;
	}
	else if (data == NULL || *data == '\0') {
		(void) fprintf(stderr,
			"%s: invalid data on " NS_LOCK_PROP
			" of window 0x%x.\n", progname, (unsigned int) win);
		return;
	}
	else if (strcmp ((char *) data, lock_data)) {
		(void) fprintf(stderr, "%s: " NS_LOCK_PROP
			" was stolen!  Expected \"%s\", saw \"%s\"!\n",
			progname, lock_data, data
		);
		return;
	}

	if (data != NULL)
		XFree(data);
}


/*
 * ns_rmtcmd
 *	Send a command to a specified netscape window
 *
 * Args:
 *	progname - The name of this program
 *	dpy - The X display
 *	win - The netscape window
 *	cmd - The command string
 *
 * Return:
 *	0 - failure
 *	1 - success
 */
int
ns_rmtcmd(char *progname, Display *dpy, Window win, char *cmd)
{
	int	status = 0,
		done = 0;

	if (!ns_getlock(progname, dpy, win))
		return 0;

	XChangeProperty(dpy, win, xa_nscmd, XA_STRING, 8,
			PropModeReplace, (unsigned char *) cmd,
			strlen(cmd));

	while (!done) {
		XEvent	event;

		XNextEvent(dpy, &event);
		if (event.xany.type == DestroyNotify &&
		    event.xdestroywindow.window == win) {
			/* Print to warn user...*/
			fprintf (stderr, "%s: window 0x%x was destroyed.\n",
			    progname, (unsigned int) win);
			status = 6;
			done = True;
		}
		else if (event.xany.type == PropertyNotify &&
			 event.xproperty.state == PropertyNewValue &&
			 event.xproperty.window == win &&
			 event.xproperty.atom == xa_nsresp) {
			Atom		actual_type;
			int		actual_format;
			unsigned long	nitems,
					bytes_after;
			unsigned char	*data = NULL;

			status = XGetWindowProperty(
				dpy, win, xa_nsresp,
				0, (65536 / sizeof (long)),
				True, /* atomic delete after */
				XA_STRING,
				&actual_type, &actual_format,
				&nitems, &bytes_after,
				&data
			);

			if (status != Success) {
				(void) fprintf(stderr,
					"%s: failed reading "
					NS_RESPONSE_PROP
					" from window 0x%0x.\n",
					progname, (unsigned int) win
				);
				status = 6;
				done = True;
			}
			else if (data == NULL || strlen((char *) data) < 5) {
				(void) fprintf(stderr,
					"%s: invalid data on "
					NS_RESPONSE_PROP
					" property of window 0x%0x.\n",
					progname, (unsigned int) win
				);
				status = 6;
				done = True;
			}
			else if (*data == '1') {
				/* Positive preliminary reply */
				(void) fprintf(stderr,
					"%s: %s\n", progname, data + 4);
				/* keep going */
				done = False;
			}
			else if (strncmp((char *)data, "200", 3) == 0) {
				/* Positive completion */
				status = 0;
				done = True;
			}
			else if (*data == '2') {
				/* positive completion */
				(void) fprintf(stderr,
					"%s: %s\n", progname, data + 4);
				status = 0;
				done = True;
			}
			else if (*data == '3') {
				/* Positive intermediate reply */
				(void) fprintf(stderr, "%s: internal error: "
				    "server wants more information? (%s)\n",
				    progname, data);
				status = 3;
				done = True;
			}
			else if (*data == '4' || *data == '5') {
				/* transient/permanent negative completion */
				(void) fprintf(stderr,
					"%s: %s\n", progname, data + 4);
				status = (*data - '0');
				done = True;
			}
			else {
				(void) fprintf(stderr,
					"%s: unrecognised "
					NS_RESPONSE_PROP
					" from window 0x%x: %s\n",
					progname, (unsigned int) win, data
				);
				status = 6;
				done = True;
			}

			if (data != NULL)
				XFree(data);
		}
	}

	/* When status = 6, it means the window has been destroyed
	 * It is invalid to free the lock when window is destroyed.
	 */
	if (status != 6)
		ns_freelock(progname, dpy, win);

	return ((int) (status == 0));
}


/*
 * ns_sendcmd
 *	Locate a running netscape window and send a command to it.
 *
 * Args:
 *	progname - The name of this program
 *	dispstr - The DISPLAY environment string
 *	cmd - The command string to send
 *
 * Return:
 *	0 - failure
 *	1 - success
 */
int
ns_sendcmd(char *progname, char *dispstr, char *cmd)
{
	int		ret;
	Display		*dpy;
	Window		win;

	if ((dpy = XOpenDisplay(dispstr)) == NULL)
		return 0;

	xa_nsver = XInternAtom(dpy, NS_VERSION_PROP, False);
	xa_nslock = XInternAtom(dpy, NS_LOCK_PROP, False);
	xa_nscmd = XInternAtom(dpy, NS_COMMAND_PROP, False);
	xa_nsresp = XInternAtom(dpy, NS_RESPONSE_PROP, False);

	if ((win = ns_findwin(progname, dpy)) == 0) {
		XCloseDisplay(dpy);
		return 0;
	}

	ret = ns_rmtcmd(progname, dpy, win, cmd);
	XCloseDisplay(dpy);
	return (ret);
}


/*
 * locate_browser
 *	If BROWSER_PATH environment is set, check to see if it's executable.
 *	Otherwise, search the system for a supported browser.
 *
 * Args:
 *	progname - The name of this program
 *
 * Return
 *	The path to the found executable, or NULL if none found.
 */
char *
locate_browser(char *progname)
{
	int		i,
			j;
	char		*path_env,
			*cp,
			*p,
			*q;
	struct stat	stbuf;
	static char	file[1024];

	/* Get PATH environment */
	path_env = getenv("PATH");

	/* Locate the browser executable */
	if ((cp = getenv("BROWSER_PATH")) != NULL) {
		if (gbdebug) {
			(void) fprintf(stderr,
				"BROWSER_PATH is set to %s\n", cp);
		}

		/* Check executable */
		if (stat(cp, &stbuf) < 0) {
			(void) fprintf(stderr, "%s Error: %s: %s\n",
				progname, "Cannot stat browser executable",
				cp
			);
			return NULL;
		}
		if (!S_ISREG(stbuf.st_mode) || !(stbuf.st_mode & S_IXOTH)) {
			(void) fprintf(stderr, "%s Error: %s: %s\n",
				progname, "browser is not executable",
				cp
			);
			return NULL;
		}
		return (cp);
	}

	for (i = 0; browser_names[i] != NULL; i++) {
		for (j = 0; search_paths[j] != NULL; j++) {
			(void) sprintf(file, "%s/%s",
				       search_paths[j], browser_names[i]);

			if (gbdebug) {
				(void) fprintf(stderr,
					"Checking for %s\n", file);
			}

			if (stat(file, &stbuf) < 0)
				continue;

			if (S_ISREG(stbuf.st_mode) &&
			    (stbuf.st_mode & S_IXOTH)) {
				if (gbdebug) {
					(void) fprintf(stderr,
						"Located %s\n", file);
				}
				return (file);
			}
		}

		if (path_env == NULL)
			continue;

		p = q = path_env;
		for (p = path_env; ; p = q + 1) {
			if ((q = strchr(p, ':')) != NULL)
				*q = '\0';

			(void) sprintf(file, "%s/%s", p, browser_names[i]);

			if (gbdebug) {
				(void) fprintf(stderr,
					"Checking for %s\n", file);
			}

			if (stat(file, &stbuf) < 0) {
				if (q != NULL) {
					*q = ':';
					continue;
				}
				else
					break;
			}

			if (S_ISREG(stbuf.st_mode) &&
			    (stbuf.st_mode & S_IXOTH)) {
				if (q != NULL)
					*q = ':';

				if (gbdebug) {
					(void) fprintf(stderr,
						"Located %s\n", file);
				}
				return (file);
			}

			if (q != NULL)
				*q = ':';
			else
				break;
		}
	}
	return NULL;
}


/*
 * usage
 *	Display command line usage help
 *
 * Args:
 *	progname - The name of this program
 *
 * Return:
 *	Nothing
 */
void
usage(char *progname)
{
	fprintf(stderr, "Usage: %s url_string\n", progname);
}


/*
 * main
 *	The main function
 */
int
main(int argc, char **argv)
{
	char	*dbgenv,
		*browser_path,
		*dispstr,
		*cmd;

	/* Check command line */
	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}

	/* Set debug flag if specified */
	if ((dbgenv = getenv("GBDEBUG")) != NULL)
		gbdebug = atoi(dbgenv);

	/* Get DISPLAY environment */
	if ((dispstr = getenv("DISPLAY")) == NULL) {
		(void) fprintf(stderr, "%s Error: %s.\n",
		    argv[0],
		    "The DISPLAY environment variable is not set"
		);
		exit(1);
	}

	if ((cmd = (char *) malloc(strlen(argv[1]) + 2048)) == NULL) {
		(void) fprintf(stderr, "%s Error: Out of memory.\n", argv[0]);
		exit(1);
	}

	if ((browser_path = locate_browser(argv[0])) == NULL) {
		(void) fprintf(stderr, "%s Error: %s\n%s\n%s\n",
		    argv[0],
		    "Cannot locate a supported browser.",
		    "To fix: Set the BROWSER_PATH environment variable to the",
		    "web browser executable file path."
		);
		free(cmd);
		exit(1);
	}

	if (strcmp(path_basename(browser_path), "opera") == 0) {
		/* Special handling for the opera browser:
		 * A single command is used to remote control an existing
		 * opera browser or start a new instance.
		 */
		(void) sprintf(cmd,
			"exec %s -remote 'openURL(%s)' >/dev/null 2>&1 &",
			browser_path, argv[1]);
	}
	else {
		/* Try remote controlling a running Netscape or Mozilla
		 * session.
		 */
		(void) sprintf(cmd, "openURL(%s)", argv[1]);
		if (ns_sendcmd(argv[0], dispstr, cmd)) {
			/* Success: just exit */
			free(cmd);

			if (gbdebug) {
				(void) fprintf(stderr,
					"Remote control of running browser "
					"successful.\n");
			}
			exit(0);
		}

		/* Start a new instance of the browser */
		(void) sprintf(cmd,
			"exec %s '%s' >/dev/null 2>&1 &",
			browser_path, argv[1]);
	}

	if (gbdebug)
		(void) fprintf(stderr, "Starting browser: %s\n", browser_path);

	exit_stat = run_cmd(cmd);
	free(cmd);

#ifdef DO_POPUP
	if (exit_stat == 0) {
		char	*cp;

		cp = (char *) malloc(strlen(browser_path) + 80);
		if (cp == NULL) {
			(void) fprintf(stderr, "%s Error: Out of memory.\n",
				       argv[0]);
		}
		else {
			(void) sprintf(cp,
			    "Starting web browser:\n( %s )\n\nPlease wait...",
			    browser_path
			);

			/* Pop up a dialog box */
			popup_dialog(cp, &argc, argv);

			free(cp);
		}
	}
#endif

	exit(exit_stat);
	/*NOTREACHED*/
}

