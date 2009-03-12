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

#ifndef GAIN_ANALYSIS_H
#define GAIN_ANALYSIS_H

#undef NULL
#include <e32base.h>

#include <stddef.h>

#define GAIN_NOT_ENOUGH_SAMPLES  -24601
#define GAIN_ANALYSIS_ERROR           0
#define GAIN_ANALYSIS_OK              1

#define INIT_GAIN_ANALYSIS_ERROR      0
#define INIT_GAIN_ANALYSIS_OK         1

typedef double  Float_t;         // Type used for filtering

typedef unsigned short  Uint16_t;
typedef signed short    Int16_t;
typedef unsigned int    Uint32_t;
typedef signed int      Int32_t;

#define YULE_ORDER         10
#define BUTTER_ORDER        2
#define RMS_PERCENTILE      0.95        // percentile which is louder than the proposed level
#define MAX_SAMP_FREQ   48000.          // maximum allowed sample frequency [Hz]
#define RMS_WINDOW_TIME     0.050       // Time slice size [s]
#define STEPS_per_dB      100.          // Table entries per dB
#define MAX_dB            120.          // Table entries for 0...MAX_dB (normal max. values are 70...80 dB)

#define MAX_ORDER               (BUTTER_ORDER > YULE_ORDER ? BUTTER_ORDER : YULE_ORDER)
#define MAX_SAMPLES_PER_WINDOW  (size_t) (MAX_SAMP_FREQ * RMS_WINDOW_TIME)      // max. Samples per Time slice
#define PINK_REF                64.82 //298640883795                              // calibration value

class GainAnalysis : public CBase {
public:
	GainAnalysis();
	int     InitGainAnalysis ( long samplefreq );
	int     AnalyzeSamples   ( const Float_t* left_samples, const Float_t* right_samples, size_t num_samples, int num_channels );
	int		ResetSampleFrequency ( long samplefreq );

	Float_t   GetTitleGain     ( void );
	Float_t   GetAlbumGain     ( void );
private:
	Float_t	GainAnalysis::analyzeResult ( Uint32_t* Array, size_t len );
	void filter ( const Float_t* input, Float_t* output, size_t nSamples, const Float_t* a, const Float_t* b, size_t order );
	Float_t          linprebuf [MAX_ORDER * 2];
	Float_t*         linpre;                                          // left input samples, with pre-buffer
	Float_t          lstepbuf  [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	Float_t*         lstep;                                           // left "first step" (i.e. post first filter) samples
	Float_t          loutbuf   [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	Float_t*         lout;                                            // left "out" (i.e. post second filter) samples
	Float_t          rinprebuf [MAX_ORDER * 2];
	Float_t*         rinpre;                                          // right input samples ...
	Float_t          rstepbuf  [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	Float_t*         rstep;
	Float_t          routbuf   [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	Float_t*         rout;
	unsigned int              sampleWindow;                                    // number of samples required to reach number of milliseconds required for RMS window
	unsigned long    totsamp;
	double           lsum;
	double           rsum;
	int              freqindex;
	int              first;
	Uint32_t  A [(size_t)(STEPS_per_dB * MAX_dB)];
	Uint32_t  B [(size_t)(STEPS_per_dB * MAX_dB)];
	static const Float_t  AYule [9] [11];
	static const Float_t  BYule [9] [11];
	static const Float_t  AButter [9] [3];
	static const Float_t  BButter [9] [3];
};


#endif /* GAIN_ANALYSIS_H */
