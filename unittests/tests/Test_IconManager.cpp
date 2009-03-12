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

#include "Test_IconManager.h"

#include "juik_iconmanager.h"

#include "app_context_impl.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "raii_array.h"

#include <JaikuUnitTest.mbg>

#include <gulicon.h>

void CTest_IconManager::testProviderIdL()
{
	TInt id1 = iIconManager->GetNewProviderIdL();
	TInt id2 = iIconManager->GetNewProviderIdL();
	TS_ASSERT_DIFFERS( id1, id2 );

	CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
	TS_ASSERT( array );
	TS_ASSERT( array->Count() == 0 );
}


void CTest_IconManager::testAddIconL()
{
		TInt providerId = iIconManager->GetNewProviderIdL();
		
		const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
												EMbmJaikuunittestTestsvg1,
												EMbmJaikuunittestTestsvg1_mask );			
		auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
 		TS_ASSERT( icon.get() );
		AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
 		CGulIcon* iconOriginal = icon.get();
 		iIconManager->AddIconL( providerId, icon.release() );
		
		{
			CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
			TS_ASSERT( array );
			TS_ASSERT_EQUALS( array->Count(), 1 );
		}

		TInt lbIx = iIconManager->GetListBoxIndexL( providerId, 0 );
		TS_ASSERT_EQUALS( lbIx, 0 );
		
		{
			auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( providerId ) );
			TS_ASSERT( array.get() );
			TS_ASSERT_EQUALS( array->Count(), 1 );
			TS_ASSERT( array->At(0) == iconOriginal );
		}
}


void CTest_IconManager::testAddSeveralIconsL()
{
	for (TInt p=0; p < 10; ++p )
		{
		TInt providerId = iIconManager->GetNewProviderIdL();
		
		for ( TInt i=0; i < 10; i++ )
			{
				const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
														EMbmJaikuunittestTestsvg1,
														EMbmJaikuunittestTestsvg1_mask );			
				auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
				TS_ASSERT( icon.get() );
				AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
				CGulIcon* iconOriginal = icon.get();
				iIconManager->AddIconL( providerId, icon.release() );
				
				{
					auto_ptr< CArrayPtr<CGulIcon> > array(iIconManager->GetProviderIconsL( providerId ) );
					TS_ASSERT( array.get() );
					TS_ASSERT_EQUALS( array->Count(), i+1 );
				}
			}
		}
	CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
	TS_ASSERT( array );
	TS_ASSERT_EQUALS( array->Count(), 100 );
}


void CTest_IconManager::testReplaceManyIconsL()
{
	const TInt KProviderCount(5);
	const TInt KIconPerProvider(10);

	RAArray<TInt> providerIds;

	for (TInt p=0; p < KProviderCount; ++p )
		{
		TInt providerId = iIconManager->GetNewProviderIdL();
		providerIds.Append( providerId );
		
		
		for ( TInt i=0; i < KIconPerProvider; i++ )
			{
				auto_ptr<CGulIcon> icon( NULL );
				if ( i % 2 == 0 )
					{
						const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
																EMbmJaikuunittestTestsvg1,
																EMbmJaikuunittestTestsvg1_mask );			
						icon.reset( JuikIcons::LoadSingleIconL( KIconId ) );
					}
				else
					{
						const TIconID KIconId = 
							_INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
											EMbmJaikuunittestTestsvg2, 
											KErrNotFound );
						icon.reset( JuikIcons::LoadSingleIconL( KIconId ) );
					}
				TS_ASSERT( icon.get() );
				AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
				CGulIcon* iconOriginal = icon.get();
				iIconManager->AddIconL( providerId, icon.release() );
			}
		}
	CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
	TS_ASSERT( array );
	TS_ASSERT_EQUALS( array->Count(), KProviderCount * KIconPerProvider );
	
	for (TInt i=0; i < providerIds.Count(); i++)
		{
			TInt p = providerIds[i];
			auto_ptr<CGulIcon> icon(NULL);
			const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
													EMbmJaikuunittestTestsvg3,
													EMbmJaikuunittestTestsvg3_mask );			
			
			
			icon.reset( JuikIcons::LoadSingleIconL( KIconId ) );			
			TInt fifth = 5 - 1;
			
			iIconManager->ReplaceIconL( p, fifth, icon.release() );
		}
	
	
	for (TInt i=0; i < providerIds.Count(); i++)
		{
			TInt p = providerIds[i];
			auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( p ) );
			TS_ASSERT( array.get() );
			TS_ASSERT_EQUALS( array->Count(), KIconPerProvider );
		}
}


void CTest_IconManager::testDeleteAddDeleteL()
{	
	TInt providerId = iIconManager->GetNewProviderIdL();
	
	CGulIcon* iconOriginal = NULL;
	{ // INSERT
		const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
												EMbmJaikuunittestTestsvg1,
												EMbmJaikuunittestTestsvg1_mask );			
		auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
		TS_ASSERT( icon.get() );
		AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
		iconOriginal = icon.get();
		iIconManager->AddIconL( providerId, icon.release() );
	}

	
	{ // TEST
		CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
		TS_ASSERT( array );
		TS_ASSERT_EQUALS( array->Count(), 1 );

		TInt lbIx = iIconManager->GetListBoxIndexL( providerId, 0 );
		TS_ASSERT_EQUALS( lbIx, 0 );
	}

	{
		auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( providerId ) );
		TS_ASSERT( array.get() );
		TS_ASSERT_EQUALS( array->Count(), 1 );
	}
	

	{ // DELETE 
		iIconManager->DeleteIconL(providerId, 0);
	}
	
	{
		auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( providerId ) );
		TS_ASSERT( array.get() );
		TS_ASSERT_EQUALS( array->Count(), 1 );
		TS_ASSERT( array->At(0) == NULL );
	}


	{ // INSERT
		const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
												EMbmJaikuunittestTestsvg2,
												KErrNotFound );			
		auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
		TS_ASSERT( icon.get() );
		AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
		iconOriginal = icon.get();
		iIconManager->AddIconL( providerId, icon.release() );
	}
	
	{ // TEST
		CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
		TS_ASSERT( array );
		TS_ASSERT_EQUALS( array->Count(), 1 );
		
		TInt lbIx = iIconManager->GetListBoxIndexL( providerId, 0 );
		TS_ASSERT_EQUALS( lbIx, KErrNotFound );
		lbIx = iIconManager->GetListBoxIndexL( providerId, 1 );
		TS_ASSERT_EQUALS( lbIx, 0 );
	}

	{
		auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( providerId ) );
		TS_ASSERT( array.get() );
		TS_ASSERT_EQUALS( array->Count(), 2 );
	}
}

void CTest_IconManager::testRemoveProviderL()
{ 
 	TInt providerId = iIconManager->GetNewProviderIdL();
	const TInt KIconPerProvider(10);
	{ // INSERT
		for (TInt i=0; i < KIconPerProvider; i++)
			{
				const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
														EMbmJaikuunittestTestsvg1,
														EMbmJaikuunittestTestsvg1_mask );			
				auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
				TS_ASSERT( icon.get() );
				AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
				iIconManager->AddIconL( providerId, icon.release() );
			}
	}
	{
		CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
		TS_ASSERT( array );
		TS_ASSERT_EQUALS( array->Count(), KIconPerProvider );
		for (TInt i=0; i < array->Count(); i++ )
			{
				TS_ASSERT( array->At(i)  );
			}
	}

	{
		auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( providerId ) );
		TS_ASSERT( array.get() );
		TS_ASSERT_EQUALS( array->Count(), KIconPerProvider );
		for (TInt i=0; i < array->Count(); i++ )
			{
				TS_ASSERT( array->At(i)  );
			}
	}

	iIconManager->RemoveProviderIconsL( providerId );
	
	{
		CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
		TS_ASSERT( array );
		TS_ASSERT_EQUALS( array->Count(), KIconPerProvider );
		for (TInt i=0; i < array->Count(); i++ )
			{
				TS_ASSERT( array->At(i) == NULL );
			}
	}
	
	{
		TS_ASSERT_THROWS( iIconManager->GetProviderIconsL( providerId ), KErrNotFound );
	}

	TInt provId2 = iIconManager->GetNewProviderIdL();
	{ // INSERT
		for (TInt i=0; i < KIconPerProvider; i++)
			{
				const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
														EMbmJaikuunittestTestsvg1,
														EMbmJaikuunittestTestsvg1_mask );			
				auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
				TS_ASSERT( icon.get() );
				AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
				iIconManager->AddIconL( providerId, icon.release() );
			}
	}

	{
		CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
		TS_ASSERT( array );
		TS_ASSERT_EQUALS( array->Count(), KIconPerProvider );
		for (TInt i=0; i < array->Count(); i++ )
			{
				TS_ASSERT( array->At(i)  );
			}
	}

	{
		auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( provId2 ) );
		TS_ASSERT( array.get() );
		TS_ASSERT_EQUALS( array->Count(), KIconPerProvider );
		for (TInt i=0; i < array->Count(); i++ )
			{
				TS_ASSERT( array->At(i)  );
			}
	}
}

// void CTest_IconManager::testDeletesL()
// {	

// 	const TInt KProviderCount(5);
// 	const TInt KIconPerProvider(10);
	
// 	RAArray<TInt> providerIds;

// 	for (TInt p=0; p < KProviderCount; ++p )
// 		{
// 		TInt providerId = iIconManager->GetNewProviderIdL();
// 		providerIds.Append( providerId );
		
		
// 		for ( TInt i=0; i < KIconPerProvider; i++ )
// 			{
// 				auto_ptr<CGulIcon> icon( NULL );
// 				if ( i % 2 == 0 )
// 					{
// 						const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
// 																EMbmJaikuunittestTestsvg1,
// 																EMbmJaikuunittestTestsvg1_mask );			
// 						icon.reset( JuikIcons::LoadSingleIconL( KIconId ) );
// 					}
// 				else
// 					{
// 						const TIconID KIconId = 
// 							_INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
// 											EMbmJaikuunittestTestsvg2, 
// 											KErrNotFound );
// 						icon.reset( JuikIcons::LoadSingleIconL( KIconId ) );
// 					}
// 				TS_ASSERT( icon.get() );
// 				AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
// 				CGulIcon* iconOriginal = icon.get();
// 				iIconManager->AddIconL( providerId, icon.release() );
// 			}
		
// 		for (TInt pr = 0; pr < providerIds.Count(); pr++)
// 			{
// 				for (TInt i=0; 
// 					 }
		
// 		}

	
// 	CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
// 	TS_ASSERT( array );
// 	TS_ASSERT_EQUALS( array->Count(), KProviderCount * KIconPerProvider );
	
// 	for (TInt i=0; i < providerIds.Count(); i++)
// 		{
// 			TInt p = providerIds[i];
// 			auto_ptr<CGulIcon> icon(NULL);
// 			const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
// 													EMbmJaikuunittestTestsvg3,
// 													EMbmJaikuunittestTestsvg3_mask );			
			
			
// 			icon.reset( JuikIcons::LoadSingleIconL( KIconId ) );			
// 			TInt fifth = 5 - 1;
			
// 			iIconManager->ReplaceIconL( p, fifth, icon.release() );
// 		}
	
	
// 	for (TInt i=0; i < providerIds.Count(); i++)
// 		{
// 			TInt p = providerIds[i];
// 			auto_ptr< CArrayPtr<CGulIcon> > array( iIconManager->GetProviderIconsL( p ) );
// 			TS_ASSERT( array.get() );
// 			TS_ASSERT_EQUALS( array->Count(), KIconPerProvider );
// 		}
// }


// void CTest_IconManager::testDeleteIconL()
// {
// 	for (TInt p=0; p < 10; ++p )
// 		{
// 		TInt providerId = iIconManager->GetNewProviderIdL();
		
// 		for ( TInt i=0; i < 10; i++ )
// 			{
// 				const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
// 														EMbmJaikuunittestTestsvg1,
// 														EMbmJaikuunittestTestsvg1_mask );			
// 				auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
// 				TS_ASSERT( icon.get() );
// 				AknIconUtils::SetSize( icon->Bitmap(), TSize(30,25), EAspectRatioPreservedAndUnusedSpaceRemoved);
// 				CGulIcon* iconOriginal = icon.get();
// 				iIconManager->AddIconL( providerId, icon.release() );
				
// 				{
// 					CArrayPtr<CGulIcon>* array = iIconManager->GetProviderIconsL( providerId );
// 					TS_ASSERT( array );
// 					TS_ASSERT_EQUALS( array->Count(), i+1 );
// 					delete array;
// 				}
// 			}
// 		}
// 	CArrayPtr<CGulIcon>* array = iIconManager->GetIconArray();
// 	TS_ASSERT( array );
// 	TS_ASSERT_EQUALS( array->Count(), 100 );
// }


void InitIconResourcesL()
{
	// Loading icon once will init icon server resources, and we get rid
	// of superficial memory leak warnings in unit tests 
 	const TIconID KIconId = _INIT_T_ICON_ID("C:\\system\\data\\JaikuUnitTest.mif", 
 											EMbmJaikuunittestTestsvg1,
 											EMbmJaikuunittestTestsvg1_mask );			
 	auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL( KIconId ) );
}

void CTest_IconManager::setUp()
{
	MContextTestBase::setUp();
	iIconManager = CJuikIconManager::NewL();

	InitIconResourcesL();	
}


 void CTest_IconManager::tearDown()
 {
	delete iIconManager;
	MContextTestBase::tearDown();
}


