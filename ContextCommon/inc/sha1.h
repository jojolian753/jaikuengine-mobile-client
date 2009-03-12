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

#ifndef _SHA1_H_
#define _SHA1_H_

#include <e32base.h>

IMPORT_C void DoSHA1(const TDesC& aMessage, TDes& digest);

class SHA1 : public CBase
{

	public:

		IMPORT_C SHA1();
		IMPORT_C virtual ~SHA1();

		/*
		 *	Re-initialize the class
		 */
		IMPORT_C void Reset();

		/*
		 *	Returns the message digest
		 */
		IMPORT_C bool Result(unsigned *message_digest_array);

		/*
		 *	Provide input to SHA1
		 */
		IMPORT_C void Input(	const unsigned char	*message_array,
					unsigned			length);
		IMPORT_C void Input(	const char	*message_array,
					unsigned	length);
		IMPORT_C void Input(unsigned char message_element);
		IMPORT_C void Input(char message_element);
		IMPORT_C SHA1& operator<<(const char *message_array);
		IMPORT_C SHA1& operator<<(const unsigned char *message_array);
		IMPORT_C SHA1& operator<<(const char message_element);
		IMPORT_C SHA1& operator<<(const unsigned char message_element);

	private:

		/*
		 *	Process the next 512 bits of the message
		 */
		void ProcessMessageBlock();

		/*
		 *	Pads the current message block to 512 bits
		 */
		void PadMessage();

		/*
		 *	Performs a circular left shift operation
		 */
		inline unsigned CircularShift(int bits, unsigned word);

		unsigned H[5];						// Message digest buffers

		unsigned Length_Low;				// Message length in bits
		unsigned Length_High;				// Message length in bits

		unsigned char Message_Block[64];	// 512-bit message blocks
		int Message_Block_Index;			// Index into message block array

		bool Computed;						// Is the digest computed?
		bool Corrupted;						// Is the message digest corruped?
	
};


#endif
