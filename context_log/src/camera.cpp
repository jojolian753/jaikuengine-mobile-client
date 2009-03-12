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
#include "camera.h"

#ifndef __S60V2__
#include <cameraserv.h>
#else
#include <ecam.h>
#endif

#include "list.h"
#include "symbian_auto_ptr.h"
#include <bautils.h>
#include <MdaImageConverter.h>
#include "mda_recorder.h"
#include "timeout.h"
#include <aknsoundsystem.h> 
#include "contextlog_resource.h"
#include <aknappui.h>
#include "cl_settings.h"

/*
 * Concepts:
 * !Taking a photo!
 */

class CSnapShotImpl : public CSnapShot, public CCheckedActive, public MContextBase, public MMdaImageUtilObserver, 
	public MTimeOut, public Mrecorder_callback, public MSettingListener
#ifdef __S60V2__
	, public MCameraObserver
#endif
	{
private:
	CSnapShotImpl(MApp_context& Context);
	void ConstructL();
	static CSnapShot* NewL(MApp_context& Context);
	virtual void TakeSnapShot(const TDesC& aDir, MSnapShotNofifier* aNotifier);
	virtual void CancelSnapShot(MSnapShotNofifier* aNotifier);

	virtual void CheckedRunL();
	virtual TInt CheckedRunError(TInt aCode);
	virtual void DoCancel();

	void Next(bool ok);
	void Async(TInt aError=KErrNone);
	void CreateSaveImage();
	void MakeSaveImageName();
	void CloseCamera();
	void CloseRecorder();
	void Convert();
	void Retry();
	void NotifyTaken();

	virtual void MiuoConvertComplete(TInt aError);
	virtual void MiuoCreateComplete(TInt aError);
	virtual void MiuoOpenComplete(TInt aError);

	virtual void expired(CBase* Source);

#ifdef __S60V2__
	virtual void ReserveComplete(TInt aError);
	virtual void PowerOnComplete(TInt aError);
	virtual void ViewFinderFrameReady(CFbsBitmap& aFrame);
	virtual void ImageReady(CFbsBitmap* aBitmap, HBufC8* aData, TInt aError);
	virtual void FrameBufferReady(MFrameBuffer* aFrameBuffer, TInt aError);
#endif

	virtual void stopped(bool reset=false);
	virtual void error(const TDesC& descr);
	virtual void opened();

	virtual void SettingChanged(TInt /*Setting*/);
public:
	~CSnapShotImpl();

private:
	struct TSnapShotRequest {
		TFileName	iDir;
		MSnapShotNofifier* iNotifier;
		bool recorded, taken_picture;

		TSnapShotRequest(const TDesC& aDir, MSnapShotNofifier* aNotifier) :
		iDir(aDir), iNotifier(aNotifier), recorded(false), taken_picture(false) { }
		TSnapShotRequest() : iNotifier(0), recorded(false), taken_picture(false) { }
	};

	CList<TSnapShotRequest>* iRequests;
	
	enum TState { EIdle, EStarting, ERecording, EReserving, ETurningOn, EGettingImage, ECreatingFile, EConverting };
	TState iState;
#ifndef __S60V2__
	RCameraServ iCamera; 
#else
	CCamera*	iCamera;
#endif

	bool iCameraIsOpen; bool iCameraIsOn;
	CFbsBitmap* iBitMap;
	MSnapShotNofifier* iNotifier;
	CMdaImageBitmapToFileUtility* iFileSaver;
	TFileName	iFileName;
	TMdaJfifClipFormat jfifFormat;
	TInt	retries;
	CMda_recorder* iRecorder;
	bool		iRecordError;
	CAknKeySoundSystem* iSoundSystem;
	TInt		iRecRetries;
	TTime		iRecStarted;
	TInt		iRecordTime;
	
	friend class CSnapShot;
};

CSnapShot* CSnapShot::NewL(MApp_context& Context)
{
	CALLSTACKITEM_N(_CL("CSnapShot"), _CL("NewL"));


	auto_ptr<CSnapShotImpl> ret(new (ELeave) CSnapShotImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CSnapShotImpl::CSnapShotImpl(MApp_context& Context) : MContextBase(Context), CCheckedActive(EPriorityNormal, _L("CSnapShotImpl"))
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("CSnapShotImpl"));


}

void CSnapShotImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("ConstructL"));


	Settings().GetSettingL(SETTING_RECORD_TIME, iRecordTime);
	Settings().NotifyOnChange(SETTING_RECORD_TIME, this);

	iSoundSystem = (STATIC_CAST(CAknAppUi*,
			      CEikonEnv::Static()->AppUi()))->KeySounds();
	CC_TRAPD(error, iSoundSystem->AddAppSoundInfoListL( R_CAMERA_SNAP_SOUND ));

	iRequests=CList<TSnapShotRequest>::NewL();
	CActiveScheduler::Add(this);
}

void CSnapShotImpl::TakeSnapShot(const TDesC& aDir, MSnapShotNofifier* aNotifier)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("TakeSnapShot"));


	iRequests->AppendL(TSnapShotRequest(aDir, aNotifier));

	if (iState==EIdle) {
		iState=EStarting;
		Async();
	}
	return;
}

CSnapShotImpl::~CSnapShotImpl()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("~CSnapShotImpl"));


	{
		if (iFileName.Length()>3) {
			Fs().Delete(iFileName);
			TFileName recfilename=iFileName;
			recfilename.Replace(recfilename.Length()-3, 3, _L("amr"));
			Fs().Delete(recfilename);
		}
	}

	if (iRequests) {
		TSnapShotRequest r;
		for (r=iRequests->Pop(); r.iNotifier; r=iRequests->Pop()) {
			r.iNotifier->Error(KErrCancel, _L("Cancelled"));
		}
	}
	delete iRequests; 
	Settings().CancelNotifyOnChange(SETTING_RECORD_TIME, this);

	Cancel();

	CloseRecorder();
	CloseCamera();

#ifndef __S60V2__
	delete iBitMap;
#endif
	delete iFileSaver;
}

void CSnapShotImpl::Async(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("Async"));


	TRequestStatus* s=&iStatus;
	User::RequestComplete(s, aError);
	SetActive();
}

void CSnapShotImpl::CloseCamera()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("CloseCamera"));


#ifndef __S60V2__
	if (iCameraIsOn) iCamera.TurnCameraOff();
	if (iCameraIsOpen) iCamera.Close();
#else
	if (iCamera) {
		if (iCameraIsOn) iCamera->PowerOff();
		iCamera->Release();
		delete iCamera; iCamera=0;
	}
#endif
	iCameraIsOn=iCameraIsOpen=false;
}

void CSnapShotImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("DoCancel"));


	CloseCamera();
	TRequestStatus* s=&iStatus;
	User::RequestComplete(s, KErrCancel);
}

void CSnapShotImpl::Retry()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("Retry"));


}

void CSnapShotImpl::CloseRecorder()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("CloseRecorder"));

	delete iRecorder;
	iRecorder=0;
}

void CSnapShotImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("CheckedRunL"));


	if (iStatus.Int()!=KErrNone) {
		CheckedRunError(iStatus.Int());
		return;
	}

	retries=0;

	switch(iState) {
	case EStarting:
		iNotifier=iRequests->iFirst->Item.iNotifier;
		CloseRecorder();
		MakeSaveImageName();
#ifndef __WINS__
		iState=ERecording;
		iRecordError=false;
		if (iRecordTime>0) {
			iNotifier->Info(_L("Starting record"));
			iRecorder=CMda_recorder::NewL(AppContext(), this, 1, 8000, iRecordTime);
			TFileName recname=iFileName;
			recname.Replace(recname.Length()-3, 3, _L("amr"));
			iRecStarted.HomeTime();
			iRecorder->record(recname);
		} else {
			Async();
		}
#else
		{
			TFileName from2=_L("C:\\nokia\\images\\old\\img.jpg");
			BaflUtils::CopyFile(Fs(), from2, iFileName);
			iRequests->iFirst->Item.recorded=iRequests->iFirst->Item.taken_picture=true;
			NotifyTaken();
		}
#endif

		break;
	case ERecording:
#ifndef __S60V2__
		User::LeaveIfError(iCamera.Connect());
		iCameraIsOpen=true;
		iCamera.TurnCameraOn(iStatus);
		SetActive();
		iState=ETurningOn;
		iNotifier->Info(_L("Turning Camera On"));
#else
		iNotifier->Info(_L("Reserving Camera"));
		iState=EReserving;
		iCamera=CCamera::NewL(*this, 0);
		iCamera->Reserve();
#endif
		break;
#ifdef __S60V2__
	case EReserving:
		iState=ETurningOn;
		iNotifier->Info(_L("Turning Camera On"));
		iCamera->PowerOn();
		break;
#endif
	case ETurningOn:
		iCameraIsOn=true;
		iState=EGettingImage;
#ifndef __S60V2__
		delete iBitMap; iBitMap=0;
		iBitMap=new (ELeave) CFbsBitmap;
		User::LeaveIfError(iCamera.SetImageQuality(RCameraServ::EQualityHigh));
		User::LeaveIfError(iCamera.SetLightingConditions(RCameraServ::ELightingNormal));
		iCamera.GetImage(iStatus, *iBitMap);
		SetActive();
#else
		iCamera->PrepareImageCaptureL(CCamera::EFormatFbsBitmapColor64K, 0);
		iCamera->CaptureImage();
#endif
		iNotifier->Info(_L("Getting Image"));
		break;
	case EGettingImage:
		iState=ECreatingFile;
		iNotifier->Info(_L("Creating File"));
		iSoundSystem->PlaySound(1);
		CreateSaveImage();
		break;
	case ECreatingFile:
		iState=EConverting;
		iNotifier->Info(_L("Saving File"));
		Convert();
		break;
	case EConverting:
		iRequests->iFirst->Item.taken_picture=true;
		NotifyTaken();
		break;
	}
}

void CSnapShotImpl::Next(bool ok)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("Next"));


	if (iState==EStarting || iState==ETurningOn) return;

	if (iRequests->iFirst) {
		if (ok && iRecordTime==0) {
			iState=ETurningOn;
		} else {
			CloseCamera();
			CloseRecorder();
			iState=EStarting;
		}
		Async();
	} else {
		iState=EIdle;
		CloseCamera();
		CloseRecorder();
	}
}

void CSnapShotImpl::MakeSaveImageName()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("MakeSaveImageName"));


	TFileName base=iRequests->iFirst->Item.iDir;
	if (base.Right(1).Compare(_L("\\"))) {
		base.Append(_L("\\"));
	}
	base.Append(_L("SnapShot"));
	iFileName=base;
	TInt i=1; iFileName.AppendNum(i); iFileName.Append(_L(".jpg"));
	while (BaflUtils::FileExists(Fs(), iFileName)) {
		iFileName=base;
		++i; iFileName.AppendNum(i); iFileName.Append(_L(".jpg"));
	}
}

void CSnapShotImpl::CreateSaveImage()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("CreateSaveImage"));


	delete iFileSaver; iFileSaver=0;
	iFileSaver=CMdaImageBitmapToFileUtility::NewL(*this);

	jfifFormat.iSettings.iSampleScheme  = TMdaJpgSettings::EColor420;
	jfifFormat.iSettings.iQualityFactor = 50;

	iFileSaver->CreateL(iFileName, &jfifFormat, NULL, NULL);

}

TInt CSnapShotImpl::CheckedRunError(TInt aCode)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("CheckedRunError"));


	{
		Fs().Delete(iFileName);
		TFileName recfilename=iFileName;
		recfilename.Replace(recfilename.Length()-3, 3, _L("amr"));
		Fs().Delete(recfilename);
	}

	retries++;
	if (0 && retries<5) {
		Retry();
		return KErrNone;
	}
#ifndef __S60V2__
	delete iBitMap; iBitMap=0;
#endif
	delete iFileSaver; iFileSaver=0;
	TBuf<60> msg=_L("Error taking picture while ");
	if (aCode!=-1031) {
		switch(iState) {
			case EStarting:
				msg.Append(_L("initializing camera"));
				break;
#ifdef __S60V2__
			case EReserving:
				msg.Append(_L("reserving camera"));
				break;
#endif
			case ETurningOn:
				msg.Append(_L("turning camera on"));
				break;
			case EGettingImage:
				msg.Append(_L("getting image"));
				break;
			case ECreatingFile:
				msg.Append(_L("creating file"));
				break;
			case EConverting:
				msg.Append(_L("saving file"));
				break;
			default:
				msg.Append(_L("[unknown]"));
				break;
		};
	} else {
		msg=_L("Error recording sound");
	}

	msg.Append(_L(" ")); msg.AppendNum(aCode);

	CloseCamera();
	CloseRecorder();
	iRequests->Pop();
	iState=EIdle;
	iNotifier->Error(aCode, msg);
	Next(false);
	return KErrNone;
}

void CSnapShotImpl::CancelSnapShot(MSnapShotNofifier* aNotifier)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("CancelSnapShot"));


	CList<TSnapShotRequest>::Node *n=0, *temp=0;
	for (n=iRequests->iFirst; n; ) {
		temp=n->Next;
		if (n->Item.iNotifier==aNotifier) {
			iRequests->DeleteNode(n, true);
		}
		n=temp;
	}
}

void CSnapShotImpl::Convert()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("Convert"));


	iFileSaver->ConvertL(*iBitMap);
}

void CSnapShotImpl::MiuoConvertComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("MiuoConvertComplete"));


	Async(aError);
}

void CSnapShotImpl::MiuoCreateComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("MiuoCreateComplete"));


	Async(aError);
}

void CSnapShotImpl::MiuoOpenComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("MiuoOpenComplete"));


	Async(aError);
}

void CSnapShotImpl::expired(CBase* /*Source*/)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("expired"));

	Async(KErrNone);
}

void CSnapShotImpl::stopped(bool reset)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("stopped"));

	if (iRecordError) return;

	TTime now; now.HomeTime(); now-=TTimeIntervalSeconds(iRecordTime-5);
	if ( now < iRecStarted) {
		if (iRecRetries>5) {
			Async(-1050);
		} else {
			iNotifier->Info(_L("retrying record"));
			iRecRetries++;
			TFileName recname=iFileName;
			recname.Replace(recname.Length()-3, 3, _L("amr"));
			Fs().Delete(recname);
			iState=EStarting;
			Async();
		}
		return;
	}
	iRecRetries=0;

	iRequests->iFirst->Item.recorded=true;
	Async(KErrNone);
	//NotifyTaken();
}

void CSnapShotImpl::error(const TDesC& descr)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("error"));

	iNotifier->Info(descr);
	iRecordError=true;
	Async(-1031);	
}

void CSnapShotImpl::opened()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("opened"));

}

void CSnapShotImpl::NotifyTaken()
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("NotifyTaken"));

	if ( (
		iRequests->iFirst->Item.recorded || iRecordTime==0
		)	&& iRequests->iFirst->Item.taken_picture) {

		iRequests->Pop();
	#ifndef __S60V2__
		delete iBitMap; iBitMap=0;
	#endif
		delete iFileSaver; iFileSaver=0;
		iNotifier->Taken(iFileName);
		iFileName=_L("nosuch");
		Next(true);
	}
}

#ifdef __S60V2__
void CSnapShotImpl::ReserveComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("ReserveComplete"));

	Async(aError);
}

void CSnapShotImpl::PowerOnComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("PowerOnComplete"));

	Async(aError);
}

void CSnapShotImpl::ViewFinderFrameReady(CFbsBitmap& aFrame)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("ViewFinderFrameReady"));

}
void CSnapShotImpl::ImageReady(CFbsBitmap* aBitmap, HBufC8* aData, TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("ImageReady"));

	iBitMap=aBitmap;
	Async(aError);
}

void CSnapShotImpl::FrameBufferReady(MFrameBuffer* aFrameBuffer, TInt aError)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("FrameBufferReady"));

}
#endif

void CSnapShotImpl::SettingChanged(TInt /*Setting*/)
{
	CALLSTACKITEM_N(_CL("CSnapShotImpl"), _CL("SettingChanged"));

	Settings().GetSettingL(SETTING_RECORD_TIME, iRecordTime);
}
