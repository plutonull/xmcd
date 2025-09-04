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
#ifndef __CDINFO_H__
#define __CDINFO_H__

#ifndef lint
static char *_cdinfo_h_ident_ = "@(#)cdinfo.h	7.99 04/04/20";
#endif


/* Macros to extract info from cdinfo_ret_t */
#define CDINFO_SET_CODE(stat, arg)	((stat) | (arg) << 16)
#define CDINFO_GET_STAT(code)		((code) & 0xffff)
#define CDINFO_GET_ARG(code)		((code) >> 16)

#if defined(__VMS) && !defined(SYNCHRONOUS)
#define SYNCHRONOUS		/* VMS must always run in SYNCHRONOUS mode */
#endif

#ifdef SYNCHRONOUS
#define CH_RET(code)						\
	{							\
		if ((code) == 0)				\
			errno = 0;				\
		return CDINFO_SET_CODE((code), (errno));	\
	}
#else
#define CH_RET(code)		_exit((code))
#endif

/* Error status codes */
#define OPEN_ERR		50
#define CLOSE_ERR		51
#define SETUID_ERR		52
#define READ_ERR		53
#define WRITE_ERR		54
#define LINK_ERR		55
#define FORK_ERR		56
#define KILLED_ERR		57
#define WAIT_ERR		58
#define MEM_ERR			60
#define CMD_ERR			61
#define AUTH_ERR		62
#define FLUSH_ERR		63
#define INIT_ERR		64
#define OFLN_ERR		65
#define REGI_ERR		66
#define NAME_ERR		67
#define HINT_ERR		68
#define MAIL_ERR		69
#define BUSY_ERR		70
#define ARG_ERR			71
#define SUBMIT_ERR		72
#define LIBCDDB_ERR		73

/* Default HTTP port */
#ifndef HTTP_PORT
#define HTTP_PORT		80
#endif

/* cdinfoPath and XMCD_CDINFOPATH component separator character */
#define CDINFOPATH_SEPCHAR	';'

/* Web browser invocation script name */
#define CDINFO_GOBROWSER	"gobrowser"


/* Message dialog macros */
#define CDINFO_FATAL(msg)		{				\
	if (cdinfo_clinfo->fatal_msg != NULL && !cdinfo_ischild)	\
		cdinfo_clinfo->fatal_msg(app_data.str_fatal, (msg));	\
	else {								\
		(void) fprintf(errfp, "Fatal: %s\n", (msg));		\
		_exit(1);						\
	}								\
}
#define CDINFO_WARNING(msg)		{				\
	if (cdinfo_clinfo->warning_msg != NULL && !cdinfo_ischild)	\
		cdinfo_clinfo->warning_msg(app_data.str_warning, (msg));\
	else								\
		(void) fprintf(errfp, "Warning: %s\n", (msg));		\
}
#define CDINFO_INFO(msg)		{				\
	if (cdinfo_clinfo->info_msg != NULL && !cdinfo_ischild)		\
		cdinfo_clinfo->info_msg(app_data.str_info, (msg));	\
	else								\
		(void) fprintf(errfp, "Info: %s\n", (msg));		\
}
#define CDINFO2_INFO(msg, txt)		{				\
	if (cdinfo_clinfo->info2_msg != NULL && !cdinfo_ischild)	\
		cdinfo_clinfo->info2_msg(app_data.str_info,(msg),(txt));\
	else								\
		(void) fprintf(errfp, "Info: %s\n%s\n", (msg), (txt));	\
}


#define XMCD_CLIENT_ID	"8885248"		/* Client ID */
#define Cddb_ISCDDB1	0x000cddb1		/* CDDB1 server interface */
#define CDINFO_PRODUCTION			/* Enable CDDB submit */


/* cdinfo services return type */
typedef int		cdinfo_ret_t;


/* Client information structure */
typedef struct {
	char	prog[FILE_PATH_SZ];		/* Client program name */
	char	host[HOST_NAM_SZ];		/* Client host name */
	char	user[STR_BUF_SZ];		/* Client user name */
	bool_t	(*isdemo)(void);		/* Demo mode func */
	curstat_t * (*curstat_addr)(void);	/* Current status structure */
	void	(*fatal_msg)(char *, char *);	/* Fatal message popup func */
	void	(*warning_msg)(char *, char *);	/* Warning message popup func */
	void	(*info_msg)(char *, char *);	/* Info message popup func */
	void	(*info2_msg)(char *, char *, char *);
						/* Info2 message popup func */
	void	(*workproc)(int);		/* Function to run while
						 * waiting for I/O
						 */
	int	arg;				/* Argument to workproc */

} cdinfo_client_t;


/* Definitions for the type field in cdinfo_path_t */
#define CDINFO_LOC	1
#define CDINFO_RMT	2
#define CDINFO_CDTEXT	3

/* CD info path structure */
typedef struct cdinfo_path {
	int		type;			/* CDINFO_LOC or CDINFO_RMT */
	char		*categ;			/* CDDB1-style category */
	char		*path;			/* File path */
	struct cdinfo_path *next;		/* Link to next path */
} cdinfo_path_t;


/* Full name info structure */
typedef struct {
	char		*dispname;		/* display name */
	char		*lastname;		/* last name */
	char		*firstname;		/* first name */
	char		*the;			/* "The" */
} cdinfo_fname_t;


/* Disc information structure */
/* CD info entry multiple match list structure */
typedef struct cdinfo_match {
	char		*artist;		/* CD artist of match */
	char		*title;			/* CD title of match */
	char		*genre;			/* CD genre */
	long		tag;			/* Selection tag */
	struct cdinfo_match *next;		/* Link to next match */
} cdinfo_match_t;


/* CD genre structure */
typedef struct cdinfo_genre {
	char		*id;			/* Genre ID */
	char		*name;			/* Genre name */
	void		*aux;			/* Auxiliary field */
	void		*aux2;			/* Auxiliary field */
	void		*aux3;			/* Auxiliary field */
	void		*aux4;			/* Auxiliary field */
	struct cdinfo_genre *child;		/* Child-genre pointer */
	struct cdinfo_genre *parent;		/* Parent-genre pointer */
	struct cdinfo_genre *next;		/* Next genre pointer */
} cdinfo_genre_t;


/* Region structure */
typedef struct cdinfo_region {
	char		*id;			/* Region ID */
	char		*name;			/* Region name */
	struct cdinfo_region *next;		/* Next region pointer */
} cdinfo_region_t;


/* Language structure */
typedef struct cdinfo_lang {
	char		*id;			/* Language ID */
	char		*name;			/* Language name */
	struct cdinfo_lang *next;		/* Next language pointer */
} cdinfo_lang_t;


/* Role structure */
typedef struct cdinfo_role {
	char		*id;			/* Role ID */
	char		*name;			/* Role name */
	void		*aux;			/* Auxiliary field */
	struct cdinfo_role *child;		/* Child-role pointer */
	struct cdinfo_role *parent;		/* Parent-role pointer */
	struct cdinfo_role *next;		/* Next role pointer */
} cdinfo_role_t;


/* Credit structure */
typedef struct cdinfo_credit {
	struct {
		cdinfo_role_t	*role;		/* Role */
		char		*name;		/* Name */
		cdinfo_fname_t	fullname;	/* Full name */
	}		crinfo;			/* Credit information */
	char		*notes;			/* Credit notes */
	struct cdinfo_credit *prev;		/* Prev credit pointer */
	struct cdinfo_credit *next;		/* Next credit pointer */
} cdinfo_credit_t;


/* Segment structure */
typedef struct cdinfo_segment {
	char		*name;			/* Name */
	char		*notes;			/* Notes */
	char		*start_track;		/* Start track */
	char		*start_frame;		/* Start Frame */
	char		*end_track;		/* End track */
	char		*end_frame;		/* End Frame */
	cdinfo_credit_t	*credit_list;		/* Credits list */
	struct cdinfo_segment *prev;		/* Prev segment pointer */
	struct cdinfo_segment *next;		/* Next segment pointer */
} cdinfo_segment_t;


/* User registration structure */
typedef struct {
	char	*handle;			/* User handle */
	char	*passwd;			/* password */
	char	*vpasswd;			/* verify password */
	char	*hint;				/* Password hint */
	char	*email;				/* E-mail address */
	char	*region;			/* Region */
	char	*postal;			/* Postal code */
	char	*age;				/* Age */
	char	*gender;			/* Gender */
	bool_t	allowemail;			/* Allow e-mail */
	bool_t	allowstats;			/* Allow stats */
} cdinfo_ureg_t;


/* CDDB handle structure */
typedef struct {
	void		*ctrlp;			/* CDDB control pointer */
} cdinfo_cddb_t;


typedef struct {
	bool_t		compilation;		/* Is compilation CD */
	cdinfo_fname_t	artistfname;		/* Artist Full name */
	char		*artist;		/* Artist */
	char		*title;			/* Title */
	char		*sorttitle;		/* Sort title */
	char		*title_the;		/* Sort title "The" */
	char		*year;			/* Year */
	char		*label;			/* Label */
	char		*genre;			/* Primary genre ID */
	char		*genre2;		/* Secondary genre ID */
	char		*dnum;			/* Disc number in set */
	char		*tnum;			/* Number of discs in set */
	char		*region;		/* Region ID */
	char		*notes;			/* Disc notes */
	char		*mediaid;		/* Media ID */
	char		*muiid;			/* Media unique ID */
	char		*titleuid;		/* Title unique ID */
	char		*revision;		/* Revision */
	char		*revtag;		/* Revision tag */
	char		*certifier;		/* Certifier */
	char		*lang;			/* language ID */
	cdinfo_credit_t	*credit_list;		/* Credits list */
	cdinfo_segment_t *segment_list;		/* Segments list */
} cdinfo_disc_t;


/* Track information structure */
typedef struct {
	cdinfo_fname_t	artistfname;		/* Artist Full name */
	char		*artist;		/* Track artist */
	char		*title;			/* Track title */
	char		*sorttitle;		/* Sort title */
	char		*title_the;		/* Sort title "The" */
	char		*year;			/* Year */
	char		*label;			/* Label */
	char		*genre;			/* Primary genre */
	char		*genre2;		/* Secondary genre */
	char		*bpm;			/* Beats per minute */
	char		*notes;			/* Track notes */
	char		*isrc;			/* Intl Std Recording Code */
	cdinfo_credit_t	*credit_list;		/* Credits list */
} cdinfo_track_t;


/* wwwwarp.cfg file name */
#define WWWWARP_CFG	"wwwwarp.cfg"

/* Definitions for the type field in w_ent_t */
#define WTYPE_NULL	0
#define WTYPE_TITLE	1
#define WTYPE_SEP	2
#define WTYPE_SEP2	3
#define WTYPE_SUBMENU	4
#define WTYPE_GOTO	5
#define WTYPE_ABOUT	6
#define WTYPE_HELP	7
#define WTYPE_MOTD	8
#define WTYPE_DISCOG	9
#define WTYPE_BROWSER	10
#define WTYPE_GEN	11
#define WTYPE_ALBUM	12
#define WTYPE_SUBMITURL	13

/* wwwwarp menu entry structure */
typedef struct w_ent {
	int		type;		/* Entry type */
	char		*name;		/* Menu name */
	char		*desc;		/* Entry description/label */
	char		*arg;		/* Entry argument */
	char		*modifier;	/* Shortcut key modifier */
	char		*keyname;	/* Shortcut key name */
	url_attrib_t	attrib;		/* URL attribute */
	void		*aux;		/* Auxiliary field */
	struct w_ent	*submenu;	/* Submenu pointer */
	struct w_ent	*nextent;	/* Next entry pointer */
	struct w_ent	*nextmenu;	/* Next menu pointer */
	struct w_ent	*chknext;	/* Link pointer for circular check */
} w_ent_t;


/* CDDB URL information structure - Note: No URL is actually stored in
 * this structure.  It's handled internally by CDDB via an index number.
 */
typedef struct cdinfo_url {
	int		wtype;			/* WTYPE_GEN or WTYPE_ALBUM */
	char		*type;			/* Type */
	char		*href;			/* URL string */
	char		*displink;		/* Display link */
	char		*disptext;		/* Display text */
	char		*categ;			/* Category */
	char		*desc;			/* Description */
	char		*size;			/* Size */
	char		*weight;		/* Weight */
	char		*modifier;		/* Shortcut key modifier */
	char		*keyname;		/* Shortcut key name */
	void		*aux;			/* Auxiliary */
	struct cdinfo_url *next;		/* Next URL */
} cdinfo_url_t;


/* Pipe data direction definition (for cdinfo_openpipe) */
#define CDINFO_DATAIN	0
#define CDINFO_DATAOUT	1

#define XMCD_PIPESIG	"%_XmCd_% "
#define CDINFO_CACHESZ	1024

/* CD info pipe info structure */
typedef struct {
	int		fd;			/* File descriptor */
	int		rw;			/* O_RDONLY or O_WRONLY */
	int		pos;			/* Cache position */
	int		cnt;			/* Cache count */
	unsigned char	*cache;			/* I/O cache */
} cdinfo_pinfo_t;

/* CD info pipe structure */
typedef struct {
	int		dir;			/* Pipe data direction */
	cdinfo_pinfo_t	r;			/* Read pipe */
	cdinfo_pinfo_t	w;			/* Write pipe */
} cdinfo_pipe_t;


/* Flags bits for cdinfo_incore_t */
#define CDINFO_CHANGED	0x0001			/* In-core info was edited */
#define CDINFO_MATCH	0x0002			/* CD info load success */
#define CDINFO_NEEDREG	0x0004			/* CDDB need registration */
#define CDINFO_FROMLOC	0x0008			/* CD info from local DB */
#define CDINFO_FROMCDT	0x0010			/* CD info from CD-TEXT */

/* In-core CD database info structure */
typedef struct {
	word32_t	discid;			/* Integer disc identifier */
	int		flags;			/* Flags */

	cdinfo_disc_t	disc;			/* Disk information */
	cdinfo_track_t	track[MAXTRACK];	/* Track information */
	cdinfo_ureg_t	userreg;		/* User registration info */

	cdinfo_lang_t	*langlist;		/* CDDB language list */
	cdinfo_genre_t	*genrelist;		/* CDDB Genre list */
	cdinfo_region_t	*regionlist;		/* CDDB Region list */
	cdinfo_role_t	*rolelist;		/* CDDB Role list */
	cdinfo_match_t	*matchlist;		/* CDDB match list */
	void		*match_aux;		/* Save CddbDiscsPtr */
	long		match_tag;		/* user selection tag */

	w_ent_t		*wwwwarp_list;		/* wwwWarp menu list */
	cdinfo_url_t	*gen_url_list;		/* general URLs list */
	cdinfo_url_t	*disc_url_list;		/* album-related URLs list */

	cdinfo_path_t	*pathlist;		/* CD info path list */

	char		*playorder;		/* Track play order */

	cdinfo_cddb_t	*sav_cddbp;		/* Save CDDB pointer */
	cdinfo_pipe_t	*sav_rpp;		/* Save pipe pointer */
	cdinfo_pipe_t	*sav_spp;		/* Save pipe pointer */

	char		*proxy_user;		/* Proxy server user name */
	char		*proxy_passwd;		/* Proxy server password */

	char		*ctrl_ver;		/* CDDB control version */

	di_cdtext_t	cdtext;			/* CD-TEXT information */

	char		*cddb_errstr;		/* CDDB error message string */
} cdinfo_incore_t;


#define DLIST_BUF_SZ	128


/* Defines for the type field in cdinfo_dlist_t.  This is made to be
 * somewhat compatible with xmcd-2.x.
 */
#define CDINFO_DLIST_LOCAL	1		/* Local CD database entry */
#define CDINFO_DLIST_REMOTE1	2		/* CDDBP-queried entry */
#define CDINFO_DLIST_REMOTE	3		/* HTTP-queried entry */

/* Disc list information structure */
typedef struct cdinfo_dlist {
	char		*device;		/* Device */
	int		discno;			/* Disc number */
	int		type;			/* Type flags */
	word32_t	discid;			/* Integer disc identifier */
	char		*genre;			/* CD genre */
	char		*artist;		/* CD artist */
	char		*title;			/* CD title */
	time_t		time;			/* Time */
	void		*aux;			/* Auxiliary pointer */
	struct cdinfo_dlist *prev;		/* Prev entry on list */
	struct cdinfo_dlist *next;		/* Next entry on list */
} cdinfo_dlist_t;


/* Private functions: for libcdinfo internal use only */
#ifdef _CDINFO_INTERN
extern void		cdinfo_onterm(int);
extern int		cdinfo_sum(int);
extern bool_t		cdinfo_forkwait(cdinfo_ret_t *);
#ifndef SYNCHRONOUS
extern cdinfo_pipe_t	*cdinfo_openpipe(int);
extern bool_t		cdinfo_closepipe(cdinfo_pipe_t *);
extern bool_t           cdinfo_write_datapipe(cdinfo_pipe_t *, curstat_t *);
extern bool_t           cdinfo_read_datapipe(cdinfo_pipe_t *);
extern bool_t           cdinfo_write_selpipe(cdinfo_pipe_t *);
extern bool_t           cdinfo_read_selpipe(cdinfo_pipe_t *);
#endif
extern cdinfo_cddb_t	*cdinfo_opencddb(curstat_t *, bool_t, cdinfo_ret_t *);
extern bool_t		cdinfo_closecddb(cdinfo_cddb_t *);
extern bool_t		cdinfo_initcddb(cdinfo_cddb_t *, cdinfo_ret_t *);
extern bool_t		cdinfo_querycddb(cdinfo_cddb_t *, curstat_t *,
					 cdinfo_ret_t *);
extern bool_t		cdinfo_uregcddb(cdinfo_cddb_t *, cdinfo_ret_t *);
extern bool_t		cdinfo_passhintcddb(cdinfo_cddb_t *, cdinfo_ret_t *);
extern bool_t		cdinfo_submitcddb(cdinfo_cddb_t *, curstat_t *s,
					  cdinfo_ret_t *);
extern bool_t		cdinfo_submiturlcddb(cdinfo_cddb_t *, cdinfo_url_t *,
					     cdinfo_ret_t *);
extern bool_t		cdinfo_flushcddb(cdinfo_cddb_t *);
extern bool_t		cdinfo_infobrowsercddb(cdinfo_cddb_t *);
extern bool_t		cdinfo_urlcddb(cdinfo_cddb_t *, int, int);
extern void		cdinfo_free_matchlist(void);
extern char		*cdinfo_skip_whitespace(char *);
extern char		*cdinfo_skip_nowhitespace(char *);
extern char		*cdinfo_fgetline(FILE *);
extern void		cdinfo_wwwchk_cleanup(void);
extern bool_t		cdinfo_wwwmenu_chk(w_ent_t *, bool_t);
extern void		cdinfo_scan_url_attrib(char *, url_attrib_t *);
extern bool_t		cdinfo_add_pathent(char *);
extern bool_t		cdinfo_load_locdb(char *, char *, curstat_t *, int *);
extern bool_t		cdinfo_out_discog(char *, curstat_t *, char *);
extern void		cdinfo_map_cdtext(curstat_t *, di_cdtext_t *);

/* The following two funcs are in libcddbkey */
extern bool_t		cddb_ifver(void);
extern bool_t		cddb_setkey(cdinfo_cddb_t *, cdinfo_client_t *,
				    appdata_t *, curstat_t *, FILE *);

#endif /* _CDINFO_INTERN */


/* Public functions */
extern cdinfo_incore_t	*cdinfo_addr(void);
extern word32_t		cdinfo_discid(curstat_t *);
extern char		*cdinfo_txtreduce(char *, bool_t);
extern void		cdinfo_tmpl_to_url(curstat_t *, char *, char *, int);
extern int		cdinfo_url_len(char *, url_attrib_t *, int *);
extern void		cdinfo_init(cdinfo_client_t *);
extern void		cdinfo_halt(curstat_t *);
extern void		cdinfo_reinit(void);
extern cdinfo_ret_t	cdinfo_submit(curstat_t *);
extern cdinfo_ret_t	cdinfo_submit_url(curstat_t *, cdinfo_url_t *);
extern cdinfo_ret_t	cdinfo_flush(curstat_t *);
extern cdinfo_ret_t	cdinfo_offline(curstat_t *);
extern cdinfo_ret_t	cdinfo_load(curstat_t *);
extern cdinfo_ret_t	cdinfo_load_matchsel(curstat_t *);
extern cdinfo_ret_t	cdinfo_load_prog(curstat_t *);
extern cdinfo_ret_t	cdinfo_save_prog(curstat_t *);
extern cdinfo_ret_t	cdinfo_del_prog(void);
extern cdinfo_ret_t	cdinfo_reguser(curstat_t *s);
extern cdinfo_ret_t	cdinfo_getpasshint(curstat_t *s);
extern cdinfo_ret_t	cdinfo_go_musicbrowser(curstat_t *);
extern cdinfo_ret_t	cdinfo_go_url(char *url);
extern cdinfo_ret_t	cdinfo_go_cddburl(curstat_t *, int, int);
extern cdinfo_ret_t	cdinfo_gen_discog(curstat_t *);
extern void		cdinfo_clear(bool_t);
extern void		cdinfo_hist_init(void);
extern cdinfo_ret_t	cdinfo_hist_addent(cdinfo_dlist_t *, bool_t);
extern cdinfo_ret_t	cdinfo_hist_delent(cdinfo_dlist_t *, bool_t);
extern cdinfo_ret_t	cdinfo_hist_delall(bool_t);
extern cdinfo_dlist_t	*cdinfo_hist_list(void);
extern void		cdinfo_chgr_init(void);
extern cdinfo_ret_t	cdinfo_chgr_addent(cdinfo_dlist_t *);
extern cdinfo_dlist_t	*cdinfo_chgr_list(void);
extern cdinfo_genre_t	*cdinfo_genre(char *);
extern char		*cdinfo_genre_name(char *);
extern char		*cdinfo_genre_path(char *);
extern char		*cdinfo_region_name(char *);
extern char		*cdinfo_lang_name(char *);
extern cdinfo_role_t	*cdinfo_role(char *);
extern char		*cdinfo_role_name(char *);
extern void		cdinfo_load_cancel(void);
extern int		cdinfo_cddb_ver(void);
extern char		*cdinfo_cddbctrl_ver(void);
extern bool_t		cdinfo_cddb_iscfg(void);
extern bool_t		cdinfo_cdtext_iscfg(void);
extern void		cdinfo_wwwwarp_parmload(void);
extern bool_t		cdinfo_wwwwarp_ckkey(char *);
extern void		cdinfo_curfileupd(void);
extern bool_t		cdinfo_issync(void);
extern void		cdinfo_dump_incore(curstat_t *s);

#endif	/* __CDINFO_H__ */

