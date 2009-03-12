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
#include "log_calendar.h"
#include <e32base.h>
#include "log_base_impl.h"
#include "app_context.h"
#include "symbian_auto_ptr.h"
#include "csd_calendar.h"

#include <agmcallb.h>
#include <agmmiter.h>
#include "timeout.h"
#include "cl_settings.h"

TInt AgnNotify(TAny* aPtr);


class CLogCalendarImpl : public CLogCalendar, 
	public MAgnProgressCallBack, public MAgnModelStateCallBack, public MTimeOut,
	public MSettingListener {
private:
	CLogCalendarImpl(MApp_context& Context, HBufC* aBusyText);
	void ConstructL();
	~CLogCalendarImpl();

	void ReadEntriesL(TBool aForce=EFalse);
	void ListCurrentL();
	void MakeCalendarEvent(TBBCalendarEvent& aInto, CAgnDayDateTimeInstanceList* aFromList, CAgnModel* aModel);

	// MAgnProgressCallBack
	void Completed(TInt aError = KErrNone);
	void Progress(TInt aPercentageCompleted) { }
	// MAgnModelStateCallBack
	void StateCallBack(CAgnEntryModel::TState aState); 
	// MTimeOut
	void expired(CBase* Source);
	// MSettingListener
	void SettingChanged(TInt Setting);

	enum TState { EInitializing, EOpening, EOpened };
	TState iState;
	CAgnEntryModel::TState iModelState;

	RAgendaServ* iAgnServ;
	CAgnModel*	iModel;
	CAgnDayDateTimeInstanceList*	iDayList;
	TDateTime	iDayListDate;
	CTimeOut	*iTimeOut;

	TBBCalendar	iValue;
	TBool			iFreeBusyOnly;
	HBufC			*iBusyText;

	friend class CLogCalendar;
	friend class auto_ptr<CLogCalendarImpl>;
	friend TInt AgnNotify(TAny* aPtr);
};

EXPORT_C CLogCalendar* CLogCalendar::NewL(class MApp_context& Context, HBufC* aBusyText)
{
	CALLSTACKITEM2_N(_CL("CLogCalendar"), _CL("NewL"), &Context);
	auto_ptr<CLogCalendarImpl> ret(new CLogCalendarImpl(Context, aBusyText));
	if (! ret.get() ) delete aBusyText;
	ret->ConstructL();
	return ret.release();
}

CLogCalendar::CLogCalendar(MApp_context& Context) : Mlog_base_impl(Context, KCalendar, KCalendarTuple, 24*60*60) { }

CLogCalendarImpl::CLogCalendarImpl(MApp_context& Context, HBufC* aBusyText) : CLogCalendar(Context),
	iBusyText(aBusyText) { }

TInt AgnNotify(TAny* aPtr)
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("AgnNotify"));
	CLogCalendarImpl* log=(CLogCalendarImpl*)aPtr;
	TInt err;
	CC_TRAP(err, log->ReadEntriesL(ETrue));
	if (err!=KErrNone) return err;
	CC_TRAP(err, log->ListCurrentL());
	if (err!=KErrNone) return err;
	return KErrNone;
}

void CLogCalendarImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("ConstructL"));

	Settings().GetSettingL(SETTING_FREEBUSY_ONLY, iFreeBusyOnly);

	iDayListDate=TTime(0).DateTime();
	Mlog_base_impl::ConstructL();

	iTimeOut=CTimeOut::NewL(*this);

	TTimeIntervalMinutes KDefaultTimeForEvents(9 * 60); // 9.00 pm
	TTimeIntervalMinutes KDefaultTimeForAnnivs(13 * 60); // 1.00 am
	TTimeIntervalMinutes KDefaultTimeForDayNote(13 * 60); // 1.00 am

	iAgnServ = RAgendaServ::NewL();
	iAgnServ->Connect();

	iAgnServ->StartNotifierL( TCallBack(AgnNotify, this) );

	TTime now; now.HomeTime();
	iDayList=CAgnDayDateTimeInstanceList::NewL(now);

	iModel=CAgnModel::NewL(this);
	iModel->SetServer(iAgnServ);
	iModel->SetMode(CAgnEntryModel::EClient); 

	iState = EOpening;
	TInt err;
	CC_TRAPIGNORE(err, KErrNotFound, iModel->OpenL(_L("C:\\system\\data\\calendar"),
		KDefaultTimeForEvents, KDefaultTimeForAnnivs, KDefaultTimeForDayNote,
		this));
	if (err!=KErrNone) {
		TBuf<50> msg=_L("Error opening calendar: ");
		msg.AppendNum(err);
		post_error(msg, err);
	} else {
		iEvent.iData.SetValue(&iValue); iEvent.iData.SetOwnsValue(EFalse);
		post_new_value(&iValue);
	}
}

void CLogCalendarImpl::Completed(TInt aError)
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("Completed"));
	User::LeaveIfError(aError);
	if (iState==EOpening) iState=EOpened;
	if (iModelState==CAgnEntryModel::EOk) {
		ReadEntriesL();
		ListCurrentL();
	}
}

void CLogCalendarImpl::StateCallBack(CAgnEntryModel::TState aState)
{
	iModelState=aState;
	if (iModelState==CAgnEntryModel::EOk) {
		ReadEntriesL();
		ListCurrentL();
	}
}

void CLogCalendarImpl::MakeCalendarEvent(TBBCalendarEvent& aInto, 
					 CAgnDayDateTimeInstanceList* aFromList, CAgnModel* aModel)
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("MakeCalendarEvent"));
	TInt count=aFromList->Count();
	TTime min(0), max(0);
	aInto.iStartTime() = min;
	aInto.iEndTime() = max;

	for (TInt i=0; i<count; i++) {
		TAgnInstanceDateTimeId inst=(*aFromList)[i];
		auto_ptr<CAgnEntry> entry(aModel->FetchInstanceL(inst));
		TInt space_left=aInto.iDescription().MaxLength()-aInto.iDescription().Length();
		if ( aInto.iDescription().Length() > 0  && space_left>=2) {
			aInto.iDescription().Append(_L(":"));
			space_left-=2;
		}
		TBuf<30> desc;
		entry->RichTextL()->Extract(desc,0,30);
		TInt to_append_len=desc.Length();
		if (to_append_len > space_left) to_append_len=space_left;
		aInto.iDescription().Append ( desc.Left(to_append_len ) );
		aInto.iEventId()=inst.Id().Value();

		if ( min==TTime(0) || entry->InstanceStartDate() < min ) min=entry->InstanceStartDate();
		if ( max==TTime(0) || entry->InstanceEndDate() > max ) max=entry->InstanceEndDate();
	}

	if (iFreeBusyOnly && aInto.iDescription().Length()>0) aInto.iDescription=*iBusyText;
	aInto.iStartTime() = min;
	aInto.iEndTime() = max;
}

void CLogCalendarImpl::ListCurrentL()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("ListCurrentL"));

	iTimeOut->Wait( 10*60 );

	if (iState!=EOpened) return;
	if (iModelState!=CAgnEntryModel::EOk) return;

	TTime now; now.HomeTime();
	TDateTime midnight=now.DateTime(); midnight.SetHour(0); 
	midnight.SetMinute(0); midnight.SetSecond(0);
	midnight.SetMicroSecond(0);
	TTimeIntervalMinutes now_min_int;
	User::LeaveIfError(now.MinutesFrom(midnight, now_min_int));
	TInt now_min=now_min_int.Int();

	TInt count=iDayList->Count();

	auto_ptr<CAgnDayDateTimeInstanceList> prev(CAgnDayDateTimeInstanceList::NewL(now));
	auto_ptr<CAgnDayDateTimeInstanceList> current(CAgnDayDateTimeInstanceList::NewL(now));
	auto_ptr<CAgnDayDateTimeInstanceList> next(CAgnDayDateTimeInstanceList::NewL(now));

	TBBCalendar current_cal;

	for (TInt i=0; i<count; i++) {
		TAgnInstanceDateTimeId inst=(*iDayList)[i];

		if (inst.HasNullStartTime() || inst.HasNullEndTime()) continue;

		TInt start_min=inst.StartTime().Int();
		TInt end_min=inst.EndTime().Int();

		if (inst.StartDate() < TTime(midnight)) 
			start_min=0;
#ifdef __WINS__
		_LIT(KStart, "start");
		TBBTime start(KStart);
		start()=inst.StartDate();
		TBuf<40> msg=_L("START: ");
		start.IntoStringL(msg);
		RDebug::Print(msg);
		msg=_L("MIDNIGTH: ");
		start()=TTime(midnight);
		start.IntoStringL(msg);
		RDebug::Print(msg);
#endif
		if (inst.EndDate() > TTime(midnight)) 
			end_min=24*60+1;

		if (start_min <= now_min &&
				end_min >= now_min) {
			current->AppendL(inst);
		} else if (start_min < now_min &&
				end_min >= now_min-15 ) {
			prev->AppendL(inst);
		} else if ( start_min > now_min &&
				start_min <= now_min+60 ) {
			next->AppendL(inst);
		}
	}

	MakeCalendarEvent(current_cal.iPrevious, prev.get(), iModel);
	MakeCalendarEvent(current_cal.iCurrent, current.get(), iModel);
	MakeCalendarEvent(current_cal.iNext, next.get(), iModel);

	if (current_cal==iValue) return;

	iValue=current_cal;
	post_new_value(&iValue);

}

void CLogCalendarImpl::ReadEntriesL(TBool aForce)
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("ReadEntriesL"));
	if (iState!=EOpened) return;
	if (iModelState!=CAgnEntryModel::EOk) return;

	TTime now; now.HomeTime();

	TDateTime dt=now.DateTime();
	dt.SetHour(0); dt.SetMinute(0); dt.SetSecond(0);
	if ( !aForce && TTime(dt) < TTime(iDayListDate) ) return;
	delete iDayList; iDayList=0;
	iDayList=CAgnDayDateTimeInstanceList::NewL(now);

	TAgnFilter filter;
	filter.SetIncludeEvents(EFalse);
	filter.SetIncludeTodos(EFalse);

	iDayListDate=now.DateTime();
	iModel->PopulateDayDateTimeInstanceListL(iDayList, filter, iDayListDate);
}

void CLogCalendarImpl::expired(CBase* Source)
{
	ReadEntriesL();
	ListCurrentL();
}

CLogCalendarImpl::~CLogCalendarImpl()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("~CLogCalendarImpl"));
	delete iModel;

	if (iAgnServ) iAgnServ->Close();
	delete iAgnServ;

	delete iDayList;
	delete iTimeOut;
	delete iBusyText;
}

void CLogCalendarImpl::SettingChanged(TInt Setting)
{
	Settings().GetSettingL(SETTING_FREEBUSY_ONLY, iFreeBusyOnly);
}
