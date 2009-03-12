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

#include "cellnaming.h"
#include "symbian_tree.h"
#include "current_loc.h"
#include "locationing.h"
#include "alerter.h"
#include "csd_cell.h"
#include <AknQueryDialog.h> 
#include "app_context_fileutil.h"
#include <f32file.h>
#include <bautils.h>
#include "mcc.h"
#include "cl_settings.h"
#include "cellmap.h"
#include "csd_presence.h"
#include "contextcommon.h"
#include "raii_f32file.h"
#include "concretedata.h"
#include "cu_common.h"

#include "contextlog_resource.h"

#include "emptytext.h"
#include "contextappui.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "csd_base.h"
#include <contextui.rsg>
#include "csd_cellnaming.h"
#include "contextvariant.hrh"

_LIT(KCellAnnotation, "cell_ann");
_LIT(cellid_file_v1, "cellid_names.txt");
_LIT(cellid_file_v2, "cellid_names_v2.txt");
_LIT(cellid_file, "cellid_names_v3.txt");
_LIT(install_cellid_file, "cellid_names_inst_v3.txt");
_LIT(transfer_cellid_file, "cellid_names_trans_v3.txt");

void delete_bufc(void* data)
{
	HBufC* p;
	p=(HBufC*)data;
	delete p;
}

class CCellNamingImpl : public CCellNaming, public MContextBase, public Mlogger {
private:

	CGenericIntMap* cellid_names;
	CCurrentLoc*	iCurrentLoc;

	TInt iCurrentBaseId, iCurrentCellId, iCurrentCityId;
	TBBLongString iCity;
	TBBShortString iCountry;
	TBBLongString iCell;
	TBBCellId iCurrentCell;
	TBool iSeenLoc;
	MCellMapping*	iSensorRunner;
	CCnvCharacterSetConverter* iCC;
	TInt iResource;

	CCellNamingImpl(MApp_context& aContext) : MContextBase(aContext),
		iCity(KCity), iCountry(KCountry), iCell(KCellName), iCurrentCell(KCellName) { }

	void ConstructL(MCellMapping* aSensorRunner) {
		iSensorRunner=aSensorRunner;
		Mlogger::ConstructL(AppContextAccess());
		SubscribeL(KCellIdTuple);
		SubscribeL(KLocationTuple);
		SubscribeL(KGivenCellNameTuple);
		SubscribeL(KGivenCityNameTuple);
		SubscribeL(KIncomingPresence);

		iCC=CCnvCharacterSetConverter::NewL();
		convert_v2_to_v3();

		TUint cellid_file_version=2;
		{
			MoveIntoDataDirL(AppContext(), cellid_file);
			// copy the installed cellid_names over the real one, if
			// one has been installed

			TFileName install_cellid_filen, cellid_filen, cellid_filen_v1;
			install_cellid_filen.Format(_L("%S%S"), &AppDir(), &install_cellid_file);
			cellid_filen.Format(_L("%S%S"), &DataDir(), &cellid_file);
			cellid_filen_v1.Format(_L("%S%S"), &DataDir(), &cellid_file_v1);
			if (BaflUtils::FileExists(Fs(), install_cellid_filen)) {
				User::LeaveIfError(BaflUtils::CopyFile(Fs(), install_cellid_filen, cellid_filen));
				User::LeaveIfError(BaflUtils::DeleteFile(Fs(), install_cellid_filen));
			}
			TEntry e;
			if ( ( Fs().Entry(cellid_filen, e)!=KErrNone || e.iSize==0 ) 
					&& cellid_name_file_v1.Open(Fs(), cellid_filen_v1, 
					EFileShareAny | EFileStreamText | EFileRead | EFileWrite)==KErrNone) {
				cellid_file_version=1;
			}

			if (cellid_name_file.Open(Fs(), cellid_filen, 
				EFileShareAny | EFileStreamText | EFileRead | EFileWrite)!=KErrNone) {
				User::LeaveIfError(cellid_name_file.Create(Fs(), cellid_filen, 
					EFileShareAny | EFileStreamText | EFileRead | EFileWrite));
			}
		}

		cellid_names=CGenericIntMap::NewL();
		cellid_names->SetDeletor(delete_bufc);

		iCurrentLoc=CCurrentLoc::NewL(AppContext(), cellid_names);

		cell_ann=Clog_base_impl::NewL(AppContext(), KCellAnnotation, KCellAnnotationTuple, 0);
		city_name=Clog_base_impl::NewL(AppContext(), KCity, KCityNameTuple, 5*24*60*60);
		cell_name=Clog_base_impl::NewL(AppContext(), KCellName, KCellNameTuple, 5*24*60*60);
		country_name=Clog_base_impl::NewL(AppContext(), KCountry, KCountryNameTuple, 5*24*60*60);

		if (cellid_file_version==1) {
			iCC->PrepareToConvertToOrFromL(KCharacterSetIdentifierIso88591, Fs());
			read_cellid_names(cellid_file_version);
			cellid_name_file_v1.Close();
		} else {
			iCC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());
			read_cellid_names(cellid_file_version);
		}

		TInt pos=0;
		cellid_name_file.Seek(ESeekEnd, pos);

	}

	Mlog_base_impl * cell_ann, *city_name, *country_name, * cell_name;
	~CCellNamingImpl() {

		delete cellid_names;
		delete iCurrentLoc;
		delete cell_ann;
		delete city_name;
		delete country_name;
		delete cell_name;
		cellid_name_file.Close();
		delete iCC;
	}

	void add_name_to_map(const TBBCellId& Cell, const TDesC& name) {
		if (Cell.iCellId()==0 && Cell.iLocationAreaCode()==0) return;
		HBufC* n=HBufC::NewL(name.Length());
		CleanupStack::PushL(n);
		*n=name;
		TInt id=Cell.iMappedId();
		cellid_names->AddDataL(id, (void*)n, true); 
		CleanupStack::Pop(); // n
	}
	void add_cellid_name(const TBBCellId& Cell, const TDesC& name) {
		if (Cell.iCellId()==0 && Cell.iLocationAreaCode()==0) return;
		add_cellid_name(Cell, name, ETrue);
	}
	void add_cellid_name(const TBBCellId& Cell, const TDesC& name, TBool
			aCurrent) {

		if (Cell.iCellId()==0 && Cell.iLocationAreaCode()==0) return;
		if (name.Length()==0) return;

		if (aCurrent) {
			if (Cell.iCellId()==0) {
				if (name.Length()>iCity.Value().MaxLength()) {
					iCity()=name.Left(iCity.Value().MaxLength());
				} else {
					iCity()=name;
				}
				city_name->post_new_value(&iCity);
			} else {
				if (name.Length()>iCell.Value().MaxLength()) {
					iCell()=name.Left(iCell.Value().MaxLength());
				} else {
					iCell()=name;
				}
				cell_name->post_new_value(&iCell);
			}
		}

		TBuf<50> b;
		Cell.IntoStringL(b);
		write_to_output(b);
		_LIT(tab, "\t");
		write_to_output(tab);
		write_to_output(name);
		_LIT(nl, "\n");
		write_to_output(nl);

		_LIT(KCellAnn, "cell_ann");
		TBBShortString m(name, KCellAnn);
		cell_ann->post_new_value(&m);

		add_name_to_map(Cell, name);
		TInt id=Cell.iMappedId();
		if (id==iCurrentLoc->CurrentBaseId()) {
			TTime now=GetTime();
			iCurrentLoc->now_at_location(&Cell, id, true, false, now);
		}
	}
	bool is_named(TInt id)
	{
		CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("is_named"));

		if (cellid_names->GetData(id)) return true;
		return false;
	}


	CCircularLog*	BaseLog()
	{
		return iCurrentLoc->BaseLog();
	}

	CGenericIntMap* get_cellid_names()
	{
		return cellid_names;
	}
	CCurrentLoc* CurrentLoc()
	{
		return iCurrentLoc;
	}


	enum ui_state { IDLE, NAME_CELLS };
	ui_state current_state;

	Clocationing* loc;

	void NewSensorEventL(const TTupleName& aName, 
						const TDesC& aSubName, const CBBSensorEvent& aEvent)
	{
		if (aName==KLocationTuple) {
			const TBBLocation* loc=bb_cast<TBBLocation>(aEvent.iData());
			if (!loc) return;
			now_at_location(&(loc->iCellId), loc->iLocationId(), loc->iIsBase(), loc->iLocationChanged(),
				loc->iEnteredLocation());
		} else if (aName==KCellIdTuple) {
			const TBBCellId* cell=bb_cast<TBBCellId>(aEvent.iData());
			if (!cell) return;
			now_at_cell(cell, aEvent.iStamp());
		} else if (aName==KGivenCellNameTuple || aName==KGivenCityNameTuple) {
			TInt id=-1;
			const TBBFixedLengthStringBase* name=0;
			const TBBCellNaming* naming=bb_cast<TBBCellNaming>(aEvent.iData());
			if (naming) {
				name=&(naming->iName);
				id=naming->iMappedId();
			} else {
				name=bb_cast<TBBShortString>(aEvent.iData());
				if (!name) name=bb_cast<TBBLongString>(aEvent.iData());
			}
			if (!name) return;

			TBBCellId ask_for(KCellName);
			if (id==-1 && aName==KGivenCellNameTuple) id=iCurrentBaseId;
			if (id!=-1) {
				ask_for.iMappedId()=id;
				TRAPD(err,
					iSensorRunner->CellMap()->GetCellL(id, ask_for));
				if (err==KErrNotFound) id=-1;
				else if (err!=KErrNone) return;
			}
			if (id==-1) {
				id=iCurrentCellId;
				ask_for.iMappedId()=id;
				TRAPD(err,
					iSensorRunner->CellMap()->GetCellL(id, ask_for));
				if (err!=KErrNone) return;
			}
			if (aName==KGivenCellNameTuple) {
				HBufC* existing_name=(HBufC*)get_cellid_names()->GetData( id );
				if (!existing_name || existing_name->Compare(name->Value())) {
					add_cellid_name(ask_for, name->Value());
				}
			} else {
				ask_for.iCellId()=0;
				TRAPD(err,
					id=iSensorRunner->CellMap()->GetId(ask_for));
				if (id==-1 || err!=KErrNone) return;
				ask_for.iMappedId()=id;
				HBufC* existing_name=(HBufC*)get_cellid_names()->GetData( id );
				if (!existing_name || existing_name->Compare(name->Value())) {
					add_cellid_name(ask_for, name->Value());
				}
			}
		}
	}
	virtual void NewValueL(TUint , const TTupleName& aName, 
		const TDesC& aSubName, 
		const TComponentName& /*aComponent*/,
		const MBBData* aData) {
		const CBBSensorEvent* e=bb_cast<CBBSensorEvent>(aData);
		if (e) {
			NewSensorEventL(aName, aSubName, *e);
		} else if (aName==KIncomingPresence) {
			const CBBPresence* pres=bb_cast<CBBPresence>(aData);
			if (!pres) return;

			TBBCellId cell=pres->iCellId;
			if (cell.iMCC()==0 || cell.iMNC()==0 || cell.iLocationAreaCode()==0) return;
			if (pres->iCellName().Length()>0) {
				TInt id=-1;
				TRAPD(err,
					id=iSensorRunner->CellMap()->GetId(cell));
				if (err==KErrNone && id>-1 && !is_named(id)) {
					cell.iMappedId()=id;
					add_cellid_name(cell, pres->iCellName(), iCurrentCellId==id);
				}
			}
			if (pres->iCity().Length()>0) {
				TInt id=-1;
				cell.iCellId()=0;
				TRAPD(err,
					id=iSensorRunner->CellMap()->GetId(cell));
				if (err==KErrNone && id>-1 && !is_named(id)) {
					cell.iMappedId()=id;
					add_cellid_name(cell, pres->iCity(), iCurrentCityId==id);
				}
			}
		}
	}


	void now_at_cell(const TBBCellId* Cell, TTime now)
	{
		TTime time=now;
		iCurrentCell=*Cell;
		GetCountryName( Cell->iMCC(), iCountry() );
		country_name->post_new_value(&iCountry, time);
		HBufC* cellname=0;
		TInt id_for_cell=Cell->iMappedId();
		iCurrentCellId=id_for_cell;
		cellname=(HBufC*)get_cellid_names()->GetData( id_for_cell );
		if (cellname) {
			iCell()=cellname->Des().Left(iCell().MaxLength());
		} else {
			iCell().Zero();
		}
		cell_name->post_new_value(&iCell, time);
		TBBCellId city=*Cell; city.iCellId()=0;
		iCurrentCityId=city.iMappedId()=iSensorRunner->CellMap()->GetId(city);
		HBufC* cityname=(HBufC*)get_cellid_names()->GetData( city.iMappedId() );
		if (cityname) {
			iCity()=cityname->Des().Left(iCity().MaxLength());
		} else {
			iCity().Zero();
		}
		city_name->post_new_value(&iCity, time);

	}
	void now_at_location(const TBBCellId* Cell, TInt id, 
						bool is_base, bool loc_changed, TTime time)
	{
		iCurrentLoc->now_at_location(Cell, id, is_base, loc_changed, time);
		if (! Cell) return;
		TInt id_for_cell=Cell->iMappedId();
		iCurrentCellId=id_for_cell;
		iCurrentBaseId=id;
	}

	RFile cellid_name_file, cellid_name_file_v1;
	TBuf8<128> buf;
	
	void convert_v2_to_v3()
	{
		auto_ptr<HBufC> cellid_filen(HBufC::NewL(256));
		auto_ptr<HBufC> cellid_filen_v2(HBufC::NewL(256));
		cellid_filen->Des().Format(_L("%S%S"), &DataDir(), &cellid_file);
		cellid_filen_v2->Des().Format(_L("%S%S"), &DataDir(), &cellid_file_v2);
		RFile v2; TEntry e;
		if ( ( Fs().Entry(*cellid_filen_v2, e)==KErrNone || e.iSize>0 ) 
				&& v2.Open(Fs(), *cellid_filen_v2, 
				EFileShareAny | EFileStreamText | EFileRead | EFileWrite)==KErrNone ) {
			CleanupClosePushL(v2);
			{
				auto_ptr<CCnvCharacterSetConverter> utf8CC(CCnvCharacterSetConverter::NewL());
				utf8CC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());
				auto_ptr<CCnvCharacterSetConverter> latin1CC(CCnvCharacterSetConverter::NewL());
				latin1CC->PrepareToConvertToOrFromL(KCharacterSetIdentifierIso88591, Fs());
				RAFile v3; v3.ReplaceLA(Fs(), *cellid_filen, EFileWrite | EFileShareAny | EFileStreamText);
				auto_ptr<HBufC8> latin1(HBufC8::NewL(512));
				auto_ptr<HBufC8> utf8(HBufC8::NewL(1024+10));
				auto_ptr<HBufC> unic(HBufC::NewL(512));
				TInt state=CCnvCharacterSetConverter::KStateDefault;
				TPtr8 latin1p=latin1->Des();
				while(v2.Read(latin1p)==KErrNone && latin1->Des().Length()>0) {
					TPtr unicp=unic->Des();
					latin1CC->ConvertToUnicode(unicp, latin1p, state);
					TPtr8 utf8p=utf8->Des();
					utf8CC->ConvertFromUnicode(utf8p, *unic);
					User::LeaveIfError(v3.Write(*utf8));
					unic->Des().Zero();
					latin1->Des().Zero();
					utf8->Des().Zero();
					latin1p.Set(latin1->Des());
				}
			}
			CleanupStack::PopAndDestroy();
		}
		TInt err=Fs().Delete(*cellid_filen_v2);
		if (err!=KErrNone && err!=KErrNotFound) User::Leave(err);
	}
	void read_cellid_names(TUint version)
	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("read_cellid_names"));

		TBuf<BB_LONGSTRING_MAXLEN> cell_name, cellid;
		TBuf<128> unibuf;
		TInt pos=0; TChar cur; TInt len;
		bool read_id=false;

		TInt state=CCnvCharacterSetConverter::KStateDefault;
		if (version==1) {
			cellid_name_file_v1.Read(buf);
		} else {
			cellid_name_file.Read(buf);
		}
		while ( (len=buf.Length()) ) {
			iCC->ConvertToUnicode(unibuf, buf, state);
			len=unibuf.Length();
			while (pos<len) {
				while (!read_id && pos<len && (cur=unibuf[pos])!=L'\t') {
					cellid.Append(cur);
					pos++;
				}
				if (pos<len && !read_id) {
					read_id=true;
					pos++; // skip tab
				}
				while (pos<len && (cur=unibuf[pos])!=L'\n') {
					cell_name.Append(cur);
					pos++;
				}
				if (pos<len) {
					pos++; //skip nl for next
					
					if (cellid.Length() && cell_name.Length()) {
						TBBCellId cell(KCell);

						if (version==1) {
							TUint id, lac;
							CCellMap::Parse(cellid, id, lac, cell.iShortName());
							cell.iCellId()=id; cell.iLocationAreaCode()=lac;
							iSensorRunner->OpMap()->NameToMccMnc(cell.iShortName(), cell.iMCC(), cell.iMNC());
							cell.iMappedId()=iSensorRunner->CellMap()->GetId(cell);
							add_name_to_map(cell, cell_name);
						} else {
								CC_TRAPD(err, cell.FromStringL(cellid));
							if (err==KErrNone) {
								cell.iMappedId()=iSensorRunner->CellMap()->GetId(cell);
								add_name_to_map(cell, cell_name);
							}
						}
					}
					cellid.Zero();
					cell_name.Zero();
					read_id=false;
				}
			}
			buf.Zero();
			pos=0;
			if (version==1) {
				cellid_name_file_v1.Read(buf);
			} else {
				cellid_name_file.Read(buf);
			}
		}
	}

	void write_to_output(const TDesC& str)
	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("write_to_output"));

		TInt pos=0, len;
		len=buf.MaxLength();
		while (pos < str.Length()) {
			TInt real_len;
			if (pos+len > str.Length()) {
				real_len=str.Length()-pos;
			} else {
				real_len=len;
			}
			iCC->ConvertFromUnicode(buf, str.Mid(pos, real_len));
			pos+=len;
			cellid_name_file.Write(buf);
		}

	}


	friend class CCellNaming;
	friend class auto_ptr<CCellNamingImpl>;
};

CCellNaming* CCellNaming::NewL(class MApp_context& aContext,
	class MCellMapping* aSensorRunner)
{
	auto_ptr<CCellNamingImpl> ret(new (ELeave) CCellNamingImpl(aContext));
	ret->ConstructL(aSensorRunner);
	return ret.release();
}
