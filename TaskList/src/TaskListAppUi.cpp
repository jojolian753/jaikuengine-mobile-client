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

#include "TaskListAppUi.h"
#include "TaskListContainer.h" 
#include <TaskList.rsg>
#include "tasklist.hrh"

#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <bautils.h>
#include <aknquerydialog.h>

#ifdef __S80__
#include <ckninfo.h>
#endif

enum TSWStartupReason
{
	// Normal startup reasons (100..149)

	// Nothing set the (default value).
	ESWNone = 100,

	// Restore Factory Settings (Normal)
	ESWRestoreFactorySet = 101,

	// Language Switched
	ESWLangSwitch = 102,

	// Warranty transfer
	ESWWarrantyTransfer = 103,

	// Possibly needed for handling
	// power off & charger connected use case.
	ESWChargerConnected = 104,

	// Restore Factory Settings (Deep)
	ESWRestoreFactorySetDeep = 105
};

class SysStartup {
public:
	IMPORT_C static TInt ShutdownAndRestart( const class TUid& aSource, TSWStartupReason aReason);
};

// ================= MEMBER FUNCTIONS =======================
//
// ----------------------------------------------------------
// CTaskListAppUi::ConstructL()
// ?implementation_description
// ----------------------------------------------------------
//
void CTaskListAppUi::ConstructL()
{
	BaseConstructL();

	iThreadNames=new (ELeave) CDesC16ArrayFlat(4);
	iThreadIds=new (ELeave) CArrayFixFlat<TThreadId>(4);

	GetThreads();

	iAppContainer = new (ELeave) CTaskListContainer;
#ifndef __S80__
	iAppContainer->SetMopParent(this);
#endif
	iAppContainer->ConstructL( ClientRect(), iThreadNames, this );
	AddToStackL( iAppContainer );
}

// ----------------------------------------------------
// CTaskListAppUi::~CTaskListAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CTaskListAppUi::~CTaskListAppUi()
{
	if (iAppContainer)
	{
		RemoveFromStack( iAppContainer );
		delete iAppContainer;
	}
	delete iThreadNames;
	delete iThreadIds;
}

void CTaskListAppUi::GetThreads()
{
	TFindThread f(_L("*"));
	TFullName t;
	RThread r;
	iThreadNames->Reset();
	iThreadIds->Reset();
	while (f.Next(t)==KErrNone) {
		r.Open(t);
		CleanupClosePushL(r);
		iThreadNames->AppendL(t);
		iThreadIds->AppendL(r.Id());
		CleanupStack::PopAndDestroy();
	}
}

// ------------------------------------------------------------------------------
// CTaskListAppUi::::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
//  This function is called by the EIKON framework just before it displays
//  a menu pane. Its default implementation is empty, and by overriding it,
//  the application can set the state of menu items dynamically according
//  to the state of application data.
// ------------------------------------------------------------------------------
//
void CTaskListAppUi::DynInitMenuPaneL(
				      TInt /*aResourceId*/,CEikMenuPane* /*aMenuPane*/)
{
}

// ----------------------------------------------------
// CTaskListAppUi::HandleKeyEventL(
//     const TKeyEvent aKeyEvent,TEventCode /*aType*/)
// ?implementation_description
// ----------------------------------------------------
//
TKeyResponse CTaskListAppUi::HandleKeyEventL(
	const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
{
	return EKeyWasNotConsumed;
}

// ----------------------------------------------------
// CTaskListAppUi::HandleCommandL(TInt aCommand)
// ?implementation_description
// ----------------------------------------------------
//
void CTaskListAppUi::KillCurrent()
{
	TInt idx=iAppContainer ->GetCurrentIdx();
	TThreadId id=iThreadIds->At(idx);
	RThread t;
	t.Open(id);
	t.Kill(-1000);
	t.Close();
	GetThreads();
	iAppContainer->ContentsChanged();
}

void CTaskListAppUi::HandleCommandL(TInt aCommand)
{
	switch ( aCommand )
	{
	case EAknSoftkeyBack:
	case EEikCmdExit:
		{
			Exit();
			break;
		}
	case ETaskListCmdAppKill:
		{
			KillCurrent();
			break;
		}
	case ETaskListCmdAppReboot:
		{
			TUid t={0x101FBAD3};
			SysStartup::ShutdownAndRestart( t, ESWNone);
		}

	case ETaskListCmdCopyRom:
		{
			RFs& fs=CEikonEnv::Static()->FsSession();
			BaflUtils::EnsurePathExistsL(fs, _L("e:\\copy\\"));
			User::LeaveIfError(BaflUtils::CopyFile(fs, _L("z:\\system\\*.*"), _L("e:\\copy\\"),
				CFileMan::ERecurse));
		}
	case ETaskListCmdSettings:
		{
			TInt id=1;
			CAknNumberQueryDialog* d=CAknNumberQueryDialog::NewL(id);
			d->ExecuteLD(R_VIEWID_INPUT);
			ActivateViewL(TVwsViewId( TUid::Uid(0x100058ec), TUid::Uid(id) ));
		}
	case ETaskListCmdAppRefresh:
		{
			GetThreads();
			iAppContainer->ContentsChanged();
		}
	case ETaskListCmdCopyFile:
		{
			CopyFile();
		}
	default:
		break;      
	}
}

void FormatHex(const TDesC8& from, TDes& into)
{
	TInt len=from.Length();
	TInt space=into.MaxLength()/2;

	if (len > space) len=space;
	TBuf<4> hex, b;
	for (int i=0; i<len; i++) {		
		hex.Num( from[i], EHex );
		if (hex.Length()==1) b=_L("0");
		else b.Zero();
		b.Append(hex);
		into.Append(b);
	}

}

/*
#include <CAknFileSelectionDialog.h>

void CTaskListAppUi::CopyFile()
{
	TFileName filename;

	// Create default filename. (contains only the folder, 
	// e.g. C:\Nokia\Images\) This is used as a starting folder for browsing.
	filename.Append(_L("C:\\"));

	_LIT(KDialogTitle, "Browse files");
	TBool ret = CAknFileSelectionDialog::RunDlgLD(
		filename,		// on return, contains the selected file's name
		KNullDesC, // default root path for browsing
		R_FILE_SELECTION_DIALOG_C,
		0			// Pointer to class implementing 
						// MAknFileSelectionObserver. OkToExitL is called
						// when user has selected an file.
		);

	// "ret" is true, if user has selected a file
	if( ret ) {
	}

}
*/

void CTaskListAppUi::CopyFile()
{
	_LIT(KFrom, "c:\\system\\data\\contacts.cdb");
	_LIT(KTo, "c:\\contacts.cdb");
	TInt pushed=0;
	RFile f; 
	RFs& fs=iEikonEnv->FsSession();
	User::LeaveIfError(f.Replace(fs, KTo, EFileWrite));
	CleanupClosePushL(f); ++pushed;
	HBufC8* buf=HBufC8::NewL(16384);
	CleanupStack::PushL(buf); ++pushed;
	TInt i=0;
	TPtr8 p=buf->Des();
	User::LeaveIfError(fs.ReadFileSection(KFrom, i, p=buf->Des(), 16384));
	while ( (*buf).Length() ) {
		User::LeaveIfError(f.Write(*buf));
		i+=16384;
		p=buf->Des();
		User::LeaveIfError(fs.ReadFileSection(KFrom, i, p=buf->Des(), 16384));
	}
	CleanupStack::PopAndDestroy(pushed);
}

void CTaskListAppUi::HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType)
{
	if (aEventType != EEventItemClicked && aEventType!=EEventEnterKeyPressed) return;
	TBuf<400> msg;

	TInt idx=iAppContainer ->GetCurrentIdx();
	TThreadId id=iThreadIds->At(idx);
	TUint i=id;
	RProcess p;
	RThread t;
	t.Open(id);

	TInt aHeapSize=0; TInt aStackSize=0;
	TBuf8<256> context;
#ifndef __S60V3__
	t.GetRamSizes(aHeapSize, aStackSize);
	t.Context(context);
#endif
	TBuf<16> context_beg;
	FormatHex(context, context_beg);


	TInt etype, ereason;
	TExitCategoryName ecat;
	etype=t.ExitType();
	ereason=t.ExitReason();
	ecat=t.ExitCategory();

	t.Process(p);
	t.Close();
	TUint pi=p.Id();

#ifndef __S60V3__
	HBufC* cmd=HBufC::NewLC(p.CommandLineLength());
	p.Open(p.Id());
	TPtr cmdp=cmd->Des();
	p.CommandLine(cmdp);
#else
	HBufC* cmd=HBufC::NewLC(1);
#endif

	//TInt aCodeSize=0; TInt aConstDataSize=0; TInt anInitialisedDataSize=0; TInt anUninitialisedDataSize=0;
	//p.GetRamSizes(aCodeSize, aConstDataSize, anInitialisedDataSize, anUninitialisedDataSize);

	TFileName file=p.FileName();
	p.Close();

	//msg.Format(_L("T Id: %x\nP Id: %x\nEType %d EReason %d\nECat %S\nCmd: %S\n"), i, pi, etype, ereason, ecat,
	//	(*cmd));
	msg.Format(_L("T Id: %x\nContext: %S\nP Id: %x\nFile: %S\nCmd: %S\nHeap: %d\nStack: %d\n"), i, &context_beg, pi, &file,
		&(*cmd), aHeapSize/1024, aStackSize/1024);
#ifndef __S80__
	CAknMessageQueryDialog* note=CAknMessageQueryDialog::NewL(msg);
	note->ExecuteLD(R_LOGVIEW_EVENT_DIALOG);
#else
	CCknInfoDialog::RunDlgDefaultIconLD(msg, 0);
#endif
	CleanupStack::PopAndDestroy(); //cmd
}

// End of File  
