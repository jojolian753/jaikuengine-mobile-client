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

#include "call_recorder.h"
#include "file_output_base.h"
#include "cl_settings.h"

CCall_record::CCall_record(MApp_context& Context) : Ccall_listener(Context), can_record(false), recorder(0)
{
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("CCall_record"));

}

EXPORT_C CCall_record::~CCall_record()
{	
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("~CCall_record"));
	Settings().CancelNotifyOnChange(SETTING_RECORD_ALL, this);
	delete recorder;
}

void CCall_record::SettingChanged(TInt /*Setting*/)
{
	TInt all;
	if (Settings().GetSettingL(SETTING_RECORD_ALL, all) && all) record_all=true;
}

void CCall_record::ConstructL(i_status_notif* i_cb, TDirection dir)
{
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("ConstructL"));

	Ccall_listener::ConstructL(i_cb, dir);
	
	recorder=CMda_recorder::NewL(AppContext(), this, 1, 8000, 30);

	iSpecialGroups->AddGroupL(_L("Record"));
	iSpecialGroups->read_contact_groups();	

	record_all=false;
	TInt all;
	if (Settings().GetSettingL(SETTING_RECORD_ALL, all) && all) {
		if (i_cb) i_cb->status_change(_L("recording all calls"));
		record_all=true;
	}
	Settings().NotifyOnChange(SETTING_RECORD_ALL, this);
	opened();
}

EXPORT_C CCall_record* CCall_record::NewL(MApp_context& Context, i_status_notif* i_cb, TDirection dir)
{
	CALLSTACKITEMSTATIC_N(_CL("CCall_record"), _CL("NewL"));

	auto_ptr<CCall_record> ret(new (ELeave) CCall_record(Context));
	ret->ConstructL(i_cb, dir);
	return ret.release();
}

EXPORT_C void CCall_record::test()
{
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("test"));

	cb->status_change(_L("recording call"));
	TFileName filen;
	Mfile_output_base::make_filename(filen, DataDir(), _L("rec"), _L("amr"));
	recorder->record(filen);
}

void CCall_record::handle_answered()
{
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("handle_answered"));

	if (!can_record || NoSpaceLeft() || !recorder->IsIdle()) {
		cb->error(_L("Cannot record"));
		return;
	}

	if ( record_all || iSpecialGroups->is_special_contact() ) {
		cb->status_change(_L("recording call"));
		TFileName filen;
		Mfile_output_base::make_filename(filen, DataDir(), _L("rec"), _L("amr"));
		recorder->record(filen);
		can_record=false;
	}
}

void CCall_record::stopped(bool reset)
{
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("stopped"));

	can_record=true;

	if (reset) {
		cb->error(_L("record failed"));
		return;
	}
	cb->status_change(_L("recorded"));
}

void CCall_record::opened()
{
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("opened"));

	can_record=true;
}

void CCall_record::error(const TDesC& descr)
{
	CALLSTACKITEM_N(_CL("CCall_record"), _CL("error"));

	cb->error(descr);
}
