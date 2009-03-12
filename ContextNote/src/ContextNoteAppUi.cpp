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

#include "ContextNoteAppUi.h"
#include "ContextNoteContainer.h" 
#include <ContextNote.rsg>
#include "contextnote.hrh"

#include <avkon.hrh>
#include <bautils.h>

// ================= MEMBER FUNCTIONS =======================
//
// ----------------------------------------------------------
// CContextNoteAppUi::ConstructL()
// ?implementation_description
// ----------------------------------------------------------
//
void CContextNoteAppUi::ConstructL()
{
	BaseConstructL();
	iAppContainer = new (ELeave) CContextNoteContainer;
	iAppContainer->SetMopParent(this);
	iAppContainer->ConstructL( ClientRect() );
	AddToStackL( iAppContainer );
}

// ----------------------------------------------------
// CContextNoteAppUi::~CContextNoteAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CContextNoteAppUi::~CContextNoteAppUi()
{
	if (iAppContainer)
        {
		RemoveFromStack( iAppContainer );
		delete iAppContainer;
        }
}

// ------------------------------------------------------------------------------
// CContextNoteAppUi::::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
//  This function is called by the EIKON framework just before it displays
//  a menu pane. Its default implementation is empty, and by overriding it,
//  the application can set the state of menu items dynamically according
//  to the state of application data.
// ------------------------------------------------------------------------------
//
void CContextNoteAppUi::DynInitMenuPaneL(
					 TInt /*aResourceId*/,CEikMenuPane* /*aMenuPane*/)
{
}

// ----------------------------------------------------
// CContextNoteAppUi::HandleKeyEventL(
//     const TKeyEvent& aKeyEvent,TEventCode /*aType*/)
// ?implementation_description
// ----------------------------------------------------
//
TKeyResponse CContextNoteAppUi::HandleKeyEventL(
						const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
{
	return EKeyWasNotConsumed;
}

// ----------------------------------------------------
// CContextNoteAppUi::HandleCommandL(TInt aCommand)
// ?implementation_description
// ----------------------------------------------------
//
void CContextNoteAppUi::HandleCommandL(TInt aCommand)
{
	switch ( aCommand )
        {
        case ECmdSave:
		Save();
		break;
        case ECmdDiscard:
		Discard();
		break;
        default:
		break;      
        }
}

void CContextNoteAppUi::Save()
{
	TFileName dir;
#ifndef __WINS__
	TParsePtrC parse(Application()->DllName());
	dir=parse.DriveAndPath();
#else
	dir=_L("c:\\system\\apps\\ContextNote\\");
#endif
	RFs& fs=CEikonEnv::Static()->FsSession();
	TFileName filen;
	dir.Append(_L("note"));
	filen.Append(dir);
	TInt i=1; filen.AppendNum(i); filen.Append(_L(".txt"));
	while (BaflUtils::FileExists(fs, filen)) {
		filen=dir;
		++i; filen.AppendNum(i); filen.Append(_L(".txt"));
	}
	
	RFile f;
	TInt ret=f.Replace(fs, filen, EFileWrite);
	User::LeaveIfError(ret);
	const TDesC& t=iAppContainer->GetText();
	TPtrC8 p( (TText8*)t.Ptr(), t.Length()*2);
	f.Write(p);
	f.Close();
	Exit();
}

void CContextNoteAppUi::Discard()
{
	Exit();
}

// End of File  
