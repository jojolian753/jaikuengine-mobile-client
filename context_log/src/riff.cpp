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

#define EOF -1

#include <stdlib.h>
#include <string.h>

#undef NULL
#include <f32file.h>
#include <charconv.h>

// DDCLIB includes
#include "riff.h"

#include "app_context.h"

UINT32 FourCC ( const char *ChunkName )
{
	UINT32 retbuf = 0x20202020;   // four spaces (padding)
	char *p = ((char *)&retbuf);
	
	// Remember, this is Intel format!
	// The first character goes in the LSB
	
	for ( int i=0; i<4 && ChunkName[i]; i++ )
	{
		*p++ = ChunkName[i];
	}
	
	return retbuf;
}


//----------------------------------------------------------------------


RiffFile::RiffFile()
{
	fmode = RFM_UNKNOWN;
	
	riff_header.ckID = FourCC("RIFF");
	riff_header.ckSize = 0;
}


RiffFile::~RiffFile()
{
	if ( fmode != RFM_UNKNOWN )
	{
		Close();
	}
}

DDCRET RiffFile::Open ( const TDesC& Filename, RiffFileMode NewMode )
{
	User::LeaveIfError(session.Connect()); 
	
	DDCRET retcode = DDC_SUCCESS;
	
	if ( fmode != RFM_UNKNOWN )
	{
		retcode = Close();
	}
	
	int retval=0;
	if ( retcode == DDC_SUCCESS )
	{
		switch ( NewMode )
		{
		case RFM_WRITE:
			retval=file.Replace(session, Filename, EFileShareAny | EFileStream | EFileWrite);
			if ( retval==KErrNone )
			{
				// Write the RIFF header...
				// We will have to come back later and patch it!
				
				fmode=RFM_WRITE;
				
				if ( Write ( &riff_header, sizeof(riff_header) ) != DDC_SUCCESS )
				{
					Close();
					// TODO?
					//unlink(Filename);
					fmode = RFM_UNKNOWN;
				}
				else
				{
					fmode = RFM_WRITE;
				}
			}
			else
			{
				fmode = RFM_UNKNOWN;
				retcode = DDC_FILE_ERROR;
			}
			break;
			
		case RFM_READ:
			retval=file.Open(session, Filename, EFileShareAny | EFileStream | EFileRead);
			if ( retval==KErrNone )
			{
				// Try to read the RIFF header...
				fmode=RFM_READ;
				if ( Read ( &riff_header, sizeof(riff_header) ) != DDC_SUCCESS )
				{
					file.Close();
					fmode = RFM_UNKNOWN;
				}
				else
				{
					fmode = RFM_READ;
				}
			}
			else
			{
				fmode = RFM_UNKNOWN;
				retcode = DDC_FILE_ERROR;
			}
			break;
			
		default:
			retcode = DDC_INVALID_CALL;
		}
	}
	
	return retcode;
}


DDCRET RiffFile::Write ( const void *Data, unsigned NumBytes )
{
	if ( fmode != RFM_WRITE )
	{
		return DDC_INVALID_CALL;
	}
	
	TPtr8 ptr((unsigned char*)Data, NumBytes, NumBytes);
	if ( file.Write(ptr) != KErrNone )
	{
		return DDC_FILE_ERROR;
	}
	
	riff_header.ckSize += NumBytes;
	
	return DDC_SUCCESS;
}


DDCRET RiffFile::Close()
{
	DDCRET retcode = DDC_SUCCESS;
	
	TInt pos=0;
	switch ( fmode )
	{
	case RFM_WRITE:
		if ( file.Flush()!=KErrNone ||
			file.Seek(ESeekStart, pos)!=KErrNone ||
			Write ( &riff_header, sizeof(riff_header) ) != DDC_SUCCESS ||
			file.Flush()!=KErrNone )
		{
			retcode = DDC_FILE_ERROR;
		}
		file.Close();
		break;
		
	case RFM_READ:
		file.Close();
		break;
	case RFM_UNKNOWN:
		//NOP
		break;
	}
	
	fmode = RFM_UNKNOWN;

	session.Close();
	
	return retcode;
}

int fgetc(RFile& file)
{
	TBuf8<2> buf;
	TInt ret=file.Read(buf, 1);
	if (ret!=KErrNone || buf.Length()==0) return EOF;
	return buf[0];
}

long RiffFile::FindStringInFile ( const void *Data , unsigned int NumBytes)
{
	char *p = (char *)Data;
	int ch = fgetc(file);
	unsigned int okcount = 0;
	
	while ((okcount != NumBytes) && (ch != EOF)) {
		if (*p++ == ch) {
			if (++okcount < NumBytes) {
				ch = fgetc(file);
			}
		}
		else {
			if (okcount == 0) {
				ch = fgetc(file);
			}
			else {
				okcount = 0;
			}
			p = (char*)Data;
		}
		
	}
	
	if (ch == EOF)
		return -1;
	
	

	TInt pos=-(int)(NumBytes);
	file.Seek(ESeekCurrent, pos);

	pos=0;
	file.Seek(ESeekCurrent, pos);
	return pos;
}


long RiffFile::CurrentFilePosition() const
{
	TInt pos=0;
	file.Seek(ESeekCurrent, pos);
	return pos;
}


DDCRET RiffFile::Seek ( long offset )
{
	file.Flush();
	
	DDCRET rc;
	
	TInt pos=offset;
	if ( file.Seek ( ESeekStart, pos ) != KErrNone )
	{
		rc = DDC_FILE_ERROR;
	}
	else
	{
		rc = DDC_SUCCESS;
	}
	
	return rc;
}


DDCRET RiffFile::Backpatch ( long FileOffset,
			    const void *Data,
			    unsigned NumBytes )
{
	if ( fmode!= RFM_WRITE )
	{
		return DDC_INVALID_CALL;
	}
	
	if ( Seek(FileOffset)!=DDC_SUCCESS )
	{
		return DDC_FILE_ERROR;
	}
	
	return Write ( Data, NumBytes );
}


DDCRET RiffFile::Expect ( const void *Data, unsigned NumBytes )
{
	char *p = (char *)Data;
	
	while ( NumBytes-- )
	{
		if ( fgetc(file) != *p++ )
		{
			return DDC_FILE_ERROR;
		}
	}
	
	return DDC_SUCCESS;
}


DDCRET RiffFile::Read ( void *Data, unsigned NumBytes )
{
	DDCRET retcode = DDC_SUCCESS;
	
	TPtr8 p((unsigned char*)Data, NumBytes);
	if ( file.Read(p, NumBytes)!=KErrNone )
	{
		retcode = DDC_FILE_ERROR;
	}
	if ((unsigned)p.Length()!=NumBytes) return DDC_FILE_ERROR;
	
	return retcode;
}


//-----------------------------------------------------------------------

WaveFile::WaveFile()
{
	pcm_data.ckID = FourCC("data");
	pcm_data.ckSize = 0;
	num_samples = 0;
}

int RiffFile::filelength(const TDesC& Filename)
{
	TEntry entry;
	User::LeaveIfError(session.Entry(Filename, entry)); 
	return entry.iSize;
}


DDCRET WaveFile::OpenForRead ( const TDesC& Filename )
{
	// Verify filename parameter as best we can...
	if ( ! Filename.Length() )
	{
		return DDC_INVALID_CALL;
	}
	
	DDCRET retcode = Open ( Filename, RFM_READ );
	
	if ( retcode == DDC_SUCCESS )
	{
		retcode = Expect ( "WAVE", 4 );
		
		if ( retcode == DDC_SUCCESS )
		{
			retcode = Read ( &wave_format, sizeof(wave_format) );
			
			if ( retcode == DDC_SUCCESS &&
				!wave_format.VerifyValidity() )
			{
				// This isn't standard PCM, so we don't know what it is!
				
				retcode = DDC_FILE_ERROR;
			}
			
			if ( retcode == DDC_SUCCESS )
			{
				if ((pcm_data_offset = FindStringInFile("data",4)) == -1) { ; // CurrentFilePosition();
				retcode = DDC_FILE_ERROR;
				}
				
				else {
					
					// Figure out number of samples from
					// file size, current file position, and
					// WAVE header.
					
					
					
					retcode = Read ( &pcm_data, sizeof(pcm_data) );
					// 
					//num_samples = filelength(fileno(file)) - CurrentFilePosition();
					num_samples = filelength(Filename) - CurrentFilePosition();
					num_samples /= NumChannels();
					num_samples /= (BitsPerSample() / 8);
				}
			}
		}
	}
	
	return retcode;
}


DDCRET WaveFile::OpenForWrite ( const TDesC&  Filename,
			       UINT32       SamplingRate,
			       UINT16       BitsPerSample,
			       UINT16       NumChannels )
{
	// Verify parameters...
	
	if ( ! Filename.Length() ||
		(BitsPerSample != 8 && BitsPerSample != 16) ||
		NumChannels < 1 || NumChannels > 2 )
	{
		return DDC_INVALID_CALL;
	}
	
	wave_format.data.Config ( SamplingRate, BitsPerSample, NumChannels );
	
	DDCRET retcode = Open ( Filename, RFM_WRITE );
	
	if ( retcode == DDC_SUCCESS )
	{
		retcode = Write ( "WAVE", 4 );
		
		if ( retcode == DDC_SUCCESS )
		{
			retcode = Write ( &wave_format, sizeof(wave_format) );
			
			if ( retcode == DDC_SUCCESS )
			{
				pcm_data_offset = CurrentFilePosition();
				retcode = Write ( &pcm_data, sizeof(pcm_data) );
			}
		}
	}
	
	return retcode;
}


DDCRET WaveFile::Close()
{
	DDCRET rc = DDC_SUCCESS;
	
	if ( fmode == RFM_WRITE )
		rc = Backpatch ( pcm_data_offset, &pcm_data, sizeof(pcm_data) );
	
	if ( rc == DDC_SUCCESS )
		rc = RiffFile::Close();
	
	return rc;
}


DDCRET WaveFile::WriteSample ( const INT16 Sample [MAX_WAVE_CHANNELS] )
{
	DDCRET retcode = DDC_SUCCESS;
	
	switch ( wave_format.data.nChannels )
	{
	case 1:
		switch ( wave_format.data.nBitsPerSample )
		{
		case 8:
			pcm_data.ckSize += 1;
			retcode = Write ( &Sample[0], 1 );
			break;
			
		case 16:
			pcm_data.ckSize += 2;
			retcode = Write ( &Sample[0], 2 );
			break;
			
		default:
			retcode = DDC_INVALID_CALL;
		}
		break;
		
		case 2:
			switch ( wave_format.data.nBitsPerSample )
			{
			case 8:
				retcode = Write ( &Sample[0], 1 );
				if ( retcode == DDC_SUCCESS )
				{
					retcode = Write ( &Sample[1], 1 );
					if ( retcode == DDC_SUCCESS )
					{
						pcm_data.ckSize += 2;
					}
				}
				break;
				
			case 16:
				retcode = Write ( &Sample[0], 2 );
				if ( retcode == DDC_SUCCESS )
				{
					retcode = Write ( &Sample[1], 2 );
					if ( retcode == DDC_SUCCESS )
					{
						pcm_data.ckSize += 4;
					}
				}
				break;
				
			default:
				retcode = DDC_INVALID_CALL;
			}
			break;
			
			default:
				retcode = DDC_INVALID_CALL;
	}
	
	return retcode;
}


DDCRET WaveFile::WriteMonoSample ( INT16 SampleData )
{
	switch ( wave_format.data.nBitsPerSample )
	{
	case 8:
		pcm_data.ckSize += 1;
		return Write ( &SampleData, 1 );
		
	case 16:
		pcm_data.ckSize += 2;
		return Write ( &SampleData, 2 );
	}
	
	return DDC_INVALID_CALL;
}


DDCRET WaveFile::WriteStereoSample ( INT16 LeftSample,
				    INT16 RightSample )
{
	DDCRET retcode = DDC_SUCCESS;
	
	switch ( wave_format.data.nBitsPerSample )
	{
	case 8:
		retcode = Write ( &LeftSample, 1 );
		if ( retcode == DDC_SUCCESS )
		{
			retcode = Write ( &RightSample, 1 );
			if ( retcode == DDC_SUCCESS )
			{
				pcm_data.ckSize += 2;
			}
		}
		break;
		
	case 16:
		retcode = Write ( &LeftSample, 2 );
		if ( retcode == DDC_SUCCESS )
		{
			retcode = Write ( &RightSample, 2 );
			if ( retcode == DDC_SUCCESS )
			{
				pcm_data.ckSize += 4;
			}
		}
		break;
		
	default:
		retcode = DDC_INVALID_CALL;
	}
	
	return retcode;
}



DDCRET WaveFile::ReadSample ( INT16 Sample [MAX_WAVE_CHANNELS] )
{
	DDCRET retcode = DDC_SUCCESS;
	
	switch ( wave_format.data.nChannels )
	{
	case 1:
		switch ( wave_format.data.nBitsPerSample )
		{
		case 8:
			unsigned char x;
			retcode = Read ( &x, 1 );
			Sample[0] = INT16(x);
			break;
			
		case 16:
			retcode = Read ( &Sample[0], 2 );
			break;
			
		default:
			retcode = DDC_INVALID_CALL;
		}
		break;
		
		case 2:
			switch ( wave_format.data.nBitsPerSample )
			{
			case 8:
				unsigned char  x[2];
				retcode = Read ( x, 2 );
				Sample[0] = INT16 ( x[0] );
				Sample[1] = INT16 ( x[1] );
				break;
				
			case 16:
				retcode = Read ( Sample, 4 );
				break;
				
			default:
				retcode = DDC_INVALID_CALL;
			}
			break;
			
			default:
				retcode = DDC_INVALID_CALL;
	}
	
	return retcode;
}


DDCRET WaveFile::ReadSamples ( INT32 num, WaveFileSample sarray[] )
{
	DDCRET retcode = DDC_SUCCESS;
	INT32 i;
	
	switch ( wave_format.data.nChannels )
	{
	case 1:
		switch ( wave_format.data.nBitsPerSample )
		{
		case 8:
			for ( i=0; i < num && retcode == DDC_SUCCESS; i++ )
			{
				unsigned char x;
				retcode = Read ( &x, 1 );
				sarray[i].chan[0] = INT16(x);
			}
			break;
			
		case 16:
			for ( i=0; i < num && retcode == DDC_SUCCESS; i++ )
			{
				retcode = Read ( &sarray[i].chan[0], 2 );
			}
			break;
			
		default:
			retcode = DDC_INVALID_CALL;
		}
		break;
		
		case 2:
			switch ( wave_format.data.nBitsPerSample )
			{
			case 8:
				for ( i=0; i < num && retcode == DDC_SUCCESS; i++ )
				{
					unsigned char x[2];
					retcode = Read ( x, 2 );
					sarray[i].chan[0] = INT16 ( x[0] );
					sarray[i].chan[1] = INT16 ( x[1] );
				}
				break;
				
			case 16:
				retcode = Read ( sarray, 4 * num );
				break;
				
			default:
				retcode = DDC_INVALID_CALL;
			}
			break;
			
			default:
				retcode = DDC_INVALID_CALL;
	}
	
	return retcode;
}


DDCRET WaveFile::ReadMonoSample ( INT16 *Sample )
{
	DDCRET retcode = DDC_SUCCESS;
	
	switch ( wave_format.data.nBitsPerSample )
	{
	case 8:
		unsigned char x;
		retcode = Read ( &x, 1 );
		*Sample = INT16(x);
		break;
		
	case 16:
		retcode = Read ( Sample, 2 );
		break;
		
	default:
		retcode = DDC_INVALID_CALL;
	}
	
	return retcode;
}


DDCRET WaveFile::ReadStereoSample ( INT16 *L, INT16 *R )
{
	DDCRET retcode = DDC_SUCCESS;
	UINT8          x[2];
	INT16          y[2];
	
	switch ( wave_format.data.nBitsPerSample )
	{
	case 8:
		retcode = Read ( x, 2 );
		*L = INT16 ( x[0] );
		*R = INT16 ( x[1] );
		break;
		
	case 16:
		retcode = Read ( y, 4 );
		*L = INT16 ( y[0] );
		*R = INT16 ( y[1] );
		break;
		
	default:
		retcode = DDC_INVALID_CALL;
	}
	
	return retcode;
}


DDCRET WaveFile::SeekToSample ( unsigned long SampleIndex )
{
	if ( SampleIndex >= NumSamples() )
	{
		return DDC_INVALID_CALL;
	}
	
	unsigned SampleSize = (BitsPerSample() + 7) / 8;
	
	DDCRET rc = Seek ( pcm_data_offset + sizeof(pcm_data) +
		SampleSize * NumChannels() * SampleIndex );
	
	return rc;
}


UINT32 WaveFile::SamplingRate() const
{
	return wave_format.data.nSamplesPerSec;
}


UINT16 WaveFile::BitsPerSample() const
{
	return wave_format.data.nBitsPerSample;
}


UINT16 WaveFile::NumChannels() const
{
	return wave_format.data.nChannels;
}


UINT32 WaveFile::NumSamples() const
{
	return num_samples;
}


DDCRET WaveFile::WriteData ( const INT16 *data, UINT32 numData )
{
	UINT32 extraBytes = numData * sizeof(INT16);
	pcm_data.ckSize += extraBytes;
	return RiffFile::Write ( data, extraBytes );
}


DDCRET WaveFile::WriteData ( const UINT8 *data, UINT32 numData )
{
	pcm_data.ckSize += numData;
	return RiffFile::Write ( data, numData );
}


DDCRET WaveFile::ReadData ( INT16 *data, UINT32 numData )
{
	return RiffFile::Read ( data, numData * sizeof(INT16) );
}


DDCRET WaveFile::ReadData ( UINT8 *data, UINT32 numData )
{
	return RiffFile::Read ( data, numData );
}


/*--- end of file riff.cpp ---*/
