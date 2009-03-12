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

#ifndef __DDC_DDC_H
#define __DDC_DDC_H

// If you add something to DDCRET, please add the appropriate string
// to the function DDCRET_String() in the file 'source\ddcret.cpp'.

enum DDCRET
{
   DDC_SUCCESS,           // The operation succeded
   DDC_FAILURE,           // The operation failed for unspecified reasons
   DDC_OUT_OF_MEMORY,     // Operation failed due to running out of memory
   DDC_FILE_ERROR,        // Operation encountered file I/O error
   DDC_INVALID_CALL,      // Operation was called with invalid parameters
   DDC_USER_ABORT,        // Operation was aborted by the user
   DDC_INVALID_FILE       // File format does not match
};


const char *DDCRET_String ( DDCRET );   // See source\ddcret.cpp


#define  TRUE     1
#define  FALSE    0

typedef int dBOOLEAN;

typedef unsigned char BYTE;

typedef unsigned char        UINT8;
typedef signed   char        INT8;

typedef unsigned short int   UINT16;
typedef signed   short int   INT16;
typedef unsigned long  int   UINT32;
typedef signed   long  int   INT32;

#ifdef __BORLANDC__
	#if sizeof(UINT16) != 2
	  #error Need to fix UINT16 and INT16
	#endif

	#if sizeof(UINT32) != 4
	  #error Need to fix UINT32 and INT32
	#endif
#endif

#endif /* __DDC_DDC_H */

/*--- end of file ddc.h ---*/
