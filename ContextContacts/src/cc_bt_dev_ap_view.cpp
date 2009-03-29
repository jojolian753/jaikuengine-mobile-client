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

#include "cc_bt_dev_ap_view.h"

#include "app_context.h"
#include "ccu_mainbgcontainer.h"
#include "cl_settings.h"
#include "contextcontacts.hrh"
#include "break.h"
#include "symbian_auto_ptr.h"


#include <aknviewappui.h>
#include <contextcontacts.rsg>
#include <eiktxlbx.h>
#include <eikfrlb.h>
#include <aknlists.h>
#include <btmanclient.h>
#include <btextnotifiers.h>
#include <btsdp.h>
#include <bt_sock.h>

class CBTDevApContainer;

class CBTDiscoverer : public CCheckedActive, public MContextBase {
public:
	static CBTDiscoverer * NewL(MApp_context& Context, MListObserver * aObserver, CBTDeviceList* aList);
	void ConstructL(MListObserver * aObserver);
	~CBTDiscoverer();
	void SelectDevice();
	void MoveUp(TInt idx);
	void DeleteDevice(TInt idx);
	void MoveDown(TInt idx);

	void DoCancel();
	void CheckedRunL();

	MDesC16Array * GetNameArray();


private:
	CBTDiscoverer(MApp_context& Context, MListObserver * aObserver, CBTDeviceList* aList);

	void AddToArrays(const TDesC8& bt_addr, const TDesC16& bt_name);

	RNotifier iNotifier; bool not_is_open;
	TBTDeviceSelectionParamsPckg pckg;
	TBTDeviceResponseParamsPckg result;

	enum comm_state { IDLE, SELECTING};
	comm_state current_state;

	CBTDeviceList	*iList;
	MListObserver * iObserver;

#ifdef __WINS__
	int	iDevCount;
#endif
};

class CBTDevApContainer : public CCoeControl, MCoeControlObserver, public MContextBase, public MListObserver {
public:
	CBTDevApContainer(MApp_context& Context, TBool aShowMove);
	~CBTDevApContainer();
	void ConstructL(CCoeControl& aParent, CBTDeviceList* aList);
	TInt GetCurrentIndex();
	void ContentsChanged();
private:
	void SizeChanged();
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void Draw(const TRect& aRect) const;
	void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
private:
	CEikTextListBox* iListBox;
public:
	CBTDiscoverer * iDiscoverer;
	TBool iShowMove;
};

class CBTDevApViewImpl : public CBTDevApView  {
private:

	CBTDevApViewImpl(MApp_context& Context, CBTDeviceList* aList, TBool aShowMove,
		TUid aViewId, TVwsViewId* aNextViewId);
	void ConstructL();
	void SetList(CBTDeviceList* aList);

	TUid Id() const;

	void HandleCommandL(TInt aCommand);
  void HandleResourceChange( TInt aType );

private: // from CJaikuViewBase 
	void RealDoActivateL(const TVwsViewId& aPrevViewId,
						 TUid aCustomMessageId,
						 const TDesC8& aCustomMessage);
	void RealDoDeactivateL();
	void ReleaseViewImpl();
	
private:
	friend class CBTDevApView;
	CBTDevApContainer* iContainer;
	CBTDeviceList* iList;
	CMainBgContainer* iBgContainer;


	void RemoveContainerL();
	void CreateContainerL();
	
public:
	virtual ~CBTDevApViewImpl();
	TBool iShowMove;
	TUid iViewId; 
	TVwsViewId* iNextViewId;
	TVwsViewId iPreviousViewId;

};

//---------------------------------------------------------------------------
CBTDiscoverer * CBTDiscoverer::NewL(MApp_context& Context, MListObserver * aObserver, CBTDeviceList* aList)
{
	CALLSTACKITEM2_N(_CL("CBTDiscoverer"), _CL("NewL"), &Context);

	auto_ptr<CBTDiscoverer> ret(new (ELeave) CBTDiscoverer(Context, aObserver, aList));
	ret->ConstructL(aObserver);
	return ret.release();
}


void CBTDiscoverer::ConstructL(MListObserver * aObserver)
{
	CALLSTACKITEM_N(_CL("CBTDiscoverer"), _CL("ConstructL"));

	iList->AddObserver(aObserver);

	current_state=IDLE;
	CActiveScheduler::Add(this);
}

MDesC16Array * CBTDiscoverer::GetNameArray()
{
	return iList->NameArray();
}

void CBTDiscoverer::DeleteDevice(TInt idx)
{
	if (iList->NameArray()->MdcaCount()>0) 
		iList->RemoveDeviceL(idx);
}

void CBTDiscoverer::MoveUp(TInt idx)
{
	iList->MoveUpL(idx);
}

void CBTDiscoverer::MoveDown(TInt idx)
{
	iList->MoveDownL(idx);
}

CBTDiscoverer::CBTDiscoverer(MApp_context& Context, MListObserver * aObserver, CBTDeviceList* aList) : 
CCheckedActive(EPriorityNormal, _L("CBTDiscoverer")), MContextBase(Context), 
iList(aList),
iObserver(aObserver) { }

CBTDiscoverer::~CBTDiscoverer()
{
	CALLSTACKITEM_N(_CL("CBTDiscoverer"), _CL("~CBTDiscoverer"));

	Cancel();
	if (iList && iObserver) iList->RemoveObserver(iObserver);
}

void CBTDiscoverer::SelectDevice()
{
	CALLSTACKITEM_N(_CL("CBTDiscoverer"), _CL("SelectDevice"));

#ifdef __WINS__

	TBuf8<10> addr=_L8("12345");
	if (iDevCount==0) {
		unsigned char a[]={ 0x00, 0x02, 0xee, 0x51, 0xc4, 0x37};
		TPtrC8 ap(&a[0], 6);
		addr=ap;
	} else {
		addr.AppendNum(iDevCount);
	}

	TBuf<20> name=_L("test name ");

	name.AppendNum(iDevCount);
	AddToArrays(addr, name);
	++iDevCount;
	return;
#endif

	TInt ret;
	if ( (ret=iNotifier.Connect())!= KErrNone ) return;

	not_is_open=true;

	result()=TBTDeviceResponseParams();
	pckg()=TBTDeviceSelectionParams();
	iNotifier.StartNotifierAndGetResponse(iStatus, KDeviceSelectionNotifierUid, pckg, result);

	SetActive();	
	current_state=SELECTING;
}

void CBTDiscoverer::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBTDiscoverer"), _CL("CheckedRunL"));

	if (iStatus.Int() != 0)
	{
		if (not_is_open) iNotifier.Close(); not_is_open=false;
		current_state=IDLE;
		return;
	}

	switch(current_state) {
case SELECTING:
	if(!result().IsValidBDAddr()) {
		current_state=IDLE;
	} else {
		AddToArrays(result().BDAddr().Des(), result().DeviceName());
		current_state=IDLE;
	}
	if (not_is_open) iNotifier.Close(); not_is_open=false;
	break;
case IDLE:
	break;
	}
}

void CBTDiscoverer::AddToArrays(const TDesC8& bt_addr, const TDesC16& bt_name)
{
	iList->AddDeviceL(bt_name, bt_addr);
}

void CBTDiscoverer::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBTDiscoverer"), _CL("DoCancel"));

	switch(current_state) {
case SELECTING:
	iNotifier.CancelNotifier(KDeviceSelectionNotifierUid);
	break;
default:
	break;
	}
	current_state=IDLE;
}

CBTDevApView* CBTDevApView::NewL(MApp_context& Context, 
				 CBTDeviceList* aList, TUid aViewId, 
				 TVwsViewId* aNextViewId, TBool aShowMove)
{
	CALLSTACKITEMSTATIC_N(_CL("CBTDevApView"), _CL("NewL"));

	auto_ptr<CBTDevApViewImpl> ret(new (ELeave) CBTDevApViewImpl(Context, aList, aShowMove, aViewId, aNextViewId));
	ret->ConstructL();
	return ret.release();
}

CBTDevApContainer::CBTDevApContainer(MApp_context& Context, TBool aShowMove) : 
MContextBase(Context), iShowMove(aShowMove)
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("CBTDevApContainer"));

}

CBTDevApContainer::~CBTDevApContainer()
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("~CBTDevApContainer"));

	delete iListBox;
	delete iDiscoverer;
}

void CBTDevApContainer::ContentsChanged()
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("ContentsChanged"));

	CC_TRAPD(err,	
		if (iListBox) iListBox->HandleItemRemovalL();
	iListBox->SetCurrentItemIndexAndDraw(0);
	iListBox->DrawNow(););

}


TKeyResponse CBTDevApContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("OfferKeyEventL"));
	return iListBox->OfferKeyEventL(aKeyEvent, aType);
}

TInt CBTDevApContainer::GetCurrentIndex()
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("GetCurrentIndex"));

	TInt idx = iListBox->View()->CurrentItemIndex();

	return idx;
}

void CBTDevApContainer::ConstructL(CCoeControl& aParent, CBTDeviceList* aList)
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("ConstructL"));
	
	iDiscoverer = CBTDiscoverer::NewL(AppContext(), this, aList);
	
	//CreateWindowL(); 
	SetContainerWindowL( aParent );
	iListBox = new (ELeave) CEikTextListBox; //CAknSingleStyleListBox; //CAknSingleNumberStyleListBox; 
	iListBox->SetMopParent(this);
	iListBox->ConstructL(this, EAknListBoxSelectionList);

	iListBox->View()->SetMatcherCursor(EFalse);

	iListBox->Model()->SetItemTextArray(iDiscoverer->GetNameArray());
	iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);

	iListBox->MakeVisible(ETrue);

	iListBox->ActivateL();
	iListBox->DrawNow();

	//SetRect(aRect);
	ActivateL();
}

void CBTDevApContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("SizeChanged"));
	iListBox->SetRect(Rect());
}

TInt CBTDevApContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("CountComponentControls"));

	return 1;
}

void CBTDevApContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
}

CCoeControl* CBTDevApContainer::ComponentControl(TInt aIndex) const
{
	if (aIndex==0) return iListBox;
	return 0;
}

void CBTDevApContainer::HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType)
{
	CALLSTACKITEM_N(_CL("CBTDevApContainer"), _CL("HandleControlEventL"));
	// TODO: Add your control event handler code here
}


CBTDevApViewImpl::CBTDevApViewImpl(MApp_context& Context, 
				   CBTDeviceList* aList, TBool aShowMove,
				   TUid aViewId, TVwsViewId* aNextViewId) : 
	iList(aList), iShowMove(aShowMove),
	iViewId(aViewId), iNextViewId(aNextViewId) { }

void CBTDevApViewImpl::SetList(CBTDeviceList* aList)
{
	iList=aList;
}

void CBTDevApViewImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBTDevApViewImpl"), _CL("ConstructL"));

	if (iShowMove) 
		User::Leave(KErrNotSupported);
	//BaseConstructL( R_BT_DEV_AP_VIEW );
	else
		BaseConstructL( R_BT_DEV_SET_VIEW );
}

TUid CBTDevApViewImpl::Id() const {
	return iViewId;
}

void CBTDevApViewImpl::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CBTDevApViewImpl"), _CL("HandleCommandL"));

	switch(aCommand) {
case EContextContactsAddDevice:
	iContainer->iDiscoverer->SelectDevice();
	break;
case EContextContactsDeleteDevice:
	iContainer->iDiscoverer->DeleteDevice(iContainer->GetCurrentIndex());
	break;
case EAknSoftkeyBack:
	ActivateParentViewL();
	break;
default:
	break;
	}
}

void CBTDevApViewImpl::HandleResourceChange( TInt aType ) {
  if ( aType == KEikDynamicLayoutVariantSwitch ) {
    if ( iContainer ) {
      TRect r = ClientRect();
      iBgContainer->SetRect( r );
    }
  }
}

#include "ccu_utils.h"

void CBTDevApViewImpl::RealDoActivateL(const TVwsViewId& aPrevViewId,
				   TUid /*aCustomMessageId*/,
				   const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CBTDevApViewImpl"), _CL("DoActivateL"));

// 	iPreviousViewId=aPrevViewId;
	CreateContainerL();
	StatusPaneUtils::SetContextPaneIconToDefaultL();
	StatusPaneUtils::SetTitlePaneTextToDefaultL();
}

void CBTDevApViewImpl::RealDoDeactivateL()
{
	CALLSTACKITEM_N(_CL("CBTDevApViewImpl"), _CL("RealDoDeactivateL"));
	RemoveContainerL();
}

void CBTDevApViewImpl::CreateContainerL()
{
	CALLSTACKITEM_N(_CL("CBTDevApViewImpl"), _CL("CreateContainerL"));
	RemoveContainerL();

	if (!iBgContainer )
		{
			iBgContainer = CMainBgContainer::NewL( this, ClientRect(), ThemeColors(), ProgressBarModel() );
		}

	if (!iContainer) {
		iContainer=new (ELeave) CBTDevApContainer(AppContext(), iShowMove);
		iContainer->SetMopParent(iBgContainer);
		iContainer->ConstructL(*iBgContainer, iList);
		iBgContainer->SetContentL( iContainer );
	} 
	
	iBgContainer->ActivateL();
	AppUi()->AddToStackL( *this, iBgContainer );
}

void CBTDevApViewImpl::RemoveContainerL()
{
	CALLSTACKITEM_N(_CL("CBTDevApViewImpl"), _CL("RemoveContainerL"));
	if ( iBgContainer )
		{
			AppUi()->RemoveFromStack( iBgContainer );
			delete iBgContainer;
			iBgContainer = NULL;
		}
	delete iContainer;
	iContainer = 0;
}


CBTDevApViewImpl::~CBTDevApViewImpl() {
	CC_TRAPD(err, ReleaseViewImpl());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

void CBTDevApViewImpl::ReleaseViewImpl()
{
	CALLSTACKITEM_N(_CL("CBTDevApViewImpl"), _CL("~CBTDevApViewImpl"));
	RemoveContainerL();

}

