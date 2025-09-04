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
#ifndef _ROLETBL_H_
#define _ROLETBL_H_

#ifndef lint
static char	*_roletbl_h_ident_ = "@(#)roletbl.h	7.8 04/03/08";
#endif


/*
 * Role table
 */
cddb_role_t	fcddb_role_tbl[] = {
    {	"",	"m",	"1",	"Author/Publisher",		NULL	},
    {	"",	"s",	"2",	"Author",			NULL	},
    {	"",	"s",	"3",	"Composer",			NULL	},
    {	"",	"s",	"4",	"Copyright",			NULL	},
    {	"",	"s",	"5",	"Librettist",			NULL	},
    {	"",	"s",	"6",	"Lyricist",			NULL	},
    {	"",	"s",	"7",	"Publisher",			NULL	},
    {	"",	"s",	"8",	"Score",			NULL	},
    {	"",	"s",	"9",	"Songwriter (Music & Lyrics)",	NULL	},
    {	"",	"m",	"10",	"Conductor/Arranger",		NULL	},
    {	"",	"s",	"11",	"Arranger",			NULL	},
    {	"",	"s",	"12",	"Conductor",			NULL	},
    {	"",	"s",	"13",	"Director",			NULL	},
    {	"",	"s",	"14",	"Orchestrator",			NULL	},
    {	"",	"m",	"15",	"Drums/Percussion",		NULL	},
    {	"",	"s",	"16",	"Drums and Percussion",		NULL	},
    {	"",	"s",	"17",	"Drum Programming",		NULL	},
    {	"",	"s",	"18",	"Gong Chimes",			NULL	},
    {	"",	"s",	"19",	"Marimba",			NULL	},
    {	"",	"s",	"20",	"Metallophone",			NULL	},
    {	"",	"s",	"21",	"Percussion-Various",		NULL	},
    {	"",	"s",	"22",	"Steel Drum",			NULL	},
    {	"",	"s",	"23",	"Tabla",			NULL	},
    {	"",	"s",	"24",	"Timpani",			NULL	},
    {	"",	"s",	"25",	"Vibraphone",			NULL	},
    {	"",	"s",	"26",	"Washboard",			NULL	},
    {	"",	"s",	"27",	"Xylophone",			NULL	},
    {	"",	"m",	"28",	"Guitar/Bass",			NULL	},
    {	"",	"s",	"29",	"Acoustic Bass",		NULL	},
    {	"",	"s",	"30",	"Acoustic Guitar",		NULL	},
    {	"",	"s",	"31",	"Banjo",			NULL	},
    {	"",	"s",	"32",	"Bass Guitar",			NULL	},
    {	"",	"s",	"33",	"Cittern",			NULL	},
    {	"",	"s",	"34",	"Chapman Stick",		NULL	},
    {	"",	"s",	"35",	"Guitar-Electric",		NULL	},
    {	"",	"s",	"36",	"Koto",				NULL	},
    {	"",	"s",	"37",	"Lute",				NULL	},
    {	"",	"s",	"38",	"Lyre",				NULL	},
    {	"",	"s",	"39",	"Mandolin",			NULL	},
    {	"",	"s",	"40",	"Pedal Steel Guitar",		NULL	},
    {	"",	"s",	"41",	"Samisen",			NULL	},
    {	"",	"s",	"42",	"Sitar",			NULL	},
    {	"",	"s",	"43",	"Tambura",			NULL	},
    {	"",	"s",	"44",	"Ukulele",			NULL	},
    {	"",	"s",	"45",	"Vina",				NULL	},
    {	"",	"m",	"46",	"Keyboards",			NULL	},
    {	"",	"s",	"47",	"Clavichord",			NULL	},
    {	"",	"s",	"48",	"Harpsichord",			NULL	},
    {	"",	"s",	"49",	"Keyboards-Various",		NULL	},
    {	"",	"s",	"50",	"Harmonium",			NULL	},
    {	"",	"s",	"51",	"Organ",			NULL	},
    {	"",	"s",	"52",	"Piano",			NULL	},
    {	"",	"s",	"53",	"Synthesizer",			NULL	},
    {	"",	"s",	"54",	"Virginal",			NULL	},
    {	"",	"m",	"55",	"Other Instrument",		NULL	},
    {	"",	"s",	"56",	"Accordion",			NULL	},
    {	"",	"s",	"57",	"Bagpipe",			NULL	},
    {	"",	"s",	"58",	"Concertina",			NULL	},
    {	"",	"s",	"59",	"Dulcimer",			NULL	},
    {	"",	"s",	"60",	"Harmonica",			NULL	},
    {	"",	"s",	"61",	"Historical Instrument",	NULL	},
    {	"",	"s",	"62",	"Jew's Harp",			NULL	},
    {	"",	"s",	"63",	"Kazoo",			NULL	},
    {	"",	"s",	"64",	"Penny Whistle",		NULL	},
    {	"",	"s",	"65",	"Regional Instrument",		NULL	},
    {	"",	"s",	"66",	"DJ (Scratch)",			NULL	},
    {	"",	"s",	"67",	"Unlisted Instrument",		NULL	},
    {	"",	"m",	"68",	"Production",			NULL	},
    {	"",	"s",	"69",	"Assistant Engineer",		NULL	},
    {	"",	"s",	"70",	"Associate Producer",		NULL	},
    {	"",	"s",	"71",	"Coproducer",			NULL	},
    {	"",	"s",	"72",	"Engineer",			NULL	},
    {	"",	"s",	"73",	"Executive Producer",		NULL	},
    {	"",	"s",	"74",	"Mastering",			NULL	},
    {	"",	"s",	"75",	"Mastering Location",		NULL	},
    {	"",	"s",	"76",	"Mixing",			NULL	},
    {	"",	"s",	"77",	"Mixing Location",		NULL	},
    {	"",	"s",	"78",	"Producer",			NULL	},
    {	"",	"s",	"79",	"Programming",			NULL	},
    {	"",	"s",	"80",	"Recording Location",		NULL	},
    {	"",	"s",	"147",	"Remixing",			NULL	},
    {	"",	"m",	"81",	"Saxophones",			NULL	},
    {	"",	"s",	"82",	"Alto Saxophone",		NULL	},
    {	"",	"s",	"83",	"Baritone Saxophone",		NULL	},
    {	"",	"s",	"84",	"Bass Saxophone",		NULL	},
    {	"",	"s",	"85",	"Contrabass Saxophone",		NULL	},
    {	"",	"s",	"86",	"Saxophone",			NULL	},
    {	"",	"s",	"87",	"Sopranino Saxophone",		NULL	},
    {	"",	"s",	"88",	"Soprano Saxophone",		NULL	},
    {	"",	"s",	"89",	"Subcontrabass Saxophone",	NULL	},
    {	"",	"s",	"90",	"Tenor Saxophone",		NULL	},
    {	"",	"m",	"91",	"Strings",			NULL	},
    {	"",	"s",	"92",	"Cello",			NULL	},
    {	"",	"s",	"93",	"Double bass",			NULL	},
    {	"",	"s",	"94",	"Fiddle",			NULL	},
    {	"",	"s",	"95",	"Harp",				NULL	},
    {	"",	"s",	"96",	"Viol (Viola da Gamba)",	NULL	},
    {	"",	"s",	"97",	"Viola",			NULL	},
    {	"",	"s",	"98",	"Violin",			NULL	},
    {	"",	"s",	"99",	"Zither",			NULL	},
    {	"",	"m",	"100",	"Voice",			NULL	},
    {	"",	"s",	"101",	"Actor",			NULL	},
    {	"",	"s",	"102",	"Alto",				NULL	},
    {	"",	"s",	"103",	"Baritone",			NULL	},
    {	"",	"s",	"104",	"Basso",			NULL	},
    {	"",	"s",	"105",	"Contralto",			NULL	},
    {	"",	"s",	"106",	"Counter-tenor",		NULL	},
    {	"",	"s",	"107",	"Mezzo-soprano",		NULL	},
    {	"",	"s",	"108",	"Narrator",			NULL	},
    {	"",	"s",	"109",	"Rap",				NULL	},
    {	"",	"s",	"110",	"Reader",			NULL	},
    {	"",	"s",	"111",	"Soprano",			NULL	},
    {	"",	"s",	"112",	"Tenor",			NULL	},
    {	"",	"s",	"113",	"Vocals",			NULL	},
    {	"",	"s",	"114",	"Vocals-Backing",		NULL	},
    {	"",	"s",	"115",	"Vocals-Lead",			NULL	},
    {	"",	"s",	"116",	"Voice-Other",			NULL	},
    {	"",	"s",	"117",	"Voice-Spoken",			NULL	},
    {	"",	"m",	"118",	"Wind/Brass",			NULL	},
    {	"",	"s",	"119",	"Bugle",			NULL	},
    {	"",	"s",	"120",	"Cornet",			NULL	},
    {	"",	"s",	"121",	"English Horn",			NULL	},
    {	"",	"s",	"122",	"Euphonium",			NULL	},
    {	"",	"s",	"123",	"French Horn",			NULL	},
    {	"",	"s",	"124",	"Horn",				NULL	},
    {	"",	"s",	"125",	"Sousaphone",			NULL	},
    {	"",	"s",	"126",	"Trombone",			NULL	},
    {	"",	"s",	"127",	"Trombone-Valve",		NULL	},
    {	"",	"s",	"128",	"Trumpet",			NULL	},
    {	"",	"s",	"129",	"Trumpet-Slide",		NULL	},
    {	"",	"s",	"130",	"Tuba",				NULL	},
    {	"",	"m",	"131",	"Wind/Woodwind",		NULL	},
    {	"",	"s",	"132",	"Bass Clarinet",		NULL	},
    {	"",	"s",	"133",	"Bassoon",			NULL	},
    {	"",	"s",	"134",	"Clarinet",			NULL	},
    {	"",	"s",	"135",	"Contrabassoon",		NULL	},
    {	"",	"s",	"136",	"Cor Anglais",			NULL	},
    {	"",	"s",	"137",	"Didgeridoo",			NULL	},
    {	"",	"s",	"138",	"Fife",				NULL	},
    {	"",	"s",	"139",	"Flageolet",			NULL	},
    {	"",	"s",	"140",	"Flute",			NULL	},
    {	"",	"s",	"141",	"Oboe",				NULL	},
    {	"",	"s",	"142",	"Oboe d'amore",			NULL	},
    {	"",	"s",	"143",	"Panpipes",			NULL	},
    {	"",	"s",	"144",	"Piccolo",			NULL	},
    {	"",	"s",	"145",	"Recorder",			NULL	},
    {	"",	"s",	"146",	"Shawm",			NULL	},
    {	"",	"",	NULL,	NULL,				NULL	}
};


#endif	/* _ROLETBL_H_ */

