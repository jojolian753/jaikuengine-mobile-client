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

#ifndef CCU_FEEDFOUNDATION_H
#define CCU_FEEDFOUNDATION_H

#include <e32base.h>
#include <coecntrl.h>

//#include "ccu_streamrenderers.h"
#include "ccu_uidelegates.h"
#include "ccu_userpics.h"

#include "app_context.h"
#include "bbdata.h"
#include "ccu_storage.h"
#include "csd_feeditem.h"
#include "jabberdata.h"
#include "juik_icons.h"
#include "juik_control.h"
#include "juik_focushandling.h"
class CGulIcon;
class CBBFeedItem;
class CJuikLabel;
class CJuikImage;
class CPresenceHolder;

namespace FeedUi
{
	enum TControlFlags
	{
		EShowBuddyIcon      = 0x01,
		EHideAuthorName     = 0x02,
		EDrawSeparator      = 0x04,
		EFocusInsideControl = 0x08,
		EShowParentInfo     = 0x10,
		EShowFullImage      = 0x20,
		EDrawFullFocus      = 0x40
	};
}



/**
 * Convenience stuff for feed controls
 */ 
class MFeedFoundation 
{
public:
	MFeedFoundation( const TUiDelegates& aDelegates) : iDelegates( aDelegates ) {}

	CThemeColors& ThemeColors() const { return *(iDelegates.iThemeColors); }
	CJaicons& Jaicons() const { return *(iDelegates.iJaicons); }
	CUserPics& UserPics() const { return *(iDelegates.iUserPics); }
	CTimePeriodFormatter& PeriodFormatter() const { return *(iDelegates.iPeriodFormatter); }
	CJabberData& JabberData() const { return *(iDelegates.iJabberData); }
	CPresenceHolder& PresenceHolder() const { return *(iDelegates.iPresenceHolder); }
	CJuikGfxStore& Graphics() const { return *(iDelegates.iFeedGraphics); }
	CFeedItemStorage& FeedStorage() const { return *(iDelegates.iFeedStorage); }
	
	TRgb PrimaryTextColor() const;
	TRgb SecondaryTextColor() const;
	TRgb SpecialPrimaryTextColor() const;
	TRgb PrimaryHighlightTextColor() const;
	TRgb SecondaryHighlightTextColor() const;

	TRgb BubbleColor() const;
	TRgb BubbleFocusColor() const;
	TRgb CommentBubbleColor() const;
	TRgb CommentBubbleFocusColor() const;

	
	const CFont* PrimaryFont() const;
	const CFont* SecondaryFont() const;
	const CFont* SmallPrimaryFont() const;

protected:
	TUiDelegates iDelegates;
};

class CFeedControl : public CCoeControl, public MJuikControl, public MFeedFoundation, public MFeedNotify
{
 public:
	class MMinimumSizeObserver 
	{
	public:
		virtual void MinimumSizeChangedL(CFeedControl* aSource, TSize aNewSize) = 0;
	};
	
 public:
	virtual TUint Id() const = 0;
	virtual const TTypeName& TypeName() const = 0;	
	virtual void UpdateUnreadL( CBBFeedItem& /*aItem*/ ) {}
 protected:
	virtual void UpdateL(CBBFeedItem& aItem) = 0;
	virtual void ZeroL() = 0;
	
 public:
    CFeedControl(const TUiDelegates& aDelegates);

	void InitL(CBBFeedItem* aItem);
	void ResetL();

	virtual ~CFeedControl();

	virtual void SetMinimumSizeObserverL(MMinimumSizeObserver* aObserver);
	virtual void MinimumSizeChangedL();

 protected: // from MFeedNotify
	void FeedItemEvent(CBBFeedItem* aItem, TEvent aEvent);
	void AuthorCountEvent(const TDesC& aAuthor,
						  TInt aNewItemCount, TInt aNewUnreadCount);


 protected:
	void SetItem(CBBFeedItem* aItem);
	void ClearItem();
	
 public:
	const CCoeControl* CoeControl() const { return this; } 
	CCoeControl* CoeControl() { return this; }
 
 protected:
	MMinimumSizeObserver* iObserver;
	CBBFeedItem* iFeedItem;
};






class CCommentIndicator : public CCoeControl, public MJuikControl
{
public:	
	static CCommentIndicator* NewL(const TUiDelegates& aDelegates);	
	virtual ~CCommentIndicator() {} 
	virtual void UpdateL( CBBFeedItem& aItem ) = 0;	
	virtual void ZeroL() = 0;
	
	virtual const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }
};


class CFocusableBuddy : public CCoeControl, public MJuikControl
{
 public:
	
	static CFocusableBuddy* NewL( CGulIcon& aDummyIcon, const TUiDelegates& aDelegates, TBool aBubbleFocus=ETrue );
	virtual ~CFocusableBuddy() {}

	virtual void ZeroL() = 0;
	virtual void UpdateL( CGulIcon* aIcon ) = 0;	
	virtual const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }
};



class CGeneralFeedControl : public CFeedControl,
	public MContextBase
{
public:
	CGeneralFeedControl(const TUiDelegates& aDelegates,TInt aFlags);
	virtual ~CGeneralFeedControl();
 public:
	virtual void SetContainerWindowL(const CCoeControl& aControl);
	
 protected:  
	class CJuikSizerContainer& Content() const;	
	virtual void BaseConstructL(); 
	void DrawNormalFocus(CWindowGc& aGc, const TRect& aRect) const;
	void DrawJaikuFocus(CWindowGc& aGc, const TRect& aRect) const;

 protected:
	TBool PreFocusGained(TFocusFrom aFocusFrom);

 protected: // from CCoeControl, needed for MJuikSizers 
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	TSize MinimumSize();
	void PositionChanged();
	void SizeChanged();
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
	void FocusChanged(TDrawNow aDrawNow);
	void Draw(const TRect& aRect) const;


protected:	
	CJuikSizerContainer* iContainer;	
	TMargins8 iMargin;
	TInt iFlags;
	CFocusHandling* iFocusHandling;
};

class CBubbledStreamItem : public CGeneralFeedControl, public MUserPicObserver
{
 public:
	virtual ~CBubbledStreamItem();

	enum TBubbleType
	{
		ENormal = 0,
		EComment,
		ENoBubble
	}; // should change to UUID based typing
	virtual TBubbleType BubbleType() const = 0;

	virtual void UpdateUnreadL( CBBFeedItem& aItem );

	class CJuikSizerContainer& BubbleContent() const;
 protected:
	CBubbledStreamItem(const TUiDelegates& aDelegates, TInt aFlags);
	virtual void BaseConstructL();
		
 protected: // from MUserPicObserver
	void UserPicChangedL( const TDesC& aNick, TBool aNew );


 protected: // own
	void UpdateBuddyPicL();
	void ZeroBaseL();
	void UpdateBaseL(CBBFeedItem& aItem);
		
	CGulIcon* GetIconL(TComponentName aName, TInt aId, TSize aSize, TRgb aRgb);
	CGulIcon* LoadIconL(TComponentName aName, TInt aId, TSize aSize, TRgb aRgb);
	


	void UpdateWithContentL( CBBFeedItem& aItem, CJuikLabel* aLabel );
	void UpdateJaiconL(CJuikImage& aJaicon, CBBFeedItem& aItem);
	CJuikImage* CreateJaiconL(CCoeControl& aParent);
	CJuikImage* CreateJaiconL();

 protected:
	void FocusChanged(TDrawNow aDrawNow);
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
	void Draw(const TRect& aRect) const;
	
 public: // from MFeedControl
	TUint Id() const { return iId; }

 protected:
	TUint iId;

	TRgb iServerColor;
	TRgb iLocalColor;

 protected:
	class CFocusableBuddy* iBuddyIcon;
	class CBubbleContainer* iBubbleContainer;
	class CBubbleRenderer* iBubbleRenderer;
	
	TBuf<1000> iWorkBuf;


	CGulIcon* iSeparatorIcon;
	CJuikImage* iSeparator;

	CJabberData::TNick iNick;
};


class CBubbleContainer : public CCoeControl, public MJuikControl
{
public:
	static CBubbleContainer* NewL(CBubbleRenderer& aBubbleRenderer, CBubbledStreamItem::TBubbleType aType,const TUiDelegates& aDelegates);
	
	virtual ~CBubbleContainer() {}

	virtual class CJuikSizerContainer& Content() const = 0;
	virtual void UpdateL( CBBFeedItem& aItem ) = 0;
	virtual void UpdateUnreadL( CBBFeedItem& aItem ) = 0;

	virtual const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }
};



enum  TFeedGfxId // feed graphic ids
	{
		EGfxPostBubble    = 300,
		EGfxFocusBubble   = 400,
		EGfxCommentBubble = 500,
		EGfxCommentIndicator = 1000,
		EGfxSeparator = 2000,
		EGfxNonFocusedButton = 1200,
		EGfxFocusedButton = 1300,
	};



#endif 
