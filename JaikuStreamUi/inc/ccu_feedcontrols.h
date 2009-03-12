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

#ifndef CCU_FEEDCONTROLS_H
#define CCU_FEEDCONTROLS_H

#include <e32base.h>
#include <coecntrl.h>

#include "app_context.h"
#include "ccu_feedfoundation.h"
#include "contextvariant.hrh"

const TTypeName KPostInStreamControlType     = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 100, 1, 0 };
const TTypeName KCommentInStreamControlType  = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 101, 1, 0 };
const TTypeName KRssItemInStreamControlType  = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 102, 1, 0 };
const TTypeName KCommentButtonControlType    = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 103, 1, 0 };
const TTypeName KCommentInPostControlType    = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 104, 1, 0 };
const TTypeName KAuthorHeaderControlType     = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 105, 1, 0 };
const TTypeName KButtonControlType           = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 106, 1, 0 };
const TTypeName KIndividualPostControlType   = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 107, 1, 0 };
const TTypeName KMissingPostControlType      = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 108, 1, 0 };

class CPostControl : public CBubbledStreamItem
{

public:
	static CPostControl* NewL(const TUiDelegates& aDelegates, TInt aFlags = 0);

	virtual ~CPostControl() {}
	
 protected:
 CPostControl(const TUiDelegates& aDelegates, TInt aFlags) 
	 :  CBubbledStreamItem( aDelegates, aFlags ) {}
};


class CFeedCommentControl : public CBubbledStreamItem
{

public:
	static CFeedCommentControl* NewL(const TUiDelegates& aDelegates, TInt aFlags = 0);

	virtual ~CFeedCommentControl() {}
	
 protected:
 CFeedCommentControl(const TUiDelegates& aDelegates, TInt aFlags) 
	 :  CBubbledStreamItem( aDelegates, aFlags) {}
};



class CRssItemControl : public CBubbledStreamItem
{

public:
	static CRssItemControl* NewL(const TUiDelegates& aDelegates, TInt aFlags);

	virtual ~CRssItemControl() {}
	
 protected:
 CRssItemControl(const TUiDelegates& aDelegates, TInt aFlags) 
	 :  CBubbledStreamItem( aDelegates, aFlags ) {}
};


class CButtonControl : public CGeneralFeedControl
{
 public:
	static CButtonControl* NewL(TInt aId, const TDesC& aText, 
								const TUiDelegates& aDelegates, TInt aFlags = 0);

	CButtonControl(const TUiDelegates& aDelegates, TInt aFlags = 0) 
		: CGeneralFeedControl(aDelegates, aFlags) {}
	
	virtual ~CButtonControl() {}

	virtual void UpdateL() = 0;
};


class CIndividualPostControl : public CBubbledStreamItem
{

public:
	static CIndividualPostControl* NewL(const TUiDelegates& aDelegates, TInt aFlags = 0);

	virtual ~CIndividualPostControl() {}
	
 protected:
 CIndividualPostControl(const TUiDelegates& aDelegates, TInt aFlags) 
	 :  CBubbledStreamItem( aDelegates, aFlags ) {}
};


class CMissingPost : public CBubbledStreamItem
{

public:
	static CMissingPost* NewL(const TUiDelegates& aDelegates, TInt aFlags = 0);

	virtual ~CMissingPost() {}
	
	void UpdateViaChildL(CBBFeedItem& aChildItem);
	
 protected:
 CMissingPost(const TUiDelegates& aDelegates, TInt aFlags) 
	 :  CBubbledStreamItem( aDelegates, aFlags ) {}
};

#ifdef __JAIKU_PHOTO_DOWNLOAD__
class CMediaPostControl : public CBubbledStreamItem
{

public:
	static CMediaPostControl* NewL(CCoeControl& aParent, const TUiDelegates& aDelegates, TInt aFlags = 0);

	virtual ~CMediaPostControl() {}
	
 protected:
 CMediaPostControl(const TUiDelegates& aDelegates, TInt aFlags) 
	 :  CBubbledStreamItem( aDelegates, aFlags ) {}
};
#endif // __JAIKU_PHOTO_DOWNLOAD__

#endif // CCU_FEEDCONTROLS_H
