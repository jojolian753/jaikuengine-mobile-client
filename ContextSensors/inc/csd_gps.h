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

#ifndef CONTEXT_CSD_GPS_H_INCLUDED
#define CONTEXT_CSD_GPS_H_INCLUDED 1

#include "bbdata.h"
#include "concretedata.h"
#include "context_uids.h"

const TTypeName KGpsLineType = { { CONTEXT_UID_SENSORDATAFACTORY }, 12, 1, 0 };
const TTupleName KGpsTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 12 };
const TTupleName KLastKnownGpsTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 46 };

_LIT(KGpsLine, "gps.nmea");
_LIT(KGps, "gps");
_LIT(KLastKnownGps, "last_known_gps");

class TGpsLine : public TBBFixedLengthStringBase {
public:
	TBuf<200> iValue;
    IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	TDes& operator()() { return iValue; }
	const TDesC& operator()() const { return iValue; }

	virtual TDes& Value() { return iValue; }
	virtual const TDesC& Value() const  { return iValue; }
	IMPORT_C TGpsLine& operator=(const TGpsLine& aGpsLine);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:
	TGpsLine() : TBBFixedLengthStringBase(KGpsLine) { }
	TGpsLine& operator=(const TDesC& aValue) { iValue=aValue.Left(200); return *this; }
	bool operator==(const TGpsLine& aRhs) const { return !(iValue.Compare(aRhs.iValue)); }
};

struct TGeoLatLong {
	// signed decimal representation, '.' separator
	TBuf<20>	iLat;
	TBuf<20>	iLong;
};

IMPORT_C void NmeaToLatLong(const TDesC& aNmea, TGeoLatLong& aInto);

#endif
