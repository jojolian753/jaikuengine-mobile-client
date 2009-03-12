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

#include "ccu_activeitem.h"

#include "ccu_storage.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"

class CActiveItemImpl : public CActiveItem, public MContextBase, public MFeedNotify
{
public:
	CActiveItemImpl(CFeedItemStorage& aStorage)
		: iId( KNullItemId ), iStorage( aStorage ) {}
	
	void ConstructL()
	{
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("ConstructL") );
		iStorage.SubscribeL( this );
	}
	
	virtual ~CActiveItemImpl()
	{
		ReleaseCActiveItemImplL();
	}
	
	void ReleaseCActiveItemImplL()
	{
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("ReleaseCActiveItemImplL") );
		iStorage.UnSubscribeL( this );
	}
	
protected: // Public API
	virtual void SetL(TUint aItemId)
	{
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("SetL") );
		if ( iId != aItemId )
			{
				iId = aItemId;
				NotifyObservers( MActiveItemObserver::EChanged, -1 );
			}
	}
	
	
	virtual void ClearL() 
	{
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("ClearL") );
		iId = KNullItemId;
		NotifyObservers( MActiveItemObserver::ECleared, -1 );
	}
	
	virtual TUint GetId()
	{
		return iId;
	}

	
	virtual CBBFeedItem* GetL()
	{
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("GetL") );
		if ( iId == KNullItemId ) 
			return NULL;
		else
			return iStorage.GetByLocalIdL(iId, ETrue);
	}
	
	virtual void AddObserverL( MActiveItemObserver& aObserver ) 
	{
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("AddObserverL") );
		iObservers.AppendL( &aObserver );
	}
	
	virtual void RemoveObserverL( MActiveItemObserver& aObserver )
	{		
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("RemoveObserverL") );
		TInt ix = iObservers.Find( &aObserver );
		if ( ix >= 0 )
			{
				iObservers.Remove( ix );
			}
	}
	

private: //  
	void NotifyObservers( MActiveItemObserver::TChangeType aChangeType, TInt aEvent )
	{
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("NotifyObservers") );
		for (TInt i=0; i < iObservers.Count(); i++)
			{
				iObservers[i]->ActiveItemChanged( aChangeType, aEvent );
			}
	}
	
private: // from phonebook_observer
	void FeedItemEvent(CBBFeedItem* aItem, MFeedNotify::TEvent aEvent)
	{ 
		CALLSTACKITEM_N( _CL("CActiveItemImpl"), _CL("FeedItemEvent") );
		if ( aItem && aItem->iLocalDatabaseId == iId ) 
			{
				switch ( aEvent )
					{
					case MFeedNotify::EFeedItemDeleted:
						iId = KNullItemId;
						NotifyObservers( MActiveItemObserver::EChanged, aEvent );
						break;
					default:
						NotifyObservers( MActiveItemObserver::EDataUpdated, aEvent );
						break;
					}
			}
	}

	virtual void AuthorCountEvent(const TDesC& aAuthor,
								  TInt aNewItemCount, TInt aNewUnreadCount)
	{
	}



private:
	TUint iId;
	CFeedItemStorage& iStorage;
	RPointerArray< MActiveItemObserver > iObservers;
	
};


EXPORT_C CActiveItem* CActiveItem::NewL(CFeedItemStorage& aStorage)
{
	auto_ptr<CActiveItemImpl> self( new (ELeave) CActiveItemImpl( aStorage ) );
	self->ConstructL();
	return self.release();
}
