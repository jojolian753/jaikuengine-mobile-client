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

#include "hideview.h"

#include "contextlog_resource.h"
#include <aknviewappui.h>
#include <eiklabel.h>
#include "symbian_auto_ptr.h"
#include "context_log.hrh"
#include "app_context.h"

class CHideContainer : public CCoeControl {
public:
	CHideContainer(CAknViewAppUi* AppUi);
	~CHideContainer();
	void ConstructL(const TRect& aRect);

private:
        void SizeChanged();
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);

	CArrayPtrFlat< CCoeControl > *iControls;
	TInt				iPresses;
	CAknViewAppUi*			iAppUi;
};

class CHideViewImpl : public CHideView {
private:
	CHideViewImpl();
	void ConstructL();

        TUid Id() const;
	
        void HandleCommandL(TInt aCommand);
        void DoActivateL(const TVwsViewId& aPrevViewId,
		TUid aCustomMessageId,
		const TDesC8& aCustomMessage);
        void DoDeactivate();

	friend class CHideView;
	CHideContainer* iContainer;
public:
	virtual ~CHideViewImpl();
};

CHideView* CHideView::NewL()
{
	CALLSTACKITEM_N(_CL("CHideView"), _CL("NewL"));

	auto_ptr<CHideViewImpl> ret(new (ELeave) CHideViewImpl());
	ret->ConstructL();
	return ret.release();
}

CHideContainer::CHideContainer(CAknViewAppUi* AppUi) : iAppUi(AppUi)
{
}

CHideContainer::~CHideContainer()
{
	CALLSTACKITEM_N(_CL("CHideContainer"), _CL("~CHideContainer"));

	if (iControls) iControls->ResetAndDestroy();
	delete iControls;

}

TKeyResponse CHideContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CHideContainer"), _CL("OfferKeyEventL"));

	if (aType==EEventKey) {
		if (aKeyEvent.iCode=='5') {
			iPresses++;
			if (iPresses==3) {
				iAppUi->HandleCommandL(Econtext_logDetailedView);
			}
		} else {
			iPresses=0;
		}
	}
	return EKeyWasNotConsumed;
}

void CHideContainer::ConstructL(const TRect& aRect)
{
	CALLSTACKITEM_N(_CL("CHideContainer"), _CL("ConstructL"));

	iControls=new (ELeave) CArrayPtrFlat< CCoeControl >(1);
	CreateWindowL();

	TRect r(aRect);
	r.Move(10, 10); r.Resize(-20, -20);

	CEikLabel *l=new (ELeave) CEikLabel;
	iControls->AppendL(l);
	l->SetContainerWindowL( *this );
	l->SetRect(r);

	TBuf<100> msg;
	iEikonEnv->ReadResourceAsDes16(msg, R_HIDE_MESSAGE);
	l->SetTextL(msg); 

	SetRect(aRect);
	ActivateL();	
}

void CHideContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CHideContainer"), _CL("SizeChanged"));

}

TInt CHideContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CHideContainer"), _CL("CountComponentControls"));

	return iControls->Count();
}

CCoeControl* CHideContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CHideContainer"), _CL("ComponentControl"));

	return iControls->At(aIndex);
}

void CHideContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CHideContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
	// TODO: Add your drawing code here
	// example code...
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);

}

CHideViewImpl::CHideViewImpl()
{

}

void CHideViewImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CHideViewImpl"), _CL("ConstructL"));

	BaseConstructL( R_HIDE_VIEW );
}

TUid CHideViewImpl::Id() const
{
	CALLSTACKITEM_N(_CL("CHideViewImpl"), _CL("Id"));

	return KHideView;
}

void CHideViewImpl::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CHideViewImpl"), _CL("HandleCommandL"));
	AppUi()->HandleCommandL(aCommand);

}

void CHideViewImpl::DoActivateL(const TVwsViewId& /*aPrevViewId*/,
	TUid /*aCustomMessageId*/,
	const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CHideViewImpl"), _CL("DoActivateL"));

	if (!iContainer) {
		iContainer=new (ELeave) CHideContainer(AppUi());
		iContainer->SetMopParent(this);
		iContainer->ConstructL(ClientRect());
		AppUi()->AddToStackL( *this, iContainer );
        } 
}

void CHideViewImpl::DoDeactivate()
{
	CALLSTACKITEM_N(_CL("CHideViewImpl"), _CL("DoDeactivate"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
	iContainer = 0;
}

CHideViewImpl::~CHideViewImpl()
{
	CALLSTACKITEM_N(_CL("CHideViewImpl"), _CL("~CHideViewImpl"));

	delete iContainer;
}
