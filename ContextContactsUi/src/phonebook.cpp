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

#include "phonebook.h"

#include "reporting.h"
#include "settings.h"
#include "cl_settings.h"

#include <cpbkcontactengine.h> 
#include <cpbkcontactiter.h> 
#include <cpbkcontactitem.h> 
#include <cpbkmmsaddressselect.h>
#include <cpbkemailaddressselect.h>

#include <bamatch.h>
#include <contextcommon.mbg>
#include <contextcommon.rsg>
#include <cntdb.h>
#include <CPbkPhoneNumberSelect.h>
#include <cpbksmsaddressselect.h>
#include <e32math.h>

#include <eikenv.h>
#include <bcardeng.h>
#include "raii_f32file.h"
#include "ccu_contactmatcher.h"
#include "symbian_auto_ptr.h"
#include "phonebook_static.h"
#include "break.h"

#include "jaikucacherclientsession.h"

#pragma warning(disable: 4706)




class contacts_key : public TKey 
{
public:
	
	contacts_key(CArrayFixFlat<contact*> & i_arr, 				 
				 HBufC*& aname1, HBufC*& aname2, 
				 bool alast_name_first=true) 
		: arr(i_arr), 
		  name1(aname1), 
		  name2(aname2), 
		  last_name_first(alast_name_first) 
	{ 
	}

	TInt Compare(contact& left, contact& right) const
	{
		if (left.is_myself && !right.is_myself) return -1;
		if (right.is_myself && !left.is_myself) return 1;
		if (left.has_nick && !right.has_nick) return -1;
		if (right.has_nick && !left.has_nick) return 1;
		
		TInt res=0;
		HBufC *left_first=0, *left_second=0;
		HBufC *right_first=0, *right_second=0;
		if (last_name_first) {
			if (left.last_name && left.last_name->Length()) {
				left_first=left.last_name;
				left_second=left.first_name;
			} else {
				left_first=left.first_name;
			}
			if (right.last_name && right.last_name->Length()) {
				right_first=right.last_name;
				right_second=right.first_name;
			} else {
				right_first=right.first_name;
			}
		} else {
			if (left.first_name && left.first_name->Length()) {
				left_first=left.first_name;
				left_second=left.last_name;
			} else {
				left_first=left.last_name;
			}
			if (right.first_name && right.first_name->Length()) {
				right_first=right.first_name;
				right_second=right.last_name;
			} else {
				right_first=right.last_name;
			}
		}
		if (left_first && left_first->Length()==0) left_first=0;
		if (right_first && right_first->Length()==0) right_first=0;
		if (left_second && left_second->Length()==0) left_second=0;
		if (right_second && right_second->Length()==0) right_second=0;
		
		if (left_first && right_first) {
			res=left_first->CompareF(*right_first);
			if (res==0) {
				left_first=left_second;
				right_first=right_second;
			}
		}
		if (res==0) {
			if (left_first && right_first) {
				res=left_first->CompareF(*right_first);
			} else if (left_first) {
				res=-1;
			} else if (right_first) {
				res=1;
			}
		}
		return res;
	}

	
	TInt Compare(TInt aLeft,TInt aRight) const 
	{
		contact* left=arr[aLeft];
		contact* right=arr[aRight];
		return Compare(*left, *right);
	}
	
private:
	CArrayFixFlat<contact*>& arr;
	HBufC*& name1; HBufC*& name2;
	bool last_name_first;
};

class contacts_swap : public TSwap {
public:
	contacts_swap(CArrayFixFlat<contact*> & i_arr) : arr(i_arr) { }
	void Swap(TInt aLeft,TInt aRight) const {
		contact* tmp=arr[aLeft];
		arr[aLeft]=arr[aRight];
		arr[aRight]=tmp;
	}
private:
	CArrayFixFlat<contact*>& arr;
};

class CCachedContacts : public CCheckedActive, public MContextBase {
public:
	static CCachedContacts* NewL(MCachedContactsObserver& aObserver) {
		auto_ptr<CCachedContacts> ret(new (ELeave) CCachedContacts(aObserver));
		ret->ConstructL();
		return ret.release();
	}
	const TDesC8& HashData() {
		return iHashPtr;
	}
	const TDesC8& ListData() {
		return iListPtr;
	}
	~CCachedContacts() {
		Cancel();
		iJaikuCacher.Close();
		CloseChunks();
	}
private:
	CCachedContacts(MCachedContactsObserver& aObserver) : 
		CCheckedActive(EPriorityIdle, _L("CCachedContacts")), 
		iObserver(aObserver), iHashPtr(0, 0), iListPtr(0, 0) { }
	void StartL() {
		User::LeaveIfError(iJaikuCacher.Connect());
		TRequestStatus s;
		iJaikuCacher.GetContactsData(iHashName, iListName, s);
		User::WaitForRequest(s);
		TInt err=s.Int();
		if (err!=KErrNone) {
			User::Leave(err);
		}
		OpenChunksL();
	}
	void Listen() {
		iJaikuCacher.NotifyOnContactsChange(iHashName, iListName, iStatus);
		SetActive();
	}
	void ConstructL() {
		StartL();
		CActiveScheduler::Add(this);
		Listen();
	}
	void CloseChunks()
	{
		iHashPtr.Set(0, 0);
		iListPtr.Set(0, 0);
		if (iHashChunk.Handle()) iHashChunk.Close();
		iHashChunk.SetHandle(0);
		if (iListChunk.Handle()) iListChunk.Close();
		iListChunk.SetHandle(0);
	}
	void OpenChunksL() {
		CloseChunks();
		User::LeaveIfError(iHashChunk.OpenGlobal(iHashName, ETrue));
		User::LeaveIfError(iListChunk.OpenGlobal(iListName, ETrue));
		iHashPtr.Set(iHashChunk.Base(), iHashChunk.Size());
		iListPtr.Set(iListChunk.Base(), iListChunk.Size());
	}
	TInt CheckedRunError(TInt aError) {
		CloseChunks();
		TRAPD(err, iObserver.CachedContactsEvent(aError, ETrue));
		return err;
	}
	TBool iSucceeded;
	void CheckedRunL() {
		TInt err=iStatus.Int();
		if (err==KErrNone) {
			OpenChunksL();
		}
		if (err!=KErrNone) {
			iJaikuCacher.Close();
			if (iSucceeded) {
				StartL();
			} else {
				CloseChunks();
				iObserver.CachedContactsEvent(err, ETrue);
				return;
			}
		}
		Listen();
		iObserver.CachedContactsEvent(err, EFalse);
		iSucceeded=ETrue;
	}
	void DoCancel() {
		iJaikuCacher.Cancel();
	}
	RJaikuCacher		iJaikuCacher;
	MCachedContactsObserver& iObserver;
	TName			iHashName, iListName;
	RChunk			iHashChunk, iListChunk;
	TPtrC8			iHashPtr, iListPtr;
};

EXPORT_C phonebook::phonebook(MApp_context& Context, CJabberData* JabberData, CPresenceHolder* PresenceHolder) : 
MContextBase(Context), iJabberData(JabberData), iPresenceHolder(PresenceHolder)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("phonebook"));

}

EXPORT_C phonebook::~phonebook()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("~phonebook"));

	if (iObservers) {
		CList<phonebook_observer*>::Node *n=iObservers->iFirst;
		while (n) {
			if ( n->Item ) 
				n->Item->exiting();
			n=n->Next;
		}
	}
	delete iObservers;
	
	delete current_ids;
	
	delete iComparator;
	delete iComparatorWorkBuffer1;
	delete iComparatorWorkBuffer2;

	if (contacts) {
		reset_contacts();
	}
	delete contacts;
	
	delete contactitem;
	delete chnot;
	
	delete iContactToIndex;
	if (iPresenceHolder) iPresenceHolder->SetMatcher(0);
	delete iMatcher;
	delete iCachedContacts;
	
	delete iTimer;
	
}

EXPORT_C TInt phonebook::Count()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("Count"));

	return current_ids->Count();
}

EXPORT_C TInt phonebook::GetContactId(TInt Index)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("GetContactId"));


	if (Index<0 || Index >= current_ids->Count()) return KErrNotFound;
	return (*current_ids)[Index];
}

EXPORT_C void phonebook::ConstructL()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("ConstructL"));

	iComparator = NULL;
	iComparatorWorkBuffer1 = HBufC::NewL(1000);
	iComparatorWorkBuffer2 = HBufC::NewL(1000);

	iContactToIndex =CGenericIntMap::NewL();

	current_ids=new (ELeave) CArrayFixFlat<TContactItemId>(50);
	iMatcher=CContactMatcher::NewL(EFalse);
	if (iPresenceHolder) iPresenceHolder->SetMatcher(iMatcher);

	Settings().NotifyOnChange(SETTING_JABBER_NICK, this);

	Settings().GetSettingL( SETTING_PRESENCE_ENABLE, iPresenceEnabled );
	Settings().NotifyOnChange( SETTING_PRESENCE_ENABLE, this );

	iObservers=CList<phonebook_observer*>::NewL();
	iUseCachedContacts=ETrue;
	
	if (iUseCachedContacts) {
		CC_TRAPD(err, iCachedContacts=CCachedContacts::NewL(*this));
		if (err!=KErrNone) {
			TBuf<50> msg=_L("Failed to construct cached contacts: ");
			msg.AppendNum(err);
			Reporting().UserErrorLog(msg);
			iUseCachedContacts=EFalse;
		}
	}
	if (!iUseCachedContacts) {
		chnot=get_engine()->CreateContactChangeNotifierL(this);
	}
	read_db();
	
	iTimer=CTimeOut::NewL(*this, CActive::EPriorityIdle);
}

void phonebook::read_db()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("read_db"));

	Reporting().DebugLog( _L("Read contact data") );
	if (iUseCachedContacts) {
		read_data(0);
	} else {
		read_data(get_engine()->CreateContactIteratorLC(ETrue));
		CleanupStack::PopAndDestroy();
	}
	
	{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("sort"));
	//Reporting().DebugLog( _L("Sort contacts") );
	User::QuickSort(contacts->Count(), *iComparator, contacts_swap(*contacts));
	}
	
	Reporting().DebugLog( _L("Filter contacts") );
	filter(previous_filter, true);
}

void phonebook::CachedContactsEvent(TInt aError, TBool aFailed)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("CachedContactsEvent"));
	if (aError!=KErrNone) {
		TBuf<50> msg=_L("CachedContactsEvent ");
		msg.AppendNum(aError);
		if (aFailed) msg.Append(_L(" -failed"));
		else msg.Append(_L(" -not failed"));
		Reporting().UserErrorLog(msg);
		
		if (!aFailed) {
			// cacher will retry
			return;
		}
		iUseCachedContacts=EFalse;
		chnot=get_engine()->CreateContactChangeNotifierL(this);
	}
	read_db();
}

void phonebook::HandleDatabaseEventL(TContactDbObserverEvent aEvent)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("HandleDatabaseEventL"));
	
	Reporting().DebugLog( _L("Contact db event from"), aEvent.iConnectionId );
	Reporting().DebugLog( _L("  id"), aEvent.iContactId );
	Reporting().DebugLog( _L("  type"), aEvent.iType );

#if 1
	if (aEvent.iConnectionId == get_engine()->Database().ConnectionId()) {
		iTimer->WaitShort(100);
	} else {
		if (iTimer->IsActive()) {
			iTimer->Wait(1);
		} else {
			iTimer->WaitShort(300);
		}
	}
#else
	iTimer->WaitShort(0);
	//read_db();
#endif
}

#include "break.h"

void phonebook::expired(CBase*)
{
	read_db();
}

EXPORT_C void phonebook::PresenceChangedL(TInt aId, CBBPresence* aPresence)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("PresenceChangedL"));
	
	NotifyBeforeChangeL();

	TBool found = EFalse;
	TBool needsSorting =EFalse;
	for ( TInt i=0; i< contacts->Count(); i++ ) {
		contact* c= (*contacts)[i];
		if ( c->id == aId ) {
			needsSorting = ! c->has_nick;
			c->set_presence( aPresence );
			
			// Update last name and first name for dummy items 
			if ( aPresence && iJabberData->IsDummyContactId( aId ) )
				{
					if ( aPresence->iFirstName().Length() > 0 || 
						 aPresence->iLastName().Length() > 0 )
						{
							if ( aPresence->iFirstName().Compare( c->FirstName() ) != 0 )
								{
									c->SetFirstNameL( aPresence->iFirstName() );
									needsSorting = ETrue;
								}
							if ( aPresence->iLastName().Compare( c->LastName() ) != 0 )
								{
									c->SetLastNameL( aPresence->iLastName() );							
									needsSorting = ETrue;
								}
						}
				}
			found = ETrue; 
			break;
		}
	}
	
	if ( found )
		{
			if ( needsSorting ) {
				User::QuickSort(contacts->Count(), *iComparator, contacts_swap(*contacts));
				filter(previous_filter, true);
				// filter calls NotifyContentsChanged
			}
			else
				{
					TBool presenceOnly = ETrue; 
					NotifyContentsChanged(aId, presenceOnly);
				}
		}
	else
		{
			auto_ptr<contact> c( CreateDummyContactL( aId, NULL ) );
			if ( c.get() )
				{
					InsertInOrderL( c.release() );
					filter(KNullDesC, false); 
					// filter calls NotifyContentsChanged
					//NotifyContentsChanged(aId, EFalse );
				}
			else 
				{
					// NOP 
				}
			
		}
}


EXPORT_C void phonebook::NotifyContentsChanged(TInt aContactId, TBool aPresenceOnly)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("NotifyContentsChanged"));

	CList<phonebook_observer*>::Node *n=iObservers->iFirst;
	while (n) {
		n->Item->contents_changed(aContactId, aPresenceOnly);
		n=n->Next;
	}
}


EXPORT_C void phonebook::Notify(const TDesC & /*aMessage*/)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("Notify"));

	// no impl
}

EXPORT_C TInt phonebook::GetIndex(TInt aContactId)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("GetIndex"));

	if ( aContactId == KErrNotFound ) { User::Leave( KErrNotSupported ); }

	TInt idx=(TInt)iContactToIndex->GetData(aContactId);
	if (!idx) return KErrNone;
	contact* c=(*contacts)[idx-1];
	TInt curr_idx=c->current_idx;
	if (!curr_idx) return KErrNone;
	return curr_idx-1;
}

void phonebook::reset_contacts()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("reset_contacts"));

	//iContactToIndex->Reset();
	
	for (int i=0; i<contacts->Count(); i++) {
		delete (*contacts)[i];
	}
	contacts->Reset();
}

//#define DEBUG_FIELDS 0

void phonebook::read_data(CPbkContactIter * iter)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("read_data"));
	
	Reporting().DebugLog( _L("BEGIN of reading contacts.") );

	Reporting().DebugLog( _L("Initialization and reseting data") );
	iMatcher->Reset();
	if (contacts) {
		TInt c=contacts->Count();
		reset_contacts();
		contacts->SetReserveL(c+1);
	} else {
		contacts=new (ELeave) CArrayFixFlat<contact*>(50);
	}
	
	delete iComparator; iComparator=0;


// #ifdef DEBUG_FIELDS
// 	RAFile debugf; debugf.ReplaceLA(Fs(), _L("c:\\pb.txt"),
// 		EFileWrite);
// 	debugf.Write(_L8("\xff\xfe"));
// #endif

	// 1 Real contact items 
	Reporting().DebugLog( _L("Process contacts") );
	lastNameFirst=ETrue;
	
	if (!iUseCachedContacts) {
		CPbkContactItem *item = 0;

		lastNameFirst = get_engine()->NameDisplayOrderL() == CPbkContactEngine::EPbkNameOrderLastNameFirstName;
		auto_ptr<CGenericIntMap> seen_contacts(CGenericIntMap::NewL());

		TContactItemId id = iter->FirstL();
		while ( id != KNullContactId )
		    {
			// Reporting().DebugLog( _L("Loop id"), id );
			item = iter->CurrentL();
			if ( item )
			    {
				// Reporting().DebugLog( _L("Processing contact"), item->Id() );
				
				// Reporting().DebugLog( _L("Add phone number to map") );
				iMatcher->AddPhoneNumbersToMapL(*item);
				
				// Reporting().DebugLog( _L("Get first, last and extra name") );
				GetNamesForContactL(item); // stores to internal buffers first_name, last_name, extra_name
				
				// Reporting().DebugLog( _L("Create contact") );
				auto_ptr<contact> c( CreateContactL( item->Id(), first_name, last_name, extra_name) );
				// Reporting().DebugLog( _L("Append it") );
				contacts->AppendL(c.release());
			    }
			else
			    {
				Reporting().UserErrorLog( _L("NULL contact. Id="), id );
			    }
			    
			seen_contacts->AddDataL( id, (void*)1 );
			TContactItemId nextId = iter->NextL();
			if ( seen_contacts->GetData(nextId) ) 
			    {
				Reporting().UserErrorLog( _L("Contact reading. Repeated id! Id="), id
	);
				EnvErr(_L("Database iterator broken because of Sync")).ErrorCode(KSyncBrokeIterator).Raise();
					
			    break;
			    }
			else
			    id = nextId;
		    }
	} else {
		{
			RDesReadStream hashes(iCachedContacts->HashData());
			TBuf8<16> hash;
			TInt contactid;
			while (contactid=hashes.ReadInt32L()) {
				hashes.ReadL(hash, 16);
				iMatcher->AddPhoneHashToMapL(contactid, hash);
			}
			hashes.Close();
		}
		{
			RDesReadStream contactst(iCachedContacts->ListData());
			TInt order=contactst.ReadInt32L();
			lastNameFirst = (order == CPbkContactEngine::EPbkNameOrderLastNameFirstName);
			TInt contactid;
			TInt length;
			while (contactid=contactst.ReadInt32L()) {
				length=contactst.ReadInt32L();
				contactst.ReadL(first_name, length);
				length=contactst.ReadInt32L();
				contactst.ReadL(last_name, length);
				length=contactst.ReadInt32L();
				contactst.ReadL(extra_name, length);
				auto_ptr<contact> c( CreateContactL( contactid,
					first_name, last_name, extra_name) );
				contacts->AppendL(c.release());
			}
			contactst.Close();
		}
	}
	
	iComparator = new contacts_key(*contacts, iComparatorWorkBuffer1, 
		iComparatorWorkBuffer2, lastNameFirst);
	// 2 Dummy Contact items
	Reporting().DebugLog( _L("Read dummies") );
	ReadDummiesL();
	
	Reporting().DebugLog( _L("END of reading contacts.") );
}


// void phonebook::DebugPrintContactL()
// #ifdef DEBUG_FIELDS
// 		TBuf<100> msg;
// 		TInt id=item->Id();
// 		msg=_L("Contact id "); msg.AppendNum(id);
// 		msg.Append(_L("\n"));
// 		//RDebug::Print(msg);
// 		{
// 			TPtrC8 p( (const TText8*)msg.Ptr(), msg.Length()*2);
// 			debugf.Write(p);
// 		}
// 		CPbkFieldArray& fields=item->CardFields();
// 		for (int i=0; i<fields.Count(); i++) {
// 			const TPbkContactItemField& f=fields.At(i);
// 			msg=_L("field ");
// 			id=f.PbkFieldId();
// 			msg.AppendNum( id );
// 			msg.Append(_L(" "));
// 			msg.Append( f.Text().Left(50) );
// 			msg.Append(_L("\n"));
// 			{
// 				TPtrC8 tp( (const TText8*)msg.Ptr(), msg.Length()*2);
// 				debugf.Write(tp);
// 			}
// 		}
// #endif


void phonebook::GetNamesForContactL(CPbkContactItem* item)
{
    CALLSTACKITEM_N(_CL("phonebook"), _CL("GetNamesForContactL"));
    ::GetNamesForContactL(item, first_name, last_name, extra_name);
}    

void phonebook::ReadDummiesL()
{
    CALLSTACKITEM_N(_CL("phonebook"), _CL("ReadDummiesL"));

    if ( iJabberData )
	{
	    auto_ptr<CContactIdArray> dummies( iJabberData->GetDummyIdsL() );
	    for (TInt i=0; i < dummies->Count(); i++)
		{
		    TContactItemId id = (*dummies)[i];
		    auto_ptr<contact> c(CreateDummyContactL( id, NULL )); 
		    if ( c.get() )
			{
			    contacts->AppendL( c.release() ) ;
			}
		}
	}

}


EXPORT_C void phonebook::ReRead()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("ReRead"));

	read_db();
}

/*
EXPORT_C void phonebook::set_observer(phonebook_observer* i_obs)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("set_observer"));


	obs=i_obs;
}*/

void phonebook::AddObserverL(phonebook_observer* i_obs)
{
	if (!i_obs) return;
	iObservers->AppendL(i_obs);
}

void phonebook::RemoveObserverL(phonebook_observer* i_obs)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("RemoveObserverL"));

	if (!i_obs) return;
	CList<phonebook_observer*>::Node *n=iObservers->iFirst;
	while (n) {
		if (n->Item == i_obs) {
			iObservers->DeleteNode(n, true);
			break;
		}
		n=n->Next;
	}
}




EXPORT_C contact* phonebook::GetContact(TInt index)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("GetContact"));

	
	TInt contactId = GetContactId(index);
	return GetContactById(contactId);
// 	TInt idx=(TInt)iContactToIndex->GetData(contactId);
// 	if (!idx) return KErrNone;
// 	contact* c=(*contacts)[idx-1];
// 	return c;
}

EXPORT_C contact* phonebook::GetContactById(TInt aContactId)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("GetContact"));
	TInt idx=(TInt)iContactToIndex->GetData(aContactId);
	if (!idx) return NULL;
	contact* c=(*contacts)[idx-1];
	return c;
}

EXPORT_C bool phonebook::filter(const TDesC& /*substr*/, bool /*force*/)
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("filter"));
	
	CList<phonebook_observer*>::Node *n=iObservers->iFirst;
	while (n) {
		n->Item->before_change();
		n=n->Next;
	}
	
	current_ids->Reset();
	
	contact* ci;
	TInt idx=0;
	for (int i=0; i<contacts->Count(); i++) {
		ci=(*contacts)[i];

		current_ids->AppendL( ci->id);
		iContactToIndex->AddDataL(ci->id, (void*) (i+1), true);
		ci->current_idx=idx+1;
		idx++;
	}

	NotifyContentsChanged(0, EFalse);
	return true;
}

#include "app_context_impl.h"

EXPORT_C CPbkContactEngine* phonebook::get_engine() const
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("get_engine"));

	// we have to create our own here to get the desctruction order right
	if ( !iMyEngine )
		{
			iMyEngine = CPbkContactEngine::Static();
			if ( ! iMyEngine )
				{
					iMyEngine = CPbkContactEngine::NewL();
					GetContext()->TakeOwnershipL( iMyEngine );
				}
		}
	return iMyEngine;
}

EXPORT_C CArrayFixFlat<contact*>* phonebook::GetContacts()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("GetContacts"));


	return contacts;
}

EXPORT_C class CContactMatcher* phonebook::GetMatcher()
{
	return iMatcher;
}

void phonebook::SettingChanged(TInt aSetting) 
{
    switch ( aSetting )
	{
	case SETTING_PRESENCE_ENABLE:
	    {
		Settings().GetSettingL( SETTING_PRESENCE_ENABLE, iPresenceEnabled );
	    }
	    break;
	case SETTING_JABBER_NICK:
	    {
		ReRead();
	    }
	    break;
	}
}

TBool phonebook::IsPresenceEnabled() const
{
	return iPresenceEnabled;
}

TBool phonebook::ShowLastNameFirstL() const
{
	return lastNameFirst;
}



void phonebook::NotifyBeforeChangeL()
{
	CALLSTACKITEM_N(_CL("phonebook"), _CL("NotifyBeforeChangeL"));

	CList<phonebook_observer*>::Node *n=iObservers->iFirst;
	while (n) {
		n->Item->before_change();
		n=n->Next;
	}
}


contact* phonebook::CreateDummyContactL(TContactItemId aId,
										CBBPresence* aPresence)
{
	CBBPresence* p = aPresence;
	if ( ! p && iPresenceHolder )
		{
			p = iPresenceHolder->GetPresence(aId);
		}
	

	auto_ptr<HBufC> fname( NULL );
	auto_ptr<HBufC> lname( NULL );
	
	CJabberData::TNick nick;
	iJabberData->GetJabberNickL(aId, nick);
	CJabberData::TransformToUiNickL( nick );


	if ( p )
		{
			fname.reset( p->iFirstName().AllocL() );
			lname.reset( p->iLastName().AllocL() );
		}
	else 
		{
			fname.reset( iJabberData->GetFirstNameL(aId) );
			lname.reset( iJabberData->GetLastNameL(aId) );
		}
	
	// use nick as contact name if both first name and last name are missing
	if ( (!fname.get() || fname->Length() == 0) && (!lname.get() || lname->Length() == 0) )
		{ 
			return CreateContactL( aId, nick, KNullDesC, KNullDesC, NULL );
		}
	else 
		{
			return CreateContactL( aId, 
								   fname.get() ? *fname : KNullDesC,
								   lname.get() ? *lname : KNullDesC, 
								   KNullDesC, 
								   p );
		}
}

contact* phonebook::CreateContactL(TContactItemId aId, 
								   const TDesC& aFirstName,
								   const TDesC& aLastName, 
								   const TDesC& aExtraName,
								   CBBPresence* aPresence)
{	
	CBBPresence* p = aPresence;
	auto_ptr<contact> c( contact::NewL( aId, aFirstName, aLastName, aExtraName ) );
	if (iJabberData)
		{
			if ( iJabberData->GetJabberNickL(aId, jabber_nick) && jabber_nick.Length()>1 )
				{
					c->has_nick=true;
					c->is_myself = iJabberData->IsUserNickL( jabber_nick );
				}
			
			c->show_details_in_list=iJabberData->GetShowDetailsInListL(aId);
		}
	
	if ( p )
		{
			c->set_presence( p );
		}
	else if (iPresenceHolder) {
		c->set_presence( iPresenceHolder->GetPresence(aId) );
	}
	
	return c.release();
}

void phonebook::InsertInOrderL(contact* aContact)
{
	auto_ptr<contact> c(aContact);
	TInt ix = 0;
	while ( ix < contacts->Count() && iComparator->Compare( *(contacts->At(ix)), *c ) < 0 )
		{
			ix++;
		}
	
	if ( ix < contacts->Count() )
		{
			contacts->InsertL( ix, c.release() );					
		}
	else
		{
			contacts->AppendL( c.release() );
		}
}


// -------------------------------------------------------------

const TInt CPresenceUpdater::KTimeOut = 60;

EXPORT_C CPresenceUpdater * CPresenceUpdater::NewL(phonebook_i& book)
{
	CALLSTACKITEMSTATIC_N(_CL("CPresenceUpdater"), _CL("NewL"));


	auto_ptr<CPresenceUpdater> ret(new (ELeave) CPresenceUpdater(book));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C CPresenceUpdater::CPresenceUpdater(phonebook_i& book) : book(book), iWaiting(EFalse)
{
	CALLSTACKITEM_N(_CL("CPresenceUpdater"), _CL("CPresenceUpdater"));

}

EXPORT_C CPresenceUpdater::~CPresenceUpdater()
{
	CALLSTACKITEM_N(_CL("CPresenceUpdater"), _CL("~CPresenceUpdater"));

	delete iWait;
}

EXPORT_C void CPresenceUpdater::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPresenceUpdater"), _CL("ConstructL"));


	iWait=CTimeOut::NewL(*this);
	Start();
}


EXPORT_C void CPresenceUpdater::Start()
{
	CALLSTACKITEM_N(_CL("CPresenceUpdater"), _CL("Start"));


	Refresh();
	if (!iWaiting)
	{
		iWait->Wait(KTimeOut);
		iWaiting= ETrue;
	}
}

EXPORT_C void CPresenceUpdater::Stop()
{
	CALLSTACKITEM_N(_CL("CPresenceUpdater"), _CL("Stop"));


	iWait->Reset();	
	iWaiting = EFalse;
}

void CPresenceUpdater::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CPresenceUpdater"), _CL("expired"));


	RDebug::Print(_L("refreshing..."));
	Refresh();
	if (iWaiting)
	{
		iWaiting = EFalse;
		Start();
	}
}

EXPORT_C void CPresenceUpdater::Refresh()
{
	CALLSTACKITEM_N(_CL("CPresenceUpdater"), _CL("Refresh"));
}
