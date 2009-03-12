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
#pragma warning(disable: 4800)
#pragma warning(disable: 4702)

#include <e32cons.h>
#include <f32file.h>
#include <s32strm.h>
#include "xml.h"
#include <limits.h>
#include "symbian_auto_ptr.h"
#include "concretedata.h"
#include "bblist.h"
#include "reporting.h"

class MOutput : public MActiveErrorReporter{
public:
	CConsoleBase* cons;
	RFile	foutput;
	MActiveErrorReporter* iAnotherReporter;

	MOutput() : cons(0), iAnotherReporter(0) { }

	void Write(const TDesC& str) {
		if (cons) {
			
			cons->Write(str);
		}
		if (foutput.SubSessionHandle()!=0) {
			TInt len=str.Length()*2;
			TPtrC8 p( (const TUint8*)str.Ptr(), len );
			foutput.Write(p);
			foutput.Flush();
		}
	}
	TBuf<2000> msg;
	virtual void ReportError(const TDesC& Source,
		const TDesC& Reason, TInt Code) 
	{
		if (iAnotherReporter) {
			iAnotherReporter->ReportError(Source,
				Reason, Code);
		}
		msg.Append(Source);
		msg.Append(Reason);
		msg.AppendNum(Code);
	}

	virtual void LogFormatted(const TDesC& aMsg)
	{
		if (iAnotherReporter) {
			iAnotherReporter->LogFormatted(aMsg);
		}
		msg.Append(aMsg);
	}
	virtual void SetInHandlableEvent(TBool ) { }

	void Getch() {
		if (cons) cons->Getch();
	}
};

MOutput* output=0;

TInt ok=0;
TInt not_ok=0;

TBuf<100> test_state;

template<typename T>
bool operator!=(const T& aLhs, const T& aRhs) { return !(aLhs==aRhs); }

template<typename T>
void WRITE_VALUES(const T& lhs, const T& rhs) {
	TBuf<200> s1, s2;
	lhs.IntoStringL(s1);
	rhs.IntoStringL(s2);
	output->Write(s1);
	output->Write(_L(" "));
	output->Write(s2);
}

template<>
void WRITE_VALUES(const TDesC& lhs, const TDesC& rhs) {
	output->Write(lhs);
	output->Write(_L(" "));
	output->Write(rhs);
}

template<>
void WRITE_VALUES(const MBBData& lhs, const MBBData& rhs) {
	TBuf<200> b=_L("|");
	CC_TRAPD(err, lhs.IntoStringL(b));
	output->Write(b);
	output->Write(_L("| |"));
#ifdef __WINS__
	RDebug::Print(b);
#endif
	b.Zero();
	CC_TRAP(err, rhs.IntoStringL(b));
	b.Append(_L("|"));
	output->Write(b);
#ifdef __WINS__
	RDebug::Print(b);
#endif
}

template<>
void WRITE_VALUES<TTypeName>(const TTypeName& lhs, const TTypeName& rhs) {
	TBuf<200> b;
	b=_L("[");
	b.AppendNum((TInt)lhs.iModule.iUid); b.Append(_L(" "));
	b.AppendNum(lhs.iId); b.Append(_L(" "));
	b.AppendNum(lhs.iMajorVersion); b.Append(_L(" "));
	b.AppendNum(lhs.iMinorVersion); b.Append(_L("]"));
	output->Write(b);

	output->Write(_L(" "));

	b=_L("[");
	b.AppendNum((TInt)rhs.iModule.iUid); b.Append(_L(" "));
	b.AppendNum(rhs.iId); b.Append(_L(" "));
	b.AppendNum(rhs.iMajorVersion); b.Append(_L(" "));
	b.AppendNum(rhs.iMinorVersion); b.Append(_L("]"));
	output->Write(b);
}
template<>
void WRITE_VALUES<int>(const int& lhs, const int& rhs) {
	TBuf<15> b;
	b.AppendNum(lhs);
	output->Write(b);
	output->Write(_L(" "));
	b.Zero();
	b.AppendNum(rhs);
	output->Write(b);
}

template<>
void WRITE_VALUES<TTupleName>(const TTupleName& lhs, const TTupleName& rhs) {
	TBuf<200> b;
	b.AppendNum((TInt)lhs.iModule.iUid);
	b.Append(_L(" "));
	b.AppendNum(lhs.iId);
	b.Append(_L(" "));

	output->Write(b);
	output->Write(_L(" "));
	b.Zero();
	b.AppendNum((TInt)rhs.iModule.iUid);
	b.Append(_L(" "));
	b.AppendNum(rhs.iId);
	b.Append(_L(" "));
	
	output->Write(b);
}

bool operator==(const CBBGenericList& lhs, const CBBGenericList& rhs) {
	return lhs.Equals(&rhs);
}

template<typename T, typename T1>
bool TEST_EQUALS(const T& lhs, const T1& rhs, const TDesC& name, bool match=true) {
	TBuf<70> b;
	TBool fail=EFalse;
	if (name.Length()<20) {
		b.Format(_L("%-20S"), &name);
	} else {
		b.Append(name.Left(50));
	}
	if ((match && lhs==rhs) || (!match && !(lhs==rhs))) {
		++ok; return true;
		b.Append(_L(": OK"));
	} else {
		++not_ok;
		fail=ETrue;
		b.Append(_L(": ERROR "));
	}
	output->Write(b);
	if (fail) WRITE_VALUES(lhs, rhs);
	output->Write(_L("\n"));
	return false;
}

template<typename T, typename T1>
void TEST_NOT_EQUALS(const T& lhs, const T1& rhs, const TDesC& name) {
	TBuf<40> b;
	b.Format(_L("%-20S"), &name);
	TBool fail=EFalse;
	if (lhs!=rhs) {
		++ok; return;
		b.Append(_L(": OK"));
	} else {
		++not_ok;
		b.Append(_L(": ERROR "));
		fail=ETrue;
	}
	output->Write(b);
	if (fail) WRITE_VALUES(lhs, rhs);
	output->Write(_L("\n"));
}


const TDesC& test_name(TDes& buf, int num1, int num2)
{
	buf=_L("test:");
	buf.AppendNum(num1);
	buf.Append(_L(":"));
	buf.AppendNum(num2);
	return buf;
}

void run_tests();

GLDEF_C int E32Main(void)
{	
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	CC_TRAPD(err, run_tests());
	delete cleanupStack;
	return 0;
}

class TStringMatch8 {
public:
	const TDesC8& s;
	bool operator==(const TStringMatch8& rhs) const {
		if (s.Compare(rhs.s)) return false;
		return true;
	}
	TStringMatch8(const TDesC8& as) : s(as) { }
};

class TStringMatch {
public:
	const TDesC& s;
	bool operator==(const TStringMatch& rhs) const {
		if (s.Compare(rhs.s)) return false;
		return true;
	}
	TStringMatch(const TDesC& as) : s(as) { }
};

void WRITE_VALUES(const void* lhs, const void* rhs) {
        TBuf<20> b;
	b.AppendNum((TInt)lhs);
        output->Write(b);
        output->Write(_L(" "));
	b.Zero();
	b.AppendNum((TInt)rhs);
        output->Write(b);
}

void WRITE_VALUES(const TDesC8& lhs, const TDesC8& rhs) {
        TBuf<100> b;
	b.Copy(lhs.Left(100));
        output->Write(b);
        output->Write(_L(" "));
	b.Copy(rhs.Left(100));
        output->Write(b);
}

void WRITE_VALUES(const TStringMatch& lhs, const TStringMatch& rhs) {
	WRITE_VALUES(lhs.s, rhs.s);
}

void WRITE_VALUES(const TStringMatch8& lhs, const TStringMatch8& rhs) {
	WRITE_VALUES(lhs.s, rhs.s);
}


class TBinMatch8 {
public:
        const TDesC8& s;
        bool operator==(const TBinMatch8& rhs) const {
		if (rhs.s.Length()!=s.Length()) return false;
		TInt i;
		for(i=0; i<rhs.s.Length(); i++) {
			if (rhs.s[i]!=s[i]) return false;
		}
		return true;

        }
        TBinMatch8(const TDesC8& as) : s(as) { }
};


void WRITE_VALUES(const TBinMatch8&, const TBinMatch8&) {
	output->Write(_L("binary strings differ"));
}

void test_for_error_leave(TInt aOccurred, TInt aExpected, const TDesC& name)
{
	if (aOccurred!=aExpected) User::LeaveIfError(aOccurred);
	TEST_EQUALS(aOccurred, aExpected, name);
}

class MRunnable {
public:
	virtual void run() = 0;
};

void test_oom(MRunnable& r)
{
	void* p=User::Alloc(16*1024*1024);
	User::Free(p);

	TInt err=0;
	TInt fail_on=1;
	err=KErrNoMemory;
	while (err==KErrNoMemory) {
		if (fail_on==13) {
			TInt x;
			x=0;
		}
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::EDeterministic, fail_on);
		User::__DbgMarkStart(RHeap::EUser);
		CC_TRAPIGNORE(err, KErrNoMemory, r.run());
		User::__DbgMarkEnd(RHeap::EUser,0);
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
		++fail_on;
	}
	User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
	test_state.Append(_L(":oom_")); test_state.AppendNum(fail_on);
	TEST_EQUALS(err, KErrNone, test_state);
}

class MRunnable2 {
public:
	virtual void run() = 0;
	virtual void stop() = 0;
};

void test_oom2(MRunnable2& r, TInt aMinFail=0, TInt start_with=1)
{
#ifdef __WINS__
	// grow heap so that memory leak addresses
	// are available early
	void* p=User::Alloc(16*1024*1024);
	User::Free(p);
#endif

	TInt err=0;
	TInt fail_on=start_with;
	err=KErrNoMemory;

	//fail_on=108;
	while (err==KErrNoMemory || (fail_on<aMinFail && err==KErrNone) ) {
		TBuf<20> m;
		m.AppendNum(fail_on); m.Append(_L("\n"));
		//output->Write(m);
		RDebug::Print(m);
		if (fail_on==109) {
			TInt x;
			x=0;
		}
		User::__DbgMarkStart(RHeap::EUser);
		{
			User::__DbgSetAllocFail(RHeap::EUser, RHeap::EDeterministic, fail_on);
			CC_TRAP(err, r.run());
			User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
			r.stop();
		}
		User::__DbgMarkEnd(RHeap::EUser,0);
		++fail_on;
	}
	User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
	test_state.Append(_L(":oom_")); test_state.AppendNum(fail_on);
	TEST_EQUALS(err, KErrNone, test_state);
}
