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

#include "bb_incoming.h"
#include "xml.h"
#include "symbian_auto_ptr.h"

EXPORT_C const TTypeName& TBBAck::Type() const
{
	CALLSTACKITEM_N(_CL("TBBAck"), _CL("Type"));

	return KAckType;
}

EXPORT_C TBool TBBAck::Equals(const MBBData* aRhs) const
{
	CALLSTACKITEM_N(_CL("TBBAck"), _CL("Equals"));

	const TBBAck* rhs=bb_cast<TBBAck>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBAck::StaticType()
{
	CALLSTACKITEM_N(_CL("TBBAck"), _CL("StaticType"));

	return KAckType;
}

EXPORT_C const MBBData* TBBAck::Part(TUint aPartNo) const
{
	CALLSTACKITEM_N(_CL("TBBAck"), _CL("Part"));

	if (aPartNo==0) return &iId;
	return 0;
}

EXPORT_C TBBAck::TBBAck() : TBBCompoundData(KAck), iId(KId)
{
	CALLSTACKITEM_N(_CL("TBBAck"), _CL("TBBAck"));

}

EXPORT_C MBBData* TBBAck::CloneL(const TDesC&) const
{
	TBBAck* ret=new (ELeave) TBBAck;
	ret->iId()=iId();
	return ret;
}

const TDesC& TBBAck::StringSep(TUint) const
{
	CALLSTACKITEM_N(_CL("TBBAck"), _CL("StringSep"));

	return KNullDesC;
}

EXPORT_C bool TBBAck::operator==(const TBBAck& aRhs) const
{
	CALLSTACKITEM_N(_CL("TBBAck"), _CL("operator"));

	return (iId==aRhs.iId);
}

class CStreamXmlImpl : public CStreamXml {
public:
	MBBData* iData; MNestedXmlHandler* iCurrentHandler;
	MIncomingObserver& iObserver;
	CXmlParser* iParser;
	bool iCheckType;
	HBufC* iBuf;

	CStreamXmlImpl(MBBData* aData, MIncomingObserver& aObserver,
		bool aCheckType=true) : iData(aData), iObserver(aObserver), 
		iCheckType(aCheckType) { }
		void ConstructL() { iParser=CXmlParser::NewL(*this); }
		~CStreamXmlImpl() { delete iBuf; delete iCurrentHandler; delete iParser; }

		virtual void StartElement(const XML_Char *name,
			const XML_Char **atts) {

				TPtrC namep( (TUint16*)name);
				if (namep.Compare(iData->Name())==0) {
					iObserver.StreamOpened();
				} else {
					User::Leave(KUnexpectedElement);
				}
				if (!iCurrentHandler) iCurrentHandler=iData->FromXmlL(this, iParser, iBuf, iCheckType);
				else User::Leave(KErrGeneral);

				if (iCurrentHandler) iCurrentHandler->StartElement(name, atts);
				iParser->SetHandler(iCurrentHandler);
			}

		virtual CXmlParser* Parser() {
			return iParser;
		}

		virtual void EndElement(const XML_Char * /*name*/) {
			iObserver.StreamClosed();
		}

		virtual void CharacterData(const XML_Char * /*s*/,
			int /*len*/) {
				User::Leave(KErrGeneral);
			}

		virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex) {
			SetError(KErrInvalidXml);
			User::Leave(KErrInvalidXml);
		}
		virtual void SetError(TInt aError) { 
			iObserver.StreamError(aError, _L(""));
		}

};


EXPORT_C CStreamXml* CStreamXml::NewL(MBBData* aData, MIncomingObserver& aObserver,
									  bool aCheckType)
{
	auto_ptr<CStreamXmlImpl> ret(new (ELeave) CStreamXmlImpl(aData, aObserver, aCheckType));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C MBBData* MBBStream::CloneL(const TDesC&) const { return 0; }

class CStreamImpl : public CStream, public MContextBase
{
private:
	CStreamImpl(MIncomingObserver& aObserver, MApp_context& aContext);
	void ConstructL();
	~CStreamImpl();

	const MBBData* Part(TUint aPartNo) const;
	virtual const TTypeName& Type() const;
	virtual void ResetPart(TUint aPart);

	TBBAck	iAck;
	CBBTuple* iTuple;

	friend class CStream;
	friend class auto_ptr<CStreamImpl>;
};

EXPORT_C CStream* CStream::NewL(MIncomingObserver& aObserver, MApp_context& aContext)
{
	CALLSTACKITEM_N(_CL("CStream"), _CL("NewL"));

	auto_ptr<CStreamImpl> ret(new (ELeave) CStreamImpl(aObserver, aContext));
	ret->ConstructL();
	return ret.release();
}

_LIT(KStream, "stream");

EXPORT_C MBBStream::MBBStream(const TDesC& aName, MIncomingObserver& aObserver) : TBBCompoundData(aName),
iObserver(aObserver) { }

CStreamImpl::CStreamImpl(MIncomingObserver& aObserver, MApp_context& aContext) : CStream(aObserver),
MContextBase(aContext) { }

CStream::CStream(MIncomingObserver& aObserver) : MBBStream(KStream, aObserver) { }

void CStreamImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CStreamImpl"), _CL("ConstructL"));
	MBBStream::ConstructL();
	iTuple=new (ELeave) CBBTuple(BBDataFactory());
}

CStreamImpl::~CStreamImpl()
{
	delete iTuple;
}

void CStreamImpl::ResetPart(TUint aPartNo)
{
	if (aPartNo==0) {
		iAck.iId()=0;
	} else if (aPartNo==1) {
		iTuple->iData.SetValue(0);
		iTuple->iTupleId()=0;
		iTuple->iTupleMeta.iModuleUid()=0;
		iTuple->iTupleMeta.iModuleId()=0;
		iTuple->iTupleMeta.iSubName().Zero();
	}
}

EXPORT_C void MBBStream::ConstructL()
{
	iStreamXml=CStreamXml::NewL(this, iObserver, false);
}

EXPORT_C void MBBStream::Reset()
{
	delete iStreamXml; iStreamXml=0;
	iStreamXml=CStreamXml::NewL(this, iObserver, false);
}

EXPORT_C MBBStream::~MBBStream()
{
	CALLSTACKITEM_N(_CL("MBBStream"), _CL("~MBBStream"));

	delete iStreamXml;
}

EXPORT_C void MBBStream::ParseL(const TDesC8& aXml)
{
	CALLSTACKITEM_N(_CL("MBBStream"), _CL("ParseL"));

	//Log(aXml);
	iStreamXml->Parser()->Parse( (char*)(aXml.Ptr()), aXml.Size(), 0);
}

const TTypeName& CStreamImpl::Type() const
{
	CALLSTACKITEM_N(_CL("CStreamImpl"), _CL("Type"));

	return KStreamType;
}

EXPORT_C TBool MBBStream::Equals(const MBBData* ) const { return EFalse; }

const MBBData* CStreamImpl::Part(TUint aPartNo) const
{
	CALLSTACKITEM_N(_CL("CStreamImpl"), _CL("Part"));

	if (aPartNo==0) return &iAck;
	if (aPartNo==1) return iTuple;
	return 0;
}

EXPORT_C const TDesC& MBBStream::StringSep(TUint ) const
{
	CALLSTACKITEM_N(_CL("MBBStream"), _CL("StringSep"));

	return KNullDesC;
}

EXPORT_C void MBBStream::ReadPart(TUint aPart, TBool aErrors)
{
	CALLSTACKITEM_N(_CL("MBBStream"), _CL("ReadPart"));

	iObserver.IncomingData(Part(aPart), aErrors);
	ResetPart(aPart);
}
