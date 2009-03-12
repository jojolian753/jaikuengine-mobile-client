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
#include "bbl_protocol.h"

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


#include "bbl_incoming.h"
#include "bb_incoming.h"

class CBBLocalProtocolImpl : public CBBLocalProtocol, public MTimeOut, public MEngineNotifier,
	public MContextBase, public MIncomingObserver {
private:
	
	CBBLocalProtocolImpl(MApp_context& Context, MBBLocalProtocolOwner& aOwner);
	~CBBLocalProtocolImpl();
	void ConstructL();
	
	virtual void AddObserverL(MBBLocalNotifier* aObserver);
	
	void Disconnect(TBool closeConnection);
	
	void DisconnectSocket();
	
	virtual void WriteL(const TDesC16& aData);
	
	virtual void SendXMLStreamHeaderL();
	
	virtual void SendDisconnectionL();
	//virtual void SendIdentificationL();
	virtual void SendXmlPacketL(const TDesC& aPacket); // data has to live until acked
	void Read();
	TBool Connected() const;
	
	
	// from MTimeOut
	void expired(CBase* Source);
	
	// from MEngineNotifier
	void ReportError(MEngineNotifier::TErrorType aErrorType, TInt aErrorCode);
	void ResponseReceived(const TDesC8& aBuffer);
	void CanWrite();
	
	CList<MBBLocalNotifier*>* iObservers;
	
	//from MsocketObserver
	
	void success(CBase* source);
	
	void error(CBase* source, TInt code, const TDesC& reason);
	void info(CBase* source, const TDesC& msg);
	
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

	virtual RSocket& GetSocket(RSocketServ& aServer);
	virtual void AcceptFinished(TInt aError);

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

	HBufC8		*iTransferBuffer;
	TBool		iDisconnected;
	MBBLocalProtocolOwner& iOwner;

	friend class CBBLocalProtocol;
	friend class auto_ptr<CBBLocalProtocolImpl>;
};


const TInt CBBLocalProtocolImpl::KTimeOut = 120; // 120 seconds time-out
const TInt CBBLocalProtocolImpl::KMaxIdentificationRetry = 1;

#ifndef __WINS__
const TInt CBBLocalProtocolImpl::KTimeBeforeRetry = 120; //120 seconds before retry
#else
const TInt CBBLocalProtocolImpl::KTimeBeforeRetry = 10; //120 seconds before retry
#endif

const TInt CBBLocalProtocolImpl::KDefaultPortNumber = 5222;


CBBLocalProtocol::CBBLocalProtocol() { }

EXPORT_C CBBLocalProtocol* CBBLocalProtocol::NewL(MApp_context& aContext, MBBLocalProtocolOwner& aOwner)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocol"), _CL("NewL"));

	auto_ptr<CBBLocalProtocolImpl> ret(new (ELeave) CBBLocalProtocolImpl(aContext, aOwner));
	ret->ConstructL();
	return ret.release();
}

void CBBLocalProtocolImpl::expired(CBase* Source) { }

CBBLocalProtocolImpl::CBBLocalProtocolImpl(MApp_context& Context, MBBLocalProtocolOwner& aOwner) : MContextBase(Context),
	iTimeBeforeRetry(KTimeBeforeRetry), iOwner(aOwner)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("CBBLocalProtocolImpl"));
	
	Reporting().DebugLog(_L("Socket engine created."));
}


CBBLocalProtocolImpl::~CBBLocalProtocolImpl()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("~CBBLocalProtocolImpl"));

	NotifyDisconnected();

	iOwner.ProtocolDeleted(this);

	delete iCC;

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

	delete iTransferBuffer;
}


void CBBLocalProtocolImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("ConstructL"));

	iCC=CCnvCharacterSetConverter::NewL();
	iCC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());;

	iTransferBuffer=HBufC8::NewL(2048);

	User::LeaveIfError(iSocketServ.Connect());
	
	iXmlBuf=CXmlBuf::NewL(256);

	ChangeStatus(ENotConnected);
	
	// Start a timer
	iTimer = CTimeOut::NewL(*this);
	iIdentTimer = CTimeOut::NewL(*this);
	
	// Create socket read and write active objects
	iSocketsReader = CSocketsReader::NewL(*this, iSocket, AppContext());
	iSocketsWriter = CSocketsWriter::NewL(*this, iSocket, AppContext());

	iObservers=CList<MBBLocalNotifier*>::NewL();

}

RSocket& CBBLocalProtocolImpl::GetSocket(RSocketServ& aServer)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("GetSocket"));
	User::LeaveIfError(iSocket.Open(aServer));
	return iSocket;
}

void CBBLocalProtocolImpl::Disconnect(TBool closeConnection) 
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("Disconnect"));

	ReadyToWrite(EFalse);

	if (Connected()) {
		ChangeStatus(EDisconnecting);
		iCloseConnAfterDisconnect=closeConnection;
		SendDisconnectionL();
		DisconnectSocket();
		NotifyDisconnected();
	}
}

void CBBLocalProtocolImpl::DisconnectSocket()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("DisconnectSocket"));


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

void CBBLocalProtocolImpl::WriteL(const TDesC16& aData)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("WriteL"));

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
		
		iSocketsWriter->IssueWriteL(*iTransferBuffer);
        } else {
		User::Leave(KErrNotReady);
	}
}

void CBBLocalProtocolImpl::SendXMLStreamHeaderL()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("SendXMLStreamHeaderL"));

	Reporting().DebugLog(_L("SendXMLStreamHeaderL"));

	if ( (iEngineStatus == EConnected) )
	{
		delete iStream; iStream=0;
		iStream=CStream::NewL(*this, AppContext());

		TBuf16<200> header;
		
		header.Append(_L16("<?xml version='1.0'?>\n<stream>\n"));
		
		WriteL(header);
	}
}

void CBBLocalProtocolImpl::SendDisconnectionL()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("SendDisconnectionL"));

	if ( (iEngineStatus == EConnected)  || (iEngineStatus == EDisconnecting))
	{
		TBuf16<200> msg;
		msg.Append(_L16("</stream>\n"));
		WriteL(msg);
	}
}

void CBBLocalProtocolImpl::Read()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("Read"));

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

// from CActive
void CBBLocalProtocolImpl::AcceptFinished(TInt aError)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("AcceptFinished"));

	// Active object request complete handler.
	// iEngineStatus flags what request was made, so its
	// completion can be handled appropriately
	iTimer->Reset(); // Cancel TimeOut timer before completion
	
	if (aError == KErrNone)
		// Connection completed successfully
	{
		ChangeStatus(EConnected);
		iConnectionRetry = 0;
		iLoggedIn=ETrue;
		SendXMLStreamHeaderL();
		// Read();
	} else {
		iSocket.Close();
		iSocketOpen = EFalse;
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Conn. failed"), aError);
		ChangeStatus(ENotConnected);
		delete this;
	}

}

void CBBLocalProtocolImpl::ReportError(MEngineNotifier::TErrorType aErrorType, TInt aErrorCode)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("ReportError"));

	if (iLoggedIn && aErrorType==MEngineNotifier::ETimeOutOnRead) {
		// we don't mind, we assume the timeouts on various
		// things are set on a higher level
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Time Out on Read"), aErrorCode);
		iSocketsReader->Start();
		return;
	}
		
	switch (aErrorType)
        {
	case MEngineNotifier::ESocketEngineTimeOut:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Timed out"), aErrorCode);			
		break;
		
		
	case MEngineNotifier::ETimeOutOnRead:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Time Out on Read"), aErrorCode);
		break;
	case MEngineNotifier::EStreamError:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: StreamError"), aErrorCode);
		// report to context_log ?
		break;
		
	case MEngineNotifier::EDisconnected:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Disconnected"), aErrorCode);
		break;
		
	case MEngineNotifier::EGeneralReadError:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Read Error"), aErrorCode);
		break;
		
	case MEngineNotifier::ETimeOutOnWrite:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Time Out on write"), aErrorCode);
		break;
		
	case MEngineNotifier::EGeneralWriteError:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Write Error"), aErrorCode);
		break;
		
	case MEngineNotifier::EXmlParseError:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: XML Parse Error"), aErrorCode);
		break;
	case MEngineNotifier::ENetworkConnectError:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Connect to net error"), aErrorCode);
		break;
	case MEngineNotifier::ESocketConnectError:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Connect to server error"), aErrorCode);
		break;
	case MEngineNotifier::EUnknownError:
		Reporting().UserErrorLog(_L("CBBLocalProtocolImpl: Unknown error"), aErrorCode);
		break;
	default:
		User::Panic(KPanicSocketsEngine, ESocketsBadStatus);
		break;
        }	
}

void CBBLocalProtocolImpl::ResponseReceived(const TDesC8& aBuffer)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("ResponseReceived"));

	Reporting().DebugLog(_L("Receive:"));

	iStream->ParseL(aBuffer);
}

void CBBLocalProtocolImpl::ChangeStatus(TSocketsEngineState aNewStatus)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("ChangeStatus"));


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

TBool CBBLocalProtocolImpl::Connected() const
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("Connected"));

	return (iEngineStatus == EConnected);
}

void CBBLocalProtocolImpl::ReportError(TInt aError, TInt aOrigError, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("ReportError"));

	for (CList<MBBLocalNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->Error(aError, aOrigError, aDescr);
	}
}

void CBBLocalProtocolImpl::NotifyDisconnected()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("NotifyDisconnected"));

	if (iDisconnected) return;
	iDisconnected=ETrue;
	if (! iObservers ) return;
	for (CList<MBBLocalNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->Disconnected();
	}
}

void CBBLocalProtocolImpl::Acked(TUint aId)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("Acked"));

	if (aId==0) {
		iLoggedIn=ETrue;
		iIdentTimer->Reset();
		iIdAttempt=0;
		ReadyToWrite(ETrue);
	} else {
		for (CList<MBBLocalNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
			n->Item->Acked(aId);
		}
	}
}

void CBBLocalProtocolImpl::IncomingTuple(const CBBTuple* aTuple)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("IncomingTuple"));
	for (CList<MBBLocalNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->IncomingTuple(aTuple);
	}
}


void CBBLocalProtocolImpl::ReadyToWrite(TBool aReady)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("ReadyToWrite"));

	for (CList<MBBLocalNotifier*>::Node* n=iObservers->iFirst; n; n=n->Next) {
		n->Item->ReadyToWrite(aReady);
	}
}

void CBBLocalProtocolImpl::IncomingData(const MBBData* aData, TBool aErrors)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("IncomingData"));

	const TBBAck* ack=bb_cast<TBBAck>(aData);
	if (ack) {
		Acked(ack->iId());
	}
	const CBBTuple* tuple=bb_cast<CBBTuple>(aData);
	if (tuple) {
		IncomingTuple(tuple);
	}
}

void CBBLocalProtocolImpl::StreamOpened()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("StreamOpened"));

	//SendIdentificationL();
}

void CBBLocalProtocolImpl::StreamClosed()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("StreamClosed"));

	if (Connected() || (iEngineStatus==EDisconnecting) ) {
		DisconnectSocket();
		if (iCloseConnAfterDisconnect)
		{
			iConnectionOpener->CloseConnection();
		}
		NotifyDisconnected();
	}
}

void CBBLocalProtocolImpl::StreamError(TInt aError, const TDesC& aDescr)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("StreamError"));

	ReportError(MEngineNotifier::EStreamError, aError);
}

void CBBLocalProtocolImpl::AddObserverL(MBBLocalNotifier* aObserver)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("AddObserverL"));

	iObservers->AppendL(aObserver);
}

void CBBLocalProtocolImpl::SendXmlPacketL(const TDesC& aPacket)
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("SendXmlPacketL"));

	if (Connected()) WriteL(aPacket);
}

void CBBLocalProtocolImpl::CanWrite()
{
	CALLSTACKITEM_N(_CL("CBBLocalProtocolImpl"), _CL("CanWrite"));

	if (Connected() && iLoggedIn) ReadyToWrite(ETrue);
}
