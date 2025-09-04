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

/*
 *   SGI IRIX sound driver support
 */
#ifndef	__WR_IRIX_H__
#define	__WR_IRIX_H__

#ifndef lint
static char *_wr_irix_h_ident_ = "@(#)wr_irix.h	7.13 04/02/10";
#endif

#if defined(CDDA_WR_IRIX) && defined(CDDA_SUPPORTED)


/*
 * Detect IRIX audio library version and map to appropriate API
 */
#ifdef AL_PARAM_BIT
/* New API */
#define NEW_IRIX_AUDIO_API

#define al_new_config		alNewConfig
#define al_free_config		alFreeConfig
#define al_set_samp_fmt		alSetSampFmt
#define al_set_width		alSetWidth
#define al_set_channels		alSetChannels
#define al_set_queuesize	alSetQueueSize
#define al_set_params		alSetParams
#define al_get_params		alGetParams
#define al_open_port		alOpenPort
#define al_close_port		alClosePort
#define al_get_filled		alGetFilled
#define al_write_samps(p,d,n)	alWriteFrames((p), (d), (n) >> 1)

typedef int			al_device_t;
typedef ALpv			al_param_t;
typedef ALfixed			al_fixed_t;
typedef double			al_hwvol_t;

#else
/* Old API */

#define al_new_config		ALnewconfig
#define al_free_config		ALfreeconfig
#define al_set_samp_fmt		ALsetsampfmt
#define al_set_width		ALsetwidth
#define al_set_channels		ALsetchannels
#define al_set_queuesize	ALsetqueuesize
#define al_set_params		ALsetparams
#define al_get_params		ALgetparams
#define al_open_port		ALopenport
#define al_close_port		ALcloseport
#define al_get_filled		ALgetfilled
#define al_write_samps		ALwritesamps

typedef long			al_device_t;
typedef long			al_param_t;
typedef long			al_fixed_t;
typedef long			al_hwvol_t;

#endif	/* NEW_IRIX_AUDIO_LIB */


/* Exported function prototypes */
extern word32_t	irix_winit(void);
extern bool_t	irix_write(curstat_t *);
extern void	irix_wdone(bool_t);
extern void	irix_winfo(char *);

#else

#define irix_winit	NULL
#define irix_write	NULL
#define irix_wdone	NULL
#define irix_winfo	NULL

#endif	/* CDDA_WR_IRIX CDDA_SUPPORTED */

#endif	/* __WR_IRIX_H__ */

