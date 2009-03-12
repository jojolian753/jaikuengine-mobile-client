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

#include "independent.h"
#include "symbian_auto_ptr.h"
#include <e32svr.h>

class CThreadNotify : public CActive {
public:
	static CThreadNotify*	NewL(worker_info* info) {
		auto_ptr<CThreadNotify> ret(new (ELeave) CThreadNotify(info));
		ret->ConstructL();
		return ret.release();
	}
	~CThreadNotify() {
		Cancel();
		iThread.Close();
	}	
	void StartL() {
		User::LeaveIfError(iThread.Open(iInfo->worker_threadid));
		iThread.Logon(iStatus);
		SetActive();
	}

private:
	worker_info*	iInfo;
	RThread		iThread;

	CThreadNotify(worker_info* aInfo) : CActive(EPriorityLow), iInfo(aInfo) { }
	void ConstructL() {
		CActiveScheduler::Add(this);
	}
	void RunL() {
		TInt err=KErrNone;
		iInfo->exit_type=iThread.ExitType();
		iInfo->exit_reason=iThread.ExitReason();
		iInfo->exit_cat=iThread.ExitCategory();
		err=iStatus.Int();
		if (*iInfo->is_stopped == KRequestPending) {
			TRequestStatus *s=iInfo->is_stopped;
			User::RequestComplete(s, err);
		}
	}
	void DoCancel() {
		iThread.LogonCancel(iStatus);
	}

};

EXPORT_C worker_info::worker_info() : notify(0), is_started(false)
{
	do_stop=&i_do_stop;
	is_stopped=&i_is_stopped;
	*do_stop=KErrNone;
	*is_stopped=KErrNone;
}

EXPORT_C worker_info::~worker_info()
{
}

EXPORT_C bool worker_info::stop_requested()
{
	if (*do_stop!=KRequestPending) return true;
	return false;
}

EXPORT_C void worker_info::set_do_stop(TRequestStatus* s)
{
	// FIXME: race condition
	do_stop=s;
	*do_stop=KRequestPending;
}

EXPORT_C void worker_info::set_has_stopped(TRequestStatus* s)
{
	is_stopped=s;
	*is_stopped=KRequestPending;
}

EXPORT_C void worker_info::please_stop()
{
	is_stopped=&i_is_stopped;
	*is_stopped=KRequestPending;
	TInt tries=0;
	// give the other thread time to update the do_stop
	do {
		User::After(10*1000);
		tries++;
	} while(tries<2 && do_stop==&i_do_stop);
	
	RThread t;
	if (t.Open(worker_threadid)==KErrNone) {
		TRequestStatus* s;
		s=do_stop;
		t.RequestComplete(s, 1);
		t.Close();
	}
}

EXPORT_C void worker_info::stopped(TInt aCode)
{
	/*
	TRequestStatus* s=is_stopped;
	if (*s != KRequestPending) return;
	RThread t;
	t.Open(parent_threadid);
	t.RequestComplete(s, aCode);
	t.Close();
	*/
}

EXPORT_C void worker_info::wait_for_stop()
{
	if (is_stopped->Int()==KRequestPending) {
		RThread t;
		if (t.Open(worker_threadid)!=KErrNone) return;
		TRequestStatus s;
		TRequestStatus s2;
		s=s2=KRequestPending;
		t.Logon(s);
		if (t.ExitType()!=EExitPending) {
			t.LogonCancel(s);
			t.Close();
			return;
		}
		RTimer tr; tr.CreateLocal();
		tr.After(s2, TTimeIntervalMicroSeconds32(10*1000*1000));
		User::WaitForRequest(s, s2);
		if (s==KRequestPending) {
			t.Kill(1);
			User::WaitForRequest(s);
		} else {
			tr.Cancel();
			User::WaitForRequest(s2);
		}
		t.Close();
		tr.Close();
	}
}

EXPORT_C void worker_info::restart()
{
	delete notify; notify=0;
	exit_type=EExitPending;
	notify=CThreadNotify::NewL(this);
	RThread t;
	TInt err=t.Create(name, function, 16384, 8192, 2048*1024, (TAny*)this);
	if (err!=KErrNone) {
		User::Leave(err);
	}
	worker_threadid=t.Id(),
	t.SetPriority(priority);
	notify->StartL();
	t.Resume();
	t.Close();
	is_started=true;
}

EXPORT_C void independent_worker::start(const TDesC& name, TThreadFunction aFunction, TAny* worker_args,
					TThreadPriority aPriority)
{
	info.name=name.Left(info.name.MaxLength());
	RThread me;
	info.parent_threadid=me.Id();
	info.worker_args=worker_args;
	info.priority=aPriority;
	info.function=aFunction;
	info.restart();
}

EXPORT_C void independent_worker::stop()
{
	delete info.notify; info.notify=0;

	if (!info.is_started) return;
	if (info.exit_type!=EExitPending) return;

	info.please_stop();

	TTimeIntervalMicroSeconds32 w(50*1000);
	User::After(w);

	info.wait_for_stop();
	info.is_started=false;
}

EXPORT_C independent_worker::~independent_worker()
{
	stop();
}
	
EXPORT_C independent_worker::independent_worker()
{
	info.is_started=false;
}
