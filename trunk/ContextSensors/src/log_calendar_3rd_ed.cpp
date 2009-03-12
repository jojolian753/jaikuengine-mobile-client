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

#include "log_calendar.h"

#include "app_context.h"
#include "break.h"
#include "csd_calendar.h"
#include "cl_settings.h"
#include "log_base_impl.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"
#include "settings.h"

#include <calcommon.h>
#include <calentry.h>
#include <calentryview.h>
#include <calinstance.h>
#include <calinstanceview.h>
#include <calnotification.h>
#include <calprogresscallback.h>
#include <calsession.h>
#include <e32property.h>
#include <e32base.h>


class CCalendarViewCreator : public CBase, public MContextBase, public MCalProgressCallBack
{
public:
	class MListener 
	{
	public:
		virtual void CalendarViewsReady(TInt aError) = 0;
	};


public:
	enum TState
		{
			EInit,
			ECreatingEntryView,
			ECreatingInstanceView,
			EReady,
			EStopping
		} iState;
	MListener&        iListener;
	
	CCalSession*      iSession;
	CCalEntryView*    iEntryView;
	CCalInstanceView* iInstanceView;

public:	
	static CCalendarViewCreator* NewL(MListener& aListener) 
	{
		auto_ptr<CCalendarViewCreator> self( new (ELeave) CCalendarViewCreator(aListener));
		self->ConstructL();
		return self.release();
	}

	CCalendarViewCreator(MListener& aListener) : iState( EInit ), iListener( aListener ) {}
	
	~CCalendarViewCreator()
	{
		Stop();
	}

	void Stop()
	{
		iState = EStopping;
		delete iInstanceView;
		iInstanceView = NULL;
		delete iEntryView;
		iEntryView = NULL;
		delete iSession;
		iSession = NULL;
		iState = EInit;
	}

	void StartL()
	{
		if ( iState != EInit )
			return;

		iSession = CCalSession::NewL();
		iSession->OpenL( KNullDesC );
		iState = ECreatingEntryView;
		iEntryView = CCalEntryView::NewL( *iSession, *this );
	}
	
	void ConstructL() 
	{
	}
	
	TBool NotifyProgress()
	{
		return EFalse; // we don't listen for progress notifications
	}
	
	void Progress(TInt /*aPercentageCompleted*/)
	{
		// NOP we don't listen for progress notifications
	}
	
	void Completed( TInt aError ) 
	{
		if ( aError == KErrNone )
			{
				switch ( iState ) 
					{
					case ECreatingEntryView:
						iState = ECreatingInstanceView;
						iInstanceView = CCalInstanceView::NewL( *iSession, *this );
						break;
					case ECreatingInstanceView:
						iState = EReady;
						iListener.CalendarViewsReady(KErrNone);
						break;
					default:
						Stop();
						iListener.CalendarViewsReady(KErrGeneral);
						break;
					}
			}
		else if ( aError == KErrCancel ) 
			{
				// no op 
			}
		else 
			{
				if ( iState != EStopping )
					{
						Stop();
					}
				iListener.CalendarViewsReady( aError );
			}
	}
};


class CLogCalendarImpl : public CLogCalendar, public CCalendarViewCreator::MListener, public MSettingListener
{
public: 
	void StartUpdateL();

private: 
	// from MListener
	void CalendarViewsReady(TInt aError);

	// MSettingListener
	void SettingChanged(TInt Setting);

	// from CCheckedActive
	void DoCancel();
	virtual TInt CheckedRunError(TInt aError);
	void CheckedRunL();	

private:
	CLogCalendarImpl(MApp_context& Context, HBufC* aBusyText);
	void ConstructL();
	~CLogCalendarImpl();
	void StartListeningL();

	void ReadEntriesL();
	void MakeCalendarEvent(TBBCalendarEvent& aInto, RPointerArray<CCalInstance>& aInstances);
	CCalInstanceView& InstanceView();
	void CompleteSelf();

	void StartTimerL(TBool aImmediate = EFalse);
	void StopTimer();

private:
	TInt			iSharing;
	HBufC			*iBusyText;
	TBBCalendar	iValue;

	RProperty iPubSubProperty;
	TCalPubSubData iPubSubData;
	
	CCalendarViewCreator* iCalendarCreator;
	friend class CLogCalendar;

	CPeriodic* iPeriodic;
};


static TInt StartUpdateCallBack(TAny *aObj) 
{
	CLogCalendarImpl* logCal = (CLogCalendarImpl*) aObj;
	CC_TRAPD(err, logCal->StartUpdateL()); 
	return ETrue;
}




EXPORT_C CLogCalendar* CLogCalendar::NewL(class MApp_context& Context, HBufC* aBusyText)
{
	CALLSTACKITEM2_N(_CL("CLogCalendar"), _CL("NewL"), &Context);
	auto_ptr<CLogCalendarImpl> ret(new CLogCalendarImpl(Context, aBusyText));
	if (! ret.get() ) delete aBusyText;
	ret->ConstructL();
	return ret.release();
}


CLogCalendar::CLogCalendar(MApp_context& Context)
	: CCheckedActive( CCheckedActive::EPriorityIdle, _L("CLogCalendar") ),
	  Mlog_base_impl(Context, KCalendar, KCalendarTuple, 24*60*60) { }

CLogCalendarImpl::CLogCalendarImpl(MApp_context& Context, HBufC* aBusyText) : CLogCalendar(Context),  iBusyText(aBusyText) { }


void CLogCalendarImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("NewL"));
	Mlog_base_impl::ConstructL();

	Settings().GetSettingL(SETTING_CALENDAR_SHARING, iSharing);

	iCalendarCreator = CCalendarViewCreator::NewL(*this);

	User::LeaveIfError( iPubSubProperty.Attach( KCalPubSubCategory, ECalPubSubEventNotification ) );
	CActiveScheduler::Add(this);

	iPeriodic = CPeriodic::NewL(EPriorityIdle);
	
	if (iSharing!=SHARE_CALENDAR_NONE) {
		StartTimerL( ETrue );
	} else {
		BBSession()->DeleteL( KCalendarTuple, KNullDesC, ETrue );
	}

	Settings().NotifyOnChange(SETTING_CALENDAR_SHARING, this);
}

CLogCalendarImpl::~CLogCalendarImpl()
{
	Cancel();
	Settings().CancelNotifyOnChange(SETTING_CALENDAR_SHARING, this);
	iPubSubProperty.Close();
	delete iPeriodic;

	delete iCalendarCreator;
	delete iBusyText;
}


void CLogCalendarImpl::StartListeningL()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("StartListeningL"));
	Cancel();
	iPubSubProperty.Subscribe(iStatus);
	SetActive();
}

void CLogCalendarImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("DoCancel"));
	if (iPeriodic) iPeriodic->Cancel();
	iPubSubProperty.Cancel();
}


void CLogCalendarImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("CheckedRunL"));
	Reporting().DebugLog(_L("Calendar modified"), iStatus.Int());
	if ( iStatus==KErrNone )
		{
			StartUpdateL();
		}
}

void CLogCalendarImpl::StartUpdateL()
{
	Cancel();
	iCalendarCreator->StartL();
}


TInt CLogCalendarImpl::CheckedRunError(TInt /*aError*/)
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("CheckedRunError"));
	Cancel();
	StartListeningL();
	StartTimerL( EFalse );
	return KErrNone;
}



void CLogCalendarImpl::CalendarViewsReady(TInt aError)
{
	if ( aError == KErrNone )
		{
			if (iSharing!=SHARE_CALENDAR_NONE) {			
				ReadEntriesL();
				StartListeningL();
				StartTimerL(EFalse);
			}
			iCalendarCreator->Stop();
		}
	else
		{
			iCalendarCreator->Stop();
		}
}

void CLogCalendarImpl::StartTimerL(TBool aImmediate)
{
	iPeriodic->Cancel();
 	TInt firstMins = aImmediate ? 0 : 10;
	TInt intervalMins = 10;
	TInt minute =  60* 1000 * 1000; 

 	TTimeIntervalMicroSeconds32 firstEvent(firstMins  * minute ) ;
 	TTimeIntervalMicroSeconds32 interval(intervalMins * minute); 
	
 	iPeriodic->Start( firstEvent, interval, TCallBack(&StartUpdateCallBack, this) );
}

void CLogCalendarImpl::CompleteSelf()
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("CompleteSelf"));
	TRequestStatus* pStat = &iStatus;
	iStatus=KRequestPending;
	SetActive();
	User::RequestComplete(pStat, KErrNone);
}


TTime Now()
{
	TTime now;
	now.HomeTime();
	return now;
}

TTime BeginningOfDay(TTime aTime)
{
	TDateTime dt = aTime.DateTime();
	return TTime(TDateTime( dt.Year(), dt.Month(), dt.Day(), 0, 0, 0, 0) );
}

CalCommon::TCalTimeRange TimeRange(const TTime& s, const TTime& e)
{
	TCalTime start;
	start.SetTimeLocalL( s );
	TCalTime end;
	end.SetTimeLocalL( e );
	
	return CalCommon::TCalTimeRange(start, end);
}

CalCommon::TCalTimeRange DayRangeOnwards(const TTime& aTime, TInt aDays)
{
	TTime s = BeginningOfDay( aTime );
	TTime e = s + TTimeIntervalDays(aDays);
	return TimeRange(s, e);
}


CalCommon::TCalTimeRange SingleDayRange(const TTime& aTime)
{
	return DayRangeOnwards( aTime, 1 );
}

TBool Includes(TTime x, TTime s, TTime e)
{
	return (s <= x && x <= e);
}


void CLogCalendarImpl::ReadEntriesL()
{
	RPointerArray<CCalInstance> instances;
	// Current entries
	CalCommon::TCalViewFilter filter = CalCommon::EIncludeAppts; //
	TTime now = Now();
	CalCommon::TCalTimeRange range = DayRangeOnwards(now, 7);  
	InstanceView().FindInstanceL( instances, filter, range );

	RPointerArray<CCalInstance> current;
	CCalInstance* prev = NULL;
	CCalInstance* next = NULL;

	TTime prevEnd;
	TTime nextStart; 

	for (TInt i = 0; i < instances.Count(); i++) 
		{
			CCalInstance* instance = instances[i];
			TTime s = instance->StartTimeL().TimeLocalL();
			TTime e = instance->EndTimeL().TimeLocalL();

			if ( Includes(now, s, e) )
				{
				current.AppendL(instance);
				}
			else if ( e < now )
				{
					if ( !prev || prevEnd <= e )
						{
							prev = instance;
							prevEnd = e;
						}
				}
			else if ( s > now )
				{
					if ( !next || s <= nextStart )
						{
							next = instance;
							nextStart = s;
						}
				}
		}

	TBBCalendar current_cal;

	RPointerArray<CCalInstance> prevs;
	if ( prev ) prevs.AppendL(prev);
	MakeCalendarEvent(current_cal.iPrevious, prevs);
	prevs.Close();

	MakeCalendarEvent(current_cal.iCurrent, current);
	current.Close();

	RPointerArray<CCalInstance> nexts;
	if ( next ) nexts.AppendL(next);
	MakeCalendarEvent(current_cal.iNext, nexts);
	nexts.Close();

	instances.ResetAndDestroy();
	instances.Close();


	if (current_cal==iValue) return;
	iValue = current_cal;
	post_new_value(&iValue);
}



void CLogCalendarImpl::MakeCalendarEvent(TBBCalendarEvent& aInto, RPointerArray<CCalInstance>& aInstances)
{
	CALLSTACKITEM_N(_CL("CLogCalendarImpl"), _CL("MakeCalendarEvent"));	

	aInto.iStartTime() = TTime(0);
	aInto.iEndTime() = TTime(0);


	TTime KMax = Time::MaxTTime();
	TTime KMin = Time::MinTTime();

	TTime min =	KMax;
	TTime max = KMin;

	for (TInt i=0; i<aInstances.Count(); i++) {
		CCalInstance& instance = *(aInstances[i]);
		CCalEntry& entry = instance.Entry();
		
		TInt space_left=aInto.iDescription().MaxLength()-aInto.iDescription().Length();
		if ( aInto.iDescription().Length() > 0  && space_left>=2) {
			aInto.iDescription().Append(_L(":"));
			space_left-=2;
		}
		
		// 50 char limit comes from TBBShortString
		TInt to_append_len= entry.SummaryL().Length();
		if (to_append_len > space_left) to_append_len=space_left;
		aInto.iDescription().Append ( entry.SummaryL().Left(to_append_len ) );

		aInto.iEventId()=entry.LocalUidL();
		
		TTime s = instance.StartTimeL().TimeLocalL();
		TTime e = instance.EndTimeL().TimeLocalL();
		
		if ( min==TTime(0) || s < min ) min=s;			
		if ( max==TTime(0) || e > max ) max=e;
	}

	if (iSharing==SHARE_CALENDAR_FREEBUSY && aInto.iDescription().Length()>0) aInto.iDescription=*iBusyText;
	if ( min != KMax )
		{
			aInto.iStartTime() = min;
			aInto.iEndTime() = max;
		}
}
		
CCalInstanceView& CLogCalendarImpl::InstanceView()
{
	return *(iCalendarCreator->iInstanceView);
	
}

void CLogCalendarImpl::SettingChanged(TInt Setting)
{
	Settings().GetSettingL(SETTING_CALENDAR_SHARING, iSharing);
	if (iSharing!=SHARE_CALENDAR_NONE) {
		StartUpdateL();
	} else {
		BBSession()->DeleteL( KCalendarTuple, KNullDesC, ETrue );
		Cancel();
	}
}
