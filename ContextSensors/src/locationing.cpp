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

#include "locationing.h"
#include "cc_imei.h"

#include "cl_settings.h"

#ifdef __S60V2__
#include <etel.h>
#include <stripped_etelmm.h>
#else
#include <etelagsm.h>
#  ifndef NO_ETELAGSM_H
#    include <etelagsm.h>
#  endif
#endif

#ifndef __WINS__
#define WAIT_TIME	90
#else
#define WAIT_TIME	1
#endif

#ifndef __WINS__
#define WAIT_TIME_DISCONNECTED	90
#else
#define WAIT_TIME_DISCONNECTED	1
#endif 

#include "reporting.h"

void Clocationing::ConstructL(i_status_notif* i_cb, MNaming* i_naming, sms* i_sms_handler, MNetworkConnection * aPublisher)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("ConstructL"));

	iName=(TText*)L"Clocationing";	
	iNaming=i_naming;
	cb=i_cb;
	
	smsh = i_sms_handler;

	iPublisher = aPublisher;

	iTimer=CTimeOut::NewL(*this);

	TInt enable;
	if (Settings().GetSettingL(SETTING_LOCATIONSERVICE_ENABLE, enable)) {
		iEnabled=enable;
	} else {
		iEnabled=true;
	}

/*
#if !defined(__WINS__) && !defined(__S60V3__)

	// FIXME: refactor
	bool success=false;

#if defined(__S60V2__) || ( !defined(__S60V2__) && !defined(NO_ETELAGSM_H) )

#  if !defined(__S60V2__)
	MAdvGsmPhoneInformation::TSubscriberId i;
	RAdvGsmPhone phone;
#  else
	RMobilePhone phone;
	RMobilePhone::TMobilePhoneSubscriberId i;

#  endif
	RTelServer::TPhoneInfo info;
	TInt ret;
	if ( (ret=TelServer().GetPhoneInfo( 0, info ))==KErrNone) {
		if ((ret=phone.Open( TelServer(), info.iName ))==KErrNone) {
			CleanupClosePushL(phone);
			TRequestStatus r;
			phone.GetSubscriberId(r, i);
			User::WaitForRequest(r);
			if (r==KErrNone) {
				imsi=i;
				success=true;
			} else {
				TBuf<80> msg=_L("error getting imsi ");
				msg.AppendNum(r.Int());
				msg.Append(_L(" ")); msg.Append(i);
				if (cb) cb->status_change(msg);
			}
			CleanupStack::PopAndDestroy();
		} else {
			TBuf<40> msg=_L("error opening phone ");
			msg.AppendNum(ret);
			if (cb) cb->status_change(msg);
		}
	} else {
		TBuf<40> msg=_L("error getting phone info ");
		msg.AppendNum(ret);
		if (cb) cb->status_change(msg);
	}
#endif
	if (!success) {
		GetImeiL(imsi);
	}

#else
	// Return a fake IMEI when working on emulator
	_LIT(KEmulatorImsi, "244050000000000");
	imsi.Copy(KEmulatorImsi);
#endif
*/
	smsh->AddHandler(this);

	Settings().NotifyOnChange(SETTING_LOCATIONSERVICE_ENABLE, this);

}

Clocationing::Clocationing(MApp_context& Context) : MContextBase(Context)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("Clocationing"));

}

EXPORT_C Clocationing* Clocationing::NewL(MApp_context& Context, i_status_notif* i_cb, MNaming* i_naming, sms * i_sms_handler, MNetworkConnection * aPublisher)
{
	CALLSTACKITEMSTATIC_N(_CL("Clocationing"), _CL("NewL"));

	auto_ptr<Clocationing> ret(new (ELeave) Clocationing(Context));

	ret->ConstructL(i_cb, i_naming, i_sms_handler, aPublisher);
	return ret.release();
}

void Clocationing::handle_error(const TDesC& descr)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("handle_error"));
	
	cb->error(descr);
	/* 
	 * there are many kinds of errors, most don't
	 * concern us
	 */

	return;

	// pending sms location reception
	iTimer->Reset();

	//resume presence
	if (iPublisher) iPublisher->ResumeConnection();

}

void Clocationing::handle_move(const TMsvId& /*msg_id*/, const TMsvId& /*from_folder*/, const TMsvId& /*to_folder*/, const TDesC& /*sender*/)
{
}

void Clocationing::handle_delete(const TMsvId& /*msg_id*/, const TMsvId& /*parent_folder*/, const TDesC& /*sender*/)
{
}

void Clocationing::handle_sending(const TMsvId& /*entry_id*/, const TDesC& /*sender*/, const TDesC & /*body*/)
{
}
void Clocationing::handle_change(const TMsvId& /*msg_id*/, const TDesC& /*sender*/)
{
}

void Clocationing::handle_read(const TMsvId& /*msg_id*/, const TDesC& /*sender*/, TUid, CBaseMtm*)
{
}


bool Clocationing::handle_reception(const TMsvId& /*entry_id*/, 
									const TMsvId& /*folder_id*/, 
									const TDesC& sender, const TDesC& body)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("handle_reception"));

#ifndef __WINS__
	if (iStyle==ENone) {
		return false;
	} else if (iStyle==ERadiolinja) {
		if (sender.Left(5).Compare(_L("16507"))==0 ) {
			// real reply
			;
		} else if (sender.Left(5).Compare(_L("15507"))==0 ) {
			// 'not registered'
			;
		} else { 
			return false;
		}
	} else if (iStyle==ESonera) {
		if (sender.Compare(_L("15400")) ) {
			return false;
		}
	}
#endif
	
	iTimer->Reset();

	if (iToDiscard) {
		--iToDiscard;
		cb->status_change(_L("discarded unreliable result"));
		return true;
	}
	if (!iToName.SentRequest) return false;

	auto_ptr<HBufC> content(0);
	bool error_reply=false;

	if (sender.Left(5).Compare(_L("16507"))==0 || sender.Left(5).Compare(_L("15507"))==0) {
		CALLSTACKITEM_N(_CL("Clocationing"), _CL("radiolinja"));
		if (body.Left(8).Compare(_L("itse  on"))) {
			if (body.FindF(_L("vaatii rekis"))!=KErrNotFound) {
				Settings().WriteSettingL(SETTING_LOCATIONSERVICE_ENABLE, 0);
				Reporting().ShowGlobalNote( 4, _L("Elisa locationing "
					L"requires registration. Locationing disabled. "
					L"See the website for instructions."));

			} else if (body.FindF(_L("ydy listaltasi"))!=KErrNotFound) {
				Settings().WriteSettingL(SETTING_LOCATIONSERVICE_ENABLE, 0);
				Reporting().ShowGlobalNote( 4, _L("You must register "
					L"as 'itse' in Elisa locationing. "
					L"Locationing disabled. "
					L"See the website for instructions."));
			} else if (body.FindF(_L("ei ole antanut"))!=KErrNotFound) {
				Settings().WriteSettingL(SETTING_LOCATIONSERVICE_ENABLE, 0);
				Reporting().ShowGlobalNote( 4, _L("You must give "
					L"'itse' permission to locate you. "
					L"Locationing disabled. "
					L"See the website for instructions."));
			} else if (body.FindF(_L("ei saatavilla"))!=KErrNotFound ||
				body.FindF(_L("itse dated"))!=KErrNotFound) {
				Settings().WriteSettingL(SETTING_LOCATIONSERVICE_ENABLE, 0);
				Reporting().ShowGlobalNote( 4, _L("You have "
					L"set yourself as Invisible in Elisa. "
					L"Locationing disabled. "
					L"See the website for instructions."));
			}

			error_reply=true;
		} else {
			content.reset(body.Mid(19, body.Length()-19-1).AllocL());
			content->Des().Trim();
		}
	} else if (sender.Compare(_L("15400"))==0) {
		TInt colon_pos=body.Find(_L(":"));
		if (colon_pos==KErrNotFound || body.Find(_L("sijainti")) == KErrNotFound ) {
			error_reply=true;
		} else {
			TBuf<30> city, district;

			TLex l(body);
			while (l.Get() != ':');
			l.Inc();
			l.Mark();

			while (l.Get() != ',');
			l.UnGet();
			city=l.MarkedToken().Left(30);

			l.Inc(2);
			l.Mark();
			while (l.Get() != ',');
			l.UnGet();
			district=l.MarkedToken().Left(30);

			content.reset(HBufC::NewL(60));
			content->Des().Append(district);
			content->Des().Append(_L(", "));
			content->Des().Append(city);
		}

	}

	if (error_reply) {
		if (true || iRetryCount>5) 
		{
			/*
			 * It seems that if the locationing doesn't
			 * work, it won't work again before a cell change.
			 * The Radiolinja service probably sometimes doesn't
			 * get updated correctly, and doesn't retry.
			 * It's not worth trying again before we go
			 * to a new cell.
			 */
			iToName.Reset();
			cb->error(_L("Error result"));
			cb->status_change(body);

			//resume presence
			if (iPublisher) iPublisher->ResumeConnection();
		} 
		else 
		{
			iToName.SentRequest=false;
			iRetryCount++;
			iTimer->Wait(WAIT_TIME*4);
		}
		
		return true;
	}

	cb->status_change(*content);

	iNaming->add_cellid_name(iToName.CellId, *content);
	iToName.Reset();

	//resume presence
	if (iPublisher) iPublisher->ResumeConnection();
	return true;
}

EXPORT_C void Clocationing::GetNameL(const TBBCellId* Cell, TBool aForce)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("GetNameL"));

	if (!Cell) return;
	if (!iLocationingAvailable || (!iEnabled && !aForce) ) return;

	if (iToName.SentRequest) {
		User::Leave(KErrServerBusy);
		return;
	}

	iToName.CellId=*Cell;
	iToName.SentRequest=false;
	
	//suspend connection
	if (iPublisher) iPublisher->SuspendConnection();
	if (aForce) {
		iTimer->Wait(0);
	} else {
		iTimer->Wait(WAIT_TIME_DISCONNECTED);
	}
	iRetryCount=0;
}

void Clocationing::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("expired"));
	TBuf<20> msg; volatile TInt err;

	if (iToName.SentRequest == false)
	{	
		err=KErrNotFound;
		if (iStyle==ERadiolinja) 
			err=smsh->send_message(_L("16507"), _L("PAIKKA itse"), false);
		else if (iStyle==ESonera) {
			// temp fix for gcc-3.0 mishandling
			TBuf<5> missa;
			missa=_L("MISSA");
			missa[4]=0x00c4;
			err=smsh->send_message(_L("15400"), missa, false);
		}

		if (err!=KErrNone) {
			msg.Format(_L("sms error %d"), err);
			cb->error(msg);
			
		} else {
			iToName.SentRequest=true;
			iTimer->Wait(WAIT_TIME);
			return;
		}
	}
	if (iPublisher) iPublisher->ResumeConnection();
}

void Clocationing::test()
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("test"));

	if (!iLocationingAvailable) {
		cb->error(_L("Locationing not available"));
		return;
	}
	if (!iEnabled) {
		cb->error(_L("Locationing not enabled"));
		return;
	}

	volatile TInt err=0;
	TBuf<20> msg;
	smsh->send_message(_L("16507"), _L("PAIKKA itse"), false);
	if (err!=KErrNone) {
		msg.Format(_L("sms error %d"), err);
		cb->error(msg);
	}
}

EXPORT_C Clocationing::~Clocationing()
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("~Clocationing"));

	Settings().CancelNotifyOnChange(SETTING_LOCATIONSERVICE_ENABLE, this);
	delete iTimer;
}

EXPORT_C void Clocationing::now_at_location(const TBBCellId* Cell, TInt /*id*/, bool /*is_base*/, 
					    bool loc_changed, TTime /*time*/)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("now_at_location"));

	if ( Cell && Cell->iMCC()==244 && Cell->iMNC()==5) {
		iLocationingAvailable=true;
		iStyle=ERadiolinja;
	} else if ( Cell && Cell->iMCC()==244 && Cell->iMNC()==91) {
		iLocationingAvailable=true;
		iStyle=ESonera;
	} else {
		iLocationingAvailable=false;
		iStyle=ENone;
	}

	if (loc_changed) {
		if (iToName.SentRequest) {
			iToName.SentRequest=false;
			++iToDiscard;
			if (iPublisher) iPublisher->ResumeConnection();
		} else if (iTimer->IsActive()) {
			if (iPublisher) iPublisher->ResumeConnection();
		}
		iTimer->Reset();
	}
}

void Clocationing::SettingChanged(TInt /*Setting*/)
{
	CALLSTACKITEM_N(_CL("Clocationing"), _CL("SettingChanged"));

	TInt enable;
	if (Settings().GetSettingL(SETTING_LOCATIONSERVICE_ENABLE, enable)) {
		iEnabled=enable;
	} else {
		iEnabled=true;
	}
}

/*
EXPORT_C const TDesC& Clocationing::GetImsi() const { return imsi; }
*/

EXPORT_C bool Clocationing::LocationingAvailable() const { return iLocationingAvailable; }

EXPORT_C bool Clocationing::LocationingAllowed() const { return iEnabled; }

