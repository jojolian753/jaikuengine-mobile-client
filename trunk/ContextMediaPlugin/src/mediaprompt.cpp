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

#include "mediaprompt.h"
#include <symbian_auto_ptr.h>
#include "viewids.h"
#include "contextmediaappview2.h"
#include "contextmediaappview.h"
#include "presencemaintainer.h"
#include "cm_storage.h"
#include "break.h"
#include "view_ids.h"
#include "cm_autotaglist.h"
#include "reporting.h"
#include "callstack.h"
#include "cu_common.h"

#include <contextmediaplugin.rsg>
#include <aknglobalnote.h>
#include "contextvariant.hrh"
#include "cm_autotaglist.h"

class CMediaPromptImpl : public CMediaPrompt, public MContextBase, public MUploadCallBack,
	public MPostUploadCallback, public MContextMediaAppUi, public MAskForNames {
private:

	CCMNetwork*	iNetwork;
	CContextMediaEditorView* iEditorView;
	CContextMediaAppThreadView* iThreadView;

	TInt		iCmuiResourceFile, iPluginResourceFile;

	struct TThreadInfo {
		TInt		iId;
		TInt		iResourceId;
	};
	void ConstructL() {
		CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("ConstructL"));

		iTransferer->AddPostCallbackL(this);
		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("LoadResources"));
			iCmuiResourceFile = LoadCmuiResourceL();
			iPluginResourceFile = LoadSystemResourceL(CEikonEnv::Static(), 
				_L("ContextMediaPlugin"));

			iNetwork=NewDummyNetworkL();
			iPhoneHelper=new (ELeave) phonehelper(0);
			iPhoneHelper->ConstructL();
		}

		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("LoadIcons"));
			iTagIcons=CAutoTagListBox::CreateIconList();
		}
		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("CreateEditor"));
			iEditorView=new (ELeave) CContextMediaEditorView(KEditorViewId, 
#ifndef FLICKR
				iNextViewId,
#else
				&iNextViewId,
#endif
				AppContext(), *iNetwork, iStorage, BBDataFactory(), iTransferer, ETrue, 
				iTagIcons, this);
			auto_ptr<CContextMediaEditorView> p(iEditorView);
			iEditorView->ConstructL();
			iAppUi->AddViewL( iEditorView );
			p.release();
		}

		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("DeleteOldTrash"));
			TTime from; from.HomeTime(); 
#ifndef __WINS__
			from-=TTimeIntervalDays(7);
#else
			//from-=TTimeIntervalSeconds(5);
			from-=TTimeIntervalDays(7);
#endif
			CC_TRAPD(err, iStorage.RemovePostsL( TInt64(9), from, CPostStorage::EByUpdated));
			if (err!=KErrNone) {
				TBuf<100> msg=_L("Could not purge the Trash folder (error ");
				msg.AppendNum(err); msg.Append(_L(")"));
				Reporting().UserErrorLog(msg);
				auto_ptr<HBufC> stack( CallStackMgr().GetFormattedCallStack(KNullDesC) );
				Reporting().UserErrorLog(*stack);
				Reporting().ShowGlobalNote( EAknGlobalErrorNote, msg);
			}
		}
		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("DeleteStupidThreads"));
			TInt threads[] = { 8, -1 };
			for (TInt* t=threads; *t>0; t++) {
				CCMPost* p=0;
				TRAPD(err, p=iStorage.GetByPostIdL(0, TInt64(*t)) );
				if (p) {
					refcounted_ptr<CCMPost> ph(p);
					TRAP(err, iStorage.RemovePostL(p));
				}
			}
		}
		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("CreateThreads"));

			TThreadInfo threads[]= {
				{ CPostStorage::ERoot, R_ALL },
				{ CPostStorage::EDrafts, R_DRAFTS },
				{ CPostStorage::EQueue, R_QUEUE },
				{ CPostStorage::EFailed, R_FAILED },
				{ CPostStorage::EPublished, R_PUBLISHED },
				{ CPostStorage::ENotPublished, R_NOT_PUBLISHED },
				{ CPostStorage::ETrash, R_TRASH },
				{ -1, 0 }
			};
			TThreadInfo* t;
			for (t=threads; t->iId>=0; ++t) {
				CCMPost* p=0;
				CC_TRAPD(err, p=iStorage.GetByPostIdL(0, t->iId));
				if (err!=KErrNone && err!=KErrNotFound) User::Leave(err);
				if (p 
						&& TInt64(t->iId)==iStorage.MediaPoolId()
						&& p->iParentId()!=TInt64(0)) {
					p->Release(); p=0;
				}
				auto_ptr<HBufC> name( CEikonEnv::Static()->AllocReadResourceL(t->iResourceId) );
				if (!p) {
					auto_ptr<CFbsBitmap> icon(0);
					p=CCMPost::NewL(BBDataFactory());
					refcounted_ptr<CCMPost> pp(p);
					p->iBodyText->Append( *name );
					p->iParentId()=TInt64(0);
					p->iPostId()=TInt64(t->iId);
					p->iTimeStamp()=TTime(t->iId);
					iStorage.AddLocalL(p, icon);
				} else {
					refcounted_ptr<CCMPost> pp(p);
					if (p->iBodyText->Value().Compare( *name ) ) {
						p->iBodyText->Value().Zero();
						p->iBodyText->Append( *name );
						iStorage.UpdatePostL(p);
					}
				}
			}
		}

		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("CreatePostView"));
			iPostView = new (ELeave) CContextMediaAppThreadView(*this, KPostViewId, KNullUid,
						iStorage, CPostStorage::EDescending, CPostStorage::EByDate,
						CPostStorage::RootId(), ETrue, ETrue, *iPhoneHelper, *iNetwork, 0,
						ETrue, iTagIcons, iTransferer, this);

			iPostView->ConstructL();
			iAppUi->AddViewL( iPostView );		// transfer ownership to CAknViewAppUi
		}

		{
			CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("CreateThreadView"));
			TInt resource=0;
#ifdef FLICKR
			resource=R_CL_FLICKR_MEDIAVIEW;
#endif
			iThreadView = new (ELeave) CContextMediaAppThreadView(*this, 
																  KThreadsByLastPostViewId, 
																  KPostViewId,
																  iStorage, 
																  CPostStorage::EAscending, CPostStorage::EByDate,
																  CPostStorage::RootId(), 
																  ETrue, EFalse, 
																  *iPhoneHelper, 
																  *iNetwork, 
																  0, ETrue, iTagIcons, 0, 0, resource);
			auto_ptr<CContextMediaAppThreadView> p(iThreadView);
			iThreadView->ConstructL();
			iAppUi->AddViewL( iThreadView );      // transfer ownership to CAknViewAppUi
			p.release();
		}
	}

	CAknViewAppUi *iAppUi;
	MPresenceMaintainer *iPublisher;
#ifndef FLICKR
	TVwsViewId* iNextViewId;
#else
	TVwsViewId iNextViewId;
#endif
	CPostStorage &iStorage;
	CHttpTransfer2* iTransferer;
	CAknIconArray *iTagIcons;

	CMediaPromptImpl(CAknViewAppUi *aAppUi, MPresenceMaintainer* aPublisher, 
		TVwsViewId* NextViewId, MApp_context& Context,  
		CPostStorage &aStorage, CHttpTransfer2* aTransferer
		) : MContextBase(Context),
		iAppUi(aAppUi), iPublisher(aPublisher), 
#ifndef FLICKR
		iNextViewId(NextViewId), 
#endif
		iStorage(aStorage), iTransferer(aTransferer) { }

	CBBPresence*	iPresence;
	MUploadCallBack* iCallBack;
	phonehelper*	iPhoneHelper;
	CContextMediaAppThreadView* iPostView;

#ifdef FLICKR
	virtual TUid LastViewBeforePrompt() { return KThreadsByLastPostViewId; }
	virtual TVwsViewId NextView() { return iNextViewId; }
	virtual void SetNextView(TVwsViewId aViewId) { iNextViewId=aViewId; }
#else
	virtual TUid LastViewBeforePrompt() { return KStatusView; }
	virtual TVwsViewId NextView() { return *iNextViewId; }
	virtual void SetNextView(TVwsViewId aViewId) { *iNextViewId=aViewId; }
#endif
	virtual void SetTab(TInt tabId) { return; }

	virtual TUid MediaPoolViewId() { return KThreadsByLastPostViewId; }
	virtual void ShowMediaPoolL() {
		/*
		if (iStorage.FirstL(iStorage.MediaPoolId(), CPostStorage::EByDate, 
				CPostStorage::EDescending, EFalse)) {
			iAppUi->ActivateLocalViewL(iEditorView->Id());
		} */
#ifndef FLICKR
		iThreadView->SetPreviousView( TVwsViewId( TUid::Uid(0), KStatusView) );
#else
		iThreadView->SetPreviousView( TVwsViewId( TUid::Uid(0), TUid::Uid(0)) );
#endif
		iAppUi->ActivateLocalViewL(iThreadView->Id());
	}

	virtual void Prompt(const TDesC& FileName, MUploadCallBack* CallBack) {
		CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("Prompt"));
		delete iPresence; iPresence=0;
		iPresence=bb_cast<CBBPresence>(iPublisher->Data()->CloneL(KNullDesC));
		iCallBack=CallBack;
		iEditorView->SetPresence(iPresence); iPresence=0;
		iEditorView->Prompt(FileName, this);
	}
	virtual TFileOpStatus Back(bool Upload, bool DeleteFromPhone, MBBData* Packet) {
		TFileOpStatus ret;
		CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("Back"));
		if (!Packet) {
			return iCallBack->Back(Upload, DeleteFromPhone, Packet);
		}
		CCMPost* post=bb_cast<CCMPost>(Packet);
		ret=iCallBack->Back(Upload, DeleteFromPhone, Packet);
		TInt resource=0;
		TAknGlobalNoteType  notetype=EAknGlobalConfirmationNote;
		if (!Upload && DeleteFromPhone) {
			iStorage.RemovePostL(post);
		} else {
			if (ret.err==KErrNone) {
				post->iMediaUrl() = ret.fn;
				post->iMediaFileName() = ret.fn;
				iStorage.UpdatePostL(post);
				if (Upload) {
					resource=R_QUEUED_FOR_PUBLISH;
				} else {
					if (post->iParentId()==TInt64(CPostStorage::ETrash))
						resource=R_MOVED_TO_TRASH;
					else 
						resource=R_POSTPONED;
				}
			} else {
				if (Upload)
					resource=R_FAILED_TO_QUEUE;
				else
					resource=R_FAILED_GENERAL;
				notetype=EAknGlobalErrorNote;
			}
		}
 		if (resource!=0) {
			auto_ptr<CAknGlobalNote> note (CAknGlobalNote::NewL());
			auto_ptr<HBufC> text(CEikonEnv::Static()->AllocReadResourceL(resource));
			if (notetype==EAknGlobalErrorNote) {
				iStorage.UpdateError(post->iPostId(), ret.err, *text);
			}
			note->ShowNoteL(notetype, 
				*text);
		}
		iStorage.CommitL();

#ifndef FLICKR
		iAppUi->ActivateLocalViewL(KStatusView);
#else
		ShowMediaPoolL();
#endif
		return ret;
	}
	~CMediaPromptImpl() {
		delete iPresence;
		delete iNetwork;
		if (iCmuiResourceFile) CEikonEnv::Static()->DeleteResourceFile(iCmuiResourceFile);
		if (iPluginResourceFile) CEikonEnv::Static()->DeleteResourceFile(iPluginResourceFile);
		delete iPhoneHelper;
		delete iTagIcons;
		iTransferer->RemovePostCallback(this);

#ifdef CONTEXTMEDIA_AS_PLUGIN
		if (iThreadView) iAppUi->RemoveView( iThreadView->Id() );
		if (iPostView) iAppUi->RemoveView( iPostView->Id() );
		if (iEditorView) iAppUi->RemoveView( iEditorView->Id() );
#endif
	}
	virtual class CCMNetwork* GetDummyNetwork() {
		return iNetwork;
	}
	virtual class CAknView* MediaView() {
		return iThreadView;
	}

	virtual void InnerStateChangedL(CCMPost* aPost, TState aState, const TDesC& aFileName,
		const class CBBErrorInfo* aError) {

		CALLSTACKITEM_N(_CL("CMediaPromptImpl"), _CL("InnerStateChangedL"));
		if (aState==EDeleted) {
			iStorage.RemovePostL(aPost);
			return;
		}
		if (aFileName.Length()>0) {
			aPost->iMediaUrl() = aFileName;
			aPost->iMediaFileName() = aFileName;
		}
		if (aPost->iParentId() != TInt64( CPostStorage::ETrash )) {
			aPost->iParentId()=TInt64(aState);
		}
		iStorage.UpdatePostL(aPost);
	}
	virtual void StateChangedL(TInt64 aPostId, TState aState, const TDesC& aFileName,
		const class CBBErrorInfo* aError) {

		CCMPost* p=0;
		TInt err;
		CC_TRAP(err, iStorage.UpdateErrorInfo(aPostId, aError));
		if (err==KErrNotFound) return;
		User::LeaveIfError(err);

		TInt64 parent=iStorage.GetParentId(aPostId);
		if (aState!=EDeleted && parent==aState && aFileName.Length()==0) {
			// no change
			return;
		}

		CC_TRAP(err, p=iStorage.GetByPostIdL(0, aPostId));
		if (err==KErrNotFound) return;
		User::LeaveIfError(err);

		CC_TRAP(err, InnerStateChangedL(p, aState, aFileName, aError));
		iStorage.Release(p, 0);
		User::LeaveIfError(err);
	}

	virtual TBool NameCell(TInt aBaseId, const class TBBCellId& aCellId, TDes& aNameInto) {
		//FIXME3RD
		return EFalse;
	}
	virtual TBool NameCity(const class TBBCellId& aCellId, TDes& aNameInto) {
		//FIXME3RD
		return EFalse;
	}
	virtual TBool GetExistingCell(TInt aBaseId, const class TBBCellId& aCellId, TDes& aNameInto) {
		//FIXME3RD
		return EFalse;
	}
	virtual TBool GetExistingCity(const class TBBCellId& aCellId, TDes& aNameInto) {
		//FIXME3RD
		return EFalse;
	}

	friend class CMediaPrompt;
	friend class auto_ptr<CMediaPromptImpl>;
};

CMediaPrompt* CMediaPrompt::NewL(CAknViewAppUi *aAppUi,
				MPresenceMaintainer *aPublisher,
				 TVwsViewId* NextViewId, MApp_context& Context,  
	CPostStorage &aStorage, CHttpTransfer2* aTransferer)
{
	auto_ptr<CMediaPromptImpl> ret(new (ELeave) CMediaPromptImpl(aAppUi, aPublisher,
		NextViewId,
		Context, aStorage, aTransferer));
	ret->ConstructL();
	return ret.release();
}
