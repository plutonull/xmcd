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
 *
 */
#ifndef _ID3MAP_H_
#define _ID3MAP_H_

#ifndef lint
static char     *_id3map_h_ident_ = "@(#)id3map.h	7.5 03/12/12";
#endif


/* Mapping table between CDDB genre IDs and ID3 genre IDs.
 * This table may need to change if CDDB modifies its genres and IDs.
 */
struct {
	char	*cddbgenre;	/* CDDB2 meta-genre ID */
	char	*id3genre;	/* ID3 genre ID */
} id3_gmap[] = {
	{ "1",	 "20"	},	/* Alternative & Punk -> Alternative */
	{ "28",	 "0"	},	/* Blues -> Blues */
	{ "18",	 "101"	},	/* Books & Spoken -> Speech */
	{ "38",	 "12"	},	/* Children's Music -> Other */
	{ "40",	 "32"	},	/* Classical -> Classical */
	{ "55",	 "2"	},	/* Country -> Country */
	{ "70",	 "12"	},	/* Data -> Other */
	{ "75",	 "98"	},	/* Easy Listening -> Easy Listening */
	{ "83",	 "3"	},	/* Electronica/Dance -> Dance */
	{ "94",	 "80"	},	/* Folk -> Folk */
	{ "176", "38"	},	/* Gospel & Religious -> Gospel */
	{ "131", "15"	},	/* Hip Hop/Rap -> Rap */
	{ "147", "12"	},	/* Holiday -> Other */
	{ "106", "19"	},	/* Industrial -> Industrial */
	{ "153", "8"	},	/* Jazz -> Jazz */
	{ "125", "86"	},	/* Latin -> Latin */
	{ "98",	 "9"	},	/* Metal -> Metal */
	{ "166", "10"	},	/* New Age -> New Age */
	{ "171", "13"	},	/* Pop -> Pop */
	{ "64",	 "14"	},	/* R&B -> R&B */
	{ "113", "16"	},	/* Reggae -> Reggae */
	{ "180", "17"	},	/* Rock -> Rock */
	{ "213", "24"	},	/* Soundtrack -> Soundtrack */
	{ "220", "12"	},	/* Unclassifiable -> Other */
	{ "222", "12"	},	/* World -> Other */
	{ NULL,	 NULL	}
};

#endif	/* _ID3MAP_H_ */

