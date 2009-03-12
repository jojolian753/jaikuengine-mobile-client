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

#include "cb_presence.h"
#include "symbian_auto_ptr.h"
#include "contextCommon.h"

const TInt CPresenceHolder::KDefaultBufferSize = 256;

CPresenceHolder* CPresenceHolder::NewL(CJabberData& JabberData)
{
	CALLSTACKITEM(_L("CPresenceHolder::NewL"));

	auto_ptr<CPresenceHolder> ret(new (ELeave) CPresenceHolder(JabberData));
	ret->ConstructL();
	return ret.release();
}

CPresenceHolder::~CPresenceHolder()
{
	CALLSTACKITEM(_L("CPresenceHolder::~CPresenceHolder"));

	Cancel();
	if (iSessionOpen) iSession.Close();
	delete iWait;
	delete iListeners;
	delete iPresenceData;

	delete iContact;
	delete iPresence;
	
}

void CPresenceHolder::AddListener(MPresenceListener* Listener)
{
	CALLSTACKITEM(_L("CPresenceHolder::AddListener"));

	iListeners->AppendL(Listener);
}

const CPresenceData* const CPresenceHolder::GetPresence(TInt ContactId) const
{
	CALLSTACKITEM(_L("CPresenceHolder::GetPresence"));

	return (CPresenceData*) iPresenceData->GetData(ContactId);
}

CPresenceHolder::CPresenceHolder(CJabberData& JabberData) : CCheckedActive(EPriorityStandard, _L("CPresenceHolder")), iJabberData(JabberData),
	  iC(0,0), iP(0,0)
{
	CALLSTACKITEM(_L("CPresenceHolder::CPresenceHolder"));

    	CActiveScheduler::Add(this);
}

void CPresenceHolder::Start()
{
	CALLSTACKITEM(_L("CPresenceHolder::Start"));

	if (iSession.ConnectToContextServer() == KErrNone)
	{
		iSessionOpen=true;
		RequestPresence();
	} else {
		Restart();
	}
}

void CPresenceHolder::Restart()
{
	CALLSTACKITEM(_L("CPresenceHolder::Restart"));

	Cancel();
	iWait->Cancel();
	if (iSessionOpen) {
		iSession.Close();
	}
	iSessionOpen=false;
	RDebug::Print(_L("restarting"));
	iWait->Wait(20);
}

void CPresenceHolder::expired(CBase*)
{
	CALLSTACKITEM(_L("CPresenceHolder::expired"));

	Start();
}

void CPresenceHolder::ConstructL()
{
	CALLSTACKITEM(_L("CPresenceHolder::ConstructL"));

	iWait=CTimeOut::NewL(*this);

	iPresenceData=CGenericIntMap::NewL();
	iPresenceData->SetDeletor(CPresenceDataDeletor);
	iListeners=CList<MPresenceListener*>::NewL();

	iContact = HBufC::NewL(KDefaultBufferSize/4);
	iPresence= HBufC::NewL(KDefaultBufferSize);

	iSendTimeStamp = TTime();

	Start();
}

void CPresenceHolder::NewPresence(const TDesC& Nick, const TDesC& Info, const TTime & send_timestamp)
{
	CALLSTACKITEM(_L("CPresenceHolder::NewPresence"));

	TInt contact;
	contact=iJabberData.GetContactIdL(Nick);
	if (contact==KErrNotFound) return;

	auto_ptr<CPresenceData> data( CPresenceData::NewL(Info, send_timestamp) );
	iPresenceData->AddDataL(contact, data.get(), true);
	CPresenceData* dp=data.release();
	dp->AddRef();

	CList<MPresenceListener*>::Node* i=iListeners->iFirst;
	while (i) {
		MPresenceData* mdp=dp;
		TRAPD(err, i->Item->PresenceChangedL(contact, *mdp));
		i->Item->Notify(_L(""));
		i=i->Next;
	}
}

CPresenceData* CPresenceHolder::GetPresence(TInt ContactId)
{
	CALLSTACKITEM(_L("CPresenceHolder::GetPresence"));

	return (CPresenceData*)iPresenceData->GetData(ContactId);
}

void CPresenceHolder::CPresenceDataDeletor(void* p)
{
	CALLSTACKITEM(_L("CPresenceHolder::CPresenceDataDeletor"));

	CPresenceData* b=(CPresenceData*)p;
	b->Release();
}

void CPresenceHolder::CheckedRunL()
{
	CALLSTACKITEM(_L("CPresenceHolder::CheckedRunL"));

	if (iStatus == ERequestCompleted)
	{
		switch (current_state)
		{
			case EWaitingForPresence:
				{
					NewPresence(iC,iP, iSendTimeStamp);
					RequestPresenceNotification();
					break;
				}
			default:
				Restart();
				// ASSERT(0); //Unexpected error
				break;
		}
	}
	else if (iStatus == EBufferTooSmall)
	{
		iContact = iContact->ReAllocL(iC.MaxLength() *2);
		iC.Set(iContact->Des());

		iPresence = iPresence->ReAllocL(iP.MaxLength() *2);
		iP.Set(iPresence->Des());

		RequestPresenceNotification();
	}
	else if (iStatus == EServerUnreachable || iStatus == EIdentificationError )
	{
		CList<MPresenceListener*>::Node* i=iListeners->iFirst;
		while (i) 
		{
			i->Item->Notify(_L("Server unreachable"));
			i=i->Next;
		}
		// ok, now we now that the connection is not up
		// let's just wait for next presence notification
		RequestPresenceNotification();
	}
	else if (iStatus == EContextServerTerminated)
	{
		CList<MPresenceListener*>::Node* i=iListeners->iFirst;
		while (i) 
		{
			i->Item->Notify(_L("Server Stopped"));
			i=i->Next;
		}
		// do nothing, it's a proper stop of the context server ...
	}
	else 
	{
		Restart();
	}
}

void CPresenceHolder::RequestPresence()
{
	CALLSTACKITEM(_L("CPresenceHolder::RequestPresence"));

	if(!IsActive())
	{
		current_state = EWaitingForPresence;
		
		iC.Set(iContact->Des());
		iP.Set(iPresence->Des());

		iSession.MsgRequestPresenceInfo(iC, iP, iSendTimeStamp, iStatus);
		SetActive();
	}	
}

void CPresenceHolder::RequestPresenceNotification()
{
	CALLSTACKITEM(_L("CPresenceHolder::RequestPresenceNotification"));

	if(!IsActive())
	{
		current_state = EWaitingForPresence;

		iC.Set(iContact->Des());
		iP.Set(iPresence->Des());

		iSession.MsgRequestPresenceNotification(iC, iP, iSendTimeStamp, iStatus);
		SetActive();
	}		
}

void CPresenceHolder::CancelRequest()
{
	CALLSTACKITEM(_L("CPresenceHolder::CancelRequest"));

    	Cancel() ;
}

void CPresenceHolder::DoCancel()
{
	CALLSTACKITEM(_L("CPresenceHolder::DoCancel"));

	iSession.Cancel();
}

