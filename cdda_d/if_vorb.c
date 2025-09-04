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
static char *_if_vorb_c_ident_ = "@(#)if_vorb.c	7.11 03/12/12";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"

#if defined(CDDA_SUPPORTED) && defined(HAS_VORBIS)

#include "cdinfo_d/cdinfo.h"
#include "cdda_d/wr_gen.h"
#include "cdda_d/if_faac.h"
#include <vorbis/vorbisenc.h>
#include <vorbis/codec.h>


extern appdata_t	app_data;
extern FILE		*errfp;
extern cdda_client_t	*cdda_clinfo;

extern char		*tagcomment;	/* Tag comment */


/* Ogg Vorbis descriptor structure */
typedef struct {
	ogg_stream_state	os;
	vorbis_comment		vc;
	vorbis_info		vi;
	vorbis_dsp_state	vd;
	vorbis_block		vb;
	int			serno;
} ov_desc_t;


/*
 * if_vorbis_addtags
 *	Initialize and fill the Vorbis comment information with CD
 *	information data.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
if_vorbis_addtags(gen_desc_t *gdp)
{
	ov_desc_t	*ovdesc = (ov_desc_t *) gdp->aux;
	curstat_t	*s = cdda_clinfo->curstat_addr();
	cdinfo_incore_t	*cdp = cdinfo_addr();
	int		i,
			idx = (int) gdp->cdp->i->trk_idx;
	char		tmpstr[32];

	if (ovdesc == NULL)
		return;

	DBGPRN(DBG_SND)(errfp, "\nSetting comment tags for %s\n", gdp->path);

	vorbis_comment_init(&ovdesc->vc);

	/* Vorbis comments should be encoded in UTF-8 so we don't need
	 * any conversion here.
	 */

	if (cdp->disc.title != NULL)
		vorbis_comment_add_tag(&ovdesc->vc, "ALBUM", cdp->disc.title);

	if (app_data.cdda_trkfile && cdp->track[idx].artist != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "ARTIST",
			cdp->track[idx].artist
		);
	}
	else if (cdp->disc.artist != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "ARTIST",
			cdp->disc.artist
		);
	}

	if (app_data.cdda_trkfile && cdp->track[idx].title != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "TITLE",
			cdp->track[idx].title
		);
	}
	else if (cdp->disc.title != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "TITLE",
			cdp->disc.title
		);
	}

	if (app_data.cdda_trkfile && cdp->track[idx].year != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "DATE",
			cdp->track[idx].year
		);
	}
	else if (cdp->disc.year != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "DATE",
			cdp->disc.year
		);
	}

	if (app_data.cdda_trkfile && cdp->track[idx].genre != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "GENRE",
			cdinfo_genre_name(cdp->track[idx].genre)
		);
	}
	else if (cdp->disc.genre != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "GENRE",
			cdinfo_genre_name(cdp->disc.genre)
		);
	}

	if (app_data.cdda_trkfile && cdp->track[idx].label != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "ORGANIZATION",
			cdp->track[idx].label
		);
	}
	else if (cdp->disc.label != NULL) {
		vorbis_comment_add_tag(
			&ovdesc->vc, "ORGANIZATION",
			cdp->disc.label
		);
	}

	if (app_data.cdda_trkfile) {
		(void) sprintf(tmpstr, "%d", (int) s->trkinfo[idx].trkno);
		vorbis_comment_add_tag(&ovdesc->vc, "TRACKNUMBER", tmpstr);

		if (cdp->track[idx].bpm != NULL) {
			vorbis_comment_add_tag(&ovdesc->vc, "BPM",
					       cdp->track[idx].bpm);
		}
		if (cdp->track[idx].isrc != NULL) {
			vorbis_comment_add_tag(&ovdesc->vc, "ISRC",
					       cdp->track[idx].isrc);
		}
	}
	else for (i = 0; i < (int) s->tot_trks; i++) {
		cd_info_t	*cdi = gdp->cdp->i;

		if (cdi->end_lba < s->trkinfo[i].addr ||
		    cdi->start_lba > s->trkinfo[i].addr)
			continue;

		if (cdp->track[i].artist != NULL) {
			(void) sprintf(tmpstr, "TRACK%02dARTIST",
					(int) s->trkinfo[i].trkno);
			vorbis_comment_add_tag(&ovdesc->vc, tmpstr,
					       cdp->track[i].artist);
		}
		if (cdp->track[i].title != NULL) {
			(void) sprintf(tmpstr, "TRACK%02dTITLE",
					(int) s->trkinfo[i].trkno);
			vorbis_comment_add_tag(&ovdesc->vc, tmpstr,
					       cdp->track[i].title);
		}
	}

	if (s->mcn[0] != '\0')
		vorbis_comment_add_tag(&ovdesc->vc, "MCN", s->mcn);

	(void) sprintf(tmpstr, "%08x", (int) cdp->discid);
	vorbis_comment_add_tag(&ovdesc->vc, "XMCDDISCID", tmpstr);

	vorbis_comment_add_tag(&ovdesc->vc, "ENCODER", tagcomment);
}


/*
 * if_vorbis_init
 *	Initialize Ogg Vorbis encoder and set up encoding parameters
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
if_vorbis_init(gen_desc_t *gdp)
{
	int		bitrate,
			minbr,
			maxbr;
	float		qual;
	ov_desc_t	*ovdesc;
	ogg_page	og;
	ogg_packet	hdr_main;
	ogg_packet	hdr_comments;
	ogg_packet	hdr_codebooks;
	static bool_t	first = TRUE;

	/* Allocate a Ogg Vorbis descriptor structure */
	ovdesc = (ov_desc_t *) MEM_ALLOC("ov_desc_t", sizeof(ov_desc_t));
	if (ovdesc == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"if_vorbis_init: Out of memory.");
		DBGPRN(DBG_GEN)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}
	(void) memset(ovdesc, 0, sizeof(ov_desc_t));

	gdp->aux = (void *) ovdesc;

	if (first) {
		first = FALSE;
		srand(time(NULL));
	}
	ovdesc->serno = rand();

	if (app_data.add_tag)
		if_vorbis_addtags(gdp);

	vorbis_info_init(&ovdesc->vi);

	switch (app_data.comp_mode) {
	case COMPMODE_0:
	case COMPMODE_3:
		bitrate = (app_data.bitrate > 0) ?
			  (app_data.bitrate * 1000) : 128000;
		minbr = (app_data.bitrate_min > 0) ?
			(app_data.bitrate_min * 1000) : -1;
		maxbr = (app_data.bitrate_max > 0) ?
			(app_data.bitrate_max * 1000) : -1;

		/* Managed bitrate */
		if (vorbis_encode_init(&ovdesc->vi, 2, 44100,
				       minbr, bitrate, maxbr) < 0) {
			vorbis_info_clear(&ovdesc->vi);
			(void) strcpy(gdp->cdp->i->msgbuf,
					"if_vorbis_init: "
					"Encoder init (CBR/ABR) failed.");
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}
		break;

	case COMPMODE_1:
	case COMPMODE_2:
		qual = (float) app_data.qual_factor / 10.0f;

		/* Variable bitrate */
		if (vorbis_encode_init_vbr(&ovdesc->vi, 2, 44100, qual) < 0) {
			vorbis_info_clear(&ovdesc->vi);
			(void) strcpy(gdp->cdp->i->msgbuf,
					"if_vorbis_init: "
					"Encoder init (VBR) failed.");
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}
		break;

	default:
		DBGPRN(DBG_GEN)(errfp,
			"NOTICE: Invalid compressionMode parameter.\n");
		break;
	}

	vorbis_analysis_init(&ovdesc->vd, &ovdesc->vi);
	vorbis_block_init(&ovdesc->vd, &ovdesc->vb);
	ogg_stream_init(&ovdesc->os, ovdesc->serno++);

	/* Build the header packets and stream them out */
	vorbis_analysis_headerout(
		&ovdesc->vd,
		&ovdesc->vc,
		&hdr_main,
		&hdr_comments,
		&hdr_codebooks
	);
	ogg_stream_packetin(&ovdesc->os, &hdr_main);
	ogg_stream_packetin(&ovdesc->os, &hdr_comments);
	ogg_stream_packetin(&ovdesc->os, &hdr_codebooks);

	while (ogg_stream_flush(&ovdesc->os, &og) != 0) {
		gdp->flags |= GDESC_WRITEOUT;
		if (!gen_write_chunk(gdp, (byte_t *) og.header, og.header_len))
			return FALSE;

		gdp->flags |= GDESC_WRITEOUT;
		if (!gen_write_chunk(gdp, (byte_t *) og.body, og.body_len))
			return FALSE;
	}

	return TRUE;
}


/*
 * if_vorbis_encode_chunk
 *	Encodes the audio data using Ogg Vorbis and write to the fd.
 *
 * Args:
 *	gdp  - Pointer to the gen_desc_t structure
 *	data - Pointer to data
 *	len  - Data length in bytes
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
if_vorbis_encode_chunk(gen_desc_t *gdp, byte_t *data, size_t len)
{
	ov_desc_t	*ovdesc;
	ogg_page	og;
	ogg_packet	op;
	int		samples,
			i,
			j,
			ret;
	sbyte_t		*p;
	bool_t		eos;

	if (gdp == NULL || gdp->aux == NULL)
		return FALSE;

	ovdesc = (ov_desc_t *) gdp->aux;

	samples = len >> 2;

	DBGPRN(DBG_SND)(errfp, "\nEncoding %d samples\n", samples);

	if (samples == 0 || data == NULL) {
		vorbis_analysis_wrote(&ovdesc->vd, 0);
	}
	else {
		float	**venc_buf;

		venc_buf = vorbis_analysis_buffer(&ovdesc->vd, samples);

		/* De-interleave audio data into encode buffer */
		p = (sbyte_t *) data;
		for (i = 0; i < samples; i++) {
		    j = i << 2;
		    venc_buf[0][i]=
			((p[j+1] << 8) | (0x00ff & (int) p[j])) / 32768.0f;
		    venc_buf[1][i]=
			((p[j+3] << 8) | (0x00ff & (int) p[j+2])) / 32768.0f;
		}

		vorbis_analysis_wrote(&ovdesc->vd, samples);
	}

	eos = FALSE;
	while (vorbis_analysis_blockout(&ovdesc->vd, &ovdesc->vb) == 1) {
		/* Do the main analysis, creating a packet */
		vorbis_analysis(&ovdesc->vb, NULL);
		vorbis_bitrate_addblock(&ovdesc->vb);

		while (vorbis_bitrate_flushpacket(&ovdesc->vd, &op)) {
			/* Add packet to bitstream */
			ogg_stream_packetin(&ovdesc->os, &op);

			while (!eos) {
				ret = ogg_stream_pageout(&ovdesc->os, &og);
				if (ret == 0)
					break;

				gdp->flags |= GDESC_WRITEOUT;
				if (!gen_write_chunk(gdp, (byte_t *) og.header,
						     og.header_len))
					return FALSE;

				gdp->flags |= GDESC_WRITEOUT;
				if (!gen_write_chunk(gdp, (byte_t *) og.body,
						     og.body_len))
					return FALSE;

				if (ogg_page_eos(&og))
					eos = TRUE;
			}
		}
	}

	return TRUE;
}


/*
 * if_vorbis_halt
 *	Flush buffers and shut down Ogg Vorbis encoder
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	Nothing.
 */
void
if_vorbis_halt(gen_desc_t *gdp)
{
	ov_desc_t	*ovdesc;

	if (gdp == NULL || gdp->aux == NULL)
		return;

	ovdesc = (ov_desc_t *) gdp->aux;

	/* Signal end of output */
	(void) if_vorbis_encode_chunk(gdp, NULL, 0);

	ogg_stream_clear(&ovdesc->os);
	vorbis_block_clear(&ovdesc->vb);
	vorbis_dsp_clear(&ovdesc->vd);
	vorbis_comment_clear(&ovdesc->vc);
	vorbis_info_clear(&ovdesc->vi);

	MEM_FREE(ovdesc);
	gdp->aux = NULL;
}

#endif	/* CDDA_SUPPORTED HAS_VORBIS */

