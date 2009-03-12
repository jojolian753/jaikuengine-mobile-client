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

#if !defined(RECORDER_H_INCLUDED)


#define RECORDER_H_INCLUDED 1

#include <e32base.h>
#include <MdaAudioInputStream.h>
#include <mda/common/audio.h>
#include <f32file.h>
#include "app_context.h"
#include "riff.h"


class Mrecorder : public MMdaAudioInputStreamCallback {
public:
	virtual ~Mrecorder();

	const TDesC& get_value();

	void CheckedRunL();

	static Mrecorder* NewL();
	void  MaiscOpenComplete (TInt aError);
	void  MaiscBufferCopied (TInt aError, const TDesC8 &aBuffer);
	void  MaiscRecordComplete (TInt aError);	

	virtual void stopped(bool reset=false) = 0;
	virtual void error(const TDesC& descr) = 0;
	virtual void opened() = 0;
	virtual void record();
	void save(const TDesC& filename);

protected:
	void ConstructL(TInt channels, TInt freq, TInt seconds);
	HBufC8*	buf; TPtr8 bufdes;
	Mrecorder();
	void open();
private:
	CMdaAudioInputStream* istream;

	TMdaWavClipFormat format;
	TMdaPcmWavCodec codec; 
	TMdaAudioDataSettings args;
	TMdaFileClipLocation audio_loc;

	TBuf<100> msg;
	int rec_seconds;
	int rec_freq;
	int rec_channels;
};

#endif
