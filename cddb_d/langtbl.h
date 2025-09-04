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
#ifndef _LANGTBL_H_
#define _LANGTBL_H_

#ifndef lint
static char     *_langtbl_h_ident_ = "@(#)langtbl.h	1.3 03/12/12";
#endif


/*
 * Language table
 */
cddb_language_t	fcddb_language_tbl[] = {
    {	"",	"1",	"English",				NULL	},
    {	"",	"2",	"Afrikaans",				NULL	},
    {	"",	"3",	"Albanian",				NULL	},
    {	"",	"4",	"Arabic",				NULL	},
    {	"",	"5",	"Armenian",				NULL	},
    {	"",	"6",	"Asturian",				NULL	},
    {	"",	"7",	"Azerbaijani",				NULL	},
    {	"",	"8",	"Basque",				NULL	},
    {	"",	"9",	"Bengali",				NULL	},
    {	"",	"10",	"Bosnian",				NULL	},
    {	"",	"11",	"Breton",				NULL	},
    {	"",	"12",	"Bulgarian",				NULL	},
    {	"",	"13",	"Simplified Chinese",			NULL	},
    {	"",	"14",	"Catalan",				NULL	},
    {	"",	"15",	"Creole",				NULL	},
    {	"",	"16",	"Croatian",				NULL	},
    {	"",	"17",	"Czech",				NULL	},
    {	"",	"18",	"Danish",				NULL	},
    {	"",	"19",	"Dutch",				NULL	},
    {	"",	"20",	"Elvish",				NULL	},
    {	"",	"21",	"Esperanto",				NULL	},
    {	"",	"22",	"Estonian",				NULL	},
    {	"",	"23",	"Europortuguese",			NULL	},
    {	"",	"24",	"Farsi",				NULL	},
    {	"",	"25",	"Finnish",				NULL	},
    {	"",	"26",	"French",				NULL	},
    {	"",	"27",	"Frisian",				NULL	},
    {	"",	"28",	"Gaelic",				NULL	},
    {	"",	"29",	"Galician",				NULL	},
    {	"",	"30",	"German",				NULL	},
    {	"",	"31",	"Greek",				NULL	},
    {	"",	"32",	"Gujarati",				NULL	},
    {	"",	"33",	"Hawaiian",				NULL	},
    {	"",	"34",	"Hebrew",				NULL	},
    {	"",	"35",	"Hindi",				NULL	},
    {	"",	"36",	"Holooe",				NULL	},
    {	"",	"37",	"Hungarian",				NULL	},
    {	"",	"38",	"Icelandic",				NULL	},
    {	"",	"39",	"Ido",					NULL	},
    {	"",	"40",	"Bahasa Indonesia",			NULL	},
    {	"",	"41",	"Interlingua",				NULL	},
    {	"",	"42",	"Irish",				NULL	},
    {	"",	"43",	"Italian",				NULL	},
    {	"",	"44",	"Japanese",				NULL	},
    {	"",	"45",	"Javanese",				NULL	},
    {	"",	"46",	"Khmer",				NULL	},
    {	"",	"47",	"Klingon",				NULL	},
    {	"",	"48",	"Korean",				NULL	},
    {	"",	"49",	"Kurdish",				NULL	},
    {	"",	"50",	"Lao",					NULL	},
    {	"",	"51",	"Latin",				NULL	},
    {	"",	"52",	"Latvian",				NULL	},
    {	"",	"53",	"Lithuanian",				NULL	},
    {	"",	"54",	"Malay",				NULL	},
    {	"",	"55",	"Traditional Chinese",			NULL	},
    {	"",	"56",	"Marshallese",				NULL	},
    {	"",	"57",	"Nepali",				NULL	},
    {	"",	"58",	"Norwegian",				NULL	},
    {	"",	"59",	"Occitan",				NULL	},
    {	"",	"60",	"Polish",				NULL	},
    {	"",	"61",	"Portuguese",				NULL	},
    {	"",	"62",	"Punjabi",				NULL	},
    {	"",	"63",	"Romanian",				NULL	},
    {	"",	"64",	"Russian",				NULL	},
    {	"",	"65",	"Sanskrit",				NULL	},
    {	"",	"66",	"Serbian",				NULL	},
    {	"",	"67",	"Sesotho",				NULL	},
    {	"",	"68",	"Slovak",				NULL	},
    {	"",	"69",	"Slovenian",				NULL	},
    {	"",	"70",	"Sinhala",				NULL	},
    {	"",	"71",	"Spanish",				NULL	},
    {	"",	"72",	"Swahili",				NULL	},
    {	"",	"73",	"Swedish",				NULL	},
    {	"",	"74",	"Tagalog",				NULL	},
    {	"",	"75",	"Tamil",				NULL	},
    {	"",	"76",	"Thai",					NULL	},
    {	"",	"77",	"Turkish",				NULL	},
    {	"",	"78",	"Ukrainian",				NULL	},
    {	"",	"79",	"Urdu",					NULL	},
    {	"",	"80",	"Uzbek",				NULL	},
    {	"",	"81",	"Vietnamese",				NULL	},
    {	"",	"82",	"Welsh",				NULL	},
    {	"",	"83",	"Xhosa",				NULL	},
    {	"",	"84",	"Yiddish",				NULL	},
    {	"",	"85",	"Zulu",					NULL	},
    {	"",	"86",	"Other",				NULL	},
    {	"",	NULL,	NULL,					NULL	}
};


#endif	/* _LANGTBL_H_ */

