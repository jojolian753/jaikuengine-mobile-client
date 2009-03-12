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
#include "blackboardserver.h"
#include <basched.h>
#include "context_uids.h"
#include "bbtypes.h"
#include "subscriptions.h"
#include "raii_f32file.h"

void get_by_nameL(CTupleStore* ts, const TTupleName& name, const TDesC& subname, TUint& id, TDes8& data_into, TUint len)
{
	RADbColReadStream rs;
	TUint size; 
	if (! ts->FirstL(ETupleData, name, subname, ETrue) ) User::Leave(KErrNotFound);

	TTupleName name_into;
	TBuf<128> subname_into;
	TComponentName cn;
	ts->GetCurrentL(name_into, subname_into, id, cn, rs, size);
	TBuf<100> n=state; n.Append(_L(":datasize"));
	TEST_EQUALS(len, size, n);
	data_into.Zero();
	rs.ReadL(data_into, len);
}

void get_by_idL(CTupleStore* ts, TUint id, TTupleName& name_into, TDes& subname_into, TDes8& data_into, TUint len)
{
	TUint size;
	RADbColReadStream rs;
	TComponentName cn;
	ts->SeekL(id);
	ts->GetCurrentL(name_into, subname_into, id, cn, rs, size);

	TBuf<100> n=state; n.Append(_L(":datasize"));
	TEST_EQUALS(len, size, n);

	data_into.Zero();
	rs.ReadL(data_into, len);
}

TUint put_and_get_test(CTupleStore* ts, const TTupleName& tuplename, const TDesC& subname, const TDesC8& data,
		      const TDesC& i_testname, TBool aReplace=ETrue, TBool remove=EFalse)
{
	TComponentName cn1={ CONTEXT_UID_BLACKBOARDSERVER, 1 };
	TBuf<100> name; TBuf<100> testname;

	int i; TUint id=(TUint)-1;
	for (i=0; i<2; i++) {
		testname=i_testname;
		testname.Append(_L(":")); testname.AppendNum(i);

		auto_ptr<HBufC8> des(HBufC8::NewL(data.Length()));
		TInt err_subname_before=KErrNone; 

		if (subname.Length()>0) {
			name=testname; name.Append(_L("subname:before"));
			TUint id_before;
			TPtr8 p=des->Des();
			CC_TRAP(err_subname_before, get_by_nameL(ts, tuplename, _L(""), id_before, p, data.Length()));
			test_for_error_leave(err_subname_before, KErrNotFound, name);
			des->Des().Zero();
		}

		id=ts->PutL(ETupleData, tuplename, subname, cn1, data, EBBPriorityNormal, aReplace);

		if (subname.Length()>0) {
			name=testname; name.Append(_L("subname:after"));
			TInt err_subname_after=KErrNone; TUint id_after;
			TPtr8 p=des->Des();
			CC_TRAP(err_subname_after, get_by_nameL(ts, tuplename, _L(""), id_after, p, data.Length()));
			test_for_error_leave(err_subname_after, err_subname_before, name);
			des->Des().Zero();
		}

		TTupleName ret_tn; TBuf<128> ret_sub;
		TInt err=0;
		if (aReplace) {
			TUint idr;
			TPtr8 p=des->Des();
			get_by_nameL(ts, tuplename, subname, idr, p, data.Length());
			name=testname; name.Append(_L(":0:0"));
			TEST_EQUALS(id, idr, name);
			name=testname; name.Append(_L(":0:1"));
			TEST_EQUALS(TStringMatch8(*des), TStringMatch8(data), name);

			TTupleName tn=tuplename;
			name=testname; name.Append(_L(":0:2"));
			tn.iModule.iUid=KBBNoUidValue;
			p=des->Des();
			CC_TRAP(err, get_by_nameL(ts, tn, subname, idr, p, data.Length()));
			test_for_error_leave(err, KErrNotFound, name);

			tn=tuplename;
			name=testname; name.Append(_L(":0:3"));
			tn.iId=KBBNoId;
			p=des->Des();
			CC_TRAP(err, get_by_nameL(ts, tn, subname, idr, p, data.Length()));
			test_for_error_leave(err, KErrNotFound, name);
		}

		TPtr8 p=des->Des();
		get_by_idL(ts, id, ret_tn, ret_sub, p, data.Length());
		name=testname; name.Append(_L(":n0"));
		TEST_EQUALS(ret_tn, tuplename, name);
		name=testname; name.Append(_L(":n1"));
		TEST_EQUALS(TStringMatch8(data), TStringMatch8(*des), name);
		name=testname; name.Append(_L(":n2"));
		TEST_EQUALS(TStringMatch(ret_sub), TStringMatch(subname), name);
		name=testname; name.Append(_L(":n3"));
		p=des->Des();
		if (!aReplace) {
			CC_TRAP(err, get_by_idL(ts, id+1, ret_tn, ret_sub, p, data.Length()));
			name=testname; name.Append(_L(":n4"));
			test_for_error_leave(err, KErrNotFound, name);
		}

		if (i==0 || remove) {
			TTupleType ret_tt;
			ts->DeleteL(id, ret_tt, ret_tn, ret_sub);
			TPtr8 p=des->Des();
			CC_TRAP(err, get_by_idL(ts, id, ret_tn, ret_sub, p, data.Length()));
			name=testname; name.Append(_L(":n5"));
			test_for_error_leave(err, KErrNotFound, name);
		}
	}

	return id;
}

void test_iterate_count(CTupleStore* ts, const TTupleName& tuplename, const TDesC& subname,
		      TInt count, const TDesC& testname)
{
	TInt got_count=0;
	TBool more=ts->FirstL(ETupleData, tuplename, subname);
	while (more) {
		got_count++;
		more=ts->NextL();
	}
	TEST_EQUALS(got_count, count, testname);
}

void remove_all(CTupleStore* ts)
{
	RArray<TUint> ids(20);
	CleanupClosePushL(ids);
	TBool more=ts->FirstL(ETupleData, KAnyTuple, _L(""));
	while (more) {
		User::LeaveIfError(ids.Append(ts->GetCurrentIdL()));
		more=ts->NextL();
	}
	TInt i;
	TTupleType tt;
	TTupleName tn; TBuf<128> s;
	for (i=0; i<ids.Count(); i++) {
		ts->DeleteL(ids[i], tt, tn, s);
	}
	CleanupStack::PopAndDestroy();
}

void test_tuplestore(MApp_context* c, TInt& restart_at)
{
	TBuf<128> subname;

	state=_L("ts:create:0");
	auto_ptr<CDb> db=CDb::NewL(*c, _L("TUPLE"), EFileRead|EFileWrite|EFileShareAny);
	state=_L("ts:create:1");
	auto_ptr<CTupleStore> ts=CTupleStore::NewL(db->Db(), *c);
	TTupleName tn1={ CONTEXT_UID_BLACKBOARDSERVER, 1};
	TComponentName cn1={ CONTEXT_UID_BLACKBOARDSERVER, 1 };
	_LIT8(input1, "test");
	_LIT8(input2, "tes2");
	TTupleName tn2={ CONTEXT_UID_BLACKBOARDSERVER, 2};
	_LIT8(input3, "kjdsafaj");
	TTupleName tn4={ CONTEXT_UID_BLACKBOARDSERVER, 4};
	TTupleName tn5={ 1, 1 };
	_LIT8(input5, "0ihdsvp0+i1");

	TTupleName tn5a={ CONTEXT_UID_BLACKBOARDSERVER, KBBAnyId};
	TTupleName tn6={ CONTEXT_UID_BLACKBOARDSERVER, 6};
	_LIT8(input6, "98324jx");
	_LIT(subname6, "s1");
	_LIT(subname7, "s2");
	TTupleName tn3;
	TBuf8<200> des; 


	switch(restart_at) {
	case 0:
		goto g0;
	case 1:
		goto g1;
	case 2:
		goto g2;
	case 3:
		goto g3;
	case 4:
		goto g4;
	case 5:
		goto g5;
	case 6:
		goto g6;
	case 7:
		goto g7;
	case 8:
		goto g8;
	};

g0:
	state=_L("ts:remove_all");
	remove_all(ts.get());

	restart_at=1;
g1:
	static TUint id, id1, id2;

	state=_L("ts:put");
	id=id1=put_and_get_test(ts.get(), tn1, _L(""), input1, state);

	restart_at=2;
g2:
	state=_L("ts:put2");
	id1=put_and_get_test(ts.get(), tn1, _L(""), input2, state);

	restart_at=3;
g3:
	state=_L("ts:second");
	id2=put_and_get_test(ts.get(), tn2, _L(""), input3, state);
	
	state=_L("ts:second:1");
	des.Zero();
	state=_L("ts:second:2");
	get_by_idL(ts.get(), id2, tn3, subname, des, input3().Length());
	TEST_EQUALS(tn2, tn3, state);
	state=_L("ts:second:3");
	TEST_EQUALS(TStringMatch8(input3), TStringMatch8(des), state);
	state=_L("ts:second:4");
	get_by_idL(ts.get(), id1, tn3, subname, des, input2().Length());
	TEST_EQUALS(tn1, tn3, state);
	state=_L("ts:second:5");
	TEST_EQUALS(TStringMatch8(input2), TStringMatch8(des), state);
	
#ifdef __WINS__
	restart_at=4;
g4:
	{
		state=_L("ts:big");

		auto_ptr<HBufC8> b1(HBufC8::NewL(1024*16));
		b1->Des().Fill('x', b1->Des().MaxLength());
		auto_ptr<HBufC8> b2(HBufC8::NewL(1024*16));

		id=ts->PutL(ETupleData, tn4, _L(""), cn1, *b1, EBBPriorityNormal, ETrue);
		TPtr8 p=b2->Des();
		get_by_idL(ts.get(), id, tn3, subname, p, b1->Des().Length());
		TEST_EQUALS(TStringMatch8(*b1), TStringMatch8(*b2), state);

		state=_L("ts:delete");
		ts->DeleteL(ETupleData, tn4, _L(""));
		RADbColReadStream rs;
		TInt err;
		CC_TRAP(err, ts->SeekL(id));
		test_for_error_leave(err, KErrNotFound, state);
		TBool found=ts->FirstL(ETupleData, tn4, _L(""));
		TEST_EQUALS(found, EFalse, state);
	}
#endif

	restart_at=5;
g5:
	state=_L("ts:iterate:1");
	ts->PutL(ETupleData, tn5, _L(""), cn1, input5, EBBPriorityNormal, ETrue);

	test_iterate_count(ts.get(), tn5, _L(""), 1, state);
	state=_L("ts:iterate:any");
	test_iterate_count(ts.get(), KAnyTuple, _L(""), 3, state);
	state=_L("ts:iterate:bbuid");
	test_iterate_count(ts.get(), tn5a, _L(""), 2, state);

	restart_at=6;
g6:
	state=_L("ts:subname:0:0");
	id2=put_and_get_test(ts.get(), tn6, subname6, input6, state);
	state=_L("ts:subname:0:1");
	test_iterate_count(ts.get(), KAnyTuple, _L(""), 4, state);
	state=_L("ts:subname:0:2");
	test_iterate_count(ts.get(), tn6, _L(""), 1, state);
	state=_L("ts:subname:0:3");
	test_iterate_count(ts.get(), tn6, subname6, 1, state);

	restart_at=7;
g7:

	id2=put_and_get_test(ts.get(), tn6, subname7, input6, state);
	test_iterate_count(ts.get(), tn6, _L(""), 2, state);
	test_iterate_count(ts.get(), tn6, subname6, 1, state);
	restart_at=8;
g8:
	{
		_LIT8(input8, "xx");
		_LIT8(input9, "xy");
		TBuf<128> subname8; subname8.Fill('x', 128); 
		TBuf<128> subname9; subname9.Fill('x', 128); 
		subname9.Replace(127, 1, _L("y"));
		state=_L("ts:long_subname:0");
		id2=put_and_get_test(ts.get(), tn6, subname8, input8, state);
		state=_L("ts:long_subname:1");
		id2=put_and_get_test(ts.get(), tn6, subname9, input9, state);
		state=_L("ts:long_subname:2");
		id2=put_and_get_test(ts.get(), tn6, subname8, input8, state);
	}
}

class RunTupleStore : public MRunnable {
public:
	MApp_context*& ic; TInt& ir;
	RunTupleStore(MApp_context*&c, TInt& restart_on) : ic(c), ir(restart_on) { }
	void run() { test_tuplestore(ic, ir); }
};


void test_tx(MApp_context* c)
{
	state=_L("tx:create:0");
	auto_ptr<CDb> db=CDb::NewL(*c, _L("TUPLE"), EFileRead|EFileWrite|EFileShareAny);
	state=_L("tx:create:1");
	auto_ptr<CTupleStore> ts=CTupleStore::NewL(db->Db(), *c);

	state=_L("tx:remove_all");
	remove_all(ts.get());

	TTupleName tn1={ 2, 1 };
	TTupleName tn2={ 3, 1 };
	TUint id1, id2=(TUint)-1;
	_LIT8(input1, "inp1");
	_LIT8(input2, "inp2");
	TComponentName cn1={ 1, 1 };

	TTupleName tn3, tn4;
	TBuf<128> subname3, subname4;
	state=_L("tx:put1");
	id1=ts->PutL(ETupleData, tn1, _L(""), cn1, input1, EBBPriorityNormal, ETrue);
	CC_TRAPD(errtx, {
		db->BeginL();
		{
			TTransactionHolder th(*db);
			state=_L("tx:delete1");
			TTupleType tt;
			ts->DeleteL(id1, tt, tn3, subname3);
			state=_L("tx:put2");
			id2=ts->PutL(ETupleData, tn2, _L(""), cn1, input2, EBBPriorityNormal, ETrue);
		}
		db->CommitL();
	});

	TBuf8<10> des;
	state=_L("tx:get1");
	CC_TRAPD(err1, get_by_idL(ts.get(), id1, tn3, subname3, des, input1().Length()));
	if (err1!=KErrNone && err1!=KErrNotFound) User::Leave(err1);
	state=_L("tx:get2");
	CC_TRAPD(err2, get_by_idL(ts.get(), id2, tn4, subname4, des, input2().Length()));
	if (err2!=KErrNone && err2!=KErrNotFound) User::Leave(err2);

	if (err1==KErrNone) TEST_EQUALS(err2, KErrNotFound, _L("tx:1_and_2_exist"));
	if (err1==KErrNotFound) TEST_EQUALS(err2, KErrNone, _L("tx:1_and_2_both_missing"));

}

class RunTx : public MRunnable {
public:
	MApp_context*& ic;;
	RunTx(MApp_context*&c) : ic(c){ }
	void run() { test_tx(ic); }
};

void test_tx_outer(MApp_context* c)
{
	RunTx tr(c);
	CC_TRAPD(err, tr.run());
	state.Append(_L("tx_outer"));
	TEST_EQUALS(err, KErrNone, state);
	test_oom(tr);
}

void test_tuplestore_outer(MApp_context* c)
{
	TInt restart_at=0;
	RunTupleStore tr(c, restart_at);
	CC_TRAPD(err, tr.run());
	state.Append(_L(":outer0"));
	TEST_EQUALS(err, KErrNone, state);
	restart_at=0;
	test_oom(tr);
}

class TDummyObserver : public MBlackBoardObserver {
private:
	virtual void NotifyL(TUint , TBBPriority ,
			const TTupleName& , const TDesC& , 
			const TComponentName& ,
			const TDesC8& ) { return; }
	virtual void NotifyL(TUint , TBBPriority ) { return; }
};

const TDesC& make_not_num(const TDesC& base, TDes& Into, const TDesC& str)
{
	Into=base;
	Into.Append(str);
	return Into;
}

void test_Subscriptions_inner(MApp_context* /*c*/, CSubscriptions*& n, TInt& step,
			      TBBPriority prio)
{
	TBuf<200> name, basename;
	basename=_L("not::"); basename.AppendNum(step); basename.Append(_L("::"));
	
	static TDummyObserver t1;
	static TTupleName n2=KAnyTuple;
	static TTupleName n1={ { CONTEXT_UID_BLACKBOARDSERVER }, 1 };
	static TTupleName n3={ { CONTEXT_UID_BLACKBOARDSERVER }, 2 };
	static TDummyObserver t2;
	TBBPriority pr;

	switch(step) {
	case 1: goto g1; 
	case 2: goto g2;
	case 3: goto g3;
	case 4: goto g4;
	case 5: goto g5;
	case 6: goto g6;
	case 7: goto g7;
	case 8: goto g8;
	case 9: goto g9;
	case 10: goto g10; 
	case 20: goto g20;
	case 30: goto g30;
	case 40: goto g40;
	case 50: goto g50;
	case 60: goto g60;
	case 90: goto g90;
	}

	if (!n) n=CSubscriptions::NewL();

	
	step=1;
g1:
	n2=KAnyTuple;
	n->AddNotificationL(&t1, n2, prio);

	step=10;
g10:
	TEST_EQUALS( &t1, n->FirstL(n1, pr), make_not_num(basename, name, _L("not::0")) );

	step=2;
g2:
	n->DeleteNotificationL(&t1, n2);
	step=20;
g20:
	TEST_EQUALS( (void*)0, n->FirstL(n1, pr), make_not_num(basename, name, _L("not::1") ));

	step=3;
g3:
	n2.iModule=n1.iModule;
	n->AddNotificationL(&t1, n2, prio);

	step=30;
g30:
	TEST_EQUALS( &t1, n->FirstL(n1, pr), make_not_num(basename, name, _L("not::2") ));

	step=4;
g4:
	n->DeleteNotificationL(&t1, n2);
	step=40;
g40:
	TEST_EQUALS( (void*)0, n->FirstL(n1, pr), make_not_num(basename, name, _L("not::3") ));

	step=5;
g5:
	n2.iId=n1.iId;
	n->AddNotificationL(&t1, n2, prio);

	step=50;
g50:	
	TEST_EQUALS( &t1, n->FirstL(n1, pr), make_not_num(basename, name, _L("not::4") ));

	step=6;
g6:
	n->DeleteNotificationL(&t1, n2);
	step=60;
g60:
	TEST_EQUALS( (void*)0, n->FirstL(n1, pr), make_not_num(basename, name, _L("not::5") ));

	step=7;
g7:
	n->AddNotificationL(&t1, n2, prio);

	step=8;
g8:
	n->AddNotificationL(&t2, n3, prio);

	step=9;
g9:
	TEST_EQUALS( &t1, n->FirstL(n2, pr), make_not_num(basename, name, _L("not::6") ));
	step=90;
g90:
	TEST_EQUALS( &t2, n->FirstL(n3, pr), make_not_num(basename, name, _L("not::7") ));

}

class RunNot : public MRunnable {
public:
	MApp_context*& ic;
	CSubscriptions* n;
	TBBPriority p;
	RunNot(MApp_context*&c) : ic(c), n(0), p(EBBPriorityNormal) { }
	void run() { TInt step=0; CC_TRAPD(err, test_Subscriptions_inner(ic, n, step, p)); delete n; n=0; User::LeaveIfError(err); }
};

class RunNot2 : public MRunnable2 {
public:
	MApp_context*& ic;
	CSubscriptions* n;
	TInt step;
	TBBPriority p;
	RunNot2(MApp_context*&c) : ic(c), n(0), step(0), p(EBBPriorityNormal) { }
	void run() { test_Subscriptions_inner(ic, n, step, p); }
	void stop() { delete n; n=0; }
};

void test_Subscriptions(MApp_context* c)
{
	RunNot tr(c);
	CC_TRAPD(err, tr.run());
	state.Append(_L("not_outer"));
	TEST_EQUALS(err, KErrNone, state);
	test_oom(tr);

	RunNot2 tr2(c);
	CC_TRAP(err, tr2.run());
	state.Append(_L("not_outer2"));
	TEST_EQUALS(err, KErrNone, state);
	test_oom2(tr2);

	RunNot2 tr3(c);
	tr3.p=EBBPriorityHigh;
	CC_TRAP(err, tr3.run());
	state.Append(_L("not_outer3"));
	TEST_EQUALS(err, KErrNone, state);
	test_oom2(tr3);
}

class TStopObserver : public MBlackBoardObserver {
public:
	TUint iLastId;
	TInt iNotifyCount;
	TStopObserver() : iLastId(0), iNotifyCount(0) { }
	virtual void NotifyL(TUint aId, TBBPriority ,
			const TTupleName& , const TDesC& , 
			const TComponentName& ,
			const TDesC8& ) { iLastId=aId; ++iNotifyCount; }
	virtual void NotifyL(TUint aId, TBBPriority ) { iLastId=aId; ++iNotifyCount; }
};

void test_bbserver_inner(MApp_context* c, CBlackBoardServer*& s)
{
	if (!s) {
		s=CBlackBoardServer::NewL(*c, EFalse);
	}
	s->DeleteAllNotificationsL();

	TTupleName n1={ { CONTEXT_UID_BLACKBOARDSERVER }, 2 };
	TComponentName c1={ { CONTEXT_UID_BLACKBOARDSERVER }, 1 };
	TStopObserver t1;

	s->AddNotificationL(&t1, n1, ETrue, EBBPriorityNormal);
	t1.iLastId=0; t1.iNotifyCount=0;

	TStopObserver tc1;
	s->AddNotificationL(&tc1, c1, ETrue, EBBPriorityNormal);
	tc1.iLastId=0; tc1.iNotifyCount=0;

	TUint id;
	TInt err=s->PutL(n1, _L(""), c1, _L8(""), EBBPriorityNormal, ETrue, id);
	if (err==KErrNone) {
		TEST_EQUALS(t1.iLastId, id, _L("bbs::0"));
		TEST_EQUALS(t1.iNotifyCount, 1, _L("bbs::1"));

		TEST_EQUALS(tc1.iLastId, id, _L("bbs::0.1"));
		TEST_EQUALS(tc1.iNotifyCount, 1, _L("bbs::1.1"));
	}
	err=s->PutL(n1, _L(""), c1, _L8(""), EBBPriorityNormal, ETrue, id);

	if (err==KErrNone) {
		TEST_EQUALS(t1.iLastId, id, _L("bbs::2"));
		TEST_EQUALS(t1.iNotifyCount, 2, _L("bbs::3"));

		TEST_EQUALS(tc1.iLastId, id, _L("bbs::2.1"));
		TEST_EQUALS(tc1.iNotifyCount, 2, _L("bbs::3.1"));
	}

	TStopObserver t2, tc2;
	err=s->AddNotificationL(&t2, n1, ETrue, EBBPriorityNormal);
	err=s->AddNotificationL(&tc2, c1, ETrue, EBBPriorityNormal);
	if (err==KErrNone) {
		TEST_EQUALS(t2.iLastId, id, _L("bbs::4"));
		TEST_EQUALS(t2.iNotifyCount, 1, _L("bbs::5"));

		TEST_EQUALS(tc2.iLastId, id, _L("bbs::4.1"));
		TEST_EQUALS(tc2.iNotifyCount, 1, _L("bbs::5.1"));
	}
}

class RunBBS2 : public MRunnable2 {
public:
	MApp_context*& ic;
	CBlackBoardServer* s;
	TInt step;
	RunBBS2(MApp_context*&c) : ic(c), s(0), step(0) { }
	void run() { /*ic->ResetCallStack();*/ test_bbserver_inner(ic, s); }
	void stop() { delete s; s=0; }
};

void test_bbserver(MApp_context* c)
{
	c->Fs().Delete(_L("TUPLE.db"));

	RunBBS2 r(c);
	{
		CC_TRAPD(err, r.run());
		r.stop();
		User::LeaveIfError(err);
	}
	test_oom2(r);
}

void run_tests()
{
	User::__DbgMarkStart(RHeap::EUser);
	{
	RAFs fs; fs.ConnectLA();
        TInt err, pushed=0;
	output=new (ELeave) MOutput;
        CC_TRAP(err, output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen)));
	TEST_EQUALS(err, KErrNone, _L("output create"));
	output->foutput.Replace(fs, _L("blackboardservertest.txt"), EFileWrite);
	
	if (!err) {
		CleanupStack::PushL(output); ++pushed;
		CApp_context* c=0;
		CC_TRAP(err, c=CApp_context::NewL(true, _L("BlackBoardServer")));
		TEST_EQUALS(err, KErrNone, _L("appcontext create"));
		if (!err) {
			CleanupStack::PushL(c); ++pushed;

			CActiveScheduler* activeScheduler = new CBaActiveScheduler;
			TEST_NOT_EQUALS(activeScheduler, (void*)0, _L("create AS"));
			if (activeScheduler) {
				CleanupStack::PushL(activeScheduler); ++pushed;
				CActiveScheduler::Install(activeScheduler);

				c->Fs().Delete(_L("TUPLE.db"));

				CC_TRAP(err, test_bbserver(c));
				TEST_EQUALS(err, KErrNone, _L("test bbs"));

				c->Fs().Delete(_L("TUPLE.db"));

				CC_TRAP(err, test_Subscriptions(c));
				TEST_EQUALS(err, KErrNone, _L("test tx"));

				CC_TRAP(err, test_tx_outer(c));
				TEST_EQUALS(err, KErrNone, _L("test tx"));

				CC_TRAP(err, test_tuplestore_outer(c));
				TEST_EQUALS(err, KErrNone, _L("test ts"));
				
			}
		}
	}

        TBuf<30> b=_L("OK: "); b.AppendNum(ok); b.Append(_L("/")); b.AppendNum(ok+not_ok); b.Append(_L("\n"));
        output->Write(b);
	output->Getch();

	CleanupStack::PopAndDestroy(pushed);
	}
        User::__DbgMarkEnd(RHeap::EUser,0);
}
