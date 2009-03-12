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
#include "mms.h"
#include "util.h"

#include <mtclreg.h>                        // for CClientMtmRegistry 
#include <msvuids.h>
#include <mmsclient.h>
#include <mtmdef.h>
#include <mmsconst.h>
#include <MSVIDS.H>
#include <bautils.h>

#include "checkedactive.h"
#include "app_context.h"
#include "app_context_impl.h"
#include "symbian_auto_ptr.h"
#include "context_uids.h"
//#include "transfer2.h"
#include "app_context.h"
#include "raii_f32file.h"
#include <charconv.h>

#ifdef __S60V3__
#include <CMsvMimeHeaders.h>
#endif

class dummyhandler: public MMsvSessionObserver {
public:
	virtual void HandleSessionEventL(TMsvSessionEvent, TAny*, TAny*, TAny*) { }
};

class CMMSImpl : public CMMS, public MMsvSessionObserver, public MContextBase
{
public:
	~CMMSImpl();
private:
	void ConstructL();
	void CheckedRunL();
	void DoCancel();
	TInt SendMessage(const TDesC& recipient, const TDesC& body, const TDesC& attachment, TDes& Errmsg, bool keep_sent=true);
	TInt SendMessage(const TDesC& recipient, const TDesC& body, const MDesCArray& attachments,
		TDes& Errmsg, bool keep_sent=true);
	virtual void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3);	
	void SendMessageL(const TDesC& recipient, const TDesC& body, const TDesC& attachment, bool keep_sent=true);
	void SendMessageL(const TDesC& recipient, const TDesC& body, const MDesCArray& attachments, bool keep_sent=true);
	
	TBool DeleteSentEntry(TMsvId aEntryId);
	
	CMsvSession* iSession;
	CClientMtmRegistry* iMtmReg;
	CBaseMtm*	iMtm;
	
	CMsvSession* iReceiveSession;
	CClientMtmRegistry* iReceiveMtmReg;
	CBaseMtm*	iReceiveMtm;
	
	TBuf<50> state;
	CMsvOperation* op;
	dummyhandler dummy;

	friend class CMMS;
};


EXPORT_C CMMS* CMMS::NewL()
{
	CALLSTACKITEM_N(_CL("CMMS"), _CL("NewL"));
	auto_ptr<CMMSImpl> ret(new (ELeave) CMMSImpl);
	ret->ConstructL();
	return ret.release();
}

CMMS::CMMS() : CCheckedActive(EPriorityIdle, _L("mms")) { }

CMMSImpl::~CMMSImpl()
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("~CMMSImpl"));
	
	Cancel();
	
	delete op;
	delete iMtm;
	delete iMtmReg;
	delete iSession;

	delete iReceiveMtm;
	delete iReceiveMtmReg;
	delete iReceiveSession;
}

void CMMSImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("ConstructL"));
	
	iSession = CMsvSession::OpenSyncL(dummy); // new session is opened synchronously
	iMtmReg = CClientMtmRegistry::NewL(*iSession);
	iMtm = iMtmReg->NewMtmL(KUidMsgTypeMultimedia);
	
	iReceiveSession = CMsvSession::OpenSyncL(*this); // new session is opened synchronously
	iReceiveMtmReg = CClientMtmRegistry::NewL(*iReceiveSession);
	iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeMultimedia);
	
	CActiveScheduler::Add(this); // add to scheduler
}

TInt CMMSImpl::SendMessage(const TDesC& recipient, const TDesC& body, const TDesC& attachment, TDes& ErrMsg, bool keep_sent)
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("SendMessage"));
	
	CC_TRAPD(err, SendMessageL(recipient, body, attachment, keep_sent));
	if (err!=KErrNone) ErrMsg=state;
	else ErrMsg.Zero();
	return err;
}

TInt CMMSImpl::SendMessage(const TDesC& recipient, const TDesC& body, const MDesCArray& attachments, TDes& ErrMsg, bool keep_sent)
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("SendMessage"));
	
	CC_TRAPD(err, SendMessageL(recipient, body, attachments, keep_sent));
	if (err!=KErrNone) ErrMsg=state;
	else ErrMsg.Zero();
	return err;
}

void CMMSImpl::SendMessageL(const TDesC& recipient, const TDesC& body, const TDesC& attachment, bool keep_sent)
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("SendMessageL"));

	auto_ptr<CDesCArrayFlat> a(new (ELeave) CDesCArrayFlat(1));
	a->AppendL(attachment);
	SendMessageL(recipient, body, *a, keep_sent);
}

void CMMSImpl::SendMessageL(const TDesC& recipient, const TDesC& body, const MDesCArray& attachments, bool keep_sent)
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("SendMessageL"));
	
	if (iStatus==KRequestPending) {
		User::Leave(KErrServerBusy);
	}
	
	state=_L("send_messageL");
	
	// To handle the sms specifics we start using MmsMtm
	CMmsClientMtm* iMmsMtm = STATIC_CAST(CMmsClientMtm*, iMtm);
	
	state=_L("SwitchCurrentEntryL");
	// Set context to the parent folder (Drafts)
	iMmsMtm->SwitchCurrentEntryL(KMsvDraftEntryId);
	// Create new message in the parent folder and set it
	// as the current context. Note that this method marks
	// the message as invisible and in preparation. This will
	// prevent it from being tampered with during
	// preparation.
	state=_L("CreateMessageL");
#ifndef __S60V3__
	iMmsMtm->CreateMessageL(iMmsMtm->DefaultSettingsL());	
#else
	iMmsMtm->CreateMessageL(iMmsMtm->DefaultServiceL());	
	auto_ptr<CMsvStore> store(iMmsMtm->Entry().EditStoreL());
#endif
	
	state=_L("AddAddresseeL");
	iMmsMtm->AddAddresseeL(recipient);
	
	if (body.Length()>0) {
		state=_L("CreateTextAttachmentL");
#ifndef __S60V3__
		TMsvId textid;
		iMmsMtm->CreateTextAttachmentL(textid, body);
#else
		TMsvAttachmentId textid;
		iMmsMtm->CreateTextAttachmentL( *(store.get()),
			textid, body, _L("body.txt"));
#endif
	}

	for (int i=0; i< attachments.MdcaCount(); i++) {
		RFs& fs=GetContext()->Fs();
		if (BaflUtils::FileExists(fs, attachments.MdcaPoint(i))) {
			state=_L("CreateAttachment2L");
#ifndef __S60V3__
			TMsvId attid;
			iMmsMtm->CreateAttachment2L(attid, attachments.MdcaPoint(i));
#else
			TMsvAttachmentId attid;
			TBuf<100> mime; GetMimeTypeL(attachments.MdcaPoint(i), mime);
			TBuf8<100> mime8;
			CC()->ConvertFromUnicode( mime8, mime );
			RAFile f; f.OpenLA(Fs(), attachments.MdcaPoint(i), EFileRead|EFileShareAny);
			TParse l; l.Set(attachments.MdcaPoint(i), 0, 0);
			auto_ptr<CMsvMimeHeaders> head(CMsvMimeHeaders::NewL());
			head->SetSuggestedFilenameL(l.NameAndExt());
			CMsvAttachment* info=CMsvAttachment::NewL(CMsvAttachment::EMsvFile);
			
			iMmsMtm->CreateAttachment2L( *(store.get()),
				f, mime8, *head, info, 
				attid);
#endif
		}
	}
	
	state=_L("set flags");
	// Set the message’s status flags appropriately
	TMsvEntry ent = iMmsMtm->Entry().Entry();
	ent.SetInPreparation(EFalse);
	ent.SetVisible(ETrue);
	// if message is to be deleted after sending, mark with our UID
	if (!keep_sent) {
		ent.iMtmData3 = KUidcontext_log.iUid;
	}
	state=_L("save");
	// Save changes (If you do not call this method, all changes
	// made will be lost when the context is changed.)
	iMmsMtm->Entry().ChangeL(ent); // Commit changes
	iMmsMtm->SaveMessageL();
	
	state=_L("send");
	iStatus=KRequestPending;
	op = iMmsMtm->SendL(iStatus);
	
	SetActive();
	
}

void CMMSImpl::HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3)
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("HandleSessionEventL"));
	
	switch(aEvent) {
		
	case EMsvEntriesMoved:      // this event is given when message entries are moved
		{
			// An entry has been moved to another parent
			// We are interested messages that have been moved to Sent folder
			if (!aArg1 || !aArg2 || !aArg3 ) return;
			
			TMsvId* toEntryId;
			TMsvId* fromEntryId;
			toEntryId = static_cast<TMsvId*>(aArg2); 
			fromEntryId = static_cast<TMsvId*>(aArg3);
			
			CMsvEntrySelection* entries = static_cast<CMsvEntrySelection*>(aArg1);
			
			if ( *toEntryId == KMsvSentEntryId )   // the entry has been moved into Sent folder
			{
				// We take the moved entries into a selection
				
				//Process each created entry, one at a time.
				for(TInt i = 0; i < entries->Count(); i++)
				{
					DeleteSentEntry(entries->At(i)); // this checks the entry and deletes if it is created by GDSMS app
				}
				
			}
		}
		break;
	default:
		break;
	}
	
}

TBool CMMSImpl::DeleteSentEntry(TMsvId entry)
{
	CALLSTACKITEM_N(_CL("CMMSImpl"), _CL("DeleteSentEntry"));
	
	TInt err;
	// Load this entry to our mtm
	CC_TRAP(err, iReceiveMtm->SwitchCurrentEntryL(entry));
	// probably wasn't compatible, ignore
	if (err!=KErrNone) return EFalse;
	CC_TRAP(err, iReceiveMtm->LoadMessageL());
	// probably wasn't compatible, ignore
	if (err!=KErrNone) return EFalse;
	
	TMsvEntry msvEntry( (iReceiveMtm->Entry()).Entry() );
	
        if (msvEntry.iMtmData3 == KUidcontext_log.iUid)    // this entry has been created by our app
	{
		// Taking a handle to the Sent folder...
		TMsvSelectionOrdering sort;
		sort.SetShowInvisibleEntries(ETrue);    // we want to handle also the invisible entries
		// Take a handle to the parent entry
		auto_ptr<CMsvEntry> parentEntry(CMsvEntry::NewL(iReceiveMtm->Session(), msvEntry.Parent(), sort));
		
		// here parentEntry is the Sent folder (must be so that we can call DeleteL) 
		parentEntry->DeleteL(msvEntry.Id());
		
		return ETrue; // entry was deleted
	}
	
	return EFalse; // no entries deleted
}

void CMMSImpl::CheckedRunL()
{
}

void CMMSImpl::DoCancel()
{
}
