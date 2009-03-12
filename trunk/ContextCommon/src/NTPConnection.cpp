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

#include "NTPConnection.h"
#include <eikenv.h>
#include <es_sock.h>
#include <in_sock.h>
#include "connectioninit.h"

class CNTPConnectionImpl : public CNTPConnection, public MSocketObserver {
private:
	virtual ~CNTPConnectionImpl();
	
	virtual void Sync(TInt aAP);
	virtual void SetHost( TUint32 anAddr, TUint aPort );
	
	CNTPConnectionImpl(MNTPObserver& aObserver);
	void ConstructL();
	
	static TInt TimeOutCallBack(TAny* aPtr);
	void StartTimeoutTimer();
	void StopTimeoutTimer();
	
	void RunL();
	void DoCancel();
	
	void success(CBase* source);
	void error(CBase* source, TInt code, const TDesC& reason);
	void info(CBase* source, const TDesC& msg);

	void DoSync();

	CPeriodic *iTimeOutTimer;
	RSocketServ iSocketServ;
	RSocket 	iSocket; TBool iSocketOpen;
	TPtr8 dataPtr;;
	
#ifdef __S60V2__
	RConnection		iConnection;
#endif
	CConnectionOpener*	iOpener;

	TUint8 iReply[1024];
	TPtr8 iReplyPtr;
	TInetAddr iReplyAddr;

	enum TState { EIdle, EOpening, ESync1, ESync2 };

	TState iState;

	TInetAddr NTPHost;
	
	MNTPObserver& iObserver;
	
	friend class CNTPConnection;
};

EXPORT_C CNTPConnection* CNTPConnection::NewL(MNTPObserver& aObserver) 
{
	CALLSTACKITEM_N(_CL("CNTPConnection"), _CL("NewL"));
	CNTPConnectionImpl* self=new (ELeave) CNTPConnectionImpl(aObserver); 
	CleanupStack::PushL(self);
	self->ConstructL(); 
	CleanupStack::Pop();
	return self;
};

CNTPConnection::CNTPConnection() : CActive( EPriorityNormal ) { }

CNTPConnectionImpl::CNTPConnectionImpl(MNTPObserver& aObserver) : 
	iReplyPtr( iReply, 1024, 1024 ),
	iObserver(aObserver), dataPtr(0, 0, 0)  { }

void CNTPConnectionImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("ConstructL"));
	
	TInt ret=iSocketServ.Connect();
	if (ret!=KErrNone)
	{
		info(this, _L("NTP Connection: Connecting to SocketServ failed."));
		return;
	}
	
	iTimeOutTimer=CPeriodic::NewL(EPriorityHigh);
	CActiveScheduler::Add( this );
	
	//	SetHost( INET_ADDR(131,188,3,220), 123 ); // default
	SetHost( INET_ADDR(130,149,17,8), 123 ); // default
	
}

EXPORT_C CNTPConnection::~CNTPConnection() { }

CNTPConnectionImpl::~CNTPConnectionImpl()
{
#ifndef __WINS__
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("~CNTPConnectionImpl"));
#endif
	Cancel();
	if (iSocketOpen) iSocket.Close();
	delete iOpener; iOpener=0;
	iSocketServ.Close();
	StopTimeoutTimer();
	delete iTimeOutTimer;
}


void CNTPConnectionImpl::SetHost( TUint32 anAddr, TUint aPort )
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("SetHost"));
	NTPHost.SetAddress( anAddr );
	NTPHost.SetPort( aPort );
}


TInt CNTPConnectionImpl::TimeOutCallBack(TAny* aPtr)
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("TimeOutCallBack"));
	CNTPConnectionImpl* that = (CNTPConnectionImpl*) aPtr;
	that->StopTimeoutTimer();
	that->iSocket.CancelAll();
	
	return 0;	
}

void CNTPConnectionImpl::StartTimeoutTimer()
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("StartTimeoutTimer"));
	if( !iTimeOutTimer->IsActive() )
		iTimeOutTimer->Start(2000*1000, 10000*1000, TCallBack(TimeOutCallBack,this));
}

void CNTPConnectionImpl::StopTimeoutTimer()
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("StopTimeoutTimer"));
	if( iTimeOutTimer->IsActive() )
		iTimeOutTimer->Cancel();
}

void CNTPConnectionImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("DoCancel"));
	iSocket.CancelAll();
	User::WaitForRequest(iStatus);
	iSocket.Shutdown(RSocket::EImmediate, iStatus);
}

void CNTPConnectionImpl::success(CBase* source)
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("success"));
	iObserver.NTPInfo(_L("Connection opened"));
	DoSync();
}

void CNTPConnectionImpl::error(CBase* source, TInt code, const TDesC& reason)
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("error"));
	Cancel();
	if (iSocketOpen) { iSocket.Close(); iSocketOpen=EFalse; }
	StopTimeoutTimer();
	iState=EIdle;
	delete iOpener; iOpener=0;
	iObserver.NTPError(reason, code);
}

void CNTPConnectionImpl::info(CBase* source, const TDesC& msg)
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("info"));
	iObserver.NTPInfo(msg);
}

void CNTPConnectionImpl::Sync(TInt aAP)
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("Sync"));
	if ( iState != EIdle ) User::Leave(KErrInUse);

	delete iOpener; iOpener=0;
#ifndef __S60V2__
	iOpener=CConnectionOpener::NewL(*this, iSocketServ);
#else
	iOpener=CConnectionOpener::NewL(*this, iSocketServ, iConnection, EFalse);
#endif
	iOpener->MakeConnectionL(aAP);
}
	
void CNTPConnectionImpl::DoSync()
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("DoSync"));
	/*
	TIME1970 = 2208988800L      # Thanks to F.Lundh
	client = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
	data = '\x1b' + 47 * '\0'
	client.sendto( data, ( sys.argv[1], 123 ))
	data, address = client.recvfrom( 1024 )
	if data:
	print 'Response received from:', address
	t = struct.unpack( '!12I', data )[10]
	t -= TIME1970
	print '\tTime=%s' % time.ctime(t)
	*/
	//+The timestamp in the NTP packet is a 32bit integer which represents
	//+the number of seconds after 00:00 01/01/1900. This 32bit number will
	// Open channel to Socket Server
	
	// Open a UDP socket
#ifndef __S60V2__
	int ret=iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp) ;
#else
	int ret=iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection) ;
#endif
	if (ret!=KErrNone) {
		error(this, ret, _L("NTP Connection: Opening socket failed."));		
		return;
	} else {
		iSocketOpen=ETrue;
	}
	
	TUint8 data[48];
	for( int i = 0 ; i < 48 ; i++) data[i] = 0;
	/*    http://www.faqs.org/rfcs/rfc2030.html
	LI       Value     Meaning
	-------------------------------------------------------
	00       0         no warning
	01       1         last minute has 61 seconds
	10       2         last minute has 59 seconds)
	11       3         alarm condition (clock not synchronized)
	
	  
	    Version Number (VN): This is a three-bit integer indicating the
	    NTP/SNTP version number. The version number is 3 for Version 3 (IPv4
	    only) and 4 for Version 4 (IPv4, IPv6 and OSI). If necessary to
	    distinguish between IPv4, IPv6 and OSI, the encapsulating context
	    must be inspected.
	    
	      
		Mode: This is a three-bit integer indicating the mode, with values
		defined as follows:
		Mode     Meaning
		------------------------------------
		0        reserved
		1        symmetric active
		2        symmetric passive
		3        client
		4        server
		5        broadcast
		6        reserved for NTP control message
		7        reserved for private use
	*/
	data[0] = 0x0b; // LLVVVMMM Leap=0, V=3, M=3  00011011
	
	dataPtr.Set(data, 48, 48);
	
	iSocket.SendTo( dataPtr , NTPHost , 0, iStatus );
	SetActive();
	iState = ESync1;
	
	return;
}

void CNTPConnectionImpl::RunL()
{
	CALLSTACKITEM_N(_CL("CNTPConnectionImpl"), _CL("RunL"));
	StopTimeoutTimer();
	
	if( iStatus != KErrNone )
	{
		error(this, iStatus.Int(), _L("NTP Connection: Server timed out") );
		return;
	}
	else
	{
		if( iState == ESync1 )
		{
			for( int i = 0 ; i < 1024 ; i++) iReply[i] = 0;
			iSocket.RecvFrom( iReplyPtr, iReplyAddr, 0,  iStatus );
			SetActive();
			StartTimeoutTimer();
			iState = ESync2;
		}
		else
		{
			iState = EIdle;
			//	                           1                   2                   3
			//	       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                          Root Delay                           |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                       Root Dispersion                         |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                     Reference Identifier                      |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                                                               |
			//	      |                   Reference Timestamp (64)                    |
			//	      |                                                               |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                                                               |
			//	      |                   Originate Timestamp (64)                    |
			//	      |                                                               |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                                                               |
			//	      |                    Receive Timestamp (64)                     |
			//	      |                                                               |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                                                               |
			//	      |                    Transmit Timestamp (64)                    |
			//	      |                                                               |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                 Key Identifier (optional) (32)                |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	      |                                                               |
			//	      |                                                               |
			//	      |                 Message Digest (optional) (128)               |
			//	      |                                                               |
			//	      |                                                               |
			//	      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//	
			//	
			//	   Reference Timestamp: This is the time at which the local clock was
			//	   last set or corrected, in 64-bit timestamp format.
			//	   Originate Timestamp: This is the time at which the request departed
			//	   the client for the server, in 64-bit timestamp format.
			//	   Receive Timestamp: This is the time at which the request arrived at
			//	   the server, in 64-bit timestamp format.
			//	   Transmit Timestamp: This is the time at which the reply departed the
			//	   server for the client, in 64-bit timestamp format.
			//
			TUint seconds   = 0;
			TUint fractions = 0;
			seconds   = (iReply[40]<<24) | (iReply[41]<<16) | (iReply[42]<<8) | (iReply[43]<<0);
			fractions = (iReply[44]<<24) | (iReply[45]<<16) | (iReply[46]<<8) | (iReply[47]<<0);
			
			TInt64 microseconds = seconds;
			microseconds *= (1000 * 1000);
			TInt64 foo = fractions;
			foo *= 1000*1000;
			foo /= 0xffffffff;
			microseconds += foo;
			
			TTime currentTime;
			currentTime.HomeTime();
			
			TTime correctTime( _L("19000000:") );
			correctTime += TTimeIntervalMicroSeconds( microseconds );
			
			TLocale locale;
			TTimeIntervalSeconds universalTimeOffset(locale.UniversalTimeOffset());
			correctTime+=universalTimeOffset;
			if( locale.QueryHomeHasDaylightSavingOn() )
			{
				correctTime += TTimeIntervalHours(1);
			}
			
			_LIT(KTimeString,"%1/%2/%3 %-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
			TBuf<32> timeString1;
			TBuf<32> timeString2;
			currentTime.FormatL(timeString1,KTimeString);
			correctTime.FormatL(timeString2,KTimeString);
			
			TBuf<1100> buf;
			buf.Copy(_L( "Current time: " ) );
			buf.Append( timeString1 );
			buf.Append( _L("\n") );
			
			buf.Append(_L( "Correct time: " ) );
			buf.Append( timeString2 );
			buf.Append( _L("\n") );
			
			info(this, buf );
			//User::SetHomeTime( correctTime );
			iObserver.NTPSuccess( correctTime );
		}
	}
}


