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

#include "break.h"
#include "presencedetailview.h"
#include "contextcontacts.hrh"
#include "cu_common.h"

#include "app_context.h"

#include "settings.h"
#include "cl_settings.h"

#include <contextcontactsui.rsg>
#include <aknviewappui.h>
#include <bautils.h>
#include <eiklabel.h>
#include <eikimage.h>
#include "presence_icons.h"
#include <contextcommon.mbg>
#include <contextcontactsui.mbg>
#include <gulalign.h>
#include <aknnavi.h>
#include <akntitle.h> 
#include "list.h"
#include <aknmessagequerydialog.h>
#include "presence_ui_helper.h"

#include <contextvariant.hrh>
#include "ccu_presencestatus.h"
#include "reporting.h"
#include "app_context_impl.h"

const TInt KScreenWidth=176;
const TInt KLabelHeight=15;
const TInt KOffset = 4;

_LIT(KQuestion, "     ?");
_LIT(KTab, "     ");
_LIT(KBlank, " ");
_LIT(KAbrev, "...");

typedef CList<CEikLabel*> CLabelGroup;

class CPresenceDetailContainer : public CCoeControl, public MEikScrollBarObserver, public MContextBase {
public:
	CPresenceDetailContainer(const TDesC& Name, const CBBPresence* PresenceData, TTime aAtTime,
							 CAknView *aView, TBool aIsMe);
	~CPresenceDetailContainer();
	void ConstructL(const TRect& aRect);

	void HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent aEventType);
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	void CreateScrollBars();
	void ShowDetail();
        
private:
	CEikLabel* CreateLabelAndAddToArray(TRgb color, TBool underline);
	CEikImage* CreateImageAndAddToArray();
	CLabelGroup* CreateAndAddLabelGroup();
	void SetTextFromResource(CEikLabel* l, TInt aResource);
	void SetData();
	void MoveUp();
	void MoveDown();
	void SizeChanged();
	
	TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
	void SetSizeOfComponents();
	void SetPositionOfComponents();

	TFileName iMbm;
	const TDesC& GetIconMbmCorrected(TInt aIndes);

	const TDesC&		iName;
	const CBBPresence*	iPresence;
	TTime			iAtTime;
	TEikScrollBarModel iModel;
	CEikScrollBarFrame * iSBFrame; 

	CEikLabel * title;

	CEikLabel * user_given_title;
	CEikLabel * user_given;
	CEikLabel * user_given_time;

	CEikLabel * prev_loc;
	CEikLabel * prev_loc_time;
	CEikLabel * curr_loc;
	CEikLabel * curr_loc_time;

	CEikLabel * cal1;
	CEikLabel * cal1_time;
	CEikLabel * cal2;
	CEikLabel * cal2_time;

	CEikLabel * curr_profile_title;
	CEikLabel * curr_profile_name;
	CEikLabel * curr_profile_speaker;
	CEikLabel * curr_profile_vib;
	
	CEikLabel * bt_nb_title;
	CEikLabel * bt_nb_buddies;
	CEikLabel * bt_nb_others;
	CEikLabel * bt_desktop;
	CEikLabel * bt_laptop;
	CEikLabel * bt_pda;

	CEikLabel * user_active;

	CEikImage * icon_presence;
	CEikImage * icon_speaker;
	CEikImage * icon_vibra;
	CEikImage * icon_buddies;
	CEikImage * icon_others;
	CEikImage * icon_user_activity;
	CEikImage * icon_desktop;
	CEikImage * icon_laptop;
	CEikImage * icon_pda;

	CPtrList<CLabelGroup> * iClickList;

	CLabelGroup* group_user_given;
	CLabelGroup* group_loc;
	CLabelGroup* group_cal;
	CLabelGroup* group_profile;
	CLabelGroup* group_bt;
	CLabelGroup* group_activity;
	
	TInt iScrollSpan; 
	TInt iNbPosition; 
	TInt iThumbPosition; 
	TInt iCurrentPos;

	RPointerArray<CCoeControl> iControls;
	CPtrList<CLabelGroup>::Node * iCurrentGroup;

	CAknView *iView;

	TBool       iIsMe;

};

class CPresenceDetailViewImpl : 
	public CPresenceDetailView,
	public MContextBase
{
private:
	CPresenceDetailViewImpl();
	void ConstructL();

	virtual void SetData(const TDesC& Name, const TDesC& aJaikuNick, const CBBPresence* PresenceData, TTime aAtTime);
	
	TUid Id() const;

	void ShowDetail();
	
        void HandleCommandL(TInt aCommand);
        void DoActivateL(const TVwsViewId& aPrevViewId,
		TUid aCustomMessageId,
		const TDesC8& aCustomMessage);
        void DoDeactivate();

	void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);

	friend class CPresenceDetailView;

	TBuf<100>	iName;
	const CBBPresence*	iPresence;
	TTime		iAtTime;
	CPresenceDetailContainer* iContainer;
	TVwsViewId	iPrevView;
	TInt		iResource;

	TBool       iIsMe;
public:
	virtual ~CPresenceDetailViewImpl();
	void ReleaseCPresenceDetailViewImpl();
};

const TDesC& CPresenceDetailContainer::GetIconMbmCorrected(TInt aIndex)
{
	iMbm=GetIconMbm(aIndex);
	if (iMbm.Left(1).CompareF(_L("c")) == 0) {
		if (! BaflUtils::FileExists(iEikonEnv->FsSession(), iMbm)) {
			iMbm.Replace(0, 1, _L("e"));
		}
	} 
	return iMbm;
}

EXPORT_C CPresenceDetailView* CPresenceDetailView::NewL()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailView"), _CL("NewL"));

	auto_ptr<CPresenceDetailViewImpl> ret(new (ELeave) CPresenceDetailViewImpl);
	ret->ConstructL();
	return ret.release();
}

CPresenceDetailContainer::CPresenceDetailContainer(const TDesC& Name, 
						   const CBBPresence* PresenceData, TTime aAtTime,
												   CAknView *aView, TBool aIsMe) : 
	iName(Name), iPresence(PresenceData), iAtTime(aAtTime), iView(aView), iIsMe( aIsMe ) { }

CPresenceDetailContainer::~CPresenceDetailContainer()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("~CPresenceDetailContainer"));

	delete iClickList;

	iControls.ResetAndDestroy();

	delete iSBFrame;
}

CEikLabel* CPresenceDetailContainer::CreateLabelAndAddToArray(TRgb color,
							      TBool underline)
{
	auto_ptr<CEikLabel> l(new (ELeave) CEikLabel);
	l->SetContainerWindowL(*this);
	l->SetFont(CEikonEnv::Static()->DenseFont());
	l->OverrideColorL(EColorLabelText, color);
	l->SetUnderlining(underline);
	l->SetAllMarginsTo(1);

	User::LeaveIfError(iControls.Append(l.get()));
	return l.release();
}

CEikImage* CPresenceDetailContainer::CreateImageAndAddToArray()
{
	auto_ptr<CEikImage> i(new (ELeave) CEikImage);
	User::LeaveIfError(iControls.Append(i.get()));
	return i.release();
}

CLabelGroup* CPresenceDetailContainer::CreateAndAddLabelGroup()
{
	auto_ptr<CLabelGroup> g(CLabelGroup::NewL());
	iClickList->AppendL(g.get());
	return g.release();
}

void CPresenceDetailContainer::ConstructL(const TRect& aRect)
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("ConstructL"));

	CreateWindowL(); 


	iClickList = CPtrList<CLabelGroup>::NewL();
	
	// Modify colors
	TRgb dark_gray = TRgb(225,225,225);
	TRgb light_gray  = TRgb(210,210,210);
	TRgb original_blue = TRgb(170,170,255);
	
	// TITLE
	title = CreateLabelAndAddToArray(KRgbBlack, ETrue);
	title->SetAlignment(TGulAlignment(EHCenterVCenter));

	// USER GIVEN
	user_given_title = CreateLabelAndAddToArray(KRgbBlack, ETrue);

	user_given = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	
	user_given_time = CreateLabelAndAddToArray(KRgbBlue, EFalse);
	user_given_time->SetAlignment(TGulAlignment(EHRightVCenter));

	//CURRENT BASE
	curr_loc = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	curr_loc_time = CreateLabelAndAddToArray(KRgbBlue, EFalse);
	curr_loc_time->SetAlignment(TGulAlignment(EHRightVCenter));

	// PREVIOUS BASE
	prev_loc = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	prev_loc_time = CreateLabelAndAddToArray(KRgbBlue, EFalse);
	prev_loc_time->SetAlignment(TGulAlignment(EHRightVCenter));
	
	// CALENDAR
	cal2 = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	cal2_time = CreateLabelAndAddToArray(KRgbBlue, EFalse);
	cal2_time->SetAlignment(TGulAlignment(EHRightVCenter));
	cal1 = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	cal1_time = CreateLabelAndAddToArray(KRgbBlue, EFalse);
	cal1_time->SetAlignment(TGulAlignment(EHRightVCenter));
	
	// PROFILE
	curr_profile_title = CreateLabelAndAddToArray(KRgbBlack, ETrue);
	curr_profile_name = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	curr_profile_speaker = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	curr_profile_vib = CreateLabelAndAddToArray(KRgbBlack, EFalse);

	// BT
	bt_nb_title = CreateLabelAndAddToArray(KRgbBlack, ETrue);
	bt_nb_buddies = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	bt_nb_others = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	bt_desktop = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	bt_laptop = CreateLabelAndAddToArray(KRgbBlack, EFalse);
	bt_pda = CreateLabelAndAddToArray(KRgbBlack, EFalse);

	// USER ACTIVITY		
	user_active=CreateLabelAndAddToArray(KRgbBlack, EFalse);

	// ICONS
	icon_presence = CreateImageAndAddToArray();
	icon_speaker = CreateImageAndAddToArray();
	icon_vibra = CreateImageAndAddToArray();
	icon_buddies = CreateImageAndAddToArray();
	icon_others = CreateImageAndAddToArray();
	icon_desktop = CreateImageAndAddToArray();
	icon_laptop = CreateImageAndAddToArray();
	icon_pda = CreateImageAndAddToArray();
	icon_user_activity = CreateImageAndAddToArray();

	group_user_given = CreateAndAddLabelGroup();
	group_user_given->AppendL(user_given_title);
	group_user_given->AppendL(user_given_time);
	group_user_given->AppendL(user_given);

	group_loc = CreateAndAddLabelGroup();
	group_loc->AppendL(prev_loc_time);
	group_loc->AppendL(prev_loc);
	group_loc->AppendL(curr_loc_time);
	group_loc->AppendL(curr_loc);

	group_cal = CreateAndAddLabelGroup();
	group_cal->AppendL(cal1_time);
	group_cal->AppendL(cal1);
	group_cal->AppendL(cal2_time);
	group_cal->AppendL(cal2);
        
	group_profile = CreateAndAddLabelGroup();
	group_profile->AppendL(curr_profile_title);
	group_profile->AppendL(curr_profile_name);
	group_profile->AppendL(curr_profile_speaker);
	group_profile->AppendL(curr_profile_vib);

	group_bt = CreateAndAddLabelGroup();
	group_bt->AppendL(bt_nb_title);
	group_bt->AppendL(bt_nb_buddies);
	group_bt->AppendL(bt_nb_others);
	group_bt->AppendL(bt_desktop);
	group_bt->AppendL(bt_laptop);
	group_bt->AppendL(bt_pda);

	group_activity = CreateAndAddLabelGroup();
	group_activity->AppendL(user_active);
		
	iCurrentGroup = iClickList->iFirst;

	SetData();
	SetRect(aRect);

	SetSizeOfComponents();
	SetPositionOfComponents();
		
	ActivateL();	
}

void CPresenceDetailContainer::SetTextFromResource(CEikLabel* l, TInt aResource)
{
	HBufC * res_reader = iEikonEnv->AllocReadResourceLC(aResource);
	l->SetTextL(*res_reader);
	CleanupStack::PopAndDestroy();	
}

void MakeEventTime(TDes& time, const TBBCalendarEvent& event)
{
	time.Zero();
	TDateTime dt1=event.iStartTime().DateTime();
	TDateTime dt2=event.iEndTime().DateTime();

	TDateTime now=GetTime().DateTime();
	if (dt1.Day()==now.Day() && dt1.Month()==now.Month() && dt1.Year()==now.Year()) {
		time.AppendNumFixedWidth( dt1.Hour(), EDecimal, 2);
		time.Append(_L(":"));
		time.AppendNumFixedWidth( dt1.Minute(), EDecimal, 2);
	} else {
		time.Append(_L("00:00"));
	}
	time.Append(_L(" - "));
	if (dt2.Day()==now.Day() && dt2.Month()==now.Month() && dt2.Year()==now.Year()) {
		time.AppendNumFixedWidth( dt2.Hour(), EDecimal, 2);
		time.Append(_L(":"));
		time.AppendNumFixedWidth( dt2.Minute(), EDecimal, 2);
	} else {
		time.Append(_L("24:00"));
	}
}

void CPresenceDetailContainer::SetData()
{
	// SetTextFromResource(title, R_TITLE);

	SetTextFromResource(user_given_title, R_USER_GIVEN);
     	user_given->SetTextL(KNullDesC);
	user_given_time->SetTextL(KBlank);

    prev_loc->SetTextL(KNullDesC);
	prev_loc_time->SetTextL(KBlank);
	curr_loc->SetTextL(KNullDesC);
	curr_loc_time->SetTextL(KBlank);

	SetTextFromResource(curr_profile_title, R_CURRENT_PROFILE);
	SetTextFromResource(curr_profile_name, R_PROFILE_UNKNOWN);
	curr_profile_speaker->SetTextL(KNullDesC);
	curr_profile_vib->SetTextL(KNullDesC);

	SetTextFromResource(bt_nb_title, R_BT_NB);
    bt_nb_buddies->SetTextL(KNullDesC);
	bt_nb_others->SetTextL(KNullDesC);
	SetTextFromResource(bt_desktop, R_DESKTOP);
	SetTextFromResource(bt_laptop, R_LAPTOP);
	SetTextFromResource(bt_pda, R_PDA);

	user_active->SetTextL(KQuestion);

	icon_presence->MakeVisible(EFalse);
	icon_speaker->MakeVisible(EFalse);
	icon_vibra->MakeVisible(EFalse);
	icon_buddies->MakeVisible(EFalse);
	icon_others->MakeVisible(EFalse);
	icon_user_activity->MakeVisible(EFalse);

        if (iPresence) {
		HBufC * formatted;
		TInt icon = 0;

		TBool out_of_date = IsOutOfDate(iPresence->iSentTimeStamp(), iAtTime);
		if (! iPresence->iSent) {
			// only our own data can be non-sent
			out_of_date=EFalse;
		}
		TBool prev_known=iPresence->iBaseInfo.iPreviousStay.IsSet();
		TBool curr_known=iPresence->iBaseInfo.iCurrent.IsSet();
		TInt user_given_length = iPresence->iUserGiven.iDescription.Value().Length();
		
		HBufC* res_reader = 0;
		_LIT(KRichPresence, "'s Rich Presence"); 
#if 1
		if (title->Text()) {
			res_reader = HBufC::NewLC(title->Text()->Length() + iName.Length() + KBlank().Length() + KRichPresence().Length());
			res_reader->Des().Append(*(title->Text()));
			res_reader->Des().Append(KBlank);
		} else {
			res_reader = HBufC::NewLC(iName.Length() + KRichPresence().Length() );
		}
		res_reader->Des().Append(iName);
		res_reader->Des().Append( KRichPresence() );
		title->SetTextL(*res_reader);
		CleanupStack::PopAndDestroy();
#else
		title->SetTextL(_L("Niina saw you as"));
#endif

		if (user_given_length == 0) {
		        _LIT( KNoPresenceLine, "(No Presence Line)");
	  	        user_given->SetTextL(KNoPresenceLine);
		} else {
			res_reader = HBufC::NewLC(iPresence->iUserGiven.iDescription().Length() + KTab().Length());
			res_reader->Des().Append(KTab);
			res_reader->Des().Append(iPresence->iUserGiven.iDescription());
			user_given->SetTextL(*res_reader);
			CleanupStack::PopAndDestroy();
			res_reader = iEikonEnv->AllocReadResourceLC(R_TIME_FOR_LAST);
			formatted = HBufC::NewLC(res_reader->Des().Length() + 10);
			TBuf<16> timesince; TimeSinceStamp(timesince, 
				iPresence->iUserGiven.iSince(), iAtTime, 2);
			formatted->Des().AppendFormat(*res_reader, &timesince);
			user_given_time->SetTextL(*formatted);
			CleanupStack::PopAndDestroy(2);
		}

		CEikLabel* current_into=0;
		CEikLabel* current_time_into=0;
		CEikLabel* cell_into=0;
		CEikLabel* prev_stay_into=0;
		CEikLabel* prev_stay_time_into=0;
		CEikLabel* prev_visit_into=0;
		CEikLabel* prev_visit_time_into=0;
		CEikLabel* city_into=0;
		if ( iPresence->iBaseInfo.iCurrent.IsSet() ) {
			current_into=curr_loc;
			current_time_into=curr_loc_time;
		} else if (iPresence->iCellName().Length()>0) {
			cell_into=curr_loc;
		}
		if ( iPresence->iBaseInfo.iPreviousStay.IsSet() || iPresence->iBaseInfo.iPreviousVisit.IsSet() ) {
			if (iPresence->iBaseInfo.iCurrent.IsSet()) {
				if (iPresence->iBaseInfo.iPreviousStay.IsSet()) {
					prev_stay_into=prev_loc;
					prev_stay_time_into=prev_loc_time;
				} else {
					prev_visit_into=prev_loc;
					prev_visit_time_into=prev_loc_time;
				}
			} else if ( cell_into==0) {
				if (iPresence->iBaseInfo.iPreviousStay.IsSet()) {
					prev_stay_into=prev_loc;
					prev_stay_time_into=prev_loc_time;
				}
				if (iPresence->iBaseInfo.iPreviousVisit.IsSet()) {
					prev_visit_into=curr_loc;
					prev_visit_time_into=curr_loc_time;
				} else {
					city_into=curr_loc;
				}
			} else {
				if (iPresence->iBaseInfo.iPreviousVisit.IsSet()) {
					prev_visit_into=prev_loc;
					prev_visit_time_into=prev_loc_time;
				} else if (iPresence->iBaseInfo.iPreviousStay.IsSet()) {
					prev_stay_into=prev_loc;
					prev_stay_time_into=prev_loc_time;
				}
			}
		} else {
			city_into=prev_loc;
		}
		if (current_into) {
			current_into->SetTextL(iPresence->iBaseInfo.iCurrent.iBaseName());
		}
		if (current_time_into) {
			res_reader = iEikonEnv->AllocReadResourceLC(R_TIME_FOR_LAST);
			formatted = HBufC::NewLC(res_reader->Des().Length() + 10);
			TBuf<16> timesince; TimeSinceStamp(timesince, 
				iPresence->iBaseInfo.iCurrent.iEntered(), iAtTime, 2);
			formatted->Des().AppendFormat(*res_reader, &timesince);
			current_time_into->SetTextL(*formatted);
			CleanupStack::PopAndDestroy(2);
		}
		if (cell_into) {
			cell_into->SetTextL(iPresence->iCellName());
		}
		if (prev_stay_into) {
			prev_stay_into->SetTextL(iPresence->iBaseInfo.iPreviousStay.iBaseName());
		}
		if (prev_visit_into) {
			prev_visit_into->SetTextL(iPresence->iBaseInfo.iPreviousVisit.iBaseName());
		}
		if (prev_stay_time_into || prev_visit_time_into) {
			res_reader = iEikonEnv->AllocReadResourceLC(R_TIME_LEFT_AGO);
			formatted = HBufC::NewLC(res_reader->Des().Length() + 10);
			TBuf<16> timesince; 
			if (prev_stay_time_into) {
				TimeSinceStamp(timesince, 
					iPresence->iBaseInfo.iPreviousStay.iLeft(), iAtTime, 2);
				formatted->Des().AppendFormat(*res_reader, &timesince);
				prev_stay_time_into->SetTextL(*formatted);
			} else {
				TimeSinceStamp(timesince, 
					iPresence->iBaseInfo.iPreviousVisit.iLeft(), iAtTime, 2);
				formatted->Des().AppendFormat(*res_reader, &timesince);
				prev_visit_time_into->SetTextL(*formatted);
			}
			CleanupStack::PopAndDestroy(2);
		}
		if (city_into) {
			auto_ptr<HBufC> city(HBufC::NewL(iPresence->iCity().Length() + 2 + iPresence->iCountry().Length()));
			city->Des().Append(iPresence->iCity());
			if (city->Length()>0 && iPresence->iCountry().Length()>0) {
				city->Des().Append(_L(", "));
				city->Des().Append(iPresence->iCountry());
			}
			city_into->SetTextL(*city);
		}
		if (!(current_into || prev_stay_into || prev_visit_into || cell_into || city_into)) {
			SetTextFromResource(prev_loc, R_LOCATION_UNKNOWN);
		}
		
		const TBBCalendar& cal = iPresence->iCalendar;
		const TBBCalendarEvent* event1=0, *event2=0;

		TBool hasPrev = cal.iPrevious.iDescription().Length() > 0;
		TBool hasCurrent = cal.iCurrent.iDescription().Length() > 0;
		TBool hasNext = cal.iNext.iDescription().Length() > 0;
		if ( hasPrev )
		    {
			event1=&(cal.iPrevious);
			}

		if ( hasCurrent )
		    {
		    if (event1) event2=&(cal.iCurrent);
		    else        event1=&(cal.iCurrent);
			}

		TInt nextOnLine = KErrNotFound;
		TBuf<100> nextTxt;
		if ( hasNext )
		    {
		    nextTxt.Append( _L("Next:") );
		    nextTxt.Append( cal.iNext.iDescription().Left( nextTxt.MaxLength() - nextTxt.Length() ) );
		    if (event1 && !event2) 
			{
			event2=&(cal.iNext);
			nextOnLine = 2;
			}
		    if (!event1) 
			{
			event1=&(cal.iNext);
			nextOnLine = 1;
		}
		}

		if (event1)
		    {		    
		    if (nextOnLine == 1)
			cal1->SetTextL( nextTxt );
		    else
			cal1->SetTextL( event1->iDescription() );
		    
			TBuf<16> time;
			MakeEventTime(time, *event1);
			cal1_time->SetTextL(time);
		    }
		else {
			_LIT(KNoCalendar, "No current calendar events");
			cal1->SetTextL(KNoCalendar);
			cal1_time->SetTextL(KNullDesC);
		}

		if (event2) 
		    {
		    if (nextOnLine == 2)
			cal2->SetTextL( nextTxt );
		    else
			cal2->SetTextL( event2->iDescription() );
			TBuf<16> time;
			MakeEventTime(time, *event2);
			cal2_time->SetTextL(time);
		    } 
		else
		    {
			cal2->SetTextL(KNullDesC);
			cal2_time->SetTextL(KNullDesC);
		}
		
#ifdef __JAIKU__
		// PRESENCE STATUS ICON
		icon = KErrNotFound;

		// FIXME fix this kind of shit when new presence details view is written 
		TBool enabled = EFalse;
		Settings().GetSettingL( SETTING_PRESENCE_ENABLE, enabled );


		switch ( PresenceStatusL( iPresence, enabled, iIsMe ) )
			{
		    case EPresenceNone: 
			icon = KErrNotFound;
			break;
		    case EPresenceGray:
			icon = GetIconIndex( EMbmContextcontactsuiLight_gray );
			break;
		    case EPresenceRed: 
			icon = GetIconIndex( EMbmContextcontactsuiLight_red );
			break;
		    case EPresenceYellow:
			icon = GetIconIndex( EMbmContextcontactsuiLight_yellow );
			break;
		    case EPresenceGreen: 
			icon = GetIconIndex( EMbmContextcontactsuiLight_green );
			break;
		    default:
			break;
			}
		if ( icon >= 0 )
			{
		icon_presence->CreatePictureFromFileL(GetIconMbmCorrected(icon), GetIconBitmap(icon), GetIconMask(icon));
		icon_presence->MakeVisible(ETrue);
		    }
#endif


		// PROFILE NAME
		res_reader = HBufC::NewLC(iPresence->iProfile.iProfileName().Length()+KBlank().Length());
		res_reader->Des().Append(KBlank);
		res_reader->Des().Append(iPresence->iProfile.iProfileName());
		curr_profile_name->SetTextL(*res_reader);
		CleanupStack::PopAndDestroy();
		
		// SPEAKER
		res_reader = iEikonEnv->AllocReadResourceLC(R_SPEAKER);
		formatted = HBufC::NewLC(KTab().Length() + res_reader->Des().Length() + 10);
		formatted->Des().Append(KTab);
		formatted->Des().Append(*res_reader);
		formatted->Des().Append(KBlank);
		CleanupStack::Pop();
		CleanupStack::PopAndDestroy();
		CleanupStack::PushL(formatted);
		if ( iPresence->iProfile.iRingingType() == TBBProfile::ERingingTypeSilent ) {
			res_reader = iEikonEnv->AllocReadResourceLC(R_SILENT);
			formatted->Des().Append(*res_reader);
			if (out_of_date) {
				icon = GetIconIndex(EMbmContextcommonSpeaker_off_grey);
			} else {
				icon = GetIconIndex(EMbmContextcommonSpeaker_off);
			}
		} else {
			res_reader = iEikonEnv->AllocReadResourceLC(R_SP_VOLUME);
			formatted->Des().AppendFormat(*res_reader, iPresence->iProfile.iRingingVolume());
			if (out_of_date) {
				icon = GetIconIndex(EMbmContextcommonSpeaker_on_grey);
			} else {
				icon = GetIconIndex(EMbmContextcommonSpeaker_on);
			}	   
		}
		curr_profile_speaker->SetTextL(*formatted);
		CleanupStack::PopAndDestroy(2);
		icon_speaker->CreatePictureFromFileL(GetIconMbmCorrected(icon), GetIconBitmap(icon), GetIconMask(icon));
		icon_speaker->MakeVisible(ETrue);


		// VIBRATOR
		res_reader = iEikonEnv->AllocReadResourceLC(R_VIBRATOR);
		formatted = HBufC::NewLC(KTab().Length() + res_reader->Des().Length() +20);
		formatted->Des().Append(KTab);
		formatted->Des().Append(*res_reader);
		formatted->Des().Append(KBlank);
		CleanupStack::Pop();
		CleanupStack::PopAndDestroy();
		CleanupStack::PushL(formatted);
		if ( iPresence->iProfile.iVibra() ) {
			res_reader = iEikonEnv->AllocReadResourceLC(R_ON);
			if (out_of_date) {
				icon = GetIconIndex(EMbmContextcommonVibrator_on_grey);
			} else {
				icon = GetIconIndex(EMbmContextcommonVibrator_on);
			}
		} else {
			if (out_of_date) {
				icon = GetIconIndex(EMbmContextcommonVibrator_off_grey);
			} else {
				icon = GetIconIndex(EMbmContextcommonVibrator_off);
			}
			res_reader = iEikonEnv->AllocReadResourceLC(R_OFF);
		}
		formatted->Des().Append(*res_reader);
                curr_profile_vib->SetTextL(*formatted);
		CleanupStack::PopAndDestroy(2);
		icon_vibra->CreatePictureFromFileL(GetIconMbmCorrected(icon), GetIconBitmap(icon), GetIconMask(icon));
		icon_vibra->MakeVisible(ETrue);
		
		// BT Buddies
		res_reader = iEikonEnv->AllocReadResourceLC(R_BUDDIES_TEMPLATE);
		formatted = HBufC::NewLC(res_reader->Des().Length() + 10);
                formatted->Des().AppendFormat(*res_reader, iPresence->iNeighbourhoodInfo.iBuddies());
		bt_nb_buddies->SetTextL(*formatted);
		CleanupStack::PopAndDestroy(2);
		switch (iPresence->iNeighbourhoodInfo.iBuddies()) {
			case 0:
				icon = 0;
				break;
			case 1:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonBuddy1_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonBuddy1);
				}
				break;
			case 2:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonBuddy2_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonBuddy2);
				}
				break;
			case 3:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonBuddy3_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonBuddy3);
				}
				break;
			default:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonBuddy4_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonBuddy4);
				}
				break;
		}
		icon_buddies->CreatePictureFromFileL(GetIconMbmCorrected(icon), GetIconBitmap(icon), GetIconMask(icon));
		if (icon!=0) icon_buddies->MakeVisible(ETrue);

		// BT Others
		res_reader = iEikonEnv->AllocReadResourceLC(R_OTHERS_TEMPLATE);
		formatted = HBufC::NewLC(res_reader->Des().Length() + 10);
                formatted->Des().AppendFormat(*res_reader, iPresence->iNeighbourhoodInfo.iOtherPhones());
		bt_nb_others->SetTextL(*formatted);
		CleanupStack::PopAndDestroy(2);
		switch (iPresence->iNeighbourhoodInfo.iOtherPhones())
		{
			case 0:
				icon = 0;
				break;
			case 1:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonOther1_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonOther1);
				}
				break;
			case 2:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonOther2_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonOther2);
				}
				break;
			case 3:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonOther3_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonOther3);
				}
				break;
			default:
				if (out_of_date) {
					icon = GetIconIndex(EMbmContextcommonOther4_grey);
				} else {
					icon = GetIconIndex(EMbmContextcommonOther4);
				}
				break;
		}
		icon_others->CreatePictureFromFileL(GetIconMbmCorrected(icon), 
			GetIconBitmap(icon), GetIconMask(icon));
		if (icon!=0) icon_others->MakeVisible(ETrue);

		// BT DESKTOP
		if (iPresence->iNeighbourhoodInfo.iDesktops() > 0) {
			if (out_of_date) {
				icon = GetIconIndex(EMbmContextcommonDesktop_grey);
			} else {
				icon = GetIconIndex(EMbmContextcommonDesktop);
			}
			icon_desktop->CreatePictureFromFileL(GetIconMbmCorrected(icon), 
				GetIconBitmap(icon), GetIconMask(icon));
			icon_desktop->MakeVisible(ETrue);
		} else {
			icon_desktop->MakeVisible(EFalse);
			bt_desktop->SetTextL(KBlank);
		}
		// BT LAPTOP
		if (iPresence->iNeighbourhoodInfo.iLaptops() > 0) {
			if (out_of_date) {
				icon = GetIconIndex(EMbmContextcommonLaptop_grey);
			} else {
				icon = GetIconIndex(EMbmContextcommonLaptop);
			}
			icon_laptop->CreatePictureFromFileL(GetIconMbmCorrected(icon), 
				GetIconBitmap(icon), GetIconMask(icon));
			icon_laptop->MakeVisible(ETrue);
		} else {
			icon_laptop->MakeVisible(EFalse);
			bt_laptop->SetTextL(KBlank);
		}
		// BT PDA
		if (iPresence->iNeighbourhoodInfo.iPDAs() > 0) {
			if (out_of_date) {
				icon = GetIconIndex(EMbmContextcommonPda_grey);
			} else {
				icon = GetIconIndex(EMbmContextcommonPda);
			}
			icon_pda->CreatePictureFromFileL(GetIconMbmCorrected(icon), 
				GetIconBitmap(icon), GetIconMask(icon));
			icon_pda->MakeVisible(ETrue);
		} else {
			icon_pda->MakeVisible(EFalse);
			bt_pda->SetTextL(KBlank);
		}

		// USER ACTIVITY
		if (out_of_date) {
			res_reader = iEikonEnv->AllocReadResourceLC(R_OUT_OF_DATE);
			formatted = HBufC::NewLC(res_reader->Des().Length() +10);
			TBuf<16> timesince; TimeSinceStamp(timesince, iPresence->iSentTimeStamp(), 
				iAtTime, 10);
			formatted->Des().AppendFormat(*res_reader, &timesince );
			user_active->SetTextL(*formatted);
			CleanupStack::PopAndDestroy(2);
			user_active->OverrideColorL(EColorLabelText, KRgbRed);
		} else {
			if (iPresence->iUserActive.iActive() ) {
				res_reader = iEikonEnv->AllocReadResourceLC(R_USER_ACTIVE_TEMPLATE);
				formatted = HBufC::NewLC(res_reader->Des().Length() + KTab().Length());
				formatted->Des().Append(KTab);
				formatted->Des().Append(*res_reader);
				user_active->SetTextL(*formatted);
				CleanupStack::PopAndDestroy(2);
				icon = GetIconIndex(EMbmContextcommonUser_active);
			} else {
				if (iPresence->iUserActive.iSince() != TTime(0) ) {
					res_reader = iEikonEnv->AllocReadResourceLC(R_USER_INACTIVE_TEMPLATE);
					formatted = HBufC::NewLC(res_reader->Des().Length() +10);
					formatted->Des().Append(KTab);
					TBuf<16> timesince; TimeSinceStamp(timesince, 
						iPresence->iUserActive.iSince(), iAtTime, 2);
					formatted->Des().AppendFormat(*res_reader,&timesince);
					user_active->SetTextL(*formatted);
					CleanupStack::PopAndDestroy(2);
					TTimeIntervalMinutes minutes; TTime now=GetTime();
					now.MinutesFrom(iPresence->iUserActive.iSince(), minutes);
					if (minutes.Int() <= 10) {
						icon = GetIconIndex(EMbmContextcommonUser_inactive_lvl_1);
					} else if (minutes.Int() <= 60) {
						icon = GetIconIndex(EMbmContextcommonUser_inactive_lvl_2);
					} else if (minutes.Int() <= 240) {
						icon = GetIconIndex(EMbmContextcommonUser_inactive_lvl_3);
					} else {
						icon = GetIconIndex(EMbmContextcommonUser_inactive);
					}
				} else {
					icon = GetIconIndex(EMbmContextcommonUser_inactive);
					user_active->SetTextL(_L("[activity unknown]"));
				}
			}
			icon_user_activity->CreatePictureFromFileL(GetIconMbmCorrected(icon), GetIconBitmap(icon), GetIconMask(icon));
			icon_user_activity->MakeVisible(ETrue);
		}	
		
	}
}



void CPresenceDetailContainer::HandleScrollEventL(CEikScrollBar* /*aScrollBar*/, TEikScrollEvent /*aEventType*/)
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("HandleScrollEventL"));

	//no impl
}


void CPresenceDetailContainer::CreateScrollBars()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("CreateScrollBars"));

	iScrollSpan = 6;
	iNbPosition = 0;
	iThumbPosition = 0;
	iCurrentPos = 0;

	iModel = TEikScrollBarModel(iScrollSpan, iNbPosition, iThumbPosition);
	iSBFrame = new (ELeave) CEikScrollBarFrame(this, this, ETrue);
	iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EAuto);
#ifndef __S60V3__
	// FIXME3RD: do we have to do something
	iSBFrame->SetScrollBarManagement(CEikScrollBar::EVertical, CEikScrollBarFrame::EFloating);
#endif
	iSBFrame->Tile(&iModel);
	iSBFrame->DrawScrollBarsNow();
}

void CPresenceDetailContainer::MoveUp()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("MoveUp"));

	CPtrList<CLabelGroup>::Node *n = iClickList->iFirst, *prev=iClickList->iFirst;

	while (n!=iCurrentGroup) {
		prev=n;
		n=n->Next;
	}
	
	iCurrentGroup = prev;
}

void CPresenceDetailContainer::MoveDown()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("MoveDown"));

	if ( iCurrentGroup->Next) {
		iCurrentGroup = iCurrentGroup->Next;
	}
}

void CPresenceDetailContainer::SetPositionOfComponents()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("SetPositionOfComponents"));

	// line 0;
	title->SetPosition(TPoint(0, 0));

	// line 1
	user_given_title->SetPosition(TPoint(0, KLabelHeight+KOffset));
	user_given_time->SetPosition(TPoint(user_given_title->Size().iWidth, KLabelHeight +KOffset));

	// line2
	user_given->SetPosition(TPoint(0, 2*KLabelHeight + KOffset));

	// line 3
	curr_loc_time->SetPosition(TPoint(KScreenWidth - curr_loc_time->Size().iWidth, 3*KLabelHeight + KOffset));
	curr_loc->SetPosition(TPoint(0, 3*KLabelHeight + KOffset));

	// line 4
	prev_loc_time->SetPosition(TPoint(KScreenWidth - prev_loc_time->Size().iWidth, 4*KLabelHeight + KOffset));
	prev_loc->SetPosition(TPoint(0, 4*KLabelHeight + KOffset));

	// line 5
	cal2_time->SetPosition(TPoint(KScreenWidth - cal2_time->Size().iWidth, 5*KLabelHeight + KOffset));
	cal2->SetPosition(TPoint(0, 5*KLabelHeight + KOffset));

	// line 6
	cal1_time->SetPosition(TPoint(KScreenWidth - cal1_time->Size().iWidth, 6*KLabelHeight + KOffset));
	cal1->SetPosition(TPoint(0, 6*KLabelHeight + KOffset));

	//line 7
	curr_profile_title->SetPosition(
	    TPoint(0, 7*KLabelHeight + KOffset) );
	TInt profileW = curr_profile_title->Size().iWidth;	
	TInt statusIconW = 0;
#ifdef __JAIKU__
	statusIconW = 15; 
 	icon_presence->SetPosition(
	    TPoint( profileW, 7*KLabelHeight  + KOffset ) );
#endif 
	curr_profile_name->SetPosition( 
	    TPoint( profileW + statusIconW, 7*KLabelHeight + KOffset ) );

	//line 8
	icon_speaker->SetPosition(TPoint(0, 8*KLabelHeight -1 + KOffset ));
	curr_profile_speaker->SetPosition(TPoint(0, 8*KLabelHeight + KOffset));
	icon_vibra->SetPosition(TPoint(88, 8*KLabelHeight -1 + KOffset));
	curr_profile_vib->SetPosition(TPoint(88, 8*KLabelHeight + KOffset));

	//line 9
	bt_nb_title->SetPosition(TPoint(0, 9 * KLabelHeight + KOffset));
	TInt x=bt_nb_title->MinimumSize().iWidth;
	TInt w = KScreenWidth - x - 2*18; // icons
	w /= 2;
	icon_buddies->SetPosition(TPoint(x, 9*KLabelHeight -1 + KOffset));
	x+=18;
	bt_nb_buddies->SetPosition(TPoint(x, 9*KLabelHeight + KOffset));
	x+=w;
	icon_others->SetPosition(TPoint(x, 9*KLabelHeight -1 + KOffset));
	x+=18;
	bt_nb_others->SetPosition(TPoint(x, 9*KLabelHeight + KOffset));

	//line 10
	w=KScreenWidth/3;
	w -= 16;
	x = 0;
	icon_desktop->SetPosition(TPoint(x, 10*KLabelHeight -1 + KOffset));
	bt_desktop->SetPosition(TPoint(x+16, 10*KLabelHeight -1 + KOffset));
	x+=w+16;
	icon_laptop->SetPosition(TPoint(x, 10*KLabelHeight -1 + KOffset));
	bt_laptop->SetPosition(TPoint(x+16, 10*KLabelHeight -1 + KOffset));
	x+=w+16;
	icon_pda->SetPosition(TPoint(x, 10*KLabelHeight -1 + KOffset));
	bt_pda->SetPosition(TPoint(x+16, 10*KLabelHeight -1 + KOffset));
	
	//line 11
	user_active->SetPosition(TPoint(0, 11 *KLabelHeight + KOffset));
	icon_user_activity->SetPosition(TPoint(0, 11*KLabelHeight + KOffset));
}



void CPresenceDetailContainer::SetSizeOfComponents()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("SetSizeOfComponents"));

	title->SetSize(TSize(KScreenWidth, KLabelHeight+ KOffset));

	user_given_time->SetSize( TSize(user_given_time->MinimumSize().iWidth, KLabelHeight) );
	user_given_title->SetSize(TSize(KScreenWidth-(user_given_time->Size().iWidth), KLabelHeight));
	user_given->SetSize(TSize(KScreenWidth, KLabelHeight));

	prev_loc_time->SetSize(TSize(prev_loc_time->MinimumSize().iWidth, KLabelHeight));
	prev_loc->SetSize(TSize(KScreenWidth-prev_loc_time->MinimumSize().iWidth, KLabelHeight));

	curr_loc_time->SetSize(TSize(curr_loc_time->MinimumSize().iWidth, KLabelHeight));
	curr_loc->SetSize(TSize(KScreenWidth-curr_loc_time->MinimumSize().iWidth, KLabelHeight));

	cal1_time->SetSize(TSize(cal1_time->MinimumSize().iWidth, KLabelHeight));
	cal1->SetSize(TSize(KScreenWidth, KLabelHeight));

	cal2_time->SetSize(TSize(cal2_time->MinimumSize().iWidth, KLabelHeight));
	cal2->SetSize(TSize(KScreenWidth, KLabelHeight));

	curr_profile_title->SetSize(TSize(curr_profile_title->MinimumSize().iWidth, KLabelHeight));
        curr_profile_name->SetSize(TSize(KScreenWidth - curr_profile_title->Size().iWidth, KLabelHeight));

	curr_profile_speaker->SetSize(TSize(KScreenWidth/2,KLabelHeight));
	curr_profile_vib->SetSize(TSize(KScreenWidth/2,KLabelHeight));

	TInt w=bt_nb_title->MinimumSize().iWidth;
	bt_nb_title->SetSize(TSize(w,KLabelHeight));
	w = KScreenWidth - w - 2*18; // icon
	w /= 2;
	bt_nb_buddies->SetSize(TSize(w,KLabelHeight));
	bt_nb_others->SetSize( TSize(w,KLabelHeight));
	w=KScreenWidth/3;
	w -= 16;
	bt_desktop->SetSize(TSize(w, KLabelHeight));
	bt_laptop->SetSize(TSize(w, KLabelHeight));
	bt_pda->SetSize(TSize(w, KLabelHeight));

	user_active->SetSize(TSize(KScreenWidth,KLabelHeight+2));
	
	icon_presence->SetSize(TSize(15,15));
	icon_speaker->SetSize(TSize(16,16));
	icon_vibra->SetSize(TSize(16,16));
	icon_buddies->SetSize(TSize(18,16));
	icon_others->SetSize(TSize(18,16));
	icon_desktop->SetSize(TSize(16, 16));
	icon_laptop->SetSize(TSize(16, 16));
	icon_pda->SetSize(TSize(16, 16));
	icon_user_activity->SetSize(TSize(16,16));
}

void CPresenceDetailContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("SizeChanged"));

	SetSizeOfComponents();
	SetPositionOfComponents();
}

TInt CPresenceDetailContainer::CountComponentControls() const {

	return iControls.Count();
}

TKeyResponse CPresenceDetailContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("OfferKeyEventL"));

	if (aType!=EEventKey) return EKeyWasNotConsumed;

	if (aKeyEvent.iCode==EKeyDownArrow)
	{
		if (iCurrentPos < iScrollSpan-1)
		{
			iCurrentPos++;
			iSBFrame->MoveThumbsBy(0, 1);
			MoveDown();
			DrawNow();
		}
		return EKeyWasConsumed;
	}
	if (aKeyEvent.iCode==EKeyUpArrow)
	{
		if (iCurrentPos > 0)
		{
			iCurrentPos--;
			iSBFrame->MoveThumbsBy(0, -1);
			MoveUp();
			DrawNow();
		}
		return EKeyWasConsumed;
	}
	if (aKeyEvent.iCode==EKeyOK)
	{
		ShowDetail();
	}
	if ( aKeyEvent.iCode==EKeyRightArrow )
	{
		iView->HandleCommandL(EAknCmdExit);
		return EKeyWasConsumed;
	}
	return EKeyWasNotConsumed;
}


void CPresenceDetailContainer::ShowDetail()
{
	TInt count = iCurrentGroup->Item->iCount;
	CLabelGroup::Node * l = iCurrentGroup->Item->iFirst;
	HBufC * header=0;
	HBufC * body =0;

	// only one item in the group
	if (count == 1) {
		body = HBufC::NewLC(l->Item->Text()->Length());
		body->Des().Append(*(l->Item->Text()));
		CAknMessageQueryDialog * note = CAknMessageQueryDialog::NewL(*body);
		note->ExecuteLD(R_FULLSTRING_DIALOG);
		CleanupStack::PopAndDestroy();
	} else {
		TInt header_length = 0;
		TInt body_length = 0;
		while (l) {
			if (l->Item->IsUnderlined()) {
				header_length += l->Item->Text()->Length() +1;
			} else {
				body_length += l->Item->Text()->Length() +1;
			}
			l=l->Next;
		}
		header = HBufC::NewLC(header_length);
		body = HBufC::NewLC(body_length);

		l = iCurrentGroup->Item->iFirst;
		while (l) {
			if (l->Item->IsUnderlined()) {
				if (l->Item->Text()->Left(1).Compare(_L("[")) != 0) {
					header->Des().Append(*(l->Item->Text()));
				}
			} else {
				body->Des().Append(*(l->Item->Text()));
				body->Des().Append(_L("\n"));
			}
			l=l->Next;
		}
		CAknMessageQueryDialog * note = CAknMessageQueryDialog::NewL(*body);
		CleanupStack::PushL(note);
		note->SetHeaderTextL(*header);
		note->ExecuteLD(R_FULLSTRING_DIALOG);
		CleanupStack::Pop();
		CleanupStack::PopAndDestroy(2);
	}
}


CCoeControl* CPresenceDetailContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("ComponentControl"));

	return (CCoeControl*)iControls[aIndex];
}

void CPresenceDetailContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CPresenceDetailContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);

	gc.SetBrushColor(KRgbWhite);
	TRect r( TPoint(0, -KLabelHeight+KOffset), TSize(KScreenWidth, KLabelHeight*2) );
	gc.SetBrushColor(KRgbWhite);
	gc.DrawRect(r);

	TRgb dark_gray = TRgb(225,225,225);
	TRgb light_gray  = TRgb(210,210,210);
	TRgb original_blue = TRgb(170,170,255);

	for (int i=0; i<6; i++) {
		r.Move(0, 2*KLabelHeight);
		if ( i % 2 == 0 ) {
			gc.SetBrushColor(dark_gray);
		} else {
			gc.SetBrushColor(light_gray);
		}
		if (i == iCurrentPos) gc.SetBrushColor(original_blue);

		gc.DrawRect(r);
	}
}

CPresenceDetailViewImpl::CPresenceDetailViewImpl()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("CPresenceDetailViewImpl"));

}

#include "cu_common.h"

void CPresenceDetailViewImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("ConstructL"));
	iResource=LoadSystemResourceL(iEikonEnv, _L("contextcontactsui"));

	BaseConstructL( R_PRESENCEDETAIL_VIEW );
}

TUid CPresenceDetailViewImpl::Id() const {
	return KPresenceDetailView;
}

void CPresenceDetailViewImpl::ShowDetail()
{
	iContainer->ShowDetail();
}

void CPresenceDetailViewImpl::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("HandleCommandL"));

	switch (aCommand) 
		{
		case EContextContactsCmdShowHistory:			
			ShowDetail();
			break;

		case EAknCmdExit:
		case EAknSoftkeyBack:
			ActivateViewL(iPrevView);
			break;

		default:
			AppUi()->HandleCommandL( aCommand );
			break;
		}
}

void CPresenceDetailViewImpl::DoActivateL(const TVwsViewId& aPrevViewId,
	TUid /*aCustomMessageId*/,
	const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("DoActivateL"));
	MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);
#ifdef __WINS__
	TInt dummy;
	TBreakItem b(GetContext(), dummy);
#endif

	iPrevView=aPrevViewId;
	if (!iContainer) {
		TRect r = AppUi()->ApplicationRect();
		iContainer=new (ELeave) CPresenceDetailContainer(iName, iPresence, iAtTime, this, iIsMe);
		iContainer->ConstructL( r );
		iContainer->SetMopParent(this);
		iContainer->CreateScrollBars();
	
		AppUi()->AddToStackL( *this, iContainer );
    } 
}

void CPresenceDetailViewImpl::DoDeactivate()
{
	CC_TRAPD(err, {
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("DoDeactivate"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
	iContainer = 0; });
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}


CPresenceDetailViewImpl::~CPresenceDetailViewImpl()
{
	CC_TRAPD(err, ReleaseCPresenceDetailViewImpl());
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}

void CPresenceDetailViewImpl::ReleaseCPresenceDetailViewImpl()
{
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("~CPresenceDetailViewImpl"));

	if (iResource) iEikonEnv->DeleteResourceFile(iResource);
	delete iContainer;
	
}


void CPresenceDetailViewImpl::SetData(const TDesC& Name, const TDesC& aJaikuNick, const CBBPresence* PresenceData, TTime aAtTime)
{
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("SetData"));

	iName=Name;
	iPresence=PresenceData;
	iAtTime=aAtTime;

	TBuf< CJabberData::KNickMaxLen > myNick;   	
	Settings().GetSettingL(SETTING_JABBER_NICK, myNick);

	iIsMe = CJabberData::EqualNicksL( aJaikuNick, myNick );
}

void CPresenceDetailViewImpl::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CPresenceDetailViewImpl"), _CL("DynInitMenuPaneL"));


	if (aResourceId == R_RICH_PRESENCE_VIEW_MENUPANE) 
		{		
			

			// Add Jaiku Nick
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdAddJabber,  ETrue );
			// Edit Jaiku Nick
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdShowJabber, ETrue );
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdEditJabber, ETrue );
			// Invite to Jaiku
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdInvite, ETrue );
						
			TBool enabled=ETrue;
			Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled);
			SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppSuspendPresence, ! enabled);	
			SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppResumePresence, enabled);
		}
}
