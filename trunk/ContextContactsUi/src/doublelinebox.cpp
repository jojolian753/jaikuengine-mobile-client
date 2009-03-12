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
#include "presence_ui_helper.h"

#define rowheight 17

//#define CB_TRANSLATE_LOG 1
 
/*
 * Concepts:
 * !Listbox filtering!
 */


class COptionalFormattedCellListBoxData : public CFormattedCellListBoxData {
public:
	static COptionalFormattedCellListBoxData* NewL() {
		COptionalFormattedCellListBoxData* ret=new (ELeave) COptionalFormattedCellListBoxData;
		CleanupStack::PushL(ret);
		ret->ConstructLD();
		CleanupStack::Pop();
		return ret;
	}
	void SetOriginalSubCellSizeL(TInt aSubCellIndex, TSize aSize) {
		while (iOptionalData.Count() -1 < aSubCellIndex) {
			User::LeaveIfError(iOptionalData.Append( TOptionalItemData() ) );
		}
		CFormattedCellListBoxData::SetSubCellSizeL(aSubCellIndex, aSize);
		iOptionalData[aSubCellIndex].iWidth=aSize.iWidth;
	}
	void SetSubCellGrowIndexL(TInt aSubCellIndex, TInt aGrowIndex) {
		while (iOptionalData.Count() -1 < aSubCellIndex) {
			User::LeaveIfError(iOptionalData.Append( TOptionalItemData() ) );
		}
		iOptionalData[aSubCellIndex].iGrowIndex=aGrowIndex;
	}
	TInt SubCellGrowIndex(TInt aSubCellIndex) {
		if (iOptionalData.Count() -1 < aSubCellIndex) return -1;
		return iOptionalData[aSubCellIndex].iGrowIndex;
	}
	TBool SubCellOptional(TInt aSubCellIndex) {
		if (iOptionalData.Count() -1 < aSubCellIndex) return EFalse;
		return (iOptionalData[aSubCellIndex].iGrowIndex!=-1);
	}
	TInt SubCellVisibleWidth(TInt aSubCellIndex) {
		if (iOptionalData.Count() -1 < aSubCellIndex) return 0;
		return iOptionalData[aSubCellIndex].iWidth;
	}
	~COptionalFormattedCellListBoxData() {
		iOptionalData.Close();
	}
private:
	struct TOptionalItemData {
		TInt	iWidth;
		TInt	iGrowIndex;
		TOptionalItemData() : iWidth(0), iGrowIndex(-1) { }
	};
	RArray<TOptionalItemData> iOptionalData;
};

class drawer : public CFormattedCellListBoxItemDrawer 
{
public:
	drawer(MTextListBoxModel *aTextListBoxModel, 
					const CFont *aFont, 
					COptionalFormattedCellListBoxData *aFormattedCellData, 
					phonebook_i* aBook, Mfile_output_base * aLog, 
					doublelinebox *aListBox);

private:
	void DrawItemText (TInt aItemIndex, 
								const TRect &aItemTextRect,
                                TBool aItemIsCurrent, 
								TBool aViewIsEmphasized, 
								TBool aItemIsSelected) const;
	void ResizeItems(TInt aItemIndex, TBool aReverse);
	
	COptionalFormattedCellListBoxData *itemd;
	MTextListBoxModel * iTextListBoxModel;
	phonebook_i * iBook;
	Mfile_output_base * iLog;
	doublelinebox* iListBox;
};


EXPORT_C CPresenceModel* CPresenceModel::NewL(MDesCArray* aNameArray, MDesCArray* aPresenceArray)
{
	CPresenceModel* ret=new (ELeave) CPresenceModel(aNameArray, aPresenceArray);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop();
	return ret;
}

CPresenceModel::CPresenceModel(MDesCArray* aNameArray, MDesCArray* aPresenceArray) : 
	iNameArray(aNameArray), iPresenceArray(aPresenceArray)
{
}

void CPresenceModel::ConstructL()
{
	CAknFilteredTextListBoxModel::ConstructL(iPresenceArray, ELbmDoesNotOwnItemArray);
}

EXPORT_C void CPresenceModel::SetFilter(CAknListBoxFilterItems * aFilter)
{
	iFilter=aFilter;
}

TPtrC CPresenceModel::ItemText(TInt aItemIndex) const
{
	TInt idx=aItemIndex;
	if (iFilter) idx=iFilter->FilteredItemIndex(aItemIndex);

	return iPresenceArray->MdcaPoint(idx);
}

TInt CPresenceModel::NumberOfItems() const
{
	if (iFilter) return iFilter->FilteredNumberOfItems();
	return iPresenceArray->MdcaCount();
}

const MDesCArray* CPresenceModel::MatchableTextArray() const
{
	return iNameArray;
}

EXPORT_C void doublelinebox::ConstructL(MDesCArray* aNameArray, MDesCArray* aPresenceArray, 
			       const CCoeControl* aParent,TInt aFlags)
{
	iPresenceModel=CPresenceModel::NewL(aNameArray, aPresenceArray);
	CEikFormattedCellListBox::ConstructL(aParent, aFlags);
	SetItemHeightL(rowheight * 2);
}

void doublelinebox::SizeChanged()
{
	CEikFormattedCellListBox::SizeChanged();
#ifdef __S60V2__
	// FIXME3RDED
	// hardcoded offset: the height of the status pane
	itemd->SetSkinParentPos( TPoint(0, 44) );
#endif
}

void doublelinebox::LogItem(const TDesC& anItem)
{
	CALLSTACKITEM_N(_CL("doublelinebox"), _CL("LogItem"));


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

/*
 * Concepts:
 * !Customizing a Listbox!
 */

drawer::drawer(MTextListBoxModel *aTextListBoxModel, const CFont *aFont, 
	       COptionalFormattedCellListBoxData *aFormattedCellData, phonebook_i* aBook, 
	       Mfile_output_base * aLog, doublelinebox *aListBox) :
		CFormattedCellListBoxItemDrawer(aTextListBoxModel, aFont, aFormattedCellData)
		{
			itemd=aFormattedCellData;
			iTextListBoxModel=aTextListBoxModel;
			iBook = aBook;
			iLog = aLog;
			iListBox = aListBox;
		}


void drawer::ResizeItems(TInt aItemIndex, TBool aReverse)
{
	const TDesC& aItemText=iTextListBoxModel->ItemText(aItemIndex);
#ifdef __WINS__
	RDebug::Print(aItemText);
#endif
	TInt tab_pos=0, prev_pos=0;
	TInt index=1; TInt grow=-1;
	TBool seen_grow=EFalse;
	while ( (tab_pos=aItemText.Mid(prev_pos).Locate('\t')) != KErrNotFound) {
		tab_pos+=prev_pos;
		prev_pos=tab_pos+1;
		if ( (itemd->SubCellIsGraphics(index) && aItemText[prev_pos]=='0') ||
				aItemText[prev_pos]=='\t' ) {
			if (itemd->SubCellOptional(index)) {
				grow=itemd->SubCellGrowIndex(index);
				if (!seen_grow) {
					TSize s=itemd->SubCellSize(grow);
					s.iWidth=itemd->SubCellVisibleWidth(grow);
					itemd->SetSubCellSizeL(grow, s);
					seen_grow=ETrue;
				}
				TSize s=itemd->SubCellSize(grow);
				if (!aReverse) {
					s.iWidth+=itemd->SubCellVisibleWidth(index);
				}
				itemd->SetSubCellSizeL(grow, s);
				s=itemd->SubCellSize(index);
				if (aReverse) {
					s.iWidth=itemd->SubCellVisibleWidth(index);
				} else {
#ifdef __WINS__
					TBuf<30> msg=_L("***not showing ");
					msg.AppendNum(index);
					RDebug::Print(msg);
#endif
					s.iWidth=0;
				}
				itemd->SetSubCellSizeL(index, s);
			}
		}
		index++;
	}
	// this makes the assumption that there is only one growing item, and that
	// everything after that follows on the same line
	if (grow!=-1) {
		TInt left=0; TInt top=0;
		{
			TPoint p=itemd->SubCellPosition(grow);
			left=p.iX+itemd->SubCellSize(grow).iWidth+1;
			top=p.iY;
		}
		for (int i=grow+1; i<index; i++) {
			TPoint p=itemd->SubCellPosition(i);
			if (p.iY==top) {
				p.iX=left;
				itemd->SetSubCellPositionL(i, p);
				left+=itemd->SubCellSize(i).iWidth;
			}
		}
	}
}

void drawer::DrawItemText (TInt aItemIndex, const TRect &aItemTextRect, 
		TBool aItemIsCurrent, TBool aViewIsEmphasized, TBool aItemIsSelected) const 
{	
	CALLSTACKITEM_N(_CL("drawer"), _CL("DrawItemText"));

	((drawer*)this)->ResizeItems(aItemIndex, EFalse);

	TBool out_of_date = EFalse;
	contact * con = 0;
	{
		CALLSTACKITEM_N(_CL("drawer"), _CL("DrawItemText1"));
		// we draw the mark on top of the vibrator as necessary
		if (!aItemIsSelected) {
			itemd->SetSubCellSizeL(11, TSize(0, rowheight));
		} else {
			itemd->SetSubCellSizeL(11, TSize(16, rowheight));
		}

		//RDebug::Print(iListBox->Model()->ItemText(aItemIndex));

		if (iBook != 0) {
			CALLSTACKITEM_N(_CL("drawer"), _CL("DrawItemText1.1"));
			con = iBook->GetContact(aItemIndex); 
		} else {
			CFormattedCellListBoxItemDrawer::DrawItemText(aItemIndex, aItemTextRect,
					aItemIsCurrent, aViewIsEmphasized, aItemIsSelected);
			((drawer*)this)->ResizeItems(aItemIndex, ETrue);
			return;
		}
	}

	{
		CALLSTACKITEM_N(_CL("drawer"), _CL("DrawItemText2"));

		if ( (con != NULL) && (con->presence != NULL) )
		{
			TTime stamp = con->presence->iSentTimeStamp();
			out_of_date = IsOutOfDate(stamp, GetTime() );	
		}

		COptionalFormattedCellListBoxData::TColors c = itemd->SubCellColors(1);
		if ( out_of_date ) {
			c.iText = TRgb(128,128,128);
			c.iHighlightedText = TRgb(128,128,128);
				
			c.iBack = TRgb(255,255,255);
			c.iHighlightedBack = TRgb(230,230,230); 
		} else {
			c.iText = TRgb(0,0,0);
			c.iHighlightedText = TRgb(0,0,0);
			c.iBack = TRgb(255,255,255);	
			c.iHighlightedBack = TRgb(170,170,255); // original blue of the list items
		}
		
		itemd->SetSubCellColorsL(1, c);
		itemd->SetSubCellColorsL(2, c);
		itemd->SetSubCellColorsL(3, c);
		itemd->SetSubCellColorsL(4, c);
		itemd->SetSubCellColorsL(5, c);
		itemd->SetSubCellColorsL(6, c);
		itemd->SetSubCellColorsL(7, c);

		COptionalFormattedCellListBoxData::TColors c2 = itemd->SubCellColors(1);
		c2.iText = TRgb(0,0,0);
		c2.iHighlightedText = TRgb(0,0,0);
			
		c2.iBack = TRgb(255,255,255);
		c2.iHighlightedBack = TRgb(230,230,230); // original blue of the list items
		itemd->SetSubCellColorsL(0, c2); // sub cell 0 should always draw text in black

		CFormattedCellListBoxItemDrawer::DrawItemText(aItemIndex, aItemTextRect,
				aItemIsCurrent, aViewIsEmphasized, aItemIsSelected);
			
		if ( aItemIsCurrent) { iListBox->LogVisibleItems(aItemIndex); }
	}

	((drawer*)this)->ResizeItems(aItemIndex, ETrue);
}	

EXPORT_C doublelinebox::doublelinebox(phonebook_i * aBook, Mfile_output_base * aLog) : CEikFormattedCellListBox(), book(aBook), iLog(aLog), iLastCurrentItemIndex(-1)
{
	CALLSTACKITEM_N(_CL("doublelinebox"), _CL("doublelinebox"));

}

TInt doublelinebox::AddSubCell(COptionalFormattedCellListBoxData* itemd,
			       TMargins marg, TSize size, TPoint pos, TInt baselinepos, 
			       CGraphicsContext::TTextAlign aAlign, TBool aGraphic,
			       TInt aGrowIndex)
{
	itemd->SetSubCellMarginsL(iCells, marg);		
	itemd->SetOriginalSubCellSizeL(iCells,  size);
	itemd->SetSubCellPositionL(iCells,  pos);
	itemd->SetSubCellBaselinePosL(iCells, baselinepos);
	itemd->SetSubCellFontL(iCells, iEikonEnv->DenseFont() );
	itemd->SetSubCellAlignmentL(iCells, aAlign);
	if (aGraphic)
		itemd->SetGraphicsSubCellL(iCells, ETrue);
	itemd->SetSubCellGrowIndexL(iCells, aGrowIndex);

	return iCells++;
}

void doublelinebox::CreateItemDrawerL(void)
{
	CALLSTACKITEM_N(_CL("doublelinebox"), _CL("CreateItemDrawerL"));

	itemd=COptionalFormattedCellListBoxData::NewL();

	CleanupStack::PushL(itemd);

	TMargins marg, marg2;
	marg.iBottom=marg.iTop=marg.iLeft=marg.iRight=0;
	marg2.iBottom=marg2.iTop=marg2.iRight=0;
	marg2.iLeft=2;

	TInt iBtCount=4;
	TInt iBtWidth=16;
	TInt iBtWidth2=20;

	//Name of contact
	AddSubCell(itemd, marg, TSize(176- 3*16-3, rowheight), TPoint(16, 0), 13, CGraphicsContext::ELeft);
	itemd->SetSubCellFontL(iCells-1, LatinBold12() );
	
	//Textual PresenceInfo
	TInt infoIndex=AddSubCell(itemd, marg2, TSize(176-3-iBtCount*iBtWidth-2*iBtWidth2, rowheight), TPoint(0, rowheight), 
		rowheight+13, CGraphicsContext::ELeft);

	// Contact Icon (type of number)
	AddSubCell(itemd, marg, TSize(16, rowheight), TPoint(0, 0), 16, CGraphicsContext::ECenter, ETrue);

	// Ringing volume
	AddSubCell(itemd, marg, TSize(16, rowheight), TPoint(144, 0), 16, CGraphicsContext::ECenter, ETrue);

	// Vibrator
	AddSubCell(itemd, marg, TSize(16, rowheight), TPoint(160, 0), 16, CGraphicsContext::ECenter, ETrue);

	// Calendar
	TInt cal=AddSubCell(itemd, marg, TSize(iBtWidth, rowheight), TPoint(134-4*iBtWidth, rowheight), 32, 
		CGraphicsContext::ECenter, ETrue, infoIndex);
#ifdef __WINS__
	TBuf<50> msg=_L("***calendar index: ");
	msg.AppendNum(cal);
	RDebug::Print(msg);
#endif

	// Desktop
	AddSubCell(itemd, marg, TSize(iBtWidth, rowheight), TPoint(134-3*iBtWidth, rowheight), 32, 
		CGraphicsContext::ECenter, ETrue, infoIndex);
	// Laptop
	AddSubCell(itemd, marg, TSize(iBtWidth, rowheight), TPoint(134-2*iBtWidth, rowheight), 32, CGraphicsContext::ECenter, 
		ETrue, infoIndex);
	// PDA
	AddSubCell(itemd, marg, TSize(iBtWidth, rowheight), TPoint(134-1*iBtWidth, rowheight), 32, CGraphicsContext::ECenter, 
		ETrue, infoIndex);

	// BT buddies
	AddSubCell(itemd, marg, TSize(iBtWidth2, rowheight), TPoint(134, rowheight), 32, CGraphicsContext::ECenter, 
		ETrue, infoIndex);

	// other phones
	AddSubCell(itemd, marg, TSize(iBtWidth2, rowheight), TPoint(154, rowheight), 32, CGraphicsContext::ECenter, 
		ETrue, infoIndex);

	//Marked or not
	AddSubCell(itemd, marg, TSize(14, rowheight), TPoint(160, 0), 16, CGraphicsContext::ECenter, ETrue);

	iItemDrawer=new (ELeave) drawer(iPresenceModel, LatinBold12(), 
		itemd, book, iLog, this);

	CleanupStack::Pop();	
}

EXPORT_C doublelinebox::~doublelinebox()
{
	CALLSTACKITEM_N(_CL("doublelinebox"), _CL("~doublelinebox"));

	delete iPresenceModel;
}

EXPORT_C void doublelinebox::LogVisibleItems(TInt currentItemIndex)
{
	CALLSTACKITEM_N(_CL("doublelinebox"), _CL("LogVisibleItems"));

	if (! iLog) return;

		TBuf<800> buf;
		if (this->View()->ItemIsVisible(currentItemIndex-2))
		{
			buf.Append(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex-2));
		}
		if (this->View()->ItemIsVisible(currentItemIndex-1))
		{
			buf.Append(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex-1));
		}
		if (this->View()->ItemIsVisible(currentItemIndex))
		{
			buf.Append(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex));
		}
		if (this->View()->ItemIsVisible(currentItemIndex+1))
		{
			buf.Append(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex+1));
		}
		if (this->View()->ItemIsVisible(currentItemIndex+2))
		{
			buf.Append(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex+2));
		}

		if (currentItemIndex == iLastCurrentItemIndex && buf.Compare(iBuf) == 0)
		{
			return;
		}

//-----------------------------------

		iLog->write_time();
		iLog->write_to_output(_L("Items:"));

		if (this->View()->ItemIsVisible(currentItemIndex-2))
		{
			LogItem(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex-2));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-2));
			if (iLog) iLog->write_to_output(_L("/"));
		}

		if (this->View()->ItemIsVisible(currentItemIndex-1))
		{
			LogItem(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex-1));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex-1));
			if (iLog) iLog->write_to_output(_L("/"));
		}

		iLog->write_to_output(_L("["));
		LogItem(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex));
		iLog->write_to_output(_L("]"));

		if (this->View()->ItemIsVisible(currentItemIndex+1))
		{
			if (iLog) iLog->write_to_output(_L("/"));
			LogItem(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex+1));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+1));
		}

		if (this->View()->ItemIsVisible(currentItemIndex+2))
		{
			if (iLog) iLog->write_to_output(_L("/"));
			//if (iLog) iLog->write_to_output(this->Model()->ItemTextArray()->MdcaPoint(currentItemIndex+2));
			LogItem(iPresenceModel->ItemTextArray()->MdcaPoint(currentItemIndex+2));
		}
		iLog->write_nl();

		iBuf=buf;
		iLastCurrentItemIndex = currentItemIndex;
}


EXPORT_C TKeyResponse doublelinebox::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	CALLSTACKITEM_N(_CL("doublelinebox"), _CL("OfferKeyEventL"));


	TKeyResponse resp = CEikFormattedCellListBox::OfferKeyEventL(aKeyEvent, aType);
	//LogVisibleItems(this->View()->CurrentItemIndex());
	return resp;
}
