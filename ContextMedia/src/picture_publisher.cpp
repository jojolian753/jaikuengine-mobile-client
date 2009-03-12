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
#include "picture_publisher.h"
#include "transfer2.h"
#include <bautils.h>
#include "cm_post.h"
#include "cl_settings.h"

_LIT(KClassName, "CPicturePublisherImpl");

class CPicturePublisherImpl : public CPicturePublisher, public MContextBase,
	public i_status_notif, public MSettingListener  {
public:
	~CPicturePublisherImpl();
private:
	CPicturePublisherImpl(MApp_context& Context, i_status_notif& notif, MUploadPrompt& Prompt, MUploadPrompt& OldPrompt,
		CTransferDir*	aTransferDir);
	void ConstructL(const TDesC& Path, const TDesC& Files, 
		TInt Setting, TInt SettingValue,
		TInt aUrlSetting, TInt aScriptSetting,
		const TDesC& AdditionalFiles=KNullDesC);

	virtual void PublishOld();

	void CheckedRunL();
	TInt CheckedRunError(TInt aError);
	void DoCancel();

	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent);

	virtual void finished();
	virtual void error(const TDesC& descr);
	virtual void status_change(const TDesC& status);

	virtual void SettingChanged(TInt Setting);

	void Transfer(const TDesC& aSubDir, MUploadPrompt* aPrompt, const TTime& aFromTime);
	void Transfer();
	bool IsEnabled();
	void StartL();
	void Stop();

	i_status_notif& iNotif;
	TBuf<200>	iLoc;
	bool		iNoPics;
	TFileName	iPath; 
#ifdef __S60V2__
	TFileName	iMonthPath;
	TBool		iMonthPathExists;
	void CheckMonthPath();
#endif
	TBuf<30>	iFiles, iAddFiles;
	TInt		iSetting;
	CTransferDir*	iTransferDir;
	//TOldPrompt*	iOldPrompt;
	TBool		iPathExists;
	TInt		iSettingValue;

	TInt		iUrlSetting, iScriptSetting;
#ifdef __WINS__test
	RTimer		iTimer;
#endif
	MUploadPrompt&	iPrompt;
	MUploadPrompt&  iOldPrompt;

	friend class CPicturePublisher;
};

EXPORT_C CPicturePublisher* CPicturePublisher::NewL(MApp_context& Context, i_status_notif& notif,
					const TDesC& Path, const TDesC& Files, TInt Setting, 
					TInt SettingValue, 
					TInt aUrlSetting, TInt aScriptSetting,
					MUploadPrompt& Prompt, MUploadPrompt& OldPrompt, 
					CTransferDir*	aTransferDir,
					const TDesC& AdditionalFiles)
{
	CALLSTACKITEM_N(_CL("CPicturePublisher"), _CL("NewL"));

	auto_ptr<CPicturePublisherImpl> ret(new (ELeave) 
		CPicturePublisherImpl(Context, notif, Prompt, OldPrompt, aTransferDir));
	ret->ConstructL(Path, Files, Setting, SettingValue, aUrlSetting,
		aScriptSetting, AdditionalFiles);
	return ret.release();
}

CPicturePublisherImpl::~CPicturePublisherImpl()
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("~CPicturePublisherImpl"));

	Settings().CancelNotifyOnChange(iSetting, this);
	Stop();
#ifdef __WINS__test
	iTimer.Close();
#endif
}

CPicturePublisher::CPicturePublisher() : CCheckedActive(EPriorityNormal, KClassName)
{
}

CPicturePublisherImpl::CPicturePublisherImpl(MApp_context& Context, i_status_notif& notif, MUploadPrompt& Prompt, MUploadPrompt& OldPrompt, CTransferDir*	aTransferDir) :
	MContextBase(Context), iNotif(notif), iTransferDir(aTransferDir), iPrompt(Prompt), iOldPrompt(OldPrompt)
{
}

void CPicturePublisherImpl::ConstructL(const TDesC& Path, const TDesC& Files, 
					TInt Setting, TInt aSettingValue,
					TInt aUrlSetting, TInt aScriptSetting,
				const TDesC& AdditionalFiles)
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("ConstructL"));

	Mlogger::ConstructL(AppContextAccess());

	iUrlSetting=aUrlSetting;
	iScriptSetting=aScriptSetting;

	TBool delete_after=ETrue;
	Settings().GetSettingL(SETTING_DELETE_UPLOADED, delete_after);
	//iOldPrompt=new (ELeave) TOldPrompt(delete_after);
	iSettingValue = aSettingValue;
	
	iPath=Path; iFiles=Files; iSetting=Setting; iAddFiles=AdditionalFiles;

	{	
		TFileName t=iPath;
		//
		// only listen on directories that exist, otherwise
		// we seem to get a lot of spurious events
		//
		t.Replace(0, 1, _L("c"));
		t.Append(_L("\\"));
		iPathExists=BaflUtils::PathExists(Fs(), t);
		if (!iPathExists) {
			t.Replace(0, 1, _L("e"));
			iPath.Replace(0, 1, _L("e"));
			iPathExists=BaflUtils::PathExists(Fs(), t);
		} else {
			t.Replace(0, 1, _L("e"));
			if (! (BaflUtils::PathExists(Fs(), t))) iPath.Replace(0, 1, _L("c"));
		}
	}

	Settings().NotifyOnChange(iSetting, this);
#ifdef __WINS__test
	iTimer.CreateLocal();
#endif
	CActiveScheduler::Add(this);

	if (IsEnabled()) {
		StartL();
	}
}

void CPicturePublisherImpl::Stop()
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("Stop"));

	Cancel();
}

void CPicturePublisherImpl::StartL()
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("StartL"));

#ifdef __S60V2__
	CheckMonthPath();
#endif
	Stop();

	iStatus=KRequestPending;
#ifndef __WINS__test
	if (iPathExists) {
		Fs().NotifyChange(ENotifyEntry, iStatus, iPath);
	} else {
		return;
	}
#else
	iTimer.After( iStatus, 30*1000*1000 );
#endif
	SetActive();
}

bool CPicturePublisherImpl::IsEnabled()
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("IsEnabled"));

	TInt publisher=0;
	if ( (Settings().GetSettingL(iSetting, publisher)) 
		&& (publisher==iSettingValue)) return true;
	return false;
}

void CPicturePublisherImpl::SettingChanged(TInt setting)
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("SettingChanged"));

	if (setting==iSetting) { 
		if (IsEnabled()) {
			CC_TRAPD(err, StartL());
			if (err!=KErrNone) {
				TBuf<100> msg;
				msg.Format(_L("Error starting PicturePublisher: %d"), err);
				iNotif.error(msg);
			}
		} else {
			Stop();
		}
	} 
}
			

#ifdef __S60V2__
void CPicturePublisherImpl::CheckMonthPath()
{
	if (!iMonthPathExists) {
		TBool c=EFalse, e=EFalse;
		iMonthPath=iPath;
		iMonthPath.Append(_L("\\"));
		TTime now; now.HomeTime();
		TDateTime dt(now.DateTime());
		iMonthPath.AppendNumFixedWidth( dt.Year(), EDecimal, 4);
		iMonthPath.AppendNumFixedWidth( dt.Month()+1, EDecimal, 2);
		iMonthPath[0]='c';
		iMonthPath.Append(_L("\\"));
		TInt len_without_subdir=iMonthPath.Length();
		if (BaflUtils::PathExists(Fs(), iMonthPath)) {
			c=ETrue;
			iMonthPath.AppendNumFixedWidth( dt.Year(), EDecimal, 4);
			iMonthPath.AppendNumFixedWidth( dt.Month()+1, EDecimal, 2);
			iMonthPath.Append(_L("A0"));
			iMonthPath.Append(_L("\\"));
			if (! BaflUtils::PathExists(Fs(), iMonthPath)) {
				iMonthPath.SetLength(len_without_subdir);
			}
		}
		iMonthPath[0]='e';
		if (BaflUtils::PathExists(Fs(), iMonthPath)) {
			e=ETrue;
			if (iMonthPath.Length()==len_without_subdir) {
				iMonthPath.AppendNumFixedWidth( dt.Year(), EDecimal, 4);
				iMonthPath.AppendNumFixedWidth( dt.Month()+1, EDecimal, 2);
				iMonthPath.Append(_L("A0"));
				iMonthPath.Append(_L("\\"));
				if (! BaflUtils::PathExists(Fs(), iMonthPath)) {
					iMonthPath.SetLength(len_without_subdir);
				} else {
					c=EFalse;
				}
			}
		}
		iMonthPath.SetLength( iMonthPath.Length() -1 );
		if (c || e) {
			iMonthPathExists=ETrue;
		}
		if (c && e) {
			iMonthPath[0]='?';
		} else if (c) {
			iMonthPath[0]='c';
		} else if (e) {
			iMonthPath[0]='e';
		}
		if (iMonthPathExists) {
			iPath=iMonthPath;
		}
	}
}
#endif
void CPicturePublisherImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("CheckedRunL"));

	if (iStatus==KErrNone) {
#ifdef __S60V2__
		CheckMonthPath();
#endif
		Transfer();
	} else {
		TBuf<30> msg=_L("error ");
		msg.AppendNum(iStatus.Int());
		iNotif.error(msg);
	}
	iStatus=KRequestPending;
#if 1
	Fs().NotifyChange(ENotifyEntry, iStatus, iPath);
#else
	iTimer.After( iStatus, 10*1000*1000 );
#endif
	SetActive();
}

void CPicturePublisherImpl::Transfer()
{
	/*
	TBuf<200> msg=_L("new pictures noticed ");
	msg.Append(iPath);

	iNotif.status_change(msg);
	*/

	TTime now; 
#ifndef __S60V3__
	now.HomeTime(); 
#else
	now.UniversalTime(); 
#endif

#ifndef __WINS__
	now-=TTimeIntervalMinutes(2);
#else
	now-=TTimeIntervalDays(2);
#endif

	Transfer(_L(""), &iPrompt, now);
}

void CPicturePublisherImpl::Transfer(const TDesC& aSubDir, MUploadPrompt* aPrompt, const TTime& aFromTime)
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("Transfer"));

	iNoPics=true;
	if (iLoc.Length()==0) iLoc=_L("Unknown");
	TFileName p;
	if (iPath.Left(1).Compare(_L("?"))) {
		p=iPath;
		p.Append(_L("\\"));
		p.Append(aSubDir);
		p.Append(iFiles);
		iTransferDir->ProcessDir(p, iUrlSetting, iScriptSetting,
			iLoc, aPrompt, aFromTime, EFalse, EFalse);
		if (iAddFiles.Length()>0) {
			p=iPath;
			p.Append(_L("\\"));
			p.Append(aSubDir);
			p.Append(iAddFiles);
			iTransferDir->ProcessDir(p, iUrlSetting, iScriptSetting,
				iLoc, aPrompt, aFromTime, EFalse, EFalse);
		}

	} else {
		p=iPath;
		p.Replace(0, 1, _L("c"));
		p.Append(_L("\\"));
		p.Append(aSubDir);
		p.Append(iFiles);
		iTransferDir->ProcessDir(p, iUrlSetting, iScriptSetting,
			iLoc, aPrompt, aFromTime, EFalse, EFalse);
		if (iAddFiles.Length()>0) {
			p=iPath;
			p.Replace(0, 1, _L("c"));
			p.Append(_L("\\"));
			p.Append(aSubDir);
			p.Append(iAddFiles);
			iTransferDir->ProcessDir(p, iUrlSetting, iScriptSetting,
				iLoc, aPrompt, aFromTime, EFalse, EFalse);
		}

		p=iPath;
		p.Replace(0, 1, _L("e"));
		p.Append(_L("\\"));
		p.Append(aSubDir);
		p.Append(iFiles);
		iTransferDir->ProcessDir(p, iUrlSetting, iScriptSetting,
			iLoc, aPrompt, aFromTime, EFalse, EFalse);
		if (iAddFiles.Length()>0) {
			p=iPath;
			p.Replace(0, 1, _L("e"));
			p.Append(_L("\\"));
			p.Append(aSubDir);
			p.Append(iAddFiles);
			iTransferDir->ProcessDir(p, iUrlSetting, iScriptSetting,
				iLoc, aPrompt, aFromTime, EFalse, EFalse);
		}
	}

	iNoPics=false;
}

TInt CPicturePublisherImpl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("CheckedRunError"));

	return aError;
}

void CPicturePublisherImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("DoCancel"));

#ifndef __WINS__test
	Fs().NotifyChangeCancel();
#else
	iTimer.Cancel();
#endif
}

void CPicturePublisherImpl::NewSensorEventL(const TTupleName& , const TDesC& , const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("NewSensorEventL"));

	if (aEvent.iPriority()!=CBBSensorEvent::VALUE) iLoc=_L("");
	else if (aEvent.iData()) { iLoc.Zero(); CC_TRAPD(err, aEvent.iData()->IntoStringL(iLoc)); }

}

void CPicturePublisherImpl::finished()
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("finished"));

	if (! iNoPics ) {
		iNotif.status_change(_L("uploaded pics"));
	}
}

void CPicturePublisherImpl::error(const TDesC& descr)
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("error"));

	iNotif.error(descr);
}

void CPicturePublisherImpl::status_change(const TDesC& status)
{
	CALLSTACKITEM_N(_CL("CPicturePublisherImpl"), _CL("status_change"));

	iNotif.status_change(status);
}

EXPORT_C TOldPrompt::TOldPrompt(bool Delete) : iDelete(Delete)
{
}

EXPORT_C void TOldPrompt::Prompt(const TDesC& /*FileName*/, MUploadCallBack* CallBack)
{
	CALLSTACKITEM_N(_CL("TOldPrompt"), _CL("Prompt"));

	auto_ptr<CCMPost> b(CCMPost::NewL(0));
	CallBack->Back(true, iDelete, b.get());
}

void CPicturePublisherImpl::PublishOld()
{
	if (!iPathExists) return;

	TTime now(0);
	Transfer(_L(""), &iOldPrompt, now);
	Transfer(_L("Old\\"), &iOldPrompt, now);
}

EXPORT_C CPicturePublisher::~CPicturePublisher() { }
