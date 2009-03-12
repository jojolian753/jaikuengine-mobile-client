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

#include "socketslistener.h"

#include "break.h"
#include "app_context.h"
#include "in_sock.h"
#include "symbian_auto_ptr.h"

class CSocketListenerImpl: public CSocketListener, public MContextBase {
	CSocketListenerImpl(MSocketAcceptorFactory& aAcceptorFactory);
	void ConstructL(TUint aListenPort);
	~CSocketListenerImpl();

	void CheckedRunL();
	void DoCancel();
	TInt CheckedRunError();

	void RestartL();

	TInt	iErrorCount;

	enum TState { EIdle, EAccepting };
	TState iState;

	RSocketServ iServ;
	RSocket	iSocket;
	MSocketAcceptor* iCurrentAcceptor;
	MSocketAcceptorFactory &iFactory;
	TInetAddr               iAddress;
	TBool			iSocketIsOpen;

	friend class CSocketListener;
	friend class auto_ptr<CSocketListenerImpl>;
};

_LIT(KSocketListener, "CSocketListener");

CSocketListener::CSocketListener() : CCheckedActive(CActive::EPriorityLow, KSocketListener) { }

CSocketListenerImpl::CSocketListenerImpl(MSocketAcceptorFactory& aAcceptorFactory) : 
	iFactory(aAcceptorFactory) { }

EXPORT_C CSocketListener* CSocketListener::NewL(MSocketAcceptorFactory& aAcceptorFactory,
	TUint aListenPort)
{
	auto_ptr<CSocketListenerImpl> ret(new (ELeave) CSocketListenerImpl(aAcceptorFactory));
	ret->ConstructL(aListenPort);
	return ret.release();
}

void CSocketListenerImpl::ConstructL(TUint aListenPort)
{
	User::LeaveIfError(iServ.Connect());
	iAddress.SetAddress(INET_ADDR(127,0,0,1));
	iAddress.SetPort(aListenPort);
	CActiveScheduler::Add(this);
	RestartL();
}

CSocketListenerImpl::~CSocketListenerImpl()
{
	Cancel();
	iSocket.Close();
	iServ.Close();
	delete iCurrentAcceptor;
}

void CSocketListenerImpl::CheckedRunL()
{
	switch(iState) {
	case EAccepting:
		iCurrentAcceptor->AcceptFinished(iStatus.Int());
		iCurrentAcceptor=0;
		iCurrentAcceptor=iFactory.CreateAcceptorL();
		iSocket.Accept(iCurrentAcceptor->GetSocket(iServ), iStatus);
		iState=EAccepting;
		SetActive();
		break;
	}
}

void CSocketListenerImpl::DoCancel()
{
	switch(iState) {
	case EAccepting:
		iSocket.CancelAccept();
		delete iCurrentAcceptor; iCurrentAcceptor=0;
	}
}

TInt CSocketListenerImpl::CheckedRunError()
{
	TInt err;
	CC_TRAP(err, RestartL());
	return err;
}

void CSocketListenerImpl::RestartL()
{
	Cancel();
	iErrorCount++;
	if (iSocketIsOpen) {
		iSocket.Close();
		iSocketIsOpen=EFalse;
	}
	User::LeaveIfError(iSocket.Open(iServ, KAfInet, KSockStream, KProtocolInetTcp));
	iSocketIsOpen=ETrue;
	//iSocket.SetLocalPort(iAddress.Port());
	User::LeaveIfError(iSocket.Bind(iAddress));
	User::LeaveIfError(iSocket.Listen(2));
	delete iCurrentAcceptor; iCurrentAcceptor=0;
	iCurrentAcceptor=iFactory.CreateAcceptorL();
	iSocket.Accept(iCurrentAcceptor->GetSocket(iServ), iStatus);
	iState=EAccepting;
	SetActive();
}
