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
#ifndef lint
static char *_main_c_ident_ = "@(#)main.c	7.36 04/03/17";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "xmcd_d/xmcd.h"
#include "xmcd_d/resource.h"
#include "xmcd_d/widget.h"
#include "xmcd_d/callback.h"
#include "xmcd_d/cdfunc.h"
#include "xmcd_d/command.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"
#include "xmcd_d/hotkey.h"

#define STARTUP_CMD_DELAY	1500	/* Delay interval before running
					 * startup command
					 */

/* Global data */
char			*progname,	/* The path name we are invoked with */
			*cdisplay;	/* Command-line specified display */
appdata_t		app_data;	/* Options data */
widgets_t		widgets;	/* Holder of all widgets */
pixmaps_t		pixmaps;	/* Holder of all pixmaps */
FILE			*errfp;		/* Error message stream */

/* Data global to this module only */
STATIC curstat_t	status;		/* Current CD player status */
STATIC XtAppContext	app_context;	/* Application context */
STATIC Colormap		localcmap;	/* local colormap */
STATIC char		*cmd;		/* Command string */


/***********************
 *   public routines   *
 ***********************/

/*
 * curstat_addr
 *	Return the address of the curstat_t structure.
 *
 * Args:
 *	Nothing.
 *
 * Return:
 *	Nothing.
 */
curstat_t *
curstat_addr(void)
{
	return (&status);
}


/*
 * event_loop
 *	Used to handle X events while waiting on I/O.
 *
 * Args:
 *	flag - Currently unused
 *
 * Return:
 *	Nothing.
 */
/* ARGSUSED */
void
event_loop(int flag)
{
	while (XtAppPending(app_context))
		XtAppProcessEvent(app_context, XtIMAll);
}


/*
 * shutdown_gui
 *	Perform window system shutdown cleanup.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Nothing.
 */
void
shutdown_gui(void)
{
	if (app_data.instcmap)
		XFreeColormap(XtDisplay(widgets.toplevel), localcmap);
}


/***********************
 *  internal routines  *
 ***********************/


/*
 * usage
 *	Display command line usage syntax
 *
 * Args:
 *	arg - The command line arg string that is invalid
 *
 * Return:
 *	Nothing.
 */
STATIC void
usage(char *arg)
{
	(void) fprintf(errfp,
		"%s %s.%s.%s Motif(R) CD player/ripper\n%s\n%s\n\n",
		PROGNAME, VERSION_MAJ, VERSION_MIN, VERSION_TEENY,
		COPYRIGHT, XMCD_URL
	);

	if (strcmp(arg, "-help") != 0)
		(void) fprintf(errfp, "%s: %s\n\n", app_data.str_badopts, arg);

	(void) fprintf(errfp, "Usage: %s %s %s %s\n            %s %s %s",
		PROGNAME,
		"[-dev device]",
		"[-instcmap]",
		"[-remote]",
		"[-rmthost hostname]",
		"[-debug level#]",
		"[-help]"
	);

#ifdef _SOLARIS
	/* Solaris volume manager auto-start support */
	(void) fprintf(errfp, "\n            [-c device] [-X] [-o]");
#endif

 	(void) fprintf(errfp, "\n            [command [arg ...]]\n\n");

	/* Display command usage */
	cmd_usage();

 	(void) fprintf(errfp,
	    "\nStandard Xt Intrinsics toolkit options are also supported.\n");
}


/*
 * chk_resource_ver
 *	Look for XMcd.version in the user's private X resource files
 *	and check if there is a mismatch to this version of xmcd.
 *	Display error message if mismatch found.
 *
 * Args:
 *	dpy - The X display.
 *
 * Return:
 *	FALSE - Mismatch found, or other error.
 *	TRUE  - No mismatch found.
 */
STATIC bool_t
chk_resource_ver(Display *dpy)
{
#ifdef __VMS
	return TRUE;
#else
	int		pfd[2],
			ret;
	unsigned int	ver,
			vmaj,
			vmin;
	pid_t		cpid;
	waitret_t	wstat;
	bool_t		chkok = FALSE;
	FILE		*fp;
	char		*cp,
			*path,
			buf[STR_BUF_SZ * 4];

	if (PIPE(pfd) < 0) {
		perror("chk_resource_ver: pipe failed");
		return FALSE;
	}

	if ((cp = getenv("XUSERFILESEARCHPATH")) == NULL)
		return TRUE;	/* Cannot check, just hope it's ok */

	path = XtResolvePathname(dpy, NULL, "XMcd", NULL, cp, NULL, 0, NULL);
	if (path == NULL || path[0] == '\0')
		return TRUE;	/* Cannot check, just hope it's ok */

	switch (cpid = FORK()) {
	case -1:
		/* Fork failed */
		perror("chk_resource_ver: fork failed");
		return FALSE;
	case 0:
		/* Child */
		(void) util_signal(SIGPIPE, SIG_IGN);
		(void) close(pfd[0]);
		break;
	default:
		/* Parent */
		(void) close(pfd[1]);

		(void) read(pfd[0], &ver, sizeof(ver));
		(void) close(pfd[0]);

		ret = util_waitchild(cpid, 5, NULL, 0, FALSE, &wstat);

		if (ret < 0)
			chkok = FALSE;
		else if (WIFEXITED(wstat))
			chkok = (bool_t) (WEXITSTATUS(wstat) == 0);
		else if (WTERMSIG(wstat))
			chkok = FALSE;

		if (!chkok)
			return TRUE; /* Cannot check, just hope it's ok */

		vmaj = (ver & 0xff00) >> 8;
		vmin = (ver & 0x00ff);

		if (vmaj != atoi(VERSION_MAJ) || vmin != atoi(VERSION_MIN)) {
			(void) fprintf(errfp, "%s Fatal Error:\n"
				"The XMcd.version (%u.%u) in the %s file\n"
				"is not compatible with this version of "
				"%s (%s.%s).\n",
				PROGNAME, vmaj, vmin, path,
				PROGNAME, VERSION_MAJ, VERSION_MIN
			);
			return FALSE;
		}

		return TRUE;
	}

	/* Set defaults */
	vmaj = atoi(VERSION_MAJ);
	vmin = atoi(VERSION_MIN);

	/* Force uid and gid to original setting */
	if (!util_set_ougid())
		_exit(errno);

	/* Read resource file and look for XMcd.version */
	if ((fp = fopen(path, "r")) == NULL)
		_exit(0);

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] == '!' || buf[0] == '#' ||
		    buf[0] == '\n' || buf[0] == '\0')
			continue;

		if (sscanf(buf, "XMcd.version: %u.%u", &vmaj, &vmin) == 2)
			break;	/* Found it */
	}

	(void) fclose(fp);

	ver = ((vmaj << 8) | vmin);
	(void) write(pfd[1], &ver, sizeof(ver));
	(void) close(pfd[1]);

	_exit(0);
	/*NOTREACHED*/
#endif	/* __VMS */
}


/*
 * x_error
 *	X error handler - Used when user interface debugging is enabled
 *
 * Args:
 *	dpy - The X display
 *	ev - The X event
 *
 * Return:
 *	Does not return.
 */
STATIC int
x_error(Display *dpy, XErrorEvent *ev)
{
	char	str[ERR_BUF_SZ],
		msg[ERR_BUF_SZ],
		num[32];
	char	*mtyp = "XlibMessage";

	(void) fprintf(errfp, "x_error: X error on \"%s\"\n",
		DisplayString(dpy)
	);
	XGetErrorText(dpy, ev->error_code, str, sizeof(str));
	XGetErrorDatabaseText(
		dpy, mtyp, "XError", "X Error", msg, sizeof(msg)
	);
	(void) fprintf(errfp, "%s: %s\n  ", msg, str);
	XGetErrorDatabaseText(
		dpy, mtyp, "MajorCode", "Request Major code %d",
		msg, sizeof(msg)
	);
	(void) fprintf(errfp, msg, ev->request_code);
	if (ev->request_code < 128) {
		(void) sprintf(num, "%d", ev->request_code);
		XGetErrorDatabaseText(
			dpy, "XRequest", num, "", str, sizeof(str)
		);
		(void) fprintf(errfp, " (%s)\n", str);
	}
	else {
		XGetErrorDatabaseText(
			dpy, mtyp, "MinorCode", "Request Minor code %d",
			msg, sizeof(msg)
		);
		(void) fputs("  ", errfp);
		(void) fprintf(errfp, msg, ev->minor_code);
		(void) fputs("\n", errfp);
	}
	if (ev->error_code >= 128) {
		(void) fprintf(errfp, "  Error code 0x%x\n", ev->error_code);
	}
	else {
		switch (ev->error_code) {
		case BadValue:
			XGetErrorDatabaseText(
				dpy, mtyp, "Value", "Value 0x%x",
				msg, sizeof(msg)
			);
			(void) fputs("  ", errfp);
			(void) fprintf(errfp, msg, ev->resourceid);
			(void) fputs("\n", errfp);
			break;
		case BadAtom:
			XGetErrorDatabaseText(
				dpy, mtyp, "AtomID", "AtomID 0x%x",
				msg, sizeof(msg)
			);
			(void) fputs("  ", errfp);
			(void) fprintf(errfp, msg, ev->resourceid);
			(void) fputs("\n", errfp);
			break;
		case BadWindow:
		case BadPixmap:
		case BadCursor:
		case BadFont:
		case BadDrawable:
		case BadColor:
		case BadGC:
		case BadIDChoice:
			XGetErrorDatabaseText(
				dpy, mtyp, "ResourceID", "ResourceID 0x%x",
				msg, sizeof(msg)
			);
			(void) fputs("  ", errfp);
			(void) fprintf(errfp, msg, ev->resourceid);
			(void) fputs("\n", errfp);

			break;
		default:
			(void) fprintf(errfp, "  Error code: 0x%x\n",
				ev->error_code
			);
			break;
		}
	}
	XGetErrorDatabaseText(
		dpy, mtyp, "ErrorSerial", "Error Serial #%d",
		msg, sizeof(msg)
	);
	(void) fputs("  ", errfp);
	(void) fprintf(errfp, msg, ev->serial);
	(void) fputs("\n", errfp);
	XGetErrorDatabaseText(
		dpy, mtyp, "CurrentSerial", "Current Serial #%d",
		msg, sizeof(msg)
	);
	(void) fputs("\n", errfp);

	if (ev->error_code == BadImplementation)
		return 0;

	/* Cause a core dump if possible, and exit immediately */
	abort();
	return 0;	/* Just to silence some picky compilers */
}


/*
 * x_ioerror
 *	X IO error handler - Used when user interface debugging is enabled
 *
 * Args:
 *	dpy - The X display
 *	ev - The X event
 *
 * Return:
 *	Does not return.
 */
STATIC int
x_ioerror(Display *dpy)
{
	if (errno == EPIPE) {
		(void) fprintf(errfp,
			"x_ioerror: X connection to \"%s\" broken:\n"
			"explicit kill or server shutdown.\n",
			DisplayString(dpy)
		);
	}
	else {
		(void) fprintf(errfp,
			"x_ioerror: fatal IO error (errno %d) on \"%s\"\n",
			errno, DisplayString(dpy)
		);
	}

	/* Cause a core dump if possible, and exit immediately */
	abort();
	return 0;	/* Just to silence some picky compilers */
}


/*
 * main
 *	The main function
 */
int
main(int argc, char **argv)
{
	int	i;
	Display	*display;
	media_grab_t *p;
	XEvent *ev = (XEvent *)malloc(sizeof(XEvent));
#ifdef HAS_EUID
	uid_t	euid,
		ruid;
	gid_t	egid,
		rgid;
#endif

	/* Program startup multi-threading initializations */
	cdda_preinit();

	/* Error message stream */
	errfp = stderr;

	/* Initialize variables */
	progname = argv[0];
	cmd = NULL;
	localcmap = (Colormap) 0;
	(void) memset(&status, 0, sizeof(curstat_t));

	/* Handle some signals */
	if (util_signal(SIGINT, onsig) == SIG_IGN)
		(void) util_signal(SIGINT, SIG_IGN);
	if (util_signal(SIGHUP, onsig) == SIG_IGN)
		(void) util_signal(SIGHUP, SIG_IGN);
	if (util_signal(SIGTERM, onsig) == SIG_IGN)
		(void) util_signal(SIGTERM, SIG_IGN);
	if (util_signal(SIGQUIT, onsig) == SIG_IGN)
		(void) util_signal(SIGQUIT, SIG_IGN);

	/* Set SIGCHLD handler to default */
#ifndef __VMS_VER
	(void) util_signal(SIGCHLD, SIG_DFL);
#else
#if (__VMS_VER >= 70000000) && (__DECC_VER > 50230003)
	/* OpenVMS v7.0 and DECCV5.2 have these defined */
	(void) util_signal(SIGCHLD, SIG_DFL);
#endif
#endif

	/* Ignore SIGALRMs until we need them */
	(void) util_signal(SIGALRM, SIG_IGN);

	/* Hack: Aside from stdin, stdout, stderr, there shouldn't
	 * be any other files open, so force-close them.  This is
	 * necessary in case xmcd was inheriting any open file
	 * descriptors from a parent process which is for the
	 * CD-ROM device, and violating exclusive-open requirements
	 * on some platforms.
	 */
	for (i = 3; i < 10; i++)
		(void) close(i);

#if (XtSpecificationRelease >= 5)
	/* Set locale */
	XtSetLanguageProc(NULL, NULL, NULL);
#endif

	/* Snoop command line for -display before XtVaAppInitialize
	 * consumes it  Also, handle -help here before we attempt
	 * to open the X display.
	 */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-display") == 0)
			cdisplay = argv[++i];
		else if (strcmp(argv[i], "-help") == 0) {
			usage(argv[i]);
			exit(1);
		}
	}

	/* Initialize libutil */
	util_init();

#ifdef HAS_EUID
	/* Get real IDs */
	ruid = getuid();
	rgid = getgid();

	/* Save effective IDs */
	euid = geteuid();
	egid = getegid();

	/* Give up root until we have a connection to the X server, so
	 * that X authentication will work correctly.
	 */
	(void) util_seteuid(ruid);
	(void) util_setegid(rgid);
#endif

	/* Initialize X toolkit */
	widgets.toplevel = XtVaAppInitialize(
		&app_context,
		"XMcd",
		options, XtNumber(options),
		&argc, argv,
		NULL,
		XmNmappedWhenManaged, False,
		NULL
	);

	display = XtDisplay(widgets.toplevel);

	/* Check for old xmcd X resources */
	if (!chk_resource_ver(display))
		exit(1);

	/* Get application options */
	XtVaGetApplicationResources(
		widgets.toplevel,
		(XtPointer) &app_data,
		resources,
		XtNumber(resources),
		NULL
	);
		
	/* Build command */
	if (argc > 1) {
		for (i = 1;  i < argc; i++) {
			if (argv[i][0] == '-') {
				usage(argv[i]);
				exit(1);
			}

			if (i == 1) {
				cmd = (char *) MEM_ALLOC(
					"cmd",
					strlen(argv[i]) + 1
				);
				if (cmd == NULL) {
					CD_FATAL(app_data.str_nomemory);
					exit(1);
				}
				cmd[0] = '\0';
			}
			else {
				cmd = (char *) MEM_REALLOC(
					"cmd",
					cmd,
					strlen(cmd) + strlen(argv[i]) + 2
				);
				if (cmd == NULL) {
					CD_FATAL(app_data.str_nomemory);
					exit(1);
				}
				(void) strcat(cmd, " ");
			}
			(void) strcat(cmd, argv[i]);
		}
	}

	/* Set X error handlers if user interface debugging is enabled */
	if ((app_data.debug & DBG_UI) != 0) {
		(void) XSetErrorHandler(x_error);
		(void) XSetIOErrorHandler(x_ioerror);
	}

	/* Remote control specified - handle that here */
	if (app_data.remotemode) {
		cmd_init(&status, display, TRUE);
		exit(cmd_sendrmt(display, cmd));
	}

	/* Install colormap if specified */
	if (app_data.instcmap) {
		localcmap = XCopyColormapAndFree(
			display,
			DefaultColormap(display, DefaultScreen(display))
		);
		XtVaSetValues(widgets.toplevel, XmNcolormap, localcmap, NULL);
	}

	/* Create all widgets */
	create_widgets(&widgets);

	/* Configure resources before realizing widgets */
	pre_realize_config(&widgets);

	/* Display widgets */
	XtRealizeWidget(widgets.toplevel);

	/* Configure resources after realizing widgets */
	post_realize_config(&widgets, &pixmaps);

	/* Register callback routines */
	register_callbacks(&widgets, &status);

#ifdef HAS_EUID
	/* Ok, back to root */
	(void) util_seteuid(euid);
	(void) util_setegid(egid);
#endif

	/* Initialize various subsystems */
	cd_init(&status);

	/* Start various subsystems */
	cd_start(&status);

	/* Make main window appear */
	XtMapWidget(widgets.toplevel);

	/* Start remote control */
	cmd_init(&status, display, FALSE);

	if (cmd != NULL) {
		/* Change to watch cursor */
		cd_busycurs(TRUE, CURS_ALL);

		/* Schedule to perform start-up command */
		(void) cd_timeout( 
			STARTUP_CMD_DELAY,
			cmd_startup,
			(byte_t *) cmd
		);
	}

	XSelectInput(display, XDefaultRootWindow(display), KeyPressMask);
	/* Main event processing loop */

	do{
		XtAppNextEvent(app_context, ev);

		/* If the current event is a KeyPress or KeyRelease,
		 *  Walk the media key tree to find the corresponding entry,
		 *  then activate its associated widget, Otherwise, pass the event along. */
		if(ev->type == KeyPress){
			XKeyEvent *kev = (XKeyEvent *)ev;
			for(i=0; i<TOTAL_MEDIA_GRABS; i++){
				p = &media_grablist[i];
				if(kev->keycode == p->me_keycode){
					XmProcessTraversal(*(p->me_assoc_widget), XmTRAVERSE_CURRENT);
					XtCallActionProc(*(p->me_assoc_widget), "ArmAndActivate", ev, NULL, 0);
					break;
				}
			}
		} 
		XtDispatchEvent(ev);

	} while(!XtAppGetExitFlag(app_context));
	shutdown_gui();
	exit(0);

	/*NOTREACHED*/
}

