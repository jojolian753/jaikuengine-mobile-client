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

#include "ver.h"

#include "answering.h"
#include <e32std.h>
#include <cntdb.h>
#include <cntitem.h>
#include <cpbkcontactitem.h> 
#include <Mda/Common/Resource.h>

#include "sms.h"
#ifdef __S60V2__
#include <etelmm.h>
#endif


static const int RETRY_WINDOW_MINUTES=15;
static const int MIN_CALL_LENGTH_SECONDS=20;

Canswering::Canswering(MApp_context& Context) : Ccall_listener(Context)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("Canswering"));

}

Canswering::~Canswering()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("~Canswering"));

	delete previous_calls;
	
	delete mobile_prefixes;
	
#ifndef NO_CFLDRINGINGTONEPLAYER_H
	if (ringp)
		CEikonEnv::Static()->AddForegroundObserverL(*ringp);
	delete ringp;
#endif
	
	delete profile_table_ids;
	
	delete player;
	
#ifndef NO_PROFILEAPI_H
	delete profileapi;
#endif
	
}

void Canswering::ConstructL(i_status_notif* i_cb)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("ConstructL"));

	Ccall_listener::ConstructL(i_cb, INCOMING);
	
	mobile_prefixes=new (ELeave) CDesCArrayFlat(4);
	
	iSpecialGroups->AddGroupL(_L("VIP"));
	iSpecialGroups->read_contact_groups();
	
	mobile_prefixes->AppendL(_L("+35850"));
	mobile_prefixes->AppendL(_L("+3584"));
	
#ifndef NO_PROFILEAPI_H
	really_silent_profile=CProfileDb::EPager;
#endif
	
	previous_calls=new (ELeave) CArrayFixFlat<call_item>(10);	
	
#ifndef NO_CFLDRINGINGTONEPLAYER_H
	ringp=CFLDRingingTonePlayer::NewL(ETrue);
	CEikonEnv::Static()->RemoveForegroundObserver(*ringp);
#endif
	
	profile_table_ids=new (ELeave) CArrayFixFlat<TInt>(5);

#ifndef NO_PROFILEAPI_H
	profileapi=CProfileAPI::NewL();
	
	auto_ptr< CArrayFixFlat<CProfileDb::TProfileStruct> > aProfileArray(
		new (ELeave) CArrayFixFlat<CProfileDb::TProfileStruct>(5) );
	profileapi->GetProfileNameList(aProfileArray.get());
	for (int i=0; i<aProfileArray->Count(); i++) {
		CProfileDb::TProfileStruct p=(*aProfileArray)[i];
		profile_table_ids->InsertL(p.iUID, p.iTableId);
	}
#endif
	
}

Canswering* Canswering::NewL(MApp_context& Context, i_status_notif* i_cb)
{

	auto_ptr<Canswering> ret(new (ELeave) Canswering(Context));
	ret->ConstructL(i_cb);
	return ret.release();
}

void Canswering::register_source(const TDesC& name, const TDesC& initial_value, const TTime& time)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("register_source"));

	if (initial_value.Length()>0)
		new_value(Mlogger::VALUE, name, initial_value, time);
}

void Canswering::new_value(log_priority priority, const TDesC& name, const TDesC& value, const TTime& /*time*/)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("new_value"));

	if (priority!=Mlogger::VALUE) return;
	
	if (! name.Compare(KLog_cellname) ) {
		if (last_place.Length()>0 && value.Length()==0) {
			prev_place=last_place;
			prev_time.HomeTime();
		}
		if (last_place.CompareF(value) ) {
			last_place=value;
			last_place_time.HomeTime();
		}
	} else if (! name.Compare(KLog_profile) ) {
		TLex lex;
		lex.Assign(value);
		TInt ret; TInt val;
		ret=lex.Val(val);
		if (ret==KErrNone) {
			profile=val;
		}
	}
}

void Canswering::increase_volume()
{
#ifndef NO_CFLDRINGINGTONEPLAYER_H


	if (profile==really_silent_profile) return;
	
	CArrayFixFlat<TContactItemId>* aContactList=new (ELeave) CArrayFixFlat<TContactItemId>(1);
	TFileName aRingingTone1FileName; TFileName aRingingTone2FileName;
	TInt aRingingVolume;
	TInt aKeypad; TInt aAlertForSize;
	TBool current_vibra; TInt current_ringtype;
	TInt err;
	err=profileapi->GetProfileMultiData(aRingingTone1FileName, aRingingTone2FileName,
		current_ringtype, aRingingVolume, current_vibra, aKeypad, aContactList,
		aAlertForSize, (*profile_table_ids)[profile]);
	
	if (err!=KErrNone) {
		// TODO
		// what should we do?
		return;
	}
	
	auto_ptr< CArrayFixFlat<CProfileDb::TSoundStruct> > 
		sounds(new (ELeave) CArrayFixFlat<CProfileDb::TSoundStruct>(3) );
	profileapi->GetProfileSoundList(sounds.get(), (*profile_table_ids)[CProfileDb::EGeneral]);
	CProfileDb::TSoundStruct s=(*sounds)[CProfileDb::ERingingTone];
	
	if (cb) cb->status_change(s.iFileName);
	
	if (current_vibra==EFalse && current_ringtype==4) {
		
		ringp->SetVibra(true);
		ringp->SetVolume(10);
		ringp->SetRingingType(0); // ringing
		((MCoeForegroundObserver*)ringp)->HandleGainingForeground();
		
		if (err!=KErrNone) 
			((MFLDFileProcessor*)ringp)->ProcessFileL(_L("c:\\system\\apps\\context_log\\tone.rng"));
		else
			((MFLDFileProcessor*)ringp)->ProcessFileL(s.iFileName);
	} else {
		// we should play a sound, ringing tone player
		// doesn't work if the phone is ringing
		
		// if the profile plays a sound, this doesn't work, but
		// that's quite OK in that case
		
		delete player;
		player=0;
		player=CMdaAudioPlayerUtility::NewFilePlayerL(s.iFileName, *this,
			EMdaPriorityMax);
	}
#endif
}

void Canswering::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& /*aDuration*/)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("MapcInitComplete"));

	if (current_state!=CALL_IN_PROGRESS) {
		delete player;
		player=0;
		return;
	}
	
	if (aError!=KErrNone) {
		TBuf<50> msg;
		msg.Format(_L("play error %d"), aError);
		if (cb) cb->error(msg);
		return;
	}
	
	player->SetVolume(player->MaxVolume());
	player->SetRepeats(KMdaRepeatForever, TTimeIntervalMicroSeconds(1*1000*1000));
	player->Play();
}

void Canswering::MapcPlayComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("MapcPlayComplete"));

	if (aError!=KErrNone) {
		TBuf<50> msg;
		msg.Format(_L("play error %d"), aError);
		if (cb) cb->error(msg);
	}
}

void Canswering::send_reply()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("send_reply"));

#if !defined(__S60V2__) && defined(NO_ETELAGSM_H)
	// feature not available
	reply_sent_for_current=true;
	return;
#else
	TBuf<20> number;
#if !defined(__S60V2__) || defined(__WINS__) 
	MAdvGsmCallInformation::TRemotePartyInfo info;
#if defined(__S60V2__)
	User::LeaveIfError(call.GetRemotePartyInfoNS(info));
#else
	User::LeaveIfError(call.GetRemotePartyInfo(info));
#endif
	if (info.iRemoteIdentityStatus!=MAdvGsmCallInformation::ERemotePartyIdentityAvailable) return;
	number = info.iNumber.iTelNumber;
#else
	RMobileCall::TMobileCallInfoV1 callInfo;
	RMobileCall::TMobileCallInfoV1Pckg callInfoPckg(callInfo);
	call.GetMobileCallInfo(callInfoPckg);
	if (callInfo.iRemoteParty.iRemoteNumber.iTelNumber.Length() != 0) return;
	number = callInfo.iRemoteParty.iRemoteNumber.iTelNumber;
#endif
	
	bool is_mobile_number=false;
	for (int i=0; i<mobile_prefixes->Count(); i++) {
		TPtrC pfx=(*mobile_prefixes)[i];
		TPtrC comp= number.Left(pfx.Length());
		if (! pfx.Compare(comp) ) {
			is_mobile_number=true;
			break;
		}
	}
	
	if (!is_mobile_number) return;
	
	TBuf<160> buf;
	if (profile==really_silent_profile) {
		buf.Append(_L("En voi vastata nyt puheluun.") );
	} else {
		buf.Append(_L("En vastannut puheluusi. "));
	}

#ifndef  NO_PROFILEAPI_H
	if ( (profile==CProfileDb::EMeeting || profile==CProfileDb::ESilent || profile==CProfileDb::EPager )
#else
	if ( (profile==2 || profile==1 || profile==4 )
#endif
		&& profile!=really_silent_profile) {
		buf.Append(_L("Puhelimeni on äänettömällä. Jos soitat uudelleen 15 min sisällä, se hälyttää silloin."));
	}
	
	if (last_place.Length()>0) {
		buf.Append(_L("Puhelin on paikassa: "));
		buf.Append(last_place);
		buf.Append(_L(", "));
	} else {
		buf.Append(_L("Puhelin oli viimeksi paikassa "));
		buf.Append(prev_place);
		TBuf<12> time;
		TDateTime dt=prev_time.DateTime();
		time.Format(_L(" klo %02d:%02d, "), dt.Hour(), dt.Minute());
		buf.Append(time);
	}
	TInt idle=User::InactivityTime().Int();
	
	buf.Append(_L("ja sitä on käytetty viimeksi ") );
	TBuf<20> idlestr;
	if (idle>=60 && idle <60*60) {
		idlestr.Format(_L("%d min "), idle/60);
	} else if (idle>=60) {
		TInt h=idle/(60*60);
		idlestr.Format(_L("%d h %d min "), h, (idle-h*60*60)/60);
	} else {
		idlestr.Format(_L("%d s "), idle);
	}
	buf.Append(idlestr);
	buf.Append(_L(" sitten. "));
	
	auto_ptr<sms> sender(new (ELeave) sms);
	sender->ConstructL();
	sender->send_message(number, buf);
	
	reply_sent_for_current=true;
#endif
}

bool Canswering::sent_reply()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("sent_reply"));

	for (int current=0; current < iSpecialGroups->current_contact_ids()->Count(); current++) {
		for (int prev=0; prev < previous_calls->Count(); prev++) {
			if ( (*previous_calls)[prev].contact == (*(iSpecialGroups->current_contact_ids()))[current] &&
				(*previous_calls)[prev].reply_sent )
				return true;
		}
	}
	return false;
	
}

bool Canswering::is_long_call()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("is_long_call"));

	TTime now;
	now.HomeTime();
		
	TTimeIntervalSeconds min_time(MIN_CALL_LENGTH_SECONDS);
	
	if (now-min_time < call_start_time) return false;
	
	return true;
}

void Canswering::store_call_status(call_status status)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("store_call_status"));

	int i;
	TTime limit;
	limit.HomeTime();
	
	limit-=TTimeIntervalMinutes(RETRY_WINDOW_MINUTES);
	for (i=0; i < previous_calls->Count(); i++) {
		if ( (*previous_calls)[i].call_time < limit) {
			previous_calls->Delete(i);
		}
	}
	previous_calls->Compress();
	
	if (status==ANSWERED) return;
	
	if (! is_long_call() ) return;
	
	call_item c;
	c.call_time=call_start_time;
	c.status=status;
	c.reply_sent=reply_sent_for_current;
	for (i=0; i< iSpecialGroups->current_contact_ids()->Count(); i++) {
		c.contact=(*iSpecialGroups->current_contact_ids())[i];
		previous_calls->AppendL(c);
	}
}

bool Canswering::is_repeated_call()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("is_repeated_call"));

	for (int current=0; current < iSpecialGroups->current_contact_ids()->Count(); current++) {
		for (int prev=0; prev < previous_calls->Count(); prev++) {
			if ( (*previous_calls)[prev].contact == (*iSpecialGroups->current_contact_ids())[current] )
				return true;
		}
	}
	return false;
}


void Canswering::HandleSessionEventL(TMsvSessionEvent /*aEvent*/, TAny* /*aArg1*/, TAny* /*aArg2*/, TAny* /*aArg3*/)
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("HandleSessionEventL"));

}

TBool Canswering::CapabilityOK(TUid /*aCapabilty*/, TInt /*aResponse*/) { return ETrue; }

_LIT(CLASS_NAME, "Canswering");

const TDesC& Canswering::name() const
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("name"));

	return CLASS_NAME;
}

void Canswering::handle_incoming()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("handle_incoming"));

	reply_sent_for_current=false;
	if ( iSpecialGroups->is_special_contact() && is_repeated_call() && profile!=really_silent_profile) {
		increase_volume();
	}
}

void Canswering::handle_disconnected()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("handle_disconnected"));

#ifndef NO_CFLDRINGINGTONEPLAYER_H
	((MFLDFileProcessor*)ringp)->Cancel();
#endif
	if (player) player->Stop();
	
	//if (cb) cb->status_change(_L("disconnecting: missed") );
	if (iSpecialGroups->is_special_contact() && ! sent_reply() && is_long_call()) {
		send_reply();
	}
	store_call_status(MISSED);
}

void Canswering::handle_answered()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("handle_answered"));

#ifndef NO_CFLDRINGINGTONEPLAYER_H
	((MFLDFileProcessor*)ringp)->Cancel();
#endif
	if (player) player->Stop();
	//if (cb) cb->status_change(_L("connected: answered"));
	store_call_status(ANSWERED);
}

void Canswering::handle_refused()
{
	CALLSTACKITEM_N(_CL("Canswering"), _CL("handle_refused"));

#ifndef NO_CFLDRINGINGTONEPLAYER_H
	((MFLDFileProcessor*)ringp)->Cancel();
#endif
	if (player) player->Stop();
	store_call_status(REFUSED);
}
