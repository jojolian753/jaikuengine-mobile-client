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

#ifndef __CCU_CONTACT_H__
#define __CCU_CONTACT_H__

#include "app_context.h"
#include "cb_presence.h"
#include <cntdef.h>


class contact : public CBase, public MContextBase
{
public:
	TContactItemId	id;
	HBufC*		first_name;
	HBufC*		last_name;
	HBufC*		extra_name;
	TInt		current_idx;
	TBool		has_nick;
	TBool		is_myself; // true, if contacts has same jaiku nick as is set as user's jaiku nick
	TBool		show_details_in_list;
	TBuf<10>	time;
	CBBPresence* presence;

	/** 
	 * Cached unread and stream count. KErrNotFound is set, when value is unknown (and should be
	 * read from db)
	 */ 
	TInt        iUnreadCount;
	TInt        iStreamCount;
	
	IMPORT_C void Name(HBufC*& Into, bool last_name_first);
	IMPORT_C contact();

	IMPORT_C static contact* NewL(TContactItemId i_id, const TDesC& i_first_name, const TDesC& i_last_name, const TDesC& i_extra_name=KNullDesC);

	
	IMPORT_C void set_presence(CBBPresence* data);
	IMPORT_C ~contact();

	// Utility methods to avoid NULL pointer checks 

	/** 
	 * @returns first name.
	 *  If first name is not set, return KNullDesC
	 */ 
	IMPORT_C TPtrC FirstName() const;
	/** 
	 * @returns last name.
	 *  If last name is not set, return KNullDesC
	 */ 
	IMPORT_C TPtrC LastName() const;

	IMPORT_C TPtrC ExtraName() const;

	IMPORT_C void SetFirstNameL(const TDesC& aName);
	IMPORT_C void SetLastNameL(const TDesC& aName);

	IMPORT_C void AppendName(TDes& aTgt, bool aLastNameFirst);
	IMPORT_C HBufC* contact::NameL(TBool aLastNameFirst);

	IMPORT_C contact* CloneL();
	
	IMPORT_C bool operator==(const contact& aRhs) const;
};

#endif
