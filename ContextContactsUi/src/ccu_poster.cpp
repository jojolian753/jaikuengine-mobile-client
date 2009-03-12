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

#include "ccu_poster.h"

#include "cb_presence.h"
#include "ccu_feeditem.h"
#include "ccu_storage.h"
#include "PresenceTextFormatter.h"

#include "app_context.h"
#include "cbbsession.h"
#include "cc_uuid.h"
#include "cl_settings.h"
#include "contextcommon.h"
#include "settings.h"
#include "csd_feeditem.h"
#include "jabberdata.h"
#include "cl_settings.h"

const TInt KUuidGeneratorId(3); // see ../ids.txt

class CPosterImpl : public CPoster, public MContextBase, public MSettingListener
{
public:

	virtual void PostJaikuL(const TDesC& aContent)
	{
		CALLSTACKITEM_N(_CL("CPosterImpl"), _CL("PostJaikuL"));


 		refcounted_ptr<CBBFeedItem> item( new (ELeave) CBBFeedItem );
		item->iContent.Append( aContent );
		item->iKind.iValue = _L("presence");
		FillLocationL( *item );
		FillAndPostL( item.get() );
		
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION_TIME, GetTime());
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION, aContent);

	}

	virtual void PostCommentToRootItemL(CBBFeedItem& aItem, const TDesC& aContent)
	{
		CALLSTACKITEM_N(_CL("CPosterImpl"), _CL("PostCommentToRootItemL"));
		TGlobalId rootId;
		FeedItem::FindRootParentL( aItem, rootId, iStorage);
		PostCommentToItemL( rootId, aContent );
	}

	
	virtual void PostCommentToItemL(const TGlobalId& aParent, const TDesC& aContent)
	{
		CALLSTACKITEM_N(_CL("CPosterImpl"), _CL("PostCommentToItemL"));

		refcounted_ptr<CBBFeedItem> item( new (ELeave) CBBFeedItem );

		refcounted_ptr<CBBFeedItem> parent = iStorage.GetByGlobalIdL( aParent, ETrue );
		if ( parent.get() )
			{
				item->iParentAuthorNick = parent->iAuthorNick();
				TInt max =item->iParentTitle().MaxLength();
				item->iParentTitle = parent->iContent().Left( max );
			}
		
 		item->iParentUuid.iValue = aParent;
		item->iContent.Append( aContent );
		item->iKind.iValue = KNullDesC;
		FillAndPostL( item.get() );
	}
	
	void FillAndPostL( CBBFeedItem* aItem )
	{
		CALLSTACKITEM_N(_CL("CPosterImpl"), _CL("FillAndPostL"));
		TGlobalId guid;
		iUuidGenerator->MakeUuidL( guid );
 		aItem->iUuid.iValue = guid;
		aItem->iAuthorNick = iUserNick;
		
 		TTime now = GetTime();
		aItem->iCreated = now;


		// Put comment to local database
		iStorage.AddLocalL( aItem );
		
		// Put comment to blackboard
 		TTime expires = now; expires+=TTimeIntervalDays(7);
		BBSession()->PutRequestL( KOutgoingFeedItem, KNullDesC, aItem, expires, KOutgoingTuples );		
	}
		
	void FillLocationL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CPosterImpl"), _CL("FillLocationL"));

		TInt userId = iJabberData.GetContactIdL( iUserNick );
		const CBBPresence* p = iPresenceHolder.GetPresence( userId );
		iFormatter->LocationL(p, aItem.iLocation.iValue) ; 
	}
	

	CPosterImpl( CJabberData& aJabberData, CFeedItemStorage& aStorage,
				 CPresenceHolder& aPresenceHolder ) : 
		iJabberData( aJabberData ), iStorage( aStorage ), 
		iPresenceHolder(aPresenceHolder)
	{}
	
	
	~CPosterImpl() 
	{
		delete iUuidGenerator;
		delete iFormatter;
	}

	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CPosterImpl"), _CL("ConstructL"));
		iFormatter = CPresenceTextFormatter::NewL();
		iUuidGenerator = CUuidGenerator::NewL( iJabberData.UserNickL(), KUidContextContactsUi, KUuidGeneratorId );
		ReadUserNickL();
		Settings().NotifyOnChange( SETTING_JABBER_NICK, this );
	}

	
	void ReadUserNickL()
	{
		CALLSTACKITEM_N(_CL("CPosterImpl"), _CL("ReadUserNickL"));
		Settings().GetSettingL( SETTING_JABBER_NICK, iUserNick);
		TInt at = iUserNick.Locate('@');
		if (at >= 0) iUserNick = iUserNick.Left(at);
	}
	

	void SettingChanged(TInt aSetting)
	{
		if ( aSetting == SETTING_JABBER_NICK )
			{
				ReadUserNickL();
			}
	}

	
private:
	CJabberData::TNick iUserNick;
	CJabberData& iJabberData;

	CUuidGenerator* iUuidGenerator;
	CFeedItemStorage& iStorage;

	CPresenceHolder& iPresenceHolder;
	CPresenceTextFormatter* iFormatter;
};


EXPORT_C CPoster* CPoster::NewL( CJabberData& aJabberData, CFeedItemStorage& aStorage, CPresenceHolder& aPresenceHolder )
{
	CALLSTACKITEM_N(_CL("CPoster"), _CL("NewL"));
	auto_ptr<CPosterImpl> self( new (ELeave) CPosterImpl( aJabberData, aStorage, aPresenceHolder ) );
	self->ConstructL();
	return self.release();
}
