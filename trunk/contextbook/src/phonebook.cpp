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

#include <flogger.h>


#include "phonebook.h"

#include <cpbkcontactengine.h> 
#include <cpbkcontactiter.h> 
#include <cpbkcontactitem.h> 

#include <bamatch.h>
#include <cntdb.h>
#include <CPbkPhoneNumberSelect.h>
#include "presence_data.h"
#include <e32math.h>

#include "contextbook.hrh"
#include <eikenv.h>
#include <contextbook.rsg>

#pragma warning(disable: 4706)



contact* contact::NewL(TContactItemId i_id, const TDesC& i_first_name, const TDesC& i_last_name) {
	contact* c=new (ELeave) contact;
	CleanupStack::PushL(c);
	c->id=i_id;
	
	c->first_name=HBufC::NewL(i_first_name.Length());
	*(c->first_name)=i_first_name;
	
	c->last_name=HBufC::NewL(i_last_name.Length());
	*(c->last_name)=i_last_name;
	
	c->name=HBufC::NewL(i_first_name.Length()+i_last_name.Length()+50);
	*(c->name)=i_last_name;
	if (i_last_name.Length()>0 && i_first_name.Length()>0) {
		c->name->Des().Append(_L(" "));
	}
	c->name->Des().Append(i_first_name);
	c->name->Des().Append(_L("\t \t0\t0\t0")); //type No_icon_to_display = 0
	
	CleanupStack::Pop();
	
	return c;
}

void contact::PrintPresenceToListBox()
{
	CALLSTACKITEM(_L("contact::PrintPresenceToListBox"));

	
}

void contact::set_presence(MPresenceData* data)
{
	CALLSTACKITEM(_L("contact::set_presence"));

	if (presence) presence->Release();
	presence=data;
	if (data) data->AddRef();

	if (! has_nick) return;
	TBuf<6> prev; TBuf<30> not_avail;
	CEikonEnv::Static()->ReadResourceAsDes16(prev, R_PREVIOUS_CAPTION);
	CEikonEnv::Static()->ReadResourceAsDes16(not_avail, R_JABBER_NOT_AVAIL);
	
	PresenceToListBoxL(presence, name, last_name, first_name, prev, not_avail);
}


contact::~contact() {
	if (presence) presence->Release();
	delete name;
	delete first_name;
	delete last_name;
}


class contacts_key : public TKey {
public:
	contacts_key(CArrayFixFlat<contact*> & i_arr) : arr(i_arr) { }
	TInt Compare(TInt aLeft,TInt aRight) const {
		contact* left=arr[aLeft];
		contact* right=arr[aRight];
		if (left->has_nick && !right->has_nick) return -1;
		if (right->has_nick && !left->has_nick) return 1;
		return left->name->Des().Compare(right->name->Des());
	}
private:
	CArrayFixFlat<contact*>& arr;
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

phonebook::phonebook(MApp_context& Context, CJabberData& JabberData, CPresenceHolder& PresenceHolder) : 
MContextBase(Context), iJabberData(JabberData), iPresenceHolder(PresenceHolder)
{
	CALLSTACKITEM(_L("phonebook::phonebook"));

}

phonebook::~phonebook()
{
	CALLSTACKITEM(_L("phonebook::~phonebook"));

	if (obs) obs->exiting();
	
	delete current_array;
	delete current_ids;
	
	if (contacts) {
		reset_contacts();
	}
	delete contacts;
	
	delete contactitem;
	delete chnot;
	
	if (owns_engine) {
		delete eng;
	}
	
	delete iContactToIndex;
}

TInt phonebook::GetContactId(TInt Index)
{
	CALLSTACKITEM(_L("phonebook::GetContactId"));

	if (Index<0 || Index >= current_ids->Count()) return KErrNotFound;
	return (*current_ids)[Index];
}

void phonebook::ConstructL()
{
	CALLSTACKITEM(_L("phonebook::ConstructL"));

	iContactToIndex=CGenericIntMap::NewL();
	
	current_array = new (ELeave) CPtrCArray(50);
	current_ids=new (ELeave) CArrayFixFlat<TContactItemId>(50);
	
	eng=CPbkContactEngine::Static();
	
	if (eng) {
		owns_engine=false;
	} else {
		eng=CPbkContactEngine::NewL();
		owns_engine=true;
	}
	
	read_db();
	
	chnot=eng->CreateContactChangeNotifierL(this);
}

void phonebook::read_db()
{
	CALLSTACKITEM(_L("phonebook::read_db"));

	read_data(eng->CreateContactIteratorLC(ETrue));
	CleanupStack::PopAndDestroy();
	User::QuickSort(contacts->Count(), contacts_key(*contacts), contacts_swap(*contacts));
	
	filter(previous_filter, true);
}

void phonebook::HandleDatabaseEventL(TContactDbObserverEvent /*aEvent*/)
{
	CALLSTACKITEM(_L("phonebook::HandleDatabaseEventL"));

	read_db();
}

void phonebook::PresenceChangedL(TInt ContactId, MPresenceData& Info)
{
	CALLSTACKITEM(_L("phonebook::PresenceChangedL"));

	if (obs) obs->before_change();
	
	TInt idx=(TInt)iContactToIndex->GetData(ContactId);
	if (!idx) return;
	contact* c=(*contacts)[idx-1];
	c->set_presence(&Info);
	TInt curr_idx=c->current_idx;
	if (!curr_idx) return;
	(*current_array)[curr_idx-1].Set(c->name->Des());
	
	if (obs) obs->contents_changed();
}

void phonebook::Notify(const TDesC & /*aMessage*/)
{
	CALLSTACKITEM(_L("phonebook::Notify"));

	// no impl
}

phonebook_observer * phonebook::get_observer()
{
	CALLSTACKITEM(_L("phonebook::get_observer"));

	return obs;
}

TInt phonebook::GetIndex(TInt ContactId)
{
	CALLSTACKITEM(_L("phonebook::GetIndex"));

	TInt idx=(TInt)iContactToIndex->GetData(ContactId);
	if (!idx) return KErrNone;
	contact* c=(*contacts)[idx-1];
	TInt curr_idx=c->current_idx;
	if (!curr_idx) return KErrNone;
	return curr_idx-1;
}

void phonebook::reset_contacts()
{
	CALLSTACKITEM(_L("phonebook::reset_contacts"));

	//iContactToIndex->Reset();
	
	for (int i=0; i<contacts->Count(); i++) {
		delete (*contacts)[i];
	}
	contacts->Reset();
}

void phonebook::read_data(CPbkContactIter * iter)
{
	CALLSTACKITEM(_L("phonebook::read_data"));

	if (contacts) {
		TInt c=contacts->Count();
		reset_contacts();
		contacts->SetReserveL(c+1);
	} else {
		contacts=new (ELeave) CArrayFixFlat<contact*>(50);
	}
	
	CPbkContactItem *item=0;
	TBuf<100> first_name;
	TBuf<100> last_name;
	TBuf<100> jabber_nick;
	
	for (iter->FirstL(); item=iter->CurrentL(); iter->NextL()) {
		
		TPbkContactItemField* f;
		
		f=item->FindField(EPbkFieldIdLastName);
		if (f) {
			last_name=f->Text();
		} else {
			last_name=_L("");
		}
		f=item->FindField(EPbkFieldIdFirstName);
		if (f) {
			first_name=f->Text();
		} else {
			first_name=_L("");
		}
		contact* c=contact::NewL(item->Id(), first_name, last_name);
		if (iJabberData.GetJabberNickL(item->Id(), jabber_nick) && jabber_nick.Length()>1) {
			c->has_nick=true;
		}
		c->set_presence(iPresenceHolder.GetPresence(item->Id()));
		contacts->AppendL(c);
	}
}

void phonebook::ReRead()
{
	CALLSTACKITEM(_L("phonebook::ReRead"));

	read_db();
}

MDesCArray* phonebook::get_array()
{
	CALLSTACKITEM(_L("phonebook::get_array"));

	return current_array;
}

TPtrC phonebook::get_phone_no(TInt index)
{
	CALLSTACKITEM(_L("phonebook::get_phone_no"));

	delete contactitem;
	
	TContactItemId id=(*current_ids)[index];
	if (id!=0) {
		contactitem=eng->ReadContactL(id);
		CPbkPhoneNumberSelect* sel=new (ELeave) CPbkPhoneNumberSelect;
		return sel->ExecuteLD(*contactitem, NULL, EFalse);
	}
	return TPtrC();
}

void phonebook::set_observer(phonebook_observer* i_obs)
{
	CALLSTACKITEM(_L("phonebook::set_observer"));

	obs=i_obs;
}

contact * phonebook::GetContact(TInt index)
{
	CALLSTACKITEM(_L("phonebook::GetContact"));

	TInt contactId = GetContactId(index);
	TInt idx=(TInt)iContactToIndex->GetData(contactId);
	if (!idx) return KErrNone;
	contact* c=(*contacts)[idx-1];;
	return c;
}

bool phonebook::filter(const TDesC& substr, bool force)
{
	CALLSTACKITEM(_L("phonebook::filter"));

	if (!force && ! previous_filter.Compare(substr)) return false;
	
	if (obs) obs->before_change();
	
	current_ids->Reset();
	current_array->Reset();
	
	RIncrMatcherBuf<30> matcher;
	matcher.SetMatchText(substr);
	
	bool match=false;
	contact* ci;
	TInt idx=0;
	for (int i=0; i<contacts->Count(); i++) {
		ci=(*contacts)[i];
		if (substr.Length()==0) match=true;
		else {
			if (matcher.IsMatchF( *(ci->first_name) ) || 
				matcher.IsMatchF( *(ci->last_name) ) ) match=true;
		}
		if (match) {
			current_array->AppendL(ci->name->Des() );
			current_ids->AppendL( ci->id);
			iContactToIndex->AddDataL(ci->id, (void*) (i+1), true);
			ci->current_idx=idx+1;
			idx++;
			match=false;
		} else {
			ci->current_idx=0;
			iContactToIndex->AddDataL(ci->id, (void*) (0), true);
		}
	}
	previous_filter=substr;
	
	if (obs) obs->contents_changed();
	return true;
}

CPbkContactEngine* phonebook::get_engine()
{
	CALLSTACKITEM(_L("phonebook::get_engine"));

	return eng;
}

CArrayFixFlat<contact*>* phonebook::GetContacts()
{
	CALLSTACKITEM(_L("phonebook::GetContacts"));

	return contacts;
}


// -------------------------------------------------------------

const TInt CPresenceUpdater::KTimeOut = 60;

CPresenceUpdater * CPresenceUpdater::NewL(phonebook& book)
{
	CALLSTACKITEM(_L("CPresenceUpdater::NewL"));

	auto_ptr<CPresenceUpdater> ret(new (ELeave) CPresenceUpdater(book));
	ret->ConstructL();
	return ret.release();
}

CPresenceUpdater::CPresenceUpdater(phonebook& book) : book(book), iWaiting(EFalse)
{
	CALLSTACKITEM(_L("CPresenceUpdater::CPresenceUpdater"));

}

CPresenceUpdater::~CPresenceUpdater()
{
	CALLSTACKITEM(_L("CPresenceUpdater::~CPresenceUpdater"));

	delete iWait;
}

void CPresenceUpdater::ConstructL()
{
	CALLSTACKITEM(_L("CPresenceUpdater::ConstructL"));

	
	iWait=CTimeOut::NewL(*this);
	
	Start();
}

void CPresenceUpdater::Start()
{
	CALLSTACKITEM(_L("CPresenceUpdater::Start"));

	Refresh();
	if (!iWaiting)
	{
		iWait->Wait(KTimeOut);
		iWaiting= ETrue;
	}
}

void CPresenceUpdater::Stop()
{
	CALLSTACKITEM(_L("CPresenceUpdater::Stop"));

	iWait->Reset();	
	iWaiting = EFalse;
}

void CPresenceUpdater::expired(CBase*)
{
	CALLSTACKITEM(_L("CPresenceUpdater::expired"));

	RDebug::Print(_L("refreshing..."));
	
	Refresh();
	
	if (!iWaiting)
	{
		
	}
	else
	{
		iWaiting = EFalse;
		Start();
	}
}

void CPresenceUpdater::Refresh()
{
	CALLSTACKITEM(_L("CPresenceUpdater::Refresh"));

	CArrayFixFlat<contact*> * contacts = book.GetContacts();
	
	for (int i=0; i< contacts->Count();i++)
	{
		if ( (*contacts)[i]->presence )
		{
			book.PresenceChangedL( (*contacts)[i]->id, *((*contacts)[i]->presence) );
		}
	}
}


