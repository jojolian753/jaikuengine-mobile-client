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

#pragma warning(disable: 4706)

#include "ver.h"
#include "ftp.h"
#include <eikenv.h>
#include <cdbcols.h>
#include <commdb.h>
#include "pointer.h"
#include "symbian_auto_ptr.h"

#ifndef __S60V2__
CSocketBase::CSocketBase(MSocketObserver& obs, RSocketServ& Serv) : 
	CCheckedActive(EPriorityNormal, _L("CSocketBase")), current_state(IDLE), sockserv(Serv), readbuf(0, 0), observer(obs) 
#else
CSocketBase::CSocketBase(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection) : 
	CCheckedActive(EPriorityNormal, _L("CSocketBase")), current_state(IDLE), sockserv(Serv), iConnection(Connection), readbuf(0, 0), observer(obs)
#endif
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("CSocketBase"));

	iFlags=0;
	iListenIsOpen=false;
	iSocketIsOpen=false;
	iResolverOpen=false;
	iErrorWait=false;
}

void CSocketBase::DoCancel()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("DoCancel"));

	if (current_state==LOOKUP) iResolver.Cancel();
	else if (current_state==LISTENING) listensocket.CancelAll();
	else socket.CancelAll();
}

CSocketBase::~CSocketBase()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("~CSocketBase"));

	Cancel();
	CloseResolver();
	socket.Close();
	delete timer;
	delete readbuf_data;
	delete iHost;
}

void CSocketBase::CancelRead()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("CancelRead"));

	if (current_state!=RECEIVING) return;
	socket.CancelAll();
	current_state=IDLE;
}

void CSocketBase::ConstructL(int buf_size)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("ConstructL"));

	readbuf_data=new (ELeave) TUint8[buf_size];
	readbuf.Set(readbuf_data, buf_size, buf_size);

	CActiveScheduler::Add(this);
	timer=CTimeOut::NewL(*this);
}

void CSocketBase::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("CheckedRunL"));

	if (iStatus==KErrCancel) return;

	DoCheckedRunL();
}

void CSocketBase::CloseResolver()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("CloseResolver"));

	if (!iResolverOpen) return;
	iResolver.Close();
	iResolverOpen=false;
}

bool CSocketBase::DoCheckedRunL()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("DoCheckedRunL"));

	if (current_state==CLOSING) {
		// don't worry about errors in close
		socket.Close();
		current_state=IDLE;
		iSocketIsOpen=false;
		if (!close_is_internal)  observer.success(this);
		return true;
	}

	if (iStatus!=KErrNone) {
		switch(current_state) {
		case CONNECTING:
		case LISTENING:
			current_state=IDLE;
			break;
		case LOOKUP:
			iResolverOpen=false;
			break;
		default:
			break;
		};
		iErrorWait=true;
		timer->Wait(1);

		return true;
	}

	switch(current_state) {
	case IDLE:
	case CONNECTED:
		inconsistent(_L("Socket internal error, CheckedRunL when idle"));
		return true;
		break;
	case LOOKUP:
		{
		current_state=IDLE;
		observer.info(this, _L("Lookup returned"));
		TInetAddr a1(iLookupResult().iAddr);
		TInetAddr a2(a1.Address(), iPort);
		CloseResolver();

		Connect(a2, 0, iFlags);
		return true;
		}
		break;
	case CONNECTING:
		timer->Reset();
		current_state=CONNECTED;
		observer.success(this);
		return true;
		break;
	case LISTENING:
		{
		timer->Reset();
		current_state=CONNECTED;
		observer.success(this);
		return true;
		}
		break;
	default:
		return false;
		break;
	}

	return false;
}

void CSocketBase::LocalName(TSockAddr& anAddr)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("LocalName"));

	socket.LocalName(anAddr);
}

void CSocketBase::RemoteName(TSockAddr& anAddr)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("RemoteName"));

	socket.RemoteName(anAddr);
}

void CSocketBase::CloseImmediate()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("CloseImmediate"));
	Cancel();

	TRequestStatus s;
	CloseResolver();
	if (iListenIsOpen) {
		listensocket.Shutdown(RSocket::EImmediate, s);
		User::WaitForRequest(s);
		listensocket.Close();
		iListenIsOpen=false;
	}
	if (iSocketIsOpen) {
		socket.Shutdown(RSocket::EImmediate, s);
		User::WaitForRequest(s);
		socket.Close();
		iSocketIsOpen=false;
	}
	current_state=IDLE;
}

void CSocketBase::Listen(TSockAddr& aNetAddr, int timeout)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("Listen"));

	if (iListenIsOpen) {
		TRequestStatus s;
		listensocket.Shutdown(RSocket::EImmediate, s);
		User::WaitForRequest(s);
		listensocket.Close();
		iListenIsOpen=false;
	}

	timer->Wait(timeout);

	if (current_state!=IDLE) {
		User::Leave(5);
	}
#ifndef __S60V2__
	User::LeaveIfError(listensocket.Open(sockserv, KAfInet, KSockStream, KUndefinedProtocol));
	User::LeaveIfError(socket.Open(sockserv));
#else
	User::LeaveIfError(listensocket.Open(sockserv, KAfInet, KSockStream, KUndefinedProtocol, iConnection));
	User::LeaveIfError(socket.Open(sockserv, KAfInet, KSockStream, KUndefinedProtocol, iConnection));
#endif
	iListenIsOpen=true;

	addr=aNetAddr;
	User::LeaveIfError(listensocket.Bind(addr));
	User::LeaveIfError(listensocket.Listen(2));
	listensocket.LocalName(addr);
	TInetAddr ia(addr);
	aNetAddr=ia;


	listensocket.Accept(socket, iStatus);

	current_state=LISTENING;

	SetActive();
}

#ifndef __S60V2__
CProtoSocket::CProtoSocket(MSocketObserver& obs, RSocketServ& Serv) : CSocketBase(obs, Serv)
#else
CProtoSocket::CProtoSocket(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection) : CSocketBase(obs, Serv, Connection)
#endif
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("CProtoSocket"));

}

void CProtoSocket::ConstructL()
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("ConstructL"));

	CSocketBase::ConstructL(1);

	sendbuf=HBufC8::NewMaxL(128);

	User::LeaveIfError(debug.Replace(CEikonEnv::Static()->FsSession(), _L("C:\\socket.txt"), EFileWrite));

	expect=CExpect::NewL();
	err=CExpect::NewL();
}

#ifndef __S60V2__
CProtoSocket* CProtoSocket::NewL(MSocketObserver& obs, RSocketServ& Serv)
#else
CProtoSocket* CProtoSocket::NewL(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection)
#endif
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("NewL"));

#ifndef __S60V2__
	auto_ptr<CProtoSocket> ret(new (ELeave) CProtoSocket(obs, Serv));
#else
	auto_ptr<CProtoSocket> ret(new (ELeave) CProtoSocket(obs, Serv, Connection));
#endif

	ret->ConstructL();
	return ret.release();
}

CProtoSocket::~CProtoSocket()
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("~CProtoSocket"));

	Cancel();
	socket.Close();
	delete sendbuf;
	delete expect;
	delete err;
	debug.Close();
}

void CSocketBase::Connect(const TSockAddr& aNetAddr, int timeout, int flags)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("Connect"));

	if (timeout) timer->Wait(timeout);

	if (current_state!=IDLE) {
		User::Leave(6);
		return;
	}
	iFlags=flags;
#ifndef __S60V2__
	User::LeaveIfError(socket.Open(sockserv, KAfInet, KSockStream, KProtocolInetTcp ));
#else
	User::LeaveIfError(socket.Open(sockserv, KAfInet, KSockStream, KProtocolInetTcp, iConnection ));
#endif

	if (iFlags & ESSL ) {
		User::LeaveIfError(socket.SetOpt( KSoSecureSocket, KSolInetSSL ));
	}
	iSocketIsOpen=true;
	addr=aNetAddr;
	socket.Connect(addr, iStatus);
	current_state=CONNECTING;

	SetActive();
}

void CSocketBase::Connect(const TDesC& Host, int port, int timeout, int flags)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("Connect"));

	iPort=port;
	iSocketBaseRetries=0;
	iFlags=flags;

	if (timeout) timer->Wait(timeout);
	if (current_state!=IDLE) {
		User::Leave(6);
		return;
	}
	CloseResolver();

#ifndef __S60V2__
	User::LeaveIfError(iResolver.Open(sockserv, KAfInet, KProtocolInetUdp));
#else
	User::LeaveIfError(iResolver.Open(sockserv, KAfInet, KProtocolInetUdp, iConnection));
#endif

	iResolverOpen=true;
	delete iHost; iHost=0; iHost=Host.AllocL();
	iResolver.GetByName(*iHost, iLookupResult, iStatus);
	current_state=LOOKUP;

	TBuf<100> msg;
	msg.Format(_L("Looking up %S"), &Host);
	observer.info(this, msg);

	SetActive();
}

void CSocketBase::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("expired"));

	if (iErrorWait) {
		iErrorWait=false;
		TBuf<50> msg;
		msg.Format(_L("Socket error: %d at state %d"), iStatus, current_state);
		observer.error(this, iStatus.Int(), msg);
		return;
	}

	if (current_state==CLOSING) {
		socket.Shutdown(RSocket::EImmediate, iStatus);
		SetActive();
		return;
	}
	Cancel();
	observer.error(this, 2, _L("Timed out"));
}

bool CProtoSocket::DoCheckedRunL()
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("DoCheckedRunL"));

	if (current_state==CONNECTING) {
		expect->Reset();
		err->Reset();
	}

	if (CSocketBase::DoCheckedRunL()) return true;

	expect_match m;

	switch(current_state) {
	case SENDING:
		debug.Write(*sendbuf);
		current_state=CONNECTED;
		timer->Reset();
		if (!send_and_expect) {
			observer.success(this);
		} else {
			timer->Wait(wait_time);
			read();
		}
		break;
	case RECEIVING:
		m=handle_input();
		switch(m) {
		case MATCH:
			current_state=CONNECTED;
			timer->Reset();
			observer.success(this);
			break;
		case ERRORMATCH:
			current_state=CONNECTED;
			timer->Reset();
			observer.error(this, 1, _L("Error string received"));
			break;
		case NOMATCH:
			read();
			break;
		}
		break;
	default:
		observer.error(this, 1, _L("inconsistent state"));
		break;
	}
	return true;
}

void CSocketBase::read()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("read"));

	socket.RecvOneOrMore(readbuf, 0, iStatus, read_len);
	current_state=RECEIVING;
	SetActive();
}

CProtoSocket::expect_match CProtoSocket::handle_input()
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("expect_match"));

	debug.Write(readbuf);

	if (return_read) {
		if (read_ret->Length()==read_ret->MaxLength()) {
			observer.error(this, KErrNoMemory, _L("Read buffer too small"));
			return ERRORMATCH;
		}
		read_ret->Append(readbuf);
	}

	if (!expect || !err) {
		inconsistent(_L("expextstr=0 in handle_input"));
		return ERRORMATCH;
	}

	if (expect->handle_input(readbuf[0])) {
		err->SetPatternL(_L8(""));
		if (expect_line) {
			expect->SetPatternL(_L8("\n"));
			expect_line=false;
		} else {
			err->handle_input(readbuf[0]);
			return MATCH;
		}
	}

	if (err->handle_input(readbuf[0])) {
		if (expect_line) {
			err->SetPatternL(_L8("\n"));
			expect_line=false;
		} else {
			return ERRORMATCH;
		}
	}

	return NOMATCH;
}

void CProtoSocket::Expect(const TDesC8& str, const TDesC8& err_str, bool whole_line, int timeout)
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("Expect"));

	Expect(str, err_str, whole_line, timeout, 0, false);
}

void CProtoSocket::SetUpExpect(const TDesC8& str, const TDesC8& err_str, 
			       bool whole_line, int timeout, TDes8* into, bool read_into)
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("SetUpExpect"));

	if (read_into) {
		read_ret=into;
		read_ret->Zero();
		return_read=true;
	} else {
		return_read=false;
	}

	if (current_state!=CONNECTED) {
		inconsistent(_L("Expect when not connected"));
		return;
	}

	expect->SetPatternL(str);
	err->SetPatternL(err_str);

	expect_line=whole_line;
	wait_time=timeout;
}

void CProtoSocket::SendExpect(const TDesC8& cmd_str, const TDesC8& exp_str, const TDesC8& err_str, bool whole_line, int timeout,
		TDes8 *into, bool read_into)
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("SendExpect"));

	SetUpExpect(exp_str, err_str, whole_line, timeout, into, read_into);
	Send(cmd_str, timeout, true);
}

void CProtoSocket::Expect(const TDesC8& str, const TDesC8& err_str, bool whole_line, int timeout, TDes8* into, bool read_into)
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("Expect"));

	send_and_expect=false;
	SetUpExpect(str, err_str, whole_line, timeout, into, read_into);
	timer->Wait(wait_time);
	read();
}

void CSocketBase::inconsistent(const TDesC& reason)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("inconsistent"));

	observer.error(this, -1, reason);
	return;
}

void CSocketBase::DoClose(bool internal)
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("DoClose"));

	close_is_internal=internal;
	Cancel();
	current_state=CLOSING;
	socket.Shutdown(RSocket::EStopOutput, iStatus);
	SetActive();
}

void CSocketBase::Close()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("Close"));

	DoClose(false);
}

void CSocketBase::reset()
{
	CALLSTACKITEM_N(_CL("CSocketBase"), _CL("reset"));

	socket.Close();
	iSocketIsOpen=false;
	current_state=IDLE;
}

void CProtoSocket::Send(const TDesC8& str, int timeout)
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("Send"));

	Send(str, timeout, false);
}

void CProtoSocket::Send(const TDesC8& str, int timeout, bool and_expect)
{
	CALLSTACKITEM_N(_CL("CProtoSocket"), _CL("Send"));

	send_and_expect=and_expect;

	if (current_state!=CONNECTED) {
		inconsistent(_L("Sending when not connected"));
		return;
	}

	if (!sendbuf) {
		sendbuf=HBufC8::NewMax(str.Length());
	} else if (sendbuf->Size()<str.Length()) {
		sendbuf=sendbuf->ReAlloc(str.Length());
	}
	if (!sendbuf) {
		observer.error(this, KErrNoMemory, _L("Cannot allocate expect buffer"));
		return;
	}
	*sendbuf=str;
	socket.Write(*sendbuf, iStatus);
	current_state=SENDING;
	timer->Wait(timeout);

	SetActive();
}

#ifndef __S60V2__
CFileSocket::CFileSocket(MSocketObserver& obs, RSocketServ& Serv) : CSocketBase(obs, Serv)
#else
CFileSocket::CFileSocket(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection) : CSocketBase(obs, Serv, Connection)
#endif
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("CFileSocket"));

}

void CFileSocket::ConstructL()
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("ConstructL"));

	User::LeaveIfError(fs.Connect());
	CSocketBase::ConstructL(4096);
	User::LeaveIfError(debug.Replace(fs, _L("c:\\filesocket.txt"), EFileWrite));
}


#ifndef __S60V2__
CFileSocket* CFileSocket::NewL(MSocketObserver& obs, RSocketServ& Serv)
#else
CFileSocket* CFileSocket::NewL(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection)
#endif
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("NewL"));

#ifndef __S60V2__
	auto_ptr<CFileSocket> ret(new (ELeave) CFileSocket(obs, Serv));
#else
	auto_ptr<CFileSocket> ret(new (ELeave) CFileSocket(obs, Serv, Connection));
#endif
	ret->ConstructL();
	return ret.release();
}

bool CFileSocket::DoCheckedRunL()
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("DoCheckedRunL"));

	TBuf8<100> d;
	d.Format(_L8("DoCheckedRunL, state %d, iStatus %d\n"), current_state, iStatus);

	debug.Write(d);

	timer->Reset();

	if (current_state==CLOSING) {
		// don't worry about errors in close
		if (!close_is_internal) observer.success(this);
		socket.Close();
		current_state=IDLE;
		return true;
	}

	TBuf<50> msg;

	if (iStatus!=KErrNone && !(current_state==RECEIVING && iStatus==KErrEof) ) {
		msg.Format(_L("Socket error: %d at state %d"), iStatus, current_state);
		debug.Write(_L8("Socket error\n"));
		observer.error(this, iStatus.Int(), msg);
		socket.Close();
		current_state=IDLE;
		return true;
	}

	switch(current_state) {
	case IDLE:
	case CONNECTED:
		inconsistent(_L("Socket internal error, CheckedRunL when idle"));
		break;
	case CONNECTING:
		timer->Reset();
		current_state=CONNECTED;
		observer.success(this);
		break;
	case LISTENING:
		{
		timer->Reset();
		current_state=CONNECTED;
		observer.success(this);
		}
		break;
	case RECEIVING:
		readbuf.SetLength(read_len());
		if (iStatus==KErrEof) {
			if (!write_to_file()) {
				DoClose(true);
			} else {
				observer.success(this);
				DoClose(true);
			}
			file.Close();
		} else {
			transfer_file();
		}
		break;
	case SENDING:
		if (transfer_file()) {
			file.Close();
			DoClose(true);
			observer.success(this);
		}
		break;
	default:
		inconsistent(_L("Socket internal error, inconsistent state"));
		break;
	}

	return true;
}

bool CFileSocket::write_to_file()
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("write_to_file"));

	TInt ret=file.Write(readbuf);
	if (ret!=KErrNone) {
		observer.error(this, 4, _L("Error writing local file"));
		file.Close();
		return false;
	} 
	return true;
}

bool CFileSocket::transfer_file()
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("transfer_file"));

	debug.Write(_L8("transfer_file"));

	timer->Reset();
	if (transfer_dir==SEND) {
		readbuf.Zero();
		TInt ret=file.Read(readbuf);
		if (ret==KErrEof || readbuf.Size()==0) return true;
		if (ret!=KErrNone) {
			observer.error(this, 3, _L("Error reading local file"));
			file.Close();
			return false;
		}
		current_state=SENDING;
		socket.Write(readbuf, iStatus);
		timer->Wait(60);
		SetActive();
	} else {
		if (!write_to_file()) return false;
		timer->Wait(60);
		readbuf.Zero();
		read();
	}

	return false;
}

TInt CFileSocket::SendFile(const TDesC& filename)
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("SendFile"));

	if (current_state!=CONNECTED) inconsistent(_L("SendFile while not connected"));

	User::LeaveIfError(file.Open(fs, filename, EFileShareAny | EFileRead));
	TInt size;
	User::LeaveIfError(file.Size(size));

	transfer_dir=SEND;
	readbuf.Zero();
	transfer_file();
	return size;
}

void CFileSocket::ReceiveFile(const TDesC& filename)
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("ReceiveFile"));

	if (current_state!=CONNECTED) inconsistent(_L("Receive while not connected"));

	User::LeaveIfError(file.Replace(fs, filename, EFileWrite));
	transfer_dir=RECEIVE;
	readbuf.Zero();
	transfer_file();
}

CFileSocket::~CFileSocket()
{
	CALLSTACKITEM_N(_CL("CFileSocket"), _CL("~CFileSocket"));

	Cancel();
	socket.Close();
	file.Close();
	debug.Close();
	fs.Close();

}

