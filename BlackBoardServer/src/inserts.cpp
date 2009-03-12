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

#include "inserts.h"

#include "break.h"
#include "list.h"
#include "concretedata.h"
#include "bbxml.h"
#include "raii_f32file.h"
#include "bbtypes.h"
#include "bb_incoming.h"
#include "csd_event.h"
#include <s32mem.h>

class CInsertsImpl : public CInserts, public MContextBase,
	public MBBStream, public MIncomingObserver {
private:
	CInsertsImpl(MApp_context& aContext, CBlackBoardServer* aServer);
	~CInsertsImpl();
	void ConstructL();
	void ReadInsertsL();
	void HandleFileL(const TDesC& aFileName);

	// MBBStream
	const MBBData* Part(TUint aPartNo) const;
        virtual const TTypeName& Type() const;
	virtual void ResetPart(TUint aPart);

	// MBBIncomingObserver
	virtual void StreamOpened() { }
	virtual void StreamClosed() { }
	virtual void IncomingData(const MBBData* aData, TBool aErrors);
	virtual void StreamError(TInt aError, const TDesC& aDescr);

	CBBTuple		*iTuple;
	HBufC8			*iGetPutBuf;
	CBBDataFactory		*iFactory;

	CBlackBoardServer* iServer;

	friend class CInserts;
	friend class auto_ptr<CInsertsImpl>;
};

CInserts* CInserts::NewL(MApp_context& Context, CBlackBoardServer* aServer)
{
	auto_ptr<CInsertsImpl> ret(new (ELeave) CInsertsImpl(Context, aServer));
	ret->ConstructL();
	return ret.release();
}

_LIT(KInserts, "inserts");

CInsertsImpl::CInsertsImpl(MApp_context& aContext, 
							CBlackBoardServer* aServer) : 
	MContextBase(aContext), MBBStream(KInserts, *this),
	iServer(aServer)
{
}

void CInsertsImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("ConstructL"));

	iFactory=CBBDataFactory::NewL();
	MBBStream::ConstructL();
	iTuple=new CBBTuple(iFactory);
	iGetPutBuf=HBufC8::NewL(1024);
}

CInsertsImpl::~CInsertsImpl()
{
	delete iTuple;
	delete iGetPutBuf;
	delete iFactory;
}

void CInsertsImpl::ReadInsertsL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("ReadInsertsL"));

	TFileName filename;
	TFileName dirname= DataDir();
	dirname.Append(KInsertsDir);
	dirname.Append(_L("*.xml"));
	for (int i=0; i<2; i++) {
		CDir* dir=0;
		TInt err=Fs().GetDir(dirname, KEntryAttMaskSupported, 
			ESortByName, dir);
		if (err!=KErrNone) {
			dirname.Replace(0, 1, _L("c"));
			continue;
		}
		auto_ptr<CDir> auto_dir(dir);
		for (int f=0; f<dir->Count(); f++) {
			filename=dirname.Left(dirname.Length()-5);
			filename.Append( (*dir)[f].iName );
			CC_TRAPD(err, HandleFileL( filename ));
			if (err!=KErrNone) {
				//TODO: log
			} else {
				Fs().Delete(filename);
			}
		}
		dirname.Replace(0, 1, _L("c"));
	}
}

void CInsertsImpl::HandleFileL(const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("HandleFileL"));

	RAFile file; file.OpenLA(Fs(), aFileName, EFileRead|EFileShareAny);
	MBBStream::Reset();
	TBuf8<256> buf;
	while (file.Read(buf)==KErrNone && buf.Length()) {
		CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("ParseL"));
		ParseL(buf);
		buf.Zero();
	}
}

// MBBStream
const MBBData* CInsertsImpl::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return iTuple;
	default:
		return 0;
	}
}

const TTypeName& CInsertsImpl::Type() const
{
	return KNoType;
}

void CInsertsImpl::ResetPart(TUint aPart)
{
	switch(aPart) {
	case 0:
		delete iTuple; iTuple=0;
		iTuple=new CBBTuple(iFactory);
	}
}


void CInsertsImpl::IncomingData(const MBBData* aData, TBool aErrors)
{
	if (aErrors) {
		return;
	}
	if (aData==iTuple) {
		CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("IncomingData"));
		TInt err=KErrNone;
		{
			CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("IncomingData1"));
			TPtr8 bufp(iGetPutBuf->Des());
			RDesWriteStream ws(bufp);
			iTuple->iData()->Type().ExternalizeL(ws);
			TRAP(err, iTuple->iData()->ExternalizeL(ws));
			if (err==KErrNone) ws.CommitL();
		}
		while (err==KErrOverflow) {
			CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("IncomingData2"));
			iGetPutBuf=iGetPutBuf->ReAllocL( iGetPutBuf->Des().MaxLength() *2);
			TPtr8 bufp(iGetPutBuf->Des());
			RDesWriteStream ws(bufp);
			iTuple->iData()->Type().ExternalizeL(ws);
			TRAP(err, iTuple->iData()->ExternalizeL(ws));
			if (err==KErrNone) ws.CommitL();
		}

		{
			CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("IncomingData3"));
			TUint dummy;
			TTime expires=Time::MaxTTime();
			TTupleName tn;
			TUint uid=iTuple->iTupleMeta.iModuleUid();
			GetBackwardsCompatibleUid(uid);
			tn.iModule=TUid::Uid(uid);
			tn.iId=iTuple->iTupleMeta.iModuleId();
			iServer->PutL(tn, iTuple->iTupleMeta.iSubName(), KNoComponent, *iGetPutBuf,
				EBBPriorityNormal, ETrue, dummy, expires);
		}
	}
}

void CInsertsImpl::StreamError(TInt aError, const TDesC& aDescr)
{
	// TODO: log
}
