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

#include "csd_gps.h"

EXPORT_C const TTypeName& TGpsLine::Type() const
{
	return KGpsLineType;
}

EXPORT_C const TTypeName& TGpsLine::StaticType()
{
	return KGpsLineType;
}

EXPORT_C TBool TGpsLine::Equals(const MBBData* aRhs) const
{
	const TGpsLine* rhs=bb_cast<TGpsLine>(aRhs);
	return (rhs && *this==*rhs);
}


EXPORT_C TGpsLine& TGpsLine::operator=(const TGpsLine& aGpsLine)
{
	Value()=aGpsLine.Value();
	return *this;
}

EXPORT_C MBBData* TGpsLine::CloneL(const TDesC&) const
{
	TGpsLine* ret=new (ELeave) TGpsLine();
	*ret=*this;
	return ret;
}

void DegAndMinutesToGeo(const TDesC& aMin, TDes& aInto, TInt aSign)
{
	aInto.Zero();
	if (aMin.Compare(_L("0000.0000"))==0  || aMin.Compare(_L("00000.0000"))==0  || aMin.Length()<3) {
		return;
	}
	TInt decpos=aMin.Locate('.');
	if (decpos==KErrNotFound || decpos<3) return;

	TReal deg, min;
	{
		TLex lex(aMin.Left(decpos-2));
		if (lex.Val(deg)!=KErrNone) {
			return;
		}
	}

	{
		TLex lex(aMin.Mid(decpos-2));
		if (lex.Val(min)!=KErrNone) {
			return;
		}
		if (deg==TReal(0) && min==TReal(0)) return;
		min/=TReal(60);
	}

	deg+=min;

	if (deg > TReal(180)) return;

	deg*=aSign;
	TRealFormat fmt; fmt.iTriLen=0;
	fmt.iPoint='.'; fmt.iPlaces=8;
	fmt.iType=KRealFormatCalculator;
	aInto.Num(deg, fmt);
}

EXPORT_C void NmeaToLatLong(const TDesC& aNmea, TGeoLatLong& aInto)
{
	aInto.iLat.Zero();
	aInto.iLong.Zero();
	// invalid $GPRMC,071546.557,V,6010.6681,N,02457.6963,E,,,070205,,*12
	// valid $GPRMC,143041.940,A,6009.7988,N,02454.3641,E,0.00,,010206,,*10
	TLex lex(aNmea);
	TInt sign=1;
	TBuf<20>  tmp;
	int i=0;
	for (i=0; i<3; i++) {
		// skip $GPRMC,071546.557,V,
		while (! lex.Eos() && lex.Peek() != ',') lex.Inc();
		if (lex.Eos()) goto no_data;
		lex.Inc();
	}
	lex.Mark();
	while (! lex.Eos() && lex.Peek() != ',') lex.Inc();
	if (lex.Eos()) goto no_data;

	tmp=lex.MarkedToken(); lex.Inc();
	if (lex.Get()=='S') {
		sign=-1;
	}
	DegAndMinutesToGeo(tmp, aInto.iLat, sign);

	lex.Inc();
	lex.Mark();
	while (! lex.Eos() && lex.Peek() != ',') lex.Inc();
	if (lex.Eos()) goto no_data;

	tmp=lex.MarkedToken(); lex.Inc();
	sign=1;
	if (lex.Get()=='W') {
		sign=-1;
	}
	DegAndMinutesToGeo(tmp, aInto.iLong, sign);

	// elevation/lock flag

	lex.Inc();
	lex.Mark();
	while (! lex.Eos() && lex.Peek() != ',') lex.Inc();
	if (lex.Eos()) 
		goto no_data;
	if (lex.MarkedToken().Length()==0) 
		goto no_data;


	return;

no_data:
	aInto.iLat.Zero();
	aInto.iLong.Zero();
	return;
}
