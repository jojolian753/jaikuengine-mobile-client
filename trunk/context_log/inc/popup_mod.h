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

#if !defined(__AKNPOPUPFIELDTEXT_H__)
#define __AKNPOPUPFIELDTEXT_H__

#include "AknPopupField.h"
#include "AknQueryValueText.h"


class CAknPopupFieldText : public CAknPopupField
	{
	friend class CWebForm;
public: 
	/**
	* C++ constructor
	*/
	IMPORT_C CAknPopupFieldText();

	/**
	* C++ Destructor
	*/
	IMPORT_C ~CAknPopupFieldText();

	/**
	* Gets the Current Value Text from the CAknQueryValueText member
	*/
	IMPORT_C HBufC* CurrentValueTextLC();

	/**
	* Gets the Current Value index from the CAknQueryValueText member
	*/
	IMPORT_C TInt CurrentValueIndex() const;

	/**
	* Sets the Current Value index using the CAknQueryValueText member
	*/
	IMPORT_C void SetCurrentValueIndex(const TInt aIndex);

	/**
	* Gets the MdcArray from the CAknQueryValueText member
	*/
	IMPORT_C const MDesCArray* MdcArray() const;

	/**
	* Calls the SetAutoAppend method on the CAknQueryValueText member
	*/
	IMPORT_C void SetAutoAppend(TBool aAppend);

public: // from CCoeControl
	IMPORT_C void ConstructFromResourceL(TResourceReader& aReader);

private: // Methods from CAknPopupField that were public
	/**
	* 2nd phase constructor
	*/
	void ConstructL();

	/**
	* This method should not be used. 
	* It is here to hide it from users, this will panic if you use it.
	*/
	void SetQueryValueL(MAknQueryValue* aValue);

private: // from CCoeControl
	IMPORT_C void Reserved_1();
	IMPORT_C void Reserved_2();

private: // personal
	void CommonConstructL(TInt aTextArrayResourceId, TInt aInitialIndex);

private:
	// the following members are owned
	CDesCArray* iArray;	// the array of text items
	CAknQueryValueTextArray* iTextArray;	
	CAknQueryValueText* iTextValue;
	};

#endif
// End of File
