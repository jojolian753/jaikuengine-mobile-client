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

#ifndef __CWU_WELCOMEVIEWS_H__
#define __CWU_WELCOMEVIEWS_H__

#include "app_context.h"

#include "cwu_welcomeaction.h"

#include <e32std.h>
#include <aknview.h>
#include <coecntrl.h>

class CWelcomePageBase;

class TJuikLayoutItem;

class MPageObserver 
{
 public:
	virtual void SelectedItemL(TInt aIndex) = 0;
	virtual void LeftSoftKeyL(TInt aIndex) = 0;
	virtual void RightSoftKeyL(TInt aIndex) = 0;
};



const TUid KWelcomeUiViewId = {0x160};

class CWelcomeViewBase : public CAknView, public MContextBase, public MWelcomeView
{
 public:
	static CWelcomeViewBase* NewL();

	virtual ~CWelcomeViewBase();

 public: // from MWelcomeView 
	void HidePageL(TBool aHide);
	void SetPageL( CWelcomePageBase* aPage );
	MObjectProvider& ObjectProviderL();

	
 public: // From CAknview
	TUid Id() const;	

 public: // own (to be hidden)
	void ChangePageL( CWelcomePageBase* aPage );
	void SetFirstPageL( CWelcomePageBase* aPage );

	// wait dialogs
	void ShowWaitDialogL( TInt aNoteResource );
	void StopWaitDialogL();

	void HandleResourceChangeL( TInt aType );
   

 protected: // From CAknView
	void DoActivateL(const TVwsViewId& /*aPrevViewId*/,
					 TUid /*aCustomMessageId*/,
					 const TDesC8& /*aCustomMessage*/);
	void DoDeactivate();
	
	void HandleCommandL(TInt aCommand);
  	
	void ReleaseCWelcomeViewBase();

 protected: // own
	void ConstructL();
	
 protected:
	TBool iIsDone;
	TBool iInStack;
	CWelcomePageBase* iPage;

	class CAknWaitDialog* iWaitDialog;
};
  

class CWelcomePageBase : public CCoeControl, public MContextBase {
 public:
	virtual ~CWelcomePageBase();

 public:
	enum THeaderText
		{
			EHeaderText_NoText = -1,
			EHeaderText_AccessPoint,
			EHeaderText_Bluetooth,
			EHeaderText_Calendar,
			EHeaderText_Login,
			EHeaderText_YourNumber,			
			EHeaderText_Internet,			
			EHeaderText_AutoStart,
			EHeaderText_BatteryUsage,		
		};
	
 protected:
	CWelcomePageBase();
	void ConstructL(MPageObserver& aObserver);
	void SetMainTextL( const TDesC& aTxt );
	void SetHeaderTextL( CWelcomePageBase::THeaderText aTextTag );
	
	void SizeChanged();
	
	void Draw( const TRect& aRect ) const;
	void DrawGraphicsL(CWindowGc& aGc, TInt aIndex, const TJuikLayoutItem& aParent, TBool aDoCenter=ETrue) const;
	void DrawRectL(CWindowGc& aGc, const TRgb& aColor, const TJuikLayoutItem& aL) const;
	TBool IsE90();


	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;

	virtual TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	
 private:
	void WrapBodyTextL();

 public:
	virtual void LeftSoftKeyL();
	virtual void RightSoftKeyL();
	void UpdateLayoutsL();

protected:
	RPointerArray<CCoeControl> iControls;
	class CAknIconArray* iIcons;
	HBufC* iBodyText;
	class CEikLabel* iMainText;	
	class CEikLabel* iTitleText;	
	class CEikLabel* iLeftSoftkey;	
	class CEikLabel* iRightSoftkey;	
	MPageObserver* iObserver;
	
	enum TWelcomeLayoutStyle
		{
			EWelcomeLayout,
			ESelectionLayout,
			ECongratulationsLayout 
		} iLayoutStyle;

	TRect iPageRect;


	TInt iHeaderGraphicsIndex;
};



class CWelcomeInfoPage : public CWelcomePageBase {
 public:
	static CWelcomeInfoPage* NewL(MObjectProvider& aMopParent, 
								  const TDesC& aText,
								  MPageObserver& aObserver,
								  CWelcomePageBase::THeaderText aHeader,
								  const TDesC& aSoftkeyText = KNullDesC,
								  TBool aCongratulationsLayout = EFalse);
public:
	CWelcomeInfoPage();
	~CWelcomeInfoPage();	
	
	void ConstructL(MObjectProvider& aMopParent, 
					const TDesC& aText, MPageObserver& aObserver, 
					const TDesC& aSoftkey, TBool aCongratulationsLayout);
private:
};


class CWelcomeIntroPage : public CWelcomePageBase {
 public:
	static CWelcomeIntroPage* NewL(MObjectProvider& aMopParent, 
								  const TDesC& aText,
								  MPageObserver& aObserver
								  );
public:
	CWelcomeIntroPage();
	~CWelcomeIntroPage();	
	
	void ConstructL(MObjectProvider& aMopParent, 
					const TDesC& aText, MPageObserver& aObserver);
private:
};


class CWelcomeSelectionPage : public CWelcomePageBase {
public:
	static CWelcomeSelectionPage* NewL(MObjectProvider& aMopParent, 
									   const TDesC& aText,
									   MDesCArray& aSelectionModel,
									   MPageObserver& aObserver,
									   CWelcomePageBase::THeaderText,
									   TBool aCongratulationsLayout = EFalse);

public:
	CWelcomeSelectionPage(TBool aCongratulationsLayoutt);
	~CWelcomeSelectionPage();	

	void SizeChanged();

	void ConstructL(MObjectProvider& aMopParent, 
					const TDesC& aText, MDesCArray& aSelectionModel,
					MPageObserver& aObserver);
	
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

	virtual void LeftSoftKeyL();
	virtual void RightSoftKeyL();

private:
	class CSelectionList* iList;
};

#endif
