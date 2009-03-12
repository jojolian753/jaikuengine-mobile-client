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

#ifndef CB_DBLLINEBOX_H_INCLUDED
#define CB_DBLLINEBOX_H_INCLUDED

#include "contextvariant.hrh"
#ifdef __JAIKU__
#error "Header shouldn't be used in Jaiku variant"
#endif 

#include <aknlists.h>
#include <eikfrlbd.h> 
#include "phonebook.h"
#include "file_output_base.h"

class CPresenceModel : public CAknFilteredTextListBoxModel
{
public:
	IMPORT_C static CPresenceModel* NewL(MDesCArray* aNameArray, MDesCArray* aPresenceArray);
	IMPORT_C void SetFilter(CAknListBoxFilterItems * aFilter);
private:
	CPresenceModel(MDesCArray* aNameArray, MDesCArray* aPresenceArray);
	void ConstructL();
	virtual TPtrC ItemText(TInt aItemIndex) const;
	virtual TInt NumberOfItems() const;
	virtual const MDesCArray* MatchableTextArray() const;

	MDesCArray* iNameArray;
	MDesCArray* iPresenceArray;
	CAknListBoxFilterItems * iFilter;
};

class doublelinebox : public CEikFormattedCellListBox
{		
public:
	IMPORT_C doublelinebox(phonebook_i * aBook, Mfile_output_base * aLog);
	IMPORT_C void ConstructL(MDesCArray* aNameArray, MDesCArray* aPresenceArray, 
		const CCoeControl* aParent,TInt aFlags);
	IMPORT_C TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	IMPORT_C ~doublelinebox();
	IMPORT_C void LogVisibleItems(TInt currentItemIndex);
	
	virtual void SizeChanged();
	CPresenceModel *iPresenceModel;
	
private:
	void CreateItemDrawerL(void);
	void LogItem(const TDesC& item);

	TInt AddSubCell(class COptionalFormattedCellListBoxData* itemd,
		TMargins marg, TSize size, TPoint pos, TInt baselinepos, 
		CGraphicsContext::TTextAlign aAlign, TBool aGraphic=EFalse,
		TInt aGrowIndex=-1);
private:
	phonebook_i * book;
	Mfile_output_base * iLog;
	TBuf<800> iBuf;
	TInt iLastCurrentItemIndex;
	class COptionalFormattedCellListBoxData* itemd;
	
	TInt iCells, iOffset;

	friend class CContextContactsContainer;
	friend class CContextCallLogContainer;
	
};

#endif
