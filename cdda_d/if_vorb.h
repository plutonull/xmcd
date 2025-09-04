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
#ifndef lint
static char *_if_vorb_h_ident_ = "@(#)if_vorb.h	7.3 03/12/12";
#endif

#ifndef __IF_VORB_H__
#define __IF_VORB_H__

#if defined(CDDA_SUPPORTED) && defined(HAS_VORBIS)

extern bool_t	if_vorbis_init(gen_desc_t *);
extern bool_t	if_vorbis_encode_chunk(gen_desc_t *, byte_t *, size_t);
extern void	if_vorbis_halt(gen_desc_t *);

#endif

#endif	/* __IF_VORB_H__ */

