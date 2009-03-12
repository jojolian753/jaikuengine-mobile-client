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

#include "ContextFetchAppUi.h"
#include <ContextFetch.rsg>
#include "contextfetch.hrh"

#include "browser_interface.h"
#include <avkon.hrh>
#include <viewcli.h> //CVwsSessionWrapper
#include <vwsdef.h>
#include <charconv.h>

// ================= MEMBER FUNCTIONS =======================
//
// ----------------------------------------------------------
// CContextFetchAppUi::ConstructL()
// ?implementation_description
// ----------------------------------------------------------
//
void CContextFetchAppUi::ConstructL()
    {
    BaseConstructL();


    TUid v={1};
    CRecognizerView* view = CRecognizerView::NewL(this, v, v, &iDummy);

    AddViewL( view );

    SetDefaultViewL(*view);

    }

// ----------------------------------------------------
// CContextFetchAppUi::~CContextFetchAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CContextFetchAppUi::~CContextFetchAppUi()
    {
   }


// ----------------------------------------------------
// CContextFetchAppUi::HandleCommandL(TInt aCommand)
// ?implementation_description
// ----------------------------------------------------
//
void CContextFetchAppUi::HandleCommandL(TInt aCommand)
    {
    switch ( aCommand )
        {
        case EEikCmdExit:
            {
            Exit();
            break;
            }
        default:
            break;      
        }
    }

void CContextFetchAppUi::CodeSelected(const CCodeInfo& aCode)
{
	TBuf<256> url=_L("http://db.cs.helsinki.fi/~mraento/cgi-bin/pics.pl?vc;");
	aCode.code->AppendToString(url);


#ifndef __WINS__
#  ifndef __S60V2__
		CDorisBrowserInterface* ido=CDorisBrowserInterface::NewL();
		CleanupStack::PushL(ido);
		ido->AppendL(CDorisBrowserInterface::EOpenURL_STRING, url);
		ido->ExecuteL();
		CleanupStack::PopAndDestroy();
#  else
		CCnvCharacterSetConverter* cc;
		cc=CCnvCharacterSetConverter::NewLC();
		cc->PrepareToConvertToOrFromL(KCharacterSetIdentifierIso88591, CEikonEnv::Static()->FsSession());
		HBufC8* addr8=HBufC8::NewLC(url.Length());
		TPtr8 addrp=addr8->Des();
		cc->ConvertFromUnicode(addrp, url);
		TUid KUidOperaBrowserUid = {0x101F4DED};
		TUid KUidOperaRenderViewUid = {0};
		TVwsViewId viewId(KUidOperaBrowserUid, KUidOperaRenderViewUid);
		CVwsSessionWrapper* vws;
		vws=CVwsSessionWrapper::NewLC();
		vws->ActivateView(viewId, TUid::Uid(0), *addr8);
		CleanupStack::PopAndDestroy(3);
#  endif
#endif
	Exit();
}

void CContextFetchAppUi::Cancelled()
{
	Exit();
}

// End of File  
