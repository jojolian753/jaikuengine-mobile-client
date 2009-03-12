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

#include <flogger.h>

#include "doublelinebox.h"
#include <eikfrlbd.h> 
#include <eiklbv.h>

//#define CB_TRANSLATE_LOG 1
 
void doublelinebox::LogItem(const TDesC& anItem)
{
	CALLSTACKITEM(_L("doublelinebox::LogItem"));


#ifdef CB_TRANSLATE_LOG
	TInt length = anItem.Length();
	
	if (iLog) 
		iLog->write_to_output(anItem.Mid(0,length-8 ));
	
	RArray<TInt> icons;

	TLex lex;
	TInt err, tmp;

	lex=anItem.Mid(length-2,2);
	err = lex.Val(tmp);
	if(!err)  icons.Append( tmp );
	
	lex=anItem.Mid(length-5,2);
	err = lex.Val(tmp);
	if(!err)  icons.Append( tmp );
	
	lex=anItem.Mid(length-8,2);
	err = lex.Val(tmp);
	if(!err)  icons.Append( tmp );
		
	TBool grey = EFalse;

	for (int i=0; i<icons.Count();i++)
	{
		switch (icons[i])
		{
		case 0:
			break;

		case 1:
			if (iLog) iLog->write_to_output(_L(" spkrOn"));
			break;

		case 2:
			if (iLog) iLog->write_to_output(_L(" spkrOff"));
			break;

		case 3:
			if (iLog) iLog->write_to_output(_L(" spkrOn"));
			grey= ETrue;
			break;

		case 4:
			if (iLog) iLog->write_to_output(_L(" spkrOff"));
			grey= ETrue;
			break;

		case 5:
			if (iLog) iLog->write_to_output(_L(" usrAct"));
			break;

		case 6:
			if (iLog) iLog->write_to_output(_L(" usrInact"));
			break;

		case 7:
			if (iLog) iLog->write_to_output(_L(" vibOn"));
			break;

		case 8:
			if (iLog) iLog->write_to_output(_L(" vibOff"));
			break;

		case 9:
			if (iLog) iLog->write_to_output(_L(" vibOn"));
			break;

		case 10:
			if (iLog) iLog->write_to_output(_L(" vibOff"));
			break;

		case 11:
			if (iLog) iLog->write_to_output(_L(" usrInact1"));
			break;

		case 12:
			if (iLog) iLog->write_to_output(_L(" usrInact2"));
			break;

		case 13:
			if (iLog) iLog->write_to_output(_L(" usrInact3"));
			break;
		
		default:
			break;
		}

		if (grey)
		{
			if (iLog) iLog->write_to_output(_L("(?)"));
		}
	}
	icons.Close();
#else
	if (iLog) iLog->write_to_output(anItem);
#endif
}

//-------------------------------------------------------------------------------------------

drawer::drawer(MTextListBoxModel *aTextListBoxModel, const CFont *aFont, CFormattedCellListBoxData *aFormattedCellData, phonebook_i* aBook, Cfile_output_base * aLog, doublelinebox *aListBox) :
		CFormattedCellListBoxItemDrawer(aTextListBoxModel, aFont, aFormattedCellData)
		{
			itemd=aFormattedCellData;
			iTextListBoxModel=aTextListBoxModel;
			iBook = aBook;
			iLog = aLog;
			iListBox = aListBox;
		}


void drawer::DrawItemText (TInt aItemIndex, const TRect &aItemTextRect, 
		TBool aItemIsCurrent, TBool aViewIsEmphasized, TBool aItemIsSelected) const 
{	
	CALLSTACKITEM(_L("drawer::DrawItemText"));

		
	
	
		TBool out_of_date = EFalse;
		contact * con = 0;
		if (iBook != NULL)
		{
			
			con = iBook->GetContact(aItemIndex); 
			
		}
		if ( (con != NULL) && (con->presence != NULL) )
		{
			TTime stamp = con->presence->SendTimeStamp();
			out_of_date = IsOutOfDate(stamp );	
		}

		if ( out_of_date )
		{
			CFormattedCellListBoxData::TColors c = itemd->SubCellColors(1);
				
			c.iText = TRgb(128,128,128);
			c.iHighlightedText = TRgb(128,128,128);
			
			c.iBack = TRgb(255,255,255);
			c.iHighlightedBack = TRgb(170,170,255);
		
			itemd->SetSubCellColorsL(1, c);
			itemd->SetSubCellColorsL(2, c);
			itemd->SetSubCellColorsL(3, c);
			itemd->SetSubCellColorsL(4, c);

			CFormattedCellListBoxData::TColors c2 = itemd->SubCellColors(1);
			c2.iText = TRgb(0,0,0);
			c2.iHighlightedText = TRgb(0,0,0);
			
			c2.iBack = TRgb(255,255,255);
			c.iHighlightedBack = TRgb(170,170,255);
			itemd->SetSubCellColorsL(0, c2);
		}
		else
		{
			
			CFormattedCellListBoxData::TColors c = itemd->SubCellColors(1);
			
			c.iText = TRgb(0,0,0);
			c.iHighlightedText = TRgb(0,0,0);

			c.iBack = TRgb(255,255,255);	
			c.iHighlightedBack = TRgb(170,170,255);

			itemd->SetSubCellColorsL(1, c);
			itemd->SetSubCellColorsL(2, c);
			itemd->SetSubCellColorsL(3, c);
			itemd->SetSubCellColorsL(4, c);

			CFormattedCellListBoxData::TColors c2 = itemd->SubCellColors(1);
			c2.iText = TRgb(0,0,0);
			c2.iHighlightedText = TRgb(0,0,0);
			
			c2.iBack = TRgb(255,255,255);
			c2.iHighlightedBack = TRgb(170,170,255);
			itemd->SetSubCellColorsL(0, c2);
		}
		
		CFormattedCellListBoxItemDrawer::DrawItemText(aItemIndex, aItemTextRect,
			aItemIsCurrent, aViewIsEmphasized, aItemIsSelected);
			
	

		if (aItemIsCurrent)
		{
			
			iListBox->LogVisibleItems(aItemIndex);
				
		}
	}
	

doublelinebox::doublelinebox(phonebook_i * aBook, Cfile_output_base * aLog) : CEikFormattedCellListBox(), book(aBook), iLog(aLog), iLastCurrentItemIndex(-1)
{
	CALLSTACKITEM(_L("doublelinebox::doublelinebox"));

}


void doublelinebox::CreateItemDrawerL(void)
{
	CALLSTACKITEM(_L("doublelinebox::CreateItemDrawerL"));

	CFormattedCellListBoxData* itemd=CFormattedCellListBoxData::NewL();

	CleanupStack::PushL(itemd);

	TMargins marg;
	TMargins marg_image;
	marg.iBottom=marg.iTop=marg_image.iBottom=marg_image.iTop=marg_image.iLeft=marg_image.iRight=0;
	marg.iLeft= 2;
	marg.iRight=0;

	//Name of contact
	itemd->SetSubCellMarginsL(0, marg);		
	itemd->SetSubCellSizeL(0, TSize(114, 20) );
	itemd->SetSubCellPositionL(0, TPoint(18, 0) );
	itemd->SetSubCellBaselinePosL(0, 16);
	itemd->SetSubCellFontL(0, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(0, CGraphicsContext::ELeft);
	
	//Textual PresenceInfo
	itemd->SetSubCellMarginsL(1, marg);
	itemd->SetSubCellSizeL(1, TSize(168, 20) );
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
	itemd->SetSubCellPositionL(3, TPoint(132, 0) );
	itemd->SetSubCellBaselinePosL(3, 16);
	itemd->SetSubCellFontL(3, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(3, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(3,ETrue);

	// Vibrator
	itemd->SetSubCellMarginsL(4, marg_image);
	itemd->SetSubCellSizeL(4, TSize(18, 20) );
	itemd->SetSubCellPositionL(4, TPoint(150, 0) );
	itemd->SetSubCellBaselinePosL(4, 16);
	itemd->SetSubCellFontL(4, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(4, CGraphicsContext::ECenter);
	itemd->SetGraphicsSubCellL(4,ETrue);

	iItemDrawer=new (ELeave) drawer(Model(), iEikonEnv->NormalFont(), itemd, book, iLog, this);

	CleanupStack::Pop();	
}

doublelinebox::~doublelinebox()
{
	CALLSTACKITEM(_L("doublelinebox::~doublelinebox"));


}


void doublelinebox::LogVisibleItems(TInt currentItemIndex)
{
	CALLSTACKITEM(_L("doublelinebox::LogVisibleItems"));

		TBuf<800> buf;
		if (this->View()->ItemIsVisible(currentItemIndex-2))
		{
			buf.Append(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-2));
		}
		if (this->View()->ItemIsVisible(currentItemIndex-1))
		{
			buf.Append(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-1));
		}
		if (this->View()->ItemIsVisible(currentItemIndex))
		{
			buf.Append(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex));
		}
		if (this->View()->ItemIsVisible(currentItemIndex+1))
		{
			buf.Append(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+1));
		}
		if (this->View()->ItemIsVisible(currentItemIndex+2))
		{
			buf.Append(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+2));
		}

		if (currentItemIndex == iLastCurrentItemIndex && buf.Compare(iBuf) == 0)
		{
			return;
		}

//-----------------------------------

		if (iLog) iLog->write_time();
		if (iLog) iLog->write_to_output(_L("Items: "));

		if (this->View()->ItemIsVisible(currentItemIndex-2))
		{
			LogItem(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-2));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-2));
			if (iLog) iLog->write_to_output(_L(" // "));
		}

		if (this->View()->ItemIsVisible(currentItemIndex-1))
		{
			LogItem(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-1));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-1));
			if (iLog) iLog->write_to_output(_L(" // "));
		}

		if (iLog) iLog->write_to_output(_L("["));
		LogItem(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex));
		if (iLog) iLog->write_to_output(_L("]"));

		if (this->View()->ItemIsVisible(currentItemIndex+1))
		{
			if (iLog) iLog->write_to_output(_L(" // "));
			LogItem(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+1));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+1));
		}

		if (this->View()->ItemIsVisible(currentItemIndex+2))
		{
			if (iLog) iLog->write_to_output(_L(" // "));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+2));
			LogItem(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+2));
		}
		if (iLog) iLog->write_nl();

		iBuf=buf;
		iLastCurrentItemIndex = currentItemIndex;
}


TKeyResponse doublelinebox::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	CALLSTACKITEM(_L("doublelinebox::OfferKeyEventL"));

	TKeyResponse resp = CEikFormattedCellListBox::OfferKeyEventL(aKeyEvent, aType);
	//LogVisibleItems(this->View()->CurrentItemIndex());
	return resp;
}






