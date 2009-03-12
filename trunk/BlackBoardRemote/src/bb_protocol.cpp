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
#include "bb_protocol.h"

#include <in_sock.h>

#include <plpvariant.h>
#include "timeout.h"
#include "EngineNotifier.h"
#include "xmlbuf.h"

#include "expat.h"
#include "list.h"

#include "connectioninit.h"
#include "app_context.h"

#include "SocketsEngine.h"
#include <TimeOut.h>
#include "SocketsReader.h"
#include "SocketsWriter.h"
#include "Sockets.pan"
#include "presence_data.h"

#include <flogger.h>

#include <string.h>
#include <es_sock.h>
#include "app_context.h"
#include "symbian_auto_ptr.h"

#include <charconv.h>
#include "reporting.h"
#include "settings.h"
#include "cl_settings.h"
#include "md5.h"
#include <e32math.h>


#include "bb_incoming.h"

class CBBProtocolImpl : public CBBProtocol, public MTimeOut, public MEngineNotifier,
public MSocketObserver, public MContextBase, public MIncomingObserver
{
private:
	
	CBBProtocolImpl(MApp_context& Context);
	~CBBProtocolImpl();
	void ConstructL();
	
	virtual void AddObserverL(MBBNotifier* aObserver);
	
	virtual void ConnectL(TUint aAccessPoint, const TDesC& aServerName, TUint port, const TDesC& aAuthorName);
	void Disconnect(TBool aStopByRequest, TBool closeConnection);
	
	void DisconnectSocket();
	
	virtual void WriteL(const TDesC16& aData);
	
	virtual void SendXMLStreamHeaderL();
	
	virtual void SendDisconnectionL();
	virtual void SendIdentificationL();
	virtual void SendXmlPacketL(const TDesC& aPacket); // data has to live until acked
	void Read();
	TBool Connected() const;
	
	
	// from MTimeOut
	IMPORT_C void expired(CBase* Source);
	
	// from MEngineNotifier
	void ReportError(MEngineNotifier::TErrorType aErrorType, TInt aErrorCode);
	void ResponseReceived(const TDesC8& aBuffer);
	void CanWrite();
	
	CList<MBBNotifier*>* iObservers;
	
	//from MsocketObserver
	
	void success(CBase* source);
	
	void error(CBase* source, TInt code, const TDesC& reason);
	void info(CBase* source, const TDesC& msg);
	
	
	// from CActive
	void DoCancel();
	void CheckedRunL();
	TInt CheckedRunError(TInt aError);
	
	void ConnectL(TUint32 aAddr);
	void ConnectL();
	
	/*!
	@enum TSocketsEngineState
	
	  @discussion Tracks the state of this object through the connection process
	  @value ENotConnected The initial (idle) state
	  @value EConnecting A connect request is pending with the socket server
	  @value EConnected A connection has been established
	  @value ELookingUp A DNS lookup request is pending with the socket server
	*/
	enum TSocketsEngineState 
	{
		ENotConnected,
			EConnecting,
			EConnected,
			ELookingUp,
			EDisconnecting,
			EWaitingForRetry
	};

	TBool	iLoggedIn;

	void IncomingData(const MBBData* aData, TBool aErrors);
	void StreamOpened();
	void StreamClosed();
	void StreamError(TInt aError, const TDesC& aDescr);

	void ChangeStatus(TSocketsEngineState aNewStatus);
	void ReportError(TInt aError, TInt aOrigError, const TDesC& aDescr);
	void Acked(TUint id);
	void IncomingTuple(const CBBTuple* aTuple);
	void ReadyToWrite(TBool aReady);
	void NotifyDisconnected();
	
private: // Member variables
	
	/*! @const The maximum time allowed for a lookup or connect requests to complete */
	static const TInt KTimeOut;
	
	/*! @const The initial port number displayed to the user */
	static const TInt KDefaultPortNumber;
	static const TInt KMaxIdentificationRetry;
	static const TInt KTimeBeforeRetry;
	
	/*! @var this object's current status */
	TSocketsEngineState         iEngineStatus;
	
	/*! @var the actual socket */
	RSocket                     iSocket;
	
	/*! @var active object to control reads from the socket */
	CSocketsReader*             iSocketsReader;
	
	/*! @var active object to control writes to the socket */
	CSocketsWriter*             iSocketsWriter;
	
	/*! @var DNS name resolver */
	RHostResolver               iResolver;
	
	/*! @var The result from the name resolver */
	TNameEntry                  iNameEntry;
	
	/*! @var The anme record found by the resolver */
	TNameRecord                 iNameRecord;
	
	/*! @var timer active object */
	CTimeOut*		iTimer;
	CTimeOut*		iIdentTimer;
	
	/*! @var The address to be used in the connection */
	TInetAddr                   iAddress;
	
	/*! @var port number to connect to */
	TInt                        iPort;
	
	/*! @var server name to connect to */
	TBuf16<KMaxServerNameLength>  iServerName;
	TBuf<50>			iAuthorName;
	
	TInt iConnectionRetry;
	TInt iTimeBeforeRetry;
	
	CConnectionOpener *	iConnectionOpener;
	TUint32		iAccessPoint;
	RSocketServ		iSocketServ;
	
	TBool iSocketOpen;
	TBool iResolverOpen;
	TBool iCloseConnAfterDisconnect;
	
	CXmlBuf	*iXmlBuf;
	TInt iIdAttempt;
	
	CCnvCharacterSetConverter*	iCC;
	
#ifdef __S60V2__
	RConnection		iConnection;
#endif

	CStream*	iStream;

	void InitZStreamsL();

	z_stream	in_z_stream, out_z_stream;
	TBool		in_z_open, out_z_open;

	HBufC8		*in_z_buf, *out_z_buf;

	void AppendToOutZBufL(const TDesC8& aData);
	Bytef		out_z_buf_tmp[1024];
	HBufC8		*iTransferBuffer;
	TBool		iStopByRequest;
#ifdef __WINS__
	RFile		iDebugFile, iDebugFileOut;
#endif

	friend class CBBProtocol;
	friend class auto_ptr<CBBProtocolImpl>;
};


const TInt CBBProtocolImpl::KTimeOut = 120; // 120 seconds time-out
const TInt CBBProtocolImpl::KMaxIdentificationRetry = 1;

#ifndef __WINS__
const TInt CBBProtocolImpl::KTimeBeforeRetry = 120; //120 seconds before retry
#else
const TInt CBBProtocolImpl::KTimeBeforeRetry = 10; //120 seconds before retry
#endif

const TInt CBBProtocolImpl::KDefaultPortNumber = 5222;


CBBProtocol::CBBProtocol() : CCheckedActive(EPriorityNormal, _L("CBBProtocol"))
{
	CALLSTACKITEM_N(_CL("CBBProtocol"), _CL("CBBProtocol"));

}

EXPORT_C CBBProtocol* CBBProtocol::NewL(MApp_context& aContext)
{
	CALLSTACKITEM_N(_CL("CBBProtocol"), _CL("NewL"));

	auto_ptr<CBBProtocolImpl> ret(new (ELeave) CBBProtocolImpl(aContext));
	ret->ConstructL();
	return ret.release();
}

CBBProtocolImpl::CBBProtocolImpl(MApp_context& Context) : MContextBase(Context),
	iTimeBeforeRetry(KTimeBeforeRetry)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("CBBProtocolImpl"));
	
	Reporting().DebugLog(_L("Socket engine created."));
}


CBBProtocolImpl::~CBBProtocolImpl()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("~CBBProtocolImpl"));

#ifdef __WINS__
	iDebugFile.Close();
	iDebugFileOut.Close();
#endif

	delete iCC;

	Cancel();
	
	delete iSocketsReader;
	
	delete iSocketsWriter;
	
	delete iConnectionOpener;
	
	if (iSocketOpen) iSocket.Close();

	if (iResolverOpen) iResolver.Close();

#ifdef __S60V2__
	iConnection.Close();
#endif
	iSocketServ.Close();
	
	delete iTimer;
	delete iIdentTimer;
	delete iXmlBuf;
	delete iStream;

	delete iObservers;

	if (in_z_open) inflateEnd(&in_z_stream);
	if (out_z_open) deflateEnd(&out_z_stream);

	delete in_z_buf;
	delete out_z_buf;
	delete iTransferBuffer;
}


void CBBProtocolImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ConstructL"));

	iCC=CCnvCharacterSetConverter::NewL();
	iCC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());;

#ifdef __WINS__
	if (iDebugFile.Open(Fs(), 
		_L("c:\\bbdebug.txt"), EFileWrite)==KErrNone) {
			TInt pos=0;
			iDebugFile.Seek(ESeekEnd, pos);
	} else {
		User::LeaveIfError(iDebugFile.Replace(Fs(), _L("c:\\bbdebug.txt"), EFileWrite));
	}
	if (iDebugFileOut.Open(Fs(), 
		_L("c:\\bbdebugout.txt"), EFileWrite)==KErrNone) {
			TInt pos=0;
			iDebugFileOut.Seek(ESeekEnd, pos);
	} else {
		User::LeaveIfError(iDebugFileOut.Replace(Fs(), _L("c:\\bbdebugout.txt"), EFileWrite));
	}
#endif
	iTransferBuffer=HBufC8::NewL(2048);

	User::LeaveIfError(iSocketServ.Connect());
	
	iXmlBuf=CXmlBuf::NewL(256);

#ifdef __S60V2__
	iConnection.Open(iSocketServ);
#endif
	
#ifndef __S60V2__
	iConnectionOpener = CConnectionOpener::NewL(*this, iSocketServ);
#else
	iConnectionOpener = CConnectionOpener::NewL(*this, iSocketServ, iConnection);
#endif
	
	ChangeStatus(ENotConnected);
	
	// Start a timer
	iTimer = CTimeOut::NewL(*this);
	iIdentTimer = CTimeOut::NewL(*this);
	CActiveScheduler::Add(this); 
	
	// Create socket read and write active objects
	iSocketsReader = CSocketsReader::NewL(*this, iSocket, AppContext());
	iSocketsWriter = CSocketsWriter::NewL(*this, iSocket, AppContext());

	iObservers=CList<MBBNotifier*>::NewL();

	in_z_buf=HBufC8::NewL(4096);
	out_z_buf=HBufC8::NewL(4096);	
}

void CBBProtocolImpl::InitZStreamsL()
{
	if (in_z_open) inflateEnd(&in_z_stream);
	if (out_z_open) deflateEnd(&out_z_stream);

	TInt ret=Z_OK;
	in_z_stream.zalloc = Z_NULL;
	in_z_stream.zfree = Z_NULL;
	in_z_stream.opaque = Z_NULL;
	ret = inflateInit(&in_z_stream);
	if (ret == Z_MEM_ERROR) User::Leave(-11002);
	if (ret == Z_VERSION_ERROR) User::Leave(-11003);
	if (ret != Z_OK) User::Leave(-11006);
	in_z_open=ETrue;

	out_z_stream.zalloc = Z_NULL;
	out_z_stream.zfree = Z_NULL;
	out_z_stream.opaque = Z_NULL;
	ret = deflateInit(&out_z_stream, Z_DEFAULT_COMPRESSION);
	if (ret == Z_MEM_ERROR) User::Leave(-11004);
	if (ret == Z_VERSION_ERROR) User::Leave(-11005);
	if (ret != Z_OK) User::Leave(-11007);
	out_z_open=ETrue;
}

void CBBProtocolImpl::ConnectL(TUint aAccessPoint, const TDesC& aServerName, TUint port, const TDesC& aAuthorName)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ConnectL"));


	iLoggedIn=EFalse;

	iServerName=aServerName;
	iAccessPoint = aAccessPoint;
	iPort=port;
	iAuthorName=aAuthorName;

	iConnectionOpener->MakeConnectionL(iAccessPoint);
}

void CBBProtocolImpl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ConnectL"));


	// Initiate connection process
	if ( (iEngineStatus == ENotConnected ) )
	{
		iLoggedIn=EFalse;

		TInetAddr addr;
		if (addr.Input(iServerName) == KErrNone)
		{
			// server name is already a valid ip address
			ConnectL(addr.Address());
		}
		else // need to look up name using dns
		{
			if (iResolverOpen) {
				iResolver.Close();
				iResolverOpen=EFalse;
			}

			// Initiate DNS
#ifndef __S60V2__
			User::LeaveIfError(iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp));
#else
			User::LeaveIfError(iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp, iConnection));
#endif
			iResolverOpen = ETrue;
			
			// DNS request for name resolution
			iResolver.GetByName(iServerName, iNameEntry, iStatus);
			
			ChangeStatus(ELookingUp);
			// Request time out
			iTimer->Wait(KTimeOut);
			SetActive();
		}
        }
	
}


void CBBProtocolImpl::ConnectL(TUint32 aAddr) // <a name="ConnectL32">
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ConnectL"));

	iConnectionRetry++;
	//FIXME
	Reporting().DebugLog(_L("Connection attempt :") /*,iConnectionRetry*/);
	
	
	// Initiate attempt to connect to a socket by IP address	
	if (iEngineStatus == ENotConnected)
        {
		iLoggedIn=EFalse;
		
		// Open a TCP socket
		if (iSocketOpen) {
			iSocket.Close();
			iSocketOpen=EFalse;
		}

#ifndef __S60V2__
		User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp));
#else
		User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, iConnection));
#endif
		iSocketOpen = ETrue;
		
		// Set up address information
		iAddress.SetAddress(aAddr);
		iAddress.SetPort(iPort);	
		
		// Initiate socket connection
		iSocket.Connect(iAddress, iStatus);
		ChangeStatus(EConnecting);
		
		// Start a timeout
		iTimer->Wait(KTimeOut);
		SetActive();
        }
}


void CBBProtocolImpl::Disconnect(TBool aByStopRequest, TBool closeConnection) 
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("Disconnect"));

	iStopByRequest=aByStopRequest;

	ReadyToWrite(EFalse);

	if (Connected()) {
		ChangeStatus(EDisconnecting);
		iCloseConnAfterDisconnect=closeConnection;
		SendDisconnectionL();
	} else {
		NotifyDisconnected();
	}

}

void CBBProtocolImpl::DisconnectSocket()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("DisconnectSocket"));


	// cancel all outstanding operations
	// since we are connected, the only possibilities are read and write
	iSocketsReader->Cancel();
	iSocketsReader->SetIssueRead(EFalse);
	iSocketsWriter->Cancel();
	
	if (iResolverOpen)
	{
		iResolver.Close();
		iResolverOpen=EFalse;
	}
	
	if (iSocketOpen)
	{
		iSocket.Close();
		iSocketOpen=EFalse;
	}
	
	iTimer->Reset();
	iIdentTimer->Reset();
	
	ChangeStatus(ENotConnected);
}

// from CActive
void CBBProtocolImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("DoCancel"));


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
		//User::Panic(KPaniCBBProtocolImpl, ESocketsBadStatus);
		
		break;
	}
	
	ChangeStatus(ENotConnected);
}

void CBBProtocolImpl::AppendToOutZBufL(const TDesC8& aData)
{
	if ( out_z_buf->Des().Length() + aData.Length() > out_z_buf->Des().MaxLength() ) {
		out_z_buf=out_z_buf->ReAllocL(out_z_buf->Des().MaxLength()*2);
	}
	out_z_buf->Des().Append(aData);
}

void CBBProtocolImpl::WriteL(const TDesC16& aData)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("WriteL"));

	// Write data to socket
	if ( iEngineStatus == EConnected || iEngineStatus == EDisconnecting )
        {
		iTransferBuffer->Des().Zero();
		while (aData.Size() > 	iTransferBuffer->Des().MaxLength() )
		{
			iTransferBuffer=
				iTransferBuffer->ReAllocL(iTransferBuffer->Des().MaxLength()*2);
		}
		
		TPtr8 p=iTransferBuffer->Des();
		iCC->ConvertFromUnicode(p, aData);
#ifdef __WINS__
		iDebugFileOut.Write(p);
#endif

		out_z_buf->Des().Zero();
		out_z_stream.avail_in = iTransferBuffer->Des().Length();
		out_z_stream.next_in = (Bytef*)iTransferBuffer->Des().Ptr();

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
        } else {
		User::Leave(KErrNotReady);
	}
}

void CBBProtocolImpl::SendXMLStreamHeaderL()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("SendXMLStreamHeaderL"));

	Reporting().DebugLog(_L("SendXMLStreamHeaderL"));

	if ( (iEngineStatus == EConnected) )
	{
		delete iStream; iStream=0;
		InitZStreamsL();
		iStream=CStream::NewL(*this, AppContext());

		TBuf16<200> header;
		
#ifdef __WINS__
		//header.Append(_L("test\n"));
#endif
		header.Append(_L16("<?xml version='1.0'?>\n<stream>\n"));
		
		WriteL(header);
	}
}

void CBBProtocolImpl::SendDisconnectionL()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("SendDisconnectionL"));


	if ( (iEngineStatus == EConnected)  || (iEngineStatus == EDisconnecting))
	{
		TBuf16<200> msg;
		msg.Append(_L16("</stream>\n"));
		WriteL(msg);
	}
}

void CBBProtocolImpl::SendIdentificationL()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("SendIdentificationL"));

	Reporting().DebugLog(_L("SendIdentificationL"));

	if (iEngineStatus == EConnected)
	{
		iXmlBuf->Zero();
		iXmlBuf->BeginElement(_L("ident"));
		iXmlBuf->Leaf(_L("id"), _L("0"));
		TPlpVariantMachineId machineId;
#ifndef __WINS__
		PlpVariant::GetMachineIdL(machineId);
#else
		if (! Settings().GetSettingL(SETTING_WINS_IMEI, machineId) ) {
#  ifdef __S60V1__
			machineId=_L("emu01");
#  else
			machineId=_L("emu02");
#  endif
		}
#endif
		iXmlBuf->Leaf(_L("imei"), machineId);
		iXmlBuf->Leaf(_L("name"), iAuthorName);

		MD5_CTX md5;
		MD5Init(&md5);
		{
			TBuf8<20> imei;
			iCC->ConvertFromUnicode(imei, machineId);
			MD5Update(&md5, (TUint8*)imei.Ptr(), imei.Length());
		}
		{
			TInt64 t(User::TickCount(),
					GetTime().Int64().Low());
			TInt salt=Math::Rand(t);
			TBuf8<12> saltbuf8;
			TBuf<12> saltbuf;
			saltbuf.AppendNum( (TUint)salt, EHex );
			saltbuf8.AppendNum( (TUint)salt, EHex );
			MD5Update(&md5, (TUint8*)saltbuf8.Ptr(), saltbuf8.Length());
			iXmlBuf->Leaf(_L("salt"), saltbuf);
		}
		{
			TInt sequence=1;
			Settings().GetSettingL(SETTING_AUTH_SEQ, sequence);
			TBuf8<12> seqbuf8; TBuf<12> seqbuf;
			seqbuf.AppendNum( (TUint)sequence );
			seqbuf8.AppendNum( (TUint)sequence );
			MD5Update(&md5, (TUint8*)seqbuf8.Ptr(), seqbuf8.Length());
			iXmlBuf->Leaf(_L("auth.seqno"), seqbuf);
			sequence++;
			Settings().WriteSettingL(SETTING_AUTH_SEQ, sequence);
		}
		{
			TBuf<50> passw; TBuf8<50> passw8;
			Settings().GetSettingL(SETTING_PUBLISH_PASSWORD, passw);
			iCC->ConvertFromUnicode(passw8, passw);
			MD5Update(&md5, (TUint8*)passw8.Ptr(), passw8.Length());
		}
		{
			TBuf8<16> hash; hash.SetLength(hash.MaxLength());
			MD5Final( (TUint8*)hash.Ptr(), &md5);
			TBuf<32> sig;
			for (int i=0; i<16; i++) {
				sig.AppendNumFixedWidth( hash[i], EHex, 2 );
			}
			iXmlBuf->Leaf(_L("signature"), sig);
		}
		
		iXmlBuf->EndElement(_L("ident"));
		
		iXmlBuf->Characters(_L("\n"));
		WriteL(iXmlBuf->Buf());
		iIdentTimer->Wait(KTimeOut);
	}
}


void CBBProtocolImpl::Read()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("Read"));


	// Initiate read of data from socket
	if ((iEngineStatus == EConnected) )
	{
		if(!iSocketsReader->IsActive()) 
		{
			//Log(_L("Issue Read()"));
			iSocketsReader->SetIssueRead(ETrue);
			iSocketsReader->Start();
		}
	}    
}

TInt CBBProtocolImpl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("CheckedRunError"));

	Reporting().UserErrorLog(_L("Error in CBBProtocolImpl::RunL %d"), aError);
	return aError;
}

// from CActive
void CBBProtocolImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("CheckedRunL"));

	// Active object request complete handler.
	// iEngineStatus flags what request was made, so its
	// completion can be handled appropriately
	iTimer->Reset(); // Cancel TimeOut timer before completion
	
	switch(iEngineStatus)
	{
	case EConnecting:
		// IP connection request
		if (iStatus == KErrNone)
			// Connection completed successfully
		{
			ChangeStatus(EConnected);
			iConnectionRetry = 0;
			SendXMLStreamHeaderL();
			Read();
		}
		else
		{
			iSocket.Close();
			iSocketOpen = EFalse;
			Reporting().UserErrorLog(_L("CBBProtocolImpl: Conn. failed"), iStatus.Int());
			ChangeStatus(ENotConnected);
			ReportError(ESocketConnectError, iStatus.Int());
		}
		break;
		
	case ELookingUp:
		iResolver.Close();
		iResolverOpen = EFalse;
		if (iStatus == KErrNone)
		{
			// DNS look up successful
			iNameRecord = iNameEntry();
			// Extract domain name and IP address from name record
			//Print(_L("Domain name = "));
			//Print(iNameRecord.iName);
			TBuf<15> ipAddr;
			TInetAddr::Cast(iNameRecord.iAddr).Output(ipAddr);
			//Print(_L("\r\nIP address = "));
			//Print(ipAddr);
			//	Print(_L("\r\n"));
			// And connect to the IP address
			ChangeStatus(ENotConnected);
			ConnectL(TInetAddr::Cast(iNameRecord.iAddr).Address());
		}
		else
		{	
			// DNS lookup failed
			// iConsole.ErrorNotify(_L("CBBProtocolImpl\nDNS lookup failed"), iStatus.Int());
			ChangeStatus(ENotConnected);
			ReportError(EDNSLookupFailed, 0);
		}
		
		break;
		
	default:
		User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
		
	};

}

void CBBProtocolImpl::expired(CBase* Source)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("expired"));


	//User::Panic(_L("TEST"), 13);

	if (Source==iTimer) 
	{
		if (iEngineStatus == EWaitingForRetry)
		{
			ChangeStatus(ENotConnected);
			ConnectL(iAccessPoint, iServerName, iPort, iAuthorName);
		}
		else
		{
			Cancel();
			Reporting().UserErrorLog(_L("CBBProtocolImpl: Timed out"), KErrTimedOut);
			ReportError(ESocketEngineTimeOut, KErrTimedOut);
		}
	}
	else if (Source==iIdentTimer)
	{
		ReportError(EIdentificationTimeOut,0);
	}
}

void CBBProtocolImpl::ReportError(MEngineNotifier::TErrorType aErrorType, TInt aErrorCode)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ReportError"));

	if (iLoggedIn && aErrorType==MEngineNotifier::ETimeOutOnRead) {
		// we don't mind, we assume the timeouts on various
		// things are set on a higher level
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Time Out on Read"), aErrorCode);
		iSocketsReader->Start();
		return;
	}
		

	DisconnectSocket();
	if (iEngineStatus==EDisconnecting) {
		NotifyDisconnected();
		return;
	}
	
	TBool retry = ETrue;
	
	switch (aErrorType)
        {
	case MEngineNotifier::EDNSLookupFailed:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: DNS Lookup Failed"), aErrorCode);
		if (iConnectionRetry >= 3 )
		{
			iConnectionOpener->CloseConnection();
		}
		break;
		
	case MEngineNotifier::EIdentificationFailed:
		
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Identification Failed"), aErrorCode);
		ReportError(EIdentificationError, aErrorCode, _L("ident failed"));
		iIdAttempt=0;
		retry = EFalse;
		break;
		
	case MEngineNotifier::EIdentificationTimeOut:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Identifiation Timed out"), aErrorCode);			
		break;
	case MEngineNotifier::ESocketEngineTimeOut:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Timed out"), aErrorCode);			
		break;
		
		
	case MEngineNotifier::ETimeOutOnRead:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Time Out on Read"), aErrorCode);
		break;
	case MEngineNotifier::EStreamError:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: StreamError"), aErrorCode);
		// report to context_log ?
		break;
		
	case MEngineNotifier::EDisconnected:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Disconnected"), aErrorCode);
		break;
		
	case MEngineNotifier::EGeneralReadError:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Read Error"), aErrorCode);
		break;
		
	case MEngineNotifier::ETimeOutOnWrite:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Time Out on write"), aErrorCode);
		break;
		
	case MEngineNotifier::EGeneralWriteError:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Write Error"), aErrorCode);
		break;
		
	case MEngineNotifier::EXmlParseError:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: XML Parse Error"), aErrorCode);
		break;
	case MEngineNotifier::ENetworkConnectError:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Connect to net error"), aErrorCode);
		break;
	case MEngineNotifier::ESocketConnectError:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Connect to server error"), aErrorCode);
		break;
	case MEngineNotifier::EUnknownError:
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Unknown error"), aErrorCode);
		break;
	default:
		User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
        }
	
#ifndef __WINS__
	if ( iConnectionRetry == 5 && retry)
#else
	if ( iConnectionRetry == 150 && retry)
#endif
	{
		ReportError(EServerUnreachable,aErrorCode, _L("server unreachable"));
		Reporting().UserErrorLog(_L("CBBProtocolImpl: Server Unreachable"), aErrorCode);
		retry=EFalse;
	}
	
	if (retry)
	{
		//wait and retry
		ChangeStatus(EWaitingForRetry);
		//iTimeBeforeRetry = iTimeBeforeRetry;
		iTimer->Wait(iTimeBeforeRetry);
	}
	
}

void CBBProtocolImpl::ResponseReceived(const TDesC8& aBuffer)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ResponseReceived"));

	Reporting().DebugLog(_L("Receive:"));

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
#ifdef __WINS__
		iDebugFile.Write(inflated);
#endif
		iStream->ParseL(inflated);
		
	} while(in_z_stream.avail_out == 0);
	
}

void CBBProtocolImpl::ChangeStatus(TSocketsEngineState aNewStatus)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ChangeStatus"));


	// Update the status (and the status display)
	switch (aNewStatus)
        {
        case ENotConnected:
		Reporting().DebugLog(_L("Not connected"));
		break;
		
        case EConnecting:
		Reporting().DebugLog(_L("Connecting"));
		break;
		
	case EConnected:
		Reporting().DebugLog(_L("Connected"));
		break;
		
        case ELookingUp:
		Reporting().DebugLog(_L("Looking up"));
		break;
		
	case EDisconnecting:
		Reporting().DebugLog(_L("Disconnecting"));
		break;
		
	case EWaitingForRetry:
		Reporting().DebugLog(_L("Waiting For Retry"));
		break;
		
        default:
		User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
        }
	
	iEngineStatus = aNewStatus;
}

TBool CBBProtocolImpl::Connected() const
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("Connected"));

	return (iEngineStatus == EConnected);
}

// from MConnectionOpenerObserver --------------------------------------------------------------------
void CBBProtocolImpl::success(CBase* /*source*/)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("success"));


	CC_TRAPD(err, ConnectL());
	if (err!=KErrNone) {
		ReportError(EUnknownError, err);
	}
}

void CBBProtocolImpl::error(CBase* , TInt code, const TDesC& reason)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("error"));


	Reporting().DebugLog(reason);
	ReportError(ENetworkConnectError, code);
}

void CBBProtocolImpl::info(CBase* , const TDesC& msg)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("info"));


	Reporting().DebugLog(msg);
}

void CBBProtocolImpl::ReportError(TInt aError, TInt aOrigError, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ReportError"));

	for (CList<MBBNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->Error(aError, aOrigError, aDescr);
	}
}

void CBBProtocolImpl::NotifyDisconnected()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("NotifyDisconnected"));

	for (CList<MBBNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->Disconnected(iStopByRequest);
	}
}
void CBBProtocolImpl::Acked(TUint aId)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("Acked"));

	if (iLoggedIn==EFalse && aId==0) {
		iLoggedIn=ETrue;
		iIdentTimer->Reset();
		iIdAttempt=0;
		ReadyToWrite(ETrue);
	} else {
		for (CList<MBBNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
			n->Item->Acked(aId);
		}
	}
}

void CBBProtocolImpl::IncomingTuple(const CBBTuple* aTuple)
{
	for (CList<MBBNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->IncomingTuple(aTuple);
	}
}


void CBBProtocolImpl::ReadyToWrite(TBool aReady)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("ReadyToWrite"));

	for (CList<MBBNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->ReadyToWrite(aReady);
	}
}

void CBBProtocolImpl::IncomingData(const MBBData* aData, TBool aErrors)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("IncomingData"));

	const TBBAck* ack=bb_cast<TBBAck>(aData);
	if (ack) {
		Acked(ack->iId());
	}
	const CBBTuple* tuple=bb_cast<CBBTuple>(aData);
	if (tuple) {
		IncomingTuple(tuple);
	}
}

void CBBProtocolImpl::StreamOpened()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("StreamOpened"));

	SendIdentificationL();
}

void CBBProtocolImpl::StreamClosed()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("StreamClosed"));

	if (Connected() || (iEngineStatus==EDisconnecting) ) {
		DisconnectSocket();
		if (iCloseConnAfterDisconnect)
		{
			iConnectionOpener->CloseConnection();
		}
		NotifyDisconnected();
	}
}

void CBBProtocolImpl::StreamError(TInt aError, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("StreamError"));

	ReportError(MEngineNotifier::EStreamError, aError);
}

void CBBProtocolImpl::AddObserverL(MBBNotifier* aObserver)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("AddObserverL"));

	iObservers->AppendL(aObserver);
}

void CBBProtocolImpl::SendXmlPacketL(const TDesC& aPacket)
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("SendXmlPacketL"));

	if (Connected()) WriteL(aPacket);
}

void CBBProtocolImpl::CanWrite()
{
	CALLSTACKITEM_N(_CL("CBBProtocolImpl"), _CL("CanWrite"));

	if (Connected() && iLoggedIn) ReadyToWrite(ETrue);
}
