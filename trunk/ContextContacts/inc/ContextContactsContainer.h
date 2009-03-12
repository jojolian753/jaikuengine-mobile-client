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

#ifndef CONTEXTCONTACTSCONTAINER_H
#define CONTEXTCONTACTSCONTAINER_H

#include <coecntrl.h>
#include <aknlists.h>
#include <cntitem.h>
#include <e32base.h>
#include <CPbkContactEngine.h>
#include <eikedwin.h>
#include <sendui.h>
#include <aknsfld.h> 
#include <aknglobalnote.h>
#include <cpbkcontactengine.h> 
#include <akniconarray.h>

#include "phonebook.h"

#include "file_output_base.h"
#include "icons.h"
#include <AknView.h>

#include "contextvariant.hrh"
#include "ccu_contactuidelegates.h"
#ifndef __JAIKU__

#include "doublelinebox.h"

#endif

class CContextContactsContainer : public CCoeControl, 

	public MCoeControlObserver, 
	public MEikListBoxObserver, 
	public MListBoxItemChangeObserver,
	public phonebook_observer, 
	public MContextBase
{
 public: 
 CContextContactsContainer(TContactUiDelegates& aDelegates) : iDelegates( aDelegates ) {}
	void ConstructL(CCoeControl* aParent, 
					Mfile_output_base * log, 
					CAknView * view);
	
	~CContextContactsContainer();
    public:

	TInt get_current_idx();
	TInt get_current_top_idx();
	TInt get_current_bottom_idx();
	void set_current_idx(TInt real_idx);
	void set_current_top_idx(TInt real_idx);
	

	/**
	 * Note: this can be heavy operation, cache count instead of calling this repeatedly.
	 * @returns count of selected items
	 */
	TInt SelectedItemsCountL();

	/**
	 * Note: this can be heavy operation, use sparingly when needed
	 * @returns copy of array of indexes of selected items. Ownership is transferred to caller
	 */
	CArrayFix<TInt>* GetCopyOfSelectionIndexesL();
       
 
	// MARK-related functions 
	void MarkCurrentItemL();
	void MarkAllL();
	void UnmarkAll();

	void ResetSearchField();
	void ResetAndHideSearchField();

	TBool IsCurrentMarked();
	TInt CountVisibleItems();

	void BeforeAppExitL();

	// phonebook_observer
	void contents_changed(TInt aContactId, TBool aPresenceOnly);	
	void before_change();
	void exiting();

	void SizeChangedForFindBox();

	// Functions that should migrate to the view
	void ShowMyRichPresenceL();
	void show_presence_details_current();
	void show_presence_description_current();
	void StoreCurrentContactIdL();
	void HandleResourceChange( TInt aType, const TRect& aRect );
	 
private:
	void ReleaseCContextContactsContainer();
	void SizeChanged();
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	
	void Draw(const TRect& aRect) const;
	void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
	void HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType);
	void ListBoxItemsChanged (CEikListBox *aListBox);
	
	CAknSearchField* CreateFindBoxL(CEikListBox* aListBox,CTextListBoxModel* aModel, CAknSearchField::TSearchFieldStyle aStyle );
	TKeyResponse OfferKeyEventL(const TKeyEvent &, TEventCode);


private:
	TContactUiDelegates iDelegates;

#ifdef __JAIKU_ENABLED__
	//	class CJaikuContactsListBox* 
	class CJuikGenListBox* iListbox;
	class CJaikuContactsListController*	iContactsList;
#else
	doublelinebox* iListbox;
	class CPresenceArray *iListBoxArray;
	class CNameArray* iNameArray;
#endif

	phonebook * iPhonebook; 	// Remove, because delegates
	class CJabberData*	iJabberData; 	// Remove, because delegates 

	CAknSearchField* iFindBox;
	
	TBool iFindBoxVisible;
	
	CAknListBoxFilterItems * iFilter;
	CArrayFix<TInt>* iCachedSelection;


	Mfile_output_base * aLog;
	CAknView * aView;
	TInt iCurrentIdx;
	TInt iTopIdx;
	TBuf<100> iSearchText, iPrevSearch;
	class CBBSubSession*	iBBSession;
};
#endif

