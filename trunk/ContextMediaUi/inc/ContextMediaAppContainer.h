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

#ifndef CONTEXTMEDIACONTAINER_H
#define CONTEXTMEDIACONTAINER_H

#include <coecntrl.h>
#include <aknlists.h> 
#include "cm_storage.h"
#include <akniconarray.h>
#include <aknview.h>
#include <eikrted.h>
#include "cm_network.h"
#include "medialistbox.h"
#include "videoengine.h"
#include "audioengine.h"
#include "timeout.h"
#include <aknwaitdialog.h>
#include "reporting.h"
#include "app_context.h"
#include "mplayeruicontrollerlistener.h"
#include <eikedwin.h>

#include "juik_photo.h"

#ifndef __S60V2__
#include <PAlbImageViewerBasic.h>
#endif

IMPORT_C TInt LoadCmuiResourceL();

class MContextMediaAppUi {
public:
	virtual TUid LastViewBeforePrompt() = 0;
	virtual TVwsViewId NextView() = 0;
	virtual void SetNextView(TVwsViewId aViewId) = 0;
	virtual void SetTab(TInt tabId) = 0;
};


class OurOwnRichTextEditor;

class CContextMediaAppGeneralContainer : public CCoeControl, public MContextBase {
public: 
	CContextMediaAppGeneralContainer(CCMNetwork &aCMNetwork, CPostStorage &aStorage, TInt64 aNode, 
					CPostStorage::TSortBy aSort, CPostStorage::TOrder aOrder, CAknView * aView);
	virtual void ConstructL(const TRect& aRect) = 0;
        virtual ~CContextMediaAppGeneralContainer();

public:
	virtual TInt get_current_idx()=0;
	virtual TInt get_top_idx()=0;
	virtual void set_current_idx(TInt idx)=0;
       	virtual void set_top_idx(TInt idx)=0;
	virtual TInt get_item_count()=0;

public:
	virtual CCMPost * get_current_post()=0;
	void mark_as_read();
	virtual TBool IsCurrentPlayableMedia()=0;
	virtual TBool IsLoadingMedia()=0;
	virtual TBool CloseCurrentMedia()=0;
	
protected:
	void BaseConstructL();
	void GetVideoMbmPath(TFileName &aFilename);
	void GetAudioMbmPath(TFileName &aFilename);
	void GetUnknownMbmPath(TFileName &aFilename);

	void CtrlSetRect(CCoeControl* aControl, const TRect& aRect) const;
	void CtrlSetSize(CCoeControl* aControl, const TSize& aSize) const;
	void CtrlSetExtent(CCoeControl* aControl, const TPoint& aTl, const TSize& aSize) const;
	void CtrlSetPosition(CCoeControl* aControl, const TPoint& aTl) const;
	void ScaleRect(TRect& aTo, const TRect& aFrom) const;
	void ScaleRect(TSize& aTo, const TSize& aFrom) const;
	const CFont* Latin12();
	friend class OurOwnRichTextEditor;

	TFileName iVideoMbmPath;
	TFileName iAudioMbmPath;
	TFileName iUnknownMbmPath;

	CPostStorage &iStorage;
	CCMNetwork &iCMNetwork;
	TInt64 iNode;
	CPostStorage::TSortBy iSort;
	CPostStorage::TOrder iOrder;
	CAknView * iView;
	CFont*	iLatin12;
};

class CContextMediaAppListboxContainer : public CContextMediaAppGeneralContainer,
	public MEikListBoxObserver, public MPostNotify, public MTimeOut/*, public MBusyIndicator*/ {
public: 
	IMPORT_C CContextMediaAppListboxContainer(CCMNetwork &aCMNetwork, CPostStorage &aStorage, TInt64 aNode, 
		CPostStorage::TSortBy aSort, CPostStorage::TOrder aOrder, CAknView * aView,
		TBool aStandAlone=EFalse);
	virtual void ConstructL(const TRect &aRect);
        virtual ~CContextMediaAppListboxContainer();
private:
	void SizeChanged();
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);

private: 
	void HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType);
	virtual void PostEvent(CCMPost* aParent, CCMPost* aChild, TEvent aEvent);

private: 
	void expired(CBase * Source);

public:
	virtual TInt get_current_idx();
	virtual TInt get_top_idx();
	virtual void set_current_idx(TInt idx);
       	virtual void set_top_idx(TInt idx);
	virtual TInt get_item_count();
	virtual CCMPost * get_current_post();
	virtual TBool CloseCurrentMedia() { return ETrue; }

public:
	virtual TBool IsCurrentPlayableMedia();
	virtual TBool IsLoadingMedia();

private:
	CContextMediaBox * iListbox;
	CAknIconArray * iIconArray;
	CContextMediaArray * iMediaTextArray;

	CTimeOut * iTimer;
	
	HBufC * iNoTitleBuf;
	HBufC * iNoItemBuf;
	HBufC * iNoVisibleItemBuf;
	HBufC * iNewThreadBuf;
	
	CEikLabel * iNoItemLabel;
	TBool	iStandAlone;
#ifdef __S60V2__
	class CAknsBasicBackgroundControlContext *iBackground;
#endif
};

class CContextMediaAppPostContainer 
	: public CContextMediaAppGeneralContainer, 
	  public MPlayerUIControllerListener,
	  public MPostNotify, 
	  public MTimeOut, 
	  public MEikEdwinObserver,
	  public CJuikPhoto::MListener
{
public: 
	IMPORT_C CContextMediaAppPostContainer(CCMNetwork &aCMNetwork, CPostStorage &aStorage, TInt64 aNode, 
		CPostStorage::TSortBy aSort, CPostStorage::TOrder aOrder, CAknView * aView, 
		TBool aStandAlone=EFalse, class CTagStorage* aTagStorage=0, 
		const TDesC& aFileName=KNullDesC, MBBDataFactory * aFactory=0,
		class CAknIconArray * iTagIcons=0, class MAskForNames* aAskForNames=0);

	virtual void ConstructL(const TRect &aRect);
        virtual ~CContextMediaAppPostContainer();

protected: // from CJuikPhoto::MListener
	virtual void MediaLoaded(TBool aMediaReady);

private:
	void SizeChanged();
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
	void RestoreOriginalCBAL();
	TBool DisplayPicture();
#ifndef __S60V2__
	void DisplayPictureInnerL(TInt& aLive, class CPAlbImageViewerBasic* & localMediaDisplay);
#else
	void DisplayPictureInnerL();
#endif

	TBool iAutoTagsIsVisible;
	void AnimateAutoTags(TBool aIsVisible, TBool aWithBackground=EFalse);
	void ShowAutoTags(TBool aDraw, const TRect& aRect);
	void DrawMediaDisplay();
	void HandleEdwinEventL(CEikEdwin* /*aEdwin*/, TEdwinEvent /*aEventType*/);
	void SetCountTextL();
public:
	virtual TInt get_current_idx();
	virtual TInt get_top_idx();
	virtual void set_current_idx(TInt idx);
       	virtual void set_top_idx(TInt idx);
	virtual CCMPost * get_current_post();
	virtual TInt get_item_count();
	virtual TBool CloseCurrentMedia();
public:
	virtual TBool IsCurrentPlayableMedia();
	virtual TBool IsLoadingMedia();
	IMPORT_C void Play();
	IMPORT_C void Pause();
	IMPORT_C void Resume();
	IMPORT_C void Stop();

private:
	void expired(CBase*source);

private: // PRIVATE MEDIA FUNCTIONS
	void PlayMediaFileL(const TDesC& aFilename);
	void PauseMediaFileL();
	void StopMediaFileL();
	void ResumeMediaFileL();

private: // video callbacks
	void InitControllerCompletedL(TInt aError);
        void PlayCompletedL(TInt aError);
	void ShowVolume(TInt volume);

private: // post callback
	void PostEvent(CCMPost* aParent, CCMPost* aChild, TEvent aEvent);

public: // Browsing related
	TBool has_next();
	TBool has_previous();   
	void display_next(TInt aRepeats=0);
	void display_previous(TInt aRepeats=0);
	void display_current(TInt aError=KErrNone);

private: // focus related
	void FocusPrevious();
	void FocusNext();


public:
	void save_contribution_to_post(TBool aConfirmed);

public:
	enum TMediaFileType {EUnknown, EImage, EAudio, EVideo};
	TMediaFileType iMediaFileType;

	virtual void SetPresence(CBBPresence* aPresence); // takes ownership

private: 
	TInt acceleration(TInt aRepeats);
	void ShowHintByControl(CCoeControl* aCurrentControl);

	/////////////////////////// 
	// UI controls
	//  


	// ??? Scrollbar, but where it's shown ?
	TEikScrollBarModel iModel;
	CEikScrollBarFrame * iSBFrame;
	TInt iMaxScrollPos;
	TInt iCurrentScrollPos;

	// ??? 
	CEikLabel* iLoadingLabel;

	// Shows errors on top of images
	CEikLabel* iErrorLabel;

	// Edit or show comment below image 
	OurOwnRichTextEditor * iComment;

	// Edit or show tags below image
	OurOwnRichTextEditor * iTags;

	// ??? 
	OurOwnRichTextEditor * iSignature;

	// Arrows in left and right edges of screen 
	class CBorderedContainer* iLeftArrow;
	class CBorderedContainer* iRightArrow;

	// Time or date of image shown bottm right of image
	class CBorderedContainer* iTimeBox;
	CEikLabel* iTimeLabel; // owned by box

	// Text showing image number and count and set that it belons to. Top Right of image
	class CBorderedContainer* iCountBox;
	CEikLabel* iCountLabel; // owned by box
	
	// ??? 
	CEikLabel * iVolumeIndicator;

	// Header when "select tags" dialog is visible. Could be replaced with platform component?
	CEikLabel * iSelectTagLabel;
	
	// Notification icons
	class CNotifyWindowControl* iNotifyControl;

	/////////////////////////// 
	// Non-UI controls
	//  

	CTimeOut * iVolumeDisplayTimeOut;


	HBufC * iLoadingBuf;
	HBufC * iDownloadingBuf;
	HBufC * iDefaultSignatureBuf;
	HBufC * iDefaultTagsBuf;
	HBufC * iDefaultCommentBuf;

	TBool iMediaReady;   // whether media is ready (to be displayed and/or played)
	TBool iLoadingMedia; // whether we're opening the media file for display
	TBool iPlayingMedia; // whether we currently are playing media
	TBool iMediaBrowsingOn; //in captured media mode, whether we can go left&right

/* #ifndef __S60V2__ */
/* 	CPAlbImageViewerBasic * iMediaDisplay; */
/* #else */
/* 	class CJpegView*	iMediaDisplay; */
/* #endif */
	CJuikPhoto* iMediaDisplay;

	CArrayFixFlat<TUint>* iPostArray;
	CCMPost	*iPost, *iParentPost; 
	TInt iLoadError; TBuf<100> iErrorMessage;
	TInt iCurrentPostIndex;

	CVideoEngine * iVideoEngine;
	CAudioEngine * iAudioEngine;
	CTimeOut * iDisplayTimeOut; TBool iCurrentChanged;

	TFileName iFileName;

	CArrayPtrFlat< CCoeControl > * iControls;
	TBool	iStandAlone;

	MBBDataFactory * iFactory;
	class CTagStorage* iTagStorage;
	TInt	*iLive;

	class CAutoTagArray*	iAutoTagArray;
	class CTimeOut*		iAutoTimer;
	class CBBPresence*	iPresence;
	TInt	iWaitCount;
	class CAknIconArray * iTagIcons;
	TBool	iGoneUpFromAutoTags;
	MAskForNames* iAskForNames;

	TSize KPictureSize;
	TSize KPictureSizeStandAlone;

	TRect KPictureRectangle;
	TRect KPictureRectangleStandAlone;
	TRect KVideoRectangle;
	TRect KVideoRectangle2;

	TBool iInPress; TInt iRepeats;

	////////////////////////
	// Extra UI controls
	
	// List that pops up when you move up from "This is public" label"
	class CAutoTagListBox*	iAutoTags;
	
	class CHintBox* iHints;
};

#endif
