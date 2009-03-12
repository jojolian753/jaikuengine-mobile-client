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

#include "jabberpics.h"
#include "symbian_auto_ptr.h"
#include "raii_d32dbms.h"
#include <fbs.h>
#include <badesca.h>
#include "jabberdata.h"
#include "break.h"

enum TColumns {
	ENick=1,
	EImage
};

enum TIndices {
	EIndexNick = 0
};


// Class methods 

EXPORT_C CJabberPics* CJabberPics::NewL(CDb& aDb)
{
	auto_ptr<CJabberPics> ret(new (ELeave) CJabberPics(aDb));
	ret->ConstructL();
	return ret.release();
}


EXPORT_C CJabberPics::~CJabberPics()
{	
}


CJabberPics::CJabberPics(CDb& Db) 
	: MDBStore(Db.Db()),
	  iJabberDb(Db)
{
}


void CJabberPics::ConstructL()
{
	// Columns:   ENick,       EImage (blob)
	TInt cols[]={ EDbColText, EDbColLongBinary , -1 };
	// Indexed by:
	TInt idx[]= { ENick, -1 };

	TBool canAlter=EFalse;
	if (iJabberDb.iFileMode & EFileWrite) canAlter=ETrue;
	MDBStore::ConstructL(cols, idx, ETrue, _L("PIC"), canAlter);
	
	RemoveDuplicatePicsL();
}


EXPORT_C void CJabberPics::SetPicL(const TDesC& aNick, CFbsBitmap& aBitmap,
	TBool &aIsNewInto)
{
	// First, check if old nick format (teemu) exists and remove image for it
	iNickBuffer=aNick;
	TInt at = iNickBuffer.Locate('@');
	if (at >= 0) iNickBuffer = iNickBuffer.Left(at);
	
	if ( FindL( iNickBuffer, EFalse ) )
		{
			iTable.GetL();
			DeleteL();
		}
	CJabberData::CanonizeNickL( iNickBuffer );

	// Then insert new image for the new format (teemu@jaiku.com)
	if ( FindL( iNickBuffer ) )
		{
			iTable.UpdateL();
			aIsNewInto=EFalse;
		}	
	else
		{
			iTable.InsertL();
			iTable.SetColL(ENick, iNickBuffer);
			aIsNewInto=ETrue;
		}	
	
	{
		RADbColWriteStream w; 
		w.OpenLA(iTable, EImage);
		aBitmap.ExternalizeL(w);
		w.CommitL();	
	}

	MDBStore::PutL();
}


EXPORT_C CFbsBitmap* CJabberPics::GetPicL(const TDesC& aNick)
{
	if ( FindL(aNick) )
		{
			return GetCurrentPicL();
		}
	else
		{
			return NULL;
		}
}




EXPORT_C TBool CJabberPics::FindL(const TDesC& aNick, TBool aCanonize)
{
	CJabberData::TNick nick( aNick );
	if ( aCanonize )
		{
			CJabberData::CanonizeNickL( nick );
		}
	
	SwitchIndexL(EIndexNick);
	TDbSeekKey rk( nick );
	return iTable.SeekL(rk);
}


CFbsBitmap* CJabberPics::GetCurrentPicL()
{	
	RADbColReadStream stream;
	if (GetPicDataL( stream )) {
		auto_ptr<CFbsBitmap> bmp( new (ELeave) CFbsBitmap() );
		bmp->InternalizeL( stream );
		return bmp.release();
	} else {
		return 0;
	}
}

TBool CJabberPics::GetPicDataL(RADbColReadStream& aDataInto)
{
	iTable.GetL();
	if (iTable.IsColNull(EImage)) {
		return EFalse;
	} else {
		aDataInto.OpenLA(iTable, EImage);
		return ETrue;
	}
}


EXPORT_C void CJabberPics::GetAllPicsL(CDesCArray& aNicks, CArrayPtr<CFbsBitmap>& aPics)
{
		
 	if (! iTable.FirstL() ) return;

	do {		
		iTable.GetL();
		TPtrC nick( iTable.ColDes(ENick) );
		auto_ptr<CFbsBitmap> bmp( GetCurrentPicL() );

		if (bmp.get()) {
			aNicks.AppendL( nick );
			aPics.AppendL( bmp.get() );
			bmp.release();
		}
		
	} while ( iTable.NextL() );
	
}



EXPORT_C void CJabberPics::GetAllNicksL(CDesCArray& aNicks)
{
		
 	if (! iTable.FirstL() ) return;

	do {		
		iTable.GetL();
		TPtrC nick( iTable.ColDes(ENick) );
		aNicks.AppendL( nick );
	} while ( iTable.NextL() );	
}




_LIT(KAtJaikuDotCom, "@jaiku.com");

void CJabberPics::RemoveDuplicatePicsL()
{
	// for each nick in format teemu, which has also pic in format teemu@jaiku
	// 
	auto_ptr<CDesCArray> oldNicks( new (ELeave) CDesCArrayFlat(10) );

 	if (! iTable.FirstL()) return;
	
	TBuf<50> lc;
	do {		
		iTable.GetL();
		TPtrC nick( iTable.ColDes(ENick) );
		if ( nick.Locate('@') == KErrNotFound ) {
			oldNicks->AppendL( nick );
		} 
		lc=nick;
		lc.LowerCase();
		if (lc.Compare(nick)!=0) {
			iTable.UpdateL();
			iTable.SetColL(ENick, lc);
			TInt err;
			CC_TRAP(err, PutL());
			if (err!=KErrNone) {
				iTable.Cancel();
				if (err==KErrAlreadyExists) {
					DeleteL();
				}
			}
		}
	} while ( iTable.NextL() );
	
	for( TInt i=0; i < oldNicks->Count(); i++)
		{
			CJabberData::TNick newNick( oldNicks->MdcaPoint(i) );
			newNick.Append( KAtJaikuDotCom );
			if ( FindL( newNick, EFalse ) )
				{
					if ( FindL( oldNicks->MdcaPoint(i), EFalse ) )
						{
							DeleteL();
						}
				}
		}
}
