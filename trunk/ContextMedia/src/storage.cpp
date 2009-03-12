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
#include "cm_storage.h"
#include "symbian_refcounted_ptr.h"
#include <eikenv.h> 
#include <gulicon.h>
#include "raii_d32dbms.h"
#include "symbian_tree.h"
#include <contextmedia.mbg>
#include <bautils.h>
#include "transfer2.h"
#include <contextmedia.rsg>
#include "cl_settings.h"
#include "reporting.h"
#include <charconv.h>
#include "bberrorinfo.h"
#include "cu_common.h"
#include "cm_network.h"

#define DEBUG_STORAGE
#ifdef DEBUG_STORAGE
#include "callstack.h"
#endif

/*
 * The NotifyPostEvent(...) functions may call SwitchIndex or move around in the
 * table, either directly or by the functions they call. The manipulation functions
 * that call NotifyPostEvent() must take care that they return to where they
 * were in the table after calling.
 */

class CPostStorageImpl: public CPostStorage, 
	public MContextBase, public MDBStore, public MPostDeletionNotify {
	enum TColumns {
		EDbId = 1,
		EParentId,
		EPostId,
		EDatetime,
		ELastAccess,
		EAuthor,
		EUnread,
		EIcon, 
		EContents,
		ETitle,
		EHidden,
		ELastPostAuthor,
		ELastPostDate,
		EErrorInfo,
		EUpdated
	};
	enum TIndices {
		EIdxDbId = 0,
		EIdxParentDatetime ,
		EIdxParentAuthor,
		EIdxPostId,
		EIdxParentUnread,
		EIdxParentTitle,
		EIdxParentPostId,
		EIdxParentLastPost,
		EIdxParentUpdated,
		EIdxDatetime
	};

	virtual CArrayPtr<CGulIcon>* IconArray();

	// adding
	virtual void AddLocalL(CCMPost* aPost, auto_ptr<CFbsBitmap>& aIcon);
	virtual void DoAddLocalL(CCMPost* aPost, auto_ptr<CFbsBitmap>& aIcon, TBool aReplace);
	virtual void MarkAsRead(CCMPost* aPost);
	virtual void RemovePostL(const CCMPost * aPost, TBool aMediaRemoveFile=EFalse);
	virtual void RemovePostsL(TInt64 aThreadId, TTime aFromTime,
		TFromType aFromType);
	virtual void MovePostsL(TInt64 aFromThreadId, TInt64 aToThreadId,
		TTime aFromTime,
		TFromType aFromType);
	virtual void DoRemovePostL(const CCMPost * aPost, TBool aMediaRemoveFile=ETrue);
	void DoMarkAsReadL(TUint db_id);
	void UpdateReadCounters(TInt64 from_id, TInt read_counter);
	void UpdateLastPost(TInt64 from_id, const TDesC& aAuthorName, const TTime& aDateTime);
	void RereadUnreadL();

	virtual void AddPlaceHolderL(TInt64 aThreadId);
	virtual void AddNewThreadPlaceHolderL(TInt64 aThreadId);
	virtual void AddErrorPlaceHolderL(TInt64 aThreadId);
	virtual TBool IsPlaceHolderL(TInt64 aThreadId);

	virtual void UpdatePostL(CCMPost * aPost);
	virtual void UpdateFileName(TInt64 aPostId, const TDesC& aFileName, const TDesC& aContentType);
	virtual void UpdateIcon(TInt64 aPostId, auto_ptr<CFbsBitmap>& aIcon);
	virtual void UpdateError(TInt64 aPostId, TInt aErrorCode, const TDesC& aError);
	virtual void UpdateErrorInfo(TInt64 aPostId, const CBBErrorInfo* aErrorInfo);

	virtual void CommitL();

	virtual void SetThreadVisibleL(TInt64 aPostId, TBool visible);
	virtual void SetAllThreadsVisibleL();
	virtual TInt HasHiddenThreads();

	TBool IsFirstPostInThread(TInt64 aThreadId, TInt64 aPostId);

	// return parent id
	TInt64 DoUpdateFileName(TInt64 aPostId, const TDesC& aFileName, const TDesC& aContentType);
	TInt64 DoUpdateIcon(TInt64 aPostId, auto_ptr<CFbsBitmap>& aIcon);

	// iteration
	virtual TBool FirstL(const TInt64& aParentId, TSortBy aSortBy, 
		TOrder aSortOrder, TBool aOnlyUnread, TBool aOnlyVisible=ETrue);
	virtual TBool FirstAllL();
	virtual TBool NextL();
	virtual TUint GetCurrentIndexL();
	virtual TInt64 GetCurrentIdL();

	// accessing
	virtual CCMPost* GetCurrentL(MPostNotify* aNotify); // addrefs, sets up for notification
	virtual CCMPost* GetByIndexL(MPostNotify* aNotify, TUint aIdx); // addrefs, sets up for notification
	virtual CCMPost* GetByPostIdL(MPostNotify* aNotify, TInt64 aPostId); // addrefs, sets up for notification
	virtual void SubscribeL(CCMPost* aPost, MPostNotify* aNotify); // addrefs, sets up for notification
	virtual void Release(CCMPost* aPost, MPostNotify* aNotify); // releases
	virtual CCMPost* GetRootL(MPostNotify* aNotify); // sets up for notification 

	virtual TInt64 GetParentId(TInt64 aPostId);
	virtual TInt64 GetLastPostId(TInt64 aParentId);

	CPostStorageImpl(MApp_context& Context, CDb& Db, MBBDataFactory* aBBFactory);
	void ConstructL();
	~CPostStorageImpl();

	void DoCancel() { }
	void CheckedRunL();
	void Async();

#ifdef DEBUG_STORAGE
	void PrintDebugObserver(MPostNotify* aNotify);
	void PrintDebugRelease(MPostNotify* aNotify);
	void PrintDebugUpdate(CCMPost* aPost);
#else
	void PrintDebugObserver(MPostNotify* ) { }
	void PrintDebugRelease(MPostNotify* ) { }
	void PrintDebugUpdate(CCMPost* ) { }
#endif
	friend class CPostStorage;
	friend class auto_ptr<CPostStorageImpl>;

	TBool SeekToPostIdL(const TInt64& aPostId);
	TBool SeekToDbIdL(TUint aDbId);

	TInt CountHiddenThreadsL();

	void PostDeleted(const class CCMPost* aPost);

	CDb&	iDb;
	CArrayPtr<CGulIcon>* iIconArray;
	TInt64 iSeekParentId; TOrder iSeekOrder; TBool iSeekOnlyUnread; TBool iSeekOnlyVisible;
	TSortBy iSeekBy;
	CEikonEnv* iEikEnv;
	TBool iFound;

	TInt iNbHiddenThreads;

	// notification support
	CGenericIntMap*		iDbIdToObjectMap;

	struct TObservation {
		CCMPost*	iPost;
		MPostNotify*	iNotify;
	};
	class TObsKey : public MBtreeKey {
	public:
		virtual void Between(const TAny* aLeft,const TAny* aRight, TBtreePivot& aPivot) const;
		virtual TInt Compare(const TAny* aLeft,const TAny* aRight) const;
		virtual const TAny* Key(const TAny* anEntry) const;
	};
	TObsKey		iObsKey;
	TBtreeFix<TObservation, CCMPost*> iObsTree; bool iObsIsOpen;
	CMemPagePool  *iObsPool;
	CCMPost		*iRoot;
	CCMPost		*iMediaPool;
	TInt		iNextFreeIconSlot;

	CList<MPostNotify*>	*iToNotify;
	void NotifyObservers(CCMPost* aPost, MPostNotify::TEvent aEvent);
	void NotifyObservers(CCMPost* aPost, CCMPost* aPost2, MPostNotify::TEvent aEvent);
	void NotifyObserversL(CCMPost* aPost, CCMPost* aPost2, MPostNotify::TEvent aEvent);
	static void ReleasePost(void* data);

	MBBDataFactory*		iBBFactory;

	TBuf<50>		iPlaceholderAuthor, iPlaceholderTitle, iNewThreadTitle, iErrorTitle;

	TInt iNbDefaultIcons;
	TInt iResource;
};

EXPORT_C TInt64 CPostStorage::RootId() {
	return TInt64(0);
}

EXPORT_C TInt64 CPostStorage::MediaPoolId() {
	return TInt64(1); 
}

EXPORT_C TInt64 CPostStorage::AddMeToMediaPoolId() {
	return TInt64(2);
}

EXPORT_C CPostStorage* CPostStorage::NewL(MApp_context& Context, CDb& Db, MBBDataFactory* aBBFactory)
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("NewL"));

	auto_ptr<CPostStorageImpl> ret(new (ELeave) CPostStorageImpl(Context, Db, aBBFactory));
	ret->ConstructL();
	return ret.release();
}

void CPostStorageImpl::ReleasePost(void* data)
{
	CCMPost* p=(CCMPost*)data;
	p->SetDeletionNotify(0);
	p->SetThumbNailIndex(KNoIconIndex);
}

CPostStorageImpl::~CPostStorageImpl()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("~CPostStorageImpl"));

	Cancel();

	int i=0;
	if (iIconArray) {
		while (iNextFreeIconSlot!=-1) {
			TInt temp=iNextFreeIconSlot;
			iNextFreeIconSlot=(TInt)iIconArray->At(temp);
			iIconArray->At(temp)=0;
		}
		for(i=0;i<iIconArray->Count();i++) {
			CGulIcon * ic = iIconArray->At(i);
			delete ic;
			iIconArray->Delete(i);
		}
	}
	if (iIconArray) iIconArray->ResetAndDestroy();
	delete iIconArray;

	if (iObsIsOpen) { CC_TRAPD(err, iObsTree.ClearL()); }
	delete iObsPool;

	if (iDbIdToObjectMap) {
		iDbIdToObjectMap->SetDeletor(ReleasePost);
	}
	delete iDbIdToObjectMap;
	if (iMediaPool) iMediaPool->Release();
	if (iRoot) 
		iRoot->Release();
	if (iResource) iEikEnv->DeleteResourceFile(iResource);
}

_LIT(KThread, "thread");

void CPostStorageImpl::AddLocalL(CCMPost* aPost, auto_ptr<CFbsBitmap>& aIcon)
{
	PrintDebugUpdate(aPost);

	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("AddLocalL"));

	{
		SwitchIndexL(6);
		TDbSeekMultiKey<2> k;
		k.Add(aPost->iParentId());
		k.Add(aPost->iPostId());
		if (iTable.SeekL(k)) {
			User::Leave(KErrAlreadyExists);
		}

	}
	if (! iDb.Db().InTransaction() ) BeginL();
	TTransactionHolder th(*this);

	if ( aPost->iPostId()==MediaPoolId() ) {
		if (iMediaPool) {
			iMediaPool->Release();
			iMediaPool=0;
		}
	}
	if ( (aPost->iParentId() != RootId()) && (aPost->iParentId() != MediaPoolId()) ) {
		SwitchIndexL(3);
		TDbSeekKey k(aPost->iParentId());
		TBool found=EFalse;
		found=iTable.SeekL(k);
		if (! found ) {
			User::Leave(KErrArgument);
		}
		iTable.GetL();
		if (iTable.IsColNull(EContents)) {
			// placeholder
			TUint dbid=iTable.ColUint32(EDbId);
			CCMPost* existing=(CCMPost*)iDbIdToObjectMap->GetData(dbid);
			refcounted_ptr<CCMPost> parent(0);

			if (!existing) {
				parent.reset(CCMPost::NewL(iBBFactory));
				parent->SetDeletionNotify(this);
				iDbIdToObjectMap->AddDataL(iTable.ColUint32(EDbId), parent.get());
			} else {
				existing->AddRef();
				parent.reset(existing);
			}
			*parent=*aPost;
			parent->iParentId()=RootId();
			parent->iUnreadCounter()=0;
			parent->iPostId()=aPost->iParentId();

			auto_ptr<CFbsBitmap> icon2(0);
			if (aIcon.get()) {
				icon2.reset(new (ELeave) CFbsBitmap);
				icon2->Duplicate(aIcon->Handle());
			}

			DoAddLocalL(parent.get(), icon2, ETrue);
		}
	}

	DoAddLocalL(aPost, aIcon, EFalse);

	Async();
}

void CPostStorageImpl::DoAddLocalL(CCMPost* aPost, auto_ptr<CFbsBitmap>& aIcon, TBool aReplace)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("DoAddLocalL"));

	if (!aPost) 
		User::Leave(KErrArgument);
	if (aPost->iPostId() == RootId()) 
		User::Leave(KErrArgument);
	/*if (aPost->iPostId() == MediaPoolId()) 
		User::Leave(KErrArgument);*/
		
	if (aPost->iPostId() == AddMeToMediaPoolId()) {
		TInt64 id = GetLastPostId(MediaPoolId());
		aPost->iPostId() = --id;
	}

	if (!aReplace) {
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Insert"));
		InsertL();
	} else {
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Update"));
		iTable.UpdateL();
	}

	{
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("SetCol1"));

		iTable.SetColL(EParentId, aPost->iParentId());
		iTable.SetColL(EPostId, aPost->iPostId());
		iTable.SetColL(EDatetime, aPost->iTimeStamp());
		iTable.SetColL(EUpdated, aPost->iTimeStamp());
		iTable.SetColL(EHidden, EFalse);

		TTime t; t=GetTime();
		iTable.SetColL(ELastAccess, t);

		iTable.SetColL(EAuthor, aPost->iSender.iName());
		iTable.SetColL(EUnread, aPost->iUnreadCounter());
		{
			CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Contents"));
			iTable.SetColNullL(EContents);
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			aPost->ExternalizeL(w);
			w.CommitL();
		}
	}

	{
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("SetCol2"));
		if (aPost->iBodyText->iPtr.Length() > 50) {
			iTable.SetColL(ETitle, aPost->iBodyText->iPtr.Left(50));
		} else {
			iTable.SetColL(ETitle, aPost->iBodyText->iPtr);
		}
	}
	if (aIcon.get()) {
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Icon"));
		RADbColWriteStream w; w.OpenLA(iTable, EIcon);
		aIcon->ExternalizeL(w);
		w.CommitL();
		auto_ptr<CGulIcon> icon(CGulIcon::NewL(aIcon.get()));
		aIcon.release();
		TInt idx;
		if (iNextFreeIconSlot==-1) {
			iIconArray->AppendL(icon.get());
			idx=iIconArray->Count()-1;
		} else {
			TInt temp=(TInt)iIconArray->At(iNextFreeIconSlot);
			iIconArray->At(iNextFreeIconSlot)=icon.get();
			idx=iNextFreeIconSlot;
			iNextFreeIconSlot=temp;
		}
		icon.release();
		aPost->SetThumbNailIndex(idx);
	} else {
		aPost->SetThumbNailIndex(KUnknownIconIndex);
	}

	{
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Put"));
		TUint newid=aPost->iLocalDatabaseId()=iTable.ColUint(EDbId);
		PutL();

		if (!aReplace) {
			iDbIdToObjectMap->AddDataL(newid, aPost);
			aPost->SetDeletionNotify(this);
		}

		aPost->AddRef();
		refcounted_ptr<CCMPost> p(aPost);
	}

	if (!aReplace) {
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Notify1"));
		NotifyObservers(aPost, MPostNotify::EChildAdded);
	} else {
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Notify2"));
		NotifyObservers(0, aPost, MPostNotify::EPlaceholderFilled);
	}

	{
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdateRead"));
		UpdateReadCounters(aPost->iParentId(), aPost->iUnreadCounter());
	}
	{
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdateLastPost"));
		UpdateLastPost(aPost->iParentId(), aPost->iSender.iName(), aPost->iTimeStamp());
	}
}

void CPostStorageImpl::RemovePostL(const CCMPost * aPost, TBool aMediaRemoveFile)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostL"));

	if (! iDb.Db().InTransaction()) iDb.BeginL();
	TTransactionHolder th(*this);

	DoRemovePostL(aPost, aMediaRemoveFile);

	Async();
}

void CPostStorageImpl::DoRemovePostL(const CCMPost * aPost, TBool aMediaRemoveFile)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("DoRemovePostL"));

	if (aPost->iPostId()==RootId() || aPost->iPostId()==MediaPoolId()) {
		User::Leave(KErrArgument);
	}
	SeekToPostIdL(aPost->iPostId());
	if (!iFound)  User::Leave(KErrNotFound);
	iTable.GetL();
	TInt unread=iTable.ColInt32(EUnread);
	if (unread!=0) {
		UpdateReadCounters(aPost->iParentId(), -unread);
	}
	if (aMediaRemoveFile) 
		BaflUtils::DeleteFile(Fs(), aPost->iMediaFileName(), 0);

	SeekToPostIdL(aPost->iPostId());
	if (!iFound)  User::Leave(KErrNotFound);
	iTable.DeleteL();

	NotifyObservers( (CCMPost*)aPost, MPostNotify::EPostDeleted);
}

void CPostStorageImpl::MovePostsL(TInt64 aFromThreadId, TInt64 aToThreadId,
	TTime aFromTime, TFromType aFromType)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("MovePostsL"));

	if (! iDb.Db().InTransaction()) iDb.BeginL();
	TTransactionHolder th(*this);

	refcounted_ptr<CCMPost> prev_parent(0);
	refcounted_ptr<CCMPost> new_parent(0);
	TInt err;
	TRAP(err, prev_parent.reset(GetByPostIdL(0, aFromThreadId)) );
	TRAP(err, new_parent.reset(GetByPostIdL(0, aToThreadId)) );

	TDbSeekKey rk(aFromThreadId);
	if (aFromType==EByDateTime) {
		SwitchIndexL(EIdxParentDatetime);
	} else {
		SwitchIndexL(EIdxParentUpdated);
	}
	if (! iTable.SeekL(rk)) return;
	TTime now; now=GetTime();
	TInt unread=0;
	for (;;) {
		iTable.GetL();
		if (iTable.ColInt64(EParentId) != aFromThreadId) break;

		if (aFromType==EByDateTime && iTable.ColTime(EDatetime) > aFromTime) break;
		if (aFromType==EByUpdated && (
			iTable.IsColNull(EUpdated) ||
			iTable.ColTime(EUpdated) > aFromTime) ) break;

		TUint dbid=iTable.ColUint32(EDbId);
		TDbBookmark row=iTable.Bookmark();
		unread+=iTable.ColInt32(EUnread);
		if (! iTable.IsColNull(EContents) ) {
			refcounted_ptr<CCMPost> p(
				(CCMPost*)iDbIdToObjectMap->GetData(dbid));
			if (! p.get() ) {
				p.reset(CCMPost::NewL(iBBFactory));
				RADbColReadStream s; s.OpenLA(iTable, EContents);
				p->InternalizeL(s);
			} else {
				p->AddRef();
			}
			NotifyObservers(prev_parent.get(), p.get(), MPostNotify::EParentChanged);
			p->iParentId()=aToThreadId;
			NotifyObservers(new_parent.get(), p.get(), MPostNotify::EChildAdded);
		}

		if (aFromType==EByDateTime) {
			SwitchIndexL(EIdxParentDatetime);
		} else {
			SwitchIndexL(EIdxParentUpdated);
		}
		iTable.GotoL(row);
		iTable.UpdateL();
		iTable.SetColL(EUpdated, now);
		iTable.SetColL(EParentId, aToThreadId);
		PutL();
		if (! iTable.SeekL(rk)) break;
	}
	if (unread!=0) {
		UpdateReadCounters(aFromThreadId, -unread);
		UpdateReadCounters(aToThreadId, unread);
	}
	Async();
}

void CPostStorageImpl::RereadUnreadL()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RereadUnreadL"));

	if (! iDb.Db().InTransaction()) iDb.BeginL();
	TTransactionHolder th(*this);

	TDbSeekKey rk(RootId());
	SwitchIndexL(EIdxParentPostId);
	TBool found=iTable.SeekL(rk);
	while (found) {
		TDbBookmark row=iTable.Bookmark();
		iTable.GetL();

		TInt unread=0;
		TInt64 parent=iTable.ColInt64(EPostId);
		TUint dbid=iTable.ColUint(EDbId);
		TDbSeekKey rk(parent);
		TInt prev_unread=iTable.ColInt(EUnread);
		SwitchIndexL(EIdxParentPostId);
		TBool found_child=iTable.SeekL(rk);
		while (found_child) {
			iTable.GetL();
			if (iTable.ColInt64(EParentId) != parent) break;
			unread+=iTable.ColInt32(EUnread);
			found_child=iTable.NextL();
		}

		SwitchIndexL(EIdxParentPostId);
		iTable.GotoL(row);

		if (unread!=prev_unread) {
			iTable.UpdateL();
			iTable.SetColL(EUnread, unread);
			PutL();
			CCMPost* p=(CCMPost*)iDbIdToObjectMap->GetData(dbid);
			if (p) {
				p->iUnreadCounter()=unread;
				NotifyObservers(p, MPostNotify::EUnreadChanged);
				SwitchIndexL(EIdxParentPostId);
				iTable.GotoL(row);
			}
		}
		found=iTable.NextL();
		if (found) {
			iTable.GetL();
			if ( iTable.ColInt64(EParentId) != RootId() ) found=EFalse;
		}
	}

	Async();
}

void CPostStorageImpl::RemovePostsL(TInt64 aThreadId, TTime aFromTime,
				    TFromType aFromType)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL"));

	//User::Leave(-6);

	if (! iDb.Db().InTransaction()) iDb.BeginL();
	TTransactionHolder th(*this);

	refcounted_ptr<CCMPost> parent(0);
	TRAPD(err, parent.reset(GetByPostIdL(0, aThreadId)) );

	TDbSeekKey rk(aThreadId);
	if (aFromType==EByDateTime) {
		SwitchIndexL(EIdxParentDatetime);
	} else {
		SwitchIndexL(EIdxParentUpdated);
	}
	if (! iTable.SeekL(rk)) return;
	TInt unread=0;
	for (;;) {
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL.1"));
		{
			CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL.1.1"));
			iTable.GetL();
			if (iTable.ColInt64(EParentId) != aThreadId) break;
		}

		{
			CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL.1.2"));
			if (aFromType==EByDateTime && iTable.ColTime(EDatetime) > aFromTime) break;
			if (aFromType==EByUpdated && (
				iTable.IsColNull(EUpdated) ||
				iTable.ColTime(EUpdated) > aFromTime) ) break;
		}

		TDbBookmark row;
		{
			CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL.1.3"));
			TUint dbid=iTable.ColUint32(EDbId);
			row=iTable.Bookmark();
			unread+=iTable.ColInt32(EUnread);
		}
		if (! iTable.IsColNull(EContents) ) {
			CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL.2"));
			TUint dbid=iTable.ColUint32(EDbId);
			refcounted_ptr<CCMPost> p(
				(CCMPost*)iDbIdToObjectMap->GetData(dbid));
			if (! p.get() ) {
				p.reset(CCMPost::NewL(iBBFactory));
				RADbColReadStream s; s.OpenLA(iTable, EContents);
				p->InternalizeL(s);
			} else {
				p->AddRef();
			}
			TInt err=Fs().Delete( p->iMediaFileName() );
			if (err!=KErrNone && err!=KErrBadName && err!=KErrNotFound && err!=KErrPathNotFound) {
				CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("file_error"));
				User::Leave(err);
			}
			{
				CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL.3"));
				NotifyObservers(parent.get(), p.get(), MPostNotify::EPostDeleted);
			}
		}
		if (aFromType==EByDateTime) {
			SwitchIndexL(EIdxParentDatetime);
		} else {
			SwitchIndexL(EIdxParentUpdated);
		}
		{
			CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("goto-delete"));
			iTable.GotoL(row);
			iTable.DeleteL();
		}
		if (! iTable.NextL() ) break;
	}
	if (unread!=0) {
		CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("RemovePostsL.4"));
		UpdateReadCounters(aThreadId, -unread);
	}
	Async();
}

TBool CPostStorageImpl::FirstAllL()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("FirstAllL"));
	if (!iHasTransactionHolder) CommitL();
	iFound=EFalse;
	iSeekBy=EAll;
	SwitchIndexL(EIdxDatetime);
	if (! iTable.LastL()) return EFalse;
	for(;;) {
		iTable.GetL();
		if (iTable.ColInt64(EParentId) != RootId()) break;
		if ( ! iTable.PreviousL() ) return EFalse;
	}
	return iFound=ETrue;
}

// iteration
TBool CPostStorageImpl::FirstL(const TInt64& aParentId, TSortBy aSortBy, TOrder aSortOrder, 
			       TBool aOnlyUnread, TBool aOnlyVisible)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("FirstL"));
	if (!iHasTransactionHolder) CommitL();

	iSeekParentId=aParentId;
	iSeekOnlyUnread=aOnlyUnread;
	iSeekOnlyVisible=aOnlyVisible;
	iSeekOrder=aSortOrder;
	iSeekBy=aSortBy;
	TBool& ret=iFound;
	ret=EFalse;

	TDbSeekKey rk(aParentId);
	if (aSortBy==EByDate) {
		SwitchIndexL(1);
	} else if (aSortBy==EByAuthor){
		SwitchIndexL(2);
	} else if (aSortBy == EByUnread){
		SwitchIndexL(4);
	} else if (aSortBy == EByTitle) {
		SwitchIndexL(5);
	} else if (aSortBy == EByLastPost) {
		SwitchIndexL(7);
	} else {
		SwitchIndexL(6);
	}

	if (aSortOrder==EAscending) {
		TDbSeekKey rk(aParentId);
		ret=iTable.SeekL(rk);
	} else {
		TDbSeekKey rk(aParentId+1);
		ret=iTable.SeekL(rk, RDbTable::ELessThan);
	}
	if (!ret) return ret;

	if (aOnlyUnread) {
		TInt unread=0;
		TInt64 parent=aParentId;
		for(;;) {
			iTable.GetL();
			unread=iTable.ColInt32(EUnread);
			parent=iTable.ColInt64(EParentId);
			if (parent!=aParentId) return EFalse;
			
			if (unread>0) return ETrue;
			if (iSeekOrder==EAscending) {
				ret=iTable.NextL();
			} else {
				ret=iTable.PreviousL();
			}
			if (!ret) return ret;
		}
	} else if (aOnlyVisible) {
		TBool hidden=EFalse;
		TInt64 parent=aParentId;
		for(;;) {
			iTable.GetL();
			parent=iTable.ColInt64(EParentId);
			if (! iTable.IsColNull(EHidden) )
				hidden=iTable.ColUint32(EHidden);
			else
				hidden=EFalse;
			if (parent!=aParentId) return EFalse;
			
			if (!hidden) return ETrue;
			if (iSeekOrder==EAscending) {
				ret=iTable.NextL();
			} else {
				ret=iTable.PreviousL();
			}
			if (!ret) return ret;
		}
	}
	return ETrue;
}

TBool CPostStorageImpl::NextL()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("NextL"));

	TBool& ret=iFound;
	if (!ret) return EFalse;

	if (iSeekBy==EAll) {
		ret=iTable.PreviousL();
		while(ret) {
			iTable.GetL();
			if (iTable.ColInt64(EParentId) != RootId()) break;
			ret=iTable.PreviousL();
		}
		return ret;
	}
	TInt64 parent; TInt unread; TBool hidden;
	for(;;) {
		if (iSeekOrder==EAscending) {
			ret=iTable.NextL();
		} else {
			ret=iTable.PreviousL();
		}
		if (!ret) return ret;
		iTable.GetL();
		parent=iTable.ColInt64(EParentId);
		if (parent!=iSeekParentId) return EFalse;
		if (iSeekOnlyUnread) {
			unread=iTable.ColInt32(EUnread);
			if (unread>0) return ETrue;
		} else if (iSeekOnlyVisible) {
			if (! iTable.IsColNull(EHidden) )
				hidden = iTable.ColUint32(EHidden);
			else
				hidden=EFalse;
			if (!hidden) return ETrue;
		} else	{
			return ETrue;
		}
	}
}

TUint CPostStorageImpl::GetCurrentIndexL()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("GetCurrentIndexL"));

	if (!iFound) User::Leave(KErrNotFound);

	iTable.GetL();
	return iTable.ColUint32(EDbId);
}

// accessing
CCMPost* CPostStorageImpl::GetCurrentL(MPostNotify* aNotify) // addrefs, sets up for notification
{
	if (!iHasTransactionHolder) CommitL();

	PrintDebugObserver(aNotify);
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("GetCurrentL"));

	if (!iFound) User::Leave(KErrNotFound);

	TTime t; t=GetTime();
	if (!iHasTransactionHolder) {
		TAutomaticTransactionHolder th(*this);
		iTable.UpdateL();
		iTable.SetColL(ELastAccess, t);
		PutL();
	} else {
		iTable.UpdateL();
		iTable.SetColL(ELastAccess, t);
		PutL();
	}
	iTable.GetL();
	TUint dbid=iTable.ColUint32(EDbId);
	CCMPost* existing=(CCMPost*)iDbIdToObjectMap->GetData(dbid);
	TObservation obs;
	if (existing) {
		if (aNotify) {
			obs.iPost=existing;
			obs.iNotify=aNotify;
			TBtreePos dummy;
			iObsTree.InsertL(dummy, obs, EAllowDuplicates);
		}

		existing->AddRef();
		return existing;
	}

	TInt defaulticon=KUnknownIconIndex;
	refcounted_ptr<CCMPost> p(CCMPost::NewL(iBBFactory));
	{
		if (! iTable.IsColNull(EContents) ) {
			RADbColReadStream s; s.OpenLA(iTable, EContents);
			p->InternalizeL(s);
		} else {
			// placeholder
			p->iParentId()=RootId();
			p->iLocalDatabaseId()=iTable.ColUint32(EDbId);
			p->iSender.iName()=iTable.ColDes(EAuthor);
			p->iBodyText->Append(iTable.ColDes(ETitle));
			if (p->iBodyText->iPtr.Compare(iNewThreadTitle)==0) {
				defaulticon=KNewThreadIconIndex;
			} else if (p->iBodyText->iPtr.Compare(iErrorTitle)==0) {
				defaulticon=KErrorIconIndex;
			} else {
				defaulticon=KPlaceholderIconIndex;
			}
			p->iTimeStamp=iTable.ColTime(EDatetime);
		}
		if (! iTable.IsColNull(ELastPostAuthor) && ! iTable.IsColNull(ELastPostDate) ) {
			p->SetLastPostInfo(iTable.ColDes(ELastPostAuthor), iTable.ColTime(ELastPostDate));
		}
	}
	if (iEikEnv) {
		if (! iTable.IsColNull(EIcon) ) {
			RWsSession& ws=iEikEnv->WsSession();
			RADbColReadStream s; s.OpenLA(iTable, EIcon);
			auto_ptr<CWsBitmap> bitmap(new (ELeave) CWsBitmap(ws));
			bitmap->InternalizeL(s);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap->SetSizeInTwips(screen);
			auto_ptr<CGulIcon> icon(CGulIcon::NewL(bitmap.get()));
			bitmap.release();
			iIconArray->AppendL(icon.get());
			icon.release();
			p->SetThumbNailIndex(iIconArray->Count()-1);
		} else {
			p->SetThumbNailIndex(defaulticon);
		}
	} else {
		p->SetThumbNailIndex(KNoIconIndex);
	}
	p->iParentId()=iTable.ColInt64(EParentId);
	p->iPostId()=iTable.ColInt64(EPostId);
	p->iLocalDatabaseId()=iTable.ColUint32(EDbId);
	p->iTimeStamp()=iTable.ColTime(EDatetime);
	p->iSender.iName()=iTable.ColDes(EAuthor);
	p->iUnreadCounter()=iTable.ColInt32(EUnread);

	if (! iTable.IsColNull(EErrorInfo) ) {
		RADbColReadStream s; s.OpenLA(iTable, EErrorInfo);
		refcounted_ptr<CBBErrorInfo> info(CBBErrorInfo::NewL(BBDataFactory()));
		CC_TRAPD(err, info->InternalizeL(s));
		if (err!=KErrNone) {
			info->iUserMsg->Append(_L("Errors exist, but cannot retrieve fully"));
			if (info->iErrorCode.iCode()==0)
				info->iErrorCode=MakeErrorCode(0, err);
		}
		p->iErrorInfo=info.release();
	}

	obs.iPost=p.get();
	obs.iNotify=aNotify;
	TBtreePos dummy;
	TInt err;
	CC_TRAP(err, iDbIdToObjectMap->AddDataL(dbid, p.get()));

	if (err==KErrNone && aNotify) {
		CC_TRAP(err, iObsTree.InsertL(dummy, obs, EAllowDuplicates));
	}
	if (err!=KErrNone) {
		iDbIdToObjectMap->DeleteL(dbid);
		User::Leave(err);
	}

	p->SetDeletionNotify(this);
	return p.release();
}

void CPostStorageImpl::SubscribeL(CCMPost* aPost, MPostNotify* aNotify) // addrefs, sets up for notification
{
	PrintDebugObserver(aNotify);
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("SubscribeL"));

	if (aNotify) {
		TObservation obs;

		obs.iPost=aPost;
		obs.iNotify=aNotify;
		TBtreePos dummy;

		iObsTree.InsertL(dummy, obs, EAllowDuplicates);
	}
	aPost->AddRef();
}

CCMPost* CPostStorageImpl::GetByIndexL(MPostNotify* aNotify, TUint aIdx) // addrefs, sets up for notification
{
	if (!iHasTransactionHolder) CommitL();
	PrintDebugObserver(aNotify);
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("GetByIndexL"));

	if (aIdx==-1) {
		SubscribeL(iRoot, aNotify);
		return iRoot;
	}
	if (aIdx==-2) {
		SubscribeL(iMediaPool, aNotify);
		return iMediaPool;
	}

	SeekToDbIdL(aIdx);
	return GetCurrentL(aNotify);
}

CCMPost* CPostStorageImpl::GetByPostIdL(MPostNotify* aNotify, TInt64 aPostId) // addrefs, sets up for notification
{
	if (!iHasTransactionHolder) CommitL();
	PrintDebugObserver(aNotify);
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("GetByPostIdL"));

	if (aPostId==RootId()) {
		return GetRootL(aNotify);
	}
	SeekToPostIdL(aPostId);
	if (!iFound && aPostId==MediaPoolId() && iMediaPool) {
		iMediaPool->AddRef();
		if (aNotify) {
			TObservation obs;
			obs.iPost=iMediaPool;
			obs.iNotify=aNotify;
			TBtreePos dummy;
			iObsTree.InsertL(dummy, obs, EAllowDuplicates);
		}

		return iMediaPool;
	}

	return GetCurrentL(aNotify);
}


void CPostStorageImpl::Release(CCMPost* aPost, MPostNotify* aNotify) // releases
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("Release"));

	if (!aPost) return;

	if (!aNotify) {
		aPost->Release();
		return;
	}
	PrintDebugRelease(aNotify);

	if (iToNotify) {
		CList<MPostNotify*>::Node*	i=0, *temp=0;
		for (i=iToNotify->iFirst; i; ) {
			if (i->Item==aNotify) {
				temp=i->Next;
				iToNotify->DeleteNode(i, ETrue);
				i=temp;
			} else {
				i=i->Next;
			}
		}
	}

	TBtreePos pos;
	TObservation obs;
	TBool found=iObsTree.FindL(pos, aPost, TBtree::EEqualTo);
	while(found) {
		iObsTree.ExtractAtL(pos, obs);
		if (obs.iPost != aPost) {
			found=EFalse;
		} else {
			if (obs.iNotify==aNotify) {
				CC_TRAPD(err, iObsTree.DeleteAtL(pos));
				if (err!=KErrNone) User::Panic(_L("ContextMedia"), KErrCorrupt);
				aPost->Release();
				return;
			}
			found=iObsTree.NextL(pos);
		}
	}
	aPost->Release();
	User::Leave(KErrNotFound);
}


_LIT(KClassName, "CPostStorageImpl");

CPostStorage::CPostStorage() : CCheckedActive(EPriorityLow, KClassName) { }

CPostStorageImpl::CPostStorageImpl(MApp_context& Context, CDb& Db, MBBDataFactory* aBBFactory) : MContextBase(Context),
		MDBStore(Db.Db()), iDb(Db), iObsTree(EBtreeFast), iBBFactory(aBBFactory), iSeekOnlyVisible(ETrue) { }


void CPostStorageImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("ConstructL"));

	iEikEnv=CEikonEnv::Static();

	CActiveScheduler::Add(this);
	iResource=LoadSystemResourceL(iEikEnv, _L("contextmedia"));

	TInt cols[]= { 
		EDbColUint32,	// dbid
		EDbColInt64,	// parentid
		EDbColInt64,	// postid
		EDbColDateTime,	// timestamp
		EDbColDateTime,	// last access

		EDbColText,	// author
		EDbColInt32,	// unread counter
		EDbColLongBinary, // icon
		EDbColLongBinary, // contents
		EDbColText,	// title sample

		EDbColBit,      // hidden
		EDbColText,	// last post author
		EDbColDateTime,	// last post datetime
		EDbColLongBinary, // error info
		EDbColDateTime, // updated stamp
		EDbColInt32,
		-1
	};
	TInt col_flags[]={ TDbCol::EAutoIncrement, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0 };
	TInt idxs[]= { EDbId, -2, EParentId, EDatetime, -2, EParentId, EAuthor, -2, EPostId, -2, 
		EParentId, EUnread, -2, EParentId, ETitle, -2, EParentId, EPostId, -2,
		EParentId, ELastPostDate, -2, EParentId, EUpdated, -2,
		EDatetime, -1 };

	_LIT(KPosts, "POSTS");
	MDBStore::ConstructL(cols, idxs, false, KPosts, ETrue, col_flags);
	/*
	if (iTable.ColCount()==10) {
		// add hidden
		iTable.Close();
		CDbColSet* colset=iDb.Db().ColSetL(KPosts);
		CleanupStack::PushL(colset);
		TDbCol n(_L("C11"), EDbColBit);
		colset->AddL(n);
		User::LeaveIfError(iDb.Db().AlterTable(KPosts, *colset));
		CleanupStack::PopAndDestroy();
		MDBStore::ConstructL(cols, idxs, false, KPosts, EFalse, col_flags);
	}
	*/

	iNextFreeIconSlot=-1;
	iIconArray=new (ELeave) CArrayPtrFlat<CGulIcon>(32);

	if (iEikEnv) {
		/*DEFAULT ICONS TO ICON ARRAY: */
		RWsSession& ws=iEikEnv->WsSession();
#ifndef __S60V3__
#ifdef __WINS__
		_LIT(KMBMFileName,"z:\\system\\data\\contextmedia.mbm");
#else
		_LIT(KMBMFileName,"c:\\system\\data\\contextmedia.mbm");
#endif
#else
#ifdef __WINS__
		_LIT(KMBMFileName,"z:\\resource\\contextmedia.mbm");
#else
		_LIT(KMBMFileName,"c:\\resource\\contextmedia.mbm");
#endif
#endif
		TFileName fn=KMBMFileName();
		if (! BaflUtils::FileExists(Fs(), fn)) {
			fn.Replace(0, 1, _L("e"));
		}
		
		//icons should be loaded in the order they were declared in mmp file!
		/* 1: Question mark icon*/
		auto_ptr<CWsBitmap> bitmap(new (ELeave) CWsBitmap(ws));
		TInt err;
		if (err=bitmap->Load(fn, EMbmContextmediaDefaultthumb) != KErrNone) {
			bitmap->Create(TSize(48, 36), EGray2);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap->SetSizeInTwips(screen);
		}
		auto_ptr<CGulIcon> icon(CGulIcon::NewL(bitmap.get()));
		bitmap.release();
		iIconArray->AppendL(icon.get());
		icon.release();

		/* 2: Post-a-reply icon */
		auto_ptr<CWsBitmap> bitmap2(new (ELeave) CWsBitmap(ws));
		if (err=bitmap2->Load(fn, EMbmContextmediaReply) != KErrNone) {
			bitmap2->Create(TSize(48, 36), EGray2);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap2->SetSizeInTwips(screen);
		}
		auto_ptr<CGulIcon> icon2(CGulIcon::NewL(bitmap2.get()));
		bitmap2.release();
		iIconArray->AppendL(icon2.get());
		icon2.release();

		/* 3: audio icon */
		auto_ptr<CWsBitmap> bitmap3(new (ELeave) CWsBitmap(ws));
		if (err=bitmap3->Load(fn, EMbmContextmediaAudio) != KErrNone) {
			bitmap3->Create(TSize(48, 36), EGray2);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap3->SetSizeInTwips(screen);
		}
		auto_ptr<CGulIcon> icon3(CGulIcon::NewL(bitmap3.get()));
		bitmap3.release();
		iIconArray->AppendL(icon3.get());
		icon3.release();

		/* 4: video icon */
		auto_ptr<CWsBitmap> bitmap4(new (ELeave) CWsBitmap(ws));
		if (err=bitmap4->Load(fn, EMbmContextmediaVideo) != KErrNone) {
			bitmap4->Create(TSize(48, 36), EGray2);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap4->SetSizeInTwips(screen);
		}
		auto_ptr<CGulIcon> icon4(CGulIcon::NewL(bitmap4.get()));
		bitmap4.release();
		iIconArray->AppendL(icon4.get());
		icon4.release();

		/* 5: new thread icon */
		auto_ptr<CWsBitmap> bitmap5(new (ELeave) CWsBitmap(ws));
		if (err=bitmap5->Load(fn, EMbmContextmediaNew) != KErrNone) {
			bitmap5->Create(TSize(48, 36), EGray2);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap5->SetSizeInTwips(screen);
		}
		auto_ptr<CGulIcon> icon5(CGulIcon::NewL(bitmap5.get()));
		bitmap5.release();
		iIconArray->AppendL(icon5.get());
		icon5.release();

		/* 6: use code icon */
		auto_ptr<CWsBitmap> bitmap6(new (ELeave) CWsBitmap(ws));
		if (err=bitmap6->Load(fn, EMbmContextmediaCode) != KErrNone) {
			bitmap6->Create(TSize(48, 36), EGray2);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap6->SetSizeInTwips(screen);
		}
		auto_ptr<CGulIcon> icon6(CGulIcon::NewL(bitmap6.get()));
		bitmap6.release();
		iIconArray->AppendL(icon6.get());
		icon6.release();

		/* 7: error icon */
		auto_ptr<CWsBitmap> bitmap7(new (ELeave) CWsBitmap(ws));
		if (bitmap7->Load(fn, EMbmContextmediaError) != KErrNone) {
			bitmap7->Create(TSize(48, 36), EGray2);
			CWsScreenDevice* screen=iEikEnv->ScreenDevice();
			bitmap7->SetSizeInTwips(screen);
		}
		auto_ptr<CGulIcon> icon7(CGulIcon::NewL(bitmap7.get()));
		bitmap7.release();
		iIconArray->AppendL(icon7.get());
		icon7.release();
	}

	iNbDefaultIcons = iIconArray->Count();

	iEikEnv->ReadResource(iPlaceholderTitle, R_LOADING_TITLE);
	iEikEnv->ReadResource(iNewThreadTitle, R_NEW_THREAD_TITLE);
	iEikEnv->ReadResource(iErrorTitle, R_ERROR_TITLE);
	iEikEnv->ReadResource(iPlaceholderAuthor, R_NO_AUTHOR);
	
	iObsPool=CMemPagePool::NewL();
	iObsTree.Connect(iObsPool, &iObsKey);
	iObsIsOpen=true;

	iDbIdToObjectMap=CGenericIntMap::NewL();

	refcounted_ptr<CCMPost> root(CCMPost::NewL(iBBFactory));
	root->iParentId()=RootId();
	root->iPostId()=RootId();
	root->iLocalDatabaseId()=-1;
	iDbIdToObjectMap->AddDataL(root->iLocalDatabaseId(), root.get());
	iRoot=root.release();

	if (! SeekToPostIdL( MediaPoolId() ) ) {
		refcounted_ptr<CCMPost> mediapool(CCMPost::NewL(iBBFactory));
		mediapool->iParentId()=MediaPoolId();
		mediapool->iPostId()=MediaPoolId();
		mediapool->iLocalDatabaseId()=-2;
		iDbIdToObjectMap->AddDataL(mediapool->iLocalDatabaseId(), mediapool.get());
		iMediaPool=mediapool.release();
	}
	{
		TDbSeekKey rk( RootId() );
		SwitchIndexL(EIdxParentDatetime);
		TBool found=iTable.SeekL(rk);
		while (found) {
			iTable.GetL();
			if ( iTable.ColInt64(EParentId)!=RootId() ) {
				break;
			}
			iRoot->iUnreadCounter()+=iTable.ColInt(EUnread);
			found=iTable.NextL();
		}
		iRoot->iBodyText->Append(_L("All"));
	}

	iNbHiddenThreads = CountHiddenThreadsL();
}

void CPostStorageImpl::CheckedRunL()
{
	if (iDb.Db().InTransaction()) {
		TAutoBusy busy(Reporting());
		iDb.CommitL();
	}
}

void CPostStorageImpl::Async()
{
	if (IsActive()) return;
	TRequestStatus *s=&iStatus;
	User::RequestComplete(s, KErrNone);
	SetActive();
}

void CPostStorageImpl::UpdateReadCounters(TInt64 from_id, TInt read_counter)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdateReadCounters"));

	if (read_counter==0) return;

	TInt64 parent_id=from_id;
	while (parent_id != RootId() && SeekToPostIdL(parent_id)) {
		iTable.GetL();
		TUint dbid=iTable.ColUint32(EDbId);
		TInt count=iTable.ColInt32(EUnread)+read_counter;
		if (count<0) {
			read_counter-=count;
			count=0;
		}
			
		parent_id=iTable.ColInt64(EParentId);
		iTable.UpdateL();
		iTable.SetColL(EUnread, count);
		iTable.SetColL(ELastAccess, GetTime());
		PutL();
		CCMPost* p=(CCMPost*)iDbIdToObjectMap->GetData(dbid);
		if (p) {
			p->iUnreadCounter()=count;
			NotifyObservers(p, MPostNotify::EUnreadChanged);
		}
	}

	iRoot->iUnreadCounter()+=read_counter;
	NotifyObservers(iRoot, MPostNotify::EUnreadChanged);
}

void CPostStorageImpl::UpdateLastPost(TInt64 from_id, const TDesC& aAuthorName, const TTime& aDateTime)
{
	TInt64 parent_id=from_id;
	while (parent_id != RootId() && SeekToPostIdL(parent_id)) {
		iTable.GetL();
		TUint dbid=iTable.ColUint32(EDbId);

		parent_id=iTable.ColInt64(EParentId);
		iTable.UpdateL();
		iTable.SetColL(ELastPostAuthor, aAuthorName);
		iTable.SetColL(ELastPostDate, aDateTime);
		PutL();
		CCMPost* p=(CCMPost*)iDbIdToObjectMap->GetData(dbid);
		if (p) {
			p->SetLastPostInfo(aAuthorName, aDateTime);
			NotifyObservers(p, MPostNotify::ELastPostChanged);
		}
	}
}

void CPostStorageImpl::MarkAsRead(CCMPost* aPost)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("MarkAsRead"));

	if (aPost==iRoot) return;
	if (aPost==iMediaPool) return;

	TInt64 parent_id;
	TInt read_counter=0;
	if (! iDb.Db().InTransaction()) iDb.BeginL();
	TTransactionHolder th(*this);

	if (SeekToPostIdL(aPost->iPostId())) {
                iTable.GetL();
                TUint dbid=iTable.ColUint32(EDbId);
		parent_id=iTable.ColInt64(EParentId);
		read_counter = iTable.ColInt32(EUnread);
                DoMarkAsReadL(dbid);
	}
	//NotifyObservers(aPost, MPostNotify::EUnreadChanged);

	UpdateReadCounters(parent_id, -read_counter);

	Async();
}

void CPostStorageImpl::DoMarkAsReadL(TUint db_id)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("DoMarkAsReadL"));

	auto_ptr<CArrayFixFlat<TUint> > aPostIdArray (new (ELeave) CArrayFixFlat<TUint>(50));

	if (SeekToDbIdL(db_id)) {
		iTable.GetL();
		TInt64 post_id   = iTable.ColInt64(EPostId);
		CCMPost* p=(CCMPost*)iDbIdToObjectMap->GetData(db_id);
		iTable.UpdateL();
		iTable.SetColL(EUnread, 0);
		iTable.SetColL(ELastAccess, GetTime());
		PutL();

		if (p) {
			p->iUnreadCounter()=0;
			NotifyObservers(p, MPostNotify::EUnreadChanged);
		}
			
		TBool ok = FirstL(post_id, EByDate, EAscending, ETrue);
		while (ok) {
			TUint dbid=GetCurrentIndexL();
			aPostIdArray->AppendL(dbid);
			ok = NextL();
		}
	}
	
	if (aPostIdArray->Count()!=0) {
		int i;
		for (i=0; i< aPostIdArray->Count(); i++) {
			TUint idx=(*aPostIdArray)[i];
			DoMarkAsReadL(idx);
		}
	} 
}

TBool CPostStorageImpl::SeekToPostIdL(const TInt64& aPostId)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("SeekToPostIdL"));

	SwitchIndexL(3);
	TDbSeekKey rk(aPostId);
	TBool &ret=iFound;
	ret=EFalse;

	ret=iTable.SeekL(rk);
	return ret;
}

TBool CPostStorageImpl::SeekToDbIdL(TUint aDbId)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("SeekToDbIdL"));

	SwitchIndexL(0);
	TDbSeekKey rk(aDbId);
	TBool &ret=iFound;
	ret=EFalse;

	ret=iTable.SeekL(rk);
	return ret;
}

CArrayPtr<CGulIcon>* CPostStorageImpl::IconArray()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("IconArray"));

	return iIconArray;
}

void CPostStorageImpl::TObsKey::Between(const TAny* aLeft,const TAny* aRight, TBtreePivot& aPivot) const
{
	TUint left=(TUint)*(CCMPost**)aLeft;
	TUint right=(TUint)*(CCMPost**)aRight;

	TUint mid=left+(right-left)/2;
	aPivot.Copy((TUint8*)&mid, sizeof(mid));
}

TInt CPostStorageImpl::TObsKey::Compare(const TAny* aLeft,const TAny* aRight) const
{
	TUint left=(TUint)*(CCMPost**)aLeft;
	TUint right=(TUint)*(CCMPost**)aRight;

	if (left < right) return -1;
	if (right < left) return 1;
	return 0;
}

const TAny* CPostStorageImpl::TObsKey::Key(const TAny* anEntry) const
{
	TObservation* o=(TObservation*)anEntry;
	return (TAny*)&(o->iPost);
}

void CPostStorageImpl::NotifyObservers(CCMPost* aPost, MPostNotify::TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("NotifyObservers-1"));

	if (aPost->iParentId()==RootId()) {
		NotifyObservers(iRoot, aPost, aEvent);
	} else if (iMediaPool && aPost->iParentId()==MediaPoolId()) {
		NotifyObservers(iMediaPool, aPost, aEvent);
	} else {
		TInt err; TBool parent_found=EFalse;
		CC_TRAP(err, parent_found=SeekToPostIdL(aPost->iParentId()));
		if (err==KErrNone && parent_found) {
			CC_TRAP(err, iTable.GetL());
			if (err==KErrNone) {
				TUint dbid=iTable.ColUint32(EDbId);
				CCMPost* aParent=(CCMPost*)iDbIdToObjectMap->GetData(dbid);
				NotifyObservers(aParent, aPost, aEvent);
			}
		}
	}
}

void CPostStorageImpl::NotifyObservers(CCMPost* aPost, CCMPost* aPost2, MPostNotify::TEvent aEvent)
{
	if (iToNotify) 
		User::Leave(-12005);
	iToNotify=CList<MPostNotify*>::NewL();
	CC_TRAPD(err, NotifyObserversL(aPost, aPost2, aEvent));
	delete iToNotify; iToNotify=0;
	User::LeaveIfError(err);
}

void CPostStorageImpl::NotifyObserversL(CCMPost* aPost, CCMPost* aPost2, MPostNotify::TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("NotifyObservers-2"));

	TBtreePos pos;
	TObservation obs;
	TBool found;

	CList<MPostNotify*>* to_notify=iToNotify;
	if (aPost) {
		found=iObsTree.FindL(pos, aPost, TBtree::EEqualTo);
		while(found) {
			iObsTree.ExtractAtL(pos, obs);
			if (obs.iPost != aPost) {
				found=EFalse;
			} else {
				to_notify->AppendL(obs.iNotify);
				found=iObsTree.NextL(pos);
			}
		}
	}

	MPostNotify* notif=0;
	while( (notif=to_notify->Pop()) ) {
		notif->PostEvent(aPost, aPost2, aEvent);
	}

	if (aPost2) {
		found=iObsTree.FindL(pos, aPost2, TBtree::EEqualTo);
		while(found) {
			iObsTree.ExtractAtL(pos, obs);
			if (obs.iPost != aPost2) {
				found=EFalse;
			} else {
				to_notify->AppendL(obs.iNotify);
				found=iObsTree.NextL(pos);
			}
		}
	}
	while( (notif=to_notify->Pop()) ) {
		notif->PostEvent(0, aPost2, aEvent);
	}
}

void CPostStorageImpl::PostDeleted(const class CCMPost* aPost)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("PostDeleted"));

	CC_TRAPD(err, iDbIdToObjectMap->DeleteL(aPost->iLocalDatabaseId()));
	if (err!=KErrNone) { User::Panic(_L("ContextMedia"), KErrCorrupt); }
	TInt icon=aPost->GetThumbnailIndex();
	if (icon>=iNbDefaultIcons) { 
		// the icon array should not be reduced in terms of indices indices !!
		delete iIconArray->At(icon);
		iIconArray->At(icon) = (CGulIcon*)iNextFreeIconSlot;
		iNextFreeIconSlot=icon;
	}
}

CCMPost* CPostStorageImpl::GetRootL(MPostNotify* aNotify) // sets up for notification 
{
	PrintDebugObserver(aNotify);
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("GetRootL"));

	if (aNotify) SubscribeL(iRoot, aNotify);
	else iRoot->AddRef();
	return iRoot;
}


void CPostStorageImpl::AddPlaceHolderL(TInt64 aThreadId)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("AddPlaceHolderL"));

	TBool exists=EFalse;
	CommitL();
	if (SeekToPostIdL(aThreadId)) { exists=ETrue;}
	
	TUint dbid;
	{
		TAutomaticTransactionHolder th(*this);
		if (exists) {
			iTable.UpdateL();
		} else {
			InsertL();
		}

		dbid=iTable.ColUint(EDbId);
		iTable.SetColL(EParentId, RootId());
		iTable.SetColL(EPostId, aThreadId);
		
		TTime t; t=GetTime();
		iTable.SetColL(EDatetime, t);
		iTable.SetColL(EUpdated, t);
		iTable.SetColL(ELastAccess, t);

		iTable.SetColL(EAuthor, iPlaceholderAuthor);
		iTable.SetColL(ETitle, iPlaceholderTitle);
		iTable.SetColL(EUnread, 0);
		iTable.SetColL(EHidden, EFalse);

		PutL();
	}

	refcounted_ptr<CCMPost> placeholder(GetByIndexL(0, dbid));

	if (exists) {
		NotifyObservers(placeholder.get(), MPostNotify::EPlaceholderFilled);
	} else {
                NotifyObservers(placeholder.get(), MPostNotify::EChildAdded);
	}

	
}

void CPostStorageImpl::AddNewThreadPlaceHolderL(TInt64 aThreadId)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("AddNewThreadPlaceHolderL"));

	CommitL();
	if (!SeekToPostIdL(aThreadId)) User::Leave(KErrNotFound);

	TUint dbid;
	{
		TAutomaticTransactionHolder th(*this);
		iTable.UpdateL();

		dbid=iTable.ColUint(EDbId);
		TTime t; t=GetTime();
		iTable.SetColL(EDatetime, t);
		iTable.SetColL(EUpdated, t);
		iTable.SetColL(ELastAccess, t);
		iTable.SetColL(ETitle, iNewThreadTitle);
		
		TBuf<50> buf;
		Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, buf);
		
		iTable.SetColL(EAuthor, buf);
		iTable.SetColL(EUnread, 0);
		iTable.SetColL(EHidden, EFalse);

		PutL();
	}
	
	refcounted_ptr<CCMPost> placeholder(GetByIndexL(0, dbid));

	NotifyObservers(placeholder.get(), MPostNotify::EPlaceholderFilled);
}

void CPostStorageImpl::AddErrorPlaceHolderL(TInt64 aThreadId)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("AddErrorPlaceHolderL"));

	CommitL();
	if (!SeekToPostIdL(aThreadId)) User::Leave(KErrNotFound);

	TUint dbid;
	{
		TAutomaticTransactionHolder th(*this);
		iTable.UpdateL();

		dbid=iTable.ColUint(EDbId);
		TTime t; t=GetTime();
		iTable.SetColL(EDatetime, t);
		iTable.SetColL(EUpdated, t);
		iTable.SetColL(ELastAccess, t);
		iTable.SetColL(ETitle, iErrorTitle);
		iTable.SetColL(EAuthor, iPlaceholderAuthor);
		iTable.SetColL(EUnread, 0);
		iTable.SetColL(EHidden, EFalse);

		PutL();
	}
	
	refcounted_ptr<CCMPost> placeholder(GetByIndexL(0, dbid));

	NotifyObservers(placeholder.get(), MPostNotify::EPlaceholderFilled);
}

TBool CPostStorageImpl::IsFirstPostInThread(TInt64 aThreadId, TInt64 aPostId)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("IsFirstPostInThread"));

	SwitchIndexL(6);
	TDbSeekMultiKey<2> k;
	k.Add(aThreadId); k.Add(TInt64(0));
	TBool found=EFalse;
	found=iTable.SeekL(k, RDbTable::EGreaterEqual);
	if (!found) return EFalse;

	iTable.GetL();
	if (iTable.ColInt64(EParentId) != aThreadId) return EFalse;

	if (iTable.ColInt64(EPostId) == aPostId) return ETrue;
	return EFalse;
}

void CPostStorageImpl::UpdatePostL(CCMPost * aPost)
{
	PrintDebugUpdate(aPost);
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdatePostL"));

	if (aPost->iPostId() != RootId() ) {
		SeekToPostIdL(aPost->iPostId());
		if (!iFound) User::Leave(KErrNotFound);

		if (! iDb.Db().InTransaction()) iDb.BeginL();
		TTransactionHolder th(*this);

		iTable.UpdateL();
		{
			// we have to set this to null, since we might have
			// updated it in the same transaction, in which case
			// the new contents would get appended instead of replacing
			iTable.SetColNullL(EContents);
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			aPost->ExternalizeL(w);
			w.CommitL();
		}

		TTime t; t=GetTime();
		iTable.SetColL(ELastAccess, t);
		iTable.SetColL(EUpdated, t);
		iTable.SetColL(EAuthor, aPost->iSender.iName());

		TInt64 prev_parent=iTable.ColInt64(EParentId);
		if (prev_parent != aPost->iParentId() ) {
			iTable.SetColL(EParentId, aPost->iParentId());
		}

		TInt unread=0;
		unread=iTable.ColInt32(EUnread);
		PutL();
		TInt64 new_parentid=aPost->iParentId();
		if (prev_parent != new_parentid) {
			aPost->iParentId()=prev_parent;
			NotifyObservers(aPost, MPostNotify::EParentChanged);
			aPost->iParentId()=new_parentid;
			NotifyObservers(aPost, MPostNotify::EChildAdded);
			if (unread!=0) {
				UpdateReadCounters(prev_parent, -unread);
				UpdateReadCounters(aPost->iParentId(), unread);
			}
		}

		refcounted_ptr<CCMPost> parent(0);
		if (aPost->iParentId()==RootId()) {
			iRoot->AddRef();
			parent.reset(iRoot);
		} else if (iMediaPool && aPost->iParentId()==MediaPoolId()) {
			iMediaPool->AddRef();
			parent.reset(iMediaPool);
		} else {
			parent.reset(GetByPostIdL(0, aPost->iParentId()));
		}
 
		NotifyObservers(parent.get(), aPost, MPostNotify::EPostUpdated);
		Async();
	} else {
		NotifyObservers(0, aPost, MPostNotify::EPostUpdated);
	}
}
	
void CPostStorageImpl::UpdateFileName(TInt64 aPostId, const TDesC& aFileName, const TDesC& aContentType)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdateFileName"));

	if (! iDb.Db().InTransaction() ) iDb.BeginL();
	TTransactionHolder th(*this);
	TInt64 parent=DoUpdateFileName(aPostId, aFileName, aContentType);
	if (IsFirstPostInThread(parent, aPostId)) {
		DoUpdateFileName(parent, aFileName, aContentType);
	}
}

TInt64 CPostStorageImpl::DoUpdateFileName(TInt64 aPostId, const TDesC& aFileName, const TDesC& aContentType)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("DoUpdateFileName"));

	refcounted_ptr<CCMPost> post(GetByPostIdL(0, aPostId));
	post->iMediaFileName()=aFileName;
	post->iContentType()=aContentType;

	if (aContentType.Left(5).CompareF(_L("video"))==0) {
		post->SetThumbNailIndex(EMbmContextmediaVideo);
	} else if (aContentType.Left(5).CompareF(_L("audio"))==0) {
		post->SetThumbNailIndex(EMbmContextmediaAudio);
	}

	{
		iTable.UpdateL();
		{
			iTable.SetColNullL(EContents);
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			post->ExternalizeL(w);
			w.CommitL();
		}
		PutL();
	}

	refcounted_ptr<CCMPost> parent(0);
	if (post->iParentId()==RootId()) {
		iRoot->AddRef();
		parent.reset(iRoot);
	} else if (iMediaPool && post->iParentId()==MediaPoolId()) {
		iMediaPool->AddRef();
		parent.reset(iMediaPool);
	} else {
		parent.reset(GetByPostIdL(0, post->iParentId()));
	}
	
	NotifyObservers(parent.get(), post.get(), MPostNotify::EMediaLoaded);

	return post->iParentId();
}

void CPostStorageImpl::UpdateIcon(TInt64 aPostId, auto_ptr<CFbsBitmap>& aIcon)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdateIcon"));

	auto_ptr<CFbsBitmap> icon2(0);
	if (aIcon.get()) {
		icon2.reset(new (ELeave) CFbsBitmap);
		icon2->Duplicate(aIcon->Handle());
	}

	TInt64 parent=DoUpdateIcon(aPostId, aIcon);
	if (IsFirstPostInThread(parent, aPostId)) {
		DoUpdateIcon(parent, icon2);
	}
}

TInt64 CPostStorageImpl::DoUpdateIcon(TInt64 aPostId, auto_ptr<CFbsBitmap>& aIcon)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("DoUpdateIcon"));

	refcounted_ptr<CCMPost> post(GetByPostIdL(0, aPostId));

	iTable.UpdateL();
	{
		RADbColWriteStream w; w.OpenLA(iTable, EIcon);
		aIcon->ExternalizeL(w);
		w.CommitL();
	}
	PutL();

	auto_ptr<CGulIcon> icon(CGulIcon::NewL(aIcon.get()));
	aIcon.release();
	TInt idx;
	if (iNextFreeIconSlot==-1) {
		iIconArray->AppendL(icon.get());
		idx=iIconArray->Count()-1;
	} else {
		TInt temp=(TInt)iIconArray->At(iNextFreeIconSlot);
		iIconArray->At(iNextFreeIconSlot)=icon.get();
		idx=iNextFreeIconSlot;
		iNextFreeIconSlot=temp;
	}
	icon.release();

	post->SetThumbNailIndex(idx);

	refcounted_ptr<CCMPost> parent(0);
	if (post->iParentId()==RootId()) {
		iRoot->AddRef();
		parent.reset(iRoot);
	} else if (iMediaPool && post->iParentId()==MediaPoolId()) {
		iMediaPool->AddRef();
		parent.reset(iMediaPool);
	} else {
		parent.reset(GetByPostIdL(0, post->iParentId()));
	}
	
	NotifyObservers(parent.get(), post.get(), MPostNotify::EThumbnailLoaded);

	return post->iParentId();
}

TInt64 CPostStorageImpl::GetParentId(TInt64 aPostId)
{
	if (!iHasTransactionHolder) CommitL();
	if (!SeekToPostIdL(aPostId)) User::Leave(KErrNotFound);

	iTable.GetL();
	return iTable.ColInt64(EParentId);
}


void CPostStorageImpl::UpdateErrorInfo(TInt64 aPostId, const CBBErrorInfo* aErrorInfo)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdateErrorInfo"));

	if (!iHasTransactionHolder) CommitL();
	if (!SeekToPostIdL(aPostId)) User::Leave(KErrNotFound);
	if (aPostId==RootId() || aPostId==AddMeToMediaPoolId()) User::Leave(KErrArgument);

	iTable.GetL();
	if (!aErrorInfo && iTable.IsColNull(EErrorInfo)) return;

	TUint dbid=iTable.ColUint32(EDbId);
	CCMPost* existing=(CCMPost*)iDbIdToObjectMap->GetData(dbid);

	{
		TAutomaticTransactionHolder th(*this);
		iTable.UpdateL();
		iTable.SetColNullL(EErrorInfo);
		if (aErrorInfo) {
			RADbColWriteStream w; w.OpenLA(iTable, EErrorInfo);
			aErrorInfo->ExternalizeL(w);
			w.CommitL();
		}
		PutL();
	}

	if (existing) {
		if (existing->iErrorInfo) existing->iErrorInfo->Release();
		existing->iErrorInfo=0;
		if (aErrorInfo) {
			existing->iErrorInfo=bb_cast<CBBErrorInfo>(aErrorInfo->CloneL(KNullDesC));
		} else {
			existing->iErrorInfo=0;
		}
		refcounted_ptr<CCMPost> parent(GetByPostIdL(0, existing->iParentId()));
		NotifyObservers(parent.get(), existing, MPostNotify::EErrorUpdated);
	}
}

void CPostStorageImpl::UpdateError(TInt64 aPostId, TInt aErrorCode, const TDesC& aError)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("UpdateError"));

	refcounted_ptr<CCMPost> post(GetByPostIdL(0, aPostId));
	post->iErrorDescr()=aError;
	post->iErrorCode()=aErrorCode;

	CommitL();
	{
		TAutomaticTransactionHolder th(*this);
		iTable.UpdateL();
		{
			iTable.SetColNullL(EContents);
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			post->ExternalizeL(w);
			w.CommitL();
		}
		PutL();
	}

	refcounted_ptr<CCMPost> parent(0);
	if (post->iParentId()==RootId()) {
		iRoot->AddRef();
		parent.reset(iRoot);
	} else if (iMediaPool && post->iParentId()==MediaPoolId()) {
		iMediaPool->AddRef();
		parent.reset(iMediaPool);
	} else {
		parent.reset(GetByPostIdL(0, post->iParentId()));
	}
	
	NotifyObservers(parent.get(), post.get(), MPostNotify::EErrorUpdated);
}

void CPostStorageImpl::CommitL()
{
	if (! iDb.Db().InTransaction() ) return;
	TAutoBusy busy(Reporting());
	iDb.CommitL();
}

TInt64 CPostStorageImpl::GetCurrentIdL()
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("GetCurrentIdL"));

	if (!iFound) 
		User::Leave(KErrNotFound);

	iTable.GetL();
	return iTable.ColInt64(EPostId);
}

TInt64 CPostStorageImpl::GetLastPostId(TInt64 aParentId)
{
	CALLSTACKITEM_N(_CL("CPostStorageImpl"), _CL("GetLastPostId"));

	if (aParentId==MediaPoolId()) {
		SwitchIndexL(3);
		TDbSeekKey k(TInt64(-1500000));

		/* typically, posts in the media pool have negative ids*/
		TBool found=EFalse;
		found=iTable.SeekL(k, RDbTable::EGreaterEqual);
		if (!found) return TInt64(0);

		iTable.GetL();
		if (iTable.ColInt64(EPostId) >= TInt64(0)) return TInt64(0);

		TInt64 postid=iTable.ColInt64(EPostId);
		return postid;
	} else {
		SwitchIndexL(6);
		TDbSeekMultiKey<2> k;
		k.Add(aParentId+1);
		k.Add(TInt64(0));

		TBool found=EFalse;
		found=iTable.SeekL(k, RDbTable::ELessThan);
		if (!found) return TInt64(0);

		iTable.GetL();
		if (iTable.ColInt64(EParentId) != aParentId) return TInt64(0);

		return iTable.ColInt64(EPostId);
	}
}

void CPostStorageImpl::SetThreadVisibleL(TInt64 aPostId, TBool visible)
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("SetThreadVisibleL"));

	CommitL();
	if (aPostId == RootId()) 
		User::Leave(KErrArgument);
	if (aPostId == MediaPoolId()) 
		User::Leave(KErrArgument);
	if (aPostId == AddMeToMediaPoolId()) {
		User::Leave(KErrArgument);
	}

	SeekToPostIdL(aPostId);
	if (!iFound)  User::Leave(KErrNotFound);
	
	iTable.GetL();
	TBool hidden = EFalse;
	if (! iTable.IsColNull(EHidden) )
		hidden = iTable.ColUint32(EHidden);

	if (hidden != visible) {
		{
			TAutomaticTransactionHolder th(*this);
			iTable.UpdateL();
			iTable.SetColL(EHidden, !visible);
			TTime t; t=GetTime();
			iTable.SetColL(ELastAccess, t);

			PutL();
		}

		refcounted_ptr<CCMPost> placeholder(GetCurrentL(0));
		
		if (visible) {
			NotifyObservers(placeholder.get(), MPostNotify::EPostVisible);
			iNbHiddenThreads--;
		} else {
			NotifyObservers(placeholder.get(), MPostNotify::EPostHidden);
			iNbHiddenThreads++;
		}
	}
}

void CPostStorageImpl::SetAllThreadsVisibleL()
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("SetAllThreadsVisibleL"));

	CommitL();
	TBool ok = FirstL(RootId(), CPostStorage::EByDate, CPostStorage::EAscending, EFalse, EFalse);

	while (ok) {
		iTable.GetL();
		TBool hidden=EFalse;
		if ( ! iTable.IsColNull(EHidden) )
			hidden = iTable.ColUint32(EHidden);
		if (hidden) {
			{
				TAutomaticTransactionHolder th(*this);
				iTable.UpdateL();
				iTable.SetColL(EHidden, EFalse);
				TTime t; t=GetTime();
				iTable.SetColL(ELastAccess, t);
				PutL();
			}

			refcounted_ptr<CCMPost> placeholder(GetCurrentL(0));
			NotifyObservers(placeholder.get(), MPostNotify::EPostVisible);
			iNbHiddenThreads--;
		}
		ok = NextL();
	}
}

TInt CPostStorageImpl::HasHiddenThreads()
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("HasHiddenThreads"));

	return (iNbHiddenThreads!=0);
}

TInt CPostStorageImpl::CountHiddenThreadsL()
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("CountHiddenThreadsL"));

	TBool ok = FirstL(RootId(), CPostStorage::EByAuthor, CPostStorage::EAscending, EFalse, EFalse);
	
	TInt ret=0;
		
	while (ok) {
		iTable.GetL();
		TBool hidden = EFalse;
		if (! iTable.IsColNull(EHidden) )
			hidden = iTable.ColUint32(EHidden);
		if (hidden) ret++;
		ok=NextL();
	}
	return ret;
}

TBool CPostStorageImpl::IsPlaceHolderL(TInt64 aThreadId)
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("IsPlaceHolderL"));

	if (aThreadId == RootId()) 
		return EFalse;
	if (iMediaPool && aThreadId == MediaPoolId()) 
		return EFalse;
	if (aThreadId == AddMeToMediaPoolId()) 
		return EFalse;

	SeekToPostIdL(aThreadId);
	if (!iFound)  User::Leave(KErrNotFound);
	
	iTable.GetL();
	
	return  ( !iTable.IsColNull(EAuthor) && 
		  !iTable.IsColNull(ETitle) &&
		  iTable.IsColNull(EContents) &&
		  (iTable.ColDes(EAuthor).Compare(iPlaceholderAuthor)==0 ) &&
		  ( (iTable.ColDes(ETitle).Compare(iPlaceholderTitle)==0) || (iTable.ColDes(ETitle).Compare(iErrorTitle)==0) )
		   );
}

#ifdef DEBUG_STORAGE
void CPostStorageImpl::PrintDebugObserver(MPostNotify* aNotify)
{
	if (!aNotify) return;
	if (CallStackMgr().GetCurrentClass().Compare(_CL("CPostStorageImpl")) ) {
		TBuf<100> msg=_L("**Observer ");
		msg.AppendNum( (TUint32)aNotify, EHex );
		msg.Append(_L(" "));
		TBuf<50> cn;
		TInt state;
		CC()->ConvertToUnicode(cn, CallStackMgr().GetCurrentClass(), state);
		msg.Append(cn);
		RDebug::Print(msg);
	}
}
void CPostStorageImpl::PrintDebugRelease(MPostNotify* aNotify)
{
	if (!aNotify) return;
	TBuf<100> msg=_L("**Release ");
	msg.AppendNum( (TUint32)aNotify, EHex );
	RDebug::Print(msg);
}

void CPostStorageImpl::PrintDebugUpdate(CCMPost* aPost)
{
	if (!aPost) return;
	TBuf<256> msg=_L("**Post update: id ");
	msg.AppendNum(aPost->iPostId());
	msg.Append(_L(" parent "));
	msg.AppendNum(aPost->iParentId());
	msg.Append(_L(" filename "));
	msg.Append(aPost->iMediaFileName());
	msg.Append(_L(" by"));
	TBuf<50> cn;
	TInt state;
	CC()->ConvertToUnicode(cn, CallStackMgr().GetCurrentClass(), state);
	msg.Append(cn);
	//RDebug::Print(msg);
	Reporting().DebugLog(msg);
}
#endif

class CDummyCMNetwork : public CCMNetwork {
private:
	virtual void SetFetchUrl(const TDesC& ) { }
	virtual void SetFetchPeriod(TInt ) { }
	virtual void SetIap(TInt ) { }

	virtual TBool FetchThread(TInt64 ) { return EFalse; }
	virtual void FetchMedia(const CCMPost* , TBool force=EFalse) { }
	virtual void FetchThreads() { }

        virtual void AddStatusListener(MNetworkStatus* ) { }
	virtual void RemoveStatusListener(MNetworkStatus* ) { }
	virtual MNetworkStatus::TFetchStatus GetFetchStatus(TInt64 ) { return MNetworkStatus::EDone; }
	virtual TBool GetDownloadingStatus() { return EFalse; }
};

EXPORT_C CCMNetwork* NewDummyNetworkL()
{
	return new (ELeave) CDummyCMNetwork;
}

