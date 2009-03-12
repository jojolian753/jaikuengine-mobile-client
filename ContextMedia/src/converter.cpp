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
#include "converter.h"
#include "symbian_auto_ptr.h"
#include "cn_http.h"
#include "cl_settings.h"
#include "util.h"

#ifndef __S60V2__
#include <MdaImageConverter.h>
#else
#include <BitmapTransforms.h>
#include <ImageConversion.h>
#endif

#include "raii_d32dbms.h"
#include "cbbsession.h"
#include "fetch_request.h"
#include "independent.h"
#include <basched.h>
#include "bb_settings.h"
#include "transfer2.h"
#include "reporting.h"
#include "notifystate.h"
#include <contextnetwork.mbg>
#include "compat_int64.h"

#define MAX_ERRORS 5

#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\contextnetwork.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\contextnetwork.mbm");
#endif

class CConvertingDownloaderImpl : public CConvertingDownloader, 
	public MContextBase, public MDBStore, public MDownloadObserver
#ifndef __S60V2__
	, public MMdaImageUtilObserver
#endif
	{

	CConvertingDownloaderImpl(MApp_context& aContext, RDbDatabase& Db, MConvertingDownloadObserver &aObserver);
	void ConstructL(const TDesC& aDirectory);
	~CConvertingDownloaderImpl();

	virtual void DownloadL(const CBBFetchMediaRequest * aReq);

	// MBBObserver 
//	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
//		const TComponentName& aComponentName, const MBBData* aData);


	friend class CConvertingDownloader;
	friend class auto_ptr<CConvertingDownloaderImpl>;

	virtual void DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
		const TDesC& aContentType);
	virtual void DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr);

	virtual void MiuoConvertComplete(TInt aError);
	virtual void MiuoConvertCompleteL(TInt aError);
	virtual void MiuoCreateComplete(TInt aError);
	virtual void MiuoOpenComplete(TInt aError);

	void CheckedRunL();
	void DoCancel();

	void Reset();
	void StartConvert();
	void Async(TInt aSeconds=0);
	TBool MarkError(TInt aError, const TDesC& aDescr);
	TBool SeekToRequestId(TInt64 aId);
	virtual void RemoveRequest(TInt64 aRequestId);
	void DoScaleL(TSize aSize);

	enum TState { EIdle, EFetching, EConverting, EScaling };
	TState iCurrentState;

	enum TColumns {
		ERequestId = 1,
		EPriority,
		EFilename,
		EErrors,
		EErrorCode,
		EErrorDescr,
		EFinished,
		EIcon
	};
	enum TIndices {
		ERequestIdx = 0,
		EPriorityIdx
	};

	MConvertingDownloadObserver& iObserver;
	TInt		iPriority;
	TFileName	iDirectory;
	TBuf<255>	iMsgBuf;
	CDownloader*	iDownloader;

	TFileName	iCurrentFilename, iCurrentContentType;
	TInt64		iCurrentRequestId;

	TBool		iReseting;
	RTimer		iTimer; TBool iTimerIsOpen;

	RFbsSession	iFbsSession; TBool iFbsSessionIsOpen;
#ifndef __S60V2__
	CMdaImageFileToBitmapUtility*	iFileUtil;
	CMdaBitmapScaler*		iScaler;
#else
	CImageDecoder*			iFileUtil;
	CBitmapScaler*			iScaler;
#endif

	CFbsBitmap	*iBitmap, *iIcon;

	//CBBSubSession * iBBSubSession;
	
#ifndef __S60V2__
	CMdaServer*	iMdaServer;
#else

	CImageDecoder* iImageDecoder;
#endif
	CNotifyState*	iNotifyState;
};

EXPORT_C CConvertingDownloader* CConvertingDownloader::NewL(MApp_context& aContext, RDbDatabase& Db, const TDesC& aDirectory, MConvertingDownloadObserver& aObserver)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloader"), _CL("NewL"));

	auto_ptr<CConvertingDownloaderImpl> ret(new (ELeave) CConvertingDownloaderImpl(aContext, Db, aObserver));
	ret->ConstructL(aDirectory);
	return ret.release();
}

_LIT(KDownloader, "CConvertingDownloader");

CConvertingDownloader::CConvertingDownloader() : CCheckedActive(EPriorityIdle, KDownloader) { }

CConvertingDownloaderImpl::CConvertingDownloaderImpl(MApp_context& aContext, RDbDatabase& Db, 
				 MConvertingDownloadObserver& aObserver) : MContextBase(aContext), MDBStore(Db),
				iObserver(aObserver) { }

void CConvertingDownloaderImpl::ConstructL(const TDesC& aDirectory)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("ConstructL"));

	//iBBSubSession=BBSession()->CreateSubSessionL(this);
	//iBBSubSession->AddNotificationL(KFetchMediaRequestTuple);

	iDirectory=aDirectory;

	if (iDirectory.Right(1).Compare(_L("\\"))) iDirectory.Append(_L("\\"));
	TInt err=Fs().MkDirAll(iDirectory);
	if (err!=KErrNone && err!=KErrAlreadyExists) User::Leave(err);

	TInt cols[]={ EDbColInt64, EDbColInt32, EDbColText, EDbColInt32, EDbColInt32, 
		EDbColText, EDbColInt32, EDbColLongBinary, -1 };
	TInt idx[]={ ERequestId, -2, EPriority, -1 };
	SetTextLen(255);

	MDBStore::ConstructL(cols, idx, EFalse, _L("conversion"));

	SwitchIndexL(EPriorityIdx);
	if (iTable.LastL()) {
		iTable.GetL();
		iPriority=iTable.ColInt(EPriority)+1;
	} else {
		iPriority=1;
	}
	iCurrentState=EIdle;
	User::LeaveIfError(iTimer.CreateLocal()); iTimerIsOpen=ETrue;
	CActiveScheduler::Add(this);
	iDownloader=CDownloader::NewL(AppContext(), iDb, iDirectory, *this);

	iFbsSession.Connect(); iFbsSessionIsOpen=ETrue;

	Async();
}

CConvertingDownloaderImpl::~CConvertingDownloaderImpl()
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("~CConvertingDownloaderImpl"));

	Cancel();
	if (iTimerIsOpen) iTimer.Close();
	Reset();
	delete iDownloader;
#ifndef __S60V2__
	delete iMdaServer;
#endif
	if (iFbsSessionIsOpen) iFbsSession.Disconnect();
	delete iNotifyState;
	//delete iBBSubSession;
}

void CConvertingDownloaderImpl::Async(TInt aSeconds)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("Async"));

	if (iCurrentState==EIdle && !IsActive()) {
		if (aSeconds==0) {
			TRequestStatus *s=&iStatus;
			User::RequestComplete(s, KErrNone);
		} else {
			TTimeIntervalMicroSeconds32 w(aSeconds*1000*1000);
			iTimer.After(iStatus, w);
		}
		SetActive();
	}
}

void CConvertingDownloaderImpl::DownloadL(const CBBFetchMediaRequest * aReq)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("DownloadL"));

	TInt iap=-1;
	if ( !Settings().GetSettingL(aReq->iIAPSetting(), iap ) || iap == -1) {
		//auto_ptr<CBBFetchMediaRequest> reply (CBBFetchMediaRequest::NewL(0, iCurrentRequestId, KNullDesC));
		//reply->iErrorDescr() = _L("access point error");
		//reply->iErrorCode() = KErrCouldNotConnect;
		//iBBSubSession->PutReplyL(KFetchMediaRequestTuple, KNullDesC, reply.get(), KContextMediaComponent);
		User::Leave(KErrCouldNotConnect);
	} 
	if ( NoSpaceLeft() ) {
		User::Leave(KErrDiskFull);
	}
	// FIXME!
	iDownloader->SetFixedIap(iap);
	
	if (aReq->iTargetUrl().Length()<4) return;

	TBool found=SeekToRequestId(aReq->iPostId());
	if (found) {
		iTable.UpdateL();
		if (aReq->iForce()) {
			iTable.SetColL(EErrors, 0);
		}
		iTable.SetColL(EPriority, ++iPriority);
		PutL();
		Async();
	} else {
		Reporting().DebugLog(_L("CConvertingDownloaderImpl::DownloadL"));
		Reporting().DebugLog(aReq->iTargetUrl());
		iDownloader->DownloadL(aReq->iPostId(), aReq->iTargetUrl(), aReq->iForce());
	}
}

void CConvertingDownloaderImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("CheckedRunL"));
	TAutoBusy busy(Reporting());

	if (iCurrentState==EConverting) {
		MiuoConvertComplete(iStatus.Int());
		return;
	} else if (iCurrentState==EScaling) {
		MiuoConvertComplete(iStatus.Int());
		return;
	}
	SwitchIndexL(EPriorityIdx);
	TBool found=iTable.LastL();

	TBool requests_left=EFalse;

	while (found) {
		iTable.GetL();
		if (iTable.ColInt(EFinished)) {
			iCurrentRequestId=iTable.ColInt64(ERequestId);
			iCurrentFilename=iTable.ColDes(EFilename);

			auto_ptr<CFbsBitmap> bitmap(new (ELeave) CFbsBitmap);
			{
				RADbColReadStream s; s.OpenLA(iTable, EIcon);
				bitmap->InternalizeL(s);
			}
			Async();

			TBuf<40> mime;
			GetMimeTypeL(iCurrentFilename, mime);
			CC_TRAPD(err, iObserver.DownloadFinished(iCurrentRequestId, iCurrentFilename, mime, bitmap));
			if (err!=KErrNone) {
				MarkError(err, _L("error notifying"));
			} else {
				iTable.DeleteL();
			}
			/*
			auto_ptr<CBBFetchMediaRequest> reply (CBBFetchMediaRequest::NewL(0, iCurrentRequestId, KNullDesC, EFalse, bitmap.get()));
			reply->iFileName() = fn;
			reply->iContentType() = KFbsImage;
			iBBSubSession->PutReplyL(KFetchMediaRequestTuple, KNullDesC, reply.get(), KContextMediaComponent);
			iTable.DeleteL();
			return;*/
		}
		if (iTable.ColInt(EErrors) <= MAX_ERRORS) {
			iCurrentRequestId=iTable.ColInt64(ERequestId);
			iCurrentFilename=iTable.ColDes(EFilename);
			CC_TRAPD(err, StartConvert());
			if (err!=KErrNone) {
				iCurrentState=EIdle;
				if (MarkError(err, _L("error starting download")))
					requests_left=ETrue;
				Reset();
			} else {
				break;
			}
		} 
		found=iTable.PreviousL();
	}
	if (!found) {
		Reset();
		if (requests_left) {
			Async(10);
		} else {
			//ShowDownloading(EFalse);
		}
	} else {
		//ShowDownloading(ETrue);
	}
}

void CConvertingDownloaderImpl::StartConvert()
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("StartConvert"));

	iCurrentState=EFetching;
	iReseting=EFalse;

	delete iBitmap; iBitmap=0;
	delete iIcon; iIcon=0;

	if (! iNotifyState ) {
		iNotifyState=CNotifyState::NewL(AppContext(), KIconFile);
		iNotifyState->SetCurrentState(
			EMbmContextnetworkConverting, EMbmContextnetworkConverting);
	}
	
#ifndef __S60V2__
	if (!iFileUtil) {
		if (!iMdaServer) iMdaServer=CMdaServer::NewL();
		iMdaServer->SetPriority(EPriorityIdle);
		iFileUtil=CMdaImageFileToBitmapUtility::NewL(*this, iMdaServer);
		iFileUtil->SetPriority(EPriorityIdle);
	}
	iFileUtil->OpenL(iCurrentFilename);
#else
	delete iFileUtil; iFileUtil=0;
	CC_TRAPD(err, iFileUtil=CImageDecoder::FileNewL(Fs(), iCurrentFilename, 
		CImageDecoder::EOptionAlwaysThread));
	MiuoOpenComplete(err);
#endif		
}


TBool CConvertingDownloaderImpl::SeekToRequestId(TInt64 aId)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("SeekToRequestId"));

	SwitchIndexL(ERequestIdx);
	TDbSeekKey k(aId);
	return iTable.SeekL(k);
}

void CConvertingDownloaderImpl::Reset()
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("Reset"));

	delete iNotifyState; iNotifyState=0;
	
	iReseting=ETrue;
	delete iBitmap; iBitmap=0;
	delete iIcon; iIcon=0;
#ifndef __S60V2__
	if (iFileUtil) 
		iFileUtil->Close();
#endif
	delete iFileUtil; iFileUtil=0;
	delete iScaler; iScaler=0;
	iCurrentState=EIdle;
	iReseting=EFalse;
	return;
}

TBool CConvertingDownloaderImpl::MarkError(TInt aError, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("MarkError"));

	iCurrentState=EIdle;
	iMsgBuf=aDescr;
	iMsgBuf.Append(_L("(file: "));
	iMsgBuf.Append(iCurrentFilename.Left(254-iMsgBuf.Length()) );
	iMsgBuf.Append(_L(")"));

	TBool ret=ETrue;
	if (SeekToRequestId(iCurrentRequestId)) {
		iTable.GetL();
		TInt count=iTable.ColInt(EErrors)+1;
		if (count>MAX_ERRORS) {
			//USE BB HERE
			//auto_ptr<CBBFetchMediaRequest> reply (CBBFetchMediaRequest::NewL(0, iCurrentRequestId, KNullDesC));
			//reply->iErrorDescr() = iMsgBuf;
			//reply->iErrorCode() = aError;
			//iBBSubSession->PutReplyL(KFetchMediaRequestTuple, KNullDesC, reply.get(), KContextMediaComponent);
			iObserver.DownloadError(iCurrentRequestId, aError, iMsgBuf);
			ret=EFalse;
		} 
		iTable.UpdateL();
		iTable.SetColL(EErrors, count);
		iTable.SetColL(EErrorCode, aError);
		iTable.SetColL(EErrorDescr, iMsgBuf);
		PutL();
	} else {
		iObserver.DownloadError(iCurrentRequestId, KErrGeneral, _L("internal error: row disappeared"));
		ret=EFalse;
	}
	return ret;
}

void CConvertingDownloaderImpl::RemoveRequest(TInt64 aRequestId)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("RemoveRequest"));

	RDebug::Print(_L("Remove request"));

	iDownloader->RemoveRequest(aRequestId);
	if (SeekToRequestId(aRequestId)) {
		iTable.DeleteL();
	}
}

void CConvertingDownloaderImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("DoCancel"));

#ifdef __S60V2__
	if (iCurrentState==EConverting) {
		iFileUtil->Cancel();
	} else if (iCurrentState==EScaling) {
		iScaler->Cancel();
	} else 
#endif
	{
		if (iStatus==KRequestPending)
			iTimer.Cancel();
	}
}

void CConvertingDownloaderImpl::MiuoConvertComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("MiuoConvertComplete"));

	CC_TRAPD(err, MiuoConvertCompleteL(aError));
	if (err!=KErrNone) {
		MarkError(aError, _L("error converting image"));
		Async(10);
	}
}

void CConvertingDownloaderImpl::DoScaleL(TSize aSize)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("DoScaleL"));

#ifndef __S60V2__
	if (!iScaler) iScaler=CMdaBitmapScaler::NewL();
	iIcon=iBitmap; iBitmap=0;
	iScaler->ScaleL(*this, *iIcon,
		aSize);
#else
	if (!iScaler) iScaler=CBitmapScaler::NewL();
	iIcon=iBitmap; iBitmap=0;
	iCurrentState=EScaling;
	iScaler->Scale(&iStatus, *iIcon, aSize);
	SetActive();
#endif
}

void CConvertingDownloaderImpl::MiuoConvertCompleteL(TInt aError)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("MiuoConvertCompleteL"));

#ifdef __WINS__
	RDebug::Print(_L("CConvertingDownloaderImpl::MiuoConvertComplete"));
#endif

	User::LeaveIfError(aError);
	if (!iIcon) {
		TFileName fn=iCurrentFilename;
		if (!SeekToRequestId(iCurrentRequestId)) User::Leave(KErrNotFound);
		iTable.UpdateL();
		iTable.SetColL(EFilename, fn);
		PutL();
		DoScaleL(TSize(48,36));
	} else {
		if (!SeekToRequestId(iCurrentRequestId)) User::Leave(KErrNotFound);

		iTable.GetL();
		iTable.UpdateL();
		TFileName fn=iTable.ColDes(EFilename);
		TBuf<40> mime;
		GetMimeTypeL(fn, mime);
		iTable.SetColL(EFinished, 1);
		{
			RADbColWriteStream w; w.OpenLA(iTable, EIcon);
			iIcon->ExternalizeL(w);
			w.CommitL();
		}
		PutL();

		CFbsBitmap* temp=iIcon;
		iIcon=iBitmap=0;
		auto_ptr<CFbsBitmap> icon(temp);
		
#ifdef __WINS__
		RDebug::Print(_L("conversion done for h:%d l:%d"), I64HIGH(iCurrentRequestId), 
			I64LOW(iCurrentRequestId));
		RDebug::Print(fn);
		RDebug::Print(mime);
#endif

		//USE BB HERE
		//auto_ptr<CBBFetchMediaRequest> reply (CBBFetchMediaRequest::NewL(0, iCurrentRequestId, KNullDesC, EFalse, icon.get()));
		//reply->iFileName() = fn;
		//reply->iContentType() = KFbsImage;
		//iBBSubSession->PutReplyL(KFetchMediaRequestTuple, KNullDesC, reply.get(), KContextMediaComponent);
		
		delete iNotifyState; iNotifyState=0;
		iObserver.DownloadFinished(iCurrentRequestId, fn, mime, icon);
		if (SeekToRequestId(iCurrentRequestId)) {
			iTable.DeleteL();
		}
		iCurrentState=EIdle;
		Async();
	}
}

void CConvertingDownloaderImpl::MiuoCreateComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("MiuoCreateComplete"));

#ifdef __WINS__
	RDebug::Print(_L("CConvertingDownloaderImpl::MiuoCreateComplete"));
#endif

	if (aError!=KErrNone) {
		MarkError(aError, _L("error creating"));
		Async(10);
	}
}

void CConvertingDownloaderImpl::DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
						 const TDesC& aContentType)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("DownloadFinished"));

	Reporting().DebugLog(_L("CConvertingDownloaderImpl::DownloadFinished"));
	Reporting().DebugLog(aFileName);

	if (aContentType.Left(5).CompareF(_L("image"))==0) {
		iTable.InsertL();
		iTable.SetColL(ERequestId, aRequestId);
		iTable.SetColL(EPriority, ++iPriority);
		iTable.SetColL(EFilename, aFileName);
		iTable.SetColL(EErrors, 0);
		iTable.SetColL(EErrorCode, 0);
		PutL();

		Async();
	} else {
		//USE BB HERE
		//auto_ptr<CBBFetchMediaRequest> reply (CBBFetchMediaRequest::NewL(0, aRequestId, KNullDesC));
		//reply->iFileName()=aFileName;
		//reply->iContentType()=aContentType;
		//iBBSubSession->PutReplyL(KFetchMediaRequestTuple, KNullDesC, reply.get(), KContextMediaComponent);
		((MDownloadObserver&)iObserver).DownloadFinished(aRequestId, aFileName, aContentType);
	}
}

void CConvertingDownloaderImpl::DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("DownloadError"));

	//auto_ptr<CBBFetchMediaRequest> reply (CBBFetchMediaRequest::NewL(0, aRequestId, KNullDesC));
	//reply->iErrorDescr() = aDescr;
	//reply->iErrorCode() = aCode;
	//iBBSubSession->PutReplyL(KFetchMediaRequestTuple, KNullDesC, reply.get(), KContextMediaComponent);
			
	iObserver.DownloadError(aRequestId, aCode, aDescr);
}


void CConvertingDownloaderImpl::MiuoOpenComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("MiuoOpenComplete"));

#ifdef __WINS__
	RDebug::Print(_L("CConvertingDownloaderImpl::MiuoOpenComplete"));
#endif

	if (aError!=KErrNone) {
		MarkError(aError, _L("error opening image"));
		iCurrentState=EIdle;
		Async(10);
	} else {
		TFrameInfo frameInfo;
#ifndef __S60V2__
		iFileUtil->FrameInfo(0, frameInfo);
#else
		frameInfo=iFileUtil->FrameInfo(0);
#endif
		delete iBitmap; iBitmap=0;
		iBitmap=new CFbsBitmap;
		if (!iBitmap) {
			MarkError(KErrNoMemory, _L("cannot create bitmap"));
			Async(10);
		} else {
			iBitmap->Create(frameInfo.iOverallSizeInPixels, EColor64K);
#ifndef __S60V2__
			CC_TRAPD(err, iFileUtil->ConvertL(*iBitmap));
			if (err!=KErrNone) {
				MarkError(KErrNoMemory, _L("cannot create bitmap"));
				Async(10);
			}
#else
			iCurrentState=EConverting;
			iFileUtil->Convert(&iStatus, *iBitmap);
			SetActive();
#endif
		}
	}
}
/*
// MBBObserver
void CConvertingDownloaderImpl::NewValueL(TUint aId, const TTupleName& aName, const TDesC& , 
		const TComponentName& , const MBBData* aData)

{
	CALLSTACKITEM_N(_CL("CConvertingDownloaderImpl"), _CL("NewValueL"));

	if (aName==KFetchMediaRequestTuple) {
		const CBBFetchMediaRequest *req=bb_cast<CBBFetchMediaRequest>(aData);
		if (req) DownloadL(req);
		iBBSubSession->DeleteL(aId);
	}
}

class COwnActiveScheduler : public CBaActiveScheduler {
public:
	void Error(TInt aError) const {
		User::Leave(aError);
	}
};

class CStopActive : public CActive {
public:
	CStopActive() : CActive(EPriorityNormal) { }
	void ConstructL(worker_info *wi) { 
		CActiveScheduler::Add(this);
		wi->set_do_stop(&iStatus);
		SetActive();
	}
	void RunL() {
		CActiveScheduler::Stop();
	}
	void DoCancel() {
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrNone);
	}
	~CStopActive() {
		Cancel();
	}
};

void do_run_converter(TAny* aPtr)
{
	worker_info *wi=(worker_info*)aPtr;

	TConverterArgs *args=(TConverterArgs*)wi->worker_args;

	auto_ptr<COwnActiveScheduler> s(new (ELeave) COwnActiveScheduler);
	CActiveScheduler::Install(s.get());
	auto_ptr<CStopActive> stop(new (ELeave) CStopActive);
	stop->ConstructL(wi);

	auto_ptr<CApp_context> appc(CApp_context::NewL(true, _L("converter")));
	appc->SetDataDir(_L("c:\\system\\apps\\contextmediaapp\\"), false);
	TNoDefaults t;
	CBlackBoardSettings* settings=
		CBlackBoardSettings::NewL(*appc, t, KCLSettingsTuple);
	appc->SetSettings(settings);

	auto_ptr<CBBDataFactory> bbf(CBBDataFactory::NewL());
	auto_ptr<CBBSession> bbs(CBBSession::NewL(*appc, bbf.get()));
	appc->SetBBSession(bbs.get());
	appc->SetBBDataFactory(bbf.get());

	auto_ptr<CDb> db(CDb::NewL(*appc, _L("MEDIA_CONVERTION"), EFileRead|EFileWrite|EFileShareAny));

	auto_ptr<CConvertingDownloader> b(CConvertingDownloader::NewL(*appc, db->Db(), args->iDir));

	s->Start();
}

EXPORT_C TInt CConvertingDownloader::RunConverterInThread(TAny* aPtr)
{
        CTrapCleanup *cl;
        cl=CTrapCleanup::New();

        TInt err=0;
        CC_TRAP(err, do_run_converter(aPtr));

	delete cl;

	TTimeIntervalMicroSeconds32 w(50*1000);
	User::After(w);
	worker_info* wi=(worker_info*)aPtr;
	wi->stopped(err);
        return err;
}
*/
