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
#include "call_log.h"
#include <eikenv.h>
#include <rpbkviewresourcefile.h>
#include <cpbkcontactitem.h>
#include <pbkiconinfo.h> 
#include "icons.h"
#include <contextcalllog.rsg>
#include <flogger.h>

//#define CREATE_EVENT 1

call_log::call_log(MApp_context& Context, CJabberData* JabberData, CPresenceHolder* PresenceHolder) : 
CCheckedActive(CActive::EPriorityIdle, _L("call_log")), MContextBase(Context), current_state(IDLE), iJabberData(JabberData), iPresenceHolder(PresenceHolder)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("call_log"));
}

void call_log::Notify(const TDesC & /*aMessage*/)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("Notify"));
}

void call_log::NotifyContentsChanged(TInt aContactId, TBool aPresenceOnly)
{
	CList<phonebook_observer*>::Node *n=iObservers->iFirst;
	while (n) {
		n->Item->contents_changed(aContactId, aPresenceOnly);
		n=n->Next;
	}
}

void call_log::PresenceChangedL(TInt ContactId, CBBPresence* Info)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("PresenceChangedL"));

	CList<phonebook_observer*>::Node *n=iObservers->iFirst;
	while (n) {
		n->Item->before_change();
		n=n->Next;
	}

	int i=0;
	bool change =false;
	for (i=0; i< current_contacts->Count(); i++)
	{
		contact* c= (*current_contacts)[i];
		if ( c->id == ContactId)
		{
			c->set_presence(Info);
			change =true;
		}
	}
	if (change) {
		NotifyContentsChanged(ContactId, ETrue);
	}
}

TInt call_log::Count()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("Count"));
	
	if (current_contacts) 
	{
		return current_contacts->Count();
	}
	else
	{
		return 0;
	}
}


call_log::~call_log()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("~call_log"));

	if (iObservers) {
		CList<phonebook_observer*>::Node *n=iObservers->iFirst;
		while (n) {
			n->Item->exiting();
			n=n->Next;
		}
	}
	delete iObservers;

	Cancel();
	delete recent;
	delete logclient;
	delete logfilter;
	delete current_nos;
	delete all_nos;

	
	if (current_events) {
		for (int i=0; i<current_events->Count(); i++) {
			delete (*current_events)[i];
		}
	}
	delete current_events;
	delete all_events;
	
	if (current_contacts) {
		for (int i=0; i<current_contacts->Count(); i++) {
			delete (*current_contacts)[i];
		}
	}
        delete current_contacts;
	delete all_contacts;

	if (owns_engine) delete eng;
}

void call_log::ConstructL()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("ConstructL"));

	iObservers=CList<phonebook_observer*>::NewL();

	eng=CPbkContactEngine::Static();
	if (eng) {
		owns_engine=false;
	} else {
		eng=CPbkContactEngine::NewL();
		owns_engine=true;
	}

	all_contacts=new (ELeave) CArrayFixFlat<contact*>(20);
	current_contacts = new (ELeave) CArrayFixFlat<contact*>(20);
	all_nos = new (ELeave) CDesCArrayFlat(50);
	current_nos = new (ELeave) CPtrCArray(8);
	all_events = new (ELeave) CArrayFixFlat<CLogEvent*>(20);
	current_events = new (ELeave) CArrayFixFlat<CLogEvent*>(20);
	
	logclient=CLogClient::NewL(CEikonEnv::Static()->FsSession());
	recent=CLogViewRecent::NewL(*logclient);
	logfilter=CLogFilter::NewL();
	logfilter->SetEventType(KLogCallEventTypeUid);

	CActiveScheduler::Add(this);

#ifdef __WINS__
	
#ifdef CREATE_EVENT	
	CLogEvent * ev = CLogEvent::NewL();
	ev->SetEventType(KLogCallEventTypeUid);
	ev->SetDirection(_L("Outgoing"));
	ev->SetDurationType(KLogDurationValid);
	ev->SetDuration(0);
	TTime t;
	ev->SetTime(t);
	ev->SetRemoteParty(_L(""));
	ev->SetNumber(_L("29382"));
	TRequestStatus st;
	logclient->AddEvent(*ev, st);
	User::WaitForAnyRequest();
#endif
#endif 
	do_refresh=false; 
	new_from=0;
}

void call_log::ReRead()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("ReRead"));

	if (current_state!=IDLE) {
		do_refresh=true;
		return;
	}
	getting_first=true;
	new_from=all_contacts->Count();
	
	TBool issued = recent->SetRecentListL(KLogNullRecentList, *logfilter, iStatus);
	
	if (issued && iStatus==KRequestPending) {
		current_state=WAITING_SET;
		SetActive();
	} else {
		current_state=IDLE;
		NotifyContentsChanged(0, EFalse);
	}
}

TInt call_log::GetIndex(TInt ) 
{ 
	CALLSTACKITEM_N(_CL("call_log"), _CL("GetIndex"));

	return KErrNotFound; 
}

TTime call_log::GetAtTime(TInt aIndex)
{
	return GetTime();
}

const TDesC& call_log::PresenceSuffix()
{
	return KNullDesC;
}


TPtrC call_log::get_phone_no(TInt index)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("get_phone_no"));

	if (index < 0) return 0;

	return (*current_nos)[index];
}

bool call_log::handle_event(const CLogEvent& ev)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("handle_event"));

	if (ev.Id()==last_seen_id ) return false;
	
	if (getting_first) {
		last_seen_id=ev.Id();
		getting_first=false;
	}

	CLogEvent * aEv = CLogEvent::NewL();
	CleanupStack::PushL(aEv);
	aEv->CopyL(ev);
	CleanupStack::Pop();

	all_events->AppendL(aEv);
	all_nos->AppendL( aEv->Number() );

	contact *c =0;

	TBuf<100> first_name;
	TBuf<100> jabber_nick;
	TLocale locale;

	// TIME OF CALL FORMATTING
	TTime tt=aEv->Time();
	TTimeIntervalSeconds offset(locale.UniversalTimeOffset());
	tt+=offset;
	if (locale.QueryHomeHasDaylightSavingOn()) {
		TTimeIntervalHours ds(1);
		tt+=ds;
	}
	TDateTime t=tt.DateTime();

	// Converting phone number to Int
	TInt nbDigits, phoneno, errorCode = 0;
	phoneno = eng->Database().TextToPhoneMatchNumber(aEv->Number(), nbDigits, 8);
	CContactIdArray * matching_contact_ids = 0;
	if (phoneno != 0) {
		TInt co = eng->Database().CountL();
		if  (co > 0) {
			CC_TRAP(errorCode, matching_contact_ids = eng->Database().MatchPhoneNumberL(phoneno) );
		} else {
			errorCode =KErrNotFound;
		}
	}
	if (phoneno && (errorCode != KErrNotFound) ) {
		// there is (at least) 1 matching contact in db! Let's take 1st one
                		
		CleanupStack::PushL(matching_contact_ids);
		CPbkContactItem * item=eng->ReadContactL((*matching_contact_ids)[0]);
		CleanupStack::PushL(item);
		
		TPbkContactItemField* f;
		
		f=item->FindField(EPbkFieldIdFirstName);
		if (f) {
			first_name = f->Text();
		} else {
			first_name = _L("");
		}

		f=item->FindField(EPbkFieldIdLastName);
		if (f) {
			if (first_name.Length() >0) {first_name.Append(_L(" "));}
			first_name.Append( f->Text());
		} 

		TDateTime now; TTime nowt; nowt.HomeTime();
		now=nowt.DateTime();
		
		c=contact::NewL(item->Id(), first_name, _L(""));
		if (now.Month()==t.Month() && t.Day()==now.Day()) {
			c->time.AppendFormat(_L(" %02d:%02d"),
				(TInt)t.Hour(), (TInt)t.Minute() );
		} else {
			c->time.AppendFormat(_L(" %02d/%02d"),
				(TInt)t.Day()+1, (TInt)t.Month()+1
				);
		}

		CleanupStack::PushL(c);
		if (iJabberData && iJabberData->GetJabberNickL(item->Id(), jabber_nick) && jabber_nick.Length()>1) {
			c->has_nick=true;
		}
		all_contacts->AppendL(c);
		CleanupStack::Pop(c); // pop?
		CleanupStack::PopAndDestroy(item);
		CleanupStack::PopAndDestroy(matching_contact_ids);
	} else {
		TInt contact_id; /* 0 if a call with valid number but no contact id in db */
				/* -1 if no info about caller */

		if (aEv->RemoteParty().Length()>0){ 
			first_name.Append(aEv->RemoteParty());
			contact_id = 0;
		} else if (aEv->Number().Length()>0) {
			first_name.Append(aEv->Number());
			contact_id = 0;
		} else {
			HBufC* no_number = CEikonEnv::Static()->AllocReadResourceLC(R_NO_NUMBER);
			first_name.Append(*no_number);
                        delete no_number; no_number = 0;
			contact_id = -1;
		}

		TDateTime now; TTime nowt; nowt.HomeTime();
		now=nowt.DateTime();

		c = contact::NewL(contact_id, first_name, _L(""));
		if (now.Month()==t.Month() && t.Day()==now.Day()) {
			c->time.AppendFormat(_L(" %02d:%02d"),
				(TInt)t.Hour(), (TInt)t.Minute() );
		} else {
			c->time.AppendFormat(_L(" %02d/%02d"),
				(TInt)t.Day()+1, (TInt)t.Month()+1
				);
		}
		CleanupStack::PushL(c);
		c->has_nick = false;
		all_contacts->AppendL(c);
		CleanupStack::Pop(c);  //pop?
	}
	return true;
}

void call_log::copy_new_to_current()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("copy_new_to_current"));

	// trying to be efficient
	int new_count;
	new_count=all_contacts->Count()-(new_from);
	if (new_count <= 0) return;

	int i;
	current_contacts->ResizeL(new_count);
	current_nos->ResizeL(new_count);
	current_events->ResizeL(new_count);
	
	for (i=0;i<new_count;i++) {
		current_events->At(i) = ((*all_events)[new_from+i]);
		contact *c = current_contacts->At(i) = ((*all_contacts)[new_from+i]);
		if (iPresenceHolder) {c->set_presence(iPresenceHolder->GetPresence(c->id));}
		current_nos->At(i).Set((*all_nos)[new_from+i]);	
	}
}
		
void call_log::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("CheckedRunL"));

	if (iStatus!=KErrNone) {
		Cancel();
		//auto_ptr<CAknGlobalNote> note (CAknGlobalNote::NewL());
		//note->ShowNoteL(EAknGlobalConfirmationNote, _L("RunL Error"));
		return;
	}
	
	switch (current_state) {

	case WAITING_DELETE:
		current_state=IDLE;
		reset_call_log();
		do_refresh = true;
		break;

	case WAITING_CLEAR:
		delete (*current_events)[current_events->Count()-1];
		current_events->Delete(current_events->Count()-1);

		if (current_events->Count() > 0) {
			logclient->DeleteEvent( ((*current_events)[current_events->Count()-1])->Id(), iStatus);
			SetActive();
		} else {
			current_state=IDLE;
			reset_call_log();
			do_refresh = true;	
		}
		break;
		
	
	case WAITING_SET:
		if (recent->FirstL(iStatus) && iStatus==KRequestPending) {
			SetActive();
			current_state=WAITING_NEXT;
		} else {
			NotifyContentsChanged(0, EFalse);
			current_state=IDLE;
		}
		break;

	case WAITING_NEXT:
		if ( handle_event(recent->Event()) ) {
			if (recent->NextL(iStatus) && iStatus==KRequestPending) {
				SetActive();
			} else {
				copy_new_to_current();
				NotifyContentsChanged(0, EFalse);
				current_state=IDLE;
			}
		} else {
			// seen rest already
			copy_new_to_current();
			NotifyContentsChanged(0, EFalse);
			current_state=IDLE;
		}
		break;

	default:
		break;
	}

	if (current_state==IDLE && do_refresh) {
		// redo queued
		do_refresh=false;
		ReRead();
	}
}

void call_log::DoCancel()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("DoCancel"));

	switch (current_state) {

	case WAITING_DELETE:
	case WAITING_CLEAR:
		if (logclient) logclient->Cancel();
		break;
	
	case WAITING_SET:
	case WAITING_NEXT:
		if (recent) recent->Cancel();
		break;

	default:
		break;
	}
	current_state=IDLE;
}

void call_log::AddObserverL(phonebook_observer* i_obs)
{
	if (!i_obs) return;
	iObservers->AppendL(i_obs);
}

void call_log::RemoveObserverL(phonebook_observer* i_obs)
{
	if (!i_obs) return;
	CList<phonebook_observer*>::Node *n=iObservers->iFirst;
	while (n) {
		if (n->Item == i_obs) {
			iObservers->DeleteNode(n, true);
			break;
		}
	}
}
contact * call_log::GetContact(TInt index)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("GetContact"));

	if ( (current_contacts->Count() == 0) || (index >= (current_contacts->Count())) ) return 0;
	return (*current_contacts)[index];
}

void call_log::reset_call_log()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("reset_call_log"));

	int i=0;
	for (i=0; i<current_contacts->Count(); i++) {
		delete (*current_contacts)[i];
	}
	for (i=0; i<current_events->Count(); i++) {
		delete (*current_events)[i];
	}
	current_contacts->Reset();
	current_nos->Reset();
	current_events->Reset();
	do_refresh=false; 
	new_from=0;
	last_seen_id = KLogNullId;
}


void call_log::DeleteEvent(TInt index)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("DeleteEvent"));

	if ( (index < current_events->Count()) && (current_state==IDLE) ) {
		logclient->DeleteEvent(((*current_events)[index])->Id(), iStatus);
		if (iStatus==KRequestPending) {
			current_state=WAITING_DELETE;
			SetActive();
		}
	}
}

void call_log::ClearEventList()
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("ClearEventList"));

	if ( (current_state==IDLE) && (current_events->Count()>0) ) {
		logclient->DeleteEvent(((*current_events)[current_events->Count()-1])->Id(), iStatus);
		if (iStatus==KRequestPending)  {
			current_state=WAITING_CLEAR;
			SetActive();
		}
	}
}

TInt call_log::GetContactId(TInt Index)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("GetContactId"));

	if (Index >= current_contacts->Count() ) return -1;
	return ((*current_contacts)[Index])->id;
}

void call_log::SetFilter(TFilter aFilter)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("SetFilter"));

	if (aFilter==EMissed) {
		if (current_state!=IDLE) Cancel();
		reset_call_log();
		logfilter->SetDirection(_L("Missed call"));
	} else if (aFilter==EReceived) {
		if (current_state!=IDLE) Cancel();
		reset_call_log();
		logfilter->SetDirection(_L("Incoming"));
	} else if (aFilter==EDialled) {
		if (current_state!=IDLE) Cancel();
		reset_call_log();
		logfilter->SetDirection(_L("Outgoing"));
	}
}

CLogEvent * call_log::get_event(TInt index)
{
	CALLSTACKITEM_N(_CL("call_log"), _CL("get_event"));

	if ( (index < 0) || (index>= current_events->Count()) ) return 0;

	return current_events->At(index);
}
