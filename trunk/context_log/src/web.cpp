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
#include "web.h"
#include "popup_mod.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "context_log.hrh"
#include "contextlog_resource.h"
#include "w32std.h"
#include "browser_interface.h"
#include <viewcli.h> //CVwsSessionWrapper
#include <vwsdef.h>
#include "cn_http.h" // url encoding
#include <apacmdln.h>
#include <charconv.h>

_LIT(KWeb, "web");

CSavedPoints* CSavedPoints::NewL(MApp_context& Context, RDbDatabase& Db)
{
	auto_ptr<CSavedPoints> ret(new (ELeave) CSavedPoints(Context, Db));
	ret->ConstructL();
	return ret.release();
}

const CDesCArray& CSavedPoints::Points(TEndPoint p) const
{
	return *iPoints[p];
}

void CSavedPoints::AddPoint(TEndPoint p, const TDesC& Point)
{
	TUint counts[2]={0, 0};
	counts[p]=1;

	iTable.InsertL();
	iTable.SetColL(1, iNextIdx++);
	iTable.SetColL(2, Point);
	iTable.SetColL(3, counts[0]);
	iTable.SetColL(4, counts[1]);
	PutL();
}

void CSavedPoints::IncUsage(TEndPoint p, TInt PointIdx)
{
	SwitchIndexL(0);
	TDbSeekKey rk( (*iIdxs[p])[PointIdx]);
	if (iTable.SeekL(rk) ) {
		iTable.GetL();
		TUint count=iTable.ColUint32(3+p)+1;
		iTable.UpdateL();
		iTable.SetColL(3+p, count);
		PutL();
	} else {
		// TODO
	}
}

CSavedPoints::~CSavedPoints()
{
	for (int i=0; i<2; i++) {
		delete iPoints[i];
		delete iIdxs[i];
	}
}

CSavedPoints::CSavedPoints(MApp_context& Context, RDbDatabase& Db) : 
	MDBStore(Db), MContextBase(Context)
{
}

void CSavedPoints::ConstructL()
{
#if defined(__S60V2__) && !defined(__WINS__)
	auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL());
	cmd->SetLibraryNameL(_L("e:\\system\\apps\\opera.app"));
	cmd->SetCommandL(EApaCommandBackground);
	CC_TRAPD(err, EikDll::StartAppL(*cmd));
#endif

	TInt cols[]={ EDbColInt32, EDbColText, EDbColUint32, EDbColUint32, -1 };
	TInt idx[]={ 1, -2, 3, -2, 4, -1 };
	MDBStore::ConstructL(cols, idx, true, _L("POINTS"));

	for (int i=0; i<2; i++) {
		iPoints[i]=new (ELeave) CDesC16ArrayFlat(8);
		iIdxs[i]=new (ELeave) CArrayFixFlat<TInt>(8);
		SwitchIndexL(1+i);
		TBool rows=iTable.LastL();
		TInt idx;
		while (rows) {
			/*
			auto_ptr<HBufC> p(iTable.ColDes16(1).AllocL());
			iPoints[i]->AppendL(p.get());
			p.release();
			
			*/
			iTable.GetL();
			iPoints[i]->AppendL(iTable.ColDes16(2));
			idx=iTable.ColInt32(1);
			iIdxs[i]->AppendL(idx);
			if (idx>=iNextIdx) iNextIdx=idx+1;
			rows=iTable.PreviousL();
		}
	}
	
}


CWebForm* CWebForm::NewL(MApp_context& Context, CConnectionOpener& Opener, Mlog_base_impl& Log)
{
	auto_ptr<CWebForm> self (
		new (ELeave) CWebForm(Context, Opener, Log));
	self->ConstructL();
	return self.release();
}
CWebForm::~CWebForm()
{
	delete iPoints;
	delete iDb;
}
CWebForm::CWebForm(MApp_context& Context, CConnectionOpener& Opener, Mlog_base_impl& Log) : 
	MContextBase(Context), iOpener(Opener), iLog(Log)
{
}
void CWebForm::ConstructL()
{
	TInt ap;
	bool ap_set=Settings().GetSettingL(SETTING_IP_AP, ap);
	
	


	if (ap_set) {
		// fire and forget
#ifndef __WINS__
		iOpener.MakeConnectionL(ap);
#endif
	}
	iDb=CDb::NewL(AppContext(), _L("WEB"),EFileRead|EFileWrite);
	iPoints=CSavedPoints::NewL(AppContext(), iDb->Db());

	CAknForm::ConstructL();
}

TInt CWebForm::ExecuteLD()
{
	TBBShortString s(_L("form opened"), KWeb);
	iLog.post_new_value(&s);
	return CAknForm::ExecuteLD(R_CL_WEB_DIALOG);
}
void CWebForm::PrepareLC()
{
	CAknForm::PrepareLC(R_CL_WEB_DIALOG);
}

TInt  CWebForm::GetCurrentItem(TInt Id)
{
	CAknPopupFieldText* f=(CAknPopupFieldText*)Control(Id);
	return f->CurrentValueIndex();
}

TBool CWebForm::SaveFormDataL()
{
	bool value[2]={ false, false };
	TBuf<MAX_ADDRESS_LEN> points[2];
	TInt text_ids[]={ CL_WEB_FROM_TEXT_ID, CL_WEB_TO_TEXT_ID };
	TInt list_ids[]={ CL_WEB_FROM_LIST_ID, CL_WEB_TO_LIST_ID };

	for (int i=0; i<2; i++) {
		CSavedPoints::TEndPoint p=(CSavedPoints::TEndPoint)i;
		GetEdwinText(points[i], text_ids[i]);
		points[i].TrimAll();
		if (points[i].Length()>3) {
			iPoints->AddPoint(p, points[i]);
			value[i]=true;
		} else {
			if (iSaved[i]) {
				TInt idx=GetCurrentItem(list_ids[i]);
				iPoints->IncUsage(p, idx);
				points[i]=iPoints->Points(p)[idx];
				value[i]=true;
			}
		}
	}
	TTime time; time.HomeTime();

	TDateTime dt=time.DateTime();

    
	_LIT(KUrlFormat, "http://aikataulut.ytv.fi/reittiopas-pda/en/?test=1&keya=%S&keyb=%S&hour=%d&min=%d&vm=1&day=%d&month=%d&year=%d");

	if (value[0] && value[1]) {
		TBuf<MAX_ADDRESS_LEN*3> enc_points[2];
		for (int p=0; p<2; p++) {
			CHttp::AppendUrlEncoded(enc_points[p], points[p]);
		}
		auto_ptr<HBufC> addr(HBufC::NewL(KUrlFormat().Length()+enc_points[0].Length()+enc_points[1].Length()+10));
		addr->Des().Format(KUrlFormat, &enc_points[0], &enc_points[1],
			(TInt)dt.Hour(), (TInt)dt.Minute(), 
			(TInt)(dt.Day()+1), (TInt)(dt.Month()+1),
			(TInt)(dt.Year()));

		auto_ptr<CBBString> msg(CBBString::NewL(KWeb, points[0].Length()+points[1].Length()+20));
		msg->iBuf->Des().Format(_L("HKL search from %S to %S"), &(points[0]), &(points[1]));
		iLog.post_new_value(msg.get());
#ifndef __WINS__
#  ifndef __S60V2__
		auto_ptr<CDorisBrowserInterface> ido(CDorisBrowserInterface::NewL());
		ido->AppendL(CDorisBrowserInterface::EOpenURL_STRING, *addr);
		ido->ExecuteL();
#  else
		auto_ptr<HBufC8> addr8(HBufC8::NewL(addr->Length()));
		TPtr8 addrp=addr8->Des();
		CC()->ConvertFromUnicode(addrp, *addr);
		TUid KUidOperaBrowserUid = {0x101F4DED};
		TUid KUidOperaRenderViewUid = {0};
		TVwsViewId viewId(KUidOperaBrowserUid, KUidOperaRenderViewUid);
		auto_ptr<CVwsSessionWrapper> vws(CVwsSessionWrapper::NewL());
		vws->ActivateView(viewId, TUid::Uid(0), *addr8);
#  endif
#endif
	} else {
#ifndef __WINS__
		//iLog.post_new_value(_L("form closed"));
		iOpener.CloseConnection();
#endif
	}

	return true;
}

TBool CWebForm::OkToExitL( TInt aButtonId )
{
	if (aButtonId==EAknSoftkeyShow) {
		CC_TRAPD(err, SaveFormDataL());
		if (err!=KErrNone) {
			//TODO: show error
		}
	}
	return true;
}

void CWebForm::DoNotSaveFormDataL()
{
}
void CWebForm::PostLayoutDynInitL()
{
	CAknForm::PostLayoutDynInitL();
	// To Following line change the edit mode
	SetEditableL(ETrue);

	// The following line set to change status
	// This avoid back set form View status in the beginning 
	// Usually without change back do that, like when we start an empty Form.
	SetChangesPending(ETrue);

	TInt list_count=2;
	TInt lists[]={ CL_WEB_FROM_LIST_ID, CL_WEB_TO_LIST_ID };

	for (int list=0; list<list_count; list++) {
		CSavedPoints::TEndPoint p=(CSavedPoints::TEndPoint)list;
		const CDesCArray& texts=iPoints->Points(p);
		if (texts.Count()>0) {
			CAknPopupFieldText* f=(CAknPopupFieldText*)Control(lists[list]);
			CDesCArray* contents=f->iArray;
			contents->Delete(0);
			for (int t=0; t<texts.Count(); t++) {
				contents->AppendL(texts[t]);
			}
			f->SetCurrentValueIndex(0);
			iSaved[list]=true;
		}
	}
}

TBool CWebForm::QuerySaveChangesL()
{
	TBool isAnsYes(CAknForm::QuerySaveChangesL());
	
	if (isAnsYes)
        {
		SaveFormDataL();
        }
	else 
        {
		// Case that answer "No" to query.
		DoNotSaveFormDataL();
        }
	
	return isAnsYes;
}

