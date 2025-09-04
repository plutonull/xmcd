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
#ifndef lint
static char *_userreg_c_ident_ = "@(#)userreg.c	1.16 03/12/12";
#endif

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"
#include "cda_d/cda.h"
#include "cda_d/userreg.h"


extern appdata_t	app_data;
extern cdinfo_incore_t	*dbp;
extern FILE		*errfp;


/***********************
 *  internal routines  *
 ***********************/

/*
 * cda_get_cddbhandle
 *	Prompt user for the CDDB user handle
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 *	FALSE - failure
 */
STATIC bool_t
cda_get_cddbhandle(FILE *ttyfp)
{
	char	input[64];

	for (;;) {
		(void) fprintf(ttyfp,
		    "    Please enter your CDDB user handle%s%s%s: ",
		    dbp->userreg.handle == NULL ? "" : " [",
		    dbp->userreg.handle == NULL ? "" : dbp->userreg.handle,
		    dbp->userreg.handle == NULL ? "" : "]");
		(void) fflush(ttyfp);

		if (fgets(input, 63, stdin) == NULL) {
			(void) fprintf(ttyfp, "\n");
			(void) fflush(ttyfp);
			return FALSE;
		}
		input[strlen(input)-1] = '\0';
		if (input[0] != '\0') {
			if (!util_newstr(&dbp->userreg.handle, input)) {
				(void) fflush(ttyfp);
				CDA_FATAL(app_data.str_nomemory);
				return FALSE;
			}
			break;
		}
		else if (dbp->userreg.handle != NULL)
			break;
	}
	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_get_cddbpass
 *	Prompt user for the CDDB password
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 */
STATIC bool_t
cda_get_cddbpass(FILE *ttyfp)
{
	char	input[64],
		input2[64];

	for (;;) {
		(void) fprintf(ttyfp,
			"    Please enter your CDDB password: ");
		(void) fflush(ttyfp);
		cda_echo(ttyfp, FALSE);
		if (fgets(input, 63, stdin) == NULL) {
			cda_echo(ttyfp, TRUE);
			(void) fprintf(ttyfp, "\n");
			(void) fflush(ttyfp);
			return FALSE;
		}
		cda_echo(ttyfp, TRUE);
		input[strlen(input)-1] = '\0';
		if (input[0] == '\0') {
			(void) fprintf(ttyfp, "\n");
			continue;
		}

		(void) fprintf(ttyfp,
			"\n    Please re-type your CDDB password: ");
		(void) fflush(ttyfp);
		cda_echo(ttyfp, FALSE);
		if (fgets(input2, 63, stdin) == NULL) {
			cda_echo(ttyfp, TRUE);
			(void) fprintf(ttyfp, "\n");
			(void) fflush(ttyfp);
			return FALSE;
		}
		cda_echo(ttyfp, TRUE);
		input2[strlen(input2)-1] = '\0';
		if (input2[0] == '\0' || strcmp(input, input2) != 0) {
			(void) fprintf(ttyfp,
				"\nPasswords do not match.  Try again.\n");
			continue;
		}

		if (!util_newstr(&dbp->userreg.passwd, input)) {
			(void) fprintf(ttyfp, "\n");
			(void) fprintf(ttyfp, "\n");
			(void) fflush(ttyfp);
			CDA_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		if (!util_newstr(&dbp->userreg.vpasswd, input2)) {
			(void) fprintf(ttyfp, "\n");
			(void) fprintf(ttyfp, "\n");
			(void) fflush(ttyfp);
			CDA_FATAL(app_data.str_nomemory);
			return FALSE;
		}
		break;
	}
	(void) fprintf(ttyfp, "\n");
	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_get_hint
 *	Prompt user for the password hint
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 */
STATIC bool_t
cda_get_hint(FILE *ttyfp)
{
	char	input[64];

	(void) fprintf(ttyfp,
		"\nEntering a password hint allows CDDB to e-mail your\n"
		"password hint to you in case you forget it in the future.\n");

	/* Password hint */
	(void) fprintf(ttyfp, "\n    Please enter a password hint: ");
	(void) fflush(ttyfp);

	if (fgets(input, 63, stdin) == NULL) {
		(void) fprintf(ttyfp, "\n");
		(void) fflush(ttyfp);
		return TRUE;
	}
	input[strlen(input)-1] = '\0';
	if (!util_newstr(&dbp->userreg.hint, input)) {
		(void) fflush(ttyfp);
		CDA_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_get_email
 *	Prompt user for the e-mail address
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 */
STATIC bool_t
cda_get_email(FILE *ttyfp)
{
	char	input[64];

	(void) fprintf(ttyfp,
		"\nProviding your e-mail address will allow CDDB to send you\n"
		"your password hint if you forget your password.\n");

	/* E-mail address */
	(void) fprintf(ttyfp,
		"\n    Please enter your e-mail address: ");
	(void) fflush(ttyfp);

	if (fgets(input, 63, stdin) == NULL) {
		(void) fprintf(ttyfp, "\n");
		(void) fflush(ttyfp);
		return TRUE;
	}
	input[strlen(input)-1] = '\0';
	if (!util_newstr(&dbp->userreg.email, input)) {
		(void) fflush(ttyfp);
		CDA_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_get_age
 *	Prompt user for the age
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 */
STATIC bool_t
cda_get_age(FILE *ttyfp)
{
	char	input[64];

	(void) fprintf(ttyfp,
		"\n    Please enter your age: ");
	(void) fflush(ttyfp);

	if (fgets(input, 63, stdin) == NULL) {
		(void) fprintf(ttyfp, "\n");
		(void) fflush(ttyfp);
		return TRUE;
	}
	input[strlen(input)-1] = '\0';
	if (!util_newstr(&dbp->userreg.age, input)) {
		(void) fflush(ttyfp);
		CDA_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_get_gender
 *	Prompt user for the gender
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 */
STATIC bool_t
cda_get_gender(FILE *ttyfp)
{
	char	input[64];
	bool_t	again;

	do {
		again = FALSE;

		(void) fprintf(ttyfp,
			"\n    Please enter your gender.\n"
			"\t1. Male\n"
			"\t2. Female\n"
			"\t3. Unspecified\n"
			"    Select [1-3]: ");
		(void) fflush(ttyfp);

		if (fgets(input, 63, stdin) == NULL) {
			(void) fprintf(ttyfp, "\n");
			(void) fflush(ttyfp);
			return TRUE;
		}
		input[strlen(input)-1] = '\0';

		if (input[0] != '\0') {
			switch (input[0]) {
			case '1':
				(void) strcpy(input, "m");
				break;
			case '2':
				(void) strcpy(input, "f");
				break;
			case '3':
				input[0] = '\0';
				break;
			default:
				(void) fprintf(ttyfp, "Invalid selection.\n");
				again = TRUE;
				break;
			}
		}
	} while (again);

	if (!util_newstr(&dbp->userreg.gender, input)) {
		(void) fflush(ttyfp);
		CDA_FATAL(app_data.str_nomemory);
		return FALSE;
	}
	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_get_allowemail
 *	Prompt user whether to allow CDDB email info/promotions
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 */
STATIC bool_t
cda_get_allowemail(FILE *ttyfp)
{
	char	input[64];

	if (dbp->userreg.email == NULL)
		return TRUE;

	(void) fprintf(ttyfp,
		"\n    Would you like CDDB to send you information and\n"
		"    promotions via e-mail? [yn] ");
	(void) fflush(ttyfp);

	if (fgets(input, 63, stdin) == NULL) {
		(void) fprintf(ttyfp, "\n");
		(void) fflush(ttyfp);
		return TRUE;
	}
	input[strlen(input)-1] = '\0';
	if (input[0] == 'y' || input[0] == 'Y')
		dbp->userreg.allowemail = TRUE;
	else
		dbp->userreg.allowemail = FALSE;

	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_get_allowstats
 *	Prompt user whether to allow playback stats to customize content
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal
 *
 * Return:
 *	TRUE - success
 */
STATIC bool_t
cda_get_allowstats(FILE *ttyfp)
{
	char	input[64];

	(void) fprintf(ttyfp,
	    "\n    Would you allow CDDB to use your playback statistics\n"
	    "    to customize the information delivered "
	    "(such as URLs)? [yn] ");
	(void) fflush(ttyfp);

	if (fgets(input, 63, stdin) == NULL) {
		(void) fprintf(ttyfp, "\n");
		(void) fflush(ttyfp);
		return TRUE;
	}
	input[strlen(input)-1] = '\0';
	if (input[0] == 'y' || input[0] == 'Y')
		dbp->userreg.allowstats = TRUE;
	else
		dbp->userreg.allowstats = FALSE;

	(void) fflush(ttyfp);
	return TRUE;
}


/***********************
 *   public routines   *
 ***********************/

/*
 * cda_cddbhint
 *	Have CDDB e-mail the password.
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal.
 *
 * Return:
 *	TRUE for success
 *	FALSE for failure
 */
bool_t
cda_cddbhint(FILE *ttyfp)
{
	curstat_t	*s = curstat_addr();
	cdinfo_ret_t	ret;

	if (cdinfo_cddb_ver() != 2) {
		(void) fprintf(ttyfp, "%s\n%s\n",
		    "This copy of cda uses the \"classic\" CDDB service.",
		    "No user registration is required."
		);
		(void) fflush(ttyfp);
		return TRUE;
	}

	(void) fprintf(ttyfp,
		"\nIf you had forgotten your CDDB2 user registration\n"
		"password, Enter your user handle and have CDDB e-mail\n"
		"your password hint to you.\n\n");

	if (!cda_get_cddbhandle(ttyfp))
		return FALSE;

	(void) fprintf(ttyfp, "\nContacting CDDB... ");

	if ((ret = cdinfo_getpasshint(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp,
			"cdinfo_getpasshint: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	switch (CDINFO_GET_STAT(ret)) {
	case 0:	/* Success */
		(void) fprintf(ttyfp, " Success.\n"
		    "Your password hint is being e-mailed to you.\n");
		break;

	case NAME_ERR:
		(void) fprintf(ttyfp, " Failed.\n"
		    "Invalid user handle.  Please try again.\n");
		break;

	case HINT_ERR:
		(void) fprintf(ttyfp, " Failed.\n"
		    "You did not specify a hint when you first registered.\n"
		    "There is nothing to e-mail to you.\n");
		break;

	case MAIL_ERR:
		(void) fprintf(ttyfp, " Failed.\n"
		    "You did not specify a e-mail address when you first\n"
		    "registered.  There is nothing to e-mail to you.\n");
		break;

	default:
		(void) fprintf(ttyfp, " Failed.");
		break;
	}

	(void) fflush(ttyfp);
	return TRUE;
}


/*
 * cda_userreg
 *	Ask the user to register with CDDB
 *
 * Args:
 *	ttyfp - Opened file stream to the user's terminal.
 *
 * Return:
 *	TRUE for success
 *	FALSE for failure
 */
bool_t
cda_userreg(FILE *ttyfp)
{
	curstat_t	*s = curstat_addr();
	cdinfo_ret_t	ret;

	if (cdinfo_cddb_ver() != 2) {
		(void) fprintf(ttyfp, "%s\n%s\n",
		    "This copy of cda uses the \"classic\" CDDB service.",
		    "No user registration is required."
		);
		(void) fflush(ttyfp);
		return TRUE;
	}

	(void) fprintf(ttyfp, "\n\tGracenote(R) CDDB2 User Registration\n"
		"\nYou must register in order to access the CDDB2 service.\n"
		"The user handle and password are required, all other\n"
		"answers are optional, and will help CDDB provide more\n"
		"personalized service.  See the PRIVACY file that comes\n"
		"with the xmcd/cda distribution for CDDB's user information\n"
		"privacy statement.\n\n");
	(void) fprintf(ttyfp,
		"You may also use this form to change or update your CDDB2\n"
		"user registration information.\n\n");

	if (!cda_get_cddbhandle(ttyfp))
		return FALSE;

	if (!cda_get_cddbpass(ttyfp))
		return FALSE;

	(void) fprintf(ttyfp,
		"\nAll answers below are optional.  If you wish to skip,\n"
		"Just press the <Enter> key.\n");

	if (!cda_get_hint(ttyfp))
		return FALSE;

	if (!cda_get_email(ttyfp))
		return FALSE;

	/*
	 * Region and postal code not implemented -- too clumsy with
	 * character interface
	 */

	if (!cda_get_age(ttyfp))
		return FALSE;

	if (!cda_get_gender(ttyfp))
		return FALSE;

	if (!cda_get_allowemail(ttyfp))
		return FALSE;

	if (!cda_get_allowstats(ttyfp))
		return FALSE;

	(void) fprintf(ttyfp, "\nRegistering with CDDB...");
	(void) fflush(ttyfp);
	if ((ret = cdinfo_reguser(s)) != 0) {
		DBGPRN(DBG_CDI)(errfp, "cdinfo_reguser: status=%d arg=%d\n",
			CDINFO_GET_STAT(ret), CDINFO_GET_ARG(ret));
	}

	switch (CDINFO_GET_STAT(ret)) {
	case 0:
		/* Success */
		(void) fprintf(ttyfp, " Succeeded.\n");
		break;

	case NAME_ERR:
		(void) fprintf(ttyfp, " Failed.\n");
		(void) fprintf(ttyfp,
		    "Either you got your password wrong, or someone else.\n"
		    "is already using this handle.\n"
		    "Please try again, or use the \"cda cddbhint\" command\n"
		    "to have your password hint sent to you via e-mail.\n");
		break;
	default:
		(void) fprintf(ttyfp, " Failed.\n");
		break;
	}

	(void) fflush(ttyfp);
	return TRUE;
}


