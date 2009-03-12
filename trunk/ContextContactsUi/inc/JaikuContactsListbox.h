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

#ifndef __JAIKUCONTACTSLISTBOX_H__
#define __JAIKUCONTACTSLISTBOX_H__

// From Context
#include "app_context.h"

// From S60
#include <aknutils.h>
#include <e32base.h>
#include <eikfrlb.h>

#include "ccu_userpics.h"

class CUserPics;
class CCoeControl;
class MDesCArray;
class CGulIcon;
class phonebook_i;
class CBBPresence;
class contact;

namespace JaikuUtils
{
	enum TPresenceStatus
		{
			EPresenceNone = -1,
			EPresenceGray = 0,
			EPresenceRed,
			EPresenceYellow,
			EPresenceGreen
		};

	TPresenceStatus GetPresenceStatus( contact& aContact, TBool aPresenceEnabled );

	TPresenceStatus GetPresenceStatus( const CBBPresence* p, TBool aPresenceEnabled, TBool aMyPresence );

	void LoadJaikuIcons(CArrayPtrFlat<CGulIcon> * aIconList);

	TInt GetJaikuIconIndex(TInt identifier);

}


class CJaikuContactsLBModel : public CAknFilteredTextListBoxModel,
	public MContextBase
{
 public: 
	IMPORT_C void SetFilterArray( MDesCArray* aFilterArray );
	IMPORT_C static CJaikuContactsLBModel* NewL();	  

 public:
	virtual const MDesCArray* MatchableTextArray() const;
 private:
	MDesCArray* iFilterArray;
};


class CJaikuContactsListBox : public CEikFormattedCellListBox,
	public MContextBase, public MUserPicObserver
{
 public: // public API 
	IMPORT_C static CJaikuContactsListBox* CJaikuContactsListBox::CreateListboxL(CCoeControl* aParent, 
																		  phonebook_i& aPhonebook,
																		  CUserPics& aUserPics );
	
	IMPORT_C static MDesCArray* CreateLBArrayL( phonebook_i& aPhonebook, CUserPics& aUserPics );


 public:
	
	/**
	 * Adjust rect and calls SetRect
	 * @returns adjusted rect for later reference
	 */
	IMPORT_C TRect AdjustAndSetRect( TRect aRect );

	IMPORT_C CJaikuContactsLBModel* JaikuModel() const;

 protected: // from base classes
	void SizeChanged();
	void CreateItemDrawerL();
	CJaikuContactsListBox( phonebook_i& aPhonebook, CUserPics& aUserPics );

	void SettingChanged(TInt aSetting);

 protected: // From MUserPicObserver
	void UserPicChangedL(const TDesC& aNick);


 private:
	void SetupSubcellsL();

	void UpdateIconsL();

	void LoadTestIconsL( class CAknIconArray& aIconArray );
 private:
	phonebook_i& iPhonebook;
	CUserPics& iUserPics;
};
#endif // __JAIKUCONTACTSLISTBOX_H__
