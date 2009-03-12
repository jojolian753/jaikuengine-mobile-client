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
#include "bbxml.h"
#include "bberrors.h"
#include "bbtypes.h"
#include "app_context_impl.h"
#include "reporting.h"

class CXmlBufExternalizerImpl : public CXmlBufExternalizer
{
private:
	virtual void BeginList(const TDesC& aName, TBool aIncludeBBType, const TTypeName& aBBType);
	virtual void BeginCompound(const TDesC& aName, TBool aIncludeBBType, const TTypeName& aBBType);
	virtual void Field(const TDesC& aName, TBasicType aBasicTyype, const TDesC& aValue,
		TBool aIncludeBBType, const TTypeName& aBBType,
		TBool aAsAttribute=EFalse);

	virtual void EndCompound(const TDesC& aName);
	virtual void EndList(const TDesC& aName);

	void BeginElement(const TDesC& aName,
		TBool aIncludeBBType, const TTypeName& aBBType);

	TBool iInOpenElement;
};

EXPORT_C CXmlBufExternalizer* CXmlBufExternalizer::NewL(TInt aInitialSize)
{
	auto_ptr<CXmlBufExternalizerImpl> ret(new (ELeave) CXmlBufExternalizerImpl);
	ret->ConstructL(aInitialSize);
	return ret.release();
}

void CXmlBufExternalizerImpl::BeginElement(const TDesC& aName, 
										   TBool aIncludeBBType, 
										   const TTypeName& aBBType)
{
	if (iInOpenElement) { iInOpenElement=EFalse; CloseOpenElement(); }

	if (aIncludeBBType) {
		MDesCArray* attrs=aBBType.MakeAttributesLC();
		CXmlBuf::BeginElement(aName, attrs);
		CleanupStack::PopAndDestroy();
	} else {
		CXmlBuf::BeginOpenElement(aName);
		iInOpenElement=ETrue;
	}
}

void CXmlBufExternalizerImpl::BeginList(const TDesC& aName, TBool aIncludeBBType, const TTypeName& aBBType)
{
	BeginElement(aName, aIncludeBBType, aBBType);
}

void CXmlBufExternalizerImpl::BeginCompound(const TDesC& aName, TBool aIncludeBBType, const TTypeName& aBBType)
{
	BeginElement(aName, aIncludeBBType, aBBType);
}

void CXmlBufExternalizerImpl::Field(const TDesC& aName, TBasicType, const TDesC& aValue,
	TBool aIncludeBBType, const TTypeName& aBBType,
		TBool aAsAttribute)
{
	if (! aAsAttribute ) {
		BeginElement(aName, aIncludeBBType, aBBType);
		if (iInOpenElement) { iInOpenElement=EFalse; CloseOpenElement(); }
		CXmlBuf::Characters(aValue);
		EndElement(aName);
	} else {
		if (aIncludeBBType) User::Leave(KErrGeneral);
		Attribute(aName, aValue);
	}
}

void CXmlBufExternalizerImpl::EndCompound(const TDesC& aName)
{
	if (iInOpenElement) { iInOpenElement=EFalse; CloseOpenElement(); }
	EndElement(aName);
}
void CXmlBufExternalizerImpl::EndList(const TDesC& aName)
{
	if (iInOpenElement) { iInOpenElement=EFalse; CloseOpenElement(); }
	EndElement(aName);
}

EXPORT_C TSingleXml::TSingleXml(MNestedXmlHandler* aParent, CXmlParser* aParser, HBufC*& aBuf, MBBData& aValue, TBool aCheckType) : TBBXml(aParent, aParser, aBuf, aValue, aCheckType)
{
}

EXPORT_C TCheckingXml::TCheckingXml(MNestedXmlHandler* aParent, CXmlParser* aParser) :
		MNestedXmlHandler(aParent), iIgnore(this, aParser), iIgnoreElement(EFalse), iParser(aParser)
{
}

EXPORT_C TBBXml::TBBXml(MNestedXmlHandler* aParent, CXmlParser* aParser, 
		HBufC*& aBuf, MBBData& aValue, TBool aCheckType) : 
		TCheckingXml(aParent, aParser), iBuf(aBuf), 
		iValue(aValue), iCheckType(aCheckType)
{
	if (iBuf) {
		iBuf->Des().Zero();
	} 
}
EXPORT_C CContainerXml::CContainerXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, MBBData& aValue, TBool aCheckType) :
		TBBXml(aParent, aParser, aBuf, aValue, aCheckType)
{
}

EXPORT_C void TCheckingXml::StartElement(const XML_Char * name,
 	const XML_Char ** atts)
{
	TInt err;
	CC_TRAPIGNORE(err, KErrNoMemory, StartElementL(name, atts));
	if (err!=KErrNone) {
		ErrorInHandler(err);
		// ErrorInHandler throws, if the top-level parser thinks that's
		// necessary. If not, we can ignore the element
		iIgnoreElement=ETrue;
		iIgnore.StartElement(name, atts);
		iParser->SetHandler(&iIgnore);
	}
}

_LIT(KContextNS, "http://www.cs.helsinki.fi/group/context");
EXPORT_C TPtrC RemoveContextNamespace(const XML_Char* name)
{
	TPtrC n((const TUint16*)name);
	if (n.Left( KContextNS().Length()).Compare( KContextNS ) == 0) {
		return n.Mid( KContextNS().Length() +1 );
	}
	return n;
}

EXPORT_C void TSingleXml::StartElementL(const XML_Char * name,
 	const XML_Char ** atts)
{
	TPtrC n=RemoveContextNamespace(name);
	if (n.Compare(iValue.Name()) ) {
		User::Leave(KNameDoesNotMatch);
	}
	if (iCheckType) {
		TTypeName read_type=TTypeName::IdFromAttributes(atts);
		read_type.CompareMajorL(iValue.Type());
	}
}

EXPORT_C void TCheckingXml::EndElement(const XML_Char * name)
{
	TInt err;
	CC_TRAPIGNORE(err, KErrNoMemory, EndElementL(name));
	if (err!=KErrNone) {
		iIgnoreElement=ETrue;
		ErrorInHandler(err);
	}
	iParent->EndElement(name);
	iParser->SetHandler(iParent);
}


EXPORT_C void TSingleXml::EndElementL(const XML_Char * /*name*/)
{
	if (!iIgnoreElement) {
		if (iBuf) {
			iBuf->Des().Trim();
			CC_TRAPD(err, iValue.FromStringL(*iBuf));
			if (err!=KErrNone) {
				MApp_context* c=GetContext();
				if (c) {
					c->Reporting().UserErrorLog(_L("Cannot parse value '"),
						*iBuf, _L("'"));
				}
				User::Leave(err);
			}
		} else {
			iValue.FromStringL(KNullDesC);
		}
	}
}

EXPORT_C void TCheckingXml::CharacterData(const XML_Char *s,
 	    int len)
{
	TInt err;
	CC_TRAPIGNORE(err, KErrNoMemory, CharacterDataL(s, len));
	if (err!=KErrNone) {
		ErrorInHandler(err);
		iIgnoreElement=ETrue;
		iIgnore.StartElement(0, 0);
		iParser->SetHandler(&iIgnore);
	}
}

EXPORT_C void TCheckingXml::ErrorInHandler(TInt aError)
{
	iParent->SetError(aError);
}

EXPORT_C void TSingleXml::CharacterDataL(const XML_Char *s,
 	    int len)
{
	if (!iBuf) {
		iBuf=HBufC::NewL(len*2);
	}
	while (iBuf->Length() + len > iBuf->Des().MaxLength()) {
		iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength() * 2);
	}
	TPtrC p((const TUint16*)s, len);
	iBuf->Des().Append(p);
}

EXPORT_C void TSingleXml::Error(XML_Error Code, const XML_LChar * String, long ByteIndex)
{
	iParent->Error(Code, String, ByteIndex);
}

EXPORT_C void TSingleXml::SetError(TInt aError)
{
	iParent->SetError(aError);
}

EXPORT_C void CContainerXml::StartElementL(const XML_Char *name,
			const XML_Char **atts)
{
	// the depth has to be before doing possible Leave's, so that if an error
	// occurs and TCheckingXml sets an TIgnore to eat the element,
	// the EndElement will work on the right depth
	if (iDepth==0) {
		++iDepth;
		TPtrC n=RemoveContextNamespace(name);
		if (n.Compare(iValue.Name()) ) {
			User::Leave(KNameDoesNotMatch);
		}
		if (iCheckType) {
			TTypeName read_type=TTypeName::IdFromAttributes(atts);
			read_type.CompareMajorL(iValue.Type());
		}
		HandleAttributesL(name, atts);

	} else {
		++iDepth;
		StartInnerElementL(name, atts);
	}
}

EXPORT_C void CContainerXml::CharacterDataL(const XML_Char * /*s*/, int /*len*/)
{
	// ignore
}

EXPORT_C void CContainerXml::EndElementL(const XML_Char *s)
{
	if (iDepth==2) {
		EndInnerElementL(s);
	}
}

EXPORT_C void CContainerXml::EndElement(const XML_Char *s)
{
	TInt err;
	CC_TRAPIGNORE(err, KErrNoMemory, EndElementL(s));
	if (err!=KErrNone) {
		ErrorInHandler(err);
	}
	--iDepth;
	if (iDepth==0) {
		iParent->EndElement(s);
		iParser->SetHandler(iParent);
	}
}


EXPORT_C void CContainerXml::SetCurrentHandler(MNestedXmlHandler* aHandler)
{
	delete iCurrentHandler;
	iCurrentHandler=aHandler;
	iParser->SetHandler(aHandler);
}

EXPORT_C MNestedXmlHandler* CContainerXml::GetCurrentHandler()
{
	return iCurrentHandler;
}

EXPORT_C CContainerXml::~CContainerXml()
{
	delete iCurrentHandler;
}

EXPORT_C void CContainerXml::HandleAttributesL(const XML_Char *name, const XML_Char **atts)
{
	// do nothing
}

EXPORT_C CPartContainerXml::CPartContainerXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
					      HBufC*& aBuf, TBBCompoundData& aValue, TBool aCheckType) :
	CContainerXml(aParent, aParser, aBuf, aValue,  aCheckType), iCompoundValue(aValue), iErrorsInCurrent(EFalse)
{
}

EXPORT_C void CPartContainerXml::HandleAttributesL(const XML_Char *name, const XML_Char **atts)
{
	TUint part;
	const XML_Char** ap=&atts[0];
	while ( *ap && *(ap+1)) {
		TPtrC att((const TUint16*)*ap), val((const TUint16*)*(ap+1));

		if (att.Compare(KAttModule) && att.Compare(KAttId) &&
			att.Compare(KAttMajorVersion) && att.Compare(KAttMinorVersion)) {

			MBBData *p=iCompoundValue.GetPart(att,
				KNoType, part);
			if (p) p->FromStringL(val);
		}
		ap+=2;
	}
}

EXPORT_C void CPartContainerXml::StartInnerElementL(const XML_Char *name, 
		const XML_Char **atts)
{
	//RDebug::Print(_L("CPartContainerXml::StartInnerElementL"));

	TUint partno;
	TPtrC namep=RemoveContextNamespace(name);
	MBBData *p=iCompoundValue.GetPart(namep,
		TTypeName::IdFromAttributes(atts), partno);
	iCurrentPart=partno;
#ifdef __WINS__
	if (namep.Compare(_L("eventdata"))==0) {
		TInt x;
		x=0;
	}
#endif
	if (p) {
		SetCurrentHandler(p->FromXmlL(this, iParser, iBuf, iCheckType) );
		GetCurrentHandler()->StartElement(name, atts);
	} else {
		User::Leave(KUnexpectedElement);
	}
}

EXPORT_C void CPartContainerXml::EndInnerElementL(const XML_Char *)
{
	//RDebug::Print(_L("CPartContainerXml::EndInnerElementL"));

	if (iCurrentPart>=0 && !iIgnoreElement) 
		iCompoundValue.ReadPart(iCurrentPart, iErrorsInCurrent);
	iErrorsInCurrent=EFalse;
}

EXPORT_C void CPartContainerXml::Error(XML_Error Code, const XML_LChar * /*String*/, long /*ByteIndex*/)
{
	SetError(Code);
}

EXPORT_C void CPartContainerXml::SetError(TInt aError)
{
	iErrorsInCurrent=ETrue;
	if (!iParent) User::Leave(aError);
	else iParent->SetError(aError);
}

class CSingleParserImpl : public CSingleParser, public MNestedXmlHandler {
public:
	MBBData* iData; 
	MNestedXmlHandler* iCurrentHandler;
	CXmlParser* iParser;
	HBufC* iBuf;
	TBool  iCheckType;
	TBool	iIgnoreUnknown;
	MBBData* iOwnedData;
	MBBDataFactory* iFactory;
	HBufC*	iName;
	
	void SetParser(CXmlParser* aParser) { iParser=aParser; }
	CSingleParserImpl(MBBData* aData, TBool  aCheckType,
		TBool aIgnoreUnknown, MBBDataFactory* aFactory=0) : MNestedXmlHandler(0), iData(aData), iCheckType(aCheckType),
		iIgnoreUnknown(aIgnoreUnknown), iFactory(aFactory) { }
	void ConstructL() {
		iParser=CXmlParser::NewL(*this);
	}
	~CSingleParserImpl() { 
		delete iBuf; delete iCurrentHandler; delete iParser;
		delete iOwnedData; delete iName;
	}

	virtual void ParseL(const TDesC8& aXml) {
		iParser->Parse( (char*)(aXml.Ptr()), aXml.Size(), 0);
	}

	virtual void StartElement(const XML_Char *name,
				const XML_Char **atts) {

		if (!iData) {
			TTypeName tn=TTypeName::IdFromAttributes(atts);
			TPtrC namep(name);
			iName=namep.AllocL();
			iOwnedData=iFactory->CreateBBDataL(tn, *iName, iFactory);
			iData=iOwnedData;
		}
		if (!iCurrentHandler) iCurrentHandler=iData->FromXmlL(this, iParser, iBuf, iCheckType);
		else User::Leave(KErrGeneral);

		if (iCurrentHandler) iCurrentHandler->StartElement(name, atts);
		iParser->SetHandler(iCurrentHandler);
	}
		
	virtual void EndElement(const XML_Char * /*name*/) {
	}

	virtual void CharacterData(const XML_Char * /*s*/,
				    int /*len*/) {
		User::Leave(KErrGeneral);
	}

	virtual void Error(XML_Error /*Code*/, const XML_LChar * /*String*/, long /*ByteIndex*/) {
		User::Leave(KErrInvalidXml);
	}
	virtual void SetError(TInt aError) { 
		if (iIgnoreUnknown && aError==KUnexpectedElement) return;
		User::Leave(aError); 
	}
	virtual MBBData* Data() {
		return iData;
	}
};

EXPORT_C CSingleParser* CSingleParser::NewL(MBBData* aParseInto, TBool  aCheckType,
					    TBool aIgnoreUnknown, MBBDataFactory* aFactory)
{
	CSingleParserImpl *ret=new (ELeave) CSingleParserImpl(aParseInto, aCheckType, 
		aIgnoreUnknown, aFactory);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop();
	return ret;
}


EXPORT_C TIgnoreXml::TIgnoreXml(MNestedXmlHandler* aParent, CXmlParser* aParser) : 
	MNestedXmlHandler(aParent), iDepth(0), iParser(aParser) { }

void TIgnoreXml::SetError(TInt aError)
{
	// cannot get called
}

void TIgnoreXml::StartElement(const XML_Char *name,
			const XML_Char **atts)
{
	++iDepth;
}

void TIgnoreXml::EndElement(const XML_Char *name)
{
	--iDepth;
	if (iDepth==0) {
		iParser->SetHandler(iParent);
		iParent->EndElement(name);
	}
}

void TIgnoreXml::CharacterData(const XML_Char *s,
			    int len)
{
}

void TIgnoreXml::Error(XML_Error Code, const XML_LChar * String, long ByteIndex)
{
}
