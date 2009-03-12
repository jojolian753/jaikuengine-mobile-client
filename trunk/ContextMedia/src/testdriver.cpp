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
#include "..\..\BlackBoard\src\testdriver_bbdata_base.cpp"

#include "app_context.h"
#include "db.h"

#include "cm_post.h"
#include "cm_storage.h"
#include <basched.h>
#include "fetch.h"
#include "downloader.h"
#include "converter.h"

_LIT(name, "name");
void storage(CApp_context* c)
{
	CALLSTACKITEM_N(_CL("RDbTable"), _CL("ELessEqual"));

	auto_ptr<CBBDataFactory> factory(CBBDataFactory::NewL());

	auto_ptr<CDb> db(CDb::NewL(*c, _L("contextmedia"),  EFileRead|EFileWrite|EFileShareAny));
	auto_ptr<CPostStorage> ps(CPostStorage::NewL(*c, *db, factory.get()));

	refcounted_ptr<CCMPost> p1(CCMPost::NewL(factory.get()));
	p1->iParentId()=CPostStorage::RootId();
	p1->iPostId()=1;
	p1->iBodyText->Append(_L("testbody"));

	auto_ptr<CFbsBitmap> icon(0);
	ps->AddLocalL(p1.get(), icon);

	refcounted_ptr<CCMPost> ps1(bb_cast<CCMPost>(p1->CloneL(name)));

	ps->FirstL(CPostStorage::RootId(), CPostStorage::EByDate, CPostStorage::EAscending, EFalse);

	refcounted_ptr<CCMPost> p2(ps->GetCurrentL(0));

	TEST_EQUALS(*p1, *p2, _L("comp1"));
	TEST_EQUALS(EFalse, ps->NextL(), _L("comp2"));

	TBool found=ps->FirstL(CPostStorage::RootId(), CPostStorage::EByDate, CPostStorage::EAscending, ETrue);
	TEST_EQUALS(EFalse, found, _L("comp3"));


	refcounted_ptr<CCMPost> p11(bb_cast<CCMPost>(p1->CloneL(name)));
	p11->iPostId()=2;
	p11->iUnreadCounter()=1;
	p11->iSender.iName()=_L("testauth");
	p11->iMediaFileName()=_L("testfile");
	p11->iMediaUrl()=_L("testurl");
	ps->AddLocalL(p11.get(), icon);

	TEST_NOT_EQUALS(*ps1, *p11, _L("comp4.0"));
	found=ps->FirstL(CPostStorage::RootId(), CPostStorage::EByDate, CPostStorage::EAscending, ETrue);
	TEST_EQUALS(found, ETrue, _L("comp4"));
	if (found) {
		refcounted_ptr<CCMPost> p2(ps->GetCurrentL(0));

		TEST_EQUALS(*p11, *p2, _L("comp5"));
	}
	TEST_EQUALS(EFalse, ps->NextL(), _L("comp5.1"));
	{
		refcounted_ptr<CCMPost> p2(ps->GetByIndexL(0, ps1->iLocalDatabaseId()));
		TEST_EQUALS(*ps1, *p2, _L("comp6"));
	}

	refcounted_ptr<CCMPost> p12(bb_cast<CCMPost>(p1->CloneL(name)));
	p12->iParentId()=1;
	found=ps->FirstL(2, CPostStorage::EByDate, CPostStorage::EAscending, EFalse);
	TEST_EQUALS(found, EFalse, _L("comp7"));
	found=ps->FirstL(1, CPostStorage::EByDate, CPostStorage::EAscending, EFalse);
	TEST_EQUALS(found, EFalse, _L("comp7"));
	ps->AddLocalL(p12.get(), icon);

	found=ps->FirstL(1, CPostStorage::EByDate, CPostStorage::EAscending, EFalse);
	TEST_EQUALS(found, ETrue, _L("comp7"));
	if (found) {
		refcounted_ptr<CCMPost> p2(ps->GetCurrentL(0));
		TEST_EQUALS(*p12, *p2, _L("comp7.1"));
	}

	TInt count=0;
	found=ps->FirstL(CPostStorage::RootId(), CPostStorage::EByDate, CPostStorage::EAscending, EFalse);
	while(found) {
		count++;
		found=ps->NextL();
	}
	TEST_EQUALS(count, 2, _L("iter1"));
}

class RunStorage : public MRunnable2
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("RootId"));

	CApp_context* c;
public:
	RunStorage() { c=0; }
	void run() {
		if (c) { delete c; c=0; }
		c=CApp_context::NewL(true, _L("BlackBoardServer"));
		c->Fs().Delete(_L("contextmedia.db"));
		storage(c);
	}
	void stop() { delete c; c=0;}
};

class TPostReceiver : public MPostReceiver {
public:
	TInt	iCount;
	TInt	iMissingCount, iCrucialCount, iMainCount;
	TInt	iErrors; TInt iError;
	TBool	iStopped;
	TBool	iStopOnErrors;
	TPostReceiver(TBool StopOnErrors=ETrue) : 
		iCount(0), iMissingCount(0), iCrucialCount(0), iMainCount(0),
		iErrors(0), iError(0), iStopped(EFalse), iStopOnErrors(StopOnErrors) { }
	virtual void NewPost(CCMPost* aPost, TBool aErrors) {
		++iCount;
		if (aPost->HasCrucialFields()) {
			++iCrucialCount;
			if (aPost->HasMainFields()) {
				++iMainCount;
			}
		} else {
			++iMissingCount;
		}
	}
	virtual void Error(TInt aCode, const TDesC& aMsg) {
		iError=aCode;
		if (iStopOnErrors) {
			if (!iStopped) CActiveScheduler::Stop();
			iStopped=ETrue;
			return;
		}

		if (0) {
			TBuf<200> msg=_L("err: ");
			msg.AppendNum(aCode); msg.Append(_L(" - "));
			msg.Append(aMsg.Left(170));
			output->Write(msg);
		}
		++iErrors;
	}
	virtual void Finished() {
		if (!iStopped) CActiveScheduler::Stop();
		iStopped=ETrue;
	}
};

void fetch(MApp_context* c, TInt test_type)
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Stop"));

	if (test_type!=1)
	{
		auto_ptr< CList<CFetchPosts::TFetchItem> > items(CList<CFetchPosts::TFetchItem>::NewL());
		items->AppendL(CFetchPosts::TFetchItem(104, 0));
		TPostReceiver r; r.iStopOnErrors=EFalse;
		auto_ptr<CFetchPosts> f(CFetchPosts::NewL(*c, r));

		f->FetchL(1, _L("http://aware.uiah.fi/contextxml/debug.php"), items.get());
		CActiveScheduler::Start();
		TEST_EQUALS(r.iCount, 2, _L("count3.1"));
		TEST_EQUALS(r.iMainCount, 2, _L("count3.2"));
		TEST_EQUALS(r.iErrors, 0, _L("errcount1"));
	}

	if (test_type!=1)
	{
		auto_ptr< CList<CFetchPosts::TFetchItem> > items(CList<CFetchPosts::TFetchItem>::NewL());
		items->AppendL(CFetchPosts::TFetchItem(104, 0));
		TPostReceiver r; r.iStopOnErrors=EFalse;
		auto_ptr<CFetchPosts> f(CFetchPosts::NewL(*c, r));

		f->FetchL(1, _L("http://db.cs.helsinki.fi/~mraento/cgi-bin/getvc.pl"), items.get());
		CActiveScheduler::Start();
		TEST_EQUALS(r.iCount, 1, _L("count3.1"));
		TEST_EQUALS(r.iMainCount, 0, _L("count3.2"));
		TEST_EQUALS(r.iErrors, 1, _L("errcount1"));
	}

	{
		auto_ptr< CList<CFetchPosts::TFetchItem> > items(CList<CFetchPosts::TFetchItem>::NewL());
		items->AppendL(CFetchPosts::TFetchItem(102, 0));
		TPostReceiver r;
		auto_ptr<CFetchPosts> f(CFetchPosts::NewL(*c, r));

		f->FetchL(1, _L("http://db.cs.helsinki.fi/~mraento/cgi-bin/getvc.pl"), items.get());
		CActiveScheduler::Start();
		User::LeaveIfError(r.iError);
		TEST_EQUALS(r.iCount, 1, _L("count1"));
		TEST_EQUALS(r.iErrors, 0, _L("errcount1"));
	}

	if (test_type!=1)
	{
		auto_ptr< CList<CFetchPosts::TFetchItem> > items(CList<CFetchPosts::TFetchItem>::NewL());
		items->AppendL(CFetchPosts::TFetchItem(103, 0));
		TPostReceiver r;
		auto_ptr<CFetchPosts> f(CFetchPosts::NewL(*c, r));

		f->FetchL(1, _L("http://db.cs.helsinki.fi/~mraento/cgi-bin/getvc.pl"), items.get());
		CActiveScheduler::Start();
		User::LeaveIfError(r.iError);
		TEST_EQUALS(r.iCount, 3, _L("count2.1"));
		TEST_EQUALS(r.iMissingCount, 1, _L("count2.2"));
		TEST_EQUALS(r.iCrucialCount, 2, _L("count2.3"));
		TEST_EQUALS(r.iMainCount, 1, _L("count2.4"));
		TEST_EQUALS(r.iErrors, 0, _L("errcount2"));
	}
}

class RunFetch : public MRunnable2
{
	CALLSTACKITEM_N(_CL("User"), _CL("LeaveIfError"));

	CApp_context* c;
	CActiveScheduler* s;
	TInt iTestType;
public:
	RunFetch(TInt test_type=0) : iTestType(test_type) { c=0; s=0; }
	void run() {
		if (c) { delete c; c=0; }
		if (s) { delete s; s=0; }
		c=CApp_context::NewL(true, _L("ContextMediaTest"));
		if (CActiveScheduler::Current() != 0) {
			 CActiveScheduler::Install(0);
		}
		s=new (ELeave) CBaActiveScheduler;
		CActiveScheduler::Install(s);
		fetch(c, iTestType);
	}
	void stop() { delete c; c=0; CActiveScheduler::Install(0); delete s; s=0; }
};

class TDownloadObserver : public MDownloadObserver {
public:
	TInt	iExpect;
	TInt	iCount;
	TInt	iErrors, iSuccess; TInt iError;
	TBool	iStopped, iStopOnErrors;

	TDownloadObserver(TInt aExpected, TBool StopOnErrors=ETrue) : 
		iExpect(aExpected),
		iCount(0), iErrors(0), iSuccess(0),
		iError(0), iStopped(EFalse), iStopOnErrors(StopOnErrors) { }

	void DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
			const TDesC& aContentType) {
		++iCount;
		++iSuccess;
		if (iCount==iExpect) {
			if (!iStopped) 
				CActiveScheduler::Stop();
			iStopped=ETrue;
			return;
		}
	}
	virtual void DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr) {
		++iCount;
		++iErrors;
		iError=aCode;
		if (iStopOnErrors || iCount==iExpect) {
			if (!iStopped) 
				CActiveScheduler::Stop();
			iStopped=ETrue;
		}
	}
	void Reset() {
		iStopped=iCount=iSuccess=iErrors=iError=0;
	}

};

void download(MApp_context* c, TInt test_type) 
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Stop"));

	c->Fs().Delete(_L("c:\\netwtest.db"));
	TDownloadObserver o(1, ETrue);
	auto_ptr<CDb> db(CDb::NewL(*c, _L("netwtest"), EFileRead|EFileWrite|EFileShareAny));
	auto_ptr<CDownloader> d(CDownloader::NewL(*c, db->Db(), _L("c:\\netwtest\\"), o));
	d->SetFixedIap(1);
	d->DownloadL( TInt64(1), _L("http://www.cs.helsinki.fi/u/mraento/index.html") );
	CActiveScheduler::Start();
	User::LeaveIfError(o.iError);
	TEST_EQUALS(o.iCount, 1, _L("dlcount1"));
	TEST_EQUALS(o.iErrors, 0, _L("dlerrcount1"));

	o.Reset();
	
	o.iExpect=2;
	d->DownloadL( TInt64(2), _L("http://db.cs.helsinki.fi/~mraento/cgi-bin/pics.pl?mika;Context") );
	d->DownloadL( TInt64(3), _L("http://db.cs.helsinki.fi/~mraento/pics/mika/Image(196).jpg") );
	CActiveScheduler::Start();
	User::LeaveIfError(o.iError);
	TEST_EQUALS(o.iCount, 2, _L("dlcount2"));
	TEST_EQUALS(o.iErrors, 0, _L("dlerrcount2"));
}

class COwnActiveScheduler : public CBaActiveScheduler {
public:
	void Error(TInt aError) const {
		User::Leave(aError);
	}
};

class RunDownload : public MRunnable2
{
	CALLSTACKITEM_N(_CL("User"), _CL("Leave"));

	CApp_context* c;
	CActiveScheduler* s;
	TInt iTestType;
public:
	RunDownload(TInt test_type=0) : iTestType(test_type) { c=0; s=0; }
	void run() {
		if (c) { delete c; c=0; }
		if (s) { delete s; s=0; }
		c=CApp_context::NewL(true, _L("ContextMediaTest"));
		if (CActiveScheduler::Current() != 0) {
			 CActiveScheduler::Install(0);
		}
		s=new (ELeave) COwnActiveScheduler;
		CActiveScheduler::Install(s);
		download(c, iTestType);
	}
	void stop() { delete c; c=0; CActiveScheduler::Install(0); delete s; s=0; }
};

void convert(MApp_context* c, TInt test_type) 
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Install"));

	c->Fs().Delete(_L("c:\\netwtest.db"));
	TDownloadObserver o(1, ETrue);
	auto_ptr<CDb> db(CDb::NewL(*c, _L("netwtest"), EFileRead|EFileWrite|EFileShareAny));
	auto_ptr<CConvertingDownloader> d(CConvertingDownloader::NewL(*c, db->Db(), _L("c:\\netwtest\\"), o));
	d->SetFixedIap(1);
	d->DownloadL( TInt64(1), _L("http://db.cs.helsinki.fi/~mraento/img.jpg") );
	CActiveScheduler::Start();
	User::LeaveIfError(o.iError);
	TEST_EQUALS(o.iCount, 1, _L("dlccount1"));
	TEST_EQUALS(o.iErrors, 0, _L("dlcerrcount1"));

	o.Reset();
	o.iExpect=2;
	d->DownloadL( TInt64(2), _L("http://db.cs.helsinki.fi/~mraento/cgi-bin/pics.pl?mika;Context") );
	d->DownloadL( TInt64(3), _L("http://db.cs.helsinki.fi/~mraento/pics/mika/Image(196).jpg") );
	CActiveScheduler::Start();
	User::LeaveIfError(o.iError);
	TEST_EQUALS(o.iCount, 2, _L("dlccount2"));
	TEST_EQUALS(o.iErrors, 0, _L("dlcerrcount2"));
}

class RunConvert : public MRunnable2
{
	CALLSTACKITEM_N(_CL("User"), _CL("LeaveIfError"));

	CApp_context* c;
	CActiveScheduler* s;
	TInt iTestType;
public:
	RunConvert(TInt test_type=0) : iTestType(test_type) { c=0; s=0; }
	void run() {
		if (c) { delete c; c=0; }
		if (s) { delete s; s=0; }
		c=CApp_context::NewL(true, _L("ContextMediaTest"));
		if (CActiveScheduler::Current() != 0) {
			 CActiveScheduler::Install(0);
		}
		s=new (ELeave) COwnActiveScheduler;
		CActiveScheduler::Install(s);
		convert(c, iTestType);
	}
	void stop() { delete c; c=0; CActiveScheduler::Install(0); delete s; s=0; }
};

void run_oom()
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Install"));

	{
		RunStorage r;
		test_oom2(r);
	}
}

void run_oomf()
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Install"));

	{
		RunFetch r(1);
		test_oom2(r);
	}
}

void run_tests()
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Install"));

	User::__DbgMarkStart(RHeap::EUser);
	{
		output=new (ELeave) MOutput;

		RAFs fs; fs.ConnectLA();
		output->foutput.Replace(fs, _L("contextmediatest.txt"), EFileWrite);

		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
		TInt err=KErrNone;

		User::__DbgMarkStart(RHeap::EUser);
		{
			RunConvert r;
			CC_TRAP(err, r.run());
			TEST_EQUALS(err, KErrNone, _L("run convert"));
			r.stop();
		}
		User::__DbgMarkEnd(RHeap::EUser,0);

		User::__DbgMarkStart(RHeap::EUser);
		{
			RunDownload r;
			CC_TRAP(err, r.run());
			TEST_EQUALS(err, KErrNone, _L("run download"));
			r.stop();
		}
		User::__DbgMarkEnd(RHeap::EUser,0);
		/*

		{
			RunDownload r(1);
			test_oom2(r, 200, 121);
		}
		*/
		/*
		if (not_ok==0) {
			run_oomf();
		}

		User::__DbgMarkStart(RHeap::EUser);
		{
			RunFetch r;
			CC_TRAP(err, r.run());
			TEST_EQUALS(err, KErrNone, _L("run all"));
			r.stop();
		}
		User::__DbgMarkEnd(RHeap::EUser,0);
		*/

		/*
		User::__DbgMarkStart(RHeap::EUser);
		{
			RunStorage r;
			CC_TRAP(err, r.run());
			TEST_EQUALS(err, KErrNone, _L("run all"));
			r.stop();
		}
		User::__DbgMarkEnd(RHeap::EUser,0);

		if (not_ok==0) {
			run_oom();
		}
		*/

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

#ifdef __WINS__

TInt do_main(TAny* )
{
	CALLSTACKITEM_N(_CL("User"), _CL("__DbgMarkEnd"));

	E32Main();
	return 0;
}

EXPORT_C TInt InitEmulator()
{
	CALLSTACKITEM_N(_CL("User"), _CL("__DbgMarkEnd"));

	RThread t; 
	t.Create(_L("contextmediatest_main"), &do_main, 8192, 4096, 1024*1024, 0);
	TRequestStatus s;
	t.Logon(s);

        t.Resume();


	User::WaitForRequest(s);
	t.Close();

	return 0;
}

int GLDEF_C E32Dll(TDllReason)
{
	CALLSTACKITEM_N(_CL("User"), _CL("WaitForRequest"));

        return(KErrNone);
}
#endif
