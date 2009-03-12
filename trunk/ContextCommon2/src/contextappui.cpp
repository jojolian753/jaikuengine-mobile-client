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

#include "contextappui.h"

#include "list.h"
#include "file_logger.h"
#include "app_context_impl.h"
#include "cl_settings.h"
#include "reporting.h"
#include "break.h"
#include "callstack.h"

#include "contextvariant.hrh"

#include <aknglobalnote.h> 
#include <apgtask.h>
#include <basched.h>
#include <eikenv.h>
#include <errorui.h> 

EXPORT_C void MContextAppUi::ConstructL(MApp_context& aContext, 
										const TDesC& aLogFileName, 
										TInt aMaxLogCount)
{
#ifdef __WINS__
	User::Check();
#endif
	iWait=CTimeOut::NewL(*this);
	iWaitingOps=CList<TCallBack>::NewL();
	StepDone();

	iAppContext=&aContext;
	iLog=Cfile_logger::NewL(aContext, aLogFileName, 
		CBBSensorEvent::INFO, aMaxLogCount);
	aContext.SetFileLog(iLog);
#ifndef __DEV__
	iLog->pause();
#endif
	LogFormatted(_L("starting"));
	StepDone();
#ifdef __WINS__
	User::Check();
#endif
}

EXPORT_C TInt MContextAppUi::InitializationSteps()
{
	return 2;
}

EXPORT_C void MContextAppUi::StepDone()
{
}

EXPORT_C void MContextAppUi::ReleaseContextAppUi()
{
#ifdef __WINS__
	User::Check();
#endif
	delete iWait; iWait=0;
	delete iWaitingOps; iWaitingOps=0;
	delete iLog; iLog=0;
	if (GetContext()) GetContext()->SetFileLog(0);
#ifdef __WINS__
	User::Check();
#endif
}
EXPORT_C MContextAppUi::~MContextAppUi()
{
	CC_TRAPD(err, ReleaseContextAppUi());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

EXPORT_C void MContextAppUi::QueueOp(TCallBack Op, int AfterSeconds)
{
	CALLSTACKITEM_N(_CL("MContextAppUi"), _CL("QueueOp"));

	iWaitingOps->AppendL(Op);
	iWait->Wait(AfterSeconds);
}

EXPORT_C void MContextAppUi::RunOp(TCallBack op)
{
	((*this).*op)();
}

EXPORT_C void MContextAppUi::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("expired"));

	TCallBack op=iWaitingOps->Pop();
	RunOp(op);
	while (iWaitingOps->iFirst && iWaitingOps->Top()==op) iWaitingOps->Pop();
	if (iWaitingOps->iFirst) {
		iWait->Wait(1);
	}
}

EXPORT_C void MContextAppUi::hide()
{
	CALLSTACKITEM_N(_CL("MContextAppUi"), _CL("hide"));

	RWsSession& wsSession=CEikonEnv::Static()->WsSession();
	
	TApaTask task(wsSession);
	TInt id = CEikonEnv::Static()->RootWin().Identifier();
	task.SetWgId(id);
		
	if ( wsSession.GetFocusWindowGroup() == id )
	{
		task.SendToBackground();
	}
}

EXPORT_C void MContextAppUi::unhide()
{
	CALLSTACKITEM_N(_CL("MContextAppUi"), _CL("unhide"));

	RWsSession& wsSession=CEikonEnv::Static()->WsSession();
	TApaTask task(wsSession);
	task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
	task.BringToForeground();
}

#include "bberrorinfo.h"

EXPORT_C void MContextAppUi::ReportError(const TDesC& Source,
	const TDesC& Reason, TInt Code)
{
	iLog->unpause();
	TRAPD(ignore, {
		const MErrorInfo* ei=GetContext()->ErrorInfoMgr().GetLastErrorInfo(MakeErrorCode(0, Code));
		if (ei) {
			refcounted_ptr<CBBErrorInfo> e(CBBErrorInfo::NewL( GetContext()->BBDataFactory(), ei) );
			TInt err=KErrNone;
			auto_ptr<HBufC> buf(HBufC::NewL(1024));
			for(;;) {
				buf->Des().Zero();
				TPtr p=buf->Des();
				TRAP(err, e->IntoStringL( p ) );
				if (err==KErrOverflow) {
					buf.reset( buf->ReAllocL( buf->Des().MaxLength() * 2) );
				} else {
					break;
				}
			}
			iLog->new_value(CBBSensorEvent::ERR, _L("APP"), *buf, GetTime());
		}
			
	});
	GetContext()->ErrorInfoMgr().ResetLastErrorInfo();
}

EXPORT_C void MContextAppUi::LogFormatted(const TDesC& aMsg)
{
	if (iLog) {
		CC_TRAPD(err, iLog->new_value(CBBSensorEvent::ERR, _L("APP"), aMsg, GetTime()));
	}
}

static void ShowErrorNoteL(TInt aError)
{
	
	{
		auto_ptr<CErrorUI> eui(CErrorUI::NewL());
		auto_ptr<CAknGlobalNote> note(CAknGlobalNote::NewL());
		const TDesC& msg=eui->TextResolver().ResolveErrorString( aError );
		if (msg.Length()>0) {
			note->ShowNoteL(EAknGlobalErrorNote, msg);
		} else {
			TBuf<30> msg=_L("Error: ");
			msg.AppendNum( aError );
			note->ShowNoteL(EAknGlobalErrorNote, msg);
			}
	}
}

EXPORT_C void MContextAppUi::WrapInnerConstructL()
{
	CC_TRAPD(err, InnerConstructL());
	if (err!=KErrNone) {
		ReportError(_L("AppUi::ConstructL"), _L("Leave"), err);
		ShowErrorNoteL( err );
		User::Leave(err);
	}
}

#include <akndoc.h>
#include <aknappui.h>
#include <aknapp.h>

EXPORT_C TErrorHandlerResponse MContextAppUi::HandleError(TInt aError,
     const SExtendedError& aExtErr,
     TDes& aErrorText,
     TDes& aContextText)
{
	// TODO: handle aExtErr
	TInt err;
	if (iLog) {
		iLog->unpause();
		TBuf<128> msg;
		msg.Format(_L("Unhandled error %d %x %d %S %S"), aError,
			aExtErr.iComponent.iUid, aExtErr.iErrorNumber
			, &aContextText, &aErrorText);
		TTime t=GetTime();
		CC_TRAP(err, iLog->new_value(CBBSensorEvent::ERR, _L("app_event"), msg, t));
#ifdef __WINS__
		RDebug::Print(msg);
#endif

		HBufC* stack=0;
		if (iAppContext) { 
			stack=iAppContext->CallStackMgr().GetFormattedCallStack(_L("AppUi"));
		}
		if (stack) {
			CC_TRAP(err, iLog->new_value(CBBSensorEvent::ERR, _L("app_event"), *stack, t));
		}
		delete stack;
		if (iAppContext && iHandlableError) {
			iAppContext->CallStackMgr().ResetCallStack();
		}
	}

	if (!iHandlableError) {	
		if ( aError==KErrNoMemory || aError==KErrDiskFull) {
			// FIXME: why did we do this?
			//return EErrorNotHandled;
		}
		
		TInt exit_with;
		exit_with=iExitValue=aError;
		CAknDocument* doc=(CAknDocument*)((CAknAppUi*)CEikonEnv::Static()->AppUi())->Document();
		CAknApplication* app=(CAknApplication*)((CAknAppUi*)CEikonEnv::Static()->AppUi())->Application();
		MApp_context* appc=GetContext();
		
		CEikonEnv::Static()->AppUi()->PrepareToExit();
		CEikonEnv::Static()->DisableExitChecks(ETrue);
		CEikonEnv::Static()->DestroyEnvironment();

		RDebug::Print(_L("User::Exit"));

		User::Exit(exit_with);
		return ENoDisplay;
	} else {
		if (aErrorText.Length()==0) {
			aErrorText=_L("Error ");
			aErrorText.AppendNum(aError);
		}
		return EAlertDisplay;
	}
}

EXPORT_C void MContextAppUi::SetInHandlableEvent(TBool aHandlable)
{
	iHandlableError=aHandlable;
}
