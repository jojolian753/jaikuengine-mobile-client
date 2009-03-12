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

#include <avkon.hrh>
#include <aknnotewrappers.h> 

#include "MeaningApp.pan"
#include "MeaningAppAppUi.h"
#include "MeaningApp.hrh"

#include "connectioninit.h"
#include "context_log.hrh"
#include "mediarunner.h"
#include "presencemaintainer.h"
#include "status_notif.h"

#include "juik_layout_impl.h"
#include "app_context_impl.h"

#include <aknview.h>
#include <bautils.h>

class TDummyObserver : public i_status_notif, public MSocketObserver {
	virtual void success(CBase* source) { }
	virtual void error(CBase* source, TInt code, const TDesC& reason) { 
		TBuf<10> msg; msg.AppendNum(code);
		GetContext()->Reporting().UserErrorLog(reason);
		GetContext()->Reporting().UserErrorLog(msg);
	}
	virtual void info(CBase* source, const TDesC& msg) {
		GetContext()->Reporting().UserErrorLog(msg);
	}
	virtual void finished() { }
	virtual void error(const TDesC& descr) {
		GetContext()->Reporting().UserErrorLog(descr);
	}
	virtual void status_change(const TDesC& status) {
		GetContext()->Reporting().UserErrorLog(status);
	}
};

// ConstructL is called by the application framework
void CMeaningAppAppUi::InnerConstructL()
{
	BaseConstructL(EAknEnableSkin);

	iLayout = CJuikLayout::NewL();
	GetContext()->TakeOwnershipL( iLayout );
	GetContext()->SetLayout( iLayout );


	iObserver=new (ELeave) TDummyObserver;
	iPresenceMaintainer=CPresenceMaintainer::NewL(AppContext(), 0, 0, 0, 0);
	iMediaRunner=CMediaRunner::NewL(this, iPresenceMaintainer, &iNextView, *iObserver, *iObserver);
	iMediaRunner->CreateSettingsViewsL();
	SetDefaultViewL(*(iMediaRunner->MediaView()));
	iEikonEnv->SetSystem(ETrue);
}

void CMeaningAppAppUi::ConstructL()
{
	MContextAppUi::ConstructL(AppContext(), _L("meaning"), 3);
	Reporting().SetActiveErrorReporter(this);
	WrapInnerConstructL();
}

CMeaningAppAppUi::CMeaningAppAppUi(MApp_context& aContext) : MContextBase(aContext) { }

#include <errorui.h>
#include <aknglobalnote.h>

TErrorHandlerResponse CMeaningAppAppUi::HandleError(TInt aError,
     const SExtendedError& aExtErr,
     TDes& aErrorText,
     TDes& aContextText) {
     
     	CErrorUI* eui=CErrorUI::NewLC();
	CAknGlobalNote *note=CAknGlobalNote::NewLC();
	const TDesC& msg=eui->TextResolver().ResolveErrorString(aError);
	if (msg.Length()>0) {
		note->ShowNoteL(EAknGlobalErrorNote, msg);
	} else {
		TBuf<30> msg=_L("Error: ");
		msg.AppendNum(aError);
		note->ShowNoteL(EAknGlobalErrorNote, msg);
	}
	CleanupStack::PopAndDestroy(2);
	MContextAppUi::HandleError(aError, aExtErr, aErrorText, aContextText);
	return ENoDisplay;
     }
     
void CMeaningAppAppUi::HandleCommandL(TInt aCommand)
{
	if (iMediaRunner->HandleCommandL(aCommand)) return;
	switch(aCommand) {
		case EEikCmdExit:
			Exit();
			break;
		case Econtext_logCmdAppTest:
			{
			TBuf<50> filen, from;
			from=_L("c:\\data\\pic.jpg");
			TInt i=0;
			for (;;) {
				filen=_L("c:\\data\\images\\pic"); filen.AppendNum(i); filen.Append(_L(".jpg"));
				if (! BaflUtils::FileExists(Fs(), filen)) {
					User::LeaveIfError(BaflUtils::CopyFile(Fs(), from, filen));
					break;
				}
				i++;
			}
			}
			break;
	}
}

CMeaningAppAppUi::~CMeaningAppAppUi()
{
    delete iMediaRunner;
    delete iObserver;
    delete iPresenceMaintainer;
}

void CMeaningAppAppUi::HandleResourceChangeL( TInt aType )
{
	CAknViewAppUi::HandleResourceChangeL( aType );
	if ( aType == KEikDynamicLayoutVariantSwitch )
		{
			iLayout->UpdateLayoutDataL();
		}
}
