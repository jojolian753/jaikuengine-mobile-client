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
#include "downloader.h"
#include "symbian_auto_ptr.h"
#include "cn_http.h"
#include "settings.h"
#include "cl_settings.h"
#include "reporting.h"
#include "notifystate.h"
#include <contextnetwork.mbg>
#include "compat_int64.h"
#include "db.inl"
#include "connectioninit.h"
#include "cc_stringtools.h"

#define MAX_ERRORS 5

static void AppendHttpErrorL(TInt aCode, TDes& b);


#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\contextnetwork.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\contextnetwork.mbm");
#endif

class CDownloaderImpl : public CDownloader, public MContextBase, public MDBStore, 
		public MHttpObserver, public MConnectivityCallback {

	CDownloaderImpl(MApp_context& aContext, RDbDatabase& Db, 
		MDownloadObserver& aObserver, HttpFactory aHttpFactory);
	void ConstructL(const TDesC& aDirectory, const TDesC& aConnectionName);
	~CDownloaderImpl();

	virtual void DownloadL(TInt64 aRequestId, const TDesC& aUrl, TBool aForce, TInt8 aLevel);
	virtual void SetLevelL(TInt8 aLevel);
	virtual TInt8 GetLevelL();
	virtual void SetDownloadLimitLevelL(TInt8 aLevel);
	virtual void SetNoDownloadLimitLevelL();
	virtual CDownloader::TDownloadable IsDownloadable(TInt64 aRequestId);

	friend class CDownloader;
	friend class auto_ptr<CDownloaderImpl>;

	virtual void NotifyHttpStatus(THttpStatus st, TInt aError);
	virtual void NotifyNewHeader(const CHttpHeader &aHeader);
	virtual void NotifyNewBody(const TDesC8 &chunk);
	
	virtual void ConnectivityStateChanged();

	void CheckedRunL();
	void DoCancel();

	void Reset();
	void StartDownload();
	void Async(TInt aSeconds=0);
	void GotAllL();
	TBool MarkError(TInt aError, const TDesC& aDescr);
	TBool MarkErrorL(TInt aError, const TDesC& aDescr);
	void CloseFile();
	void OpenFileL();
	TBool SeekToRequestId(TInt64 aId);
	virtual void RemoveRequest(TInt64 aRequestId);
	virtual void SetFixedIap(TInt aIap);
	TInt GetErrorWait();
	void GetFileExtensionL(const TDesC& aMimeType, TDes& aExtensionInto);
	TBool CheckForLocal(const TDesC& aUrl);

	enum TState { EIdle, EFetching, ENotConnecting, EWaitingForReconnectInfo };
	TState iCurrentState;

	enum TDownloadingStatus { EDone, EDownloading, EDownloadErrors, EDownloadNotConnecting };
	void ShowDownloading(TDownloadingStatus aStatus);
	void CheckConnectivity();
	CConnectivityListener* ConnectivityListener() { return iConnectivity; }

	enum TColumns {
		ERequestId = 1,
		EPriority,
		EUrl,
		EErrors,
		EErrorCode,
		EErrorDescr,
		EFileName,
		EContentType,
		EFinished,
		ELevel,
		EGeneration,
	};
	enum TIndices {
		ERequestIdx = 0,
		EPriorityIdx
	};
	
	enum TStateStoreItem {
		ESSGeneration,
		ESSLevel,
		ESSDownloadLimitLevel,
		ESSDownloadLimitLastPriority
	};

	MDownloadObserver& iObserver;
	TInt		iPriority;
	TFileName	iDirectory;
	CHttp*		iHttp;
	RFile		iFile; TBool iFileIsOpen;
	TFileName	iCurrentFileName, iCurrentUrl, iCurrentContentType;
	TInt		iSize, iGot, iOffset;
	TInt64		iCurrentRequestId;
	TBool		iGotError, iGotReply, iDisabled;
	TInt		iIAP, iFixedIAP;
	TBool		iReseting;
	RTimer		iTimer; TBool iTimerIsOpen;
	CNotifyState*	iNotifyState;
	TBuf<50>	iConnectionName;
	HttpFactory	iHttpFactory;
	TInt		iLevel, iDownloadLimitLevel;
	TInt		iGeneration, iDownloadLimitLastPriority;
	CSingleColDb<TInt>	*iStateStore;
	CConnectivityListener*	iConnectivity;
};

EXPORT_C CDownloader* CDownloader::NewL(MApp_context& aContext, RDbDatabase& Db, 
	const TDesC& aDirectory, MDownloadObserver& aObserver, const TDesC& aConnectionName,
	HttpFactory aHttpFactory)
{
	CALLSTACKITEM_N(_CL("CDownloader"), _CL("NewL"));
	auto_ptr<CDownloaderImpl> ret(new (ELeave) CDownloaderImpl(aContext, Db, aObserver, aHttpFactory));
	ret->ConstructL(aDirectory, aConnectionName);
	return ret.release();
}

_LIT(KDownloader, "CDownloader");

CDownloader::CDownloader() : CCheckedActive(EPriorityIdle, KDownloader) { }

CDownloaderImpl::CDownloaderImpl(MApp_context& aContext, RDbDatabase& Db, 
				 MDownloadObserver& aObserver, HttpFactory aHttpFactory) : MContextBase(aContext), MDBStore(Db),
				iObserver(aObserver), iHttpFactory(aHttpFactory) { }

void CDownloaderImpl::ConstructL(const TDesC& aDirectory, const TDesC& aConnectionName)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("ConstructL"));
	
	if (iHttpFactory==0) iHttpFactory=&CHttp::NewL;
	
	iDirectory=aDirectory;
	iConnectionName=aConnectionName;

	if (iDirectory.Right(1).Compare(_L("\\"))) iDirectory.Append(_L("\\"));
	TInt err=Fs().MkDirAll(iDirectory);
	if (err!=KErrNone && err!=KErrAlreadyExists) User::Leave(err);

	TInt cols[]={ EDbColInt64, EDbColInt32, EDbColText, EDbColInt32, EDbColInt32, 
		EDbColText, EDbColText, EDbColText, EDbColInt32, EDbColInt8, EDbColInt32, -1 };
	TInt idx[]={ ERequestId, -2, EPriority, -1 };
	SetTextLen(255);
	MDBStore::ConstructL(cols, idx, EFalse, _L("downloads"), ETrue);

	iStateStore=CSingleColDb<TInt>::NewL(AppContext(), iDb, _L("SS"));
	
	SwitchIndexL(EPriorityIdx);
	TBool found;
	found=iTable.LastL();
	if (found) {
		iTable.GetL();
		iPriority=iTable.ColInt(EPriority)+1;
	} else {
		iPriority=1;
	}
	iStateStore->GetValueL( ESSLevel, iLevel);
	iStateStore->GetValueL( ESSGeneration, iGeneration);
	iStateStore->GetValueL( ESSDownloadLimitLevel, iDownloadLimitLevel);
	iStateStore->GetValueL( ESSDownloadLimitLastPriority, iDownloadLimitLastPriority);
	
	iCurrentState=EIdle;
	User::LeaveIfError(iTimer.CreateLocal()); iTimerIsOpen=ETrue;
	CActiveScheduler::Add(this);
	iConnectivity=CConnectivityListener::NewL(*this);
	iConnectivity->SuspendExpensive();
	Async();
}

CDownloaderImpl::~CDownloaderImpl()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("~CDownloaderImpl"));
	Cancel();
	if (iTimerIsOpen) iTimer.Close();
	CloseFile();
	Reset();
	delete iConnectivity;
	delete iNotifyState;
	delete iStateStore;
}

void CDownloaderImpl::Async(TInt aSeconds)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("Async"));
	if ( (iCurrentState==EIdle || iCurrentState==ENotConnecting) && !IsActive()) {
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

TBool CDownloaderImpl::CheckForLocal(const TDesC& aUrl)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("CheckForLocal"));
	if (aUrl.Mid(1, 2).Compare(_L(":\\"))==0) {
		TBuf<50> aContentType;
		if (aUrl.Right(3).CompareF(_L("jpg"))==0) {
			aContentType.Append(_L("image/jpeg"));
		} else if (aUrl.Right(3).CompareF(_L("amr"))==0) {
			aContentType.Append(_L("audio/amr"));
		} else if (aUrl.Right(3).CompareF(_L("3gp"))==0) {
			aContentType.Append(_L("video/3gp"));
		} else {
			aContentType.Append(_L("text/plain"));
		}
		iTable.SetColL(EFileName, aUrl);
		iTable.SetColL(EContentType, aContentType);
		iTable.SetColL(EFinished, 1);
		return ETrue;
	}
	return EFalse;
}

void CDownloaderImpl::ConnectivityStateChanged()
{
	if (iCurrentState!=ENotConnecting && iCurrentState!=EWaitingForReconnectInfo) {
		return;
	}
	if (! iConnectivity->AllowReconnect()) {
		iCurrentState=ENotConnecting;
	} else {
		if (iCurrentState!=EIdle) {
			iCurrentState=EIdle;
			Async(2);
		}
	}
	iObserver.ConnectivityChanged( iConnectivity->AllowReconnect(),
		iConnectivity->OfflineMode(),
		iConnectivity->CallInProgress(), iConnectivity->LowSignal() );
}

void CDownloaderImpl::CheckConnectivity()
{
	iCurrentState=EWaitingForReconnectInfo;
	iConnectivity->ResumeExpensiveL();
}

void CDownloaderImpl::SetNoDownloadLimitLevelL()
{
	iDownloadLimitLevel=-1;
	iDownloadLimitLastPriority=-1;
	iStateStore->SetValueL(ESSDownloadLimitLevel, iDownloadLimitLevel);
	iStateStore->SetValueL(ESSDownloadLimitLastPriority, iDownloadLimitLastPriority);
	Async();
}

void CDownloaderImpl::SetDownloadLimitLevelL(TInt8 aLevel)
{
	if (aLevel<0) User::Leave(KErrArgument);
	if (iDownloadLimitLevel==aLevel) return;
	
	iDownloadLimitLevel=aLevel;
	iDownloadLimitLastPriority=iPriority;
	
	iStateStore->SetValueL(ESSDownloadLimitLevel, iDownloadLimitLevel);
	iStateStore->SetValueL(ESSDownloadLimitLastPriority, iDownloadLimitLastPriority);
	
	Reset();
}

void CDownloaderImpl::SetLevelL(TInt8 aLevel)
{
	if (aLevel<0) User::Leave(KErrArgument);
	
	if (iLevel==aLevel) return;

	iStateStore->SetValueL(ESSLevel, aLevel);	
	iGeneration++;
	iStateStore->SetValueL(ESSGeneration, iGeneration);	
	iLevel=aLevel;
}

TInt8 CDownloaderImpl::GetLevelL()
{
	return iLevel;
}

CDownloader::TDownloadable CDownloaderImpl::IsDownloadable(TInt64 aRequestId)
{
	if (!SeekToRequestId(aRequestId)) return CDownloader::ENotQueued;
	
	iTable.GetL();
	TInt8 level=iTable.ColInt8(ELevel);
	TInt generation=iTable.ColInt(EGeneration);
	if (	level < iLevel || 
		( level == iLevel && generation<iGeneration ) ) {
		
		return CDownloader::ENotQueued;
	}
	
	if (iTable.ColInt(EPriority)<=iDownloadLimitLastPriority) return CDownloader::ENotDownloadable;
	
	return CDownloader::EDownloadable;
}

void CDownloaderImpl::DownloadL(TInt64 aRequestId, const TDesC& aUrl, TBool aForce, TInt8 aLevel)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("DownloadL"));
#ifdef __WINS__
	RDebug::Print(_L("h:%d"), I64HIGH(aRequestId));
	RDebug::Print(_L("l:%d"), I64LOW(aRequestId));
	RDebug::Print(aUrl);
#endif

	TBool found=SeekToRequestId(aRequestId);
	TAutomaticTransactionHolder th(*this);
	if (aLevel==-1) aLevel=iLevel;
	
	TInt priority;
	if (aLevel<iDownloadLimitLevel) {
		priority=iDownloadLimitLastPriority;
	} else {
		priority=++iPriority;
	}

	if (found) {
		iTable.GetL();
		if (iTable.ColDes(EUrl).Compare(aUrl)) 
			User::Leave(KErrArgument);
		iTable.UpdateL();
		if (aForce) {
			iTable.SetColL(EErrors, 0);
		}
		iTable.SetColL(EPriority, priority);
		iTable.SetColL(EGeneration, iGeneration);
		iTable.SetColL(ELevel, aLevel);
		PutL();
	} else {
		iTable.InsertL();
		iTable.SetColL(ERequestId, aRequestId);
		iTable.SetColL(EPriority, priority);
		iTable.SetColL(EGeneration, iGeneration);
		iTable.SetColL(ELevel, aLevel);
		iTable.SetColL(EUrl, aUrl);
		iTable.SetColL(EErrors, 0);
		iTable.SetColL(EErrorCode, 0);
		if (! CheckForLocal(aUrl)) {
			iTable.SetColL(EFinished, 0);
		}
		PutL();
	}
	if (aLevel>=iDownloadLimitLevel) Async();
}

void CDownloaderImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("CheckedRunL"));
#ifdef __WINS__
	RDebug::Print(_L("CheckedRunL()"));
#endif
	SwitchIndexL(EPriorityIdx);
	TBool found;
	TInt errcount=0; TInt err; TInt err_row=-1, current_row;
again:
	current_row=0;
	if (errcount > 5) User::Leave(err);
	found=iTable.LastL();
	
	TBool requests_left=EFalse;

	while (found) {
		current_row++;
		CC_TRAP(err, iTable.GetL());
		if (err==KErrCorrupt) {
			++errcount;
			if (errcount==2 && current_row==err_row) {
				TAutomaticTransactionHolder th(*this);
				iTable.DeleteL();
			}
			err_row=current_row;
			MDBStore::Reset(err);
			goto again;
		}
		if (iTable.ColInt(EPriority)<=iDownloadLimitLastPriority) {
			requests_left=EFalse;
			goto done;
		}
		if (iTable.ColInt(EFinished)) {
			iCurrentFileName=iTable.ColDes(EFileName);
			iCurrentContentType=iTable.ColDes(EContentType);
			iCurrentRequestId=iTable.ColInt64(ERequestId);
			Async();
			GotAllL();
			return;
		}
		TInt8 level=iTable.ColInt8(ELevel);
		TInt generation=iTable.ColInt(EGeneration);
		if (	level < iLevel || 
			( level == iLevel && generation<iGeneration ) ) {
			
			iObserver.Dequeued(iTable.ColInt64(ERequestId));
			iTable.DeleteL();
			goto again;
		}
		if (iCurrentState==ENotConnecting || iCurrentState==EWaitingForReconnectInfo) {
			return;
		}
		if (iTable.ColInt(EErrors) <= MAX_ERRORS) {
			iCurrentRequestId=iTable.ColInt64(ERequestId);
			iCurrentUrl=iTable.ColDes(EUrl);
			CC_TRAPD(err, StartDownload());
			if (err!=KErrNone) {
				iCurrentState=EIdle;
				if (MarkError(err, _L("Error starting download")))
					requests_left=ETrue;
				Reset();
			} else {
				break;
			}
		} 
		found=iTable.PreviousL();
	}
done:
	if (iCurrentState==ENotConnecting || iCurrentState==EWaitingForReconnectInfo) {
		return;
	}
	if (!found) {
		Reset();
		if (requests_left) {
			ShowDownloading(EDownloading);
			Async(GetErrorWait());
		} else {
			ShowDownloading(EDone);
		}
	} else {
		ShowDownloading(EDownloading);
	}
}

TInt CDownloaderImpl::GetErrorWait()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("GetErrorWait"));
#ifdef __WINS__
	return 1;
#else
	return 10;
#endif
}

void CDownloaderImpl::OpenFileL()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("OpenFileL"));
	if (iFileIsOpen) return;

	SeekToRequestId(iCurrentRequestId);
	iTable.GetL();

	iCurrentFileName.Zero();
	iCurrentFileName.Append(iDirectory);
	iCurrentFileName.AppendNum(iCurrentRequestId);

	TBuf<10> extension;
	GetFileExtensionL(iCurrentContentType, extension);
	iCurrentFileName.Append(extension);

	{
		TAutomaticTransactionHolder th(*this);
		iTable.UpdateL();
		iTable.SetColL(EFileName, iCurrentFileName);
		iTable.SetColL(EContentType, iCurrentContentType);
		PutL();
	}

	User::LeaveIfError(iFile.Replace(Fs(), iCurrentFileName, EFileWrite));
	iFileIsOpen=ETrue;

}

void CDownloaderImpl::StartDownload()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("StartDownload"));
#ifdef __WINS__
	TBuf<100> msg=_L("StartDownload()");
	RDebug::Print(msg);
#endif
	iReseting=iGotReply=iGotError=EFalse;
	CloseFile();

	iObserver.DownloadStarted(iCurrentRequestId);
	
	iOffset=0;
	iCurrentFileName.Zero();

	if (! iTable.IsColNull(EFileName)) {
		iCurrentFileName=iTable.ColDes(EFileName);
		TEntry entry;
		TInt err=Fs().Entry(iCurrentFileName, entry);
		if (err==KErrNone) {
			err=iFile.Open(Fs(), iCurrentFileName, EFileWrite);
			if (err==KErrNone) {
				iFileIsOpen=ETrue;
				iOffset=entry.iSize;
				err=iFile.Seek(ESeekStart, iOffset);
				if (err!=KErrNone) {
					CloseFile();
				}
			}
		}
		if (err!=KErrNone) {
			Fs().Delete(iCurrentFileName);
			iCurrentFileName.Zero();
			iOffset=0;
		}
	}

	if (!iHttp) {
		iHttp=(*iHttpFactory)(*this, AppContext(), iConnectionName);
	}

	if (iFixedIAP>0) {
		iIAP=iFixedIAP;
	} else {
		Settings().GetSettingL(SETTING_IP_AP, iIAP);
	}
	iHttp->GetL(iIAP, iCurrentUrl, TTime(0), iOffset);
	iCurrentState=EFetching;
}

void CDownloaderImpl::NotifyHttpStatus(THttpStatus st, TInt aError)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("NotifyHttpStatus"));
#ifdef __WINS__
	TBuf<100> msg=_L("NotifyHttpStatus: ");
	msg.AppendNum(st); msg.Append(_L(" err: "));
	msg.AppendNum(aError);
	RDebug::Print(msg);
#endif
	if (iReseting) return;

	TInt err;
	switch(st) {
		case EHttpConnected:
			iConnectivity->SuspendExpensive();
			break;
		case EHttpDisconnected:
			iCurrentState=EIdle;
			iHttp->Disconnect();
			if (iGotReply || (aError==KErrEof && iSize==-1 && !iGotError)) {
				CC_TRAP(err, GotAllL());
				if (err!=KErrNone) {
					MarkError(err, _L("Error notifying"));
					Reset();
				}
				Async();
			} else if (iGotError) {
				Reset();
				CheckConnectivity();
				Async(2);
			} else {
				Reset();
				MarkError(aError, _L("HTTP disconnected"));
				CheckConnectivity();
				Async(2);
			}
			break;
		case EHttpError:
			iGotError=true;
			MarkError(aError, _L("Got http error"));
			break;
	}
}

void CDownloaderImpl::NotifyNewHeader(const CHttpHeader &aHeader)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("NotifyNewHeader"));
#ifdef __WINS__
	TBuf<100> msg=_L("NotifyNewHeader: ");
	msg.AppendNum(aHeader.iHttpReplyCode);
	RDebug::Print(msg);
#endif
	iSize=aHeader.iSize;
	if (aHeader.iHttpReplyCode==301 || aHeader.iHttpReplyCode==302 || aHeader.iHttpReplyCode==307) {
		TBuf<100> msg=_L("Redirecting to ");
		msg.Append(aHeader.iLocation.Left(70));
		MarkError(KErrGeneral, msg);
		if (aHeader.iLocation.Length()>0) {
			UpdateL();
			iTable.SetColL(EUrl, aHeader.iLocation);
			PutL();
			iGotError=ETrue;
			CloseFile();
		}
	} else if (aHeader.iHttpReplyCode<200 || aHeader.iHttpReplyCode>299) {
		TBuf<100> msg;
		AppendHttpErrorL(aHeader.iHttpReplyCode, msg);
		//TBuf<100> msg=_L("Server replied with ");
		//msg.AppendNum(aHeader.iHttpReplyCode);
		MarkError(KErrGeneral, msg);
		iGotError=ETrue;
		CloseFile();
	} else {
		if (aHeader.iChunkStart > iOffset) {
			iGotError=ETrue;
			MarkError(KErrGeneral, _L("Server gave offset larger than what we asked for"));
			CloseFile();
		} else {
			iCurrentContentType=aHeader.iContentType;
			if (iOffset > aHeader.iChunkStart && iOffset>0) {
				// offset is non-zero only if
				// the file is already open
				iOffset=aHeader.iChunkStart;
				if (iOffset<0) iOffset=0;
				User::LeaveIfError(iFile.Seek(ESeekStart, iOffset));
			}
			OpenFileL();
		}
	}
}

void CDownloaderImpl::NotifyNewBody(const TDesC8 &chunk)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("NotifyNewBody"));
#ifdef __WINS__1
	TBuf<100> msg=_L("NotifyNewBody");
	RDebug::Print(msg);
#endif
	if (iGotError) return;

	iFile.Write(chunk);
	iGot+=chunk.Length();
	if (iGot>=iSize) iGotReply=ETrue;
}

void CDownloaderImpl::GotAllL()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("GotAllL"));
	TAutoBusy busy(Reporting());
	CloseFile();
#ifdef __WINS__
	TBuf<100> msg=_L("GotAllL");
	RDebug::Print(msg);
#endif
	if (SeekToRequestId(iCurrentRequestId)) {
		{
			TAutomaticTransactionHolder th(*this);
			iTable.UpdateL();
			iTable.SetColL(EFinished, 1);
			PutL();
		}
		iObserver.DownloadFinished(iCurrentRequestId, iCurrentFileName, iCurrentContentType);
		{
			TAutomaticTransactionHolder(*this);
			iTable.DeleteL();
		}
	} else {
		iObserver.DownloadFinished(iCurrentRequestId, iCurrentFileName, iCurrentContentType);
	}
}

void CDownloaderImpl::CloseFile()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("CloseFile"));
	if (iFileIsOpen) {
		iFile.Close();
		iFileIsOpen=EFalse;
	}
}

TBool CDownloaderImpl::SeekToRequestId(TInt64 aId)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("SeekToRequestId"));
	SwitchIndexL(ERequestIdx);
	TDbSeekKey k(aId);
	TBool ret=EFalse;
	ret=iTable.SeekL(k);
	return ret;
}

void CDownloaderImpl::Reset()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("Reset"));
	iReseting=ETrue;
	CloseFile();
	if (iHttp) iHttp->Disconnect();
	delete iHttp; iHttp=0;
	iReseting=EFalse;
	iCurrentState=EIdle;
	return;
}

TBool CDownloaderImpl::MarkError(TInt aError, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("MarkError"));
	TBool ret=ETrue;
	CC_TRAPD(err, ret=MarkErrorL(aError, aDescr));
	if (err!=KErrNone) return ETrue;
	return ret;
}

TBool CDownloaderImpl::MarkErrorL(TInt aError, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("MarkErrorL"));
	TBool ret=ETrue;
	if (SeekToRequestId(iCurrentRequestId)) {
		iTable.GetL();
		TInt count=iTable.ColInt(EErrors)+1;
		if (count>MAX_ERRORS) {
			iObserver.DownloadError(iCurrentRequestId, aError, aDescr, EFalse);
			ret=EFalse;
		} else {
			iObserver.DownloadError(iCurrentRequestId, aError, aDescr, ETrue);
		}
		{
			TAutomaticTransactionHolder th(*this);

			iTable.UpdateL();
			iTable.SetColL(EErrors, count);
			iTable.SetColL(EErrorCode, aError);
			iTable.SetColL(EErrorDescr, aDescr);
			PutL();
		}
	} else {
		iObserver.DownloadError(iCurrentRequestId, 
			KErrGeneral, _L("internal error: row disappeared"), EFalse);
		ret=EFalse;
	}
	return ret;
}

void CDownloaderImpl::RemoveRequest(TInt64 aRequestId)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("RemoveRequest"));
	// TODO: implement
}

void CDownloaderImpl::SetFixedIap(TInt aIap)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("SetFixedIap"));
	iFixedIAP=aIap;
}

void CDownloaderImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("DoCancel"));
	iTimer.Cancel();
}

void CDownloaderImpl::GetFileExtensionL(const TDesC& aMimeType, TDes& aExtensionInto)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("GetFileExtensionL"));
	TBuf<50> basictype;
	TInt semicol_pos;
	semicol_pos=aMimeType.Locate(';');
	if (semicol_pos==KErrNotFound) {
		basictype=aMimeType;
	} else {
		basictype=aMimeType.Left(semicol_pos);
	}

	if (basictype.CompareF(_L("image/jpeg"))==0) {
		aExtensionInto=_L(".jpg");
	} else if (basictype.CompareF(_L("image/jpg"))==0) {
		aExtensionInto=_L(".jpg");
	} else if (basictype.CompareF(_L("audio/wav"))==0) {
		aExtensionInto=_L(".wav");
	} else if (basictype.CompareF(_L("audio/amr"))==0) {
		aExtensionInto=_L(".amr");
	} else if (basictype.Left(5).CompareF(_L("video"))==0) {
		aExtensionInto=_L(".3gp");
	} else if (basictype.CompareF(_L("text/plain"))==0) {
		aExtensionInto=_L(".txt");
	} else if (basictype.CompareF(_L("text/html"))==0) {
		aExtensionInto=_L(".html");
	} else {
		aExtensionInto=_L(".dat");
	}
}


#include "testsupport.h"

void CDownloaderImpl::ShowDownloading(TDownloadingStatus aStatus)
{
	CALLSTACKITEM_N(_CL("CDownloaderImpl"), _CL("ShowDownloading"));
	
	if ( TestSupport().NoAvkonIcons() ) return;
	
	if ( aStatus == EDone ) {
		delete iNotifyState; iNotifyState=0;
	} else {
		if (! iNotifyState ) {
			iNotifyState=CNotifyState::NewL(AppContext(), KIconFile);
		}
		if (aStatus == EDownloading) {
			iNotifyState->SetCurrentState(EMbmContextnetworkDownloading, EMbmContextnetworkDownloading);
		} else {
			iNotifyState->SetCurrentState(EMbmContextnetworkDownloading, EMbmContextnetworkDownloading);
		}
	}
}


static void AppendHttpErrorL(TInt aCode, TDes& b)
{
	switch ( aCode )
		{
		case 300: SafeAppend(b,_L("Multiple choices")); break;
		case 301: SafeAppend(b,_L("Image moved permanently")); break;
		case 302: SafeAppend(b,_L("Image not available")); break;
			
		case 400: SafeAppend(b,_L("Bad Request")); break;
		case 401: SafeAppend(b,_L("Unauthorized access")); break;
		case 402: SafeAppend(b,_L("Payment Required")); break;
		case 403: SafeAppend(b,_L("Forbidden")); break;
		case 404: SafeAppend(b,_L("Not found")); break;
		case 408: SafeAppend(b,_L("Request timeout")); break;

		case 500: SafeAppend(b,_L("Internal Server Error")); break;
		case 501: SafeAppend(b,_L("Not implemented in server")); break;
		case 502: SafeAppend(b,_L("Bad gateway")); break;
		case 503: SafeAppend(b,_L("Service unavailable")); break;
		case 504: SafeAppend(b,_L("Gateway timeout")); break;
		case 505: SafeAppend(b,_L("HTTP Version Not Supported")); break;

		default:
			SafeAppend(b, _L("Server replied with HTTP error") );
		}
	TBuf<10> errStr; 
	errStr.Format( _L(" (%d)" ), aCode);
	SafeAppend(b, errStr);
}

class CPurgerImpl : public CPurger, public MContextBase {
private:
	CPurgerImpl(MPurgeObserver &aObserver, const TDesC& aDirectory);
	void CheckedRunL();
	TInt CheckedRunError();
	void DoCancel();
	~CPurgerImpl();
	void TriggerL(TInt aLimitInKilobytes);
	void Stop();
	void ConstructL();
	void Async();
	
	MPurgeObserver &iObserver;
	TFileName	iDirectory;
	friend class CPurger;
	CDir*		iDir;
	TInt		iLimit;
	TInt		iSeenSoFar;
	TInt		iIndex;
};

_LIT(KPurger, "CPurger");

CPurger::CPurger() : CCheckedActive(CActive::EPriorityIdle, KPurger) { }

EXPORT_C CPurger* CPurger::NewL(MPurgeObserver &aObserver, const TDesC& aDirectory)
{
	auto_ptr<CPurgerImpl> ret(new (ELeave) CPurgerImpl(aObserver, aDirectory));
	ret->ConstructL();
	return ret.release();
}

CPurgerImpl::CPurgerImpl(MPurgeObserver &aObserver, const TDesC& aDirectory) : iObserver(aObserver) {
	iDirectory=aDirectory;
	if (iDirectory.Right(1).Compare(_L("\\"))) {
	        iDirectory.Append(_L("\\"));
	}
}

void CPurgerImpl::Async()
{
	TRequestStatus* s=&iStatus;
	User::RequestComplete(s, KErrNone);
	SetActive();
}

void CPurgerImpl::CheckedRunL()
{
	if (!iDir) {
		TInt err=Fs().GetDir(iDirectory, KEntryAttNormal, ESortByDate, iDir);
		if (err!=KErrNone) return;
	} else {
		if ( iIndex >= iDir->Count()) return;
		const TEntry& e=(*iDir)[iIndex];
		++iIndex;
		iSeenSoFar+=e.iSize;
		if (iSeenSoFar>iLimit) {
			TFileName f=iDirectory;
			f.Append(e.iName);
			TInt err=Fs().Delete(f);
			if (err==KErrNone) iObserver.FilePurged(f);
		}
	}

	Async();	
}

TInt CPurgerImpl::CheckedRunError()
{
	Stop();
}

void CPurgerImpl::DoCancel() { }

CPurgerImpl::~CPurgerImpl()
{
	Stop();
}

void CPurgerImpl::TriggerL(TInt aLimitInKilobytes)
{
	Stop();
	iSeenSoFar=0;
	iLimit=aLimitInKilobytes*1024;
	iIndex=0;
	Async();
}

void CPurgerImpl::Stop()
{
	Cancel();
	delete iDir;
        iDir = NULL;
}

void CPurgerImpl::ConstructL()
{
	CActiveScheduler::Add(this);
}
