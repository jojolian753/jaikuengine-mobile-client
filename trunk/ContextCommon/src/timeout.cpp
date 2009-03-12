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

#include "timeout.h"
#include "symbian_auto_ptr.h"
#include "app_context.h"
#include "app_context_impl.h"
#include "callstack.h"

class CTimeOutImpl : public CTimeOut {
public:
	~CTimeOutImpl();
private:
	virtual void Wait(int seconds);
	virtual void WaitMax(int seconds); 
	virtual void Reset();
	virtual void WaitShort(int milliseconds);
	virtual const TTime& WaitingUntil() const;

	CTimeOutImpl(MTimeOut& i_cb, CActive::TPriority aPriority);
	void ConstructL();
	void RunL();

	MTimeOut& cb;
	TTime	iWaitUntil;
	MApp_context* iContext;

	friend class CTimeOut;
};

CTimeOut::CTimeOut(CActive::TPriority aPriority) : CTimer(aPriority)
{
}

CTimeOut::~CTimeOut()
{
}

CTimeOutImpl::CTimeOutImpl(MTimeOut& i_cb, CActive::TPriority aPriority) : CTimeOut(aPriority), cb(i_cb)
{
}

void CTimeOutImpl::ConstructL()
{
	iContext=GetContext();

	CTimer::ConstructL();
	CActiveScheduler::Add(this);
}

EXPORT_C CTimeOut* CTimeOut::NewL(MTimeOut& i_cb, CActive::TPriority aPriority)
{
	CALLSTACKITEMSTATIC_N(_CL("CTimeOut"), _CL("NewL"));

	auto_ptr<CTimeOutImpl> ret (new (ELeave) CTimeOutImpl(i_cb, aPriority));
	ret->ConstructL();
	return ret.release();
}

void CTimeOutImpl::RunL()
{
	// there's no guarantee that we aren't deleted
	// from the expired() callback, so we have to cache
	// the member variable iContext in a stack variable

	MApp_context* ctx=iContext;
	if (ctx) ctx->CallStackMgr().ResetCallStack();

	TTime now; now.HomeTime(); 
	
	now+=TTimeIntervalMicroSeconds(100*1000);
	if (now < iWaitUntil) {
		At(iWaitUntil);
	} else {
		iWaitUntil=TTime(0);
		cb.expired(this);
	}

	if (ctx) ctx->CallStackMgr().ResetCallStack();
}

const TTime& CTimeOutImpl::WaitingUntil() const
{
	return iWaitUntil;
}

void CTimeOutImpl::WaitShort(int milliseconds)
{
	CALLSTACKITEM2_N(_CL("CTimeOutImpl"), _CL("WaitShort"), iContext);
	Reset();
	TTime at;
	at.HomeTime();
	
	if (milliseconds<0) milliseconds=0;
	at+=TTimeIntervalMicroSeconds(milliseconds*1000);
	iWaitUntil=at;
	At(at);
}

void CTimeOutImpl::Wait(int seconds)
{
	CALLSTACKITEM2_N(_CL("CTimeOutImpl"), _CL("Wait"), iContext);
	Reset();
	TTime at;
	at.HomeTime();
	
	if (seconds<0) seconds=0;
	at+=TTimeIntervalSeconds(seconds);
	iWaitUntil=at;
	At(at);
}

void CTimeOutImpl::WaitMax(int seconds)
{
	TTime t; t.HomeTime();
	if (seconds<0) seconds=0;
	t+=TTimeIntervalSeconds(seconds);
	if (IsActive() && t >= iWaitUntil) return;
	Reset();
	iWaitUntil=t;
	At(t);
}

void CTimeOutImpl::Reset()
{
	CALLSTACKITEM2_N(_CL("CTimeOutImpl"), _CL("Reset"),  iContext);

	iWaitUntil=TTime(0);
	Cancel();
}

CTimeOutImpl::~CTimeOutImpl() {
	iContext=0;
	Cancel();
}

EXPORT_C void SetTime()
{
	TTime now;
	now.HomeTime();
	now+=TTimeIntervalHours(3);
	User::SetHomeTime(now);
}
