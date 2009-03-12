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

#ifndef CONTEXTBOOKVIEW_H
#define CONTEXTBOOKVIEW_H

// INCLUDES
#include <aknview.h>
#include "phonebook.h"
#include "jabberdata.h"
#include "file_output_base.h"
#include <akniconarray.h> 

// CONSTANTS
// UID of view
const TUid KViewId = {1};
const TUid KView2Id = {2};

// FORWARD DECLARATIONS
class CContextbookContainer;

// CLASS DECLARATION

/**
*  CContextbookView view class.
* 
*/
class CContextbookView : public CAknView
    {
    public: // Constructors and destructor

	    CContextbookView(CJabberData& JabberData, Cfile_output_base * aLog) : iJabberData(JabberData), iLog(aLog), exiting (false) { }
        void ConstructL(TUid i_id, phonebook_i* i_book, bool i_searchable, const TDesC& i_title, CAknIconArray * aIconlist);

	TInt GetCurrentIdx();
        /**
        * Destructor.
        */
        ~CContextbookView();

	
    public: // Functions from base classes
        
        /**
        * From ?base_class ?member_description
        */
        TUid Id() const;

        /**
        * From ?base_class ?member_description
        */
        void HandleCommandL(TInt aCommand);

        /**
        * From ?base_class ?member_description
        */
        void HandleClientRectChange();

		void ResetSearchField();

		void before_exit();



    private:

        /**
        * From AknView, ?member_description
        */
        void DoActivateL(const TVwsViewId& aPrevViewId,TUid aCustomMessageId,
            const TDesC8& aCustomMessage);

        /**
        * From AknView, ?member_description
        */
        void DoDeactivate();

	virtual CContextbookContainer* create_container();
	void set_nick();
	void ShowEditor();

    private: // Data
        CContextbookContainer* iContainer;
	TUid id;
	phonebook_i* book;
	bool searchable;
	TBuf<50> title;
	CJabberData& iJabberData;

	TInt current_item_index;
	TInt top_item_index;
	TBuf<20> current_edit_filter;


	Cfile_output_base * iLog;

	CAknIconArray * iconlist;
	bool exiting;

	};

#endif

// End of File
