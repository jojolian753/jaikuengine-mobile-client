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
#include "phonehelper.h"
#include "app_context.h"
#include <txtrich.h>
#include <SenduiMtmUids.h>
#include <MsgBioUids.h>
#include <msvapi.h>
#include <msvids.h>
#include "symbian_auto_ptr.h"
#ifdef __S60V3__
#include <cmessagedata.h>
#endif

class dummyhandler: public MMsvSessionObserver {
public:
	virtual void HandleSessionEventL(TMsvSessionEvent, TAny*, TAny*, TAny*) { }
};

EXPORT_C phonehelper::phonehelper(Mfile_output_base * log) : CCheckedActive(EPriorityIdle, _L("phonehelper")), aLog(log)
#ifndef __S60V3__
		, iServerOpen(EFalse), iPhoneOpen(EFalse), iLineOpen(EFalse), iCallOpen(EFalse) 
#else
		, callParamsPckg(callParams)
#endif
		{}

EXPORT_C void phonehelper::ConstructL()
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("ConstructL"));

#ifndef __S60V3__
	iSendAppUi = CSendAppUi::NewL(0);
#else
	iSendAppUi = CSendUi::NewL();
#endif

#ifndef __S60V3__
#if !defined(__WINS__) || defined(__S60V2__)
	iStatus=KErrNone;
	
	User::LeaveIfError( server.Connect() );
	iServerOpen = ETrue;

	// load a phone profile
#  ifndef __WINS__
		_LIT(KGsmModuleName, "phonetsy.tsy");
#  else
		_LIT(KGsmModuleName, "mm.tsy");
#  endif
	User::LeaveIfError( server.LoadPhoneModule(KGsmModuleName) );

	TInt numberPhones;
	User::LeaveIfError(server.EnumeratePhones(numberPhones));
	if (numberPhones < 1) {
		User::Leave(KErrNotFound);
	}

	//Get info about the first available phone
	RTelServer::TPhoneInfo info;
	User::LeaveIfError(server.GetPhoneInfo(0, info));
	User::LeaveIfError( phone.Open( server, info.iName ) );
	iPhoneOpen = ETrue;

	// RPhone::TLineInfo lineinfo;
	// phone.GetLineInfo(0, lineinfo);
	// line.Open(phone, lineinfo.iName);

#endif
#endif // !__S60V3__

	CActiveScheduler::Add(this); 
}

EXPORT_C void phonehelper::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("CheckedRunL"));

#ifndef __S60V3__
	if (iCallOpen) {
		iCallOpen=EFalse;
		call.Close();
	}
	if (iLineOpen) {
		iLineOpen=EFalse;
		line.Close();
	}
#endif

}

EXPORT_C void phonehelper::DoCancel()
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("DoCancel"));

#ifndef __S60V3__
	if (iCallOpen) {
		call.DialCancel();
		iCallOpen = EFalse;
		call.Close();
	}

	if (iLineOpen) {
		iLineOpen = EFalse;
		line.Close();
	}
#else
	Telephony().CancelAsync(CTelephony::EDialNewCallCancel);
#endif
}

EXPORT_C phonehelper::~phonehelper()
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("~phonehelper"));
	Cancel();

#ifndef __S60V3__
	if (iPhoneOpen) {
		iPhoneOpen=EFalse;
		phone.Close();
	}

	if (iServerOpen) { 
#ifndef __WINS__
		_LIT(KGsmModuleName, "phonetsy.tsy");
#else
#  ifdef __S60V2__
		_LIT(KGsmModuleName, "mm.tsy");
#  else
		_LIT(KGsmModuleName, "hayes.tsy");
#  endif
#endif
		server.UnloadPhoneModule( KGsmModuleName );
		iServerOpen=EFalse;
		server.Close();
	}
#endif
	delete iSendAppUi;
}

/*RCall::TStatus phonehelper::line_status()
{
	CALLSTACKITEM_N(_CL("RCall"), _CL("TStatus"));


	RLine::TLineInfo lineinfo;
	#ifndef __WINS__
		line.GetInfo(lineinfo);
	#else
		lineinfo.iStatus=RCall::EStatusIdle;
	#endif
	return lineinfo.iStatus;
}*/

// Concepts:
// !Making a phone call!
EXPORT_C TInt phonehelper::make_callL(const TDesC& aNumber)
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("make_callL"));

	auto_ptr<HBufC> number(HBufC::NewL(aNumber.Length()));
	TInt i;

	for (i=0; i<aNumber.Length(); i++) {
		TChar c( aNumber[i] );
		if (c.IsDigit() || c=='+' || c=='p' || c=='w' || c=='*') {
			number->Des().Append(c);
		}
	}
	if (aLog) {
		aLog->write_time();
		aLog->write_to_output(_L("make call to "));
		aLog->write_to_output(*number);
		aLog->write_nl();
	}

	// The status isn't reliable if no calls
	// have been made yet after startup
	/*
	if (line_status()!=RCall::EStatusIdle) {
		return -1;
	}

	*/


	if (IsActive()) return -2;

#ifndef __S60V3__
	//Get info about the first available line
	TInt numberLines;
	User::LeaveIfError(phone.EnumerateLines(numberLines));
	if (numberLines <1) {
		User::Leave(KErrNotFound);
	}

	RPhone::TLineInfo lineInfo;
	for (i=0; i<numberLines; i++) {
		User::LeaveIfError(phone.GetLineInfo(i, lineInfo));
#ifdef __WINS__
		if ( lineInfo.iLineCapsFlags & RPhone::KCapsVoice ) 
			break;
#else
		break;
#endif
	}
	//if ( ! (lineInfo.iLineCapsFlags & RPhone::KCapsVoice) ) 
		//User::Leave(KErrNotSupported);
	User::LeaveIfError(line.Open(phone, lineInfo.iName));
	iLineOpen = ETrue;

	TInt ret=call.OpenNewCall(line);
	User::LeaveIfError(ret);
	iCallOpen = ETrue;
	if (ret==KErrNone) {
		call.Dial(iStatus, *number);
		SetActive();
	}
	return ret;
#else
	telNumber=*number;
    callParams.iIdRestrict = CTelephony::EIdRestrictDefault;

    Telephony().DialNewCall(iStatus, callParamsPckg, telNumber, iCallId);
    SetActive();
    return KErrNone;
#endif
}

#ifdef __S60V3__ 
void DoSendL(CSendUi* iSendAppUi, CDesCArrayFlat* recip, CDesCArrayFlat* alias,
	CRichText* body, TUid uid) {

	TInt pushed=0;	
	CMessageData *data=CMessageData::NewLC(); ++pushed;
	data->SetBodyTextL(body);
	for (int i=0; i<recip->Count(); i++) {
		if (alias) {
			data->AppendToAddressL( (*recip)[i], (*alias)[i] );
		} else {
			data->AppendToAddressL( (*recip)[i] );
		}
	}
	iSendAppUi->CreateAndSendMessageL(uid, data, KNullUid, EFalse);
	CleanupStack::PopAndDestroy(pushed);
}
#endif

EXPORT_C void phonehelper::send_sms(CDesCArrayFlat* recip, CDesCArrayFlat* alias,
									const TDesC& aMessage)
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("send_sms"));
	TInt pushed=0;

	if (aLog) {
		aLog->write_time();
		aLog->write_to_output(_L("write sms to "));
		for (int i=0; i<recip->Count(); i++)
		{
			aLog->write_to_output( (*recip)[i] );
			aLog->write_to_output(_L(" "));
		}
		aLog->write_nl();
	}

	CParaFormatLayer* paraf=CParaFormatLayer::NewL();
	CleanupStack::PushL(paraf); ++pushed; 

	CCharFormatLayer* charf=CCharFormatLayer::NewL();
	CleanupStack::PushL(charf); ++pushed;

	CRichText* body;
	body=CRichText::NewL(paraf, charf);
	CleanupStack::PushL(body); ++pushed;
	if (aMessage.Length()>0) body->InsertL(0, aMessage);
		
#ifndef __S60V3__
	iSendAppUi->CreateAndSendMessageL(KSenduiMtmSmsUid, body, 0, KNullUid, recip, alias,
		EFalse);
#else
	DoSendL(iSendAppUi, recip, alias, body, KSenduiMtmSmsUid);
#endif
	CleanupStack::PopAndDestroy(pushed); // body, paraf, charf
}

EXPORT_C void phonehelper::send_mms(CDesCArrayFlat* recip, CDesCArrayFlat* alias)
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("send_mms"));

	if (aLog) {
		aLog->write_time();
		aLog->write_to_output(_L("write mms to "));
		for (int i=0; i<recip->Count(); i++)
		{
			aLog->write_to_output( (*recip)[i] );
			aLog->write_to_output(_L(" "));
		}
		aLog->write_nl();
	}

	CParaFormatLayer* paraf=CParaFormatLayer::NewL();
	CleanupStack::PushL(paraf);

	CCharFormatLayer* charf=CCharFormatLayer::NewL();
	CleanupStack::PushL(charf);

	CRichText* body;
	body=CRichText::NewL(paraf, charf);
	CleanupStack::PushL(body);
		
#ifndef __S60V3__
	iSendAppUi->CreateAndSendMessageL(KSenduiMtmMmsUid, body, 0, KNullUid, recip, alias);
#else
	DoSendL(iSendAppUi, recip, alias, body, KSenduiMtmMmsUid);
#endif
	CleanupStack::PopAndDestroy(3); // body, paraf, charf
}

EXPORT_C void phonehelper::send_email(CDesCArrayFlat* recip, CDesCArrayFlat* alias)
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("send_email"));

	if (aLog) {
		aLog->write_time();
		aLog->write_to_output(_L("write email to "));
		for (int i=0; i<recip->Count(); i++)
		{
			aLog->write_to_output( (*recip)[i] );
			aLog->write_to_output(_L(" "));
		}
		aLog->write_nl();
	}

	CParaFormatLayer* paraf=CParaFormatLayer::NewL();
	CleanupStack::PushL(paraf);

	CCharFormatLayer* charf=CCharFormatLayer::NewL();
	CleanupStack::PushL(charf);

	CRichText* body;
	body=CRichText::NewL(paraf, charf);
	CleanupStack::PushL(body);
		
#ifndef __S60V3__
	iSendAppUi->CreateAndSendMessageL(KSenduiMtmSmtpUid, body, 0, KNullUid, recip, alias);
#else
	DoSendL(iSendAppUi, recip, alias, body, KSenduiMtmSmtpUid);
#endif
	CleanupStack::PopAndDestroy(3); // body, paraf, charf
}

EXPORT_C TBool phonehelper::mailbox_defined()
{
	CALLSTACKITEM_N(_CL("phonehelper"), _CL("mailbox_defined"));

	TBool mailbox_defined = EFalse;

	dummyhandler * dummy=new (ELeave) dummyhandler;
	CleanupStack::PushL(dummy);
	CMsvSession * session = CMsvSession::OpenSyncL(*dummy);
	CleanupStack::PushL(session);
	CMsvEntry* entry = CMsvEntry::NewL(*session, KMsvRootIndexEntryId ,TMsvSelectionOrdering());
	CleanupStack::PushL(entry);
	CMsvEntrySelection* imap = entry->ChildrenWithMtmL(KSenduiMtmImap4Uid);
	CleanupStack::PushL(imap);
	CMsvEntrySelection* pop = entry->ChildrenWithMtmL(KSenduiMtmPop3Uid);
	CleanupStack::PushL(pop);
	if ((imap->Count() >0) || (pop->Count()>0)) mailbox_defined = ETrue;
	CleanupStack::PopAndDestroy(5); //dummy, session, entry
	return mailbox_defined;
}
