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
#include "Context_logContainer.h"

#include <eiklabel.h>  // for example label control
#include <eikenv.h>

#include "cl_settings.h"
#include "basestack.h"
#include "symbian_auto_ptr.h"
#include <aknmessagequerydialog.h> 
#include "contextlog_resource.h"

#include "concretedata.h"
#include "csd_cell.h"
#include "csd_base.h"
#include "csd_bluetooth.h"
#include "csd_gps.h"
#include "csd_loca.h"
#include "csd_profile.h"
#include "csd_unread.h"
#include "csd_presence.h"
#include "csd_calendar.h"
#include "cellnaming.h"

#ifdef USE_SKIN
#include <AknsControlContext.h>
#include <AknsBasicBackgroundControlContext.h>
#include <AknsDrawUtils.h>
#endif

enum KEYCODES {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = 0xF845,
	KEY_CALL = 0xF862,
	KEY_CANCEL = 0xF863
};

// ================= MEMBER FUNCTIONS =======================

void CContext_logContainer::register_source(const TDesC& name)
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("register_source"));

	int this_label_index=label_store->Count()+1;

	auto_ptr<CEikLabel> label(new (ELeave) CEikLabel);
	label->SetContainerWindowL( *this );
	label->SetFont(dense);
	label->SetTextL( name );
	log_labels->AppendL(name);
	label->SetExtent( TPoint(10, (this_label_index-1)*line_height+margin), TSize(sep-5, height));
	label_store->AppendL(label.get());
	label.release();
	
	label.reset(new (ELeave) CEikLabel);
	label->SetContainerWindowL( *this );
	label->SetFont(CEikonEnv::Static()->DenseFont());
	label->SetTextL( _L("") );
	label->SetExtent( TPoint(sep+5, (this_label_index-1)*line_height+margin), TSize(width-sep-10, height));
	value_store->AppendL(label.get());
	label.release();

	if (iCursorPos==-1) iCursorPos=0;

	SizeChanged();
	DrawNow();
}

TKeyResponse CContext_logContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	if (iCursorPos==-1) return EKeyWasNotConsumed;

	if (aKeyEvent.iCode==JOY_CLICK) {
		CEikLabel* label=value_store->At(iCursorPos);
		const TDesC *text=label->Text();
		HBufC* buf=HBufC::NewLC(text->Length());
		*buf=*text;

		label=label_store->At(iCursorPos);
		text=label->Text();
		HBufC* caption=HBufC::NewLC(text->Length());
		*caption=*text;

		CAknMessageQueryDialog *note=CAknMessageQueryDialog::NewL(*buf);
		note->SetHeaderText(*caption);
		note->ExecuteLD(R_LOGVIEW_EVENT_DIALOG);
		CleanupStack::PopAndDestroy(2);

	} else if (aKeyEvent.iCode==JOY_UP) {
		if (iCursorPos>0) {
			--iCursorPos;
		} if (iCursorPos<iCurrentMin) {
			iCurrentMin--;
			iCurrentMax--;
			SizeChanged();
		}
		DrawNow();
	} else if (aKeyEvent.iCode==JOY_DOWN ) {
		if (iCursorPos < value_store->Count()-1 ) ++iCursorPos;
		if (iCursorPos > iCurrentMax) {
			iCurrentMin++;
			iCurrentMax++;
			SizeChanged();
		}
		DrawNow();
	} else {
		return EKeyWasNotConsumed;
	}
	return EKeyWasConsumed;

}

CContext_logContainer::CContext_logContainer(MApp_context& Context) : MContextBase(Context),
	iCursorPos(-1)
{

}

void CContext_logContainer::NewSensorEventL(const TTupleName& , const TDesC& , const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("NewSensorEventL"));

	const TDesC& name=aEvent.iName();
	TBuf<100> value; 
	if (aEvent.iData()) {
		const MBBData* d=aEvent.iData();
		CC_TRAPD(err, d->IntoStringL(value) );
	}

	CEikLabel* label=find_value_label(name);
	if (!label) {
		register_source(name);
		label=find_value_label(name);
		if (!label) return;
	}
	if (! name.Compare(KBase) && aEvent.iPriority()==CBBSensorEvent::VALUE) {
#ifdef __WINS__
		TBuf<20> t;
		_LIT(KFormatTxt,"%04d%02d%02dT%02d%02d%02d ");
		TDateTime dt;
		dt=aEvent.iStamp().DateTime();
		t.Format(KFormatTxt, dt.Year(), (TInt)dt.Month()+1, (TInt)dt.Day()+1,
			dt.Hour(), dt.Minute(), dt.Second());
		RDebug::Print(t); RDebug::Print(value);
#endif
		if (value.Length()==0 && !(label->Text()->Left(5).Compare(_L("last:")))) return;
	}
	label->SetTextL(value);
	DrawNow();
}

CEikLabel*	CContext_logContainer::find_value_label(const TDesC& name)
{
	TInt pos;
	TInt ret=log_labels->Find(name, pos);
	if (ret==0) 
		return value_store->At(pos);
	return 0;
}

void CContext_logContainer::ConstructL(const TRect& aRect)
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("ConstructL"));

	iMaxLines=8;
	iCurrentMin=0;
	iCurrentMax=iMaxLines-1;
	Mlogger::ConstructL(AppContextAccess());

#ifdef USE_SKIN
	TRect rect=aRect;
	rect.Move( 0, -rect.iTl.iY );
	iBackground=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain,
		rect, EFalse );
	iHighLight=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMainHigh,
		rect, EFalse );
#endif
	dense=CEikonEnv::Static()->DenseFont();
	line_height=dense->HeightInPixels();
	if (line_height>13) line_height+=dense->DescentInPixels();
	margin=line_height/3;
	width=aRect.Width();
	height=aRect.Height();
	sep=width/3;

	SubscribeL(KCellIdTuple);
	SubscribeL(KCellNameTuple);
	SubscribeL(KBaseTuple);
	SubscribeL(KCityNameTuple);
	SubscribeL(KCountryNameTuple);
	SubscribeL(KProfileTuple);
	SubscribeL(KBluetoothTuple);
	SubscribeL(KOwnBluetoothTuple);
	SubscribeL(KGpsTuple);
	SubscribeL(KLastKnownGpsTuple);
	SubscribeL(KLocaMessageStatusTuple);
	SubscribeL(KAlarmTuple);
	SubscribeL(KUnreadTuple);
	SubscribeL(KCalendarTuple);
	iDataCounters=CDataCounterReader::NewL(*this);

	CreateWindowL();
	
	label_store=new (ELeave) CArrayFixFlat<CEikLabel*>(1);
	value_store=new (ELeave) CArrayFixFlat<CEikLabel*>(1);

	log_labels=new (ELeave) CDesCArrayFlat(8);
	
	status_label=new (ELeave) CEikLabel;
	status_label->SetContainerWindowL(*this);
	status_label->SetFont(CEikonEnv::Static()->DenseFont());
	status_label->SetTextL( _L("starting") );
	status_label->SetExtent(TPoint(10, height-margin-2*line_height), status_label->MinimumSize());
	
	err_label=new (ELeave) CEikLabel;
	err_label->SetContainerWindowL(*this);
	err_label->SetFont(CEikonEnv::Static()->DenseFont());
	err_label->SetTextL( _L("    ") );
	err_label->SetExtent(TPoint(10, height-margin-line_height), err_label->MinimumSize());

	SetRect(aRect);
	ActivateL();
}

// Destructor
CContext_logContainer::~CContext_logContainer()
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("~CContext_logContainer"));

	delete iDataCounters;
#ifdef USE_SKIN
	delete iBackground;
	delete iHighLight;
#endif

	delete iLabel;
	delete status_label;
	delete err_label;
	if (label_store) {
		TInt count=label_store->Count();
		for (int i=count-1; i>=0; i--)  {
			delete (*label_store)[i];
		}
		delete label_store;
	}
	if (value_store) {
		TInt count=value_store->Count();
		for (int i=count-1; i>=0; i--)  {
			delete (*value_store)[i];
		}
		delete value_store;
	}
	delete log_labels;
}

void CContext_logContainer::set_status(const TDesC& status)
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("set_status"));

	status_label->SetTextL(status);
	status_label->SetExtent(TPoint(10, height-margin-2*line_height), status_label->MinimumSize());
	DrawNow();
}

void CContext_logContainer::set_error(const TDesC& err)
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("set_error"));

	err_label->SetTextL(err);
	err_label->SetExtent(TPoint(10, height-margin-line_height), err_label->MinimumSize());
	DrawNow();
}

// ---------------------------------------------------------
// CContext_logContainer::SizeChanged()
// Called by framework when the view size is changed
// ---------------------------------------------------------
//
void CContext_logContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("SizeChanged"));

	width=Rect().Width();
	sep=width/3;
	height=Rect().Height();
	status_label->SetExtent(TPoint(10, height-margin-2*line_height), status_label->MinimumSize());
	err_label->SetExtent(TPoint(10, height-margin-line_height), status_label->MinimumSize());

	if (!label_store) return;
	
	TInt pos_i=0;
	for (int i=0; i<label_store->Count(); i++)  {
		if (i >= iCurrentMin && i <= iCurrentMax) {
			(*label_store)[i]->MakeVisible(ETrue);
			(*value_store)[i]->MakeVisible(ETrue);
			(*label_store)[i]->SetExtent( TPoint(10, (pos_i)*line_height+margin), TSize(sep-5-10, line_height) );
			(*value_store)[i]->SetExtent( TPoint(sep+5, (pos_i)*line_height+margin), TSize(width-sep-5, line_height) );
			pos_i++;
		} else {
			(*label_store)[i]->MakeVisible(EFalse);
			(*value_store)[i]->MakeVisible(EFalse);
		}
	}	
}

// ---------------------------------------------------------
// CContext_logContainer::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CContext_logContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("CountComponentControls"));

	if (!label_store) return 2;
	
	return label_store->Count()+value_store->Count()+2; // return nbr of controls inside this container
}

// ---------------------------------------------------------
// CContext_logContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CContext_logContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("ComponentControl"));

	if (aIndex==0) {
		return status_label;
	}
	--aIndex;

	if (aIndex==0) {
		return err_label;
	} else if (label_store && aIndex>0 && aIndex <= label_store->Count()) {
		return (*label_store)[aIndex-1];
	} else if (label_store && aIndex>label_store->Count() && aIndex <= 
		label_store->Count()+value_store->Count()) {
		return (*value_store)[aIndex-label_store->Count()-1];
	} else {
		return NULL;
	}
}


// ---------------------------------------------------------
// CContext_logContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CContext_logContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
#ifndef USE_SKIN
	// TODO: Add your drawing code here
	// example code...
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbGray);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);

	if (iCursorPos!=-1) {
		TRect r(TPoint(0, (iCursorPos-iCurrentMin)*line_height+margin), TSize( width, line_height));
		gc.SetBrushColor(KRgbWhite);
		gc.DrawRect(r);
	}
#else
	AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, aRect );
	if (iCursorPos!=-1) {
		TRect r(TPoint(0, (iCursorPos-iCurrentMin)*line_height+margin), TSize( width, line_height));
		TRect r2=r; r2.Intersection(aRect);
		gc.SetPenStyle(CGraphicsContext::ENullPen);
		gc.SetBrushColor(KRgbGray);
		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		gc.DrawRect(r2);
		r.Move(2, 2); r.Resize(-2, -2); r2=r;
		r2.Intersection(aRect);
		AknsDrawUtils::Background( AknsUtils::SkinInstance(), iHighLight, gc, r2 );
	}
#endif
}

// ---------------------------------------------------------
// CContext_logContainer::HandleControlEventL(
//     CCoeControl* aControl,TCoeEvent aEventType)
// ---------------------------------------------------------
//
void CContext_logContainer::HandleControlEventL(
						CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	CALLSTACKITEM_N(_CL("CContext_logContainer"), _CL("HandleControlEventL"));

	// TODO: Add your control event handler code here
}

_LIT(KReadCounter, "recv kB");
_LIT(KWriteCounter, "sent kB");

void CContext_logContainer::CountersChanged(TInt aReadCounter, TInt aWriteCounter)
{
	TBuf<20> value;
	TRealFormat f; f.iTriLen=0;
	f.iPoint='.'; f.iPlaces=2;
	f.iType=KRealFormatFixed;

	TReal val(aReadCounter);
	val/=1024.0;
	value.Num( val, f);
	CEikLabel* label=find_value_label(KReadCounter);
	if (!label) {
		register_source(KReadCounter);
		label=find_value_label(KReadCounter);
		if (!label) return;
	}
	label->SetTextL(value);
	value.Zero();
	val = aWriteCounter;
	val/=1024.0;
	value.Num( val, f);
	label=find_value_label(KWriteCounter);
	if (!label) {
		register_source(KWriteCounter);
		label=find_value_label(KWriteCounter);
		if (!label) return;
	}
	label->SetTextL(value);
}

_LIT(CLASS_NAME, "CContext_logContainer");

// End of File  
