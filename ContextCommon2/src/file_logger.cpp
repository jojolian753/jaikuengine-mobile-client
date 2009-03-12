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

#include "file_logger.h"
#include "cl_settings.h"
#include "break.h"

#include "e32std.h"
#include "csd_event.h"

void Cfile_logger::get_value(const MBBData* d)
{
	if (!d) return;
	iBuf->Des().Zero();
	TInt err=KErrOverflow;
	while (err==KErrOverflow) {
		iBuf->Des().Zero();
		TPtr p=iBuf->Des();
		CC_TRAP(err, d->IntoStringL(p));
		if (err==KErrOverflow) {
			CC_TRAP(err, iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength()*2));
			if (err==KErrNone) err=KErrOverflow;
		}
	}
}
void Cfile_logger::get_value(const CBBSensorEvent& aEvent)
{
	const MBBData* d=aEvent.iData();
	get_value(d);
}

void Cfile_logger::NewSensorEventL(const TTupleName& aTuple, const TDesC& , const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("Cfile_logger"), _CL("NewSensorEventL"));

	if (aEvent.iPriority()<priority_limit) return;
	if (aEvent.iPriority()==CBBSensorEvent::UNCHANGED_VALUE) return;
	if (aTuple==KStatusTuple) return;

	get_value(aEvent);
	new_value(aEvent.iPriority(), aEvent.iName(), *iBuf, aEvent.iStamp());
}

void Cfile_logger::NewValueL(TUint, const TTupleName& aName, const TDesC& aSubName, const MBBData* aData)
{
	const CBBSensorEvent* e=bb_cast<CBBSensorEvent>(aData);
	if (e) {
		NewSensorEventL(aName, aSubName, *e);
	} else {
		get_value(aData);
		new_value(CBBSensorEvent::ERR, _L("unknown"), *iBuf, GetTime());
	}
}

void Cfile_logger::new_value(TInt priority, const TDesC& aSource, const TDesC& aValue, const TTime& time)
{
	if (priority<priority_limit) return;
	if (priority==CBBSensorEvent::UNCHANGED_VALUE) return;

	write_time(time);
	write_to_output(aSource);
	_LIT(sep, ": ");
	write_to_output(sep);
	write_to_output(aValue);
	write_nl();
}

EXPORT_C void Cfile_logger::write_line(const TDesC& aLine)
{
	write_time(GetTime());
	write_to_output(aLine);
	write_nl();
}

void Cfile_logger::ConstructL(const TDesC& prefix, TInt aMaxLogCount)
{
	CALLSTACKITEM_N(_CL("Cfile_logger"), _CL("ConstructL"));
	Mlogger::ConstructL(AppContextAccess());

	iBuf=HBufC::NewL(512);

	TBool logging=ETrue;
	Settings().GetSettingL(SETTING_LOGGING_ENABLE, logging);
	Settings().NotifyOnChange(SETTING_LOGGING_ENABLE, this);
	enabled=logging;

	Mfile_output_base::ConstructL(prefix, logging, aMaxLogCount);
}

void Cfile_logger::write_to_output(const TDesC& str)
{
	CALLSTACKITEM_N(_CL("Cfile_logger"), _CL("write_to_output"));

	if (paused) return;

	Mfile_output_base::write_to_output(str);
}

void Cfile_logger::SettingChanged(TInt /*Setting*/)
{
	TBool logging;
	if (Settings().GetSettingL(SETTING_LOGGING_ENABLE, logging) && ! logging) {
		enabled=false;
		pause();
	} else {
		enabled=true;
		unpause();
	}
}

EXPORT_C void Cfile_logger::pause()
{
	CALLSTACKITEM_N(_CL("Cfile_logger"), _CL("pause"));
	if (paused) return;
	paused=true;

	write_time();
	_LIT(pausedtxt, "PAUSED");
	write_to_output(pausedtxt);
	write_nl();

}

EXPORT_C void Cfile_logger::unpause()
{
	CALLSTACKITEM_N(_CL("Cfile_logger"), _CL("unpause"));

	if (!paused || !enabled) return;
	//switch_file();

	write_time();
	_LIT(unpaused, "CONTINUING");
	write_to_output(unpaused);
	write_nl();
	paused=false;
}

EXPORT_C bool Cfile_logger::is_paused()
{
	CALLSTACKITEM_N(_CL("Cfile_logger"), _CL("is_paused"));

	return paused;
}

Cfile_logger::Cfile_logger(MApp_context& Context, CBBSensorEvent::TPriority limit): Mfile_output_base(Context),
	priority_limit(limit) { }

Cfile_logger::~Cfile_logger() 
{
	CALLSTACKITEM_N(_CL("Cfile_logger"), _CL("~Cfile_logger"));

	Settings().CancelNotifyOnChange(SETTING_LOGGING_ENABLE, this);

	if (!paused && enabled) {
		TInt err;
		_LIT(stopped, "STOPPED");
		CC_TRAP(err, 
			write_time();
			write_to_output(stopped);
			write_nl();
		);
	}
	delete iBuf;	
}

EXPORT_C Cfile_logger* Cfile_logger::NewL(MApp_context& Context,
				 const TDesC& prefix, CBBSensorEvent::TPriority limit,
				 TInt aMaxLogCount)
{
	CALLSTACKITEMSTATIC_N(_CL("Cfile_logger"), _CL("NewL"));

	auto_ptr<Cfile_logger> ret(new (ELeave) Cfile_logger(Context, limit));
	ret->ConstructL(prefix, aMaxLogCount);
	return ret.release();
}
