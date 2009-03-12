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

#ifdef __WINS__
#pragma warning(disable: 4244) // 'conversion from 'int' to 'short', possible loss of data'
#endif 

#include "log_soundlevel.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <fstream.h>

#include "riff.h"
#include "gain_analysis.h"

#define MAX_CHUNK 100

#ifndef __WINS__
#define INTERVAL 60*5
#else 
#define INTERVAL 1
#endif

const int MAX_CHANNELS = 4;   // make larger if needed

_LIT(KRecorderFile, "C:\\System\\Apps\\context_log\\record.wav");

Clog_soundlevel::Clog_soundlevel(MApp_context& Context) : CTimer(CActive::EPriorityIdle), Mlog_base_impl(Context), 
soundlevel_value(0)
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("Clog_soundlevel"));

}

void Clog_soundlevel::ConstructL()
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("ConstructL"));

	soundlevel_value=HBufC::NewL(20);

	Mlog_base_impl::ConstructL(_L("soundlevel"), _L("%f"));

	prev_soundlevel=-99;
	
	current_state=OPENING;
	
	Mrecorder::ConstructL(1, 8000, 1);

#ifndef __WINS__

        RPhone::TLineInfo lineinfo;
        Phone().GetLineInfo(0, lineinfo);
        line.Open(Phone(), lineinfo.iName);
	
#endif

	CTimer::ConstructL();
	CActiveScheduler::Add(this);

}

Clog_soundlevel* Clog_soundlevel::NewL(MApp_context& Context)
{
	CALLSTACKITEMSTATIC_N(_CL("Clog_soundlevel"), _CL("NewL"));

	Clog_soundlevel* ret=new (ELeave) Clog_soundlevel(Context);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop();
	return ret;
}


Clog_soundlevel::~Clog_soundlevel()
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("~Clog_soundlevel"));

	Cancel();

	line.Close();

	if (soundlevel_value) delete soundlevel_value;
}

void Clog_soundlevel::stopped(bool reset)
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("stopped"));


	if (!reset) {
#ifdef __WINS__
		save(_L("c:\\system\\apps\\context_log\\test.wav"));
#else
		calculate();
#endif
	}
	
	current_state=IDLE;
	set_timer(INTERVAL);
	
	return;
}

void Clog_soundlevel::opened()
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("opened"));

	current_state=IDLE;
	set_timer(INTERVAL);
}

void Clog_soundlevel::error(const TDesC& descr)
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("error"));

	post_new_value(descr, Mlogger::ERR);
}

void Clog_soundlevel::calculate()
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("calculate"));

	double soundlevel=0.0;

	unsigned long lastFreq=44100;
	long maxSample;

	*soundlevel_value=(TText*)L"calculating";
	post_new_value(soundlevel_value->Des(), Mlogger::INFO);

	int retval;
	
	MemWaveFile wf(16, 1, 8000, bufdes);	
	retval=doFile(wf, &lastFreq, &maxSample);

	if (retval<=0) soundlevel=retval;
	soundlevel=(double)maxSample/32767.0;

	if (soundlevel==prev_soundlevel) {
		*soundlevel_value=(TText*)L"no change";
		post_new_value(soundlevel_value->Des(), Mlogger::INFO);
	} else {
		prev_soundlevel=soundlevel;
		soundlevel_value->Des().Format(*log_format, soundlevel);
		post_new_value(soundlevel_value->Des());
	}

}

void Clog_soundlevel::set_timer(TInt seconds)
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("set_timer"));

	TTimeIntervalMicroSeconds32 i(seconds*1000*1000);
	After(i);
	//SetActive();
}

void Clog_soundlevel::record()
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("record"));

	RLine::TLineInfo lineinfo;

#ifndef __WINS__
	line.GetInfo(lineinfo);
#else
	lineinfo.iStatus=RCall::EStatusIdle;
#endif

	// don't record if a call is in progress, so that
	// the call isn't interrupted with warning beeps
	if (! (lineinfo.iStatus==RCall::EStatusIdle || lineinfo.iStatus==RCall::EStatusUnknown)) {
		_LIT(d, "calling %d");
		soundlevel_value->Des().Format(d, lineinfo.iStatus);
		post_new_value(soundlevel_value->Des(), Mlogger::INFO);
		current_state=IDLE;
		set_timer(INTERVAL);
		return;
	}

	Mrecorder::record();
	*soundlevel_value=_L("recording");
	post_new_value(soundlevel_value->Des(), Mlogger::INFO);
}


void Clog_soundlevel::RunL()
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("RunL"));

	switch(current_state) {
	case IDLE:
		record();
		break;
	case OPENING:
	case RECORDING:
		soundlevel_value->Des().Format(_L("Unexpected state %d"), current_state);
		post_new_value(soundlevel_value->Des(), Mlogger::ERR);
	}
}

const TDesC& Clog_soundlevel::get_value()
{
	CALLSTACKITEM_N(_CL("Clog_soundlevel"), _CL("get_value"));

	*soundlevel_value=(TText*)L"";
	return *soundlevel_value;
}

int Clog_soundlevel::doFile(WaveFile& inWave, unsigned long *lastFreq, long *maxSample) {
	long maxFileSample = 0;
	long totsamp = 0;

	*maxSample=0;

	GainAnalysis *ga = new (ELeave) GainAnalysis;
	CleanupStack::PushL(ga);

	ga->InitGainAnalysis(*lastFreq);

	DDCRET rc = DDC_SUCCESS;

	int bit8;

	switch (inWave.BitsPerSample()) {
		case 8:
			bit8 = 1;
			break;

		case 16:
			bit8 = 0;
			break;

		default:
			//TODO
			//cerr << "Sorry, only 8-bit or 16-bit WAV files" << endl;
			return 0;
	}

	if (inWave.SamplingRate() != *lastFreq) {
		*lastFreq = inWave.SamplingRate();
		ga->ResetSampleFrequency(*lastFreq);
	}

	if (inWave.NumChannels() == 2) {
		Float_t lsamps[MAX_CHUNK], rsamps[MAX_CHUNK];

		int done = 0;
		int cursamp = 0;
		INT16 lsamp, rsamp;

		rc = inWave.ReadStereoSample(&lsamp,&rsamp);

		while (!done) {

			while ((cursamp < MAX_CHUNK) && (rc == DDC_SUCCESS)) {

				if (bit8) {
					lsamp = ((lsamp - 128) << 8) - 1;
					rsamp = ((rsamp - 128) << 8) - 1;
				}

				if (lsamp > maxFileSample)
					maxFileSample = lsamp;
				else if (-lsamp > maxFileSample)
					maxFileSample = -lsamp;

				if (rsamp > maxFileSample)
					maxFileSample = rsamp;
				else if (-rsamp > maxFileSample)
					maxFileSample = -rsamp;

				lsamps[cursamp] = (Float_t)(lsamp); // << multiplier);
				rsamps[cursamp++] = (Float_t)(rsamp); // << multiplier);

				rc = inWave.ReadStereoSample(&lsamp,&rsamp);
			}

			if (cursamp > 0) {
				totsamp += cursamp;
				ga->AnalyzeSamples(lsamps,rsamps,cursamp,2);
				cursamp = 0;
			}

			done = (rc != DDC_SUCCESS);
		}
	}
	else if (inWave.NumChannels() == 1) {
		Float_t samps[MAX_CHUNK];

		int done = 0;
		int cursamp = 0;
		INT16 samp;

		rc = inWave.ReadMonoSample(&samp);

		while (!done) {

			while ((cursamp < MAX_CHUNK) && (rc == DDC_SUCCESS)) {
				
				if (bit8) {
					samp = ((samp - 128) << 8) - 1;
				}

				if (samp > maxFileSample)
					maxFileSample = samp;
				else if (-samp > maxFileSample)
					maxFileSample = -samp;

				samps[cursamp++] = (Float_t)(samp); // << multiplier);

				rc = inWave.ReadMonoSample(&samp);
			}

			if (cursamp > 0) {
				totsamp += cursamp;
				ga->AnalyzeSamples(samps, 0,cursamp,1);
				cursamp = 0;
			}

			done = (rc != DDC_SUCCESS);
		}
	}
	else {
		//TODO
		//cerr << "Can't handle more than 2 channels." << endl;
		return -3;
	}

	*maxSample = maxFileSample;

	inWave.Close();

	CleanupStack::Pop(); // ga
	delete ga;

	if (totsamp > 0) {
		//TODO
		//cout << inFilename << "\t" << GetTitleGain() << "\t" << ((double)(maxFileSample) / 32767.0) << endl;
		return 1;
	}

	return 0;
}

