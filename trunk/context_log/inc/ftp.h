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

#ifndef DBTEST_HTTP_H_INCLUDED
#define DBTEST_HTTP_H_INCLUDED 1

#include "discover.h"
#include <in_sock.h>
#include <es_sock.h>
#ifndef __S60V3__
#include <AgentClient.h>
#ifndef __S60V2FP3__
#include <capcodec.h>
#endif
#endif
#include "expect.h"
#include "app_context.h"
#include "timeout.h"
#include "connectioninit.h"

#define USE_WAP 0

const TInt CL_SOCKET_ERRBASE = -1000;

class CSocketBase : public CCheckedActive, public MTimeOut {
public:
	virtual void Connect(const TSockAddr& aNetAddr, int timeout, int flags=0);
	virtual void Connect(const TDesC& Host, int port, int timeout, int flags=0);
	virtual void Listen(TSockAddr& aNetAddr, int timeout);
	void LocalName(TSockAddr& anAddr);
	void RemoteName(TSockAddr& anAddr);
	virtual void Close();
	virtual void CloseImmediate();
	void CancelRead();

	enum TFlags { ENone=0x0, ESSL=0x1 };

protected:
	virtual void expired(CBase*);
	void DoClose(bool internal);

#ifndef __S60V2__
	CSocketBase(MSocketObserver& obs, RSocketServ& Serv);
#else
	CSocketBase(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection);
#endif
	void ConstructL(int buf_size);
	void CloseResolver();


	virtual ~CSocketBase();


	enum state { IDLE, LOOKUP, CONNECTING, LISTENING, CONNECTED, SENDING, RECEIVING, CLOSING };
	state current_state;
	virtual void CheckedRunL();
	virtual void DoCancel();

	virtual bool DoCheckedRunL();

	RSocket	socket;
	RSocket listensocket;
	RSocketServ& sockserv;
#ifdef __S60V2__
	RConnection& iConnection;
#endif
	TSockAddr addr;
	TNameEntry iLookupResult;
	HBufC	*iHost;
	TInt	iPort;
	RHostResolver iResolver;
	bool	iErrorWait;

	TPtr8	readbuf;
	TUint8*	readbuf_data;
	TSockXfrLength	read_len;

	MSocketObserver& observer;

	CTimeOut*	timer;
	bool		close_is_internal;
	int	iSocketBaseRetries;

	int	iFlags;

	// you are not allowed to close the R* objects if they
	// have not been opened, so we have to keep tabs
	bool		iListenIsOpen;
	bool		iSocketIsOpen;
	bool		iResolverOpen;

	void	read();
	void	inconsistent(const TDesC& reason);
	void	reset();
};

class CFileSocket : public CSocketBase {
public:
#ifndef __S60V2__
	static CFileSocket* NewL(MSocketObserver& obs, RSocketServ& Serv);
#else
	static CFileSocket* NewL(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection);
#endif

	TInt SendFile(const TDesC& filename); // returns size of file
	void ReceiveFile(const TDesC& filename);

	virtual ~CFileSocket();
private:
	virtual bool DoCheckedRunL();

#ifndef __S60V2__
	CFileSocket(MSocketObserver& obs, RSocketServ& Serv);
#else
	CFileSocket(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection);
#endif
	void ConstructL();

	RFs	fs;
	RFile	file;
	RFile	debug;

	enum dir { SEND, RECEIVE };
	dir	transfer_dir;
	bool	transfer_file();
	bool	write_to_file();
};

class CProtoSocket : public CSocketBase {
public:
#ifndef __S60V2__
	static CProtoSocket* NewL(MSocketObserver& obs, RSocketServ& Serv);
#else
	static CProtoSocket* NewL(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection);
#endif

	void Send(const TDesC8& str, int timeout);
	void Expect(const TDesC8& str, const TDesC8& err_str, bool whole_line, int timeout);
	void Expect(const TDesC8& str, const TDesC8& err_str, bool whole_line, int timeout, TDes8 *into, bool read_into);
	void SendExpect(const TDesC8& cmd_str, const TDesC8& exp_str, const TDesC8& err_str, bool whole_line, int timeout,
		TDes8 *into=0, bool read_into=false);
	// Expect handles characters, '^', '?', [xy] and [^xy]

	~CProtoSocket();
private:
	virtual bool DoCheckedRunL();

#ifndef __S60V2__
	CProtoSocket(MSocketObserver& obs, RSocketServ& Serv);
#else
	CProtoSocket(MSocketObserver& obs, RSocketServ& Serv, RConnection& Connection);
#endif
	void ConstructL();

	void Send(const TDesC8& str, int timeout, bool and_expect);
	void SetUpExpect(const TDesC8& str, const TDesC8& err_str, 
		       bool whole_line, int timeout, TDes8* into, bool read_into);

	HBufC8*	sendbuf;
	bool	expect_line;
	TDes8	*read_ret;
	bool	return_read;
	bool	send_and_expect;
	int	wait_time;

	CExpect *expect, *err;

	enum expect_match { MATCH, NOMATCH, ERRORMATCH };
	expect_match	handle_input(); // true if finished reading

	RFile	debug;
};


#endif
