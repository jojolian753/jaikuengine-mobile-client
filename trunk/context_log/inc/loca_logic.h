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

#ifndef CONTEXT_LOCA_LOGIC_H_INCLUDED
#define CONTEXT_LOCA_LOGIC_H_INCLUDED 1

#include "csd_bluetooth.h"
#include <e32std.h>
#include <e32base.h>
#include "db.h"
#include "symbian_auto_ptr.h"

const TTupleName KLocaLogicTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 35 };
const TTupleName KRemoteLocaLogicTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 37 };
const TTupleName KRemoteBluetoothTuple = { { CONTEXT_UID_CONTEXTNETWORK }, 3 };

const TComponentName KLocaComponent =  { { CONTEXT_UID_CONTEXT_LOG }, 2 };
const TTupleName KLocaScriptTuple = { { CONTEXT_UID_CONTEXT_LOG }, 1001 };
const TTupleName KLocaErrorTuple = { { CONTEXT_UID_CONTEXT_LOG }, 1002 };

class CLocaLogic : public CBase {
public:
	static CLocaLogic* NewL(MApp_context& Context, RDbDatabase& aDb);

	// name and title max length 240
	virtual void UpdateStats(const TDesC& aNodeName,
		const CBBBtDeviceList* devices, const TTime& aAtTime) = 0;
	virtual void GetMessage(const CBBBtDeviceList* devices,
		const TTime& aAtTime,
		TInt& doSendToIndex, 
		TInt& aMessageCode, TDes& aWithName,
		TDes& aWithTitle, auto_ptr<HBufC8>& aBody) = 0;

	enum TSendFailure {
		ETimeOut,
		ERefused,
		EUnknown
	};
	virtual void Failed(const TBBBtDeviceInfo& aDevice,
		TInt aMessageCode,
		const TTime& aAtTime,
		TSendFailure aErrorCode,
		TBool	aLocal) = 0;
	virtual void Success(const TBBBtDeviceInfo& aDevice,
		TInt aMessageCode,
		const TTime& aAtTime,
		TBool	aLocal) = 0;
	virtual void NewScriptL(const TDesC& aSubName, const MBBData* aData) = 0;
};

#endif
