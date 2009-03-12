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

#include "phonebook_static.h"
#include <cpbkcontactengine.h> 
#include <cpbkcontactchangenotifier.h>
#include "md5.h"
#include <TPbkContactItemField.h>
#include <CPbkContactItem.h>
#include "app_context.h"

struct TFieldPref {
	TInt	iFieldId1;
	TInt	iFieldId2;
	TBool	iUsed;
};

void ResetUsed(TFieldPref* p) {
	while (p->iFieldId1) {
		p->iUsed=EFalse;
		p++;
	}
}
void SetAsUsed(TFieldPref* p, TInt aFieldId) {
	while (p->iFieldId1) {
		if (p->iFieldId1 == aFieldId || p->iFieldId2 == aFieldId) {
			p->iUsed=ETrue;
			break;
		}
		p++;
	}
}

void GetPreferredField(TFieldPref* p, CPbkContactItem *item, TDes& aInto)
{
	aInto.Zero();
	while (p->iFieldId1) {
		if (! p->iUsed ) {
			TPbkContactItemField *f1=0, *f2=0;
			f1=item->FindField(p->iFieldId1);
			if (p->iFieldId2) f2=item->FindField(p->iFieldId2);
			if (f2) {
				if (f2->PbkFieldType() != KStorageTypeText) f2=0;
				if (f2 && f2->IsEmptyOrAllSpaces()) f2=0;
			}
			if (f1) {
				if (f1->PbkFieldType() != KStorageTypeText) f1=0;
				if (f1 && f1->IsEmptyOrAllSpaces()) f1=0;
			}
			if (f1) {
				aInto=f1->Text().Left(aInto.MaxLength());
				aInto.Trim();
				if (f1 && f2 && aInto.MaxLength()-aInto.Length()>3) {
					aInto.Append(_L(", "));
				}
			}
			if (f2) {
				aInto.Append( f2->Text().Left( 
					aInto.MaxLength() - aInto.Length() ) );
				aInto.TrimRight();
			}
			if (f1 || f2) {
				p->iUsed=ETrue;
				return;
			}
		}
		p++;
	}
}


void GetNamesForContactL(class CPbkContactItem* item, TDes& first_name, TDes& last_name, TDes& extra_name)
{
    TFieldPref preferred[] = {
	{ EPbkFieldIdSecondName, 0, 0 },
	{ EPbkFieldIdJobTitle, EPbkFieldIdCompanyName, 0 },
	{ EPbkFieldIdPhoneNumberMobile, 0, 0 },
	{ EPbkFieldIdPhoneNumberStandard, 0, 0 },
	{ EPbkFieldIdPhoneNumberHome, 0, 0 },
	{ EPbkFieldIdPhoneNumberWork, 0, 0 },
	{ EPbkFieldIdEmailAddress, 0, 0 },
	{ EPbkFieldIdURL, 0, 0 },
	{ 0, 0, 0 }
    };
    
#if 1
    TPbkContactItemField* f;
    
    f=item->FindField(EPbkFieldIdLastName);
    if (f && f->PbkFieldType() == KStorageTypeText) {
	last_name=f->Text();
    } else {
	last_name=_L("");
    }
    f=item->FindField(EPbkFieldIdFirstName);
    if (f && f->PbkFieldType() == KStorageTypeText) {
	first_name=f->Text();
    } else {
	first_name=_L("");
    }
    
    ResetUsed(preferred);
    if (first_name.Length()==0 && last_name.Length()==0) {
	GetPreferredField(preferred, item, last_name);
    }
    if (first_name.Length()==0 && last_name.Length()==0) 
	{
	    CPbkFieldArray& fields=item->CardFields();
	    for (int f_i=0; f_i < fields.Count(); f_i++) {
		const TPbkContactItemField& f=fields.At(f_i);
		if ( f.PbkFieldType() == KStorageTypeText ) {
		    last_name=f.Text();
		    SetAsUsed(preferred, f.PbkFieldId());
		    break;
		}
	    }			
	}
    GetPreferredField(preferred, item, extra_name);
#else
    auto_ptr<HBufC> title( eng->GetContactTitleL(*item) );
    last_name=*title;
#endif

}

void HashPhoneNumberL( const TDesC& aNumber, TDes8& aHash )
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("HashPhoneNumberL"));
	const TInt KNumbersToMatchFromRight = 8;
	HashStringL( aNumber.Right(KNumbersToMatchFromRight), aHash );
}


void HashStringL( const TDesC& aString, TDes8& aHash)
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("HashStringL"));
	if (aHash.MaxLength()<16) User::Leave(KErrOverflow);
	const TInt KMax = 100; 
	ASSERT( aString.Length() <= KMax );
	aHash.SetLength( aHash.MaxLength() );
	TBuf8<KMax> buf;
	buf.Copy( aString );
	
	MD5_CTX md5;
	MD5Init(&md5);
	MD5Update(&md5, (TUint8*)buf.Ptr(), buf.Length());
	MD5Final( (TUint8*) aHash.Ptr(), &md5 );
}

const TInt fieldIds[] = { EPbkFieldIdPhoneNumberGeneral,
						  EPbkFieldIdPhoneNumberHome,
						  EPbkFieldIdPhoneNumberWork,  		
						  EPbkFieldIdPhoneNumberMobile,
						  EPbkFieldIdEmailAddress,
						  EPbkFieldIdNone }; // Terminal item, this has to be last

const TInt* PhoneNumberFields()
{
	return fieldIds;
}

TBool GetPhoneNumberHash(class CPbkContactItem* aItem, TInt aField, TDes8& aHashInto)
{
	if (! aItem ) return EFalse;
	
	TPbkContactItemField* field = aItem->FindField( aField );
	if ( field ) {
		HashPhoneNumberL( field->Text(), aHashInto );
		return ETrue;
	}
	return EFalse;
}
