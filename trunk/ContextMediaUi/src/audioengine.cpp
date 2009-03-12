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

#include "break.h"
#include "audioengine.h"
#include "app_context.h"
#include <MdaAudioSamplePlayer.h>
#include "mplayeruicontrollerlistener.h"

class CAudioEngineImpl: public CAudioEngine, public MMdaAudioPlayerCallback
{
public:
	static CAudioEngineImpl* NewL();
	static CAudioEngineImpl* NewLC();
	~CAudioEngineImpl();

	void DoStopL();
	void DoPlayL();
	void SetNewFileL(const TDesC& aFileName);
	void DoPauseL();

	TInt IncreaseVolume();
	TInt DecreaseVolume();
   
	void MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration);
	void MapcPlayComplete(TInt aError);

	void InitControllerL( MPlayerUIControllerListener* aCallback, TInt volume );
	void CloseController();
	TInt GetEngineState();
	enum TEngineState {EPNotInitialised, EPInitialising, EPStopped, EPPlaying, EPPaused};

private:
	CAudioEngineImpl();
	void ConstructL();

private:
	
	TEngineState iEngineState;
	MPlayerUIControllerListener* iListener;
	CMdaAudioPlayerUtility* iMdaPlayer;
	HBufC*	iMediaFile;
	TInt iVolume;

	friend class CAudioEngine;
};

EXPORT_C CAudioEngine* CAudioEngine::NewL()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("NewL"));

	CAudioEngineImpl* self = new (ELeave) CAudioEngineImpl();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
}

CAudioEngineImpl::CAudioEngineImpl()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("CAudioEngineImpl"));

}


CAudioEngineImpl::~CAudioEngineImpl()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("~CAudioEngineImpl"));

	delete iMediaFile;
	delete iMdaPlayer;    
	iMdaPlayer = NULL;
}


void CAudioEngineImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("ConstructL"));

	iMediaFile = HBufC::NewL(0);
	iEngineState = EPNotInitialised; 
}

void CAudioEngineImpl::SetNewFileL(const TDesC &aFileName)
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("SetNewFileL"));

	HBufC* newFile = aFileName.AllocL();
	delete iMediaFile;		 
	iMediaFile = newFile;
}

void CAudioEngineImpl::InitControllerL( MPlayerUIControllerListener* aCallback, TInt volume )   
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("InitControllerL"));

	iEngineState = EPInitialising;
	iListener = aCallback;
	iVolume = volume;
    
	delete iMdaPlayer;
	iMdaPlayer = NULL;
	{
		CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("InitControllerL1"));
		iMdaPlayer=CMdaAudioPlayerUtility::NewFilePlayerL(*iMediaFile,*this);
	}
	{
		CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("InitControllerL2"));
	}
}

void CAudioEngineImpl::DoPlayL()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("DoPlayL"));

	iMdaPlayer->Play();
	iEngineState = EPPlaying;
}


void CAudioEngineImpl::DoStopL()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("DoStopL"));

	iMdaPlayer->Stop();
	iEngineState = EPStopped;
	MapcPlayComplete(KErrNone);
}


void CAudioEngineImpl::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& /*aDuration*/)
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("MapcInitComplete"));

	if (aError == KErrNone) {
		iMdaPlayer->SetVolume(iVolume);
		if ( iListener ) {CC_TRAPD( ignore, iListener->InitControllerCompletedL( aError ) ); }
        } else {
		// acts as playing is over
		MapcPlayComplete(aError);
	}
}

void CAudioEngineImpl::DoPauseL()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("DoPauseL"));

	if ( iEngineState == EPPlaying ) {
		iMdaPlayer->Stop();
		iEngineState = EPPaused;
	}
}

void CAudioEngineImpl::CloseController()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("CloseController"));

	iListener=0;
	delete iMdaPlayer;
	iMdaPlayer = 0;
}

void CAudioEngineImpl::MapcPlayComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("MapcPlayComplete"));

	iListener->PlayCompletedL(aError);
	iEngineState = EPStopped;
}

TInt CAudioEngineImpl::GetEngineState()
{
	CALLSTACKITEM_N(_CL("CAudioEngineImpl"), _CL("GetEngineState"));

	return iEngineState;
}

TInt CAudioEngineImpl::IncreaseVolume()
{
	if ( iVolume< iMdaPlayer->MaxVolume()) {
		iMdaPlayer->SetVolume(++iVolume);
	}
	return iVolume;
}

TInt CAudioEngineImpl::DecreaseVolume()
{
	if ( iVolume> 0) {
		iMdaPlayer->SetVolume(--iVolume);
	}
	return iVolume;
}
