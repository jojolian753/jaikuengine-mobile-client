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

#include "juik_iconmanager.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"
#include "juik_icons.h"
#include "reporting.h"
#include "stringmap.h"

#include <gulicon.h>
#include <akniconarray.h>



class CStaticIconProvider : public CBase, public MContextBase, public MStaticIconProvider
{
public:
	static CStaticIconProvider* NewL( MJuikIconManager& aIconManager, 
									  const TDesC& aFileName, const TIconID2* aIconDefs, TInt aNbIcons )
	{
		CALLSTACKITEMSTATIC_N(_CL("CStaticIconProvider"), _CL("NewL"));
		auto_ptr<CStaticIconProvider> self( new (ELeave) CStaticIconProvider(aIconManager) );
		self->ConstructL(aFileName, aIconDefs, aNbIcons);
		return self.release();
	}


	
	CStaticIconProvider( MJuikIconManager& aIconManager ) : iIconManager( aIconManager ) {}


	~CStaticIconProvider()
	{
		delete iIconFile;
	}

	void ConstructL(const TDesC& aFileName, const TIconID2* aIconDefs, TInt aNbIcons)
	{
		CALLSTACKITEM_N(_CL("CStaticIconProvider"), _CL("ConstructL"));
		iIconDefs = aIconDefs;
		iNbIcons = aNbIcons;
		
		iIconFile = aFileName.AllocL();
		iProviderId = iIconManager.GetNewProviderIdL();

		auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(aNbIcons) );
		JuikIcons::LoadIconsViaFileNameL( icons.get(), *iIconFile, aIconDefs, aNbIcons );
		iIconManager.SetIconsL( iProviderId, *icons );
	}
	

	TInt GetListBoxIndexL(TInt aIconId)
	{
		CALLSTACKITEM_N(_CL("CStaticIconProvider"), _CL("GetListBoxIndexL"));
		return iIconManager.GetListBoxIndexL( iProviderId, JuikIcons::GetIconIndex(aIconId, iIconDefs, iNbIcons));
	}
	

	CGulIcon* GetIconL(TInt aIconId)
	{
		return iIconManager.GetIconArray()->At( GetListBoxIndexL( aIconId ) );
	}

private:
	MJuikIconManager& iIconManager;
	TInt iProviderId;
	HBufC* iIconFile;
	const TIconID2* iIconDefs; 
	TInt iNbIcons;

};


static void DeleteStaticIconProvider( void* ptr )
{
	CStaticIconProvider* p = static_cast<CStaticIconProvider*>(ptr);
	delete p;
}


class CJuikIconManagerImpl : public CJuikIconManager, public MContextBase
{
public:
 	virtual ~CJuikIconManagerImpl();
	
	TInt GetNewProviderIdL();
	
	void LoadStaticIconsL(const TDesC& aFileName, const TIconID2* aIconDefs, TInt aNbIcons);
	MStaticIconProvider* GetStaticIconProviderL(const TDesC& aIconFile);

	/**
	 * Ownership of icons transferred. Sets NULL pointers to array  
	 */
	void LoadIconsL(TInt aProviderId, const TIconID* aIconDefs, TInt aNbIcons);
	void SetIconsL(TInt aProviderId, CArrayPtr<CGulIcon>& aIcons); 

	CArrayPtr<CGulIcon>* GetProviderIconsL(TInt aProviderId); 	
	TInt AddIconL(TInt aProviderId, CGulIcon* aIcon);
	void ReplaceIconL(TInt aProviderId, TInt aIconId, CGulIcon* aIcon);
	void DeleteIconL(TInt aProviderId, TInt aIconId);
	void RemoveProviderIconsL(TInt aProvider);

	TInt GetListBoxIndexL(TInt aProviderId, TInt aIconId);	


	CArrayPtr<CGulIcon>* GetIconArray() const;


private:
	CJuikIconManagerImpl();
	void ConstructL();
	
	TInt LastIndex(CArrayFixBase& aArray) const;
	void AssertIndexL(CArrayFixBase& aArray, TInt aIndex) const;
	CArrayFix<TInt>& ProviderIconIndexL(TInt aProviderId) const;

	TInt FindEmptySlotL();



private:
	CArrayPtr< CArrayFix<TInt> >* iProviderIconIndexes;
	CArrayPtr< CGulIcon >* iIconArray;
	TBool iOwnsIconArray; 

	CGenericStringMap* iStaticProviders;

	friend class CJuikIconManager;

	TInt iEmptySlotCount;
};



EXPORT_C CJuikIconManager* CJuikIconManager::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CJuikIconManager"), _CL("NewL"));
	auto_ptr<CJuikIconManagerImpl> impl( new (ELeave) CJuikIconManagerImpl());
	impl->ConstructL();
	return impl.release();
}

TInt CJuikIconManagerImpl::GetNewProviderIdL()
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("GetNewProviderIdL"));
	TInt providerIx = KErrNotFound;
	for ( TInt i = 0; i < iProviderIconIndexes->Count(); i++)
		{
			if ( iProviderIconIndexes->At(i) == NULL )
				{
					providerIx = i;
					break;
				}
		}
	
	if ( providerIx == KErrNotFound )
		{
			iProviderIconIndexes->AppendL( new (ELeave) CArrayFixFlat<TInt>(3) );
			return LastIndex( *iProviderIconIndexes );
		}
	else
		{
			(*iProviderIconIndexes)[providerIx] =  new (ELeave) CArrayFixFlat<TInt>(3);
			return providerIx;
		}
} 

void CJuikIconManagerImpl::LoadStaticIconsL(const TDesC& aFileName, const TIconID2* aIconDefs, TInt aNbIcons)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("LoadStaticIconsL"));

	// Make sure that static icon provider for this doesn't already exist
	ASSERT( aNbIcons > 0 );
	if ( iStaticProviders->GetData(aFileName) )
		{
			TErrorCode code = MakeErrorCode( CONTEXT_UID_JAIKUUIKIT, KErrAlreadyExists );
			Bug( _L("Tried to load static icons for file that already exists") ).ErrorCode( code ).Raise();
		}
	
	auto_ptr<CStaticIconProvider> provider( CStaticIconProvider::NewL( *this, aFileName, aIconDefs, aNbIcons ));
	iStaticProviders->AddDataL( aFileName, provider.release() );
}


MStaticIconProvider* CJuikIconManagerImpl::GetStaticIconProviderL(const TDesC& aIconFile)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("GetStaticIconProviderL"));
	return (CStaticIconProvider*)( iStaticProviders->GetData( aIconFile ) );
}



void CJuikIconManagerImpl::LoadIconsL(TInt aProviderId, const TIconID* aIconDefs, TInt aNbIcons)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("LoadIconsL"));
	auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(aNbIcons) );
	JuikIcons::LoadIconsL( icons.get(), aIconDefs, aNbIcons );
	SetIconsL( aProviderId, *icons );
}



void CJuikIconManagerImpl::SetIconsL(TInt aProviderId, CArrayPtr<CGulIcon>& aIcons)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("SetIconsL"));
	// FIXME: this can be optimized by inlining addiconl functionality
	for ( TInt i=0; i < aIcons.Count(); i++ )
		{
			CGulIcon* icon = aIcons[i];
			aIcons[i] = NULL;
			AddIconL( aProviderId, icon );
		}
}

CArrayPtr<CGulIcon>* CJuikIconManagerImpl::GetProviderIconsL( TInt aProviderId ) 
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("GetProviderIconsL"));
	CArrayFix<TInt>& map = ProviderIconIndexL(aProviderId);
	auto_ptr< CArrayPtr<CGulIcon> >  result( new (ELeave) CArrayPtrFlat<CGulIcon>(map.Count()) );
	for( TInt i=0; i < map.Count(); i++)
		{
			if ( map[i] >= 0 )
				result->AppendL( iIconArray->At( map[i] ) );
			else
				result->AppendL( NULL );
		}
	return result.release();
}

TInt CJuikIconManagerImpl::FindEmptySlotL()
{
	for ( TInt i = 0; i < iIconArray->Count(); i++ )
		{
			if ( iIconArray->At(i) == NULL ) 
				return i;
		}
	return KErrNotFound;
}


TInt CJuikIconManagerImpl::AddIconL(TInt aProviderId, CGulIcon* aIcon)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("AddIconL"));
	auto_ptr<CGulIcon> icon( aIcon );
	CArrayFix<TInt>& map = ProviderIconIndexL(aProviderId);
	
	TInt ix = KErrNotFound;
	if ( iEmptySlotCount > 0 )
		ix = FindEmptySlotL();
	
	if ( ix == KErrNotFound )
		{
			iIconArray->AppendL( icon.release() );					   
			map.AppendL( LastIndex( *iIconArray ) );
		}
	else
		{
			iEmptySlotCount--;
			if ( iEmptySlotCount < 0 ) Bug( _L("Empty slot count underflow") ).Raise();
			
			(*iIconArray)[ix] = icon.release();
			map.AppendL( ix );
		}
	return LastIndex( map );
}
			  

void CJuikIconManagerImpl::ReplaceIconL(TInt aProviderId, TInt aIconId, CGulIcon* aIcon)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("ReplaceIconL"));
	auto_ptr<CGulIcon> icon( aIcon );
	CArrayFix<TInt>& map = ProviderIconIndexL(aProviderId);
	AssertIndexL( map, aIconId );

	TInt ix = map.At( aIconId );
	if ( ix != KErrNotFound ) 
		{
			AssertIndexL( *iIconArray, ix );	
			delete (*iIconArray)[ix];
			(*iIconArray)[ix] = icon.release();
		}
	else
		{
			iIconArray->AppendL( icon.release() );					   
			map[aIconId] = LastIndex( *iIconArray );
		}
}


void CJuikIconManagerImpl::DeleteIconL(TInt aProviderId, TInt aIconId)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("DeleteIconL"));
	CArrayFix<TInt>& map = ProviderIconIndexL(aProviderId);
	AssertIndexL( map, aIconId );

	TInt ix = map.At( aIconId );
	AssertIndexL( *iIconArray, ix );

	map[aIconId] = KErrNotFound;
	
	if ( ix >= 0 && (*iIconArray)[ix] )
		{
			delete (*iIconArray)[ix];
			(*iIconArray)[ix] = NULL;
			iEmptySlotCount++;
		}
}


void CJuikIconManagerImpl::RemoveProviderIconsL(TInt aProviderId)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("RemoveProviderIconsL"));
	CArrayFix<TInt>& map = ProviderIconIndexL(aProviderId);
	for ( TInt i=0; i < map.Count(); i++ )
		{
			DeleteIconL( aProviderId, i );
		}
	
	delete (*iProviderIconIndexes)[aProviderId];
	(*iProviderIconIndexes)[aProviderId] = NULL;
}


TInt CJuikIconManagerImpl::GetListBoxIndexL(TInt aProviderId, TInt aIconId)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("GetListBoxIndexL"));
	CArrayFix<TInt>& map = ProviderIconIndexL(aProviderId);
	TInt ix = map[ aIconId ];
	AssertIndexL( *iIconArray, ix );
	return ix;
}

CArrayPtr<CGulIcon>* CJuikIconManagerImpl::GetIconArray() const
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("GetIconArray"));
	return iIconArray;
}


CArrayFix<TInt>& CJuikIconManagerImpl::ProviderIconIndexL(TInt aProviderId) const
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("ProviderIconIndexL"));
	AssertIndexL( *iProviderIconIndexes, aProviderId );
	CArrayFix<TInt>* map = iProviderIconIndexes->At(aProviderId);
	if ( map )
		return *map;
	else
		Bug( _L("Trying to get removed provider") ).ErrorCode( MakeErrorCode( CONTEXT_UID_JAIKUUIKIT, KErrNotFound ) ).Raise();
	return *map; // to please compiler
}


TInt CJuikIconManagerImpl::LastIndex(CArrayFixBase& aArray) const
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("LastIndex"));
	return aArray.Count() - 1;
}


void CJuikIconManagerImpl::AssertIndexL(CArrayFixBase& aArray, TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("AssertIndexL"));
	if ( aIndex < 0 && aArray.Count() <= aIndex) Bug( _L("Index out of bounds") ).Raise();
}


CJuikIconManagerImpl::CJuikIconManagerImpl() : iEmptySlotCount(0)
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("CJuikIconManagerImpl"));
}

void CJuikIconManagerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("ConstructL"));
	iStaticProviders = CGenericStringMap::NewL();
	iStaticProviders->SetDeletor( &DeleteStaticIconProvider );
	iProviderIconIndexes = new (ELeave) CArrayPtrFlat< CArrayFix<TInt> >(5);	
	iOwnsIconArray = ETrue;
	iIconArray = new (ELeave) CArrayPtrFlat< CGulIcon >(5);
}

CJuikIconManagerImpl::~CJuikIconManagerImpl()
{
	CALLSTACKITEM_N(_CL("CJuikIconManagerImpl"), _CL("~CJuikIconManagerImpl"));
	if ( iProviderIconIndexes ) iProviderIconIndexes->ResetAndDestroy();
	delete iProviderIconIndexes;
	if ( iOwnsIconArray ) 
		{
			if ( iIconArray ) iIconArray->ResetAndDestroy();
			delete iIconArray;
		}
	delete iStaticProviders;
}

