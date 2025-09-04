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
static char *_if_flac_c_ident_ = "@(#)if_flac.c	7.19 03/12/12";
#endif

#include "common_d/appenv.h"
#include "common_d/version.h"
#include "common_d/util.h"
#include "libdi_d/libdi.h"
#include "cdda_d/cdda.h"

#if defined(CDDA_SUPPORTED) && defined(HAS_FLAC)

#include "cdinfo_d/cdinfo.h"
#include "cdda_d/wr_gen.h"
#include "cdda_d/if_flac.h"

/* Hack: use our own FLAC types instead of using those from <FLAC/ordinals.h>
 * to be independent of the compiler that was used to build the FLAC library.
 * This is necessary to resolve a conflict on some platforms where FLAC was
 * compiled with gcc (which has "long long"), but this file is being compiled
 * with a compiler that does not have a 64-bit integer type.
 */
#define FLAC__ORDINALS_H	/* Override <FLAC/ordinals.h> */

typedef int			FLAC__bool;
typedef byte_t			FLAC__uint8;
typedef sbyte_t			FLAC__int8;
typedef word16_t		FLAC__uint16;
typedef sword16_t		FLAC__int16;
typedef word32_t		FLAC__uint32;
typedef sword32_t		FLAC__int32;
typedef word64_t		FLAC__uint64;
typedef sword64_t		FLAC__int64;
typedef byte_t			FLAC__byte;
typedef float			FLAC__real;

#include <FLAC/format.h>
#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>


extern appdata_t		app_data;
extern FILE			*errfp;
extern cdda_client_t		*cdda_clinfo;

extern char			*tagcomment;	/* Tag comment */


/* Convenience macros */
#ifdef MIN
#undef MIN
#endif
#define MIN(x,y)		(((x) < (y)) ? (x) : (y))

#ifdef MAX
#undef MAX
#endif
#define MAX(x,y)		(((x) > (y)) ? (x) : (y))


/* Max number of metadata blocks */
#define META_MAXBLOCKS		8


/* Container union for encoder instance pointers */
typedef union {
	FLAC__StreamEncoder		*st;	/* Stream encoder */
} flac_desc_t;


/* Structure of user-adjustable parameters */
typedef struct {
	FLAC__bool	mid_side,	/* Enable mid/side stereo */
			adap_ms,	/* Enable adaptive mid/side stereo */
			exh_srch,	/* Enable exhaustive model search */
			vfy_mode,	/* Enable verify mode */
			qlp_srch;	/* Enable LP coeff quant search */
	unsigned int	lpc_order,	/* LPC order */
			block_sz;	/* Block size */
	int		min_rpo,	/* Minimum residual partition order */
			max_rpo;	/* Maximum residual partition order */
} flac_parms_t;


/* Pointers to wide samples buffers for each channel */
STATIC FLAC__int32		*fenc_buf[2] = { NULL, NULL };
STATIC FLAC__StreamMetadata	**flac_mlist = NULL;
STATIC int			flac_mblks = 0;


/*
 * if_flac_write_callback
 *	FLAC stream encoder write callback function
 *
 * Args:
 *	stp - Stream encoder instance descriptor
 *	buf - Output data buffer
 *	len - data length
 *	samples - number of samples
 *	frame - Current frame
 *	client_data - Callback data
 *
 * Return:
 *	FLAC__STREAM_ENCODER_WRITE_STATUS_OK - success
 *	FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR - failure
 */
/*ARGSUSED*/
STATIC FLAC__StreamEncoderWriteStatus
if_flac_write_callback(
	const FLAC__StreamEncoder	*stp,
	const FLAC__byte		*buf,
	unsigned int			len,
	unsigned int			samples,
	unsigned int			frame,
	void				*client_data
)
{
	gen_desc_t	*gdp = (gen_desc_t *) client_data;

	if (buf == NULL || len == 0)
		/* Nothing to do */
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;

	/* Write out the encoded data */
	gdp->flags |= GDESC_WRITEOUT;
	if (!gen_write_chunk(gdp, (byte_t *) buf, (size_t) len))
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}


/*
 * if_flac_metadata_callback
 *	FLAC stream encoder metadata callback function
 *
 * Args:
 *	skp - Encoder instance descriptor
 *	data - Metadata buffer
 *	client_data - Callback data
 *
 * Return:
 *	Nothing.
 */
/*ARGSUSED*/
STATIC FLAC__StreamEncoderSeekStatus
if_flac_metadata_callback(
	const FLAC__StreamEncoder		*skp,
	FLAC__uint64				offset,
	void					*client_data
)
{
	gen_desc_t	*gdp = (gen_desc_t *) client_data;
	off_t		val;

	val = (off_t) ASSIGN32(offset);

	if (gen_seek(gdp, val, SEEK_SET))
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
}


/*
 * if_flac_addseek_spaced
 *	Add spaced seek points template to the seektable metadata block.
 *	A seek point is added to approximately each 10 seconds of audio.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *	mdp - The metadata descriptor
 *	cdi - Pointer to the cd_info_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
if_flac_addseek_spaced(
	gen_desc_t		*gdp,
	FLAC__StreamMetadata	*mdp,
	cd_info_t		*cdi
)
{
	FLAC__bool	ret;
	FLAC__uint64	val64;
	unsigned int	nsectors,
			nsamples,
			nseekpts;

	nsectors = (unsigned int) cdi->end_lba - cdi->start_lba + 1;
	nsamples = (unsigned int) (nsectors / FRAME_PER_SEC) * 44100;
	nseekpts = nsectors / (FRAME_PER_SEC * 10);

	if (nseekpts == 0)
		return;

	val64 = ASSIGN64(nsamples);

	ret = FLAC__metadata_object_seektable_template_append_spaced_points(
		mdp, nseekpts, val64
	);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_addseek_spaced: "
			    "Failed inserting spaced seek point.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return;
	}

	ret = FLAC__metadata_object_seektable_template_sort(mdp, FALSE);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_addseek_spaced: "
			    "Failed sorting the seek table.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
	}
}


/*
 * if_flac_addseek_tracks
 *	Add start-of-track seek points template to the seektable metadata
 *	block.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *	mdp - The metadata descriptor
 *	cdi - Pointer to the cd_info_t structure
 *	s   - Pointer to the curstat_t structure
 *
 * Return:
 *	Nothing.
 */
STATIC void
if_flac_addseek_tracks(
	gen_desc_t		*gdp,
	FLAC__StreamMetadata	*mdp,
	cd_info_t		*cdi,
	curstat_t		*s
)
{
	FLAC__bool	ret;
	FLAC__uint64	val64;
	unsigned int	start,
			end,
			nsectors,
			nsamples,
			seekpt;
	int		i,
			n;

	seekpt = 0;
	for (i = n = 0; i < (int) s->tot_trks; i++) {
		if (cdi->end_lba < s->trkinfo[i].addr ||
		    cdi->start_lba > s->trkinfo[i].addr)
			continue;

		start = (unsigned int) MAX(s->trkinfo[i].addr, cdi->start_lba);
		end   = (unsigned int) MIN(s->trkinfo[i+1].addr, cdi->end_lba);
		nsectors = end - start + 1;
		nsamples = (unsigned int) (nsectors / FRAME_PER_SEC) * 44100;

		if (n++ > 0)
			seekpt += nsamples;

		val64 = ASSIGN64(seekpt);

		ret = FLAC__metadata_object_seektable_template_append_point(
			mdp, val64
		);
		if (!ret) {
			(void) sprintf(gdp->cdp->i->msgbuf,
				    "if_flac_addseek_tracks: "
				    "Failed adding seek point for track %d.",
				    (int) s->trkinfo[i].trkno);
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		}
	}

	ret = FLAC__metadata_object_seektable_template_sort(mdp, FALSE);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_addseek_tracks: "
			    "Failed sorting the seek table.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
	}
}


/*
 * if_flac_addtagent
 *	Add one entry into the vorbis comment metadata block.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *	mdp - The metadata descriptor
 *	attr - Attribute name string
 *	val - Value string name
 *
 * Return:
 *	Nothing.
 */
STATIC void
if_flac_addtagent(
	gen_desc_t		*gdp,
	FLAC__StreamMetadata	*mdp,
	char			*attr,
	char			*val
)
{
	FLAC__StreamMetadata_VorbisComment_Entry ent;
	FLAC__bool	ret;
	char		txtstr[STR_BUF_SZ * 2];

	/* Vorbis comments should be encoded in UTF-8 so we don't need
	 * any conversion here.
	 */

	(void) sprintf(txtstr, "%.16s=%.100s", attr, val);
	ent.entry = (FLAC__byte *) txtstr;
	ent.length = (FLAC__uint32) strlen(txtstr);

	ret = FLAC__metadata_object_vorbiscomment_insert_comment(
		mdp, mdp->data.vorbis_comment.num_comments, ent, TRUE
	);
	if (!ret) {
		(void) sprintf(gdp->cdp->i->msgbuf,
			    "if_flac_addtagent: "
			    "Failed inserting vorbis comment: attr=[%s]",
			    attr);
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
	}
}


/*
 * if_flac_addmeta
 *	Initialize and set the metadata areas of the FLAC output.
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *	ufdp - Pointer to the FLAC descriptor union
 *
 * Return:
 *	Nothing.
 */
STATIC void
if_flac_addmeta(gen_desc_t *gdp, flac_desc_t *ufdp)
{
	FLAC__StreamMetadata	*mdp;
	FLAC__bool		ret;
	curstat_t		*s = cdda_clinfo->curstat_addr();
	cdinfo_incore_t		*cdp = cdinfo_addr();
	int			i,
				idx = (int) gdp->cdp->i->trk_idx;
	size_t			listsz;
	char			tmpstr[32];

	listsz = sizeof(FLAC__StreamMetadata *) * META_MAXBLOCKS;

	flac_mlist = (FLAC__StreamMetadata **) MEM_ALLOC("flac_mlist", listsz);
	if (flac_mlist == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_addmeta: Out of memory.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return;
	}
	(void) memset(flac_mlist, 0, listsz);
	flac_mblks = 0;

	/* Application-specific metadata */
	mdp = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION);
	if (mdp == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			"if_flac_addmeta: "
			"Failed creating application metadata block.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
	}
	else {
		char	data[STR_BUF_SZ * 4];

		flac_mlist[flac_mblks++] = mdp;

		/* Set the application ID - This has been registered
		 * with the FLAC developers.
		 */
		(void) memcpy(mdp->data.application.id, "xmcd", 4);

		/* For future expansion: just some app information
		 * in here for now.
		 */
		(void) sprintf(data, "%s.%s.%s\n%s\n%s\n%s\n",
			VERSION_MAJ, VERSION_MIN, VERSION_TEENY,
			COPYRIGHT, EMAIL, XMCD_URL
		);

		ret = FLAC__metadata_object_application_set_data(
			mdp, (FLAC__byte *) data, (unsigned) strlen(data), TRUE
		);
		if (!ret) {
			(void) strcpy(gdp->cdp->i->msgbuf,
				"if_flac_addmeta: "
				"Failed setting application metadata.");
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		}
	}

	/* Seek table metadata */
	if ((gdp->flags & GDESC_ISPIPE) == 0) {
		mdp = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
		if (mdp == NULL) {
			(void) strcpy(gdp->cdp->i->msgbuf,
				"if_flac_addmeta: "
				"Failed creating seektable metadata block.");
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		}
		else {
			cd_info_t	*cdi = gdp->cdp->i;

			flac_mlist[flac_mblks++] = mdp;

			if (app_data.cdda_trkfile) {
				/* Insert evenly spaced seek points */
				if_flac_addseek_spaced(gdp, mdp, cdi);
			}
			else {
				/* Insert seek points at start of tracks */
				if_flac_addseek_tracks(gdp, mdp, cdi, s);
			}
		}
	}

	/* Vorbis comment metadata */
	if (app_data.add_tag) {
		mdp = FLAC__metadata_object_new(
			FLAC__METADATA_TYPE_VORBIS_COMMENT
		);
		if (mdp == NULL) {
			(void) strcpy(gdp->cdp->i->msgbuf,
				"if_flac_addmeta: "
				"Failed creating vorbis metadata block.");
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		}
		else {
			flac_mlist[flac_mblks++] = mdp;

			mdp->data.vorbis_comment.num_comments = 0;

			if (cdp->disc.title != NULL)
				if_flac_addtagent(gdp, mdp, "ALBUM",
						  cdp->disc.title);

			if (app_data.cdda_trkfile &&
			    cdp->track[idx].artist != NULL)
				if_flac_addtagent(gdp, mdp, "ARTIST",
						  cdp->track[idx].artist);
			else
				if_flac_addtagent(gdp, mdp, "ARTIST",
						  cdp->disc.artist);

			if (app_data.cdda_trkfile &&
			    cdp->track[idx].title != NULL)
				if_flac_addtagent(gdp, mdp, "TITLE",
						  cdp->track[idx].title);
			else if (cdp->disc.title != NULL)
				if_flac_addtagent(gdp, mdp, "TITLE",
						  cdp->disc.title);

			if (app_data.cdda_trkfile &&
			    cdp->track[idx].year != NULL)
				if_flac_addtagent(gdp, mdp, "DATE",
						  cdp->track[idx].year);
			else if (cdp->disc.year != NULL)
				if_flac_addtagent(gdp, mdp, "DATE",
						  cdp->disc.year);

			if (app_data.cdda_trkfile &&
			    cdp->track[idx].genre != NULL)
				if_flac_addtagent(gdp, mdp, "GENRE",
				    cdinfo_genre_name(cdp->track[idx].genre));
			else if (cdp->disc.genre != NULL)
				if_flac_addtagent(gdp, mdp, "GENRE",
				    cdinfo_genre_name(cdp->disc.genre));

			if (app_data.cdda_trkfile &&
			    cdp->track[idx].label != NULL)
				if_flac_addtagent(gdp, mdp, "ORGANIZATION",
						  cdp->track[idx].label);
			else if (cdp->disc.label != NULL)
				if_flac_addtagent(gdp, mdp, "ORGANIZATION",
						  cdp->disc.label);

			if (app_data.cdda_trkfile) {
				(void) sprintf(tmpstr, "%d",
						(int) s->trkinfo[idx].trkno);
				if_flac_addtagent(gdp, mdp, "TRACKNUMBER",
						  tmpstr);

				if (cdp->track[idx].bpm != NULL) {
				    if_flac_addtagent(gdp, mdp, "BPM",
						      cdp->track[idx].bpm);
				}

				if (cdp->track[idx].isrc != NULL) {
				    if_flac_addtagent(gdp, mdp, "ISRC",
						      cdp->track[idx].isrc);
				}
			}
			else for (i = 0; i < (int) s->tot_trks; i++) {
				cd_info_t	*cdi = gdp->cdp->i;

				if (cdi->end_lba < s->trkinfo[i].addr ||
				    cdi->start_lba > s->trkinfo[i].addr)
					continue;

				if (cdp->track[i].artist != NULL) {
					(void) sprintf(tmpstr,
						"TRACK%02dARTIST",
						(int) s->trkinfo[i].trkno
					);
					if_flac_addtagent(gdp, mdp, tmpstr,
						cdp->track[i].artist
					);
				}
				if (cdp->track[i].title != NULL) {
					(void) sprintf(tmpstr,
						"TRACK%02dTITLE",
						(int) s->trkinfo[i].trkno
					);
					if_flac_addtagent(gdp, mdp, tmpstr,
						cdp->track[i].title
					);
				}
			}

			if (s->mcn[0] != '\0')
				if_flac_addtagent(gdp, mdp, "MCN", s->mcn);

			(void) sprintf(tmpstr, "%08x", (int) cdp->discid);
			if_flac_addtagent(gdp, mdp, "XMCDDISCID", tmpstr);

			if_flac_addtagent(gdp, mdp, "ENCODER", tagcomment);
		}
	}

	if (flac_mblks == 0)
		return;	/* No metadata to set */

	ret = FLAC__stream_encoder_set_metadata(
		ufdp->st, flac_mlist, flac_mblks
	);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_addmeta: Failed setting metadata.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
	}
}


/*
 * if_flac_encoder_setup
 *	Create and set up the FLAC stream encoder.
 *
 * Args:
 *	gdp  - Pointer to the gen_desc_t structure
 *	ufdp - Pointer to the flac_desc_t structure
 *	pp   - Pointer to the flac_parms_t structure
 *
 * Return:
 *	TRUE  - success
 *	FALSE - failure
 */
STATIC bool_t
if_flac_encoder_setup(
	gen_desc_t	*gdp,
	flac_desc_t	*ufdp,
	flac_parms_t	*pp
)
{
	FLAC__StreamEncoder		*stp;
	FLAC__StreamEncoderState	enc_state;
	FLAC__uint64			val;
	FLAC__bool			ret;

	ufdp->st = stp = FLAC__stream_encoder_new();
	if (stp == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "FLAC encoder instantiation failed.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	/* Set some basic parameters */

	ret = FLAC__stream_encoder_set_channels(stp, 2);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting channels.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_bits_per_sample(stp, 16);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting bits per sample.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_sample_rate(stp, 44100);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting sample rate.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	val = ASSIGN64(gdp->datalen >> 2);
	ret = FLAC__stream_encoder_set_total_samples_estimate(stp, val);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting samples estimate.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_streamable_subset(stp, TRUE);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting streamable subset.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_do_qlp_coeff_prec_search(
		stp, pp->qlp_srch
	);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			"if_flac_encoder_setup: "
			"Failed setting LP coefficient quantization search."
		);
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	/* Just use default for now
	ret = FLAC__stream_encoder_set_qlp_coeff_precision(stp, 0);
	*/

	ret = FLAC__stream_encoder_set_verify(stp, pp->vfy_mode);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting verify mode.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_max_lpc_order(stp, pp->lpc_order);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting max LPC order.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_blocksize(stp, pp->block_sz);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting block size.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_do_mid_side_stereo(stp, pp->mid_side);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting M/S stereo.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	ret = FLAC__stream_encoder_set_loose_mid_side_stereo(stp, pp->adap_ms);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting adaptive M/S stereo.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	if (pp->min_rpo >= 0) {
		ret = FLAC__stream_encoder_set_min_residual_partition_order(
			stp, (unsigned int) pp->min_rpo
		);
		if (!ret) {
			(void) strcpy(gdp->cdp->i->msgbuf,
				    "if_flac_encoder_setup: "
				    "Failed setting minimum RPO.");
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}
	}

	if (pp->max_rpo >= 0) {
		ret = FLAC__stream_encoder_set_max_residual_partition_order(
			stp, (unsigned int) pp->max_rpo
		);
		if (!ret) {
			(void) strcpy(gdp->cdp->i->msgbuf,
				    "if_flac_encoder_setup: "
				    "Failed setting maximum RPO.");
			DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
			return FALSE;
		}
	}

	ret = FLAC__stream_encoder_set_do_exhaustive_model_search(
		stp, pp->exh_srch
	);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			    "if_flac_encoder_setup: "
			    "Failed setting exhaustive model search.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	/* Initialize and add metadata to FLAC output */
	if_flac_addmeta(gdp, ufdp);

	/* Initialize FLAC encoder */
	enc_state = FLAC__stream_encoder_init_stream(
		stp, if_flac_write_callback, NULL, NULL,
		if_flac_metadata_callback, gdp
	);
	if (enc_state != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		(void) sprintf(gdp->cdp->i->msgbuf,
			"if_flac_encoder_setup: Encoder init error: %s",
			FLAC__stream_encoder_get_resolved_state_string(stp)
		);
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	/* Allocate wide samples buffers for the two channels */
	fenc_buf[0] = (FLAC__int32 *) MEM_ALLOC(
		"fenc_buf0",
		(size_t) gdp->cdp->cds->chunk_bytes
	);
	if (fenc_buf[0] == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"if_flac_encoder_setup: Out of memory.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}
	fenc_buf[1] = (FLAC__int32 *) MEM_ALLOC(
		"fenc_buf1",
		(size_t) gdp->cdp->cds->chunk_bytes
	);
	if (fenc_buf[1] == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"if_flac_encoder_setup: Out of memory.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	return TRUE;
}


/*
 * if_flac_init
 *	Initialize FLAC encoder and set up encoding parameters
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	FALSE - failure
 *	TRUE  - success
 */
bool_t
if_flac_init(gen_desc_t *gdp)
{
	FLAC__bool	ret;
	flac_desc_t	*ufdp;
	flac_parms_t	parms;

	/* Allocate descriptor */
	ufdp = (flac_desc_t *) MEM_ALLOC("flac_desc_t", sizeof(flac_desc_t));
	if (ufdp == NULL) {
		(void) strcpy(gdp->cdp->i->msgbuf,
				"if_flac_init: Out of memory.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	gdp->aux = (void *) ufdp;

	/* Set up adjustable parameters */

	switch (app_data.comp_mode) {
	case COMPMODE_3:
		parms.qlp_srch = parms.vfy_mode = TRUE;
		break;
	case COMPMODE_2:
		parms.qlp_srch = FALSE;
		parms.vfy_mode = TRUE;
		break;
	case COMPMODE_1:
		parms.qlp_srch = TRUE;
		parms.vfy_mode = FALSE;
		break;
	case COMPMODE_0:
	default:
		parms.qlp_srch = parms.vfy_mode = FALSE;
		break;
	}

	switch (app_data.comp_algo) {
	case 10:
	case 9:
		parms.lpc_order = 12;
		parms.block_sz = 4608;
		parms.min_rpo = 6;
		parms.max_rpo = -1;
		parms.mid_side = TRUE;
		parms.adap_ms = FALSE;
		parms.exh_srch = TRUE;
		break;
	case 8:
		parms.lpc_order = 8;
		parms.block_sz = 4608;
		parms.min_rpo = 6;
		parms.max_rpo = -1;
		parms.mid_side = TRUE;
		parms.adap_ms = FALSE;
		parms.exh_srch = TRUE;
		break;
	case 7:
		parms.lpc_order = 8;
		parms.block_sz = 4608;
		parms.min_rpo = 4;
		parms.max_rpo = -1;
		parms.mid_side = TRUE;
		parms.adap_ms = FALSE;
		parms.exh_srch = FALSE;
		break;
	case 6:
		parms.lpc_order = 8;
		parms.block_sz = 4608;
		parms.min_rpo = parms.max_rpo = 3;
		parms.mid_side = TRUE;
		parms.adap_ms = FALSE;
		parms.exh_srch = FALSE;
		break;
	case 5:
		parms.lpc_order = 8;
		parms.block_sz = 4608;
		parms.min_rpo = parms.max_rpo = 3;
		parms.mid_side = parms.adap_ms = TRUE;
		parms.exh_srch = FALSE;
		break;
	case 4:
		parms.lpc_order = 6;
		parms.block_sz = 4608;
		parms.min_rpo = parms.max_rpo = 3;
		parms.mid_side = parms.adap_ms = FALSE;
		parms.exh_srch = FALSE;
		break;
	case 3:
		parms.lpc_order = 0;
		parms.block_sz = 1152;
		parms.min_rpo = 3;
		parms.max_rpo = -1;
		parms.mid_side = TRUE;
		parms.adap_ms = FALSE;
		parms.exh_srch = FALSE;
		break;
	case 2:
		parms.lpc_order = 0;
		parms.block_sz = 1152;
		parms.min_rpo = parms.max_rpo = 2;
		parms.mid_side = TRUE;
		parms.adap_ms = TRUE;
		parms.exh_srch = FALSE;
		break;
	case 1:
	default:
		parms.lpc_order = 0;
		parms.block_sz = 1152;
		parms.min_rpo = parms.max_rpo = 2;
		parms.mid_side = parms.adap_ms = FALSE;
		parms.exh_srch = FALSE;
		break;
	}

	/* Set up encoder */
	ret = if_flac_encoder_setup(gdp, ufdp, &parms);

	return (ret);
}


/*
 * if_flac_encode_chunk
 *	Encodes the audio data using FLAC and write to the fd.
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
if_flac_encode_chunk(gen_desc_t *gdp, byte_t *data, size_t len)
{
	flac_desc_t	*ufdp;
	FLAC__bool	ret;
	int		i,
			j,
			samples;
	sword16_t	*p;

	if (gdp == NULL || gdp->aux == NULL)
		return FALSE;

	if (data == NULL || len == 0)
		/* Nothing to do */
		return TRUE;

	ufdp = (flac_desc_t *) gdp->aux;

	/* De-interleave data and sign-extend into wide samples buffer */
	samples = (len >> 2);
	p = (sword16_t *)(void *) data;
	for (i = 0, j = 0; i < samples; i++) {
		fenc_buf[0][i] = (FLAC__int32) p[j++];
		fenc_buf[1][i] = (FLAC__int32) p[j++];
	}

	DBGPRN(DBG_SND)(errfp, "\nEncoding %d samples\n", samples);

	ret = FLAC__stream_encoder_process(
		ufdp->st, (void *) fenc_buf, (unsigned int) samples
	);
	if (!ret) {
		(void) strcpy(gdp->cdp->i->msgbuf,
			"if_flac_encode_chunk: Process encoding failed.");
		DBGPRN(DBG_SND)(errfp, "%s\n", gdp->cdp->i->msgbuf);
		return FALSE;
	}

	return TRUE;
}


/*
 * if_flac_halt
 *	Flush buffers and shut down FLAC encoder
 *
 * Args:
 *	gdp - Pointer to the gen_desc_t structure
 *
 * Return:
 *	Nothing.
 */
void
if_flac_halt(gen_desc_t *gdp)
{
	flac_desc_t	*ufdp;
	int		i;

	if (gdp == NULL || gdp->aux == NULL)
		return;

	ufdp = (flac_desc_t *) gdp->aux;

	FLAC__stream_encoder_finish(ufdp->st);
	FLAC__stream_encoder_delete(ufdp->st);

	MEM_FREE(ufdp);
	gdp->aux = NULL;

	/* Deallocate metadata objects */
	for (i = 0; i < flac_mblks; i++) {
		if (flac_mlist[i] != NULL)
			FLAC__metadata_object_delete(flac_mlist[i]);
	}
	MEM_FREE(flac_mlist);
	flac_mlist = NULL;
	flac_mblks = 0;

	/* Free wide samples buffers */
	if (fenc_buf[0] != NULL)
		MEM_FREE(fenc_buf[0]);
	if (fenc_buf[1] != NULL)
		MEM_FREE(fenc_buf[1]);

	fenc_buf[0] = fenc_buf[1] = NULL;
}

#endif	/* CDDA_SUPPORTED HAS_FLAC */

