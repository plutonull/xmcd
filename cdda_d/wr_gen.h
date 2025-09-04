/*
 *   cdda - CD Digital Audio support
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
 */
#ifndef __WR_GEN_H__
#define __WR_GEN_H__

#ifndef lint
static char *_wr_gen_h_ident_ = "@(#)wr_gen.h	7.41 04/02/02";
#endif

#ifdef CDDA_SUPPORTED

/* Definitions used for the flags field of gen_desc_t */
#define GDESC_WRITEOUT	0x1	/* Force a write of an encoded audio buffer */
#define GDESC_ISPIPE	0x2	/* The output stream is to a pipe */

/* Default buffer size for buffered file I/O */
#define GDESC_BUFSZ	(16 * 1024)

/* Data structure used in most of the functions in this module */
typedef struct gen_desc {
	cd_state_t	*cdp;
	char		*path;
	int		fd;
	int		fmt;
	int		flags;
	mode_t		mode;
	size_t		datalen;
	pid_t		cpid;
	FILE		*fp;
	char		*buf;
	void		*aux;
} gen_desc_t;


/* Function prototypes */
extern char		*gen_esc_shellquote(gen_desc_t *, char *);
extern char		*gen_genre_cddb2id3(char *);
extern bool_t		gen_set_eid(cd_state_t *);
extern bool_t		gen_reset_eid(cd_state_t *);
extern gen_desc_t	*gen_open_file(cd_state_t *, char *, int, mode_t,
				       int, size_t);
extern bool_t		gen_close_file(gen_desc_t *);
extern gen_desc_t	*gen_open_pipe(cd_state_t *, char *, pid_t *,
				       int, size_t);
extern bool_t		gen_close_pipe(gen_desc_t *);
extern bool_t		gen_read_chunk(gen_desc_t *, byte_t *, size_t);
extern bool_t		gen_write_chunk(gen_desc_t *, byte_t *, size_t);
extern bool_t		gen_seek(gen_desc_t *, off_t, int);
#ifndef __VMS
extern bool_t		gen_ioctl(gen_desc_t *, int, void *);
#endif
extern void		gen_byteswap(byte_t *, byte_t *, size_t);
extern bool_t		gen_filefmt_be(int);
extern void		gen_chroute_att(int, int, sword16_t *, size_t);
extern void		gen_write_init(void);

#endif	/* CDDA_SUPPORTED */

#endif	/* __WR_GEN_H__ */

