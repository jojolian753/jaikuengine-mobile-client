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
#include "sms_socket.h"

#include <smsuaddr.h>
#include <es_sock.h>
#include <gsmubuf.h>
#include <smsustrm.h>
#include <Gsmumsg.h>

#include "timeout.h"
#include "status_notif.h"
#include "symbian_auto_ptr.h"

class CSmsSocketImpl : public CSmsSocket, public MContextBase, public MTimeOut {
private:
	void ConstructL(const TDesC& aTag);
	CSmsSocketImpl(MApp_context& Context, i_status_notif* aNotif);
	virtual ~CSmsSocketImpl();

	i_status_notif* iNotif;
	void StartL();
	void Start();
	void StartRead();
	void Release();
	CTimeOut* iTimeOut;
	void expired(CBase*);
	TBuf8<20> iTag;
	TBuf<30> iState;
	TPckgBuf<TUint> iIoctlResult;

	RSocketServ	iSocketServer; bool iSocketServerIsOpen;
	RSocket		iSocket; bool iSocketIsOpen;

	void CheckedRunL();
	TInt CheckedRunError(TInt aError);
	void DoCancel();

	enum TState { IDLE, READING, DISCARDING };
	TState	iCurrentState;

	friend class CSmsSocket;
	friend class auto_ptr<CSmsSocketImpl>;
};

CSmsSocket* CSmsSocket::NewL(MApp_context& Context,
	const TDesC& aTag, i_status_notif* aNotif)
{
	CALLSTACKITEM2_N(_CL("CSmsSocket"), _CL("NewL"), &Context);

	auto_ptr<CSmsSocketImpl> ret(new (ELeave) CSmsSocketImpl(Context, aNotif));
	ret->ConstructL(aTag);
	return ret.release();
}

CSmsSocket::CSmsSocket() : CCheckedActive(EPriorityNormal, _L("CSmsSocket"))
{
	CALLSTACKITEM_N(_CL("CSmsSocket"), _CL("CSmsSocket"));

}

CSmsSocket::~CSmsSocket()
{
	CALLSTACKITEM_N(_CL("CSmsSocket"), _CL("~CSmsSocket"));

}

void CSmsSocketImpl::ConstructL(const TDesC& aTag)
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("ConstructL"));

	CC()->ConvertFromUnicode(iTag, aTag);
	iTimeOut=CTimeOut::NewL(*this);
	CActiveScheduler::Add(this);
	Start();
}

CSmsSocketImpl::CSmsSocketImpl(MApp_context& Context, i_status_notif* aNotif) : 
MContextBase(Context), iNotif(aNotif)
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("CSmsSocketImpl"));

}

CSmsSocketImpl::~CSmsSocketImpl()
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("~CSmsSocketImpl"));

	Release();
	delete iTimeOut;
}

void CSmsSocketImpl::Release()
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("Release"));

	Cancel();
	if (iSocketServerIsOpen) iSocketServer.Close(); 
	iSocketServerIsOpen=false;
	if (iSocketIsOpen) iSocket.Close();
	iSocketIsOpen=false;
	if (iTimeOut) iTimeOut->Reset();
}

void CSmsSocketImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("DoCancel"));

	iSocket.CancelIoctl();
}

void CSmsSocketImpl::Start()
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("Start"));

	CC_TRAPD(err, StartL());
	if (err!=KErrNone) {
		TBuf<100> msg=_L("SmsSocket err: ");
		msg.AppendNum(err);
		msg.Append(_L(" at "));
		msg.Append(iState);
		iNotif->error(msg);
	} else {
		iNotif->status_change(_L("SmsSocket started"));
	}
	iTimeOut->Wait(10);
}

void CSmsSocketImpl::StartRead()
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("StartRead"));

	iIoctlResult=KSockSelectRead;
	iSocket.Ioctl( KIOctlSelect, iStatus, &iIoctlResult, KSOLSocket);
	iCurrentState=READING;
	SetActive();
}

void CSmsSocketImpl::StartL()
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("StartL"));

	iState=_L("Connecting to server");
	User::LeaveIfError(iSocketServer.Connect()); iSocketServerIsOpen=true;
	iState=_L("opening socket");
	User::LeaveIfError(iSocket.Open(iSocketServer, KSMSAddrFamily, KSockDatagram, KSMSDatagramProtocol)); iSocketIsOpen=true;
	TSmsAddr smsaddr;
	smsaddr.SetSmsAddrFamily(ESmsAddrMatchText); 
	smsaddr.SetTextMatch(iTag);
	iState=_L("binding socket");
	User::LeaveIfError(iSocket.Bind(smsaddr));
	StartRead();
	iState=_L("");
}

void CSmsSocketImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("CheckedRunL"));


	if (iStatus.Int() != KErrNone) {
		TBuf<100> msg=_L("SMS error ");
		msg.AppendNum(iStatus.Int());
		if (iCurrentState==READING) {
			msg.Append(_L(" when waiting for sms"));
		} else {
			msg.Append(_L(" when discarding sms"));
		}
		iNotif->error(msg);
		Start();
		return;
	}

	switch(iCurrentState) {
	case READING:
		{
		CSmsBuffer *buf=CSmsBuffer::NewL();
		CleanupStack::PushL(buf);
#ifdef __S60V2__
		CSmsMessage* message = CSmsMessage::NewL(Fs(), CSmsPDU::ESmsDeliver, buf);
#else
		CSmsMessage* message = CSmsMessage::NewL(CSmsPDU::ESmsDeliver, buf);
#endif
		CleanupStack::Pop(); // buf
		CleanupStack::PushL(message);

		RSmsSocketReadStream readStream(iSocket);
		CleanupClosePushL(readStream);
		message->InternalizeL(readStream);

		TBuf<255> body;
		message->Buffer().Extract(body, 0, message->Buffer().Length());
		iNotif->status_change(body);

		CleanupStack::PopAndDestroy(2); // message, readStream

		iSocket.Ioctl( KIoctlReadMessageSucceeded, iStatus, &iIoctlResult, KSolSmsProv);
		iCurrentState=DISCARDING;
		SetActive();
		break;
		}
	case DISCARDING:
		{
		StartRead();
		break;
		}
	}
}

TInt CSmsSocketImpl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("CheckedRunError"));

	TBuf<100> msg=_L("SmsSocket RunL error: ");
	msg.AppendNum(aError);
	msg.Append(_L(" at "));
	msg.Append(iState);
	iNotif->error(msg);
	Start();
	return KErrNone;
}

void CSmsSocketImpl::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CSmsSocketImpl"), _CL("expired"));

	Start();
}
