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
#include "testdriver_base.cpp"
#include "raii_f32file.h"

#include <Python.h>
#include <cspyinterpreter.h>
#include "pythonexternalizer.h"
#include "concretedata.h"
#include "symbian_python_ptr.h"
#include "app_context_impl.h"
#include "loca_logic.h"
#include "db.h"
#include "cl_settings.h"
#include "bb_settings.h"
#include "cl_settings_impl.h"
#include <basched.h>

#include "loca_logic.cpp"
#include "testdata.cpp"
#include "scriptdata.cpp"
#include "loca_errorlog.h"
#include "loca_sender.h"

const TComponentName KListener = { { CONTEXT_UID_CONTEXTNETWORK}, 1 };
const TTupleName KListenerStop = { { CONTEXT_UID_CONTEXTNETWORK}, 2 };

void run_python_tests()
{
	User::__DbgMarkStart(RHeap::EUser);
	{
		output=new (ELeave) MOutput;

		RAFs fs; fs.ConnectLA();
		output->foutput.Replace(fs, _L("blackboardtest.txt"), EFileWrite);

		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));

		CSPyInterpreter* iInterpreter = CSPyInterpreter::NewInterpreterL();
		PyEval_RestoreThread(PYTHON_TLS->thread_state);

		TInt res=PyRun_SimpleString("from time import time,ctime\n"
			     "print 'Today is',ctime(time())\n");
		if (res==0) ok++;
		else not_ok++;

		{
			auto_ptr<CPythonExternalizer> e(CPythonExternalizer::NewL());
			_LIT(KInt, "int");
			TBBInt i(100011, KInt);
			i.IntoXmlL(e.get(), EFalse);
			python_ptr<PyObject> o(e->GetObject());
			PyObject* res=o.get();

			if ( TEST_EQUALS( (int)PyInt_Check(res), (int)1, _L("py1")) )
				TEST_EQUALS( (int)PyInt_AsLong(res), (int)i(), _L("py2"));
			e->Reset();
			o.reset();
		}
		{
			auto_ptr<CPythonExternalizer> e(CPythonExternalizer::NewL());
			_LIT(KInt, "int");
			TBBShortString i(_L("sdlklfd"), KInt);
			i.IntoXmlL(e.get(), EFalse);
			python_ptr<PyObject> o(e->GetObject());
			PyObject* res=o.get();

			if (TEST_EQUALS( (int)PyUnicode_Check(res), (int)1, _L("py3") ) ) {
				TPtrC parsed( PyUnicode_AsUnicode(res),
					PyUnicode_GetSize(res) );

				TEST_EQUALS( i().Compare(parsed), 0, _L("py4") );
			}
			e->Reset();
			o.reset();
		}

		PyEval_SaveThread();
		delete iInterpreter;

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

TTime from_unix(TInt aMinutes)
{
	if (aMinutes==0) return TTime(0);
	TTime zero(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	return zero+TTimeIntervalMinutes(aMinutes);
}

void CheckDevStats(const TDesC& testname, CLocaLogicImpl* cl,
				   const TBTTestStats* s, TBool aIgnoreLastSeen=EFalse)
{
	TTime zero(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	TBuf<80> testname2;
	TBTDevAddr a( TPtrC8(s->bt, 6) );
	a.GetReadable(testname2, testname, _L(":"), KNullDesC);
	TDevStats expected;
	expected.iCountStay=s->count;
	if (aIgnoreLastSeen) 
		expected.iLastSeen=TTime(0);
	else
		expected.iLastSeen=zero+TTimeIntervalMinutes(s->last_seen);
	expected.iVisitBegin=zero+TTimeIntervalMinutes(s->last_seen_begin);
	if (s->prev_seen_begin)
		expected.iPreviousVisitBegin=zero+
			TTimeIntervalMinutes(s->prev_seen_begin);
	else
		expected.iPreviousVisitBegin=TTime(0);
	if  (s->prev_seen)
		expected.iPreviousVisitEnd=zero+
			TTimeIntervalMinutes(s->prev_seen);
	else
		expected.iPreviousVisitEnd=TTime(0);

	if (s->first_seen)
		expected.iFirstSeen=zero+TTimeIntervalMinutes(s->first_seen);
	else
		expected.iFirstSeen=TTime(0);

	expected.iSquareSumStay=s->ssum;
	expected.iSumStay=s->sum;
	expected.iMaxStay=s->max_span;
	TDevStats got;
	TBBBtDeviceInfo info(TPtrC8(s->bt, 6), KNullDesC, 0x02, 0, 0);
	cl->iDevStats->GetStatsL(info, TPtrC(s->node), got);
	if (aIgnoreLastSeen) got.iLastSeen=TTime(0);
	TEST_EQUALS(expected, got, testname2);
}
void CheckMsgStats(const TDesC& testname, CLocaLogicImpl* cl,
				   const TBTTestStats* s)
{
	TTime zero(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	TBuf<80> testname2;
	TBTDevAddr a( TPtrC8(s->bt, 6) );
	a.GetReadable(testname2, testname, _L(":"), KNullDesC);

	testname2.Append(_L("_msg"));
	TMessageStats expected_msg;
	expected_msg.iFailureCount=s->failure_count;
	expected_msg.iSuccessCount=s->success_count;
	expected_msg.iLocalFailureCount=s->local_failure_count;
	expected_msg.iPreviousLocalSuccess=
		from_unix(s->previous_local_success);
	expected_msg.iPreviousRemoteSuccess=
		from_unix(s->previous_remote_success);
	expected_msg.iPreviousLocalFailure=
		from_unix(s->previous_local_failure);
	expected_msg.iPreviousRemoteFailure=
		from_unix(s->previous_remote_failure);
	expected_msg.iLastSeen=TTime(0);
	TMessageStats got_msg;
	TBBBtDeviceInfo info(TPtrC8(s->bt, 6), KNullDesC, 0x02, 0, 0);
	cl->iMessageStats->GetStatsL(info, got_msg);
	got_msg.iLastSeen=TTime(0);
	TEST_EQUALS(expected_msg, got_msg, testname2);
}

void CheckFinalStats(CLocaLogicImpl* cl)
{
	TBuf<50> mynode;
	GetContext()->Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, mynode);
	const TBTTestStats* s=KBTFinalStats;
	TBuf<40> testname;
	while (s->node) {
		testname=_L("finalstats");
		if ( TPtrC(s->node).Compare(_L("n1"))==0 ||
			TPtrC(s->node).Compare(_L("n2"))==0) {
				CheckDevStats(testname, cl, s, ETrue);
		}
		if ( TPtrC(s->node).Compare(mynode)==0 ) {
			CheckMsgStats(testname, cl, s);
		}
		s++;
	}
}

void run_test_2(CLocaLogicImpl* cl)
{
	TTime zero(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	TTime t; TInt min=-1, count=0;
	auto_ptr<CBBBtDeviceList> dev(CBBBtDeviceList::NewL());
	TBuf<100> name, title;
	TInt send_to, msgcode;
	HBufC8* body=0;
	const TBTTestItem* i=KBTTest;
	const TBTTestStats* s=KBTTestStats;
	TBuf<80> testname, testname2;
	TBuf<50> node=TPtrC(i->node);
	TBuf<50> mynode;
	GetContext()->Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, mynode);
	for(;;) {
		if (min!=i->minute || (i->node && node.Compare(TPtrC(i->node)))) {
			t=zero+TTimeIntervalMinutes(min);
			cl->UpdateStats(node, dev.get(), t);
			dev->Reset();
			TBBBtDeviceInfo* info=new (ELeave) 
				TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\0", 6), KNullDesC, 0, 0, 0);
			CleanupStack::PushL(info);
			dev->AddItemL(info);
			CleanupStack::Pop();
			while (s->node && s->minute==min) {
				testname=_L("s2 mn: ");
				testname.AppendNum(min);
				testname.Append(_L(" nd: "));
				testname.Append( TPtrC(s->node) );
				testname.Append(_L(" bt: "));
				CheckDevStats(testname, cl, s);

				if ( TPtrC(s->node).Compare(mynode) == 0) {
					CheckMsgStats(testname, cl, s);
				}
				s++;
			}
		}
		if (i->node==0) break;
		node=TPtrC(i->node);
		min=i->minute;
		TBBBtDeviceInfo* info=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8(i->bt, 6), KNullDesC, 0x02, 0, 0);
		CleanupStack::PushL(info);
		t=zero+TTimeIntervalMinutes(min);
		TBool local=( mynode.Compare(node)==0 );
		if (i->aMsgSuccess > 0) {
			cl->Success(*info, i->aMsgSuccess, t, local);
		} else if (i->aMsgSuccess < 0) {
			cl->Failed(*info, -1*(i->aMsgSuccess), t, 
				CLocaLogic::EUnknown, local);
		} 
		dev->AddItemL(info);
		CleanupStack::Pop();
		i++;
	}
	CheckFinalStats(cl);
	delete body;
}

void run_test_msg_1(CLocaLogicImpl* cl)
{
_LIT(KMsgScript1, "def g(general, dev, msg) :\n"
L"	 return (10, 2, 'bname', 'btitle', 'bbody')\n"
	 );

	auto_ptr<CBBString> ss(CBBString::NewL(KNullDesC));
	ss->Append(KMsgScript1);
	cl->NewValueL(0, KLocaScriptTuple, _L("g"), KNoComponent, ss.get());
	TBuf<1> empty;
	TEST_EQUALS(output->msg, empty, _L("loca msg 1 script 1"));

	auto_ptr<CBBBtDeviceList> dev(CBBBtDeviceList::NewL());

	dev->Reset();
	{
		TBBBtDeviceInfo* info=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\0", 6), KNullDesC, 0x02, 0, 0);
		CleanupStack::PushL(info);
		dev->AddItemL(info);
		CleanupStack::Pop();
	}
	{
		TBBBtDeviceInfo* info=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\1", 6), KNullDesC, 0x02, 0, 0);
		CleanupStack::PushL(info);
		dev->AddItemL(info);
		CleanupStack::Pop();
	}
	TBuf<100> name, title;
	TInt send_to, msgcode;
	auto_ptr<HBufC8> body(0);
	TTime t=GetTime();
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(output->msg, empty, _L("loca msg 1 call"));
	TEST_EQUALS(send_to, 1, _L("loca msg 1 send_to"));
	TEST_EQUALS(msgcode, 2, _L("loca msg 1 msgcode"));
	TEST_EQUALS(name, _L("bname"), _L("loca msg 1 name"));
	TEST_EQUALS(title, _L("btitle"), _L("loca msg 1 title"));
	TEST_NOT_EQUALS( (TInt)(body.get()), (TInt)0, _L("loca msg 1 got body"));
	if (body.get())
		TEST_EQUALS(*body, _L8("bbody"), _L("loca msg 1 body"));

_LIT(KMsgScript2, "def g(general, dev, msg) :\n"
L"	 2/'x'\n"
	 );

	ss->Zero();
	ss->Append(KMsgScript2);
	cl->NewValueL(0, KLocaScriptTuple, _L("g"), KNoComponent, ss.get());
	TEST_EQUALS(output->msg, empty, _L("loca msg 2 script 1"));
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TInt found=output->msg.FindF(_L("TypeError"));
	TEST_NOT_EQUALS(found, KErrNotFound, _L("loca msg 2 call 1"));
	output->msg.Zero();
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(output->msg, empty, _L("loca msg 2 call 2"));
}

_LIT(KFileScript, "def f(general, dev, msg) :\n"
L"	f=open('c:/python-locatest.txt', 'a')\n"
L"	for k in dev.keys():\n"
L"		f.write(\"%s %d %s \" % (k, dev[k]['last_seen'], general['mac']) )\n"
L"		f.write(\"%d \" % dev[k]['count'] )\n"
L"		f.write(\"%d \" % dev[k]['avg'] )\n"
L"		f.write(\"%f \" % dev[k]['var'] )\n"
L"		f.write(\"%d \" % dev[k]['visitbegin'] )\n"
L"		f.write(\"%d \" % dev[k]['prev_visitbegin'] )\n"
L"		f.write(\"%d\\n\" % dev[k]['prev_visitend'] )\n"
L"	f.close()\n"
L"	return (0, 0, '', '', '')\n"
	 );

void run_test_4(CLocaLogicImpl* cl)
{
_LIT(KFileScript1, "def f(general, dev, msg) :\n"
L"	 f=open('c:/python-locatest.txt', 'w')\n"
L"	 f.write(general.__str__())\n"
L"	 f.write(dev.__str__())\n"
L"	 f.write(msg.__str__())\n"
L"	 f.close()\n"
L"	 return (0, 0, '', '', '')\n"
	 );

	auto_ptr<CBBString> ss(CBBString::NewL(KNullDesC));
	ss->Append(KFileScript);
	cl->NewValueL(0, KLocaScriptTuple, _L("f"), KNoComponent, ss.get());
	TBuf<1> empty;
	TEST_EQUALS(output->msg, empty, _L("loca 4 script 1"));

	TTime zero(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	TTime t; TInt min=-1, count=0;
	auto_ptr<CBBBtDeviceList> dev(CBBBtDeviceList::NewL());
	TBuf<100> name, title;
	TInt send_to, msgcode;
	auto_ptr<HBufC8> body(0);
	const TBTTestItem* i=KBTTest;
	TBuf<50> node=TPtrC(i->node);
	for(;;) {
		if (min!=i->minute || (i->node && node.Compare(TPtrC(i->node)))) {
			t=zero+TTimeIntervalMinutes(min);
			cl->UpdateStats(node, dev.get(), t);
			output->msg.Zero();
			TBuf<1> empty;
			cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
			TEST_EQUALS(output->msg, empty, _L("msg call"));
			dev->Reset();
			TBBBtDeviceInfo* info=new (ELeave) 
				TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\0", 6), KNullDesC, 0x02, 0, 0);
			CleanupStack::PushL(info);
			dev->AddItemL(info);
			CleanupStack::Pop();
		}
		if (i->node==0) break;
		node=TPtrC(i->node);
		min=i->minute;
		TBBBtDeviceInfo* info=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8(i->bt, 6), KNullDesC, 0x02, 0, 0);
		CleanupStack::PushL(info);
		dev->AddItemL(info);
		CleanupStack::Pop();
		i++;
	}
}

void run_test_3(CLocaLogicImpl* cl)
{
	auto_ptr<CBBString> sp(CBBString::NewL(KNullDesC));
	CBBString&s =*sp;
	s.Append(_L("def x(d) :\n  return d['x']\n"));
	output->msg.Zero();
	cl->NewValueL(1, KLocaScriptTuple, _L("x"), KNoComponent, &s);
	TBuf<1> empty;
	TEST_EQUALS(output->msg, empty, _L("loca script 1"));

	TEST_EQUALS(1, cl->iFunctions->Count(), _L("loca script 1:1"));
	TInt pos;
	TInt err=cl->iFunctions->Find( TPtrC8( (const TUint8*)"x", 2), pos);
	TEST_EQUALS(0, err, _L("loca script 1:2"));
	TEST_EQUALS(0, pos, _L("loca script 1:3"));

	cl->NewValueL(1, KLocaScriptTuple, _L("x2"), KNoComponent, &s);
	TInt found_msg=output->msg.FindC(_L("didn't create a function with that name"));
	TEST_NOT_EQUALS(found_msg, KErrNotFound, _L("loca script 2"));
	s.Zero();
	s.Append(_L("def x(d) :\n  return d['x]\n"));
	output->msg.Zero();
	cl->NewValueL(1, KLocaScriptTuple, _L("x"), KNoComponent, &s);
	found_msg=output->msg.FindC(_L("Cannot compile code in script"));
	TEST_NOT_EQUALS(found_msg, KErrNotFound, _L("loca script 2"));
	
	output->msg.Zero();
	s.Zero();
	s.Append(_L("def x(d) :\n  return d['x']\n"));
	TBuf<1> nonlatin1; nonlatin1.Append(TChar(0xac00));
	s.Append( nonlatin1 );
	cl->NewValueL(1, KLocaScriptTuple, _L("x"), KNoComponent, &s);
	found_msg=output->msg.FindC(_L("to Latin1"));
	TEST_NOT_EQUALS(found_msg, KErrNotFound, _L("loca script 3"));

	cl->DeletedL(KLocaScriptTuple, _L("x"));
	TEST_EQUALS(0, cl->iFunctions->Count(), _L("loca script 4:1"));
	err=cl->iFunctions->Find( TPtrC8( (const TUint8*)"x", 2), pos);
	TEST_EQUALS(KErrNotFound, err, _L("loca script 4:2"));
}

void run_test_1(CLocaLogicImpl* cl)
{
	auto_ptr<CBBBtDeviceList> dev(CBBBtDeviceList::NewL());
	TInt send_to, msgcode;
	TTime now; now=GetTime();
	TBuf<100> name, title;
	auto_ptr<HBufC8> body=0;
	cl->GetMessage(dev.get(), now, send_to, msgcode,
		name, title, body);
	TDevStats s1, s2;
	Mem::FillZ(&s2, sizeof(s2));
	TBBBtDeviceInfo info;
	cl->iDevStats->GetStatsL(info, _L("1"), s1);

	TEST_EQUALS(s1, s2, _L("empty1"));
}

void run_test_large(CLocaLogicImpl* cl)
{
	auto_ptr<CBBBtDeviceList> dev(CBBBtDeviceList::NewL());
	TTime t=GetTime();
	for (int i=0; i<40; i++) {
		for (int j=0; j<256; j++) {
			TBuf8<6> addrp=_L8("xxxxxx");
			addrp[0]=0;
			addrp[1]=0;
			addrp[2]=0;
			addrp[3]=0;
			addrp[4]=i;
			addrp[5]=j;
			dev->Reset();
			{
				TBBBtDeviceInfo* info=new (ELeave) 
					TBBBtDeviceInfo(addrp, KNullDesC, 0x02, 0, 0);
				CleanupStack::PushL(info);
				dev->AddItemL(info);
				CleanupStack::Pop();
			}
			{
				TBBBtDeviceInfo* info=new (ELeave) 
					TBBBtDeviceInfo(addrp, KNullDesC, 0x02, 0, 0);
				CleanupStack::PushL(info);
				dev->AddItemL(info);
				CleanupStack::Pop();
			}
			cl->UpdateStats(_L("n1"), dev.get(), t);
		}
	}
}

#include "ntpconnection.h"
#include <c32comm.h>

class TNtpCallback : public MNTPObserver {
	virtual void NTPInfo(const TDesC& aMsg) {
		RDebug::Print(aMsg);
	}
	virtual void NTPError(const TDesC& aMsg, TInt aErrorCode) {
		RDebug::Print(_L("NTP Failure"));
		RDebug::Print(aMsg);
		CActiveScheduler::Stop();
	}
	virtual void NTPSuccess(TTime aNewTime) {
		RDebug::Print(_L("NTP Success"));
		CActiveScheduler::Stop();
	}
};

#include <cdbcols.h>
#include <commdb.h>

void init_nw()
{
#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM") // alternatively "FCOMM"
#endif
	TInt err=User::LoadPhysicalDevice(PDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists) User::Leave(err);
	err=User::LoadLogicalDevice(LDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists) User::Leave(err);

	User::LeaveIfError(StartC32());
}

_LIT(KNwTestData, "CNwTestData");

#include "csd_bluetooth.h"
#include "csd_event.h"

class CNwTestData : public CCheckedActive, public MContextBase,
	public MBBObserver
{
public:
	TTime zero;
	CBBBtDeviceList* dev;
	TInt min;
	CBBSensorEvent* iEvent;

	RTimer t;
	const TBTTestItem* i;
	TBuf<50> node, mynode;
	CLocaLogic *iCL;
	CNwTestData(CLocaLogic* aCL) : 
		CCheckedActive(EPriorityNormal, KNwTestData), iCL(aCL) { }
	TInt count;
	CBBSubSession* iSubSession;
	void ConstructL() {
		i=KBTTest;
		node=TPtrC(i->node);
		Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, mynode);
		zero=TTime(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
		dev=CBBBtDeviceList::NewL();
		TBBBtDeviceInfo* info=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\0", 6), KNullDesC, 0, 0, 0);
		CleanupStack::PushL(info);
		dev->AddItemL(info);
		CleanupStack::Pop();
		iEvent=new (ELeave) CBBSensorEvent(KCSDBtList, KBluetoothTuple);
		iEvent->iData.SetValue(dev);
		iEvent->iData.SetOwnsValue(EFalse);
		iEvent->iPriority()=CBBSensorEvent::VALUE;
		min=-1;
		t.CreateLocal();
		CActiveScheduler::Add(this);
		t.After(iStatus, TTimeIntervalMicroSeconds32(3*1000*1000));
		SetActive();

		iSubSession=BBSession()->CreateSubSessionL(this);
		iSubSession->AddNotificationL(KRemoteBluetoothTuple);
	}
	~CNwTestData() {
		Cancel();
		delete dev;
		delete iEvent;
		delete iSubSession;
	}
	static CNwTestData* NewL(CLocaLogic* aCL=0) {
		auto_ptr<CNwTestData> ret(new (ELeave) CNwTestData(aCL));
		ret->ConstructL();
		return ret.release();
	}
	void DoCancel() {
		t.Cancel();
	}
	void Async() {
		t.After(iStatus, TTimeIntervalMicroSeconds32(200*1000));
		SetActive();
	}
	void CheckedRunL() {
		TTime t;
		TBool done=EFalse;
		TTime exp=GetTime(); exp+=TTimeIntervalMinutes(10);
		while(!done) {
			if (min!=i->minute || (i->node && node.Compare(TPtrC(i->node)))) {
				t=zero+TTimeIntervalMinutes(min);
				iEvent->iStamp()=t;
				if (node.Compare(mynode)==0) {
					BBSession()->PutL(KBluetoothTuple, _L(""),
						iEvent, exp);
					count++;
				}
				dev->Reset();
				TBBBtDeviceInfo* info=new (ELeave) 
					TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\0", 6), KNullDesC, 0, 0, 0);
				CleanupStack::PushL(info);
				dev->AddItemL(info);
				CleanupStack::Pop();
				done=ETrue;
			}
			if (i->node==0) {
				BBSession()->PutL(KListenerStop, _L(""),
					iEvent, exp, KListener);
				TBuf<50> msg=_L("SENT ");
				msg.AppendNum(count);
				msg.Append(_L(" TUPLES\n"));
				output->Write(msg);
				return;
			}
			node=TPtrC(i->node);
			min=i->minute;
			TBBBtDeviceInfo* info=new (ELeave) 
				TBBBtDeviceInfo(TPtrC8(i->bt, 6), KNullDesC, 0x02, 
					0x02, 0x02);
			if (mynode.Compare(node)==0 && iCL) {
				t=zero+TTimeIntervalMinutes(min);
				if (i->aMsgSuccess > 0) {
					iCL->Success(*info, i->aMsgSuccess, t, ETrue);
					count++;
				} else if (i->aMsgSuccess < 0) {
					iCL->Failed(*info, -1*(i->aMsgSuccess), t, 
						CLocaLogic::EUnknown, ETrue);
					count++;
				} 
			}

			CleanupStack::PushL(info);
			dev->AddItemL(info);
			CleanupStack::Pop();
			i++;
		}
		Async();
	}
	TInt received_count;
	virtual void NewValueL(TUint aId, const TTupleName& aName, 
		const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) 
	{
		received_count++;
	}
	virtual void DeletedL(const TTupleName& aName, 
		const TDesC& aSubName) 
	{
	}

};

void run_nw_test_1()
{
	init_nw();

	TInt ap=-1;
	TInt err;
	if (!GetContext()->Settings().GetSettingL(SETTING_IP_AP, ap))
		User::Leave(KErrNotFound);

	auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
	CCommsDbTableView* iViewP=db->OpenTableLC(TPtrC(IAP));
	CleanupStack::Pop();
	auto_ptr<CCommsDbTableView> iView(iViewP);
	TUint32 id;
	TBuf<50> name;
	while( (err=iView->GotoNextRecord()) == KErrNone ) {
		iView->ReadUintL(TPtrC(COMMDB_ID), id);
		if (id==ap) {
			iView->ReadTextL(TPtrC(COMMDB_NAME), name);
			break;
		}
	}
	if (name.Length()==0) User::Leave(KErrNotFound);

	TNtpCallback cb;
	auto_ptr<CNTPConnection> ntp(CNTPConnection::NewL(cb));
	ntp->Sync(ap);
	CActiveScheduler::Start();

}

#include "bb_logger.h"

_LIT(KS, "s");
void run_comp_test()
{
	CBBSession* sess=GetContext()->BBSession();
	TBBShortString s(_L("xxx"), KS);
	TTime exp=GetTime(); exp+=TTimeIntervalMinutes(10);
	sess->PutRequestL(KBluetoothTuple, KNullDesC, &s, exp, KListener);
	sess->PutRequestL(KBluetoothTuple, KNullDesC, &s, exp, KListener);
	sess->DeleteL(KListener, ETrue);
}

void run_nw_test()
{
	init_nw();
	GetContext()->Fs().Delete(_L("c:\\bbdebug.txt"));
	GetContext()->Fs().Delete(_L("c:\\bbdebugout.txt"));

	GetContext()->BBSession()->DeleteL(KListener, ETrue);
	GetContext()->Settings().WriteSettingL(SETTING_CONTEXTNW_HOST, _L("10.1.0.1"));
	GetContext()->Settings().WriteSettingL(SETTING_CONTEXTNW_PORT, 5000);
	GetContext()->Settings().WriteSettingL(SETTING_CONTEXTNW_ENABLED, ETrue);
	GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
		_L("n1"));
	GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_PASSWORD,
		_L("emulator"));
	GetContext()->BBSession()->DeleteL(KListener, ETrue);
	GetContext()->BBSession()->DeleteL(KLocaErrorTuple, 
		KNullDesC, ETrue);
	GetContext()->BBSession()->DeleteL(KListenerStop, 
		KNullDesC, ETrue);
	GetContext()->BBSession()->DeleteL(KLocaScriptTuple, 
		KNullDesC, ETrue);
	GetContext()->BBSession()->DeleteL(KLocaMessageStatusTuple, 
		KNullDesC, ETrue);

	auto_ptr<CBBLogger> logger(CBBLogger::NewL(*GetContext(), 0));
	auto_ptr<CNwTestData> nwtest(CNwTestData::NewL());

	CActiveScheduler::Start();
}

void run_nw_test_2(TBool aNode1First=ETrue)
{
	init_nw();
	GetContext()->Fs().Delete(_L("c:\\bbdebug.txt"));
	GetContext()->Fs().Delete(_L("c:\\bbdebugout.txt"));
	GetContext()->Fs().Delete(
		_L("c:\\system\\data\\context\\LOCALOGIC.db"));

	GetContext()->BBSession()->DeleteL(KListener, ETrue);
	GetContext()->BBSession()->DeleteL(KLocaErrorTuple, 
		KNullDesC, ETrue);
	GetContext()->BBSession()->DeleteL(KListenerStop, 
		KNullDesC, ETrue);
	GetContext()->BBSession()->DeleteL(KLocaScriptTuple, 
		KNullDesC, ETrue);
	GetContext()->BBSession()->DeleteL(KLocaMessageStatusTuple, 
		KNullDesC, ETrue);

	GetContext()->Settings().WriteSettingL(SETTING_CONTEXTNW_HOST, _L("10.1.0.1"));
	GetContext()->Settings().WriteSettingL(SETTING_CONTEXTNW_PORT, 5000);
	GetContext()->Settings().WriteSettingL(SETTING_CONTEXTNW_ENABLED, ETrue);
	TBuf<20> firstn;
	TBuf<20> secondn;
	TBuf<20> firstimei, secondimei;
	if (aNode1First) {
		firstn=_L("n1");
		secondn=_L("n2");
		firstimei=_L("354349000362924");
		secondimei=_L("emu02");
	} else {
		firstn=_L("n2");
		secondn=_L("n1");
		firstimei=_L("emu02");
		secondimei=_L("emu01");
	}
	GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
		firstn);
	GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_PASSWORD,
		_L("emulator"));
	GetContext()->Settings().WriteSettingL(SETTING_WINS_IMEI,
		firstimei);

	output->Write(_L("Running as n1\n"));
	{
		auto_ptr<CDb> db(CDb::NewL(*GetContext(), 
			_L("LOCALOGIC"), EFileWrite));
		auto_ptr<CLocaLogic> cl(CLocaLogic::NewL(*GetContext(), db->Db()));
		auto_ptr<CBBLogger> logger(CBBLogger::NewL(*GetContext(), 0));
		auto_ptr<CNwTestData> nwtest(CNwTestData::NewL(cl.get()));
		CActiveScheduler::Start();
	}
	output->Write(_L("done."));

	GetContext()->Fs().Delete(
		_L("c:\\system\\data\\context\\LOCALOGIC.db"));
	auto_ptr<CBBString> ss(CBBString::NewL(KNullDesC));
	ss->Append(KFileScript);
	GetContext()->BBSession()->PutL(KLocaScriptTuple, _L("f"),
		ss.get(), Time::MaxTTime());
	GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
		secondn);
	GetContext()->Settings().WriteSettingL(SETTING_WINS_IMEI,
		secondimei);
	GetContext()->Settings().WriteSettingL(SETTING_ENABLE_LOCA_BLUEJACK,
		ETrue);
	output->Write(_L("Running as n2\n"));
	{
		auto_ptr<CBBLogger> logger(CBBLogger::NewL(*GetContext(), 0));
		auto_ptr<CLocaSender> send( CLocaSender::NewL(*GetContext() ) );
		auto_ptr<CNwTestData> nwtest(CNwTestData::NewL(send->GetLogic() ));
		CActiveScheduler::Start();
		output->Write(_L("checking_stats.\n"));
		CheckFinalStats( (CLocaLogicImpl*)send->GetLogic());
	}
	output->Write(_L("done.\n"));

}

#include "independent.h"

TInt tf(TAny*)
{
	User::After( TTimeIntervalMicroSeconds32(600*1000*1000) );
	return 0;
}

void test_indep()
{

	independent_worker wt;
	wt.start(_L("indeptest"), tf, 0, EPriorityNormal);
	wt.stop();
}

void LoadScriptL(CLocaLogicImpl* cl, const TDesC& aScriptName)
{
	TFileName fn=_L("c:\\system\\data\\context\\scripts\\");
	fn.Append(aScriptName);
	RAFile f;
	f.OpenLA(GetContext()->Fs(), fn, EFileRead|EFileShareAny);
	TBuf8<256> buf8;
	TBuf<256> buf;
	_LIT(KScript, "script");
	auto_ptr<CBBString> ss(CBBString::NewL(KScript));
	while (f.Read(buf8)==KErrNone && buf8.Length()>0) {
		buf.Copy(buf8);
		ss->Append(buf);
	}
	output->msg.Zero();
	if (aScriptName[0]=='_') {
		TPtrC script=aScriptName.Left(aScriptName.Length()-3);
		cl->NewScriptL(script, ss.get());
	} else {
		TPtrC tp=aScriptName.Mid(4);
		TPtrC script=tp.Left(tp.Length()-3);
		cl->NewScriptL(script, ss.get());
	}
	TBuf<50> testname=_L("load ");
	testname.Append(aScriptName);
	TEST_EQUALS(output->msg, KNullDesC(), testname);
}

void AppendShortDate(TDes& aInto, TInt minutes)
{
	TTime zero(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	TTime t;
	t=zero+TTimeIntervalMinutes(minutes);
	t-=TTimeIntervalHours(10);
	TDateTime dt=t.DateTime();
	aInto.AppendNum(t.DayNoInMonth()+1);
	aInto.Append(_L("-"));
	if (dt.Hour()<10) aInto.AppendNum(0);
	aInto.AppendNum(dt.Hour());
	aInto.Append(_L(":"));
	if (dt.Minute()<10) aInto.AppendNum(0);
	aInto.AppendNum(dt.Minute());
}

void CreateEmptyLogic(auto_ptr<CDb>& db, auto_ptr<CLocaLogicImpl>& cl,
					  CLocaLogicImpl*& clr)
{
	db.reset(0);
	cl.reset(0);
	GetContext()->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
	db.reset(CDb::NewL(*GetContext(), _L("LOCATEST"), EFileWrite));
	cl.reset((CLocaLogicImpl*)CLocaLogic::NewL(*GetContext(), db->Db()));
	clr=cl.get();
	LoadScriptL(clr, _L("_general.py"));
}

void test_scripts()
{
	auto_ptr<CDb> dbp(0);
	auto_ptr<CLocaLogicImpl> clp(0);
	CLocaLogicImpl* cl=0;
	CreateEmptyLogic(dbp, clp, cl);

	const TBTScriptItem* si=KBTScriptTestCalls;
	const TBTTestItem* ti=KBTScriptTestBt;
	auto_ptr<CBBBtDeviceList> dev(CBBBtDeviceList::NewL());
	TBuf<100> name, title;
	TInt send_to, msgcode;
	auto_ptr<HBufC8> body(0);
	TTime zero(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	TTime t;
	TBuf<50> node;
	while (si->node) {
		if (TPtrC(si->node).Length()==0) {
			CreateEmptyLogic(dbp, clp, cl);
			si++;
			continue;
		}
		cl->iNodeName=TPtrC(si->node);
		while (ti->node && ti->minute <= si->minute && 
				TPtrC(ti->node).Compare(TPtrC(si->node2))==0) {
			dev->Reset();
			TBBBtDeviceInfo* info=new (ELeave) 
				TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\0", 6), KNullDesC, 0, 0, 0);
			CleanupStack::PushL(info);
			dev->AddItemL(info);
			CleanupStack::Pop();
			info=new (ELeave) 
				TBBBtDeviceInfo(TPtrC8(ti->bt, 6), KNullDesC, 0x02, 0, 0);
			CleanupStack::PushL(info);
			dev->AddItemL(info);
			CleanupStack::Pop();
			t=zero+TTimeIntervalMinutes(ti->minute);
			node=TPtrC(ti->node);
			cl->UpdateStats(node, dev.get(), t);
			ti++;
		}
		dev->Reset();
		TBBBtDeviceInfo* info=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8((TUint8*)"\0\0\0\0\0\0", 6), KNullDesC, 0, 0, 0);
		CleanupStack::PushL(info);
		dev->AddItemL(info);
		CleanupStack::Pop();
		info=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8(si->bt, 6), KNullDesC, 0x02, 0, 0);
		CleanupStack::PushL(info);
		dev->AddItemL(info);
		CleanupStack::Pop();
		t=zero+TTimeIntervalMinutes(si->minute);
		TPtrC script(si->scriptfile);
		if (script.Length()>0) LoadScriptL(cl, script);
		msgcode=-1;
		output->msg.Zero();
		cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
		TBuf<50> testname=_L("script ");
		testname.Append( node );
		testname.Append(_L(" "));
		AppendShortDate(testname, si->minute);
		if (si->aMsgSuccess>0) {
			TEST_EQUALS(output->msg, KNullDesC(), testname);
			TEST_EQUALS(send_to, 1, testname);
			testname.Append(_L(" code"));
			TEST_EQUALS(si->aMsgSuccess, msgcode, testname);
		} else if (si->aMsgSuccess==0) {
			TEST_EQUALS(msgcode, -1, testname);
		} else {
			TInt msg=-1*si->aMsgSuccess;
			cl->Success(*info, msg, t, TPtrC(si->node).Compare(TPtrC(si->node2))==0);
		}
		si++;
	}
}

void test_accepted(CLocaLogicImpl* cl)
{
_LIT(KScript1, "def f1(general, dev, msg) :\n"
L"	 return (2, 1, 'jack1', 'title1', 'body1')\n"
	 );
_LIT(KScript2, "def f2(general, dev, msg) :\n"
L"	 return (1, 2, 'jack2', 'title2', 'body2')\n"
	 );

	auto_ptr<CBBString> ss(CBBString::NewL(KNullDesC));
	ss->Append(KScript1);
	cl->NewScriptL(_L("f1"), ss.get());
	ss->Zero();
	ss->Append(KScript2);
	cl->NewScriptL(_L("f2"), ss.get());

	auto_ptr<CBBBtDeviceList> dev(CBBBtDeviceList::NewL());
	{
		TBBBtDeviceInfo* info1=new (ELeave) 
			TBBBtDeviceInfo(TPtrC8( (TUint8*)"000000", 6), KNullDesC, 
				0, 0, 0);
		CleanupStack::PushL(info1);
		dev->AddItemL(info1);
		CleanupStack::Pop();
	}
	TBBBtDeviceInfo* info1=new (ELeave) 
		TBBBtDeviceInfo(TPtrC8( (TUint8*)"000001", 6), KNullDesC, 
			0x02, 0, 0);
	CleanupStack::PushL(info1);
	dev->AddItemL(info1);
	CleanupStack::Pop();

	TBuf<100> name, title;
	TInt send_to, msgcode;
	auto_ptr<HBufC8> body(0);
	TTime t=GetTime();
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(1, send_to, _L("acc1"));
	TEST_EQUALS(1, msgcode, _L("acc2"));
	cl->Success(*info1, msgcode, GetTime(), ETrue);
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(1, send_to, _L("acc3"));
	TEST_EQUALS(2, msgcode, _L("acc4"));
	cl->Success(*info1, msgcode, GetTime(), ETrue);
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(-1, send_to, _L("acc5"));

	TBBBtDeviceInfo* info2=new (ELeave) 
		TBBBtDeviceInfo(TPtrC8( (TUint8*)"000002", 6), KNullDesC, 
			0x02, 0, 0);
	CleanupStack::PushL(info2);
	dev->AddItemL(info2);
	CleanupStack::Pop();
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(2, send_to, _L("acc2_1"));
	TEST_EQUALS(1, msgcode, _L("acc2_2"));
	cl->Success(*info2, msgcode, GetTime(), ETrue);
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(2, send_to, _L("acc2_3"));
	TEST_EQUALS(2, msgcode, _L("acc2_4"));
	cl->Success(*info2, msgcode, GetTime(), ETrue);
	cl->GetMessage(dev.get(), t, send_to, msgcode, name, title, body);
	TEST_EQUALS(-1, send_to, _L("acc2_5"));
}

void dt_test()
{
	TDateTime epoch; epoch.Set(1970, EJanuary, 0, 0, 0, 0, 0);
	TTime e(epoch);
	TInt unixtime=0;
	TTimeIntervalSeconds secs;
	User::LeaveIfError(GetTime().SecondsFrom(e, secs));
	unixtime=secs.Int();
	TBuf<20> t=_L("time: ");
	t.AppendNum(unixtime);
	RDebug::Print(t);
}
void run_tests_innerL()
{
	if (0) {
		dt_test();
	}

	TNoDefaults nd;
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("locatest")));
	auto_ptr<CBaActiveScheduler> sched(new (ELeave) CBaActiveScheduler);
	CActiveScheduler::Install(sched.get());

	if (0) {
		test_indep();
	}

	c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
	c->SetSettings(CBlackBoardSettings::NewL(*c, nd, KCLSettingsTuple));
	auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());
	c->SetBBDataFactory(f.get());
	auto_ptr<CBBSession> sess(CBBSession::NewL(*c, f.get()));
	c->SetBBSession(sess.get());
	c->SetActiveErrorReporter(output);
	c->Fs().Delete(_L("c:\\python-locatest.txt"));
	c->Fs().Delete(_L("c:\\REMOTE_IDS.db"));
	if (0) {
		run_comp_test();
	}
	if (0) {
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		test_accepted(cl.get());
	}
	if (0) {
		run_nw_test_2(ETrue);
	}
	if (0) {
		auto_ptr<CErrorLogger> errl(CErrorLogger::NewL());
		output->iAnotherReporter=errl.get();
		run_nw_test_2(EFalse);
		output->iAnotherReporter=0;
	}
	if (1) {
		test_scripts();
	}
	if (0) {
		GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
			_L("n1"));
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		run_test_3(cl.get());
		run_test_1(cl.get());
		run_test_2(cl.get());
	}
	if (0) {
		GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
			_L("n2"));
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		run_test_2(cl.get());
	}

	if (0) {
		auto_ptr<CErrorLogger> errl(CErrorLogger::NewL());
		//errl->LogFormatted(_L("test error from node\non two lines"));
		run_nw_test();
	}
	if (0) {
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		run_test_msg_1(cl.get());
	}
	if (0) {
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		run_test_4(cl.get());
	}

	if (0) {
		GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
			_L("n1"));
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		run_test_3(cl.get());
		run_test_1(cl.get());
		run_test_2(cl.get());
	}
	if (0) {
		GetContext()->Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
			_L("n2"));
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		run_test_2(cl.get());
	}

	if (0) {
		c->Fs().Delete(_L("c:\\system\\data\\context\\LOCATEST.db"));
		auto_ptr<CDb> db(CDb::NewL(*c, _L("LOCATEST"), EFileWrite));
		auto_ptr<CLocaLogicImpl> cl((CLocaLogicImpl*)CLocaLogic::NewL(*c, db->Db()));
		run_test_large(cl.get());
	}
}

class MRunBasic : public MRunnable {
	void run() { run_tests_innerL(); }
};

void run_testsL()
{
	TBuf<20> msg;
	{
		TFileName f;
		msg=_L("addr1: 0x");
		msg.AppendNum( (TUint)&f, EHex);
		RDebug::Print(msg);
	}
	{
		TFileName f;
		msg=_L("addr2: 0x");
		msg.AppendNum( (TUint)&f, EHex);
		RDebug::Print(msg);
	}
	User::__DbgMarkStart(RHeap::EUser);
	{
		RFs fs;
		User::LeaveIfError(fs.Connect());
		output=new (ELeave) MOutput;
		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
		User::LeaveIfError(
			output->foutput.Replace(fs, _L("c:\\locatest.txt"), EFileWrite));
		output->foutput.Write( _L8("\xff\xfe") );

		run_tests_innerL();
		//MRunBasic r;
		//test_oom(r);

		TBuf<30> b=_L("OK: "); b.AppendNum(ok); 
		b.Append(_L("/")); b.AppendNum(ok+not_ok); b.Append(_L("\n"));
		output->Write(b);
		output->Getch();
		delete output->cons;
		output->foutput.Close();
		delete output;
		fs.Close();
	}
	User::__DbgMarkEnd(RHeap::EUser,0);
}

void run_tests()
{
	CC_TRAPD(err, run_testsL());
}
