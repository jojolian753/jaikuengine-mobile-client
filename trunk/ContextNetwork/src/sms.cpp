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
#include "sms.h"

#include <mtclreg.h>                        // for CClientMtmRegistry 
#include <msvids.h>                         // for Message type IDs
#include <smscmds.h>
#include <smuthdr.h>
#include <smutset.h>
#include <msvuids.h>
#include <txtrich.h>
#include <smsclnt.h>
#include <mtmdef.h>
#include <gsmupdu.h>
#include <mmsconst.h>
#include <mmsclient.h>
#include "context_uids.h"
#include "app_context_impl.h"
#include "callstack.h"
#ifdef __S60V3__
#include <rsendas.h>
#include <rsendasmessage.h>
#endif

//#define NO_SENDING

class dummyhandler: public MMsvSessionObserver {
public:
	virtual void HandleSessionEventL(TMsvSessionEvent, TAny*, TAny*, TAny*) { }
};

EXPORT_C sms::sms() : CCheckedActive(EPriorityIdle, _L("sms"))
{
	CALLSTACKITEM_N(_CL("sms"), _CL("sms"));

}

EXPORT_C sms::~sms()
{
	CALLSTACKITEM_N(_CL("sms"), _CL("~sms"));

	Cancel();
	
	delete sendOptions;
	delete op;
	delete iMtm;
	delete iReceiveMtm;
	delete iMMSMtm;
	delete iMtmReg;
	delete iReceiveMtmReg;
	delete iReceiveSession;
	delete iSession;
	delete dummy;
	delete iHandlers;

	delete iChangedTimer;
	delete iChangedList;
#ifdef __WINS__
	delete iTimer;
#endif

}

EXPORT_C void sms::ConstructL()
{
	CALLSTACKITEM_N(_CL("sms"), _CL("ConstructL"));

	dummy=new (ELeave) dummyhandler;
	//unread_messages = new (ELeave) RArray<TMsvId>;

	// Create CMsvSession
	iReceiveSession = CMsvSession::OpenSyncL(*this); // new session is opened synchronously
	iReceiveMtmReg = CClientMtmRegistry::NewL(*iReceiveSession);
	// iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeSMS);
	// iMMSMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeMultimedia);

#ifndef NO_SENDING
	iSession = CMsvSession::OpenSyncL(*dummy); // new session is opened synchronously
	iMtmReg = CClientMtmRegistry::NewL(*iSession);
	iMtm = iMtmReg->NewMtmL(KUidMsgTypeSMS);
#endif

	iHandlers=CList<i_handle_received_sms*>::NewL();

	CActiveScheduler::Add(this); // add to scheduler

#ifdef __WINS__
	iTimer=CTimeOut::NewL(*this);
#endif
	iChangedTimer=CTimeOut::NewL(*this);
	iChangedList=CList<TMsvId>::NewL();
}

EXPORT_C bool i_handle_received_sms::handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, 
	TUid aMtmUid, CBaseMtm* aMtm)
{
	if (aMtmUid == KUidMsgTypeSMS) {
		CSmsClientMtm* smsMtm = STATIC_CAST(CSmsClientMtm*, aMtm);
		return handle_reception(entry_id, folder_id, 
			smsMtm->SmsHeader().FromAddress(), smsMtm->Body().Read(0));
	} else {
		return false;
	}
}

EXPORT_C void i_handle_received_sms::handle_sending(const TMsvId& entry_id, 
	TUid aMtmUid, CBaseMtm* aMtm)
{
	if (aMtmUid == KUidMsgTypeSMS) {
		CSmsClientMtm* smsMtm = STATIC_CAST(CSmsClientMtm*, aMtm);
		/*handle_sending(entry_id, 
			smsMtm->SmsHeader().ToAddress(), smsMtm->Body().Read(0));*/
	}
}

EXPORT_C TInt sms::send_message(const TDesC& recipient, const TDesC& body, bool keep_sent)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("send_message"));

	CC_TRAPD(err, send_messageL(recipient, body, keep_sent));
	if (err!=KErrNone /*&& reception_handler*/) {
		TBuf<50> errs;
		errs.Format(_L("err %d: %S"), err, &state);
		//reception_handler->handle_error(errs);
		handle_error(errs);
	}
	//send_messageL(recipient, body);
	return err;
}

void sms::expired(CBase* source)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("expired"));

#ifdef __WINS__
	if (source==iTimer) {
		iCount++;
		TBuf<100> msg;
		msg.Format(_L("itse  on alueella  TESTI%d   "), iCount);
		
		CList<i_handle_received_sms*>::Node* i=iHandlers->iFirst;
		while (i) 
		{
			i->Item->handle_reception(0,0,_L("16507"), msg);
			i=i->Next;
		}
		
	}		
		
#endif
	if (source==iChangedTimer) {
		TMsvId id=iChangedList->Top();
		handle_changed(id);
		iChangedList->Pop();
		if ( iChangedList->iCount > 0) {
			iChangedTimer->Wait(1);
		}
	}
}

void sms::send_messageL(const TDesC& recipient, const TDesC& body, bool keep_sent)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("send_messageL"));

#ifdef __WINS__
	iTimer->Wait(1);
	return;
#endif

#ifdef NO_SENDING
	return;
#endif
	if (iStatus==KRequestPending) {
		User::Leave(KErrServerBusy);
	}

	state=_L("send_messageL");
	
	TMsvEntry newEntry;								 // This represents an entry in the Message Server index
	newEntry.iMtm = KUidMsgTypeSMS;                         // message type is SMS
	newEntry.iType = KUidMsvMessageEntry;                   // this defines the type of the entry: message 
	newEntry.iServiceId = KMsvLocalServiceIndexEntryId;     // ID of local service (containing the standard folders)
	newEntry.iDate.HomeTime();                              // set the date of the entry to home time
			
	newEntry.SetInPreparation(ETrue);                       // a flag that this message is in preparation
	

	// get ref to outbox
	state=_L("NewL");
	auto_ptr<CMsvEntry> entry(CMsvEntry::NewL(*iSession, KMsvGlobalOutBoxIndexEntryId ,TMsvSelectionOrdering()));
	
	// create message in outbox
	state=_L("CreateL");
	entry->CreateL(newEntry);	
	
	state=_L("GetEntry");
	entry.reset(iSession->GetEntryL(newEntry.Id()));
	
	// SetCurrentEntryL takes ownership of the CMsvEntry
	state=_L("SetCurrentEntry");
	iMtm->SetCurrentEntryL(entry.get());
	entry.release();
	
	state=_L("Entry()");
	TMsvEntry msvEntry = (iMtm->Entry()).Entry();
	
	state=_L("Body()");
	CRichText& mtmBody = iMtm->Body();
	mtmBody.Reset();
	
	state=_L("set message");
	mtmBody.InsertL(0, body);   // insert our msg tag as the body text
	
	// set iRecipient into the Details of the entry
	msvEntry.iDetails.Set(recipient);  // set recipient info in details
	msvEntry.SetInPreparation(EFalse);         // set inPreparation to false
	
	msvEntry.SetSendingState(KMsvSendStateWaiting);   // set the sending state (immediately)
	msvEntry.iDate.HomeTime();                        // set time to Home Time


	
	
	// To handle the sms specifics we start using SmsMtm
	CSmsClientMtm* smsMtm = STATIC_CAST(CSmsClientMtm*, iMtm);
	
	state=_L("RestoreServiceAndSettingsL");
	smsMtm->RestoreServiceAndSettingsL();
	
	state=_L("set header");
	// CSmsHeader encapsulates data specific for sms messages,
	// like service center number and options for sending.
	CSmsHeader& header = smsMtm->SmsHeader();
	state=_L("get options");

	if (!sendOptions) {
		sendOptions = CSmsSettings::NewL();
		sendOptions->CopyL(smsMtm->ServiceSettings()); // restore existing settings
		state=_L("SetDelivery");
		sendOptions->SetDelivery(ESmsDeliveryImmediately);      // set to be delivered immediately
		sendOptions->SetDeliveryReport(EFalse); // no delivery report
	}
	
	// set send options
	state=_L("SetSmsSettingsL");
	header.SetSmsSettingsL(*sendOptions);
	
	state=_L("check sc");
	// let's check if there's sc address
	if (header.Message().ServiceCenterAddress().Length() == 0)
	{
		// no, there isn't. We assume there is at least one sc number set and use
		// the default SC number. 
		CSmsSettings* serviceSettings = &(smsMtm->ServiceSettings());
		
		// if number of scaddresses in the list is null
#ifndef __S60V3__
		if (!serviceSettings->NumSCAddresses())
#else
		if (!serviceSettings->ServiceCenterCount())
#endif
		{
			// FIXME: what to do?
			User::Leave(1);
		} else {
			// set sc address to default. 
#ifndef __S60V3__
			CSmsNumber* sc = 0;
			sc = &(serviceSettings->SCAddress(serviceSettings->DefaultSC()));
			header.Message().SetServiceCenterAddressL(sc->Address());
#else
			CSmsServiceCenter &sc=serviceSettings->GetServiceCenter(
				serviceSettings->DefaultServiceCenter());
			header.Message().SetServiceCenterAddressL(sc.Address());
#endif
		}
	}
	
	state=_L("add addresses");
	// Add our recipient to the list, takes in two TDesCs, first is real address and second is an alias
	// works also without the alias parameter.
	smsMtm->AddAddresseeL(recipient, msvEntry.iDetails);
	
	// if message is to be deleted after sending, mark with our UID
	if (!keep_sent) {
		msvEntry.iMtmData3 = KUidcontext_log.iUid;
	}
	
	state=_L("save");
	// save message
	iMtm->Entry().ChangeL(msvEntry);                // make sure that we are handling the right entry
	smsMtm->SaveMessageL();                 // closes the message
	
	state=_L("move");
	// This moves the message entry to outbox, we'll schedule it for sending after this. 
	//    TMsvId movedId = MoveMessageEntryL( KMsvGlobalOutBoxIndexEntryId );  // move message to outbox
	TMsvId movedId = iMtm->Entry().Entry().Id();
	
	// We must create an entry selection for message copies (although now we only have one message in selection)
	auto_ptr<CMsvEntrySelection> selection(new (ELeave) CMsvEntrySelection);
	
	selection->AppendL(movedId);            // add our message to the selection
	
	TBuf8<1> dummyParams;
	//    CCommandAbsorbingControl::NewLC();
	
	state=_L("InvokeAsyncFunctionL");
	// invoking async schedule copy command on our mtm
	op=iMtm->InvokeAsyncFunctionL(
		ESmsMtmCommandScheduleCopy,
		*selection,
		dummyParams,
		iStatus);
	
	SetActive();
	
	//return KErrNone;
}

void sms::HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("HandleSessionEventL"));

	//handle_error(_L("testing!"));

	switch(aEvent) {
	case EMsvEntriesCreated:
		{
			if (!aArg1 || !aArg2 ) return;
			CMsvEntrySelection* entries=(CMsvEntrySelection *)aArg1;
			TMsvId id=*(TMsvId*)aArg2;
			if (id != KMsvGlobalInBoxIndexEntryId) {
				TBuf<40> msg;
				for (int i=0; i< entries->Count(); i++) {
					/*
					msg=_L("created ");
					msg.AppendNum( TInt(entries->At(i)) );
					msg=_L("in "); msg.AppendNum( TInt(id) );
					handle_error(msg);
					*/
				}
			} else {			
				for (int i=0; i< entries->Count(); i++) {
					handle_received(entries->At(i), id);
				}
			}
		}
		break;

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
					//DeleteSentEntry(entries->At(i)); // this checks the entry and deletes if it is created by GDSMS app
					handle_sent(entries->At(i)); 
				}

			}
			
			for(TInt i = 0; i < entries->Count(); i++)
			{
				handle_moved(entries->At(i), *fromEntryId, *toEntryId);
			}
		}
		break;

	case EMsvEntriesChanged:
		{
			if (!aArg1) return;

			CMsvEntrySelection* entries = static_cast<CMsvEntrySelection*>(aArg1);
			TMsvId* ParentFolder = static_cast<TMsvId*>(aArg2);
			
			if (*ParentFolder == KMsvGlobalInBoxIndexEntryId ) {
				for(TInt i = 0; i < entries->Count(); i++) {
					iChangedList->AppendL(entries->At(i));
					//handle_changed(entries->At(i));
				}
				iChangedTimer->Wait(60);
			}
		}
		break;

	case EMsvEntriesDeleted:
		{
				
			if (!aArg1 || !aArg2) return;

			TMsvId* folderId;
			folderId = static_cast<TMsvId*>(aArg2); 
				
			CMsvEntrySelection* entries = static_cast<CMsvEntrySelection*>(aArg1);
						
			for(TInt i = 0; i < entries->Count(); i++)
			{
				handle_deleted(entries->At(i), *folderId);
			}
			
		}
		break;

	default:
		break;
	}
	delete iMMSMtm; iMMSMtm=0;
	delete iReceiveMtm; iReceiveMtm=0;
	
}

TBool sms::DeleteSentEntry(TMsvId entry)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("DeleteSentEntry"));

	if (iReceiveMtm==0) iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeSMS);

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

void sms::handle_error(const TDesC& descr)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("handle_error"));

	CList<i_handle_received_sms*>::Node* i=iHandlers->iFirst;
	while (i) 
	{
		i->Item->handle_error(descr);
		i=i->Next;
	}	
}

TUid sms::loadmessageL(const TMsvId& entry_id, TMsvEntry& entry)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("loadmessageL"));

	TInt err;
	//if (!reception_handler) return;
	
	auto_ptr<CMsvEntry> realentry(0);
	CC_TRAP(err, realentry.reset(iReceiveSession->GetEntryL(entry_id)) );
	if (err!=KErrNone) 
	{
		TBuf<50> msg;
		msg.Format(_L("Error in GetEntry %d"), err);
		//handle_error(msg);
		return KNullUid;
	}
	entry=realentry->Entry();

	if (entry.iMtm != KUidMsgTypeSMS && entry.iMtm != KUidMsgTypeMultimedia) 
	{
		//handle_error(_L("Not an SMS or MMS"));
		return KNullUid;
	}

	CBaseMtm* mtm=0;
	// SetCurrentEntryL takes ownership of the CMsvEntry
	TMsvPartList validationFlags(KMsvMessagePartDescription|KMsvMessagePartOriginator);
	if (entry.iMtm == KUidMsgTypeSMS) {
		if (iReceiveMtm==0) iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeSMS);
		mtm = iReceiveMtm;
		validationFlags |= KMsvMessagePartBody;
	} else {
		if (iMMSMtm==0) iMMSMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeMultimedia);
		mtm = iMMSMtm;
	}
	CC_TRAP(err, mtm->SetCurrentEntryL(realentry.get()));
	realentry.release();
	if (err!=KErrNone) 
	{
		TBuf<50> msg;
		msg.Format(_L("Error in SetCurrentEntryL: %d"), err);
		handle_error(msg);
		return KNullUid;
	}
	if (mtm->ValidateMessage(validationFlags != 0)) 
	{
		handle_error(_L("Not validated!"));
		return KNullUid;
	}
	
	// The message loading sometimes fails with KErrAccessDenied - the message server is
	// busy. Just retry loading. 
	TInt retry_count=0; TInt MAX_RETRIES=5;
	while (retry_count<MAX_RETRIES) 
	{
		CC_TRAP(err, mtm->LoadMessageL());
		if (err==KErrAccessDenied || err==KErrInUse) {
			retry_count++;
			User::After(TTimeIntervalMicroSeconds32(100*1000)); //100 ms
		} else if (err==-1) {
			// can ignore
			return KNullUid;
		} else if (err!=KErrNone) {
			TBuf<50> msg;
			msg.Format(_L("Error in LoadMessageL: %d"), err);
			handle_error(msg);
			return KNullUid;
		} else {
			break;
		}
	}
	if (retry_count==MAX_RETRIES) {
		//reception_handler->handle_error(_L("max retries on LoadMessageL reached"));
		handle_error(_L("max retries on LoadMessageL reached"));
		return KNullUid;
	}
	return entry.iMtm;
}

void sms::handle_received(const TMsvId& entry_id, const TMsvId& folder_id)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("handle_received"));

	TInt err;

	TMsvEntry entry;
	TUid mtmuid=loadmessageL(entry_id, entry);
	CBaseMtm* mtm = 0;
	if (mtmuid == KUidMsgTypeSMS ) {
		if (iReceiveMtm==0) iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeSMS);
		mtm=iReceiveMtm;
		//handle_error(_L("received SMS"));
	} else if ( mtmuid== KUidMsgTypeMultimedia ) {
		if (iMMSMtm==0) iMMSMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeMultimedia);
		mtm=iMMSMtm;
		//handle_error(_L("received MMS"));
	} else {
		return;
	}

	//-------------------------------------------------------------------------------------------
	
	CList<i_handle_received_sms*>::Node* i=iHandlers->iFirst;
	while (i) 
	{
		bool resp=false;
		TInt err;
		CC_TRAP(err, resp=i->Item->handle_reception(entry_id, folder_id, mtmuid, mtm));
		if (err!=KErrNone) {
			TBuf<50> msg; msg=_L("Error in handle_reception: ");
			msg.AppendNum(err);
			auto_ptr<HBufC> stack( GetContext()->CallStackMgr().GetFormattedCallStack(msg) );
			handle_error(*stack);
		}
		if ( resp )
		{
			entry.SetNew(EFalse);
			entry.SetVisible(EFalse);
		
			// if this fails, a message notification will pop up
			// but nothing drastic happens. Nothing we can do to fix,
			// so just ignore it
			CC_TRAP(err, mtm->Entry().ChangeL(entry));
			if (err!=KErrNone) 
			{
				mtm->SaveMessageL();                 // closes the message
			}
			
			TMsvSelectionOrdering sort;
			sort.SetShowInvisibleEntries(ETrue);
			
			auto_ptr<CMsvEntry> parent(CMsvEntry::NewL(*iReceiveSession, entry.Parent(), sort));					
			parent->DeleteL(entry_id);
		}
		i=i->Next;
	}

}

EXPORT_C void sms::AddHandler(i_handle_received_sms* handler)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("AddHandler"));

	iHandlers->AppendL(handler);
}

void sms::handle_deleted(const TMsvId& entry_id, const TMsvId& parent_id)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("handle_deleted"));

	auto_ptr<CMsvEntry> realentry(0);
	CC_TRAPD(err, realentry.reset(iReceiveSession->GetEntryL(entry_id)) );
	if (err!=KErrNone) 
	{
		return;
	}
	CSmsClientMtm* smsMtm = 0;
	CMmsClientMtm* mmsMtm = 0;
	if (realentry->Entry().iMtm == KUidMsgTypeSMS ) {
		if (iReceiveMtm==0) iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeSMS);
		smsMtm=STATIC_CAST(CSmsClientMtm*, iReceiveMtm);
	} else if ( realentry->Entry().iMtm == KUidMsgTypeMultimedia ) {
		if (iMMSMtm==0) iMMSMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeMultimedia);
		mmsMtm=STATIC_CAST(CMmsClientMtm*, iMMSMtm);
	} else {
		return;
	}

	CList<i_handle_received_sms*>::Node* i=iHandlers->iFirst;
	while (i) 
	{
		if (smsMtm) {
			i->Item->handle_delete(entry_id, parent_id, smsMtm->SmsHeader().FromAddress() );
		} else {
			i->Item->handle_delete(entry_id, parent_id, mmsMtm->Sender() );
		}
		i=i->Next;
	}
	
}

void sms::handle_sent(const TMsvId& entry_id)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("handle_sent"));

	TMsvEntry entry;
	TUid mtmuid=loadmessageL(entry_id, entry);
		
	CBaseMtm* mtm = 0;
	if (mtmuid == KUidMsgTypeSMS ) {
		if (iReceiveMtm==0) iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeSMS);
		mtm=iReceiveMtm;
	} else if ( mtmuid== KUidMsgTypeMultimedia ) {
		if (iMMSMtm==0) iMMSMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeMultimedia);
		mtm=iMMSMtm;
	} else {
		//handle_error(_L("sent: not an sms or mms"));
		return;
	}

	//-------------------------------------------------------------------------------------------
	
	CList<i_handle_received_sms*>::Node* i=iHandlers->iFirst;
	while (i) 
	{
		i->Item->handle_sending(entry_id, mtmuid, mtm);
		i=i->Next;
	}	

}

void sms::handle_moved(const TMsvId& /*entry_id*/, const TMsvId& /*from_entry_id*/, const TMsvId& /*to_entry_id*/)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("handle_moved"));

	/*CSmsClientMtm* smsMtm = STATIC_CAST(CSmsClientMtm*, iReceiveMtm);

	CList<i_handle_received_sms*>::Node* i=iHandlers->iFirst;
	while (i) 
	{
		i->Item->handle_move(entry_id, from_entry_id, to_entry_id, smsMtm->SmsHeader().FromAddress());
		i=i->Next;
	}*/

}

void sms::handle_changed(const TMsvId& entry_id)
{
	CALLSTACKITEM_N(_CL("sms"), _CL("handle_changed"));

	TMsvEntry entry;
	TUid mtmuid=loadmessageL(entry_id, entry);
	if (mtmuid == KNullUid) return;

	CSmsClientMtm* smsMtm = 0;
	CMmsClientMtm* mmsMtm = 0;
	if (mtmuid == KUidMsgTypeSMS ) {
		if (iReceiveMtm==0) iReceiveMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeSMS);
		smsMtm=STATIC_CAST(CSmsClientMtm*, iReceiveMtm);
	} else if ( mtmuid == KUidMsgTypeMultimedia ) {
		if (iMMSMtm==0) iMMSMtm = iReceiveMtmReg->NewMtmL(KUidMsgTypeMultimedia);
		mmsMtm=STATIC_CAST(CMmsClientMtm*, iMMSMtm);
		/*
		 * forcing to visible isn't ther right thing to do either:
		 * we just get visible but unreadable mms :-(
		 *
		if ( ! entry.Visible() ) {
			entry.SetVisible(ETrue);
			TInt err=-1;
			TInt count=0;
			while (err!=KErrNone && count<3) {
				CC_TRAP(err, mmsMtm->Entry().ChangeL(entry));
				count++;
				TTimeIntervalMicroSeconds32 w(300*1000);
				User::After(w);
			}
			if (err==KErrNone) {
				handle_error(_L("Forcing MMS to visible"));
				mmsMtm->SaveMessageL();                 // closes the message
			} else {
				TBuf<50> msg=_L("Failed to force MMS to visible ");
				msg.AppendNum(err);
				handle_error(msg);
			}
		} else {
			handle_error(_L("MMS changed and visible"));
			return;
		}
		*/
	} else {
		return;
	}

	TBool read=EFalse; TBool complete=EFalse;
	if (! entry.New() ) read=ETrue;

	CList<i_handle_received_sms*>::Node* i=iHandlers->iFirst;
	while (i) 
	{
		if ( read ) {
			if (smsMtm) {
				i->Item->handle_read(entry_id, 
					smsMtm->SmsHeader().FromAddress(), mtmuid,
					smsMtm);
			} else {
				i->Item->handle_read(entry_id, 
					mmsMtm->Sender(), mtmuid, mmsMtm);
			}
		} 
		i=i->Next;
	}
}
