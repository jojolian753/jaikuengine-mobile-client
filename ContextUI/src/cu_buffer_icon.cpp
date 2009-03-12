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

#include "cu_buffer_icon.h"
#include <s32mem.h>
#include <fbs.h>
#include <eikenv.h>
#include "symbian_auto_ptr.h"
#include <bitmap.h>
#include <s32file.h>

TInt TableOffset(const TDesC8& aBuffer)
{
	TInt table_offset=0;
	{
		RMemReadStream s(aBuffer.Ptr(), aBuffer.Length());
		TInt uid=s.ReadUint32L();
		TUid file_uid={ uid };
		if (file_uid==KMultiBitmapRomImageUid) {
			table_offset=0x4;
		} else if ( uid==KDirectFileStoreLayoutUidValue) {
			uid=s.ReadUint32L(); // uid2
			if ( TUid::Uid(uid) != KMultiBitmapFileImageUid) User::Leave(KErrCorrupt);
			uid=s.ReadUint32L(); // uid3
			uid=s.ReadUint32L(); // uid4 == checksum
			table_offset=s.ReadUint32L();
		} else {
			User::Leave(KErrCorrupt);
		}
	}
	return table_offset;
}
EXPORT_C TInt NumberOfBitmapsL(const TDesC8& aBuffer)
{
	TInt table_offset=TableOffset(aBuffer);
	TInt size=0; 
	{
		if (table_offset > aBuffer.Length()) User::Leave(KErrCorrupt);
		RMemReadStream s(aBuffer.Ptr()+table_offset, aBuffer.Length()-table_offset);
		size=s.ReadUint32L();
	}
	return size;
}

EXPORT_C class CFbsBitmap* LoadBitmapL(const TDesC8& aBuffer, TInt aIconNumber)
{
	CEikonEnv* env=CEikonEnv::Static();
	RWsSession& ws=env->WsSession();
	CWsScreenDevice* screen=env->ScreenDevice();	
	return LoadBitmapL(aBuffer, aIconNumber, ws, screen);
}

EXPORT_C class CFbsBitmap* LoadBitmapL(const TDesC8& aBuffer, TInt aIconNumber,
	RWsSession& ws, CWsScreenDevice* screen) 
{
	TInt table_offset=TableOffset(aBuffer);
	TInt offset=0;
	{
		if (table_offset > aBuffer.Length()) User::Leave(KErrCorrupt);
		RMemReadStream s(aBuffer.Ptr()+table_offset, aBuffer.Length()-table_offset);
		TInt size=s.ReadUint32L();
		if (aIconNumber<0 || aIconNumber>=size) User::Leave(KErrArgument);
		TInt i=0;
		while (i<=aIconNumber) {
			offset=s.ReadUint32L();
			i++;
		}
	}
	auto_ptr<CWsBitmap> bitmap(new (ELeave) CWsBitmap(ws));
	{

		if (offset > aBuffer.Length()) User::Leave(KErrCorrupt);
		RMemReadStream s(aBuffer.Ptr()+offset, aBuffer.Length()-offset);
		bitmap->InternalizeL(s);
		bitmap->SetSizeInTwips(screen);
	}
	return bitmap.release();
}
