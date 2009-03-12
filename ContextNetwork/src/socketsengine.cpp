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
#include <eikgted.h>
#include "SocketsEngine.h"
#include "TimeOut.h"
#include "SocketsReader.h"
#include "SocketsWriter.h"
#include <flogger.h>
#include <string.h>
#include <es_sock.h>
#include "app_context.h"
#include "concretedata.h"
#include "sha1.h"
#include "sockets.pan"
#include "util.h"
#include <charconv.h>
#include "datacounter.h"

#define SETTING_LAST_CONNECTION_ERROR 93

const TInt CSocketsEngine::KMaxConnectionRetry = 5;

#ifndef __WINS__
const TInt CSocketsEngine::KTimeOut = 120; // 120 seconds time-out
const TInt CSocketsEngine::KTimeBeforeRetry = 120; //120 seconds before retry
#else
const TInt CSocketsEngine::KTimeOut = 120; // 120 seconds time-out
const TInt CSocketsEngine::KTimeBeforeRetry = 10; //120 seconds before retry
#endif

const TInt CSocketsEngine::KDefaultPortNumber = 80;
_LIT(KDefaultServerName, "localhost");

const TTupleName KReceivedDataCounterTuple = { { CONTEXT_UID_CONTEXTNETWORK }, 5 };
const TTupleName KSentDataCounterTuple = { { CONTEXT_UID_CONTEXTNETWORK }, 4 };


EXPORT_C CSocketsEngine* CSocketsEngine::NewL(MEngineObserver& aObserver, MApp_context& Context,
				      TBool aDoCompress, const TDesC& aConnectionName)
{
	CALLSTACKITEM2_N(_CL("CSocketsEngine"), _CL("NewLC"), &Context);

	CSocketsEngine* self = new (ELeave) CSocketsEngine(aObserver, 
		Context, aDoCompress);
	CleanupStack::PushL(self);
	self->ConstructL(aConnectionName);
	CleanupStack::Pop();
	return self;
}

CSocketsEngine::CSocketsEngine(MEngineObserver& aObserver, MApp_context& Context, TBool aDoCompress)
	: CCheckedActive(EPriorityLow, _L("SocketEngine")), MContextBase(Context),
	iObserver(aObserver),
	iPort(KDefaultPortNumber),
	iServerName(KDefaultServerName),
	iConnectionRetry(0),
	iTimeBeforeRetry(KTimeBeforeRetry),
	iDoCompress(aDoCompress)
{
	Log(_L("Socket engine created."));
}


EXPORT_C CSocketsEngine::~CSocketsEngine()
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("~CSocketsEngine"));

	Cancel();

#ifdef __S60V2__
	delete iConnectionErrorListener;
#endif

	delete iSocketsReader;

	delete iSocketsWriter;
	
	if (iConnectionOpener && iConnectionOpener->MadeConnection()) iConnectionOpener->CloseConnection();
	delete iConnectionOpener;

	if (iCounterTimer) {
		TRAPD(ignored, expired(iCounterTimer));
		delete iCounterTimer;
	}
	if (iSocketOpen) iSocket.Close();
	if (iResolverOpen) iResolver.Close();
	
	iSocketServ.Close();
	
	delete iTimer;

	if (in_z_open) inflateEnd(&in_z_stream);
	if (out_z_open) deflateEnd(&out_z_stream);

	delete in_z_buf;
	delete out_z_buf;
	delete iTransferBuffer;
	delete iCC;
}


void CSocketsEngine::ConstructL(const TDesC& aConnectionName)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("ConstructL"));

	User::LeaveIfError(iSocketServ.Connect());
	
	iConnectionName=aConnectionName.Left(iConnectionName.MaxLength());

#ifndef __S60V2__
	iConnectionOpener = CConnectionOpener::NewL(*this, iSocketServ);
#else
	iConnectionOpener = CConnectionOpener::NewL(*this, iSocketServ, iConnection);
#endif
	// Create socket read and write active objects
	iSocketsReader = CSocketsReader::NewL(*this, iSocket, AppContext());
	iSocketsWriter = CSocketsWriter::NewL(*this, iSocket, AppContext());
	iMtu=1500;
	//iOverhead=11*4; // TCP and IP headers
	iOverhead=56;

	if (aConnectionName.Length() > 0) {
		iCounterTimer=CTimeOut::NewL(*this);
		DataCounter().GetInitialCountL(KReceivedDataCounterTuple, iConnectionName,
			iRecvBytes);
		DataCounter().GetInitialCountL(KSentDataCounterTuple, iConnectionName,
			iSentBytes);
	}
	
	ChangeStatus(ENotConnected);
	iTimer = CTimeOut::NewL(*this);
	CActiveScheduler::Add(this); 

	iCC=CCnvCharacterSetConverter::NewL();
	iCC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());;

	iTransferBuffer=HBufC8::NewL(2048);
	if (iDoCompress) {
		in_z_buf=HBufC8::NewL(4096);
		out_z_buf=HBufC8::NewL(4096);	
	}
}

void CSocketsEngine::InitZStreamsL()
{
	if (in_z_open) {
		inflateEnd(&in_z_stream);
		in_z_open=EFalse;
	}
	if (out_z_open) {
		deflateEnd(&out_z_stream);
		out_z_open=EFalse;
	}

	TInt ret=Z_OK;
	in_z_stream.zalloc = Z_NULL;
	in_z_stream.zfree = Z_NULL;
	in_z_stream.opaque = Z_NULL;
	ret = inflateInit(&in_z_stream);
	if (ret != Z_OK) User::Leave(-11002);
	in_z_open=ETrue;

	out_z_stream.zalloc = Z_NULL;
	out_z_stream.zfree = Z_NULL;
	out_z_stream.opaque = Z_NULL;
	ret = deflateInit(&out_z_stream, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK) User::Leave(-11002);
	out_z_open=ETrue;
}

void CSocketsEngine::AppendToOutZBufL(const TDesC8& aData)
{
	if ( out_z_buf->Des().Length() + aData.Length() > out_z_buf->Des().MaxLength() ) {
		out_z_buf=out_z_buf->ReAllocL(out_z_buf->Des().MaxLength()*2);
	}
	out_z_buf->Des().Append(aData);
}

EXPORT_C void CSocketsEngine::ConnectL(const TDesC& host, const TInt& port, TUint32 iap)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("ConnectL"));

	iPort = port;
	iServerName.Copy(host);
	iAccessPoint = iap;

	OpenConnectionAndConnectL();
}

void CSocketsEngine::OpenConnectionAndConnectL()
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("OpenConnectionAndConnectL"));

#ifdef __S60V2__
	delete iConnectionErrorListener; iConnectionErrorListener=0;
#endif
	iConnectionOpener->MakeConnectionL(iAccessPoint);	
}

void CSocketsEngine::ConnectL()
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("ConnectL"));

	if ( (iEngineStatus == EConnected) ) {
		// if already connected, no need to bother again
		iObserver.NotifyEngineStatus(MEngineObserver::ESocketConnected, KErrNone);
	} else if ( (iEngineStatus == ENotConnected ) )
	{
		TInetAddr addr;
		if (addr.Input(iServerName) == KErrNone) {
			ConnectL(addr.Address());
		} else {
			if (iResolverOpen) {
				iResolver.Close();
				iResolverOpen=EFalse;
			}
#ifndef __S60V2__
			User::LeaveIfError(iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp));
#else
			User::LeaveIfError(iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp, iConnection));
#endif
			iResolverOpen = ETrue;
			iStatus=KRequestPending;
			SetActive();
			iResolver.GetByName(iServerName, iNameEntry, iStatus);
			
			ChangeStatus(ELookingUp);
			iTimer->Wait(KTimeOut);
		}
	} else { 
		// other states: do nothing, it is still being processed
		// EConnecting, ELookingUp, EDisconnecting, EWaitingForRetry
	}
}


void CSocketsEngine::ConnectL(TUint32 aAddr) // <a name="ConnectL32">
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("ConnectL"));

	iConnectionRetry++;
	Log(_L("Socket engine: Connection attempt :"), iConnectionRetry);
		
	// Initiate attempt to connect to a socket by IP address	
	if (iEngineStatus == ENotConnected) {
		if (iDoCompress) {
			InitZStreamsL();
		}

		if (iSocketOpen) {
			iSocket.Close();
			iSocketOpen=EFalse;
		}
#ifndef __S60V2__
		User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp));
#else
#  if !defined(__S60V3__)
		User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, iConnection));
#  else
		if (iConnectionOpener->HasSubConnection()) {
			User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, 
				iConnectionOpener->SubConnection()));
		} else {
			User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, iConnection));
		}
#  endif
#endif
		iSocketOpen = ETrue;
		iAddress.SetAddress(aAddr);
		iAddress.SetPort(iPort);	
		if (iPort==443) {
			//iSocket.
		}

		iStatus=KRequestPending;
		SetActive();
		iSocket.Connect(iAddress, iStatus);
		UpdateByteCount(0, 1);
		ChangeStatus(EConnecting);

		iTimer->Wait(KTimeOut);
        }
}

#ifdef __S60V2__
void CSocketsEngine::ConnectionDisconnected(TInt aError)
{
	Log(_L("Connection disappeared: "), aError);
	Disconnect(ETrue);
}
#endif

EXPORT_C void CSocketsEngine::Disconnect(TBool closeConnection) 
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("Disconnect"));

#ifdef __S60V2__
	delete iConnectionErrorListener; iConnectionErrorListener=0;
#endif
	DisconnectSocket();
	if (closeConnection) {
		iConnectionOpener->CloseConnection();
	}
	iObserver.NotifyEngineStatus(MEngineObserver::ESocketDisconnected, KErrNone);
}

void CSocketsEngine::DisconnectSocket()
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("DisconnectSocket"));

	Cancel();

	// cancel all outstanding operations
	// since we are connected, the only possibilities are read and write
	iSocketsReader->Cancel();
	iSocketsReader->SetIssueRead(EFalse);
	iSocketsWriter->Cancel();
	iTimer->Reset();
	
	if (iResolverOpen) {
		iResolver.Close();
		iResolverOpen=EFalse;
	}
	
	if (iSocketOpen) {
		iSocket.Close();
		iSocketOpen=EFalse;
	}
	
	
//	iSendTimer->Reset();
//	iIdentTimer->Reset();
	
//	iFrom->Des().Zero();		
//	iPresenceInfo->Des().Zero();
//	iMessage->Des().Zero();
//	iSubject->Des().Zero();
	
	ChangeStatus(ENotConnected);
}

// from CActive
void CSocketsEngine::DoCancel()
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("DoCancel"));

	iTimer->Reset();
	
	// Cancel appropriate request to socket
	switch (iEngineStatus)
        {
        case EConnecting:
		iSocket.CancelConnect();
		iSocket.Close();
		iSocketOpen = EFalse;
		break;
        case ELookingUp:
		// Cancel look up attempt
		iResolver.Cancel();
		iResolver.Close();
		iResolverOpen=EFalse;
		break;
		
	case ENotConnected:
		// do nothing
		break;
		
	default:
		//User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
	}
	ChangeStatus(ENotConnected);
}

EXPORT_C void CSocketsEngine::WriteL(const TDesC8& aData)
{
	if ( iEngineStatus == EConnected || iEngineStatus == EDisconnecting ) {
		Log(_L("**Sending:"));
		Log(aData);
		Log(_L("**end"));
		if (! iDoCompress ) {
			iSocketsWriter->IssueWriteL(aData);
		} else {

			out_z_buf->Des().Zero();
			out_z_stream.avail_in = aData.Length();
			out_z_stream.next_in = (Bytef*)(aData.Ptr());

			do {
				out_z_stream.avail_out = 1024;
				out_z_stream.next_out = out_z_buf_tmp;
				TInt ret = deflate(&out_z_stream, Z_SYNC_FLUSH);
				if (ret==Z_NEED_DICT || ret==Z_DATA_ERROR || ret==Z_MEM_ERROR) {
					User::Leave(-11003);
				}
				TPtrC8 deflated( out_z_buf_tmp, 1024 - out_z_stream.avail_out );
				AppendToOutZBufL(deflated);
			} while ( out_z_stream.avail_out== 0);
			
			iSocketsWriter->IssueWriteL(*out_z_buf);
		}
	} else {
		User::Leave(KErrNotReady);
	}
}

EXPORT_C void CSocketsEngine::WriteL(const TDesC16& aData)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("WriteL"));
	Log(_L("CSocketsEngine::WriteL"));

	// might need a mecanism here to check if writing is possible, 
	// or is it taken car of in the socket writer?

	if ( iEngineStatus == EConnected || iEngineStatus == EDisconnecting ) {
		iTransferBuffer->Des().Zero();
		while (aData.Size() > 	iTransferBuffer->Des().MaxLength() )
		{
			iTransferBuffer=
				iTransferBuffer->ReAllocL(iTransferBuffer->Des().MaxLength()*2);
		}
		
		TPtr8 p=iTransferBuffer->Des();
		iCC->ConvertFromUnicode(p, aData);
	
		WriteL(*iTransferBuffer);
	}
}


EXPORT_C void CSocketsEngine::Read()
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("Read"));

	if ((iEngineStatus == EConnected) ) {
		if(!iSocketsReader->IsActive()) {
			iSocketsReader->SetIssueRead(ETrue);
			iSocketsReader->Start();
		}
	}    
}

TInt CSocketsEngine::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("CheckedRunError"));

	Log(_L("Error in CSocketsEngine::RunL %d"), aError);
	return aError;
}

void CSocketsEngine::CheckedRunL()
{
	TBuf<60> cs=_L("CSocketsEngine::RunL:");
	cs.AppendNum(iEngineStatus);

	iTimer->Reset(); 
	
	switch(iEngineStatus)
	{
	case EConnecting:
		if (iStatus == KErrNone) {
			iConnectionRetry = 0;
			ChangeStatus(EConnected);
			UpdateByteCount(1, 0);
			if (0) {
			TSoInetInterfaceInfo info;
			iSocket.LocalName(info.iAddress);
			TPckg<TSoInetInterfaceInfo> infop(info);
			TInt err=iSocket.GetOpt(
				KSolInetIfCtrl, KSoInetConfigInterface, infop);
			if (err==KErrNone) iMtu=info.iMtu;
			else iMtu=576;
			}

			iObserver.NotifyEngineStatus(MEngineObserver::ESocketConnected, KErrNone);
		} else {
			ReportError(ESocketConnectError, iStatus.Int());
		}
		break;
		
	case ELookingUp:
		iResolver.Close();
		iResolverOpen = EFalse;
		iConnectionRetry++;
		if (iStatus == KErrNone) {
			iNameRecord = iNameEntry();
			TBuf<15> ipAddr;
			TInetAddr::Cast(iNameRecord.iAddr).Output(ipAddr);
			ChangeStatus(ENotConnected);
			ConnectL(TInetAddr::Cast(iNameRecord.iAddr).Address());
		} else {	
			ReportError(EDNSLookupFailed, iStatus.Int());
		}
		break;
		
	default:
		User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
		
	};
}

void CSocketsEngine::expired(CBase* Source)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("expired"));

	if (Source==iTimer) {
		Cancel();

		if (iEngineStatus == EWaitingForRetry) {
			ChangeStatus(ENotConnected);
			OpenConnectionAndConnectL();
		} else { 
			ReportError(ESocketEngineTimeOut, KErrTimedOut);
		}
	} else {
		if (iRecvBytes != iPrevRecv) {
			DataCounter().SetNewCountL(KReceivedDataCounterTuple, 
				iConnectionName, iRecvBytes);
			iPrevRecv=iRecvBytes;
		}
		if (iSentBytes != iPrevSent) {
			DataCounter().SetNewCountL(KSentDataCounterTuple, 
				iConnectionName, iSentBytes);
			iPrevSent=iSentBytes;
		}
	}
}

//FIXME: localization

#include "settings.h"

void CSocketsEngine::ReportError(MEngineNotifier::TErrorType aErrorType, TInt aErrorCode)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("ReportError"));

	DisconnectSocket();
	TBool retry = EFalse;
	TBool gen_error=ETrue;
	
	switch (aErrorType)
        {
	case MEngineNotifier::EDNSLookupFailed:
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_ERROR, _L("DNS Lookup failed"));
		gen_error=EFalse;
		Log(_L("CSocketsEngine: DNS Lookup Failed"), aErrorCode);
		// helps to close the connector on DNS error
#ifdef __S60V2__
		delete iConnectionErrorListener; iConnectionErrorListener=0;
#endif
		iConnectionOpener->CloseConnection();
		if (iConnectionRetry < KMaxConnectionRetry) retry=ETrue;
		break;

	case MEngineNotifier::ESocketConnectError:
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_ERROR, _L("Cannot connect to server"));
		gen_error=EFalse;
		Log(_L("CSocketsEngine: Conn. failed"), aErrorCode);
		if (iConnectionRetry < KMaxConnectionRetry) retry=ETrue;
		break;	
	
	case MEngineNotifier::EUnknownError:
		Log(_L("CSocketsEngine: Unknown error"), aErrorCode);
		break;
			
	case MEngineNotifier::ESocketEngineTimeOut:
		Log(_L("CSocketsEngine: Timed out"), aErrorCode);			
		break;		
		
	case MEngineNotifier::ETimeOutOnRead:
		Log(_L("CSocketsEngine: Time Out on Read"), aErrorCode);
		break;
	
	case MEngineNotifier::EDisconnected:
		Log(_L("CSocketsEngine: Disconnected"), aErrorCode);
		break;
		
	case MEngineNotifier::EGeneralReadError:
		Log(_L("CSocketsEngine: Read Error"), aErrorCode);
		break;
		
	case MEngineNotifier::ETimeOutOnWrite:
		Log(_L("CSocketsEngine: Time Out on write"), aErrorCode);
		break;
		
	case MEngineNotifier::EGeneralWriteError:
		Log(_L("CSocketsEngine: Write Error"), aErrorCode);
		break;

	case MEngineNotifier::ENetworkConnectError:
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_ERROR, _L("Cannot connect to network"));
		gen_error=EFalse;
		Log(_L("CSocketsEngine: Connect to net error"), aErrorCode);
		break;
	case MEngineNotifier::EConnectionNotAllowed:
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_ERROR, _L(""));
		gen_error=EFalse;
		Log(_L("CSocketsEngine: connection denied by user"), aErrorCode);
		break;

	default:
		User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
        }

	// we retry only for problems happening while connecting to server
	// for all other problems, the observer should trigger the retry. 

	if (gen_error) {
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_ERROR, _L("Connection dropped"));
	}
	if (retry) {
		Log(_L("EWaitingForRetry"));
		ChangeStatus(EWaitingForRetry);
		iTimer->Wait(iTimeBeforeRetry);
	} else if (aErrorType==MEngineNotifier::EConnectionNotAllowed) {
		Log(_L("MEngineObserver::EConnectionNotAllowed"));
		iObserver.NotifyEngineStatus(MEngineObserver::EConnectionNotAllowed, aErrorCode);
	} else if (iConnectionRetry == KMaxConnectionRetry) {
		Log(_L("MEngineObserver::ESocketUnreachable"));
		iObserver.NotifyEngineStatus(MEngineObserver::ESocketUnreachable, aErrorCode);
		iConnectionRetry=0;
	} else {
		Log(_L("MEngineObserver::ESocketDisconnected"));
		iObserver.NotifyEngineStatus(MEngineObserver::ESocketDisconnected, aErrorCode);
	}
}

void CSocketsEngine::ResponseReceived(const TDesC8& aBuffer)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("ResponseReceived"));

#ifdef __WINS__
	User::Check();
#endif

	Log(_L("**Received:"));
	if (!iDoCompress) {
		iObserver.NotifyNewData(aBuffer);
		Log(aBuffer);
	} else {
		if (aBuffer.Length()==0) return;

		in_z_stream.avail_in = aBuffer.Length();
		in_z_stream.next_in = (Bytef*)aBuffer.Ptr();
		do {
			in_z_stream.avail_out = in_z_buf->Des().MaxLength();
			in_z_stream.next_out = (Bytef*)in_z_buf->Des().Ptr();
			TInt ret = inflate(&in_z_stream, Z_SYNC_FLUSH);
			if (ret==Z_NEED_DICT || ret==Z_DATA_ERROR || ret==Z_MEM_ERROR) {
				User::Leave(-11003);
			}
			TPtrC8 inflated( in_z_buf->Ptr(), 
				in_z_buf->Des().MaxLength() - in_z_stream.avail_out );
			Log(inflated);
#ifdef __WINS__
	User::Check();
#endif
			iObserver.NotifyNewData(inflated);
#ifdef __WINS__
	User::Check();
#endif
			
		} while(in_z_stream.avail_out == 0);
	}
	Log(_L("**end"));

#ifdef __WINS__
	User::Check();
#endif

}

void CSocketsEngine::ChangeStatus(TSocketsEngineState aNewStatus)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("ChangeStatus"));

	switch (aNewStatus)
        {
        case ENotConnected:
		Log(_L("Socket engine: Not connected"));
		break;
		
        case EConnecting:
		Log(_L("Socket engine: Connecting"));
		break;
		
	case EConnected:
		Log(_L("Socket engine: Connected"));
		break;
		
        case ELookingUp:
		Log(_L("Socket engine: Looking up"));
		break;
		
	case EDisconnecting:
		Log(_L("Socket engine: Disconnecting"));
		break;
		
	case EWaitingForRetry:
		Log(_L("Socket engine: Waiting For Retry"));
		break;
		
        default:
		Log(_L("Socket engine: Unknown status"));
		User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
        }
	iEngineStatus = aNewStatus;
}

// from MConnectionOpenerObserver --------------------------------------------------------------------
void CSocketsEngine::success(CBase* /*source*/)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("success"));

	CC_TRAPD(err, ConnectL());
	if (err!=KErrNone) {
		ReportError(EUnknownError, err);
#ifdef __S60V2__
	} else {
		delete iConnectionErrorListener; iConnectionErrorListener=0;
		iConnectionErrorListener=CConnectionErrorListener::NewL(iConnection, *this);
#endif
	}
}

void CSocketsEngine::error(CBase* /*source*/, TInt code, const TDesC& reason)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("error"));

	Log(reason);
	if (code==KErrAccessDenied) {
		ReportError(EConnectionNotAllowed, code);
	} else {
		ReportError(ENetworkConnectError, code);
	}
}

void CSocketsEngine::info(CBase* /*source*/, const TDesC& msg)
{
	CALLSTACKITEM_N(_CL("CSocketsEngine"), _CL("info"));

	Log(msg);
}

void CSocketsEngine::CanWrite()
{
	//Log(_L("CSocketsEngine::CanWrite"));
	iObserver.NotifyCanWrite();
}

EXPORT_C void CSocketsEngine::StopRead()
{
	iSocketsReader->SetIssueRead(EFalse);
	iSocketsReader->Cancel();
}

EXPORT_C void CSocketsEngine::NoReadTimeout()
{
	iSocketsReader->NoTimeout();
}

EXPORT_C void CSocketsEngine::ReadTimeout()
{
	iSocketsReader->Timeout();
}

void CSocketsEngine::UpdateByteCount(TInt aNewReceived, TInt aNewSent)
{
	if (iConnectionName.Length() == 0) return;

	iCounterTimer->WaitMax(5);
	TInt packets;
	if (aNewReceived > 0) {
		iRecvBytes += aNewReceived;
		packets=aNewReceived / iMtu + 1;
		iRecvBytes += packets*(iOverhead+2);
		iSentBytes += iOverhead; // ack
	}
	if (aNewSent > 0) {
		iSentBytes += aNewSent;
		packets=aNewSent / iMtu + 1;
		iSentBytes += packets*iOverhead;
		iRecvBytes += iOverhead; // ack
	}
}


#include "cl_settings.h"

EXPORT_C TUint32 CSocketsEngine::AccessPoint()
{
	TInt ap;
	if (!iAccessPoint) {
		Settings().GetSettingL(SETTING_IP_AP, ap);
	}
	iAccessPoint=ap;
	return iAccessPoint;
}
