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

#ifndef CONTEXT_CU_ERRORUI_H_INCLUDED
#define CONTEXT_CU_ERRORUI_H_INCLUDED 1

#ifndef __S80__
#include <aknview.h>
#else
#include "avkon_compat.h"
#endif

#include <bamdesca.h>


/*
 * Formats error information into rich text
 * First are shown the user-friendly texts for all errors,
 * then the technical info. If more than one error info node
 * exists, the user-friendly texts are repeated by the technical
 * ones for understandability
 */

IMPORT_C void ErrorIntoRichTextL(class CRichText& aInto, const class MErrorInfo& aFrom);

const TUid KErrorInfoView = { 0x1003 };
const TUid KStatusListView = { 0x1004 };

/*
 * not done yet -MR

class MStatusListObserver {
public:
	virtual void ItemsChanged() = 0;
	virtual void ItemsAdded() = 0;
	virtual void ItemsRemoved() = 0;
};

class MStatusList : public MDesCArray {
public:
	virtual const MErrorInfo* ErrorInfo(TInt aIndex) const = 0;
	virtual void SetObserver(MStatusListObserver* aObserver) = 0;
};

class CStatusListView : public CAknView {
public:
	IMPORT_C static CStatusListView* NewL();

	virtual void ShowWithData(MStatusList& aInfo) = 0;
};
*/

class CErrorInfoView : public CAknView {
public:
	IMPORT_C static CErrorInfoView* NewL();

	virtual void ShowWithData(const MErrorInfo& aInfo) = 0;
	virtual void HandleResourceChange( TInt aType ) = 0;
};

class CErrorInfoContainer : public CCoeControl 
{
 public:
	IMPORT_C static CErrorInfoContainer* NewL(MObjectProvider* aMopParent, const TRect& aRect);
	virtual ~CErrorInfoContainer() {}
	virtual class CRichText* RichText() = 0;
	virtual void HandleTextChangedL() = 0;
	virtual TInt TextLength() = 0;
};

#endif
