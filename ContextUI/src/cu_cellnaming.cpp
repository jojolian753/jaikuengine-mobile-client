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

#include "cu_cellnaming.h"
#include "app_context_impl.h"
#include "cbbsession.h"
#include "csd_event.h"
#include "emptytext.h"
#include "csd_cell.h"
#include "break.h"
#include "cu_common.h"
#include "csd_cellnaming.h"
#include "csd_base.h"
#include "reporting.h"
#include "AknNotifyStd.h"

#include <aknnotewrappers.h>
#include <bautils.h>
#include <coeaui.h>
#include <contextui.rsg>
#include <eikenv.h>

#include "contextvariant.hrh"

void CheckPredicatesL()
{
	if ( ((CCoeAppUi*)CEikonEnv::Static()->AppUi())->IsDisplayingMenuOrDialog()) return;

	MApp_context* c=GetContext();
	TApaTaskList tl(CEikonEnv::Static()->WsSession());
	TApaTask app_task=tl.FindApp( KUidcontext_log );
	if (! app_task.Exists() ) {
		//FIXMELOC
		c->Reporting().ShowGlobalNote(EAknGlobalErrorNote ,
			_L("Jaiku Settings is not running. Location naming requires Jaiku Settings."));
		return;
	}

}

HBufC* GetTupleValueL(const TTupleName& aNameTuple)
{
	CBBSession* bbsession=GetContext()->BBSession();
	MBBData* name_event_m=0;
	bbsession->GetL(aNameTuple, KNullDesC, name_event_m, ETrue);
	bb_auto_ptr<MBBData> eventp(name_event_m);
	const CBBSensorEvent* name_event=bb_cast<CBBSensorEvent>(name_event_m);
	const TBBFixedLengthStringBase* existing_name=0;
	if (name_event) {
		existing_name=bb_cast<TBBShortString>(name_event->iData());
		if (!existing_name) existing_name=bb_cast<TBBLongString>(name_event->iData());
	}
	
	if (existing_name) return existing_name->Value().AllocL();
	else return NULL;
}


TInt GetBaseIdL(const TTupleName& aNameTuple)
{
	CBBSession* bbsession=GetContext()->BBSession();

	TInt baseid=-1;
	TTime base_event_t(0);
	if (aNameTuple==KCellNameTuple) {
		MBBData *base_event_d=0;
		bbsession->GetL(KLocationTuple, KNullDesC, base_event_d, ETrue);
		bb_auto_ptr<MBBData> p(base_event_d);
		const CBBSensorEvent *ev=bb_cast<CBBSensorEvent>(base_event_d);
		if (ev) {
			const TBBLocation* c=bb_cast<TBBLocation>(ev->iData());
			if (c && c->iIsBase()) {
				base_event_t=ev->iStamp();
				baseid=c->iLocationId();
			}
		}
	}
	TBool no_coverage=EFalse;
	{
		MBBData *cell_event_d=0;
		bbsession->GetL(KCellIdTuple, KNullDesC, cell_event_d, ETrue);
		bb_auto_ptr<MBBData> p(cell_event_d);
		const CBBSensorEvent *ev=bb_cast<CBBSensorEvent>(cell_event_d);
		if (!ev) return KErrNotFound;
		const TBBCellId* c=bb_cast<TBBCellId>(ev->iData());
		if (!c) return KErrNotFound;
		if (c->iLocationAreaCode()==0 && c->iCellId()==0) {
			no_coverage=ETrue;
			//FIXMELOC
			return KErrNotFound; // no coverage
		}
		if (baseid==-1 || ev->iStamp() > base_event_t) {
			baseid=c->iMappedId();
		}
	}
	return baseid;
}



void StoreTupleL(const TTupleName& aNameTuple, const TTupleName& aGivenNameTuple, const TDesC& aNameName,
				 const TDesC& aValue, TInt aBaseId)
{
		TTime exp=GetTime();
		exp+=TTimeIntervalMinutes(10);
		bb_auto_ptr<CBBSensorEvent> e(new (ELeave) CBBSensorEvent(aNameName,
			aGivenNameTuple, GetContext()->BBDataFactory(), GetTime()));
		e->iData.SetOwnsValue(EFalse);

		if (aNameTuple==KCellNameTuple) {
			TBBCellNaming name;
			name.iMappedId=aBaseId;
			name.iName()=aValue;
			e->iData.SetValue(&name);
			GetContext()->BBSession()->PutL(aGivenNameTuple, KNullDesC, e.get(),
											exp);
		} else {
			TBBLongString name(aValue, aNameName);
			e->iData.SetValue(&name);
			GetContext()->BBSession()->PutL(aGivenNameTuple, KNullDesC, e.get(),
											exp);
		}
}

#ifdef __DEV__
#include "reporting.h"
void DebugLog(const TDesC& aMsg) { 
	GetContext()->Reporting().DebugLog(aMsg);
}
#else
inline void DebugLog(const TDesC&) { }
#endif

class CEmptyAllowingMultiLineDataQueryDialog : public CAknMultiLineDataQueryDialog {
public:
	CEmptyAllowingMultiLineDataQueryDialog() : CAknMultiLineDataQueryDialog(ENoTone) { }
	void ConstructL(TDes& aDataText, TDes& aDataText2) {
		SetDataL(aDataText, aDataText2);
	}
	static CEmptyAllowingMultiLineDataQueryDialog* NewL(TDes& aDataText, TDes& aDataText2) {
		auto_ptr<CEmptyAllowingMultiLineDataQueryDialog> ret(new (ELeave) 
			CEmptyAllowingMultiLineDataQueryDialog);
		ret->ConstructL(aDataText, aDataText2);
		return ret.release();
	}
	void UpdateLeftSoftKeyL()
	{
		MakeLeftSoftkeyVisible(ETrue);
	}
	TKeyResponse  OfferKeyEventL (const TKeyEvent &aKeyEvent, TEventCode aType) {
		return CAknMultiLineDataQueryDialog::OfferKeyEventL(aKeyEvent, aType);
	}

};

void NameLocationL()
{
	DebugLog(_L("CheckPredicatesL"));
	CheckPredicatesL();

	const TInt KMaxLen(BB_LONGSTRING_MAXLEN);
	
	auto_ptr<HBufC> hood(HBufC::NewL(KMaxLen));
	auto_ptr<HBufC>	hoodOld( GetTupleValueL( KCellNameTuple ) );
	if (hoodOld.get()) hood->Des() = *hoodOld;

	TBool no_coverage = EFalse;
	DebugLog(_L("GetBaseIdL"));
	TInt baseid = GetBaseIdL( KCellNameTuple );
	if ( baseid == KErrNotFound )
		{
			no_coverage = ETrue;
		}
		
	auto_ptr<HBufC> city(HBufC::NewL(KMaxLen));
	auto_ptr<HBufC>	cityOld( GetTupleValueL( KCityNameTuple ) );
	if (cityOld.get()) city->Des() = *cityOld;

	if ( no_coverage ) 
		{
			DebugLog(_L("no coverage"));
			_LIT(KNoCoverageNote, "No network coverage. Location naming is not possible.");
			
			CAknInformationNote* note = new (ELeave) CAknInformationNote( ETrue ); //Waiting			
			note->ExecuteLD(KNoCoverageNote); //Blocks until 
		}
	else 
		{
			DebugLog(_L("ask name"));
			
			TPtr hoodP = hood->Des();
			TPtr cityP = city->Des(); 
			CEmptyAllowingMultiLineDataQueryDialog* dlg = CEmptyAllowingMultiLineDataQueryDialog::NewL(hoodP, cityP);
			dlg->SetPredictiveTextInputPermitted( ETrue );
			if (dlg->ExecuteLD(R_LOCATION_NAME_QUERY) && ! no_coverage )
				{
					TBool cityModified = ( ! cityOld.get() ) || (cityOld->Compare(*city) != 0);
					TBool hoodModified = ( ! hoodOld.get() ) || (hoodOld->Compare(*hood) != 0);
					
					if (hoodModified) StoreTupleL( KCellNameTuple, KGivenCellNameTuple, KCellName, *hood, baseid);
					if (cityModified) StoreTupleL( KCityNameTuple, KGivenCityNameTuple, KCity,     *city, baseid);
				}
		}
}




void name_cellL(const TTupleName& aNameTuple, 
				const TTupleName& aGivenNameTuple,
				const TDesC& aNameName)
{
	CheckPredicatesL();

	const TInt NAME_LENGTH=255;
	auto_ptr<HBufC> textData(HBufC::NewL(NAME_LENGTH));
	auto_ptr<HBufC>	existing_name( GetTupleValueL( aNameTuple ) );
	if (existing_name.get()) textData->Des() = *existing_name;

	TBool no_coverage = EFalse;
	TInt baseid = GetBaseIdL( aNameTuple );
	if ( baseid == KErrNotFound )
		{
			_LIT(KNoCoverage, "[no coverage]");
			textData->Des() = KNoCoverage;
			no_coverage = ETrue;
		}
		
		
	TPtr16 p=textData->Des();

	auto_ptr<HBufC> f(0);
	if (aNameTuple==KCellNameTuple) {
		f.reset(CEikonEnv::Static()->AllocReadResourceL(R_CL_NAME_OLD_CELL_CAPTION));
	} else {
		// city
		f.reset(CEikonEnv::Static()->AllocReadResourceL(R_CL_NAME_OLD_CITY_CAPTION));
	}
	auto_ptr<HBufC> pr(HBufC::NewL(f->Des().Length()+4));
	pr->Des()=*f;

	CAknTextQueryDialog* dlg = new(ELeave) CEmptyAllowingTextQuery(p);
	CleanupStack::PushL(dlg);
	dlg->SetMaxLength(NAME_LENGTH);
	dlg->SetPromptL(*pr);
	CleanupStack::Pop();
	TInt resource=R_CONTEXT_LOG_NAME_INPUT;
	if (no_coverage) resource=R_CONTEXT_LOG_NAME_INPUT_READONLY;
	dlg->SetPredictiveTextInputPermitted(ETrue);
	if (dlg->ExecuteLD(resource) && textData->Length() && !no_coverage)
	{
		if (existing_name.get() && existing_name->Compare(*textData)==0) return;

		StoreTupleL( aNameTuple, aGivenNameTuple, aNameName, *textData, baseid);
	}
}

TInt LoadResourceFileL() {
	return LoadSystemResourceL(CEikonEnv::Static(), _L("contextui"));
}

EXPORT_C void AskUserForCurrentCellNameL()
{
	TInt res=LoadResourceFileL();
	CC_TRAPD(err, name_cellL(KCellNameTuple, KGivenCellNameTuple, KCellName));
	if (res) CEikonEnv::Static()->DeleteResourceFile(res);
	User::LeaveIfError(err);
}

EXPORT_C void AskUserForCurrentCityNameL()
{
	TInt res=LoadResourceFileL();
	CC_TRAPD(err, name_cellL(KCityNameTuple, KGivenCityNameTuple, KCity));
	if (res) CEikonEnv::Static()->DeleteResourceFile(res);
	User::LeaveIfError(err);
}


EXPORT_C void AskUserForCurrentLocationL()
{
	TInt res=LoadResourceFileL();
	CC_TRAPD(err, NameLocationL());
	if (res) CEikonEnv::Static()->DeleteResourceFile(res);
	User::LeaveIfError(err);	
}
