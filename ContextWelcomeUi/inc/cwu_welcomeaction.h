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

#ifndef __CWU_WELCOMEACTION_H__
#define __CWU_WELCOMEACTION_H__

#include <e32base.h>
#include "app_context.h"

class MWelcomeActionObserver 
{
 public:
	virtual void ActionReadyL(class CWelcomeAction& aAction) = 0;
	virtual void QuitWelcomeL(class CWelcomeAction& aAction) = 0;
};


class MWelcomeView 
{
 public:
	virtual void HidePageL(TBool aHide) = 0;
	virtual void SetPageL( class CWelcomePageBase* aPage ) = 0;

	virtual void ShowWaitDialogL( TInt aResource ) = 0;
	virtual void StopWaitDialogL() = 0;

	virtual class MObjectProvider& ObjectProviderL() = 0;
};

/**
 * Welcome action is (almost) self-contained action, that welcome process performs.
 * Each welcome action can have one ui step in welcome process (launched with ShowUiL)
 * in addition CWelcomeAction can perform preliminary background activities, which
 * are started with StartBackgroundActivitiesL. 
 * Dependencies between welcome action results and welcome actions has thus 
 * to be modelled outside welcome action, or welcome action has to register as a listener
 * to action that it's results depend on.
 */
class CWelcomeAction : public CBase, public MContextBase  
{
 public:

	virtual void StartBackgroundActivitiesL() = 0;
	virtual void ShowUiL(MWelcomeView& aView) = 0;
  virtual void HandleOrientationChangeL() {}

 public:   
	virtual ~CWelcomeAction();

	void AddObserverL( MWelcomeActionObserver& aObserver );
	void RemoveObserverL( MWelcomeActionObserver& aObserver );
	
 protected:
	CWelcomeAction();
	void ConstructL();
	void NotifyActionReadyL();
	void NotifyQuitWelcomeL();
	
	RPointerArray<MWelcomeActionObserver> iObservers;
};

CWelcomeAction* CreatePrivacyStatementActionL();
CWelcomeAction* CreateApSelectionActionL();
CWelcomeAction* CreateUserSelectionActionL();
CWelcomeAction* CreateCalendarSharingActionL();
CWelcomeAction* CreateBluetoothQueryActionL();
CWelcomeAction* CreateNickAndPasswordActionL();
CWelcomeAction* CreateIntroPageL();
CWelcomeAction* CreateFinalPageL();

CWelcomeAction* CreateNetworkAccessActionL();
CWelcomeAction* CreateAutoStartupActionL();
CWelcomeAction* CreateBatteryQueryActionL();

#endif 
