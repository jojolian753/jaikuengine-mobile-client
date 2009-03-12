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

#include "current_loc.h"
#include "basestack.h"
#include "cl_settings.h"

#include "csd_cell.h"
#include "csd_base.h"

class CCurrentLocImpl : public CCurrentLoc {
	CCurrentLocImpl(MApp_context& Context, CGenericIntMap* acellid_names);
	~CCurrentLocImpl();

        void ConstructL();

	void now_at_location(const TBBCellId* cellid, TInt id, bool is_base, bool loc_changed, TTime time);
	CCircularLog*	BaseLog();

	void AddToLog(const CBaseStack::TBaseItem& b);
	void ReplaceLastLog(const CBaseStack::TBaseItem& b);
	void ReconstructLog();
	void EmptyLog();
	void PostValue(const TTime& aTime);
	TInt CurrentBaseId(); // returns -1 if not at base

	CBaseStack::TBaseItem	iBase[2];
	CBaseStack::TBaseItem	iLastSeen;
	CBaseStack*	iBaseStack;
	CCircularLog*	iBaseLog;
	bool	iFirstLoc; bool iIsBase;
	CGenericIntMap* cellid_names;

	TBBBaseInfo	iValue;

	friend class auto_ptr<CCurrentLocImpl>;
	friend class CCurrentLoc;
};

void Set(TBBBaseVisit& aVisit, const CBaseStack::TBaseItem& aItem)
{
	aVisit.iBaseId()=aItem.iBaseId;
	aVisit.iBaseName()=aItem.iBaseName;
	aVisit.iEntered()=aItem.iEntered;
	aVisit.iLeft()=aItem.iLeft;
}

void CCurrentLocImpl::PostValue(const TTime& aTime)
{
	CBaseStack::TBaseItem dummy; dummy.Reset();
	if (iBase[0].iLeft==TTime(0)) {
		Set(iValue.iPreviousStay, iBase[1]);
		Set(iValue.iCurrent, iBase[0]);
	} else {
		Set(iValue.iPreviousStay, iBase[0]);
		Set(iValue.iCurrent, dummy);
	}
	Set(iValue.iPreviousVisit, iLastSeen);
	post_new_value(&iValue, aTime);
}

EXPORT_C CCurrentLoc* CCurrentLoc::NewL(MApp_context& Context, CGenericIntMap* acellid_names)
{
	CALLSTACKITEM2_N(_CL("CCurrentLoc"), _CL("NewL"),  &Context);

	auto_ptr<CCurrentLocImpl> ret(new (ELeave) CCurrentLocImpl(Context, acellid_names));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C CCurrentLoc::~CCurrentLoc()
{

}

CCurrentLoc::CCurrentLoc(MApp_context& Context) : Mlog_base_impl(Context, KBase, KBaseTuple, 5*24*60*60)
{

}

CCurrentLocImpl::CCurrentLocImpl(MApp_context& Context, CGenericIntMap* acellid_names) : 
	CCurrentLoc(Context), cellid_names(acellid_names), iValue()
{
}

CCurrentLocImpl::~CCurrentLocImpl()
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("~CCurrentLocImpl"));

	delete iBaseStack;
	delete iBaseLog;
}

void CCurrentLocImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("ConstructL"));

	Mlog_base_impl::ConstructL();

	iBaseStack=CBaseStack::NewL(AppContext());
	iBaseLog=CCircularLog::NewL(20, true);
	ReconstructLog();

	if (iBaseStack->LastL(iBase[0])) {
		iBaseStack->PrevL(iBase[1]);
	}

	iFirstLoc=true;
}

void CCurrentLocImpl::now_at_location(const TBBCellId* /*cellid*/, TInt id, bool is_base, bool loc_changed, TTime time) 
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("now_at_location"));

	//
	// FIXME: this is a hack for
	// the no coverage case, so that the code doesn't crash
	//
	if (id==-1) return;
	bool do_post=false;

	if (iFirstLoc) {
		iFirstLoc=false;
		/*
		 * get the stored last base and post
		 *
		 */
		int previ=1;
		if (iBase[0].iLeft!=TTime(0)) {
			previ=0;
		} else {
			iIsBase=true;
		}
		do_post=true;

		TTime iBaseStamp, now=GetTime();
		now-=TTimeIntervalMinutes(15);
		Settings().GetSettingL(SETTING_LAST_BASE_STAMP, iBaseStamp);

		if (iBase[0].iBaseId==-1 && is_base) {
			// no previous data
			HBufC* user_given_name=0;
			user_given_name=(HBufC*)cellid_names->GetData(id);
			if (user_given_name) {
				iBase[0]=CBaseStack::TBaseItem(id, *user_given_name, time);
			} else {
				iBase[0]=CBaseStack::TBaseItem(id, _L(""), time);
			}
			iBaseStack->PushBackL(iBase[0]);
			PostValue(time);
			AddToLog(iBase[0]);
			iIsBase=is_base;
			Settings().WriteSettingL(SETTING_LAST_BASE_STAMP, time);
			return;
		} else if (id!=-1 && id==iBase[0].iBaseId && now < iBaseStamp 
				&& (iBase[0].iLeft==TTime(0) || now < iBase[0].iLeft)) {

			// still in same base or came back within 15 mins
			if (now < iBase[0].iLeft) {
				iIsBase=true;
			}
			HBufC* user_given_name=(HBufC*)cellid_names->GetData(id);
			if (user_given_name)
				iBase[0].iBaseName=user_given_name->Left(50);
			iBase[0].iLeft=0; iBaseStack->SetLastLeft(TTime(0));
			PostValue(time);
			ReplaceLastLog(iBase[0]);
			iIsBase=is_base;
			Settings().WriteSettingL(SETTING_LAST_BASE_STAMP, time);
			return;
		} else if (id!=iBase[0].iBaseId && now >= iBaseStamp && iBase[0].iLeft==TTime(0)) {
			// moved out of the base while not running
			// cannot know the right time, let's use last observation
			iBase[0].iLeft=iBaseStamp; iBaseStack->SetLastLeft(iBaseStamp);
			ReplaceLastLog(iBase[0]);
			do_post=true;
		}
	}

	Settings().WriteSettingL(SETTING_LAST_BASE_STAMP, time);

	if (loc_changed) {
		// did we stay long enough to count in current base
		if (iBase[0].iLeft==TTime(0)) {
			if (iBase[0].iEntered+TTimeIntervalMinutes(7) > time) {
				// didn't stay
				iLastSeen=iBase[0];
				iLastSeen.iLeft=time;
				iBaseStack->DeleteLastL();
				if (iBaseLog) iBaseLog->DeleteLast();
				if (iBaseStack->LastL(iBase[0]))
					iBaseStack->PrevL(iBase[1]);
			} else {
				iLastSeen.Reset();
				iBase[0].iLeft=time;
				iBaseStack->SetLastLeft(time);
				ReplaceLastLog(iBase[0]);
			}
			do_post=true;
		}

		// did we come back to previous
		TTime comp=time; comp-=TTimeIntervalMinutes(15);
		int previ=1;
		if (iBase[0].iLeft!=TTime(0)) previ=0;
		bool came_back=false;
		if (is_base && iBase[previ].iBaseId==id && comp < iBase[previ].iLeft) {
			came_back=true;
			// came back to previous quickly
			if (previ==0) {
				iLastSeen.Reset();
				iBase[0].iLeft=0;
				iBaseStack->SetLastLeft(TTime(0));
			} else {
				iBaseStack->DeleteLastL();
				if (iBaseStack->LastL(iBase[0]))
					iBaseStack->PrevL(iBase[1]);
				if (iBaseLog) iBaseLog->DeleteLast();
			}
			do_post=true;
			ReplaceLastLog(iBase[0]);
		}

		// what base are we in now
		if (is_base && !came_back) {
			iBase[1]=iBase[0];
			HBufC* user_given_name=0;
			user_given_name=(HBufC*)cellid_names->GetData(id);
			if (user_given_name) {
				iBase[0]=CBaseStack::TBaseItem(id, *user_given_name, time);
			} else {
				iBase[0]=CBaseStack::TBaseItem(id, _L(""), time);
			}
			iBaseStack->PushBackL(iBase[0]);
			AddToLog(iBase[0]);
			do_post=true;
		}

		// prune stack
		while (iBaseStack->CountL()>20) {
			iBaseStack->DeleteFirstL();
		}
	} else {
		if (iIsBase!=is_base) {
			// noticed this cell is a base
			iBase[1]=iBase[0];
			HBufC* user_given_name=0;
			user_given_name=(HBufC*)cellid_names->GetData(id);
			if (user_given_name) {
				iBase[0]=CBaseStack::TBaseItem(id, *user_given_name, time);
			} else {
				iBase[0]=CBaseStack::TBaseItem(id, _L(""), time);
			}
			iBaseStack->PushBackL(iBase[0]);
			AddToLog(iBase[0]);
			do_post=true;
		} else {

			if (is_base) {
				// skip if not changed location and no new name
				HBufC* user_given_name=0;
				user_given_name=(HBufC*)cellid_names->GetData(iBase[0].iBaseId);
				if (user_given_name==0) {
					if (!do_post) return;
				} else {
					if ((*user_given_name).Left(50).Compare(iBase[0].iBaseName)==0) {
						if (!do_post) return;
					}
					iBase[0].iBaseName=user_given_name->Left(50);
					iBaseStack->SetLastName(*user_given_name);
					ReplaceLastLog(iBase[0]);
					do_post=true;
				}
			}
		}
	}
	iIsBase=is_base;

	if (do_post) {
		PostValue(time);
	}
}

CCircularLog*	CCurrentLocImpl::BaseLog()
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("BaseLog"));

	return iBaseLog;
}

void CCurrentLocImpl::ReplaceLastLog(const CBaseStack::TBaseItem& b)
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("ReplaceLastLog"));

	if (!iBaseLog) return;

	iBaseLog->DeleteLast();
	AddToLog(b);
}

void CCurrentLocImpl::AddToLog(const CBaseStack::TBaseItem& b)
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("AddToLog"));

	if (!iBaseLog) return;

	TBuf<80> s;
	TDateTime ent= ContextToLocal(b.iEntered).DateTime();
	TDateTime left= ContextToLocal(b.iLeft).DateTime();
	if (b.iLeft!=TTime(0)) {
		s.Format(_L("%S %02d/%02d %02d:%02d - %02d/%02d %02d:%02d"), &(b.iBaseName.Left(60)), 
			(TInt)ent.Day()+1, (TInt)ent.Month()+1, (TInt)ent.Hour(), (TInt)ent.Minute(),
			(TInt)left.Day()+1, (TInt)left.Month()+1, (TInt)left.Hour(), (TInt)left.Minute());
	} else {
		s.Format(_L("%S %02d/%02d %02d:%02d -"), &(b.iBaseName.Left(60)),
			(TInt)ent.Day()+1, (TInt)ent.Month()+1, (TInt)ent.Hour(), (TInt)ent.Minute());
	}
	iBaseLog->AddL(s);
}

void CCurrentLocImpl::ReconstructLog()
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("ReconstructLog"));

	if (!iBaseLog) return;

	CBaseStack::TBaseItem b;
	if (iBaseStack->FirstL(b)) {
		AddToLog(b);
		while (iBaseStack->NextL(b)) {
			AddToLog(b);
		}
	}
}

void CCurrentLocImpl::EmptyLog()
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("EmptyLog"));

	iBase[1]=iBase[0]=CBaseStack::TBaseItem();
	while (iBaseStack->CountL()>20) {
		iBaseStack->DeleteFirstL();
	}
	iFirstLoc=true;
}

TInt CCurrentLocImpl::CurrentBaseId() // returns -1 if not at base
{
	CALLSTACKITEM_N(_CL("CCurrentLocImpl"), _CL("CurrentBaseId"));

	if (iBase[0].iLeft!=TTime(0)) return -1;
	return iBase[0].iBaseId;
}
