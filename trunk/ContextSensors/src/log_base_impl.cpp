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

#include "log_base_impl.h"

#include <e32std.h>
#include <eikenv.h>

#include "bbdata.h"

EXPORT_C void Mlog_base_impl::ConstructL()
{
	CALLSTACKITEM_N(_CL("Mlog_base_impl"), _CL("ConstructL"));

	iBBSubSession=BBSession()->CreateSubSessionL(0);
}

EXPORT_C Mlog_base_impl::Mlog_base_impl(MApp_context& Context, const TDesC& aName, const TTupleName& aTupleName,
					TInt aLeaseTimeInSeconds) : MContextBase(Context),
	iEvent(aName, aTupleName, 0), iLeaseTimeInSeconds(aLeaseTimeInSeconds)
{
	iEvent.iData.SetOwnsValue(EFalse);
}

EXPORT_C Mlog_base_impl::~Mlog_base_impl()
{
	CALLSTACKITEM_N(_CL("Mlog_base_impl"), _CL("~Mlog_base_impl"));

	delete iBBSubSession;
}

EXPORT_C void Mlog_base_impl::add_sinkL(Mlogger* sink)
{
	CALLSTACKITEM_N(_CL("Mlog_base_impl"), _CL("add_sinkL"));

	if (!sink) return;

	sink->iBBSubSessionNotif->AddNotificationL(iEvent.TupleName());
}

EXPORT_C TTime Mlog_base_impl::GetLeaseExpires()
{
	TTime t; 
	if (iLeaseTimeInSeconds>=0) {
		t=GetTime();
		t+=TTimeIntervalSeconds(iLeaseTimeInSeconds);
	} else {
		t=Time::MaxTTime();
	}
	return t;
}

EXPORT_C void Mlog_base_impl::post_new_value(const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("Mlog_base_impl"), _CL("post_new_value"));

	TTime expires=GetLeaseExpires();
	iBBSubSession->PutL(aEvent.TupleName(), KNullDesC, &aEvent, expires);
}


EXPORT_C void Mlog_base_impl::post_new_value(CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("Mlog_base_impl"), _CL("post_new_value"));
	if (aEvent.iStamp()==TTime(0)) aEvent.iStamp()=GetTime();

	TTime expires=GetLeaseExpires();
	iBBSubSession->PutL(aEvent.TupleName(), KNullDesC, &aEvent, expires);
}

EXPORT_C void Mlog_base_impl::post_error(const TDesC& aMsg, TInt aCode, const TTime& time)
{
	// FIXME
	TBuf<10> name;
	if (aCode==KErrNone) {
		iEvent.iPriority()=CBBSensorEvent::INFO;
		name=_L("info");
	} else {
		name=_L("error");
		iEvent.iPriority()=CBBSensorEvent::ERR;
	}

	TBBLongString s(aMsg, name);
	iEvent.iData.SetValue(&s); iEvent.iData.SetOwnsValue(EFalse);
	iEvent.iStamp()=time;
	post_new_value(iEvent);
	iEvent.iData.SetValue(0);
	iEvent.iPriority()=CBBSensorEvent::VALUE;
}

EXPORT_C void Mlog_base_impl::post_error(const TDesC& aMsg, TInt aCode)
{
	post_error(aMsg, aCode, GetTime());
}

EXPORT_C const CBBSensorEvent& Mlog_base_impl::get_value()
{
	return iEvent;
}

EXPORT_C void Mlog_base_impl::post_new_value(MBBData* aData)
{
	post_new_value(aData, GetTime());
}

EXPORT_C void Mlog_base_impl::post_new_value(MBBData* aData, const TTime& time)
{
	iEvent.iData.SetValue(aData);
	iEvent.iStamp()=time;
	iEvent.iPriority()=CBBSensorEvent::VALUE;
	post_new_value(iEvent);
}

EXPORT_C void Mlog_base_impl::post_unchanged_value(MBBData* aData)
{
	iEvent.iData.SetValue(aData);
	iEvent.iStamp()=GetTime();
	iEvent.iPriority()=CBBSensorEvent::UNCHANGED_VALUE;
	post_new_value(iEvent);
}

EXPORT_C Clog_base_impl::Clog_base_impl(MApp_context& Context, const TDesC& aName, 
			       const TTupleName& aTupleName, TInt aLeaseTime) : 
	Mlog_base_impl(Context, aName, aTupleName, aLeaseTime) { }

EXPORT_C Clog_base_impl* Clog_base_impl::NewL(MApp_context& Context, const TDesC& aName, 
					      const TTupleName& aTupleName, TInt aLeaseTime)
{
	CALLSTACKITEM2_N(_CL("Clog_base_impl"), _CL("NewL"),  &Context);

	auto_ptr<Clog_base_impl> ret(new (ELeave) Clog_base_impl(Context, aName, aTupleName, aLeaseTime));
	ret->ConstructL();
	return ret.release();
}
