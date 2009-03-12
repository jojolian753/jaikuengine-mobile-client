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

#include "cu_shortcutreplacer.h"

#include "raii_s32file.h"
#include "raii_f32file.h"
#include "symbian_auto_ptr.h"

#include <bautils.h>
#include <s32mem.h>
#include <eikfutil.h>

_LIT(KShortcutFile, "c:\\system\\data\\ScShortcutEngine.ini");
_LIT(KN70ShortcutFile, "\\system\\data\\context\\N70shortcut.ini");
const TUid KN70TemplateUid = { 0x20006E48 };


EXPORT_C CShortcutReplacer* CShortcutReplacer::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CShortcutReplacer"), _CL("NewL"));
	return new (ELeave) CShortcutReplacer();
}


EXPORT_C CShortcutReplacer::~CShortcutReplacer()
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("~CShortcutReplacer"));
} 


TInt FileSizeL(RFs& fs, const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("~CShortcutReplacer"));
	RAFile file;
	file.OpenLA( fs, aFileName, EFileRead|EFileShareAny);
	
	TInt size = 0;
	User::LeaveIfError( file.Size(size) );
	return size;
}


HBufC8* CShortcutReplacer::ReadDataL(const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("ReadDataL"));

	TInt size = FileSizeL( Fs(), aFileName);
	auto_ptr<HBufC8> buf( HBufC8::NewL( 2 * size ) ); // extra size

	RAFile file2;
	file2.OpenLA( Fs(), aFileName, EFileRead|EFileShareAny);

	TBuf8<100> tmp; 
	TInt err = file2.Read(tmp);
	while (err == KErrNone && tmp.Length() > 0 )
		{ 
			buf->Des().Append(tmp);
			tmp.Zero();
			err= file2.Read(tmp);
		}
	return buf.release();
}


void CShortcutReplacer::WriteDataL(const TDesC& aFileName, const TDesC8& aData)
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("WriteDataL"));
	RAFileWriteStream stream;
	stream.OpenLA( Fs(), aFileName, EFileWrite);
	stream.WriteL( aData );  
}


void CShortcutReplacer::ReplaceDataL(const TDesC8& aOriginal, const TDesC8& aNew, const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("ReplaceDataL"));
	auto_ptr<HBufC8> buf( ReadDataL(aFileName) );
  
	TPtr8 ptr = buf->Des();

	TInt pos = 0;
	while (pos >= 0)
		{
			TInt ix = buf->Mid(pos).Find( aOriginal );
			if ( ix >= 0 )
				{
					pos += ix;
					buf->Des().Replace(pos, aOriginal.Length(), aNew);
				}
			else
				{
					pos = ix;
				}
		}
  
	WriteDataL( aFileName, *buf );
}


void CShortcutReplacer::UidToStringL(const TUid& aUid, TDes8& aTgt)
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("UidToStringL"));
	TUidName name = aUid.Name();
	aTgt.Zero();
	
	RDesWriteStream stream( aTgt );
	stream.WriteInt32L( aUid.iUid );
	stream.Close();
}


EXPORT_C void CShortcutReplacer::ReplaceShortcutL(const TUid& aOriginal, const TUid& aNew)
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("ReplaceShortcutL"));
	ReplaceShortcutL( aOriginal, aNew, KShortcutFile );
}


EXPORT_C void CShortcutReplacer::ReplaceShortcutL(const TUid& aOriginal, const TUid& aNew, const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CShortcutReplacer"), _CL("ReplaceShortcutL"));
	TUid tobereplaced = aOriginal;

	TBool useTemplateFile = EFalse;
	if ( BaflUtils::FileExists(Fs(), aFileName) )
		{
			TInt size = FileSizeL(Fs(), aFileName);
			if ( size == 0) useTemplateFile = ETrue;
			else {
				TFileName* bak=new (ELeave) TFileName;
				*bak=aFileName;
				bak->Replace(bak->Length()-3, 3, _L("bak"));
				EikFileUtils::CopyFile(aFileName, *bak);
				delete bak;
			}
		}
	else 
		{
			useTemplateFile = ETrue;
		}
	
	// Prevent template file use. Not safe with different phonemodels.
	//useTemplateFile = EFalse;
	if ( useTemplateFile )
		{
			EikFileUtils::CopyFile(KN70ShortcutFile, aFileName);
			tobereplaced = KN70TemplateUid;
		}
	
	if ( tobereplaced == aNew )
	{
		return;
	}
	TBuf8<8> orig;
	TBuf8<8> thenew;
	UidToStringL( tobereplaced, orig );
	UidToStringL( aNew, thenew );
	ReplaceDataL( orig, thenew, aFileName );
}
