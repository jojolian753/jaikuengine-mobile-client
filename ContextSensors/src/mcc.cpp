// Copyright (c) 2007-2009 Google Inc.
// Copyright (c) 2006-2007 Jaiku Ltd.
// Copyright (c) 2002-2006 Mika Raento and Renaud Petit
//
// This software is licensed at your choice under either 1 or 2 below.
//
// 1. MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// 2. Gnu General Public license 2.0
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//
// This file is part of the JaikuEngine mobile client.

#include "mcc.h"

struct TMccItem {
	TInt		iMcc;
	const TText*	iCountryName;
};

/*
 * from the wikipedia
 */

static const TMccItem KMccItems[]= {
	{ 202,  _S("Greece") },
	{ 204,  _S("Netherlands") },
	{ 206,  _S("Belgium") },
	{ 208,  _S("France") },
	{ 212,  _S("Monaco") },
	{ 213,  _S("Andorra") },
	{ 214,  _S("Spain") },
	{ 216,  _S("Hungary") },
	{ 218,  _S("Bosnia and Herzegovina") },
	{ 219,  _S("Croatia") },
	{ 220,  _S("Serbia and Montenegro") },
	{ 222,  _S("Italy") },
	{ 225,  _S("Vatican") },
// 	{ 225,  _S("Vatican City State") },
	{ 226,  _S("Romania") },
	{ 228,  _S("Switzerland") },
	{ 230,  _S("Czech Republic") },
	{ 231,  _S("Slovakia") },
	{ 232,  _S("Austria") },
 	{ 234,  _S("UK") },
 	{ 235,  _S("UK") },
// 	{ 234,  _S("United Kingdom") },
// 	{ 235,  _S("United Kingdom") },
	{ 238,  _S("Denmark") },
	{ 240,  _S("Sweden") },
	{ 242,  _S("Norway") },
	{ 244,  _S("Finland") },
	{ 246,  _S("Lithuania") },
	{ 247,  _S("Latvia") },
	{ 248,  _S("Estonia") },
	{ 250,  _S("Russia") },
	//	{ 250,  _S("Russian Federation") },
	{ 255,  _S("Ukraine") },
	{ 257,  _S("Belarus") },
	{ 259,  _S("Moldova") },
	{ 260,  _S("Poland") },
	{ 262,  _S("Germany") },
	{ 266,  _S("Gibraltar") },
	{ 268,  _S("Portugal") },
	{ 270,  _S("Luxembourg") },
	{ 272,  _S("Ireland") },
	{ 274,  _S("Iceland") },
	{ 276,  _S("Albania") },
	{ 278,  _S("Malta") },
	{ 280,  _S("Cyprus") },
	{ 282,  _S("Georgia") },
	{ 283,  _S("Armenia") },
	{ 284,  _S("Bulgaria") },
	{ 286,  _S("Turkey") },
	{ 288,  _S("Faroe Islands") },
	{ 290,  _S("Greenland") },
	{ 292,  _S("San Marino") },
	{ 293,  _S("Slovenia") },
	{ 294,  _S("Macedonia") },
	{ 295,  _S("Liechtenstein") },
	{ 302,  _S("Canada") },
	{ 308,  _S("Saint Pierre and Miquelon") },
	{ 310,  _S("USA") },
	{ 311,  _S("USA") },
	{ 312,  _S("USA") },
	{ 313,  _S("USA") },
	{ 314,  _S("USA") },
	{ 315,  _S("USA") },
	{ 316,  _S("USA") },
// 	{ 310,  _S("United States of America") },
// 	{ 311,  _S("United States of America") },
// 	{ 312,  _S("United States of America") },
// 	{ 313,  _S("United States of America") },
// 	{ 314,  _S("United States of America") },
// 	{ 315,  _S("United States of America") },
// 	{ 316,  _S("United States of America") },
	{ 330,  _S("Puerto Rico") },
	{ 332,  _S("Virgin Islands") },
	//	{ 332,  _S("United States Virgin Islands") },
	{ 334,  _S("Mexico") },
	{ 338,  _S("Jamaica") },
	{ 340,  _S("Guadeloupe") },
	{ 340,  _S("Martinique") },
	{ 342,  _S("Barbados") },
	{ 344,  _S("Antigua and Barbuda") },
	{ 346,  _S("Cayman Islands") },
	{ 348,  _S("British Virgin Islands") },
	{ 350,  _S("Bermuda") },
	{ 352,  _S("Grenada") },
	{ 354,  _S("Montserrat") },
	{ 356,  _S("Saint Kitts and Nevis") },
	{ 358,  _S("Saint Lucia") },
	{ 360,  _S("Saint Vincent and the Grenadines") },
	{ 362,  _S("Netherlands Antilles") },
	{ 363,  _S("Aruba") },
	{ 364,  _S("Bahamas") },
	{ 365,  _S("Anguilla") },
	{ 366,  _S("Dominica") },
	{ 368,  _S("Cuba") },
	{ 370,  _S("Dominican Republic") },
	{ 372,  _S("Haiti") },
	{ 374,  _S("Trinidad and Tobago") },
	{ 376,  _S("Turks and Caicos Islands") },
	{ 400,  _S("Azerbaijani Republic") },
	{ 401,  _S("Kazakhstan") },
	{ 402,  _S("Bhutan") },
	{ 404,  _S("India") },
	{ 410,  _S("Pakistan") },
	{ 412,  _S("Afghanistan") },
	{ 413,  _S("Sri Lanka") },
	{ 414,  _S("Myanmar") },
	{ 415,  _S("Lebanon") },
	{ 416,  _S("Jordan") },
	{ 417,  _S("Syria") },
	{ 418,  _S("Iraq") },
	{ 419,  _S("Kuwait") },
	{ 420,  _S("Saudi Arabia") },
	{ 421,  _S("Yemen") },
	{ 422,  _S("Oman") },
	{ 424,  _S("United Arab Emirates") },
	{ 425,  _S("Israel") },
	{ 426,  _S("Bahrain") },
	{ 427,  _S("Qatar") },
	{ 428,  _S("Mongolia") },
	{ 429,  _S("Nepal") },
	{ 430,  _S("Abu Dhabi") },
	{ 431,  _S("Dubai") },
// 	{ 430,  _S("United Arab Emirates (Abu Dhabi)") },
// 	{ 431,  _S("United Arab Emirates (Dubai)") },
	{ 432,  _S("Iran") },
	{ 434,  _S("Uzbekistan") },
	{ 436,  _S("Tajikistan") },
	{ 437,  _S("Kyrgyz Republic") },
	{ 438,  _S("Turkmenistan") },
	{ 440,  _S("Japan") },
	{ 441,  _S("Japan") },
	{ 450,  _S("South Korea") },
	{ 452,  _S("Viet Nam") },
	{ 454,  _S("Hong Kong") },
// 	{ 454,  _S("Hong Kong, China") },
	{ 455,  _S("Macao") },
// 	{ 455,  _S("Macao, China") },
	{ 456,  _S("Cambodia") },
	{ 457,  _S("Laos") },
	{ 460,  _S("China") },
	{ 461,  _S("China") },
	{ 466,  _S("Taiwan") },
	{ 467,  _S("North Korea") },
	{ 470,  _S("Bangladesh") },
	{ 472,  _S("Maldives") },
	{ 502,  _S("Malaysia") },
	{ 505,  _S("Australia") },
	{ 510,  _S("Indonesia") },
	{ 514,  _S("East Timor") },
	{ 515,  _S("Philippines") },
	{ 520,  _S("Thailand") },
	{ 525,  _S("Singapore") },
	{ 528,  _S("Brunei Darussalam") },
	{ 530,  _S("New Zealand") },
	{ 534,  _S("Northern Mariana Islands") },
	{ 535,  _S("Guam") },
	{ 536,  _S("Nauru") },
	{ 537,  _S("Papua New Guinea") },
	{ 539,  _S("Tonga") },
	{ 540,  _S("Solomon Islands") },
	{ 541,  _S("Vanuatu") },
	{ 542,  _S("Fiji") },
	{ 543,  _S("Wallis and Futuna") },
	{ 544,  _S("American Samoa") },
	{ 545,  _S("Kiribati") },
	{ 546,  _S("New Caledonia") },
	{ 547,  _S("French Polynesia") },
	{ 548,  _S("Cook Islands") },
	{ 549,  _S("Samoa") },
	{ 550,  _S("Micronesia") },
	{ 551,  _S("Marshall Islands") },
	{ 552,  _S("Palau") },
	{ 602,  _S("Egypt") },
	{ 603,  _S("Algeria") },
	{ 604,  _S("Morocco") },
	{ 605,  _S("Tunisia") },
	{ 606,  _S("Libya") },
	{ 607,  _S("Gambia") },
	{ 608,  _S("Senegal") },
	{ 609,  _S("Mauritania") },
	{ 610,  _S("Mali") },
	{ 611,  _S("Guinea") },
	{ 612,  _S("Cote d'Ivoire") },
	{ 613,  _S("Burkina Faso") },
	{ 614,  _S("Niger") },
	{ 615,  _S("Togolese Republic") },
	{ 616,  _S("Benin") },
	{ 617,  _S("Mauritius") },
	{ 618,  _S("Liberia") },
	{ 619,  _S("Sierra Leone") },
	{ 620,  _S("Ghana") },
	{ 621,  _S("Nigeria") },
	{ 622,  _S("Chad") },
	{ 623,  _S("Central African Republic") },
	{ 624,  _S("Cameroon") },
	{ 625,  _S("Cape Verde") },
	{ 626,  _S("Sao Tome and Principe") },
	{ 627,  _S("Equatorial Guinea") },
	{ 628,  _S("Gabonese Republic") },
	{ 629,  _S("Republic of the Congo") },
	{ 630,  _S("Democratic Republic of the Congo") },
	{ 631,  _S("Angola") },
	{ 632,  _S("Guinea-Bissau") },
	{ 633,  _S("Seychelles") },
	{ 634,  _S("Sudan") },
	{ 635,  _S("Rwandese Republic") },
	{ 636,  _S("Ethiopia") },
	{ 637,  _S("Somalia") },
	{ 638,  _S("Djibouti") },
	{ 639,  _S("Kenya") },
	{ 640,  _S("Tanzania") },
	{ 641,  _S("Uganda") },
	{ 642,  _S("Burundi") },
	{ 643,  _S("Mozambique") },
	{ 645,  _S("Zambia") },
	{ 646,  _S("Madagascar") },
	{ 647,  _S("Reunion") },
	{ 648,  _S("Zimbabwe") },
	{ 649,  _S("Namibia") },
	{ 650,  _S("Malawi") },
	{ 651,  _S("Lesotho") },
	{ 652,  _S("Botswana") },
	{ 653,  _S("Swaziland") },
	{ 654,  _S("Comoros") },
	{ 655,  _S("South Africa") },
	{ 657,  _S("Eritrea") },
	{ 702,  _S("Belize") },
	{ 704,  _S("Guatemala") },
	{ 706,  _S("El Salvador") },
	{ 708,  _S("Honduras") },
	{ 710,  _S("Nicaragua") },
	{ 712,  _S("Costa Rica") },
	{ 714,  _S("Panama") },
	{ 716,  _S("Peru") },
 	{ 722,  _S("Argentina") },
// 	{ 722,  _S("Argentine Republic") },
	{ 724,  _S("Brazil") },
	{ 730,  _S("Chile") },
	{ 732,  _S("Colombia") },
	{ 734,  _S("Venezuela") },
	{ 736,  _S("Bolivia") },
	{ 738,  _S("Guyana") },
	{ 740,  _S("Ecuador") },
	{ 742,  _S("French Guiana") },
	{ 744,  _S("Paraguay") },
	{ 746,  _S("Suriname") },
	{ 748,  _S("Urugua") }
};

#define MCC_COUNT 231
#define OFFSET_OF(type, field) ((int)&( ((type *)0)->field) )

class TMccKey : public TKey {
public:
	TMccKey() : TKey(OFFSET_OF(TMccItem, iMcc), ECmpTInt) { }
private:
	TAny* At(TInt anIndex) const {
		const char * item;
		if (anIndex==KIndexPtr) {
			item=(const char *)iPtr;
		} else {
			item=(const char *)&(KMccItems[anIndex]);
		}
		return (void*)(item+iKeyOffset);
	}
};

EXPORT_C void GetCountryName(TInt aMcc, TDes& aInto)
{
	TMccItem i= { aMcc, _S("") };
	TMccKey k;
	k.SetPtr(&i);
	TInt pos;
	if ( User::BinarySearch(MCC_COUNT, k, pos)==0 ) {
		aInto = (TText16*)KMccItems[pos].iCountryName;
	} else {
		aInto.Zero();
	}
}
