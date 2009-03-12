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
#include "permanent.h"
#include "list.h"
#include "concretedata.h"
#include "bb_incoming.h"
#include "bbxml.h"
#include "raii_f32file.h"
#include "bbtypes.h"

class CPermanentProxy : public CBase, public MBlackBoardObserver {
public:
	static CPermanentProxy* NewL(CBlackBoardServer* aServer, TComponentName& aComponentName)
	{
		auto_ptr<CPermanentProxy> ret(new (ELeave) CPermanentProxy(aServer, aComponentName));
		return ret.release();
	}
	~CPermanentProxy() {
		iServer->DeleteAllNotificationsL(this);
	}
private:
	CPermanentProxy(CBlackBoardServer* aServer, TComponentName& aComponentName) : 
		iComponentName(aComponentName), iServer(aServer) { }
	virtual void NotifyL(TUint /*aId*/, TBBPriority aPriority,
			TTupleType /*aTupleType*/,
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& /*aComponent*/,
			const TDesC8& aSerializedData, const TTime& ) {

		TUint dummy;
		TTime t=GetTime(); t+=TTimeIntervalDays(14);
		iServer->PutL(aTupleName, aSubName, iComponentName,
			aSerializedData, aPriority, EFalse,
			dummy, t, ETrue, ETuplePermanentSubscriptionEvent);
	}
	virtual void NotifyDeletedL(const TTupleName& aTupleName, const TDesC& aSubName) {
	}

	virtual void NotifyL(TUint aId, TBBPriority aPriority) {
		TFullArgs args;
		TUint size, dummy;
		RADbColReadStream rs;
		iServer->GetL(aId, args.iTupleType, args.iTupleName, args.iSubName, 
			args.iComponentName, rs, size, args.iLeaseExpires);
		auto_ptr< HBufC8 > buf(HBufC8::NewL(size));
		TPtr8 p(buf->Des());
		if (size>0) {
			rs.ReadL(p, size);
		}
		TTime t=GetTime(); t+=TTimeIntervalDays(14);
		iServer->PutL(args.iTupleName, args.iSubName, iComponentName,
			*buf, aPriority, EFalse,
			dummy, t, ETrue, ETupleRequest);
	}

	CBlackBoardServer* iServer;
	TComponentName iComponentName;
};

class CPermanentSubscriptionsImpl : public CPermanentSubscriptions, public MContextBase,
	public MBBStream, public MIncomingObserver {
private:
	CPermanentSubscriptionsImpl(MApp_context& aContext, CBlackBoardServer* aServer);
	~CPermanentSubscriptionsImpl();
	void ConstructL();
	void ReadSubscriptionsL();
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

	TBBComponentName	iComponent;
	TBBTupleName		iTuple;

	CPtrList<CPermanentProxy> *iProxies;
	CPermanentProxy*	iCurrentProxy;
	CBlackBoardServer* iServer;

	friend class CPermanentSubscriptions;
	friend class auto_ptr<CPermanentSubscriptionsImpl>;
};

CPermanentSubscriptions* CPermanentSubscriptions::NewL(MApp_context& Context, CBlackBoardServer* aServer)
{
	auto_ptr<CPermanentSubscriptionsImpl> ret(new (ELeave) CPermanentSubscriptionsImpl(Context, aServer));
	ret->ConstructL();
	return ret.release();
}

_LIT(KComponent, "component");
_LIT(KTupleName, "tuplename");
_LIT(KSubscriptions, "subscriptions");

CPermanentSubscriptionsImpl::CPermanentSubscriptionsImpl(MApp_context& aContext, 
							CBlackBoardServer* aServer) : 
	MContextBase(aContext), MBBStream(KSubscriptions, *this),
	iComponent(KComponent), iTuple(KTupleName), iServer(aServer)
{
}

void CPermanentSubscriptionsImpl::ConstructL()
{
	MBBStream::ConstructL();
	iProxies=CPtrList<CPermanentProxy>::NewL();
}

CPermanentSubscriptionsImpl::~CPermanentSubscriptionsImpl()
{
	delete iProxies;
}

void CPermanentSubscriptionsImpl::ReadSubscriptionsL()
{
	TFileName dirname=_L("e:"), filename;
	dirname.Append(KSubscriptionDir);
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
			}
		}
		dirname.Replace(0, 1, _L("c"));
	}
}

void CPermanentSubscriptionsImpl::HandleFileL(const TDesC& aFileName)
{
	RAFile file; file.OpenLA(Fs(), aFileName, EFileRead|EFileShareAny);
	MBBStream::Reset();
	TBuf8<256> buf;
	while (file.Read(buf)==KErrNone && buf.Length()) {
		ParseL(buf);
		buf.Zero();
	}
}

// MBBStream
const MBBData* CPermanentSubscriptionsImpl::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iComponent;
	case 1:
		return &iTuple;
	default:
		return 0;
	}
}

const TTypeName& CPermanentSubscriptionsImpl::Type() const
{
	return KNoType;
}

void CPermanentSubscriptionsImpl::ResetPart(TUint aPart)
{
	switch(aPart) {
	case 0:
		iComponent=KNoComponent;
	case 1:
		iTuple=KNoTuple;
	}
}


void CPermanentSubscriptionsImpl::IncomingData(const MBBData* aData, TBool aErrors)
{
	if (aErrors) {
		// TODO: log
		if (aData==&iComponent) iCurrentProxy=0;
		return;
	}
	if (aData==&iComponent) {
		TComponentName n;
		n.iModule.iUid=iComponent.iModuleUid();
		n.iId=iComponent.iModuleId();
		auto_ptr<CPermanentProxy> p(CPermanentProxy::NewL(iServer, n));
		iCurrentProxy=p.get();
		iProxies->AppendL(p.get());
		p.release();
	} else {
		if (iCurrentProxy) {
			TTupleName n;
			n.iModule.iUid=iTuple.iModuleUid();
			n.iId=iTuple.iModuleId();
			iServer->AddNotificationL(iCurrentProxy, n, EFalse, EBBPriorityNormal);
		} else {
			// TODO: log
		}
	}
}

void CPermanentSubscriptionsImpl::StreamError(TInt aError, const TDesC& aDescr)
{
	// TODO: log
}
