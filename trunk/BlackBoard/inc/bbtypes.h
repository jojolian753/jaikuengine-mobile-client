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

#ifndef BB_TYPES_H_INCLUDED
#define BB_TYPES_H_INCLUDED 1

#include "bbdata.h"
#include "context_uids.rh"
#include <e32std.h>

#define KBBUidValue CONTEXT_UID_BLACKBOARDFACTORY // 0x1020811A

const TUid KBBUid = { KBBUidValue };
const TTypeName KIntType = { { KBBUidValue }, 1, 1, 0 };
const TTypeName KShortStringType = { { KBBUidValue }, 2, 1, 0 };
const TTypeName KListType = { { KBBUidValue }, 3, 1, 0 };
const TTypeName KBoolType = { { KBBUidValue }, 4, 1, 0 };
const TTypeName KTimeType = { { KBBUidValue }, 5, 1, 0 };
const TTypeName KUintType = { { KBBUidValue }, 6, 1, 0 };
const TTypeName KNullType = { { KBBUidValue }, 7, 1, 0 };
const TTypeName KGeneralType = { { KBBUidValue }, 8, 1, 0 };
const TTypeName KUidType = { { KBBUidValue }, 9, 1, 0 };
const TTypeName KLongStringType = { { KBBUidValue }, 10, 1, 0 };
const TTypeName KStringType = { { KBBUidValue }, 11, 1, 0 };
const TTypeName KInt64Type = { { KBBUidValue }, 12, 1, 0 };
const TTypeName KShortString8Type = { { KBBUidValue }, 13, 1, 0 };
const TTypeName KTupleNameType = { { KBBUidValue }, 14, 1, 0 };
const TTypeName KComponentNameType = { { KBBUidValue }, 15, 1, 0 };
const TTypeName KString8Type = { { KBBUidValue }, 16, 1, 0 };
const TTypeName KErrorInfoType = { { KBBUidValue }, 17, 1, 0 };
const TTypeName KErrorCodeType = { { KBBUidValue }, 18, 1, 0 };
const TTypeName KSeverityType = { { KBBUidValue }, 19, 1, 0 };
const TTypeName KErrorKindType = { { KBBUidValue }, 20, 1, 0 };

#define KBBAnyUidValue 0x0
const TUid KBBAnyUid = { KBBAnyUidValue };
const TInt KBBAnyId = 0x0;
const TTypeName KAnyType = { { KBBAnyUidValue }, KBBAnyId, 0, 0 };
const TComponentName KAnyComponent = { { KBBAnyUidValue }, KBBAnyId };
const TTupleName KAnyTuple = { { KBBAnyUidValue }, KBBAnyId };

#define KBBNoUidValue -0x1
const TUid KBBNoUid = { KBBNoUidValue };
const TInt KBBNoId = -0x1;
const TTypeName KNoType = { { KBBNoUidValue }, KBBNoId, 0, 0 };
const TComponentName KNoComponent = { { KBBNoUidValue }, KBBNoId };
const TTupleName KNoTuple = { { KBBNoUidValue }, KBBNoId };


#endif
