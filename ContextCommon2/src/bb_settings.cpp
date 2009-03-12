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
#include "bb_settings.h"

#ifdef BB_SETTINGS

#include "bberrors.h"
#include "blackboardclientsession.h"
#include <bautils.h>
#include "bbutil.h"
#include "concretedata.h"

#include "db.h"
#include <db.inl>
#include <s32mem.h>

class CBlackBoardSettingsImpl : public CBlackBoardSettings, public MContextBase {
private:
	CBlackBoardSettingsImpl(MApp_context& aContext, const MDefaultSettings& DefaultSettings,
		const TTupleName& aSettingTupleName);
	~CBlackBoardSettingsImpl();

	virtual bool GetSettingL(TInt Setting, TDes& Value);
	virtual bool GetSettingL(TInt Setting, TDes8& Value);
	virtual bool GetSettingL(TInt Setting, TInt& Value);
	virtual bool GetSettingL(TInt Setting, TTime& Value);
	virtual void WriteSettingL(TInt Setting, const TDesC& Value);
	virtual void WriteSettingL(TInt Setting, const TDesC8& Value);
	virtual void WriteSettingL(TInt Setting, const TInt& Value);
	virtual void WriteSettingL(TInt Setting, const TTime& Value);
	virtual void NotifyOnChange(TInt Setting, MSettingListener* Listener);
	virtual void CancelNotifyOnChange(TInt Setting, MSettingListener* Listener);

	void NotifyOfChange(TInt Setting);
	void ConstructL();

	void ListenForNotification();
	void ReconnectL();
	//
	void CheckedRunL();
	void DoCancel();
	TInt GetSetting(TInt Setting);
	TInt WriteSetting(TInt Setting);

	// implementation
	RBBClient	iBBClient;
	TTupleName iSettingTupleName;
	CGenericIntMap*	iListeners;
	const MDefaultSettings& iDefaultSettings;
	HBufC8*		iBuf;
	HBufC8*		iNotifyBuf; TPtr8 iNotifyBufP;
	HBufC8*		iGetPutBuf;
	TFullArgs	iFullArgs;
	MBBDataFactory* iFactory;

	TInt		iSettingInPutBuf, iSettingInNotifyBuf;

	MBBData*	MakeBBData(const TDesC8& aBuf);
	void		WriteBBData(MBBData& aData, HBufC8*& aBuf);

	friend class CBlackBoardSettings;
	friend class auto_ptr<CBlackBoardSettingsImpl>;
};

_LIT(KClassName, "CBlackBoardSettingsImpl");

CBlackBoardSettingsImpl::CBlackBoardSettingsImpl(MApp_context& aContext, const MDefaultSettings& DefaultSettings,
			const TTupleName& aSettingTupleName) : MContextBase(aContext),
			iDefaultSettings(DefaultSettings), iSettingTupleName(aSettingTupleName),
			iNotifyBufP(0, 0) { }

CBlackBoardSettings::CBlackBoardSettings() : CCheckedActive(EPriorityUserInput, KClassName) { }


EXPORT_C CBlackBoardSettings* CBlackBoardSettings::NewL(MApp_context& Context, 
	const MDefaultSettings& DefaultSettings,
	const TTupleName& aSettingTupleName)
{
	CALLSTACKITEM2_N(_CL("CBlackBoardSettings"), _CL("NewL"), &Context);

	auto_ptr<CBlackBoardSettingsImpl> ret(new (ELeave) CBlackBoardSettingsImpl(Context, 
		DefaultSettings, aSettingTupleName));
	ret->ConstructL();
	return ret.release();
}

CBlackBoardSettingsImpl::~CBlackBoardSettingsImpl()
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("~CBlackBoardSettingsImpl"));

	delete iListeners;
	Cancel();
	iBBClient.Close();
	delete iGetPutBuf;
	delete iNotifyBuf;
	delete iFactory;
}

_LIT(KSetting, "setting");

MBBData* CBlackBoardSettingsImpl::MakeBBData(const TDesC8& aBuf)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("MakeBBData"));

	RDesReadStream rs(aBuf);
	TTypeName tn=TTypeName::IdFromStreamL(rs);
	bb_auto_ptr<MBBData> data(iFactory->CreateBBDataL(tn, KSetting, iFactory));
	data->InternalizeL(rs);
	rs.Close();
	return data.release();
}

void CBlackBoardSettingsImpl::WriteBBData(MBBData& aData, HBufC8*& aBuf)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("WriteBBData"));

	TInt err;
	for(;;) {
		aBuf->Des().Zero();
		TPtr8 p(aBuf->Des());
		RDesWriteStream ws(p);
		aData.Type().ExternalizeL(ws);
		CC_TRAP(err, aData.ExternalizeL(ws));
		if (err==KErrOverflow) {
			ws.Close();
			aBuf=aBuf->ReAllocL(aBuf->Des().MaxLength()*2);
		} else {
			User::LeaveIfError(err);
			ws.CommitL();
			ws.Close();
			break;
		}
	}
}

void CBlackBoardSettingsImpl::ListenForNotification()
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("ListenForNotification"));

	iSettingInNotifyBuf=-1;
	iNotifyBufP.Set(iNotifyBuf->Des());
	iStatus=KRequestPending;
	SetActive();
	iBBClient.WaitForNotify(iFullArgs, iNotifyBufP, iStatus);
}

TInt CBlackBoardSettingsImpl::GetSetting(TInt Setting)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("GetSetting"));

	if (iSettingInNotifyBuf==Setting) {
		iBuf=iNotifyBuf;
		return KErrNone;
	}
	if (iSettingInPutBuf==Setting) {
		iBuf=iGetPutBuf;
		return KErrNone;
	}

	iSettingInPutBuf=-1;
	TRequestStatus s;
	TFullArgs a;
	iGetPutBuf->Des().Zero();
	TPtr8 p(iGetPutBuf->Des());
	TBuf<10> num; num.AppendNum(Setting);
	iBBClient.Get(iSettingTupleName, num, a, p, s);
	User::WaitForRequest(s);
	while (s.Int()==KClientBufferTooSmall) {
		iGetPutBuf->Des().Zero();
		iGetPutBuf=iGetPutBuf->ReAllocL(iGetPutBuf->Des().MaxLength()*2);
		TPtr8 p(iGetPutBuf->Des());
		iBBClient.Get(iSettingTupleName, num, a, p, s);
		User::WaitForRequest(s);
	}
	if (s.Int()==KErrNone) {
		iBuf=iGetPutBuf;
		iSettingInPutBuf=Setting;
	}

	return s.Int();
}

TInt CBlackBoardSettingsImpl::WriteSetting(TInt Setting)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("WriteSetting"));

	iSettingInPutBuf=Setting;

	TComponentName cn={{ CONTEXT_UID_CONTEXTCOMMON2 }, 1 };
	TRequestStatus s;
	TUint id;
	TBuf<10> num; num.AppendNum(Setting);
	TTime expires=Time::MaxTTime();
	iBBClient.Put(iSettingTupleName, num, cn, *iGetPutBuf, EBBPriorityNormal, ETrue, id, s, expires);
	User::WaitForRequest(s);

	if (s.Int()==KErrNone) NotifyOfChange(Setting);

	return s.Int();
}

bool CBlackBoardSettingsImpl::GetSettingL(TInt Setting, TDes& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("GetSettingL"));

	Value.Zero();

	TInt err=GetSetting(Setting);
	if (err==KErrDiskFull) return false;
	if (err==KErrNotFound) return iDefaultSettings.GetDefaultL(Setting, Value);
	User::LeaveIfError(err);

	bb_auto_ptr<MBBData> d;
	CC_TRAP(err, d.reset(MakeBBData(*iBuf)));
	if (err!=KErrNone) return false;
	d->IntoStringL(Value);
	return true;
}

bool CBlackBoardSettingsImpl::GetSettingL(TInt Setting, TDes8& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("GetSettingL"));

	Value.Zero();

	TInt err=GetSetting(Setting);
	if (err==KErrDiskFull) return false;
	if (err==KErrNotFound) return iDefaultSettings.GetDefaultL(Setting, Value);
	User::LeaveIfError(err);

	bb_auto_ptr<MBBData> d;
	CC_TRAP(err, d.reset(MakeBBData(*iBuf)));
	if (err!=KErrNone) return false;
	TBBShortString8* s8=bb_cast<TBBShortString8>(d.get());
	if (!s8) return false;

	Value=(*s8)();
	return true;
}

bool CBlackBoardSettingsImpl::GetSettingL(TInt Setting, TInt& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("GetSettingL"));

	TInt err=GetSetting(Setting);
	if (err==KErrDiskFull) return false;
	if (err==KErrNotFound) return iDefaultSettings.GetDefaultL(Setting, Value);
	User::LeaveIfError(err);

	bb_auto_ptr<MBBData> d;
	CC_TRAP(err, d.reset(MakeBBData(*iBuf)));
	if (err!=KErrNone) return false;

	const TBBInt* i=bb_cast<TBBInt>(d.get());
	if (!i) {
		const TBBBool* b=bb_cast<TBBBool>(d.get());
		if (!b)	return false;
		if ( (*b)() ) Value=1;
		else Value=0;
	} else {
		Value=(*i)();
	}
	return true;
}

bool CBlackBoardSettingsImpl::GetSettingL(TInt Setting, TTime& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("GetSettingL"));

	TInt err=GetSetting(Setting);
	if (err==KErrDiskFull) return false;
	if (err==KErrNotFound) return iDefaultSettings.GetDefaultL(Setting, Value);
	User::LeaveIfError(err);

	bb_auto_ptr<MBBData> d;
	CC_TRAP(err, d.reset(MakeBBData(*iBuf)));
	if (err!=KErrNone) return false;
	TBBTime* t=bb_cast<TBBTime>(d.get());
	if (!t) return false;

	Value=(*t)();
	return true;
}

void CBlackBoardSettingsImpl::WriteSettingL(TInt Setting, const TDesC& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("WriteSettingL"));

	iSettingInPutBuf=-1;
	TBBLongString s(KSetting);
	s()=Value;
	WriteBBData(s, iGetPutBuf);
	User::LeaveIfError(WriteSetting(Setting));
}

void CBlackBoardSettingsImpl::WriteSettingL(TInt Setting, const TDesC8& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("WriteSettingL"));

	iSettingInPutBuf=-1;
	TBBShortString8 s(KSetting);
	s()=Value;
	WriteBBData(s, iGetPutBuf);
	User::LeaveIfError(WriteSetting(Setting));
}

void CBlackBoardSettingsImpl::WriteSettingL(TInt Setting, const TInt& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("WriteSettingL"));

	iSettingInPutBuf=-1;
	TBBInt s(KSetting);
	s()=Value;
	WriteBBData(s, iGetPutBuf);

	User::LeaveIfError(WriteSetting(Setting));
}

void CBlackBoardSettingsImpl::WriteSettingL(TInt Setting, const TTime& Value)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("WriteSettingL"));

	iSettingInPutBuf=-1;
	TBBTime s(KSetting);
	s()=Value;
	WriteBBData(s, iGetPutBuf);

	User::LeaveIfError(WriteSetting(Setting));
}

void CBlackBoardSettingsImpl::NotifyOnChange(TInt Setting, MSettingListener* Listener)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("NotifyOnChange"));

	CList<MSettingListener*>* l=(CList<MSettingListener*>*)iListeners->GetData(Setting);
	if (!l) {
		auto_ptr<CList<MSettingListener*> > l2(CList<MSettingListener*>::NewL());
		iListeners->AddDataL(Setting, (void*)l2.get());
		l=l2.release();
	}
	l->AppendL(Listener);
}

void CBlackBoardSettingsImpl::NotifyOfChange(TInt Setting)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("NotifyOfChange"));

	CList<MSettingListener*>* l=(CList<MSettingListener*>*)iListeners->GetData(Setting);
	if (!l) {
		return;
	}
	CList<MSettingListener*>::Node *i;
	i=l->iFirst;
	while (i) {
		i->Item->SettingChanged(Setting);
		i=i->Next;
	}
}

void CBlackBoardSettingsImpl::CancelNotifyOnChange(TInt Setting, MSettingListener* Listener)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("CancelNotifyOnChange"));

	CList<MSettingListener*>* l=(CList<MSettingListener*>*)iListeners->GetData(Setting);
	if (!l) {
		return;
	}
	CList<MSettingListener*>::Node *i, *prev, *tmp;
	i=l->iFirst; prev=0;
	while (i) {
		if (i->Item==Listener) {
			if (prev) {
				prev->Next=i->Next;
			} else {
				l->iFirst=i->Next;
			}
			if (i==l->iCurrent) l->iCurrent=prev;
			tmp=i;
			i=i->Next;
			delete tmp;
		} else {
			prev=i;
			i=i->Next;
		}
	}
}

void ListDeletor(void* p)
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("CancelNotifyOnChange"));

	CList<MSettingListener*>* l=(CList<MSettingListener*>*)p;
	delete l;
}

void CBlackBoardSettingsImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("ConstructL"));

	{
		CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("iFactory"));
		iFactory=CBBDataFactory::NewL();
	}

	_LIT(db_base, "SETTINGS");
	TBuf<100> dbname;
	dbname.Append(db_base);
	TFileName oldfilen, newfilen;
	oldfilen.Format(_L("%S%S.db"), &AppDir(), &db_base);
	newfilen.Format(_L("%S%S.db"), &DataDir(), &dbname);
	if (BaflUtils::FileExists(Fs(), oldfilen)) {
		User::LeaveIfError(Fs().Rename(oldfilen, newfilen));
	}

	iGetPutBuf=HBufC8::NewL(128);
	iNotifyBuf=HBufC8::NewL(128);

	iListeners=CGenericIntMap::NewL();
	iListeners->SetDeletor(ListDeletor);

	if (BaflUtils::FileExists(Fs(), newfilen)) {
		{
		auto_ptr<CDb> db(CDb::NewL(AppContext(), dbname, EFileRead|EFileShareAny ));

		auto_ptr< CSingleColDb<TDesC, TDes> > desStore(CSingleColDb<TDesC, TDes>::NewL(AppContext(), db->Db(), _L("DES")));
		auto_ptr< CSingleColDb<TDesC8, TDes8> > des8Store(CSingleColDb<TDesC8, TDes8>::NewL(AppContext(), db->Db(), _L("DES8")));
		auto_ptr< CSingleColDb<TInt> > intStore(CSingleColDb<TInt>::NewL(AppContext(), db->Db(), _L("INT")));
		auto_ptr< CSingleColDb<TTime> > timeStore(CSingleColDb<TTime>::NewL(AppContext(), db->Db(), _L("TIME")));

		TUint setting; 
		{
			TBuf<50> value;
			TBool found=desStore->FirstL();
			while(found) {
				desStore->GetL(setting, value);
				WriteSettingL(setting, value);
				found=desStore->NextL();
			}
		}
		{
			TBuf8<50> value;
			TBool found=des8Store->FirstL();
			while(found) {
				des8Store->GetL(setting, value);
				WriteSettingL(setting, value);
				found=des8Store->NextL();
			}
		}
		{
			TInt value;
			TBool found=intStore->FirstL();
			while(found) {
				intStore->GetL(setting, value);
				WriteSettingL(setting, value);
				found=intStore->NextL();
			}
		}
		{
			TTime value;
			TBool found=timeStore->FirstL();
			while(found) {
				timeStore->GetL(setting, value);
				WriteSettingL(setting, value);
				found=timeStore->NextL();
			}
		}
		}
		Fs().Delete(newfilen);
	}

	CActiveScheduler::Add(this);

	ReconnectL();
}

void CBlackBoardSettingsImpl::ReconnectL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("ReconnectL"));
	if ( DataDir().Left(1).CompareF(_L("e"))==0 && !HasMMC(Fs() ) ) {
		User::Leave(KErrNotReady);
	}

	{
		CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("iBBClient.Connect"));
		User::LeaveIfError(iBBClient.Connect());
	}
	TRequestStatus s;
	iBBClient.AddNotificationL(iSettingTupleName, EFalse, EBBPriorityNormal, s);
	User::WaitForRequest(s);
	User::LeaveIfError(s.Int());
	ListenForNotification();
}

void CBlackBoardSettingsImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("CheckedRunL"));

	if (iStatus.Int()!=KErrNone) {
		if (iStatus.Int()==KClientBufferTooSmall) {
			iNotifyBuf->Des().Zero();
			iNotifyBuf=iNotifyBuf->ReAllocL(iNotifyBuf->Des().MaxLength()*2);
			ListenForNotification();
			return;
		} else if ( iStatus.Int()==KErrServerTerminated ) {
			iBBClient.Close();
			ReconnectL();
			return;
		} else {
			User::Leave(iStatus.Int());
		}
	}

	TLex l(iFullArgs.iSubName);
	TInt setting;
	User::LeaveIfError(l.Val(setting));

	iSettingInNotifyBuf=setting;
	if (iSettingInPutBuf==setting) iSettingInPutBuf=-1;
	NotifyOfChange(setting);

	ListenForNotification();
}

void CBlackBoardSettingsImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBlackBoardSettingsImpl"), _CL("DoCancel"));

	iBBClient.CancelNotify();
}

#else
EXPORT_C void DummyExportIfNotFunctionalityCompiledInSoThatLIBIsCreated() { }
#endif
