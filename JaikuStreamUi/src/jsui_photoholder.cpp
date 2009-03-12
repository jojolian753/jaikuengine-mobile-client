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

#include "jsui_photoholder.h"

#include "ccu_jaicons.h"
#include "ccu_feedfoundation.h"
#include "jsui_button.h"

#include "app_context.h"
#include "csd_feeditem.h"
#include "juik_animation.h"
#include "juik_fonts.h"
#include "juik_image.h"
#include "juik_keycodes.h"
#include "juik_label.h"
#include "juik_photo.h"

#include "jaiku_layout.h"
#include "juik_sizercontainer.h"
#include "cc_stringtools.h"
#include "ccu_launchbrowser.h"

#include <eiklabel.h>
#include <avkon.hrh>
#include <documenthandler.h>
#include <apmstd.h> 

// handles focus 
// image 



// class CDownloadAnimation : public CCoeControl, public MJuikControl , public CJuikAnimation::MAnimated 
// {
// public:
// 	static CDownloadAnimation* NewL(CCoeControl& aParent)
// 	{
// 		auto_ptr<CDownloadAnimation> self( new (ELeave) CDownloadAnimation );
// 		self->ConstructL(aParent);
// 		return self.release();
// 	}
		
// 	void Show()
// 	{
// 		if ( ! iIsShown )
// 			{
// 				iIsShown = ETrue;
// 				Start();
// 			}
// 	}

// 	void Hide()
// 	{
// // 		if ( iIsShown )
// // 			iIsShown = EFalse;
// // 		Stop();
// 	}

// 	TBool IsShown() const { return iIsShown; }
// protected:
// 	TBool iIsShown;
	
// 	CDownloadAnimation() : iIsShown(EFalse)  {}
// 	~CDownloadAnimation()
// 	{
// 		delete iAnimation;
// 	}


// 	void ConstructL(CCoeControl& aParent)
// 	{
// 		SetContainerWindowL( aParent );
// 		iAlpha = 0;
// 		iAnimation = CJuikAnimation::NewL();
// 	}
	
// 	TSize MinimumSize() 
// 	{
// 		if ( iIsShown ) 
// 			return TSize(10,10);
// 		else
// 			return TSize(0,0);
// 	}
	
// 	virtual void ProgressL(TReal aProgress) 
// 	{
// 		TInt maxV = 255; 
// 		TReal multiplier = iUpAlpha ? aProgress : 1.0 - aProgress;
// 		TInt c = (TInt)  (multiplier * maxV );
// 		iAlpha = Max(0, Min(c, 255));		
// 		DrawNow();
// 	}
	
// 	virtual void FinishedL()
// 	{
// 		if ( iIsShown )
// 			{
// 				iUpAlpha = !iUpAlpha;
// 				Start();
// 			}
// 	}

// 	void Start()
// 	{
// 		if ( iAnimation )
// 			{
// 				iAlpha = iUpAlpha ? 0 : 255;
// 				iAnimation->StartL( *this, TTimeIntervalMicroSeconds(1 * 500000) );
// 			}
// 	}
	
// 	void Stop()
// 	{
// 		if ( iAnimation )
// 			iAnimation->Stop();
// 	}


// 	void Draw( const TRect& aRect ) const
// 	{
// 		CWindowGc& gc = SystemGc();
// 		TRect r = Rect();
		
// 		TSize box(10,10); 
		
// 		TPoint p( r.iTl.iX + r.Width() / 2 - box.iWidth / 2, r.iTl.iY + r.Height() / 2 - box.iHeight / 2);
	   
// 		TRgb c( KRgbWhite );
// 		c.SetAlpha( iAlpha );

// 		gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
// 		gc.SetPenStyle( CGraphicsContext::ESolidPen );
// 		gc.SetBrushColor( c );
// 		gc.SetPenColor( c );
		
// 		gc.DrawEllipse( TRect(p, box) );
// 	}

// public: // from MJuikControl
 
// 	virtual const CCoeControl* CoeControl() const { return this; }
// 	virtual CCoeControl* CoeControl() { return this; }
// private:	
// 	TBool iUpAlpha;
// 	TInt iAlpha;
// 	CJuikAnimation* iAnimation;
// };


 
class CPhotoHolderImpl : public CPhotoHolder, 
						  public MFeedFoundation, 
						  public MFeedNotify, 
						  public CJuikPhoto::MListener
{
public:
	CPhotoHolderImpl(CFeedControl& aStreamControl, 
					 const TUiDelegates& aDelegates, 
					 TInt aFlags) : MFeedFoundation( aDelegates ), 
									iStreamControl(aStreamControl),
									iFlags(aFlags) {} 

	virtual ~CPhotoHolderImpl()
	{
		if ( iPhoto ) iPhoto->RemoveListener( *this );
		FeedStorage().UnSubscribeL(this);
		delete iDocHandler;
		iShownFile.Close();
	}

	void ConstructL(CCoeControl& aParent)
	{
		SetContainerWindowL( aParent );
		BaseConstructL();
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			RootContainer().SetRootSizerL( sizer );
		}

		TInt runningId = 1000;

		const TInt KMetaDataSizer = runningId++;
		const TInt KMetaTextSizer = runningId++;
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );	
			RootContainer().AddSizerL( sizer, KMetaDataSizer );
			RootContainer().GetRootSizerL()->AddL( *sizer, 1, Juik::EExpand );
		}
		
		{
			TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__jaicon);			
			TSize sz = TSize(40,40); //lay.Size();
			iStateIcon = CJuikImage::NewL( NULL, sz );
			RootContainer().AddControlL( iStateIcon, runningId++ );
			
			lay = Layout().GetLayoutItemL(LG_feed_controls_margins, LI_feed_controls_margins__jaicon);			
			iStateIcon->iMargin = lay.Margins();
			
			CGulIcon* icon = Jaicons().GetJaiconL( 106  ); // FIXME
			if ( icon )
				iStateIcon->UpdateL( *icon );
			
			RootContainer().GetSizerL( KMetaDataSizer )->AddL( *iStateIcon, 0, Juik::EExpandNot ); 
		}

		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );	
			RootContainer().AddSizerL( sizer, KMetaTextSizer );
			RootContainer().GetSizerL( KMetaDataSizer )->AddL( *sizer, 1, Juik::EExpand );
		}
		
		
		{
			iState = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
									   JuikFonts::Tiny(), RootContainer() );
			RootContainer().AddControlL( iState, runningId++ );
			iState->SetAlignment( EHCenterVCenter );
			iState->SetLabelAlignment( ELayoutAlignCenter );
			RootContainer().GetSizerL( KMetaTextSizer )->AddL( *iState, 0, Juik::EExpand ); 
		}

		{
			iDownloadButton = CJsuiButton::NewL( _L("Download image"), iDelegates );
			RootContainer().AddControlL( iDownloadButton, runningId++ );
			RootContainer().GetSizerL( KMetaTextSizer )->AddL( *iDownloadButton, 0, Juik::EExpandNot | Juik::EAlignCenterHorizontal ); 
		}


// 		{
// 			iDownloadAnimation = CDownloadAnimation::NewL( RootContainer() );
// 			RootContainer().AddControlL( iDownloadAnimation, runningId++ );
// 			RootContainer().GetSizerL( KMetaTextSizer )->AddL( *iDownloadAnimation, 0, Juik::EExpand ); 
// 		}
		
		
		{
			TSize sz = Layout().GetLayoutItemL(LG_feed_controls, 
											   LI_feed_controls__thumbnail).Size();
			if ( iFlags & FeedUi::EShowFullImage )
				{
					TSize screen = MJuikLayout::ScreenSize();
					if ( screen.iWidth >= 240 ||screen.iHeight >= 240 )
						sz = TSize(240,240);
					else
						sz = TSize(100,100);
				}
			iPhoto = CJuikPhoto::NewL( iContainer, sz, TSize(0,0) );
			RootContainer().AddControlL( iPhoto, runningId++ );
			iPhoto->SetBorderColor( KRgbWhite );
			if ( iFlags & FeedUi::EFocusInsideControl )
				{
					TRgb focusBgColor =  BubbleFocusColor();
					iPhoto->EnableFocusColor( focusBgColor );
				}
			RootContainer().GetRootSizerL()->AddL( *iPhoto, 0, Juik::EExpand ); 
			iPhoto->AddListenerL( *this );


		}
		
		
		FeedStorage().SubscribeL(this);		
	}
	
	virtual void FocusChanged(TDrawNow aDrawNow)
	{
		if ( iFlags & FeedUi::EFocusInsideControl )
			{
				iDownloadButton->SetFocus( IsFocused(), aDrawNow );
				iPhoto->SetFocus( IsFocused(), aDrawNow );
			}
		else
			CPhotoHolder::FocusChanged(aDrawNow);
	}

											

	void ShowInViewerL(const TDesC& aFileName) 
	{
 		if ( ! iDocHandler )
			iDocHandler = CDocumentHandler::NewL();

		

		_LIT8(KImageMime, "image/jpeg");
		TDataType mimeType(KImageMime);

		//iShownFile.Close();
		RFs fs;
		User::LeaveIfError( fs.Connect() );
		CleanupClosePushL( fs );
		User::LeaveIfError( fs.ShareProtected() );
		iShownFile.Open( fs, aFileName, EFileShareAny | EFileRead );
		CleanupClosePushL( iShownFile );
		
		TDataType empty;
		iDocHandler->OpenFileL(iShownFile, empty);
		//		iDocHandler->OpenFileL(iShownFile, mimeType);

// 		iDocHandler->OpenFileL(aFileName, empty);
		CleanupStack::PopAndDestroy( &iShownFile );
		CleanupStack::PopAndDestroy( &fs );
	}

	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
		if ( aType == EEventKey )
			{
				if ( aKeyEvent.iCode == JOY_CLICK )
					{
						// Download item
						if ( iFeedItem )
							{
								TInt state = iFeedItem->iMediaDownloadState();

								if ( ShowDownloadButton() )
									{
										FeedStorage().DownloadMediaForFeedItemL( iFeedItem, ETrue );
									}
								else
									{
//  										if ( iFeedItem->iMediaFileName().Length() > 0 )
//  											ShowInViewerL(iFeedItem->iMediaFileName());
//  										else
										LaunchBrowserL( iFeedItem->iLinkedUrl() );
									}
								return EKeyWasConsumed;
							}
					}
			}
		return EKeyWasNotConsumed;
	}
	

	TBool HasLoadingError(CBBFeedItem& aItem) const 
	{
		TInt state = aItem.iMediaDownloadState(); 
		return (state == CBBFeedItem::EMediaDownloaded && iPhoto && iPhoto->LoadingError().Length() > 0);
	}

	TBool UpdateDevStateL(CBBFeedItem& aItem) 
	{
		TInt state = aItem.iMediaDownloadState(); 

		if ( HasLoadingError( aItem )  )
			{
				TBuf<200> msg; 
				SafeAppend( msg, _L("Couldn't draw loaded image:\n") );
				SafeAppend( msg, iPhoto->LoadingError() );
				iState->UpdateTextL( msg );
				return ETrue;
			}
		
			
		if (state == CBBFeedItem::ENotDownloading ||
			state == CBBFeedItem::EMediaDownloaded )
			{
				iState->ZeroL();
				return ETrue;
			}
		
		
 		TBuf<50> n;	
		switch ( state )
			{
			case CBBFeedItem::ENoMedia:               n = _L("No media"); break;
			case CBBFeedItem::ENotDownloading:        n = _L("Click to download"); break; // NOT USED 
			case CBBFeedItem::EQueued:                n = _L("Queued"); break;
			case CBBFeedItem::EDownloading:           n = _L("Downloading"); break;
			case CBBFeedItem::EMediaDownloaded:       n = _L("Downloaded"); break; // NOT USED 
			case CBBFeedItem::EDownloadErrorRetrying: n = _L("Download error. Retrying"); break;
			case CBBFeedItem::EDownloadErrorFailed:   n = _L("Download failed"); break;
			case CBBFeedItem::EDownloadPausedOffline: n = _L("Download paused: Offline"); break;
			case CBBFeedItem::EDownloadPausedNotConnecting: n = _L("Download paused: Not connecting"); break;
			default:                                  n = _L("Unknown"); break;
			}

		TBool hasError = 
			state == CBBFeedItem::EDownloadErrorRetrying ||
			state == CBBFeedItem::EDownloadErrorFailed;
		
		if ( aItem.iErrorInfo )
			{
				TBuf<200> msg;
				msg.Append(n);
				msg.Append( _L(":\n") );
				//msg.Append( _L(": ") );
				CBBString* s = aItem.iErrorInfo->iUserMsg;
				if ( s )
					SafeAppend(msg, s->Value() );
				else
					SafeAppend(msg, _L("Unknown error"));
				
				// #ifdef __DEV__
				// User and technical messages are equivalent
				// 				s = aItem.iErrorInfo->iTechnicalMsg;
				// 				if ( s )
				// 					{
				// 						SafeAppend(msg, _L("\nDEV:"));
				// 						SafeAppend(msg, s->Value() );
				// 					}
				// #endif // __DEV__
				iState->UpdateTextL(msg);
				return ETrue;
			}
		else
			{
				iState->UpdateTextL(n);
				return EFalse;
			}
	}
	
	TBool ShowDownloadButton() const
	{
		if ( iFeedItem )
			{
				TBool hasUrl = iFeedItem->iThumbnailUrl().Length() > 0;
				
				TInt state = iFeedItem->iMediaDownloadState();
				return state == CBBFeedItem::ENotDownloading 
					|| state == CBBFeedItem::EDownloadErrorFailed 
					|| (hasUrl && state == CBBFeedItem::ENoMedia);
			}
		else
			return EFalse;
	}


	void ZeroL()
	{
	}


	void UpdateL(CBBFeedItem& aItem)
	{
		
		//ZeroL(); // this is plain wrong, because these call CJuikLabel::ZeroL which setsize to 0,0.
		// #ifdef __DEV__
// 		{
// 			TInt id = aItem.iLocalDatabaseId;
// 			BubbleContent().SetDebugId( 20000 + id );
// 			iPost->SetDebugId( 30000 + id );
// 			iMetadata->SetDebugId( 40000 + id );
// 		}
// #endif

// 		UpdateBaseL( aItem );
// 		SetItem( &aItem );
		if ( iFeedItem != &aItem )
			{
				iFeedItem = &aItem; 
			}
		
		CFeedItemStorage::TDownloadMode dlMode = FeedStorage().DownloadMode();
		if ( dlMode == CFeedItemStorage::EOnLookDL || dlMode == CFeedItemStorage::EAutomaticDL )
			{
				if ( aItem.iMediaDownloadState() == CBBFeedItem::ENotDownloading 
					 || aItem.iMediaDownloadState() == CBBFeedItem::EQueued )
					FeedStorage().DownloadMediaForFeedItemL( &aItem );
			}
		
		

		if ( UpdateControlsL() )
			{
				iStreamControl.MinimumSizeChangedL();
			}
	}

	TBool HasPhoto() const
	{
		return iFeedItem->iMediaFileName().Length() > 0;
	}

	TBool UpdateControlsL()
	{
		TBool minSizeChanged = EFalse;
		
		if ( ! iFeedItem )
			return EFalse;
		
		// Photo
		TBool hasPhoto = HasPhoto();
		
		if ( hasPhoto  )
			{
				iPhoto->SetFileL( iFeedItem->iMediaFileName() );
				minSizeChanged = ETrue;
			}

		// Download button
		TBool hide = ! ShowDownloadButton();
		if ( iDownloadButton->IsHidden() != hide )
			{
				iDownloadButton->SetHidden( hide );
				minSizeChanged = ETrue;
			}

		if ( ShowDownloadButton() )
			{
				if ( iFeedItem->iMediaDownloadState() == CBBFeedItem::EDownloadErrorFailed )
					iDownloadButton->SetTextL( _L("Try again") );
				else
					iDownloadButton->SetTextL( _L("Download image") );
			}

		
		// Download animation
//  		TInt state = iFeedItem->iMediaDownloadState();
//  		TBool run = (state == CBBFeedItem::EQueued || 
//  					 state == CBBFeedItem::EDownloading ); 

// 		iDownloadAnimation->Show();
// //  		if ( iDownloadAnimation->IsShown() != run )
// // 			{
// // 				if ( run ) iDownloadAnimation->Show();
// //  				else       iDownloadAnimation->Hide();
// //  				minSizeChanged = ETrue;
// //  			}
		
		// Label
		minSizeChanged |= UpdateDevStateL( *iFeedItem );

		if ( hasPhoto )
			{
				iStateIcon->ClearL();
			}
		
		return minSizeChanged;
	}

	void Draw(const TRect& aRect) const
	{
		TInt i = 0;
		CWindowGc& gc = SystemGc();
		CPhotoHolder::Draw(aRect);
	}


	void FeedItemEvent(CBBFeedItem* aItem, TEvent aEvent)
	{

		if ( iFeedItem && iFeedItem == aItem )
			{
				switch ( aEvent )
					{
					case MFeedNotify::EMediaDownloadStateChanged:
						{
							if ( UpdateControlsL() )
								iStreamControl.MinimumSizeChangedL();
							else
								iStreamControl.DrawDeferred();
							break;
						}
					}
			}
	}


	void AuthorCountEvent(const TDesC& /*aAuthor*/,
						  TInt /*aNewItemCount*/, 
						  TInt /*aNewUnreadCount*/) 
	{
	}
	


private:	// from CJuikPhoto::MListener
	
	void MediaLoaded(TBool aSuccess)
	{
		if ( ! aSuccess )
			{
				UpdateControlsL();
			}
		iStreamControl.MinimumSizeChangedL();
		//DrawNow(); //Deferred();
	}

	
public: // from MJuikControl
 
	virtual const CCoeControl* CoeControl() const { return this; }
	virtual CCoeControl* CoeControl() { return this; }

private:
	CFeedControl& iStreamControl;
	TInt iFlags;

// 	CDownloadAnimation* iDownloadAnimation;
	CJsuiButton* iDownloadButton;
	CJuikPhoto* iPhoto;
	CJuikLabel* iState;
	CJuikImage* iStateIcon;
	
	CBBFeedItem* iFeedItem;


	TMargins8 iMargin;

	CDocumentHandler* iDocHandler;

	RFile iShownFile;
};


CPhotoHolder* CPhotoHolder::NewL(CFeedControl& aStreamControl, const TUiDelegates& aDelegates, TInt aFlags, CCoeControl& aParent)
{
	auto_ptr<CPhotoHolderImpl> self( new (ELeave) CPhotoHolderImpl(aStreamControl, aDelegates, aFlags) );
	self->ConstructL(aParent);
	return self.release();
}
