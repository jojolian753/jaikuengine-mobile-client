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

#include "log_appuse.h"
#include "symbian_auto_ptr.h"
#include "apgwgnam.h"
#include <e32std.h>
#include "foreground.h"

class CLog_AppUseImpl : public CLog_AppUse, public MForegroundObserver {
private:
	~CLog_AppUseImpl();

	const CBBSensorEvent& get_value();

	CLog_AppUseImpl(MApp_context& Context);
	void ConstructL();
	
	TBBCurrentApp iValue;
	TTime	iPrevTime;
	RWsSession iWsSession;
	CForeground *iFg;

	const CBBSensorEvent& get_value(CApaWindowGroupName* gn);
	void ForegroundChanged(CApaWindowGroupName* gn);
	friend class CLog_AppUse;
	friend class auto_ptr<CLog_AppUseImpl>;
};

CLog_AppUseImpl::~CLog_AppUseImpl()
{
	CALLSTACKITEM_N(_CL("CLog_AppUseImpl"), _CL("~CLog_AppUseImpl"));
	delete iFg;
	iWsSession.Close();
}

const CBBSensorEvent& CLog_AppUseImpl::get_value()
{
	CALLSTACKITEM_N(_CL("CLog_AppUseImpl"), _CL("get_value"));

	TInt wgid=iWsSession.GetFocusWindowGroup();
	auto_ptr<CApaWindowGroupName> gn(CApaWindowGroupName::NewL(iWsSession, wgid));
	
	return get_value(gn.get());
}

const CBBSensorEvent& CLog_AppUseImpl::get_value(CApaWindowGroupName* gn) {
	iValue.iUid()=gn->AppUid().iUid;
	iValue.iCaption()=gn->Caption().Left(iValue.iCaption().MaxLength());
	iEvent.iData.SetOwnsValue(EFalse);
	iEvent.iData.SetValue(&iValue); iEvent.iStamp=GetTime();
	return iEvent;
}

void CLog_AppUseImpl::ForegroundChanged(CApaWindowGroupName* gn)
{
	CALLSTACKITEM_N(_CL("CLog_AppUseImpl"), _CL("ForegroundChanged"));

	TUint prev=iValue.iUid();
	get_value(gn);
	if (prev!=iValue.iUid()) {
		post_new_value(&iValue);
		iPrevTime.HomeTime();
	} else {
		TTime now; now.HomeTime();
		if (now > (iPrevTime+TTimeIntervalMinutes(10))) {
			post_unchanged_value(&iValue);
			iPrevTime.HomeTime();
		}
	}
}

EXPORT_C CLog_AppUse* CLog_AppUse::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CLog_AppUseImpl"), _CL("NewL"),  &Context);

	auto_ptr<CLog_AppUseImpl> ret(new (ELeave) CLog_AppUseImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CLog_AppUse::CLog_AppUse(MApp_context& Context) : Mlog_base_impl(Context, KCurrentApp, KCurrentAppTuple, 15*60) { }

CLog_AppUseImpl::CLog_AppUseImpl(MApp_context& Context) : CLog_AppUse(Context) { }

void CLog_AppUseImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLog_AppUseImpl"), _CL("ConstructL"));

	Mlog_base_impl::ConstructL();
	User::LeaveIfError(iWsSession.Connect());
	iFg=CForeground::NewL(iWsSession);
	iFg->AddObserver(this);

	post_new_value(get_value());
}
