/*
 *   cdinfo - CD Information Management Library
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
static char     *_motd_c_ident_ = "@(#)motd.c	7.27 04/04/20";
#endif

#include "common_d/appenv.h"

#ifndef NOREMOTE
#ifndef __VMS
/* UNIX */

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#if defined(_AIX) || defined(__QNX__)
#include <sys/select.h>
#endif

#else
/* OpenVMS */

#include <socket.h>
#include <time.h>
#include <in.h>
#include <inet.h>
#include <netdb.h>

#endif	/* __VMS */
#endif	/* NOREMOTE */

#define _CDINFO_INTERN	/* Expose internal function protos in cdinfo.h */

#include "common_d/util.h"
#include "common_d/version.h"
#include "libdi_d/libdi.h"
#include "cdinfo_d/cdinfo.h"
#include "cdinfo_d/motd.h"


#define MOTD_PATH		"motd.cgi"
#define MOTD_BUFLEN		2048


extern appdata_t        app_data;
extern FILE		*errfp;

extern cdinfo_client_t	*cdinfo_clinfo;		/* Client info */
extern bool_t		cdinfo_ischild;		/* Is a child process */


STATIC char		*motd_auth_buf = NULL;

/*
 * Data used by motd_b64encode
 */
STATIC char	motd_b64map[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '+', '/'
};

#define B64_PAD		'='


/*
 * motd_b64encode
 *	Base64 encoding function
 *
 * Args:
 *	ibuf - Input string buffer
 *	len - Number of characters
 *	obuf - Output string buffer
 *	brklines - Whether to break output into multiple lines
 *
 * Return:
 *	Nothing.
 */
STATIC void
motd_b64encode(byte_t *ibuf, int len, byte_t *obuf, bool_t brklines)
{
	int	i, j, k, n,
		c[4];
	byte_t	sbuf[4];

	for (i = k = 0; (i + 3) <= len; i += 3, ibuf += 3) {
		c[0] = ((int) ibuf[0] >> 2);
		c[1] = ((((int) ibuf[0] & 0x03) << 4) |
			(((int) ibuf[1] & 0xf0) >> 4));
		c[2] = ((((int) ibuf[1] & 0x0f) << 2) |
			(((int) ibuf[2] & 0xc0) >> 6));
		c[3] = ((int) ibuf[2] & 0x3f);

		for (j = 0; j < 4; j++)
			*obuf++ = motd_b64map[c[j]];

		if (brklines && ++k == 16) {
			k = 0;
			*obuf++ = '\n';
		}
	}

	if (i < len) {
		n = len - i;
		(void) strncpy((char *) sbuf, (char *) ibuf, n);
		for (j = n; j < 3; j++)
			sbuf[j] = (unsigned char) 0;

		n++;
		ibuf = sbuf;
		c[0] = ((int) ibuf[0] >> 2);
		c[1] = ((((int) ibuf[0] & 0x03) << 4) |
			(((int) ibuf[1] & 0xf0) >> 4));
		c[2] = ((((int) ibuf[1] & 0x0f) << 2) |
			(((int) ibuf[2] & 0xc0) >> 6));
		c[3] = ((int) ibuf[2] & 0x3f);

		for (j = 0; j < 4; j++)
			*obuf++ = (j < n) ? motd_b64map[c[j]] : B64_PAD;

		if (brklines && ++k == 16)
			*obuf++ = '\n';
	}

	if (brklines)
		*obuf++ = '\n';

	*obuf = '\0';
}


/*
 * motd_set_auth
 *	Set the HTTP/1.1 proxy-authorization string
 *
 * Args:
 *	name - The proxy user name
 *	passwd - The proxy password
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
STATIC bool_t
motd_set_auth(char *name, char *passwd)
{
	int	i,
		j,
		n1,
		n2;
	char	*buf;

	if (name == NULL || passwd == NULL)
		return FALSE;

	i = strlen(name);
	j = strlen(passwd);
	n1 = i + j + 2;
	n2 = (n1 * 4 / 3) + 8;

	if ((buf = (char *) MEM_ALLOC("strbuf", n1)) == NULL)
		return FALSE;

	(void) sprintf(buf, "%s:%s", name, passwd);

	(void) memset(name, 0, i);
	(void) memset(passwd, 0, j);

	if (motd_auth_buf != NULL)
		MEM_FREE(motd_auth_buf);

	motd_auth_buf = (char *) MEM_ALLOC("motd_auth_buf", n2);
	if (motd_auth_buf == NULL)
		return FALSE;

	/* Base64 encode the name/password pair */
	motd_b64encode(
		(byte_t *) buf,
		strlen(buf),
		(byte_t *) motd_auth_buf,
		FALSE
	);

	(void) memset(buf, 0, n1);
	MEM_FREE(buf);
	return TRUE;
}


/*
 * motd_connect
 *	Make a network connection to the server
 *
 * Args:
 *	host - Server host name
 *	port - Server port
 *	fd - Return file descriptor
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
#ifdef NOREMOTE
/*ARGSUSED*/
#endif
STATIC bool_t
motd_connect(char *host, unsigned short port, int *fd)
{
#ifdef NOREMOTE
	return FALSE;
#else
	struct hostent		*hp;
	struct in_addr		ad;
	struct sockaddr_in	sin;

	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;

	DBGPRN(DBG_MOTD)(errfp, "\nmotd_connect: %s:%d\n", host, (int) port);
	/* Find server host address */
	if ((hp = gethostbyname(host)) != NULL) {
		(void) memcpy((char *) &sin.sin_addr, hp->h_addr,
			      hp->h_length);
	}
	else {
		if ((ad.s_addr = inet_addr(host)) != (unsigned long) -1)
			(void) memcpy((char *) &sin.sin_addr,
				      (char *) &ad.s_addr, sizeof(ad.s_addr));
		else
			return FALSE;
	}

	/* Open socket */
	if ((*fd = SOCKET(AF_INET, SOCK_STREAM, 0)) < 0) {
		*fd = -1;
		return FALSE;
	}

	/* Connect to server */
	if (CONNECT(*fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		(void) close(*fd);
		*fd = -1;
		return FALSE;
	}

	return TRUE;
#endif	/* NOREMOTE */
}


/*
 * motd_disconnect
 *	Disconnect from a server
 *
 * Args:
 *	fd - File descriptor returned from motd_connect()
 *
 * Return:
 *	Nothing
 */
STATIC void
motd_disconnect(int fd)
{
	(void) close(fd);
}


/*
 * motd_query_server
 *	Get MOTD from the server.
 *
 * Args:
 *	None.
 *
 * Return:
 *	Pointer to the string buffer containing the raw MOTD data,
 *	or NULL on failure.  The caller should deallocate the buffer
 *	using MEM_FREE() when done.
 */
STATIC char *
motd_query_server(void)
{
#ifdef NOREMOTE
	return NULL;
#else
	int		fd,
			code,
			n;
	size_t		buflen,
			rbuflen;
	unsigned short	srvport,
			conport;
	char		*plat,
			*url,
			*srvhost,
			*conhost,
			*buf,
			*p,
			*q;
	struct utsname	*un;
	unsigned int	oa = 0;
	void		(*oh)(int) = NULL;

	if (app_data.use_proxy && app_data.proxy_server == NULL)
		return FALSE;

	/*
	 * Do server read
	 */

	SOCKSINIT(cdinfo_clinfo->prog);

	buflen = MOTD_BUFLEN;
	if ((buf = (char *) MEM_ALLOC("http_buf", buflen)) == NULL)
		return NULL;

	if (app_data.use_proxy && app_data.proxy_auth &&
	    motd_auth_buf == NULL) {
		cdinfo_incore_t	*cdp = cdinfo_addr();

		if (cdp->proxy_user == NULL || cdp->proxy_passwd == NULL) {
			/* Just abort for now and wait until the
			 * next time a CD info lookup happens.
			 */
			MEM_FREE(buf);
			return NULL;
		}

		motd_set_auth(cdp->proxy_user, cdp->proxy_passwd);
	}

	/* Get platform information */
	plat = (char *) MEM_ALLOC("plat",
		sizeof(struct utsname) + (STR_BUF_SZ * 3)
	);
	if (plat == NULL) {
		MEM_FREE(buf);
		return NULL;
	}

	(void) sprintf(plat, "client=xmcd-%s.%s.%s&subclient=%s&cddb=%d",
		VERSION_MAJ, VERSION_MIN, VERSION_TEENY,
		cdinfo_clinfo->prog,
		cdinfo_cddb_ver()
	);

	un = util_get_uname();
	if ((p = util_urlencode(un->sysname)) == NULL) {
		MEM_FREE(buf);
		MEM_FREE(plat);
		return NULL;
	}
	(void) sprintf(plat, "%s&sys=%s", plat, p);
	MEM_FREE(p);

	if ((p = util_urlencode(un->machine)) == NULL) {
		MEM_FREE(buf);
		MEM_FREE(plat);
		return NULL;
	}
	(void) sprintf(plat, "%s&mach=%s", plat, p);
	MEM_FREE(p);

	if ((p = util_urlencode(un->release)) == NULL) {
		MEM_FREE(buf);
		MEM_FREE(plat);
		return NULL;
	}
	(void) sprintf(plat, "%s&rel=%s", plat, p);
	MEM_FREE(p);

	if ((p = util_urlencode(un->version)) == NULL) {
		MEM_FREE(buf);
		MEM_FREE(plat);
		return NULL;
	}
	(void) sprintf(plat, "%s&ver=%s", plat, p);
	MEM_FREE(p);

	url = (char *) MEM_ALLOC("url",
		strlen(XMCD_URL) + strlen(MOTD_PATH) +
		(sizeof(struct utsname) * 2) + 16
	);
	if (url == NULL) {
		MEM_FREE(buf);
		MEM_FREE(plat);
		return NULL;
	}

	(void) sprintf(url, "%s%s?%s", XMCD_URL, MOTD_PATH, plat);

	conhost = NULL;
	srvhost = NULL;

	for (;;) {
		code = 0;

		p = url + 7;
		if ((q = strchr(p, '/')) != NULL)
			*q = '\0';

		if (!util_newstr(&srvhost, p)) {
			MEM_FREE(url);
			MEM_FREE(plat);
			MEM_FREE(buf);
			return NULL;
		}

		if (q != NULL)
			*q = '/';

		if ((p = strrchr(srvhost, ':')) != NULL) {
			*p = '\0';
			srvport = (unsigned short) atoi(++p);
		}
		else
			srvport = 80;

		if (app_data.use_proxy) {
			if (!util_newstr(&conhost, app_data.proxy_server)) {
				MEM_FREE(srvhost);
				MEM_FREE(url);
				MEM_FREE(plat);
				MEM_FREE(buf);
				return NULL;
			}

			if ((p = strrchr(conhost, ':')) != NULL) {
				*p = '\0';
				conport = (unsigned short) atoi(++p);
			}
			else
				conport = 80;
		}
		else {
			if (!util_newstr(&conhost, srvhost)) {
				MEM_FREE(srvhost);
				MEM_FREE(url);
				MEM_FREE(plat);
				MEM_FREE(buf);
				return NULL;
			}

			conport = srvport;
		}

		/*
		 * Set up MOTD read command string
		 */

		if ((p = strchr(url + 7, '/')) == NULL) {
			MEM_FREE(srvhost);
			MEM_FREE(conhost);
			MEM_FREE(url);
			MEM_FREE(plat);
			MEM_FREE(buf);
			return NULL;
		}

		(void) sprintf(buf, "GET %s HTTP/1.0\r\n",
			app_data.use_proxy ? url : p
		);

		if (app_data.use_proxy && motd_auth_buf != NULL) {
			(void) sprintf(buf,
				"%sProxy-Authorization: Basic %s\r\n",
				buf, motd_auth_buf
			);
		}

		(void) sprintf(buf,
		       "%sHost: %s\r\n"
		       "User-Agent: Mozilla/5.0 (compatible; %s %s.%s.%s)\r\n"
		       "Accept: text/plain\r\n\r\n",
		       buf, srvhost,
		       cdinfo_clinfo->prog,
		       VERSION_MAJ, VERSION_MIN, VERSION_TEENY
		);

		/* Set timeout */
		oh = util_signal(SIGALRM, util_onsig);
		oa = alarm(app_data.srv_timeout);

		/* Open network connection to server */
		if (!motd_connect(conhost, conport, &fd)) {
			(void) alarm(oa);
			(void) util_signal(SIGALRM, oh);

			MEM_FREE(srvhost);
			MEM_FREE(conhost);
			MEM_FREE(url);
			MEM_FREE(plat);
			MEM_FREE(buf);
			return NULL;
		}
		(void) alarm(oa);
		(void) util_signal(SIGALRM, oh);

		DBGPRN(DBG_MOTD)(errfp, "\nSending:\n-------------------\n%s",
				buf);

		/* Send command */
		if ((n = send(fd, buf, strlen(buf), 0)) < 0) {
			/* Close server connection */
			motd_disconnect(fd);
			MEM_FREE(srvhost);
			MEM_FREE(conhost);
			MEM_FREE(url);
			MEM_FREE(plat);
			MEM_FREE(buf);
			return NULL;
		}

		/* Get response */
		p = buf;
		rbuflen = buflen;
		for (;;) {
			if (rbuflen <= 64) {
				int	offset = p - buf;

				buf = (char *) MEM_REALLOC("http_buf", buf,
					buflen + MOTD_BUFLEN
				);
				if (buf == NULL) {
					MEM_FREE(srvhost);
					MEM_FREE(conhost);
					MEM_FREE(url);
					MEM_FREE(plat);
					return NULL;
				}

				rbuflen += MOTD_BUFLEN;
				buflen += MOTD_BUFLEN;
				p = buf + offset;
			}

			n = recv(fd, p, rbuflen-1, 0);

			*(p + n) = '\0';
			if (n == 0)
				break;

			rbuflen -= n;
			p += n;
		}

		DBGPRN(DBG_MOTD)(errfp, "Received:\n-------------------\n%s",
				buf);

		if (strncmp(buf, "HTTP/", 5) == 0) {
			p = strchr(buf+5, ' ');
			if (p != NULL && isdigit((int) *(p+1)))
				code = atoi(p+1);
		}

		/* Close server connection */
		motd_disconnect(fd);

		if (code >= 200 && code <= 299) {
			/* Success */
			break;
		}
		else if (code >= 300 && code <= 399) {
			/* Handle redirections */
			p = util_strstr(buf, "Location: ");
			if (p == NULL) {
				MEM_FREE(buf);
				buf = NULL;
				break;
			}

			p += 10;
			while (*p == ' ' || *p == '\t')
				p++;

			if (strncmp(p, "http://", 7) != 0)
				break;

			if ((q = strchr(p+7, '\r')) != NULL ||
			    (q = strchr(p+7, '\n')) != NULL) {
				*q = '\0';

				MEM_FREE(url);

				url = (char *) MEM_ALLOC("url",
					strlen(p) +
					(sizeof(struct utsname) * 2) + 16
				);
				if (url == NULL) {
					MEM_FREE(buf);
					buf = NULL;
					break;
				}

				(void) sprintf(url, "%s?%s", p, plat);
			}
		}
		else {
			/* Error or unsupported status */
			MEM_FREE(buf);
			buf = NULL;
			break;
		}
	}

	MEM_FREE(srvhost);
	MEM_FREE(conhost);
	MEM_FREE(url);
	MEM_FREE(plat);
	return (buf);
#endif	/* NOREMOTE */
}


/*
 * motd_parse
 *	Parse MOTD data.
 *
 * Args:
 *	str - The raw MOTD string.
 *
 * Return:
 *	Pointer to the string buffer containing the messages to be
 *	displayed to the user, or NULL on failure.  The caller should
 *	deallocate the buffer using MEM_FREE() when done.
 */
STATIC char *
motd_parse(char *str)
{
	char	*p,
		*q,
		*motd_msg;
	size_t	msg_len;

	if (str == NULL)
		return NULL;

	motd_msg = NULL;
	msg_len = 0;

	for (p = q = str; q != NULL; p = q + 1) {
		for (;;) {
			if ((q = strchr(p, '\r')) != NULL ||
			    (q = strchr(p, '\n')) != NULL)
				*q = '\0';

			if (p == q)
				p = q + 1;
			else
				break;
		}

		if (*p == '\0')
			break;

		if (*p == '#')
			continue;

		if (strncmp(p, "XMCD_MOTD=", 10) == 0) {
			p += 10;

			for (;;) {
				bool_t	cont;

				if (motd_msg == NULL) {
					msg_len = strlen(p) + STR_BUF_SZ;
					motd_msg = (char *) MEM_ALLOC(
						"motd_msg",
						msg_len
					);
					motd_msg[0] = '\0';
				}
				else {
					msg_len += strlen(p) + STR_BUF_SZ;
					motd_msg = (char *) MEM_REALLOC(
						"motd_msg",
						motd_msg,
						msg_len
					);
				}

				if (motd_msg == NULL)
					return NULL;

				if (q != NULL && p[strlen(p)-1] == '\\') {
					p[strlen(p)-1] = '\0';
					cont = TRUE;
				}
				else
					cont = FALSE;

				(void) sprintf(motd_msg, "%s%s\n",
					motd_msg, p
				);

				/* Check for continuation */
				if (!cont) {
					(void) strcat(motd_msg, "\n");
					break;
				}

				p = q + 1;
				for (;;) {
					if ((q = strchr(p, '\r')) != NULL ||
					    (q = strchr(p, '\n')) != NULL)
						*q = '\0';

					if (p == q)
						p = q + 1;
					else
						break;
				}

				if (*p == '\0' || *p == '#') {
					(void) strcat(motd_msg, "\n");
					break;
				}
			}
		}
	}

	return (motd_msg);
}


/*
 * motd_get
 *	Display the messages of the day.
 *
 * Args:
 *	arg -   Set this to non-NULL to indicate that the MOTD
 *		query is a result of explicit user request.
 *		Automatic queries should call this function with
 *		a NULL arg.
 *
 * Return:
 *	Nothing
 */
void
motd_get(byte_t *arg)
{
	char		*motd_str,
			*msg;
	bool_t		useract;
	static bool_t	querying = FALSE;
#ifndef __VMS
	int		pfd[2],
			ret,
			i,
			n;
	pid_t		cpid;
	waitret_t	wstat;
	char		*p;
#endif

	if (querying)
		return;	/* Prevent recursion */

	querying = TRUE;
	useract = (bool_t) (arg != NULL);

#ifdef __VMS
	/* Get MOTD data */
	motd_str = motd_query_server();
#else
	motd_str = NULL;

	if (PIPE(pfd) < 0) {
		DBGPRN(DBG_MOTD)(errfp,
			"motd_get: pipe failed (errno=%d)\n", errno);
		querying = FALSE;
		return;
	}

	/* Fork child to perform actual I/O */
	switch (cpid = FORK()) {
	case 0:
		/* Child process */
		cdinfo_ischild = TRUE;

		(void) util_signal(SIGTERM, cdinfo_onterm);
		(void) util_signal(SIGPIPE, SIG_IGN);

		/* Close un-needed pipe descriptor */
		(void) close(pfd[0]);

		/* Force uid and gid to original setting */
		if (!util_set_ougid()) {
			(void) close(pfd[1]);
			_exit(1);
		}

		/* Get MOTD data */
		if ((motd_str = motd_query_server()) == NULL) {
			(void) close(pfd[1]);
			_exit(1);
		}

		n = strlen(motd_str);

		/* Write data length (in bytes) into pipe */
		if ((ret = write(pfd[1], &n, sizeof(n))) != sizeof(n)) {
			DBGPRN(DBG_MOTD)(errfp,
				"motd_get: error writing pipe "
				"(errno=%d)\n", errno);
			(void) close(pfd[1]);
			MEM_FREE(motd_str);
			_exit(1);
		}

		/* Write MOTD data into pipe */
		for (p = motd_str; n > 0; n -= ret, p += ret) {
			ret = write(pfd[1], p, n);
			if (ret < 0) {
				DBGPRN(DBG_MOTD)(errfp,
					"motd_get: error writing pipe "
					"(errno=%d)\n", errno);
				(void) close(pfd[1]);
				MEM_FREE(motd_str);
				_exit(1);
			}
		}

		MEM_FREE(motd_str);
		(void) close(pfd[1]);

		/* Child exits here */
		_exit(0);
		/*NOTREACHED*/

	case -1:
		DBGPRN(DBG_MOTD)(errfp,
			"motd_get: fork failed (errno=%d)\n", errno);
		querying = FALSE;
		return;

	default:
		/* Parent process */

		/* Close un-needed pipe descriptor */
		(void) close(pfd[1]);

		/* Read MOTD data length */
		n = 0;
		if (read(pfd[0], &n, sizeof(n)) == sizeof(n) && n > 0) {
			/* Allocate MOTD data storage */
			motd_str = (char *) MEM_ALLOC("motd_str", n + 1);
			if (motd_str != NULL) {
				i = n;

				/* Read MOTD from pipe */
				for (p = motd_str; n > 0; n -= ret, p += ret) {
					ret = read(pfd[0], p, n);
					if (ret < 0) {
						MEM_FREE(motd_str);
						motd_str = NULL;
						break;
					}
				}
				motd_str[i] = '\0';
			}
		}

		(void) close(pfd[0]);

		/* Wait for child to exit */
		ret = util_waitchild(cpid, app_data.srv_timeout + 5,
				     cdinfo_clinfo->workproc,
				     cdinfo_clinfo->arg,
				     TRUE, &wstat);
		if (app_data.debug & DBG_MOTD) {
			if (ret < 0) {
				(void) fprintf(errfp,
					"motd_get: waitpid fail errno=%d\n",
					errno);
			}
			else if (WIFEXITED(wstat)) {
				(void) fprintf(errfp,
					"motd_get: child exit status %d\n",
					WEXITSTATUS(wstat));
			}
			else if (WIFSIGNALED(wstat)) {
				(void) fprintf(errfp,
					"motd_get: child killed sig=%d\n",
					WTERMSIG(wstat));
			}
		}
		break;
	}
#endif	/* __VMS */

	/* Parse MOTD data */
	if ((msg = motd_parse(motd_str)) != NULL) {
		CDINFO_INFO(msg);
		MEM_FREE(msg);
	}
	else if (useract) {
		CDINFO_INFO(app_data.str_nomsg);
	}

	if (motd_str != NULL)
		MEM_FREE(motd_str);

	querying = FALSE;
}


