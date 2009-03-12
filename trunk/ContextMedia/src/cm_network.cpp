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
#include "cm_network.h"

#include "fetch_request.h"
#include "fetch.h"
#include "converter.h"
#include "timeout.h"
#include "symbian_auto_ptr.h"
#include <bautils.h>
#include <flogger.h>
#include <contextmedia.mbg>
#include "cbbsession.h"
#include "independent.h"
#include "notifystate.h"

#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\contextmedia.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\contextmedia.mbm");
#endif

class CCMNetworkImpl : public CCMNetwork, public MContextBase, 
	public MConvertingDownloadObserver, public MPostReceiver,
	public MTimeOut {
public:
	CCMNetworkImpl(CPostStorage* aStorage, MApp_context& aContext);
	void ConstructL(const TDesC& aDir, const TDesC& aFetchUrl, TInt aPeriod, TInt aIapSetting);
	~CCMNetworkImpl();

	virtual void SetFetchUrl(const TDesC& aFetchUrl);
	virtual void SetFetchPeriod(TInt aSeconds);
	virtual void SetIap(TInt aIapSetting);

	virtual TBool FetchThread(TInt64 aThreadId);
	virtual void FetchMedia(const CCMPost* aPost, TBool force);
	virtual void FetchThreads();

	virtual void AddStatusListener(MNetworkStatus* aListener);
	virtual void RemoveStatusListener(MNetworkStatus* aListener);
	virtual MNetworkStatus::TFetchStatus GetFetchStatus(TInt64 aThreadId);
	virtual TBool GetDownloadingStatus();
	virtual void ShowDownloading(TBool aDownloading);

	// MPostReceiver
	virtual void NewPost(CCMPost* aPost, TBool aErrors); // may AddRef()
	virtual void Error(TInt aCode, const TDesC& aMsg);
	virtual void Finished();

	// MConvertingDownloadObserver
	virtual void DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
		const TDesC& aContentType);
	virtual void DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr);
	virtual void DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
		const TDesC& aContentType, auto_ptr<CFbsBitmap>& aIcon);

	// MTimeOut
	virtual void expired(CBase* aSource);

	// own
	void DoFetchThreads();
	void NotifyThreadStatusChange(TInt64 aThreadId, MNetworkStatus::TFetchStatus aStatus);

	CPostStorage*	iStorage;
	CFetchPosts*	iFetcher;
	CConvertingDownloader*	iConverter;
	CTimeOut*	iTimer;
	TFileName	iFetchUrl;
	TInt		iFetchPeriod;
	TInt		iIapSetting;

	TBuf<256>	iErrorDescr;
	TInt		iErrorCode;
	TInt64		iCurrentlyFetchThreadId;

	TBool		iBusy;

	CDb * iConverterDb;
	CList<MNetworkStatus*>	*iObservers;
	CList<TInt64>		*iThreadsToBeFetched;
	CList<TInt64>::Node	*iCurrentFetchNode;
	TBool   iPostFetcherDownloading;
	MNetworkStatus::TFetchStatus iFetchStatus;
	TInt64	iCurrentFetchThread;

	CNotifyState*	iNotifyState;

	friend class CCMNetwork;
	friend class auto_ptr<CCMNetworkImpl>;
};


CCMNetworkImpl::CCMNetworkImpl(CPostStorage* aStorage, MApp_context& aContext) 
	: MContextBase(aContext), iStorage(aStorage)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("CCMNetworkImpl"));

}

void CCMNetworkImpl::ConstructL(const TDesC& aDir, const TDesC& aFetchUrl, 
				TInt aPeriod, TInt aIapSetting)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("ConstructL"));

	iFetcher=CFetchPosts::NewL(AppContext(), *this);

	TFileName dir=aDir;
	if (HasMMC(Fs())) {
		dir.Replace(0, 1, _L("e"));
	}
	BaflUtils::EnsurePathExistsL(Fs(), dir);
	iConverterDb = CDb::NewL(AppContext(), _L("MEDIA_CONVERTION"), EFileRead|EFileWrite|EFileShareAny);

	iConverter=CConvertingDownloader::NewL(AppContext(), iConverterDb->Db(), dir, *this);

	iTimer=CTimeOut::NewL(*this);
	iFetchUrl=aFetchUrl;
	iFetchPeriod=aPeriod;
	iIapSetting=aIapSetting;
	if (iFetchPeriod>0) {
		iTimer->Wait(5);
	}
	iObservers=CList<MNetworkStatus*>::NewL();
	iThreadsToBeFetched=CList<TInt64>::NewL();
}

CCMNetworkImpl::~CCMNetworkImpl()
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("~CCMNetworkImpl"));

	//delete iBBSubSession;
	delete iNotifyState;
	delete iTimer;
	delete iConverter;
	delete iFetcher;
	delete iConverterDb;
	delete iThreadsToBeFetched;
	delete iObservers;
}

EXPORT_C CCMNetwork* CCMNetwork::NewL(CPostStorage* aStorage, MApp_context& aContext,
	const TDesC& aFetchUrl, TInt aFetchPeriod, const TDesC& aDir, TInt aIapSetting)
{
	CALLSTACKITEM_N(_CL("CCMNetwork"), _CL("NewL"));

	auto_ptr<CCMNetworkImpl> ret(new (ELeave) CCMNetworkImpl(aStorage, aContext));
	ret->ConstructL(aDir, aFetchUrl, aFetchPeriod, aIapSetting);
	return ret.release();
}

void CCMNetworkImpl::AddStatusListener(MNetworkStatus* aListener)
{
	for ( CList<MNetworkStatus*>::Node* n=iObservers->iFirst; n; n=n->Next ) {
		if (n->Item==aListener) {
			return;
		}
	}
	iObservers->AppendL(aListener);
}

void CCMNetworkImpl::RemoveStatusListener(MNetworkStatus* aListener)
{
	for ( CList<MNetworkStatus*>::Node* n=iObservers->iFirst; n; n=n->Next ) {
		if (n->Item==aListener) {
			iObservers->DeleteNode(n, ETrue);
			return;
		}
	}
}

MNetworkStatus::TFetchStatus CCMNetworkImpl::GetFetchStatus(TInt64 aThreadId)
{
	switch(iFetchStatus) {
	case MNetworkStatus::EConnecting:
		if (iCurrentlyFetchThreadId==0) {
			return iFetchStatus;
		} else {
			if (iCurrentlyFetchThreadId==aThreadId) {
				return iFetchStatus;
			} else {
				return MNetworkStatus::EDone;
			}
		}

	case MNetworkStatus::EDone:
		return iFetchStatus;
	case MNetworkStatus::EFetching:
		if (aThreadId==iCurrentFetchThread) return MNetworkStatus::EFetching;
		if (aThreadId<iCurrentFetchThread) return MNetworkStatus::EDone;
		return MNetworkStatus::EWaiting;
	case MNetworkStatus::EFailed:
		if (aThreadId<iCurrentFetchThread) return MNetworkStatus::EDone;
		return MNetworkStatus::EFailed;
	default:
		return MNetworkStatus::EDone;
	}
}

TBool CCMNetworkImpl::GetDownloadingStatus()
{
	return (iNotifyState!=0);
}

TBool CCMNetworkImpl::FetchThread(TInt64 aThreadId)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("FetchThread"));

	iBusy=EFalse;
	
	iErrorCode=KErrNone;
	iErrorDescr=KNullDesC;
	
	iCurrentlyFetchThreadId=aThreadId;

	TInt64 lastpost=iStorage->GetLastPostId(aThreadId);

	auto_ptr<CBBFetchItemList> list (CBBFetchItemList::NewL());
	bb_auto_ptr<TBBFetchItem> item (new (ELeave) TBBFetchItem(aThreadId, lastpost));
	list->AddItemL(item.release());

	auto_ptr<CBBFetchPostRequest> req (CBBFetchPostRequest::NewL(iIapSetting, iFetchUrl));
	req->SetFetchItemList(list.get());

	CC_TRAPD(err2, iFetcher->FetchL(req.get()) );

	if (err2!=KErrNone) return EFalse;

	TInt ret=EFalse;
	CC_TRAPD(err, ret=iStorage->IsPlaceHolderL(iCurrentlyFetchThreadId));
	if ((err==KErrNotFound) || ret) {
		iStorage->AddPlaceHolderL(iCurrentlyFetchThreadId);
	}

	ShowDownloading(ETrue);
	iBusy=ETrue;

	iThreadsToBeFetched->reset();
	iThreadsToBeFetched->AppendL(aThreadId);
	iFetchStatus=MNetworkStatus::EConnecting;
	iCurrentFetchThread=aThreadId;
	iCurrentFetchNode=iThreadsToBeFetched->iFirst;
	NotifyThreadStatusChange(aThreadId, MNetworkStatus::EConnecting);
	return ETrue;
}

void CCMNetworkImpl::FetchMedia(const CCMPost* aPost, TBool force)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("FetchMedia"));

	if (force) RDebug::Print(_L("Forced FetchMedia"));
	auto_ptr<CBBFetchMediaRequest> req (CBBFetchMediaRequest::NewL(iIapSetting, 
		aPost->iPostId(), aPost->iMediaUrl(), force));
	iConverter->DownloadL(req.get());
}

void CCMNetworkImpl::SetFetchUrl(const TDesC& aFetchUrl)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("SetFetchUrl"));

	iFetchUrl=aFetchUrl;
}

void CCMNetworkImpl::SetIap(TInt aIapSetting)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("SetIap"));

	iIapSetting=aIapSetting;
}

void CCMNetworkImpl::SetFetchPeriod(TInt aSeconds)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("SetFetchPeriod"));

	iFetchPeriod=aSeconds;
}

// MPostReceiver
void CCMNetworkImpl::NewPost(CCMPost* aPost, TBool aErrors)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("NewPost"));

	if (aErrors) {
		aPost->iErrorDescr()=iErrorDescr;
		aPost->iErrorCode()=iErrorCode;
	}
	aPost->iUnreadCounter()=1;
	aPost->iMediaFileName().Zero();
	aErrors=EFalse;

	auto_ptr<CFbsBitmap> dummy(0);

	CC_TRAPD(err, iStorage->AddLocalL(aPost, dummy));
	if (err) {
		RDebug::Print(_L("Add Post error: %d"), err);
	} else {
		auto_ptr<CBBFetchMediaRequest> req (CBBFetchMediaRequest::NewL(iIapSetting, aPost->iPostId(), aPost->iMediaUrl(), false));
		CC_TRAPD(err, iConverter->DownloadL(req.get()));
		if (err!=KErrNone) {
			iStorage->UpdateError(aPost->iPostId(), err, _L("Cannot fetch media"));
		}
	}

	iCurrentFetchThread=aPost->iParentId();
	iFetchStatus=MNetworkStatus::EFetching;
	
	while (iCurrentFetchNode && iCurrentFetchNode->Item < aPost->iParentId() ) {
		NotifyThreadStatusChange(iCurrentFetchNode->Item, MNetworkStatus::EDone);
		iCurrentFetchNode=iCurrentFetchNode->Next;
	}
	NotifyThreadStatusChange(aPost->iParentId(), MNetworkStatus::EFetching);
}


void CCMNetworkImpl::Error(TInt aCode, const TDesC& aMsg)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("Error"));

	iErrorDescr=aMsg;
	iErrorCode=aCode;
}

void CCMNetworkImpl::Finished()
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("Finished"));

	iBusy=EFalse;
	ShowDownloading(EFalse);

	if (iErrorCode!=KErrNone) {
		TBool ok = iStorage->FirstL(iCurrentlyFetchThreadId, CPostStorage::EByDate, CPostStorage::EAscending, EFalse);
		if (!ok) {
			iStorage->AddErrorPlaceHolderL(iCurrentlyFetchThreadId);
		}
	} else {
		TBool ok = iStorage->FirstL(iCurrentlyFetchThreadId, CPostStorage::EByDate, CPostStorage::EAscending, EFalse);
		if (!ok) { 
			iStorage->AddNewThreadPlaceHolderL(iCurrentlyFetchThreadId);
		}
	}
	iFetchStatus=MNetworkStatus::EDone;
}

// MDownloadObserver
void CCMNetworkImpl::DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
	const TDesC& aContentType)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("DownloadFinished"));
	
	auto_ptr<CFbsBitmap> bm (new (ELeave) CFbsBitmap);

	if (aContentType.Left(5).CompareF(_L("video"))==0) {
		CGulIcon * icon = iStorage->IconArray()->At(EMbmContextmediaVideo);
		bm->Duplicate(icon->Bitmap()->Handle());
	} else if (aContentType.Left(5).CompareF(_L("audio"))==0) {
		CGulIcon * icon = iStorage->IconArray()->At(EMbmContextmediaAudio);
		bm->Duplicate(icon->Bitmap()->Handle());
	}
	DownloadFinished(aRequestId, aFileName, aContentType, bm);
}

void CCMNetworkImpl::DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
	const TDesC& aContentType, auto_ptr<CFbsBitmap>& aIcon)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("DownloadFinished"));

	TPckg<CFbsBitmap> xBitmap(*(aIcon.get()));
	RDebug::Print(_L("Size=%d"), xBitmap.Size());
	RDebug::Print(aContentType);

	iStorage->UpdateFileName(aRequestId, aFileName, aContentType);
	iStorage->UpdateIcon(aRequestId, aIcon);
}

void CCMNetworkImpl::DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("DownloadError"));

	iStorage->UpdateError(aRequestId, aCode, aDescr);
}

// MTimeOut
void CCMNetworkImpl::expired(CBase* /*aSource*/)
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("expired"));

	if (iFetchPeriod>0) iTimer->Wait(iFetchPeriod);
	if (!iBusy) {
		DoFetchThreads();
	}
}

void CCMNetworkImpl::FetchThreads()
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("FetchThreads"));

	iTimer->Reset();
	if (iFetchPeriod>0) iTimer->Wait(iFetchPeriod);
	if (!iBusy) {
		DoFetchThreads();
	}
}

void CCMNetworkImpl::DoFetchThreads()
{
	CALLSTACKITEM_N(_CL("CCMNetworkImpl"), _CL("DoFetchThreads"));

	iErrorCode=KErrNone;
	iErrorDescr=KNullDesC;

	auto_ptr< CList<TInt64> > threads(CList<TInt64>::NewL());

	TBool found=iStorage->FirstL(iStorage->RootId(), CPostStorage::EByDate, 
		CPostStorage::EDescending, 
		EFalse);
	while (found) {
		threads->AppendL(iStorage->GetCurrentIdL());
		found=iStorage->NextL();
	}

	iThreadsToBeFetched->reset();
	auto_ptr<CBBFetchItemList> items (CBBFetchItemList::NewL());
	for ( CList<TInt64>::Node* n=threads->iFirst; n; n=n->Next ) {
		bb_auto_ptr<TBBFetchItem> item (new (ELeave) TBBFetchItem(n->Item, iStorage->GetLastPostId(n->Item)));
		items->AddItemL(item.release() );
		TBool ret=EFalse;
		CC_TRAPD(err, ret=iStorage->IsPlaceHolderL(n->Item));
                if ((err==KErrNotFound) || ret) {
			iStorage->AddPlaceHolderL(n->Item);
		}
		iThreadsToBeFetched->AppendL(n->Item);
	}
	threads.reset();
	iCurrentFetchNode=iThreadsToBeFetched->iFirst;

	if (items->Count() > 0) {
		auto_ptr<CBBFetchPostRequest> req (CBBFetchPostRequest::NewL(iIapSetting, iFetchUrl));
		req->SetFetchItemList(items.get());
		
		CC_TRAPD(err, iFetcher->FetchL(req.get()) );
		if (err==KErrNone) {
			ShowDownloading(ETrue);
			iBusy=ETrue;
			iFetchStatus=MNetworkStatus::EConnecting;
			iCurrentFetchThread=TInt64(0);
			NotifyThreadStatusChange(iCurrentFetchThread, MNetworkStatus::EConnecting);
		} else {
			// do nothing? the timer will generate a retry
			// there is no indication of the error to the user though
		}
	}
}

void CCMNetworkImpl::ShowDownloading(TBool downloading)
{
	if (!downloading) {
		delete iNotifyState; iNotifyState=0;
	} else {
		if (! iNotifyState ) {
			iNotifyState=CNotifyState::NewL(AppContext(), KIconFile);
			iNotifyState->SetCurrentState(
				EMbmContextmediaThreaddownloading, EMbmContextmediaThreaddownloading);
		}
	}
	for ( CList<MNetworkStatus*>::Node* n=iObservers->iFirst; n; n=n->Next ) {
		n->Item->Downloading(downloading);
	}
}

void CCMNetworkImpl::NotifyThreadStatusChange(TInt64 aThreadId, MNetworkStatus::TFetchStatus aStatus)
{
#ifdef __WINS__
	TBuf<100> msg=_L("NotifyThreadStatusChange id ");
	msg.AppendNum(aThreadId);
	msg.Append(_L(" status "));
	msg.AppendNum(aStatus);
	RDebug::Print(msg);
#endif

	for ( CList<MNetworkStatus*>::Node* n=iObservers->iFirst; n; n=n->Next ) {
		n->Item->FetchStatusChange(aThreadId, aStatus);
	}
}

