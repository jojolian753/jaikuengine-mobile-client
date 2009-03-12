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

#include "JaikuContactsListbox.h"

#include "JaikuLayout.h"
#include "JaikuLayoutIds.hrh"

#include "ccu_userpics.h" 
#include "PresenceTextFormatter.h"
#include "phonebook.h"
#include "presence_ui_helper.h"

// From context
#include <icons.h>
#include <pointer.h>
#include <reporting.h>
#include "cl_settings.h"
#include "settings.h"

// From S60
#include <AknIconArray.h>
#include <aknlists.h>
#include <avkon.hrh>
#include <avkon.mbg>
#include <eikenv.h>
#include <eikfrlb.h>
#include <eikfrlbd.h>
#include <eiklbi.h>
#include <gdi.h>

#include <testbuddypics.mbg>
#include <contextcontactsui.mbg>

// 

const TInt KNonJaikuNameSubCellIx   = 0;
const TInt KNonJaikuDetailSubCellIx = 1;
const TInt KNameSubCellIx           = 2;
const TInt KPresenceSubCellIx       = 3;
const TInt KBuddyIconSubCellIx      = 4;
const TInt KStatusIconSubCellIx     = 5;
const TInt KMarkerSubCellIx         = 6;

////////////////
// ListBox model

const MDesCArray* CJaikuContactsLBModel::MatchableTextArray() const
{
	CALLSTACKITEM_N(_CL("CJaikuContactsLBModel"), _CL("MatchableTextArray"));
	return iFilterArray;
}

EXPORT_C void CJaikuContactsLBModel::SetFilterArray( MDesCArray* aFilterArray )
{
	CALLSTACKITEM_N(_CL("CJaikuContactsLBModel"), _CL("SetFilterArray"));
	iFilterArray = aFilterArray;
}


EXPORT_C CJaikuContactsLBModel* CJaikuContactsLBModel::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CJaikuContactsLBModel"), _CL("NewL"));
	auto_ptr<CJaikuContactsLBModel> self( new (ELeave) CJaikuContactsLBModel );
	self->ConstructL();
	return self.release();
}




////////////////
// Icons

namespace JaikuUtils {	
	const TInt KNbIcons = 5;
	const TInt KDummyIconId = KNbIcons;

	static const TIconID iconId[KNbIcons]=
		{
			// System 
			_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_indi_marked_add, EMbmAvkonQgn_indi_marked_add_mask),
			// Jaiku 
#ifndef __WINS__
			_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_green,  EMbmContextcommonLight_mask ), 
			_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_red,	  EMbmContextcommonLight_mask ), 
			_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_yellow, EMbmContextcommonLight_mask ), 
			_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_gray,	  EMbmContextcommonLight_mask ), 

#else
			_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_green,  EMbmContextcommonLight_mask ), 
			_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_red,	  EMbmContextcommonLight_mask ), 
			_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_yellow, EMbmContextcommonLight_mask ), 
			_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonLight_gray,	  EMbmContextcommonLight_mask ), 
#endif

		};




	void LoadJaikuIcons(CArrayPtrFlat<CGulIcon> * aIconList)
	{
		LoadIcons(aIconList, iconId, KNbIcons);
	}

	TInt GetJaikuIconIndex(TInt identifier)
	{

		return GetIconIndex(identifier, iconId, KNbIcons);
	}
	
	
	


	/** 
	 * This only works internally for above icon list.
	 */
	TInt GetJaikuPresenceIconIndex(TPresenceStatus aStatus)
	{
		switch ( aStatus )
			{
			case EPresenceNone: 
				return KErrNotFound;
			case EPresenceGray:
				return GetJaikuIconIndex( EMbmContextcommonLight_gray );
			case EPresenceRed: 
				return GetJaikuIconIndex( EMbmContextcommonLight_red );
			case EPresenceYellow:
				return GetJaikuIconIndex( EMbmContextcommonLight_yellow );
			case EPresenceGreen: 
				return GetJaikuIconIndex( EMbmContextcommonLight_green );
			default: 
				// FIXME: throw an error or warning
				return KErrNotFound;
			}
	}

	JaikuUtils::TPresenceStatus GetPresenceStatus( contact& aContact, TBool aPresenceEnabled )
	{
		return GetPresenceStatus( aContact.presence,
								  aPresenceEnabled,
								  aContact.is_myself);
	}

	JaikuUtils::TPresenceStatus GetPresenceStatus( const CBBPresence* p, 
												   TBool aPresenceEnabled, 
												   TBool aMyPresence )
	{
		
		
		TPresenceStatus ix = EPresenceNone;
		if ( !p || ! p->iSent || ! aPresenceEnabled )
			{
				if ( aMyPresence )
					{
						ix = EPresenceGray;
					}
				else
					{
						ix = EPresenceNone;
					}
			}
		else
			{
				TTime stamp = p->iSentTimeStamp();
				TBool outOfDate = IsOutOfDate( stamp, GetTime() );

				// out of date 

				if ( outOfDate )
					{
						if ( aMyPresence )
							{
								ix = EPresenceGray;
							}
						else
							{
								ix = EPresenceNone;
							}
					}
				else
					{

						// FIXME: this a hack, but we currently have no better way to do this
						// If profiles name is empty, we consider profile to missing
						// Web updates don't have profiles, but we don't know it any other way.
						if ( p->iProfile.iProfileName().Length() == 0 )
							{
								ix = EPresenceNone;
							}
						else 
							{
								TInt ringType = p->iProfile.iRingingType();
								TBool vibraOn = p->iProfile.iVibra();
								TInt volumeOn = p->iProfile.iRingingVolume() != 0;
								
								// RED 
								if ( ( ringType == TBBProfile::ERingingTypeSilent || ! volumeOn ) && ! vibraOn )
									{
										ix = EPresenceRed;
									}
								
								// YELLOW
								else if ( (volumeOn && ringType == TBBProfile::ERingingTypeBeepOnce) || // beeponce 
										  (vibraOn && ! volumeOn ) ||	  // volume == 0 and vibra 
										  (vibraOn && ringType == TBBProfile::ERingingTypeSilent) )	 //silent and vibra
									{
										ix = EPresenceYellow;
									}
								// GREEN 
								else
									{
										ix = EPresenceGreen;
									}
							}
					}
			}
		
		return ix;
	}
}// End of namespace JaikuUtils 

	
////////////////
// Helper functions to create listbox item texts
	
	


void AppendStatusIconL( TPtr& aBuf, contact& aContact, TBool aPresenceEnabled )
{
	CALLSTACKITEMSTATIC_N(_CL("TBBProfile"), _CL("ERingingTypeSilent"));
	TInt ix = JaikuUtils::GetJaikuPresenceIconIndex( 
													JaikuUtils::GetPresenceStatus( aContact, aPresenceEnabled ) );
	if ( ix >= 0 )
		{
			TBuf<4> icon;
			icon.Format( _L("%02d"), ix );
			aBuf.Append( icon );
		}
}

void AppendSelectionMarkerL( TPtr aBuf )
{
	CALLSTACKITEMSTATIC_N(_CL("JaikuUtils"), _CL("GetPresenceStatus"));
	TBuf<4> icon;
	icon.Format(_L("%02d"), JaikuUtils::GetJaikuIconIndex(EMbmAvkonQgn_indi_marked_add) );
	aBuf.Append(icon);
}


void AppendBuddyImageL( TPtr& aBuf, TInt aIconId )
{
	CALLSTACKITEMSTATIC_N(_CL("JaikuUtils"), _CL("GetJaikuIconIndex"));
	TBuf<4>icon;
	icon.Format(_L("%02d"), aIconId );
	aBuf.Append(icon);
}

////////////////
// Custom array for jaiku listbox 


class CJaikuContactsArray : public CBase,
							public MDesCArray,
							public MContextBase {

public: 
	CJaikuContactsArray( phonebook_i& aPhonebook, CUserPics& aUserPics ) 
		: iPhonebook( aPhonebook ), iUserPics(aUserPics), iShowLastNameFirst(ETrue) {
	
	}

	void ConstructL( )
	{
		CALLSTACKITEM_N(_CL("JaikuContactsArray"), _CL("ConstructL"));
		
		iBuf = HBufC::NewL( 1000 );
		iFormatter = CPresenceTextFormatter::NewL();
		iShowLastNameFirst = iPhonebook.get_engine()->NameDisplayOrderL() == CPbkContactEngine::EPbkNameOrderLastNameFirstName;
	}


	~CJaikuContactsArray()
	{
		delete iFormatter;
		delete iBuf;
	}

private: // from MDesCArray
	TPtrC MdcaPoint(TInt aIndex) const;
	TInt MdcaCount() const;

private:
	/**
	 * Internal buffer to format contact text
	 * own. 
	 */ 
	mutable HBufC* iBuf;
	TBool iShowLastNameFirst; 
	phonebook_i& iPhonebook;
	CPresenceTextFormatter* iFormatter;
	CUserPics& iUserPics;
};

TInt CJaikuContactsArray::MdcaCount() const
{
	CALLSTACKITEM_N(_CL("CJaikuContactsArray"), _CL("MdcaCount"));
	return iPhonebook.Count();
}

_LIT( KTab, "\t" );
TPtrC CJaikuContactsArray::MdcaPoint(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CJaikuContactsArray"), _CL("MdcaPoint"));
	contact* c = iPhonebook.GetContact( aIndex );
	
	TPtr ptr = iBuf->Des();
	ptr.Zero();
	// 1) no contact found, create empty list item
	if ( !c )
		{
	
			_LIT(KEmptyName, " ");
			ptr.AppendFormat( _L("%S\t\t\t\t\t\t"), &KEmptyName());
		}
	
	// 2) Jaiku contact
	else if ( c->has_nick )
		{
			const TInt KPresenceMaxLen = 20;

			c->Name( iBuf, iShowLastNameFirst ); 
			ptr = iBuf->Des();

			ptr.Insert(0, KTab ); // Non-jaiku contact name done
			ptr.Insert(0, KTab ); // Non-jaiku contact details done

			ptr.Append( KTab ); // Jaiku Contact Name done
			iFormatter->ShortTextL(c->presence, ptr, KPresenceMaxLen);
			ptr.Append( KTab ); // Jaiku Contact Presence Line done			
			TInt ix = iUserPics.GetIconIndexL( c->id );
			if ( ix != KErrNotFound )
				{
					AppendBuddyImageL( ptr, ix );
				}
			else 
				{
					AppendBuddyImageL( ptr, JaikuUtils::KDummyIconId );
				}
			ptr.Append( KTab ); // Jaiku Contact Image done 
			AppendStatusIconL( ptr, *c, iPhonebook.IsPresenceEnabled() );
			ptr.Append( KTab ); // Jaiku Contact Presence Icon Done
		}

	// 3) Normal contact
	else  
		{
			c->Name( iBuf, iShowLastNameFirst );
			ptr = iBuf->Des();
			ptr.Append( KTab ); // Non-jaiku contact name done
 			if ( c->extra_name )
 				{
 					ptr.Append( *(c->extra_name) );
 				}

			ptr.Append( KTab ); // Non-jaiku contact details done
			ptr.Append( KTab ); // Jaiku Contact Name done
			ptr.Append( KTab ); // Jaiku Contact Presence Line done			
			ptr.Append( KTab ); // Jaiku Contact Image done 
			ptr.Append( KTab ); // Jaiku Contact Presence Icon Done
		}
	

	AppendSelectionMarkerL( ptr );

	// FIXME: teemu. const_cast hack to make reporting possible
	const_cast<CJaikuContactsArray*>(this)->Reporting().DebugLog( *iBuf );
	return *iBuf;
}

////////////////
// custom item drawer

class CJaikuLBItemDrawer : public CFormattedCellListBoxItemDrawer,
						   public MContextBase
{
public:
	static CJaikuLBItemDrawer* NewL(MTextListBoxModel* aModel,
									const CFont* aFont,
									CFormattedCellListBoxData* aLBData,
									CJaikuContactsListBox* aListBox,
									phonebook_i& aPhonebook)
	{
		CALLSTACKITEMSTATIC_N(_CL("CJaikuLBItemDrawer"), _CL("NewL"));

		auto_ptr<CJaikuLBItemDrawer> self( new (ELeave) CJaikuLBItemDrawer( aModel, aFont, aLBData, aListBox, aPhonebook ) );
		self->ConstructL();
		return self.release();		
	}
	
	CJaikuLBItemDrawer( MTextListBoxModel* aModel,
						const CFont* aFont,
						CFormattedCellListBoxData* aLBData,
						CJaikuContactsListBox* aListBox,
						phonebook_i& aPhonebook )
		: CFormattedCellListBoxItemDrawer( aModel, aFont, aLBData ),
		  iPhonebook( aPhonebook )
	{			
		CALLSTACKITEM_N(_CL("CJaikuLBItemDrawer"), _CL("CJaikuLBItemDrawer"));		
	}

	void ConstructL()
	{		
		CALLSTACKITEM_N(_CL("CJaikuLBItemDrawer"), _CL("ConstructL"));		
	}
	

	void DrawItemText( TInt aItemIndex, 
					   const TRect& aItemTextRect,
					   TBool aItemIsCurrent,
					   TBool aViewIsEmphasized,
					   TBool aItemIsSelected ) const
	{
		CALLSTACKITEM_N(_CL("CJaikuLBItemDrawer"), _CL("DrawItemText"));		
		CFormattedCellListBoxData* lbData = FormattedCellData();
		const_cast<CJaikuLBItemDrawer*>(this)->Reporting().DebugLog( _L("DrawItemText "));
	

		if ( aItemIsSelected )
			{
				lbData->SetSubCellSizeL( KMarkerSubCellIx, TSize(14, 15) );
			}
		else
			{
				lbData->SetSubCellSizeL( KMarkerSubCellIx, TSize(0, 0) );
			}


		TBool outOfDate = EFalse;
		TBool isJaikuContact = EFalse;
		TBool isOffline = EFalse;

		if ( iPhonebook.IsPresenceEnabled() )
			{
				// FIXME: when text is inputted, presence is grayed/ungrayed incorrectly
				// because aItemIndex refers to filtered listbox model, not iPhonebook...
				contact* con = iPhonebook.GetContact( aItemIndex );
				if ( con )
					{
						if ( con->has_nick )
							{
								isJaikuContact = ETrue;
								if ( con->presence )
									{								
										TTime stamp = con->presence->iSentTimeStamp();
										outOfDate = IsOutOfDate( stamp, GetTime() );
									}
								else
									{
										isOffline = ETrue;
									}
							}
					}
			}

		CFormattedCellListBoxData::TColors c = lbData->SubCellColors( KPresenceSubCellIx );
		// We use gray in second line for
		// 0) Jaiku is disconnected
		// 1) non-jaiku contacts 
		// 2) off line jaiku contacts
		// 3) and out-of-date data of Jaiku contacts 
		if ( (! iPhonebook.IsPresenceEnabled() ) ||  (! isJaikuContact) || isOffline || outOfDate )
			{
				// Set colors gray		
				TRgb grayed( 128,128,128 );
				c.iText = grayed;
				c.iHighlightedText = grayed;
				//lbData->SetSubCellColorsL(1, c);
			}
		else
			{
				// We need to set colors back to "default", because previous
				// items affect this. 
				TRgb black( 0,0,0 );
				c.iText = black;
				c.iHighlightedText = black;
			}
		lbData->SetSubCellColorsL(KPresenceSubCellIx, c);		
		lbData->SetSubCellColorsL(KNonJaikuDetailSubCellIx, c);		
		
	
		CFormattedCellListBoxItemDrawer::DrawItemText( aItemIndex,
													   aItemTextRect,
													   aItemIsCurrent,
													   aViewIsEmphasized,
													   aItemIsSelected );

	}
private:
	phonebook_i& iPhonebook;
};



// class CJaikuListBoxIcons : public CArrayPtr<CGulIcon>, public MContextBase
// {
// public:
// 	CJaikuListBoxIcons(CAknIconArray* aStaticIcons,
// 					   CUserPics& aUserPics) : CArrayPtr,
// 											   iStaticIcons(aStaticIcons),
// 											   iUserPics( aUserPics ) {}
// 	virtual ~CJaikuListBoxIcons() {}
	
// 	TA
// 	CGulIcon& At(TInt aIndex) { GetIconL(aIndex); }
// 	const CGulIcon& At(TInt aIndex) const { GetIconL(aIndex); }
// 	const CGulIcon& operator[](TInt aIndex) const { GetIconL(aIndex); }
// 	CGulIcon& operator[](TInt aIndex) { GetIconL(aIndex); }
	
// 	const CGulIcon& GetIconL(TInt aIndex) const;
// 	CGulIcon& GetIconL(TInt aIndex);
// private:
// 	CAknIconArray* iStaticIcons;
// 	CUserPics& iUserPics;
// };

// const CGulIcon& CJaikuListBoxIcons::GetIconL(TInt aIndex) const
// {
// 	return const_cast<CJaikuListBoxIcons*>(this)->GetIconL(aIndex);
// }



// CGulIcon& CJaikuListBoxIcons::GetIconL(TInt aIndex)
// {
// 	if (0 <= aIndex && aIndex < iStaticIcons->Count() )
// 		{
// 			return iStaticIcons->At(aIndex);
// 		}
// 	else 
// 		{
// 			return *(iUserPics.GetIconL( aIndex ));
// 		}
// }

////////////////
// Custom jaiku listbox


/**
 * We need this only because we need to create own text list box model.
 * It also helps with debugging, because it derives from MContextBase
 */

void CJaikuContactsListBox::CreateItemDrawerL()
{
	CALLSTACKITEMSTATIC_N(_CL("CJaikuContactsListBox"), _CL("CreateItemDrawerL"));
	// Replace model
	delete iModel;
	iModel = NULL;
	iModel = CJaikuContactsLBModel::NewL();

	// Create item drawer
	CFormattedCellListBoxData* lbData = CFormattedCellListBoxData::NewL();	  
	iItemDrawer = CJaikuLBItemDrawer::NewL( Model(),
											iEikonEnv->NormalFont(),
											lbData,
											this,
											iPhonebook );

}


EXPORT_C CJaikuContactsLBModel* CJaikuContactsListBox::JaikuModel() const
{
	CALLSTACKITEM_N(_CL("CJaikuContactsListBox"), _CL("JaikuModel"));
	return static_cast<CJaikuContactsLBModel*>( Model() );
}


EXPORT_C TRect CJaikuContactsListBox::AdjustAndSetRect( TRect aRect )
{
	CALLSTACKITEM_N(_CL("CJaikuContactsListBox"), _CL("AdjustAndSetRect"));
	TRect lbRect = aRect;
	AdjustRectHeightToWholeNumberOfItems( lbRect );
	SetRect( lbRect );	  
	return lbRect;
}


void CJaikuContactsListBox::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CJaikuContactsListBox"), _CL("SizeChanged"));
	CEikFormattedCellListBox::SizeChanged();
	SetupSubcellsL();
}


CJaikuContactsListBox::CJaikuContactsListBox( phonebook_i& aPhonebook, CUserPics& aUserPics )
	: iPhonebook( aPhonebook), iUserPics(aUserPics)
{
	CALLSTACKITEM_N(_CL("CJaikuContactsListBox"), _CL("CJaikuContactsListBox"));
}


// Layouts
// const TInt KRowHeight = 34;
// const TInt KItemHeight = 2 * KRowHeight;



EXPORT_C CJaikuContactsListBox* CJaikuContactsListBox::CreateListboxL(CCoeControl* aParent, 
																	  phonebook_i& aPhonebook,
																	  CUserPics& aUserPics )
{ 
	CALLSTACKITEMSTATIC_N(_CL("CJaikuContactsListBox"), _CL("CreateListboxL"));

	// Create list box 
	auto_ptr<CJaikuContactsListBox> lb( new (ELeave) CJaikuContactsListBox(aPhonebook, aUserPics) );
	lb->ConstructL( aParent, EAknListBoxMarkableList ); 
	
	lb->UpdateIconsL();
	
	auto_ptr<CJaikuLayout> layout( CJaikuLayout::NewL() );
	TJaikuLayout l = layout->GetLayoutItemL( LG_contacts_list, LI_contacts_list__item );
	lb->SetItemHeightL( l.h  );

	lb->View()->SetListEmptyTextL( _L("(DEV: Empty)") );
	lb->CreateScrollBarFrameL( ETrue );
	lb->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	return lb.release();
}


void CJaikuContactsListBox::LoadTestIconsL( CAknIconArray& aIconArray )
{
	CALLSTACKITEM_N(_CL("CJaikuContactsListBox"), _CL("LoadTestIconsL"));
	// Load test icons:
#ifndef __WINS__
	TIconID id1 = _INIT_T_ICON_ID("C:\\system\\data\\testbuddypics.mbm", EMbmTestbuddypicsJoi, KErrNotFound );
	TIconID id2 = _INIT_T_ICON_ID("C:\\system\\data\\testbuddypics.mbm", EMbmTestbuddypicsPetteri, KErrNotFound );
#else
	TIconID id1 = _INIT_T_ICON_ID("Z:\\system\\data\\testbuddypics.mbm", EMbmTestbuddypicsJoi, KErrNotFound );
	TIconID id2 = _INIT_T_ICON_ID("Z:\\system\\data\\testbuddypics.mbm", EMbmTestbuddypicsPetteri, KErrNotFound );

		//TIconID id = _INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive, KErrNotFound );
#endif 
	TRAPD(err, LoadIcons( &aIconArray, &id1, 1) );
	TRAP(err, LoadIcons( &aIconArray, &id2, 1) );
}


void CJaikuContactsListBox::UpdateIconsL()
{
	CALLSTACKITEM_N(_CL("CJaikuContactsListBox"), _CL("UpdateIconsL"));

  	auto_ptr<CAknIconArray> staticIcons( new (ELeave) CAknIconArray(10) );
  	JaikuUtils::LoadJaikuIcons( staticIcons.get() );	
	

	const TIconID iconId[1]= {
        _INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
						EMbmContextcontactsuiDummybuddy,
						EMbmContextcontactsuiDummybuddy_mask ),
	};
	
	
	LoadIcons(staticIcons.get(), iconId, 1);

	iUserPics.SetExternalIconArrayL( staticIcons.get() );
	iUserPics.AddObserverL( *this ); // we don't need notifications for initial set
	
	ItemDrawer()->FormattedCellData()->SetIconArray( staticIcons.release() );
 }


void CJaikuContactsListBox::UserPicChangedL(const TDesC& aNick)
{
	DrawDeferred();
}

EXPORT_C MDesCArray* CJaikuContactsListBox::CreateLBArrayL(phonebook_i& aPhonebook, CUserPics& aUserPics)
{
	CALLSTACKITEMSTATIC_N(_CL("CJaikuContactsListBox"), _CL("CreateLBArrayL"));
	auto_ptr<CJaikuContactsArray> array( new (ELeave) CJaikuContactsArray( aPhonebook, aUserPics ) );
	array->ConstructL();
	return array.release();
}





void CJaikuContactsListBox::SetupSubcellsL(	 ) 
{
	CALLSTACKITEM_N(_CL("CJaikuContactsListBox"), _CL("SetupSubcellsL"));
	CFormattedCellListBoxItemDrawer* itemDrawer = ItemDrawer();

	auto_ptr<CJaikuLayout> layout( CJaikuLayout::NewL() );
	TJaikuLayout l = layout->GetLayoutItemL( LG_contacts_list, LI_contacts_list__item );
	SetItemHeightL( l.h  );

	AknListBoxLayouts::SetupStandardListBox( *this );
	AknListBoxLayouts::SetupStandardFormListbox( itemDrawer );	  
	

	TSize itemSize = View()->ItemSize();
	TJaikuLayout parent( TJaikuLayout::ERect, TPoint(0,0), itemSize );
	
	/**
	 *
	 * Old subcells
	 * 0 Contact Name text
	 * 1 Presence info text
	 * 2 Presence status icon (0 green, 1 orange, 2 red)
	 *
	 * New subcells
	 * 0 Non-Jaiku Contact name (text)
	 * 1 Non-Jaiku Contact extra details (text)
	 * 2 Jaiku Contact Name (text)
	 * 3 Jaiku Contact Presence line (text)
	 * 4 Jaiku Contact buddy icon (bitmap)
	 * 5 Jaiku Contact presence status icon (0 green, 1 orange, 2 red) (icon)
	 *
	 */
 

	CArrayFix<TJaikuLayout>* layouts = 
		layout->GetLayoutGroupL( LG_contacts_list_item_content );

	

	TBool optional[] = { ETrue, ETrue, ETrue, ETrue, ETrue, ETrue, ETrue };

	// FIXME: margins still hardcoded. (might be ok for time being, as is not
	// that layout dependend)

	TMargins m = { 4, 4, 1, 1 };

	TInt subCell =	0;	  
	//const TInt KSubcellCount = 5;

	for (subCell=0; subCell < layouts->Count(); subCell++)
		{
			TJaikuLayout l = parent.Combine( m ).Combine( layouts->At(subCell) );
			TPoint p0 = l.TopLeft();
			TPoint p1 = l.BottomRight();
			
			switch ( l.ltype )
				{
				case TJaikuLayout::EText:
					{
						TInt b = l.Baseline();
						const CFont* font = l.Font();						
						AknListBoxLayouts::SetupFormTextCell( *this, itemDrawer, subCell, 
															  font,
															  215, // C
															  l.x,
															  -1, // rm but ignored
															  b, 
															  l.w,
															  CGraphicsContext::ELeft, 
															  p0,
															  p1 );
						break;
					}
				case TJaikuLayout::EIcon:
					{
						AknListBoxLayouts::SetupFormGfxCell( *this, itemDrawer, subCell, 
															 l.x, l.y, 
															 -1, -1, // r,b but ignored
															 l.w, // W
															 l.h, // H
															 p0,
															 p1 );
						break;
					}
				}
			itemDrawer->FormattedCellData()->SetNotAlwaysDrawnSubCellL( subCell, optional[subCell] );			
		}
	return;

}

