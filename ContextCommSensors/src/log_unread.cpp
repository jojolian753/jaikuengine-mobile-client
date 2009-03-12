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

#include "break.h"
#include "log_unread.h"

#include "symbian_auto_ptr.h"
#include "checkedactive.h"
#include "timeout.h"

#include "cc_shareddata.h"
#include <msvapi.h>
#include <msvids.h>

#define MAX_ERRORS	5

#include "csd_unread.h"

/*
 * Concepts:
 * !Accessing missed calls!
 * !Accessing unread messages!
 *
 */

class CLogUnreadImpl : public CLogUnread, public MMsvSessionObserver, public MTimeOut,
	public MSharedDataNotifyHandler {
public:
	virtual ~CLogUnreadImpl();
private:
	CLogUnreadImpl(MApp_context& Context);
	void ConstructL();

	virtual void HandleSessionEventL(TMsvSessionEvent aEvent, 
		TAny* aArg1, TAny* aArg2, TAny* aArg3);
	void expired(CBase* aSource);

	void HandleNotifyL( const TUid aUid, const TDesC& aKey,
                        const TDesC& aValue );

	void GetMissedCalls(const TDesC& aValue=KNullDesC);
	void GetUnreadMessages();

	RSharedDataClient iAgent;

	TInt		iErrorCount;

	CMsvSession	*iSession;
	CTimeOut	*iTimer;

	TBBUnread	iValue;

	friend class CLogUnread;
};

EXPORT_C CLogUnread* CLogUnread::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CLogUnread"), _CL("NewL"), &Context);
	auto_ptr<CLogUnreadImpl> ret(new (ELeave) CLogUnreadImpl(Context));
	ret->ConstructL();
	return ret.release();
}

_LIT(KClass, "CLogUnread");

CLogUnread::CLogUnread(MApp_context& Context) : Mlog_base_impl(Context, KUnread, KUnreadTuple, 5*24*60*60)
{
	CALLSTACKITEM_N(_CL("CLogUnread"), _CL("CLogUnread"));
}

CLogUnreadImpl::~CLogUnreadImpl()
{
	CALLSTACKITEM_N(_CL("CLogUnreadImpl"), _CL("~CLogUnreadImpl"));
	iAgent.Close();
	delete iSession;
	delete iTimer;
}

CLogUnreadImpl::CLogUnreadImpl(MApp_context& Context) : 
	CLogUnread(Context),
	iAgent(this)
	{ }

const TUid KSDUidLogs = {0x101F4CD5};
_LIT( KLogsNewMissedCalls, "NewMisCall" );

void CLogUnreadImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLogUnreadImpl"), _CL("ConstructL"));
	Mlog_base_impl::ConstructL();

	MBBData* existing=0;
	CC_TRAPD(err, iBBSubSession->GetL(KUnreadTuple, KNullDesC, existing, ETrue));
	CBBSensorEvent* event=bb_cast<CBBSensorEvent>(existing);
	const TBBUnread* unread=0;
	if (event) unread=bb_cast<TBBUnread>(event->iData());
	if (unread) iValue=*unread;
	delete existing;

	User::LeaveIfError(iAgent.Connect());
	User::LeaveIfError(iAgent.Assign(KSDUidLogs));
	User::LeaveIfError(iAgent.NotifyChange(KSDUidLogs));


	iTimer=CTimeOut::NewL(*this);
	iSession = CMsvSession::OpenSyncL(*this);
	GetUnreadMessages();
	GetMissedCalls();

	iEvent.iStamp()=GetTime();
	iEvent.iData.SetValue(&iValue); iEvent.iData.SetOwnsValue(EFalse);

	post_new_value(&iValue);
}

void CLogUnreadImpl::GetMissedCalls(const TDesC& aValue)
{
	CALLSTACKITEM_N(_CL("CLogUnreadImpl"), _CL("GetMissedCalls"));
	TInt missed=0;
	if (aValue.Length()==0) {
		User::LeaveIfError(iAgent.GetInt(KLogsNewMissedCalls, missed));
	} else {
		TLex l(aValue);
		l.Val(missed);
	}

	if (missed < iValue.iUnansweredCalls() || iValue.iUnansweredSince()==TTime(0) ||
			(iValue.iUnansweredCalls()==0 && missed>0) )  {
		iValue.iUnansweredSince()=GetTime();
	}
	iValue.iUnansweredCalls()=missed;
}

void CLogUnreadImpl::HandleNotifyL( const TUid /*aUid*/, const TDesC& aKey,
                        const TDesC& aValue )
{
	CALLSTACKITEM_N(_CL("CLogUnreadImpl"), _CL("HandleNotifyL"));
	if (aKey.Compare(KLogsNewMissedCalls)!=0) return;

	CC_TRAPD(err, GetMissedCalls(aValue));
	if (err==KErrNone) {
		post_new_value(&iValue);
	} else {
		post_error(_L("error reading missed calls"), err);
	}
}

void CLogUnreadImpl::GetUnreadMessages()
{
	CALLSTACKITEM_N(_CL("CLogUnreadImpl"), _CL("GetUnreadMessages"));
	TInt unread=0;

	auto_ptr<CMsvEntrySelection> entries(new(ELeave) CMsvEntrySelection);
	{
		auto_ptr<CMsvEntryFilter> filter(CMsvEntryFilter::NewL());
		iSession->GetChildIdsL(KMsvGlobalInBoxIndexEntryId, *filter, *entries);
	}

	TInt i;
	TMsvEntry entry; TMsvId service;
	TInt errcount=0;
	for (i=0; i< entries->Count(); i++) {
		TInt err=0;
		while (errcount<5) { 
			err=iSession->GetEntry(entries->At(i), service, entry);
			if (err==KErrNone || err==KErrNotFound) break;
			User::After(TTimeIntervalMicroSeconds32(100*1000)); //100 ms
			++errcount;
		}
		if (err!=KErrNone && err!=KErrNotFound)
			User::Leave(err);
		if (err!=KErrNotFound && 
			entry.Unread() && entry.Visible() && !entry.InPreparation()) ++unread;
	}
	if (unread < iValue.iUnreadMessages() || iValue.iUnreadSince()==TTime(0) ||
			(iValue.iUnreadMessages()==0 && unread>0) )  {
		iValue.iUnreadSince()=GetTime();
	}
	iValue.iUnreadMessages()=unread;
}

void CLogUnreadImpl::HandleSessionEventL(TMsvSessionEvent aEvent, 
	TAny* aArg1, TAny* aArg2, TAny* aArg3)
{
	CALLSTACKITEM_N(_CL("CLogUnreadImpl"), _CL("HandleSessionEventL"));
	TMsvId id1=0;
	TMsvId id2=0;
	switch(aEvent) {
	case EMsvEntriesCreated:
	case EMsvEntriesChanged:
	case EMsvEntriesDeleted:
		if (aArg2) id1=* static_cast<TMsvId*>(aArg2);
		break;
	case EMsvEntriesMoved:
		if (aArg2) id1 = * static_cast<TMsvId*>(aArg2); 
		if (aArg3) id2 = * static_cast<TMsvId*>(aArg3);
		break;
	};

	if (id1==KMsvGlobalInBoxIndexEntryId || id2==KMsvGlobalInBoxIndexEntryId )
		iTimer->Wait(5);
}

void CLogUnreadImpl::expired(CBase* )
{
	CALLSTACKITEM_N(_CL("CLogUnreadImpl"), _CL("expired"));
	CC_TRAPD(err, GetUnreadMessages());
	if (err!=KErrNone) {
		++iErrorCount;
		if (iErrorCount>MAX_ERRORS) {
			TBuf<50> msg=_L("Error getting unread messages ");
			msg.AppendNum(err);
			post_error(msg, err);
			return;
		}
		iTimer->Wait(10);
	} else {
		iErrorCount=0;
		post_new_value(&iValue);
	}
}
