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

#include "juik_listbox.h"

#include "juik_subcellrenderer.h"
#include "juik_iconmanager.h"
#include "juik_layout.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "icons.h"
#include "reporting.h"

#include <avkon.mbg>
#include <aknlists.h>
#include <aknutils.h>
#include <akniconarray.h>

static const TDesC& AvkonIconFile()
{
	return AknIconUtils::AvkonIconFileName();
}

class CJuikGenLBModel : public CAknFilteredTextListBoxModel, public MContextBase, public MJuikListBoxModel 
{
public: 
	static CJuikGenLBModel* NewL();	  
	
public: // 
	virtual void SetFilterArray( MDesCArray* aFilterArray );
	
public: // From CAknFilteredTextListBoxModel 
	virtual const MDesCArray* MatchableTextArray() const;
private:
	MDesCArray* iFilterArray;
};


void CJuikGenLBModel::SetFilterArray( MDesCArray* aFilterArray )
{
	CALLSTACKITEM_N(_CL("CJuikGenLBModel"), _CL("SetFilterArray"));
	iFilterArray = aFilterArray;
}


CJuikGenLBModel* CJuikGenLBModel::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CJuikGenLBModel"), _CL("NewL"));
	auto_ptr<CJuikGenLBModel> self( new (ELeave) CJuikGenLBModel );
	self->ConstructL();
	return self.release();
}


const MDesCArray* CJuikGenLBModel::MatchableTextArray() const
{
	CALLSTACKITEM_N(_CL("CJuikGenLBModel"), _CL("MatchableTextArray"));
	return iFilterArray;
}




class CJuikGenLBItemDrawer : public CFormattedCellListBoxItemDrawer,
							  public MContextBase
{
public:
	CJuikGenLBItemDrawer(MTextListBoxModel* aModel,
						  const CFont* aFont,
						  CFormattedCellListBoxData* aLBData) : CFormattedCellListBoxItemDrawer(aModel, aFont, aLBData)
	{
	}
		
	
	void DrawItemMark(TBool aItemIsSelected, TBool aViewIsDimmed, const TPoint& aMarkPos) const
	{
		CFormattedCellListBoxItemDrawer::DrawItemMark( aItemIsSelected, aViewIsDimmed, aMarkPos );
	}


	void DrawItemText( TInt aItemIndex, 
					   const TRect& aItemTextRect,
					   TBool aItemIsCurrent,
					   TBool aViewIsEmphasized,
					   TBool aItemIsSelected ) const
	{
		CALLSTACKITEM_N(_CL("CJuikGenLBItemDrawer"), _CL("DrawItemText"));

#ifdef __WINS__
		TInt dummy;
		TBreakItem b(GetContext(), dummy);	
#endif

		CFormattedCellListBoxData* lbData = FormattedCellData();

//   		TBuf<500> buf;
//   		buf.Append( _L("#"));
//   		buf.Append( iModel->ItemText(aItemIndex) );
//   		buf.Append( _L("#"));
// 		AknTextUtils::ReplaceCharacters(buf, _L("%"), TChar('Ö'));
//    		const_cast<CJuikGenLBItemDrawer*>(this)->Reporting().UserErrorLog( buf );
		

		CFormattedCellListBoxItemDrawer::DrawItemText( aItemIndex,
													   aItemTextRect,
													   aItemIsCurrent,
													   aViewIsEmphasized,
													   aItemIsSelected );
	}
};

// static const TInt KMarkerIconCount(1);
// static const TIconID KMarkerIconIds[1]= {
// 	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_indi_marked_add, EMbmAvkonQgn_indi_marked_add_mask),
// };

class CJuikGenListBoxImpl : public CJuikGenListBox, public MContextBase
{
public: // from CJuikGenListBox 
	MJuikListBoxModel* JuikListBoxModel() { return static_cast<CJuikGenLBModel*>(Model()); }

	TRect AdjustAndSetRect( TRect aRect )
	{
		CALLSTACKITEM_N(_CL("CJuikContactsListBox"), _CL("AdjustAndSetRect"));
		TJuikLayoutItem l = Layout().GetLayoutItemL( iItemLayoutIds.iGroup, 
													 iItemLayoutIds.iFull );
		SetItemHeightL( l.h );
		TRect lbRect = aRect;
		AdjustRectHeightToWholeNumberOfItems( lbRect );
		SetRect( lbRect );	  
		return lbRect;
	}

public:
	static CJuikGenListBoxImpl* NewL( CCoeControl* aParent, TItemLayoutIds aLayout, MJuikIconManager& aIconManager)
	{
		CALLSTACKITEMSTATIC_N(_CL("CJuikGenListBoxImpl"), _CL("NewL"));
		auto_ptr<CJuikGenListBoxImpl> self( new (ELeave) CJuikGenListBoxImpl(aLayout, aIconManager) );
		self->ConstructL( aParent );
		return self.release();		
	}

	void ConstructL( CCoeControl* aParent )
	{
		CEikFormattedCellListBox::ConstructL( aParent, EAknListBoxMarkableList );
		ItemDrawer()->FormattedCellData()->SetIconArray( iIconManager->GetIconArray() );
		
		View()->SetListEmptyTextL( _L("(DEV: empty" ) );
		
		TJuikLayoutItem l = Layout().GetLayoutItemL( iItemLayoutIds.iGroup, iItemLayoutIds.iFull );
		SetItemHeightL( l.h );
		
		// Scrollbar 
		if ( iItemLayoutIds.iUseScrollBar != KErrNone )
			{
				CreateScrollBarFrameL( ETrue );
				ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto );
			}
		}


	void LayoutIntoL(const TRect& aRect)
	{
		TJuikLayoutItem parent( aRect );
		TJuikLayoutItem l = parent.Combine( Layout().GetLayoutItemL(iItemLayoutIds.iGroup, iItemLayoutIds.iListbox) );
		AdjustAndSetRect( l.Rect() );
	}
	
	void SetRenderersL( CArrayPtr<MSubcellRenderer>* aRenderers )
	{
		if ( iRenderers ) 
			{
				if ( aRenderers ) aRenderers->ResetAndDestroy();
				delete aRenderers;
				User::Leave( KErrInUse );
			}
		iRenderers = aRenderers;
	}

	CJuikGenListBoxImpl(TItemLayoutIds aLayoutIds, MJuikIconManager& aIconManager) : iItemLayoutIds(aLayoutIds), iIconManager(&aIconManager)
	{
		CALLSTACKITEM_N(_CL("CJuikGenListBoxImpl"), _CL("CJuikGenListBoxImpl"));
	}


	~CJuikGenListBoxImpl() 
	{
		ItemDrawer()->FormattedCellData()->SetIconArray( NULL );
		if ( iRenderers) iRenderers->ResetAndDestroy();
		delete iRenderers;		
	}

	void EnableMarkerL(TInt aColumn)
	{
		
// 		auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(1) );
// 		LoadIcons( icons.get(), KMarkerIconIds, KMarkerIconCount );
// 		iIconManager->SetIconsL( iProviderId, *icons );
		
		// Set index of the placeholder field
		ItemDrawer()->SetItemMarkPosition( aColumn );
		
		
		// Marker replacement string descriptor has live as to
		// long as it's used,
		// although it's passed as const TDesC& (I could kill the API designer)
		// See eiklbi.h and SetItemMarkReplacement
		TInt index = iIconManager->GetStaticIconProviderL( AvkonIconFile() )->GetListBoxIndexL( EMbmAvkonQgn_indi_marked_add );
		iMarkerReplacement.Num( index );
		ItemDrawer()->SetItemMarkReplacement( iMarkerReplacement );
		// Don't display all items as marked initially
		ItemDrawer()->SetItemMarkReverse(ETrue);
	}


	void CreateItemDrawerL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CJuikGenListBoxImpl"), _CL("CreateItemDrawerL"));
		// Replace model
		delete iModel;
		iModel = NULL;
		iModel = CJuikGenLBModel::NewL();
		
		// Create item drawer
		CFormattedCellListBoxData* lbData = CFormattedCellListBoxData::NewL();	  
		iItemDrawer = new (ELeave) CJuikGenLBItemDrawer( Model(),
														  iEikonEnv->NormalFont(),
														  lbData );
	}
	
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CJuikGenListBoxImpl"), _CL("SizeChanged"));
		TJuikLayoutItem l = Layout().GetLayoutItemL( iItemLayoutIds.iGroup, 
													 iItemLayoutIds.iFull );
		SetItemHeightL( l.h );
		CEikFormattedCellListBox::SizeChanged();		
		CC_TRAPD(ignore, LayoutSubcellsL() );
		// FIXME: log error? 
	}

	void LayoutSubcellsL()
	{
		CALLSTACKITEM_N(_CL("CJuikGenListBoxImpl"), _CL("LayoutSubcellsL"));
		
		
		CFormattedCellListBoxItemDrawer* itemDrawer = ItemDrawer();
		AknListBoxLayouts::SetupStandardListBox( *this );
		AknListBoxLayouts::SetupStandardFormListbox( itemDrawer );	  

		TSize itemSize = View()->ItemSize();


		// hack to layout scrollbar correctly. Size should be set by Avkon methods above
		// but position is wrong. We can't use LayoutVerticalScrollBar from aknutils.h
		// because it needs TAknWindowLineLayout, which is not in public SDK. Blah.
		if (ScrollBarFrame())
			{
				TRect r = Rect();
				CEikScrollBar* sb = ScrollBarFrame()->VerticalScrollBar();
				if ( sb ) 
					{
						sb->SetPosition( TPoint(r.iBr.iX, r.iTl.iY) );						
						if ( ScrollBarFrame()->TypeOfVScrollBar() == CEikScrollBarFrame::EDoubleSpan )
							{
								CAknDoubleSpanScrollBar* dpSb = (CAknDoubleSpanScrollBar*) sb; 
								TRect sbRect = dpSb->Rect();
								TRect realRect( TPoint(r.iBr.iX, r.iTl.iY), sbRect.Size() );
								dpSb->SetRect( realRect );
								dpSb->SetFixedLayoutRect( realRect );
							}
					}
			}

		TJuikLayoutItem full( TRect(TPoint(0,0), itemSize) );
		TJuikLayoutItem content = full.Combine( Layout().GetLayoutItemL( iItemLayoutIds.iGroup, iItemLayoutIds.iContent ) );


		for ( TInt i = 0; i < iRenderers->Count(); i++ )
			{
				iRenderers->At(i)->LayoutSubcellL( i, content.Rect(), *this );
			}		
	}


	MJuikIconManager& IconManager() const
	{
		return *iIconManager;
	}
private:
	TItemLayoutIds iItemLayoutIds;

	CArrayPtr<MSubcellRenderer>* iRenderers; // own
	MJuikIconManager* iIconManager; // not own
	TInt iProviderId;

	// Marker replacement string descriptor has live as to long as it's used,
	// although it's passed as const TDesC& (I could kill the API designer)
	// See eiklbi.h and SetItemMarkReplacement
	TBuf<10> iMarkerReplacement;
};



EXPORT_C CJuikGenListBox* CJuikGenListBox::NewL( CCoeControl* aParent, const TItemLayoutIds& aLayout, MJuikIconManager& aIconManager )
{
	auto_ptr<CJuikGenListBoxImpl> lb( CJuikGenListBoxImpl::NewL( aParent, aLayout, aIconManager) );
	return lb.release();
}
