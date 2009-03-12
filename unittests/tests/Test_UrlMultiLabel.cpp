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

#include "Test_UrlMultiLabel.h"

#include "juik_urlmultilabel.h"
#include "symbian_auto_ptr.h"

#include <coecntrl.h>


class CParentControl : public CCoeControl
{
public:
	static CParentControl* NewL() 
	{
		auto_ptr<CParentControl> self( new (ELeave) CParentControl );
		self->CreateWindowL();
		return self.release();
	}

	void SetChildL(CCoeControl& aControl) { iChild = &aControl; }

	void SizeChanged() { iChild->SetRect( Rect() ); }
	TInt CountComponentControls() const { return iChild ? 1 : 0; }
	CCoeControl* ComponentControl(TInt aIndex) const { return iChild; }
	
	CCoeControl* iChild;
};

void CTest_UrlMultiLabelBasic::testConstructionL()
{
	auto_ptr<CParentControl> parent( CParentControl::NewL() );
	
	auto_ptr<CUrlMultiLabel> label( CUrlMultiLabel::NewL(NULL, KRgbBlack, KRgbWhite, ETrue, *parent) );
	label->EnableFocusBackgroundL( KRgbRed );
	parent->SetChildL( *label );
	parent->SetRect( TRect(TPoint(0,0), TSize(200, 2000) ) );
}


void CTest_UrlMultiLabelBasic::testUpdateTextL()
{
	auto_ptr<CParentControl> parent( CParentControl::NewL() );
	
	auto_ptr<CUrlMultiLabel> label( CUrlMultiLabel::NewL(NULL, KRgbBlack, KRgbWhite, ETrue, *parent) );
	label->EnableFocusBackgroundL( KRgbRed );
	parent->SetChildL( *label );
	parent->SetRect( TRect(TPoint(0,0), TSize(200, 2000) ) );
	label->UpdateTextL( _L("Hello World") );
}

void CTest_UrlMultiLabelBasic::setUp()
{
	MContextTestBase::setUp();
}


void CTest_UrlMultiLabelBasic::tearDown()
{
	MContextTestBase::tearDown();
}



void CTest_UrlMultiLabel::testSingleUrlL()
{
	iLabel->UpdateTextL( _L("Hello http://www.google.fi hello") );
	TS_ASSERT_EQUALS( iLabel->UrlCount(), 1);
}

void CTest_UrlMultiLabel::testMultipleUrlsL()
{
	iLabel->UpdateTextL( _L("http://www.google.fi http://www.google.fi http://www.google.fi") );
	TS_ASSERT_EQUALS( iLabel->UrlCount(), 3);
}

_LIT( KLongTextWithUrls, "I gave a talk last Sunday in the Free and Open Source Developers European Meeting (FOSDEM) 2007 on how Jaiku uses Jabber, aka XMPP, (Slides with notes, even if I dont usually do that).I realized that there was this undertone in my talk that TCP is somehow broken by GPRS, since packets get acked even if they are not received by the phone and that the connection breaks a lot and has a large latency. But thats of course not true: TCP isnt being _broken_ by this, just our mistaken view of what TCP is.TCP tends to be describe as reliable: RFC 793 introduces it as a highly reliable host-to-host protocol'; Wikipedia claims that it guarantees reliable delivery and man tcp calls it reliable as well.The trick, is that the reliability this documents talk about is quite different with what we tend to think of as reliable. TCP’s reliability means in-order delivery and integrity (no bits flipped). What people often think when they think of reliable is some vague idea of data getting to the other end. Which of course isnt the case. http://mikie.iki.fi/dissertation/intro.pdf. If you want reliability, you 0) give your data a unique identifier, 1) store persistently the data you are going to send, 2) send it to the other side 3) store the data persistently on the other side, 4) send an acknowledgement to the sender, 5) delete on the sender. TCP does something like this, but it forgets ‘persistently’, since it’s just in the OS buffers which get thrown away if you close the socket, your program, or the machine. http://google.fi So, guys and gals, TCP is just ordered: if you send x, y and z the received will never receive just x and z. But that’s all. If you want reliability’ you have to do what I described above on the application level.");




void CTest_UrlMultiLabel::testLongTextWithUrlsL()
{
	iLabel->UpdateTextL( KLongTextWithUrls );
	TS_ASSERT_EQUALS( iLabel->UrlCount(), 2);
}


void CTest_UrlMultiLabel::testEmptyTextL()
{
	iLabel->UpdateTextL( KNullDesC );
	TS_ASSERT_EQUALS( iLabel->UrlCount(), 0);
}

_LIT( KBadUrls, "http://google.fihttp://http://jaiku.com");
void CTest_UrlMultiLabel::testBadUrlsL()
{
	iLabel->UpdateTextL( KBadUrls );
// 	TInt urlCount = iLabel->UrlCount();
// 	TS_ASSERT_EQUALS( urlCount, 2);
}


void CTest_UrlMultiLabel::setUp()
{
	MContextTestBase::setUp();
	iParent =  CParentControl::NewL();
	
	iLabel = CUrlMultiLabel::NewL(NULL, KRgbBlack, KRgbWhite, ETrue, *iParent);
	iLabel->EnableFocusBackgroundL( KRgbRed );
	iParent->SetChildL( *iLabel );
	iParent->SetRect( TRect(TPoint(0,0), TSize(200, 2000) ) );
}


void CTest_UrlMultiLabel::tearDown()
{
	delete iLabel;
	delete iParent;
	MContextTestBase::tearDown();
}
