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

#include "Context_logDocument.h"
#include "Context_logAppUi.h"
#include "app_context.h"
#include "settings.h"
#include "cl_settings.h"
#include "symbian_auto_ptr.h"
#include "bb_settings.h"

#include "cc_schedulers.h"
#include <bautils.h>
#include "callstack.h"

#define DO_DEBUG
//#define LOWMEM_TEST 1
// ================= MEMBER FUNCTIONS =======================

// constructor
CContext_logDocument::CContext_logDocument(CEikApplication& aApp)
: CAknDocument(aApp)    
{
}

// destructor
CContext_logDocument::~CContext_logDocument()
{
}

void MoveSingleFileL(RFs& aFs, const TDesC& aName)
{
	TFileName from, to;
	from=_L("e:\\system\\data\\context\\"); from.Append(aName);
	to=_L("c:\\system\\data\\context\\"); to.Append(aName);
	TInt err=BaflUtils::RenameFile(aFs, from, to);
	if (err!=KErrNone && err!=KErrNotFound) User::Leave(err);
}

void MoveFromMMC(RFs& aFs)
{
	MoveSingleFileL(aFs, _L("CELLS.db"));
	MoveSingleFileL(aFs, _L("TUPLE.db"));
	MoveSingleFileL(aFs, _L("TAGS.db"));
	MoveSingleFileL(aFs, _L("CL_MEDIA.db"));
	MoveSingleFileL(aFs, _L("TRANSFER.db"));
	MoveSingleFileL(aFs, _L("cellid_names_v3.txt"));
	MoveSingleFileL(aFs, _L("BTDEV.db"));
	MoveSingleFileL(aFs, _L("BASESTACK.db"));
	MoveSingleFileL(aFs, _L("BASES.db"));
	MoveSingleFileL(aFs, _L("LOOKUPS.db"));
}

// EPOC default constructor can leave.
void CContext_logDocument::ConstructL()
{
	MContextDocument::ConstructL(Application(),
		iDefaultSettings, KCLSettingsTuple,
		_L("context_log.txt"), _L("context_log"));

#ifdef __WINS__
	iContext->SetAppDir(_L("c:\\system\\apps\\context_log\\"));
#endif
}

// Two-phased constructor.
CContext_logDocument* CContext_logDocument::NewL(
						 CEikApplication& aApp)     // CContext_logApp reference
{
#ifdef LOWMEM_TEST
	void* p=User::Alloc(16*1024*1024);
	User::Free(p);

	TInt err=KErrNoMemory, fail_on=1;
	while (err==KErrNoMemory) {
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::EDeterministic, fail_on);
		User::__DbgMarkStart(RHeap::EUser);

		TRAP(err, {
	auto_ptr<CContext_logDocument> self(new (ELeave) CContext_logDocument( aApp ));
	self->ConstructL();

		});
		User::__DbgMarkEnd(RHeap::EUser,0);
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
		++fail_on;
	}
	User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
	if (err!=KErrNone && err!=KErrNoMemory) {
		User::Panic(_L("LOWMEM"), 1);
	}
#endif
	auto_ptr<CContext_logDocument> self(new (ELeave) CContext_logDocument( aApp ));
	self->ConstructL();
	return self.release();
}

// ----------------------------------------------------
// CContext_logDocument::CreateAppUiL()
// constructs CContext_logAppUi
// ----------------------------------------------------
//
CEikAppUi* CContext_logDocument::CreateAppUiL()
{
	return new (ELeave) CContext_logAppUi(*iContext);
}

// End of File  
