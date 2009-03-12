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
#include "log_sms.h"
#include "symbian_auto_ptr.h"
#include <e32std.h>
#include "cntdb.h"

#include "csd_sms.h"
#include <mmsconst.h>
#include <mmsclient.h>
#include <smsclnt.h>
#include <SMUTHDR.h>
#include <txtrich.h>
#include <bautils.h>

#define ALWAYS_LOG_SENT_MMS 0

void ReplaceEndl(TDes& buf)
{
	CALLSTACKITEM_N(_CL(""), _CL("ReplaceEndl"));
	for (int i=0; i<buf.Length(); i++)
	{
		if ( (buf[i] == '\n') || (buf[i] == '\r') || (buf[i] == TChar(8233)) )
		{
			buf.Replace(i, 1, _L(" ") );
		}
	}
}

void CLog_sms::SaveMMSFiles(const TMsvId& entry_id, class CMmsClientMtm* mmsMtm)
{
	TInt err;
	auto_ptr<CMsvEntrySelection> atts(0);
	CC_TRAP(err, atts.reset(mmsMtm->GetAttachmentsL() ) );
	if (err!=KErrNone) {
		post_error(_L("Error in GetAttachmentsL"), err);
		return;
	}
	for (int i=0; i< atts->Count(); i++) {
		iCopyToPath=DataDir();
		iCopyToPath.Append(_L("mms-"));
		iCopyToPath.AppendNum( TInt(entry_id) );
		iCopyToPath.Append(_L("-part-"));
		iCopyToPath.AppendNum( i );
		iCopyToPath.Append(_L("-"));
		CC_TRAP(err, mmsMtm->GetAttachmentPathL(atts->At(i), iAttchPath));
		if (err!=KErrNone) {
			post_error(_L("Error in GetAttachmentPathL"), err);
		} else {
			TParse p; p.Set(iAttchPath, 0, 0);
			iCopyToPath.Append( p.NameAndExt() );
			err=BaflUtils::CopyFile(Fs(), iAttchPath, iCopyToPath);
			if (err!=KErrNone) {
				iCopyToPath=_L("Failed to copy attachment ");
				iCopyToPath.AppendNum(i);
				iCopyToPath.Append(_L(" "));
				iCopyToPath.Append( iAttchPath.Left(iCopyToPath.MaxLength() - iCopyToPath.Length()) );
				post_error(iCopyToPath, err);
			}
		}
	}

}

bool CLog_sms::handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, 
	TUid aMtmUid, CBaseMtm* aMtm)
{
	if (aMtmUid == KUidMsgTypeSMS) {
		CSmsClientMtm* smsMtm = STATIC_CAST(CSmsClientMtm*, aMtm);
		return handle_reception(entry_id, folder_id, 
			smsMtm->SmsHeader().FromAddress(), smsMtm->Body().Read(0));
	} else {
		CMmsClientMtm* mmsMtm=STATIC_CAST(CMmsClientMtm*, aMtm);
		if ( !IsFromToBuddy(mmsMtm->Sender()) ) {
			//post_error( _L("not from buddy"), KErrGeneral);
			return false;
		}
		
		bool ret=handle_reception(entry_id, folder_id, 
			mmsMtm->Sender(), mmsMtm->Body().Read(0));

		return ret;
	}
}

void CLog_sms::handle_sending(const TMsvId& entry_id, 
	TUid aMtmUid, CBaseMtm* aMtm)
{
	if (aMtmUid == KUidMsgTypeSMS) {
		CSmsClientMtm* smsMtm = STATIC_CAST(CSmsClientMtm*, aMtm);
		const CArrayPtrFlat<CSmsNumber>& addr=smsMtm->SmsHeader().Recipients();
		for (int i=0; i<addr.Count(); i++) {
			if (IsFromToBuddy(addr[i]->Address())) {			
				handle_sending(entry_id, 
					addr[i]->Address(), smsMtm->Body().Read(0));
			}
		}
	} else {
		CMmsClientMtm* mmsMtm=STATIC_CAST(CMmsClientMtm*, aMtm);

		bool buddy=false;
		TInt types[]= { EMmsTo, EMmsCc , EMmsBcc };
		for (int i=0; i<3; i++) {
			const CDesCArray& addr=mmsMtm->TypedAddresseeList( (TMmsRecipients)types[i]);
			for (int j=0; j<addr.Count(); j++) {
				if (ALWAYS_LOG_SENT_MMS || IsFromToBuddy(addr[j])) {
					buddy=true;
					handle_sending(entry_id, addr[j], mmsMtm->Body().Read(0));
				}
			}
		}
		if (buddy) SaveMMSFiles(entry_id, mmsMtm);
	}
}

bool CLog_sms::handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, const TDesC& sender, const TDesC& body)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("handle_reception"));

	if ( !IsFromToBuddy(sender) ) return false;

	iValue->Zero();
	TBuf<30> b;
	b.Format(_L("new msg #%d in "), entry_id);
	iValue->Append(b);
	MapEntry(folder_id, iValue);
	if (folder_id == KMsvGlobalInBoxIndexEntryId)
	{
		iValue->Append(_L(" from "));
	}
	else
	{
		iValue->Append(_L(" to "));
	}
	iValue->Append(sender);
	iValue->Append(_L(":["));
	iValue->Append(body);
	iValue->Append(_L("]"));
	ReplaceEndl(iValue->iPtr);
	post_new_value(iValue);
	return false;
} 

void CLog_sms::handle_change(const TMsvId& msg_id, const TDesC& sender)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("handle_change"));
	
	if (!IsFromToBuddy(sender)) return;
	// no real need to log that ... (?)
	TBuf<50> buf;
	buf.Format(_L("msg #%d modified."), msg_id);
	iValue->Zero(); iValue->Append(buf);
	post_new_value(iValue);	
}

void CLog_sms::handle_read(const TMsvId& msg_id, const TDesC& sender, TUid aMtmUid, CBaseMtm* aMtm)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("handle_read"));

	if (aMtmUid == KUidMsgTypeMultimedia) {
		if (!IsFromToBuddy(sender)) return;
	} else {
		if (!IsFromToBuddy(sender)) return;
	}

	TBuf<50> buf;
	buf.Format(_L("msg #%d read."), msg_id);
	iValue->Zero(); iValue->Append(buf);
	post_new_value(iValue);	

	if (aMtmUid == KUidMsgTypeMultimedia) {
		CMmsClientMtm* mmsMtm=STATIC_CAST(CMmsClientMtm*, aMtm);
		SaveMMSFiles(msg_id, mmsMtm);
	}
}

void CLog_sms::handle_sending(const TMsvId& entry_id, const TDesC &sender, const TDesC &body)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("handle_sending"));

	iValue->Zero();
	TBuf<30> b;
	b.Format(_L("sent msg #%d to "), entry_id);
	iValue->Append(b);
	iValue->Append(sender);
	iValue->Append(_L(": "));
	iValue->Append(body);
	ReplaceEndl(iValue->iPtr);
	post_new_value(iValue);
}

void CLog_sms::handle_delete(const TMsvId& msg_id, const TMsvId& parent_folder, const TDesC& sender)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("handle_delete"));

	if (!IsFromToBuddy(sender)) return;

	TBuf<30> buf;
	buf.Format(_L("msg #%d deleted from "),msg_id);
	iValue->Zero(); iValue->Append(buf);
	MapEntry(parent_folder, iValue);
	post_new_value(iValue);	
}

void CLog_sms::MapEntry(const TMsvId& entry_id, CBBString* aInto)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("MapEntry"));

	TBuf<15> buf;
	if ( entry_id == KMsvGlobalInBoxIndexEntryId)
	{
		buf.Append(_L("Inbox"));
	}
	else if ( entry_id == KMsvDraftEntryId)
	{
		buf.Append(_L("Drafts"));
	}
	else if ( entry_id == KMsvGlobalOutBoxIndexEntryId)
	{
		buf.Append(_L("Outbox"));
	}
	else if ( entry_id == KMsvSentEntryId)
	{
		buf.Append(_L("Sent"));
	}
	else
	{
		buf.Format(_L("dir%d"), entry_id);
	}
	aInto->Append(buf);
}

TBool CLog_sms::IsFromToBuddy(const TDesC& sender)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("IsFromToBuddy"));
	
	iSpecialGroups->store_contact(sender);
	if (iSpecialGroups->is_special_contact()) {
		return ETrue;
	} else {
		return EFalse;
	}
}

void CLog_sms::handle_move(const TMsvId& msg_id, const TMsvId& from_folder, const TMsvId& to_folder, const TDesC& sender)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("handle_move"));

	if (!IsFromToBuddy(sender)) return;

	TBuf<20> buf;
	buf.Format(_L("msg #%d "), msg_id);
	iValue->Zero(); iValue->Append(buf);
	iValue->Append(_L("moved from "));
	MapEntry(from_folder, iValue);
	iValue->Append(_L(" to "));
	MapEntry(to_folder, iValue);
	post_new_value(iValue);	
}

void CLog_sms::handle_error(const TDesC& descr)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("handle_error"));

	post_error(descr, KErrGeneral);
}

EXPORT_C CLog_sms* CLog_sms::NewL(MApp_context& Context, const TDesC& name)
{
	CALLSTACKITEMSTATIC_N(_CL("CLog_sms"), _CL("NewL"));

	auto_ptr<CLog_sms> ret(new (ELeave) CLog_sms(Context));
	ret->ConstructL(name);
	return ret.release();	
}

void CLog_sms::ConstructL(const TDesC& /*name*/)
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("ConstructL"));

	iValue=CBBString::NewL(KSMS);

	Mlog_base_impl::ConstructL();
	iSpecialGroups=CSpecialGroups::NewL(AppContext());
	iSpecialGroups->AddGroupL(_L("Record"));
	iSpecialGroups->read_contact_groups();
}

EXPORT_C CLog_sms::~CLog_sms()
{
	CALLSTACKITEM_N(_CL("CLog_sms"), _CL("~CLog_sms"));
	delete iSpecialGroups;
	delete iValue;
}

CLog_sms::CLog_sms(MApp_context &Context) : Mlog_base_impl(Context, KSMS, KSmsTuple, 0)
{
}
