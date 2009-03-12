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

#include "jaiku_layoutdata.h"

#include "jaiku_layoutids.hrh"

#include <e32base.h>

#define MARGINS(x,y) { ERect, (x), (y), P - (2*(x)), P - (2*(y)), ENoFont }
#define MARGINS4(l,t,r,b) { ERect, (l), (t), P - ((l)+(r)), P - ((t)+(b)), ENoFont }
 



namespace JuikLayoutData {
	// Shorthands
	enum 
		{
			IxLegacyPortrait = 0,
			IxLegacyLandscape,
			IxDoublePortrait,
			IxDoubleLandscape,
			IxQvgaPortrait,
			IxQvgaLandscape,
			IxE90,
			KLayoutCount
		};

	enum 
		{
			EGroup =  TJuikLayoutItem::EGroup,
			ERect  =  TJuikLayoutItem::ERect,
			EText  =  TJuikLayoutItem::EText,
			EIcon  =  TJuikLayoutItem::EIcon,
			EMarg  =  TJuikLayoutItem::EMargins,
			EOperation = -10,
		};

	enum
		{ 
			ENoFont = TJuikLayoutItem::ENoFont,
			ENormalFont = TJuikLayoutItem::ENormalFont,
			EDenseFont = TJuikLayoutItem::EDenseFont,
			ETitleFont = TJuikLayoutItem::ETitleFont, 
			ESoftkeyFont = TJuikLayoutItem::ESoftkeyFont, 
			ESmallPrimaryFont = TJuikLayoutItem::ESmallPrimaryFont, 

			ELogicFontPrimary = TJuikLayoutItem::ELogicFontPrimary,
			ELogicFontSecondary = TJuikLayoutItem::ELogicFontSecondary,
			ELogicFontTitle = TJuikLayoutItem::ELogicFontTitle,
			ELogicFontPrimarySmall = TJuikLayoutItem::ELogicFontPrimarySmall,
			ELogicFontDigital = TJuikLayoutItem::ELogicFontDigital,
		};

	enum
		{
			IxType = 0,
			IxX,
			IxY,
			IxW,
			IxH,
			IxFont,
			IxOperation = IxX,
			IxGroup = IxX
		};

	
	typedef const TInt* TRow;
	typedef TInt TMutableRow[6];

	struct TDiffRow
	{
		TJuikLayoutId item;
		TMutableRow row;
	};

	typedef const TDiffRow TConstDiffRow;

	const TInt P( KParentRelative ); // shortcut	
	const TInt KGroupOperation( KErrNotFound );

	enum
		{
			ELastOp   = -88, 
			EDoubleOp = -200,
			EQvgaPortraitOp = -201,
			EQvgaLandscapeOp = -202,
			ELegacyLandscapeOp = -203,
			EQvgaLandscapeXOp = -204,
		};

#define LAYOUTGROUP( x ) { EGroup, x, -1, -1, -1, -1 }
#define LAST()           { EGroup, ELastOp, -1, -1, -1, -1 }
#define LASTDIFF() { {-1, -1}, { EGroup, ELastOp, -1, -1, -1, -1 } }
	

#define GROUP_OPERATION( x ) { x, KGroupOperation }
#define LEGACYLANDSCAPE()  { EOperation, ELegacyLandscapeOp, -1, -1, -1, -1 }
#define DOUBLE_BASE()  { EOperation, EDoubleOp, -1, -1, -1, -1 }
#define QVGAPORTRAIT()    { EOperation,  EQvgaPortraitOp, -1, -1, -1, -1 }
#define QVGALANDSCAPE()    { EOperation, EQvgaLandscapeOp, -1, -1, -1, -1 }
#define QVGALANDSCAPE_X()    { EOperation, EQvgaLandscapeXOp, -1, -1, -1, -1 }

			
#include "jaiku_layoutdata_raw.inl"
	
	TBool IsLastRow( TRow aRow )
	{
		TInt last[] = LAST();
		return last[IxType] == aRow[IxType]  && last[1] == aRow[1];
	}

	TBool IsLastDiffRow( const TDiffRow& aDiffRow )
	{
		return IsLastRow( aDiffRow.row );
	}
	
	TBool IsGroupRow( TRow aRow )
	{
		return aRow[IxType] == EGroup;
	}
	
	CGroup* CreateGroupL(TRow row)
	{
		CGroup* d = new (ELeave) CGroup();
		d->iGroupId = row[1];
		d->iData = new (ELeave) CArrayFixFlat<TJuikLayoutItem>(6);
		return d;
	}



	TJuikLayoutItem CreateLayoutL(TRow row)
	{
		return TJuikLayoutItem( static_cast<TJuikLayoutItem::TLayoutType>(row[0]), 
							 row[1], row[2], row[3], row[4], 
							 static_cast<TJuikLayoutItem::TFontType>(row[5]) );
	}


	TJuikLayoutItem CreateLayoutL(const TMutableRow& row)
	{
		return TJuikLayoutItem( static_cast<TJuikLayoutItem::TLayoutType>(row[0]), 
							 row[1], row[2], row[3], row[4], 
							 static_cast<TJuikLayoutItem::TFontType>(row[5]) );
	}

	TBool IsParentRelative(TInt aValue)
	{
		return (aValue > KParentRelativeLimit);
	}

	TBool IsOperation(TRow row)
	{
		return row[IxType] == EOperation;
	}


	TInt Multiply(TInt aValue, float aMultiplier)
	{
		TInt v = aValue;
		TBool isrelative = IsParentRelative( v );
		if ( isrelative ) v -= KParentRelative;
		v = (TInt)((v*aMultiplier));
		if ( isrelative ) v += KParentRelative;
		return v;
	}

	TInt IdValue(TInt v) { return v; }
	TInt QvgaLandscapeX(TInt v)  { return Multiply(v, 1.366); }
	TInt QvgaLandscapeY(TInt v)  { return Multiply(v, 1.366); }
	TInt LegacyLandscape(TInt x) { return x; }
	TInt DoubleValue(TInt v)     { return Multiply(v, 2.0);	}
	TInt LesserQvgaValue(TInt v)       { return Multiply(v, 1.366); }
	TInt GreaterQvgaValue(TInt v)       { return Multiply(v, 1.538); }
	

	typedef TInt (*int2intF) (TInt);

	struct TTransformFunctions
	{
		int2intF xop;
		int2intF yop;
	};
	

	#define KTransformLegacyPortraitValue   { &IdValue, &IdValue }
	#define KTransformLegacyLandscapeValue  { &IdValue, &IdValue }
	#define KTransformDoublePortraitValue   { &DoubleValue, &DoubleValue }
	#define KTransformDoubleLandscapeValue  { &DoubleValue, &DoubleValue }
	#define KTransformQvgaPortraitValue     { &LesserQvgaValue, &LesserQvgaValue }
	#define KTransformQvgaLandscapeValue    { &LesserQvgaValue, &LesserQvgaValue }
	#define KTransformQvgaLandscapeXValue    { &LesserQvgaValue, &IdValue }
    #define KTransformE90Value              { &DoubleValue, &DoubleValue }

	const TTransformFunctions KTransformLegacyPortrait = KTransformLegacyPortraitValue;
	const TTransformFunctions KTransformLegacyLandscape = KTransformLegacyLandscapeValue;
	const TTransformFunctions KTransformDoublePortrait = KTransformDoublePortraitValue;
	const TTransformFunctions KTransformDoubleLandscape = KTransformDoubleLandscapeValue;
	const TTransformFunctions KTransformQvgaPortrait = KTransformQvgaPortraitValue;
	const TTransformFunctions KTransformQvgaLandscape = KTransformQvgaLandscapeValue;
	const TTransformFunctions KTransformQvgaLandscapeX = KTransformQvgaLandscapeXValue;
	const TTransformFunctions KTransformE90 = KTransformE90Value;


	const TTransformFunctions KTransformFunctions[KLayoutCount] = 
		{
			KTransformLegacyPortraitValue,
			KTransformLegacyLandscapeValue,
			KTransformDoublePortraitValue,
 			KTransformDoubleLandscapeValue, 
			KTransformQvgaPortraitValue,
			KTransformQvgaLandscapeValue,
			KTransformE90Value
		};
	
	void DoOperation(TRow aBase, int2intF xop, int2intF yop, TMutableRow& r)
	{
		r[IxType] = aBase[IxType];
		r[IxX]      = xop( aBase[IxX] );
		r[IxY]      = yop( aBase[IxY] );
		r[IxW]      = xop( aBase[IxW] );
		r[IxH]      = yop( aBase[IxH] );
		
		r[IxFont] = aBase[IxFont];
	}

	void DoOperation(TRow aBase, const TTransformFunctions& aF, TMutableRow& r)
	{
		DoOperation( aBase, aF.xop, aF.yop, r);
	}


   
	void DoOperationForRow(TRow row, TRow aBase, TMutableRow& r) 
	{
		ASSERT( IsOperation(row) );
		TTransformFunctions fs; 
		
		switch ( row[IxOperation] )
			{
			case EDoubleOp:				
				fs = KTransformDoublePortrait;
				break;
			case EQvgaPortraitOp:
				fs = KTransformQvgaPortrait;
				break;
			case EQvgaLandscapeOp:
				fs = KTransformQvgaLandscape;
				break;
			case ELegacyLandscapeOp:
				fs = KTransformLegacyLandscape;
				break;
			case EQvgaLandscapeXOp:
				fs = KTransformQvgaLandscapeX;
				break;
			default:
				ASSERT( EFalse );
				fs = KTransformLegacyLandscape;
				break;
			}
		DoOperation(aBase, fs.xop, fs.yop, r);
	}
		 

	void CopyRow(TRow row, TMutableRow& tgt)
	{
		for(TInt i=0; i < 6; i++) 
			{ tgt[i] = row[i]; }
	}

	void CalculateDiffL(TRow aRow, TRow aBaseRow, TMutableRow& aResult)
	{
		if ( IsOperation( aRow ) )
			{
				DoOperationForRow( aRow, aBaseRow, aResult );
			}
		else
			{
				CopyRow(aRow, aResult);
			}			
	}

	TBool EqualIdsL( TJuikLayoutId x, TJuikLayoutId y )
	{
		return x.iGroup == y.iGroup && x.iItem == y.iItem;
	}
	
	TBool IsGroupOperationFor( TJuikLayoutId aDiff, TJuikLayoutId aItem )
	{
		return aDiff.iGroup == aItem.iGroup && aDiff.iItem == KGroupOperation;
	}

	TInt FindDiffL( const TDiffRow* aDiffs, TJuikLayoutId aItem )
	{
		// brute force
		// first search for group operation
		// then search for row specific diff. 
		// assumption is that group comes first
		TInt i = 0;
		TInt groupOp = KErrNotFound;
		while ( ! IsLastDiffRow( aDiffs[i] )  )
			{
				ASSERT( i < 10000 );
				if ( IsGroupOperationFor( aDiffs[i].item, aItem ) )
					if ( groupOp == KErrNotFound )
						groupOp = i;
					else 
						ASSERT(EFalse);
				
				if ( EqualIdsL( aDiffs[i].item, aItem ) )
					return i;
				i++;
			}
		return groupOp;
	}
	
	void CalculateRowL( const TDiffRow* aDiffs, TJuikLayoutId aItem, TRow aBaseRow, TTransformFunctions& aBasicOp, TMutableRow& aResult)
	{
		TInt ix = FindDiffL( aDiffs, aItem );
		if ( ix >= 0 )
			{
				CalculateDiffL( aDiffs[ix].row, aBaseRow, aResult );
			}
		else 
			{
				DoOperation( aBaseRow, aBasicOp, aResult);
			}
	}

	TInt ConfIndex( TConfiguration aConf ) 
	{
		switch ( aConf ) {
		case ELegacyPortrait: return IxLegacyPortrait;
		case ELegacyLandscape: return IxLegacyLandscape;
		case EDoublePortrait: return IxDoublePortrait;
		case EDoubleLandscape: return IxDoubleLandscape;
		case EQvgaPortrait: return IxQvgaPortrait;
		case EQvgaLandscape: return IxQvgaLandscape;
		case EE90Inner: return IxE90;
		}
		ASSERT( EFalse );	
		return KErrNotFound;
	}

	TConfiguration BaseLayout( TConfiguration aConf ) 
	{
		switch ( aConf ) {
		case ELegacyPortrait:
			return ELegacyPortrait;
		case EQvgaPortrait:
		case EDoublePortrait:
		case ELegacyLandscape: 
			return ELegacyPortrait;
		case EDoubleLandscape:
		case EQvgaLandscape:
		case EE90Inner:
			return ELegacyLandscape;
		}
		ASSERT( EFalse );	
		return ELegacyPortrait;
	}
	
	CArrayPtr< CGroup >* ReadLayoutDataImplL( TConfiguration aConf )
	{
		const TDiffRow* KDiffs[KLayoutCount] =
			{	
				&legacyPortraitDiff[0],
				&legacyLandscapeDiff[0],
				&doublePortraitDiff[0],
				&doubleLandscapeDiff[0],
				&qvgaPortraitDiff[0],
				&qvgaLandscapeDiff[0],
				&e90Diff[0]
			};


		CArrayPtr<CGroup>* r = new (ELeave) CArrayPtrFlat<CGroup>(10);
		
		// 1) select base 
		const TInt (*rawdata)[6] = &KRawLayoutData[0];
		const TDiffRow *basediff = KDiffs[ ConfIndex( BaseLayout(aConf) ) ];
		const TDiffRow *layoutdiff = KDiffs[ ConfIndex(aConf) ];
		
		// 2) select basic operation
		TTransformFunctions baseTransform = KTransformFunctions[ ConfIndex( BaseLayout(aConf) ) ];
		TTransformFunctions layoutTransform = KTransformFunctions[ ConfIndex( aConf ) ];
		
		TInt i = 0;
		TRow rawrow = rawdata[i];

		while ( ! IsLastRow( rawrow )  )
			{
				CGroup* g = CreateGroupL(rawrow);
				r->AppendL(g);
				
				TInt group = rawrow[ IxGroup ];				
				
				++i;
				rawrow = rawdata[i];
				TInt idInGroup = 0;
				while ( ! IsGroupRow( rawrow ) )
					{
						TJuikLayoutId item = { group, idInGroup };
						TMutableRow baserow;
						CalculateRowL( basediff, item, rawrow, baseTransform, baserow);
						TMutableRow realrow;
						CalculateRowL( layoutdiff, item, baserow, layoutTransform, realrow); 
						g->iData->AppendL( CreateLayoutL( realrow ) );
						++idInGroup;
						++i;
						rawrow = rawdata[i];
					}
			}
		return r;
	}


	CArrayPtr< CGroup >* ReadLayoutDataL( TConfiguration aConf )
	{
		return ReadLayoutDataImplL( aConf );
	}
	
}

