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

#include "presence_data.h"
#include "icons.h"
#include <contextcommon.mbg>
#include <e32math.h>
#include <avkon.mbg>
#include "cPbkContactEngine.h"

EXPORT_C void TCellInfo::Persist(CXmlBuf* into) const
{
	into->BeginElement(_L("location"));
	into->BeginElement(_L("location.value"));
	into->Leaf(_L("location.network"), iNetwork);
	TBuf<10> val;
	val.AppendNum(iLAC);
	into->Leaf(_L("location.lac"), val);
	val.Zero();
	val.AppendNum(iCellId);
	into->Leaf(_L("location.cellid"), val);
	into->EndElement(_L("location.value"));
	into->EndElement(_L("location"));
}

EXPORT_C void TGpsInfo::Persist(CXmlBuf* into) const
{
	into->Leaf(_L("gps"), iGpsData);
}

EXPORT_C void TLocInfo::Persist(CXmlBuf* into) const
{
	_LIT(KFormatTxt,"%04d%02d%02dT%02d%02d%02d");
	TBuf<20> buf_prev, buf_current;

	into->BeginElement(_L("base"));
	TDateTime dt_prev=iPUpdated.DateTime();
	TDateTime dt_current=iCUpdated.DateTime();

	buf_prev.Format(KFormatTxt, dt_prev.Year(), (TInt)dt_prev.Month()+1, (TInt)dt_prev.Day()+1,
		dt_prev.Hour(), dt_prev.Minute(), dt_prev.Second());
	buf_current.Format(KFormatTxt, dt_current.Year(), (TInt)dt_current.Month()+1, (TInt)dt_current.Day()+1,
		dt_current.Hour(), dt_current.Minute(), dt_current.Second());
	if (iFlags & EPrevious) {
		into->Leaf(_L("base.previous.left"), buf_prev);
		CPresenceData::MapLocInfo(iPrevious);
		into->Leaf(_L("base.previous"), *iPrevious);
	}
	if (iFlags & ECurrent) {
		CPresenceData::MapLocInfo(iCurrent);
		into->Leaf(_L("base.current.arrived"), buf_current);
		into->Leaf(_L("base.current"), *iCurrent);
	}
	if (iFlags & ENext) {
		CPresenceData::MapLocInfo(iNext);
		into->Leaf(_L("base.next"), *iNext);
	}
	into->EndElement(_L("base"));
}

EXPORT_C void TBluetoothInfo::Persist(CXmlBuf* into) const
{
	into->BeginElement(_L("bluetooth"));
	into->BeginElement(_L("devices"));

	if (iNodes) {
		CList<TBTNode>::Node* i=iNodes->iFirst;
		while (i) {
			into->BeginElement(_L("device"));
			into->Leaf(_L("bt.mac"), i->Item.iAddr);

			TBuf<10> c;
			c.AppendNum(i->Item.iMajorClass);
			into->Leaf(_L("bt.majorclass"), c);
			c.Zero();  c.AppendNum(i->Item.iMinorClass);
			into->Leaf(_L("bt.minorclass"), c);
			c.Zero();  c.AppendNum(i->Item.iServiceClass);
			into->Leaf(_L("bt.serviceclass"), c);
			into->Leaf(_L("bt.name"), i->Item.iNick);
			i=i->Next;
			into->EndElement(_L("device"));
		}
	}
	into->EndElement(_L("devices"));
	into->EndElement(_L("bluetooth"));
}

EXPORT_C void TProfileInfo::Persist(CXmlBuf* into) const
{
	into->BeginElement(_L("profile"));
	into->BeginElement(_L("profile.value"));
	TBuf<10> val;
	val.AppendNum(iProfileId);
	into->Leaf(_L("profile.id"), val);
	into->Leaf(_L("profile.name"), iProfileName);
	val.Zero(); val.AppendNum(iRingingType);
	into->Leaf(_L("profile.ringtype"), val);
	val.Zero(); val.AppendNum(iRingingVolume);
	into->Leaf(_L("profile.ringvolume"), val);
	if (iVibra) {
		into->Leaf(_L("profile.vibrate"), _L("true"));
	} else {
		into->Leaf(_L("profile.vibrate"), _L("false"));
	}
	into->EndElement(_L("profile.value"));
	into->EndElement(_L("profile"));
}

EXPORT_C void TActivityInfo::Persist(CXmlBuf* into) const
{
	into->BeginElement(_L("useractivity"));

	if (iMode==EIdle) {
		into->Leaf(_L("useractivity.value"), _L("idle"));
	} else {
		into->Leaf(_L("useractivity.value"), _L("active"));
	}

	into->EndElement(_L("useractivity"));
}

EXPORT_C void TNeighbourhoodInfo::Persist(CXmlBuf* into) const
{
	into->BeginElement(_L("bt.presence"));

	TBuf<5> val;
	val.AppendNum(iBuddies);
	into->Leaf(_L("buddies"), val);
	val.Zero();
	val.AppendNum(iOtherPhones);
	into->Leaf(_L("other_phones"), val);

	into->EndElement(_L("bt.presence"));
}

EXPORT_C CPresenceData* CPresenceData::NewL(const TDesC& Xml, const TTime& send_timestamp)
{
	auto_ptr<CPresenceData> ret(new (ELeave) CPresenceData(send_timestamp));
	ret->ConstructL(Xml);
	return ret.release();
}

EXPORT_C CPresenceData::~CPresenceData()
{
	delete iParser;
	delete iParseState;
	delete iXml;
	delete iChars;
	delete iError;
}

bool CPresenceData::IsSent() const
{
	return true;
}

const TCellInfo& CPresenceData::CellInfo() const
{
	return iCellInfo;
}

const TGpsInfo& CPresenceData::GpsInfo() const
{
	return iGpsInfo;
}

const TLocInfo& CPresenceData::LocInfo() const
{
	return iLocInfo;
}

const TActivityInfo& CPresenceData::ActivityInfo() const
{
	return iActivityInfo;
}

const TProfileInfo& CPresenceData::ProfileInfo() const
{
	return iProfileInfo;
}

const TBluetoothInfo& CPresenceData::BluetoothInfo() const 
{
	return iBluetoothInfo;
}

const TNeighbourhoodInfo& CPresenceData::NeighbourhoodInfo() const
{
	return iNeighbourhoodInfo;
}

EXPORT_C const TDesC& CPresenceData::RawXml() const
{
	return *iXml;
}

const TTime& CPresenceData::SendTimeStamp() const
{
	return iSendTimeStamp;
}

CPresenceData::CPresenceData(const TTime& send_timestamp) : iSendTimeStamp(send_timestamp)
{
}

void CPresenceData::ConstructL(const TDesC& Xml)
{
	iChars=HBufC::NewL(512);
	iXml=Xml.AllocL();
	iParseState=CList<TParseState>::NewL();
	iParseState->Push(EIdle);
	iParser=CXmlParser::NewL(*this);
	iParser->Parse( (char*)iXml->Ptr(), iXml->Size(), true);
	iParseState->Pop();
	delete iParser; iParser=0;
	delete iParseState; iParseState=0;
}

void CPresenceData::SetStamp(MXmlInfo& Info)
{
	Info.iUpdated=iStamp;
}

void CPresenceData::StartElement(const XML_Char *name,
			const XML_Char**  /*atts*/)
{
	TPtrC elem_name((TUint16 *)name);
	TParseState next=iParseState->Top();
		if (! elem_name.Compare(_L("location") ) ) {
			SetStamp(iCellInfo);
		} else if (! elem_name.Compare(_L("profile") ) ) {
			SetStamp(iProfileInfo);
		} else if (! elem_name.Compare(_L("base") ) ) {
			SetStamp(iLocInfo);
		} else if (! elem_name.Compare(_L("useractivity") ) ) {
			SetStamp(iActivityInfo);
		} else if (! elem_name.Compare(_L("bt.presence") ) ) {
			SetStamp(iNeighbourhoodInfo);
		} else if (! elem_name.Compare(_L("profile.id") ) ) {
			next=EProfileId;
		} else if (! elem_name.Compare(_L("profile.name") ) ) {
			next=EProfileName;
		} else if (! elem_name.Compare(_L("profile.ringtype") ) ) {
			next=EProfileRingType;
		} else if (! elem_name.Compare(_L("profile.ringvolume") ) ) {
			next=EProfileRingVolume;
		} else if (! elem_name.Compare(_L("profile.vibrate") ) ) {
			next=EProfileVibra;
		} else if (! elem_name.Compare(_L("location.network") ) ) {
			next=ECellNw;
		} else if (! elem_name.Compare(_L("location.lac") ) ) {
			next=ECellLAC;
		} else if (! elem_name.Compare(_L("location.cellid") ) ) {
			next=ECellId;
		} else if (! elem_name.Compare(_L("base.previous") ) ) {
			next=ELocPrevious;
		} else if (! elem_name.Compare(_L("base.current") ) ) {
			next=ELocCurrent;
		} else if (! elem_name.Compare(_L("base.previous.left") ) ) {
			next=ELocPUpdate;
		} else if (! elem_name.Compare(_L("base.current.arrived") ) ) {
			next=ELocCUpdate;
		} else if (! elem_name.Compare(_L("base.next") ) ) {
			next=ELocNext;
		} else if (! elem_name.Compare(_L("useractivity.value") ) ) {
			next=EActivityMode;
		} else if (! elem_name.Compare(_L("datetime") ) ) {
			next=EStamp;
		} else if (! elem_name.Compare(_L("buddies") ) ) {
			next=EBuddies;
		} else if (! elem_name.Compare(_L("other_phones") ) ) {
			next=EOtherPhones;
		} 
	iChars->Des().Zero();
	iParseState->Push(next);
}

EXPORT_C TTime CPresenceData::ParseTimeL(const TDesC& Str)
{
	TInt year, month, day, hour, min, sec, err;
        TLex lex;
        
	lex=Str.Mid(0, 4);
        err =  lex.Val(year);

        lex=Str.Mid(4, 2);
        err += lex.Val(month);

        lex=Str.Mid(6, 2);
        err += lex.Val(day);

        lex=Str.Mid(9, 2);
        err += lex.Val(hour);

        lex=Str.Mid(11, 2);
        err += lex.Val(min);

        lex=Str.Mid(13, 2);
        err += lex.Val(sec);
	

	if (err ==KErrNone)
	{
		TDateTime dt; TTime Stamp;
		dt.Set(year,TMonth(month-1),day-1,hour,min,sec,0);
		Stamp=TTime(dt);
		return Stamp;
	}
	else
	{
		TTime dummy = TTime(_L("20000101:000000.000000"));
		return dummy;
	}
}

EXPORT_C void CPresenceData::MapLocInfo(HBufC * locInfo)
{
	TPtr tmp = locInfo->Des();
	TInt pos = tmp.FindC(_L(" helsinki"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 9, _L(" HKI"));
	}
	pos = tmp.FindC(_L(" tampere"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 8, _L(" TRE"));
	}
	pos = tmp.FindC(_L(" espoo"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 6, _L(" ESP"));
	}
	pos = tmp.FindC(_L(" turku"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 6, _L(" TKU"));
	}
	pos = tmp.FindC(_L(" oulu"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 5, _L(" OUL"));
	}
	pos = tmp.FindC(_L(" jyväskylä"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 10, _L(" JKL"));
	}
	pos = tmp.FindC(_L(" vantaa"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 7, _L(" VNT"));
	}
	pos = tmp.FindC(_L(" kuopio"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 7, _L(" KPO"));
	}
	pos = tmp.FindC(_L(" lahti"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 6, _L(" LHT"));
	}
	pos = tmp.FindC(_L(" kirkkoniemi"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 12, _L(" KNM"));
	}
	pos = tmp.FindC(_L(" joensuu"));
	if (pos != KErrNotFound)
	{
		tmp.Replace(pos, 8,_L(" JSU"));
	}

	
}

void CPresenceData::ParseStamp(TTime& Stamp)
{
	Stamp=ParseTimeL(*iChars);
	
}

void CPresenceData::EndElement(const XML_Char * /*name*/)
{
	iChars->Des().TrimAll();
	TLex lex(*iChars);

	switch (iParseState->Pop()) {
	case EStamp:
		ParseStamp(iStamp);
		break;
	case ELocPUpdate:
		ParseStamp(iLocInfo.iPUpdated);
		break;
	case ELocCUpdate:
		ParseStamp(iLocInfo.iCUpdated);
		break;
	case EProfileId:
		lex.Val(iProfileInfo.iProfileId);
		break;
	case EProfileRingType:
		lex.Val(iProfileInfo.iRingingType);
		break;
	case EProfileRingVolume:
		lex.Val(iProfileInfo.iRingingVolume);
		break;
	case EProfileVibra:
		if (! iChars->Compare(_L("true")) ) {
			iProfileInfo.iVibra=ETrue;
		} else {
			iProfileInfo.iVibra=EFalse;
		}
		break;
	case EProfileName:
		iProfileInfo.iProfileName=*iChars;
		break;
	case ECellNw:
		iCellInfo.iNetwork=*iChars;
		break;
	case ECellLAC:
		lex.Val(iCellInfo.iLAC);
		break;
	case ECellId:
		lex.Val(iCellInfo.iCellId);
		break;
	case ELocPrevious:
		delete iLocInfo.iPrevious; iLocInfo.iPrevious=0;
		MapLocInfo(iChars);
		iLocInfo.iPrevious=iChars->AllocL();
		
		iLocInfo.iFlags|=TLocInfo::EPrevious;
		break;

	case ELocCurrent:
		delete iLocInfo.iCurrent; iLocInfo.iCurrent=0;
		MapLocInfo(iChars);
		iLocInfo.iCurrent=iChars->AllocL();
		
		iLocInfo.iFlags|=TLocInfo::ECurrent;
		break;

	case ELocNext:
		delete iLocInfo.iNext; iLocInfo.iNext=0;
		MapLocInfo(iChars);
		iLocInfo.iNext=iChars->AllocL();
		
		iLocInfo.iFlags|=TLocInfo::ENext;
		break;

	case EActivityMode:
		if (! iChars->Compare(_L("idle")) ) {
			iActivityInfo.iMode=TActivityInfo::EIdle;
		} else {
			iActivityInfo.iMode=TActivityInfo::EActive;
		}
		break;
	case EBuddies:
		lex.Val(iNeighbourhoodInfo.iBuddies);
		break;

	case EOtherPhones:
		lex.Val(iNeighbourhoodInfo.iOtherPhones);
		break;
	default:
		break;
	}
}

void CPresenceData::CharacterData(const XML_Char *s,
			    int len)
{
	TPtrC c( (TUint16*)s, len);
	while (c.Length()+iChars->Length() > iChars->Des().MaxLength()) {
		iChars=iChars->ReAllocL(iChars->Des().MaxLength()*2);
	}
	iChars->Des().Append(c);
}

void CPresenceData::Error(XML_Error Code, const XML_LChar * String, long /*ByteIndex*/)
{
	iErrorCode=Code; 
	delete iError; iError=0; iError=TPtrC( (TUint16*) String).AllocL();
}

EXPORT_C void PresenceToListBoxL(const MPresenceData* data, HBufC*& name,
				 HBufC* last_name, HBufC* first_name, const TDesC& prev,
				 const TDesC& not_avail, bool last_name_first)
{
	//const MPresenceData* presence=data;

	/* length */
	TInt base_len=0;
	if ( data && data->LocInfo().iFlags & TLocInfo::EPrevious ) base_len=data->LocInfo().iPrevious->Length();
	else if ( data && data->LocInfo().iFlags & TLocInfo::ECurrent ) base_len=data->LocInfo().iCurrent->Length();

	TInt len=name->Length()+base_len+120;
	name->Des().Zero();
	while (len>name->Des().MaxLength()) {
		name=name->ReAllocL(name->Des().MaxLength()*2);
	}

	// - Name --------------------------------------------------------------



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

	name->Des().Append(_L("\t"));

	if (!data || ! data->IsSent()) {
		name->Des().Append(not_avail);
		name->Des().AppendFormat(_L("\t0\t0\t0\t0\t0\t%02d"), GetIconIndex(EMbmAvkonQgn_indi_marked_add) );
		return;
	}
	
	TBool out_of_date = IsOutOfDate(data->SendTimeStamp());
		
	// - Location Info -----------------------------------------------------
	
	TTime stamp; stamp.HomeTime();
	
	if (  (data->LocInfo().iFlags & TLocInfo::EPrevious) &&
		 !( data->LocInfo().iFlags & TLocInfo::ECurrent) ) 
	{
		name->Des().Append(_L("? ("));
		name->Des().Append(prev);
        name->Des().Append(_L(" "));			
		name->Des().Append(*(data->LocInfo().iPrevious));
		stamp = data->LocInfo().iPUpdated;
	} 
	else if (data->LocInfo().iFlags & TLocInfo::ECurrent) 
	{
		name->Des().Append(*(data->LocInfo().iCurrent));
		stamp = data->LocInfo().iCUpdated;
	} 
	else 
	{
		name->Des().Append(_L("?"));
	}
	
	name->Des().Append(_L(" ("));
	name->Des().Append(TimeSinceStamp(stamp));
	name->Des().Append(_L(")"));
	
	if (  (data->LocInfo().iFlags & TLocInfo::EPrevious) &&
		!( data->LocInfo().iFlags & TLocInfo::ECurrent) ) {
		name->Des().Append(_L(" )"));
	}
	
	// - User activity --------------------------------------------------
	if (data->ActivityInfo().iMode == TActivityInfo::EActive )
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
	}
	else if (data->ActivityInfo().iMode == TActivityInfo::EIdle)
	{
		TTimeIntervalMinutes minutes;
		TTime now; now.HomeTime();
		
		now.MinutesFrom(data->ActivityInfo().iUpdated, minutes);
		
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
	else // unknown
	{
		// no icon
		name->Des().Append(_L("\t0"));
	}
	
	
	// - Speaker Info -----------------------------------------------------
	if ( data->ProfileInfo().iRingingType ==  MPresenceData::ERingingTypeSilent )
	{
		if (out_of_date) 
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_off_grey) );
			name->Des().Append(icon_int);	
		}
		else
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_off) );
			name->Des().Append(icon_int);	
		}
		
	}
	else
	{
		if (out_of_date) 
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_on_grey) );
			name->Des().Append(icon_int);		
		}
		else
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonSpeaker_on) );
			name->Des().Append(icon_int);				
		}
	}
	
	// - Vibrator Info -----------------------------------------------------
	if ( data->ProfileInfo().iVibra )
	{
		if (out_of_date) 
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_on_grey) );
			name->Des().Append(icon_int);		
		}
		else
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_on) );
			name->Des().Append(icon_int);
			
		}
	}
	else
	{
		if (out_of_date) 
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_off_grey) );
			name->Des().Append(icon_int);		
		}
		else
		{
			TBuf<5> icon_int;
			icon_int.Format(_L("\t%02d"), GetIconIndex(EMbmContextcommonVibrator_off) );
			name->Des().Append(icon_int);
			
		}
	}
	// - NeighbourhoodInfo -------------------
	// - Buddies -----------------------------
	TBuf<5> icon_int;
	TInt idx = 0;

	switch (data->NeighbourhoodInfo().iBuddies)
	{
		case 0:
			idx = 0;
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
	icon_int.Format(_L("\t%02d"), idx );
	name->Des().Append(icon_int);
	
	// - Other_phones -----------------------------
	idx=0;
	icon_int.Zero();
	
	switch (data->NeighbourhoodInfo().iOtherPhones)
	{
		case 0:
			idx = 0;
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
	icon_int.Format(_L("\t%02d"), idx );
	name->Des().Append(icon_int);

	// - marked icon - has to be the final one! -------------------------------------

	idx = GetIconIndex(EMbmAvkonQgn_indi_marked_add);
	icon_int.Format(_L("\t%02d"), idx );
	name->Des().Append(icon_int);
}



EXPORT_C TBool IsOutOfDate(TTime stamp, TInt freshness_interval)
{
	//const TInt KFreshnessInterval = 10; // in minutes 
	
	TTime now = TTime();
	now.HomeTime();

	TTimeIntervalMinutes m;
	now.MinutesFrom(stamp, m);
	if (m.Int() > freshness_interval)
	{
		return ETrue;	
	}
	else
	{
		return EFalse;
	}
}

EXPORT_C TBuf<8> TimeSinceStamp(TTime stamp, TInt minuteInterval)
{
	TTime now; now.HomeTime();
	TBuf<128> ret;
	
	TTimeIntervalDays days = now.DaysFrom(stamp);
	if (days.Int() >= 2)
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
				
		ret.Format(_L("%d:"), hours.Int());
				
		TInt min = (minutes.Int() - (hours.Int() * 60));
		if (minuteInterval==0) minuteInterval=10;

		TInt32 result = 0;
		TInt err = Math::Int(result, min/minuteInterval);
		if (err==KErrNone)
		{
			result = result*minuteInterval;	
		}
		else
		{
			result = min;
		}
		ret.AppendFormat(_L("%02d"), result);
	}
	return ret;
}

void CPresenceData::AddRef()
{
	++iRefCount;
}

void CPresenceData::Release()
{
	--iRefCount;
	if (!iRefCount) delete this;
}
