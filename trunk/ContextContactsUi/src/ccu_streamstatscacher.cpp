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

#include "ccu_streamstatscacher.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"
#include "ccu_storage.h"

#include "jabberdata.h"
#include "phonebook.h"


class CStreamStatsCacherImpl : public CStreamStatsCacher, public MContextBase, public MFeedNotify
{
public:	
	TInt UnreadCountL(contact& aCon)
	{
		CALLSTACKITEM_N(_CL("CStreamStatsCacherImpl"), _CL("UnreadCountL"));
		if ( aCon.has_nick )
			{
				if ( aCon.iUnreadCount == KErrNotFound )
					ReadStatsL( aCon );
				return aCon.iUnreadCount;
			}
		return 0;
	}

	TInt StreamCountL(contact& aCon)
	{
		CALLSTACKITEM_N(_CL("CStreamStatsCacherImpl"), _CL("StreamCountL"));
		if ( aCon.has_nick )
			{
				if ( aCon.iStreamCount == KErrNotFound )
					ReadStatsL( aCon );
				return aCon.iUnreadCount;
			}
		return 0;
	}
	
	
	void ReadStatsL(contact& aCon)
	{
		CALLSTACKITEM_N(_CL("CStreamStatsCacherImpl"), _CL("ReadStatsL"));
		CJabberData::TNick nick;
		if ( iJabberData.GetJabberNickL( aCon.id, nick ) )
			{
				CJabberData::TransformToUiNickL( nick );
				TInt itemCount = 0;
				TInt unreadCount = 0;
				iFeedStorage.GetCountsByAuthorL( nick, aCon.iStreamCount, aCon.iUnreadCount );
			}
	}
	
public: 		
	CStreamStatsCacherImpl(CJabberData& aJabberData, 
						   phonebook& aPhonebook, 
						   CFeedItemStorage& aFeedStorage) : iJabberData(aJabberData),
															 iPhonebook(aPhonebook),
															 iFeedStorage(aFeedStorage)
	{
	}

	~CStreamStatsCacherImpl() 
	{
		CALLSTACKITEM_N(_CL("CStreamStatsCacherImpl"), _CL("~CStreamStatsCacherImpl"));
		iFeedStorage.UnSubscribeL( this );
	}
	
	void ConstructL( )
	{
		CALLSTACKITEM_N(_CL("CStreamStatsCacherImpl"), _CL("ConstructL"));
		iFeedStorage.SubscribeL( this );
	}

	virtual void FeedItemEvent(CBBFeedItem* /*aItem*/, MFeedNotify::TEvent /*aEvent*/) 
	{
		// do nutting
	}
	
	
	virtual void AuthorCountEvent(const TDesC& aAuthor,
								  TInt aNewItemCount, TInt aNewUnreadCount) 
	{
		CALLSTACKITEM_N(_CL("CStreamStatsCacherImpl"), _CL("AuthorCountEvent"));
		TInt id = iJabberData.GetContactIdL( aAuthor );
		if ( id != KErrNotFound )
			{
				contact* c = iPhonebook.GetContactById( id );
				if ( c )
					{
						c->iStreamCount = aNewItemCount;
						c->iUnreadCount = aNewUnreadCount;
					}
			}
	}
						
private:
	CJabberData& iJabberData;
	phonebook& iPhonebook;
	CFeedItemStorage& iFeedStorage;
};


EXPORT_C CStreamStatsCacher* CStreamStatsCacher::NewL(CJabberData& aJabber, phonebook& aPhonebook, CFeedItemStorage& aFeedStorage)
{
	auto_ptr<CStreamStatsCacherImpl> self( new (ELeave) CStreamStatsCacherImpl(aJabber, aPhonebook, aFeedStorage) );
	self->ConstructL();
	return self.release();
}
