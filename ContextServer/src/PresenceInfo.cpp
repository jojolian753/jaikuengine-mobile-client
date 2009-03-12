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

#include "PresenceInfo.h"
#include "ContextServer.h"

#include "settings.h"
#include "cl_settings.h"
#include "SocketsEngine.h"

#include <flogger.h>
#include "app_context.h"
#include "csd_presence.h"
#include "break.h"
#include "bbxml.h"
#include <e32svr.h>
#include "stringmap.h"

CPresenceInfo * CPresenceInfo::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CPresenceInfo"), _CL("NewL"));

	auto_ptr<CPresenceInfo> self( new (ELeave) CPresenceInfo );
	self->ConstructL();
	return self.release();
}


CPresenceInfo::~CPresenceInfo()
{
	CALLSTACKITEM_N(_CL("CPresenceInfo"), _CL("~CPresenceInfo"));

	delete iBBSession;
	delete iPresence;
}

void PresenceDeletor(void* data)
{
	if (!data) return;
	CBBPresence* p=(CBBPresence*) data;
	p->Release();
}

void CPresenceInfo::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPresenceInfo"), _CL("ConstructL"));

	iBBSession=BBSession()->CreateSubSessionL(this);
	iBBSession->AddNotificationL(KPresenceFrozen);

	iPresence=CGenericStringMap::NewL();
	iPresence->SetDeletor(&PresenceDeletor);

	MBBData* fr=0; TInt err;
	CC_TRAP(err, iBBSession->GetL(KPresenceFrozen, KNullDesC, fr, ETrue));
	if (err==KErrNone) {
		const TBBBool* frozen=bb_cast<TBBBool>(fr);
		if (frozen && (*frozen)()) { iFrozen=ETrue; }
	}
	delete fr;

}

void CPresenceInfo::RemoveFlag(TInt pos)
{
}

void CPresenceInfo::AppendFlag(TInt val)
{
}

void CPresenceInfo::CompressFlag()
{
}

void CPresenceInfo::NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
	const TComponentName& aComponentName, const MBBData* aData) 
{
	if (aName==KPresenceFrozen) {
		const TBBBool* frozen=bb_cast<TBBBool>(aData);
		if (frozen && (*frozen)()) {
			iFrozen=ETrue;
		} else {
			iFrozen=EFalse;
		}
	}
}

void CPresenceInfo::DeletedL(const TTupleName& aName, const TDesC& aSubName)
{
	if (aName==KPresenceFrozen) {
		iFrozen=EFalse;
	}
}

void CPresenceInfo::Update(const TDesC & contact, const CBBPresence* data, TBool aPartial)
{
	TInt expirationDays = 5;
	Settings().GetSettingL(SETTING_PRESENCE_EXPIRATION_TIME, expirationDays);

	const CBBPresence* put=0;
	if (aPartial) {
		CBBPresence* previous=(CBBPresence*)iPresence->GetData(contact);
		if (!previous) {
			refcounted_ptr<CBBPresence> copy( (CBBPresence*)data->CloneL(KNullDesC) );
			iPresence->AddDataL(contact, copy.get());
			put=copy.release();
		} else {
			previous->Assign(data);
			put=previous;
		}
	} else {
		put=data;
	}

	TTime expires; expires.HomeTime(); expires+=TTimeIntervalDays( expirationDays );
	iBBSession->PutL(KIncomingPresence, contact, put, expires);
}

void CPresenceInfo::Update(const TDesC & contact, const TDesC& presence, const TTime & time)
{
	CALLSTACKITEM_N(_CL("CPresenceInfo"), _CL("Update"));

	TInt pos;

	if (!iFrozen) {
		TBuf<15> pv2=_L("<presencev2");
		if (presence.Left(pv2.Length()).Compare(pv2)) return;

		TPtrC8 xml8( (TUint8*)presence.Ptr(), presence.Size());

		refcounted_ptr<CBBPresence> data( CBBPresence::NewL() );
		auto_ptr<CSingleParser> parser( CSingleParser::NewL(data.get(), EFalse, ETrue) );
		CC_TRAPD(err, parser->ParseL(xml8) );
		if (err!=KErrNone) return;
		
#ifdef __WINS__
		if (data->iPhoneNumberHash().Length()>0) {
			TBuf<50> msg=_L("hash: ");
			data->iPhoneNumberHash.IntoStringL(msg);
			RDebug::Print(msg);
		}
#endif
		data->iSentTimeStamp()=time;
		Update(contact, data.get(), EFalse);
	}
}


void CPresenceInfo::SetAllAsNew(RArray<TInt>* Flags)
{
}

void CPresenceInfo::SetAsNew(const TDesC & contact, RArray<TInt>* Flags)
{
}
	
void CPresenceInfo::RemoveFlags( RArray<TInt>* Flags)
{
}

void CPresenceInfo::AddFlagsL( RArray<TInt>* Flags)
{
}
