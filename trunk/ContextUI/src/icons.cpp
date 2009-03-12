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

#include "icons.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"

#include <contextcommon.mbg>

#ifdef __S60V3__
#include <avkonicons.hrh>
#include <akniconutils.h>
#endif
#include <bautils.h>
#include <eikenv.h> 
#include <gulicon.h>
#include <s32file.h>


EXPORT_C TInt GetIconBitmap(TInt index, const TIconID* aIconDefs, TInt aNbIcons)
{
	return aIconDefs[index].iBitmap;
}

EXPORT_C TInt GetIconMask(TInt index, const TIconID* aIconDefs, TInt aNbIcons)
{
	return aIconDefs[index].iMask;
}

EXPORT_C TPtrC GetIconMbm(TInt index, const TIconID* aIconDefs, TInt aNbIcons)
{
	return TPtrC((TText16*)aIconDefs[index].iMbmFile);
}

void GetIconInfo(const TDesC& FileName, RFs& Fs, RFile& File, RFileReadStream& s, RArray<TInt>& Offsets, int round)
{
	TBuf<50> c=_L("GetIconInfo round "); c.AppendNum(round);

	TInt table_offset=0;
	{
		TInt err=File.Open(Fs, FileName, EFileRead|EFileShareAny);
		if (err!=KErrNone) {
			err=File.Open(Fs, FileName, EFileRead);
		}
		User::LeaveIfError(err);
		RFile f0=File;
		s.Attach(f0, 0); 
		TInt uid=s.ReadUint32L();
		TUid file_uid={ uid };
		if (file_uid==KMultiBitmapRomImageUid) {
			table_offset=0x4;
		} else {
			uid=s.ReadUint32L(); // uid2
			uid=s.ReadUint32L(); // uid3
			uid=s.ReadUint32L(); // uid4 == checksum
			table_offset=s.ReadUint32L();
		}
	}

	TInt size=0; 
	{
		c=_L("GetIconInfo::2::"); c.AppendNum(table_offset);
		RFile f=File;
		s.Attach(f, table_offset);
		size=s.ReadUint32L();
		Offsets.Reset();
	}

/*
	MApp_context* ctx=GetContext();
	c=_L("offset::"); c.AppendNum(table_offset); c.Append(_L(" ")); 
	c.Append(_L("size::")); c.AppendNum(size); c.Append(_L(" i::0"));
*/
	int i=0;
	for (i=0; i<size; i++) {
		TInt offset=s.ReadUint32L();
		User::LeaveIfError(Offsets.Append(offset));
/*
		c=_L("offset::"); c.AppendNum(table_offset); c.Append(_L(" ")); 
		c.Append(_L("size::")); c.AppendNum(size); c.Append(_L(" i::")); c.AppendNum(i);
*/
	}
}

EXPORT_C void LoadIcons(CArrayPtrFlat<CGulIcon> * aIconList, const TIconID* aIconDefs, 
			TInt aNbIcons)
{
	LoadIcons(aIconList, aIconDefs, aNbIcons, 1);
}

EXPORT_C void ScaleFbsBitmapL(CFbsBitmap * orig, CFbsBitmap * dest, TInt aScale)
{
	TSize s = orig->SizeInPixels();
	TInt width=s.iWidth;
	TInt height=s.iHeight;
	s.iWidth*=aScale;
	s.iHeight*=aScale;
	User::LeaveIfError(dest->Create(s, orig->DisplayMode()));

	TSize twips = orig->SizeInTwips();
	twips.iWidth*=aScale;
	twips.iHeight*=aScale;
	dest->SetSizeInTwips(twips);

	TBitmapUtil orig_util(orig);
	TBitmapUtil dest_util(dest);
	orig_util.Begin(TPoint(0,0)); 
	dest_util.Begin(TPoint(0,0), orig_util); 

	TInt xPos;
	for (TInt yPos=0;yPos<height;yPos++) {
		for (TInt j=0; j<aScale; j++) {
			orig_util.SetPos(TPoint(0,yPos));
			dest_util.SetPos(TPoint(0,yPos*aScale+j));
			for (xPos=0;xPos<width;xPos++) {
				for (TInt k=0; k<aScale; k++) {
					dest_util.SetPixel(orig_util);
					dest_util.IncXPos();
				}
				orig_util.IncXPos();
			}
		}
	}
	orig_util.End();
	dest_util.End();
}

EXPORT_C void LoadIcons(CArrayPtrFlat<CGulIcon> * aIconList, const TIconID* aIconDefs, 
			TInt aNbIcons, TInt aScale)
{
	CALLSTACKITEM_N(_CL(""), _CL("LoadIcons"));

	TFileName real; TFileName prev;
	CEikonEnv* env=CEikonEnv::Static();
	RFs& fs=env->FsSession();
	CWsScreenDevice* screen=env->ScreenDevice();
	RWsSession& ws=env->WsSession();
	RArray<TInt> Offsets; RFile File; bool file_is_open=false;
	CleanupClosePushL(Offsets);
	RFileReadStream s;
	int j=0;

	bool romfile=false;
	for (int i = 0; i<aNbIcons;i++)
	{
		TPtrC file((TText16*)aIconDefs[i].iMbmFile);
#ifdef __S60V3__
		if (file.FindF(_L("avkon"))==KErrNotFound) {
#endif
			
		if (prev.Compare(file)) {
#ifndef __S60V3__
			real=file;
#else
			TParse p; p.Set(file, 0, 0);
			real=_L("c:\\resource\\");
			real.Append(p.NameAndExt());				
				
#endif

#ifdef __WINS__
			real.Replace(0, 1, _L("z"));
#else
			if (! BaflUtils::FileExists(fs, real)) {
				real.Replace(0, 1, _L("e"));
			}
#endif
			prev=file;
			if (file_is_open) {
				s.Close();
				file_is_open=false;
			}
			if (real.Left(1).CompareF(_L("z"))==0) {
				romfile=true;
			} else {
				romfile=false;
				GetIconInfo(real, fs, File, s, Offsets, j);
				file_is_open=true;
			}
			++j;
		}

		auto_ptr<CWsBitmap> bitmap(new (ELeave) CWsBitmap(ws));
		if (!romfile) {
			RFile f1=File;
			s.Attach(f1, Offsets[aIconDefs[i].iBitmap]);	
			bitmap->InternalizeL(s);
		} else {
			TInt err=bitmap->Load(real, aIconDefs[i].iBitmap);
			if (err!=KErrNone) 
				User::Leave(err);
		}
		bitmap->SetSizeInTwips(screen);
#ifdef __S60V2__
		if (aScale>1) {
			auto_ptr<CWsBitmap> scaled(new (ELeave) CWsBitmap(ws));
			ScaleFbsBitmapL(bitmap.get(), scaled.get(), aScale);
			bitmap=scaled;
		}
#endif
		
		auto_ptr<CWsBitmap> mask(NULL);
		if ( aIconDefs[i].iMask != KErrNotFound )
			{
				mask.reset(new (ELeave) CWsBitmap(ws));
				if (!romfile) {
					RFile f2=File;
					s.Attach(f2, Offsets[aIconDefs[i].iMask]);
					mask->InternalizeL(s);
				} else {
					User::LeaveIfError(mask->Load(real, aIconDefs[i].iMask));
				}
				mask->SetSizeInTwips(screen);
#ifdef __S60V2__
				if (aScale>1) {
					auto_ptr<CWsBitmap> scaled(new (ELeave) CWsBitmap(ws));
					ScaleFbsBitmapL(mask.get(), scaled.get(), aScale);
					mask=scaled;
				}
#endif
			}

		auto_ptr<CGulIcon> icon(CGulIcon::NewL(bitmap.get(), mask.get()));
		bitmap.release(); mask.release();
		aIconList->AppendL(icon.get()); icon.release();
#ifdef __S60V3__
		} else {
			auto_ptr<CFbsBitmap> bitmap(0);
			auto_ptr<CFbsBitmap> mask(0);
			if ( aIconDefs[i].iMask != KErrNotFound ) {
				CFbsBitmap *bitmapp=0, *maskp=0;
				AknIconUtils::CreateIconL(bitmapp, maskp, AknIconUtils::AvkonIconFileName(), aIconDefs[i].iBitmap, aIconDefs[i].iMask);
				bitmap.reset(bitmapp);
				mask.reset(maskp);
			} else {
				bitmap.reset(AknIconUtils::CreateIconL(AknIconUtils::AvkonIconFileName(), aIconDefs[i].iBitmap));
			}
			bitmap->SetSizeInTwips(screen);
			if (mask.get()) mask->SetSizeInTwips(screen);
			auto_ptr<CGulIcon> icon(CGulIcon::NewL(bitmap.get(), mask.get()));
			bitmap.release(); mask.release();
			aIconList->AppendL(icon.get()); icon.release();
		}
#endif
	}
	if (file_is_open) {
		s.Close();
	}
	CleanupStack::PopAndDestroy(); // Offsets
}

EXPORT_C TInt GetIconIndex(TInt identifier, const TIconID* aIconDefs, TInt aNbIcons)
{
	for (int i = 0; i<aNbIcons;i++)
	{
		if ( aIconDefs[i].iBitmap == identifier )
		{
			return i;
		}
	}	
	return 0; // index of empty icon
}
