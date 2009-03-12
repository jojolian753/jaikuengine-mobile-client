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

#ifndef CB_JABBERDATA_H_INCLUDED
#define CB_JABBERDATA_H_INCLUDED 1

#include "app_context.h"
#include "settings.h"
#include "db.h"
#include "cntdef.h"
#include "list.h"

class MJabberDataObserver {
public:
	virtual void IdChanged(const TDesC& aNick, TInt aOldId, TInt aNewId) = 0;
};

class CJabberData : public MDBStore, public MContextBase, public MSettingListener, public CBase
{
public:
	static const TInt KNickMaxLen = 100;
	typedef TBuf<KNickMaxLen> TNick;

	enum TRelationReason
		{
			EUnknown = 0, 
			ESetByUser = 1,
			ERemovedByUser, 
			EMatchedWithVerifiedNumber,
			EMatchedWithUnverifiedNumber,
			EAutomaticDummy
		};
 public: // Nick 
	/** Compares 2 nicks, ensures that nicks are in canonical form before
	 *  comparing. 
	 */
	IMPORT_C static TBool EqualNicksL(const TDesC& aX, const TDesC& aY);

	/**
	 * Converts nick to canonized form. 
	 * E.g. teemu is converted to teemu@jaiku.com
	 */ 
	IMPORT_C static void CanonizeNickL( TDes& aNick );

	/**
	 * Return UI form of nick
	 * E.g. teemu@jaiku.com is converted to teemu.
	 */ 
	IMPORT_C static void TransformToUiNickL( TDes& aNick );
	
public:
	IMPORT_C static CJabberData* NewL(MApp_context& Context, CDb& Db, TInt aMyNickSettingId);
	IMPORT_C ~CJabberData();

	IMPORT_C TBool IsUserNickL( const TDesC& aNick );
	IMPORT_C const TDesC& UserNickL();  

	IMPORT_C bool GetJabberNickL(TInt ContactId, TDes& Nick);
	IMPORT_C void SetJabberNickL(TInt ContactId, const TDesC& Nick, TRelationReason aReason);

	IMPORT_C TInt CreateDummyNickL(const TDesC& aNick, 
								   const TDesC& aLastName, const TDesC& aFirstName );
	IMPORT_C void MarkNickAsRemovedL(const TDesC& aNick);

	IMPORT_C TBool GetShowDetailsInListL(TInt ContactId);
	IMPORT_C void SetShowDetailsInListL(TInt ContactId, TBool aShowDetails);
	IMPORT_C TInt GetContactIdL(const TDesC& Nick); // KErrNotFound if not found
	
	IMPORT_C HBufC* GetFirstNameL(TInt aContactId);
	IMPORT_C HBufC* GetLastNameL(TInt aContactId);
	IMPORT_C void UpdateFirstLastNameL(TInt aContactId, 
									   const TDesC& aFirst, 
									   const TDesC& aLast);

	/**
	 * Generates new dummy contact id from free dummy ids
	 */
	IMPORT_C TInt GetNewDummyContactIdL();

	IMPORT_C TBool IsDummyContactId( TInt aId );
	IMPORT_C CContactIdArray* GetDummyIdsL();

	IMPORT_C void AddObserver(MJabberDataObserver* aObserver);
	IMPORT_C void RemoveObserver(MJabberDataObserver* aObserver);
	/**
	 * Get user -> id relation reason	
	 */ 
	EXPORT_C TBool GetRelationReasonL( TInt aId, TRelationReason& aResult );

 private: // From MSettingListener
	virtual void SettingChanged(TInt aSetting);

 private: // own 
	void LowerCaseNicksL();

	void SetJabberNickImplL(TInt ContactId, 
							const TDesC& Nick, 
							TRelationReason aReason,
							const TDesC& aFirstName,
							const TDesC& aLastName);

	HBufC* GetNameImplL(TInt aContactId, TInt aColumn);


private:
	CJabberData(MApp_context& Context, CDb& Db, TInt aMyNickSettingId);
	void ConstructL();

	void NotifyObservers(const TDesC& aNick, TInt aOldId, TInt aNewId);
	CList<MJabberDataObserver*>*	iObservers;

	CDb&	             iJabberNickDb;
	TInt                 iMyNickSettingId;
	TBuf< 256 >  iUserNick;
	};

#endif
