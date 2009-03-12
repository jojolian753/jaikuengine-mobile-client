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

#include "ccu_servermessage.h"
#include "cbbsession.h"
#include "csd_servermessage.h"
#include "bbutil.h"
#include "reporting.h"
#include "app_context.h"
#include <AknNotifyStd.h>

const TTupleName KHandledServerMessageTuple = { { CONTEXT_UID_CONTEXTCONTACTS }, 2 };

class CServerMessageListenerImpl : public CServerMessageListener, public MContextBase, public MBBObserver {
public:
	virtual TBool HasMessage() {
		return (iServerMessage!=0);
	}
	TBBServerMessage *iServerMessage;
	virtual const TBBServerMessage& GetMessage() {
		return *iServerMessage;
	}
	CBBSubSession* iBBSession;
	void ConstructL() {
		iBBSession=BBSession()->CreateSubSessionL(this);
		iBBSession->AddNotificationL(KServerMessageTuple, ETrue);
		MBBData* existing=0;
		iBBSession->GetL(KHandledServerMessageTuple, KNullDesC, existing, ETrue);
		bb_auto_ptr<MBBData> p(existing);
		iServerMessage=bb_cast<TBBServerMessage>(existing);
		if (iServerMessage) p.release();
	}
	virtual void ShowMessage(const TBBServerMessage* msg) {
		if (!msg) return;
		Reporting().ShowGlobalNote(EAknGlobalInformationNote, msg->iBody());
	}
	virtual void ShowMessage() {
		ShowMessage(iServerMessage);
	}
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) {
			const TBBServerMessage* msg=bb_cast<TBBServerMessage>(aData);
			if (!aData) return;
			TTime expires=GetTime(); expires+=TTimeIntervalDays(14);
			iBBSession->PutL( KHandledServerMessageTuple, KNullDesC, msg, expires );
			iBBSession->DeleteL(aId);
			ShowMessage(msg);
			delete iServerMessage; iServerMessage=0;
			iServerMessage=bb_cast<TBBServerMessage>(msg->CloneL(msg->Name()));
	}
	virtual void DeletedL(const TTupleName& , const TDesC& ) { }
	~CServerMessageListenerImpl() {
		delete iBBSession;
		delete iServerMessage;
	}
};


EXPORT_C CServerMessageListener* CServerMessageListener::NewL()
{
	auto_ptr<CServerMessageListenerImpl> ret(new (ELeave) CServerMessageListenerImpl);
	ret->ConstructL();
	return ret.release();
}
