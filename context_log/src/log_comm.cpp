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

#include "log_comm.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "concretedata.h"

_LIT(pfx, "comm");

Clog_comm::Clog_comm(MApp_context& Context) : 
	CCheckedActive(EPriorityIdle, _L("Clog_comm")), Mfile_output_base(Context),  
	current_state(IDLE), logclient(0), filter(0), view(0), cb(0)
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("Clog_comm"));

}

void Clog_comm::ConstructL(i_status_notif* callback) 
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("ConstructL"));

	Mfile_output_base::ConstructL(pfx, false);
	
	logclient=CLogClient::NewL(Fs(), EPriorityIdle);
	filter=CLogFilter::NewL();
	view=CLogViewEvent::NewL(*logclient, EPriorityIdle);
	CActiveScheduler::Add(this); // add to scheduler
	
	cb=callback;
}

bool Clog_comm::LogsExist(TTime& From)
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("LogsExist"));

	int j=0;
	From=0;

	bool exists=false;
	for (j=0; j<2; j++) {
		CDir *dp=0;

		TFileName n=DataDir();
		
		if (j==0) {
			n.Replace(0, 1, _L("c"));
		} else {
			n.Replace(0, 1, _L("e"));
		}
		n.Append(_L("comm*.txt"));
		if ( Fs().GetDir(n, KEntryAttNormal, ESortByName, dp)==KErrNone) {
			auto_ptr<CDir> autod(dp);

			if (autod->Count()>0) exists=true;
			TEntry fe;
			for (int i=0; i<autod->Count(); i++) {
				fe=(*autod)[i];

				// let's look at the filename for when
				// the log was created
				TInt pos_dash=KErrNotFound, pos_period=KErrNotFound;
				TDesC &name = fe.iName;
				pos_dash=name.Find(_L("-"));
				pos_period=name.Find(_L("."));
				pos_dash+=1;
				if (pos_dash==KErrNotFound || pos_period==KErrNotFound) {
					continue;
				}
				if (pos_period-pos_dash != 15) {
					continue;
				}
				_LIT(c, "c");
				TBBTime comp(c);
				comp.FromStringL(name.Mid(pos_dash, pos_period-pos_dash));

				comp()-=TTimeIntervalDays(1);
				if (comp()>From) From=comp();
			}
		}
	}

	return exists;
}

Clog_comm* Clog_comm::NewL(MApp_context& Context, i_status_notif* callback)
{
	auto_ptr<Clog_comm> ret(new (ELeave) Clog_comm(Context));
	ret->ConstructL(callback);
	return ret.release();
}

bool Clog_comm::write_comm_log()
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("write_comm_log"));

	return write_comm_log(TTime(0));
}

bool Clog_comm::write_comm_log(TTime From)
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("write_comm_log"));

	if (current_state!=IDLE) return false;
	TBool logging;
	if (Settings().GetSettingL(SETTING_LOGGING_ENABLE, logging) && ! logging) return false;

	iFromTime=From;
	
	open_file(pfx);
	if (view->SetFilterL(*filter, iStatus)) {
		current_state=WAITING_ON_FILTER;
		SetActive();
	} else {
		if (cb) cb->finished();
		finished();
		return false;
	}
	
	return true;
}

Clog_comm::~Clog_comm()
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("~Clog_comm"));

	cb=0;
	Cancel();
	delete view;
	delete filter;
	delete logclient;
}

void Clog_comm::DoCancel()
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("DoCancel"));

	if (current_state!=IDLE) view->Cancel();
	finished();
}

void Clog_comm::finished()
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("finished"));

	if (current_state==IDLE) return;
	
	if (cb) cb->finished();
	
	close_file();
	
	current_state=IDLE;
}

void Clog_comm::write_event(const CLogEvent& ev)
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("write_event"));

	if (ev.Time()<iFromTime) return;

	write_time(ev.Time());
	
	TBuf<200> buf;
	_LIT(intf, "%d");
	
	_LIT(event, " EVENT ");
	write_to_output(event);
	
	_LIT(id, "ID: ");
	write_to_output(id);
	buf.Format(intf, (TInt)ev.Id());
	write_to_output(buf);
	
	_LIT(contact, " CONTACT: ");
	write_to_output(contact);
	buf.Format(intf, (TInt)ev.Contact());
	write_to_output(buf);
	
	_LIT(desc, " DESCRIPTION: ");
	write_to_output(desc);
	write_to_output(ev.Description());
	
	_LIT(dir, " DIRECTION: ");
	write_to_output(dir);
	write_to_output(ev.Direction());
	
	_LIT(dur, " DURATION: ");
	write_to_output(dur);
	buf.Format(intf, (TInt)ev.Duration());
	write_to_output(buf);
	
	_LIT(no, " NUMBER: ");
	write_to_output(no);
	write_to_output(ev.Number());
	
	_LIT(status, " STATUS: ");
	write_to_output(status);
	write_to_output(ev.Status());
	
	/*
	* Too personal (?) - content of SMS/mail
	_LIT(subj, " SUBJECT: ");
	write_to_output(subj);
	write_to_output(ev.Subject());
	*/
	
	_LIT(rem, " REMOTE: ");
	write_to_output(rem);
	write_to_output(ev.RemoteParty());
	
	/* 
	* How should this be converted? (If?)
	_LIT(data, " DATA ");
	write_to_output(data);
	write_to_output(ev.Data());
	*/
	
	write_nl();
}

void Clog_comm::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("Clog_comm"), _CL("CheckedRunL"));

	if (iStatus!=KErrNone) {
		TBuf<100> err;
		_LIT(errf, "Error %d occurred\r\n");
		err.Format(errf, iStatus);
		write_to_output(err);
		finished();
		return;
	}
	switch(current_state) {
	case WAITING_ON_FILTER:
		if (view->LastL(iStatus)) {
			current_state=WAITING_ON_MOVE;
			SetActive();
		} else {
			finished();
		}
		break;
	case WAITING_ON_MOVE:
		write_event(view->Event());
		if (view->PreviousL(iStatus)) {
			current_state=WAITING_ON_MOVE;
			SetActive();	
		} else {
			finished();
		}
		break;
	case IDLE: 
		// DOESN'T HAPPEN
		break;
	}
}
