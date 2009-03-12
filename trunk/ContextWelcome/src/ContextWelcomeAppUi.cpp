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

#include "ContextWelcomeAppUi.h"

#include "ContextWelcome.pan"
#include "ContextWelcome.hrh"
#include "ContextWelcomeContainer.h"

#include "cwu_welcome.h"

#include "cu_common.h"
#include "juik_iconmanager.h"
#include "juik_layout_impl.h"
#include "app_context_impl.h"
#include "callstack.h"
#include "file_logger.h"
#include "break.h"

#include <eikenv.h>

// METHOD DEFINITIONS

_LIT(file_prefix, "welcome");

void CContextWelcomeAppUi::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextWelcomeAppUi"), _CL("ConstructL"));
#ifdef __WINS__
	TInt dummy;
	TBreakItem __breakitem(GetContext(), dummy); 
#endif
	MContextAppUi::ConstructL(AppContext(), file_prefix, 3);

	WrapInnerConstructL();
}


void CContextWelcomeAppUi::LoadResourcesL()
{
	CALLSTACKITEM_N(_CL("CContextWelcomeAppUi"), _CL("LoadResourcesL"));

	_LIT( KCcuResource, "ContextContactsUi");
	iResourceHandles.Append( LoadSystemResourceL( CEikonEnv::Static(), KCcuResource ) );
	_LIT( KCwuResource, "ContextWelcomeUi");
	iResourceHandles.Append( LoadSystemResourceL( CEikonEnv::Static(), KCwuResource ) );
}

void CContextWelcomeAppUi::InnerConstructL()
{
	CALLSTACKITEM_N(_CL("CContextWelcomeAppUi"), _CL("InnerConstructL"));
	
#ifndef __S60V2__
	BaseConstructL(0x1008);
#else
	BaseConstructL(EAknEnableSkin);
#endif

	LoadResourcesL();

	iLayout = CJuikLayout::NewL();
	GetContext()->TakeOwnershipL( iLayout );
	GetContext()->SetLayout( iLayout );

	iIconManager = CJuikIconManager::NewL();
	GetContext()->TakeOwnershipL( iIconManager );
	GetContext()->SetIconManager( iIconManager );

	iWelcomeController = CWelcomeController::NewL(*this);
	iWelcomeController->StartL();
}


TErrorHandlerResponse CContextWelcomeAppUi::HandleError(TInt aError,
														const SExtendedError& aExtErr,
														TDes& aErrorText,
														TDes& aContextText)
{
	MContextAppUi::HandleError(aError, aExtErr, aErrorText, aContextText);
	return CAknAppUi::HandleError(aError, aExtErr, aErrorText, aContextText);
}

void CContextWelcomeAppUi::DumpCallStackL()
{
	CALLSTACKITEM_N(_CL("CContextWelcomeAppUi"), _CL("DumpCallStackL"));

	HBufC* stack = NULL;
	stack=AppContextAccess().CallStackMgr().GetFormattedCallStack(_L("AppUi"));
	
	if (stack) {
		TInt err = KErrNone;
		TTime t=GetTime();
		CC_TRAP(err, iLog->new_value(CBBSensorEvent::ERR, _L("app_event"), *stack, t));
	}
	delete stack;
}
	

CContextWelcomeAppUi::CContextWelcomeAppUi(MApp_context& aContext) : MContextBase(aContext) {
}


CContextWelcomeAppUi::~CContextWelcomeAppUi()
{
	CC_TRAPD(err, ReleaseCContextWelcomeAppUi());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

void CContextWelcomeAppUi::ReleaseCContextWelcomeAppUi()
{
	CALLSTACKITEM_N(_CL("CContextWelcomeAppUi"), _CL("ReleaseCContextWelcomeAppUi"));
	delete iWelcomeController;

	for ( TInt i=0; i < iResourceHandles.Count(); i++)
		{
			iEikonEnv->DeleteResourceFile( iResourceHandles[i] );
		}
	iResourceHandles.Close();
}

void CContextWelcomeAppUi::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CContextWelcomeAppUi"), _CL("HandleCommandL"));
  switch(aCommand)
    {
    case EEikCmdExit:
    case EAknSoftkeyExit:
		Exit();
	break;
	
    default:
		CAknViewAppUi::HandleCommandL( aCommand );
		break;
    }
}


void CContextWelcomeAppUi::HandleResourceChangeL( TInt aType )
{
	CAknViewAppUi::HandleResourceChangeL( aType );
	if ( aType == KEikDynamicLayoutVariantSwitch )
		{
			iLayout->UpdateLayoutDataL();
			iWelcomeController->HandleResourceChangeL( aType );
		}
}
