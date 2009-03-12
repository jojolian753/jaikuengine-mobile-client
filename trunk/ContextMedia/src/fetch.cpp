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
#include "fetch.h"

#include "cn_http.h"
#include "bb_incoming.h"
#include "cl_settings.h"
#include "bbxml.h"
#include "cbbsession.h"
#include "fetch_request.h"
#include "independent.h"
#include <basched.h>
#include "bb_settings.h"
#include <charconv.h>
#include "reporting.h"

const TTypeName KFetchStreamType = { { CONTEXT_UID_CONTEXTMEDIA }, 2, 1, 0 };

class CFetchPostsImpl : public CFetchPosts, public MContextBase, public MHttpObserver,
	public MIncomingObserver, public MBBStream /*, public MBBObserver*/  {

	// own
	CFetchPostsImpl(MApp_context& Context, MPostReceiver& aReceiver);
	void ConstructL();
	~CFetchPostsImpl();

	// CFetchPosts
	virtual void FetchL(const CBBFetchPostRequest * aReq);
	virtual void Stop(); // won't call Finished()

	// MHttpObserver
	virtual void NotifyHttpStatus(THttpStatus st, TInt aError);
	virtual void NotifyNewHeader(const CHttpHeader &aHeader);
	virtual void NotifyNewBody(const TDesC8 &chunk);

	// MIncomingObserver
	virtual void IncomingData(const MBBData* aData, TBool aErrors);
	virtual void StreamOpened();
	virtual void StreamClosed();
	virtual void StreamError(TInt aError, const TDesC& aDescr); // Leave to interrupt processing

	// MBBObserver 
//	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
	//	const TComponentName& aComponentName, const MBBData* aData);
	

	// MBBStream
	const MBBData* Part(TUint aPartNo) const;
        virtual const TTypeName& Type() const;
	virtual void ResetPart(TUint aPart);

	// checkedactive
	void CheckedRunL();
	void DoCancel() { }

	// own
	void Async();

	void Reset();
	enum TState { EIdle, EFetching };
	TState		iCurrentState;

	MPostReceiver&	iReceiver;
	CHttp*		iHttp;
	
	CCMPost*	iPost;
	TBool		iGotError, iGotReply;
	CBBDataFactory*	iFactory;
	TBool		iStopping;
	CPtrList<CCMPost> *iPosts;

	//CBBSubSession * iBBSubSession;

	friend class CFetchPosts;
	friend class auto_ptr<CFetchPostsImpl>;
};

_LIT(KPosts, "posts");
_LIT(KFetchPosts, "CFetchPosts");

CFetchPosts::CFetchPosts() : CCheckedActive(EPriorityLow, KFetchPosts) { }

CFetchPostsImpl::CFetchPostsImpl(MApp_context& Context, MPostReceiver& aReceiver) : MContextBase(Context),
	MBBStream(KPosts, *this), iReceiver(aReceiver)
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("CFetchPostsImpl"));

}

void CFetchPostsImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("ConstructL"));

//	iBBSubSession=BBSession()->CreateSubSessionL(this);
//	iBBSubSession->AddNotificationL(KFetchPostRequestTuple);

	MBBStream::ConstructL();
	iFactory=CBBDataFactory::NewL();
	iPost=CCMPost::NewL(iFactory);
	iPosts=CPtrList<CCMPost>::NewL();

	CActiveScheduler::Add(this);
}

void CFetchPostsImpl::Async()
{
	if (IsActive()) return;
	TRequestStatus* s=&iStatus;
	User::RequestComplete(s, KErrNone);
	SetActive();
}

void CFetchPostsImpl::CheckedRunL()
{
	TAutoBusy busy(Reporting());
	CCMPost* p=iPosts->Pop();
	iReceiver.NewPost( p, EFalse );
	p->Release();
	if (iPosts->iCount>0) Async();
}

CFetchPostsImpl::~CFetchPostsImpl()
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("~CFetchPostsImpl"));

	Cancel();

	delete iHttp;
	if (iPost) iPost->Release();
	delete iPosts;
	delete iFactory;
}

void CFetchPostsImpl::FetchL(const CBBFetchPostRequest * aReq)
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("FetchL"));

	if (!aReq->iFetchItemList->Count() || aReq->iTargetUrl().Length()<10) User::Leave(KErrArgument);
	if (iCurrentState!=EIdle) User::Leave(KErrServerBusy);

	MBBStream::Reset();

	TInt iap=-1;
	if ( !Settings().GetSettingL(aReq->iIAPSetting(), iap ) || iap == -1) {
		Stop();
		_LIT(KErrorText, "Incorrect IAP, Check connection settings.");
		Reporting().ShowGlobalNote(4, KErrorText);
		User::Leave(KErrCouldNotConnect);
	}
	
	auto_ptr<CXmlBufExternalizer> xmlb(CXmlBufExternalizer::NewL(2048));
	xmlb->Declaration(_L("iso-8859-1"));
	aReq->iFetchItemList->IntoXmlL(xmlb.get());

	auto_ptr<HBufC8> b8(HBufC8::NewL( xmlb->Buf().Length()+20 ) );
	TPtr8 p(b8->Des());
	CC()->ConvertFromUnicode(p, xmlb->Buf() );

	auto_ptr<CBufferPart> bp(CBufferPart::NewL(b8.get(), ETrue, _L("fetch"), _L("text/xml")));
	b8.release();

	auto_ptr< CPtrList<CPostPart> > parts(CPtrList<CPostPart>::NewL());
	parts->AppendL(bp.get());
	bp.release();

	if (!iHttp) iHttp=CHttp::NewL(*this, AppContext());

	iStopping=EFalse;
	iGotReply=EFalse;

	iHttp->PostL(iap, aReq->iTargetUrl(), parts.release()); // guaranteed to take ownership of parts
}


void CFetchPostsImpl::Stop()
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("Stop"));

	if (iCurrentState==EIdle) return;

	iStopping=ETrue;
	iHttp->Disconnect();
	MBBStream::Reset();
	iStopping=EFalse;
	iCurrentState=EIdle;
}

// MHttpObserver
void CFetchPostsImpl::NotifyHttpStatus(THttpStatus st, TInt aError)
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("NotifyHttpStatus"));

	if (iStopping) return;

	switch(st) {
		case EHttpConnected:
			break;
		case EHttpDisconnected:
			{
				iCurrentState=EIdle;
				iHttp->Disconnect();
				if (iGotReply || aError==KErrEof) {
					iHttp->ReleaseParts();
				} else if (iGotError) {
					Reset();
				} else {
					Reset();
					iReceiver.Error(aError, _L("http disconnected"));
					/*auto_ptr<CCMPost> reply (CCMPost::NewL(0));
					reply->iErrorDescr() = _L("http disconnected");
					reply->iErrorCode() = aError;
					iBBSubSession->PutRequestL(KPostTuple, KNullDesC, reply.get(), KContextMediaComponent);
					*/
				}
				iReceiver.Finished();
				//auto_ptr<CBBFetchPostRequest> reply (CBBFetchPostRequest::NewL(0, KNullDesC));
				//iBBSubSession->PutReplyL(KFetchPostRequestTuple, KNullDesC, reply.get(), KContextMediaComponent);
			}
			break;
		case EHttpError:
			{
				iGotError=true;
				iReceiver.Error(aError, _L("http disconnected"));
				//auto_ptr<CCMPost> reply (CCMPost::NewL(0));
				//reply->iErrorDescr() = _L("http disconnected");
				//reply->iErrorCode() = aError;
				//iBBSubSession->PutRequestL(KPostTuple, KNullDesC, reply.get(), KContextMediaComponent);
			}
			break;
	}
}

void CFetchPostsImpl::NotifyNewHeader(const CHttpHeader &aHeader)
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("NotifyNewHeader"));

	if (aHeader.iHttpReplyCode < 200 || aHeader.iHttpReplyCode>299) {
		iGotError=true;
		TBuf<40> err=_L("server replied with ");
		err.AppendNum(aHeader.iHttpReplyCode);
		
		iReceiver.Error(KErrGeneral, err);
		//auto_ptr<CCMPost> reply (CCMPost::NewL(0));
		//reply->iErrorDescr() = err;
		//reply->iErrorCode() = KErrGeneral;
		//iBBSubSession->PutRequestL(KPostTuple, KNullDesC, reply.get(), KContextMediaComponent);
	} else {
		if (aHeader.iSize==0) iGotReply=ETrue;
	}
}

void CFetchPostsImpl::NotifyNewBody(const TDesC8 &chunk)
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("NotifyNewBody"));

	ParseL(chunk);
}

// MIncomingObserver
void CFetchPostsImpl::IncomingData(const MBBData* aData, TBool aErrors)
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("IncomingData"));

	const CCMPost *post=bb_cast<CCMPost>(aData);
	if (post) {
#ifdef __WINS__
		TBuf<100> msg=_L("*** New post ");
		msg.AppendNum( post->iPostId() );
		RDebug::Print(msg);
#endif
		auto_ptr<CCMPost> p( bb_cast<CCMPost>(post->CloneL(KNullDesC)));
		iPosts->AppendL(p.get());
		p.release();
		Async();
	} else {
		RDebug::Print(_L("*** New data but not a post"));
	}
}

void CFetchPostsImpl::StreamOpened()
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("StreamOpened"));

	iGotReply=EFalse;
}
void CFetchPostsImpl::StreamClosed()
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("StreamClosed"));

	iGotReply=ETrue;
}

void CFetchPostsImpl::StreamError(TInt aError, const TDesC& aDescr) // Leave to interrupt processing
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("StreamError"));
#ifdef __WINS__
	TBuf<100> msg=_L("*** StreamError ");
	msg.AppendNum( aError );
	msg.Append(_L(" "));
	msg.Append(aDescr.Left(60));
	RDebug::Print(msg);
#endif

	iReceiver.Error(aError, aDescr);
	//auto_ptr<CCMPost> reply (CCMPost::NewL(0));
	//reply->iErrorDescr() = aDescr;
	//reply->iErrorCode() = aError;
	//iBBSubSession->PutRequestL(KPostTuple, KNullDesC, reply.get(), KContextMediaComponent);		
}

// MBBStream
const MBBData* CFetchPostsImpl::Part(TUint aPartNo) const
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("Part"));

	if (aPartNo==0) return iPost;
	return 0;
}

const TTypeName& CFetchPostsImpl::Type() const
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("Type"));

	return KFetchStreamType;
}

void CFetchPostsImpl::ResetPart(TUint aPart)
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("ResetPart"));

	if (aPart==0) {
		if (iPost) iPost->Release(); 
		iPost=0;
		iPost=CCMPost::NewL(iFactory);
	}
}

// own
void CFetchPostsImpl::Reset()
{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("Reset"));

	delete iHttp; iHttp=0;
}

EXPORT_C CFetchPosts* CFetchPosts::NewL(MApp_context& Context, MPostReceiver& aReceiver)
{
	CALLSTACKITEM_N(_CL("CFetchPosts"), _CL("NewL"));

	auto_ptr<CFetchPostsImpl> ret(new (ELeave) CFetchPostsImpl(Context, aReceiver));
	ret->ConstructL();
	return ret.release();
}

/*
// MBBObserver

void CFetchPostsImpl::NewValueL(TUint aId, const TTupleName& aName, const TDesC& , 
		const TComponentName& , const MBBData* aData)

{
	CALLSTACKITEM_N(_CL("CFetchPostsImpl"), _CL("NewValueL"));

	if (aName==KFetchPostRequestTuple) {
		if (iCurrentState!=EIdle) Stop();
		const CBBFetchPostRequest *req=bb_cast<CBBFetchPostRequest>(aData);
		if (req && (req->iTargetUrl()!=KNullDesC)) FetchL(req);
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
*/
/*
void do_run_fetcher(TAny* aPtr)
{
	worker_info *wi=(worker_info*)aPtr;

	auto_ptr<COwnActiveScheduler> s(new (ELeave) COwnActiveScheduler);
	CActiveScheduler::Install(s.get());
	auto_ptr<CStopActive> stop(new (ELeave) CStopActive);
	stop->ConstructL(wi);

	auto_ptr<CApp_context> appc(CApp_context::NewL(true, _L("fetcher")));
	appc->SetDataDir(_L("c:\\system\\apps\\contextmediaapp\\"), false);
	TNoDefaults t;
	CBlackBoardSettings* settings=
		CBlackBoardSettings::NewL(*appc, t, KCLSettingsTuple);
	appc->SetSettings(settings);

	auto_ptr<CBBDataFactory> bbf(CBBDataFactory::NewL());
	auto_ptr<CBBSession> bbs(CBBSession::NewL(*appc, bbf.get()));
	appc->SetBBSession(bbs.get());
	appc->SetBBDataFactory(bbf.get());

	auto_ptr<CFetchPosts> b(CFetchPosts::NewL(*appc));

	s->Start();
}


EXPORT_C TInt CFetchPosts::RunFetcherInThread(TAny* aPtr)
{
        CTrapCleanup *cl;
        cl=CTrapCleanup::New();

        TInt err=0;
        CC_TRAP(err, do_run_fetcher(aPtr));

	delete cl;

	TTimeIntervalMicroSeconds32 w(50*1000);
	User::After(w);
	worker_info* wi=(worker_info*)aPtr;
	wi->stopped(err);
        return err;
}
*/
