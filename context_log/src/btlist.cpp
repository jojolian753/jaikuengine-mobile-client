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

#include "btlist.h"

#include "db.h"
#include "symbian_auto_ptr.h"
#include <bttypes.h>
#include "list.h"
#include <badesca.h>
#include "csd_bluetooth.h"
#include "cbbsession.h"

class COldBTDeviceList : public CBase, public MContextBase, public MDBStore {
public:
	COldBTDeviceList(RDbDatabase& Db) : MDBStore(Db) { }
	
	TBBBtDeviceInfo iInfo;
	void ConstructL(const TDesC& aTableName, const TTupleName& aTupleName) {
		TInt cols[] = { EDbColInt32, EDbColText, EDbColText8, -1 };
		TInt idx[] = { 1, -1 };

		MDBStore::ConstructL(cols, idx, false, aTableName);

		TBuf<50> name; TBuf8<10> addr;
		while ( iTable.NextL() ) {
			iTable.GetL();
			name=iTable.ColDes16(2);
			addr=iTable.ColDes8(3);
			if (addr.Length()==6) {
				iInfo.iMAC()=addr;
				TBTDevAddr a(addr);
				TBuf<20> readable;
				a.GetReadable(readable);
				iInfo.iNick=name;
				BBSession()->PutL(aTupleName, readable, &iInfo, Time::MaxTTime());
			}
			DeleteL();
		}
	}
};

class CBTDeviceListImpl : public CBTDeviceList, public MContextBase, public MBBObserver {
public:
	virtual ~CBTDeviceListImpl();
private:
	CBTDeviceListImpl(MApp_context& Context);
	void ConstructL(RDbDatabase* aDb, const TDesC& aTableName, const TTupleName& aTupleName);

	virtual MDesCArray* NameArray();
	virtual MDesC8Array* AddrArray();
	
	void AddObserver(MListObserver* aObserver);
	void RemoveObserver(MListObserver* aObserver);

	TInt AddDeviceL(const TDesC& Name, const TDesC8& Address);
	TInt AddDeviceToListsL(const TDesC& Name, const TDesC8& Address);
	void RemoveDeviceL(TInt Idx);
	void RemoveDeviceFromListsL(TInt Idx);
	void MoveUpL(TInt Idx);
	void MoveDownL(TInt Idx);

	TBool ContainsDevice(const TDesC8& Address);
	TInt FindByAddress(const TDesC8& Address);
	TInt  FindbyReadable(const TDesC& Readable);
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName);

	void ContentsChanged();

	friend class CBTDeviceList;

	CDesC16Array * iBTDevNames;
	CDesC16Array * iBTDevReadable;
	CDesC8Array *  iBTDevAddrs;
	CList<MListObserver*> *iObservers;
	TTupleName		iTupleName;
	CBBSubSession	*iSession;
	TBBBtDeviceInfo iInfo;
};

CBTDeviceList* CBTDeviceList::NewL(MApp_context& Context, const TTupleName& aTupleName,
	RDbDatabase* Db, const TDesC& aTableName)
{
	auto_ptr<CBTDeviceListImpl> ret(new (ELeave) CBTDeviceListImpl(Context));
	ret->ConstructL(Db, aTableName, aTupleName);
	return ret.release();
}

CBTDeviceListImpl::~CBTDeviceListImpl()
{
	delete iBTDevNames;
	delete iBTDevAddrs;
	delete iBTDevReadable;
	delete iObservers;
	delete iSession;
}

CBTDeviceListImpl::CBTDeviceListImpl(MApp_context& Context) : MContextBase(Context) { }

void CBTDeviceListImpl::ConstructL(RDbDatabase* Db, const TDesC& aTableName, const TTupleName& aTupleName)
{
	iBTDevNames = new (ELeave) CDesC16ArrayFlat(8);
	iBTDevAddrs = new (ELeave) CDesC8ArrayFlat(8);
	iBTDevReadable = new (ELeave) CDesC16ArrayFlat(8);

	if (Db) {
		auto_ptr<COldBTDeviceList> o( new (ELeave) COldBTDeviceList(*Db) );
		o->ConstructL(aTableName, aTupleName);
	}
	iObservers=CList<MListObserver*>::NewL();
	
	iSession=BBSession()->CreateSubSessionL(this);
	iTupleName=aTupleName;
	iSession->AddNotificationL(aTupleName, ETrue);
}


MDesCArray* CBTDeviceListImpl::NameArray()
{
	return iBTDevNames;
}

MDesC8Array* CBTDeviceListImpl::AddrArray()
{
	return iBTDevAddrs;
}

void CBTDeviceListImpl::AddObserver(MListObserver* aObserver)
{
	iObservers->AppendL(aObserver);
}

void CBTDeviceListImpl::RemoveObserver(MListObserver* aObserver)
{
	CList<MListObserver*>::Node *i, *prev, *tmp;
	i=iObservers->iFirst; prev=0;
	while (i) {
		if (i->Item==aObserver) {
			if (prev) {
				prev->Next=i->Next;
			} else {
				iObservers->iFirst=i->Next;
			}
			if (i==iObservers->iCurrent) iObservers->iCurrent=prev;
			tmp=i;
			i=i->Next;
			delete tmp;
		} else {
			prev=i;
			i=i->Next;
		}
	}
}

TInt CBTDeviceListImpl::AddDeviceL(const TDesC& Name, const TDesC8& Address)
{
	if (Address.Length()!=6) { return -1; }
	iInfo.iMAC()=Address;
	TBTDevAddr a(Address);
	TBuf<20> readable;
	a.GetReadable(readable);
	iInfo.iNick=Name;
	iSession->PutL(iTupleName, readable, &iInfo, Time::MaxTTime());
	return AddDeviceToListsL(Name, Address);
}

TInt CBTDeviceListImpl::AddDeviceToListsL(const TDesC& Name, const TDesC8& Address)
{
	if (ContainsDevice(Address)) return -1;
	if (Address.Length()!=6) { return -1; }

	iBTDevNames->AppendL(Name);
	iBTDevAddrs->AppendL(Address);
	TBTDevAddr a(Address);
	TBuf<20> readable;
	a.GetReadable(readable);
	iBTDevReadable->AppendL(readable);

	ContentsChanged();

	return iBTDevAddrs->Count()-1;
}

void CBTDeviceListImpl::RemoveDeviceL(TInt Idx) 
{
	const TDesC& addr=iBTDevReadable->MdcaPoint(Idx);
	iSession->DeleteL(iTupleName, addr, ETrue);
	RemoveDeviceFromListsL(Idx);
}
	
void CBTDeviceListImpl::RemoveDeviceFromListsL(TInt Idx)
{
	iBTDevNames->Delete(Idx);
	iBTDevNames->Compress();
	iBTDevAddrs->Delete(Idx);
	iBTDevAddrs->Compress();
	iBTDevReadable->Delete(Idx);
	iBTDevReadable->Compress();
	ContentsChanged();
}

void CBTDeviceListImpl::MoveUpL(TInt Idx)
{
	return;
}

void CBTDeviceListImpl::MoveDownL(TInt Idx)
{
	return;
}

void CBTDeviceListImpl::NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
	const TComponentName& aComponentName, const MBBData* aData)
{
	const TBBBtDeviceInfo* info=bb_cast<TBBBtDeviceInfo>(aData);
	if (!aData) return;
	AddDeviceToListsL(info->iNick(), info->iMAC());
}

void CBTDeviceListImpl::DeletedL(const TTupleName& aName, const TDesC& aSubName)
{
	TInt idx=FindbyReadable(aSubName);
	if (idx==KErrNotFound) return;
	RemoveDeviceFromListsL(idx);
}

TBool CBTDeviceListImpl::ContainsDevice(const TDesC8& Address)
{
	TInt idx=FindByAddress(Address);
	return (idx!=KErrNotFound);
}

TInt CBTDeviceListImpl::FindByAddress(const TDesC8& Address)
{
	for (int i=0; i<iBTDevAddrs->Count(); i++) {
		if ( ! ((*iBTDevAddrs)[i].Compare(Address)) ) {
			return i;
		}
	}
	return KErrNotFound;
}

void CBTDeviceListImpl::ContentsChanged()
{
	CList<MListObserver*>::Node *i;
	i=iObservers->iFirst;
	while (i) {
		i->Item->ContentsChanged(this);
		i=i->Next;
	}
}

TInt  CBTDeviceListImpl::FindbyReadable(const TDesC& Readable)
{
	for (int i=0; i<iBTDevReadable->Count(); i++) {
		if ( ! ((*iBTDevReadable)[i].Compare(Readable)) ) {
			return i;
		}
	}
	return KErrNotFound;
}
