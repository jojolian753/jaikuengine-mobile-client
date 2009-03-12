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

#include "bblist.h"
#include "symbian_auto_ptr.h"
#include "bbxml.h"
#include "bbutil.h"
#include "bbtypes.h"
#include "bberrors.h"

class CListXml : public CContainerXml {
public:
	CListXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, CBBGenericList& aValue, TBool aCheckType) : 
		CContainerXml(aParent, aParser, aBuf, aValue, aCheckType), iValue(aValue) { }
private:
	CBBGenericList& iValue;
	TBool	iCurrentError;
	virtual void StartInnerElementL(const XML_Char *name, const XML_Char **atts) {
		//RDebug::Print(_L("CListXml::StartInnerElementL"));
		iCurrentError=EFalse;
		TPtrC namep((const TUint16*)name);
		delete iValue.iCurrentName; iValue.iCurrentName=0;
		iValue.iCurrentName=namep.AllocL();

		TTypeName n=TTypeName::IdFromAttributes(atts);
		if (!iValue.FixedType() && (n.iModule.iUid==-1 || n.iId==-1) ) {
			InputErr(_L("Datatype attributes expected for element <%1 %2 >")).
				UserMsg(name, atts).
				ErrorCode(BBErrorCode(KTypeRequiredOnElement)).Raise();
		}

		delete iValue.iCurrentData; iValue.iCurrentData=0;
		iValue.iCurrentData=iValue.iFactory->CreateBBDataL(n, *iValue.iCurrentName, iValue.iFactory);

		SetCurrentHandler( iValue.iCurrentData->FromXmlL(this, iParser,
			iBuf, iCheckType) );
		GetCurrentHandler()->StartElement(name, atts);
	}

	virtual void EndInnerElementL(const XML_Char * /*name*/) {
		//RDebug::Print(_L("CListXml::EndInnerElementL"));
		if (!iCurrentError) {
			iValue.AddCurrentItemL();
		} else {
			delete iValue.iCurrentName; iValue.iCurrentName=0;
			delete iValue.iCurrentData; iValue.iCurrentData=0;
		}
	}

	virtual void Error(XML_Error Code, const XML_LChar * /*String*/, long /*ByteIndex*/) {
		SetError(Code);
	}
	virtual void SetError(TInt aError) {
		iCurrentError=ETrue;
		iParent->SetError(aError);
	}
};

EXPORT_C CBBGenericList* CBBGenericList::NewL(const TDesC& aName, const TDesC& aChildName,
					      const TDesC& aStringSep, MBBDataFactory* aFactory)
{
	auto_ptr<CBBGenericList> ret(new (ELeave) CBBGenericList(aName, aChildName, aStringSep, aFactory));
	ret->ConstructL();
	return ret.release();
}
EXPORT_C void CBBGenericList::SetName(const TDesC* aName)
{
	iName=aName;
}

EXPORT_C CBBGenericList::CBBGenericList(const TDesC& aName, const TDesC& aChildName, const TDesC& aStringSep, MBBDataFactory* aFactory) :
	iName(&aName), iChildName(aChildName), iStringSep(aStringSep), iFactory(aFactory) { }

EXPORT_C CBBGenericList::~CBBGenericList()
{
	Reset();
	delete iData;
	delete iCurrentName;
	delete iCurrentData;
}

EXPORT_C const TDesC& CBBGenericList::Name() const
{
	return *iName;
}

EXPORT_C void CBBGenericList::Reset()
{
	if (iData) {
		while(iData->iCurrent) {
			TDataItem d=iData->iCurrent->Item;
			delete d.iName; delete d.iData;
			iData->DeleteLast();
		}
	}
}

EXPORT_C void CBBGenericList::IntoStringL(TDes& aString) const
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("IntoStringL"));

	CList<TDataItem>::Node* n;
	for (n=iData->iFirst; n; n=n->Next) {
		if (n!=iData->iFirst) {
			CheckStringSpaceL(aString, iStringSep.Length());
			aString.Append(iStringSep);
		}
		n->Item.iData->IntoStringL(aString);
	}
}

EXPORT_C void CBBGenericList::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("IntoXmlL"));

	aBuf->BeginList(Name(), aIncludeType, Type());
	CList<TDataItem>::Node* n;
	TBool include_type_on_children=ETrue;
	if (FixedType()) include_type_on_children=EFalse;
	for (n=iData->iFirst; n; n=n->Next) {
		n->Item.iData->IntoXmlL(aBuf, include_type_on_children);
	}
	aBuf->EndList(Name());
}

EXPORT_C void CBBGenericList::ExternalizeL(RWriteStream& aStream) const
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("ExternalizeL"));

	aStream.WriteInt32L( iData->iCount );
	CList<TDataItem>::Node* n;
	TBool iFirst=ETrue;
	for (n=iData->iFirst; n; n=n->Next) {
		if (!FixedType() || iFirst) {
			n->Item.iData->Type().ExternalizeL(aStream);
			iFirst=EFalse;
		}
		n->Item.iData->ExternalizeL(aStream);
	}
}

EXPORT_C void CBBGenericList::FromStringL(const TDesC& )
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("FromStringL"));

	PlainErr(KErrNotSupported).Raise();
}

EXPORT_C MNestedXmlHandler* CBBGenericList::FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
				HBufC*& aBuf, TBool aCheckType)
{
	return new(ELeave) CListXml(aParent, aParser, aBuf, *this, aCheckType);
}

EXPORT_C void CBBGenericList::InternalizeL(RReadStream& aStream)
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("InternalizeL"));

	TInt count=aStream.ReadInt32L();
	TInt i;
	TTypeName t, ind;
	for (i=0; i<count; i++) {
		MBBData* d=0;
		if (!FixedType() || i==0) {
			t=TTypeName::IdFromStreamL(aStream);
		} 
		d=iFactory->CreateBBDataL(t, iChildName, iFactory);
		CleanupPushBBDataL(d);
		d->InternalizeL(aStream);
		AddItemL(0, d);
		CleanupStack::Pop();
	}
}

EXPORT_C const TTypeName& CBBGenericList::Type() const
{
	return KListType;
}

EXPORT_C const TTypeName& CBBGenericList::StaticType()
{
	return KListType;
}

EXPORT_C void CBBGenericList::AddItemL(HBufC* aName, MBBData* aData) // takes ownership
{
	iData->AppendL(TDataItem(aName, aData));
}

void CBBGenericList::AddCurrentItemL()
{
	AddItemL(iCurrentName, iCurrentData);
	iCurrentName=0;
	iCurrentData=0;
}

EXPORT_C void CBBGenericList::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("ConstructL"));

	iData=CList<TDataItem>::NewL();
}

EXPORT_C TInt CBBGenericList::Count() const
{
	return iData->iCount;
}

EXPORT_C MBBData* CBBGenericList::First()
{
	iIterator=iData->iFirst;
	if (iIterator) return iIterator->Item.iData;
	return 0;
}

EXPORT_C MBBData* CBBGenericList::Next()
{
	if (iIterator) {
		iIterator=iIterator->Next;
	}
	if (iIterator) return iIterator->Item.iData;
	return 0;
}

EXPORT_C const MBBData* CBBGenericList::First() const
{
	iIterator=iData->iFirst;
	if (iIterator) return iIterator->Item.iData;
	return 0;
}

EXPORT_C const MBBData* CBBGenericList::Next() const
{
	if (iIterator) {
		iIterator=iIterator->Next;
	}
	if (iIterator) return iIterator->Item.iData;
	return 0;
}

EXPORT_C TBool CBBGenericList::Equals(const MBBData* aRhs) const
{
	if (!aRhs) return EFalse;
	if (! (aRhs->Type() == Type()) ) return EFalse;
	const CBBGenericList* rhs=static_cast<const CBBGenericList*>(aRhs);
	const MBBData *l, *r;

	TInt lcount=Count(), rcount=rhs->Count();
	if (lcount!=rcount) return EFalse;

	for (l=First(), r=rhs->First(); l && r; l=Next(), r=rhs->Next()) {
		if ( ! l->Equals(r) ) return EFalse;
	}
	if (l || r) return EFalse;
	return ETrue;
}

EXPORT_C TBool CBBGenericList::FixedType() const
{
	return EFalse;
}

EXPORT_C MBBData* CBBGenericList::CloneL(const TDesC& Name) const
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("CloneL"));

	auto_ptr<CBBGenericList> list(CBBGenericList::NewL(Name, iChildName, iStringSep, iFactory));
	CList<TDataItem>::Node* n;
	for (n=iData->iFirst; n; n=n->Next) {
		
		if (n->Item.iName) {
			auto_ptr<HBufC> name(n->Item.iName->AllocL());
			MBBData* val=n->Item.iData->CloneL(*name);
			CleanupPushBBDataL(val);
			list->AddItemL(name.get(), val);
			CleanupStack::Pop();
			name.release();
		} else {
			MBBData* val=n->Item.iData->CloneL(n->Item.iData->Name());
			CleanupPushBBDataL(val);
			list->AddItemL(0, val);
			CleanupStack::Pop();
		}
	}
	return list.release();
}

EXPORT_C CBBGenericList& CBBGenericList::operator=(const CBBGenericList& aRhs)
{
	CALLSTACKITEM_N(_CL("CBBGenericList"), _CL("operator"));

	CBBGenericList* l=bb_cast<CBBGenericList>(aRhs.CBBGenericList::CloneL(aRhs.Name()));
	Reset();
	delete iData; iData=0;
	iData=l->iData; l->iData=0;
	delete l;
	return *this;
}

IMPLEMENT_ASSIGN(CBBGenericList)
