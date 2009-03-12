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

#include "cm_autotaglist.h"
#include "cm_post.h"
#include "csd_presence.h"
#include "symbian_auto_ptr.h"
#include <eikenv.h>
#include "icons.h"
#include <contextmediauitags.mbg>
#include <akniconarray.h>
#include <contextmediaui.rsg>
#include <coemain.h>
#include <coeaui.h>
#include <eikappui.h>
#include "hintbox.h"
#include "cl_settings.h"
#include "settings.h"

#include "jaiku_layout.h"
#include "app_context.h"



static const int KNbIcons=5;

// static const TIconID iconId[KNbIcons]=
// {
// 	_INIT_T_ICON_ID("c:\\system\\data\\contextmediauitags.mbm", EMbmContextmediauitagsEmpty, EMbmContextmediauitagsEmpty),
// 	_INIT_T_ICON_ID("c:\\system\\data\\contextmediauitags.mbm", EMbmContextmediauitagsUnchecked, EMbmContextmediauitagsUnchecked),
// 	_INIT_T_ICON_ID("c:\\system\\data\\contextmediauitags.mbm", EMbmContextmediauitagsChecked, EMbmContextmediauitagsChecked),
// 	_INIT_T_ICON_ID("c:\\system\\data\\contextmediauitags.mbm", EMbmContextmediauitagsDisabled, EMbmContextmediauitagsDisabled),
// 	_INIT_T_ICON_ID("c:\\system\\data\\contextmediauitags.mbm", EMbmContextmediauitagsAsk, EMbmContextmediauitagsAsk)
// };

#include <avkon.mbg>

static const TIconID iconId[KNbIcons]=
{
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_empty, EMbmAvkonQgn_prop_empty_mask),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_checkbox_off, EMbmAvkonQgn_prop_checkbox_off_mask),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_checkbox_on, EMbmAvkonQgn_prop_checkbox_on_mask),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_radiobutt_off, EMbmAvkonQgn_prop_radiobutt_off_mask),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_bt_unknown, EMbmAvkonQgn_prop_bt_unknown_mask)
};


enum TTagIcon {
	EEmptyIcon = 0,
	EUncheckedIcon = 1,
	ECheckedIcon = 2,
	EDisabledIcon = 3,
	EAskIcon = 4
};

enum EHints {
	ESharingHint = 11,
	EDisableHint,
	ECityHint,
	EPlaceHint,
	EClearHint
};

void GpsTimeSinceStamp(TDes& aInto, TTime stamp, TTime compare)
{
	CALLSTACKITEM_N(_CL(""), _CL("GpsTimeSinceStamp"));

	TTime now=compare;
	TBuf<128> ret;

	TTimeIntervalYears years = now.YearsFrom(stamp);
	TTimeIntervalMonths months = now.MonthsFrom(stamp);
	TTimeIntervalDays days = now.DaysFrom(stamp);

	if (years.Int() > 0 )
	{
		ret.Append(_L(">2D"));
	}
	else if (months.Int() > 0 )
	{
		ret.Append(_L(">2D"));
	}
	else if (days.Int() >= 2)
	{
		ret.Append(_L(">2D"));
	}
	else if (days.Int() >= 1)
	{
		ret.Append(_L(">1D"));
	}
	else
	{
		TTimeIntervalHours hours;
		now.HoursFrom(stamp, hours);
		TTimeIntervalMinutes minutes;
		now.MinutesFrom(stamp, minutes);
		TTimeIntervalSeconds seconds;
		now.SecondsFrom(stamp, seconds);
				
		if (hours.Int() > 0 ) {
			ret.AppendNum(hours.Int());
			ret.Append(_L("h"));
		} else {				
			TInt min = minutes.Int() - hours.Int() * 60;
			TInt secs1 = seconds.Int() - hours.Int() * 60 *60 - min*60;
			if (secs1<0) secs1=0;

			if (min>0) {
				TUint secs=secs1;
				ret.AppendNum(min);
				ret.Append(_L("min"));
				if (min<3) {
					ret.Append(_L(" "));
					ret.AppendNumFixedWidth(secs, EDecimal, 2);
					ret.Append(_L("s"));
				}
			} else {
				ret.AppendNum(secs1);
				ret.Append(_L("s"));
			}
		}
	}

	aInto.Append(ret);
}


EXPORT_C class CAknIconArray * CAutoTagListBox::CreateIconList()
{
	auto_ptr<CAknIconArray> icons(new (ELeave) CAknIconArray(4));
	TInt scale=1;
	if ( ((CEikAppUi*)CEikonEnv::Static()->AppUi())->ApplicationRect().Width()>300) scale=2;
	LoadIcons(icons.get(), iconId, KNbIcons, scale);
	return icons.release();
}

class CAutoTagArrayImpl : public CAutoTagArray, public MContextBase {

	CCMPost* iPost;
	MAskForNames* iAskForNames;
	CEikonEnv*	iEikEnv;
	CHintBox* iHints;
	TInt iFixedPrivacy;
	CAutoTagArrayImpl(MAskForNames* aAskForNames, CHintBox* aHints) : 
			iAskForNames(aAskForNames), iHints(aHints) {
		iEikEnv=CEikonEnv::Static();
		iFixedPrivacy=-1;
	}
	mutable TBuf<100> iBuf;
	TInt MdcaCount() const {
		return 8;
	}
	TInt iSharing;
	TGeoLatLong	iLatLong;
	virtual void SetPost(CCMPost* aPost) {
		CALLSTACKITEM_N(_CL("CAutoTagArrayImpl"), _CL("SetPost"));

		Settings().GetSettingL(SETTING_FIXED_PRIVACY, iFixedPrivacy);

		iPost=aPost;
		iLatLong.iLat.Zero();
		iLatLong.iLong.Zero();
		if (iPost) {
			CBBPresence *p=bb_cast<CBBPresence>(iPost->iPresence());
			if (p) {
				NmeaToLatLong(p->iGps(), iLatLong);
				if (iAskForNames &&
					(	p->iBaseInfo.iCurrent.iLeft()!=TTime(0) || 
						p->iBaseInfo.iCurrent.iBaseName().Length()==0
					) &&
					p->iCellName().Length()==0) {

					if (p->iBaseInfo.iCurrent.iLeft()==TTime(0)) {
						iAskForNames->GetExistingCell(p->iBaseInfo.iCurrent.iBaseId(),
							p->iCellId,
							p->iBaseInfo.iCurrent.iBaseName());
					} else {
						iAskForNames->GetExistingCell(0,
							p->iCellId,
							p->iBaseInfo.iCurrent.iBaseName());
					}
				}
					
			}
			iFlags=iPost->iIncludedTagsBitField();
			if (iFixedPrivacy!=-1) {
				aPost->iSharing()=iFixedPrivacy;
			}
			iSharing=aPost->iSharing();
		} else {
			iFlags=0x0;
			iSharing=0x0;
		}
	}


	void Ask(TInt aIndex) {
		CALLSTACKITEM_N(_CL("CAutoTagArrayImpl"), _CL("Ask"));

		if (!iAskForNames) return;
		CBBPresence *p=bb_cast<CBBPresence>(iPost->iPresence());
		if (!p) return;
		switch(aIndex) {
		case 1: {
			TBuf<50> name;
			if (iAskForNames->NameCity(p->iCellId, name))
				p->iCity()=name;
			}
			if (iHints) iHints->DismissHint(ECityHint);
			break;
		case 2: {
			TBuf<50> name;
			TInt baseid=-1;
			if (p->iBaseInfo.iCurrent.iLeft()==TTime(0)) {
				baseid=p->iBaseInfo.iCurrent.iBaseId();
			}
			if (iAskForNames->NameCell(baseid, p->iCellId, name)) {
				if (baseid>0) {
					p->iBaseInfo.iCurrent.iBaseName()=name;
				}
				p->iCellName()=name;
			}
			if (iHints) iHints->DismissHint(EPlaceHint);
			}
			break;
		}
	}
	TBool IsAskable(TInt aIndex) const {
		if (!iAskForNames) return EFalse;
		const CBBPresence *p=bb_cast<CBBPresence>(iPost->iPresence());
		if (!p) return EFalse;
		switch(aIndex) {
		case 1:
			return ETrue;
		case 2:
			return ETrue;
		/*case 5:
			return ETrue;
		case 6:
			return ETrue;*/
		default:
			return EFalse;
		}
	}
	virtual TBool IsKnown(TInt aIndex) const {
		if (!iPost || !iPost->iPresence()) {
			return EFalse;
		}
		const CBBPresence *p=bb_cast<CBBPresence>(iPost->iPresence());
		switch(aIndex) {
		case 0:
			return (p->iCountry().Length()>0);
		case 1:
			return (p->iCity().Length()>0);
		case 2:
			return (
				 (p->iBaseInfo.iCurrent.iLeft()==TTime(0) &&
				  p->iBaseInfo.iCurrent.iBaseName().Length()>0) ||
				p->iCellName().Length()>0);
		case 3:
			return (p->iCellId.iCellId()!=0);
		case 4:
			return (iSharing!=CCMPost::EPublic && p->iDevices->Count()>0);
		case 5:
			return (p->iCalendar.iCurrent.iDescription().Length()>0);
		case 6:
			return (iLatLong.iLat.Length()>0 && iLatLong.iLong.Length()>0);
		case 7:
			return ETrue;
		}
		return EFalse;
	}
	TInt iFlags;
	virtual TInt GetIncludedTagsBitField() const {
		return iFlags;
	}
	virtual void IncludeTags(TInt aBits) {
		iFlags |= aBits;
	}
	virtual TInt GetIndexBit(TInt aIndex) const {
		TInt flag=0x0;
		switch(aIndex) {
		case 0:
			flag=CCMPost::ECountry;
			break;
		case 1:
			flag=CCMPost::ECity;
			break;
		case 2:
			flag=CCMPost::EBase;
			break;
		case 3:
			flag=CCMPost::ECell;
			break;
		case 4:
			flag=CCMPost::EBt;
			break;
		case 5:
			flag=CCMPost::ECalendar;
			break;
		case 6:
			flag=CCMPost::EGps;
			break;
		}
		return flag;
	}
	virtual void ToggleField(TInt aIndex) {
		if (!iPost) return;
		if (aIndex==7 && iFixedPrivacy==-1) {
			iHints->DismissHint(ESharingHint);
			ToggleSharing();
			return;
		}
		if (!iPost->iPresence()) return;

		if (IsKnown(aIndex)) {
			TInt set=iFlags & GetIndexBit(aIndex);
			if (set)
				iFlags ^= GetIndexBit(aIndex);
			else 
				iFlags |= GetIndexBit(aIndex);
			iHints->DismissHint(EDisableHint);
		} else if (IsAskable(aIndex)) {
			Ask(aIndex);
		}
	}
	virtual void ClearField(TInt aIndex) {
		if (!iPost || !iPost->iPresence()) return;
		CBBPresence *p=bb_cast<CBBPresence>(iPost->iPresence());

		if (aIndex==1) {
			if (iHints) iHints->DismissHint(EClearHint);
			p->iCity().Zero();
		} else if (aIndex==2) {
			if (iHints) iHints->DismissHint(EClearHint);
			p->iBaseInfo.iCurrent.iBaseName().Zero();
			p->iCellName().Zero();
		}/* else if (aIndex==5) {
			p->iCalendar()=TBBCalendar();
		} else if (aIndex==6) {
			iLatLong.iLat.Zero();
			iLatLong.iLong.Zero();
		}*/
	}


	TBool IsEnabled(TInt aIndex) const {
		if (!iPost) {
			return EFalse;
		}
		if (aIndex==7) return ETrue;
		if (!iPost->iPresence()) return EFalse;

		TInt flag=GetIndexBit(aIndex);
		return (iFlags & flag);
	}
	virtual void ToggleSharing() {
		iSharing = (iSharing + 1) % 5;
	}
	virtual TInt GetSharing() const {
		return iSharing;
	}

	TPtrC MdcaPoint(TInt aIndex) const {
		iBuf.Zero();

		if (aIndex==7) {
			switch (iSharing) {
			case CCMPost::EPrivate:
				iEikEnv->ReadResourceAsDes16(iBuf, R_PRIVATE);
				break;
			case CCMPost::EFriends:
				iEikEnv->ReadResourceAsDes16(iBuf, R_FRIENDS);
				break;
			case CCMPost::EPublic:
				iEikEnv->ReadResourceAsDes16(iBuf, R_PUBLIC);
				break;
			case CCMPost::EFamily:
				iEikEnv->ReadResourceAsDes16(iBuf, R_FAMILY);
				break;
			case CCMPost::EFriendsAndFamily:
				iEikEnv->ReadResourceAsDes16(iBuf, R_FRIENDS_AND_FAMILY);
				break;
			}
			iBuf.Append(_L("\t"));
			iBuf.AppendNum(EEmptyIcon);
			return iBuf;
		}

		if (!iPost || !iPost->iPresence()) {
			if (aIndex==1) {
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_CONTEXT);
			}
			iBuf.Append(_L("\t"));
			iBuf.AppendNum(EEmptyIcon);
			return iBuf;
		}
		const CBBPresence *p=bb_cast<CBBPresence>(iPost->iPresence());
		switch(aIndex) {
		case 0:
			iBuf.Append(p->iCountry());
			if (p->iCountry().Length()==0) 
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_COUNTRY);
			break;
		case 1:
			iBuf.Append(p->iCity());
			if (p->iCity().Length()==0) 
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_CITY);
			break;
		case 2:
			if (p->iBaseInfo.iCurrent.iLeft()==TTime(0))
				iBuf=(p->iBaseInfo.iCurrent.iBaseName());
			if (iBuf.Length()==0) iBuf=p->iCellName();
			if (iBuf.Length()==0) 
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_LOCATION);
			break;
		case 3:
			if (p->iCellId.iCellId()!=0) {
				iBuf.Append(_L("GSM "));
				iBuf.AppendNum(p->iCellId.iMCC());
				iBuf.Append(_L(":"));
				iBuf.AppendNum(p->iCellId.iMNC());
				iBuf.Append(_L(":"));
				iBuf.AppendNum(p->iCellId.iLocationAreaCode());
				iBuf.Append(_L(":"));
				iBuf.AppendNum(p->iCellId.iCellId());
			} else {
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_GSM);
			}
			break;
		case 4:
			if (p->iDevices->Count()>0) {
				TBuf<40> btcount;
				iEikEnv->ReadResourceAsDes16(btcount, R_BT_DEVICES);
				iBuf.AppendNum(p->iDevices->Count());
				iBuf.Append(btcount);
			} else {
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_BT);
			}
			break;
		case 5:
			iBuf.Append(p->iCalendar.iCurrent.iDescription());
			if (p->iCalendar.iCurrent.iDescription().Length()==0) 
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_CALENDAR);
			break;
		case 6:
			if (iLatLong.iLat.Length()==0 || iLatLong.iLong.Length()==0) {
				iEikEnv->ReadResourceAsDes16(iBuf, R_NO_GPS);
			} else {
				if (p->iGpsStamp() != TTime(0)) {
					iBuf=_L("GPS [");
					GpsTimeSinceStamp(iBuf, p->iGpsStamp(), iPost->iTimeStamp());
					TBuf<10> ago; iEikEnv->ReadResourceAsDes16(ago, R_AGO);
					iBuf.Append(_L(" "));
					iBuf.Append(ago);
					iBuf.Append(_L("] "));
				}

				iBuf.Append(iLatLong.iLat); iBuf.Append(_L(", ")); iBuf.Append(iLatLong.iLong);
			}
			break;
		}
		iBuf.Append(_L("\t"));
		if (IsKnown(aIndex)) {
			if (IsEnabled(aIndex)) iBuf.AppendNum(ECheckedIcon);
			else iBuf.AppendNum(EUncheckedIcon);
		} else {
			if (IsAskable(aIndex)) iBuf.AppendNum(EAskIcon);
			else iBuf.AppendNum(EDisabledIcon);
		}
		return iBuf;
	}
	friend class CAutoTagArray;
};

EXPORT_C CAutoTagArray*	CAutoTagArray::NewL(MAskForNames* aAskForNames, CHintBox* aHints)
{
	return new (ELeave) CAutoTagArrayImpl(aAskForNames, aHints);
}

class CAutoTagDrawer : public CFormattedCellListBoxItemDrawer, public MContextBase {
public:
	CEikFormattedCellListBox* iParent;
	CAutoTagDrawer(MTextListBoxModel *aModel, 
					const CFont *aFont, 
					CFormattedCellListBoxData *aFormattedCellData, 
					CEikFormattedCellListBox* aParent) :
	CFormattedCellListBoxItemDrawer(aModel, aFont, aFormattedCellData), iParent(aParent) { }

private:
	void DrawItemText(TInt aItemIndex, const TRect &aItemTextRect,
                                TBool aItemIsCurrent, TBool aViewIsEmphasized, 
				TBool aItemIsSelected) const;
};

void CAutoTagDrawer::DrawItemText(TInt aItemIndex, const TRect &aItemTextRect,
                                TBool aItemIsCurrent, TBool aViewIsEmphasized, 
				TBool aItemIsSelected) const
{
	if (iParent->IsFocused()) {
		CFormattedCellListBoxItemDrawer::DrawItemText(aItemIndex, 
			aItemTextRect, aItemIsCurrent, aViewIsEmphasized,
			aItemIsSelected);
	} else {
		CFormattedCellListBoxItemDrawer::DrawItemText(aItemIndex, 
			aItemTextRect, EFalse, aViewIsEmphasized,
			aItemIsSelected);
	}
}
	
class CAutoTagListBoxList : public CEikFormattedCellListBox, public MContextBase {
	friend class CAutoTagListBoxImpl;

	CAutoTagArray* iArray;

	CAknIconArray* iIconList; // marker that we have set the list
	CAutoTagListBoxList(CAutoTagArray* aArray) : iArray(aArray) { }
	CHintBox* iHints;

	void ConstructL(CCoeControl *aParent, CAknIconArray* aIconlist, CHintBox* aHints) {
		iHints=aHints;
		CEikFormattedCellListBox::ConstructL(aParent, 0);
		SetMopParent( aParent );
		View()->SetMatcherCursor(EFalse);
		Model()->SetItemTextArray(iArray);
		Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
               SetItemHeightL(RowHeight());
               ItemDrawer()->FormattedCellData()->SetIconArray(aIconlist);
               iIconList=aIconlist;
       }
       ~CAutoTagListBoxList() {
               if (iIconList) ItemDrawer()->FormattedCellData()->SetIconArray(0);
       }

	TInt iCells;

	void AddSubCell(CFormattedCellListBoxData* itemd,
					TMargins marg, 
					TSize size, 
					TPoint pos, 
					TInt baselinepos, 
					CGraphicsContext::TTextAlign aAlign, TBool aGraphic)
	{
		itemd->SetSubCellMarginsL(iCells, marg);		
		itemd->SetSubCellSizeL(iCells,  size);
		itemd->SetSubCellPositionL(iCells,  pos);
		itemd->SetSubCellBaselinePosL(iCells, baselinepos);
		itemd->SetSubCellFontL(iCells, iEikonEnv->DenseFont() );
		itemd->SetSubCellAlignmentL(iCells, aAlign);
		if (aGraphic)
			itemd->SetGraphicsSubCellL(iCells, ETrue);

		++iCells;
	}

	virtual TInt RowHeight() { 
		TSize sz = Layout().GetLayoutItemL( LG_autotags, LI_autotags__itemsize  ).Size();
		return sz.iHeight;
	}

	virtual TInt Width() { 
		TSize sz = Layout().GetLayoutItemL( LG_autotags, LI_autotags__itemsize  ).Size();
		return sz.iWidth; 
	}


	CFormattedCellListBoxItemDrawer* CreateAndReturnItemDrawerL(void)
	{
		CFormattedCellListBoxData* itemd=CFormattedCellListBoxData::NewL();
		itemd->SetControl( this );
		CleanupStack::PushL(itemd);
		TSize sz = Layout().GetLayoutItemL( LG_autotags, LI_autotags__itemsize  ).Size();

#ifdef __S60V2__
		itemd->SetSkinEnabledL(ETrue);
		//itemd->SetBackgroundSkinStyle(, TRect(TPoint(0,0), sz) );
// 		itemd->SetSkinEnabledL(EFalse);
// 		itemd->SetDrawBackground(ETrue);
#endif


		TJuikLayoutItem parent = Layout().GetLayoutItemL( LG_autotags, LI_autotags__itemsize  );
		TJuikLayoutItem subL = parent.Combine( Layout().GetLayoutItemL( LG_autotags, LI_autotags__text) );
		TMargins8 textMargins =Layout().GetLayoutItemL( LG_autotags, LI_autotags__textmargins  ).Margins();
		TMargins marg = Juik::FromMargins8(textMargins);
		
		AddSubCell(itemd, marg, 
				   subL.Size(),
				   subL.Rect().iTl, 
				   subL.Baseline(),
				   CGraphicsContext::ELeft, EFalse);

		subL = parent.Combine( Layout().GetLayoutItemL( LG_autotags, LI_autotags__icon) );
		TMargins8 iconMargins =Layout().GetLayoutItemL( LG_autotags, LI_autotags__iconmargins  ).Margins();
		marg = Juik::FromMargins8(iconMargins);
		
		AddSubCell(itemd, marg, 
				   subL.Size(),
				   subL.Rect().iTl, 
				   subL.Baseline(),
				   CGraphicsContext::ELeft, ETrue);

		CFormattedCellListBoxItemDrawer* d=new (ELeave) CAutoTagDrawer(Model(), iEikonEnv->DenseFont(), 
																	   itemd, this);
		CleanupStack::Pop();
		return d;
	}
	void CreateItemDrawerL(void) {
		iItemDrawer=CreateAndReturnItemDrawerL();
	}

	const CFont* FontForList() const
	{
		return iEikonEnv->DenseFont();
	}
};

class CAutoTagListBoxImpl : public CAutoTagListBox, public MEikListBoxObserver, public MContextBase  {

	
	CAutoTagListBoxImpl() { }

	CAutoTagArray* iArray;
	CAutoTagListBoxList* iListBox;
	CHintBox* iHints;

	CAknIconArray* iIconList; // marker that we have set the list
	void ConstructL(CAutoTagArray* aArray, const CCoeControl* aParent, 
					CAknIconArray *aIconlist, CHintBox* aHints) {
		
		CALLSTACKITEM_N(_CL("CAutoTagListBoxImpl"), _CL("ConstructL"));
		iBorderWidth = 1;
		iHints=aHints;
		if (iHints) {
			TInt fixed=-1;
			Settings().GetSettingL(SETTING_FIXED_PRIVACY, fixed);
			if (fixed == -1) {
				iHints->AddHintL(ESharingHint, 1, 
					CTextBox::EBottomRight, TPoint(160, 180), 
					140, R_HINT_SHARING);
			}
			iHints->AddHintL(EDisableHint, 1, 
				CTextBox::EBottomRight, TPoint(160, 180), 
				140, R_HINT_DISABLE);
			iHints->AddHintL(ECityHint, 1, 
				CTextBox::EBottomRight, TPoint(160, 180), 
				140, R_HINT_CITY);
			iHints->AddHintL(EPlaceHint, 1, 
				CTextBox::EBottomRight, TPoint(160, 180), 
				140, R_HINT_PLACE);
			iHints->AddHintL(EClearHint, 1, 
				CTextBox::EBottomRight, TPoint(160, 180), 
				140, R_HINT_CLEAR);
		}

		SetContainerWindowL(*aParent);
		iArray=aArray;
		iListBox=new (ELeave) CAutoTagListBoxList(aArray);
		iListBox->ConstructL(this, aIconlist, iHints);

		//SetBorder(TGulBorder::ESingleBlack);
		iListBox->SetListBoxObserver(this);
		iListBox->ActivateL();
	}
	~CAutoTagListBoxImpl() {
		delete iListBox;
	}

	TInt CountComponentControls() const { return 1; }
	CCoeControl* ComponentControl(TInt aIndex) const { return iListBox; }
	
	void Draw(const TRect& aRect) const {
		TRect r = Rect();
		CWindowGc& gc = SystemGc();
		
		gc.SetBrushColor( KRgbWhite );
		gc.SetPenColor( KRgbBlack );
		gc.SetPenSize( TSize( iBorderWidth, iBorderWidth ) );
		gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
		gc.SetPenStyle( CGraphicsContext::ESolidPen );
		
		gc.DrawRect( r );
	}
	void SizeChanged() {
		TRect r=Rect();
		r.Shrink( iBorderWidth, iBorderWidth);
		iListBox->SetRect(r);
	}
	virtual void FocusChanged(TDrawNow aDrawNow) {
		iListBox->SetFocus(IsFocused(), aDrawNow);
		if (! IsFocused() && iHints) iHints->DontShow();
		else ShowHint();
	}
	void ShowHint()
	{
		if (!iHints) return;
		iHints->DontShow();
		TInt idx=iListBox->CurrentItemIndex();
		TInt bit=iArray->GetIndexBit(idx);
		if (iArray->IsKnown(idx)) {
			if (idx==7) {
				iHints->ShowHintIfNotDismissed(ESharingHint);
			} else {
				iHints->ShowHintIfNotDismissed(EDisableHint);
				switch(bit) {
				case CCMPost::ECity:
				case CCMPost::EBase:
					iHints->ShowHintIfNotDismissed(EClearHint);
					break;
				}
			}
		} else {
			switch(bit) {
			case CCMPost::ECity:
				iHints->ShowHintIfNotDismissed(ECityHint);
				break;
			case CCMPost::EBase:
				iHints->ShowHintIfNotDismissed(EPlaceHint);
				break;
			}
		}
	}
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{
		if (aKeyEvent.iCode==EKeyDelete || aKeyEvent.iCode==EKeyBackspace) {
			TInt idx=iListBox->CurrentItemIndex();
			iArray->ClearField(idx);
			ShowHint();
			DrawNow();
			return EKeyWasConsumed;
		}
		TKeyResponse resp=iListBox->OfferKeyEventL(aKeyEvent, aType);
		if (aKeyEvent.iCode==EKeyUpArrow || aKeyEvent.iCode==EKeyDownArrow) {
			ShowHint();
		}
		return resp;
	}


	virtual TInt RowHeight() { return iListBox->RowHeight(); }
	virtual TInt Width() { return iListBox->Width() + BorderWidth()*2; }
	virtual TInt BorderWidth() { return 1; }

	virtual void SetCurrentItemIndexAndDraw(TInt aItemIndex) const {
		iListBox->SetCurrentItemIndexAndDraw(aItemIndex);
		((CAutoTagListBoxImpl*)this)->ShowHint();
	}
	virtual void SetTopItemIndex(TInt aItemIndex) const {
		iListBox->SetTopItemIndex(aItemIndex);
	}
	virtual TInt CurrentItemIndex() const {
		return iListBox->CurrentItemIndex();
	}
	virtual TInt BottomItemIndex() const {
		return iListBox->BottomItemIndex();
	}
	virtual TInt TopItemIndex() const {
		return iListBox->TopItemIndex();
	}
	virtual void SetCurrentItemIndex(TInt aItemIndex) const {
		iListBox->SetCurrentItemIndex(aItemIndex);
		((CAutoTagListBoxImpl*)this)->ShowHint();
	}

	void HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType) {
		if (aEventType==EEventEnterKeyPressed) {
			TInt idx=iListBox->CurrentItemIndex();
			iArray->ToggleField(idx);
			DrawNow();
		}
	}

	friend class CAutoTagListBox;
	friend class auto_ptr<CAutoTagListBoxImpl>;

	TInt iBorderWidth;
};

EXPORT_C CAutoTagListBox* CAutoTagListBox::NewL(CAutoTagArray* aArray, const CCoeControl* aParent, 
						CAknIconArray *aIconList, CHintBox* aHintBox)
{
	auto_ptr<CAutoTagListBoxImpl> ret(new (ELeave) CAutoTagListBoxImpl);
	ret->ConstructL(aArray, aParent, aIconList, aHintBox);
	return ret.release();
}
