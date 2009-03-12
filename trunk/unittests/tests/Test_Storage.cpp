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

#include "Test_storage.h"

#include "ccu_storage.h"
#include "symbian_auto_ptr.h"
#include "contextvariant.hrh"
#include "app_context_impl.h"
#include "testutils.h"
#include "dummyhttp.inl"

void CTest_Storage::FeedItemEvent(CBBFeedItem* aItem, TEvent aEvent)
{
	iEventCount[aEvent]++;

	if (iStopOnEventCount[aEvent]==iEventCount[aEvent]) {
		CActiveScheduler::Stop();
	}
}

void CTest_Storage::AuthorCountEvent(const TDesC&, TInt, TInt)
{
}

void CTest_Storage::testCreateL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
}

void CTest_Storage::CheckCountsL(CFeedItemStorage* s, TInt aCount, TInt aUnreadCount, 
	TBool aOverViewOnly, TInt aOverViewCount)
{
	TInt count=0;
	TBool found=s->FirstAllL();
	while (found) {
		count++;
		found=s->NextL();
	}
	TS_ASSERT_EQUALS(count, aCount);
	TInt cc=0, uc=0, gc=0;
	TGlobalId root;
	s->GetChildCountsL(root, cc, uc, gc);
	TS_ASSERT_EQUALS(cc, aCount);
	if (aUnreadCount!=-1) TS_ASSERT_EQUALS(uc, aUnreadCount);
	s->GetChildCountsL(root, cc, uc, gc, aOverViewOnly);
	TS_ASSERT_EQUALS(cc, aOverViewOnly ? aOverViewCount : aCount);
}

void CTest_Storage::CheckAuthorCountsL(CFeedItemStorage* s, const TDesC& aAuthor,
 		TInt aCount, TInt aUnreadCount)
{
	TInt count=0;
	TBool found=s->FirstByAuthorL(aAuthor);
	while (found) {
		count++;
		found=s->NextL();
	}
	TS_ASSERT_EQUALS(count, aCount);
	TInt cc=0, uc=0;
	s->GetCountsByAuthorL(aAuthor, cc, uc);
	TS_ASSERT_EQUALS(cc, aCount);
	TS_ASSERT_EQUALS(uc, aUnreadCount);
}


void CTest_Storage::testPurge1L()
{
	doTestPurge1L();
	doTestPurge1L(ETrue);
	doTestPurgeJaikuL(EFalse);
	doTestPurgeJaikuL(ETrue);
}

void CTest_Storage::doTestPurgeJaikuL(TBool aDoJaiku)
{
	TBool aDontShowInOverView=EFalse;
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	s->SubscribeL(this);
	TTime t=GetTime();
	TInt author=1;
	TInt j=0;
	for (int i=0; i<4; i++) {
		refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
		f->iUuid()=_L8("00000000000000");
		f->iUuid().AppendNum(j);
		f->iUuid().AppendNum(i);
		f->iAuthorNick()=_L("a");
		f->iAuthorNick().AppendNum(author);
		f->iCreated()=t;
		f->iCreated()-=TTimeIntervalSeconds( (i+1)*10);
		f->iDontShowInOverView=aDontShowInOverView;
		if (aDoJaiku && i>1) f->iKind()=_L("presence");
		s->AddLocalL(f.get());
	}
	
	s->RemoveFeedItemsL(t-TTimeIntervalSeconds(11), CFeedItemStorage::EByDateTime, 1);
	CheckCountsL(s.get(), aDoJaiku ? 2 : 1, 0, aDontShowInOverView, 0);
	s->RemoveFeedItemsL(GetTime()+TTimeIntervalDays(1), CFeedItemStorage::EByDateTime, 0);
	CheckCountsL(s.get(), 0, 0, aDontShowInOverView, 0);
}

void CTest_Storage::doTestPurge1L(TBool aDontShowInOverView)
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	s->SubscribeL(this);
	TTime t=GetTime();
	for (int i=0; i<4; i++) {
		refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
		f->iUuid()=_L8("000000000000000");
		f->iUuid().AppendNum(i);
		f->iAuthorNick()=_L("a1");
		f->iCreated()=t;
		f->iCreated()-=TTimeIntervalSeconds(i+1);
		f->iDontShowInOverView=aDontShowInOverView;
		s->AddLocalL(f.get());
	}
	
	
	s->RemoveFeedItemsL(t, CFeedItemStorage::EByDateTime, 3);
	CheckCountsL(s.get(), 3, 0, aDontShowInOverView, 0);
	s->RemoveFeedItemsL(t-TTimeIntervalSeconds(10), CFeedItemStorage::EByDateTime, 0);
	CheckCountsL(s.get(), 3, 0, aDontShowInOverView, 0);
	s->RemoveFeedItemsL(GetTime()+TTimeIntervalDays(1), CFeedItemStorage::EByDateTime, 0);
	CheckCountsL(s.get(), 0, 0, aDontShowInOverView, 0);
}

void CTest_Storage::testPurgeLargeL()
{
	// takes a very long time, enable only if you suspect a bug in it
	return;
	
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	s->SubscribeL(this);
	TTime t=GetTime();
	for (int j=0; j<10; j++) {
		for (int i=0; i<100; i++) {
			refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
			f->iUuid()=_L8("000000000000");
			f->iUuid().AppendNumFixedWidth(j, EHex, 2);
			f->iUuid().AppendNumFixedWidth(i, EHex, 2);
			
			if (i>0) {
				f->iParentUuid()=_L8("000000000000");
				f->iParentUuid().AppendNumFixedWidth(j, EHex, 2);
				f->iParentUuid().AppendNumFixedWidth(0, EHex, 2);
			}
			f->iAuthorNick()=_L("a");
			f->iAuthorNick().AppendNum(j);
			f->iCreated()=t;
			f->iCreated()-=TTimeIntervalSeconds( 10*(i+1) );
			s->AddLocalL(f.get());
		}
	}
	
	
	CheckCountsL(s.get(), 1000, 0);
	s->RemoveFeedItemsL(t-TTimeIntervalSeconds(505), CFeedItemStorage::EByDateTime, 0);
	CheckCountsL(s.get(), 500, 0);
	CheckAuthorCountsL(s.get(), _L("a2"), 50, 0);
}

void CTest_Storage::testPurge2L()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	s->SubscribeL(this);
	TTime t=GetTime();
	for (int i=0; i<4; i++) {
		refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
		f->iUuid()=_L8("000000000000000");
		f->iUuid().AppendNum(i);
		f->iAuthorNick()=_L("a");
		f->iAuthorNick().AppendNum(i);
		f->iCreated()=t;
		f->iCreated()-=TTimeIntervalSeconds( 10*(i+1) );
		s->AddLocalL(f.get());
	}
	
	s->RemoveFeedItemsL(t, CFeedItemStorage::EByDateTime, 3);
	CheckCountsL(s.get(), 4, 0);
	s->RemoveFeedItemsL(t-TTimeIntervalSeconds(100), CFeedItemStorage::EByDateTime, 0);
	CheckCountsL(s.get(), 4, 0);
	CheckAuthorCountsL(s.get(), _L("a2"), 1, 0);
	
	s->RemoveFeedItemsL(t-TTimeIntervalSeconds(35), CFeedItemStorage::EByDateTime, 0);
	CheckCountsL(s.get(), 3, 0);
	CheckAuthorCountsL(s.get(), _L("a2"), 1, 0);
	
	s->RemoveFeedItemsL(t-TTimeIntervalSeconds(25), CFeedItemStorage::EByDateTime, 0);
	CheckCountsL(s.get(), 2, 0);
	CheckAuthorCountsL(s.get(), _L("a2"), 0, 0);
}

void CTest_Storage::testOneL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	
	s->SubscribeL(this);
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	s->UnSubscribeL(this);
}

void CTest_Storage::testAllL() {
	doTestAllL();
}

void CTest_Storage::doTestAllL(TBool aDontShowInOverView)
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	s->SubscribeL(this);
	for (int i=0; i<4; i++) {
		refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
		f->iUuid()=_L8("000000000000000");
		f->iUuid().AppendNum(i);
		f->iDontShowInOverView=aDontShowInOverView;
		s->AddLocalL(f.get());
	}
	CheckCountsL(s.get(), 4, 0, aDontShowInOverView);
	
	for (int i=0; i<4; i++) {
		TBuf8<16> uuid=_L8("000000000000000");
		uuid.AppendNum(i);
		refcounted_ptr<CBBFeedItem> fn(s->GetByGlobalIdL(uuid));
		
		TS_ASSERT(fn->iUuid()==uuid);
	}
	s->UnSubscribeL(this);
}

void CTest_Storage::testMarkAsReadL()
{
	doTestMarkAsReadL(0);
	doTestMarkAsReadL(1);
	doTestMarkAsReadL(2);
}

void CTest_Storage::doTestMarkAsReadL(TInt aMarkBy)
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	
	for (int parent=0; parent<5; parent++) {
		refcounted_ptr<CBBFeedItem> p(new (ELeave) CBBFeedItem);
		p->iUuid()=_L8("00000000000000p");
		p->iUuid().AppendNum( parent );
		p->iAuthorNick()=_L("aa"); p->iAuthorNick().AppendNum(parent);
		p->iIsUnread()=ETrue;
		p->iCreated()=GetTime()+TTimeIntervalSeconds(parent);
		s->AddLocalL(p.get());
		
		for (int child=0; child<=parent; child++) {
			refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
			f->iAuthorNick()=_L("aa"); f->iAuthorNick().AppendNum(child);
			f->iUuid()=_L8("00000000000000");
			f->iUuid().AppendNum(parent);
			f->iUuid().AppendNum(child);
			p->iCreated()=GetTime()+TTimeIntervalSeconds(child+1);
			f->iParentUuid()=_L8("00000000000000p");
			f->iParentUuid().AppendNum( parent );
			f->iIsUnread()=ETrue;
			s->AddLocalL(f.get());		
		}
	}
	
	for (int parent=0; parent<5; parent++) {
		TBuf<15> author=_L("aa"); author.AppendNum(parent);
		TInt cc=-1, uc=-1;
		s->GetCountsByAuthorL(author, cc, uc);
		TS_ASSERT_EQUALS( (5-parent)+1, cc);
		TS_ASSERT_EQUALS( (5-parent)+1, uc);
	}
	
	if (aMarkBy==0) {
		for (int parent=0; parent<5; parent++) {
			TBuf8<16> uuid=_L8("00000000000000p");
			uuid.AppendNum(parent);
			s->MarkAsRead(uuid);
			
			TBuf<15> author=_L("aa"); author.AppendNum(parent);
			TInt cc=-1, uc=-1;
			s->GetCountsByAuthorL(author, cc, uc);
			TS_ASSERT_EQUALS( (5-parent)+1, cc);
			TInt expected=4-parent;
			if (expected<0) expected=0;
			TS_ASSERT_EQUALS( expected, uc);
		}
	} else if (aMarkBy==1) {
		for (int parent=0; parent<5; parent++) {
			TBuf<15> author=_L("aa"); author.AppendNum(parent);
			s->MarkAsRead(author);
			TInt cc=-1, uc=-1;
			s->GetCountsByAuthorL(author, cc, uc);
			TS_ASSERT_EQUALS( (5-parent)+1, cc);
			TS_ASSERT_EQUALS( 0, uc);
		}
	} else {
		s->MarkAllAsRead();
		for (int parent=0; parent<5; parent++) {
			TBuf<15> author=_L("aa"); author.AppendNum(parent);
			TInt cc=-1, uc=-1;
			s->GetCountsByAuthorL(author, cc, uc);
			TS_ASSERT_EQUALS( (5-parent)+1, cc);
			TS_ASSERT_EQUALS( 0, uc);
		}
	}
	s->RemoveFeedItemsL(GetTime()+TTimeIntervalDays(1), CFeedItemStorage::EByDateTime, 0);
}

void CTest_Storage::testNonOverview1L()
{
	doTestAllL(EFalse);
}

void CTest_Storage::testNonOverview2L()
{
}

void CTest_Storage::testAuthorL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	s->SubscribeL(this);
	int j=0, i=0;
	for (i=0; i<4; i++) {
		refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
		f->iUuid()=_L8("000000000000000");
		f->iUuid().AppendNum(i);
		f->iIsUnread()=ETrue;
		f->iAuthorNick().Append(_L("A"));
		f->iAuthorNick().AppendNum( j );
		s->AddLocalL(f.get());
		if (i>1) j++;
	}
	CheckCountsL(s.get(), 4, 4);
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 4 );
	
	for (j=0; j<3; j++) {
		TInt count=0;
		TBuf<14> author; author.Append(_L("a")); author.AppendNum(j);
		TBool found=s->FirstByAuthorL(author);
		TInt unread_in_post=0;
		while (found) {
			refcounted_ptr<CBBFeedItem> f(s->GetCurrentL());
			if (f->iIsUnread()) unread_in_post++;
			count++;
			found=s->NextL();
		}
		
		TInt unread=-1, children=-1;
		s->GetCountsByAuthorL(author, children, unread);
		if (j==0) {
			TS_ASSERT_EQUALS(count, 3);
		} else if (j<2) {
			TS_ASSERT_EQUALS(count, 1);
		} else {
			TS_ASSERT_EQUALS(count, 0);
		}
		TS_ASSERT_EQUALS(unread_in_post, unread);
		TS_ASSERT_EQUALS(count, unread);
		TS_ASSERT_EQUALS(count, children);
	}
	
	for (j=0, i=0; i<4; i++) {
		refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
		f->iUuid()=_L8("000000000000000");
		f->iUuid().AppendNum(i);
		f->iAuthorNick().Append(_L("A"));
		f->iAuthorNick().AppendNum( j );
		s->MarkAsRead(f.get());
		if (i>1) j++;
	}

	int bm;
	for (bm=0; bm<2; bm++) {
		for (j=0; j<3; j++) {
			TInt count=0;
			TBuf<12> author; 
			if (bm) author.Append(_L("a"));
			else author.Append(_L("A"));
			author.AppendNum(j);
			TBool found=s->FirstByAuthorL(author);
			TInt unread_in_post=0;
			while (found) {
				refcounted_ptr<CBBFeedItem> f(s->GetCurrentL());
				if (f->iIsUnread()) unread_in_post++;
				count++;
				TBookmark *book=0;
				if (bm) {
					book=s->Bookmark();
					s->FirstAllL();
				}
				found=s->NextL(book);
			}
			
			TInt unread=-1, children=-1;
			s->GetCountsByAuthorL(author, children, unread);
			if (j==0) {
				TS_ASSERT_EQUALS(count, 3);
			} else if (j<2) {
				TS_ASSERT_EQUALS(count, 1);
			} else {
				TS_ASSERT_EQUALS(count, 0);
			}
			TS_ASSERT_EQUALS(unread_in_post, unread);
			TS_ASSERT_EQUALS(0, unread);
			TS_ASSERT_EQUALS(count, children);
		}
	}

	s->UnSubscribeL(this);
	s->RemoveFeedItemsL(GetTime()+TTimeIntervalDays(1), CFeedItemStorage::EByDateTime, 0);
}

void CTest_Storage::testParentFirstL()
{
	doTestParentL(ETrue, EFalse);
	doTestParentL(ETrue, EFalse, ETrue);
}

#include "break.h"

void CTest_Storage::testParentLastL()
{
	doTestParentL(EFalse, EFalse);
}

void CTest_Storage::testGrouping1L()
{
#ifdef __WINS__
	TInt dummy;
	TBreakItem bi(GetContext(), dummy);
#endif
	doTestParentL(ETrue, ETrue);
	doTestParentL(ETrue, ETrue, EFalse, 1);
	doTestParentL(ETrue, ETrue, EFalse, 2);
	doTestParentL(ETrue, ETrue, EFalse, 3);
}

void CTest_Storage::testGrouping2L()
{
#ifdef __WINS__
	TInt dummy;
	TBreakItem bi(GetContext(), dummy);
#endif
	doTestParentL(EFalse, ETrue, EFalse);
	doTestParentL(EFalse, ETrue, EFalse, 1);
	doTestParentL(EFalse, ETrue, EFalse, 2);
	doTestParentL(EFalse, ETrue, EFalse, 3);
}

void CTest_Storage::doTestParentL(TBool aParentFirst, TBool aGroupChild, 
	TBool aChildrenNotInOverView, TInt aUpdates)
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	s->SubscribeL(this);
	int j=0; int prev_j=-1;
	iEventCount[EAdded]=0;
	
	TTime now=GetTime();
	TInt so_far=0, parents_so_far=0;
	for (int i=0; i<4; i++) {
		refcounted_ptr<CBBFeedItem> p(new (ELeave) CBBFeedItem);
		so_far++;
		p->iUuid()=_L8("000000000000001");
		p->iUuid().AppendNum( j );
		p->iAuthorNick()=_L("aa"); p->iAuthorNick().AppendNum(j);
		p->iIsUnread()=ETrue;
		if (aParentFirst && j!=prev_j) {
			p->iCreated()=now+TTimeIntervalSeconds(i);
			s->AddLocalL(p.get());
			so_far++; parents_so_far++;
			prev_j=j;
		}
		refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
		f->iAuthorNick()=_L("aa"); f->iAuthorNick().AppendNum(j);
		f->iUuid()=_L8("000000000000000");
		f->iUuid().AppendNum(i);
		f->iCreated()=now+TTimeIntervalSeconds(i+1);
		f->iParentUuid()=_L8("000000000000001");
		f->iParentUuid().AppendNum( j );
		f->iIsUnread()=ETrue;
		f->iIsGroupChild()=aGroupChild;
		f->iDontShowInOverView=aChildrenNotInOverView;
		s->AddLocalL(f.get());
		if (!aGroupChild) {
			CheckCountsL(s.get(), so_far, -1, aChildrenNotInOverView, aChildrenNotInOverView ? parents_so_far : so_far);
		}
		if (!aParentFirst && j!=prev_j) {
			p->iCreated()=now+TTimeIntervalSeconds(i+2);
			s->AddLocalL(p.get());
			so_far++; parents_so_far++;
			prev_j=j;
		}
		if (aUpdates==1 || aUpdates==3) {
			refcounted_ptr<CBBFeedItem> f2(s->GetByGlobalIdL(f->iUuid()));
			f2->iContent.Append(_L("x"));
			s->UpdateFeedItemL(f2.get());
		}
		if (aUpdates==2 || aUpdates==3) {
			refcounted_ptr<CBBFeedItem> p2(s->GetByGlobalIdL(p->iUuid()));
			p2->iContent.Append(_L("x"));
			s->UpdateFeedItemL(p2.get());
		}
		if (i>1) j++;
	}
	TInt count=0;
	TBool found=s->FirstAllL();
	while (found) {
		count++;
		found=s->NextL();
	}
	TS_ASSERT_EQUALS(count, 6);
	if (!aGroupChild) {
		CheckCountsL(s.get(), 6, -1, aChildrenNotInOverView, aChildrenNotInOverView ? 2 : 6);
	}
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 6 );
	
	for (j=0; j<3; j++) {
		TInt count=0;
		TBuf8<16> parent=_L8("000000000000001");
		parent.AppendNum(j);
		TBool found=s->FirstByParentL(parent);
		while (found) {
			count++;
			found=s->NextL();
		}
		TInt expected=0;
		if (j==0) {
			expected=3;
		} else if (j<2) {
			expected=1;
		}
		TS_ASSERT_EQUALS(count, expected);
		TBuf<15> author=_L("aa"); author.AppendNum(j);
		if (expected!=0) {
			refcounted_ptr<CBBFeedItem> fn(s->GetByGlobalIdL(parent));
			TS_ASSERT_EQUALS(fn->iChildCount, aGroupChild ? 0 : expected);
			TS_ASSERT_EQUALS(fn->iGroupChildCount, aGroupChild ?  expected : 0);
			TS_ASSERT_EQUALS(fn->iUnreadChildCounter, aGroupChild ? 0 : expected);
			TTime ts;
			ts=s->GetCurrentTimeStampL();
			TS_ASSERT_EQUALS(fn->iCreated(), ts);
		}
		TInt cc=-1, uc=-1;
		s->GetCountsByAuthorL(author, cc, uc);
		// parent is with same author, hence +1
		if (aGroupChild && expected>0) expected=1;
		else if (expected>0) expected+=1;
		TS_ASSERT_EQUALS(expected, cc);
		TS_ASSERT_EQUALS(expected, uc);
	}
	s->UnSubscribeL(this);
	s->RemoveFeedItemsL(GetTime()+TTimeIntervalDays(1), CFeedItemStorage::EByDateTime, 0);
	for (j=0; j<3; j++) {
		TBuf<15> author=_L("aa"); author.AppendNum(j);
		TInt cc=-1, uc=-1;
		s->GetCountsByAuthorL(author, cc, uc);
		TS_ASSERT_EQUALS(0, cc);
		TS_ASSERT_EQUALS(0, uc);
	}
}

void CTest_Storage::testDeleteL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	
	s->SubscribeL(this);
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	s->RemoveFeedItemL(fn.get());
	TS_ASSERT( ! s->FirstAllL() );
	TS_ASSERT_EQUALS ( iEventCount[EFeedItemDeleted], 1 );
	
	s->UnSubscribeL(this);
	s->RemoveFeedItemsL(GetTime()+TTimeIntervalDays(1), CFeedItemStorage::EByDateTime, 0);
}

void CTest_Storage::testReadL()
{
}

void CTest_Storage::testUpdateL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	
	s->SubscribeL(this);
	
	TBuf8<16> uuid=_L8("0000000000000001");
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	refcounted_ptr<CBBFeedItem> f2(0);
	{
		refcounted_ptr<CBBFeedItem> f0(new (ELeave) CBBFeedItem);
		f0->iUuid()=uuid;
		f0->iContent.Append(_L("c1"));
		f0->iFromServer()=EFalse;
		s->AddLocalL(f0.get());
		*f=*f0;
	}
	{
		f2.reset(s->GetByGlobalIdL(uuid));
		TS_ASSERT(*f==*f2);
	}
	{
		f->iContent.Append(_L("m1"));
		f->iFromServer()=ETrue;
		s->UpdateFeedItemL(f.get());
		
		refcounted_ptr<CBBFeedItem> f3(s->GetByGlobalIdL(uuid));
		TS_ASSERT(*f==*f3);
		TS_ASSERT(*f==*f2);
	}
	s->RemoveFeedItemsL(GetTime()+TTimeIntervalDays(1), CFeedItemStorage::EByDateTime, 0);
}

void CTest_Storage::testNoMediaL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	
	s->SubscribeL(this);
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::ENoMedia );
	s->UnSubscribeL(this);
}

void CTest_Storage::testWithUrlL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	
	s->SubscribeL(this);
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	f->iThumbnailUrl.Append(_L("url1"));
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );
	s->UnSubscribeL(this);
}

void CTest_Storage::testWithLocalMediaL()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory())); 
	
	s->SubscribeL(this);
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	f->iMediaFileName()=(_L("c:\\x"));
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EMediaDownloaded );
	s->UnSubscribeL(this);
}

void CTest_Storage::testDownload1L()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory(),
		&CDummyHttp::NewL, _L("UTDL2"))); 
	
	s->SubscribeL(this);
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	f->iThumbnailUrl.Append(_L("url1"));
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );

	s->DownloadMediaForFeedItemL(fn.get());
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EQueued );
	
	iStopOnEventCount[EMediaDownloadStateChanged]=3;
	TS_ASSERT( TestUtils::WaitForActiveSchedulerStopL(5) );
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EMediaDownloaded );
		
	s->UnSubscribeL(this);
}

void CTest_Storage::testDownload2L()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory(),
		&CDummyHttp::NewL, _L("UTDL2"))); 
	
	s->SubscribeL(this);
	
	{
		refcounted_ptr<CBBFeedItem> f1(new (ELeave) CBBFeedItem);
		f1->iUuid()=_L8("0000000000000001");
		f1->iCreated()=TTime(2);
		f1->iThumbnailUrl.Append(_L("url1"));
		s->AddLocalL(f1.get());
	}
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iCreated()=TTime(1);
	f->iUuid()=_L8("0000000000000002");
	f->iThumbnailUrl.Append(_L("url2"));
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	TS_ASSERT( s->NextL() );
	refcounted_ptr<CBBFeedItem> fn2(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn2.get()) );
	TS_ASSERT( ! (fn->Equals(fn2.get())) );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );
	TS_ASSERT_EQUALS ( fn2->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );

	s->DownloadMediaForFeedItemL(fn.get());
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EQueued );
	TS_ASSERT_EQUALS ( fn2->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );
	s->DownloadMediaForFeedItemL(fn2.get());
	TS_ASSERT_EQUALS ( fn2->iMediaDownloadState(), (TInt) CBBFeedItem::EQueued );
	
	iStopOnEventCount[EMediaDownloadStateChanged]=6;
	TS_ASSERT( TestUtils::WaitForActiveSchedulerStopL(10) );
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EMediaDownloaded );
	TS_ASSERT_EQUALS ( fn2->iMediaDownloadState(), (TInt) CBBFeedItem::EMediaDownloaded );
		
	s->UnSubscribeL(this);
}

void CTest_Storage::testDownloadModeSwitch1L()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory(),
		&CDummyHttp::NewL, _L("UTDL2"))); 
	
	s->SubscribeL(this);
	s->SetDownloadModeL( CFeedItemStorage::EAutomaticDL );
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	f->iThumbnailUrl.Append(_L("url1"));
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );

	s->DownloadMediaForFeedItemL(fn.get());
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EQueued );
	
	s->SetDownloadModeL( CFeedItemStorage::EOnLookDL );
	iStopOnEventCount[EMediaDownloadStateChanged]=2;
	TS_ASSERT( TestUtils::WaitForActiveSchedulerStopL(5) );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );
		
	s->UnSubscribeL(this);
}

void CTest_Storage::testDownloadModeSwitch2L()
{
	auto_ptr<CFeedItemStorage> s(CFeedItemStorage::NewL(AppContext(), *iDb, AppContext().BBDataFactory(),
		&CDummyHttp::NewL, _L("UTDL2"))); 
	
	s->SubscribeL(this);
	s->SetDownloadModeL( CFeedItemStorage::EAutomaticDL );
	
	refcounted_ptr<CBBFeedItem> f(new (ELeave) CBBFeedItem);
	f->iUuid()=_L8("0000000000000001");
	f->iThumbnailUrl.Append(_L("url1"));
	s->AddLocalL(f.get());
	
	TS_ASSERT( s->FirstAllL() );
	
	refcounted_ptr<CBBFeedItem> fn(s->GetCurrentL());
	
	TS_ASSERT( f->Equals(fn.get()) );
	
	TS_ASSERT_EQUALS ( iEventCount[EAdded], 1 );
	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::ENotDownloading );

	s->DownloadMediaForFeedItemL(fn.get());
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EQueued );
	
	s->SetOfflineL(ETrue);	
	TestUtils::WaitForActiveSchedulerStopL(3);
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EDownloadPausedOffline );
		
	s->SetOfflineL(EFalse);	
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EQueued );
	TestUtils::WaitForActiveSchedulerStopL(3);
	TS_ASSERT_EQUALS ( fn->iMediaDownloadState(), (TInt) CBBFeedItem::EMediaDownloaded );
	
	s->UnSubscribeL(this);
}

void CTest_Storage::setUp()
{
	
	MContextTestBase::setUp();
	
	TFileName db;
	db=DataDir();
	db.Append( _L("\\") );
	db.Append( _L("UTFEED2.DB") );
	TInt err=BaflUtils::DeleteFile(Fs(), db);
	db=DataDir();
	db.Append( _L("\\") );
	db.Append( _L("UTDL2.DB") );
	err=BaflUtils::DeleteFile(Fs(), db);
	
	iDb=CDb::NewL(AppContext(), _L("UTFEED2"), EFileWrite, false);
	for (int i=0; i<EVENT_TYPES; i++) {
		iEventCount[i]=0;
		iStopOnEventCount[i]=0;
	}
}


void CTest_Storage::tearDown()
{
	delete iDb; iDb=0;
	MContextTestBase::tearDown();
}

