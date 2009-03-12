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

#ifndef CONTEXT_CCN_MESSAGE_H_INCLUDED
#define CONTEXT_CCN_MESSAGE_H_INCLUDED 1

#include <e32base.h>
#include <tpbkcontactitemfield.h>

class CMessaging;

typedef const TPbkContactItemField*(*TAddressSelectorF)(CPbkContactItem&);
typedef void(CMessaging::* TMessageSenderF)(CDesCArrayFlat*, CDesCArrayFlat*, TUid);
typedef TBool (*TPbkFieldFilterF)( const TPbkContactItemField& aField );

const TPbkContactItemField* MmsSelectorL(CPbkContactItem& aItem);
const TPbkContactItemField* SmsSelectorL(CPbkContactItem& aItem);
const TPbkContactItemField* EmailSelectorL(CPbkContactItem& aItem);

TBool SmsFieldFilterL( const TPbkContactItemField& aField );
TBool MmsFieldFilterL( const TPbkContactItemField& aField );
TBool EmailFieldFilterL( const TPbkContactItemField& aField );

class CMessaging : public CBase {
public:
	void CreateSingleMessageL( TInt aContactId, 
							   TAddressSelectorF aSelector, 
							   TMessageSenderF aSender,
							   TUid aMtm);


	void CreateMassMessageL( CArrayFix<TInt>& aContactIds,
							 TInt aWarningNoteResource,
							 TAddressSelectorF aSelector, 
							 TMessageSenderF aSender,
							 TPbkFieldFilterF aFilter,
							 TUid aMtm );

	void SmsSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid aMtm);
	void MmsSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid aMtm);
	void EmailSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid aMtm);
	void MessageSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid aMtm);
	
	static CMessaging* NewL(class phonebook* aBook, class phonehelper_ui* aPhoneHelper, class CJabberData* aJabberData);

	class phonehelper_ui *iPhoneHelper;
	class phonebook * iBook;
	class CJabberData* iJabberData;
#ifndef __S60V3__
	class	CSendAppUi	*iSendAppUi;
#else
	class 	CSendUi		*iSendAppUi;
#endif
	~CMessaging();
private:
	CMessaging(class phonebook* aBook, class phonehelper_ui* aPhoneHelper, class CJabberData* aJabberData);
	void ConstructL();
};


#endif
