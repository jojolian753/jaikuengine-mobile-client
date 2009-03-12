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

#ifndef USER_VIEW_H_INCLUDED
#define USER_VIEW_H_INCLUDED 1

#include <aknview.h>
#include <eiktxlbx.h>
#include "circular.h"
#include "doublelinebox.h"
#include "presence_publisher.h"

#include "viewids.h"

#undef __JAIKU_ENABLED__

class CUserContextContainer : public CCoeControl, public MListObserver {
public: // Constructors and destructor
        
	CUserContextContainer(CAknViewAppUi* AppUi);
        void ConstructL(const TRect& aRect, CCircularLog* Log, 
		MEikListBoxObserver* ListBoxObserver);
	
        ~CUserContextContainer();
	void ContentsChanged();
	void SetFrozen(TBool aIsFrozen);
	
private: // Functions from base classes
	
	void SetSizes(const TRect& aRect);
        void SizeChanged();
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
private: //data
#ifdef __JAIKU_ENABLED__
    class CJaikuContactsListBox* iListBox;
#else
	class doublelinebox * iListBox;
#endif
	CCircularLog*	iLog;
	TInt iPresses;
	CAknViewAppUi*			iAppUi;
	
	CEikLabel	 *iDescription, *iFrozenLabel;
	HBufC		 *iDescriptionText, *iFrozenText;
};

class CUserView : public CAknView, public MEikListBoxObserver, public MContextBase
{
public: // Constructors and destructor
	static CUserView* NewL(CCircularLog* Log, CPresencePublisher*& PresencePublisher);
        ~CUserView();
	
	void SetFrozen(TBool aIsFrozen);
public: // Functions from base classes
        
        TUid Id() const;
	
        void HandleCommandL(TInt aCommand);
	
        void HandleClientRectChange();
	
private:
	CUserView(CPresencePublisher*& PresencePublisher);
        void ConstructL(CCircularLog* Log);
	
        void DoActivateL(const TVwsViewId& aPrevViewId,
		TUid aCustomMessageId,
		const TDesC8& aCustomMessage);
        void DoDeactivate();
	
	void HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType);
        void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane);
	void ReleaseCUserView();

private: // Data
	CUserContextContainer*	iContainer;
	CCircularLog*	iLog;
	CPresencePublisher*& iPresencePublisher;
	
	
};


#endif // USER_VIEW_H_INCLUDED
