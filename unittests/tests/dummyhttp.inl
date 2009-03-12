_LIT(KFailUrl, "fail");

class CDummyHttp : public CHttp, public MTimeOut {
public:
	virtual void GetL(const TUint& iAP, const TDesC &url, const TTime &modified_since = TTime(0), 
		const TUint &chunkStart=0, const TUint &chunkEnd=0) {
		
		iTimeOut->WaitShort(10);
		iState=GET_HEAD;
		iUrl=url;
	}
	
	CDummyHttp(MHttpObserver& aObserver) : iObserver(aObserver) { }
	
	MHttpObserver& iObserver;
	CTimeOut*	iTimeOut;
	TBuf<200>	iUrl;
	
	enum TState { IDLE, GET_CONNECTING, GET_HEAD, GET_BODY, GET_CLOSING };
	TState iState;
	
	void ConstructL() {
		iTimeOut=CTimeOut::NewL(*this);
	}
	virtual void PostL(const TUint& iAP, const TDesC &url, CPtrList<CPostPart>* aBodyParts) {
		User::Leave(KErrNotSupported);
	}
	~CDummyHttp() {
		delete iTimeOut;
	}
	virtual void expired(CBase*) {
		switch(iState) {
		case GET_CONNECTING:
			iTimeOut->WaitShort(10);
			iState=GET_HEAD;
			iObserver.NotifyHttpStatus(MHttpObserver::EHttpConnected, KErrNone);
			break;
		case GET_HEAD:
			iTimeOut->WaitShort(10);
			iState=GET_BODY;
			if (iUrl==KFailUrl) {
				auto_ptr<CHttpHeader> head(CHttpHeader::NewL());
				head->iHttpVersion=1.0;
				head->iHttpReplyCode=404;
				iState=GET_CLOSING;
				iObserver.NotifyNewHeader(*head);
			} else {
				auto_ptr<CHttpHeader> head(CHttpHeader::NewL());
				head->iHttpVersion=1.0;
				head->iHttpReplyCode=200;
				head->iFilename=_L("get.jpg");
				head->iContentType=_L("image/jpeg");
				head->iSize=100;
				iObserver.NotifyNewHeader(*head);
			}
			break;
		case GET_BODY:
			iTimeOut->WaitShort(10);
			iState=GET_CLOSING;
			{
				TBuf8<100> b; b.SetLength(b.MaxLength()); b.FillZ();
				iObserver.NotifyNewBody(b);
			}
			break;
		case GET_CLOSING:
			iState=IDLE;
			iObserver.NotifyHttpStatus(MHttpObserver::EHttpDisconnected, KErrNone);
			break;
		};
	}
	//void HeadL(const TDesC &url, const TTime &modified_since);
	virtual void ReleaseParts() { }
	virtual void Disconnect() { 
		if (iState!=IDLE && iState!=GET_CLOSING) {
			iTimeOut->WaitShort(10);
			iState=GET_CLOSING;
		}
	}
	static CHttp* NewL(MHttpObserver& aObserver, MApp_context& Context, const TDesC& aConnectionName) {
		auto_ptr<CDummyHttp> ret(new (ELeave) CDummyHttp(aObserver));
		ret->ConstructL();
		return ret.release();
	}

};
