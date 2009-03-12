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

#include "autoap.h"
#include "concretedata.h"
#include "symbian_auto_ptr.h"
#include <charconv.h>
#include <f32file.h>
#include "break.h"
#include <cdbpreftable.h>
#include "timeout.h"
#include "ntpconnection.h"

#ifndef __S60V3__
#include "cc_shareddata.h"
#endif

#include <cdbcols.h>
#include <commdb.h>

class MIniFileCb {
public:
	/* return ETrue to stop processing */
	virtual TBool SectionBeginL(const TDesC& aSection) = 0;
	virtual TBool KeyL(const TDesC& aName, const TDesC& aValue) = 0;
	virtual TBool UnknownLineL(const TDesC& aLine) = 0;
};

_LIT(KLine, "line");

class CIniFile : public CBase, public MContextBase {
public:
	/* assumes UTF-8 */
	static CIniFile* NewL(MIniFileCb& aCb, const TDesC& aFileName) {
		auto_ptr<CIniFile> ret(new (ELeave) CIniFile(aCb));
		ret->ConstructL(aFileName);
		return ret.release();
	}
	TBool HandleLine(const TDesC& aLine) {
		if (aLine.Length()==0) return EFalse;
		if (aLine[0]==';') return EFalse;
		if (aLine.Length()==1) {
			return iCb.UnknownLineL(aLine);
		}

		if (aLine[0]=='[') {
			if (aLine[aLine.Length()-1]==']') {
				return iCb.SectionBeginL(aLine.Mid(1, aLine.Length()-2));
			} else {
				return iCb.UnknownLineL(aLine);
			}
		}
		TInt equals_pos=aLine.LocateF('=');
		if (equals_pos==KErrNotFound) {
			return iCb.UnknownLineL(aLine);
		} else {
			TInt name_end=equals_pos-1;
			while ( TChar(aLine[name_end]).IsSpace() && name_end>0) 
				name_end --;
			if (name_end==0) {
				return iCb.UnknownLineL(aLine);
			}
			TInt value_begin=equals_pos+1;
			if (value_begin==aLine.Length()) {
				return iCb.KeyL(aLine.Left(name_end+1), KNullDesC);
			}
			while ( TChar(aLine[value_begin]).IsSpace() && value_begin<aLine.Length()-1) 
				value_begin--;
			return iCb.KeyL( aLine.Left(name_end+1), 
				aLine.Right(aLine.Length()-value_begin) );
		}
	}

	void ReadL() {
		TEntry e;
		User::LeaveIfError(Fs().Entry(iFileName, e));
		iPos=0;
		auto_ptr<CCnvCharacterSetConverter> cc(CCnvCharacterSetConverter::NewL());
		CCnvCharacterSetConverter::TAvailability av=
			cc->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());
		TInt state;
		if (av==CCnvCharacterSetConverter::ENotAvailable) 
			User::Leave(KErrNotSupported);
		do {
			User::LeaveIfError(Fs().ReadFileSection(iFileName,
				iPos, iBuf8, iBuf8.MaxLength()));
			iPos+=iBuf8.Length();
			cc->ConvertToUnicode(iBuf, iBuf8, state);
			for (int i=0; i<iBuf.Length(); i++) {
				if (iBuf[i]=='\n' || iBuf[i]=='\r') {
					iLine->Value().Trim();
					iLine->SyncPtr();
					if (HandleLine( (*iLine)() )) return;
					iLine->Zero();
				} else {
					iLine->Append(iBuf.Mid(i,1));
				}
			}
		} while(iBuf8.Length()>0);
	}
private:
	CBBString* iLine;
	TFileName iFileName;
	TBuf8<128> iBuf8;
	TBuf<256+10> iBuf;
	TInt iPos;
	MIniFileCb& iCb;
	CIniFile(MIniFileCb& aCb) : iCb(aCb) { }
	~CIniFile() {
		delete iLine;
	}
	void ConstructL(const TDesC& aFileName) {
		iLine=CBBString::NewL(KLine);
		iFileName=aFileName;
	}
};

_LIT(KOperaSection, "epoc");
_LIT(KOperaKey, "Connection ID");

class TOperaIniFile : public MIniFileCb {
public:
	TBool iInSection;
	TInt iAp;
	virtual TBool SectionBeginL(const TDesC& aSection) {
#ifdef __WINS__
		RDebug::Print(aSection);
#endif
		if (iInSection) return ETrue;
		if (aSection.CompareF(KOperaSection())==0) iInSection=ETrue;
		return EFalse;
	}
	virtual TBool KeyL(const TDesC& aName, const TDesC& aValue) {
#ifdef __WINS__
		TBuf<101> msg;
		msg=aName.Left(50); msg.Append(_L("=")); msg.Append(aValue.Left(50));
		RDebug::Print(msg);
#endif
		if (aName.CompareF(KOperaKey())==0) {
			TLex l(aValue);
			User::LeaveIfError(l.Val(iAp));
			return ETrue;
		}
		return EFalse;
	}
	virtual TBool UnknownLineL(const TDesC& ) {
		return EFalse;
	}
	TOperaIniFile() : iInSection(0), iAp(-1) { }
};


void GetApNameL(CCommsDatabase& db, TUint& aAp, TDes& aName)
{
	aName.Zero(); aName=_L("[not found]");
	CCommsDbTableView* iViewP=db.OpenTableLC(TPtrC(IAP));
	CleanupStack::Pop();
	auto_ptr<CCommsDbTableView> iView(iViewP);
	TUint32 id;
	TInt err=iView->GotoFirstRecord();
	while( err == KErrNone ) {
		iView->ReadUintL(TPtrC(COMMDB_ID), id);
		if (id==aAp) {
#if defined(__S60V3__) && !defined(__WINS__)
			iView->ReadTextL(TPtrC(IAP_BEARER_TYPE), aName);
			if (aName.FindF(_L("LAN")) != KErrNotFound) {
				aName.Zero();
				aAp=-1;
				return;
			}
#endif

			iView->ReadTextL(TPtrC(COMMDB_NAME), aName);
			return;
		}
		err=iView->GotoNextRecord();
	}
	aAp=-1;
}

EXPORT_C void GetApNameL(TUint& aAp, TDes& aName)
{
	auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
	GetApNameL(*db, aAp, aName);
}

EXPORT_C TInt CAutoAccessPoint::GetOperaApL(TDes& aName)
{
	_LIT(KOperaIniFile, "\\system\\data\\opera\\opera.ini");

	auto_ptr<HBufC> filename(HBufC::NewL(KOperaIniFile().Length()+2));
	TInt err[2];
	TUint ap=-1;
	filename->Des()=_L("c:");
	for (int i=0; i<2 && ap==-1; i++) {
		TOperaIniFile oi;
		filename->Des().Append(KOperaIniFile);
		auto_ptr<CIniFile> f(CIniFile::NewL(oi, *filename));
		CC_TRAPIGNORE(err[i], KErrNotFound, f->ReadL());
		ap=oi.iAp;
		filename->Des()=_L("e:");
	}
	if (ap!=-1) {
		GetApNameL(ap, aName);
	} else {
		if (err[0]==KErrNone) err[0]=err[1];
		User::LeaveIfError(err[1]);
	}
	return ap;
}

EXPORT_C TInt CAutoAccessPoint::GetBrowserApL(TDes& aName)
{
#ifndef __S60V3__
	//FIXME3rd

	aName.Zero();
	RSharedDataClient sh;
	TInt pushed=0;
	CleanupClosePushL(sh); pushed++;
	TInt wapap;
	{
		User::LeaveIfError( sh.Connect() );
		TInt err=sh.Assign( TUid::Uid( 0x10008d39 ) );
		if (err==KErrNotFound) return -1;
		User::LeaveIfError(err);
		err=sh.GetInt(_L("DefaultAP"), wapap);
		if (err==KErrNotFound) return -1;
		User::LeaveIfError(err);
	}
	CleanupStack::PopAndDestroy(pushed);

	{
		auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
		CCommsDbTableView* iViewP=db->OpenTableLC(TPtrC(WAP_IP_BEARER));
		CleanupStack::Pop();
		auto_ptr<CCommsDbTableView> iView(iViewP);
		TInt err=iView->GotoFirstRecord();
		while( err == KErrNone ) {
			TUint32 apid;
			iView->ReadUintL(TPtrC(WAP_ACCESS_POINT_ID), apid);
			if (apid==wapap) {
				TUint32 iap32;
				iView->ReadUintL(TPtrC(WAP_IAP), iap32);
				TUint iap=iap32;
				GetApNameL(*db, iap, aName);
				return iap;
			}
			err=iView->GotoNextRecord();
		}
	}
#endif
	return -1;
}

_LIT(KInternet, "internet");

EXPORT_C TInt CAutoAccessPoint::GetInternetApL(TDes& aName)
{
	aName.Zero();

	TUint32 service=-1;
	auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
	{
		CCommsDbTableView* iViewP=db->OpenTableLC(TPtrC(OUTGOING_GPRS));
		CleanupStack::Pop();
		auto_ptr<CCommsDbTableView> iView(iViewP);
		TInt err=iView->GotoFirstRecord();
		TInt nonexact_match=-1;
		while( err == KErrNone ) {
			iView->ReadUintL(TPtrC(COMMDB_ID), service);
			auto_ptr<HBufC> text(HBufC::NewL(KCommsDbSvrMaxColumnNameLength));
			TPtr ptr(text->Des());
			iView->ReadTextL(TPtrC(GPRS_APN), ptr);
			if ( (*text).CompareF(KInternet)==0) {
				break;
			}
			if ( (*text).FindF(KInternet) != KErrNotFound ) {
				nonexact_match=service;
			}
			service=-1;
			err=iView->GotoNextRecord();
		}
		if (service==-1) service=nonexact_match;
		if (service==-1) return -1;
	}
	{
	CCommsDbTableView* iViewP=db->OpenTableLC(TPtrC(IAP));
	CleanupStack::Pop();
	auto_ptr<CCommsDbTableView> iView(iViewP);
	TUint32 id=-1, this_service;
	TInt err=iView->GotoFirstRecord();
	while( err == KErrNone ) {
		iView->ReadUintL(TPtrC(COMMDB_ID), id);
		iView->ReadUintL(TPtrC(IAP_SERVICE), this_service);
		auto_ptr<HBufC> text(HBufC::NewL(KCommsDbSvrMaxColumnNameLength));
		TPtr ptr(text->Des());
		iView->ReadTextL(TPtrC(IAP_SERVICE_TYPE), ptr);
		if ( this_service==service && (*text).CompareF(TPtrC(OUTGOING_GPRS))==0) {
#if defined(__S60V3__) && !defined(__WINS__)
			iView->ReadTextL(TPtrC(IAP_BEARER_TYPE), aName);
			if (aName.FindF(_L("LAN")) != KErrNotFound) {
				aName.Zero();
				return -1;
			}
#endif

			iView->ReadTextL(TPtrC(COMMDB_NAME), aName);
			return id;
		}
		err=iView->GotoNextRecord();
	}
	return -1;
	}
}

EXPORT_C TInt CAutoAccessPoint::GetDefaultApL(TDes& aName)
{
	aName.Zero();

	TUint32 service32=-1;
	auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
	CCommsDbConnectionPrefTableView* iViewP=db->OpenConnectionPrefTableInRankOrderLC(ECommDbConnectionDirectionOutgoing);
	CleanupStack::Pop();
	auto_ptr<CCommsDbConnectionPrefTableView> iView(iViewP);
	if (iView->GotoFirstRecord()==KErrNone) {
		CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref pref;
		iView->ReadConnectionPreferenceL(pref);
		service32=pref.iBearer.iIapId;
	}
	TUint service=service32;
	if (service==-1) return -1;
	GetApNameL(*db, service, aName);
	return service;
}

class CAccessPointTesterImpl : public CAccessPointTester, public MTimeOut, public MNTPObserver {
public:
	CTimeOut* iTimer;
	TInt iError;
	MAccessPointTestResult& iCb;
	CNTPConnection *iNTPConnection;
	TInt iTries;
	TInt iAp;

	CAccessPointTesterImpl(MAccessPointTestResult& aCb) : iCb(aCb) { }
	~CAccessPointTesterImpl() {
		delete iTimer;
		delete iNTPConnection;
	}

	virtual void NTPInfo(const TDesC& ) { }
	virtual void NTPError(const TDesC& , TInt aErrorCode) {
		iTimer->Cancel();
		if (iTries>1) {
			iCb.Done(iAp, aErrorCode);
		} else {
			iTries++;
			CC_TRAP(iError, RunSyncL());
			if (iError!=KErrNone) {
				iCb.Done(iAp, iError);
			}
		}
	}
	virtual void NTPSuccess(TTime ) {
		iTimer->Cancel();
		iCb.Done(iAp, KErrNone);
	}

	void expired(CBase*) {
		if (iError!=KErrNone) iCb.Done(iAp, iError);
		else iCb.Done(iAp, KErrTimedOut);
	}
	void ConstructL() {
		iTimer=CTimeOut::NewL(*this);
	}
	void RunSyncL() {
		Cancel();
		iNTPConnection=CNTPConnection::NewL(*this);
		iNTPConnection->Sync(iAp);
		iTimer->Wait(20);
	}
	void TestApL(TInt aAp) {
		iTries=0;
		iAp=aAp;
		RunSyncL();
	}
	void TestAp(TInt aAp) {
		CC_TRAP(iError, TestApL(aAp));
		if (iError!=KErrNone) {
			iTimer->Wait(0);
		}
	}
	void Cancel() {
		iTimer->Cancel();
		delete iNTPConnection; iNTPConnection=0;
	}
};

EXPORT_C CAccessPointTester* CAccessPointTester::NewL(MAccessPointTestResult& aCallback)
{
	auto_ptr<CAccessPointTesterImpl> ret(new (ELeave) CAccessPointTesterImpl(aCallback));
	ret->ConstructL();
	return ret.release();
}

class CAccessPointListerImpl : public CAccessPointLister {
public:
	CCommsDatabase* iDb;
	CCommsDbTableView* iView;
	TBool iFirst;
	void ConstructL() {
		iDb=CCommsDatabase::NewL(EDatabaseTypeIAP);
		iView=iDb->OpenTableLC(TPtrC(IAP));
		iFirst=ETrue;
		CleanupStack::Pop();
	}
	~CAccessPointListerImpl() {
		delete iView;
		delete iDb;
	}
	TBool NextRecordL(TInt& aId, TDes& aName) {
again:
		aName.Zero();
		aId=-1;
		TInt err;
		if (iFirst) err=iView->GotoFirstRecord();
		else err=iView->GotoNextRecord();
		iFirst=EFalse;
		if (err!=KErrNone) return EFalse;
#if defined(__S60V3__) && !defined(__WINS__)
		iView->ReadTextL(TPtrC(IAP_BEARER_TYPE), aName);
		if (aName.FindF(_L("LAN")) != KErrNotFound) {
			goto again;
		}
#endif

		TUint32 id;
		iView->ReadUintL(TPtrC(COMMDB_ID), id);
		aId=id;
		iView->ReadTextL(TPtrC(COMMDB_NAME), aName);
		return ETrue;
	}
};

EXPORT_C CAccessPointLister* CAccessPointLister::NewL()
{
	auto_ptr<CAccessPointListerImpl> ret (new (ELeave) CAccessPointListerImpl);
	ret->ConstructL();
	return ret.release();
}
