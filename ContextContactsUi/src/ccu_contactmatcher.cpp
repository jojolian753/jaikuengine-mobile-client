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

#include "ccu_contactmatcher.h"

#include "app_context_impl.h"
#include "symbian_auto_ptr.h"
#include "md5.h"

#include <cntdef.h>
#include <cpbkcontactitem.h>
#include <CPbkContactIter.h> 
#include <cpbkcontactengine.h>
#include <tpbkcontactitemfield.h>
#include <pbkfields.hrh>
#include "phonebook_static.h"

class CContactMatcherImpl : public CContactMatcher, public MContextBase {
public:
	typedef CContactMatcher::THash THash;

	static CContactMatcherImpl* NewL(TBool aReadNumbers);
	~CContactMatcherImpl();

	CContactIdArray* FindMatchesForHashedNumberL( const THash& aHash );
	CContactIdArray* FindMatchesForNumberL( const TDesC& aPhoneNumber );
	CContactIdArray* FindMatchesForEmailL( const TDesC& aEmail );

	   	
private:
	CContactMatcherImpl();
	void ConstructL(TBool aReadNumbers);

	void ConstructContactEngineL();

	virtual void Reset();
	void AddPhoneNumbersToMapL(CPbkContactItem& aItem);
	void AddPhoneHashToMapL(TInt aContactId, const TDesC8& aHash);
	void BuildMapL();

private:
	CPbkContactEngine* Engine() { 
		if (!iMyEngine) ConstructContactEngineL();
		return iMyEngine; 
	}
	
	class THashAndContactId
	{
	public:
		THash iHash;
		TContactItemId iId;
	};
	
	CArrayFixFlat<THashAndContactId>* iHashMap;
	
	CPbkContactEngine* iMyEngine;
	TBool iOwnsEngine;	
};



//    Thin interface class 

EXPORT_C CContactMatcher* CContactMatcher::NewL(TBool aReadNumbers)
{
	CALLSTACKITEMSTATIC_N(_CL("CContactMatcher"), _CL("NewL"));
	return CContactMatcherImpl::NewL(aReadNumbers);
}



//    Implementation class 


CContactMatcherImpl* CContactMatcherImpl::NewL(TBool aReadNumbers)
{
	CALLSTACKITEMSTATIC_N(_CL("CContactMatcherImpl"), _CL("NewL"));
	auto_ptr<CContactMatcherImpl> self( new (ELeave) CContactMatcherImpl() );
	self->ConstructL(aReadNumbers);
	return self.release();
}


CContactMatcherImpl::CContactMatcherImpl()
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("CContactMatcherImpl"));
}


void CContactMatcherImpl::ConstructContactEngineL()
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("ConstructContactEngineL"));
	// Get engine.
	iMyEngine = CPbkContactEngine::Static();
	if ( ! iMyEngine )
		{
			RFs& fs = Fs();
			iMyEngine = CPbkContactEngine::NewL( &fs );
			GetContext()->TakeOwnershipL( iMyEngine );
		}
}


void CContactMatcherImpl::ConstructL(TBool aReadNumbers)
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("ConstructL"));

	iHashMap = new (ELeave) CArrayFixFlat<THashAndContactId>(100);
	if (aReadNumbers) BuildMapL();
}


CContactMatcherImpl::~CContactMatcherImpl()
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("~CContactMatcherImpl"));
	delete iHashMap;
}


CContactIdArray* CContactMatcherImpl::FindMatchesForHashedNumberL( const THash& aHash )
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("FindMatchesForHashedNumberL"));
	auto_ptr<CContactIdArray> result( CContactIdArray::NewL() );
		
	for ( TInt i = 0; i < iHashMap->Count(); i++)
		{
			THashAndContactId& h = iHashMap->At(i);
			if ( aHash == h.iHash )
				{
					result->AddL( h.iId );
				}
		}
	
	return result.release();
}


CContactIdArray* CContactMatcherImpl::FindMatchesForEmailL( const TDesC& aEmail )
{	
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("FindMatchesForEmailL"));
	THash hash;
	HashStringL( aEmail, hash );
	return FindMatchesForHashedNumberL( hash );
}


CContactIdArray* CContactMatcherImpl::FindMatchesForNumberL( const TDesC& aPhoneNumber )
{	
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("FindMatchesForNumberL"));
	THash hash;
	HashPhoneNumberL( aPhoneNumber, hash );
	return FindMatchesForHashedNumberL( hash );
}


EXPORT_C void CContactMatcher::HashPhoneNumberL( const TDesC& aNumber, TDes8& aHash )
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("HashPhoneNumberL"));
	::HashPhoneNumberL(aNumber, aHash);
}


EXPORT_C void CContactMatcher::HashStringL( const TDesC& aString, TDes8& aHash)
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("HashStringL"));
	::HashStringL( aString, aHash);
}

void CContactMatcherImpl::AddPhoneHashToMapL(TInt aContactId, const TDesC8& aHash)
{
	THashAndContactId h;
	h.iId=aContactId;
	h.iHash=aHash;
	iHashMap->AppendL(h);
}

void CContactMatcherImpl::AddPhoneNumbersToMapL(CPbkContactItem& aItem)
{
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("AddPhoneNumbersToMapL"));

	const TInt* fieldIds=PhoneNumberFields();
		
	for ( TInt i=0; fieldIds[i] != EPbkFieldIdNone; i++ ) {
		THashAndContactId h;
		if (GetPhoneNumberHash(&aItem, fieldIds[i], h.iHash)) {
			h.iId = aItem.Id();
			iHashMap->AppendL( h );
		}
	}
}

void CContactMatcherImpl::Reset()
{
	iHashMap->Reset();
}

void CContactMatcherImpl::BuildMapL()
{	
	CALLSTACKITEM_N(_CL("CContactMatcherImpl"), _CL("BuildMapL"));
	Reset();
	CPbkContactIter* iterx = Engine()->CreateContactIteratorLC(ETrue);
	CleanupStack::Pop( iterx );
	auto_ptr<CPbkContactIter> iter( iterx );
	
	CPbkContactItem* item=NULL; 
	for (iter->FirstL(); (item=iter->CurrentL()); iter->NextL()) 
		{
			AddPhoneNumbersToMapL( *item );
		}
}
