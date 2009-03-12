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
#include "..\..\BlackBoard\src\testdriver_base.cpp"
#include "blackboardclientsession.h"

#include "raii_blackboardclientsession.h"
#include "raii_f32file.h"

#include "context_uids.h"
#include "bbtypes.h"

const TTupleName KAnySensorTuple = { { CONTEXT_UID_CONTEXTSENSORS }, KBBAnyId };

void test_notify(MApp_context* c)
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("DeleteL"));

	RABBClient bbc; bbc.ConnectLA();

	TInt count;
	TRequestStatus s;
	bbc.AddNotificationL(KAnySensorTuple, ETrue, EBBPriorityNormal, s);
	User::WaitForRequest(s);
	TFullArgs a;
	TBuf8<2048> b2;
	while(1) {
		bbc.WaitForNotify(a, b2, s);
		User::WaitForRequest(s);

		TBuf<100> msg=_L("got id: ");
		msg.AppendNum(a.iId);
		RDebug::Print(msg);

		bbc.Delete(a.iId, s);
		User::WaitForRequest(s);

		msg=_L("delete, ret: ");
		msg.AppendNum(s.Int());
		RDebug::Print(msg);
	}
}


void test_bbclient_inner(MApp_context* c)
{
	CALLSTACKITEM_N(_CL("RDebug"), _CL("Print"));

	TFileName t=c->DataDir();
	t.Append(_L("TUPLE.db"));
	c->Fs().Delete(t);

	RABBClient bbc; bbc.ConnectLA();

	TTupleName tn1={ { 0x1 }, 0x1 };
	TComponentName cn1={ { 0x1 }, 0x1 };

	TRequestStatus s=KRequestPending;
	TBuf8<10> b=_L8("xxx");
	TUint id=0;
	bbc.Put(tn1, _L(""), cn1, b, EBBPriorityNormal, ETrue, id, s);
	User::WaitForRequest(s);

	TBuf8<10> b2;
	s=KRequestPending;
	TFullArgs a;
	bbc.Get(tn1, _L(""), a, b2, s);
	User::WaitForRequest(s);
	TEST_EQUALS(b, b2, _L("bbc::0"));

	TBBPriority prio[2]={ EBBPriorityNormal, EBBPriorityHigh };
	TInt i;
	for (i=0; i<2; i++) {
		TTupleName tn2={ { 0x2 }, 0x1 };
		s=KRequestPending;
		bbc.AddNotificationL(tn2, EFalse, prio[i], s);
		User::WaitForRequest(s);
		if (i==0) 
			TEST_EQUALS(s.Int(), KErrNone, _L("bbc::1"));
		else
			TEST_EQUALS(s.Int(), KErrAlreadyExists, _L("bbc::1"));

		b=_L8("abc");
		s=KRequestPending;
		bbc.Put(tn2, _L(""), cn1, b, prio[i], ETrue, id, s);
		User::WaitForRequest(s);
		TEST_EQUALS(s.Int(), KErrNone, _L("bbc::2"));
		s=KRequestPending;
		bbc.WaitForNotify(a, b2, s);
		User::WaitForRequest(s);
		TEST_EQUALS(s.Int(), KErrNone, _L("bbc::3"));
		TEST_EQUALS(b, b2, _L("bbc::4"));
	}
}

class RunBBC : public MRunnable2
{
	CALLSTACKITEM_N(_CL("bbc"), _CL("4"));

public:
	MApp_context* ic;
	RunBBC(MApp_context* c) : ic(c) { }
	void run() { test_bbclient_inner(ic); }
	void stop() { }
};

void test_bbclient(MApp_context* c)
{
	CALLSTACKITEM_N(_CL("bbc"), _CL("4"));

	RunBBC r(c);
	r.run();
	test_oom2(r);
}

void test_bbclient_perf_i(MApp_context* c)
{
	CALLSTACKITEM_N(_CL("bbc"), _CL("4"));

	c->Fs().Delete(_L("TUPLE.db"));

	RABBClient bbc1; bbc1.ConnectLA();

	TInt lens[]={ 0, 1, 2, 5, 10, 64, 128, 256, 512, 1024, -1 };
	TInt prios[]={ EBBPriorityNormal, EBBPriorityHigh, -1 };

#ifdef __WINS__
	TInt loops_inner=100;
	TInt loops_outer=10;
#else
	TInt loops_inner=100;
	TInt loops_outer=10;
#endif

	TTupleName tn= { { 0x1 }, 0x1 };
	TComponentName cn = { { 0x1 }, 0x1 };


	TRequestStatus s1, s2;
	RAFile f; f.ReplaceLA(c->Fs(), _L("bb_stats.txt"), EFileWrite);
	
	output->Write(_L("times"));
	for (TInt *len=lens; *len >= 0; len++) {
		output->Write(_L("."));
		auto_ptr<HBufC8> buf_in(HBufC8::NewL(*len));
		buf_in->Des().Fill('z', *len);
		auto_ptr<HBufC8> buf_out(HBufC8::NewL(*len));
		TPtr8 p_out=buf_out->Des();

		for (TInt *prio=prios; *prio >= 0; prio++) {
			RABBClient bbc2; bbc2.ConnectLA();
			TBBPriority this_prio=(TBBPriority)*prio;
			bbc2.AddNotificationL(tn, EFalse, this_prio, s2);
			User::WaitForRequest(s2);
			TUint tickcount_sum=0, tickcount_min=-1, tickcount_max=0;
			TInt64 elapsed_sum=0, elapsed_min=0, elapsed_max=0;
			bool first=true;
			TBool aPersist=ETrue; 
			//if (*prio == EBBPriorityHigh) aPersist=EFalse;
			for (TInt outer=0; outer < loops_outer; outer++) {
				TTime start_time, stop_time; start_time.HomeTime();
				TUint tick_start=User::TickCount(); TUint tick_stop, ticks;
				p_out.Zero();
				for (TInt inner=0; inner < loops_inner; inner++) {
					
					TUint id;
					TFullArgs meta;
					bbc2.WaitForNotify(meta, p_out, s2);
					bbc1.Put(tn, _L(""), cn, *buf_in, this_prio, ETrue, id, s1,
						aPersist);
					User::WaitForRequest(s1, s2);
					if (s1==KRequestPending) User::WaitForRequest(s1);
					if (s2==KRequestPending) User::WaitForRequest(s2);
					User::LeaveIfError(s1.Int());
					User::LeaveIfError(s2.Int());
				}

				tick_stop=User::TickCount();
				stop_time.HomeTime();

				TEST_EQUALS(p_out.Length(), *len, _L("t3"));
				TEST_EQUALS(p_out, *buf_in, _L("t4"));

				ticks=(tick_stop-tick_start);
				tickcount_sum+=ticks;
				if (ticks > tickcount_max) tickcount_max=ticks;
				if (first || ticks < tickcount_min) tickcount_min=ticks;

				TInt64 start_int=start_time.Int64();
				TInt64 stop_int=stop_time.Int64();
				TInt64 time_int=stop_int-start_int;
				elapsed_sum+=time_int;
				if (first || time_int < elapsed_min) elapsed_min=time_int;
				if (time_int > elapsed_max) elapsed_max=time_int;

				first=false;
			}
			elapsed_sum/=1000;
			elapsed_min/=1000;
			elapsed_max/=1000;

			TBuf8<300> stats;
			stats=_L8("Times [ size: "); stats.AppendNum(*len);
			stats.Append(_L8(", prio: ")); stats.AppendNum(*prio); stats.Append(_L8(" ]\n"));

			stats.Append(_L8("ticks - avg: ")); stats.AppendNum( tickcount_sum / loops_outer );
			stats.Append(_L8(", min: ")); stats.AppendNum(tickcount_min);
			stats.Append(_L8(", max: ")); stats.AppendNum(tickcount_max); stats.Append(_L8("\n"));

			stats.Append(_L8("ms  - avg: ")); stats.AppendNum( elapsed_sum / loops_outer );
			stats.Append(_L8(", min: ")); stats.AppendNum(elapsed_min);
			stats.Append(_L8(", max: ")); stats.AppendNum(elapsed_max); stats.Append(_L8("\n"));

			f.Write(stats);
		}
	}
	f.Flush();
	output->Write(_L("\n"));
}


void run_tests()
{
	CALLSTACKITEM_N(_CL("User"), _CL("TickCount"));


	User::__DbgMarkStart(RHeap::EUser);
	{
		TInt err, pushed=0;
		output=new (ELeave) MOutput;
		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
		CleanupStack::PushL(output); ++pushed;

		{
			auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("BlackBoardClient")));
			output->foutput.Replace(c->Fs(), _L("bbclienttest.txt"), EFileWrite);

			test_notify(c.get());
			if (0) {
#if defined(__WINS__)
			CC_TRAP(err, test_bbclient(c.get()));
			TEST_EQUALS(err, KErrNone, _L("bbclient all"));
#endif
			CC_TRAP(err, test_bbclient_perf_i(c.get()));
			TEST_EQUALS(err, KErrNone, _L("bbperf all"));
			}

			TBuf<30> b=_L("OK: "); b.AppendNum(ok); b.Append(_L("/")); 
			b.AppendNum(ok+not_ok); b.Append(_L("\n"));
			output->Write(b);

			output->Getch();

			output->foutput.Close();
		}
		delete output->cons;

		CleanupStack::PopAndDestroy(pushed);
	}
        User::__DbgMarkEnd(RHeap::EUser,0);

}
