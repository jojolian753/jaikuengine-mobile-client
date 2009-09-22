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
#include "bberrors.h"
#include <badesca.h>
#include "bbdata.h"
#include "bbtypes.h"
#include "concretedata.h"
#include "symbian_tree.h"
#include <bafindf.h>
#include "context_uids.h"
#include <e32uid.h>
#include "stringarg.h"
#include "errorhandling.h"
#include "old_context_uids.rh"
#include "old2_context_uids.rh"

EXPORT_C TTypeName TTypeName::IdFromAttributes(const XML_Char ** atts) 
{
	TTypeName ret={ { 0 }, -1, -1, -1};
	if (!atts) return ret;

	const XML_Char** p=&atts[0];
	while ( *p && *(p+1)) {
		TPtrC att((const TUint16*)*p), val((const TUint16*)*(p+1));
		TLex l;

		TUint uvalue; TInt value;
		if (val.Left(2).CompareF(_L("0x"))==0) {
			l.Assign(val.Mid(2));
			l.Val(uvalue, EHex);
			value=uvalue;
		} else {
			l.Assign(val);
			l.Val(value);
			uvalue=value;
		}
		if (att.Compare(KAttModule)==0) {
			GetBackwardsCompatibleUid(uvalue);
			ret.iModule.iUid=uvalue;
		} else if (att.Compare(KAttId)==0) {
			ret.iId=value;
		} else if (att.Compare(KAttMajorVersion)==0) {
			ret.iMajorVersion=value;
		} else if (att.Compare(KAttMinorVersion)==0) {
			ret.iMinorVersion=value;
		}
		p+=2;
	}
	return ret;
}

EXPORT_C void TTypeName::ExternalizeL(RWriteStream& aStream) const
{
	aStream.WriteInt32L(iModule.iUid);
	aStream.WriteInt32L(iId);
	aStream.WriteInt32L(iMajorVersion);
	aStream.WriteInt32L(iMinorVersion);
}

EXPORT_C TTypeName TTypeName::IdFromStreamL(RReadStream& aStream)
{
	TTypeName ret={ {0}, -1, -1, -1};
	ret.iModule.iUid=aStream.ReadInt32L();
	TUint uid3=(TUint)ret.iModule.iUid;
	GetBackwardsCompatibleUid(uid3);
	ret.iModule.iUid=uid3;
	ret.iId=aStream.ReadInt32L();
	ret.iMajorVersion=aStream.ReadInt32L();
	ret.iMinorVersion=aStream.ReadInt32L();
	return ret;
}

EXPORT_C void TTypeName::IntoStringL(TDes& aString) const
{
	CheckStringSpaceL(aString, 45);
	aString.Append(_L("[ 0x"));
	aString.AppendNum(iModule.iUid, EHex);
	aString.Append(_L(" "));
	aString.AppendNum(iId);
	aString.Append(_L(" "));
	aString.AppendNum(iMajorVersion);
	aString.Append(_L(" "));
	aString.AppendNum(iMinorVersion);
	aString.Append(_L(" ]"));
}

EXPORT_C bool TTypeName::operator==(const TTypeName& rhs) const
{
	TUint l_uid3=iModule.iUid, r_uid3=rhs.iModule.iUid;
	GetBackwardsCompatibleUid(l_uid3);
	GetBackwardsCompatibleUid(r_uid3);
	return (
		(l_uid3 == r_uid3) &&
		(iId == rhs.iId) &&
		(iMajorVersion == rhs.iMajorVersion) &&
		(iMinorVersion == rhs.iMinorVersion)
		);
}

EXPORT_C const class TBBCompoundData* MBBData::IsCompound() const
{
	return 0;
}

EXPORT_C bool TTupleName::operator==(const TTupleName& rhs) const
{
	if (iModule.iUid!=rhs.iModule.iUid) return false;
	if (iId!=rhs.iId) return false;
	return true;
}

EXPORT_C void TTypeName::CompareMajorL(const TTypeName& aOther) const
{
	if (aOther.iModule!=iModule || aOther.iId!=iId) {
		MErrorInfoManager& e1=InputErr(_L("Blackboard datatype doesn't match. Given %1 expected %2"));
		MErrorInfoManager& e2=e1.UserMsg(aOther, *this);
		TErrorCode c=BBErrorCode(KTypeDoesNotMatch);
		MErrorInfoManager& e3=e2.ErrorCode(c);
		e3.Raise();
	}
	if (aOther.iMajorVersion!=iMajorVersion) {
		InputErr(gettext("Blackboard datatype version not supported. Given %1 expected %2")).
			UserMsg(aOther, *this).ErrorCode(BBErrorCode(KTypeDoesNotMatch)).
			Raise();
	}
}

EXPORT_C MDesCArray* TTypeName::MakeAttributesLC() const
{
	CDesCArrayFlat *ret=new (ELeave) CDesCArrayFlat(8);
	CleanupStack::PushL(ret);
	TBuf<20> buf;
	ret->AppendL(KAttModule);
	buf.Append(_L("0x")); buf.AppendNum((TInt)iModule.iUid, EHex); ret->AppendL(buf);

	ret->AppendL(KAttId);
	buf.Zero(); buf.AppendNum(iId); ret->AppendL(buf);

	ret->AppendL(KAttMajorVersion);
	buf.Zero(); buf.AppendNum(iMajorVersion); ret->AppendL(buf);

	ret->AppendL(KAttMinorVersion); 
	buf.Zero(); buf.AppendNum(iMinorVersion); ret->AppendL(buf);

	return ret;
}

EXPORT_C MBBExternalizer::MBBExternalizer() : iOffset(0) { }

EXPORT_C MNestedXmlHandler::~MNestedXmlHandler()
{
}

EXPORT_C MNestedXmlHandler::MNestedXmlHandler(MNestedXmlHandler* aParent) : iParent(aParent), iOffset(0), iIgnoreOffset(EFalse) { }

EXPORT_C TTimeIntervalSeconds MNestedXmlHandler::GetTimeOffset()
{
	if (iIgnoreOffset) return TTimeIntervalSeconds(0);
	if (iParent) return iParent->GetTimeOffset();
	return iOffset;
}

void DeleteBBData(TAny* p) {
	MBBData *m=(MBBData *)p;
	delete m;
}

void DeleteBBFactory(TAny* p) {
	MBBDataFactory *m=(MBBDataFactory*)p;
	delete m;
}

void DeleteRLibrary(TAny* p) {
	RLibrary *m=(RLibrary*)p;
	m->Close();
	delete m;
}

EXPORT_C void CleanupPushBBDataL(MBBData* aData)
{
	CleanupStack::PushL(TCleanupItem(&DeleteBBData, (TAny*)aData));
}

void CleanupPushBBFactoryL(MBBDataFactory* aData)
{
	CleanupStack::PushL(TCleanupItem(&DeleteBBFactory, (TAny*)aData));
}

/*
 * Concepts:
 * !Using polymorphic dlls!
 */

class CBBDataFactoryImpl : public CBBDataFactory {
private:
	CBBDataFactoryImpl();
	void ConstructL();
	~CBBDataFactoryImpl();
	void Reset();

	virtual MBBData* CreateBBDataL(const TTypeName& aType, const TDesC& aName, MBBDataFactory* aTopLevelFactory);

	friend class CBBDataFactory;
	friend class auto_ptr<CBBDataFactoryImpl>;

	CGenericIntMap*	iFactories;
	CGenericIntMap*	iLibraries;
	RFs		iFs; bool fs_open;
};

EXPORT_C CBBDataFactory* CBBDataFactory::NewL()
{
	auto_ptr<CBBDataFactoryImpl> ret(new (ELeave) CBBDataFactoryImpl);
	ret->ConstructL();
	return ret.release();
}

EXPORT_C CBBDataFactory::~CBBDataFactory()
{
}

EXPORT_C void CBBDataFactory::ConstructL()
{
}

CBBDataFactoryImpl::CBBDataFactoryImpl()
{
}

void CBBDataFactoryImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBDataFactoryImpl"), _CL("ConstructL"));

	iFactories=CGenericIntMap::NewL();
	iFactories->SetDeletor(&DeleteBBFactory);
	iLibraries=CGenericIntMap::NewL();
	iLibraries->SetDeletor(&DeleteRLibrary);
	User::LeaveIfError(iFs.Connect());
}

CBBDataFactoryImpl::~CBBDataFactoryImpl()
{
	delete iFactories;
	delete iLibraries;
	iFs.Close();
}

// Concepts:
// !Custom polymorphic dlls!
MBBData* CBBDataFactoryImpl::CreateBBDataL(const TTypeName& aType, const TDesC& aName, MBBDataFactory* )
{
	CALLSTACKITEM_N(_CL("CBBDataFactoryImpl"), _CL("CreateBBDataL"));

	TUint uid3=(uint32)aType.iModule.iUid;
	GetBackwardsCompatibleUid(uid3);
	TTypeName tn=aType; tn.iModule.iUid=uid3;
	MBBDataFactory* f=(MBBDataFactory*)iFactories->GetData(uid3);
	if (!f) {
		CALLSTACKITEM_N(_CL("CBBDataFactoryImpl"), _CL("load_library"));
		RLibrary *l=new (ELeave) RLibrary;
		TInt err;
		CC_TRAP(err, iLibraries->AddDataL(uid3, l, true));
		if (err!=KErrNone) {
			delete l;
			User::Leave(err);
		}
#ifndef __S60V3__
		auto_ptr<CFindFileByType> findf(new (ELeave) CFindFileByType(iFs));
		TUidType uid(KDynamicLibraryUid, KUidBlackBoardData, 
			TUid::Uid(uid3));
#ifdef __WINS__
		/*err=findf->FindFirst(_L("contextmediafactory.dll"), _L("z:\\system\\libs\\"), 
			TUidType(KNullUid, KNullUid, KNullUid));
		err=findf->FindFirst(_L("contextmediafactory.dll"), _L("z:\\system\\libs\\"), 
			TUidType(KNullUid, KNullUid, aType.iModule));*/
		err=findf->FindFirst(_L("*.dll"), _L("z:\\system\\libs\\"), uid);
		//err=findf->FindFirst(_L("blackboardfactory.dll"), _L("z:\\system\\libs\\"), 
		//	TUidType(KNullUid, KNullUid, aType.iModule));
#else
		err=findf->FindFirst(_L("*.dll"), _L("\\system\\libs\\"), uid);
#endif
		if (err==KErrNoMemory) User::Leave(err);
		if (err!=KErrNone) {
			EnvErr(gettext("A library installation is missing, please reinstall the software.")).
			TechMsg(_L("Cannot find dll with UID %1, error: %2"), aType.iModule.iUid, err)
				.ErrorCode(BBErrorCode(KNoDllForType)).Raise();
		}

		err=l->Load( findf->Entry().iName, TUidType(KNullUid, KNullUid, aType.iModule) );
#else
		TBuf<50> filen=_L("bbf");
		filen.AppendNum(aType.iModule.iUid, EHex);
		filen.Append(_L(".dll"));
		err=l->Load(filen);
#endif
		if (err==KErrNoMemory) User::Leave(err);
		if (err!=KErrNone) {
			EnvErr(gettext("A library installation is faulty, please reinstall the software.")).
				TechMsg(_L("Cannot load dll %1 with UID %2, error: %3"), 
#ifndef __S60V3__
					findf->Entry().iName, 
#else
					filen,
#endif
					uid3,
					err).
				ErrorCode(BBErrorCode(KNoDllForType)).
				Raise();
		}
		TLibraryFunction entry=l->Lookup(1);
		f=(MBBDataFactory*)entry();
		if (!f) User::Leave(KErrNoMemory);
		CleanupPushBBFactoryL(f);
		f->ConstructL();
		iFactories->AddDataL(uid3, f, true);
		CleanupStack::Pop();
	}
	return f->CreateBBDataL(aType, aName, this);
}

void CBBDataFactoryImpl::Reset()
{
	iLibraries->Reset();
	iFactories->Reset();
}


EXPORT_C CBBGeneralHolder::CBBGeneralHolder(const TDesC& aName, MBBDataFactory* aFactory, MBBData* aValue) 
	: iValue(aValue), iFactory(aFactory),
	iOwnsValue(ETrue), iName(&aName)
{
}

EXPORT_C MBBData* CBBGeneralHolder::Assign(const MBBData* aRhs) 
{
	// UNSAFE! you must be sure to only call Assign with something that is a CBBGeneralHolder
	const CBBGeneralHolder * rhs = (const CBBGeneralHolder*)aRhs;
	if (!rhs) return 0;
	if (rhs->iValue) {
		SetValue(rhs->iValue->CloneL(Name()));
		SetOwnsValue(ETrue);
	} else {
		SetValue(0);
	}
	return this;
}

EXPORT_C void CBBGeneralHolder::SetOwnsValue(TBool aOwns)
{
	iOwnsValue=aOwns;
}

EXPORT_C CBBGeneralHolder::~CBBGeneralHolder()
{
	if (iOwnsValue) delete iValue;
}

EXPORT_C void CBBGeneralHolder::IntoStringL(TDes& aString) const
{
	if (iValue) iValue->IntoStringL(aString);
}

EXPORT_C void CBBGeneralHolder::IntoXmlL(MBBExternalizer* aBuf, TBool ) const
{
	if (iValue) iValue->IntoXmlL(aBuf, ETrue);
}

EXPORT_C void CBBGeneralHolder::SetName(const TDesC* aName)
{
	if (iValue) iValue->SetName(aName);
	iName=aName;
}

EXPORT_C void CBBGeneralHolder::ExternalizeL(RWriteStream& aStream) const
{
	if (iValue) {
		iValue->Type().ExternalizeL(aStream);
		iValue->ExternalizeL(aStream);
	} else {
		KNullType.ExternalizeL(aStream);
	}
}

EXPORT_C void CBBGeneralHolder::FromStringL(const TDesC& )
{
	Bug(_L("CBBGeneralHolder::FromStringL called")).Raise();
}

EXPORT_C MNestedXmlHandler* CBBGeneralHolder::FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
				HBufC*& aBuf, TBool aCheckType)
{
	if (!iValue) {
		User::Leave(KErrGeneral);
	}
	iValue->SetName(iName);
	return iValue->FromXmlL(aParent, aParser, aBuf, aCheckType);
}

// FIXME: should this be a parameter to the class?
_LIT(KGeneral, "general");

EXPORT_C void CBBGeneralHolder::InternalizeL(RReadStream& aStream)
{
	TTypeName t=TTypeName::IdFromStreamL(aStream);
	if (! (t==KNullType) ) {
		iValue=iFactory->CreateBBDataL(t, Name(), iFactory);
		iValue->SetName(iName);
		iValue->InternalizeL(aStream);
	}
}

EXPORT_C const TTypeName& CBBGeneralHolder::Type() const
{
	if (iValue) return iValue->Type();
	else return KNullType;
}

EXPORT_C TBool CBBGeneralHolder::Equals(const MBBData* aRhs) const
{
	if (!aRhs && !iValue) return ETrue;
	if (aRhs && iValue) return iValue->Equals(aRhs);
	return EFalse;
}

EXPORT_C void CBBGeneralHolder::SetValue(MBBData* aValue)
{
	if (iOwnsValue && aValue!=iValue) delete iValue;
	iValue=aValue;
	if (iValue)	iValue->SetName(iName);
}

const TDesC& CBBGeneralHolder::Name() const
{
	return *iName;
}

EXPORT_C MBBData::~MBBData()
{
}

EXPORT_C TBool MBBData::IsAttribute() const
{
	return EFalse;
}

EXPORT_C MBBData* CBBGeneralHolder::CloneL(const TDesC& Name) const
{
	MBBData* val=0;
	if (iValue) {
		val=iValue->CloneL(Name);
		CleanupPushBBDataL(val);
	}
	CBBGeneralHolder* ret=new (ELeave) CBBGeneralHolder(Name, iFactory, iValue);
	if (val) CleanupStack::Pop();
	return ret;
}

EXPORT_C TBool CBBGeneralHolder::OwnsValue() const
{
	return iOwnsValue;
}

void AppendComponentNameToString(const void* aComponentName, TDes& aString)
{
	const TComponentName& n=*( (const TComponentName*)aComponentName );
	aString.Append(_L("[ 0x"));
	aString.AppendNum(n.iModule.iUid, EHex);
	aString.Append(_L(" "));
	aString.AppendNum(n.iId);
	aString.Append(_L("]"));
}

EXPORT_C TComponentName::operator TStringArg() const
{
	return TStringArg(this, 2+12+1+2+10, AppendComponentNameToString);
}

void AppendTypeNameToString(const void* aTypeName, TDes& aString)
{
	const TTypeName& n=*( (const TTypeName*)aTypeName );
	n.IntoStringL(aString);
}

EXPORT_C TTypeName::operator TStringArg() const
{
	return TStringArg(this, 45, AppendTypeNameToString);
}

EXPORT_C void GetBackwardsCompatibleUid(TUint& uid3)
{
	switch(uid3) {
		case OLD_CONTEXT_UID_STARTER:
			uid3=CONTEXT_UID_STARTER;
			break;
		case OLD_CONTEXT_UID_CONTEXTSERVER:
			uid3=CONTEXT_UID_CONTEXTSERVER;
			break;
		case OLD_CONTEXT_UID_CONTEXTCLIENT:
			uid3=CONTEXT_UID_CONTEXTCLIENT;
			break;
		case OLD_CONTEXT_UID_CONTEXTCOMMON:
			uid3=CONTEXT_UID_CONTEXTCOMMON;
			break;
		case OLD_CONTEXT_UID_CONTEXTNOTIFY:
			uid3=CONTEXT_UID_CONTEXTNOTIFY;
			break;
		case OLD_CONTEXT_UID_CONTEXTNOTIFYCLIENT:
			uid3=CONTEXT_UID_CONTEXTNOTIFYCLIENT;
			break;
		case OLD_CONTEXT_UID_BLACKBOARDDATA:
			uid3=CONTEXT_UID_BLACKBOARDDATA;
			break;
		case OLD_CONTEXT_UID_BLACKBOARDSERVER:
			uid3=CONTEXT_UID_BLACKBOARDSERVER;
			break;
		case OLD_CONTEXT_UID_BLACKBOARDCLIENT:
			uid3=CONTEXT_UID_BLACKBOARDCLIENT;
			break;
		case OLD_CONTEXT_UID_BLACKBOARDFACTORY:
			uid3=CONTEXT_UID_BLACKBOARDFACTORY;
			break;
		case OLD_CONTEXT_UID_CONTEXTSENSORS:
			uid3=CONTEXT_UID_CONTEXTSENSORS;
			break;
		case OLD_CONTEXT_UID_SENSORDATAFACTORY:
			uid3=CONTEXT_UID_SENSORDATAFACTORY;
			break;
		case OLD_CONTEXT_UID_CONTEXTNETWORK:
			uid3=CONTEXT_UID_CONTEXTNETWORK;
			break;
		case OLD_CONTEXT_UID_CONTEXTCOMMON2:
			uid3=CONTEXT_UID_CONTEXTCOMMON2;
			break;
		case OLD_CONTEXT_UID_CONTEXTMEDIA:
			uid3=CONTEXT_UID_CONTEXTMEDIA;
			break;
		case OLD_CONTEXT_UID_CONTEXTMEDIAFACTORY:
			uid3=CONTEXT_UID_CONTEXTMEDIAFACTORY;
			break;
		case OLD_CONTEXT_UID_CONTEXTSENSORDATA:
			uid3=CONTEXT_UID_CONTEXTSENSORDATA;
			break;
		case OLD_CONTEXT_UID_CONTEXTUI:
			uid3=CONTEXT_UID_CONTEXTUI;
			break;
		case OLD_CONTEXT_UID_CONTEXTMEDIADATA:
			uid3=CONTEXT_UID_CONTEXTMEDIADATA;
			break;
		case OLD_CONTEXT_UID_CONTEXTMEDIAUI:
			uid3=CONTEXT_UID_CONTEXTMEDIAUI;
			break;
		case OLD_CONTEXT_UID_SHUTTER:
			uid3=CONTEXT_UID_SHUTTER;
			break;
		case OLD_CONTEXT_UID_EXPAT:
			uid3=CONTEXT_UID_EXPAT;
			break;
		case OLD_CONTEXT_UID_CONTEXTCONTACTSUI:
			uid3=CONTEXT_UID_CONTEXTCONTACTSUI;
			break;
		case OLD_CONTEXT_UID_CONTEXTCOMMSENSORS:
			uid3=CONTEXT_UID_CONTEXTCOMMSENSORS;
			break;
		case OLD2_CONTEXT_UID_STARTER:
			uid3=CONTEXT_UID_STARTER;
			break;
		case OLD2_CONTEXT_UID_CONTEXTSERVER:
			uid3=CONTEXT_UID_CONTEXTSERVER;
			break;
		case OLD2_CONTEXT_UID_CONTEXTCLIENT:
			uid3=CONTEXT_UID_CONTEXTCLIENT;
			break;
		case OLD2_CONTEXT_UID_CONTEXTCOMMON:
			uid3=CONTEXT_UID_CONTEXTCOMMON;
			break;
		case OLD2_CONTEXT_UID_CONTEXTNOTIFY:
			uid3=CONTEXT_UID_CONTEXTNOTIFY;
			break;
		case OLD2_CONTEXT_UID_CONTEXTNOTIFYCLIENT:
			uid3=CONTEXT_UID_CONTEXTNOTIFYCLIENT;
			break;
		case OLD2_CONTEXT_UID_BLACKBOARDDATA:
			uid3=CONTEXT_UID_BLACKBOARDDATA;
			break;
		case OLD2_CONTEXT_UID_BLACKBOARDSERVER:
			uid3=CONTEXT_UID_BLACKBOARDSERVER;
			break;
		case OLD2_CONTEXT_UID_BLACKBOARDCLIENT:
			uid3=CONTEXT_UID_BLACKBOARDCLIENT;
			break;
		case OLD2_CONTEXT_UID_BLACKBOARDFACTORY:
			uid3=CONTEXT_UID_BLACKBOARDFACTORY;
			break;
		case OLD2_CONTEXT_UID_CONTEXTSENSORS:
			uid3=CONTEXT_UID_CONTEXTSENSORS;
			break;
		case OLD2_CONTEXT_UID_SENSORDATAFACTORY:
			uid3=CONTEXT_UID_SENSORDATAFACTORY;
			break;
		case OLD2_CONTEXT_UID_CONTEXTNETWORK:
			uid3=CONTEXT_UID_CONTEXTNETWORK;
			break;
		case OLD2_CONTEXT_UID_CONTEXTCOMMON2:
			uid3=CONTEXT_UID_CONTEXTCOMMON2;
			break;
		case OLD2_CONTEXT_UID_CONTEXTMEDIA:
			uid3=CONTEXT_UID_CONTEXTMEDIA;
			break;
		case OLD2_CONTEXT_UID_CONTEXTMEDIAFACTORY:
			uid3=CONTEXT_UID_CONTEXTMEDIAFACTORY;
			break;
		case OLD2_CONTEXT_UID_CONTEXTSENSORDATA:
			uid3=CONTEXT_UID_CONTEXTSENSORDATA;
			break;
		case OLD2_CONTEXT_UID_CONTEXTUI:
			uid3=CONTEXT_UID_CONTEXTUI;
			break;
		case OLD2_CONTEXT_UID_CONTEXTMEDIADATA:
			uid3=CONTEXT_UID_CONTEXTMEDIADATA;
			break;
		case OLD2_CONTEXT_UID_CONTEXTMEDIAUI:
			uid3=CONTEXT_UID_CONTEXTMEDIAUI;
			break;
	}
}
