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

#include "log_shareddata.h"

#include "symbian_auto_ptr.h"
#include "checkedactive.h"

#include "cc_shareddata.h"

#define MAX_ERRORS	10

#include "csd_system.h"

class CLog_SharedDataImpl : public CLog_SharedData, public MSharedDataNotifyHandler, public CBase {
public:
	virtual ~CLog_SharedDataImpl();
private:
	CLog_SharedDataImpl(MApp_context& Context, const TDesC& name, const TTupleName& aTupleName);
	void ConstructL(TUid Uid, const TDesC& aKeyName);

	virtual void HandleNotifyL( const TUid aUid, const TDesC& aKey,
                                const TDesC& aValue );

	RSharedDataClient iAgent;
	TInt		iErrorCount;

	TUid		iUid;
	TBuf<50>	iKeyName;

	TBBLongString	iValue;

	friend class CLog_SharedData;
};

EXPORT_C CLog_SharedData::~CLog_SharedData() { }

EXPORT_C CLog_SharedData* CLog_SharedData::NewL(MApp_context& Context, const TDesC& name, TUid Uid, 
				       const TDesC& aKeyName, const TTupleName& aTupleName)
{
	auto_ptr<CLog_SharedDataImpl> ret(new (ELeave) CLog_SharedDataImpl(Context, name, aTupleName));
	ret->ConstructL(Uid, aKeyName);
	return ret.release();
}

CLog_SharedData::CLog_SharedData(MApp_context& Context, const TDesC& name, const TTupleName& aTupleName) : 
	Mlog_base_impl(Context, name, aTupleName, 5*24*60*60)
{
}

CLog_SharedDataImpl::~CLog_SharedDataImpl()
{
	iAgent.CancelNotify(iUid, &iKeyName);
	iAgent.Close();
}

CLog_SharedDataImpl::CLog_SharedDataImpl(MApp_context& Context, const TDesC& name, const TTupleName& aTupleName) : 
	CLog_SharedData(Context, name, aTupleName), 
	iAgent(this), 
	iValue(name)
{
}

void CLog_SharedDataImpl::ConstructL(TUid Uid, const TDesC& aKeyName)
{
	Mlog_base_impl::ConstructL();
	iUid=Uid;
	iKeyName=aKeyName;
	User::LeaveIfError(iAgent.Connect());
	User::LeaveIfError(iAgent.Assign(iUid));

	iAgent.GetString(iKeyName, iValue());
	iEvent.iStamp()=GetTime();
	iEvent.iData.SetValue(&iValue); iEvent.iData.SetOwnsValue(EFalse);

	User::LeaveIfError(iAgent.NotifyChange(iUid));
	post_new_value(get_value());
}

void CLog_SharedDataImpl::HandleNotifyL( const TUid /*aUid*/, const TDesC& aKey,
                        const TDesC& aValue )
{
	iValue()=aKey;
	iValue().Append(_L(" "));
	iValue().Append(aValue);
	post_new_value(&iValue);
}
