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

#ifndef	__WAV_H__
#define	__WAV_H__

#ifndef lint
static char *_wav_h_ident_ = "@(#)wav.h	7.7 03/12/12";
#endif

typedef struct wav_filehdr {
	char		r_riff[4];		/* RIFF chunk */
	word32_t	r_length;
	char		r_wave[4];
	char		f_format[4];		/* FORMAT chunk */
	word32_t	f_length;
	word16_t	f_const;
	word16_t	f_channels;
	word32_t	f_sample_rate;
	word32_t	f_bytes_per_s;
	word16_t	f_block_align;
	word16_t	f_bits_per_sample;
	char		d_data[4];		/* DATA chunk */
	word32_t	d_length;
} wav_filehdr_t;

#endif	/* __WAV_H__ */

