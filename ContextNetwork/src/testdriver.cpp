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
#include "bb_listener.h"
#include <basched.h>
#include "cn_http.h"

#include "..\..\BlackBoard\src\testdriver_bbdata_base.cpp"

void run_network(MApp_context* c)
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("SendNextPacket"));

	auto_ptr<CBBProtocol> proto(CBBProtocol::NewL(*c));
	auto_ptr<CBBListener> listen(CBBListener::NewL(proto.get()));
	proto->AddObserverL(listen.get());

	proto->ConnectL(1, _L("spiff"), 2000, _L("test"));

	CActiveScheduler::Start();
}

class THttpN : public MHttpObserver {
public:
	TInt	iReplyCode;
	TBuf8<128> iFileBegin;
	CActiveScheduler* iSched;
	TInt	iError;
private:
	bool	beg;
	virtual void NotifyNewHeader(const CHttpHeader &aHeader) {
		iReplyCode=aHeader.iHttpReplyCode;
		iFileBegin.Zero();
	}
	void NotifyHttpStatus(THttpStatus st, TInt aError) {
		iError=aError;
		if (st==EHttpDisconnected) {
			if (aError==KErrEof) iError=KErrNone; // expected
			iSched->Stop();
		}
	}
	virtual void NotifyNewBody(const TDesC8 &chunk) {
		if (iFileBegin.Length()==iFileBegin.MaxLength()) return;
		TInt left;
		left=iFileBegin.MaxLength()-iFileBegin.Length();
		if (left > chunk.Length()) left=chunk.Length();
		iFileBegin.Append( chunk.Left(left) );
	}
};

void network(CApp_context* c, CActiveScheduler* s) {
	THttpN n;
	n.iSched=s;

	auto_ptr<CHttp> h(CHttp::NewL(n, *c));
	//h->GetL(1, _L("http://www.cs.helsinki.fi/u/mraento/index.html"));

	auto_ptr< CPtrList<CPostPart> > l(CPtrList<CPostPart>::NewL());
	if (1) {
		auto_ptr< CBufferPart > b(CBufferPart::NewL(_L8("test"), _L("photo"), _L("text/plain")));
		l->AppendL(b.get());
		b.release();
	} else {
		auto_ptr< CBufferPart > email(CBufferPart::NewL(_L8("mika.raento@cs.helsinki.fi"), _L("email"), _L("text/plain")));
		l->AppendL(email.get());
		email.release();

		auto_ptr< CBufferPart > passw(CBufferPart::NewL(_L8("raento1"), _L("password"), _L("text/plain")));
		l->AppendL(passw.get());
		passw.release();

		auto_ptr< CFilePart > photo(CFilePart::NewL(c->Fs(),
			_L("c:\\nokia\\Images\\Shared\\20041201T170705-2.jpg"),
			_L("photo"), _L("image/jpeg")));
		photo->SetFileName(_L("testing/test.jpg"));
		l->AppendL(photo.get());
		photo.release();
	}
	h->PostL(1, _L("http://db.cs.helsinki.fi/~mraento/cgi-bin/put20.pl"), l.release());
	//h->PostL(1, _L("http://www.flickr.com/tools/uploader_go.gne"), l.release());
	s->Start();
	User::LeaveIfError(n.iError);
	TEST_EQUALS(n.iReplyCode, 200, _L("replycode1"));
	//TEST_EQUALS(n.iFileBegin.Left(6), _L8("<HTML>"), _L("replycode1"));
	TEST_EQUALS(n.iFileBegin.Left(2), _L8("OK"), _L("replycode1"));
}

class RunNetwork : public MRunnable2
{
	CApp_context* c;
	CActiveScheduler* s;
public:
	RunNetwork() { c=0; s=0; }
	void run() {
		if (c) { delete c; c=0; }
		if (s) { delete s; s=0; }
		c=CApp_context::NewL(true, _L("nw"));
		if (CActiveScheduler::Current() != 0) {
			CActiveScheduler::Install(0);
		}
		s=new (ELeave) CBaActiveScheduler;
		CActiveScheduler::Install(s);
		network(c, CActiveScheduler::Current());
	}
	void stop() { delete c; delete s;}
};




/*
int E32Main(void)
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Current"));

        CTrapCleanup* cleanupStack = CTrapCleanup::New();
        CC_TRAPD(err, run_tests());
        delete cleanupStack;
        return 0;
}
*/
void run_oom()
{
	{
		RunNetwork r;
		test_oom2(r);
	}
}

#ifdef __WINS__


TInt do_main(TAny* )
{
	E32Main();
	return 0;
}

EXPORT_C TInt InitEmulator()
{
	RThread t; 
	t.Create(_L("contextnetworktest_main"), &do_main, 8192, 4096, 1024*1024, 0);
	TRequestStatus s;
	t.Logon(s);

        t.Resume();


	User::WaitForRequest(s);
	t.Close();

	return 0;
}

int GLDEF_C E32Dll(TDllReason)
{
        return(KErrNone);
}
#endif

void run_tests()
{
	User::__DbgMarkStart(RHeap::EUser);
	{
		output=new (ELeave) MOutput;

		RAFs fs; fs.ConnectLA();
		output->foutput.Replace(fs, _L("contextnetwork.txt"), EFileWrite);

		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
		TInt err=KErrNone;

		RunNetwork r;
		CC_TRAP(err, r.run());
		TEST_EQUALS(err, KErrNone, _L("run all"));
		r.stop();

		if (0 && not_ok==0) {
			run_oom();
		}

		TBuf<30> b=_L("OK: "); b.AppendNum(ok); 
		b.Append(_L("/")); b.AppendNum(ok+not_ok); b.Append(_L("\n"));
		output->Write(b);
		output->Getch();
		delete output->cons;

		output->foutput.Close();
		delete output;
	}
	User::__DbgMarkEnd(RHeap::EUser,0);
}
