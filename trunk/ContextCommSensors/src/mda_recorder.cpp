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
#include "mda_recorder.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#ifdef __S60V2__
#include <mmfcontrollerimplementationuids.hrh>
#endif

CMda_recorder::CMda_recorder(MApp_context& Context): CTimer(EPriorityNormal), MContextBase(Context) { }

void CMda_recorder::MoscoStateChangeEvent(CBase* /*aObject*/, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
{
	CALLSTACKITEM_N(_CL("CMda_recorder"), _CL("MoscoStateChangeEvent"));

	if (aErrorCode==KErrNone) {
		if (aCurrentState==CMdaAudioClipUtility::EOpen && aPreviousState==CMdaAudioClipUtility::ENotReady)  {
			current_state=STARTING_RECORD;
			do_record();
		} else if (aCurrentState==CMdaAudioClipUtility::ERecording) {
			current_state=RECORDING;
		} 
	} else {
		Cancel();
		recorder_util->Close();
		delete clip; clip=0;
		if (current_state!=STOPPING) {
			_LIT(e2, "err at state %d: %d");
			msg.Format(e2, aCurrentState, aErrorCode);
			Reporting().UserErrorLog(msg);
			cb->stopped(current_state!=RECORDING);
		} else {
			_LIT(e2, "err at state %d: %d");
			msg.Format(e2, aCurrentState, aErrorCode);
			current_state=IDLE;
			cb->error(msg);
			return;
		}
		current_state=IDLE;
	}
}


void CMda_recorder::RunL()
{
	CALLSTACKITEM_N(_CL("CMda_recorder"), _CL("RunL"));

	Reporting().UserErrorLog(_L("Recorded"));
	recorder_util->Stop();
	recorder_util->Close();
	delete clip; clip=0;
	cb->stopped();
	current_state=IDLE;
}

EXPORT_C void CMda_recorder::record(const TDesC& filen)
{
	CALLSTACKITEM_N(_CL("CMda_recorder"), _CL("record"));

	if (clip) {
		recorder_util->Close();
		delete clip; clip=0;
	}
	if (NoSpaceLeft()) {
		Reporting().UserErrorLog(_L("No space left"));
		cb->stopped();
		return;
	}
	{
		TBuf<256> msg=_L("Starting record to ");
		msg.Append(filen);
		Reporting().UserErrorLog(msg);
	}

	current_state=OPENING;
	clip=new (ELeave) TMdaFileClipLocation(filen);
#ifndef __S60V2__
	CC_TRAPD(err, recorder_util->OpenL(clip, &format, codec, &args));
#else
	const TUint32 KMMFFourCCCodeAMRNB = 0x524d4120; // (' ', 'A', 'M', 'R')
	const TUid KUidFormatAMRWrite = {0x101FAF66};
	//const TUint32 KMMFFourCCCodeAMRNB = 0x524d4121; // ('!', 'A', 'M', 'R')
	//const TUid KUidFormatAMRWrite = {0x101faf7e};
	/*CC_TRAPD(err, recorder_util->OpenFileL(filen, 
		TUid::Uid(KMmfUidControllerAudio),
		KNullUid, 
		KNullUid,//KUidFormatAMRWrite, 
		TFourCC(KMMFFourCCCodeAMRNB) ));
		*/
	CC_TRAPD(err, recorder_util->OpenFileL(filen));
#endif

	if (err!=KErrNone) {
		TBuf<50> msg=_L("Open(File)L error: ");
		msg.AppendNum(err);
		cb->error(msg);
		cb->stopped();
		current_state=IDLE;
	}
}

void CMda_recorder::do_record()
{
	CALLSTACKITEM_N(_CL("CMda_recorder"), _CL("do_record"));

	recorder_util->SetAudioDeviceMode(CMdaAudioRecorderUtility::ELocal);
	// Set maximum gain for recording
	recorder_util->SetGain(recorder_util->MaxGain());
	CC_TRAPD(err, recorder_util->RecordL());
	if (err!=KErrNone) {
		TBuf<50> msg=_L("RecordL error: ");
		msg.AppendNum(err);
		cb->error(msg);
		cb->stopped();
		current_state=IDLE;
		return;
	}
	TTimeIntervalMicroSeconds32 w(1000*1000*rec_seconds);
	After(w);
}

void CMda_recorder::ConstructL(Mrecorder_callback* i_cb, TInt channels, TInt freq, TInt seconds)
{
	CALLSTACKITEM_N(_CL("CMda_recorder"), _CL("ConstructL"));

	cb=i_cb;

	/*
	buf=HBufC8::NewMaxL(channels*freq*seconds*2); // 16 bits
	bufdes.Set(buf->Des());	
	*/

#ifndef __S60V2__
	args.iChannels=channels;
	args.iSampleRate=freq;
	codec=new (ELeave) TMdaRawAmrAudioCodec(TMdaRawAmrAudioCodec::EMR515, ETrue);
#endif
	rec_channels=channels;
	rec_freq=freq;
	rec_seconds=seconds;

#ifdef __S60V2__
	// only priority 80 works when recording calls
	recorder_util = CMdaAudioRecorderUtility::NewL(*this, 0, 80, EMdaPriorityPreferenceTimeAndQuality);
#else
	audio_server=CMdaServer::NewL();
	recorder_util = CMdaAudioRecorderUtility::NewL(*this, audio_server);
#endif
	
	CTimer::ConstructL();
	CActiveScheduler::Add(this);
	//cb->opened();
}

EXPORT_C CMda_recorder* CMda_recorder::NewL(MApp_context& Context, Mrecorder_callback* i_cb, TInt channels, TInt freq, TInt seconds)
{
	CALLSTACKITEMSTATIC_N(_CL("CMda_recorder"), _CL("NewL"));

	auto_ptr<CMda_recorder> ret(new (ELeave) CMda_recorder(Context));
	ret->ConstructL(i_cb, channels, freq, seconds);
	return ret.release();
}

EXPORT_C CMda_recorder::~CMda_recorder()
{
	CALLSTACKITEM_N(_CL("CMda_recorder"), _CL("~CMda_recorder"));

	Cancel();
	if (recorder_util) {
		recorder_util->Close();
		delete recorder_util;
	}	
	delete clip;
	//delete buf;
#ifndef __S60V2__
	delete codec;
	delete audio_server;
#endif
}

EXPORT_C void CMda_recorder::save(const TDesC& /*filename*/)
{
	CALLSTACKITEM_N(_CL("CMda_recorder"), _CL("save"));

	/*
	RFile file;
	User::LeaveIfError(file.Replace(Fs(), filename, EFileShareAny | EFileWrite));
        
	TInt ret;
        ret=file.Write(bufdes);
	if (ret!=KErrNone) {
		msg.Format(_L("Write cb->error: %d"), ret);
		cb->error(msg);
	}
	file.Close();
	*/
}

EXPORT_C bool CMda_recorder::IsIdle() const { return current_state==IDLE; }
