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

#include "ccu_buddyicon.h"

#include "ccu_userpics.h"

#include "app_context.h"
#include "jabberdata.h"
#include "juik_image.h"
#include "juik_layout.h"
#include "jaiku_layoutids.hrh"



class CBuddyIconMgrImpl : public CBuddyIconMgr, public MContextBase, public MUserPicObserver
{	
public:	
	~CBuddyIconMgrImpl()
	{
		if ( iOwnsImage )
			delete iBuddyIcon;
		UserPics().RemoveObserverL( *this );
	}
	

	CBuddyIconMgrImpl(const TUiDelegates& aDelegates) : iDelegates( aDelegates ), iOwnsImage(ETrue) {}
	
	CJuikImage* GetControl() 
	{ 
		iOwnsImage = EFalse; 
		return iBuddyIcon; 
	}

	void ConstructL(const TDesC& aNick)
	{
		CALLSTACKITEM_N(_CL("CBuddyIconMgrImpl"), _CL("ConstructL"));
		iNick = aNick;
		TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__buddy_icon);			
		TSize sz = lay.Size();
		iBuddyIcon = CJuikImage::NewL( NULL, sz);
		iBuddyIcon->iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, 
													  LI_feed_controls_margins__author_header_buddy).Margins();
		
		UpdateBuddyPicL();
		UserPics().AddObserverL( *this );
	}
	
	void SetNickL(const TDesC& aNick)
	{
		iNick = aNick;
	}
	
	CUserPics& UserPics() { return *(iDelegates.iUserPics); }
	void UpdateBuddyPicL()
	{
		CALLSTACKITEM_N(_CL("CBuddyIconMgrImpl"), _CL("UpdateBuddyPicL"));
		CGulIcon* userPic = UserPics().GetIconL( iNick );
		if ( userPic )
			{
				iBuddyIcon->UpdateL(*userPic);
			}
		else
			{
				TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__buddy_icon);
				TSize sz = lay.Size();
				iBuddyIcon->UpdateL( UserPics().DummyIconL( sz ) );
			}
	}
	
	
	virtual void UserPicChangedL( const TDesC& aNick, TBool aNew ) 
	{
		CALLSTACKITEM_N(_CL("CBuddyIconMgrImpl"), _CL("UserPicChangedL"));
		if ( CJabberData::EqualNicksL( aNick, iNick ) )
			{
				UpdateBuddyPicL();
			}
	}

private:
	TUiDelegates iDelegates;
	TBool iOwnsImage;
	CJabberData::TNick iNick;
	CJuikImage* iBuddyIcon;
};

EXPORT_C CBuddyIconMgr* CBuddyIconMgr::NewL(const TDesC& aNick, const TUiDelegates& aDelegates)
{
	CALLSTACKITEMSTATIC_N(_CL("CBuddyIconMgr"), _CL("NewL"));
	auto_ptr<CBuddyIconMgrImpl> self( new (ELeave) CBuddyIconMgrImpl(aDelegates) );
	self->ConstructL( aNick );
 	return self.release();
}
