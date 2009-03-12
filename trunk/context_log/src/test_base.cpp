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
#include <e32std.h>
#include "test_log.h"
#include "current_loc.h"
#include <e32cons.h>
#include "app_context.h"
#include "cl_settings.h"

TTime now;
void GetTime(TTime& t)
{
	t=now;
}

struct TConsoleOutput : public MOutput
{
	TConsoleOutput(CConsoleBase* console) : iConsole(console) { }
	void Print(const TDesC& aString) {
		iConsole->Printf(aString);
	}

	CConsoleBase*	iConsole;
};

void delete_bufc(void* data)
{
        HBufC* p;
        p=(HBufC*)data;
        delete p;
}

bool base_tests(MApp_context& ctx, MOutput& aOutput, int retest)
{
	int retest_count=1;

#define RECREATE	{ CleanupStack::PopAndDestroy(); CCurrentLoc* loc=CCurrentLoc::NewL(ctx, cellid_names); CleanupStack::PushL(loc); loc->add_sinkL(l); }
#define RETEST(interval) { now+=TTimeIntervalMinutes(interval); test_time=now; if(retest==0 || retest_count==retest) { RECREATE  l->Expect(prev_e); test_time-=TTimeIntervalMinutes(interval); } ++retest_count; }

	CGenericIntMap* cellid_names=CGenericIntMap::NewL();
	CleanupStack::PushL(cellid_names);
	cellid_names->SetDeletor(delete_bufc);

	TTime test_time;

	int i;
	for (i=1; i<100; i++) {
		TBuf<10> ib;
		ib.AppendNum(i);
		cellid_names->AddDataL(i, (void*)ib.AllocL());
	}
	
	CTestLog *l=CTestLog::NewL(aOutput);
	CleanupStack::PushL(l);

	CCurrentLoc* loc=CCurrentLoc::NewL(ctx, cellid_names);
	CleanupStack::PushL(loc);

	loc->add_sinkL(l);

	now=TTime(1);

	loc->EmptyLog();

	_LIT(cell, "1, 1, T");

	CTestLog::TExpectItem prev_e;

	aOutput.Print(_L("start at a base\n"));
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: "), TTime(0));
	l->Expect(Mlogger::VALUE, _L("base"), _L("1"), now);
	loc->now_at_location(cell, 1, true, true, now);
	l->GotP();

	aOutput.Print(_L("move out\n"));
	now+=TTimeIntervalMinutes(15);
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 1"), now);
	loc->now_at_location(cell, 2, false, true, now);
	l->GotP();
	RETEST(0)
	loc->now_at_location(cell, 2, false, true, now);
	l->GotP();

	aOutput.Print(_L("come to another non-base cell\n"));
	RETEST(1)
	loc->now_at_location(cell, 3, false, true, now);

	aOutput.Print(_L("come to a new base\n"));
	RETEST(10)
	l->Expect(Mlogger::VALUE, _L("base"), _L("4"), now);
	loc->now_at_location(cell, 4, true, true, now);
	l->GotP();
	aOutput.Print(_L("stay in base, different cell\n"));
	RETEST(1)
	loc->now_at_location(cell, 5, true, false, now);

	aOutput.Print(_L("move out\n"));
	RETEST(15)
	TTime ppprev=now;
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 4"), test_time);
	loc->now_at_location(cell, 6, false, true, now);
	l->GotP();
	aOutput.Print(_L("move out 2\n"));
	RETEST(5)
	loc->now_at_location(cell, 7, false, true, now);

	aOutput.Print(_L("notice this cell is a base\n"));
	TTime pprev=now;
	RETEST(0)
	l->Expect(Mlogger::VALUE, _L("base"), _L("7"), now);
	loc->now_at_location(cell, 7, true, false, now);
	l->GotP();

	aOutput.Print(_L("move out\n"));
	RETEST(15)
	TTime prev=test_time;
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 7"), test_time);
	loc->now_at_location(cell, 8, false, true, now);
	RETEST(5)
	loc->now_at_location(cell, 9, false, true, now);
	l->GotP();

	aOutput.Print(_L("come to a base\n"));
	RETEST(10)
	l->Expect(Mlogger::VALUE, _L("base"), _L("10"), now);
	loc->now_at_location(cell, 10, true, true, now);
	l->GotP();
	aOutput.Print(_L("leave quickly\n"));
	RETEST(1)
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 7"), prev);
	loc->now_at_location(cell, 11, false, true, now);
	l->GotP();

	aOutput.Print(_L("come back to previous after long time\n"));
	RETEST(1)
	l->Expect(Mlogger::VALUE, _L("base"), _L("7"), now);
	loc->now_at_location(cell, 7, true, true, now);
	l->GotP();
	now+=TTimeIntervalMinutes(20);
	loc->now_at_location(cell, 7, true, false, now);

	aOutput.Print(_L("leave\n"));
	RETEST(30)
	pprev=test_time;
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 7"), test_time);
	loc->now_at_location(cell, 12, false, true, now);
	l->GotP();
	aOutput.Print(_L("enter new base\n"));
	RETEST(30)
	prev=now;
	l->Expect(Mlogger::VALUE, _L("base"), _L("13"), now);
	loc->now_at_location(cell, 13, true, true, now);
	l->GotP();
	now+=TTimeIntervalMinutes(20);
	loc->now_at_location(cell, 13, true, false, now);
	aOutput.Print(_L("leave\n"));
	RETEST(10)
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 13"), now);
	loc->now_at_location(cell, 14, false, true, now);
	l->GotP();
	aOutput.Print(_L("come back quickly\n"));
	RETEST(8)
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 7"), pprev);
	l->Expect(Mlogger::VALUE, _L("base"), _L("13"), prev);
	loc->now_at_location(cell, 13, true, true, now);
	l->GotP();
	now+=TTimeIntervalMinutes(20);
	loc->now_at_location(cell, 13, true, false, now);
	aOutput.Print(_L("move out, to another base\n"));
	RETEST(10)
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 13"), now);
	l->Expect(Mlogger::VALUE, _L("base"), _L("15"), now);
	loc->now_at_location(cell, 15, true, true, now);
	l->GotP();
	aOutput.Print(_L("come back quickly\n"));
	RETEST(8)
	prev_e=l->Expect(Mlogger::VALUE, _L("base"), _L("last: 7"), pprev);
	l->Expect(Mlogger::VALUE, _L("base"), _L("13"), prev);
	loc->now_at_location(cell, 13, true, true, now);
	l->GotP();

	RECREATE
	prev=now;
	now+=TTimeIntervalMinutes(30);
	l->Expect(prev_e);
	l->Expect(Mlogger::VALUE, _L("base"), _L("last: 13"), prev);
	loc->now_at_location(cell, 16, false, true, now);
	l->GotP();

	CleanupStack::PopAndDestroy(3);

	if (retest>0 && retest_count <= retest) return false;
	return true;
}

void run_tests()
{
        __UHEAP_MARK;

	CConsoleBase* console=0;
	console=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(console);

	TConsoleOutput Output(console);
	Output.Print(_L("starting...\n\n"));

	CApp_context* iContext=CApp_context::NewL(true, _L("tester"));
	CleanupStack::PushL(iContext);
	iContext->SetDataDir(_L("c:\\system\\data\\context\\"), false);
	iContext->SetAppDir(_L("c:\\system\\apps\\context_log\\"));
	
	TClSettings iDefaultSettings;
	CSettings* s=CSettings::NewL(*iContext, iDefaultSettings, _L(""));
	iContext->SetSettings(s);

	int retest=-1;
	while (base_tests(*iContext, Output, retest)) ++retest;

	CleanupStack::PopAndDestroy(2);

        __UHEAP_MARKEND;
}

int E32Main(void)
{
        CTrapCleanup* cleanupStack = CTrapCleanup::New();
        CC_TRAPD(err, run_tests());
        delete cleanupStack;
        return 0;
}
