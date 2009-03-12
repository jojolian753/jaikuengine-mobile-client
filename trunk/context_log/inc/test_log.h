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

#ifndef CL_TEST_LOG_H_INCLUDED
#define CL_TEST_LOG_H_INCLUDED 1

#include <e32base.h>
#include "i_logger.h"

class MOutput {
public:
	virtual void Print(const TDesC& aString) = 0;
};

class CTestLog : public Mlogger, public CBase
{
public:
	static CTestLog* NewL(MOutput& aOutput);
	virtual ~CTestLog();

	struct TExpectItem {
		log_priority	iPriority;
		TBuf<60>	iSource;
		TBuf<100>	iString;
		TTime		iTime;

		TExpectItem(log_priority aPriority, const TDesC& aSource,
			const TDesC& aString, const TTime aTime) :
			iPriority(aPriority), iSource(aSource),
			iString(aString), iTime(aTime) { }
		TExpectItem() : iPriority(), iSource(), iString(), iTime() { }
		void Print(MOutput& Into) {
			Into.Print(iSource);
			Into.Print(_L(": '"));
			Into.Print(iString);
			Into.Print(_L("' ("));
			switch (iPriority) {
			case INFO:
				Into.Print(_L("INFO"));
				break;
			case DEBUG:
				Into.Print(_L("DEBUG"));
				break;
			case VALUE:
				Into.Print(_L("VALUE"));
				break;
			case ERR:
				Into.Print(_L("ERR"));
				break;
			}
			Into.Print(_L(" "));
			TBuf<30> dt;
			dt.AppendNum(iTime.Int64());
			Into.Print(dt);
			Into.Print(_L(")"));
		}
		bool operator==(const TExpectItem i) const {
			if (	iPriority==i.iPriority &&
				!(iSource.Compare(i.iSource)) &&
				iTime==i.iTime &&
				!(iString.Compare(i.iString)) ) return true;
			return false;
		}
	};
	virtual TExpectItem& Expect(log_priority aPriority, const TDesC& aSource,
		const TDesC& aString, const TTime aTime) = 0;
	virtual TExpectItem& Expect(TExpectItem e) = 0;
	virtual void DontExpect() = 0;
	virtual void GotP() = 0;


};

#endif
