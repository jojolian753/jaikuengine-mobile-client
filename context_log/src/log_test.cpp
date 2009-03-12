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

#include "log_test.h"

#include <mda/common/resource.h>
#include <amrmda.h> 
#include "checkedactive.h"

#define INTERVAL 4

#include "app_context.h"

_LIT(KRecorderFile, "C:\\System\\Apps\\context_log\\record.wav");

log_test::log_test() : CTimer(CCheckedActive::EPriorityIdle),
audio_server(0), recorder_util(0), clip(buf)
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("log_test"));

}

void log_test::ConstructL()
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("ConstructL"));

	audio_server=CMdaServer::NewL();
	recorder_util = CMdaAudioRecorderUtility::NewL(*this, audio_server);

	current_state=IDLE;

	args.iChannels=1;
	args.iSampleRate=8000;

	sess.Connect();
	sess.Delete(KRecorderFile);

	file.Replace(sess, KRecorderFile, EFileShareAny | EFileStream | EFileWrite);

	CTimer::ConstructL();
	CActiveScheduler::Add(this);
	set_timer(INTERVAL);
}


log_test::~log_test()
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("~log_test"));

	Cancel();

	file.Close();
	sess.Close();

	if (recorder_util) {

		if (current_state==RECORDING || current_state==STARTING_RECORD) {
			recorder_util->Stop();
			recorder_util->Close();
		} else if (current_state==OPENING) {
			recorder_util->Close();
		}

		delete recorder_util;
	}

	if (audio_server) delete audio_server;
}

void log_test::stop(bool reset)
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("stop"));


	recorder_util->Stop();

	recorder_util->Close();

	if (!reset) {
		calculate();
	}

	current_state=IDLE;
	set_timer(INTERVAL);

	return;
}

void log_test::calculate()
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("calculate"));

	
	TInt pos=0;
	file.Seek(ESeekStart, pos);
	file.SetSize(0);
	file.Write(buf);
	file.Flush();
}

void log_test::set_timer(TInt seconds)
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("set_timer"));

	TTimeIntervalMicroSeconds32 i(seconds*1000*1000);
	After(i);
}

void log_test::open()
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("open"));

	buf.Zero();
	recorder_util->OpenL(&clip, &format, &codec, &args);

	current_state=OPENING;
}

void log_test::record()
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("record"));


	recorder_util->SetAudioDeviceMode(CMdaAudioRecorderUtility::ELocal);

	recorder_util->SetGain(recorder_util->MaxGain());

	recorder_util->RecordL();
	current_state=STARTING_RECORD;
}


void log_test::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("CheckedRunL"));

	switch(current_state) {
		case IDLE:
			open();
			break;
		case OPENING:
		case STARTING_RECORD: 
		case RECORDING: 
		case STOPPING:
			// SHOULDN'T HAPPEN
			break;
	}
}


void log_test::MoscoStateChangeEvent(CBase* /*aObject*/, 
					   TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
{
	CALLSTACKITEM_N(_CL("log_test"), _CL("MoscoStateChangeEvent"));

	if (aErrorCode==KErrNone) {
		if (aCurrentState==CMdaAudioClipUtility::EOpen && aPreviousState==CMdaAudioClipUtility::ENotReady)  {
			record();
		} else if (aCurrentState==CMdaAudioClipUtility::ERecording) {
			current_state=RECORDING;
		}
	} else {
		if (current_state!=STOPPING) {
			stop(current_state!=RECORDING);
		} else {
			return;
		}
	}
}

