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

#include "MessageHolder.h"
#include "symbian_auto_ptr.h"
#include "list.h"

#include "app_context.h"

TMessage_ref::TMessage_ref(HBufC* i_contact, HBufC* i_subject, HBufC* i_message) :
	contact(i_contact), subject(i_subject), message(i_message)
{
	CALLSTACKITEM_N(_CL("TMessage_ref"), _CL("TMessage_ref"));

}

TMessage::operator TMessage_ref()
{
	CALLSTACKITEM_N(_CL("TMessage"), _CL("operator"));

	HBufC *c=contact, *s=subject, *m=message;
	contact=subject=message=0;
	TMessage_ref r(c, s, m);
	return r;
}

TMessage::TMessage() : contact(0), subject(0), message(0) { }
TMessage::~TMessage() 
{ 
	CALLSTACKITEM_N(_CL("TMessage"), _CL("~TMessage"));

	delete contact; 
	delete subject;
	delete message; 
}

TMessage& TMessage::operator=(TMessage& rhs)
{
	CALLSTACKITEM_N(_CL("TMessage"), _CL("operator"));

	if (&rhs != this) {
		delete contact; contact=0;
		delete message; message=0;
		delete subject; subject=0;
		contact=rhs.contact; rhs.contact=0;
		message=rhs.message; rhs.message=0;
		subject=rhs.subject; rhs.subject=0;
	}
	return *this;
}

TMessage::TMessage(TMessage& f)
{
	CALLSTACKITEM_N(_CL("TMessage"), _CL("TMessage"));

	contact=f.contact; f.contact=0;
	message=f.message; f.message=0;
	subject=f.subject; f.subject=0;
}

TMessage& TMessage::operator=(const TMessage_ref& rhs)
{
	CALLSTACKITEM_N(_CL("TMessage"), _CL("operator"));

	delete contact; contact=0;
	delete message; message=0;
	delete subject; subject=0;
	contact=rhs.contact;
	message=rhs.message;
	subject=rhs.subject;
	return *this;
}

TMessage::TMessage(const TMessage_ref& f)
{
	CALLSTACKITEM_N(_CL("TMessage"), _CL("TMessage"));

	contact=f.contact;
	message=f.message;
	subject=f.subject;
}

class CMessageHolderImpl : public CMessageHolder
{
public: 
	virtual ~CMessageHolderImpl();
private:
	void ConstructL();

	virtual TMessage GetMessage(); // caller gets ownership
	virtual void AppendMessageL(const TDesC& contact, const TDesC& subject, const TDesC& message);
	virtual void AppendMessageL(TMessage msg); // takes ownership

	CList<TMessage>* iList;

	friend class CMessageHolder;
};

CMessageHolder * CMessageHolder::NewL()
{
	CALLSTACKITEM_N(_CL("CMessageHolder"), _CL("NewL"));

	auto_ptr<CMessageHolderImpl> ret(new (ELeave) CMessageHolderImpl);
	ret->ConstructL();
	return ret.release();
}

CMessageHolderImpl::~CMessageHolderImpl()
{
	CALLSTACKITEM_N(_CL("CMessageHolderImpl"), _CL("~CMessageHolderImpl"));

	delete iList;
}

void CMessageHolderImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CMessageHolderImpl"), _CL("ConstructL"));

	iList=CList<TMessage>::NewL();
}

TMessage CMessageHolderImpl::GetMessage()
{
	CALLSTACKITEM_N(_CL("CMessageHolderImpl"), _CL("GetMessage"));

	TMessage m(iList->Pop());
	return m;
}

void CMessageHolderImpl::AppendMessageL(const TDesC& contact, const TDesC& subject, const TDesC& message)
{
	CALLSTACKITEM_N(_CL("CMessageHolderImpl"), _CL("AppendMessageL"));

	auto_ptr<HBufC> c(contact.AllocL());
	auto_ptr<HBufC> msg(message.AllocL());
	auto_ptr<HBufC> s(subject.AllocL());

	TMessage m;
	m.contact=c.get();
	m.message=msg.get();
	m.subject=s.get();

	iList->AppendL(m);

	c.release(); msg.release(); s.release();
}

void CMessageHolderImpl::AppendMessageL(TMessage msg)
{
	CALLSTACKITEM_N(_CL("CMessageHolderImpl"), _CL("AppendMessageL"));

	iList->AppendL(msg);
}
