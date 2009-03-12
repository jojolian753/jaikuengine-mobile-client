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

#ifndef CC_PRESENCE_DATA_H_INCLUDED
#define CC_PRESENCE_DATA_H_INCLUDED 1

#include <e32std.h>
#include "xmlbuf.h"
#include "xml.h"

struct MXmlInfo {
	TTime		iUpdated;
	virtual void	Persist(CXmlBuf* into) const = 0;
	MXmlInfo() { iUpdated.HomeTime(); }
	virtual ~MXmlInfo() { }
};

struct TCellInfo : public MXmlInfo {
	TUint		iLAC;
	TUint		iCellId;
	TBuf<20>	iNetwork;
	IMPORT_C virtual void	Persist(CXmlBuf* into) const;
};

struct TLocInfo : public MXmlInfo {
	HBufC*		iCurrent;
	HBufC*		iPrevious;
	HBufC*		iNext;
	TTime		iCUpdated, iPUpdated, iNUpdated;

	TInt		iFlags;
	enum TFlags {
		ECurrent	= 0x0001,
		ENext		= 0x0010,
		EPrevious	= 0x0100
	};
	IMPORT_C virtual void	Persist(CXmlBuf* into) const;
	
	TLocInfo() : iCurrent(0), iPrevious(0), iNext(0), iFlags(0) { }
	~TLocInfo() {
		delete iCurrent;
		delete iPrevious;
		delete iNext;
	}
};

struct TBluetoothInfo : public MXmlInfo {
	struct TBTNode {
		TBuf<30>	iAddr;
		TBuf<30>	iNick;
		TInt		iMajorClass;
		TInt		iMinorClass;
		TInt		iServiceClass;
	};
	CList<TBTNode>	*iNodes;
	
	TBluetoothInfo() : iNodes(0) { }
	~TBluetoothInfo() {
		delete iNodes;
	}
	IMPORT_C virtual void	Persist(CXmlBuf* into) const;
};

struct TActivityInfo : public MXmlInfo {
	enum TMode { EUnknown, EActive, EIdle };
	TMode	iMode;
	IMPORT_C virtual void	Persist(CXmlBuf* into) const;
};

struct TProfileInfo : public MXmlInfo {
	TInt	 iProfileId;
	TInt	 iRingingType;
	TInt	 iRingingVolume;
	TBool	 iVibra;
	TBuf<30> iProfileName;
	IMPORT_C virtual void	Persist(CXmlBuf* into) const;
};

struct TGpsInfo : public MXmlInfo {
	TBuf<100>	iGpsData;
	IMPORT_C virtual void	Persist(CXmlBuf* into) const;
};

struct TNeighbourhoodInfo : public MXmlInfo  {
	TUint	iBuddies;
	TUint	iOtherPhones;
	IMPORT_C virtual void Persist(CXmlBuf* into) const;
};

class MPresenceData {
public:
	virtual const TCellInfo& CellInfo() const = 0;
	virtual const TLocInfo& LocInfo() const = 0;
	virtual const TActivityInfo& ActivityInfo() const = 0;
	virtual const TProfileInfo& ProfileInfo() const = 0;
	virtual const TTime& SendTimeStamp() const = 0;
	virtual const TBluetoothInfo& BluetoothInfo() const = 0;
	virtual const TGpsInfo& GpsInfo() const = 0;
	virtual const TNeighbourhoodInfo& NeighbourhoodInfo() const = 0;
	virtual bool IsSent() const = 0;
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	enum TRingingTypes
            {
            ERingingTypeRinging = 0,
            ERingingTypeAscending,
            ERingingTypeRingOnce,
            ERingingTypeBeepOnce,
            ERingingTypeSilent
            };
};

class CPresenceData : public CBase, public MXmlHandler, public MPresenceData {
public:
	IMPORT_C static CPresenceData* NewL(const TDesC& Xml,const TTime& send_timestamp);
	IMPORT_C ~CPresenceData();
	virtual const TCellInfo& CellInfo() const;
	virtual const TLocInfo& LocInfo() const;
	virtual const TActivityInfo& ActivityInfo() const;
	virtual const TProfileInfo& ProfileInfo() const;
	IMPORT_C const TDesC& RawXml() const;
	virtual const TTime& SendTimeStamp() const;
	virtual const TBluetoothInfo& BluetoothInfo() const;
	virtual const TGpsInfo& GpsInfo() const;
	virtual const TNeighbourhoodInfo& NeighbourhoodInfo() const;
	virtual bool IsSent() const;
	virtual void AddRef();
	virtual void Release();

	IMPORT_C static void MapLocInfo(HBufC * locInfo);
	IMPORT_C static TTime ParseTimeL(const TDesC& Str);
private:
	CPresenceData(const TTime& send_timestamp);
	void ConstructL(const TDesC& Xml);
	void StartElement(const XML_Char *name,
				const XML_Char **atts);

	void EndElement(const XML_Char *name);
	void CharacterData(const XML_Char *s,
				    int len);
	void Error(XML_Error Code, const XML_LChar * String, long ByteIndex);

	void SetStamp(MXmlInfo& Info);
	void ParseStamp(TTime& Stamp);

	enum TParseState {
		EIdle, EEvent, EStamp,
		ECell, ECellLAC, ECellId, ECellNw,
		ELoc, ELocPrevious, ELocCurrent, ELocNext, ELocPUpdate, ELocCUpdate,
		EProfile, EProfileId, EProfileName, EProfileRingType, EProfileRingVolume, EProfileVibra,
		EActivity, EActivityMode, ENeighbourhood, EBuddies, EOtherPhones
	};

	CList<TParseState>* iParseState;

	CXmlParser*	iParser;
	TTime		iStamp;
	HBufC*		iChars;

	TCellInfo	iCellInfo;
	TLocInfo	iLocInfo;
	TActivityInfo	iActivityInfo;
	TProfileInfo	iProfileInfo;
	TBluetoothInfo	iBluetoothInfo;
	TGpsInfo	iGpsInfo;
	TNeighbourhoodInfo iNeighbourhoodInfo;
	HBufC*		iXml;
	HBufC*		iError;
	TInt		iErrorCode;
	TInt		iRefCount;

	TTime		iSendTimeStamp;
};

IMPORT_C void PresenceToListBoxL(const MPresenceData* data, HBufC*& name,
				 HBufC* last_name, HBufC* first_name, const TDesC& prev, const TDesC& not_avail,
				 bool last_name_first=true);

IMPORT_C TBool IsOutOfDate(TTime stamp, TInt freshness_interval = 10 /*in minutes*/);

IMPORT_C TBuf<8> TimeSinceStamp(TTime stamp, TInt minuteInterval=10);


#endif
