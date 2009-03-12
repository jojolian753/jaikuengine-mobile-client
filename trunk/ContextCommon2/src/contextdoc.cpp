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

#include <e32std.h>
#include "contextdoc.h"
#include "app_context_impl.h"
#include "bb_settings.h"
#include <aknapp.h>
#include "cc_schedulers.h"
#include "cbbsession.h"
#include "reporting.h"
#include "callstack.h"
#include "symbian_auto_ptr.h"
#include "break.h"
#include <eikenv.h>
#include "context_uids.h"
#include "cn_datacounter_impl.h"
#include "bb_recovery.h"

#if defined(__S60V3__) && defined(__WINS__)
#include "allocator.h"
#endif

EXPORT_C TInt MContextDocument::InitializationSteps()
{
	return 5;
}

EXPORT_C void MContextDocument::StepDone()
{
}

#include "connectioninit.h"

EXPORT_C void MContextDocument::ConstructL(class CApaApplication* aApp, 
	const class MDefaultSettings& aDefaultSettings,
	const struct TTupleName& aSettingTupleName,
	const TDesC& aDebugLog,
	const TDesC& aChunkName)
{
#if defined(__S60V3__) && defined(__WINS__)
	//iOriginal=SwitchToLoggingAllocator();
#endif
	iContext=CApp_context::NewL(false, aChunkName);

	StepDone();
#ifndef __WINS__
	{ CC_TRAPD(errs, StartStarterL(aChunkName, aApp->AppDllUid(), true)); }
#else
	if (0 && aApp->AppDllUid() == KUidcontext_log) {
		// can't pass arguments on the emulator, so only works for context_log
		{ CC_TRAPD(errs, StartStarterL(aChunkName, aApp->AppDllUid(), true, CEikonEnv::Static()->WsSession())); }
	}
#endif
	StepDone();

#ifndef __S60V3__
	iContext->Reporting().SetState(_L("Set data dir"));
#  ifndef __S60V2FP3__
	iContext->SetDataDir(_L("c:\\system\\data\\context\\"), false);
#  else
	iContext->SetDataDir(_L("c:\\system\\data\\context\\"), ETrue);
#  endif
#endif
	iContext->Reporting().SetState(_L("Set debug log"));
	iContext->SetDebugLog(_L("context"), aDebugLog);

	CC_TRAPD(err, InnerConstructL(aApp, aDefaultSettings, aSettingTupleName));
	if (err!=KErrNone) {
		iContext->Reporting().UserErrorLog(_L("Failed to construct application"), err);
		auto_ptr<HBufC> s(iContext->CallStackMgr().GetFormattedCallStack(KNullDesC));
		if (s.get()) iContext->Reporting().UserErrorLog(*s);
		User::Leave(err);
	}
}

void MContextDocument::InnerConstructL(class CApaApplication* aApp, 
	const class MDefaultSettings& aDefaultSettings,
	const struct TTupleName& aSettingTupleName)
{

	iContext->Reporting().SetState(_L("Create scheduler"));
	iDebugScheduler=new (ELeave) CDebugAppScheduler(*iContext, CCoeEnv::Static());
	iBuiltinScheduler=CActiveScheduler::Replace(iDebugScheduler);
	StepDone();

#ifndef __WINS__
	{
		iContext->Reporting().SetState(_L("set app dir"));
		TParsePtrC parse(aApp->DllName());
		iContext->SetAppDir(parse.DriveAndPath());
	}
#endif
	iContext->Reporting().SetState(_L("Create BB settings"));

	auto_ptr<CApp_context> ctx(CApp_context::NewL(true, _L("BlackBoardServer")));
	TInt err=0;
	CBlackBoardSettings* s=0;
	CC_TRAP(err, s=CBlackBoardSettings::NewL(*iContext, aDefaultSettings, aSettingTupleName));
	if (err==KErrServerTerminated) {
		auto_ptr<HBufC> stack(0);
		
		CC_TRAPD(err, stack.reset(ctx->CallStackMgr().GetFormattedCallStack(_L("BlackboardCrash"))));
		if (err==KErrNone && stack.get()!=0) {
			iContext->Reporting().UserErrorLog(*stack);
		}
		User::Leave(KErrServerTerminated);
	} else if (err!=KErrNone) {
		User::Leave(err);
	}

	iContext->SetSettings(s);
	StepDone();

	iContext->Reporting().SetState(_L("Create BB factory"));
	iBBDataFactory=CBBDataFactory::NewL();
	iContext->TakeOwnershipL(iBBDataFactory);
	iBBSession=CBBSession::NewL(*iContext, iBBDataFactory);
	iContext->TakeOwnershipL(iBBSession);
	iContext->SetBBSession(iBBSession);
	iContext->SetBBDataFactory(iBBDataFactory);
	iRecovery=CBBRecovery::NewL();
	iContext->TakeOwnershipL(iRecovery);
	iContext->SetRecovery(iRecovery);

	iCounter=CDataCounter::NewL();
	iContext->TakeOwnershipL(iCounter);
	iContext->SetDataCounter(iCounter);
	CConnectionOpener::CheckBootFileAndResetPermissionL(iContext->Fs(), iContext->DataDir()[0], *s);
	StepDone();
}


EXPORT_C MContextDocument::~MContextDocument()
{
	CC_TRAPD(err, ReleaseMContextDocument());
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
#if defined(__S60V3__) && defined(__WINS__)
	if (iOriginal) SwitchBackAllocator(iOriginal);
#endif
}

void MContextDocument::ReleaseMContextDocument()
{
#ifdef __WINS__
	User::Check();
#endif

	delete iContext;

	if (iBuiltinScheduler) {
#ifdef __WINS__
		iDebugScheduler->CheckThatEmpty();
#endif
		CActiveScheduler::Replace(iBuiltinScheduler);
	}
	delete iDebugScheduler;
#ifdef __WINS__
	User::Check();
#endif
}
