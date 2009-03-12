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
#include "cn_http.h"
#include <charconv.h>
#include "cl_settings.h"
#include "compat_int64.h"

_LIT8(HTTP_HEAD, "HEAD %S HTTP/1.0\r\n\r\n");
_LIT8(HTTP_GET_MAIN, "GET %S HTTP/1.0\r\n");
_LIT8(HTTP_POST_MAIN, "POST %S HTTP/1.0\r\n");
_LIT8(HTTP_POST_HOST, "Host: %S\r\n");
_LIT8(HTTP_POST_CONTENTTYPE, "Content-Type: %S\r\n");
_LIT8(HTTP_POST_CONTENTLENGTH, "Content-Length: %d\r\n");
_LIT8(HTTP_POST_CONTENTDISPOSITION, "Content-Disposition: form-data; name=\"%S\"\r\n");
_LIT8(HTTP_POST_CONTENTDISPOSITIONFILE, "Content-Disposition: form-data; name=\"%S\"; filename=\"%S\"\r\n");
_LIT8(FORMDATA_TYPE, "multipart/form-data; boundary=");

_LIT8(HTTP_GET_MODIFIED_SINCE, "If-Modified-Since: %S\r\n");
_LIT8(HTTP_GET_RANGE, "Range: bytes=%u-%u\r\n");
_LIT(KDefaultServerName, "www.cs.helsinki.fi");

#define SIMPLE_UPLOAD 0
#include "util.h"

class CHttpImpl : public CHttp, public MContextBase, /*public MTimeOut,*/ public MEngineObserver {
public:
	~CHttpImpl();

public:
	void GetL(const TUint& iAP, const TDesC &url, const TTime &modified_since = TTime(0), 
		const TUint &chunkStart=0, const TUint &chunkEnd=0);
	
	void PostL(const TUint& iAP, const TDesC &url, CPtrList<CPostPart>* aBodyParts);
		// guarantees to take ownership of parts even if Leaves

	//void HeadL(const TDesC &url, const TTime &modified_since);
	void ReleaseParts();
	void Disconnect();

private:
	virtual void NotifyEngineStatus(TInt st, TInt aError);
	virtual void NotifyNewData(const TDesC8& aBuffer);
	virtual void NotifyCanWrite();

	void ConstructL(const TDesC& aConnectionName);
	CHttpImpl(MHttpObserver& aObserver, MApp_context& Context);
	void ConnectL();
	TBool ParseUrl(const TDesC &url);

private:
	MHttpObserver& iObserver;
	enum THttpError {EServerError, EParseError };
	void ReportError(TInt aErrorType, TInt errorCode);

private:
	TBool Parse(const TDesC8 &aBuffer);
	TBool ParseLine(const TDesC8 &aLine);
	void ResetParser();

private:
	TTime InternetTimeIntoTime(const TDesC8 &aInternetTimeString);
	void TimeIntoInternetTime(TDes8 &buf, const TTime &time);

	void MakeBoundary();
	void DeleteParts();
	void GetProxyL();
private:
	static const TInt KDefaultPortNumber;

    CSocketsEngine * iEngine;

	enum THttpState { ENotConnected, EConnecting, EConnected, EDisconnectForNew, EDisconnecting };
	THttpState iHttpStatus;
	void ChangeStatus(THttpState aNewStatus, TInt aError);

	enum TParsingState { EUndefined, EHeader, EBody, EError };
	TParsingState iParsingState;
	
	TInt                          iPort;
	TBuf16<KMaxServerNameLength>  iServerName;
	TUint32			      iAccessPoint;

	enum TMethod {
		EGet,
		EPost
	};
	TMethod			iMethod;
	TBuf8<256>		      iUrl;
        HBufC8	* iCommand;

	enum TPartState {
		ESendingHeader,
		ESendingBody,
	};
	TPartState		iPartState;

	CPtrList<CPostPart>*	iParts;
	CPtrList<CPostPart>::Node* iCurrentPartNode;

	HBufC8		*iParseBuffer;
	TBuf8<128>	iBoundary;

	CHttpHeader * iHeader;
	RFile		iDebugFile; TBool iDebugFileIsOpen;
	TInt		iProxyPort;
	TBuf<256>	iProxy;
	TBool		iNoMoreToSend;
	
	friend class CHttp;
};

const TInt CHttpImpl::KDefaultPortNumber = 80;

EXPORT_C CHttp * CHttp::NewL(MHttpObserver& aObserver, MApp_context &Context,
const TDesC& aConnectionName)
{
	CALLSTACKITEM_N(_CL("CHttp"), _CL("NewL"));
	
	CHttpImpl* self = new (ELeave) CHttpImpl(aObserver, Context);
	CleanupStack::PushL(self);
	self->ConstructL(aConnectionName);
	CleanupStack::Pop(self);
	return self;
}

CHttpImpl::CHttpImpl(MHttpObserver& aObserver, MApp_context& Context): 
		MContextBase(Context), iObserver(aObserver), iPort(KDefaultPortNumber), 
			iServerName(KDefaultServerName) { }

void CHttpImpl::ConstructL(const TDesC& aConnectionName)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ConstructL"));
	iHttpStatus = ENotConnected;
	iEngine = CSocketsEngine::NewL(*this, *GetContext(), EFalse, aConnectionName);
	iHeader = CHttpHeader::NewL();
	
	iParsingState = EUndefined;
}

CHttpImpl::~CHttpImpl()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("~CHttpImpl"));
	if (iDebugFileIsOpen) iDebugFile.Close();

	DeleteParts();
	delete iParseBuffer;
	delete iHeader;
	delete iEngine;
	delete iCommand;
}

void CHttpImpl::MakeBoundary()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("MakeBoundary"));
	TTime t; t=GetTime();
	TInt64 i=t.Int64();
	iBoundary=_L8("------");
	iBoundary.AppendNum(I64LOW(i));
	iBoundary.AppendNum(I64HIGH(i));
	iBoundary.Append(_L8("---------\r\n"));
}

void CHttpImpl::ChangeStatus(THttpState aNewStatus, TInt aError)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ChangeStatus"));
	
	iHttpStatus = aNewStatus;
	switch (aNewStatus)
        {
        case ENotConnected:

		Log(_L("Http::Not connected"));
		iObserver.NotifyHttpStatus(MHttpObserver::EHttpDisconnected, aError);
		break;
		
        case EConnecting:
		Log(_L("Http::Connecting"));
		break;

	case EConnected:
		Log(_L("Http::Not connected"));
		iObserver.NotifyHttpStatus(MHttpObserver::EHttpConnected, KErrNone);
		break;
	case EDisconnectForNew:
		Log(_L("Http::Disconnecting For New Request"));
		break;
        default:
		Log(_L("Http::Unknown status"));
		break;
        }
}

#include <commdb.h>

void CHttpImpl::GetProxyL()
{
	iProxy.Zero(); iProxyPort=0;
	
	if (Settings().GetSettingL(SETTING_PROXY, iProxy) && 
			Settings().GetSettingL(SETTING_PROXY_PORT, iProxyPort) &&
			iProxy.Length()>0 && iProxyPort>0) {
		return;
	}
	
	TPtrC iap(IAP);
	TInt pushed=0;
	
	CCommsDatabase* db=CCommsDatabase::NewL(EDatabaseTypeIAP); 
	CleanupStack::PushL(db); ++pushed;
	TInt ap=iAccessPoint;
	CCommsDbTableView* tableView=db->OpenViewMatchingUintLC(iap, TPtrC(COMMDB_ID), 
		ap); ++pushed;
	TUint32 iapService;
	HBufC* iapServiceType;
	
	if (tableView->GotoFirstRecord() != KErrNone) User::Leave(KErrNotFound);
	
	tableView->ReadUintL(TPtrC(IAP_SERVICE), iapService);
	iapServiceType = tableView->ReadLongTextLC(TPtrC(IAP_SERVICE_TYPE)); ++pushed;
	
	// Check whether this IAP uses proxy or not.
	TBool isProxyEnabled = EFalse;
	CCommsDbTableView* proxyTableView = db->OpenViewOnProxyRecordLC(
        iapService, *iapServiceType); ++pushed;
	if (KErrNone == proxyTableView->GotoFirstRecord()) {
	    proxyTableView->ReadBoolL(TPtrC(PROXY_USE_PROXY_SERVER), isProxyEnabled);
 
    	// If proxy is enabled then do something.
	    if (isProxyEnabled) {
	    	HBufC* proxyServerName=0;
        	proxyServerName = proxyTableView->ReadLongTextLC(TPtrC(PROXY_SERVER_NAME));
        	TUint32 port;
        	proxyTableView->ReadUintL(TPtrC(PROXY_PORT_NUMBER), port);
        	iProxyPort=port;
	
			iProxy=*proxyServerName;        	
        	CleanupStack::PopAndDestroy(proxyServerName);
        }
    }
    
    CleanupStack::PopAndDestroy(pushed);
}

void CHttpImpl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ConnectL"));

	if (iHttpStatus == EConnected) {
		ChangeStatus(EDisconnectForNew, KErrNone);
		iEngine->Disconnect(EFalse);
	} else if (iHttpStatus == ENotConnected) {
		GetProxyL();
		ChangeStatus(EConnecting, KErrNone);
		if (iProxy.Length()>0 && iProxyPort>0) {
			iEngine->ConnectL(iProxy, iProxyPort, iAccessPoint);
		} else {
			iEngine->ConnectL(iServerName, iPort, iAccessPoint);
		}
		iEngine->NoReadTimeout();
	}
}

void CHttpImpl::NotifyEngineStatus(TInt st, TInt aError)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("NotifyEngineStatus"));
#ifdef __WINS__
	TBuf<100> msg=_L("CHttpImpl::NotifyEngineStatus: ");
	msg.AppendNum(st); msg.Append(_L(" http status before: "));
	msg.AppendNum(iHttpStatus); msg.Append(_L(" err: "));
	msg.AppendNum(aError);
	RDebug::Print(msg);
#endif
	if (st == MEngineObserver::ESocketConnected) {
		ChangeStatus(EConnected, KErrNone);
		ResetParser();
		if (iCommand) Log(*iCommand);
		if (iCommand) iEngine->WriteL(*iCommand);
		if (iDebugFileIsOpen) iDebugFile.Write(*iCommand);
		
		if (iMethod==EGet) iEngine->ReadTimeout();

		iEngine->Read();
		if (iMethod==EGet) iEngine->ReadTimeout();
		else iEngine->NoReadTimeout();
		
	} else if (st == MEngineObserver::ESocketDisconnected) {
		if (iHttpStatus==EDisconnectForNew) {
			iHttpStatus = EConnecting;
			ConnectL();
		} else if (iHttpStatus!=ENotConnected) {
			ChangeStatus(ENotConnected, aError);
		}
	} else {
		ChangeStatus(ENotConnected, aError);
	}
}


void CHttpImpl::ReportError(TInt aErrorType,  TInt errorCode)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ReportError"));
	switch (aErrorType)
	{
	case EServerError:
		Log(_L("Http: Server error"), errorCode);
		break;

	case EParseError:
		Log(_L("CJabber: StreamError"));
		break;
	
	default:
		break;
	}
}


void CHttpImpl::GetL(const TUint& iAP, const TDesC &url, const TTime &modified_since, const TUint &chunkStart, const TUint &chunkEnd)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("GetL"));

	iMethod=EGet;
	iAccessPoint = iAP;
	ParseUrl(url);
	iNoMoreToSend=EFalse;

	delete iCommand;
	iCommand=0;

	iCommand = HBufC8::NewL(url.Length() + HTTP_GET_MAIN().Length()+2);
	iCommand->Des().AppendFormat(HTTP_GET_MAIN(), &iUrl);
	if (modified_since != TTime(0)) {
		TBuf8<30> internet_time;
		TimeIntoInternetTime(internet_time, modified_since);
		iCommand=iCommand->ReAllocL(iCommand->Length()+HTTP_GET_MODIFIED_SINCE().Length() + internet_time.Length() );
		iCommand->Des().AppendFormat(HTTP_GET_MODIFIED_SINCE(), &internet_time);
	}
	if ( chunkEnd!=0 ) {
		iCommand=iCommand->ReAllocL(iCommand->Length()+HTTP_GET_RANGE().Length() + 10 );
		iCommand->Des().AppendFormat(HTTP_GET_RANGE(), chunkStart, chunkEnd);
	}
	iCommand->Des().Append(_L("\r\n"));
	Log(*iCommand);

	CC_TRAPD(err, ConnectL());
}


void CHttpImpl::NotifyNewData(const TDesC8 &aBuffer)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("NotifyNewData"));
#ifdef __WINS__
	Log(_L("Received: "));
	Log(aBuffer);
#endif
	if (!Parse(aBuffer)) {
		iObserver.NotifyHttpStatus(MHttpObserver::EHttpError, KErrGeneral);
		Log(_L("Parse Error"));
	}
}


void CHttpImpl::NotifyCanWrite()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("NotifyCanWrite"));
begin:
	if (iMethod==EGet) return;

	if (iNoMoreToSend) {
		iEngine->ReadTimeout();
		iObserver.NotifyHttpStatus(MHttpObserver::EHttpSentAll, 0);
		return;
	}

	iCommand->Des().Zero();

	if (iPartState==ESendingHeader) {
		if (iCurrentPartNode) {
			TBuf8<256> buf8, buf8_2;
			Log(_L("send header"));
			CPostPart* p=iCurrentPartNode->Item;
			if (iCurrentPartNode != iParts->iFirst) 
				iCommand->Des()=_L8("\r\n");

			iCommand->Des().Append(iBoundary);
			if (p->FileName().Length() > 0) {
				CC()->ConvertFromUnicode(buf8, p->iName);
				CC()->ConvertFromUnicode(buf8_2, p->FileName());
				iCommand->Des().AppendFormat(HTTP_POST_CONTENTDISPOSITIONFILE, 
					&buf8, &buf8_2);
			} else {
				CC()->ConvertFromUnicode(buf8, p->iName);
				iCommand->Des().AppendFormat(HTTP_POST_CONTENTDISPOSITION, 
					&buf8);
			}
			buf8.Zero(); CC()->ConvertFromUnicode(buf8, p->iMimeType);
			iCommand->Des().AppendFormat(HTTP_POST_CONTENTTYPE, &buf8);
			iCommand->Des().Append(_L8("\r\n"));
		} else {
			if (!SIMPLE_UPLOAD || iParts->iCount > 1) {
				Log(_L("send last boundary"));
				iCommand->Des()=_L8("\r\n");
				iCommand->Des().Append(iBoundary);
			} else {
				Log(_L("sent all"));
				iNoMoreToSend=ETrue;
				return;
			}
		}
	} else {
		Log(_L("send body"));
		if (iCurrentPartNode) {
			CPostPart* p=iCurrentPartNode->Item;
			TPtr8 cp=iCommand->Des();
			p->ReadChunkL(cp);
			if (iCommand->Des().Length()==0) {
				//EOF on part
				iCurrentPartNode=iCurrentPartNode->Next;
				iPartState=ESendingHeader;
				goto begin;
			}
		} else {
			iNoMoreToSend=ETrue;
			return;
		}
	}
	iPartState=ESendingBody;
	
	if (iDebugFileIsOpen) iDebugFile.Write(*iCommand);
	iEngine->WriteL(*iCommand);
}

TBool CHttpImpl::Parse(const TDesC8 &aBuffer)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("Parse"));
	TInt eol=KErrNotFound;
	TInt begin_substring = 0;
	TBool ok = ETrue;
	TBool cont = ETrue;
	_LIT8(KCRLF, "\r\n");

	if ( (iParsingState == EUndefined) || (iParsingState==EHeader) ) {
		while ( cont ) {
			eol=aBuffer.Mid(begin_substring, aBuffer.Length()- begin_substring).Find(KCRLF);
			if (eol != KErrNotFound) {
				/*if (eol == 24) { 
					RDebug::Print(_L("EndOfLine")); 
				}*/
				RDebug::Print(_L("found eol at %d"), eol);
				if (iParseBuffer) {
					iParseBuffer=iParseBuffer->ReAllocL(iParseBuffer->Des().Length() + eol);
					iParseBuffer->Des().Append(aBuffer.Mid(begin_substring, eol));
					ok = ParseLine(*iParseBuffer);
					delete iParseBuffer; iParseBuffer=0;
				} else {
					ok = ParseLine(aBuffer.Mid(begin_substring, eol));
				}
				cont = (iParsingState==EHeader);
				begin_substring += eol + KCRLF().Length();
			} else {
				cont = EFalse;
				if (iParseBuffer) {
					TInt len=iParseBuffer->Des().Length();
					// case when \r\n split over the parsebuffer and
					// the incoming buffer
					if (iParseBuffer->Des()[len-1]=='\r') {
						if (aBuffer[0]=='\n') {
							ok=ParseLine(iParseBuffer->Des().Mid(0, len-1));
							delete iParseBuffer; iParseBuffer=0;
							begin_substring=1;
							if (iParsingState == EHeader) cont=ETrue;
						}
					}
				}
			}
		}
		if (!ok) return ok;

      		if ( begin_substring < aBuffer.Length() ) {
			if (iParseBuffer) {
				iParseBuffer=iParseBuffer->ReAllocL(iParseBuffer->Des().Length() + aBuffer.Length() - begin_substring );
				iParseBuffer->Des().Append( aBuffer.Mid(begin_substring, aBuffer.Length() - begin_substring) );
			} else {
				iParseBuffer = HBufC8::NewL(aBuffer.Length() - begin_substring);
				iParseBuffer->Des().Append(aBuffer.Mid(begin_substring, aBuffer.Length() - begin_substring));
			}
			if (iParsingState==EBody) {
				iObserver.NotifyNewBody(*iParseBuffer);
				delete iParseBuffer; iParseBuffer=0;
			}
		}
	} else if (iParsingState == EBody) {
		iObserver.NotifyNewBody(aBuffer);
		delete iParseBuffer; iParseBuffer=0;
	} else {
		//error!
		ReportError(EParseError, 0);
	}
	return ok;
}

TBool CHttpImpl::ParseLine(const TDesC8 &aLine)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ParseLine"));
	_LIT8(HTTP_REPLY, "HTTP/*");
	_LIT8(HTTP_DATE, "Date:*");
	_LIT8(HTTP_SERVER, "Server:*");
	_LIT8(HTTP_LASTMOD, "Last-Modified:*");
	_LIT8(HTTP_ETAG, "ETag:*");
	_LIT8(HTTP_ACCEPT_RANGE, "Accept-Ranges:*");
	_LIT8(HTTP_SIZE, "Content-Length:*");
	_LIT8(HTTP_MIME, "Content-Type:*");
	_LIT8(HTTP_CONNECTION, "Connection:*");
	_LIT8(HTTP_RANGE, "Content-Range: bytes*");
	_LIT8(HTTP_LOCATION, "Location:*");
	
	TLex8 lex;
	TInt err;

	Log(aLine);
	if (aLine.Length() == 0) {
		// end of header - about to receive a body
		iParsingState = EBody;
		iObserver.NotifyNewHeader(*iHeader);
		return ETrue;
	} else if (aLine.Match(HTTP_REPLY) != KErrNotFound) {
		iParsingState = EHeader;
		iHeader->iFilename.Copy(iUrl);
		iHeader->iServername.Copy(iServerName);
		lex=aLine.Mid(5, 3);
		err = lex.Val(iHeader->iHttpVersion);
		lex=aLine.Mid(9, 3);
		err = lex.Val(iHeader->iHttpReplyCode);
		return (err==KErrNone);
	} else if (aLine.Match(HTTP_DATE) != KErrNotFound) {
		iHeader->iServerTime = InternetTimeIntoTime(aLine.Mid(6, aLine.Length()-6));
		return ETrue;
	} else if (aLine.Match(HTTP_LOCATION) != KErrNotFound) {
		TInt state=CCnvCharacterSetConverter::KStateDefault;
		CC()->ConvertToUnicode(iHeader->iLocation, aLine.Mid( HTTP_LOCATION().Length() ), state);
		return ETrue;
	} else if (aLine.Match(HTTP_SERVER) != KErrNotFound) {
		// not supported yet
		return ETrue;
	} else if (aLine.Match(HTTP_LASTMOD) != KErrNotFound) {
		iHeader->iLastMod = InternetTimeIntoTime(aLine.Mid(15, aLine.Length()-15));
		return ETrue;
	} else if (aLine.Match(HTTP_ETAG) != KErrNotFound) {
		//not handled yet
		return ETrue;
	} else if (aLine.Match(HTTP_ACCEPT_RANGE) != KErrNotFound) {
		return ETrue;
	} else if (aLine.Match(HTTP_RANGE) != KErrNotFound ) {
			// we're receiving a chunk
			TInt dash = aLine.Mid(HTTP_RANGE().Length(), aLine.Length() - HTTP_RANGE().Length()).Find(_L8("-"));
			TInt slash = aLine.Mid(HTTP_RANGE().Length(), aLine.Length() - HTTP_RANGE().Length()).Find(_L8("/"));

			if ((dash!=KErrNotFound) && (slash!=KErrNotFound)) {
				lex=aLine.Mid(HTTP_RANGE().Length(), dash);
				err=lex.Val(iHeader->iChunkStart);

				lex=aLine.Mid(HTTP_RANGE().Length()+dash+1, slash-dash-1);
			        err=lex.Val(iHeader->iChunkEnd);

				lex=aLine.Mid(HTTP_RANGE().Length()+slash+1, aLine.Length() - (slash+1+HTTP_RANGE().Length()));
				err=lex.Val(iHeader->iSize);
				return (err==KErrNone);
			} else 	return EFalse;
	} else if (aLine.Match(HTTP_SIZE) != KErrNotFound) {
		lex=aLine.Mid(16, aLine.Length()-16);
		err=lex.Val(iHeader->iSize);
		iHeader->iChunkStart=0;
		iHeader->iChunkEnd=iHeader->iSize;
		return (err==KErrNone);
	} else if (aLine.Match(HTTP_MIME) != KErrNotFound) {
		TInt state=CCnvCharacterSetConverter::KStateDefault;
		CC()->ConvertToUnicode(iHeader->iContentType, aLine.Mid(HTTP_MIME().Length()), state);
		return ETrue;
	} else if (aLine.Match(HTTP_CONNECTION) != KErrNotFound) {
		return ETrue;
	} else {
		return EFalse;
	}
}

void CHttpImpl::ResetParser()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ResetParser"));
	delete iParseBuffer; iParseBuffer=0;
	iParsingState = EUndefined;
	iHeader->Reset();
}

TBool CHttpImpl::ParseUrl(const TDesC &url)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ParseUrl"));
	_LIT(HTTP, "http://");
	_LIT(HTTPS, "https://");

	// port not handled yet
	CC()->ConvertFromUnicode(iUrl, url);
		
	if (url.Left(HTTP().Length()).Compare(HTTP()) == 0) {
		TInt length = url.Length()-HTTP().Length();
		TInt slash = url.Mid(HTTP().Length(), length).Find(_L("/"));
		if (slash==KErrNotFound) slash=length;
		TInt colon=url.Mid(HTTP().Length(), length).Find(_L(":"));
		if (colon==KErrNotFound || colon>slash) {
			iPort=80;
			iServerName=url.Mid(HTTP().Length(), slash);
		} else {
			TLex l(url.Mid(HTTP().Length(), length).Mid(colon+1,slash-colon));
			User::LeaveIfError(l.Val(iPort));
			iServerName=url.Mid(HTTP().Length(), colon);
		}
		return ETrue;
	} else if (url.Left(HTTPS().Length()).Compare(HTTPS()) == 0) {
		TInt length = url.Length()-HTTPS().Length();
		TInt slash = url.Mid(HTTPS().Length(), length).Find(_L("/"));
		if (slash==KErrNotFound) slash=length;

		iServerName=url.Mid(HTTPS().Length(), slash);
		iPort=443;
		return ETrue;
	} else {
		return EFalse;
	}
}

EXPORT_C CHttpHeader * CHttpHeader::NewL()
{
	CALLSTACKITEM_N(_CL("CHttpHeader"), _CL("NewL"));
	CHttpHeader* self = CHttpHeader::NewLC();
	CleanupStack::Pop(self);
	return self;
}

EXPORT_C CHttpHeader * CHttpHeader::NewLC()
{
	CALLSTACKITEM_N(_CL("CHttpHeader"), _CL("NewLC"));
	CHttpHeader* self = new (ELeave) CHttpHeader();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

void CHttpHeader::ConstructL()
{
	CALLSTACKITEM_N(_CL("CHttpHeader"), _CL("ConstructL"));
	Reset();
}

void CHttpHeader::Reset()
{
	CALLSTACKITEM_N(_CL("CHttpHeader"), _CL("Reset"));
	iFilename.Zero();
	iHttpReplyCode = -1;
	iHttpVersion=-1;
	iLastMod=TTime(0);
	iServername.Zero();
	iSize=-1;
	iChunkStart = -1;
	iChunkEnd = -1;
	iServerTime=TTime(0);
}

void CHttpHeader::Copy(const CHttpHeader &aHeader)
{
	CALLSTACKITEM_N(_CL("CHttpHeader"), _CL("Copy"));
	this->iFilename.Copy(aHeader.iFilename);
	this->iHttpReplyCode = aHeader.iHttpReplyCode;
	this->iHttpVersion = aHeader.iHttpVersion;
	this->iLastMod = aHeader.iLastMod;
	this->iServername.Copy(aHeader.iServername);
	this->iServerTime = aHeader.iServerTime;
	this->iSize = aHeader.iSize;
	this->iChunkStart = aHeader.iChunkStart;
	this->iChunkEnd = aHeader.iChunkEnd;
}

TTime CHttpImpl::InternetTimeIntoTime(const TDesC8 &aInternetTimeString)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("InternetTimeIntoTime"));
	// GMT offset not supported! 
	TTime t;
	TBuf<20> date_string;
	
	TBuf<2> day;
	TBuf<3> month;
	TBuf<4> year;
	TBuf<2> hour;
	TBuf<2> min;
	TBuf<2> sec;

	day.Copy(aInternetTimeString.Mid(5,2));
	month.Copy(aInternetTimeString.Mid(8,3));
	year.Copy(aInternetTimeString.Mid(12,4));
	
	hour.Copy(aInternetTimeString.Mid(17,2));
	min.Copy(aInternetTimeString.Mid(20,2));
	sec.Copy(aInternetTimeString.Mid(23,2));

	date_string.Append(day);
	date_string.Append(_L("-"));
	if (month.Compare(_L("Jan")) == 0) {
		date_string.Append(_L("01"));
	} else if (month.Compare(_L("Feb")) == 0) {
		date_string.Append(_L("02"));
	} else if (month.Compare(_L("Apr")) == 0) {
		date_string.Append(_L("03"));
	} else if (month.Compare(_L("May")) == 0) {
		date_string.Append(_L("05"));
	} else if (month.Compare(_L("Jun")) == 0) {
		date_string.Append(_L("06"));
	} else if (month.Compare(_L("Jul")) == 0) {
		date_string.Append(_L("07"));
	} else if (month.Compare(_L("Aug")) == 0) {
		date_string.Append(_L("08"));
	} else if (month.Compare(_L("Sep")) == 0) {
		date_string.Append(_L("09"));
	} else if (month.Compare(_L("Oct")) == 0) {
		date_string.Append(_L("10"));
	} else if (month.Compare(_L("Nov")) == 0) {
		date_string.Append(_L("11"));
	} else if (month.Compare(_L("Dec")) == 0) {
		date_string.Append(_L("12"));
	}

        date_string.Append(_L("-"));
	date_string.Append(year);
	date_string.Append(_L(" "));
	date_string.Append(hour);
	date_string.Append(_L(":"));
	date_string.Append(min);
	date_string.Append(_L(":"));
	date_string.Append(sec);

	t.Parse(date_string);
	return t;
}

void CHttpImpl::TimeIntoInternetTime(TDes8 &ret, const TTime &time)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("TimeIntoInternetTime"));
	switch(time.DayNoInWeek()) {
		case EMonday:
			ret.Append(_L8("Mon"));
			break;
		case ETuesday:
			ret.Append(_L8("Tue"));
			break;
		case EWednesday:
			ret.Append(_L8("Wed"));
			break;
		case EThursday:
			ret.Append(_L8("Thu"));
			break;
		case EFriday:
			ret.Append(_L8("Fri"));
			break;
		case ESaturday:
			ret.Append(_L8("Sat"));
			break;
		case ESunday:
			ret.Append(_L8("Sun"));
			break;
	}
	ret.Append(_L8(", "));
	ret.AppendFormat(_L8("%02d"), time.DayNoInMonth()+1);
	ret.Append(_L8(" "));
	switch(time.DateTime().Month()) {
		case EJanuary:
			ret.Append(_L8("Jan"));
			break;
		case EFebruary:
			ret.Append(_L8("Feb"));
			break;
		case EMarch:
			ret.Append(_L8("Mar"));
			break;
		case EApril:
			ret.Append(_L8("Apr"));
			break;
		case EMay:
			ret.Append(_L8("May"));
			break;
		case EJune:
			ret.Append(_L8("Jun"));
			break;
		case EJuly:
			ret.Append(_L8("Jul"));
			break;
		case EAugust:
			ret.Append(_L8("Aug"));
			break;
		case ESeptember:
			ret.Append(_L8("Sep"));
			break;
		case EOctober:
			ret.Append(_L8("Oct"));
			break;
		case ENovember:
			ret.Append(_L8("Nov"));
			break;
		case EDecember:
			ret.Append(_L8("Dec"));
			break;
	}
	ret.Append(_L8(" "));
	ret.AppendFormat(_L8("%04d"), time.DateTime().Year());
	ret.AppendFormat(_L8(" %02d:%02d:%02d GMT"), time.DateTime().Hour(), time.DateTime().Minute(), time.DateTime().Second());
	Log(ret);
}


CPostPart::CPostPart(const TDesC& aName, const TDesC& aMimeType)
{
	CALLSTACKITEM_N(_CL("CPostPart"), _CL("CPostPart"));
	iName=aName;
	iMimeType=aMimeType;
}

CFilePart* CFilePart::NewL(RFs& aFs, const TDesC& aFileName, 
	const TDesC& aName, const TDesC& aMimeType)
{
	CALLSTACKITEM_N(_CL("CFilePart"), _CL("NewL"));
	auto_ptr<CFilePart> ret(new (ELeave) CFilePart(aName, aMimeType));
	ret->ConstructL(aFs, aFileName);

	return ret.release();
}

CFilePart::CFilePart(const TDesC& aName, const TDesC& aMimeType) : CPostPart(aName, aMimeType) { }

void CFilePart::ConstructL(RFs& aFs, const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CFilePart"), _CL("ConstructL"));

	iFileName=aFileName;
	User::LeaveIfError(iFile.Open(aFs, aFileName, EFileShareAny | EFileRead));
	iFileIsOpen=ETrue;
	User::LeaveIfError(iFile.Size(iSize));
}

TInt	CFilePart::Size()
{
	CALLSTACKITEM_N(_CL("CFilePart"), _CL("Size"));
	return iSize;
}

void CFilePart::ReadChunkL(TDes8& aIntoBuffer)
{
	CALLSTACKITEM_N(_CL("CFilePart"), _CL("ReadChunkL"));
	aIntoBuffer.Zero();
	TInt toread;
	if (iSize-iRead > aIntoBuffer.MaxLength()) toread=aIntoBuffer.MaxLength();
	else toread=iSize-iRead;
	User::LeaveIfError(iFile.Read(aIntoBuffer, toread));
	iRead+=aIntoBuffer.Length();
}

CFilePart::~CFilePart()
{
	CALLSTACKITEM_N(_CL("CFilePart"), _CL("~CFilePart"));
	if (iFileIsOpen) iFile.Close();
}

CBufferPart* CBufferPart::NewL(HBufC8* aBuffer, TBool aTakeOwnership,
	const TDesC& aName, const TDesC& aMimeType)
{
	CALLSTACKITEM_N(_CL("CBufferPart"), _CL("NewL"));
	auto_ptr<CBufferPart> ret(new (ELeave) CBufferPart(aName, aMimeType));
	ret->ConstructL(aBuffer, aTakeOwnership);

	return ret.release();
}

CBufferPart* CBufferPart::NewL(const TDesC8& aBuffer,
	const TDesC& aName, const TDesC& aMimeType)
{
	CALLSTACKITEM_N(_CL("CBufferPart"), _CL("NewL"));
	auto_ptr<CBufferPart> ret(new (ELeave) CBufferPart(aName, aMimeType));
	ret->ConstructL(aBuffer);

	return ret.release();
}

CBufferPart::CBufferPart(const TDesC& aName, const TDesC& aMimeType) : CPostPart(aName, aMimeType) { }

CBufferPart::~CBufferPart()
{
	CALLSTACKITEM_N(_CL("CBufferPart"), _CL("~CBufferPart"));
	if (iOwnsBuffer) delete iBuffer;
}

void CBufferPart::ConstructL(const TDesC8& aBuffer)
{
	CALLSTACKITEM_N(_CL("CBufferPart"), _CL("ConstructL"));
	iBuffer=aBuffer.AllocL();
	iOwnsBuffer=ETrue;
}

void CBufferPart::ConstructL(HBufC8* aBuffer, TBool aTakeOwnership)
{
	CALLSTACKITEM_N(_CL("CBufferPart"), _CL("ConstructL"));
	iOwnsBuffer=aTakeOwnership;
	iBuffer=aBuffer;
}

TInt CBufferPart::Size()
{
	CALLSTACKITEM_N(_CL("CBufferPart"), _CL("Size"));
	return iBuffer->Des().Length();
}

void CBufferPart::ReadChunkL(TDes8& aIntoBuffer)
{
	CALLSTACKITEM_N(_CL("CBufferPart"), _CL("ReadChunkL"));
	TInt toread;
	toread=Size()-iPos;
	if (toread > aIntoBuffer.MaxLength()) toread=aIntoBuffer.MaxLength();
	aIntoBuffer=iBuffer->Des().Mid(iPos, toread);
	iPos+=toread;
}

void CHttpImpl::DeleteParts()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("DeleteParts"));
	delete iParts; iParts=0;
}

void CHttpImpl::ReleaseParts()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("ReleaseParts"));
	DeleteParts();
}


void CHttpImpl::PostL(const TUint& iAP, const TDesC &url, CPtrList<CPostPart>* aBodyParts)
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("PostL"));
	DeleteParts();
	iParts=aBodyParts;
	iNoMoreToSend=EFalse;

#ifdef __WINS__
	if (iDebugFileIsOpen) {
		iDebugFile.Close();
		iDebugFileIsOpen=EFalse;
	}
	if (iDebugFile.Replace(Fs(), _L("c:\\post.txt"), EFileWrite)==KErrNone) {
		iDebugFileIsOpen=ETrue;
	}
#endif

	if (!aBodyParts || aBodyParts->iCount==0) 
		User::Leave(KErrArgument);
	if (! ParseUrl(url)) 
		User::Leave(KErrArgument);

	MakeBoundary();
	iMethod=EPost;
	iAccessPoint = iAP;

	TInt len=url.Length() + HTTP_POST_MAIN().Length()+2;
	len+=HTTP_POST_HOST().Length()+iServerName.Length();
	len+=HTTP_POST_CONTENTTYPE().Length()+128;
	len+=HTTP_POST_CONTENTLENGTH().Length()+20;
	len+=HTTP_POST_CONTENTDISPOSITIONFILE().Length()+256+128;
	len+=iBoundary.Length();

	if (!iCommand || iCommand->Des().MaxLength() < len) {
		delete iCommand; iCommand=0;
		iCommand=HBufC8::NewL(len);
	} else {
		iCommand->Des().Zero();
	}

	iCommand->Des().AppendFormat(HTTP_POST_MAIN(), &iUrl);

	TBuf8<256> buf8, buf8_2;
	CC()->ConvertFromUnicode(buf8, iServerName);
	iCommand->Des().AppendFormat(HTTP_POST_HOST(), &buf8);

	if (SIMPLE_UPLOAD && aBodyParts->iCount == 1) {
		CPostPart* p=aBodyParts->Top();
		buf8.Zero(); CC()->ConvertFromUnicode(buf8, p->iMimeType);
		iCommand->Des().AppendFormat(HTTP_POST_CONTENTTYPE, &buf8);
		iCommand->Des().AppendFormat(HTTP_POST_CONTENTLENGTH, p->Size());
		
		if (p->FileName().Length() > 0) {
			buf8.Zero(); CC()->ConvertFromUnicode(buf8, p->iName);
			buf8_2.Zero(); CC()->ConvertFromUnicode(buf8_2, p->FileName());
			iCommand->Des().AppendFormat(HTTP_POST_CONTENTDISPOSITIONFILE, &buf8, &buf8_2);
		}
	} else {
		TBuf8<158> type=FORMDATA_TYPE();
		type.Append(iBoundary.Mid(2, iBoundary.Length()-4));
		iCommand->Des().AppendFormat(HTTP_POST_CONTENTTYPE, &type);
		TInt len=0;
		for (CPtrList<CPostPart>::Node* n=aBodyParts->iFirst; n; n=n->Next) {
			len+=iBoundary.Length();
			TInt filen_len=n->Item->FileName().Length();
			TInt name_len=n->Item->iName.Length();
			if ( filen_len > 0 ) {
				len+=HTTP_POST_CONTENTDISPOSITIONFILE().Length()-4;
			} else {
				len+=HTTP_POST_CONTENTDISPOSITION().Length()-2;
			}
			len+=filen_len; len+=name_len;
			len+=HTTP_POST_CONTENTTYPE().Length()-2;
			len+=n->Item->iMimeType.Length();
			len+=2; //linebreak after header and before content
			len+=2; //linebreak after content and before boundary
			len+=n->Item->Size();
		}
		len+=iBoundary.Length();
		iCommand->Des().AppendFormat(HTTP_POST_CONTENTLENGTH, len);
	}

	iCommand->Des().Append(_L8("\r\n"));
	Log(*iCommand);

	if (!SIMPLE_UPLOAD || iParts->iCount > 1) iPartState=ESendingHeader;
	else iPartState=ESendingBody;

	iCurrentPartNode=iParts->iFirst;

	ConnectL();
}

const TDesC& CPostPart::FileName()
{
	CALLSTACKITEM_N(_CL("CPostPart"), _CL("FileName"));
	return iFileName;
}


void CPostPart::SetFileName(const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CPostPart"), _CL("SetFileName"));
	iFileName=aFileName;
}

EXPORT_C void CHttp::AppendUrlEncoded(TDes& into, const TDesC& str)
{
	CALLSTACKITEM_N(_CL("CHttp"), _CL("AppendUrlEncoded"));
    ::AppendUrlEncoded(into, str);
}

void CHttpImpl::Disconnect()
{
	CALLSTACKITEM_N(_CL("CHttpImpl"), _CL("Disconnect"));
	iHttpStatus=ENotConnected;
	iEngine->Disconnect(ETrue);
}
