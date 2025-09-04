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

#ifndef	__AIFF_H__
#define	__AIFF_H__

#ifndef lint
static char *_aiff_h_ident_ = "@(#)aiff.h	7.4 03/12/12";
#endif

/*
 * AIFF
 */
#define AIFF_HDRSZ	54

typedef struct aiff_filehdr {
	char		a_form[4];		/* "FORM" */
	byte_t		a_length[4];
	char		a_aiff[4];		/* "AIFF" */

	char		c_comm[4];		/* "COMM" */
	byte_t		c_length[4];
	byte_t		c_channels[2];
	byte_t		c_frames[4];
	byte_t		c_sample_size[2];
	char		c_sample_rate[10];	/* IEEE 80-bit float point */

	char		s_ssnd[4];		/* "SSND" */
	byte_t		s_length[4];
	byte_t		s_offset[4];
	byte_t		s_blocksize[4];
} aiff_filehdr_t;

/*
 * AIFF-C
 */
#define AIFC_HDRSZ	86

typedef struct aifc_filehdr {
	char		a_form[4];		/* "FORM" */
	byte_t		a_length[4];
	char		a_aifc[4];		/* "AIFC" */
	
	char		f_fver[4];		/* "FVER" */
	byte_t		f_length[4];
	byte_t		f_version[4];

	char		c_comm[4];		/* "COMM" */
	byte_t		c_length[4];
	byte_t		c_channels[2];
	byte_t		c_frames[4];
	byte_t		c_sample_size[2];
	char		c_sample_rate[10];	/* IEEE 80-bit float point */
	char		c_comptype[4];		/* "NONE" */
	byte_t		c_complength;
	char		c_compstr[14];		/* "not compressed" */
	byte_t		c_pad;

	char		s_ssnd[4];		/* "SSND" */
	byte_t		s_length[4];
	byte_t		s_offset[4];
	byte_t		s_blocksize[4];
} aifc_filehdr_t;

#endif	/* __AIFF_H__ */

