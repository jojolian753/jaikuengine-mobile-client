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

#include "call_log.h"
#include "contextbookcontainer.h"

#include <eikenv.h>
#include <rpbkviewresourcefile.h>
#include <cpbkcontactitem.h>
#include <pbkiconinfo.h> 
#include "icons.h"

call_log::call_log() : CCheckedActive(EPriorityIdle, _L("call_log")), current_state(IDLE)
{
	CALLSTACKITEM(_L("call_log::call_log"));

}

call_log::~call_log()
{
	CALLSTACKITEM(_L("call_log::~call_log"));

	if (obs) obs->exiting();

	Cancel();
	delete recent;
	delete logclient;
	delete logfilter;
	delete current_array;
	delete current_nos;
	delete all_array;
	delete all_nos;
	delete iContactToItems;
}

void call_log::ConstructL()
{
	CALLSTACKITEM(_L("call_log::ConstructL"));

	iContactToItems=CGenericIntMap::NewL();

	current_array = new (ELeave) CPtrCArray(8);
	current_nos = new (ELeave) CPtrCArray(8);

	all_array = new (ELeave) CDesCArrayFlat(50);
	all_nos = new (ELeave) CDesCArrayFlat(50);

	eng=CPbkContactEngine::Static();

	if (eng) {
		owns_engine=false;
	} else {
		eng=CPbkContactEngine::NewL();
		owns_engine=true;
	}

	logclient=CLogClient::NewL(CEikonEnv::Static()->FsSession());
	recent=CLogViewRecent::NewL(*logclient);
	logfilter=CLogFilter::NewL();
	logfilter->SetDirection(_L("Outgoing"));
	logfilter->SetEventType(KLogCallEventTypeUid);

	CActiveScheduler::Add(this);
	do_refresh=false; new_from=0;
	refresh();
}

void call_log::refresh()
{
	CALLSTACKITEM(_L("call_log::refresh"));

	if (current_state!=IDLE) 
	{
		do_refresh=true;
		return;
	}

	getting_first=true;
	
	new_from=all_array->Count();
	
	TBool issued = recent->SetRecentListL(KLogNullRecentList, *logfilter, iStatus);
	
	if (issued && iStatus==KRequestPending) {
		current_state=WAITING_SET;
		SetActive();
	} else {
		current_state=IDLE;
	}
}

void call_log::ReRead()
{
	CALLSTACKITEM(_L("call_log::ReRead"));

	refresh();
}

MDesCArray* call_log::get_array()
{
	CALLSTACKITEM(_L("call_log::get_array"));

	return current_array;
}



bool call_log::filter(const TDesC& /*substr*/, bool /*force*/)
{
	CALLSTACKITEM(_L("call_log::filter"));

	return false;
}

TPtrC call_log::get_phone_no(TInt index)
{
	CALLSTACKITEM(_L("call_log::get_phone_no"));

	return (*current_nos)[index];
}

bool call_log::handle_event(const CLogEvent& ev)
{
	CALLSTACKITEM(_L("call_log::handle_event"));

	if (ev.Id()==last_seen_id ) return false;

	if (getting_first) 
	{
		last_seen_id=ev.Id();
		getting_first=false;
	}

	TTime tt=ev.Time();

	TLocale locale;
	TTimeIntervalSeconds offset(locale.UniversalTimeOffset());
	tt+=offset;
	if (locale.QueryHomeHasDaylightSavingOn()) {
		TTimeIntervalHours ds(1);
		tt+=ds;
	}

	all_nos->AppendL(ev.Number());

	TBuf<200> text;
	
	if (ev.RemoteParty().Length()>0) {
		text.Append(ev.RemoteParty());
	} else {
		text.Append(ev.Number());
	}
	text.Append(_L("\t"));

	TDateTime t=tt.DateTime();
	text.AppendFormat(_L("    %02d/%02d %02d:%02d "),
		(TInt)t.Day()+1, (TInt)t.Month()+1,
		(TInt)t.Hour(), (TInt)t.Minute() );
	
	// Converting phone number to Int
	TInt nbDigits = 0;
	TInt phoneno = eng->Database().TextToPhoneMatchNumber(ev.Number(), nbDigits,8);
	
	// Finding Contacts fitting the number...
	CContactIdArray * current_contact_ids = 0;
	TRAPD(errorCode, current_contact_ids = eng->Database().MatchPhoneNumberL(phoneno) );
	CleanupStack::PushL(current_contact_ids);
	
	// getting icon
	TInt iconId = -1;
	TInt index=0;

	CCoeEnv *env = CEikonEnv::Static();
	RPbkViewResourceFile pbkRes( *env);
	pbkRes.OpenL();
	CleanupClosePushL(pbkRes);
	
	CPbkIconInfoContainer * container = CPbkIconInfoContainer::NewL();  
	CleanupStack::PushL(container);
	
	if (errorCode != KErrNotFound)
	{
		CPbkContactItem * item=eng->ReadContactL((*current_contact_ids)[0]);
		CleanupStack::PushL(item);
		TPbkContactItemField * f = item->FindNextFieldWithPhoneNumber(ev.Number(),8,index);
		
		if (f) {	
			iconId = container->Find(f->IconId())->IconId();
		}
	
		if (iconId != -1) //icon was found
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%d"), GetIconIndex(iconId) );
			text.Append(icon_int);
		}
		else // no icon found -> use empty icon
		{
			text.Append(_L("\t0"));
		}
		
		CleanupStack::PopAndDestroy(); // item
	}
	else
	{
		text.Append(_L("\t0"));
	}
	text.Append(_L("\t0\t0")); //empty speaker and vibrator icons
	
	CleanupStack::PopAndDestroy(2); //pbkRes, container

	all_array->AppendL(text);
	RDebug::Print(text);
	CleanupStack::PopAndDestroy(); // current_contact_ids
	return true;
}

void call_log::copy_new_to_current()
{
	CALLSTACKITEM(_L("call_log::copy_new_to_current"));

	// trying to be efficient
	int new_count;

	new_count=all_array->Count()-(new_from);
	
	if (new_count <= 0) return;

	int i;
    current_array->ResizeL(new_count);
	current_nos->ResizeL(new_count);

    for (i=0;i<new_count;i++)
	{
		
		current_array->At(i).Set((*all_array)[new_from+i]);
		current_nos->At(i).Set((*all_nos)[new_from+i]);	
	}
}
		

void call_log::CheckedRunL()
{
	CALLSTACKITEM(_L("call_log::CheckedRunL"));

	if (iStatus!=KErrNone) {
		Cancel();
		//TODO: err msg
		return;
	}
	
	switch (current_state) {
	
	case WAITING_SET:

		if (recent->FirstL(iStatus) && iStatus==KRequestPending) 
		{
			SetActive();
			current_state=WAITING_NEXT;
		} 
		else 
		{
			current_state=IDLE;
		}
		break;

	case WAITING_NEXT:
		
		if ( handle_event(recent->Event()) )
		{
			if (recent->NextL(iStatus) && iStatus==KRequestPending) 
			{
				SetActive();
			} 
			else	
			{
				copy_new_to_current();
				if (obs) obs->contents_changed();
				current_state=IDLE;
			}
		} 
		else 
		{
			// seen rest already
			copy_new_to_current();
			if (obs) obs->contents_changed();
			current_state=IDLE;
		}
		break;
	default:
		break;
	}

	if (current_state==IDLE && do_refresh) {
		// redo queued
		do_refresh=false;
		refresh();
	}
}

void call_log::DoCancel()
{
	CALLSTACKITEM(_L("call_log::DoCancel"));

	if (current_state!=IDLE && recent) recent->Cancel();
	current_state=IDLE;
}

void call_log::set_observer(phonebook_observer* i_obs)
{
	CALLSTACKITEM(_L("call_log::set_observer"));

	obs=i_obs;
}
