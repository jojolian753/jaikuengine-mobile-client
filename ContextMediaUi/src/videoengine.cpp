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

#include "videoengine.h"

#ifdef __S60V2__

#include "break.h"

#ifndef __S60V3__
#include <mmferrors.h>
#endif

#include "MPlayerUIControllerListener.h"
#include "app_context.h"
#include <flogger.h>

#include <VideoPlayer.h>
#include "compat_int64.h"

class CVideoFileDetails : public CBase
{
public:
        CVideoFileDetails();
        virtual ~CVideoFileDetails();

private:    
        HBufC* iTitle;
        HBufC* iFileName;
        HBufC* iUrl;
        HBufC* iFormat;
        HBufC* iCopyright;
        TInt   iResolutionWidth;
        TInt   iResolutionHeight;
        TInt   iBitrate;
        TInt   iSize;
        TTime  iTime;
        TInt64 iDurationInSeconds;
        TBool  iAudioTrack;
        TBool  iVideoTrack;
        
public:   
	friend class CVideoEngineImpl;
};

class CVideoEngineImpl : public CVideoEngine, public MVideoPlayerUtilityObserver, public MBeating
{
public:
	static CVideoEngineImpl* NewL();

private:
        CVideoEngineImpl( );
	void ConstructL();

public:
	~CVideoEngineImpl();

public:  
	void DoStopL();
	void DoPlayL();
	void SetNewFileL(const TDesC& aFileName);
	void DoPauseL();
	TInt GetEngineState();

	TInt IncreaseVolume();
	TInt DecreaseVolume();

        void InitControllerL( class MPlayerUIControllerListener* aCallback,
                              RWsSession& aWs,
                              CWsScreenDevice& aScreenDevice,
                              RWindowBase& aWindow,
                              TRect& aScreenRect,
                              TRect& aClipRect,
			      TInt volume);  
	void CloseController();
public: 
        void Beat();
        void Synchronize();

public: 
        void MvpuoOpenComplete(TInt aError);
        void MvpuoPrepareComplete(TInt aError);
        void MvpuoFrameReady(CFbsBitmap& aFrame,TInt aError);
        void MvpuoPlayComplete(TInt aError);
        void MvpuoEvent(const TMMFEvent& aEvent);
	
private:
	CVideoPlayerUtility*	iPlayer;
	HBufC*	iMediaFile;
	CVideoFileDetails* iFileDetails;
	TEngineState iEngineState;
	MPlayerUIControllerListener* iListener;
	CHeartbeat* iProgressUpdater;  
	TInt64 iPlayPosition;
	TInt iVolume;
	TRect iScreenRect;

	friend class CVideoEngine;
};

CVideoFileDetails::CVideoFileDetails()
{
	CALLSTACKITEM_N(_CL("CVideoFileDetails"), _CL("CVideoFileDetails"));

}

CVideoFileDetails::~CVideoFileDetails()
{
	CALLSTACKITEM_N(_CL("CVideoFileDetails"), _CL("~CVideoFileDetails"));

	delete iTitle;
	delete iFileName;
	delete iUrl;
	delete iFormat;
	delete iCopyright;
}

EXPORT_C CVideoEngine* CVideoEngine::NewL( )
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("NewL"));

	CVideoEngineImpl* self = new (ELeave) CVideoEngineImpl( );
	CleanupStack::PushL( self );
	self->ConstructL( );
	CleanupStack::Pop();
	return self;
}

CVideoEngineImpl::CVideoEngineImpl()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("CVideoEngineImpl"));

}

void CVideoEngineImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("ConstructL"));

	iMediaFile = HBufC::NewL(0);
	iEngineState = EPNotInitialised; 
	iFileDetails = new (ELeave) CVideoFileDetails();
	iProgressUpdater = CHeartbeat::NewL(0);
}

    
CVideoEngineImpl::~CVideoEngineImpl()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("~CVideoEngineImpl"));

	delete iMediaFile;
	delete iFileDetails;
	delete iProgressUpdater;
}

void CVideoEngineImpl::SetNewFileL(const TDesC &aFileName)
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("SetNewFileL"));

	HBufC* newFile = aFileName.AllocL();
	delete iMediaFile;		 
	iMediaFile = newFile;
}

void CVideoEngineImpl::DoPlayL()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("DoPlayL"));

	if ( !iProgressUpdater->IsActive()) {
		iProgressUpdater->Start(ETwelveOClock,this); 
        }
	iPlayer->Play();
	switch (iEngineState){
		case EPStopped: {
			iPlayPosition = 0;
			//iListener->PlaybackPositionChangedL(iPlayPosition,iFileDetails->iDurationInSeconds);
			break;
		}
		case EPPaused:
			//iListener->PlaybackPositionChangedL(iPlayPosition,iFileDetails->iDurationInSeconds);
			break;
		default:
			break;
        }

	iEngineState = EPPlaying;
}

void CVideoEngineImpl::DoStopL()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("DoStopL"));

	if ( iEngineState != EPPlaying && iEngineState != EPPaused ) return;
	if ( iProgressUpdater->IsActive()) iProgressUpdater->Cancel();
        
	iPlayPosition = 0;
	iPlayer->Stop();
	iPlayer->Close();
	iEngineState = EPStopped;
	MvpuoPlayComplete(KErrNone);

}

void CVideoEngineImpl::MvpuoOpenComplete(TInt aError )
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("MvpuoOpenComplete"));

	if (aError == KErrNone) {
		iPlayer->Prepare();
	} else {
		// acts as playing is over
		MvpuoPlayComplete(aError);
	}
}

void CVideoEngineImpl::MvpuoFrameReady(CFbsBitmap& /*aFrame*/,TInt /*aError*/)
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("MvpuoFrameReady"));

	// no impl
}
 
void CVideoEngineImpl::MvpuoPlayComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("MvpuoPlayComplete"));

	if (iProgressUpdater->IsActive()) iProgressUpdater->Cancel();
        
	iPlayPosition = 0;
	iListener->PlayCompletedL(aError);
	iEngineState = EPStopped;
	iPlayer->Close();
}

void CVideoEngineImpl::MvpuoPrepareComplete(TInt aError )
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("MvpuoPrepareComplete"));

	TInt ret = aError;
 
	iEngineState = EPStopped;
	iFileDetails->iDurationInSeconds = 0;

#ifndef __S60V3__
	if ((ret == KErrNone )||(ret == KErrMMPartialPlayback )) {
#else
	if (ret == KErrNone) {
#endif
		TSize size( 0, 0 );
		iPlayer->VideoFrameSizeL(size);
		iFileDetails->iResolutionHeight = size.iHeight;
		iFileDetails->iResolutionWidth = size.iWidth;
		iFileDetails->iAudioTrack = iPlayer->AudioEnabledL();   
		if (iFileDetails->iAudioTrack) {
			TRAPD(ignore, iPlayer->SetVolumeL(iVolume) );
		}
		iFileDetails->iVideoTrack = iPlayer->VideoBitRateL();	            
		iFileDetails->iDurationInSeconds = iPlayer->DurationL().Int64() / KMPOneSecond; 
	} else {
		iListener->PlayCompletedL(aError);
		return;
	}

	if ( iListener ) {
		CC_TRAPD( ignore,iListener->InitControllerCompletedL( ret ) );
        }
}

void CVideoEngineImpl::MvpuoEvent(const TMMFEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("MvpuoEvent"));

	if (aEvent.iEventType == KMMFEventCategoryVideoPlayerGeneralError &&
        aEvent.iErrorCode == KErrHardwareNotAvailable) {
		CC_TRAPD(ignore,iListener->PlayCompletedL(KErrAudioLost));    
        }
}
 
void CVideoEngineImpl::InitControllerL( MPlayerUIControllerListener* aCallback,
                                    RWsSession& aWs,
                                    CWsScreenDevice& aScreenDevice,
                                    RWindowBase& aWindow,
                                    TRect& aScreenRect,
                                    TRect& aClipRect,
				    TInt volume)   
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("InitControllerL"));

	iVolume = volume*10;
	iEngineState = EPInitialising;
	iListener = aCallback;
    
	delete iPlayer;
	iPlayer = NULL;
	iPlayer = CVideoPlayerUtility::NewL( *this, EMdaPriorityNormal, 
		                             EMdaPriorityPreferenceNone, aWs, 
						aScreenDevice,
						aWindow,
						aScreenRect, 
						aClipRect 
						);
	this->iScreenRect = aScreenRect;
	iPlayer->OpenFileL( iMediaFile->Des() );
}

void CVideoEngineImpl::Beat()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("Beat"));

	// keep backlights on, if clip has video
	if ( iFileDetails->iVideoTrack ) {
		User::ResetInactivityTime();
        }
	if ( iEngineState != EPPlaying ) return;

	TInt64 ret = iPlayPosition%2;
    
	if ( I64INT(ret) ) {
		iPlayPosition = iPlayPosition + 1;
		if (iListener) {
			//CC_TRAPD(ignore,iListener->PlaybackPositionChangedL(iPlayPosition,iFileDetails->iDurationInSeconds));
		}
        } else {
		Synchronize();
        }
}


void CVideoEngineImpl::Synchronize()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("Synchronize"));

	if ( iEngineState != EPPlaying ) return;
	
	// if live stream, fake progress
	CC_TRAPD(ignore, 
		iPlayPosition = iPlayer->PositionL().Int64() / KMPOneSecond;
		if (iListener){
			//iListener->PlaybackPositionChangedL(iPlayPosition,iFileDetails->iDurationInSeconds);
		}
        );
}

TInt CVideoEngineImpl::GetEngineState()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("GetEngineState"));

	return iEngineState;
}

void CVideoEngineImpl::DoPauseL()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("DoPauseL"));

	if ( iEngineState == EPPlaying ) {
		if (iProgressUpdater->IsActive()) {
			iProgressUpdater->Cancel();
		}
		iPlayer->PauseL();
		iEngineState = EPPaused;
	}
}

void CVideoEngineImpl::CloseController()
{
	CALLSTACKITEM_N(_CL("CVideoEngineImpl"), _CL("CloseController"));

	iListener=0;
	if (iPlayer) iPlayer->Close();
	delete iPlayer;
	iPlayer = 0;
}

TInt CVideoEngineImpl::IncreaseVolume()
{
	if (!iFileDetails->iAudioTrack) return 0;
	TInt vol = iPlayer->Volume();

	if (vol+10 <= iPlayer->MaxVolume()) {
		TInt vol_temp= vol+10;
		CC_TRAPD(err, iPlayer->SetVolumeL(vol_temp));
		if (err!=KErrNone) {
			return vol/10;
		} else {
			return vol_temp/10;
		}
	} else {
		return vol/10;
	}
}

TInt CVideoEngineImpl::DecreaseVolume()
{
	if (!iFileDetails->iAudioTrack) return 0;
        TInt vol = iPlayer->Volume();

	if (vol-10 >= 0) {
		TInt vol_temp= vol-10;
		CC_TRAPD(err, iPlayer->SetVolumeL(vol_temp));
		if (err!=KErrNone) {
			return vol/10;
		} else {
			return vol_temp/10;
		}
	} else {
		return vol/10;
	}
}

#else
EXPORT_C CVideoEngine* CVideoEngine::NewL( )
{
	return 0;
}
#endif
