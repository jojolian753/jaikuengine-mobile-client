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

#ifdef __S60V3__
#include <e32base.h>
IMPORT_C void SwitchToBetterHeap(const TDesC&);
_LIT(KHeap, "context_log_heap_chunk");
#endif

#ifndef CONTEXTJAIKU
// INCLUDE FILES
#include    "Context_logApp.h"
#include    "Context_logDocument.h"

// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CContext_logApp::AppDllUid()
// Returns application UID
// ---------------------------------------------------------
//
TUid CContext_logApp::AppDllUid() const
    {
    return KUidcontext_log;
    }

   
// ---------------------------------------------------------
// CContext_logApp::CreateDocumentL()
// Creates CContext_logDocument object
// ---------------------------------------------------------
//
CApaDocument* CContext_logApp::CreateDocumentL()
    {
    return CContext_logDocument::NewL( *this );
    }

// ================= OTHER EXPORTED FUNCTIONS ==============
//
// ---------------------------------------------------------
// NewApplication() 
// Constructs CContext_logApp
// Returns: created application object
// ---------------------------------------------------------
//
#ifndef __S60V3__
EXPORT_C CApaApplication* NewApplication()
    {
    return new CContext_logApp;
    }

GLDEF_C TInt E32Dll( TDllReason )
    {
    return KErrNone;
    }
#else
#include <eikstart.h> 

GLDEF_C TInt E32Main()
    {
    SwitchToBetterHeap(KHeap);
    return EikStart::RunApplication(NewApplication);
    }

#endif

#else // CONTEXTJAIKU
#include "sensorrunner.h"
GLDEF_C TInt E32Main()
    {
#ifdef __WINS__1
    RChunk heapc;
    
    TInt err=heapc.OpenGlobal(_L("jaikusettings_heap"), ETrue);
    if (err!=KErrNone) {
    	return CSensorRunner::RunSensorsInThread(0);
    }
	RThread thread;
	TInt heap=*(TInt*)heapc.Base();
	heapc.Close();
	err=thread.Create(_L("context_log2"), 
		&CSensorRunner::RunSensorsInThread, // thread's main function
		20*1024, /* stack */
		heap, /* min heap */
		heap, /* max heap */
		0,
		EOwnerProcess);
	if (err!=KErrNone) return err;
	thread.SetPriority(EPriorityNormal);
	TRequestStatus s;
	thread.Logon(s);
	thread.Resume();
	User::WaitForRequest(s);
	TExitCategoryName n=thread.ExitCategory();
	TInt reason=thread.ExitReason();
	TExitType exittype=thread.ExitType();
	thread.Close();
	if (exittype==EExitPanic) {
		User::Panic( n, reason);
	}
	return reason;
#else
    SwitchToBetterHeap(KHeap);

    return CSensorRunner::RunSensorsInThread(0);
#endif
}
#endif

// End of File  
