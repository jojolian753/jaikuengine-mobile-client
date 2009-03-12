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
#include "..\..\BlackBoard\src\testdriver_bbdata_base.cpp"

#include "csd_cell.h"
#include "csd_bluetooth.h"
#include "csd_event.h"
#include "bbutil.h"

void csd_cell()
{
	_LIT(KCell, "location.value");
	TBBCellId c(KCell);
	c.iMCC()=15;
	c.iMNC()=16;
	c.iShortName()=_L("elisa");
	c.iLocationAreaCode()=21;
	c.iCellId()=22;

	test_conversions_inner_nonative(c, _L("15, 16, elisa, 21, 22"), 
		_L("<location.value><location.mcc>15</location.mcc><location.mnc>16</location.mnc><location.network>elisa</location.network><location.lac>21</location.lac><location.cellid>22</location.cellid></location.value>"),
		_L(""), KCell, 0);

}

void csd_bt()
{
	bb_auto_ptr<TBBBtDeviceInfo> dp(new (ELeave) TBBBtDeviceInfo());
	
	TBBBtDeviceInfo& d=*dp;
	d.iMAC()=_L8("\x13\x14\xa5\x61\x62\xb3");
	d.iNick()=_L("bogrund-4");
	d.iMajorClass()=1;
	d.iMinorClass()=3;
	d.iServiceClass()=640;

	if(1) {
		TBBBtDeviceInfo d2, d3;
	test_conversions_inner_nonative_2(d, _L("1314a56162b3 [bogrund-4,1:3:640]"), 
		_L("<device><bt.mac>1314a56162b3</bt.mac><bt.name>bogrund-4</bt.name><bt.majorclass>1</bt.majorclass><bt.minorclass>3</bt.minorclass><bt.serviceclass>640</bt.serviceclass></device>"),
		_L(""), KCSDBt, 0, true, d2, d3);
	}
	
	auto_ptr<CBBBtDeviceList> l(CBBBtDeviceList::NewL());
	l->AddItemL(dp.get());
	dp.release();

	if (1) {
		auto_ptr<CBBBtDeviceList> l2(CBBBtDeviceList::NewL());
		auto_ptr<CBBBtDeviceList> l3(CBBBtDeviceList::NewL());
		test_conversions_inner_nonative_2(*l, _L("1314a56162b3 [bogrund-4,1:3:640]"), 
			_L("<devices><device><bt.mac>1314a56162b3</bt.mac><bt.name>bogrund-4</bt.name><bt.majorclass>1</bt.majorclass><bt.minorclass>3</bt.minorclass><bt.serviceclass>640</bt.serviceclass></device></devices>"),
			_L(""), KCSDBtList, 1, true, *l2, *l3);
	}

	bb_auto_ptr<TBBBtDeviceInfo> dp2(new (ELeave) TBBBtDeviceInfo());
	
	TBBBtDeviceInfo& d2=*dp2;
	d2.iMAC()=_L8("\x13\x14\xa6\x61\x62\xb3");
	d2.iNick()=_L("bogrund-5");
	d2.iMajorClass()=7;
	d2.iMinorClass()=3;
	d2.iServiceClass()=64;
	l->AddItemL(dp2.get());
	dp2.release();

	{
		auto_ptr<CBBBtDeviceList> l2(CBBBtDeviceList::NewL());
		auto_ptr<CBBBtDeviceList> l3(CBBBtDeviceList::NewL());
		test_conversions_inner_nonative_2(*l, _L("1314a56162b3 [bogrund-4,1:3:640] 1314a66162b3 [bogrund-5,7:3:64]"), 
			_L("<devices>"
			L"<device><bt.mac>1314a56162b3</bt.mac><bt.name>bogrund-4</bt.name><bt.majorclass>1</bt.majorclass><bt.minorclass>3</bt.minorclass><bt.serviceclass>640</bt.serviceclass></device>"
			L"<device><bt.mac>1314a66162b3</bt.mac><bt.name>bogrund-5</bt.name><bt.majorclass>7</bt.majorclass><bt.minorclass>3</bt.minorclass><bt.serviceclass>64</bt.serviceclass></device>"
			L"</devices>"),
			_L(""), KCSDBtList, 1, true, *l2, *l3);
	}
}

void csd_event()
{
	_LIT(KTime, "datetime");
	_LIT(Kmoment, "20050216T200500 ");

	TBBTime t(KTime); t.FromStringL(Kmoment);

	auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());

	auto_ptr<CBBSensorEvent> e(new (ELeave) CBBSensorEvent(_L("dummyname"), KCellIdTuple,f.get(), t()));
	{
		auto_ptr<CBBSensorEvent> e1(new (ELeave) CBBSensorEvent(_L("dummyname"), KCellIdTuple,f.get()));
		auto_ptr<CBBSensorEvent> e2(new (ELeave) CBBSensorEvent(_L("dummyname"), KCellIdTuple,f.get()));
		test_conversions_inner_nonative_2(*e, Kmoment, _L("<event><datetime>20050216T200500</datetime></event>"),
			_L(""), KEvent, 1, true, *e1, *e2);
	}

	_LIT(KX, "x");
	e->iData()=new (ELeave) TBBInt(12, KX);

	{
		TBuf<30> b=Kmoment; b.AppendNum(12);
	
		auto_ptr<CBBSensorEvent> e1(new (ELeave) CBBSensorEvent(_L("dummyname"), KCellIdTuple,f.get()));
		auto_ptr<CBBSensorEvent> e2(new (ELeave) CBBSensorEvent(_L("dummyname"), KCellIdTuple,f.get()));
		test_conversions_inner_nonative_2(*e, b, _L("<event><datetime>20050216T200500</datetime><x module=\"0x1020811a\" id=\"1\" major_version=\"1\" minor_version=\"0\">12</x></event>"),
			_L(""), KEvent, 2, true, *e1, *e2);
	}

	TBBTupleMeta tm(1, 1, _L("sub"));
	TBBTupleMeta tm1, tm2;
	test_conversions_inner_nonative_2(tm, _L("1 1 sub"), _L("<tuplename><module_uid>1</module_uid><module_id>1</module_id><subname>sub</subname></tuplename>"),
		_L(""), KMeta, 3, true, tm1, tm2);

	bb_auto_ptr<TBBInt> bbi(new (ELeave) TBBInt(12, KX));
	auto_ptr<CBBTuple> tuple(new (ELeave) CBBTuple(f.get(), 17, 1, 1, _L("sub"), bbi.get()));
	bbi.release();
	{
		auto_ptr<CBBTuple> tuple1(new (ELeave) CBBTuple(f.get()));
		auto_ptr<CBBTuple> tuple2(new (ELeave) CBBTuple(f.get()));
		TBuf<100> b=_L("17 ");
		b.Append(_L("1 1 sub "));
		b.AppendNum(12);
		test_conversions_inner_nonative_2(*tuple, b, 
			_L("<tuple><id>17</id>"
			L"<tuplename><module_uid>1</module_uid><module_id>0x1</module_id><subname>sub</subname></tuplename>"
			L"<x module=\"0x1020811a\" id=\"1\" major_version=\"1\" minor_version=\"0\">12</x>"
			L"</tuple>"),
			_L(""), KEvent, 4, true, *tuple1, *tuple2);
	}
}

class RunBt : public MRunnable2
{
	void run() {
		csd_bt();
	}
	void stop() { }
};

class RunEvent : public MRunnable2
{
	void run() {
		csd_event();
	}
	void stop() { }
};

class RunCell : public MRunnable2
{
	void run() {
		csd_cell();
	}
	void stop() { }
};


void run_oom()
{
	{
		RunBt r;
		test_oom2(r);
	}
	{
		RunEvent r;
		test_oom2(r);
	}
	{
		RunCell r;
		test_oom2(r);
	}
}

void run_tests()
{
	User::__DbgMarkStart(RHeap::EUser);
	{
		output=new (ELeave) MOutput;

		RAFs fs; fs.ConnectLA();
		output->foutput.Replace(fs, _L("blackboardtest.txt"), EFileWrite);

		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
		TInt err=KErrNone;

		CC_TRAP(err, csd_event());
		TEST_EQUALS(err, KErrNone, _L("csd_event all"));

		if (not_ok==0) {
			CC_TRAP(err, csd_cell());
			TEST_EQUALS(err, KErrNone, _L("csd_cell all"));
		}

		if (not_ok==0) {
			CC_TRAP(err, csd_bt());
			TEST_EQUALS(err, KErrNone, _L("csd_bt all"));
		}

		if (not_ok==0) {
			run_oom();
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
