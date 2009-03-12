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

#include "juik_subcellrenderer.h"

#include "juik_icons.h"
#include "juik_layout.h"
#include "symbian_auto_ptr.h"
#include "break.h"
#include "reporting.h"

#include <aknenv.h>
#include <aknlists.h>
#include <eikfrlb.h>
#include <eikfrlbd.h>


EXPORT_C void CSubcellRenderer::AddProviderL(MJuikDataProvider& aProvider)
{
	iProviders.AppendL( &aProvider );
}


EXPORT_C CSubcellRenderer::CSubcellRenderer(const TJuikLayoutId& aLayoutId) : iLayoutId(aLayoutId) 
{
	iAvkonLayoutId = EAknLayoutIdELAF;
	CAknEnv* env = CAknEnv::Static();
	if (env) env->GetCurrentLayoutId(iAvkonLayoutId);
}

CSubcellRenderer::~CSubcellRenderer() 
{
	iProviders.Close();
}


TInt CSubcellRenderer::AdjustedBaseline(TJuikLayoutItem& l)	
{	
	TInt base = l.Baseline();	
	if ( iAvkonLayoutId == EAknLayoutIdAPAC ) 
		{
			const CFont* font = l.Font();
			
			TInt maxAscent = ( iAvkonLayoutId == EAknLayoutIdAPAC ) ?
				font->FontMaxAscent() :
				font->AscentInPixels();
			
			TInt realB = base - l.y;
			if (maxAscent > realB )
				{
					base = l.y + maxAscent;
				}
			
		}
	return base;
};


void CSubcellRenderer::LayoutSubcellL( TInt aSubcell, const TRect& aParentRect, CEikFormattedCellListBox& aListBox )
{		
	TJuikLayoutItem l = TJuikLayoutItem( aParentRect ).Combine( Layout().GetLayoutItemL( iLayoutId.iGroup, iLayoutId.iItem ) );

		for (TInt i=0; i < iProviders.Count(); i++)
			{
				MJuikDataProvider* provider = iProviders[i];
				
				if ( provider )
					{
						auto_ptr< CArrayPtr<CGulIcon> > icons( provider->GetIconsL() );
						if ( icons.get() )
							{
								for (TInt i = 0; i < icons->Count(); i++) 
									{
										CC_TRAPD( ignore, JuikIcons::SetIconSizeL( *(icons->At(i)), l.Size() ) );
									}
							}
					}
			}

		CFormattedCellListBoxItemDrawer* itemDrawer = aListBox.ItemDrawer();
		CFormattedCellListBoxData* lbData = itemDrawer->FormattedCellData();
		// Subcell colors
		CFormattedCellListBoxData::TColors colors = lbData->SubCellColors(aSubcell);
		l.GetTextColor(colors.iText);
		l.GetTextHighlightColor(colors.iHighlightedText);
		lbData->SetSubCellColorsL(aSubcell, colors);
		
		TPoint p0 = l.TopLeft();
		TPoint p1 = l.BottomRight();
		

		switch ( l.ltype )
			{
			case TJuikLayoutItem::EText:
				{
					TInt b = AdjustedBaseline(l);
					const CFont* font = l.Font();						
					AknListBoxLayouts::SetupFormTextCell( aListBox, itemDrawer, aSubcell, 
														  font,
														  215, // C
														  l.x,
														  -1, // rm but ignored
														  b, 
														  l.w,
														  CGraphicsContext::ELeft, 
														  p0,
														  p1 );
					lbData->SetSubCellSizeL( aSubcell, TSize(l.w,l.h) );
					lbData->SetSubCellBaselinePosL( aSubcell, b);
					break;
				}
			case TJuikLayoutItem::EIcon:
				{
					AknListBoxLayouts::SetupFormGfxCell( aListBox, itemDrawer, aSubcell, 
														 l.x, l.y, 
														 -1, -1, // r,b but ignored
														 l.w, // W
														 l.h, // H
														 p0,
														 p1 );
					break;
				}
			}
		itemDrawer->FormattedCellData()->SetNotAlwaysDrawnSubCellL( aSubcell, ETrue );
		//itemDrawer->FormattedCellData()->SetTransparentSubCellL( aSubcell, ETrue );
	}
