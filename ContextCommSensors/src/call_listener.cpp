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
#ifndef __WINS__
#  ifdef __S60V2__
#include <etel.h>
#include "stripped_etelbgsm.h"
#include "stripped_etelmm.h"
#  else /* __S60V2__ */
#include <etelagsm.h>
#ifdef NO_ETELAGSM_H
#include <etelbgsm.h>
#endif
#  endif
#endif

#include "call_listener.h"
#include <e32std.h>
#include <cntdb.h>
//#include <cntitem.h>
#ifndef __S80__
#include <cpbkcontactitem.h> 
#endif

#include <eikenv.h>
#include <flogger.h>

#define CALL_TIMEOUT 5*60
#define CALL_HANDLING_DELAY  1000*1000 //in microseconds

class CSpecialGroupsImpl: public CSpecialGroups, public CBase, public MContextBase {
public:
	~CSpecialGroupsImpl();
private:
	CSpecialGroupsImpl(MApp_context& Context);
	void ConstructL();
	void read_contact_groups();
	void store_contact(const TDesC& number);
	bool is_special_contact();
	void AddGroupL(const TDesC& GroupName);
	CContactIdArray* current_contact_ids();

	CDesCArrayFlat* special_groups;
	CContactIdArray* special_group_ids;

	CContactIdArray* iCurrent_contact_ids;
	bool	  is_special;
	bool	  special_flag_is_valid;
#ifndef __S80__
	CPbkContactEngine* contacteng;
#endif

	friend class CSpecialGroups;
};

CSpecialGroups* CSpecialGroups::NewL(MApp_context& Context)
{
	auto_ptr<CSpecialGroupsImpl> ret(new (ELeave) CSpecialGroupsImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CSpecialGroupsImpl::CSpecialGroupsImpl(MApp_context& Context) : MContextBase(Context)
{
}

void CSpecialGroupsImpl::ConstructL()
{
	special_groups=new (ELeave) CDesCArrayFlat(4);
	special_group_ids=CContactIdArray::NewL();

#ifndef __S80__
	contacteng=CPbkContactEngine::NewL();
#endif
}

void CSpecialGroupsImpl::AddGroupL(const TDesC& GroupName)
{
	special_groups->AppendL(GroupName);
}

void CSpecialGroupsImpl::read_contact_groups()
{
	CALLSTACKITEM_N(_CL("CSpecialGroupsImpl"), _CL("read_contact_groups"));

#ifndef __S80__
	auto_ptr<CContactIdArray> groups(contacteng->Database().GetGroupIdListL());

	TInt err;
	auto_ptr<CContactGroup> g(0);
	for (int i=0; i<groups->Count(); i++) {
		CC_TRAP(err, g.reset(contacteng->ReadContactGroupL( (*groups)[i])));
		if (err!=KErrNone) {
			/*TBuf<100> msg=_L("Error reading group");;
			msg.AppendNum((*groups)[i]);
			msg.Append(_L(": "));
			msg.AppendNum(err);
			cb->error(msg);*/
		} else {
			for (int j=0; j<special_groups->Count(); j++) {
				if (! g->GetGroupLabelL().CompareF( (*special_groups)[j] ) ) {
					special_group_ids->AddL( (*groups)[i] );
					break;
				}
			}
		}
	}
#endif
}

void CSpecialGroupsImpl::store_contact(const TDesC& number)
{
	delete iCurrent_contact_ids;
	iCurrent_contact_ids=0;

	if (number.Length()==0) {
		special_flag_is_valid=true;
		is_special=false;
		return;
	}

	// uses 8 last digits. This is what the phone normally
	// uses, but is it correct?
	TInt phoneno;
	TInt digits_used;
	//iState=_L("TextToPhoneMatchNumber");
#ifndef __S80__
	phoneno=contacteng->Database().TextToPhoneMatchNumber(number, digits_used);

	special_flag_is_valid=false;
	//iState=_L("MatchPhoneNumberL");
	CC_TRAPD(err, iCurrent_contact_ids=contacteng->Database().MatchPhoneNumberL(phoneno));
	if (err!=KErrNone) {
		special_flag_is_valid=true;
		is_special=false;
	}
#else
	special_flag_is_valid=true;
	is_special=false;
#endif
	return;
}

bool CSpecialGroupsImpl::is_special_contact()
{
	CALLSTACKITEM_N(_CL("CSpecialGroupsImpl"), _CL("is_special_contact"));

	if (special_flag_is_valid) return is_special;
	is_special=false;

#ifndef __S80__
	auto_ptr<CPbkContactItem> item(0);
	for (int i=0; iCurrent_contact_ids && !is_special &&
			i<iCurrent_contact_ids->Count(); i++) {
		item.reset(contacteng->ReadContactL((*iCurrent_contact_ids)[i]));
		CContactIdArray* groups=item->GroupsJoinedLC();
		for (int belongs_to=0; !is_special && 
				belongs_to<groups->Count(); belongs_to++) {
			for ( int special=0; !is_special &&
					special<special_group_ids->Count(); special++) {
				if ( (*special_group_ids)[special] == (*groups)[belongs_to] ) {
					is_special=true;
					//if (cb) cb->status_change(_L("special"));
					break;
				}
			}
		}
		CleanupStack::PopAndDestroy(); // groups
	}
#else
	is_special=false;
#endif
	special_flag_is_valid=true;

	return is_special;
}

CContactIdArray* CSpecialGroupsImpl::current_contact_ids()
{
	return iCurrent_contact_ids;
}

CSpecialGroupsImpl::~CSpecialGroupsImpl()
{
	delete iCurrent_contact_ids;
#ifndef __S80__
	delete contacteng;
#endif
	delete special_groups;
	delete special_group_ids;
	
}

//------------------------------------------------------------------------------------------------

Ccall_listener::Ccall_listener(MApp_context& Context) : 
	CCheckedActive(EPriorityNormal, _L("Ccall_listener")), MContextBase(Context)
{
}

Ccall_listener::~Ccall_listener()
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("~Ccall_listener"));

	Cancel();

	close_call();
	delete call; call=0;
	line.Close();

	delete iSpecialGroups;

	delete iTimer;
	//delete id;
}

void Ccall_listener::DoCancel()
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("DoCancel"));

	switch (current_state) {
	case LISTENING_INCOMING:
		if (iDir==OUTGOING) 
			line.NotifyCallAddedCancel();
		else
			line.NotifyIncomingCallCancel();
		break;
	case CALL_IN_PROGRESS:
#ifndef __S60V2__
		call->NotifyStatusChangeCancel();
#else
		call->NotifyStatusChangeCancel();
#endif
		break;
	};
}

void Ccall_listener::ConstructL(i_status_notif* i_cb, TDirection dir)
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("ConstructL"));

	iDir=dir;

	//id = CCallerIdMngr::NewL(this);

	iTimer=CTimeOut::NewL(*this);
	iSpecialGroups=CSpecialGroups::NewL(AppContext());

#ifdef __WINS__
	class RCall *call;
#else
#  ifndef __S60V2__
#    ifndef NO_ETELAGSM_H
	call=new (ELeave) RAdvGsmCall;
#    else
	call=new (ELeave) RCall;
#    endif
#  else
	call=new (ELeave) RMobileCall;
#  endif
#endif

#ifndef __WINS__
        RPhone::TLineInfo lineinfo;
        Phone().GetLineInfo(0, lineinfo);
        line.Open(Phone(), lineinfo.iName);

	CActiveScheduler::Add(this);
	listen_for_call();
#endif
	cb=i_cb;

}

void Ccall_listener::open_call()
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("open_call"));

	close_call();
	User::LeaveIfError(call->OpenExistingCall(line, call_name));
	call_is_open=true;
}

void Ccall_listener::close_call()
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("close_call"));

	if (!call_is_open) return;
	call->Close();
	call_is_open=false;
}

void Ccall_listener::listen_for_call()
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("listen_for_call"));

	iTimer->Reset();

	if (iDir==OUTGOING) 
		line.NotifyCallAdded(iStatus, call_name);
	else
		line.NotifyIncomingCall(iStatus, call_name);
	current_state=LISTENING_INCOMING;
	SetActive();
}

void Ccall_listener::store_contact()
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("store_contact"));

	call_start_time=GetTime();

#ifndef NO_ETELAGSM_H
	TBuf<30> msg;
	
	iState=_L("GetRemotePartyInfo");
#  ifndef __WINS__
#  ifndef __S60V2__
	MAdvGsmCallInformation::TRemotePartyInfo info;
	if (call->GetRemotePartyInfo(info)!=KErrNone) {
		msg.Format(_L("No info available"));
		cb->error(msg);
		iSpecialGroups->store_contact(_L(""));
		return;
	}
	TGsmTelNumber number;
	if (info.iDirection==MAdvGsmCallInformation::EMobileTerminated) {
		if (info.iRemoteIdentityStatus!=MAdvGsmCallInformation::ERemotePartyIdentityAvailable) {
			msg.Format(_L("No info available"));
			cb->error(msg);
			iSpecialGroups->store_contact(_L(""));
			return;
		}
		number=info.iNumber;
	} else {
		iState=_L("GetDialledNumberInfo");
		MAdvGsmCallInformation::TDialledNumberInfo dialled;
		User::LeaveIfError(call->GetDialledNumberInfo(dialled));
		number=dialled.iNumber;
	}
	
	iSpecialGroups->store_contact(number.iTelNumber);

#  else // __S60V2__

	RMobileCall::TMobileCallInfoV1 callInfo;
	RMobileCall::TMobileCallInfoV1Pckg callInfoPckg(callInfo);
	call->GetMobileCallInfo(callInfoPckg);

	if (callInfo.iRemoteParty.iDirection == RMobileCall::EMobileTerminated)
	{
		if (callInfo.iRemoteParty.iRemoteNumber.iTelNumber.Length() == 0)
		{
			TBuf<100> msg;
			msg.Format(_L("No info available"));
			cb->error(msg);
			iSpecialGroups->store_contact(_L(""));
			return;
		}
		msg.Format(callInfo.iRemoteParty.iRemoteNumber.iTelNumber);
		cb->error(msg);
		iSpecialGroups->store_contact(callInfo.iRemoteParty.iRemoteNumber.iTelNumber);
	}
	else
	{
		if (callInfo.iDialledParty.iTelNumber.Length() == 0)
		{
			TBuf<100> msg;
			msg.Format(_L("No info available"));
			cb->error(msg);
			iSpecialGroups->store_contact(_L(""));
			return;
		}
		msg.Format(callInfo.iDialledParty.iTelNumber);
		cb->error(msg);
		iSpecialGroups->store_contact(callInfo.iDialledParty.iTelNumber);
	}

	

#  endif //__S60V2__
#  endif // __WINS__

	
#else // NO_ETELAGSM_H
	{
		TBuf<100> msg;
		msg.Format(_L("No info available"));
		cb->error(msg);
		iSpecialGroups->store_contact(_L(""));
		return;
	}
#endif
}

void Ccall_listener::LastCallerId(const TDesC& caller_id)
{
	last_caller.Copy(caller_id);
}

void Ccall_listener::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("expired"));

	if (cb) cb->error(_L("Call listen timed out"));
	Cancel();
	if (got_info_for_this_call && current_state==CALL_IN_PROGRESS) {
		handle_disconnected();
	}
	listen_for_call();
}

TInt Ccall_listener::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("CheckedRunError"));

	TInt err=KErrNone;
	if (got_info_for_this_call && current_state==CALL_IN_PROGRESS) {
		handle_disconnected();
	}
	Cancel();
	CC_TRAP(err, listen_for_call());
	TBuf<100> msg;
	msg.Format(_L("Restoring from error in CheckedRunL %d, state %S"), aError, &iState);
	if (cb) cb->error(msg);
	return err;
}

void Ccall_listener::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("Ccall_listener"), _CL("CheckedRunL"));

	//((MFLDFileProcessor*)ringp)->Cancel();
	if (iStatus!=KErrNone) {
		TBuf<100> msg;
		msg.Format(_L("error %d at state %d"), iStatus, current_state);
		cb->error(msg);
		listen_for_call();
		return;
	}

	iTimer->Wait(CALL_TIMEOUT);

	TInt err;
	bool fallthru=false;

	switch (current_state) {
	case LISTENING_INCOMING:
		{
		last_caller.Zero();
		//id->GetLatest();
		if (cb) cb->status_change(_L("call"));
		iState=_L("inc; close");
		close_call();
		iState=_L("inc; OpenExisting");
		open_call();
		got_info_for_this_call=false;
		if (iDir==INCOMING) {
			iState=_L("inc; store_contact");
			store_contact();
			iState=_L("inc; handle_incoming");
			handle_incoming();
			got_info_for_this_call=true;
		}
		// The call might already be disconnected
		// when we get here. In that case there won't be
		// any further changes and we get stuck. So let's
		// get the status first, and wait for notification
		// afterwards (hence the fallthru)

		iState=_L("inc; GetStatus");
#ifdef __S60V2__
		err=call->GetStatus(current_call_status);
#else
		err=call->GetStatus(current_call_status);	
#endif
		
		if (err!=KErrNone) {
			iState=_L("inc; err; handle_disconnected");
			if (got_info_for_this_call) handle_disconnected();
			TBuf<100> msg;
			msg.Format(_L("error %d at GetGsmStatus"), err);
			if (cb) cb->error(msg);
			iState=_L("inc; err; listen_for_call");
			listen_for_call();
			return;
		}
		current_state=CALL_IN_PROGRESS;
		fallthru=true;
		// FALLTHRU!
		}
	case CALL_IN_PROGRESS:
		{
		// remote info is not available until here
		if (!fallthru && !got_info_for_this_call) {
			iState=_L("!fallthru; close");
			close_call();
			iState=_L("!fallthru; openexisting");
			open_call();
			iState=_L("!fallthru; store_contact");
			store_contact();
			iState=_L("!fallthru; handle_incoming");
			handle_incoming();
			got_info_for_this_call=true;
			iState=_L("!fallthru; GetStatus");
#ifndef __S60V2__
			err=call->GetStatus(current_call_status);
#else
			err=call->GetStatus(current_call_status);
#endif
			if (err!=KErrNone) {
				iState=_L("!fallthru; err; handle_disconnected");
				handle_disconnected();
				TBuf<100> msg;
				msg.Format(_L("error %d at GetStatus"), iStatus, current_state);
				if (cb) cb->error(msg);
				iState=_L("!fallthru; err; listen_for_call");
				listen_for_call();
				return;
			}
		}
		TBuf<30> msg;
		msg.Format(_L("call status %d"), (TInt)current_call_status);
		if (cb) cb->status_change(msg);
		}
		{
		switch (current_call_status) {
		case RCall::EStatusRinging:
			iState=_L("ringing; notifystatuschange");
#ifndef __S60V2__
			call->NotifyStatusChange(iStatus, current_call_status);
#else
			call->NotifyStatusChange(iStatus, current_call_status);
#endif
			SetActive();
			break;
		//case RCall::EStatusDisconnecting:
		case RCall::EStatusHangingUp:
			iState=_L("hangup; handle_disconnected");
			// other end disconnected
			handle_disconnected();
			iState=_L("hangup; Close");
			close_call();
			iState=_L("hangup; listen_for_call");
			listen_for_call();
			break;
		case RCall::EStatusConnected:
		case RCall::EStatusAnswering:
			// we or they answered
			iState=_L("answer; handle_answered");
			handle_answered();
			iState=_L("answer; close");
			close_call(),
			iState=_L("answer; listen");
			listen_for_call();
			break;
		case RCall::EStatusIdle:
			if (cb) cb->status_change(_L("idle"));
			// we refused
			iState=_L("idle; handle_refused");
			handle_refused();

			iState=_L("answer; Close");
			close_call();
			iState=_L("answer; listen_for_call");
			listen_for_call();
			break;
		default:
			//if (cb) cb->status_change(msg);
			iState=_L("default; NotifyStatusChange");
#ifndef __S60V2__
			call->NotifyStatusChange(iStatus, current_call_status);
#else
			call->NotifyStatusChange(iStatus, current_call_status);
#endif
			SetActive();
			break;
		}
		}
		break;
	default:
		break;
	}
}

//----------------------------------------------------------------------------------


CCallerIdMngr::CCallerIdMngr(MCallerIdMngrObserver* obs)	
	: CCheckedActive(EPriorityNormal, _L("CCallerIdMngr")), iObs(obs)
{
	CALLSTACKITEM_N(_CL("CCallerIdMngr"), _CL("CCallerIdMngr"));
}

CCallerIdMngr* CCallerIdMngr::NewLC(MCallerIdMngrObserver * obs)
{
	CALLSTACKITEM_N(_CL("CCallerIdMngr"), _CL("NewLC"));
	
	CCallerIdMngr* self=new (ELeave) CCallerIdMngr(obs);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CCallerIdMngr* CCallerIdMngr::NewL(MCallerIdMngrObserver * obs)
{
	CALLSTACKITEM_N(_CL("CCallerIdMngr"), _CL("NewL"));
	
	CCallerIdMngr* self = NewLC(obs);
	CleanupStack::Pop();
	return self;
}

void CCallerIdMngr::ConstructL()
{
	CALLSTACKITEM_N(_CL("CCallerIdMngr"), _CL("ConstructL"));
	
	current_state=IDLE;

	iLogClient = CLogClient::NewL(CEikonEnv::Static()->FsSession());
	iRecentLogView = CLogViewRecent::NewL(*iLogClient);

	CActiveScheduler::Add(this); 
}

CCallerIdMngr::~CCallerIdMngr(){
	
	Cancel();
	delete iRecentLogView;
	delete iLogClient;
}

void CCallerIdMngr::DoCancel(){
	
	if (current_state!=IDLE && iRecentLogView) iRecentLogView->Cancel();
	current_state=IDLE;
}

TInt CCallerIdMngr::RunError(TInt /*aError*/){
	
	return KErrNone;
}

void CCallerIdMngr::CheckedRunL(){
	if(iStatus == KErrNone)
	{
		if (current_state == WAITING)
		{
			const CLogEvent& event = iRecentLogView->Event();
			iLastNumber.Copy(event.Number());
			iObs->LastCallerId(iLastNumber);
		}
	}
	current_state = IDLE;
}

void CCallerIdMngr::GetLatest()
{
	CALLSTACKITEM_N(_CL("CCallerIdMngr"), _CL("GetLatest"));

        if (current_state != IDLE ) iRecentLogView->Cancel();
	iLastNumber.Zero();

	if( iRecentLogView->SetRecentListL(KLogNullRecentList, iStatus))
	{
		current_state = WAITING;
		SetActive();
	}
}
