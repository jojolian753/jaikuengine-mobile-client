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

#include "cc_imei.h"
#ifndef __S60V3__
#include <plpvariant.h>
#else
#include <Etel3rdParty.h>

#include "symbian_auto_ptr.h"
#include "context_uids.h"
#include "app_context.h"
#include "errorhandling.h"
#endif




EXPORT_C void GetImeiL(TDes& aInto)
{
#ifndef __S60V3__
	TPlpVariantMachineId machineId;
	PlpVariant::GetMachineIdL(machineId);
	aInto=machineId;
#else
	aInto.Zero();
	User::Leave( KErrNotSupported );
#endif
}

#ifdef __S60V3__
_LIT(KComponentName, "CImeiImpl");
_LIT(KComponentFunctionality, "Device IMEI");

CImei::CImei() : CComponentBase(KComponentName) {}

class CImeiImpl : public CImei, public MContextBase 
{
public:
	CImeiImpl(TUid aComponentUid, TInt aComponentId) 
		: CImei(), 
		  iState(EInit), 
		  iComponentUid(aComponentUid), 
		  iComponentId(aComponentId) {}
	
	~CImeiImpl()
	{
		CALLSTACKITEM_N(_CL("CImeiImpl"), _CL("~CImeiImpl"));
		Cancel();
		StopL();
	}
	
	TPtrC GetL() 
	{
		CALLSTACKITEM_N(_CL("CImeiImpl"), _CL("GetL"));
		MRecovery::TState s = GetState();
		if ( s == MRecovery::EFailed )
			EnvErr( _L("IMEI fetching component has failed") ).Raise();
		else if ( s == MRecovery::EDisabled )  
			EnvErr( _L("IMEI fetching component has been disabled") ).Raise();
		
		
		if ( iState == EInit )
			{
				StartL();
			}
		
		if ( iState != EReady )
			{
				// force synchronous usage
				// FIXME!
				//iActiveSchedulerWait->Start();
			}
		return iIMEI;
	}
   

public: //	
	virtual void StartL()
	{
		CALLSTACKITEM_N(_CL("CImeiImpl"), _CL("StartL"));
		if ( ! iActiveSchedulerWait )
			iActiveSchedulerWait = new (ELeave) CActiveSchedulerWait;
		
		Cancel();
		iState = EReading;
		CTelephony::TPhoneIdV1Pckg phoneIdPckg( iPhoneId );
		Telephony().GetPhoneId(iStatus, phoneIdPckg);
		SetActive(); // Tell scheduler a request is active
	}

	virtual void StopL()
	{
		CALLSTACKITEM_N(_CL("CImeiImpl"), _CL("StopL"));
		delete iActiveSchedulerWait;
		iActiveSchedulerWait = NULL;
		iState = EInit;
	}
	
	virtual void ComponentRunL()
	{
		CALLSTACKITEM_N(_CL("CImeiImpl"), _CL("ComponentRunL"));
		if ( iStatus == KErrNone )
			{
				iIMEI.Zero();
				iIMEI.Append(iPhoneId.iSerialNumber);
				iState = EReady;
				if ( iActiveSchedulerWait->IsStarted() )
					iActiveSchedulerWait->AsyncStop();
			}
		else
			{
#ifndef __WINS__
		        User::Leave( iStatus.Int() );
#else
				iIMEI.Zero();
				iIMEI.Append(_L("00000000000000"));
				iState = EReady;
				if ( iActiveSchedulerWait->IsStarted() )
					iActiveSchedulerWait->AsyncStop();		        
#endif
			}
	}
	
	virtual void ComponentCancel()
	{
		CALLSTACKITEM_N(_CL("CImeiImpl"), _CL("ComponentCancel"));
		Telephony().CancelAsync(CTelephony::EGetPhoneIdCancel);
		iState = EInit;
		iIMEI.Zero();
	}
	
	virtual void ComponentId(TUid& aComponentUid, 
							 TInt& aComponentId,
							 TInt& aVersion)
	{
		aComponentUid=iComponentUid;
		aComponentId= iComponentId; 
		aVersion=1;
	}
	
	virtual const TDesC& Name() { return KComponentName; }
	virtual const TDesC& HumanReadableFunctionality() { return KComponentFunctionality; }
	
private:
	CTelephony::TPhoneIdV1 iPhoneId;
	TBuf<CTelephony::KPhoneSerialNumberSize> iIMEI;
	
	enum TState
		{
			EInit,
			EReading,
			EReady
		} iState;

	CActiveSchedulerWait* iActiveSchedulerWait;

	// It's callers responsibility to provide component uid and component id
	TUid iComponentUid;
	TInt iComponentId;
};
#endif // __S60V3__


EXPORT_C CImei* CImei::NewL(TUid aComponentUid, 
							TInt aComponentId)
{
	CALLSTACKITEMSTATIC_N(_CL("CImei"), _CL("NewL"));
#ifdef __S60V3__
	auto_ptr<CImeiImpl> self( new (ELeave) CImeiImpl(aComponentUid, aComponentId) );
	self->CComponentBase::ConstructL();
	return self.release();
#else
	User::Leave( KErrNotSupported );
	return NULL;
#endif // __S60V3__
}
