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

#ifndef CONTEXTCALLLOGAPPUI_H
#define CONTEXTCALLLOGAPPUI_H

#include <eikapp.h>
#include <eikdoc.h>
#include <e32std.h>
#include <coeccntx.h>
#include <aknviewappui.h>
#include <akntabgrp.h>
#include <aknnavide.h>
#include <akniconarray.h>
#include "app_context.h"
#include "file_output_base.h"
#include "presencedetailview.h"
#include "call_log.h"
#include "phonehelper_ui.h"
#include "cb_presence.h"
#include "contextcalllogview.h"
#include "lookup_storage.h"
#include "contextappui.h"

class CContextCallLogContainer;

class CContextCallLogAppUi : public CAknViewAppUi, public MContextBase, MCoeForegroundObserver, 
	public MPresenceListener, public MContextAppUi
{
public: 
	CContextCallLogAppUi(MApp_context& Context) : MContextBase(Context) { }
        void ConstructL();
        ~CContextCallLogAppUi();

public: // New functions
	void Notify(const TDesC& aMessage);
	void PresenceChangedL(TInt ContactId, CBBPresence* Info);
	void DisplayPresenceDetailsL(const TDesC& Name, const CBBPresence* PresenceData, TTime aAtTime);
public:
	void SetTab(TInt tabId);
        
private:
	virtual void HandleGainingForeground();
	virtual void HandleLosingForeground();
    
private:
        void HandleCommandL(TInt aCommand);
	virtual TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

	TBool ProcessCommandParametersL(TApaCommand aCommand, 
		TFileName& aDocumentName,const TDesC8& aTail);

private:

        CAknNavigationControlContainer* iNaviPane;
        CAknNavigationDecorator*        iDecoratedTabGroup;
	CAknTabGroup*                   iTabGroup;

	CPresenceDetailView	*iPresenceDetailView; // OWNED BY FRAMEWORK
	CContextCallLogView *iDialledView, *iReceivedView, *iMissedView, *iLookupView;

	CAknIconArray * iIconlist;
	CArrayFixFlat<TInt>* iResource_files;

	call_log*	iCallLog;
	phonehelper_ui *   iPhoneHelper;
	CDb*		iJabberDb;
	CJabberData*	iJabberData;

	CPresenceHolder* iPresenceHolder;

	CDb*		iLookupDb;
	CLookupStorage * iStorage;
};

#endif

