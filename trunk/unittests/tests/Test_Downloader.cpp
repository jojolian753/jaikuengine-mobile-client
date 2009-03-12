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

#include "Test_Downloader.h"
#include "testutils.h"
#include "testsupport.h"

#include "dummyhttp.inl"

class TCountingObserver : public MDownloadObserver {
public:
	TCountingObserver(TInt aWaitForFinishedOrErrorCount, TInt aWaitForConnChangeCount=-1) : 
		iWaitForFinishedOrErrorCount(aWaitForFinishedOrErrorCount),
		iWaitForConnChangeCount(aWaitForConnChangeCount),
		finished(0), error(0), started(0), retriable_error(0), dequeued(0),
		conn_changed(0) { }
	
	virtual void DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
		const TDesC& aContentType) {
	
		finished++;
		CheckFinished();
	}
	virtual void Dequeued(TInt64 aRequestId) {
		dequeued++;
		CheckFinished();
	}
	virtual void DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr,
		TBool aWillRetry) {
		
		if (aWillRetry) retriable_error++;
		else error++;
		CheckFinished();
	}
	virtual void DownloadStarted(TInt64 aRequestId) {
		started++;
	}
	
	void CheckFinished() {
		if ( (error+finished+dequeued) == iWaitForFinishedOrErrorCount ||
			conn_changed==iWaitForConnChangeCount)
			CActiveScheduler::Stop();
	}
	virtual void ConnectivityChanged(TBool aConnecting, TBool aOfflineMode,
		TBool aLowSignal, TBool aCallInProgress) {
		
		conn_changed++;
		CheckFinished();
	}

	TInt iWaitForFinishedOrErrorCount, iWaitForConnChangeCount;
	TInt finished, error, started, retriable_error, dequeued, conn_changed;

};


void CTest_Downloader::testDownloadNetworkOneL()
{
	TCountingObserver o(1);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC));
	d->DownloadL( MAKE_TINT64(0, 1), _L("http://jaiku.com/") );
	TestUtils::WaitForActiveSchedulerStopL(5);
	
	TS_ASSERT_EQUALS ( o.finished, 1 );
	TS_ASSERT_EQUALS ( o.started, 1 );
	TS_ASSERT_EQUALS ( o.error, 0 );
}

void CTest_Downloader::testDownloadNetworkSeveralL()
{
	TCountingObserver o(3);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC));
	d->DownloadL( MAKE_TINT64(0, 1), _L("http://farm3.static.flickr.com/2416/2069068288_8dc58d03de_m.jpg") );
	d->DownloadL( MAKE_TINT64(0, 2), _L("http://jaiku.com/") );
	d->DownloadL( MAKE_TINT64(0, 3), _L("http://jaiku.com/") );
	TestUtils::WaitForActiveSchedulerStopL(20);
	
	TS_ASSERT_EQUALS ( o.finished, 3 );
	TS_ASSERT_EQUALS ( o.started, 3 );
	TS_ASSERT_EQUALS ( o.error, 0 );
}

void CTest_Downloader::testDownloadSimpleL()
{
	TCountingObserver o(1);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	d->DownloadL( MAKE_TINT64(0, 1), _L("url") );
	TestUtils::WaitForActiveSchedulerStopL(5);
	
	TS_ASSERT_EQUALS ( o.finished, 1 );
	TS_ASSERT_EQUALS ( o.started, 1 );
	TS_ASSERT_EQUALS ( o.error, 0 );
}

void CTest_Downloader::setUp()
{
	MContextTestBase::setUp();
	{
		TFileName fn=DataDir();
		fn.Append(_L("UTDL1.db"));
		Fs().Delete(fn);
		iDb=CDb::NewL(AppContext(), _L("UTDL1"), EFileWrite, false);
	}
	iNoAvkonIconsWas=TestSupport().NoAvkonIcons();
	TestSupport().SetNoAvkonIcons(ETrue);
	User::LeaveIfError(iSocketServ.Connect());
}

void CTest_Downloader::tearDown()
{
	iSocketServ.Close();
	TestSupport().SetNoAvkonIcons(iNoAvkonIconsWas);
	delete iDb;
	MContextTestBase::tearDown();
}

void CTest_Downloader::testDownloadTwoL()
{
	TCountingObserver o(2);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	d->DownloadL( MAKE_TINT64(0, 1), _L("url1") );
	d->DownloadL( MAKE_TINT64(0, 2), _L("url2") );
	TS_ASSERT(TestUtils::WaitForActiveSchedulerStopL(15));
	
	TS_ASSERT_EQUALS ( o.finished, 2 );
	TS_ASSERT_EQUALS ( o.started, 2 );
	TS_ASSERT_EQUALS ( o.error, 0 );
}

void CTest_Downloader::testDownloadFailL()
{
	TCountingObserver o(2);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	d->DownloadL( MAKE_TINT64(0, 1), _L("url1") );
	d->DownloadL( MAKE_TINT64(0, 2), KFailUrl );
	TestUtils::WaitForActiveSchedulerStopL(300);
	
	TS_ASSERT_EQUALS ( o.finished, 1 );
	TS_ASSERT_EQUALS ( o.started, o.finished+o.error+o.retriable_error );
	TS_ASSERT_EQUALS ( o.error, 1 );
}

void CTest_Downloader::testDownloadModeSwitchL()
{
	TCountingObserver o(2);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	d->DownloadL( MAKE_TINT64(0, 1), _L("url1") );
	d->SetLevelL(1);
	d->DownloadL( MAKE_TINT64(0, 2), _L("url2") );
	
	TInt dl=d->IsDownloadable(MAKE_TINT64(0, 1));
	TS_ASSERT_EQUALS ( dl, (TInt)CDownloader::ENotQueued );
	
	TestUtils::WaitForActiveSchedulerStopL(10);
	
	TS_ASSERT_EQUALS ( o.finished, 1 );
	TS_ASSERT_EQUALS ( o.dequeued, 1 );
	TS_ASSERT_EQUALS ( o.started, o.finished+o.error+o.retriable_error );
	TS_ASSERT_EQUALS ( o.error, 0 );
}

void CTest_Downloader::testDownloadOfflineL()
{
	TCountingObserver o(2);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	d->DownloadL( MAKE_TINT64(0, 1), _L("url1") );
	d->SetLevelL(1);
	d->DownloadL( MAKE_TINT64(0, 2), _L("url2") );
	d->SetDownloadLimitLevelL(1);
	
	TInt dl=d->IsDownloadable(MAKE_TINT64(0, 1));
	TS_ASSERT_EQUALS ( dl, (TInt)CDownloader::ENotQueued );
	dl=d->IsDownloadable(MAKE_TINT64(0, 2));
	TS_ASSERT_EQUALS ( dl, (TInt)CDownloader::ENotDownloadable );

	TestUtils::WaitForActiveSchedulerStopL(3);
	TS_ASSERT_EQUALS ( o.finished, 0 );
	TS_ASSERT_EQUALS ( o.dequeued, 0 );
	TS_ASSERT_EQUALS ( o.started, 0 );
	TS_ASSERT_EQUALS ( o.error, 0 );
	
	d->SetNoDownloadLimitLevelL();
	TestUtils::WaitForActiveSchedulerStopL(10);
	TS_ASSERT_EQUALS ( o.finished, 1 );
	TS_ASSERT_EQUALS ( o.dequeued, 1 );
	TS_ASSERT_EQUALS ( o.started, o.finished+o.error+o.retriable_error );
	TS_ASSERT_EQUALS ( o.error, 0 );
}

void CTest_Downloader::testDownloadOffline2L()
{
	TCountingObserver o(2);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	d->DownloadL( MAKE_TINT64(0, 1), _L("url1") );
	d->SetLevelL(1);
	d->DownloadL( MAKE_TINT64(0, 2), _L("url2") );
	d->SetDownloadLimitLevelL(1);

	d.reset();
	d.reset(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	
	TInt dl=d->IsDownloadable(MAKE_TINT64(0, 1));
	TS_ASSERT_EQUALS ( dl, (TInt)CDownloader::ENotQueued );
	dl=d->IsDownloadable(MAKE_TINT64(0, 2));
	TS_ASSERT_EQUALS ( dl, (TInt)CDownloader::ENotDownloadable );

	d.reset();
	d.reset(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
		
	TestUtils::WaitForActiveSchedulerStopL(3);
	TS_ASSERT_EQUALS ( o.finished, 0 );
	TS_ASSERT_EQUALS ( o.dequeued, 0 );
	TS_ASSERT_EQUALS ( o.started, 0 );
	TS_ASSERT_EQUALS ( o.error, 0 );
	
	d.reset();
	d.reset(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
		
	d->SetNoDownloadLimitLevelL();
	TestUtils::WaitForActiveSchedulerStopL(10);
	TS_ASSERT_EQUALS ( o.finished, 1 );
	TS_ASSERT_EQUALS ( o.dequeued, 1 );
	TS_ASSERT_EQUALS ( o.started, o.finished+o.error+o.retriable_error );
	TS_ASSERT_EQUALS ( o.error, 0 );
}

class CCallTester : public CActive {
public:
	CConnectivityListener *iListener;
	CCallTester(CConnectivityListener *aListener) : CActive(CActive::EPriorityHigh),
		iListener(aListener) { }
	void ConstructL() {
		CActiveScheduler::Add(this);
		TRequestStatus *s=&iStatus;
		User::RequestComplete(s, KErrNone);
		SetActive();
	}
	static CCallTester* NewL(CConnectivityListener *aListener) {
		auto_ptr<CCallTester> ret(new (ELeave) CCallTester(aListener));
		ret->ConstructL();
		return ret.release();
	}
	void RunL() {
		iListener->SimulateCallStart();
	}
	void DoCancel() { }
	~CCallTester() {
		Cancel();
	}
};

void CTest_Downloader::testConnectivity1L()
{
	TCountingObserver o(3, 2);
	auto_ptr<CDownloader> d(CDownloader::NewL( AppContext(), iDb->Db(), 
		_L("c:\\unitttestdl"), o, KNullDesC, &CDummyHttp::NewL));
	d->DownloadL( MAKE_TINT64(0, 1), _L("fail") );
	
	auto_ptr<CCallTester> t(CCallTester::NewL(d->ConnectivityListener()));
	
	TestUtils::WaitForActiveSchedulerStopL(3);
	
	TS_ASSERT_EQUALS ( o.finished, 0 );
	TS_ASSERT_EQUALS ( o.dequeued, 0 );
	TS_ASSERT_EQUALS ( o.started, 1 );
	TS_ASSERT_EQUALS ( o.retriable_error, 1 );
	TS_ASSERT_EQUALS ( o.conn_changed,2  );
}

void CTest_Downloader::testConnectivity2L()
{
}
