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
#include "subscriptions.h"
#include <s32btree.h>
#include "symbian_auto_ptr.h"
#include "bbtypes.h"

//FIXME: templatize the Tuple/Component functions since they duplicate stuff

class CSubscriptionsImpl : public CSubscriptions {
private:
	CSubscriptionsImpl();
	void ConstructL();
	virtual ~CSubscriptionsImpl();

	virtual MBlackBoardObserver* FirstL(const TTupleName& aTupleName, TBBPriority& aPriorityInto);
	virtual MBlackBoardObserver* FirstL(const TComponentName& aComponentName, TBBPriority& aPriorityInto);

	MBlackBoardObserver* NextL(TBBPriority& aPriorityInto);

	virtual TTupleName FirstSubscriptionTupleL();
	virtual TTupleName NextSubscriptionTupleL();
	virtual TComponentName FirstSubscriptionComponentL();
	virtual TComponentName NextSubscriptionComponentL();

	virtual void AddNotificationL(MBlackBoardObserver *aSession, const TTupleName& aTupleName, 
		TBBPriority aPriority);
	virtual void DeleteNotificationL(MBlackBoardObserver *aSession, const TTupleName& aTupleName);
	virtual void AddNotificationL(MBlackBoardObserver *aSession, const TComponentName& aComponentName, 
		TBBPriority aPriority);
	virtual void DeleteNotificationL(MBlackBoardObserver *aSession, 
		const TComponentName& aComponentName);

	virtual void DeleteAllSubscriptionsL(MBlackBoardObserver *aSession);
	virtual void DeleteAllSubscriptionsL();

	friend class CSubscriptions;
	friend class auto_ptr<CSubscriptionsImpl>;

	struct TTupleNameAndObs {
		TTupleName		iName;
		MBlackBoardObserver*	iObserver;
	};

	struct TTupleEntry {
		TTupleNameAndObs	iNameAndObs;
		TBBPriority		iPriority;
	};

	struct TComponentNameAndObs {
		TComponentName		iName;
		MBlackBoardObserver*	iObserver;
	};
	struct TComponentEntry {
		TComponentNameAndObs	iNameAndObs;
		TBBPriority		iPriority;
	};


	// relies on TTupleName and TComponentName having the same structure
	class TTupleKey : public MBtreeKey {
	public:
		virtual void Between(const TAny* aLeft,const TAny* aRight, TBtreePivot& aPivot) const;
		virtual TInt Compare(const TAny* aLeft,const TAny* aRight) const;
		virtual const TAny* Key(const TAny* anEntry) const;
	};


	TBtreeFix<TTupleEntry, TTupleNameAndObs> iTupleTree; bool iTupleTreeIsOpen;
	TBtreeFix<TComponentEntry, TComponentNameAndObs> iComponentTree; bool iComponentTreeIsOpen;
	TTupleKey iTupleKey;
	CMemPagePool  *iTuplePool, *iComponentPool;

	TBtreePos	iCurrentPos;
	TTupleNameAndObs	iCurrentSearchTuple, iPartialSearchTuple;
	TComponentNameAndObs	iCurrentSearchComponent, iPartialSearchComponent;
	TBool		iFound;
	enum TCurrentSearch { ETuple, EComponent };
	TCurrentSearch	iCurrentSearchMode;

};

CSubscriptions* CSubscriptions::NewL()
{
	CALLSTACKITEM_N(_CL("CSubscriptions"), _CL("NewL"));

	auto_ptr<CSubscriptionsImpl> ret(new (ELeave) CSubscriptionsImpl);
	ret->ConstructL();
	return ret.release();
}

CSubscriptions::~CSubscriptions()
{
	CALLSTACKITEM_N(_CL("CSubscriptions"), _CL("~CSubscriptions"));

}

CSubscriptionsImpl::CSubscriptionsImpl() : iTupleTree(EBtreeFast), iComponentTree(EBtreeFast)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("CSubscriptionsImpl"));

}

void CSubscriptionsImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("ConstructL"));

	iTuplePool=CMemPagePool::NewL();
	iTupleTree.Connect(iTuplePool, &iTupleKey);
	iTupleTreeIsOpen=true;

	iComponentPool=CMemPagePool::NewL();
	iComponentTree.Connect(iComponentPool, &iTupleKey);
	iComponentTreeIsOpen=true;
}

CSubscriptionsImpl::~CSubscriptionsImpl()
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("~CSubscriptionsImpl"));

	TInt err;
	if (iTupleTreeIsOpen) {
		CC_TRAP(err, iTupleTree.ClearL());
	}
	delete iTuplePool;

	if (iComponentTreeIsOpen) {
		CC_TRAP(err, iComponentTree.ClearL());
	}
	delete iComponentPool;
}

MBlackBoardObserver* CSubscriptionsImpl::FirstL(const TTupleName& aTupleName, TBBPriority& aPriorityInto)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("FirstL"));

	if (aTupleName.iModule.iUid==KBBAnyUidValue || aTupleName.iId==KBBAnyId) 
		User::Leave(KErrArgument);

	iCurrentSearchMode=ETuple;
	iCurrentSearchTuple.iName=aTupleName;
	iCurrentSearchTuple.iObserver=0;
	iPartialSearchTuple.iName=KAnyTuple;
	iPartialSearchTuple.iObserver=0;
	iFound=EFalse;
	return NextL(aPriorityInto);
}

TTupleName CSubscriptionsImpl::FirstSubscriptionTupleL()
{
	if ( ! iTupleTree.FirstL(iCurrentPos) ) return KNoTuple;
	TTupleEntry e;
	iTupleTree.ExtractAtL(iCurrentPos, e);
	return e.iNameAndObs.iName;
}

TTupleName CSubscriptionsImpl::NextSubscriptionTupleL()
{
	if ( ! iTupleTree.NextL(iCurrentPos) ) return KNoTuple;
	TTupleEntry e;
	iTupleTree.ExtractAtL(iCurrentPos, e);
	return e.iNameAndObs.iName;
}

TComponentName CSubscriptionsImpl::FirstSubscriptionComponentL()
{
	if ( ! iComponentTree.FirstL(iCurrentPos) ) return KNoComponent;
	TComponentEntry e;
	iComponentTree.ExtractAtL(iCurrentPos, e);
	return e.iNameAndObs.iName;
}

TComponentName CSubscriptionsImpl::NextSubscriptionComponentL()
{
	if ( ! iComponentTree.NextL(iCurrentPos) ) return KNoComponent;
	TComponentEntry e;
	iComponentTree.ExtractAtL(iCurrentPos, e);
	return e.iNameAndObs.iName;
}


MBlackBoardObserver* CSubscriptionsImpl::FirstL(const TComponentName& aComponentName, TBBPriority& aPriorityInto)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("FirstL"));

	if (aComponentName.iModule.iUid==KBBAnyUidValue || aComponentName.iId==KBBAnyId) 
		User::Leave(KErrArgument);

	iCurrentSearchMode=EComponent;
	iCurrentSearchComponent.iName=aComponentName;
	iCurrentSearchComponent.iObserver=0;
	iPartialSearchComponent.iName=KAnyComponent;
	iPartialSearchComponent.iObserver=0;
	iFound=EFalse;
	return NextL(aPriorityInto);
}

template<typename Key>
TBool IncrementPartial(Key& iPartial, Key& iFull) {
	if (iPartial.iModule.iUid == KBBAnyUidValue) {
		iPartial.iModule.iUid=iFull.iModule.iUid;
		return ETrue;
	}
	if (iPartial.iId == KBBAnyId ) {
		iPartial.iId=iFull.iId;
		return ETrue;
	}
	return EFalse;
}

#pragma warning(disable: 4127)
#pragma warning(disable: 4706)

template<typename Entry, typename Tree, typename Key>
MBlackBoardObserver* NextL(Entry& e, TBool& iFound, Tree& iTree, TBtreePos& iCurrentPos, 
			   Key& iPartialSearch, Key& iFullSearch, TBBPriority& aPriorityInto)
{
	CALLSTACKITEM_N(_CL(""), _CL("NextL"));

	while (true) {
		if (!iFound) {
			if ( (iFound=iTree.FindL(iCurrentPos, iPartialSearch, TBtree::EGreaterEqual)) ) {
				iTree.ExtractAtL(iCurrentPos, e);
				if (e.iNameAndObs.iName == iPartialSearch.iName) {
					aPriorityInto=e.iPriority;
					return e.iNameAndObs.iObserver;
				} else {
					iFound=EFalse;
				}
			}
		}
		if (iFound) {
			iFound=EFalse;
			if (iTree.NextL(iCurrentPos)) {
				iTree.ExtractAtL(iCurrentPos, e);
				if (e.iNameAndObs.iName==iPartialSearch.iName) {
					iFound=ETrue;
					aPriorityInto=e.iPriority;
					return e.iNameAndObs.iObserver;
				}
			} 
		}
		if (!IncrementPartial(iPartialSearch.iName, iFullSearch.iName)) return 0;

	}
}

MBlackBoardObserver* CSubscriptionsImpl::NextL(TBBPriority& aPriorityInto)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("NextL"));

	if (iCurrentSearchMode==ETuple) {
		TTupleEntry e;
		return ::NextL(e, iFound, iTupleTree, iCurrentPos, iPartialSearchTuple, iCurrentSearchTuple,
			aPriorityInto);
		
	} else {
		TComponentEntry e;
		return ::NextL(e, iFound, iComponentTree, iCurrentPos, iPartialSearchComponent, iCurrentSearchComponent,
			aPriorityInto);
	}
}

template<typename Tree, typename Key, typename Entry>
void AddNotificationL(MBlackBoardObserver *aSession, Tree& iTree, const Key& aKey, TBBPriority aPriority, Entry& e)
{
	CALLSTACKITEM_N(_CL(""), _CL("AddNotificationL"));

	if (!aSession) User::Leave(KErrArgument);

	//FIXME: use priority
	TBtreePos pos;
	e.iNameAndObs.iName=aKey; e.iNameAndObs.iObserver=aSession; e.iPriority=aPriority;

	if (!iTree.InsertL(pos, e, ENoDuplicates)) 
		User::Leave(KErrAlreadyExists);
}
	
void CSubscriptionsImpl::AddNotificationL(MBlackBoardObserver *aSession, const TTupleName& aTupleName, 
	TBBPriority aPriority)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("AddNotificationL"));

	TTupleEntry e;
	::AddNotificationL(aSession, iTupleTree, aTupleName, aPriority, e);

}

template<typename Tree, typename Key, typename Entry>
void DeleteNotificationL(MBlackBoardObserver *aSession, Tree& iTree, const Key& aName, Entry& e)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("AddNotificationL"));

	TBool found;
	TBtreePos pos;
	if (iTree.FindL(pos, aName) ) {
		iTree.DeleteAtL(pos);
		return;
	}
	User::Leave(KErrNotFound);
}

void CSubscriptionsImpl::DeleteNotificationL(MBlackBoardObserver *aSession, const TTupleName& aTupleName)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("DeleteNotificationL"));

	TTupleEntry e;
	TTupleNameAndObs no;
	no.iName=aTupleName; no.iObserver=aSession;
	::DeleteNotificationL(aSession, iTupleTree, no, e);
}

void CSubscriptionsImpl::AddNotificationL(MBlackBoardObserver *aSession, const TComponentName& aComponentName, 
	TBBPriority aPriority)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("AddNotificationL"));

	TComponentEntry e;
	::AddNotificationL(aSession, iComponentTree, aComponentName, aPriority, e);
}

void CSubscriptionsImpl::DeleteNotificationL(MBlackBoardObserver *aSession, 
	const TComponentName& aComponentName)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("DeleteNotificationL"));

	TComponentEntry e;
	TComponentNameAndObs no;
	no.iName=aComponentName; no.iObserver=aSession;
	::DeleteNotificationL(aSession, iComponentTree, no, e);
}

template<typename Tree, typename Entry>
void DeleteAllSubscriptionsL(MBlackBoardObserver *aSession, Tree& iTree, Entry& e)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("DeleteNotificationL"));

	TBtreePos i, prev;
	TBool more;
	more=iTree.FirstL(i);
	while (more) {
		iTree.ExtractAtL(i, e);
		if (e.iNameAndObs.iObserver==aSession) {
			iTree.DeleteL(e.iNameAndObs);
			more=iTree.FindL(i, e.iNameAndObs, TBtree::EGreaterEqual);
		} else {
			more=iTree.NextL(i);
		}
	}
}

void CSubscriptionsImpl::DeleteAllSubscriptionsL(MBlackBoardObserver *aSession)
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("DeleteAllSubscriptionsL"));

	{
		TTupleEntry e;
		::DeleteAllSubscriptionsL(aSession, iTupleTree, e);
	}

	{
		TComponentEntry e;
		::DeleteAllSubscriptionsL(aSession, iComponentTree, e);
	}
}

void CSubscriptionsImpl::DeleteAllSubscriptionsL()
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("DeleteAllSubscriptionsL"));

	CC_TRAPD(err1, iTupleTree.ClearL());
	CC_TRAPD(err2, iComponentTree.ClearL());
	User::LeaveIfError(err1);
	User::LeaveIfError(err2);
}

void CSubscriptionsImpl::TTupleKey::Between(const TAny* aLeft,const TAny* aRight, TBtreePivot& aPivot) const
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("TTupleKey"));

	TTupleNameAndObs left=*(TTupleNameAndObs*)aLeft;
	TTupleNameAndObs right=*(TTupleNameAndObs*)aRight;

	TTupleNameAndObs mid;
	if (left.iName.iModule.iUid == right.iName.iModule.iUid) {
		mid.iName.iModule.iUid=left.iName.iModule.iUid;
		if (left.iName.iId == right.iName.iId) {
			mid.iName.iId=left.iName.iId;
			mid.iObserver=left.iObserver;
		} else {
			mid.iName.iId=(left.iName.iId+right.iName.iId)/2;
			mid.iObserver=(MBlackBoardObserver*)( 
				(char*)left.iObserver + 1);
		}
	} else {
		if (left.iName.iModule.iUid == (right.iName.iModule.iUid-1)) {
			if (right.iName.iId>1) {
				mid.iName.iModule.iUid=right.iName.iModule.iUid;
				mid.iName.iId=0;
			} else {
				mid.iName.iModule.iUid=left.iName.iModule.iUid;
				mid.iName.iId=left.iName.iId+1;
			}
		} else {
			mid.iName.iModule.iUid=left.iName.iModule.iUid+(right.iName.iModule.iUid-left.iName.iModule.iUid)/2;
			mid.iName.iId=0;
		}
		mid.iObserver=0;
	}
	
	aPivot.Copy((TUint8*)&mid, sizeof(mid));
}

TInt CSubscriptionsImpl::TTupleKey::Compare(const TAny* aLeft,const TAny* aRight) const
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("TTupleKey"));

	TTupleNameAndObs left=*(TTupleNameAndObs*)aLeft;
	TTupleNameAndObs right=*(TTupleNameAndObs*)aRight;

	if (left.iName.iModule.iUid < right.iName.iModule.iUid) return -1;
	if (left.iName.iModule.iUid > right.iName.iModule.iUid) return 1;
	if (left.iName.iId < right.iName.iId) return -1;
	if (left.iName.iId > right.iName.iId) return 1;
	if (left.iObserver < right.iObserver) return -1;
	if (left.iObserver > right.iObserver) return 1;
	return 0;
}

const TAny* CSubscriptionsImpl::TTupleKey::Key(const TAny* anEntry) const
{
	CALLSTACKITEM_N(_CL("CSubscriptionsImpl"), _CL("TTupleKey"));

	TTupleEntry* e=(TTupleEntry*)anEntry;
	return (TAny*)&(e->iNameAndObs);
}
