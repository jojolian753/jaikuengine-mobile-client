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

#ifndef CONTEXTBOOKCONTAINER_H
#define CONTEXTBOOKCONTAINER_H

// INCLUDES
#include <coecntrl.h>
#include <aknlists.h>
#include <cntitem.h>
#include <e32base.h>
#include <CPbkContactEngine.h>
#include <eikedwin.h>
#include <sendui.h>
#include <aknglobalnote.h>
#include <cpbkcontactengine.h> 
#include <akniconarray.h>

#include "phonehelper.h"
#include "phonebook.h"

#include "file_output_base.h"
#include "icons.h"

// FORWARD DECLARATIONS
class CEikLabel;        // for example labels

class CContextbookContainer : public CCoeControl, MCoeControlObserver, MEikListBoxObserver, MEikEdwinObserver, public phonebook_observer
{
public: // Constructors and destructor
        
        /**
        * EPOC default constructor.
        * @param aRect Frame rectangle for container.
        */
        void ConstructL(const TRect& aRect, phonebook_i* i_book, bool i_searchable, const TDesC& title, Cfile_output_base * aLog, CAknIconArray * aIconlist, TInt current_item_index, TInt top_item_index,  TBuf<20> current_filter);
		
        /**
        * Destructor.
        */
        ~CContextbookContainer();
	
public: // New functions
	void call_current();
	void sms_current();
	void show_presence_details_current();
	TInt get_current_idx();
	TInt get_top_idx();
	void GetCurrentFilter(TDes& aBuffer);
	void ResetSearchField();
	void filter();
public: // Functions from base classes

	// phonebook_observer
	void contents_changed();	
	void before_change();

	void exiting();
private: // Functions from base classes
	
	 /**
	 * From CoeControl,SizeChanged.
        */
        void SizeChanged();
	
	/**
        * From CoeControl,CountComponentControls.
        */
        TInt CountComponentControls() const;
	
	/**
        * From CCoeControl,ComponentControl.
        */
        CCoeControl* ComponentControl(TInt aIndex) const;
	
	/**
        * From CCoeControl,Draw.
        */
        void Draw(const TRect& aRect) const;
	
	/**
        * From ?base_class ?member_description
        */
        // event handling section
        // e.g Listbox events
        void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
	
        void HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType);
	
        TKeyResponse OfferKeyEventL(const TKeyEvent &, TEventCode);
	
	void HandleEdwinEventL(CEikEdwin* aEdwin,TEdwinEvent aEventType);

	virtual bool is_searchable() const; // whether to include the search box
	
	

private: //data
        
	CEikFormattedCellListBox* listbox;
	CEikEdwin * edit;
	CArrayFixFlat<TInt>* resource_files;
	phonehelper* phone;
	phonebook_i* book;
	CSendAppUi* sendui;
	CAknGlobalNote* globalNote;
	CPbkContactEngine* pbkengine;
	bool owns_engine;
	bool searchable;

	TInt	iCurrentContactId;

	Cfile_output_base * iLog;
	CAknIconArray * iconlist;
};

#endif

// End of File
