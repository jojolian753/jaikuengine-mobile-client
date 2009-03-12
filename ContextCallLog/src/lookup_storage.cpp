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

#include "lookup_storage.h"
#include "csd_lookup.h"
#include "contextcommon.h"
#include "raii_d32dbms.h"
#include <tpbkcontactitemfield.h>
#include <cpbkcontactitem.h>
#include "list.h"
#include <contextcalllog.rsg>
#include <eikenv.h>
#include "break.h"

class CLookupStorageImpl : public CLookupStorage, public MContextBase, public MDBStore, public MBBObserver {
private:
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) ;
	
	CLookupStorageImpl(MApp_context& Context, CDb& Db, CJabberData& aJabberData);
	~CLookupStorageImpl();
	void ConstructL();

	CPbkContactEngine* eng;
	bool owns_engine;

	virtual CPbkContactEngine* get_engine() { return eng; }
	virtual void ReRead() {
		NotifyContentsChanged(0, EFalse);
	}

	CList<phonebook_observer*> *iObservers;
	TInt iCurrentIndex;
	void GotoIndexAndGet(TInt aIndex) {
		while (aIndex < iCurrentIndex) {
			iTable.NextL();
			iCurrentIndex--;
		}
		while (aIndex > iCurrentIndex) {
			iTable.PreviousL();
			iCurrentIndex++;
		}
		iTable.GetL();
	}
	virtual TInt GetContactId(TInt Index) {
		CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("GetContactId"));
		TInt ret=-1;
		CC_TRAPD(err, ret=GetContactIdL(Index));
		return ret;
	}
	virtual TInt GetContactIdL(TInt Index) {
		GotoIndexAndGet(Index);
		return iJabberData.GetContactIdL(iTable.ColDes(ELooker));
	}

	virtual TInt GetIndex(TInt aContactId) {
		return -1;
	}
	virtual TTime GetAtTime(TInt aIndex) {
		GotoIndexAndGet(aIndex);
		return iTable.ColTime(EDatetime);
	}

	virtual const TDesC& PresenceSuffix() {
		return *iPresenceSuffix;
	}

	virtual CLogEvent * get_event(TInt aIndex) {
		return 0;
	}

	contact* iContact;
	CBBPresence*	iPresence;
	HBufC*	iPresenceSuffix;
	virtual contact * GetContact(TInt index) {
		contact* c=0;
		CC_TRAPD(err, c=GetContactL(index));
		return c;
	}

	virtual CPbkContactItem* ReadMinimalContactL(TInt id) {
		CPbkContactItem* item;
		item=eng->ReadMinimalContactLC(id);
		CleanupStack::Pop();
		return item;
	}
	virtual contact * GetContactL(TInt index) {
		CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("GetContactL"));

		TInt id=GetContactId(index);

		{
			CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("GetContactL1"));
			delete iContact->first_name; iContact->first_name=0;
			delete iContact->last_name; iContact->last_name=0;
			iContact->id=id;
			if (id==-1) {
				iContact->first_name=iTable.ColDes(ELooker).AllocL();
			} else {
				CPbkContactItem* item=0;
				CC_TRAPD(err, item=ReadMinimalContactL(id));

				if (item) {
					CleanupStack::PushL(item);
					TPbkContactItemField* f=0;
					
					f=item->FindField(EPbkFieldIdLastName);
					if (f) {
						iContact->last_name=f->Text().AllocL();
					}
					f=item->FindField(EPbkFieldIdFirstName);
					if (f) {
						iContact->first_name=f->Text().AllocL();
					}
					CleanupStack::PopAndDestroy();
				} else {
					iContact->first_name=iTable.ColDes(ELooker).AllocL();
				}

			}
		}
		{
			CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("GetContactL2"));
			TDateTime t=ContextToLocal(iTable.ColTime(EDatetime)).DateTime();
			TDateTime now; TTime nowt; nowt.HomeTime();
			now=nowt.DateTime();

			iContact->time.Zero();
			if (now.Month()==t.Month() && t.Day()==now.Day()) {
				iContact->time.AppendFormat(_L("%02d:%02d"),
					(TInt)t.Hour(), (TInt)t.Minute() );
			} else {
				iContact->time.AppendFormat(_L("%02d/%02d"),
					(TInt)t.Day()+1, (TInt)t.Month()+1
					);
			}

			{
				RADbColReadStream s; s.OpenLA(iTable, EContents);
				iPresence->InternalizeL(s);
			}

			if (! iTable.IsColNull(ETimeStamp) ) {
				iPresence->iSentTimeStamp()=iTable.ColTime(ETimeStamp);
			}
			iContact->presence=iPresence;
		}
		return iContact;
	}
	virtual TInt Count() {
		return iTable.CountL();
	}

	virtual void AddObserverL(phonebook_observer* i_obs) {
		if (!i_obs) return;
		iObservers->AppendL(i_obs);
	}
	virtual void RemoveObserverL(phonebook_observer* i_obs) {
		if (!i_obs) return;
		CList<phonebook_observer*>::Node *n=iObservers->iFirst;
		while (n) {
			if (n->Item == i_obs) {
				iObservers->DeleteNode(n, true);
				break;
			}
		}
	}
	virtual void NotifyContentsChanged(TInt ContactId, TBool aPresenceOnly) {
		CList<phonebook_observer*>::Node *n=iObservers->iFirst;
		while (n) {
			n->Item->contents_changed(ContactId, aPresenceOnly);
			n=n->Next;
		}
	}

	virtual void DeleteEvent(TInt index);
	
	virtual void ClearEventList();

	virtual TPtrC get_phone_no(TInt index) {
		return TPtrC(0, 0);
	}

	virtual void SetFilter(TFilter aFilter) {
	}

private:
	enum TColumns {
		EDatetime=1,
		ELooker,
		EContents,
		ETimeStamp
	};
	CBBSubSession * iBBSubSession;
	CDb&	iDb;
	CJabberData& iJabberData;

	friend CLookupStorage;
	friend auto_ptr<CLookupStorageImpl>;
};

CLookupStorage* CLookupStorage::NewL(MApp_context& Context, CDb& Db, CJabberData& aJabberData)
{
	//CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("NewL"));

	auto_ptr<CLookupStorageImpl> ret(new (ELeave) CLookupStorageImpl(Context, Db, aJabberData));
	ret->ConstructL();
	return ret.release();
}

_LIT(KClassName, "CLookupStorageImpl");

CLookupStorageImpl::CLookupStorageImpl(MApp_context& Context, CDb& Db, CJabberData& aJabberData) : 
		MContextBase(Context),
		MDBStore(Db.Db()), iDb(Db), iJabberData(aJabberData) { }

void CLookupStorageImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("ConstructL"));

	TInt cols[]= { 
                EDbColDateTime,	// timestamp
		EDbColText,	// looker
		EDbColLongBinary, // presence content
		EDbColDateTime,
		-1
	};
	TInt col_flags[]={ 0, 0, 0, 0 };
	TInt idxs[]= { EDatetime, -1 };
	
	_LIT(KLookups, "LOOKUPS");
	MDBStore::ConstructL(cols, idxs, false, KLookups, ETrue, col_flags);

	iPresenceSuffix=CEikonEnv::Static()->AllocReadResourceL(R_LOOKUP_SUFFIX);

	TTime delete_from; delete_from=GetTime(); delete_from-=TTimeIntervalDays(14);
	if (iTable.FirstL()) {
		for(;;) {
			iTable.GetL();
			if (iTable.ColTime(EDatetime) < delete_from) {
				DeleteL();
				iTable.NextL();
			} else {
				break;
			}
		}
	}
	iTable.LastL();

	eng=CPbkContactEngine::Static();
	if (eng) {
		owns_engine=false;
	} else {
		eng=CPbkContactEngine::NewL();
		owns_engine=true;
	}

	iObservers=CList<phonebook_observer*>::NewL();
	iContact=contact::NewL(0, KNullDesC, KNullDesC);
	iPresence=CBBPresence::NewL();

	iBBSubSession= BBSession()->CreateSubSessionL(this);
	iBBSubSession->AddNotificationL(KIncomingLookupTuple, ETrue);
}

CLookupStorageImpl::~CLookupStorageImpl()
{
	CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("~CLookupStorageImpl"));

	iBBSubSession->DeleteNotifications();
	delete iBBSubSession;
	delete iContact;
	delete iObservers;

	if (owns_engine) delete eng;

	delete iPresence;
	delete iPresenceSuffix;
}

void CLookupStorageImpl::NewValueL(TUint aId, const TTupleName& aName, const TDesC& , 
	const TComponentName& , const MBBData* aData)
{
	CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("NewValueL"));

	if (! ( aName == KIncomingLookupTuple ) ) return;

	const CBBLookup *lookup=bb_cast<CBBLookup>(aData);
	if (!lookup) return;

	{
		TAutomaticTransactionHolder ah(*this);

		iTable.InsertL();
		iTable.SetColL(EDatetime, lookup->iWhen());
		iTable.SetColL(ELooker, lookup->iLooker());
		iTable.SetColL(ETimeStamp, lookup->iPresenceTimeStamp());
		{
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			lookup->iPresence->ExternalizeL(w);
			w.CommitL();
		}
		
		PutL();
	}
        
	iBBSubSession->DeleteL(aId);
	iTable.LastL();
	iCurrentIndex=0;
	NotifyContentsChanged(0, EFalse);
}

void CLookupStorageImpl::DeletedL(const TTupleName& /*aName*/, const TDesC& )
{
	// no impl
}

void CLookupStorageImpl::DeleteEvent(TInt index)
{
	CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("DeleteEvent"));

	{
		while (index < iCurrentIndex) {
			iTable.NextL();
			iCurrentIndex--;
		}
		while (index > iCurrentIndex) {
			iTable.PreviousL();
			iCurrentIndex++;
		}

		TAutomaticTransactionHolder ah(*this);
		iTable.DeleteL();
	}

	iTable.LastL();
	iCurrentIndex=0;
	NotifyContentsChanged(0, EFalse);
}

void CLookupStorageImpl::ClearEventList()
{
	CALLSTACKITEM_N(_CL("CLookupStorageImpl"), _CL("ClearEventList"));

	{
		iDb.BeginL();
		TTransactionHolder ah(*this);

		if (iTable.FirstL()) {
			iTable.DeleteL();
			while (iTable.NextL()) {
				iTable.DeleteL();
			}
		}
		iDb.CommitL();
	}

	iTable.LastL();
	iCurrentIndex=0;
	NotifyContentsChanged(0, EFalse);
}

