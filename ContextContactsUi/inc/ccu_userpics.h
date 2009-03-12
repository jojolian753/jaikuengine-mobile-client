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

#ifndef __CCU_USERPICS_H__
#define __CCU_USERPICS_H__

#include "app_context.h"
#include "cbbsession.h"

#include <badesca.h>
#include <gulicon.h>
#include "jabberdata.h"
#include "jabberpics.h"

class CUserPicInfo;
class MJuikIconManager;

class MUserPicObserver 
{
 public:
	virtual void UserPicChangedL( const TDesC& aNick, TBool aIsNew ) = 0;
};

class CUserPics : public MBBObserver, public MContextBase, public CBase
{
public:
	IMPORT_C static CUserPics* NewL(CJabberPics& aJabberPics, CJabberData& aJabberData, MJuikIconManager& aIconManager);
	IMPORT_C virtual ~CUserPics();
	
	IMPORT_C TInt GetIconIndexL(TInt aContactId);
	IMPORT_C TInt GetIconIndexL(const TDesC& aNick);

	IMPORT_C void AddObserverL( MUserPicObserver& aObserver );
	IMPORT_C void RemoveObserverL( MUserPicObserver& aObserver ); // FIXME, should be non-leaving
	
	IMPORT_C CGulIcon* GetIconL(TInt aContactId);
	IMPORT_C CGulIcon* GetIconL(const TDesC& aNick);

	IMPORT_C CGulIcon& DummyIconL(TSize aSize);

 private: // From MBBObserver
	void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
				   const TComponentName& aComponentName, const MBBData* aData);
	void DeletedL(const TTupleName& aName, const TDesC& aSubName);

 private:
	CUserPics(CJabberPics& aJabberPics, CJabberData& aJabberData, MJuikIconManager& aIconManager);	
	void ConstructL();	

	TBool LoadPictureL(const TDesC& aNick);

	CGulIcon* CreateIconL(CUserPicInfo& aInfo);
	CUserPicInfo* GetPicInfoL( const TDesC& aNick );
	TInt FindPicInfoL( const TDesC& aNick );

	void AddToIconManagerL( CUserPicInfo& aInfo );
	void ReplaceInIconManagerL( CUserPicInfo& aInfo );
	CFbsBitmap* CommonMaskL();

	CFbsBitmap* LoadBitmapFromPicL(const class CBBUserPic& aPic);

	void AddCurrentIconsToManagerL();

private:
	// New user pics arrive in message queue in black board
	CBBSubSession* iBBSession;
	
	// JabberPics is database wrapper for persistent user pic storage
	CJabberPics& iJabberPics;

	// Jabber data is needed for contact id -> nick mapping
	CJabberData& iJabberData;
		
	// Internal storage for user pics 
	RPointerArray<CUserPicInfo> iUserPicInfos;

	// ListBoxIconManager where user pics are set to
	MJuikIconManager& iIconManager;

	// Observers
	RPointerArray<MUserPicObserver> iObservers;

	// UserPics's provider id for ListBoxIconManager
	TInt iProviderId;


	enum TImageSize 
		{
			ELegacy = 0,
			EDouble,
			EQvgaPortrait,
			ESmall
		} iImageSize;

	RPointerArray<CGulIcon> iDummyIcons;
};

#endif
