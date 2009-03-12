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

#include "recorder.h"

Mrecorder::Mrecorder(): buf(0), bufdes(0, 0), istream(0)
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("Mrecorder"));

}

void Mrecorder::MaiscOpenComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("MaiscOpenComplete"));

        if (aError != KErrNone)	{
                _LIT(f, "open err %d");
		msg.Format(f, aError);
		error(msg);
	} else {
		opened();
	}
}

void Mrecorder::MaiscBufferCopied(TInt aError, const TDesC8& /*aBuffer*/)
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("MaiscBufferCopied"));

        if (aError != KErrNone)	{
                _LIT(f, "buffer err %d");
		msg.Format(f, aError);
		error(msg);
		stopped(true);
	} else {
	}
}

void Mrecorder::MaiscRecordComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("MaiscRecordComplete"));

        if (aError != KErrNone)	{
                _LIT(f, "record err %d");
		msg.Format(f, aError);
		error(msg);
		stopped(true);
	} else {
		stopped();
	}
}

void Mrecorder::record()
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("record"));

	bufdes.Zero();
	//bufdes.SetMax();
	istream->SetGain(istream->MaxGain());
	istream->ReadL(bufdes);
}

void Mrecorder::ConstructL(TInt channels, TInt freq, TInt seconds)
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("ConstructL"));

	buf=HBufC8::NewMaxL(channels*freq*seconds*2); // 16 bits
	bufdes.Set(buf->Des());
	rec_channels=args.iChannels=channels;
	rec_freq=args.iSampleRate=freq;
	rec_seconds=seconds;
	istream=CMdaAudioInputStream::NewL(*this);
	istream->SetDataTypeL(KMMFFourCCCodePCM16);
	istream->Open(&args);
}

Mrecorder::~Mrecorder()
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("~Mrecorder"));

	if (istream) {
		istream->Stop();
		delete istream;
	}	
	delete buf;
}

void Mrecorder::save(const TDesC& filename)
{
	CALLSTACKITEM_N(_CL("Mrecorder"), _CL("save"));

	WaveFile file;
        file.OpenForWrite(filename, rec_freq, 16, rec_channels);
	TInt ret;
        ret=file.WriteData((INT16*) bufdes.Ptr(), rec_freq*rec_channels*rec_seconds);
	if (ret!=DDC_SUCCESS) {
		msg.Format(_L("Write error: %d"), ret);
		error(msg);
	}
        ret=file.Close();
	if (ret!=DDC_SUCCESS) {
		msg.Format(_L("Close error: %d"), ret);
		error(msg);
	}
}
