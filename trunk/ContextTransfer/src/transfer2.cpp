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
#include "transfer2.h"

#include "db.h"
#include "symbian_auto_ptr.h"
#include "cn_http.h"
#include "settings.h"
#include "cl_settings.h"
#include <bautils.h>
#include "file_output_base.h"
#include "cc_imei.h"
#include "bbutil.h"
#include "raii_f32file.h"
#include "raii_d32dbms.h"
#include "xmlbuf.h"
#include "expect.h"
#include "bbxml.h"
#include <apgcli.h>
#include "notifystate.h"
#include <contextnetwork.mbg>
#include "cm_post.h"
#include "md5.h"
#define DONT_INCLUDE_LOG 1
#include "util.h"
#include "bberrorinfo.h"
#include "reporting.h"
#include "flickr_rsp.h"
#include "callstack.h"
#include "app_context_impl.h"
#include "csd_gps.h"
#include "flickr_keys.h"
#include "contextvariant.hrh"
#include "cc_imei.h"
#include "cc_uuid.h"
#include "csd_feeditem.h"
#include "csd_clienttuples.h"
#include "cbbsession.h"

void Log(const TDesC& aMsg) {
	GetContext()->Reporting().UserErrorLog(aMsg);
}

_LIT(KUploadDir, "ToBeUploaded\\");
_LIT(KNoUploadDir, "NotShared\\");
_LIT(KUploadedDir, "Shared\\");
_LIT(KOldDir, "Old\\");

#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\contextnetwork.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\contextnetwork.mbm");
#endif

#define MAXERRORS 6

/*
 * The functions taking a TFileOpStatus as a reference parameter
 * might use its fn member as the aFileName parameter. We have to
 * take care not to clobber it.
 */

class CHttpTransfer2Impl : public CHttpTransfer2, public MContextBase, public MDBStore, public MSocketObserver,
	MHttpObserver {
private:
	// CHttpTransfer2
	virtual TFileName AddFileToQueueL(const TDesC& aFileName, TInt UrlSetting,
			TInt ScriptSetting, TBool DeleteAfter, const TDesC& aMeta,
			MBBData* aPacket, TInt aWaitTime=0, TBool aDontMove=EFalse);
	virtual TInt DeleteFile(const TDesC& aFileName);

	virtual void DoNotTransfer(const TDesC& aFileName, TInt64 aPostId, TFileOpStatus& aStatus);
	virtual void MoveToOld(const TDesC& aFileName, TInt64 aPostId, TFileOpStatus& aStatus);
	virtual void DoNotTransfer(const TDesC& aFileName, TFileOpStatus& aStatus);
	virtual void MoveToOld(const TDesC& aFileName, TFileOpStatus& aStatus);

	virtual TBool FileIsQueued(const TDesC& aFileName);
	virtual void RemoveFromQueueL(const TDesC& aFileName);
	virtual void Trigger(TBool Force=EFalse);

	CHttpTransfer2Impl(MApp_context& Context,
		RDbDatabase& Db, MSocketObserver* aStatusNotif);
	void ConstructL();

	friend class CHttpTransfer2;

	// CheckedActive
	void CheckedRunL();
	TInt CheckedRunError(TInt aError);
	void DoCancel();

	// MSocketObserver
	virtual void success(CBase* source);
	virtual void error(CBase* source, TInt code, const TDesC& reason);
	virtual void info(CBase* source, const TDesC& msg);
	virtual void error(CBase* source, TInt code, const TDesC& reason, MErrorInfo* info,
		TBool& aReportOnly);

	// MHttpObserver
	virtual void NotifyHttpStatus(THttpStatus st, TInt aError);
	virtual void NotifyNewHeader(const CHttpHeader &aHeader);
	virtual void NotifyNewBody(const TDesC8 &chunk);

	// internal
	void StartFileTransferL();
	void InnerStartFileTransferL(TBool& aGotToFile);
	void ConnectL();
	void HandleSent();
	TBool MoveToCurrent();
	void Close();
	CPtrList<CPostPart>* GetFileInfoL(const TDesC& aFileName, const TDesC& PacketFileName);
	void CHttpTransfer2Impl::GetPublishTypeL();
	CFilePart* CHttpTransfer2Impl::GetPostFileL(const TDesC& aFileName, const TDesC& mime);
	void MD5File(MD5_CTX *md5, const TDesC& aFileName);

	void Reset();
	void MakeFileName();
	TFileName AddFileToQueueL(const TDesC& aFileName, TInt UrlSetting,
			TInt ScriptSetting, TBool DeleteAfter, const TDesC& aMeta,
			MBBData* aPacket, const TDesC& PacketFileName, TInt aWaitTime, TBool aDontMove);

	enum state { IDLE, PROCESSING_REQUEST, CONNECTING, MKDING, CWDING, SENDING_FILE, CLOSING };
	state current_state;
	enum TOp { UPLOAD, DELETE, DONTSHARE, MOVETOOLD };

	enum TCompletion { INPROGRESS, STARTED, UPLOADED, DELETED, NOTSHARED, OLD };
	void SetCompletionL(TCompletion aCompletion);

	void QueueOpL(const TDesC& aSource, TOp Op, const TDesC& aDestination=KNullDesC,
		TInt64 aPostId=0);

	void DoNotTransferL(const TDesC& aSource, const TDesC& aDestination);
	void MoveToOldL(const TDesC& aSource, const TDesC& aDestination);

	TFileName GetDoNotTransferFileName(const TDesC& aFileName);
	TFileName GetMoveToOldFileName(const TDesC& aFileName);

	enum TUploadingStatus { EDone, EUploading, EErrors };
	void ShowUploading(TUploadingStatus aStatus);

	virtual void AddPostCallbackL(MPostUploadCallback* aCallback);
	virtual void RemovePostCallback(MPostUploadCallback* aCallback);
	CList<MPostUploadCallback*>	*iCallbacks;
	void CallStateChanged(TInt64 aPostId, MPostUploadCallback::TState aState, const TDesC& aFileName, 
		const CBBErrorInfo* aError);

	CHttp*		iHttp;
	TFileName	iTempFileName, iTempFileName2, iPacketFileName;
	TDbBookmark	iCurrentFileMark;
	MSocketObserver* iStatusNotif;
	RTimer		iTimer;
	TInt		iWaitTime;

	TInt		iRetries;
	TBuf<20>	iImsi;
	TBuf<20>	iDT, iPrevDT;
	TBuf<30>	iFileNameBuf;
	TInt		iDTCount;
	TInt		iFileNameIndexLen;
	bool		iNoTransfer;
	bool		iGotReply, iGotError, iSentAll;

	TInt		iIap;
	TBuf<256>	iUrl;
	CExpect*	iExpect;
	CFlickrResponse* iFlickrResponse;
	CXmlParser*	iParser;

	CNotifyState*	iNotifyState;
	TInt		iPublishType;
	TBuf<256>	iReply; TInt iState;
	CCnvCharacterSetConverter*	iUtf8CC;

	CBBErrorInfo	*iErrorInfo;
	TFileName	iCurrentlySendingFileName;
	TBuf<100> iNick;
	CUuidGenerator	*iUuidGenerator;
	CBBFeedItem	*iCurrentFeedItem;
public:
	~CHttpTransfer2Impl();
};


EXPORT_C CHttpTransfer2* CHttpTransfer2::NewL(MApp_context& Context,
	RDbDatabase& Db, MSocketObserver* aStatusNotif)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2"), _CL("NewL"));


	auto_ptr<CHttpTransfer2Impl> ret(new (ELeave) CHttpTransfer2Impl(Context, Db, aStatusNotif));
	ret->ConstructL();
	return ret.release();
}

CHttpTransfer2::CHttpTransfer2() : CCheckedActive(EPriorityIdle, _L("CHttpTransfer2"))
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2"), _CL("CHttpTransfer2"));

}

EXPORT_C CHttpTransfer2::~CHttpTransfer2()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2"), _CL("~CHttpTransfer2"));

}

void CHttpTransfer2Impl::MakeFileName()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("MakeFileName"));

	TTime now; now=GetTime();
	format_datetime(iDT, now);
	iDT.Trim();

	iFileNameBuf=iDT;
	if (! (iDT.Compare(iPrevDT)) ) {
		++iDTCount;
		iFileNameBuf.Append(_L("-"));
		iFileNameBuf.AppendNum(iDTCount);
	} else {
		iDTCount=0;
	}
	iPrevDT=iDT;
}

void RemoveOurPath(TDes& aTempFileName, TParse& p) {
	aTempFileName=p.DriveAndPath();
	if (! aTempFileName.Right( KOldDir().Length() ).Compare( KOldDir )) {
		aTempFileName = p.DriveAndPath().Left(p.DriveAndPath().Length()-KOldDir().Length());
	}
	if (! aTempFileName.Right( KUploadDir().Length() ).Compare( KUploadDir )) {
		aTempFileName = p.DriveAndPath().Left(p.DriveAndPath().Length()-KUploadDir().Length());
	}
	if (! aTempFileName.Right( KOldDir().Length() ).Compare( KNoUploadDir )) {
		aTempFileName = p.DriveAndPath().Left(p.DriveAndPath().Length()-KNoUploadDir().Length());
	}
	if (! aTempFileName.Right( KUploadedDir().Length() ).Compare( KUploadedDir )) {
		aTempFileName = p.DriveAndPath().Left(p.DriveAndPath().Length()-KUploadedDir().Length());
	}
}

TFileName CHttpTransfer2Impl::AddFileToQueueL(const TDesC& aFileName, TInt UrlSetting,
	TInt ScriptSetting, TBool DeleteAfter, const TDesC& aMeta,
	MBBData* aPacket, const TDesC& PacketFileName, TInt aWaitTime, TBool aDontMove) 
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("AddFileToQueueL"));

	TParse p; p.Set(aFileName, 0, 0);

	{
		TAutomaticTransactionHolder th(*this);
		iTable.InsertL();
		iTable.SetColL(1, aFileName);
		iTable.SetColL(2, UrlSetting);
		iTable.SetColL(3, ScriptSetting);
		iTable.SetColL(4, DeleteAfter);
		iTable.SetColL(5, aMeta.Left(KMaxFileName));
		iTable.SetColL(6, PacketFileName);
		iTable.SetColL(7, UPLOAD);
		iTable.SetColL(8, aDontMove);
		if (aPacket) {
			const CCMPost* post=bb_cast<CCMPost>(aPacket);
			if (post) {
				iTable.SetColL(10, post->iPostId());
			} else {
				iTable.SetColL(10, TInt64(0));
			}
			RADbColWriteStream w; w.OpenLA(iTable, 9);
			aPacket->Type().ExternalizeL(w);
			aPacket->ExternalizeL(w);
			w.CommitL();
		} else {
			iTable.SetColL(10, TInt64(0));
		}
		PutL();
	}

	iWaitTime=aWaitTime;
	if (!aDontMove) {
		RemoveOurPath(iTempFileName, p);
		iTempFileName.Append(KUploadDir);
		BaflUtils::EnsurePathExistsL(Fs(), iTempFileName);
		iTempFileName.Append(iFileNameBuf);
		iTempFileName.Append(p.Ext());

		if (Fs().Rename(aFileName, iTempFileName)==KErrNone) {
			TAutomaticTransactionHolder th(*this);
			iTable.UpdateL();
			iTable.SetColL(1, iTempFileName);
			PutL();
			return iTempFileName;
		}
	}
	
	return aFileName;
}

TFileName CHttpTransfer2Impl::AddFileToQueueL(const TDesC& aFileName, TInt UrlSetting,
	TInt ScriptSetting, TBool DeleteAfter, const TDesC& aMeta,
	MBBData* aPacket, TInt aWaitTime, TBool aDontMove)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("AddFileToQueueL"));


	TParse p; p.Set(aFileName, 0, 0);

	MakeFileName();

	RDebug::Print(aFileName);
	iPacketFileName = p.DriveAndPath();
	if (! iPacketFileName.Right( KOldDir().Length() ).Compare( KOldDir )) {
		iPacketFileName = p.DriveAndPath().Left(p.DriveAndPath().Length()-KOldDir().Length());
	}
	iPacketFileName.Append(KUploadDir);
	iPacketFileName.Append(iFileNameBuf);
	iPacketFileName.Append(_L(".xml"));

	// FIXME: it's not the packet file name we are getting, but the content file name, so
	// the variable name is wrong here, logic is right
	iPacketFileName=AddFileToQueueL(aFileName, UrlSetting, ScriptSetting, DeleteAfter, aMeta, 
		aPacket, iPacketFileName, aWaitTime, aDontMove);

	Trigger();
	return iPacketFileName;
}

void CHttpTransfer2Impl::QueueOpL(const TDesC& aFileName, TOp Op, const TDesC& aFileName2, TInt64 aPostId)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("QueueOpL"));

	{
		TAutomaticTransactionHolder th(*this);

		iTable.InsertL();
		iTable.SetColL(1, aFileName);
		iTable.SetColL(2, 0);
		iTable.SetColL(3, 0);
		iTable.SetColL(4, 0);
		iTable.SetColL(5, aFileName2);
		iTable.SetColL(6, _L(""));
		iTable.SetColL(7, Op);
		iTable.SetColL(8, EFalse);
		iTable.SetColL(10, aPostId);
		PutL();
	}

	iWaitTime=5;
	Trigger();
}

TInt CHttpTransfer2Impl::DeleteFile(const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("DeleteFile"));


	if(Fs().Delete(aFileName)==KErrNone) return KErrNone;

	CC_TRAPD(err, QueueOpL(aFileName, DELETE));
	return err;
}

void CHttpTransfer2Impl::DoNotTransferL(const TDesC& aSource, const TDesC& aDestination)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("DoNotTransferL"));

	User::LeaveIfError(Fs().Rename(aSource, aDestination));
	RDebug::Print(_L("moved %S to %S"), &aSource, &aDestination);
}

void CHttpTransfer2Impl::DoNotTransfer(const TDesC& aFileName, TFileOpStatus& aStatus)
{
	DoNotTransfer(aFileName, 0, aStatus);
}

void CHttpTransfer2Impl::DoNotTransfer(const TDesC& aFileName, TInt64 aPostId, TFileOpStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("DoNotTransfer"));

	TFileOpStatus& ret=aStatus;
	TFileName f = GetDoNotTransferFileName(aFileName);

	CC_TRAP(ret.err, DoNotTransferL(aFileName, f));
	if (ret.err==KErrNone) {
		ret.fn=f;
		return;
	} else {
		CC_TRAP(ret.err, QueueOpL(aFileName, DONTSHARE, f, aPostId));
		f=aFileName;
		ret.fn=f;
	}
}

void CHttpTransfer2Impl::MoveToOldL(const TDesC& aSource, const TDesC& aDestination)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("MoveToOldL"));

	TFileName fn;
	TBool del=EFalse;
	ToDel(aSource, fn);
	if (BaflUtils::FileExists(Fs(), fn)) {
		del=ETrue;
	}
	ToPacket(aSource, fn);
	if (BaflUtils::FileExists(Fs(), fn)) {
		// compatibility with old upload
		MakeFileName();
		CC_TRAPD(err,
			AddFileToQueueL(aSource, SETTING_PUBLISH_URLBASE, 
			SETTING_PUBLISH_SCRIPT, del, _L("unknown"), 0, fn, 0, EFalse));
		if (err==KErrNone) {
			ToDel(aSource, fn);
			Fs().Delete(fn);
		}
		User::LeaveIfError(err);
		return;
	}

	User::LeaveIfError(Fs().Rename(aSource, aDestination));
	RDebug::Print(_L("moved %S to %S"), &aSource, &aDestination);
}

void CHttpTransfer2Impl::MoveToOld(const TDesC& aFileName, TFileOpStatus& aStatus)
{
	 MoveToOld(aFileName, 0, aStatus);
}

void CHttpTransfer2Impl::MoveToOld(const TDesC& aFileName, TInt64 aPostId, TFileOpStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("MoveToOld"));

	TFileOpStatus& ret=aStatus;
	TFileName f = GetMoveToOldFileName(aFileName);

	CC_TRAP(ret.err, MoveToOldL(aFileName, f));
	if (ret.err==KErrNone) {
		ret.fn=f;
		return;
	} else {
		CC_TRAP(ret.err, QueueOpL(aFileName, MOVETOOLD, f, aPostId));
		f=aFileName;
		ret.fn=f;
	}
}

CHttpTransfer2Impl::CHttpTransfer2Impl(MApp_context& Context,
				       RDbDatabase& Db, MSocketObserver* aStatusNotif) : 
	MContextBase(Context), MDBStore(Db), iStatusNotif(aStatusNotif)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("CHttpTransfer2Impl"));


}

void CHttpTransfer2Impl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("ConstructL"));

	iFileNameIndexLen=50;

	TInt cols[] = { EDbColText, EDbColInt32, EDbColInt32, EDbColBit, EDbColText, 
		EDbColText, EDbColInt32, EDbColBit, EDbColLongBinary, EDbColInt64, 
		EDbColInt32, EDbColInt32, EDbColInt32, -1 };
	TInt idx[] = { 1, -1 };

	_LIT(KFlickrAuth, "c:\\system\\data\\context\\flickr_auth_token.txt");
	if (BaflUtils::FileExists(Fs(), KFlickrAuth)) {
		TBuf8<32> buf;
		{
			RAFile f; f.OpenLA(Fs(), KFlickrAuth, EFileShareAny|EFileRead);
			f.Read(buf);
		}
		if (buf[buf.Length()-1] == '\n') buf.SetLength(buf.Length()-1);
		if (buf[buf.Length()-1] == '\r') buf.SetLength(buf.Length()-1);
		Settings().WriteSettingL(SETTING_FLICKR_AUTH, buf);
		Settings().WriteSettingL(SETTING_PUBLISH_URLBASE, _L("http://www.flickr.com/services/upload/"));
		Settings().WriteSettingL(SETTING_PUBLISH_SCRIPT, KNullDesC);
		Settings().WriteSettingL(SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL);
		Fs().Delete(KFlickrAuth);
	}
	iCallbacks=CList<MPostUploadCallback*>::NewL();

	iTimer.CreateLocal();

	iExpect=CExpect::NewL();

	SetTextLen(KMaxFileName);

	MDBStore::ConstructL(cols, idx, false, _L("TRANSFER2"), ETrue);

	iErrorInfo=CBBErrorInfo::NewL(BBDataFactory());

	CActiveScheduler::Add(this);
	Settings().GetSettingL( SETTING_JABBER_NICK, iNick);
	TInt at = iNick.Locate('@');
	if (at >= 0) iNick = iNick.Left(at);		
	iUuidGenerator = CUuidGenerator::NewL(iNick, KUidContextTransfer, 1);

	Trigger();
}

void CHttpTransfer2Impl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("CheckedRunL"));

	iTable.Cancel();

	if (current_state==IDLE) {
		if (iTable.FirstL()) {
			iCurrentFileMark=iTable.Bookmark();
			ConnectL();
			iWaitTime=0;
		} else {
			ShowUploading(EDone);
			return;
		}
		ShowUploading(EUploading);
	} else if (current_state==SENDING_FILE) {
		ShowUploading(EUploading);
		StartFileTransferL();
	}
}

TInt CHttpTransfer2Impl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("CheckedRunError"));

	if (iStatusNotif) {
		if (iLatestErrorStack)
			iStatusNotif->error(this, aError, *iLatestErrorStack);
		else
			iStatusNotif->error(this, aError, _L("http2runl"));
	}
	if (iWaitTime==0) {
		iWaitTime=10;
	} else {
		if (iWaitTime<12*60*60) {
			iWaitTime=(TInt)(iWaitTime*1.5);
		}
	}
	Reset();
	Trigger();
	return KErrNone;
}

void CHttpTransfer2Impl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("DoCancel"));

	iTimer.Cancel();
}

void CHttpTransfer2Impl::success(CBase* /*source*/)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("success"));

	iCurrentlySendingFileName.Zero();
	iTable.Cancel();

	switch(current_state) {
	case CONNECTING:
		StartFileTransferL();
		break;
	case SENDING_FILE:
		{
		if (iStatusNotif) iStatusNotif->success(this);
		if (iStatusNotif) iStatusNotif->info(this, _L("transfered file"));

		HandleSent();
		if (iTable.NextL() || iTable.FirstL()) {
			iCurrentFileMark=iTable.Bookmark();
			Trigger(ETrue);
		} else {
			if (iStatusNotif) iStatusNotif->info(this, _L("all files transfered"));
			Close();
			ShowUploading(EDone);
		}
		}
		break;
	case CLOSING:
		Reset();
		iWaitTime=1;
		Trigger();
		break;
	}
}

TBool CHttpTransfer2Impl::MoveToCurrent()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("MoveToCurrent"));


	CC_TRAPD(err, iTable.GotoL(iCurrentFileMark));
	if (err!=KErrNone) return EFalse;
	return ETrue;
}

void CHttpTransfer2Impl::error(CBase* source, TInt code, const TDesC& reason)
{
	iCurrentlySendingFileName.Zero();
	TBool dummy=EFalse;
	error(source, code, reason, 0, dummy);
}

void CHttpTransfer2Impl::error(CBase* source, TInt code, const TDesC& reason, MErrorInfo* info,
			       TBool& aReportOnly)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("error"));

	iTable.Cancel();

	if (iStatusNotif) iStatusNotif->error(this, code, reason);

	iErrorInfo->iStackTrace->TakeHBufC(CallStackMgr().GetFormattedCallStack(KNullDesC));
	if (current_state==SENDING_FILE || current_state==PROCESSING_REQUEST) {
		if (MoveToCurrent()) {
			iTable.GetL();

			if (info) iErrorInfo->SetInnerError(info);

			iErrorInfo->iErrorCode=MakeErrorCode(0, code);
			iErrorInfo->iErrorType()=EBug;
			TBool permanent=EFalse;

			if (source==iFlickrResponse) {
				iErrorInfo->iUserMsg->Append( iFlickrResponse->ErrorMessage() );
				iErrorInfo->iTechnicalMsg->Append( iFlickrResponse->XmlErrorMessage() );
				permanent=iFlickrResponse->IsPermanent();
				iErrorInfo->iErrorType()=ERemote;
			} else {
				if (current_state==SENDING_FILE) {
					iErrorInfo->iUserMsg->Append(_L("Failed to upload media: "));
				} else {
					iErrorInfo->iUserMsg->Append(_L("Failed to process media: "));
				}
				iErrorInfo->iTechnicalMsg->Append( reason );
				switch(code) {
				case -25: //disconnected
				case -33:
					iErrorInfo->iUserMsg->Append(_L("disconnected."));
					iErrorInfo->iErrorType()=ELocalEnvironment;
					break;
				case -5120:
					iErrorInfo->iUserMsg->Append(_L("DNS lookup failed. If this persists, reboot your device."));
					iErrorInfo->iErrorType()=ELocalEnvironment;
					break;
				case -5105:
					iErrorInfo->iUserMsg->Append(_L("Could not connect to service (No route to host)"));
					iErrorInfo->iErrorType()=ELocalEnvironment;
					break;
				case -1003:
					iErrorInfo->iUserMsg->Append(_L("Could not create GPRS connection. Check your settings. If this persists, reboot your device."));
					iErrorInfo->iErrorType()=ELocalEnvironment;
					break;
				}
			}

			TInt errors=0;
			if (!iTable.IsColNull(11)) errors=iTable.ColInt32(11);

			if (errors>1 && permanent) errors=MAXERRORS;
			TInt64 postid=0;
			if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);
			++errors;
			if (postid != 0) {

				if (errors>=MAXERRORS) {
					CallStateChanged(postid, MPostUploadCallback::EFailed,
						KNullDesC, iErrorInfo);
					TAutomaticTransactionHolder th(*this);
					iTable.DeleteL();
				} else {
					CallStateChanged(postid, MPostUploadCallback::EQueued,
						KNullDesC, iErrorInfo);
					TAutomaticTransactionHolder th(*this);
					iTable.UpdateL();
					iTable.SetColL(11, errors);
					iTable.SetColL(12, INPROGRESS);
					PutL();
				}
			} else {
				TAutomaticTransactionHolder th(*this);
				iTable.UpdateL();
				iTable.SetColL(11, errors);
				iTable.SetColL(12, INPROGRESS);
				PutL();
			}
		}
		if (aReportOnly && MoveToCurrent()) {
			iWaitTime=1;
		} else if (MoveToCurrent() && iTable.NextL()) {
			aReportOnly=EFalse;
			iCurrentFileMark=iTable.Bookmark();
			iWaitTime=1;	
		} else if (iTable.FirstL()) {
			aReportOnly=EFalse;
			iCurrentFileMark=iTable.Bookmark();
			++iRetries;
			iWaitTime=300;
		} else {
			aReportOnly=EFalse;
			Reset();
			ShowUploading(EDone);
			return;
		}
	} else {
		Reset();
		++iRetries;
		iWaitTime=300;
	}
	Trigger(ETrue);
	ShowUploading(EUploading);
}

void CHttpTransfer2Impl::info(CBase* /*source*/, const TDesC& msg)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("info"));


	if (iStatusNotif) iStatusNotif->info(this, msg);
}

void CHttpTransfer2Impl::Trigger(TBool Force)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Trigger"));

	if (!Force && current_state!=IDLE) return;

	Cancel();
	ShowUploading(EUploading);

	if (iWaitTime==0) {
		TRequestStatus *s=&iStatus;
		User::RequestComplete(s, KErrNone);
	} else {
		if (iWaitTime<0) iWaitTime=300;
		TTimeIntervalMicroSeconds32 w(iWaitTime*1000*1000);
		iTimer.After(iStatus, w);
	}
	SetActive();
}

void CHttpTransfer2Impl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("ConnectL"));

	iTable.Cancel();
	TInt& Iap=iIap;
	Settings().GetSettingL(SETTING_IP_AP, Iap);

	if (Iap==-1) {
		// 'No connection'
		current_state=SENDING_FILE;
		iNoTransfer=true;
		Trigger(ETrue);
	} else {
		iNoTransfer=false;

		if (!iHttp) {
			iHttp=CHttp::NewL(*this, AppContext(), _L("transferer"));
		}

		if (!MoveToCurrent()) {
			if (iStatusNotif) iStatusNotif->error(this, -1031, _L("current record disappeared"));
			Reset();
			if (iTable.FirstL()) {
				iWaitTime=5;
				Trigger();
			}
			return;
		}
		iTable.GetL();

		TInt url_setting, script_setting;

		url_setting=iTable.ColInt32(2);
		script_setting=iTable.ColInt32(3);

		TDes& url=iUrl;
		TBuf<50> script;
		Settings().GetSettingL(url_setting, url);
		Settings().GetSettingL(script_setting, script);
		url.Append(script);

		current_state=CONNECTING;

		StartFileTransferL();
	}
}

void CHttpTransfer2Impl::Close()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Close"));

	current_state=CLOSING;
	success(this);
}

CHttpTransfer2Impl::~CHttpTransfer2Impl()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("~CHttpTransfer2Impl"));

	Cancel();
	iTimer.Close();

	delete iFlickrResponse;
	delete iParser;

	delete iHttp;
	delete iExpect;
	delete iNotifyState;
	delete iUtf8CC;
	delete iCallbacks;
	delete iUuidGenerator;
	if (iErrorInfo) iErrorInfo->Release();
	
	if (iCurrentFeedItem) iCurrentFeedItem->Release();
}

_LIT(KPacket, "packet");

void ReplaceColons(TDes8& into)
{
	for (int i=0; i<into.Length(); i++) {
		if (into[i] == ':') into[i]=' ';
	}
}

void ReplaceSpaces(TDes8& into)
{
	for (int i=0; i<into.Length(); i++) {
		if (into[i] == ' ') into[i]='_';
		if (into[i] == ',') into[i]='_';
	}
}

void ReplaceQuotes(TDes8& into)
{
	for (int i=0; i<into.Length(); i++) {
		if (into[i] == '"') into[i]='\'';
	}
}

void AddSingleTag(auto_ptr<HBufC8>& aBuf, const TDesC& aLabel, const TDesC& aTag, CCnvCharacterSetConverter* aCC,
		MD5_CTX* md5)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("AddTag"));

	if (aTag.Length()==0) return;

	TInt existing_len=aBuf->Des().Length();
	TInt append_len=(aLabel.Length() + aTag.Length() +1 )*2;
	TInt appended_len=0;
	while ( existing_len + append_len > aBuf->Des().MaxLength() ) {
		HBufC8* tmp=aBuf->ReAllocL( aBuf->Des().MaxLength() *2 );
		aBuf.release(); aBuf.reset(tmp);
	}
	if (aBuf->Des().Length() > 0) {
		_LIT8(KSpace8, " ");
		aBuf->Des().Append(KSpace8);
		MD5Update(md5, KSpace8().Ptr(), KSpace8().Length());
		++existing_len;
	}
	{
		_LIT8(KQuote8, "\"");
		aBuf->Des().Append(KQuote8);
		MD5Update(md5, KQuote8().Ptr(), KQuote8().Length());
		++existing_len;
	}
	{
		TPtr8 into( (TUint8*)aBuf->Des().Ptr() + existing_len, 0, append_len );
		aCC->ConvertFromUnicode(into, aLabel);
		appended_len+=into.Length();
		ReplaceSpaces(into);
		MD5Update(md5, into.Ptr(), into.Length());
		aBuf->Des().SetLength(aBuf->Des().Length() + into.Length());
	}
	{
		TPtr8 into( (TUint8*)aBuf->Des().Ptr() + existing_len + appended_len, 0, 
			append_len-appended_len );
		aCC->ConvertFromUnicode(into, aTag);
		//ReplaceSpaces(into);
		ReplaceQuotes(into);
		//ReplaceColons(into);
		appended_len+=into.Length();
		MD5Update(md5, into.Ptr(), into.Length());
		aBuf->Des().SetLength(aBuf->Des().Length() + into.Length());
	}
	{
		_LIT8(KQuote8, "\"");
		aBuf->Des().Append(KQuote8);
		MD5Update(md5, KQuote8().Ptr(), KQuote8().Length());
	}
}

TInt MinPos(TInt pos1, TInt pos2) {
	if (pos1==KErrNotFound) return pos2;
	if (pos2==KErrNotFound) return pos1;
	if (pos1<pos2) return pos1;
	return pos2;
}
void AddTag(auto_ptr<HBufC8>& aBuf, const TDesC& aLabel, const TDesC& aTag, CCnvCharacterSetConverter* aCC,
		MD5_CTX* md5)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("AddTag"));

	if (aTag.Length()==0) return;

	TInt start_pos=0;
	for (;;) {
		TPtrC tag1=aTag.Mid(start_pos);
		TInt colon_pos1=tag1.Locate('|');
		TInt colon_pos2=tag1.Locate(':');
		TInt colon_pos3=tag1.Locate(',');
		TInt colon_pos=KErrNotFound;
		colon_pos=MinPos( MinPos(colon_pos1, colon_pos2), colon_pos3 );
		if (colon_pos==KErrNotFound) {
			AddSingleTag(aBuf, aLabel, tag1, aCC, md5);
			break;
		} else {
			TPtrC tag2=tag1.Left(colon_pos);
			AddSingleTag(aBuf, aLabel, tag2, aCC, md5);
			start_pos+=colon_pos+1;
		}
	}

}

_LIT(KNoBlue, "noblue");
_LIT(KContextFlickr, "ctxflickr");
#ifdef __JAIKU__
_LIT(KJaiku, "jaiku");
#endif

void CHttpTransfer2Impl::GetPublishTypeL()
{
	//Reporting().DebugLog(_L("GetFileInfoL0.1"));
	if (! iTable.IsColNull(9) ) {
		CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("CheckPublishType"));
		if (iUrl.FindF(_L("flickr"))!=KErrNotFound) {
			iPublishType=1;
		} else {
			iPublishType=0;
		}
	} else {
		iPublishType=0;
	}
}

CFilePart* CHttpTransfer2Impl::GetPostFileL(const TDesC& aFileName, const TDesC& mime)
{
	auto_ptr<CFilePart> ret(CFilePart::NewL(Fs(), aFileName, _L("photo"), mime));
	{
		CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("FileName"));
		TFileName aPostFileName;
		if (iPublishType==0) {
			aPostFileName.Append(iImsi);
			aPostFileName.Append(_L("/"));
		}
		TParse p; p.Set(aFileName, 0, 0);
		aPostFileName.Append(p.NameAndExt());
		ret->SetFileName(aPostFileName);
	}
	return ret.release();
}

void CHttpTransfer2Impl::MD5File(MD5_CTX *md5, const TDesC& aFileName)
{
	auto_ptr<HBufC8> buf(HBufC8::NewL(4096));
	RAFile f; f.OpenLA(Fs(), aFileName, EFileRead|EFileShareAny);
	TInt err;
	for(;;) {
		TPtr8 p=buf->Des();
		f.Read(p);
		if (buf->Length()==0) break;
		MD5Update(md5, (TUint8*)buf->Des().Ptr(), buf->Length());
	}
}

CPtrList<CPostPart>*  CHttpTransfer2Impl::GetFileInfoL(const TDesC& aFileName,
				      const TDesC& PacketFileName)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("GetFileInfoL"));

#ifndef __WINS__
	TRAPD(ignored, GetImeiL(iImsi));
#else
	// Return a fake IMEI when working on emulator
	_LIT(KEmulatorImsi, "244050000000000");
	iImsi.Copy(KEmulatorImsi);
#endif

	if (iCurrentFeedItem) {
		iCurrentFeedItem->Release();
		iCurrentFeedItem=0;
	}
	//Reporting().DebugLog(_L("GetFileInfoL0"));
	auto_ptr< CPtrList<CPostPart> > parts(CPtrList<CPostPart>::NewL());

	TBuf<40> mime;
	GetMimeTypeL(aFileName, mime);
	GetPublishTypeL();

	//Reporting().DebugLog(_L("GetFileInfoL1"));
	auto_ptr<CFilePart> file(GetPostFileL(aFileName, mime));

	//Reporting().DebugLog(_L("GetFileInfoL2"));
	if (iPublishType==0) {
		iExpect->SetPatternL(_L8("^OK"));
	} else {
		iFlickrResponse=CFlickrResponse::NewL();
		iParser=CXmlParser::NewL(*iFlickrResponse);
	}

	//Reporting().DebugLog(_L("GetFileInfoL3"));
	if (iPublishType==0) {
		CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("CheckForPacket"));
		if (BaflUtils::FileExists(Fs(), PacketFileName)) {
			TEntry fe;
			User::LeaveIfError(Fs().Entry(PacketFileName, fe));
			if (fe.iSize!=0) {
				auto_ptr<CFilePart> packet(CFilePart::NewL(Fs(), 
					PacketFileName, _L("packet"), _L("text/xml")));
				packet->SetFileName(PacketFileName);
				parts->AppendL(packet.get());
				packet.release();
			}
		}
	}

	Reporting().DebugLog(_L("GetFileInfoL4"));
	if (! iTable.IsColNull(9) ) {
		CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Packet"));
		RADbColReadStream s; s.OpenLA(iTable, 9);
		TTypeName tn=TTypeName::IdFromStreamL(s);
		auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());
		bb_auto_ptr<MBBData> d(f->CreateBBDataL(tn, KPacket, f.get()));
		{
			CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("InternalizeL"));
			d->InternalizeL(s);
		}

		if (iPublishType==0) {
			auto_ptr<HBufC> user(HBufC::NewL(50));
			TPtr uptr=user->Des();
			auto_ptr<HBufC> pass(HBufC::NewL(50));
			TPtr pptr=pass->Des();
			TBool do_hash=EFalse;
			MD5_CTX md5; 
			if (Settings().GetSettingL(SETTING_PUBLISH_USERNAME, uptr) &&
				Settings().GetSettingL(SETTING_PUBLISH_PASSWORD, pptr)) {

				do_hash=ETrue;
				MD5Init(&md5);

				MD5File(&md5, aFileName);
			}
			auto_ptr<CXmlBufExternalizer> xmlb(CXmlBufExternalizer::NewL(2048));
			xmlb->Declaration(_L("iso-8859-1"));
			d->IntoXmlL(xmlb.get());

			auto_ptr<HBufC8> b8(HBufC8::NewL( xmlb->Buf().Length()+20 ) );
			TPtr8 p=b8->Des();
			CC()->ConvertFromUnicode(p, xmlb->Buf() );
			if (do_hash) MD5Update(&md5, (TUint8*)b8->Des().Ptr(), b8->Length());

			auto_ptr<CBufferPart> bp(CBufferPart::NewL(b8.get(), ETrue, _L("packet"), _L("text/xml")));
			b8.release();

			TBuf<128> aPostFileName=iImsi;
			aPostFileName.Append(_L("/"));
			TParse fp; fp.Set(aFileName, 0, 0);
			aPostFileName.Append(fp.Name());
			aPostFileName.Append(_L("_"));
			aPostFileName.Append(fp.Ext().Mid(1));
			aPostFileName.Append(_L(".xml"));
			
			bp->SetFileName(aPostFileName);

			parts->AppendL(bp.get());
			bp.release();

			if (do_hash) {
				auto_ptr<HBufC8> user8(HBufC8::NewL(100));
				auto_ptr<HBufC8> pass8(HBufC8::NewL(100));
				TPtr8 u8ptr=user8->Des();
				TPtr8 p8ptr=pass8->Des();
				CC()->ConvertFromUnicode(u8ptr, *user);
				CC()->ConvertFromUnicode(p8ptr, *pass);
				MD5Update(&md5, (TUint8*)user8->Des().Ptr(), user8->Length());
				MD5Update(&md5, (TUint8*)pass8->Des().Ptr(), pass8->Length());

				auto_ptr<CBufferPart> up(CBufferPart::NewL(user8.get(), ETrue, _L("username"), _L("")));
				user8.release();
				parts->AppendL(up.get()); up.release();

				TBuf8<16> hash; hash.SetLength(hash.MaxLength());
				MD5Final( (TUint8*)hash.Ptr(), &md5);
				TBuf8<32> sig;
				for (int i=0; i<16; i++) {
					sig.AppendNumFixedWidth( hash[i], EHex, 2 );
				}
				auto_ptr<CBufferPart> sigpart(CBufferPart::NewL(sig, _L("signature"), _L("")));
				parts->AppendL(sigpart.get());  sigpart.release();
			}
		} else {
			CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Flickr"));
			_LIT8(KApiKey, FLICKR_API_KEY);
			_LIT8(KSharedSecret, FLICKR_SHARED_SECRET);

			TBuf8<50> auth_token;
			Settings().GetSettingL(SETTING_FLICKR_AUTH, auth_token);
			if (auth_token.Length()==0) {
				EnvErr(gettext("No Flickr authentication token set. "
					L"Go to http://meaning.3xi.org/ to get one.")).
					Raise();
			}

			_LIT8(KApiKeyName8, "api_key");
			_LIT8(KAuthName8, "auth_token");
			_LIT8(KSigName8, "api_sig");
			_LIT(KApiKeyName, "api_key");
			_LIT(KAuthName, "auth_token");
			_LIT(KSigName, "api_sig");

			CCMPost* post=bb_cast<CCMPost>(d.get());
			if (post && post->iSharing() == CCMPost::EPublic ) {
				iCurrentFeedItem=new (ELeave) CBBFeedItem;
				iCurrentFeedItem->iContent.Append( (*post->iBodyText)() );
				iUuidGenerator->MakeUuidL(iCurrentFeedItem->iUuid.iValue);
				iCurrentFeedItem->iCreated = GetTime();
				iCurrentFeedItem->iAuthorNick() = iNick;
				iCurrentFeedItem->iKind()=_L("photo");
			}

			//Log(KSharedSecret);
			auto_ptr<CBufferPart> apikeypart(CBufferPart::NewL(KApiKey, KApiKeyName, _L("")));
			//Log(KApiKeyName); Log(KApiKey);
			parts->AppendL(apikeypart.get());  apikeypart.release();
			auto_ptr<CBufferPart> authpart(CBufferPart::NewL(auth_token, KAuthName, _L("")));
			//Log(KAuthName); Log(auth_token);
			parts->AppendL(authpart.get());  authpart.release();

			MD5_CTX md5; MD5Init(&md5);

			MD5Update(&md5, KSharedSecret().Ptr(), KSharedSecret().Length());

			MD5Update(&md5, KApiKeyName8().Ptr(), KApiKeyName8().Length());
			MD5Update(&md5, KApiKey().Ptr(), KApiKey().Length());

			MD5Update(&md5, KAuthName8().Ptr(), KAuthName8().Length());
			MD5Update(&md5, auth_token.Ptr(), auth_token.Length());

			if ( ! iUtf8CC ) {
				iUtf8CC=CCnvCharacterSetConverter::NewL();
				iUtf8CC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());
			}

			{
				TEntry e;
				if (Fs().Entry(aFileName, e)==KErrNone) {
					format_datetime(mime, e.iModified);
					mime.Trim();
					file->SetFileName(mime);
					if (iCurrentFeedItem && iCurrentFeedItem->iContent().Length()==0) {
						iCurrentFeedItem->iContent.Append(mime);
					}
				}
			}
			if (post) {
				CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Post"));
				auto_ptr<CCnvCharacterSetConverter> cc(CCnvCharacterSetConverter::NewL());
				iUtf8CC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());

				if (post->iBodyText && (*post->iBodyText)().Length() ) {
					CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Body"));
					const TDesC& d=(*post->iBodyText)();

					TInt space_pos=d.Locate(' '), space_count=0;
					while (space_pos!=KErrNotFound && space_count<6) {
						++space_pos;
						TInt second_space=d.Mid(space_pos).Locate(' ');
						if (second_space==KErrNotFound) {
							space_pos=KErrNotFound;
						} else {
							space_pos+=second_space;
						}
						++space_count;
					}
					if (space_pos==KErrNotFound) space_pos=d.Length();

					file->SetFileName(d.Left(space_pos));

					_LIT8(KDescName8, "description"); _LIT(KDescName, "description");
					MD5Update(&md5, KDescName8().Ptr(), KDescName8().Length());

					auto_ptr<HBufC8> descr(HBufC8::NewL(d.Length()));
					TPtr8 into= descr->Des();
					iUtf8CC->ConvertFromUnicode(into, d);
					MD5Update(&md5, descr->Des().Ptr(), descr->Des().Length());

					//Log(KDescName); Log(into);
					auto_ptr<CBufferPart> descpart(CBufferPart::NewL(descr.get(), ETrue,
						KDescName, _L(""))); descr.release();
					parts->AppendL(descpart.get());  descpart.release();
				}
				parts->AppendL(file.get()); file.release();
			}

			TBuf8<10> is_public, is_friend, is_family;

			if ( post->iSharing() == CCMPost::EPublic) {
				is_public=_L8("1");
			} else {
				is_public=_L8("0");
			}

			if ( post->iSharing() == CCMPost::EFriends) {
				is_friend=_L8("1");
			} else if (post->iSharing() == CCMPost::EFamily) {
				is_family=_L8("1");
			} else if (post->iSharing() == CCMPost::EFriendsAndFamily) {
				is_family=_L8("1");
				is_friend=_L8("1");
			}
			if (is_family.Length()>0) {
				CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("is_family"));
				_LIT8(KFamilyName8, "is_family"); _LIT(KFamilyName, "is_family");
				auto_ptr<CBufferPart> familypart(CBufferPart::NewL(is_family, KFamilyName, _L("")));
				parts->AppendL(familypart.get());  familypart.release();
				MD5Update(&md5, KFamilyName8().Ptr(), KFamilyName8().Length());
				MD5Update(&md5, is_family.Ptr(), is_family.Length());
			}
			if (is_friend.Length()>0) {
				CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("is_friend"));
				_LIT8(KFriendsName8, "is_friend"); _LIT(KFriendsName, "is_friend");
				auto_ptr<CBufferPart> friendspart(CBufferPart::NewL(is_friend, KFriendsName, 
					_L("")));
				parts->AppendL(friendspart.get());  friendspart.release();
				MD5Update(&md5, KFriendsName8().Ptr(), KFriendsName8().Length());
				MD5Update(&md5, is_friend.Ptr(), is_friend.Length());
			}
			if (is_public.Length()>0) {
				CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("is_public"));
				_LIT8(KPublicName8, "is_public"); _LIT(KPublicName, "is_public");
				auto_ptr<CBufferPart> publicpart(CBufferPart::NewL(is_public, KPublicName, _L("")));
				parts->AppendL(publicpart.get());  publicpart.release();
				MD5Update(&md5, KPublicName8().Ptr(), KPublicName8().Length());
				MD5Update(&md5, is_public.Ptr(), is_public.Length());
			}

			if (post) {

				CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("tags"));
				_LIT8(KTagsName8, "tags"); _LIT(KTagsName, "tags");
				MD5Update(&md5, KTagsName8().Ptr(), KTagsName8().Length());

				auto_ptr<HBufC8> tags(HBufC8::NewL(128));
				if (post->iTag)
					AddTag(tags, KNullDesC, (*post->iTag)(), iUtf8CC, &md5);

				{
					TBuf<50> project;
					if (Settings().GetSettingL(SETTING_PROJECT_NAME, project) && project.Length()>0) {
						AddTag(tags, KNullDesC, project, iUtf8CC, &md5);
					}
					if (Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, project) && project.Length()>0) {
						AddTag(tags, _L("from:"), project, iUtf8CC, &md5);
					}
				}

				CBBPresence* presence=bb_cast<CBBPresence>( post->iPresence() );
				if (presence) {
					CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("presence"));
					{
						TBuf<50> base;
						if (presence->iBaseInfo.iCurrent.iEntered() != TTime(0) &&
							post->iIncludedTagsBitField() & CCMPost::EBase) {
							base=presence->iBaseInfo.iCurrent.iBaseName();
							if (base.Length()>0 && presence->iCity().Length()>0) {
								TInt citypos=base.FindF(presence->iCity());
								if (citypos!=KErrNotFound) {
									base=base.Left(citypos);
									TInt colpos=base.FindF(_L(", "));
									if (colpos!=KErrNotFound) {
										base=base.Left(colpos);
									}
								}
							}
						}
						if (base.Length()==0 && post->iIncludedTagsBitField() & CCMPost::EBase) {
							base=presence->iCellName();
						}
						AddTag(tags, KNullDesC, 
							base, iUtf8CC, &md5);
					}
					/*
					if (presence->iBaseInfo.iPreviousVisit.iEntered() != TTime(0)) {
						TBuf<50> base=presence->iBaseInfo.iPreviousVisit.iBaseName();
						if (base.Length()>0) {
							TInt citypos=base.FindF(presence->iCity());
							if (citypos!=KErrNotFound) {
								base=base.Left(citypos);
								TInt colpos=base.FindF(_L(", "));
								if (colpos!=KErrNone) {
									base=base.Left(colpos);
								}
							}
							AddTag(tags, KNullDesC, 
								base, iUtf8CC, &md5);
						}
						AddTag(tags, _L("previously at "), 
							base, iUtf8CC, &md5);
					} */
					if (post->iIncludedTagsBitField() & CCMPost::ECity) {
						AddTag(tags, KNullDesC, presence->iCity(), iUtf8CC, &md5);
					}
					if (post->iIncludedTagsBitField() & CCMPost::ECountry) {
						AddTag(tags, KNullDesC, presence->iCountry(), iUtf8CC, &md5);
					}
					if (post->iIncludedTagsBitField() & CCMPost::ECell) {
						TBBCellId& cell= presence->iCellId;

						if (cell.iLocationAreaCode()!=0 || cell.iCellId()!=0)  {
							AddTag(tags, KNullDesC, _L("celltagged"), iUtf8CC, &md5);
							TBuf<15> code;
							code.AppendNum(cell.iMCC());
							AddTag(tags, _L("cell:mcc="), code, iUtf8CC, &md5);
							code.Zero(); code.AppendNum(cell.iMNC());
							AddTag(tags, _L("cell:mnc="), code, iUtf8CC, &md5);
							code.Zero(); code.AppendNum(cell.iLocationAreaCode());
							AddTag(tags, _L("cell:lac="), code, iUtf8CC, &md5);
							code.Zero(); code.AppendNum(cell.iCellId());
							AddTag(tags, _L("cell:cellid="), code, iUtf8CC, &md5);
						}
					}

					if (post->iIncludedTagsBitField() & CCMPost::ECalendar) {
						AddTag(tags, KNullDesC, presence->iCalendar.iCurrent.iDescription(),
							iUtf8CC, &md5);
					}

					if (post->iIncludedTagsBitField() & CCMPost::EGps) {
						TGeoLatLong coord;
						NmeaToLatLong(presence->iGps(), coord);
						if (coord.iLat.Length()>0 && coord.iLong.Length()>0) {
							AddTag(tags, KNullDesC, _L("geotagged"),
								iUtf8CC, &md5);
							AddTag(tags, _L("geo:lat="), coord.iLat,
								iUtf8CC, &md5);
							AddTag(tags, _L("geo:long="), coord.iLong,
								iUtf8CC, &md5);
						}
					}
					if (post->iSharing() != CCMPost::EPublic && presence->iDevices && post->iIncludedTagsBitField() & CCMPost::EBt) {
						if (presence->iDevices->Count() > 0) {
							AddTag(tags, KNullDesC, _L("bluetagged"), iUtf8CC, &md5);
						}
						for (TBBBtDeviceInfo* i=presence->iDevices->First();
								i; i=presence->iDevices->Next()) {
							TBuf<18> mac; i->iMAC.IntoStringL(mac);
							AddTag(tags, _L("bt="), mac, iUtf8CC, &md5);
						}
					} else {
						AddTag(tags, KNullDesC, KNoBlue, iUtf8CC, &md5);
					}
				}
				{
					AddTag(tags, KNullDesC, KContextFlickr, iUtf8CC, &md5);
				}
#ifdef __JAIKU__
				{
					AddTag(tags, KNullDesC, KJaiku, iUtf8CC, &md5);
				}
#endif
				//Log(KTagsName); Log(*tags);
				auto_ptr<CBufferPart> publicpart(CBufferPart::NewL(tags.get(), ETrue,
					KTagsName, _L("")));
				tags.release();
				parts->AppendL(publicpart.get());  publicpart.release();
			}


			TBuf8<16> hash; hash.SetLength(hash.MaxLength());
			MD5Final( (TUint8*)hash.Ptr(), &md5);
			TBuf8<32> sig;
			for (int i=0; i<16; i++) {
				sig.AppendNumFixedWidth( hash[i], EHex, 2 );
			}
			auto_ptr<CBufferPart> sigpart(CBufferPart::NewL(sig, KSigName, _L("")));
			parts->AppendL(sigpart.get());  sigpart.release();
			//Log(KSigName); Log(sig);

		}
	}
	Reporting().DebugLog(_L("GetFileInfoL5"));
	if (file.get()) {
		CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Finalizing"));
		parts->AppendL(file.get()); file.release();
	}
	//User::Panic(_L("TT"), 1);
	return parts.release();
}

void CHttpTransfer2Impl::StartFileTransferL()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("StartFileTransferL"));
	TBool gotToFile=EFalse;
	TRAPD_ERRORINFO(err, info, InnerStartFileTransferL(gotToFile));
	if (err.iCode!=KErrNone && gotToFile) {
		TBool dummy=EFalse;
		error(this, err.iCode, KNullDesC, info, dummy);
	} else if (err.iCode!=KErrNone) {
		User::Leave(err.iCode);
	}
}

void CHttpTransfer2Impl::SetCompletionL(TCompletion aCompletion)
{
	iTable.UpdateL();
	iTable.SetColL(12, aCompletion);
	PutL();
	iTable.GetL();
}

void CHttpTransfer2Impl::InnerStartFileTransferL(TBool& aGotToFile)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("InnerStartFileTransferL"));

	iErrorInfo->Zero(); iReply.Zero();
	iSentAll=iGotReply=iGotError=false;
	iExpect->Reset();
	delete iFlickrResponse; iFlickrResponse=0;
	delete iParser; iParser=0;

	iTable.Cancel();

	if (! MoveToCurrent()) {
		if (iStatusNotif) iStatusNotif->error(this, -1031, _L("current record disappeared"));
		Reset();
		if (iTable.FirstL()) {
			iWaitTime=5;
			Trigger();
		}
		return;
	}

	aGotToFile=ETrue;

	bool done=false;
	while (!done) {
		bool goto_next=false;
		current_state=PROCESSING_REQUEST;
		// skip files in use
		iTable.GetL();
		iTempFileName=iTable.ColDes16(1);
		iTempFileName2=iTable.ColDes16(5);
		RFile f;
		TInt err;
		err=f.Open(Fs(), iTempFileName, EFileRead|EFileShareAny);
		TOp Op=(TOp)iTable.ColInt32(7);
		TCompletion completion=INPROGRESS;
		if (! iTable.IsColNull(12) ) {
			completion=(TCompletion)iTable.ColInt32(12);
		}
		if ( completion == STARTED) {
			TBool reportonly=ETrue;
			error(this, KErrDied, _L("Died while processing this item. See logs for reason."), 0, reportonly);
			if (reportonly) {
				completion=INPROGRESS;
				Cancel();
			} else {
				if (err==KErrNone) f.Close();
				return;
			}
		}
		if ( completion == INPROGRESS ) {
			CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("InProgress"));
			{
				SetCompletionL(STARTED);
			}

			TBool DontMove;
			if (iTable.IsColNull(8)) {
				DontMove=EFalse;
			} else {
				DontMove=iTable.ColInt32(8);
			}
			if (err==KErrNone) {
				f.Close();
				switch (Op) {
				case UPLOAD: {
					CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("UPLOAD"));
					if (!DontMove) {
						if (iTempFileName.Find(KUploadDir)==KErrNotFound) {
							iPacketFileName=iTable.ColDes16(6);
							if (iPacketFileName.Length()>0) {
								_LIT(Kcontext, "context");
								TFileName filen; TParse p;
								if (iPacketFileName.FindC(Kcontext)!=KErrNotFound) {
									p.Set(iTempFileName, 0, 0);
									MakeFileName();
									filen=p.DriveAndPath();
									RemoveOurPath(filen, p);
									filen.Append(KUploadDir);
									filen.Append(iFileNameBuf);
									filen.Append(p.Ext());
								} else {
									p.Set(iTempFileName, 0, 0);
									filen=iPacketFileName;
									filen.Replace( filen.Length()-4, 4, p.Ext());
								}
								TAutomaticTransactionHolder th(*this);
								iTable.UpdateL();
								if ( Fs().Rename(iTempFileName, filen) == KErrNone ) {
									iTempFileName=filen;
									iTable.SetColL(1, iTempFileName);
									TInt64 postid=0;
									if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);
									if (postid!=0) CallStateChanged(postid, 
										MPostUploadCallback::EQueued, 
										filen, 0);
								}
								PutL();
							}
						}
					}
					}
					break;
				case DELETE:
					{
					CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("DELETE"));
					TInt err=Fs().Delete(iTempFileName);
					TInt64 postid=0;
					if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);

					if (postid!=0) {
						if (err!=KErrNone && err!=KErrNotFound && err!=KErrBadName) {
							User::Leave(err);
						}
						CallStateChanged(postid, MPostUploadCallback::EDeleted, 
							KNullDesC, 0);
					} else {
						if (err!=KErrNone && err!=KErrNotFound && err!=KErrBadName &&
								err!=KErrInUse) {
							User::Leave(err);
						}
					}
					goto_next=true;
					}
					break;
				case DONTSHARE:
					{
					CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("DONTSHARE"));
					DoNotTransferL(iTempFileName, iTempFileName2);
					TInt64 postid=0;
					if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);
					{
						iTable.UpdateL();
						TAutomaticTransactionHolder th(*this);
						iTable.SetColL(1, iTempFileName2);
						iTable.SetColL(12, NOTSHARED);
						PutL();
					}
					if (postid!=0) CallStateChanged(postid, 
						MPostUploadCallback::ENotShared, 
						iTempFileName2, 0);
					goto_next=true;
					}
					break;
				case MOVETOOLD:
					{
					CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("MOVETOOLD"));
					MoveToOldL(iTempFileName, iTempFileName2);
					TInt64 postid=0;
					if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);
					{
						iTable.UpdateL();
						TAutomaticTransactionHolder th(*this);
						iTable.SetColL(1, iTempFileName2);
						iTable.SetColL(12, OLD);
						PutL();
					}
					if (postid!=0) CallStateChanged(postid, 
						MPostUploadCallback::ENotShared, 
						iTempFileName2, 0);
					goto_next=true;
					}
					break;
				}
			}
		} else {
			CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Done"));
			goto_next=true;
			TInt64 postid=0;
			if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);
			if (postid!=0) {
				CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("CallStateChanged"));
				switch(completion) {
					case DELETED:
						CallStateChanged(postid, 
						MPostUploadCallback::EDeleted, 
						KNullDesC, 0);
						break;
					case UPLOADED:
						CallStateChanged(postid, 
						MPostUploadCallback::EUploaded, 
						iTempFileName, 0);
						break;
					case NOTSHARED:
					case OLD:
						CallStateChanged(postid, 
						MPostUploadCallback::ENotShared, 
						iTempFileName, 0);
						break;
				}
			}
		}
		if (goto_next) {
			iTable.DeleteL();
			if (iTable.NextL()) {
				current_state=SENDING_FILE;
				iCurrentFileMark=iTable.Bookmark();
				iWaitTime=0;
				Trigger(ETrue);
			} else {
				Reset();
				iWaitTime=15;
				Trigger();
			}
			return;
		}
		if (err==KErrAccessDenied || err==KErrInUse) {
		} else if (err==KErrNotFound || err==KErrBadName) {
			TInt64 postid=0;
			if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);
			if (Op==DELETE) {
				if (postid!=0) CallStateChanged(postid, 
					MPostUploadCallback::EDeleted, 
					KNullDesC, 0);
			} else if (Op==UPLOAD) {
				//FIXME
				if (postid!=0) CallStateChanged(postid, 
					MPostUploadCallback::EFailed, 
					KNullDesC, 0);
			}
			iPacketFileName=iTable.ColDes16(6);
			Fs().Delete(iPacketFileName);
			iTable.DeleteL();
		} else {
			iCurrentFileMark=iTable.Bookmark();
			done=true;
		}
		if (!done) {
			if (! iTable.NextL()) {
				Reset();
				iWaitTime=15;
				Trigger();
				return;
			}
		}
	}

	if (iNoTransfer) {
		if (iTable.NextL()) {
			iCurrentFileMark=iTable.Bookmark();
			Trigger();
		} else {
			Reset();
		}
	} else {
		CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Sending"));
		if (!iHttp) {
			ConnectL();
		} else {
			{
				TInt url_setting, script_setting;

				url_setting=iTable.ColInt32(2);
				script_setting=iTable.ColInt32(3);

				TDes& url=iUrl;
				TBuf<50> script;
				Settings().GetSettingL(url_setting, url);
				Settings().GetSettingL(script_setting, script);
				url.Append(script);
			}

			iPacketFileName=iTable.ColDes16(6);
			TEntry e;
			if (Fs().Entry(iTempFileName, e)==KErrNone && e.iSize==0) {
				current_state=SENDING_FILE;
				Fs().Delete(iTempFileName);
				success(iHttp);
				return;
			}

			CPtrList<CPostPart>* parts=GetFileInfoL(iTempFileName, iPacketFileName);
			iCurrentlySendingFileName=iTempFileName;

			current_state=SENDING_FILE;
			iHttp->PostL(iIap, iUrl, parts);
		}
	}
}

void CHttpTransfer2Impl::Reset()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("Reset"));

	iCurrentlySendingFileName.Zero();
	Cancel();
	delete iHttp; iHttp=0;
	current_state=IDLE;
	ShowUploading(EDone);
}

void CHttpTransfer2Impl::HandleSent()
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("HandleSent"));

	iTable.Cancel();

	if (! MoveToCurrent() ) {
		if (iStatusNotif) iStatusNotif->error(this, -1031, _L("current record disappeared"));
		Reset();
		if (iTable.FirstL()) {
			iWaitTime=5;
			Trigger();
		}
		return;
	}

	iTable.GetL();
	iTable.UpdateL();

	iTempFileName=iTable.ColDes16(1);
	iPacketFileName=iTable.ColDes16(6);

	Fs().Delete(iPacketFileName);

	TBool del=iTable.ColInt(4);

	TInt64 postid=0;
	if (! iTable.IsColNull(10)) postid=iTable.ColInt64(10);
	if(del) {
		TInt err=Fs().Delete(iTempFileName);
		if (err==KErrNone || err==KErrBadName || err==KErrNotFound) {
			iTable.SetColL(12, DELETED);

			if (postid!=0) {
				CC_TRAP(err, CallStateChanged(postid, MPostUploadCallback::EDeleted, KNullDesC, 0));
			}
			if (err!=KErrNone) {
				PutL();
				return;
			}
		} else {
			iTable.SetColL(7, DELETE);
			PutL();
			return;
		}
	} else {
		iTable.SetColL(12, UPLOADED);
		TFileName moveto=iTempFileName;
		TParse p;
		if (moveto.FindF(KUploadDir)!=KErrNotFound) {
			moveto.Replace(moveto.Find(KUploadDir), KUploadDir().Length(), KUploadedDir);
		} else {
			p.Set(iTempFileName, 0, 0);
			RemoveOurPath(moveto, p);
			moveto.Append(KUploadedDir);
			MakeFileName();
			moveto.Append(iFileNameBuf);
			moveto.Append(p.Ext());
		}
		p.Set(moveto, 0, 0);
		BaflUtils::EnsurePathExistsL(Fs(), p.DriveAndPath());
		TInt err=Fs().Rename(iTempFileName, moveto);
		if (postid!=0) {
			if (err==KErrNone) {
				CC_TRAP(err, CallStateChanged(postid, MPostUploadCallback::EUploaded, moveto, 0));
			} else {
				CC_TRAP(err, CallStateChanged(postid, MPostUploadCallback::EUploaded, KNullDesC, 0));
				moveto=iTempFileName;
			}
			if (err!=KErrNone) {
				PutL();
				return;
			}
			if (iFlickrResponse && iCurrentFeedItem) {
				iCurrentFeedItem->iThumbnailUrl.Append(_L("photoid://"));
				TBuf<10> photoid; photoid.AppendNum(iFlickrResponse->PhotoId());
				iCurrentFeedItem->iThumbnailUrl.Append(photoid);
				iCurrentFeedItem->iMediaFileName()=moveto;
				TTime expires = GetTime(); expires+=TTimeIntervalDays(2);
				BBSession()->PutRequestL( KOutgoingFeedItem, KNullDesC, iCurrentFeedItem, 
					expires, KOutgoingTuples);
					
				TBuf<40> subname; iCurrentFeedItem->iUuid.IntoStringL(subname);
				BBSession()->PutL( KInternalFeedItem, subname, iCurrentFeedItem, 
					expires);
			}
		}
	}
	iTable.Cancel();
	iTable.DeleteL();
}

void CHttpTransfer2Impl::RemoveFromQueueL(const TDesC& aFileName)
{
	iTable.Cancel();

	TBool current_file=EFalse;
	if (current_file=(iCurrentlySendingFileName.Compare(aFileName)==0)) Reset();

	TDbSeekKey k(aFileName);
	TBool ret=iTable.SeekL(k);
	while(ret) {
		iTable.GetL();
		if ( aFileName.Left(iFileNameIndexLen).Compare(iTable.ColDes(1).Left(iFileNameIndexLen)) ) return;
		if ( aFileName.Compare(iTable.ColDes(1)) == 0) {
			TAutomaticTransactionHolder th(*this);
			iTable.DeleteL();
		}
		ret=iTable.NextL();
	}

	if (current_file) {
		iWaitTime=1;
		Trigger();
	}
}

TBool CHttpTransfer2Impl::FileIsQueued(const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("FileIsQueued"));
	iTable.Cancel();

	TDbSeekKey k(aFileName);
	TBool ret=iTable.SeekL(k);
	while(ret) {
		iTable.GetL();
		if ( aFileName.Left(iFileNameIndexLen).Compare(iTable.ColDes(1).Left(iFileNameIndexLen)) ) return EFalse;
		if ( aFileName.Compare(iTable.ColDes(1)) == 0) return ETrue;
		ret=iTable.NextL();
	}
	return EFalse;
}

void CHttpTransfer2Impl::NotifyHttpStatus(THttpStatus st, TInt aError)
{
	switch(st) {
		case EHttpConnected:
			if (iStatusNotif) iStatusNotif->info(this, _L("connected"));
			break;
		case EHttpSentAll:
			iSentAll=ETrue;
			break;
		case EHttpDisconnected:
			if (iGotReply) {
				iHttp->ReleaseParts();
				if (!iFlickrResponse) {
					success(iHttp);
				} else {
					if (! iFlickrResponse->Ok()) {
						error(iFlickrResponse, iFlickrResponse->ErrorCode(), 
							iFlickrResponse->ErrorMessage());
					} else {
						success(iHttp);
					}
				}
			} else if (iGotError) {
				//ignore
			} else {
				if (iReply.Length()==0) iReply=_L("http disconnected, no data received");
				error(iHttp, aError, iReply);
			}
			break;
		case EHttpError:
			iGotError=true;
			error(iHttp, aError, _L("http error"));
			break;
	};
}

void CHttpTransfer2Impl::NotifyNewHeader(const CHttpHeader &aHeader)
{
	iState=CCnvCharacterSetConverter::KStateDefault; iReply.Zero();
	if (aHeader.iHttpReplyCode < 200 || aHeader.iHttpReplyCode>299) {
		iGotError=true;
		TBuf<40> err=_L("server replied with ");
		err.AppendNum(aHeader.iHttpReplyCode);
		error(iHttp, KErrGeneral, err);
	}
}

void CHttpTransfer2Impl::NotifyNewBody(const TDesC8 &chunk)
{
	if ( ! iUtf8CC ) {
		iUtf8CC=CCnvCharacterSetConverter::NewL();
		iUtf8CC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());
		iState=CCnvCharacterSetConverter::KStateDefault;
	}
	TInt i=0;
	while (iReply.Length() < iReply.MaxLength() && i < chunk.Length() ) {
		TPtr into( (TUint16*)iReply.Ptr() + iReply.Length(), 0, iReply.MaxLength()-iReply.Length() );
		iUtf8CC->ConvertToUnicode(into, chunk.Mid(i, 1), iState);
		iReply.SetLength( iReply.Length() + into.Length() );
		i++;
	}

	if (iGotReply || iGotError) return;
	if (!iFlickrResponse) {
		for (i=0; i< chunk.Length(); i++) {
			if (iExpect->handle_input( chunk[i] )) {
				iGotReply=true;
				return;
			}
		}
	} else {
		if (!iFlickrResponse->Done()) {
			iParser->Parse( (const char*)chunk.Ptr(), chunk.Length(), 0);
			if (iFlickrResponse->Done()) {
				iGotReply=ETrue;
				if (iFlickrResponse->Ok()) {
					iGotError=EFalse;
				} else {
					iGotError=ETrue;
				}
			}
		}
	}
}

TFileName CHttpTransfer2Impl::GetDoNotTransferFileName(const TDesC& aFileName)
{
	TFileName filen;
	TParse p; p.Set(aFileName, 0, 0);
	filen = p.DriveAndPath();
	RemoveOurPath(filen, p);

	filen.Append(KNoUploadDir);
	BaflUtils::EnsurePathExistsL(Fs(), filen);

	MakeFileName();
	filen.Append(iFileNameBuf);
	filen.Append(p.Ext());

	return filen;
}


TFileName CHttpTransfer2Impl::GetMoveToOldFileName(const TDesC& aFileName)
{
	TFileName fn;
	
	TParse p; p.Set(aFileName, 0, 0);
	fn = p.DriveAndPath();
	if (! fn.Right( KOldDir().Length() ).Compare( KOldDir ) ) {
		return aFileName;
	}
	RemoveOurPath(fn, p);
	fn.Append(KOldDir);
	BaflUtils::EnsurePathExistsL(Fs(), fn);
	MakeFileName();
	fn.Append(iFileNameBuf);
	fn.Append(p.Ext());

	return fn;
}

void CHttpTransfer2Impl::ShowUploading(TUploadingStatus aStatus)
{
	if ( aStatus == EDone ) {
		delete iNotifyState; iNotifyState=0;
		delete iUtf8CC; iUtf8CC=0;
	} else {
		if (! iNotifyState ) {
			iNotifyState=CNotifyState::NewL(AppContext(), KIconFile);
		}
		if (aStatus == EUploading) {
			iNotifyState->SetCurrentState(EMbmContextnetworkUploading, EMbmContextnetworkUploading);
		} else {
			iNotifyState->SetCurrentState(EMbmContextnetworkUploading_err, EMbmContextnetworkUploading);
		}
	}
}

void CHttpTransfer2Impl::AddPostCallbackL(MPostUploadCallback* aCallback)
{
	iCallbacks->AppendL(aCallback);
}
void CHttpTransfer2Impl::RemovePostCallback(MPostUploadCallback* aCallback)
{
	CList<MPostUploadCallback*>::Node *i=0;
	for (i=iCallbacks->iFirst; i; i=i->Next) {
		if (i->Item == aCallback) {
			iCallbacks->DeleteNode(i, ETrue);
			return;
		}
	}
}

void CHttpTransfer2Impl::CallStateChanged(TInt64 aPostId, MPostUploadCallback::TState aState, 
					  const TDesC& aFileName, const CBBErrorInfo* aError)
{
	CList<MPostUploadCallback*>::Node *i=0;
	for (i=iCallbacks->iFirst; i; i=i->Next) {
		i->Item->StateChangedL(aPostId, aState, aFileName, aError);
	}
}
