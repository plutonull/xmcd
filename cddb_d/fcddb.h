/*
 *   libcddb - CDDB Interface Library for xmcd/cda
 *
 *	This library implements an interface to access the "classic"
 *	CDDB1 services.
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
#ifndef _FCDDB_H_
#define _FCDDB_H_

#ifndef lint
static char     *_fcddb_h_ident_ = "@(#)fcddb.h	1.42 04/03/11";
#endif

#define XMCD_CDDB		/* Enable correct includes in CDDB2API.h */

#include "common_d/appenv.h"
#include "common_d/util.h"
#include "cddb_d/CDDB2API.h"

/* Debug output macro */
#define FCDDBDBG		if (fcddb_debug) (void) fprintf

/* Magic value to indicate that this is a CDDB1 implementation */
#define CDDB1_MAGIC		0x000cddb1

/* HTTP port number */
#define HTTP_PORT		3128

/* CDDB1 server host and CGI path */
#define CDDB_SERVER_HOST	"gnudb.gnudb.org"
#define CDDB_CGI_PATH		"/~cddb/cddb.cgi"
#define CDDB_SUBMIT_HOST	"gnudb.gnudb.org"
#define CDDB_SUBMIT_CGI_PATH	"/~cddb/submit.cgi"

/* Web browser invocation script name */
#define CDDB_GOBROWSER		"gobrowser"

#ifdef NOREMOTE
#undef SOCKS			/* NOREMOTE implies no SOCKS support */
#endif

/* SOCKS support */
#ifdef SOCKS
#define SOCKSINIT(x)		SOCKSinit(x)
#define SOCKET			socket
#define CONNECT			Rconnect
#else
#define SOCKSINIT(x)
#define SOCKET			socket
#define CONNECT			connect
#endif  /* SOCKS */


/*
 * Internal data structures
 */

/* Disc ID list structure */
typedef struct fcddb_idlist {
	word32_t		discid;		/* Disc ID */
	struct fcddb_idlist	*next;		/* Ptr to next elem */
} fcddb_idlist_t;

/* Bits for the flags field in fcddb_gmap_t */
#define CDDB_GMAP_DYN		0x1		/* Dynamically allocated */
#define CDDB_GMAP_DUP		0x2		/* Duplicate mapping */

/* CDDB1 -> CDDB2 genre mapping structure */
typedef struct fcddb_gmap {
	char			*cddb1name;	/* CDDB1 Category name */
	struct {
		char		*id;		/* CDDB2 meta-genre ID */
		char		*name;		/* CDDB2 meta-genre name */
	} meta;
	struct {
		char		*id;		/* CDDB2 sub-genre ID */
		char		*name;		/* CDDB2 sub-genre name */
	} sub;
	unsigned int		flags;		/* Flags */
	struct fcddb_gmap	*next;		/* Pointer to next element */
} fcddb_gmap_t;


/*
 * Internal Object structures
 */

/* CddbGenre structure */
typedef struct cddb_genre {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			id;			/* Genre id */
	CddbStr			name;			/* Genre name */
	struct cddb_genre	*nextmeta;		/* Ptr to next meta */
	struct cddb_genre	*next;			/* Ptr to next elem */
} cddb_genre_t;

/* CddbGenreList structure */
typedef struct cddb_genrelist {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_genre_t		*genres;		/* Genre list */
} cddb_genrelist_t;

/* CddbGenreTree structure */
typedef struct cddb_genretree {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_genre_t		*genres;		/* Genre list */
	long			count;			/* Number of elems */
} cddb_genretree_t;

/* CddbRole structure */
typedef struct cddb_role {
	char			objtype[STR_BUF_SZ];	/* Object type */
	char			*roletype;		/* Role type */
	CddbStr			id;			/* Role ID */
	CddbStr			name;			/* Role name */
	struct cddb_role	*next;			/* Ptr to next elem */
} cddb_role_t;

/* CddbRoleList structure */
typedef struct cddb_rolelist {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_role_t		*metarole;		/* Meta role */
	cddb_role_t		*subroles;		/* Role list */
	long			count;			/* Number of elems */
	struct cddb_rolelist	*next;			/* Ptr to next elem */
} cddb_rolelist_t;

/* CddbRoleTree structure */
typedef struct cddb_roletree {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_rolelist_t		*rolelists;		/* Role list */
	long			count;			/* Number of elems */
} cddb_roletree_t;

/* CddbLanguage structure */
typedef struct cddb_language {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			id;			/* Language ID */
	CddbStr			name;			/* Language name */
	struct cddb_language	*next;			/* Ptr to next elem */
} cddb_language_t;

/* CddbLanguages structure */
typedef struct cddb_languages {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_language_t		*languages;		/* Language list */
	long			count;			/* Number of elems */
} cddb_languages_t;

/* CddbLanguageList structure */
typedef struct cddb_languagelist {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_language_t		*languages;		/* Language list */
	long			count;			/* Number of elems */
} cddb_languagelist_t;

/* CddbRegion structure */
typedef struct cddb_region {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			id;			/* Region ID */
	CddbStr			name;			/* Region name */
	struct cddb_region	*next;			/* Ptr to next elem */
} cddb_region_t;

/* CddbRegionList structure */
typedef struct cddb_regionlist {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_region_t		*regions;		/* Region list */
	long			count;			/* Number of elems */
} cddb_regionlist_t;

/* CddbURL structure */
typedef struct cddb_url {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			type;			/* Type */
	CddbStr			href;			/* HREF */
	CddbStr			displaylink;		/* Display link */
	CddbStr			displaytext;		/* Display text */
	CddbStr			category;		/* Category */
	CddbStr			description;		/* Description */
	struct cddb_url		*next;			/* Ptr to next elem */
} cddb_url_t;

/* CddbURLList structure */
typedef struct cddb_urllist {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_url_t		*urls;			/* URL list */
	long			count;			/* Number of elems */
} cddb_urllist_t;

/* CddbURLTree structure */
typedef struct cddb_urltree {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_url_t		*urls;			/* URL list */
} cddb_urltree_t;

/* CddbURLManager structure */
typedef struct cddb_urlmanager {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_urllist_t		urllist;		/* URL list */
	void			*control;		/* Ptr back to ctrl */
} cddb_urlmanager_t;

/* CddbFullName structure */
typedef struct cddb_fullname {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			name;			/* Name */
	CddbStr			lastname;		/* Last name */
	CddbStr			firstname;		/* First name */
	CddbStr			the;			/* The */
} cddb_fullname_t;

/* CddbCredit structure */
typedef struct cddb_credit {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_role_t		*role;			/* Role */
	cddb_fullname_t		fullname;		/* Name */
	CddbStr			notes;			/* Notes */
	struct cddb_credit	*next;			/* Ptr to next elem */
} cddb_credit_t;

/* CddbCredits structure */
typedef struct cddb_credits {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_credit_t		*credits;		/* Credit list */
	long			count;			/* Number of elems */
} cddb_credits_t;

/* CddbSegment structure */
typedef struct cddb_segment {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			name;			/* Segment name */
	CddbStr			notes;			/* Segment notes */
	CddbStr			starttrack;		/* Start track */
	CddbStr			startframe;		/* Start frame */
	CddbStr			endtrack;		/* End track */
	CddbStr			endframe;		/* End frame */
	cddb_credits_t		credits;		/* Credits */
	void			*control;		/* Ptr back to ctrl */
	struct cddb_segment	*next;			/* Ptr to next elem */
} cddb_segment_t;

/* CddbSegments structure */
typedef struct cddb_segments {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_segment_t		*segments;		/* Segment list */
	long			count;			/* Number of elems */
} cddb_segments_t;

/* CddbUserInfo structure */
typedef struct cddb_userinfo {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			userhandle;		/* user handle */
	CddbStr			password;		/* password */
	CddbStr			passwordhint;		/* password hint */
	CddbStr			emailaddress;		/* email address */
	CddbStr			regionid;		/* Region ID */
	CddbStr			postalcode;		/* Postal code */
	CddbStr			age;			/* Age */
	CddbStr			sex;			/* Sex */
	CddbBoolean		allowemail;		/* Allow email */
	CddbBoolean		allowstats;		/* Allow stats */
} cddb_userinfo_t;

/* CddbOptions structure */
typedef struct cddb_options {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			proxyserver;		/* Proxy server */
	long			proxyport;		/* Proxy port */
	CddbStr			proxyusername;		/* Proxy user name */
	CddbStr			proxypassword;		/* Proxy password */
	long			servertimeout;		/* Server timeout */
	CddbBoolean		testsubmitmode;		/* Test submit mode */
	CddbStr			localcachepath;		/* Cache path */
	long			localcachetimeout;	/* Cache timeout */
	long			localcacheflags;	/* Cache flags */
} cddb_options_t;

/* CddbTrack structure */
typedef struct cddb_track {
	char			objtype[STR_BUF_SZ];	/* Object type */
	CddbStr			title;			/* Title */
	CddbStr			notes;			/* Notes */
	cddb_fullname_t		fullname;		/* Artist full name */
	cddb_credits_t		credits;		/* Credits */
	void			*control;		/* Ptr back to ctrl */
} cddb_track_t;

/* CddbTracks structure */
typedef struct cddb_tracks {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_track_t		track[MAXTRACK];	/* Tracks */
	long			count;			/* Track count */
} cddb_tracks_t;

/* CddbDisc structure */
typedef struct cddb_disc {
	char			objtype[STR_BUF_SZ];	/* Object type */
	char			*category;		/* CDDB1 category */
	char			*discid;		/* Disc ID */
	CddbStr			toc;			/* TOC string */
	CddbStr			title;			/* Title */
	CddbStr			notes;			/* Notes */
	char			*revision;		/* Revision */
	char			*submitter;		/* Submitter */
	cddb_fullname_t		fullname;		/* Artist full name */
	cddb_tracks_t		tracks;			/* Tracks */
	cddb_credits_t		credits;		/* Credits */
	cddb_segments_t		segments;		/* Segments */
	cddb_genre_t		*genre;			/* Genre */
	cddb_urllist_t		urllist;		/* URL list */
	void			*control;		/* Ptr back to ctrl */
	fcddb_idlist_t		*idlist;		/* ID list */
	struct cddb_disc	*next;			/* Ptr to next elem */
} cddb_disc_t;

/* CddbDiscs structure */
typedef struct cddb_discs {
	char			objtype[STR_BUF_SZ];	/* Object type */
	cddb_disc_t		*discs;			/* Disc list */
	long			count;			/* Number of elems */
} cddb_discs_t;

/* CddbControl structure */
typedef struct cddb_control {
	char			objtype[STR_BUF_SZ];	/* Object type */
	word32_t		magic;			/* Magic number */
	char			*clientname;		/* Client name */
	char			*clientid;		/* Client ID */
	char			*clientver;		/* Client version */
	char			*hostname;		/* Hostname */
	cddb_options_t		options;		/* Options */
	cddb_userinfo_t		userinfo;		/* Userinfo */
	cddb_regionlist_t	regionlist;		/* Region list */
	cddb_languagelist_t	languagelist;		/* Language list */
	cddb_genretree_t 	genretree;		/* Genre tree */
	cddb_roletree_t 	roletree;		/* Role tree */
	cddb_urllist_t		urllist;		/* URL list */
	cddb_discs_t		discs;			/* Discs */
	cddb_disc_t		disc;			/* Disc */
} cddb_control_t;


/*
 * Support functions
 */
extern void		fcddb_onsig(int);
extern void		(*fcddb_signal(int , void (*)(int)))(int);
extern CddbResult	fcddb_init(cddb_control_t *);
extern CddbResult	fcddb_halt(cddb_control_t *);
extern void		*fcddb_obj_alloc(char *, size_t);
extern void		fcddb_obj_free(void *);
extern int		fcddb_runcmd(char *);
extern CddbResult	fcddb_set_auth(char *, char *);
extern char		*fcddb_strdup(char *);
extern char		*fcddb_basename(char *);
extern char		*fcddb_dirname(char *);
extern CddbResult	fcddb_mkdir(char *, mode_t);
extern char		*fcddb_username(void);
extern char		*fcddb_homedir(void);
extern char		*fcddb_hostname(void);
extern char		*fcddb_genre_id2categ(char *);
extern cddb_genre_t	*fcddb_genre_id2gp(cddb_control_t *, char *);
extern char		*fcddb_genre_categ2id(char *);
extern cddb_genre_t	*fcddb_genre_categ2gp(cddb_control_t *, char *);
extern int		fcddb_parse_toc(char *, unsigned int *);
extern char		*fcddb_discid(int, unsigned int *);
extern CddbResult	fcddb_read_cddb(cddb_control_t *, char *,
					char *, char *, char *,
					unsigned short, bool_t);
extern CddbResult	fcddb_lookup_cddb(cddb_control_t *, char *,
					unsigned int *, char *, char *,
					unsigned short, bool_t, int *);
extern CddbResult	fcddb_submit_cddb(cddb_control_t *, cddb_disc_t *,
					char *, unsigned short, bool_t);
extern CddbResult	fcddb_flush_cddb(cddb_control_t *, CDDBFlushFlags);

#endif	/* _FCDDB_H_ */

