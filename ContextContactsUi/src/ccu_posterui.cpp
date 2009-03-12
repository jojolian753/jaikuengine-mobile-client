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

#include "ccu_posterui.h"

#include "ccu_feeditem.h"
#include "ccu_poster.h"
#include <contextcontactsui.rsg>

#include "app_context.h"
#include "break.h"
#include "cn_networkerror.h"
#include "emptytext.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#include "settings.h"
#include "cl_settings.h"


class CPosterUiImpl : public CPosterUi, public MContextBase
{
public:
	CPosterUiImpl(CPoster& aPoster) : iPoster(aPoster)
	{
	}

	~CPosterUiImpl()
	{
		CALLSTACKITEM_N(_CL("CPosterUi"), _CL("~CPosterUi"));
	}

	void ConstructL() 
	{
		CALLSTACKITEM_N(_CL("CPosterUi"), _CL("ConstructL"));
		
	}


	void PostJaikuL(MObjectProvider *aParent)
	{
		CALLSTACKITEM_N(_CL("CPosterUi"), _CL("PostJaikuL"));
		CNetworkError::ConnectionRequestedL();
		
		// -1 is special case. Somewhere in server it assumes that 255 is max length.
		const TInt m_length = BB_LONGSTRING_MAXLEN - 1;
		
		auto_ptr<HBufC> usergiven(HBufC::NewL( m_length) );
		TPtr16 p = usergiven->Des();
		Settings().GetSettingL(SETTING_OWN_DESCRIPTION, p);
		
		auto_ptr<HBufC> oldUserGiven( usergiven->AllocL() );

		auto_ptr<HBufC> pr(CEikonEnv::Static()->AllocReadResourceL(R_USER_GIVEN));
		CAknTextQueryDialog* dlg = new(ELeave) CEmptyAllowingTextQuery(p, CAknQueryDialog::ENoTone);
		CleanupStack::PushL(dlg);
		dlg->SetMopParent( aParent );
		dlg->SetPredictiveTextInputPermitted(ETrue);
		dlg->SetMaxLength( m_length );
		dlg->SetPromptL(*pr);
		CleanupStack::Pop();
		
		{
			
			dlg->PrepareLC(R_CCU_POST_JAIKU_DIALOG);
			if (dlg->RunLD() ) {
				if (usergiven->Compare( *oldUserGiven ) != 0 )
					{
						iPoster.PostJaikuL( *usergiven );
					}
			}
		}
	}



	void PostCommentL(CBBFeedItem& aItem, MObjectProvider *aParent)
	{
		CALLSTACKITEM_N(_CL("CPosterUi"), _CL("PostCommentL"));
		if ( FeedItem::IsComment(aItem) )
			PostCommentL( aItem.iParentUuid(), aParent );
		else
			PostCommentL( aItem.iUuid(), aParent );
	}

	void PostCommentL(const TGlobalId& aUid, MObjectProvider *aParent)
	{
		CALLSTACKITEM_N(_CL("CPosterUi"), _CL("PostCommentL"));
		CNetworkError::ConnectionRequestedL();
		
		const TInt m_length = BB_LONGSTRING_MAXLEN;
		auto_ptr<HBufC> comment(HBufC::NewL( m_length) );
		
		//auto_ptr<HBufC> pr(CEikonEnv::Static()->AllocReadResourceL(R_CONTACTS_SET_USER_GIVEN));
		TPtr p = comment->Des();
		
		CAknTextQueryDialog* dlg = new(ELeave) CEmptyAllowingTextQuery(p, CAknQueryDialog::ENoTone);		
		CleanupStack::PushL(dlg);
		dlg->SetMopParent(aParent);
		dlg->SetPredictiveTextInputPermitted(ETrue);
		dlg->SetMaxLength( m_length );
		dlg->SetPromptL( _L("Post comment") );
		CleanupStack::Pop();
		
		dlg->PrepareLC(R_CCU_POST_COMMENT_DIALOG);
		if (dlg->RunLD() ) 
			{
				if ( comment->Length() > 0 )
					{
						iPoster.PostCommentToItemL( aUid, *comment );
					}
			}
	}
	
private:
	CPoster& iPoster;
};


EXPORT_C CPosterUi* CPosterUi::NewL(CPoster& aPoster)
{
	CALLSTACKITEM_N(_CL("CPosterUi"), _CL("NewL"));
	auto_ptr<CPosterUiImpl> self( new (ELeave) CPosterUiImpl(aPoster) );
	self->ConstructL();
	return self.release();
}
