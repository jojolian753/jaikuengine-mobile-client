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
#include "sms_status.h"

#include "i_logger.h"
#include "symbian_auto_ptr.h"
#include "csd_event.h"
#include "csd_battery.h"
#include "cl_settings.h"
#include "cc_imei.h"

#ifdef SMSSTATUS

class CSmsStatusReplierImpl : public CSmsStatusReplier,
	public MContextBase, public i_handle_received_sms,
	public Mlogger {
private:
	CSmsStatusReplierImpl(MApp_context& aContext, sms* aSms);
	void ConstructL();
	~CSmsStatusReplierImpl();

	void SetStatus(const TDesC& aName, const MBBData* aValue);
	void SetStatus(const TDesC& aName, const TDesC& aValue);
	void GetOtherStatus();
	void SendStatus(const TDesC& aTo);

	virtual bool handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, const TDesC& sender, const TDesC& body); // return true if message is to be deleted

	virtual void handle_change(const TMsvId& , const TDesC& ) { }
	virtual void handle_delete(const TMsvId& , const TMsvId& , const TDesC& ) { }
	virtual void handle_move(const TMsvId& , const TMsvId& , const TMsvId& , const TDesC& ) { }
	virtual void handle_error(const TDesC& ) { }
	virtual void handle_sending(const TMsvId& , const TDesC& , const TDesC& ) { }
	virtual void handle_read(const TMsvId& , const TDesC&, TUid, CBaseMtm* ) { }

	virtual void NewValueL(const TTupleName& aName, const TDesC& aSubName, const MBBData* aData);
	void NewSensorEventL(const TTupleName& aName, 
				const TDesC& aSubName, const CBBSensorEvent& aEvent);

	friend class CSmsStatusReplier;
	friend class auto_ptr<CSmsStatusReplierImpl>;

	virtual void test();

	sms* iSms;
	HBufC*	iBuf;
	CDesC16Array	*iNames, *iValues;
};

CSmsStatusReplier* CSmsStatusReplier::NewL(MApp_context& aContext,
	sms* aSms)
{
	auto_ptr<CSmsStatusReplierImpl> ret(new (ELeave) CSmsStatusReplierImpl(aContext, aSms));
	ret->ConstructL();
	return ret.release();

}

CSmsStatusReplierImpl::CSmsStatusReplierImpl(MApp_context& aContext, sms* aSms) :
	MContextBase(aContext), iSms(aSms) { }

void CSmsStatusReplierImpl::ConstructL()
{
	Mlogger::ConstructL(AppContextAccess());
	iSms->AddHandler(this);
	iBuf=HBufC::NewL(512);
	iNames=new (ELeave) CDesC16ArraySeg(128);
	iValues=new (ELeave) CDesC16ArraySeg(128);
	SubscribeL(KBatteryTuple);
	SubscribeL(KStatusTuple);

	TBuf<20>	imei;
#ifndef __WINS__
	GetImeiL(imei);
#else
	// Return a fake IMEI when working on emulator
	_LIT(KEmulatorImsi, "244050000000000");
	imei.Copy(KEmulatorImsi);
#endif
	SetStatus(_L("imei"), imei);
}

CSmsStatusReplierImpl::~CSmsStatusReplierImpl()
{
	delete iBuf;
	delete iNames;
	delete iValues;
}

_LIT(KStatusEnquiry, "CONTEXTPHONE: STATUS");

_LIT(KTime, "time");

void CSmsStatusReplierImpl::SendStatus(const TDesC& aTo)
{

	TBuf<50> author;
	Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, author);
	SetStatus(_L("authorname"), author);

	TInt count=iNames->Count(), size=30;

	TInt i=0;
	for (i=0; i<count; i++) {
		size += iNames->MdcaPoint(i).Length();
		size += iValues->MdcaPoint(i).Length();
		size += 4;
	}
	if (size > iBuf->Des().MaxLength() ) {
		iBuf=iBuf->ReAllocL(size);
	}
	iBuf->Des()=_L("status at ");
	TBBTime t(KTime); t()=GetTime();
	TPtr p=iBuf->Des();
	t.IntoStringL(p);
	iBuf->Des().Append(_L("\n"));

	for (i=0; i<count; i++) {
		iBuf->Des().Append(iNames->MdcaPoint(i));
		iBuf->Des().Append(_L(": "));
		iBuf->Des().Append(iValues->MdcaPoint(i));
		iBuf->Des().Append(_L("\n"));
	}

	iSms->send_message(aTo, *iBuf, false);
}

void CSmsStatusReplierImpl::SetStatus(const TDesC& aName, const MBBData* aValue)
{
	TInt err=KErrNone;
	iBuf->Des().Zero();
	if (aValue)
	do {
		TPtr p=iBuf->Des();
		CC_TRAP(err, aValue->IntoStringL(p));
		if (err==KErrOverflow) {
			iBuf->Des().Zero();
			iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength()*2);
		}
	} while (err==KErrOverflow);

	if (err!=KErrNone) {
		iBuf->Des().Zero();
		iBuf->Des()=_L("Error in reading data: ");
		iBuf->Des().AppendNum(err);
	}
	SetStatus(aName, *iBuf);
}

void CSmsStatusReplierImpl::SetStatus(const TDesC& aName, const TDesC& aValue)
{
	TInt pos;
	if (iNames->Find(aName, pos) == 0) {
		iValues->Delete(pos);
		iValues->InsertL(pos, aValue);
	} else {
		iNames->AppendL(aName);
		iValues->AppendL(aValue);
	}
}

bool CSmsStatusReplierImpl::handle_reception(const TMsvId& /*entry_id*/, 
					     const TMsvId& /*folder_id*/, 
					     const TDesC& sender, 
					     const TDesC& body)
{
	if (body.Left(KStatusEnquiry().Length()).Compare(KStatusEnquiry)==0) {
		GetOtherStatus();
		SendStatus(sender);
		return true;
	}
	return false;
}

void CSmsStatusReplierImpl::NewValueL(const TTupleName& aName, 
				      const TDesC& aSubName, const MBBData* aData)
{
	if (aName==KStatusTuple) {
		SetStatus(aSubName, aData);
	} else if (aName==KBatteryTuple) {
		const CBBSensorEvent* e=bb_cast<CBBSensorEvent>(aData);
		if (e) SetStatus(_L("battery"), e);
		else  SetStatus(_L("battery"), aData);
	} 
}

void CSmsStatusReplierImpl::NewSensorEventL(const TTupleName& aName, 
						    const TDesC& aSubName, const CBBSensorEvent& aEvent) { }

void CSmsStatusReplierImpl::GetOtherStatus()
{
}

void CSmsStatusReplierImpl::test()
{
}

#endif
