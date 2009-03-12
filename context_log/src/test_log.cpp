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

#include "test_log.h"
#include "list.h"
#include "symbian_auto_ptr.h"

class CTestLogImpl : public CTestLog
{
public:
	CTestLogImpl(MOutput& aOutput);
	virtual ~CTestLogImpl();
	void ConstructL();
	virtual void register_source(const TDesC& name, const TDesC& initial_value, const TTime& time);
	virtual void new_value(log_priority priority, const TDesC& name, const TDesC& value, const TTime& time);
	virtual void unregister_source(const TDesC& name, const TTime& time);
	virtual const TDesC& name() const;

	virtual TExpectItem& Expect(log_priority aPriority, const TDesC& aSource,
		const TDesC& aString, const TTime aTime);
	virtual TExpectItem& Expect(TExpectItem e);
	virtual void DontExpect();
	virtual void GotP();

	CList<TExpectItem> 	*iToExpect;
	MOutput&		iOutput;
};

CTestLog::~CTestLog()
{
}

CTestLog* CTestLog::NewL(MOutput& aOutput)
{
	auto_ptr<CTestLogImpl> ret(new (ELeave) CTestLogImpl(aOutput));
	ret->ConstructL();
	return ret.release();
}

CTestLogImpl::CTestLogImpl(MOutput& aOutput) : iOutput(aOutput)
{
}

CTestLogImpl::~CTestLogImpl()
{
	delete iToExpect;
}
void CTestLogImpl::ConstructL()
{
	iToExpect=CList<TExpectItem>::NewL();
}

void CTestLogImpl::register_source(const TDesC& /*name*/, 
	const TDesC& /*initial_value*/, const TTime& /*time*/)
{
}

void CTestLogImpl::new_value(log_priority priority, const TDesC& name, const TDesC& value, const TTime& time)
{
	TExpectItem got(priority, name, value, time);

	if (iToExpect->iCount==0) {
		iOutput.Print(_L("ERR: "));
		iOutput.Print(_L("Expected:- nothing; "));
	} else {
		TExpectItem e=iToExpect->Pop();
		if (e==got) {
			iOutput.Print(_L("OK: "));
		} else {
			iOutput.Print(_L("ERR: Expected:-"));
			e.Print(iOutput);
			iOutput.Print(_L("; "));
		}
	}
	iOutput.Print(_L("Got:- "));
	got.Print(iOutput);
	iOutput.Print(_L("\n"));
}

void CTestLogImpl::unregister_source(const TDesC& /*name*/, const TTime& /*time*/)
{
}

const TDesC& CTestLogImpl::name() const
{
	_LIT(KName, "CTestLogImpl");
	return KName;
}

CTestLogImpl::TExpectItem& CTestLogImpl::Expect(log_priority aPriority, const TDesC& aSource,
	const TDesC& aString, const TTime aTime)
{
	TExpectItem e(aPriority, aSource, aString, aTime);
	return Expect(e);
}

CTestLogImpl::TExpectItem& CTestLogImpl::Expect(TExpectItem e)
{
	iToExpect->AppendL(e);
	return iToExpect->iCurrent->Item;
}

void CTestLogImpl::DontExpect()
{
	iToExpect->reset();
}

void CTestLogImpl::GotP()
{
	while(iToExpect->iCount>0) {
		TExpectItem e=iToExpect->Pop();
		iOutput.Print(_L("ERR: Expected:-"));
		e.Print(iOutput);
		iOutput.Print(_L("; Got:- nothing\n"));
	}
}
