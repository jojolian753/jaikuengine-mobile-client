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
#pragma warning(disable: 4244)
#endif

#include "ddc.h"

#undef EOF
#define EOF -1

#include <stdlib.h>
#include <string.h>

#undef NULL
#include <f32file.h>

#ifndef __DDC_RIFF_H
#define __DDC_RIFF_H

#pragma pack(1)

unsigned long FourCC ( const char *ChunkName );


enum RiffFileMode
{
   RFM_UNKNOWN,      // undefined type (can use to mean "N/A" or "not open")
   RFM_WRITE,        // open for write
   RFM_READ          // open for read
};


struct RiffChunkHeader
{
   UINT32    ckID;       // Four-character chunk ID
   UINT32    ckSize;     // Length of data in chunk
};


class RiffFile : public CBase
{
   RiffChunkHeader   riff_header;      // header for whole file

protected:
   RiffFileMode      fmode;            // current file I/O mode
   RFile             file;             // I/O stream to use
   RFs		     session;
   DDCRET  Seek ( long offset );

public:
   RiffFile();
   ~RiffFile();

   RiffFileMode CurrentFileMode() const   {return fmode;}

   virtual DDCRET Open ( const TDesC& Filename, RiffFileMode NewMode );
   virtual DDCRET Write  ( const void *Data, unsigned NumBytes );
   virtual DDCRET Read   (       void *Data, unsigned NumBytes );
   virtual DDCRET Expect ( const void *Data, unsigned NumBytes );
   virtual DDCRET Close();

   int filelength(const TDesC& Filename);

   virtual long	   FindStringInFile ( const void *Data, unsigned int NumBytes );
   virtual long    CurrentFilePosition() const;

   virtual DDCRET  Backpatch ( long FileOffset,
                       const void *Data,
                       unsigned NumBytes );
};


struct WaveFormat_ChunkData
{
   UINT16         wFormatTag;       // Format category (PCM=1)
   UINT16         nChannels;        // Number of channels (mono=1, stereo=2)
   UINT32         nSamplesPerSec;   // Sampling rate [Hz]
   UINT32         nAvgBytesPerSec;
   UINT16         nBlockAlign;
   UINT16         nBitsPerSample;

   void Config ( UINT32   NewSamplingRate   = 44100,
                 UINT16   NewBitsPerSample  =    16,
                 UINT16   NewNumChannels    =     2 )
   {
      nSamplesPerSec  = NewSamplingRate;
      nChannels       = NewNumChannels;
      nBitsPerSample  = NewBitsPerSample;
      nAvgBytesPerSec = (nChannels * nSamplesPerSec * nBitsPerSample) / 8;
      nBlockAlign     = (unsigned short)(nChannels * nBitsPerSample) / 8;
   }

   WaveFormat_ChunkData()
   {
      wFormatTag = 1;     // PCM
      Config();
   }
};


struct WaveFormat_Chunk
{
   RiffChunkHeader         header;
   WaveFormat_ChunkData    data;

   WaveFormat_Chunk()
   {
      header.ckID     =   FourCC("fmt");
      header.ckSize   =   sizeof ( WaveFormat_ChunkData );
   }

   dBOOLEAN VerifyValidity()
   {
      return header.ckID == FourCC("fmt") &&

             (data.nChannels == 1 || data.nChannels == 2) &&

             data.nAvgBytesPerSec == ( data.nChannels *
                                       data.nSamplesPerSec *
                                       data.nBitsPerSample    ) / 8   &&

             data.nBlockAlign == ( data.nChannels *
                                   data.nBitsPerSample ) / 8;
   }
};


#define  MAX_WAVE_CHANNELS   2


struct WaveFileSample
{
   INT16  chan [MAX_WAVE_CHANNELS];
};

class MemWaveFile;

class WaveFile: private RiffFile
{
   WaveFormat_Chunk   wave_format;
   RiffChunkHeader    pcm_data;
   long               pcm_data_offset;  // offset of 'pcm_data' in output file
   UINT32             num_samples;

public:
   WaveFile();

   virtual DDCRET OpenForWrite ( const TDesC& Filename,
                         UINT32       SamplingRate   = 44100,
                         UINT16       BitsPerSample  =    16,
                         UINT16       NumChannels    =     2 );

   virtual DDCRET OpenForRead ( const TDesC& Filename );

   virtual DDCRET ReadSample   ( INT16 Sample [MAX_WAVE_CHANNELS] );
   virtual DDCRET WriteSample  ( const INT16 Sample [MAX_WAVE_CHANNELS] );
   virtual DDCRET SeekToSample ( unsigned long SampleIndex );

   // The following work only with 16-bit audio
   virtual DDCRET WriteData ( const INT16 *data, UINT32 numData );
   virtual DDCRET ReadData  ( INT16 *data, UINT32 numData );

   // The following work only with 8-bit audio
   virtual DDCRET WriteData ( const UINT8 *data, UINT32 numData );
   virtual DDCRET ReadData  ( UINT8 *data, UINT32 numData );

   virtual DDCRET ReadSamples  ( INT32 num, WaveFileSample[] );

   virtual DDCRET WriteMonoSample    ( INT16 ChannelData );
   virtual DDCRET WriteStereoSample  ( INT16 LeftChannelData, INT16 RightChannelData );

   virtual DDCRET ReadMonoSample ( INT16 *ChannelData );
   virtual DDCRET ReadStereoSample ( INT16 *LeftSampleData, INT16 *RightSampleData );

   virtual DDCRET Close();

   virtual UINT32   SamplingRate()   const;    // [Hz]
   virtual UINT16   BitsPerSample()  const;
   virtual UINT16   NumChannels()    const;
   virtual UINT32   NumSamples()     const;

   // Open for write using another wave file's parameters...

   virtual DDCRET OpenForWrite ( const TDesC& Filename,
                         WaveFile &OtherWave )
   {
      return OpenForWrite ( Filename,
                            OtherWave.SamplingRate(),
                            OtherWave.BitsPerSample(),
                            OtherWave.NumChannels() );
   }

   virtual long CurrentFilePosition() const
   {
      return RiffFile::CurrentFilePosition();
   }
   friend class MemWaveFile;
};

class MemWaveFile : public WaveFile {
private:
	const TDesC8&	wavebuf;
	int	pos;
	int	firstpos;
public:
	MemWaveFile(unsigned short bits, unsigned short channels, 
			unsigned short freq, const TDesC8& buf) : wavebuf(buf) {
		wave_format.data.Config(freq, bits, channels);
		pcm_data_offset=sizeof(RiffChunkHeader);
		firstpos=sizeof(RiffChunkHeader)+sizeof(pcm_data);
		num_samples = wavebuf.Length();
		num_samples /= channels;
		num_samples /= (bits / 8);
		pos=0;
	}
	DDCRET OpenForWrite ( const TDesC& /*Filename*/,
			 UINT32       /*SamplingRate   = 44100*/,
			 UINT16       /*BitsPerSample  =    16*/,
			 UINT16       /*NumChannels    =     2*/ ) {
		return DDC_INVALID_CALL;
	}
	DDCRET OpenForRead ( const TDesC& /*Filename*/ ) {
		return DDC_SUCCESS;
	}
	DDCRET Close() { return DDC_SUCCESS; }
	DDCRET Read ( void *Data, unsigned NumBytes ) {
		if ( (unsigned)pos+NumBytes > (unsigned)wavebuf.Length()) return DDC_FILE_ERROR;

		memcpy(Data, wavebuf.Ptr()+pos, NumBytes);
		pos+=NumBytes;
		return DDC_SUCCESS;
	}
	DDCRET Write ( const void * /*Data*/, unsigned /*NumBytes*/ ) {
		return DDC_INVALID_CALL;
	}
	long CurrentFilePosition() const {
		return pos+firstpos;
	}
	DDCRET Seek ( long offset ) {
		pos=offset-firstpos;
		return DDC_SUCCESS;
	}

};


#pragma pack()

#define NULL 0 

#endif /* __DDC_RIFF_H */


/*--- end of file riff.h ---*/
