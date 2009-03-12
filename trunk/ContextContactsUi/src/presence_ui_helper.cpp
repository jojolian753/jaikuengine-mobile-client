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

#include "presence_ui_helper.h"
#include <contextcommon.mbg>
#include <contextcontactsui.rsg>
#include "presence_icons.h"
#include <avkon.mbg>
#include <e32math.h>
#include <eikenv.h>


EXPORT_C void PresenceToListBoxL(const CBBPresence* data, HBufC*& name, 
				 HBufC* last_name, HBufC* first_name, const TDesC& time,
				 TTime compare, 
				 const TDesC& prev, const TDesC& ago,
				 const TDesC& not_avail, 
				 bool last_name_first,
				 TBool aShowDetailsInList)
{
	CALLSTACKITEM_N(_CL(""), _CL("PresenceToListBoxL"));
	/* length */
	TInt base_len=0;
	base_len+=100;

	if ( data )
		base_len += data->iUserGiven.iDescription.iValue.Length();

	TInt len=name->Length()+base_len+120+time.Length();
	name->Des().Zero();
	while (len>name->Des().MaxLength()) {
		name=name->ReAllocL(name->Des().MaxLength()*2);
	}

	// - Name --------------------------------------------------------------

	if (time.Length()>0) {
		name->Des().Append(time);
		name->Des().Append(_L(" "));
	}
	if (last_name_first)
	{
		if (last_name) name->Des().Append(*last_name);
		if (last_name && last_name->Length()>0 && first_name && first_name->Length()>0) {
			name->Des().Append(_L(" "));
		}
		if (first_name) name->Des().Append(*first_name);
	}
	else
	{
		if (first_name) name->Des().Append(*first_name);
		if (first_name && first_name->Length()>0 && last_name && last_name->Length()>0) {
			name->Des().Append(_L(" "));
		}
		if (last_name) name->Des().Append(*last_name);
	}

	if (!data || ! data->iSent) {
		name->Des().Append(_L("\t"));
		name->Des().Append(not_avail);
		name->Des().AppendFormat(_L("\t0\t0\t0\t0\t0\t0\t0\t0\t0\t%02d"), 
			GetIconIndex(EMbmAvkonQgn_indi_marked_add) );
		return;
	}

	TBool out_of_date = IsOutOfDate(data->iSentTimeStamp(), compare);
		
	if (!aShowDetailsInList) {
		name->Des().Append(_L("\t"));	
		name->Des().Append(data->iUserGiven.iDescription.Value());
	} else {
		name->Des().Append(_L(" "));
		if ( data->iUserGiven.iDescription.iValue.Length() != 0 )
		{
			if ( name->Des().Length() - time.Length() > 6) {
				TBuf<30> temp;
				temp.Append(name->Des().Left(6+time.Length()));
				name->Des().Copy(temp);
				name->Des().Append(_L("."));
			}
			name->Des().Append(_L("'"));
			name->Des().Append(data->iUserGiven.iDescription.Value());
			name->Des().Append(_L("'"));
		}
	}

	if (aShowDetailsInList) {
		name->Des().Append(_L("\t"));
		
		// - Location Info -----------------------------------------------------
		
		TTime stamp; stamp=GetTime();
		
		TBool do_stamp=EFalse;
		TBool do_ago=EFalse;
		if ( data->iBaseInfo.iCurrent.IsSet() ) {
			name->Des().Append(data->iBaseInfo.iCurrent.iBaseName());
			stamp = data->iBaseInfo.iCurrent.iEntered();
			do_stamp=ETrue;
		} else if (data->iCellName().Length()>0) {
			name->Des().Append(data->iCellName());
		} else if ( data->iBaseInfo.iPreviousStay.IsSet() || data->iBaseInfo.iPreviousVisit.IsSet() ) {
			name->Des().Append(prev);
			name->Des().Append(_L(" "));
			if (data->iBaseInfo.iPreviousStay.IsSet()) {
				name->Des().Append(data->iBaseInfo.iPreviousStay.iBaseName());
				stamp = data->iBaseInfo.iPreviousStay.iLeft();
			} else {
				name->Des().Append(data->iBaseInfo.iPreviousVisit.iBaseName());
				stamp = data->iBaseInfo.iPreviousVisit.iLeft();
			}
			do_stamp=ETrue;
			do_ago=ETrue;
		} else {
			if (data->iCity().Length()>0) {
				name->Des().Append(data->iCity());
				if (data->iCountry().Length()>0) {
					name->Des().Append(_L(", "));
				}
			}
			if (data->iCountry().Length()>0) {
				name->Des().Append(data->iCountry());
			}
		}
		if (do_stamp) {
			name->Des().Append(_L(" "));
			TPtr p=name->Des();
			TimeSinceStamp(p, stamp, GetTime(), 2);
		}
		if (do_ago) {
			name->Des().Append(ago);
		}
		
	}
	
	// - User activity --------------------------------------------------
	if (data->iUserActive.iActive() )
	{
		if (out_of_date) 
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonUser_inactive) );
			name->Des().Append(icon_int);
		}
		else
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonUser_active) );
			name->Des().Append(icon_int);
		}
	} else 	{
		TTimeIntervalMinutes minutes;
		TTime now; now=GetTime();
		
		now.MinutesFrom(data->iUserActive.iSince(), minutes);
		
		if (minutes.Int() <= 10)
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonUser_inactive_lvl_1) );
			name->Des().Append(icon_int);
		}
		else if (minutes.Int() <= 60)
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonUser_inactive_lvl_2) );
			name->Des().Append(icon_int);
		}
		else if (minutes.Int() <= 240)
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonUser_inactive_lvl_3) );
			name->Des().Append(icon_int);
		}
		else
		{
			// no icon
			name->Des().Append(_L("\t0"));
		}
	}
	
	
	TInt ringtype=data->iProfile.iRingingType();
	// - Speaker Info -----------------------------------------------------
	if ( ringtype == TBBProfile::ERingingTypeSilent || data->iProfile.iRingingVolume()==0)
	{
		if (out_of_date) {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_off_grey) );
			name->Des().Append(icon_int);	
		} else {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_off) );
			name->Des().Append(icon_int);	
		}
		
	} else {
		if (out_of_date) {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_on_grey) );
			name->Des().Append(icon_int);		
		} else {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_on) );
			name->Des().Append(icon_int);				
		}
	}
	
	// - Vibrator Info -----------------------------------------------------
	if ( data->iProfile.iVibra() )
	{
		if (out_of_date) {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_on_grey) );
			name->Des().Append(icon_int);		
		} else {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_on) );
			name->Des().Append(icon_int);
			
		}
	} else {
		if (out_of_date) {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_off_grey) );
			name->Des().Append(icon_int);		
		} else {
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_off) );
			name->Des().Append(icon_int);	
		}
	}

	// - NeighbourhoodInfo -------------------

	TBuf<5> icon_int;
	if (aShowDetailsInList) {
		const TBBCalendar& cal = data->iCalendar;
		if (cal.iPrevious.iDescription().Length()>0 ||
			cal.iCurrent.iDescription().Length()>0 ||
			cal.iNext.iDescription().Length()>0) {
			if (out_of_date) {
				icon_int.Format(_L("\t%02d"), 
					GetIconIndex(EMbmContextcommonCalendar_grey));
			} else {
				icon_int.Format(_L("\t%02d"), 
					GetIconIndex(EMbmContextcommonCalendar));
			}
		} else {
			icon_int=_L("\t0");
		}
	} else {
		icon_int=_L("\t0");
	}
	name->Des().Append(icon_int);
	if (aShowDetailsInList && data->iNeighbourhoodInfo.iDesktops() > 0) {
		if (out_of_date) {
			icon_int.Format(_L("\t%02d"), 
				GetIconIndex(EMbmContextcommonDesktop_grey));
		} else {
			icon_int.Format(_L("\t%02d"), 
				GetIconIndex(EMbmContextcommonDesktop));
		}
	} else {
		icon_int=_L("\t0");
	}
	name->Des().Append(icon_int);
	if (aShowDetailsInList && data->iNeighbourhoodInfo.iLaptops() > 0) {
		if (out_of_date) {
			icon_int.Format(_L("\t%02d"), 
				GetIconIndex(EMbmContextcommonLaptop_grey));
		} else {
			icon_int.Format(_L("\t%02d"), 
				GetIconIndex(EMbmContextcommonLaptop));
		}
	} else {
		icon_int=_L("\t0");
	}
	name->Des().Append(icon_int);
	if (aShowDetailsInList && data->iNeighbourhoodInfo.iPDAs() > 0) {
		if (out_of_date) {
			icon_int.Format(_L("\t%02d"), 
				GetIconIndex(EMbmContextcommonPda_grey));
		} else {
			icon_int.Format(_L("\t%02d"), 
				GetIconIndex(EMbmContextcommonPda));
		}
	} else {
		icon_int=_L("\t0");
	}
	name->Des().Append(icon_int);

	// - Buddies -----------------------------
	TInt idx = 0;

	if (aShowDetailsInList) {
		switch (data->iNeighbourhoodInfo.iBuddies())
		{
		case 0:
			break;
		case 1:
			if (out_of_date) {
				idx = GetIconIndex(EMbmContextcommonBuddy1_grey);
			} else {
				idx = GetIconIndex(EMbmContextcommonBuddy1);
			}
			break;
		case 2:
			if (out_of_date) {
				idx = GetIconIndex(EMbmContextcommonBuddy2_grey);
			} else {
				idx = GetIconIndex(EMbmContextcommonBuddy2);
			}
			break;
		case 3:
			if (out_of_date) {
				idx = GetIconIndex(EMbmContextcommonBuddy3_grey);
			} else {
				idx = GetIconIndex(EMbmContextcommonBuddy3);
			}
			break;
		default:
			if (out_of_date) {
				idx = GetIconIndex(EMbmContextcommonBuddy4_grey);
			} else {
				idx = GetIconIndex(EMbmContextcommonBuddy4);
			}
			break;
		}
	}
	icon_int.Format(_L("\t%02d"), idx );
	name->Des().Append(icon_int);
	
	// - Other_phones -----------------------------
	idx=0;
	icon_int.Zero();
	
	if (aShowDetailsInList) {
		switch (data->iNeighbourhoodInfo.iOtherPhones())
		{
		case 0:
			break;
			case 1:
				if (out_of_date) {
					idx = GetIconIndex(EMbmContextcommonOther1_grey);
				} else {
					idx = GetIconIndex(EMbmContextcommonOther1);
				}
				break;
			case 2:
				if (out_of_date) {
					idx = GetIconIndex(EMbmContextcommonOther2_grey);
				} else {
					idx = GetIconIndex(EMbmContextcommonOther2);
				}
				break;
			case 3:
				if (out_of_date) {
					idx = GetIconIndex(EMbmContextcommonOther3_grey);
				} else {
					idx = GetIconIndex(EMbmContextcommonOther3);
				}
				break;
			default:
				if (out_of_date) {
					idx = GetIconIndex(EMbmContextcommonOther4_grey);
				} else {
					idx = GetIconIndex(EMbmContextcommonOther4);
				}
				break;
		}
	}
	icon_int.Format(_L("\t%02d"), idx );
	name->Des().Append(icon_int);

	// - marked icon - has to be the final one! -------------------------------------

	idx = GetIconIndex(EMbmAvkonQgn_indi_marked_add);
	icon_int.Format(_L("\t%02d"), idx );
	name->Des().Append(icon_int);
}


class CPresenceArrayImpl : public CPresenceArray {
public:
	CPresenceArrayImpl(phonebook_i* aPhoneBook);
	void ConstructL();
	virtual TInt MdcaCount() const;
	virtual TPtrC16 MdcaPoint(TInt aIndex) const;
	~CPresenceArrayImpl();
private:
	TBuf<11> prev; TBuf<30> not_avail;
	TBuf<10> ago;
	bool	last_name_first;
	phonebook_i*	iPhoneBook;
	mutable HBufC*	iBuf;
};

EXPORT_C CPresenceArray* CPresenceArray::NewL(phonebook_i* aPhoneBook)
{
	CPresenceArrayImpl* ret=new (ELeave) CPresenceArrayImpl(aPhoneBook);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop();
	return ret;
}

CPresenceArrayImpl::CPresenceArrayImpl(phonebook_i* aPhoneBook) : iPhoneBook(aPhoneBook)
{
}

CPresenceArrayImpl::~CPresenceArrayImpl()
{
	delete iBuf;
}

void CPresenceArrayImpl::ConstructL()
{
	iBuf=HBufC::NewL(256);

	CEikonEnv::Static()->ReadResourceAsDes16(prev, R_PREVIOUS_CAPTION);
	CEikonEnv::Static()->ReadResourceAsDes16(ago, R_AGO);
	CEikonEnv::Static()->ReadResourceAsDes16(not_avail, R_JABBER_NOT_AVAIL);

	last_name_first=true;

#ifdef __S60V2__
	CPbkContactEngine *eng = CPbkContactEngine::Static();
	CPbkContactEngine::TPbkNameOrder order;
	if (!eng) {order = CPbkContactEngine::EPbkNameOrderLastNameFirstName;}
	else {order = eng->NameDisplayOrderL();}
	if (order != CPbkContactEngine::EPbkNameOrderLastNameFirstName) last_name_first=false;
#endif
}

TInt CPresenceArrayImpl::MdcaCount() const
{
	return iPhoneBook->Count();
}

TPtrC16 CPresenceArrayImpl::MdcaPoint(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CPresenceArrayImpl"), _CL("MdcaPoint"));

	contact* c=iPhoneBook->GetContact(aIndex);

	if (!c) {
		iBuf->Des().Zero();
		iBuf->Des().AppendFormat(_L("\t \t0\t0\t0\t0\t0\t0\t0\t0\t0\t%02d"), 
			GetIconIndex(EMbmAvkonQgn_indi_marked_add)); 
	} else {
		if (c->has_nick) {
			PresenceToListBoxL(c->presence, iBuf, 
				 c->last_name, c->first_name, c->time,
				 GetTime(),
				 prev, ago, not_avail,
				 last_name_first, c->show_details_in_list);
		} else {
			c->Name(iBuf, last_name_first);

			iBuf->Des().AppendFormat(_L("\t \t0\t0\t0\t0\t0\t0\t0\t0\t0\t%02d"), 
				GetIconIndex(EMbmAvkonQgn_indi_marked_add)); 
			//type No_icon_to_display = 0 + last one for mark sign
		}
	}

	return iBuf->Des();
}

