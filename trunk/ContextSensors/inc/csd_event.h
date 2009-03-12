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

#ifndef CONTEXT_CSD_EVENT_H_INCLUDED
#define CONTEXT_CSD_EVENT_H_INCLUDED 1

#include "bbdata.h"
#include "concretedata.h"
#include "context_uids.h"
#include "bbtypes.h"
#include "bbtuple.h"

const TTypeName KEventType = { { CONTEXT_UID_SENSORDATAFACTORY }, 10, 1, 0 };

const TTupleName KAppEventTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 19 };
const TTupleName KCellAnnotationTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 20 };
const TTupleName KWebSearchTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 21 };

const TTupleName KStatusTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 36 };

const TTupleName KAnySensorEvent = { { CONTEXT_UID_CONTEXTSENSORS }, 0 };

#if 1

#include "csd_sensorevent.h"

#else
_LIT(KEvent, "event");

class CBBSensorEvent : public CBase, public TBBCompoundData {
public:
	enum TPriority {
		INFO,
		DEBUG,
		VALUE,
		UNCHANGED_VALUE,
		ERR
	};

	TBBTime			iStamp;
	TBBInt			iPriority;
	TBBShortString		iName;
	CBBGeneralHolder	iData;

	IMPORT_C const TTupleName& TupleName() const;
	/*IMPORT_C const TDesC&    Name() const;*/

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C CBBSensorEvent(const TDesC& aName, const TTupleName& aTupleName, 
		MBBDataFactory* aFactory=0, TTime aTime=TTime(0), MBBData* iData=0, TPriority aPriority=VALUE);

	IMPORT_C bool operator==(const CBBSensorEvent& aRhs) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C CBBSensorEvent& operator=(const CBBSensorEvent& aSensorEvent);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
private:
	IMPORT_C MBBData* GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto);
	MBBDataFactory*	iFactory;
	const TTupleName& iTupleName;
};

#endif

#endif
