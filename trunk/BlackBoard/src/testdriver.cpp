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
#include "testdriver_bbdata_base.cpp"

#include "test_pair.cpp"
#include "bblist.h"
#include <s32mem.h>
#include "app_context_impl.h"
#include "bbtuple.h"

_LIT(KTestInt, "TestInt");
_LIT(KTestSS, "TestSS");
_LIT(KTestShortString, "TestSS");

void bbint()
{
	TBBInt t(1, KTestInt);
	test_conversions(1, t, _L("1"), _L("<x>1</x>"), _L(""), _L("x"), 1);
	test_conversions(3, t, _L("2"), _L("<x>2</x>"), _L(""), _L("x"), 2, false);

	int END=-2;
	int tests[]={ INT_MIN, -1023984, -56, -1, 0, 1, 2, 3, 14, 90849, INT_MAX, END };
	int i=0;
	while (tests[i]!=END) {
		TBuf<15> b; b.AppendNum(tests[i]);
		TBuf<25> xml1=_L("<x>");
		xml1.AppendNum(tests[i]);
		xml1.Append(_L("</x>"));
		TBuf<100> xml2=_L("<x module=\"0x1020811a\" id=\"1\" major_version=\"1\" minor_version=\"0\">");
		xml2.AppendNum(tests[i]);
		xml2.Append(_L("</x>"));
		
		test_conversions(tests[i], t, b, xml1, xml2, _L("x"), i+3);
		++i;
	}

	CTestXml* tx=0;
	CXmlParser* p=0;
	TBBInt t2(_L("TestInt"));

	_LIT(xml41, "<?xml version='1.0'?><TestInt module=\"0x1020811a\" id=\"1\" major_version=\"1\" minor_version=\"0\">14aa</TestInt>");
	tx=CTestXml::NewL(&t2, 0);
	p=CXmlParser::NewL(*tx);
	tx->SetParser(p);
	TInt err41;
	CC_TRAPIGNORE(err41, KCannotParseValue, p->Parse( (char*)(xml41().Ptr()), xml41().Size(), 1));
	TEST_EQUALS(err41, KCannotParseValue, _L("bbint:8"));
	delete p;
	delete tx;

	_LIT(xml42, "<?xml version='1.0'?><TestInt module=\"0x1020811a\" id=\"1\" major_version=\"1\" minor_version=\"0\">aa14</TestInt>");
	tx=CTestXml::NewL(&t2, 0);
	p=CXmlParser::NewL(*tx);
	tx->SetParser(p);
	TInt err42;
	CC_TRAPIGNORE(err42, KCannotParseValue, p->Parse( (char*)(xml42().Ptr()), xml42().Size(), 1));
	TEST_EQUALS(err42, KCannotParseValue, _L("bbint:8:0"));
	delete p;
	delete tx;

	_LIT(xml5, "<?xml version='1.0'?><TestInt module=\"0x1\" id=\"1\" major_version=\"1\" minor_version=\"0\">14</TestInt>");
	tx=CTestXml::NewL(&t2, 0);
	p=CXmlParser::NewL(*tx);
	tx->SetParser(p);
	TInt err5;
	CC_TRAPIGNORE(err5, KTypeDoesNotMatch, p->Parse( (char*)(xml5().Ptr()), xml5().Size(), 1));
	TEST_EQUALS(KTypeDoesNotMatch, err5, _L("bbint:8:1"));
	delete p;
	delete tx;

	TBuf<1> bsmall;
	TInt err6;
	CC_TRAPIGNORE(err6, KErrOverflow, t2.IntoStringL(bsmall));
	TEST_EQUALS(KErrOverflow, err6, _L("bbint:9:0"));
	t2=1;
	CC_TRAP(err6, t2.IntoStringL(bsmall));
	TEST_EQUALS(KErrNone, err6, _L("bbint:9:1"));
}

void bbbool()
{
	TBBBool t(ETrue, KTestInt);
	test_conversions(ETrue, t, _L("true"), _L("<x>true</x>"), _L(""), _L("x"), 1);
	t.iValue=EFalse;
	test_conversions(EFalse, t, _L("false"), _L("<x>false</x>"), _L(""), _L("x"), 2);
	t.iValue=ETrue;
	test_conversions(ETrue, t, _L("false"), _L("<x>false</x>"), _L(""), _L("x"), 3, false);
}


void bbshortstring()
{
	TBBShortString t(KTestSS);

	TBuf<50> tb=t();
	t()=tb;

	test_conversions(_L("jonohan tämä"), t, _L("jonohan tämä"), _L("<x>jonohan tämä</x>"), _L(""), _L("x"), 1);
	test_conversions(_L("jonohan t,ämä"), t, _L("jonohan tämä"), _L("<x>jonohan tämä</x>"), _L(""), _L("x"), 2, false);
	
	int END=-2;
	int tests[]={ 0, 1, 3, 49, 50, END };
	int i=0;
	while (tests[i]!=END) {
		TBuf<60> val; val.Fill('y', tests[i]);
		TBuf<70> xml1;
		TBuf<200> xml2;
		xml1=_L("<x>"); xml1.Append(val); xml1.Append(_L("</x>"));
		xml2=_L("<x module=\"0x1020811a\" id=\"2\" major_version=\"1\" minor_version=\"0\">");
		xml2.Append(val);
		xml2.Append(_L("</x>"));
		test_conversions(val, t, val, xml1, xml2, _L("x"), i+3);
		++i;
	}

	TBuf<1> bsmall;
	t=_L("bi");
	TInt err6;
	CC_TRAPIGNORE(err6, KErrOverflow, t.IntoStringL(bsmall));
	TEST_EQUALS(KErrOverflow, err6, _L("bbss:9:0"));

	TBuf<2> bsmall2;
	t=_L("bi");
	CC_TRAPD(err7, t.IntoStringL(bsmall2));
	TEST_EQUALS(KErrNone, err7, _L("bbss:9:1"));
}

#define BBPAIR_TESTS 2

void bbpair_inner(TInt test_num)
{
	_LIT(KPair, "pair");
	TBuf<30> test_name=_L("bbpair:"); test_name.AppendNum(test_num);
	TBBPair t(30, _L("string"), KPair);
	if (test_num==0) {
		TBuf<40> b;
		t.IntoStringL(b);
		TEST_EQUALS(b, _L("30 string"), test_name);
	} else if (test_num==1) {
		TBuf<100> xml=_L("<pair><first>30</first><second>string</second></pair>");
		test_conversions_inner_nonative(t, _L("30 string"), xml, _L(""), _L("pair"), test_num);
			
	}
}

void bbpair()
{
	TInt test_num=0;
	TBuf<30> num;
	for (test_num=0; test_num < BBPAIR_TESTS; test_num++) {
		TInt err=KErrNoMemory;
		TInt fail_on=1;
		while (err==KErrNoMemory) {
			User::__DbgSetAllocFail(RHeap::EUser, RHeap::EDeterministic, fail_on);
			User::__DbgMarkStart(RHeap::EUser);
			CC_TRAP(err, bbpair_inner(test_num));
			User::__DbgMarkEnd(RHeap::EUser,0);
			User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
			++fail_on;
		}
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
		num=_L("bbpair:memfail:"); num.AppendNum(test_num);
		TEST_EQUALS(err, KErrNone, num);
	}
}

const TTypeName KTestType = { { 0x10204BAF }, 1, 1, 0 };
_LIT(KCompoundTest, "ctest");
_LIT(KSpace, " ");
_LIT(KTX, "xx");
_LIT(KTY, "yy");

class TBBCompoundTest : public TBBCompoundData {
public:
	TBBInt			iX;
	TBBInt			iY;

	virtual const TTypeName& Type() const { return KTestType; }
	virtual TBool Equals(const MBBData* aRhs) const {
		const TBBCompoundTest *rhs=bb_cast<TBBCompoundTest>(aRhs);
		return (rhs && (*this)==(*rhs));
	}

	static const TTypeName& StaticType() { return KTestType; }
	const MBBData* Part(TUint aPartNo) const {
		if (aPartNo==0) return &iX;
		if (aPartNo==1) return &iY;
		return 0;
	}

	TBBCompoundTest(const TDesC& aName) : TBBCompoundData(aName), iX(KTX), iY(KTY) { }

	bool operator==(const TBBCompoundTest& rhs) const {
		return (iX()==rhs.iX() && iY()==rhs.iY());
	}
	virtual const TDesC& StringSep(TUint aBeforePart) const {
		return KSpace;
	}

	TBBCompoundTest& operator=(const TBBCompoundTest& aRhs) {
		iX()=aRhs.iX();
		iY()=aRhs.iY();
		return *this;
	}
	MBBData* CloneL(const TDesC& aName) const {
		TBBCompoundTest* ret=new (ELeave) TBBCompoundTest(aName);
		*ret=*this;
		return ret;
	}
};

void bbtuple_inner(TInt test_num)
{
	{
	TBBCompoundTest t(KCompoundTest);
	t.iX()=12;
	t.iX.SetIsAttribute(ETrue);
	t.iY()=-65;
	TBuf<200> xml2=_L("<ctest xx=\"12\"><yy>-65</yy></ctest>");
	test_conversions_inner_nonative(t, _L(" 12 -65 "), 
		xml2, _L(""), KCompoundTest, test_num);
	}
	{
		_LIT(Kns, "uri ct");
	TBBCompoundTest t(Kns);
	t.iX()=12;
	t.iX.SetIsAttribute(ETrue);
	t.iY()=-65;
	TBuf<200> xml2=_L("<ns0:ct xmlns:ns0=\"uri\" xx=\"12\"><yy>-65</yy></ns0:ct>");
	test_conversions_inner_nonative(t, _L(" 12 -65 "), 
		xml2, _L(""), Kns, test_num);
	}
	{
	TBuf<30> test_name=_L("bbtuple:"); test_name.AppendNum(test_num);
	TBBTupleMeta t(0x1001, 2002, _L("subname"));
	TBuf<200> xml=_L("<tuplename><module_uid>0x1001</module_uid>"
		L"<module_id>2002</module_id>"
		L"<subname>subname</subname></tuplename>");
	test_conversions_inner_nonative_named(t, _L("0x1001 2002 subname"), 
		xml, _L(""), _L("bbtuple"), test_num);
	}

	{
	TBBCompoundTest t(KCompoundTest);
	t.iX()=12;
	t.iY()=-65;
	TBuf<200> xml2=_L("<ctest><xx>12</xx><yy>-65</yy></ctest>");
	test_conversions_inner_nonative(t, _L(" 12 -65 "), 
		xml2, _L(""), KCompoundTest, test_num);
	}
}

void bbfactory_inner(TInt test_num, CBBDataFactory* f)
{
	_LIT(name, "ti");

	MBBData* d=f->CreateBBDataL(KIntType, name, 0);
	CleanupPushBBDataL(d);

	TBuf8<200> des;
	
	TBBInt wr(12, name);
	RDesWriteStream ws(des);
	CleanupClosePushL(ws);
	wr.Type().ExternalizeL(ws);
	wr.ExternalizeL(ws);
	ws.CommitL();
	CleanupStack::PopAndDestroy();

	RDesReadStream rs(des);
	CleanupClosePushL(rs);	
	TTypeName read_type=TTypeName::IdFromStreamL(rs);
	d->InternalizeL(rs);
	CleanupStack::PopAndDestroy();

	TBuf8<200> des2;
	RDesWriteStream ws2(des2);
	CleanupClosePushL(ws2);
	d->Type().ExternalizeL(ws2);
	d->ExternalizeL(ws2);
	ws2.CommitL();
	CleanupStack::PopAndDestroy();

	TEST_EQUALS(d->Type(), read_type, _L("bbfac:0"));
	TEST_EQUALS(TBinMatch8(des), TBinMatch8(des2), _L("bbfac:1"));

	CleanupStack::PopAndDestroy();
}

void bbfactory()
{
	TInt test_num=0;
	TBuf<30> num;
	CBBDataFactory* f=0;
	for (test_num=0; test_num < BBPAIR_TESTS; test_num++) {
		TInt err=KErrNoMemory;
		TInt fail_on=1;
		while (err==KErrNoMemory) {
			User::__DbgSetAllocFail(RHeap::EUser, RHeap::EDeterministic, fail_on);

			User::__DbgMarkStart(RHeap::EUser);
			CC_TRAP(err, f=CBBDataFactory::NewL());
			if (err==KErrNone) CC_TRAP(err, bbfactory_inner(test_num, f));
			//if (f) f->Reset();
			delete f; f=0;
			User::__DbgMarkEnd(RHeap::EUser,0);

			User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
			++fail_on;
		}
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
		num=_L("bbpair:memfail:"); num.AppendNum(test_num);
		TEST_EQUALS(err, KErrNone, num);
	}
	delete f;
}

void bblist_inner(TInt maxsize)
{
	_LIT(KListName, "list");
	_LIT(KSep, " ");
	
	TInt len[]={0, 1, 2, 3, 1000, -1};
	TInt i=0;
	teststate=_L("bblist::bbfac");
	auto_ptr<CBBDataFactory> fp(CBBDataFactory::NewL());
	CBBDataFactory* f=fp.get();
	while (len[i]>=0 && len[i]<maxsize) {
		teststate=_L("bblist::create1::"); testteststate.AppendNum(len[i]);
		auto_ptr<CBBGenericList> l(CBBGenericList::NewL(KListName, KListName, KSep, f));
		TInt j;
		teststate=_L("bblist::add_items::"); teststate.AppendNum(len[i]);
		for (j=0; j<len[i]; j++) {
			MBBData* d=new (ELeave) TBBInt(15, KListName);
			CleanupPushBBDataL(d);
			l->AddItemL(0, d);
			CleanupStack::Pop();
		}
		TEST_EQUALS(l->Count(), len[i], state);
		teststate=_L("bblist::create2::"); teststate.AppendNum(len[i]);
		auto_ptr<CBBGenericList> l2(CBBGenericList::NewL(KListName, KListName, KSep, f));

		{
			auto_ptr<HBufC8> desp(HBufC8::NewL(100*1000+100));
			TPtr8 des=desp->Des();
			
			teststate=_L("bblist::write::"); teststate.AppendNum(len[i]);
			RDesWriteStream ws(des);
			CleanupClosePushL(ws);
			l->ExternalizeL(ws);
			CleanupStack::PopAndDestroy();

			RDesReadStream rs(des);
			CleanupClosePushL(rs);	
			l2->InternalizeL(rs);
			CleanupStack::PopAndDestroy();
		}
		teststate=_L("bblist::comp1::"); teststate.AppendNum(len[i]);
		TEST_EQUALS(l2->Count(), len[i], state);
		teststate=_L("bblist::comp2::"); teststate.AppendNum(len[i]);
		TEST_EQUALS(*l, *l2, state);
		{
			teststate=_L("bblist::xml1::"); teststate.AppendNum(len[i]);
			auto_ptr<CXmlBufExternalizer> buf(CXmlBufExternalizer::NewL(200*1000));
			l->IntoXmlL(buf.get(), EFalse);
			auto_ptr<CBBGenericList> l3(CBBGenericList::NewL(KListName, KListName, KSep, f));

			teststate=_L("bblist::xml2::"); teststate.AppendNum(len[i]);
			auto_ptr<CTestXml> tx(CTestXml::NewL(l3.get(), 0, false));
			auto_ptr<CXmlParser> p(CXmlParser::NewL(*tx));
			tx->SetParser(p.get());
			p->Parse( (char*)(buf->Buf().Ptr()), buf->Buf().Size(), 1);

			teststate=_L("bblist::comp_xml::"); teststate.AppendNum(len[i]);
			TEST_EQUALS(*l, *l3, state);
		}

		i++;
	}
}

class RunList : public MRunnable
{
public:
	TInt maxsize;
	virtual void run() {  bblist_inner(maxsize); }
};


void bblist()
{
	User::__DbgMarkStart(RHeap::EUser);

	RunList r;
	r.maxsize=2000;
	r.run();

	User::__DbgMarkEnd(RHeap::EUser,0);

	r.maxsize=15;
	test_oom(r);
}

void run_tests()
{
	//__asm int 3;

	User::__DbgMarkStart(RHeap::EUser);
	{
		output=new (ELeave) MOutput;
		auto_ptr<CApp_context> c(CApp_context::NewL(false, _L("bbtest")));

		RAFs fs; fs.ConnectLA();
		output->foutput.Replace(fs, _L("blackboardtest.txt"), EFileWrite);

		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
		TInt err;

		CC_TRAPIGNORE(err, KErrNoMemory, bbtuple_inner(1));
		TEST_EQUALS(err, KErrNone, state);

		if (1) {
			CC_TRAPIGNORE(err, KErrNoMemory, bbbool());
			TEST_EQUALS(err, KErrNone, state);

			CC_TRAPIGNORE(err, KErrNoMemory, bblist());
			TEST_EQUALS(err, KErrNone, state);

			CC_TRAP(err, bbfactory());
			TEST_EQUALS(err, KErrNone, _L("bbfactory all"));

			CC_TRAP(err, bbint());
			TEST_EQUALS(err, KErrNone, _L("bbint all"));
			CC_TRAP(err, bbshortstring());
			TEST_EQUALS(err, KErrNone, _L("bbshortstring all"));
			CC_TRAP(err, bbpair());
			TEST_EQUALS(err, KErrNone, _L("bbpair all"));
		}

		TBuf<30> b=_L("OK: "); b.AppendNum(ok); 
		b.Append(_L("/")); b.AppendNum(ok+not_ok); b.Append(_L("\n"));
		output->Write(b);
		output->Getch();
		delete output->cons;

		output->foutput.Close();
		delete output;
	}
	User::__DbgMarkEnd(RHeap::EUser,0);
}
