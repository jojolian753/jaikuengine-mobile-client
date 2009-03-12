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

#ifndef CC_CALLSTACK_H_INCLUDED
#define CC_CALLSTACK_H_INCLUDED 1

class MCallStackVisitor {
public:
	virtual void BeginStack() = 0;
	virtual void VisitItem(const TDesC& Name) = 0;
	virtual void EndStack() = 0;
};

class MCallStack {
public:
	virtual const char* CallStack() = 0;
	virtual TInt PushCallStack(const TDesC8& Name) = 0;
	virtual TInt PushCallStack(const TDesC8& Class, const TDesC8& Func) = 0;
	virtual TPtr8 PopCallStack() = 0;
	virtual void ResetCallStack() = 0;
	virtual HBufC* GetFormattedCallStack(const TDesC& Prefix) = 0;
	virtual void IterateStack(MCallStackVisitor& aVisitor, const char* aStack=0) = 0;
	virtual TPtrC8 GetCurrentClass() = 0;
	virtual TInt PushAllocCallStack(const TDesC8& Name) = 0;
	virtual TInt PushAllocCallStack(const TDesC8& Class, const TDesC8& Func) = 0;
	virtual TPtr8 PopAllocCallStack() = 0;
	virtual TBool IsExiting(const char* aStack=0) = 0;
	virtual void SetIsExiting(TBool aValue) = 0;
	virtual TInt CallStackDepth() const = 0;
};

#endif

