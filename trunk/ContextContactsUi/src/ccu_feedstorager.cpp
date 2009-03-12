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

#include "ccu_feedstorager.h"

#include "ccu_storage.h"
#include "ccu_feeditem.h"

#include "app_context_impl.h"
#include "break.h"
#include "callstack.h"
#include "cbbsession.h"
#include "cl_settings.h"
#include "contextvariant.hrh"
#include "csd_feeditem.h"
#include "csd_clienttuples.h"
#include "errorhandling.h"
#include "jabberdata.h"
#include "juik_layout.h"
#include "reporting.h"
#include "settings.h"


#include <eikenv.h>
#include <COEAUI.H>

_LIT(KComponentFunctionality, "Stores posts and comments from server to local storage");

const TInt KMaxProcessInOneRun(1);


CFeedStorager::CFeedStorager() : CComponentBase( CActive::EPriorityIdle, KFeedStoragerName ) {}


class CFeedStoragerImpl : public CFeedStorager, public MBBObserver, public MContextBase, public MSettingListener
{
public: 
	CFeedStoragerImpl(CFeedItemStorage& aStorage, CJabberData& aJabberData)
		: CFeedStorager(), iStorage(aStorage), iJabberData( aJabberData )
	{
	}

	void InnerConstructL()
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("InnerConstructL"));
#ifndef __WINS__
		iMaxQueue=30;
#else
		iMaxQueue=3;
#endif

		Settings().NotifyOnChange(SETTING_CONNECTIVITY_MODEL, this);
		Settings().NotifyOnChange(SETTING_DOWNLOAD_IMAGES_YESNO, this);
		Settings().NotifyOnChange(SETTING_PRESENCE_ENABLE, this);
		UpdateDownloadModeL();
	}

	/* 
	 * We shouldn't read arbitrarily many items into the queueu
	 * since they all use memory. To suspend reading we delete
	 * the bbsession (the alternative would be to use RBBClient 
	 * directly - but that's too much work :-)
	 */
	void CreateSessionL() {
		ReleaseSession();
		iOwnMainSession=CBBSession::NewL(AppContext(), BBDataFactory());
		iBBSession=iOwnMainSession->CreateSubSessionL(this);
		iBBSession->AddNotificationL(KFeedItemTuple, ETrue);
		iBBSession->AddNotificationL(KInternalFeedItem, ETrue);
	}

	void ReleaseSession() {
		delete iBBSession; iBBSession=0;
		delete iOwnMainSession; iOwnMainSession=0;
	}

	void StartL()
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("StartL"));
		{ // Start listening feed items in blackboard
			if (iBBSession) Bug(_L("Already started")).ErrorCode(MakeErrorCode(0,KErrInUse)).Raise();
			
			CreateSessionL();
			User::LeaveIfError(iDeleteSession.Connect());
		}
	}


	void StopL()
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("StopL"));
		Cancel();
		ReleaseSession();
		iDeleteSession.Close();
		CleanQueue();
	}


	~CFeedStoragerImpl()
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("~CFeedStoragerImpl"));
		StopL();
		Settings().CancelNotifyOnChange(SETTING_CONNECTIVITY_MODEL, this);
		Settings().CancelNotifyOnChange(SETTING_DOWNLOAD_IMAGES_YESNO, this);
		Settings().CancelNotifyOnChange(SETTING_PRESENCE_ENABLE, this);
		iQueue.Close();
	}

	virtual TInt MaxErrorCount() 
	{
		return 3;
	}

private: // from MBBObserver
	

	virtual void NewValueL(TUint aId, 
					  const TTupleName& aName, const TDesC& aSubName, 
					  const TComponentName& aComponentName, 
					  const MBBData* aData)
	{
		// Clones feed item and pushes it to internal queue
		// Actual processing is done in ComponentRunL, which is initiated
		// by self-completing in Async
		// 
		// This ensures that crashes and problems caused by processing
		// is registered by component management, and this component
		// can be automatically shutdown, if it's malfunctioning.
		
		CallbackEnterComponent();
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("NewValueL"));
		
		if ( ! (aName == KFeedItemTuple || aName == KInternalFeedItem ) ) 
			{ CallbackExitComponent(); return; }
		
		{
			
			Reporting().DebugLog( _L("Got a feed item "), aId );
		}
		
		const CBBFeedItem* orig = bb_cast<CBBFeedItem>(aData);
		if ( !orig  ) 
			{ 
				Reporting().DebugLog( _L("casting failed "), aId );
				iBBSession->DeleteL(aId, ETrue);
				CallbackExitComponent(); 
				return; 
			}
		
		
		refcounted_ptr<CBBFeedItem> clone( bb_cast<CBBFeedItem>( orig->CloneL(KNullDesC ) ) );
		// No need to check success, contract mandates that casting always succeeds after cloning
		
		TQueueEntry e;
		e.iBlackBoardId = aId;
		e.iFeedItem = clone.get();
		e.iFromServer = (aName==KFeedItemTuple);
		
		iQueue.AppendL( e );
		clone->AddRef();

		if (iQueue.Count()>iMaxQueue) {
			ReleaseSession();
		}
		Async();
		
		CallbackExitComponent();
	}
	

	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName)
	{
		CallbackEnterComponent();
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("DeletedL"));

		// we don't care about deletions, because we do them ourself 

		CallbackExitComponent();
	}

private: // from CComponentBaseApi 
	void InnerRunL()
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("InnerRunL"));

		if (CEikonEnv::Static()->AppUi()->IsDisplayingDialog()) {
			Async();
			return;
		}
		if (iStorage.CompactIfHighWaterL()) {
			Async();
			return;
		}
		TInt processedCount = 0;
		while ( iQueue.Count() && processedCount < KMaxProcessInOneRun )
			{				
				
				const TInt first = 0;
				TQueueEntry qEntry = iQueue[first];

				{
					Reporting().DebugLog( _L("Processing item"), qEntry.iBlackBoardId );
				}

				refcounted_ptr<CBBFeedItem> item( qEntry.iFeedItem );
				item->AddRef(); // reference for iQueue and refcounted_ptr

				TweakFlickrUrlL( *item );

				refcounted_ptr<CBBFeedItem> old( iStorage.GetByGlobalIdL(item->iUuid(), ETrue) );
				item->iFromServer = qEntry.iFromServer;
				
				if ( ! old.get() )
					{
						TGlobalId group_parent;
						if (item->iParentUuid().Length()==0 && item->iCorrelation().Length()>0) {
							if (iStorage.GetFirstByCorrelationL(item->iCorrelation(), group_parent)) {
								item->iIsGroupChild()=ETrue;
								item->iParentUuid()=group_parent;
							}
						}
// #ifdef __DEV__
// 						Reporting().DebugLog( _L("Feed item add"));
// 						TBuf<40> b;
// 						item->iUuid.IntoStringL( b );
// 						Reporting().DebugLog( b );
// #endif // __DEV__			
						
						TBool isMe = IsMeL( item->iAuthorNick() );
						item->iIsUnread.iValue = ! isMe;
						item->iDontShowInOverView = ! ShowInOverViewL(*item);

						iStorage.AddLocalL(item.get());

						if ( iStorage.DownloadMode() == CFeedItemStorage::EAutomaticDL )
							{
								if ( item->iMediaDownloadState() == CBBFeedItem::ENotDownloading )
									iStorage.DownloadMediaForFeedItemL( item.get() );
							}
					}
				else
					{
// #ifdef __DEV__
// 						Reporting().DebugLog( _L("Feed item update. Db id="), old->iLocalDatabaseId );
// 						TBuf<40> b;
// 						b.Append( _L("\t") );
// 						item->iUuid.IntoStringL( b );
// 						Reporting().DebugLog( b );
// #endif // __DEV__						
						// Preserve unread status for existing items
						item->iIsUnread.iValue = old->iIsUnread();
						item->iDontShowInOverView = ! ShowInOverViewL(*item);
						item->iMediaFileName() = old->iMediaFileName();
						item->iIsGroupChild() = old->iIsGroupChild();
						item->iParentUuid() = old->iParentUuid();
						item->iMediaDownloadState() = old->iMediaDownloadState();

						*old = *item;
						iStorage.UpdateFeedItemL( old.get() );
					}

				iQueue.Remove(first);
				item->Release(); // reference from iQueue removed
				
				if (KMaxProcessInOneRun>1) {
					iBBSession->DeleteL( qEntry.iBlackBoardId, ETrue);
				} else {
					iDeleteSession.Delete(qEntry.iBlackBoardId, iStatus);
					SetActive();
					return;
				}

				processedCount++;
			}
		if ( iQueue.Count() > 0 )
			Async();
			
		if ( ! iBBSession && iQueue.Count()==0) CreateSessionL();
	}

	virtual void ComponentRunL() 
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("ComponentRunL"));
		CC_TRAPD( err, InnerRunL() ); 
		if ( err != KErrNone )
			{
				ReportErrorL( err, _L("ComponentRunL leaved") );		
				User::Leave(err);
			}
	}

	void ReportErrorL(TInt aErrorCode, const TDesC& aMsg)
	{
		Reporting().UserErrorLog( Name(), aMsg );
		Reporting().UserErrorLog( Name(), aErrorCode );
		auto_ptr<HBufC> stack( CallStackMgr().GetFormattedCallStack( Name() ) );
		if (stack.get())
			Reporting().UserErrorLog( *stack );
	}
	
	
	virtual void ComponentCancel() 
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("ComponentCancel"));
		
		// we could cancel the delete, but it will most likely
		// be no faster than waiting for it to complete
		// iDeleteSession.CancelOther();
	}



	virtual void ComponentId(TUid& aComponentUid, 
							 TInt& aComponentId,
							 TInt& aVersion)
	{
		aComponentUid=KUidContextContactsUi;
		aComponentId=1; // see ../ids.txt
		aVersion=3;
	}
	
	virtual const TDesC& Name()
	{
		return KFeedStoragerName;
	}

	virtual const TDesC& HumanReadableFunctionality() 
	{
		return KComponentFunctionality;
	}

private: 
	void CallbackEnterComponent()
	{
		TUid uid; TInt id; TInt version;
		ComponentId(uid, id, version);
		GetContext()->SetCurrentComponent(uid, id);
	}

	void CallbackExitComponent()
	{
		GetContext()->SetCurrentComponent(TUid::Uid(0), 0);
	}

	void Async()
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("Async"));
		if (IsActive()) return;
		TRequestStatus *s=&iStatus;
		User::RequestComplete(s, KErrNone);
		SetActive();
	}

	
	void CleanQueue()
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("CleanQueue"));
		for (TInt i=0; i < iQueue.Count(); i++)
			{
				iQueue[i].iFeedItem->Release();
			}
		iQueue.Reset();
	}


	TBool IsMeL(const TDesC& aNick) 
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("IsMeL"));
		return iJabberData.IsUserNickL( aNick );
	}
	
	TBool IsContactL(const TDesC& aNick) 
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("IsContactL"));
		return iJabberData.GetContactIdL( aNick ) != KErrNotFound;
	}
	
	TBool ShowInOverViewL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("ShowInOverViewL"));
		if ( FeedItem::IsComment(aItem) )
			{
				return 
					// 1. Comments to my posts
					IsMeL( aItem.iParentAuthorNick() ) 
					|| 
					// 2. comments from contacts to my contacts 
					( IsContactL( aItem.iAuthorNick() ) && IsContactL( aItem.iParentAuthorNick() ) )
					;

			}
		else
			{
				// 3. posts (and feed items) from my contacts and me 
				return IsMeL( aItem.iAuthorNick() ) 
					|| IsContactL( aItem.iAuthorNick() );
			}
	}			

	void TweakFlickrUrlL(CBBFeedItem& aItem )
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("TweakFlickrUrlL"));
		if (aItem.iThumbnailUrl().FindF(_L("flickr"))!=KErrNotFound) {
			// pick thumbnail size
			aItem.iThumbnailUrl.Value().SetLength(aItem.iThumbnailUrl().Length()-5);
			aItem.iThumbnailUrl.SyncPtr();
			
			_LIT(KSquare,    "s.jpg");
			_LIT(KThumbnail, "t.jpg");
			_LIT(KSmall,     "m.jpg");
			_LIT(KMedium,     ".jpg"); // note: no ending
			_LIT(KLarge,     "b.jpg");
			_LIT(KOriginal,  "o.jpg");

			
			TPtrC imageSize;
			TSize screen = MJuikLayout::ScreenSize();
			if ( screen.iWidth >= 240 || screen.iHeight >= 240 )
				imageSize.Set( KSmall );
			else
				imageSize.Set( KThumbnail );
			
			aItem.iThumbnailUrl.Append( imageSize );
		}
	}

	void UpdateDownloadModeL()
	{
		CALLSTACKITEM_N(_CL("CFeedStorager"), _CL("UpdateDownloadModelL"));
		{
			TInt download = 0;
			Settings().GetSettingL(SETTING_DOWNLOAD_IMAGES_YESNO, download );
			
			TInt connectivity = CONNECTIVITY_ALWAYS_ON;
			Settings().GetSettingL(SETTING_CONNECTIVITY_MODEL,
								   connectivity );
			
			CFeedItemStorage::TDownloadMode mode = CFeedItemStorage::EUnknownDL;
			if ( ! download )
				mode = CFeedItemStorage::EOnRequestDL;
			else if ( connectivity == CONNECTIVITY_ALWAYS_ON )
				mode = CFeedItemStorage::EAutomaticDL;
			else 
				mode = CFeedItemStorage::EOnLookDL;
			
			iStorage.SetDownloadModeL( mode );
		}
		{
			TBool enabled=EFalse;
			Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled);
			iStorage.SetOfflineL( !enabled );
		}
	}
	
	void SettingChanged(TInt aSetting) 
	{
		CALLSTACKITEM_N(_CL("CFeedStoragerImpl"), _CL("SettingChanged"));
		switch ( aSetting )
			{
			case SETTING_DOWNLOAD_IMAGES_YESNO:
			case SETTING_CONNECTIVITY_MODEL:
			case SETTING_PRESENCE_ENABLE:
					UpdateDownloadModeL();
					break;
			default: break;
			};
	}

private:
	CBBSubSession* iBBSession;
	CBBSession*	iOwnMainSession;
	RBBClient iDeleteSession;
	TInt iMaxQueue;


	struct TQueueEntry
	{
		TUint iBlackBoardId;
		CBBFeedItem* iFeedItem;
		TBool	iFromServer;
	};

	RArray<TQueueEntry> iQueue;

	CFeedItemStorage& iStorage;

	CJabberData& iJabberData;
};



EXPORT_C CFeedStorager*  CFeedStorager::NewL(CFeedItemStorage& aStorage, CJabberData& aJabberData)
{
	auto_ptr<CFeedStoragerImpl> self( new (ELeave) CFeedStoragerImpl( aStorage, aJabberData ) );
	self->CComponentBase::ConstructL();
	return self.release();
}

