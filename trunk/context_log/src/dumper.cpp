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

#include "dumper.h"

#include <blackboardclientsession.h>
#include <bbdata.h>
#include <symbian_auto_ptr.h>
#include <context_uids.h>
#include <bbtypes.h>
#include <csd_event.h>
#include <bbxml.h>

#include <cl_settings.h>
#include <cl_settings_impl.h>
#include <s32mem.h>

_LIT(KValue, "value");

class CBBDumperImpl : public CBBDumper {
private:
	CBBDumperImpl();
	void ConstructL();
	~CBBDumperImpl();

	void CheckedRunL();

	void ConnectL();
	void SetFilterL();
	void WaitForNotify();
	void WriteL(const TDesC& aBuf);
	void DoCancel();

	RFs		iFs;
	RFile		iFile;
	RBBClient	iBBClient;

	HBufC8*		iSerializedData;
	TPtr8		iP;
	CXmlBufExternalizer	*iCurrentBuf;

	TFullArgs	iFullArgs;
	TInt		iAsyncErrorCount;
	MBBDataFactory*	iFactory;
	CBBTuple	*iTuple;

	friend class CBBDumper;
	friend class auto_ptr<CBBDumperImpl>;

};

CBBDumper::CBBDumper() : CCheckedActive(EPriorityNormal, _L("CBBDumper"))
{
	CALLSTACKITEM_N(_CL("CBBDumper"), _CL("CBBDumper"));

}

EXPORT_C CBBDumper* CBBDumper::NewL()
{
	CALLSTACKITEM_N(_CL("CBBDumper"), _CL("NewL"));
	auto_ptr<CBBDumperImpl> ret(new (ELeave) CBBDumperImpl);
	ret->ConstructL();
	return ret.release();
}

CBBDumperImpl::CBBDumperImpl() : iP(0, 0) { }

void CBBDumperImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBDumperImpl"), _CL("ConstructL"));

	User::LeaveIfError(iFs.Connect());
	User::LeaveIfError(iFile.Replace(iFs, _L("c:\\dbbdump.xml"), EFileWrite));

	_LIT8(KUnicode, "\xff\xfe");
	iFile.Write(KUnicode());

	TBuf<100> pre=_L("<?xml version='1.0'?><bbdump>\n");

	WriteL(pre);

	iCurrentBuf=CXmlBufExternalizer::NewL(2048);
	iSerializedData=HBufC8::NewL(2048);

	iFactory=CBBDataFactory::NewL();
	iTuple=new (ELeave) CBBTuple(iFactory);

	ConnectL();
	SetFilterL();
	
	CActiveScheduler::Add(this);
	WaitForNotify();
}

CBBDumperImpl::~CBBDumperImpl()
{
	CALLSTACKITEM_N(_CL("CBBDumperImpl"), _CL("~CBBDumperImpl"));

	TBuf<30> end=_L("</bbdump>");
	WriteL(end);
	iFile.Close();
	iFs.Close();
	Cancel();
	iBBClient.Close();
	delete iCurrentBuf;
	delete iSerializedData;
	delete iFactory;
	delete iTuple;
}

void CBBDumperImpl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CBBDumperImpl"), _CL("ConnectL"));

	Cancel();
	TInt errorcount=0;
	TInt err=KErrNone;
	while (errorcount<5) {
		iBBClient.Close();
		err=iBBClient.Connect();
		if (err==KErrNone) return;
		errorcount++;
	}
	User::Leave(err);
}

void CBBDumperImpl::SetFilterL()
{
	CALLSTACKITEM_N(_CL("CBBDumperImpl"), _CL("SetFilterL"));

	TInt errorcount=0, err=KErrNone;
	while (errorcount<5) {
		TRequestStatus s;
		iBBClient.AddNotificationL(KAnyTuple, ETrue, EBBPriorityNormal, s);
		//iBBClient.AddNotificationL(KCLSettingsTuple, ETrue, EBBPriorityNormal, s);
		User::WaitForRequest(s);
		err=s.Int();
		if (err==KErrNone) return;
		ConnectL();
		errorcount++;
	}
	User::Leave(err);
}

void CBBDumperImpl::WaitForNotify()
{
	CALLSTACKITEM_N(_CL("CBBDumperImpl"), _CL("WaitForNotify"));

	if (IsActive()) return;

	iSerializedData->Des().Zero();
	iP.Set(iSerializedData->Des());
	iBBClient.WaitForNotify(iFullArgs, iP, iStatus);
	SetActive();
}

void CBBDumperImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBBDumperImpl"), _CL("DoCancel"));

	iBBClient.CancelNotify();
}

void CBBDumperImpl::WriteL(const TDesC& aBuf)
{
	TPtrC8 p( (const TText8*)aBuf.Ptr(), aBuf.Size() );
	User::LeaveIfError(iFile.Write(p));
}

void CBBDumperImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBBDumperImpl"), _CL("CheckedRunL"));


	{
		if (iStatus.Int()!=KErrNone) {
			if (iStatus.Int()==KClientBufferTooSmall) {
				iSerializedData->Des().Zero();
				iSerializedData=iSerializedData->ReAllocL(iSerializedData->Des().MaxLength()*2);
				iAsyncErrorCount=0;
				WaitForNotify();
				return;
			}
			if (iAsyncErrorCount>5) User::Leave(iStatus.Int());
			ConnectL();
			SetFilterL();
			WaitForNotify();
			return;
		}
	}

	MBBData* d=0;
	{
		RDesReadStream rs(*iSerializedData);
		CleanupClosePushL(rs);	
		TTypeName read_type=TTypeName::IdFromStreamL(rs);
		{
			d=iFactory->CreateBBDataL(read_type, KValue, iFactory);
			CleanupPushBBDataL(d);
		}
		{
			d->InternalizeL(rs);
		}
		CleanupStack::Pop();
		CleanupStack::PopAndDestroy();
	}

	{
		iTuple->iData.SetValue(d);
		iTuple->iTupleMeta.iModuleUid()=iFullArgs.iTupleName.iModule.iUid;
		iTuple->iTupleMeta.iModuleId()=iFullArgs.iTupleName.iId;
		iTuple->iTupleMeta.iSubName=iFullArgs.iSubName;
		iTuple->iTupleId()=iFullArgs.iId;
		iTuple->iExpires()=iFullArgs.iLeaseExpires;
		
		iCurrentBuf->Zero();
		iTuple->IntoXmlL(iCurrentBuf);
		iCurrentBuf->Characters(_L("\n"));

		WriteL(iCurrentBuf->Buf());
	}

	WaitForNotify();
}
