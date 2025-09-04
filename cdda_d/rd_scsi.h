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
 *   SCSI pass-through CDDA read support
 */
#ifndef	__RD_SCSI_H__
#define	__RD_SCSI_H__

#ifndef lint
static char *_rd_scsi_h_ident_ = "@(#)rd_scsi.h	7.12 03/12/12";
#endif

#if defined(CDDA_RD_SCSIPT) && defined(CDDA_SUPPORTED)

/* Exported function prototypes */
extern word32_t		scsi_rinit(void);
extern bool_t		scsi_read(di_dev_t *);
extern void		scsi_rdone(bool_t);
extern void		scsi_rinfo(char *);

#else

#define scsi_rinit	NULL
#define scsi_read	NULL
#define scsi_rdone	NULL
#define scsi_rinfo	NULL

#endif	/* CDDA_RD_SCSIPT CDDA_SUPPORTED */

#endif	/* __RD_SCSI_H__ */

