/*
 *   libcddb - CDDB Interface Library for xmcd/cda
 *
 *      This library implements an interface to access the "classic"
 *      CDDB1 services.
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
#ifndef _GENRETBL_H_
#define _GENRETBL_H_

#ifndef lint
static char     *_genretbl_h_ident_ = "@(#)genretbl.h	7.10 03/12/12";
#endif

#define GENREMAP_VERSION	1

/* Default CDDB1 <-> CDDB2 genre map */
fcddb_gmap_t	fcddb_genre_map[] = {
    {	"blues",
	{ "28",		"Blues"				},
	{ "32",		"General Blues"			},
	0,		NULL
    },
    {	"classical",
	{ "40",		"Classical"			},
	{ "46",		"General Classical"		},
	0,		NULL
    },
    {	"country",
	{ "55",		"Country"			},
	{ "60",		"General Country"		},
	0,		NULL
    },
    {	"data",
	{ "70",		"Data"				},
	{ "71",		"General Data"			},
	0,		NULL
    },
    {	"folk",
	{ "94",		"Folk"				},
	{ "96",		"General Folk"			},
	0,		NULL
    },
    {	"jazz",
	{ "153",	"Jazz"				},
	{ "160",	"General Jazz"			},
	0,		NULL
    },
    {	"misc",
	{ "220",	"Unclassifiable"		},
	{ "221",	"General Unclassifiable"	},
	0,		NULL
    },
    {	"newage",
	{ "166",	"New Age"			},
	{ "169",	"General New Age"		},
	0,		NULL
    },
    {	"reggae",
	{ "113",	"Reggae"			},
	{ "246",	"General Reggae"		},
	0,		NULL
    },
    {	"rock",
	{ "180",	"Rock"				},
	{ "191",	"General Rock"			},
	0,		NULL
    },
    {	"soundtrack",
	{ "213",	"Soundtrack"			},
	{ "214",	"General Soundtrack"		},
	0,		NULL
    },
    {	"unclass",
	{ "220",	"Unclassifiable"		},
	{ "221",	"General Unclassifiable"	},
	CDDB_GMAP_DUP,	NULL
    },
    {	NULL,
	{ NULL,		NULL				},
	{ NULL,		NULL				},
	0,		NULL
    }
};


#endif	/* _GENRETBL_H_ */

