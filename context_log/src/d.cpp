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

#include "doublelinebox.h"

#include <eikfrlbd.h> 

#include "app_context.h"
#include "symbian_auto_ptr.h"

drawer::drawer(MTextListBoxModel *aTextListBoxModel, const CFont *aFont, CFormattedCellListBoxData *aFormattedCellData, doublelinebox * aListBox) :
		CFormattedCellListBoxItemDrawer(aTextListBoxModel, aFont, aFormattedCellData)
		{
			itemd=aFormattedCellData;
			iTextListBoxModel=aTextListBoxModel;
			iListBox =aListBox;
		}


void drawer::DrawItemText (TInt aItemIndex, const TRect &aItemTextRect, 
		TBool aItemIsCurrent, TBool aViewIsEmphasized, TBool aItemIsSelected) const 
{
	CALLSTACKITEM_N(_CL("drawer"), _CL("DrawItemText"));

	//RDebug::Print(iListBox->Model()->ItemText(aItemIndex));

	HBufC * temp = HBufC::NewL(iListBox->Model()->ItemText(aItemIndex).Length());
	CleanupStack::PushL(temp);
	*temp = iListBox->Model()->ItemText(aItemIndex);
	
	TInt i=0;
	TInt err, sp, vib, buddies, others =0;

	TLex lex;
	for (i=0; i<7; i++)
	{
		TInt sep = temp->Des().Locate('\t');
		//RDebug::Print(temp->Des().Mid(0,sep));
		lex=temp->Des().Mid(0,sep);
		if (i==3) { err =  lex.Val(sp); }
		if (i==4) { err =  lex.Val(vib); }
		if (i==5) { err =  lex.Val(buddies); }
		if (i==6) { err =  lex.Val(others); }

		*temp = temp->Des().Mid(sep+1);
	}

	CleanupStack::PopAndDestroy(temp);

	if (sp == 0)
	{
		itemd->SetSubCellSizeL(0, TSize(168, 19) );
		itemd->SetSubCellSizeL(3, TSize(0, 19) ); 
		itemd->SetSubCellSizeL(4, TSize(0, 19) ); 
	}
	else
	{
		itemd->SetSubCellSizeL(0, TSize(132,19));
		itemd->SetSubCellSizeL(3, TSize(16, 19) ); 
		itemd->SetSubCellSizeL(4, TSize(16, 19) );
	}

	if ( buddies == 0 )
	{
		if (others == 0)
		{
			//let's resize some useful subcells!
			itemd->SetSubCellSizeL(1, TSize(176, 19) );
			itemd->SetSubCellSizeL(5, TSize(0, 19) );  //buddies
			itemd->SetSubCellSizeL(6, TSize(0, 19) );  // other phones
		}
		else
		{
			//let's resize some useful subcells!
			itemd->SetSubCellSizeL(1, TSize(156, 19) );
			itemd->SetSubCellSizeL(5, TSize(0, 19) );  //buddies
			itemd->SetSubCellSizeL(6, TSize(20, 19) );  // other phones
			itemd->SetSubCellPositionL(6, TPoint(156, 19) );
		}
	}
	else
	{	
		if (others == 0)
		{
			//let's resize some useless subcells!
			itemd->SetSubCellSizeL(1, TSize(150, 19) ); // location
			itemd->SetSubCellSizeL(5, TSize(20, 19) );  //buddies
			itemd->SetSubCellPositionL(5, TPoint(154, 19) );
			itemd->SetSubCellSizeL(6, TSize(0, 19) );  // other phones
		}
		else
		{
			//let's resize some useless subcells!
			itemd->SetSubCellSizeL(1, TSize(136, 19) ); // location
			itemd->SetSubCellSizeL(5, TSize(20, 19) );  //buddies
			itemd->SetSubCellPositionL(5, TPoint(134, 19) );
			itemd->SetSubCellSizeL(6, TSize(20, 19) );  // other phones
			itemd->SetSubCellPositionL(6, TPoint(154, 19) );
		}
	}





	CFormattedCellListBoxItemDrawer::DrawItemText(aItemIndex, aItemTextRect,
			aItemIsCurrent, aViewIsEmphasized, aItemIsSelected);
	
}
	
doublelinebox::doublelinebox() : CEikFormattedCellListBox() { }


void doublelinebox::CreateItemDrawerL(void)
{
	CALLSTACKITEM_N(_CL("doublelinebox"), _CL("CreateItemDrawerL"));

	auto_ptr<CFormattedCellListBoxData> itemd(CFormattedCellListBoxData::NewL());

	/*TMargins marg;
	TMargins marg_image;
	marg.iBottom=marg.iTop=marg_image.iBottom=marg_image.iTop=marg_image.iLeft=marg_image.iRight=0;
	marg.iLeft= 2;
	marg.iRight=0;

	//Name of contact
	itemd->SetSubCellMarginsL(0, marg);		
	itemd->SetSubCellSizeL(0, TSize(116, 20) );
	itemd->SetSubCellPositionL(0, TPoint(18, 0) );
	itemd->SetSubCellBaselinePosL(0, 16);
	itemd->SetSubCellFontL(0, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(0, CGraphicsContext::ELeft);
	
	//Textual PresenceInfo
	itemd->SetSubCellMarginsL(1, marg);
	itemd->SetSubCellSizeL(1, TSize(178, 20) );
	itemd->SetSubCellPositionL(1, TPoint(0, 20) );
	itemd->SetSubCellBaselinePosL(1, 32);
	itemd->SetSubCellFontL(1, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(1, CGraphicsContext::ELeft);

	// Contact Icon (type of number)
	itemd->SetSubCellMarginsL(2, marg_image);
	itemd->SetSubCellSizeL(2, TSize(18, 20) );
	itemd->SetSubCellPositionL(2, TPoint(0, 0) );
	itemd->SetSubCellBaselinePosL(2, 16);
	itemd->SetSubCellFontL(2, iEikonEnv->DenseFont() );
	itemd->SetGraphicsSubCellL(2,ETrue);
	itemd->SetSubCellAlignmentL(2, CGraphicsContext::ECenter);

	// Ringing volume
	itemd->SetSubCellMarginsL(3, marg_image);
	itemd->SetSubCellSizeL(3, TSize(18, 20) );
	itemd->SetSubCellPositionL(3, TPoint(134, 0) );
	itemd->SetSubCellBaselinePosL(3, 16);
	itemd->SetSubCellFontL(3, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(3, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(3,ETrue);

	// Vibrator
	itemd->SetSubCellMarginsL(4, marg_image);
	itemd->SetSubCellSizeL(4, TSize(28, 20) );
	itemd->SetSubCellPositionL(4, TPoint(150, 0) );
	itemd->SetSubCellBaselinePosL(4, 16);
	itemd->SetSubCellFontL(4, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(4, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(4,ETrue);
	*/

	TMargins marg, marg2;
	marg.iBottom=marg.iTop=marg.iLeft=marg.iRight=0;
	marg2.iBottom=marg2.iTop=marg2.iRight=0;
	marg2.iLeft=2;

	//Name of contact
	itemd->SetSubCellMarginsL(0, marg);		
	//itemd->SetSubCellSizeL(0, TSize(118, 18) );
	itemd->SetSubCellSizeL(0, TSize(132,19));//name of contact
	itemd->SetSubCellPositionL(0, TPoint(16, 0) );
	itemd->SetSubCellBaselinePosL(0, 14);
	itemd->SetSubCellFontL(0, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(0, CGraphicsContext::ELeft);
	
	//Textual PresenceInfo
	itemd->SetSubCellMarginsL(1, marg2);
	//itemd->SetSubCellSizeL(1, TSize(166, 19) );
	itemd->SetSubCellSizeL(1, TSize(136, 19) ); // location
	itemd->SetSubCellPositionL(1, TPoint(0, 19) );
	itemd->SetSubCellBaselinePosL(1, 32);
	itemd->SetSubCellFontL(1, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(1, CGraphicsContext::ELeft);

	// Contact Icon (type of number)
	itemd->SetSubCellMarginsL(2, marg);
	itemd->SetSubCellSizeL(2, TSize(16, 19) );
	itemd->SetSubCellPositionL(2, TPoint(0, 0) );
	itemd->SetSubCellBaselinePosL(2, 17);
	itemd->SetSubCellFontL(2, iEikonEnv->DenseFont() );
	itemd->SetGraphicsSubCellL(2,ETrue);
	itemd->SetSubCellAlignmentL(2, CGraphicsContext::ECenter);

	// Ringing volume
	itemd->SetSubCellMarginsL(3, marg);
	itemd->SetSubCellSizeL(3, TSize(16, 19) );
	itemd->SetSubCellPositionL(3, TPoint(142, 0) );
	itemd->SetSubCellBaselinePosL(3, 17);
	itemd->SetSubCellFontL(3, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(3, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(3,ETrue);

	// Vibrator
	itemd->SetSubCellMarginsL(4, marg);
	itemd->SetSubCellSizeL(4, TSize(16, 19) );
	itemd->SetSubCellPositionL(4, TPoint(158, 0) );
	itemd->SetSubCellBaselinePosL(4, 17);
	itemd->SetSubCellFontL(4, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(4, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(4,ETrue);

	// BT buddies
	itemd->SetSubCellMarginsL(5, marg);
	itemd->SetSubCellSizeL(5, TSize(20, 19) );
	itemd->SetSubCellPositionL(5, TPoint(134, 19) );
	itemd->SetSubCellBaselinePosL(5, 32);
	itemd->SetSubCellFontL(5, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(5, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(5,ETrue);

	// other phones
	itemd->SetSubCellMarginsL(6, marg);
	itemd->SetSubCellSizeL(6, TSize(20, 19) );
	itemd->SetSubCellPositionL(6, TPoint(154, 19) );
	itemd->SetSubCellBaselinePosL(6, 32);
	itemd->SetSubCellFontL(6, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(6, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(6,ETrue);

	//Marked or not
	itemd->SetSubCellMarginsL(7, marg);
	itemd->SetSubCellSizeL(7, TSize(0, 19) );
	itemd->SetSubCellPositionL(7, TPoint(174, 0) );
	itemd->SetSubCellBaselinePosL(7, 17);
	itemd->SetSubCellFontL(7, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(7, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(7,ETrue);

	iItemDrawer=new (ELeave) drawer(Model(), iEikonEnv->NormalFont(), itemd.get(), this);
	itemd.release();
}

doublelinebox::~doublelinebox() { }

