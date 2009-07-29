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

#include "break.h"
#ifdef __WINS__
#include <basched.h>
#include <e32base.h>
#include "app_context.h"
#include "app_context_impl.h"
#include "callstack.h"

TInt GetLeaveCode(TBreakItem* i) {
#if defined(__LEAVE_EQUALS_THROW__)
  // On OS versions 9 and above the parameter given to TRAP()
  // is only filled once the stack has been unwound. To get the
  // leave code during the unwind we walk up the stack to the
  // User::Leave function and grab its argument.

  // get address of real User::Leave code
  // The address of the function is to the jump table
  char* user_leave_p = (char*)&User::Leave;
  // dereference the indirect jump address after the mov
  // opcode (FF 25).
  int user_leave = *(int*)*(int*)(user_leave_p + 2);
  
  // walk the stack
  // Get current ebp (stack frame pointer)
  int ebp_value = 0;
  __asm {
    mov [ebp_value], ebp;
  }
  int* current_fp = (int*)ebp_value;
  while (true) {
    // return address of caller is at stack frame +1
    int ret_addr = *(current_fp + 1);
    // go up one stack frame
    current_fp = (int*)*((int *)current_fp);
    // check if return address is within the User::Leave function
    if (ret_addr > user_leave && ret_addr < user_leave + 200) {
      // first argument is above the return address
      return *(current_fp + 2);
    }
  }
  // We won't reach here: should we not find the leave on the stack
  // we'll crash.
#else
  return i->iError;
#endif
}

void BreakOnAllLeaves2(void* aArg)
{
	TBreakItem* i=(TBreakItem*) aArg;
#ifdef __LEAVE_EQUALS_THROW__
	i->iPopped=ETrue;
#endif
	TInt error=GetLeaveCode(i);
	if (i->ctx) i->ctx->iCurrentBreakItem=i->iPreviousItem;
	void *cname=&(i->iInClass);
	if (error == KErrNoMemory) return;
	if (error == KLeaveExit) return;
	do {
		if (error==i->iDontBreakOn) return;
		i=i->iPreviousItem;
	} while(i);
	BreakOnAllLeaves(cname);
}

EXPORT_C void BreakOnAllLeaves(void* aArg)
{
	TPtrC8* classname=(TPtrC8*)aArg;
	MApp_context* c;
	if (classname->Length()>0) {
		c=GetContext();
		if (c && classname->Compare(c->CallStackMgr().GetCurrentClass())!=0) {
			BreakInClassModule();
		}
	} else {
		BreakInClassModule();
	}
}

EXPORT_C void BreakInClassModule()
{
}

EXPORT_C void BreakInActiveObject()
{
}

EXPORT_C TBreakItem::TBreakItem(MApp_context* c, TInt& aError, TInt aDontBreakOn) : iInClass(0, 0), iError(aError), iDontBreakOn(aDontBreakOn)
{ 
	ctx=c;
	if (c) { 
		iInClass.Set(c->CallStackMgr().GetCurrentClass()); 
		iPreviousItem=c->iCurrentBreakItem;
		c->iCurrentBreakItem=this;
	} else {
		iPreviousItem=0;
	}
	CleanupStack::PushL(TCleanupItem(BreakOnAllLeaves2, this));
}

EXPORT_C TBreakItem::~TBreakItem()
{
	if (ctx) { ctx->iCurrentBreakItem=iPreviousItem; }
#ifndef __LEAVE_EQUALS_THROW__
	CleanupStack::Pop();
#else
	if (! std::uncaught_exception()) 
		CleanupStack::Pop();
	else {
		TInt x;
		x=0;
	}
#endif
}
#endif
