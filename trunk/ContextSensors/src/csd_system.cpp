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

#include "csd_system.h"

_LIT(KUid, "uid");

EXPORT_C const TTypeName& TBBSysEvent::Type() const
{
	return KSysEventType;
}

EXPORT_C TBool TBBSysEvent::Equals(const MBBData* aRhs) const
{
	const TBBSysEvent* rhs=bb_cast<TBBSysEvent>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBSysEvent::StaticType()
{
	return KSysEventType;
}

EXPORT_C const MBBData* TBBSysEvent::Part(TUint aPartNo) const
{
	if (aPartNo==0) return &iUid;
	if (aPartNo==1) return &iState;
	return 0;
}

EXPORT_C TBBSysEvent::TBBSysEvent(const TDesC& aName) : TBBCompoundData(aName),
	iUid(KUid), iState(KState)
{
}

EXPORT_C bool TBBSysEvent::operator==(const TBBSysEvent& aRhs) const
{
	return (
		iUid == aRhs.iUid &&
		iState == aRhs.iState
		);
}

_LIT(KSpace, " ");

EXPORT_C const TDesC& TBBSysEvent::StringSep(TUint aBeforePart) const
{
	if (aBeforePart==0 || aBeforePart>1) return KNullDesC;
	return KSpace;
}

EXPORT_C TBBSysEvent& TBBSysEvent::operator=(const TBBSysEvent& aSysEvent)
{
	iUid()=aSysEvent.iUid();
	iState()=aSysEvent.iState();
	return *this;
}

EXPORT_C MBBData* TBBSysEvent::CloneL(const TDesC& Name) const
{
	TBBSysEvent* ret=new (ELeave) TBBSysEvent(Name);
	*ret=*this;
	return ret;
}
