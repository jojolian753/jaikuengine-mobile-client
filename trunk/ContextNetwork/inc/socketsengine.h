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

#ifndef __SOCKETSENGINE_H__
#define __SOCKETSENGINE_H__

#include "ver.h"
#include <in_sock.h>
#include "timeout.h"
#include "EngineNotifier.h"
#include "Sockets.hrh"
#include "xmlbuf.h"

#include "connectioninit.h"
#include "app_context.h"

#include "socketsreader.h"
#include "socketswriter.h"

#ifdef __S60V2__
#include <zlib.h>
#else
// From Symbian FAQ
#include <ezlib.h>
#endif


class MEngineObserver
{
public:
	enum TEngineStatus {
		ESocketConnected=0,
		ESocketDisconnected=1,
		ESocketUnreachable=2,
		EConnectionNotAllowed=3
	};

	virtual void NotifyEngineStatus(TInt st, TInt aError) = 0;
	virtual void NotifyNewData(const TDesC8& aBuffer) = 0;
	virtual void NotifyCanWrite() = 0;
};

class CSocketsEngine : public CCheckedActive, public MTimeOut, public MEngineNotifier, 
	public MSocketObserver, public MContextBase
#ifdef __S60V3__
	, public MConnectionErrorCallback
#endif
    {
public: 
	IMPORT_C static CSocketsEngine* NewL(MEngineObserver& aObserver, MApp_context& Context,
		TBool aDoCompress=EFalse, const TDesC& aConnectioName=KNullDesC);
	IMPORT_C ~CSocketsEngine();
	
	IMPORT_C void ConnectL(const TDesC& host, const TInt& port, TUint32 iap);
	IMPORT_C void Disconnect(TBool closeConnection); // 

	IMPORT_C void WriteL(const TDesC16& aData);
	IMPORT_C void WriteL(const TDesC8& aData);
	IMPORT_C void Read();
	IMPORT_C void StopRead();
	IMPORT_C void NoReadTimeout();
	IMPORT_C void ReadTimeout();
	
	IMPORT_C TUint32 AccessPoint();

public: 
	void ReportError(MEngineNotifier::TErrorType aErrorType, TInt aErrorCode);
	virtual void ResponseReceived(const TDesC8& aBuffer);
	void CanWrite();

public: 
	void success(CBase* source);
	void error(CBase* source, TInt code, const TDesC& reason);
	void info(CBase* source, const TDesC& msg);

private: 
	MEngineObserver& iObserver;
	IMPORT_C void expired(CBase* Source);
	void DisconnectSocket();

protected: 
	void DoCancel();
	void CheckedRunL();
	TInt CheckedRunError(TInt aError);

private: 
	CSocketsEngine(MEngineObserver& aObserver, MApp_context& Context,
		TBool aDoCompress);
	void ConstructL(const TDesC& aConnectionName);
	void OpenConnectionAndConnectL();
	void ConnectL();
	void ConnectL(TUint32 aAddr);
	enum TSocketsEngineState 
		{ ENotConnected, EConnecting, EConnected, ELookingUp, 
		EDisconnecting, EWaitingForRetry };
	void ChangeStatus(TSocketsEngineState aNewStatus);

#ifdef __S60V2__
	virtual void ConnectionDisconnected(TInt aError);
#endif
private:
	static const TInt KTimeOut;
	static const TInt KDefaultPortNumber;
	static const TInt KTimeBeforeRetry;
	static const TInt KMaxConnectionRetry;

	TSocketsEngineState         iEngineStatus;

	RSocket                     iSocket;
	CSocketsReader*             iSocketsReader;
	CSocketsWriter*             iSocketsWriter;
	RHostResolver               iResolver;

	TNameEntry                  iNameEntry;
	TNameRecord                 iNameRecord;

	CTimeOut		    *iTimer, *iCounterTimer;

	TInetAddr                   iAddress;
	TInt                        iPort;
	TBuf16<KMaxServerNameLength>  iServerName;

	CConnectionOpener *	iConnectionOpener;
	TUint32			iAccessPoint;
	RSocketServ		iSocketServ;

	TBool iSocketOpen;
	TBool iResolverOpen;

	TInt iConnectionRetry;
	TInt iTimeBeforeRetry;
    
#ifdef __S60V2__
	RConnection		iConnection;
	class CConnectionErrorListener* iConnectionErrorListener;
#endif

	TBool		iDoCompress;
	void InitZStreamsL();
	virtual void UpdateByteCount(TInt aNewReceived, TInt aNewSent);

	z_stream	in_z_stream, out_z_stream;
	TBool		in_z_open, out_z_open;

	HBufC8		*in_z_buf, *out_z_buf;

	void AppendToOutZBufL(const TDesC8& aData);
	Bytef		out_z_buf_tmp[1024];
	HBufC8		*iTransferBuffer;
	class CCnvCharacterSetConverter *iCC;
	TBuf<50>	iConnectionName;
	TInt		iRecvBytes, iSentBytes, iMtu, iOverhead, iPrevRecv, iPrevSent;
};

#endif
