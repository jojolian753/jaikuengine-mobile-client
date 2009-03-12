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

#include "codeprompt.h"

#include "checkedactive.h"
#include <recognizerview.h>
#include "symbian_auto_ptr.h"
#include "cm_post.h"
#include "reporting.h"

class CVisualCodePromptImpl : public CVisualCodePrompt, 
	public MRecognizerCallback, public MContextBase {
public:
	virtual ~CVisualCodePromptImpl();
private:
	CVisualCodePromptImpl(MApp_context& Context, 
		CAknViewAppUi *ViewAppUi, TUid LocalDefaultView, TVwsViewId* NextViewId);
	void ConstructL(TUid aViewId);

	virtual void Prompt(const TDesC& FileName, MUploadCallBack* CallBack);

	virtual void CodeSelected(const CCodeInfo& aCode);
	virtual void Cancelled();

	void Async();

	void CheckedRunL();
	void DoCancel();

	CRecognizerView*	iView;
	MUploadCallBack*	iCallBack;
	CAknViewAppUi		*iViewAppUi;
	TInt64			iCode;
	TBool			iGotCode;
	TUid			iDefaultView;
	TVwsViewId* iNextViewId;

	friend class CVisualCodePrompt;
};

EXPORT_C CVisualCodePrompt* CVisualCodePrompt::NewL(MApp_context& Context, 
					   CAknViewAppUi *ViewAppUi, TUid aViewId, 
					   TUid LocalDefaultView, TVwsViewId* NextViewId)
{
	CALLSTACKITEM_N(_CL("CVisualCodePrompt"), _CL("NewL"));

	auto_ptr<CVisualCodePromptImpl> ret(new (ELeave) 
		CVisualCodePromptImpl(Context, ViewAppUi, LocalDefaultView, NextViewId));
	ret->ConstructL(aViewId);
	return ret.release();
}

CVisualCodePrompt::CVisualCodePrompt() : CCheckedActive(EPriorityIdle, _L("CVisualCodePromptImpl")) { }
CVisualCodePromptImpl::CVisualCodePromptImpl(MApp_context& Context,
					     CAknViewAppUi *ViewAppUi, 
					     TUid LocalDefaultView, TVwsViewId* NextViewId) : 
					MContextBase(Context),
					iViewAppUi(ViewAppUi), iDefaultView(LocalDefaultView), 
					iNextViewId(NextViewId)
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("CVisualCodePromptImpl"));

}

CVisualCodePromptImpl::~CVisualCodePromptImpl()
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("~CVisualCodePromptImpl"));

}

void CVisualCodePromptImpl::ConstructL(TUid aViewId)
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("ConstructL"));

//#ifdef USE_VISUALCODES
	iView=CRecognizerView::NewL(AppContext(), this, aViewId, iDefaultView, iNextViewId);
//#endif
	iViewAppUi->AddViewL(iView);

	CActiveScheduler::Add(this);
}

void CVisualCodePromptImpl::Prompt(const TDesC& /*FileName*/, MUploadCallBack* CallBack)
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("Prompt"));
	Reporting().DebugLog(_L("code:Prompt"));

	iCallBack=CallBack;
	iViewAppUi->ActivateLocalViewL(iView->Id());
}

void CVisualCodePromptImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("CheckedRunL"));
	Reporting().DebugLog(_L("code:CheckedRunL"));

	if (!iCallBack) return;

	MUploadCallBack* cb=iCallBack;
	iCallBack=0;
	if (!iGotCode) {
		cb->Back(false, false, 0);
	} else {

		auto_ptr<CCMPost> buf(CCMPost::NewL(0));
		buf->iParentId()=iCode;
		cb->Back(true, true, buf.get());
	}
}

void CVisualCodePromptImpl::CodeSelected(const CCodeInfo& aCode)
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("CodeSelected"));
	Reporting().DebugLog(_L("CodeSelected"));


	iGotCode=ETrue;
	iCode=aCode.code->ToInt64();
	
	Async();
}

void CVisualCodePromptImpl::Cancelled()
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("Cancelled"));

	iGotCode=EFalse;
	Async();
}

void CVisualCodePromptImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("DoCancel"));

	return;
}

void CVisualCodePromptImpl::Async()
{
	CALLSTACKITEM_N(_CL("CVisualCodePromptImpl"), _CL("Async"));

	if (IsActive()) return;

	TRequestStatus *s=&iStatus;
	User::RequestComplete(s, KErrNone);
	SetActive();
}
