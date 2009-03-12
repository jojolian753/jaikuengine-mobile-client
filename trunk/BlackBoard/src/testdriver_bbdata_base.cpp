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
#define __BOOL_NO_TRUE_TRAP__
#include <e32std.h>
int operator==(TBool x, enum TTrue y) { return ((x && y) || (!x && !y)); }
int operator==(enum TTrue y, TBool x) { return ((x && y) || (!x && !y)); }
template<typename T1, typename T2>
int operator!=(T1 x, T2 y) { return !(operator==(x, y)); }

#include "..\..\BlackBoard\src\testdriver_base.cpp"

#include "bberrors.h"
#include "bbxml.h"


#include "raii_f32file.h"

class CTestXml : public MNestedXmlHandler, public CBase {
public:
	MBBData* iData; MNestedXmlHandler* iCurrentHandler;
	CXmlParser* iParser;
	bool iCheckType;
	HBufC* iBuf;
	
	void SetParser(CXmlParser* aParser) { iParser=aParser; }
	CTestXml(MBBData* aData, CXmlParser* aParser, 
		bool aCheckType=true) : iData(aData), iParser(aParser), iCheckType(aCheckType) { }
	void ConstructL() { }
	~CTestXml() { delete iBuf; delete iCurrentHandler; }

	virtual void StartElement(const XML_Char *name,
				const XML_Char **atts) {
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
	static CTestXml* NewL(MBBData* aData, CXmlParser* aParser, bool aCheckType=true) {
		CTestXml *ret=new (ELeave) CTestXml(aData, aParser, aCheckType);
		CleanupStack::PushL(ret);
		ret->ConstructL();
		CleanupStack::Pop();
		return ret;
	}
	virtual void SetError(TInt aError) { 
		User::Leave(aError); 
	}
};

template<typename BB>
void test_conversions_inner_nonative_2(const BB& t,
	const TDesC& string, const TDesC& xml, const TDesC& xml2, 
	const TDesC& , int test_num, bool match,
	BB& read, BB& t2)
{
	TBuf<20> test;
	TBuf<100> b;

	t.IntoStringL(b);
	TEST_EQUALS(b, string, test_name(test, test_num, 1), match);
	auto_ptr<CXmlBufExternalizer> xb(CXmlBufExternalizer::NewL(40));
	t.IntoXmlL(xb.get(), EFalse);
	TEST_EQUALS( xb->Buf(), xml, test_name(test, test_num, 2), match);
	xb.reset();
	if (xml2.Length()>0) {
		xb.reset(CXmlBufExternalizer::NewL(40));
		t.IntoXmlL(xb.get(), ETrue);
		TEST_EQUALS( xb->Buf(), xml2, test_name(test, test_num, 3), match);
		xb.reset();
	}

	if (match) {
		TBuf8<2000> des;
		
		RDesWriteStream ws(des);
		CleanupClosePushL(ws);
		t.Type().ExternalizeL(ws);
		t.ExternalizeL(ws);
		ws.CommitL();
		CleanupStack::PopAndDestroy();

		RDesReadStream rs(des);
		CleanupClosePushL(rs);
		TTypeName read_type=TTypeName::IdFromStreamL(rs);
		read.InternalizeL(rs);
		CleanupStack::PopAndDestroy();
		TEST_EQUALS(t.Type(), read_type, test_name(test, test_num, 40));
		TEST_EQUALS(t, read, test_name(test, test_num, 4));
	}

	TBuf<800> xmlfull=_L("<?xml version='1.0'?>");
	xmlfull.Append(xml);
	for (int j=0; j<2; j++) {
		auto_ptr<CTestXml> tx(CTestXml::NewL(&t2, 0, j));
		auto_ptr<CXmlParser> p(CXmlParser::NewL(*tx));
		tx->SetParser(p.get());
		p->Parse( (char*)(xmlfull.Ptr()), xmlfull.Size(), 1);
		TEST_EQUALS(t, t2, test_name(test, test_num, j+5), match);
		if (xml2.Length()>0) {
			xmlfull=_L("<?xml version='1.0'?>");
			xmlfull.Append(xml2);
		} else  {
			break;
		}
	}
}

template<typename BB>
void test_conversions_inner_nonative(const BB& t,
	const TDesC& string, const TDesC& xml, const TDesC& xml2, 
	const TDesC& name, int test_num, bool match=true)
{
	BB read(name);
	BB t2(name);
	test_conversions_inner_nonative_2(t, string, xml, xml2, name, test_num, match, read, t2);
}
	
template<typename BB>
void test_conversions_inner_nonative_named(const BB& t,
	const TDesC& string, const TDesC& xml, const TDesC& xml2, 
	const TDesC& name, int test_num, bool match=true)
{
	BB read;
	BB t2;
	test_conversions_inner_nonative_2(t, string, xml, xml2, name, test_num, match, read, t2);
}
	
template<typename Builtin, typename BB>
void test_conversions_inner(const Builtin& val, const BB& /* dummy */,
	const TDesC& string, const TDesC& xml, const TDesC& xml2, 
	const TDesC& name, int test_num, bool match=true)
{
	TBuf<20> test;
	BB t(val, name);

	if (match) {
		TEST_EQUALS(t.iValue, val, test_name(test, test_num, 0));
	}
	test_conversions_inner_nonative(t, string, xml, xml2, name, test_num, match);

}

template<typename Builtin, typename BB>
void test_conversions(const Builtin& val, const BB& dummy,
	const TDesC& string, const TDesC& xml, const TDesC& xml2, 
	const TDesC& name, int test_num, bool match=true)
{
	TBuf<20> test;
	TInt err=KErrNoMemory;
	TInt fail_on=1;
	TInt rerun=-1;
	while (err==KErrNoMemory || err==KErrOverflow) {
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::EDeterministic, fail_on);
		User::__DbgMarkStart(RHeap::EUser);
		if (test_num==6 && fail_on==6) {
			TInt x;
			x=0;
		}
		CC_TRAPIGNORE(err, KErrNoMemory, test_conversions_inner(val, dummy, string, xml,
			xml2, name, test_num, match));
		User::__DbgMarkEnd(RHeap::EUser,0);
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
		if (err!=KErrNoMemory && err!=KErrNone) {
			fail_on+=rerun;
			err=KErrNoMemory;
			rerun=0;
		}
		++fail_on;
	}
	User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
	TEST_EQUALS(err, KErrNone, test_name(test, test_num, 0));
}



