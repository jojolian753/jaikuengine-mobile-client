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

#include "juik_sizer.h"

#include "juik_control.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"

#include <coecntrl.h> 

void LimitNegative(TSize& aSize)
{
	if ( aSize.iWidth < 0 ) aSize.iWidth = 0;
	if ( aSize.iHeight < 0 ) aSize.iHeight = 0;
}

class CJuikSizerItem : public CBase, public MContextBase, public MJuikSizerItem
{
protected:
	enum TKind
		{
			EControl,
			ESizer 
		};	

public: // constructor and destructors 
	CJuikSizerItem(TKind aKind, TInt aProportion, TInt aFlags) :
		iKind( aKind ), iProportion( aProportion ), iFlags( aFlags ) {}
	
	virtual ~CJuikSizerItem() {}

public: // SizerItem API 
	TSize MinSize() const { return iMinSize; }
	TInt  Proportion() const { return iProportion; }
	TInt  Flags() const { return iFlags; }
	
	TSize CalcMinL() 
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizerItem"), _CL("CalcMinL"));
		iMinSize = MinSizeImplL();
		return iMinSize;
	}

	void SetDimensionL( const TPoint& aPos, const TSize& aSize)
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizerItem"), _CL("SetDimensionL"));
		iPos = aPos;

		TSize sz = aSize;
		LimitNegative( sz );
		iRect = TRect( iPos, sz );
		SetDimensionImplL(iRect);		
	}


	void SetPositionL( const TPoint& aPos, TBool aCallForControls = ETrue )
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizerItem"), _CL("SetPositionL"));
		iPos = aPos;
		iRect = TRect( iPos, iRect.Size() ); 
		if ( aCallForControls || iKind == ESizer )
			{
				SetPositionImplL( aPos );
			}
	}

	const TPoint& Position()
	{
		return iPos;
	}

public: // From MJuikSizerItem
	virtual TRect Rect() { return iRect; }
protected: // subclass api. 
	/**
	 * Subclass should return minimum size of component that it represents
	 */ 
	virtual TSize MinSizeImplL() = 0;
	/**
	 * Subclass should set dimension of component based on aRect.
	 * aRect is always equivalent to iRect.
	 */
	virtual void SetDimensionImplL(const TRect& aRect) = 0; 

	virtual void SetPositionImplL(const TPoint& aPosition) = 0;
private:
	TPoint iPos;
	TSize  iMinSize;
	TRect  iRect;
	TInt   iProportion;
	TInt   iFlags;
public:
	TKind iKind; // needed for typecasting
};


class CJuikControlSizerItem : public CJuikSizerItem
{
public:
	CJuikControlSizerItem( MJuikControl& aControl, TInt aProportion, TInt aFlags )	
		: CJuikSizerItem( EControl, aProportion, aFlags), iControl( &aControl ), iFixedWidth(KErrNotFound)
	{}
	
	virtual TSize MinSizeImplL()
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikControlSizerItem"), _CL("MinSizeImplL"));
		TSize sz = iControl->CoeControl()->MinimumSize();
		if ( iFixedWidth != KErrNotFound )
			{
				sz.iWidth = iFixedWidth;
			}
		return sz;
	}

	virtual void SetDimensionImplL(const TRect& aRect)
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikControlSizerItem"), _CL("SetDimensionImplL"));
		iControl->CoeControl()->SetRect( aRect );
	}

	virtual void SetPositionImplL( const TPoint& aPoint )
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikControlSizerItem"), _CL("SetPositionImplL"));
		iControl->CoeControl()->SetPosition( aPoint );
	}

	virtual void SetFixedWidthL( TInt aWidth )
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikControlSizerItem"), _CL("SetFixedWidthL"));
		iFixedWidth = aWidth;
		TSize sz = iControl->CoeControl()->Size();
		iControl->CoeControl()->SetSize( TSize(aWidth, sz.iHeight) );

	}

	

private:	
	MJuikControl* iControl;
	TInt iFixedWidth;
};


class CJuikSizerSizerItem : public CJuikSizerItem
{
public:	
	CJuikSizerSizerItem( MJuikSizer& aSizer, TInt aProportion, TInt aFlags ) 
		: CJuikSizerItem( ESizer, aProportion, aFlags), iSizer( &aSizer )
	{}
	
	virtual TSize MinSizeImplL()
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizerSizerItem"), _CL("MinSizeImplL"));
		return iSizer->MinSize();
	}
	
	virtual void SetDimensionImplL(const TRect& aRect)
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizerSizerItem"), _CL("SetDimensionImplL"));
		iSizer->SetDimensionL( aRect.iTl, aRect.Size() );
	}
	
	virtual void SetPositionImplL( const TPoint& aPoint )
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizerSizerItem"), _CL("SetPositionImplL"));
		iSizer->SetPositionL( aPoint );
	}

	virtual void SetFixedWidthL( TInt aWidth )
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizerSizerItem"), _CL("SetFixedWidthL"));
		TSize sz = iSizer->PresetMinSize();
		iSizer->SetMinSize( TSize( aWidth, sz.iHeight) );
	}


private:
	MJuikSizer*  iSizer; 
};


class CJuikSizer : public CBase, public MJuikScrollableSizer, public MContextBase
{
	
public:	
	~CJuikSizer()
	{
		iChildren.ResetAndDestroy();
	}

public: // From MJuikSizer
	virtual void SetMinSize(TSize aSize) 
	{
		iPresetMinSize = aSize;
	}

	virtual TInt ItemCount() const
	{
		return iChildren.Count();
	}

	virtual MJuikSizerItem& GetItemL(TInt aIndex) 
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizer"), _CL("GetItemL"));
		if ( aIndex < 0 || iChildren.Count() <= aIndex ) 
			{ 
				Bug(_L("Index out of range")).TechMsg(_L("aIndex %1"), aIndex).Raise();
			}
		return *(iChildren[aIndex]);
	}



	void InsertL( TInt aPos, MJuikControl& aControl, TInt aProportion, TInt aFlags )
	{
		CALLSTACKITEM_N(_CL("CJuikSizer"), _CL("InsertL(MJuikControl"));
		auto_ptr<CJuikSizerItem> item( new (ELeave) CJuikControlSizerItem(aControl, aProportion, aFlags ) );
		InsertL( item.release(), aPos );
	}
	
	void InsertL( TInt aPos, MJuikSizer& aSizer, TInt aProportion, TInt aFlags )
	{
		CALLSTACKITEM_N(_CL("CJuikSizer"), _CL("InsertL(MJuikSizer"));
		auto_ptr<CJuikSizerItem> item( new (ELeave) CJuikSizerSizerItem(aSizer, aProportion, aFlags ) );
		InsertL( item.release(), aPos );
	}

	void AddL( MJuikControl& aControl, TInt aProportion, TInt aFlags )
	{
		CALLSTACKITEM_N(_CL("CJuikSizer"), _CL("AddL(MJuikControl"));
		auto_ptr<CJuikSizerItem> item( new (ELeave) CJuikControlSizerItem(aControl, aProportion, aFlags ) );
		InsertL( item.release(), EndIndex() );
	}
	
	void AddL( MJuikSizer& aSizer, TInt aProportion, TInt aFlags )
	{
		CALLSTACKITEM_N(_CL("CJuikSizer"), _CL("AddL(MJuikSizer"));
		auto_ptr<CJuikSizerItem> item( new (ELeave) CJuikSizerSizerItem(aSizer, aProportion, aFlags ) );
		InsertL( item.release(), EndIndex() );
	}

	TInt EndIndex() { return iChildren.Count(); }
		
	// Everything else is convenience wrapper around this 
	void InsertL( CJuikSizerItem* aItem, TInt aIndex )
	{
		CALLSTACKITEM_N(_CL("CJuikSizer"), _CL("InsertL(CJuikSizerItem"));
		auto_ptr<CJuikSizerItem> item( aItem );
		if ( aIndex < 0 && aIndex > iChildren.Count() ) { Bug(_L("Index out of range")).Raise(); }
		
		iChildren.InsertL( item.get(), aIndex );
		item.release();
	}
	
	void RemoveL( TInt aIndex )
	{
		CALLSTACKITEM_N(_CL("CJuikSizer"), _CL("RemoveL"));
		CJuikSizerItem* item = iChildren[aIndex];
		iChildren.Remove( aIndex );
		delete item;
	}

	void SetDimensionL(const TPoint& aPos, const TSize& aSize)
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizer"), _CL("SetDimensionL"));
		iPos = aPos;
		iSize = aSize;
		LayoutL();
	}

	void SetPositionL(const TPoint& aPos)
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizer"), _CL("SetPositionL"));
		TPoint delta = aPos - iPos;
		iPos = aPos;
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				TPoint p = iChildren[i]->Position();
				p += delta;
				iChildren[i]->SetPositionL( p, EFalse );
			}
		//LayoutL(); // FIXME! this should just change position for all children
	}

	void LayoutL()
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizer"), _CL("LayoutL"));
		CalcMinL();
		RecalcSizesL();
	}

	TSize MinSize()
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikSizer"), _CL("MinSize"));
		TSize sz = CalcMinL();
		sz.iWidth = Max( sz.iWidth, iPresetMinSize.iWidth );
		sz.iHeight = Max( sz.iHeight, iPresetMinSize.iHeight ); 
		return sz;
	}

	virtual TSize PresetMinSize() const
	{
		return iPresetMinSize;
	}

	virtual void SetScrollPositionL(TInt aY)
	{
		return;
	}

	virtual TInt ScrollPosition()
	{
		return 0;
	}

	virtual void LayoutTopChildL() 
	{
		return;
	}

	virtual void LayoutBottomChildL() 
	{
		return;
	}

	virtual void LayoutChildL(TInt aIx) 
	{
		return;
	}



	virtual TRect Rect() const
	{
		return TRect( iPos, iSize );
	}
protected:
	virtual TSize CalcMinL() = 0;
	virtual void RecalcSizesL() = 0;
	
protected:
	RPointerArray<CJuikSizerItem>  iChildren;
	TPoint                         iPos;
	TSize                          iSize;
	TSize                          iPresetMinSize;

};



class CJuikBoxSizer : public CJuikSizer 
{
public:
	static CJuikBoxSizer* NewL(Juik::TBoxOrientation aOrient)
	{
		CALLSTACKITEMSTATIC_N(_CL("CJuikBoxSizer"), _CL("NewL"));
		auto_ptr<CJuikBoxSizer> self( new (ELeave) CJuikBoxSizer(aOrient) );
		//self->ConstructL();
		return self.release();
	}

	
protected:
	CJuikBoxSizer(Juik::TBoxOrientation aOrient) : iOrient(aOrient) {}

	TSize CalcMinL()
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikBoxSizer"), _CL("CalcMinL"));
		if ( ! iChildren.Count() )
			return TSize(0,0);
		
		iTotalStretch = 0;
		iFixedsSize = TSize(0,0);

		// Calculate min size recursively & Calculate total stretch
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* child = iChildren[i];
				child->CalcMinL();
				
				iTotalStretch += child->Proportion();
			}
		
		// Minimum size of sizer based on proportional children
		TInt maxMin = 0; 
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* child = iChildren[i];
				if ( child->Proportion() )
					{
						TInt  cStretch = child->Proportion();
						TSize cSize = child->MinSize();
						
						TInt len = iOrient == Juik::EHorizontal ? cSize.iWidth : cSize.iHeight;
						// DOCME:
						TInt minLen = ( len * iTotalStretch + cStretch - 1 ) / cStretch;
						
						maxMin = Max( maxMin, minLen );
					}
			}

		// 
		TSize minSize(0,0);
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* child = iChildren[i];

				
				TSize cMinSz = child->MinSize();
				TInt p = child->Proportion();
				if ( p )
					{
						TInt len = ( maxMin * p ) / iTotalStretch;
						cMinSz = Assign( iOrient, cMinSz, len );
					}
				else
					{
						iFixedsSize = SumAndMax(iOrient, iFixedsSize, cMinSz);
					}
	
				minSize = SumAndMax(iOrient, minSize, cMinSz );
				
			}	
		return minSize;
	}			


	void RecalcSizesL()
	{
		CALLSTACKITEM_N_NOSTATS(_CL("CJuikBoxSizer"), _CL("RecalcSizesL"));
		if ( ! iChildren.Count() ) 
			return;
		
		// Calculate delta = extra space (distributed between proportional children)
		TInt delta = 0;
		if ( iTotalStretch  ) 
			{
				if ( iOrient == Juik::EHorizontal )
					delta = iSize.iWidth - iFixedsSize.iWidth;
				else
					delta = iSize.iHeight - iFixedsSize.iHeight; 
			}

		// 
		TPoint currentPos = iPos;
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* child = iChildren[i];
				if ( iOrient == Juik::EVertical )
					{
						// Calculate child's height 
						// c-prefix means child's 
						TSize cMinSz = child->MinSize();
						
						TInt cHeight = 0;
						if ( child->Proportion() )
							cHeight = (delta * child->Proportion()) / iTotalStretch;
						else
							cHeight = cMinSz.iHeight;
						
						TPoint cPos( currentPos );
						TSize  cSize( cMinSz.iWidth, cHeight );
						
						// These flags control non-stretch axis 
						if      ( child->Flags() & Juik::EExpand )
							cSize.iWidth = iSize.iWidth; 
						else if ( child->Flags() & Juik::EAlignRight )
							cPos.iX += iSize.iWidth - cSize.iWidth;
						else if ( child->Flags() & Juik::EAlignCenterHorizontal )
							cPos.iX += (iSize.iWidth - cSize.iWidth) / 2;
						
						child->SetDimensionL( cPos, cSize );
						currentPos += TSize( 0, cHeight );
					}
				else
					{
						// Calculate child's width
						// c-prefix means child's 
						TSize cMinSz = child->MinSize();
						
						TInt cWidth = 0;
						if ( child->Proportion() )
							cWidth = (delta * child->Proportion()) / iTotalStretch;
						else
							cWidth = cMinSz.iWidth;
						
						TPoint cPos( currentPos );
						TSize  cSize(cWidth, cMinSz.iHeight );
						
						// These flags control non-stretch axis 
						if      ( child->Flags() & Juik::EExpand )
							cSize.iHeight = iSize.iHeight; 
						else if ( child->Flags() & Juik::EAlignBottom )
							cPos.iY += iSize.iHeight - cSize.iHeight;
						else if ( child->Flags() & Juik::EAlignCenterVertical )
							cPos.iY += (iSize.iHeight - cSize.iHeight) / 2;
						
						child->SetDimensionL( cPos, cSize );
						currentPos += TSize( cWidth, 0 );
					}
			}
	}


	TSize Assign(Juik::TBoxOrientation aOrient, TSize aSize, TInt aValue)
	{
		if ( aOrient == Juik::EHorizontal )
			return TSize( aValue, aSize.iHeight );
		else
			return TSize( aSize.iWidth, aValue );
	}

	
	TSize SumAndMax(Juik::TBoxOrientation aSumOrient, TSize A, TSize B)
	{
		TInt w = 0;
		TInt h = 0;
		if ( aSumOrient == Juik::EHorizontal )
			{
				w = A.iWidth + B.iWidth;
				h = Max(A.iHeight, B.iHeight);
			}
		else 
			{
				w = Max(A.iWidth, B.iWidth);
				h = A.iHeight + B.iHeight;
			}
		return TSize(w,h);
	}

private: 
	// Box orientation, either vertical or horizontal
	Juik::TBoxOrientation iOrient;

	// Sum of proportional multipliers. Used to calculate proportions
	TInt iTotalStretch; 
		
	// Total size of fixed children 
	TSize iFixedsSize;	
};




class CJuikFixedWidthSizer : public CJuikSizer
{
public:
	static CJuikFixedWidthSizer* NewL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CJuikFixedWidthSizer"), _CL("NewL"));

		auto_ptr<CJuikFixedWidthSizer> self( new (ELeave) CJuikFixedWidthSizer() );
		//self->ConstructL();
		return self.release();
	}
	
protected:
	CJuikFixedWidthSizer() {}
	
	void LayoutTopChildL()
	{
		CALLSTACKITEM_N(_CL("CJuikFixedWidthSizer"), _CL("LayoutTopChildL"));
		if ( ! iChildren.Count() )
			return;

		// calculate size
		CJuikSizerItem* child = iChildren[0];
		child->SetFixedWidthL( iPresetMinSize.iWidth );
		TSize cSize = child->CalcMinL();
		//minSize.iHeight += cSize.iHeight;
		
		cSize = child->MinSize();
		
		TPoint pos;
		if ( iChildren.Count() == 1 )
			pos = iPos;
		else
			{
				pos = iChildren[1]->Position();
				pos -= TPoint( 0, cSize.iHeight );
			}

		child->SetDimensionL( pos, cSize );
	}

	void LayoutBottomChildL()
	{
		CALLSTACKITEM_N(_CL("CJuikFixedWidthSizer"), _CL("LayoutBottomChildL"));
		if ( ! iChildren.Count() )
			return;

		// calculate size
		TInt last = iChildren.Count() - 1;
		CJuikSizerItem* child = iChildren[last];
		child->SetFixedWidthL( iPresetMinSize.iWidth );
		TSize cSize = child->CalcMinL();
		//minSize.iHeight += cSize.iHeight;
		
		cSize = child->MinSize();
		
		TPoint pos;
		if ( iChildren.Count() == 1 )
			pos = iPos;
		else
			{
				CJuikSizerItem* before = iChildren[last-1];
				pos = before->Position();
				pos += TPoint( 0, before->Rect().Size().iHeight );
			}
		
		child->SetDimensionL( pos, cSize );
	}
	

	void LayoutChildL(TInt aIx)
	{
		CALLSTACKITEM_N(_CL("CJuikFixedWidthSizer"), _CL("LayoutChildL"));
		if ( ! iChildren.Count() )
			return;

		CJuikSizerItem* child = iChildren[aIx];
		child->SetFixedWidthL( iPresetMinSize.iWidth );
		TSize cSize = child->CalcMinL();
		//minSize.iHeight += cSize.iHeight;
		
		cSize = child->MinSize();
		
		// Figure out y-position for this child

		TPoint pos;
		// First and only child
		if ( iChildren.Count() <= 1 )
			{
				pos = iPos;
			}
		// layout to top
		else if (aIx == 0) 
			{
				CJuikSizerItem* after = iChildren[aIx+1];
				pos = after->Position();
				
			}
		// layout any other position
		else
			{
				CJuikSizerItem* before = iChildren[aIx-1];
				pos = before->Position();
				pos += TPoint( 0, before->Rect().Size().iHeight );
			}
		child->SetDimensionL( pos, cSize );

		// Transform rest of entries downwards
		for (TInt i = aIx + 1; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* c = iChildren[i];
				TPoint p = c->Position();
				c->SetPositionL( p + TSize(0, cSize.iHeight), ETrue ); 
			}
	}

   
	TSize CalcMinL()
	{
		CALLSTACKITEM_N(_CL("CJuikFixedWidthSizer"), _CL("CalcMinL"));
		if ( ! iChildren.Count() )
			return TSize(0,0);
		
		// Calculate min size recursively & Calculate total stretch
		TSize minSize(iPresetMinSize.iWidth,0);
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* child = iChildren[i];
				child->SetFixedWidthL( iPresetMinSize.iWidth );
				TSize cSize = child->CalcMinL();
				minSize.iHeight += cSize.iHeight;
			}		
		return minSize;
	}			
	


	
	void RecalcSizesL()
	{
		CALLSTACKITEM_N(_CL("CJuikFixedWidthSizer"), _CL("RecalcSizesL"));
		if ( ! iChildren.Count() ) 
			return;
		
		TPoint currentPos = iPos;
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* child = iChildren[i];
				TSize cSize = child->MinSize();
				child->SetDimensionL( currentPos, cSize );
				currentPos += TSize(0, cSize.iHeight);
			}
	}

	virtual TInt ScrollPosition()
	{
		return iScrollPos;
	}

	virtual void SetScrollPositionL(TInt aY)
	{
		CALLSTACKITEM_N(_CL("CJuikFixedWidthSizer"), _CL("SetScrollPositionL"));
		TInt delta = aY - iScrollPos;
		iScrollPos = aY;
		for (TInt i=0; i < iChildren.Count(); i++)
			{
				CJuikSizerItem* child = iChildren[i];
				TPoint p = child->Position();
				p += TSize(0, delta);
				child->SetPositionL( p, ETrue );
			}
	}
private:
	TInt iScrollPos;
};



EXPORT_C MJuikSizer* Juik::CreateBoxSizerL(Juik::TBoxOrientation aOrient)
{
	CALLSTACKITEMSTATIC_N(_CL("Juik"), _CL("CreateBoxSizerL"));
	return CJuikBoxSizer::NewL( aOrient );
}


EXPORT_C MJuikScrollableSizer* Juik::CreateFixedWidthSizerL()
{
	CALLSTACKITEMSTATIC_N(_CL("Juik"), _CL("CreateFixedWidthSizerL"));
	return CJuikFixedWidthSizer::NewL(  );
}

