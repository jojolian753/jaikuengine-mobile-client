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

#ifndef TEST_STORAGE_H
#define TEST_STORAGE_H


#include "TestSuite.h"
#include "contexttestbase.h"
#include "ccu_storage.h"

#define EVENT_TYPES 13

class CTest_Storage : public CxxTest::TestSuite, public MContextTestBase, public MFeedNotify
{
public:
  CTest_Storage( const TDesC8& aSuiteName ) 
    : CxxTest::TestSuite( aSuiteName ) {}

	/* tests go here */
	void testDownloadModeSwitch2L();
	void testNoMediaL();
	void testWithUrlL();
	void testWithLocalMediaL();
	void testDownload1L();
	void testDownload2L();
	void testDownloadModeSwitch1L();
	
	void testCreateL();
	void testOneL();
	void testAllL();
	void testAuthorL();
	void testParentFirstL();
	void testParentLastL();
	void testDeleteL();
	void testReadL();
	void testGrouping1L();
	void testGrouping2L();
	void testUpdateL();
	void testPurge1L();
	void testPurge2L();
	void testPurgeLargeL();
	void testMarkAsReadL();
	
	void testNonOverview1L();
	void testNonOverview2L();
	

 private:
 	virtual void FeedItemEvent(CBBFeedItem* aItem, TEvent aEvent);
 	virtual void AuthorCountEvent(const TDesC&, TInt, TInt);
 	void CheckCountsL(CFeedItemStorage* s, TInt aCount, TInt aUnreadCount,
 		TBool aDontShowInOverView=EFalse, TInt aOverViewCount=0);
 	void CheckAuthorCountsL(CFeedItemStorage* s, const TDesC& aAuthor,
 		TInt aCount, TInt aUnreadCount);
 	void doTestParentL(TBool aParentFirst, TBool aGroupChild=EFalse,
 		TBool aChildrenNotInOverView=EFalse, TInt aUpdates=0);
	void doTestMarkAsReadL(TInt aMarkBy);
	void doTestAllL(TBool aDontShowInOverView=EFalse);
	void doTestPurge1L(TBool aDontShowInOverView=EFalse);
	void doTestPurgeJaikuL(TBool);
 	
  void setUp();
  void tearDown();
  class CDb* iDb;
  TInt iEventCount[EVENT_TYPES];
  TInt iStopOnEventCount[EVENT_TYPES];

};



#endif // TEST_STORAGE_H
