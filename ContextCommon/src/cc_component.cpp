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

#include "cc_component.h"
#include "app_context_impl.h"
#include "cc_recovery.h"
#include "break.h"

EXPORT_C void CComponentBase::StartL()
{
	User::Leave(KErrNotSupported);
}

EXPORT_C void CComponentBase::StopL()
{
	User::Leave(KErrNotSupported);
}

EXPORT_C void CComponentBase::InnerConstructL() { }

EXPORT_C CComponentBase::CComponentBase(const TDesC& aName) : CCheckedActive(CActive::EPriorityStandard, aName),
	iAppContext(GetContext()) { }

EXPORT_C CComponentBase::CComponentBase(TInt aPriority, const TDesC& aName) : CCheckedActive(aPriority, aName),
	iAppContext(GetContext()) { }


MApp_context& CComponentBase::ComponentAppContext()
{
	return *iAppContext;
}

MRecovery& CComponentBase::ComponentRecovery()
{
	return iAppContext->Recovery();
}

MErrorInfoManager& CComponentBase::ComponentErrorInfoMgr()
{
	return iAppContext->ErrorInfoMgr();
}

EXPORT_C MRecovery::TState CComponentBase::GetState()
{
	return iActiveState;
}

EXPORT_C void CComponentBase::RanSuccessfully()
{
	iWaitTime=BaselineWaitMilliseconds();
	iErrorCount=0;
}

EXPORT_C void CComponentBase::CheckedRunL()
{
	TUid uid; TInt id; TInt version;
	ComponentId(uid, id, version);
	ComponentAppContext().SetCurrentComponent(uid, id);
	switch(iActiveState) {
	case MRecovery::EStarting:
	case MRecovery::ERestarting:
		StartL();
		iActiveState=MRecovery::ERunning;
		break;
	case MRecovery::ERunning:
		ComponentRunL();
		break;
	}
	
	ReportState();
	ComponentAppContext().SetCurrentComponent(TUid::Uid(0), 0);
}

EXPORT_C void CComponentBase::ReportState()
{
	if (iActiveState!=iReportedState) {
		TUid uid; TInt id; TInt version;
		ComponentId(uid, id, version);
		ComponentRecovery().SetState(uid, id, iActiveState);
		iReportedState=iActiveState;
	}
}

EXPORT_C void CComponentBase::DoCancel()
{
	if (iActiveState==MRecovery::EStarting || iActiveState==MRecovery::ERestarting) {
		iTimer.Cancel();
	} else {
		ComponentCancel();
	}
}

EXPORT_C TInt CComponentBase::CheckedRunError(TInt aError)
{
	if (aError==KErrDiskFull || aError==KErrNoMemory) return KErrDiskFull;
	
	TUid uid; TInt id; TInt version;
	ComponentId(uid, id, version);
	iErrorCount++;
	CC_TRAPD(err, StopL());
	if (err!=KErrNone || iErrorCount>MaxErrorCount()) {
		ComponentRecovery().ReportError(uid, id, ComponentErrorInfoMgr().GetLastErrorInfo(MakeErrorCode(uid.iUid, aError)));
		iActiveState=MRecovery::EFailed;
		ReportState();
		ComponentAppContext().SetCurrentComponent(TUid::Uid(0), 0);
		return KErrNone;
	}
	
	iActiveState=MRecovery::ERestarting;
	iStatus=KRequestPending;
	Cancel();
	SetActive();
	iTimer.After(iStatus, TTimeIntervalMicroSeconds32( iWaitTime*1000 ) );
	iWaitTime *= 1.5;
	ReportState();
	
	ComponentAppContext().SetCurrentComponent(TUid::Uid(0), 0);
	return KErrNone;
}

EXPORT_C void CComponentBase::ConstructL()
{
	TUid uid; TInt id; TInt version;
	ComponentId(uid, id, version);
	ComponentAppContext().SetCurrentComponent(uid, id);
	ComponentRecovery().RegisterComponent( uid, id, version, HumanReadableFunctionality() );
	iActiveState=iReportedState=ComponentRecovery().GetState(uid, id);
	if (iActiveState==MRecovery::EDisabled) return;
	
	iWaitTime=BaselineWaitMilliseconds();
	CActiveScheduler::Add(this);
	InnerConstructL();
	iActiveState=MRecovery::EStarting;
	User::LeaveIfError(iTimer.CreateLocal());
	TRequestStatus *s=&iStatus;
	iStatus=KRequestPending;
	SetActive();
	User::RequestComplete(s, KErrNone);
	
	ComponentAppContext().SetCurrentComponent(TUid::Uid(0), 0);
}

EXPORT_C TInt CComponentBase::BaselineWaitMilliseconds()
{
	return 500;
}

EXPORT_C TInt CComponentBase::MaxErrorCount()
{
	return 3;
}

EXPORT_C CComponentBase::~CComponentBase()
{
	iTimer.Close();
}
