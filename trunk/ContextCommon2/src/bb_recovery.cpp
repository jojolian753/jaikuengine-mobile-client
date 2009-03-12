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

#include "bb_recovery.h"
#include "cbbsession.h"
#include "concretedata.h"
#include "bbutil.h"
#include "bberrorinfo.h"

_LIT(KVersion, "version");
_LIT(KFunctionality, "functionality");
_LIT(KState, "state");
_LIT(KErrorCount, "errorcount");

class CBBRecoveryImpl : public CBBRecovery {
public:
	TBuf<20> iSubName;
	void SetSubName(TUid aComponentUid, TInt aComponentId) {
		iSubName.Zero();
		iSubName.AppendNum(aComponentUid.iUid, EHex);
		iSubName.Append(_L(":"));
		iSubName.AppendNum(aComponentId);
	}
	TInt GetVersion() {
		MBBData* d=0;
		BBSession()->GetL(KComponentVersionTuple, iSubName, d, ETrue);
		bb_auto_ptr<MBBData> dp(d);
		TBBInt* v=bb_cast<TBBInt>(d);
		if (!v) return -1;
		return (*v)();
	}
	virtual void RegisterComponent( TUid aComponentUid, TInt aComponentId, 
		TInt aVersion, const TDesC& aHumanReadableFunctionality) { 
		
		SetSubName(aComponentUid, aComponentId);
		TInt version=GetVersion();
		if (version==aVersion) return;
		TTime expires=GetTime(); expires+=TTimeIntervalYears(2);
		
		{
			bb_auto_ptr<CBBString> f(CBBString::NewL(KFunctionality, aHumanReadableFunctionality.Length()));
			f->Append(aHumanReadableFunctionality);
			BBSession()->PutL(KComponentFunctionalityTuple, iSubName, f.get(), expires);
		}
		{
			TBBInt e(0, KErrorCount);
			BBSession()->PutL(KComponentErrorCountTuple, iSubName, &e, expires);
		}
		{
			TBBInt s(MRecovery::EUnknown, KState);
			BBSession()->PutL(KComponentStateTuple, iSubName, &s, expires);
		}
		{
			TBBInt v(aVersion, KVersion);
			BBSession()->PutL(KComponentVersionTuple, iSubName, &v, expires);
		}
	}
	virtual void SetState(TUid aComponentUid, TInt aComponentId, TState aState) { 
		SetSubName(aComponentUid, aComponentId);
		TBBInt s(aState, KState);
		TTime expires=GetTime(); expires+=TTimeIntervalYears(2);
		BBSession()->PutL(KComponentStateTuple, iSubName, &s, expires);
		if (aState==EFailed) {
			TTime expires=GetTime(); expires+=TTimeIntervalYears(2);
			TBBInt e(0, KErrorCount);
			e()=GetErrorCount()+1;
			BBSession()->PutL(KComponentErrorCountTuple, iSubName, &e, expires);
		}
	}
	virtual TState GetState(TUid aComponentUid, TInt aComponentId) { 
		SetSubName(aComponentUid, aComponentId);
		MBBData* d=0;
		BBSession()->GetL(KComponentStateTuple, iSubName, d, ETrue);
		bb_auto_ptr<MBBData> dp(d);
		TBBInt* s=bb_cast<TBBInt>(d);
		if (!s) return EUnknown;
		return (TState)(*s)();
	}
	TInt GetErrorCount() {
		MBBData* d=0;
		BBSession()->GetL(KComponentErrorCountTuple, iSubName, d, ETrue);
		bb_auto_ptr<MBBData> dp(d);
		TBBInt* e=bb_cast<TBBInt>(d);
		if (!e) return 0;
		return (*e)();
	}
	virtual TInt GetErrorCount(TUid aComponentUid, TInt aComponentId) { 
		SetSubName(aComponentUid, aComponentId);
		return GetErrorCount();
	}
	virtual void ResetErrorCount(TUid aComponentUid, TInt aComponentId) { 
		SetSubName(aComponentUid, aComponentId);
		TBBInt e(0, KErrorCount);
		TTime expires=GetTime(); expires+=TTimeIntervalYears(2);
		BBSession()->PutL(KComponentErrorCountTuple, iSubName, &e, expires);
	}
	virtual void ReportError(TUid aComponentUid, TInt aComponentId,
		const class MErrorInfo* aErrorInfo) {
		
		SetSubName(aComponentUid, aComponentId);
		auto_ptr<CBBErrorInfo> ei(CBBErrorInfo::NewL( BBDataFactory(), aErrorInfo));
		TTime expires=GetTime(); expires+=TTimeIntervalYears(2);
		BBSession()->PutL(KComponentErrorInfoTuple, iSubName, ei.get(), expires);
	}

};

EXPORT_C CBBRecovery* CBBRecovery::NewL()
{
	auto_ptr<CBBRecoveryImpl> ret(new (ELeave) CBBRecoveryImpl);
	return ret.release();
}
