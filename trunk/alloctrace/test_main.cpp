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

#include <f32file.h>
#include "symbian_auto_ptr.h"
#include "app_context_impl.h"

void run_basic_and_interleaving(void)
{
	{
		CALLSTACKITEM_N(_CL(""), _CL("a0_top_1000"));
		char* p=(char*)User::Alloc(1000);
		User::Free(p);
	}
	char *a1=0;
	{
		CALLSTACKITEM_N(_CL(""), _CL("a1_top_1000"));
		a1=(char*)User::Alloc(1000);
	}
	User::Free(a1);
	char *a2=0;
	{
		CALLSTACKITEM_N(_CL(""), _CL("a2_top_1000"));
		a2=(char*)User::Alloc(1000);
	}
	{
		CALLSTACKITEM_N(_CL(""), _CL("a3_top_1000"));
		char* p=(char*)User::Alloc(1000);
		User::Free(p);
	}
	User::Free(a2);
}

void run_nested(void)
{
	{
		CALLSTACKITEM_N(_CL(""), _CL("b0_top_1000_own_0"));
		{
			CALLSTACKITEM_N(_CL(""), _CL("b0_top_1000_own_1000"));
			char* p=(char*)User::Alloc(1000);
			User::Free(p);
		}
	}
	{
		CALLSTACKITEM_N(_CL(""), _CL("b10_top_1000_own_0"));
		{
			CALLSTACKITEM_N(_CL(""), _CL("b11_top_1000_own_0"));
			{
				CALLSTACKITEM_N(_CL(""), _CL("b12_top_1000_own_1000"));
				char* p=(char*)User::Alloc(1000);
				User::Free(p);
			}
		}
	}
}

void run_seq(void)
{
	{
		CALLSTACKITEM_N(_CL(""), _CL("c0_top_250_own_250_count_4"));
		char* p=0;
		p=(char*)User::Alloc(250);
		User::Free(p); p=0;
		p=(char*)User::Alloc(250);
		User::Free(p); p=0;
		p=(char*)User::Alloc(250);
		User::Free(p); p=0;
		p=(char*)User::Alloc(250);
		User::Free(p); p=0;
	}
	{
		CALLSTACKITEM_N(_CL(""), _CL("c1_top_1000_own_1000_count_4"));
		char *p0=0, *p1=0, *p2=0, *p3=0;
		p0=(char*)User::Alloc(250);
		p1=(char*)User::Alloc(250);
		p2=(char*)User::Alloc(250);
		p3=(char*)User::Alloc(250);
		User::Free(p0); p0=0;
		User::Free(p1); p1=0;
		User::Free(p2); p2=0;
		User::Free(p3); p3=0;
	}
}

void run_realloc(void)
{
	{
		CALLSTACKITEM_N(_CL(""), _CL("d0_top_1000_own_1000_count_2"));
		char *p=0;
		p=(char*)User::Alloc(250);
		p=(char*)User::ReAlloc(p, 1000);
		User::Free(p);
	}
}

#ifdef __S60V3__
#include "allocator.h"
#endif

void run_tests(void)
{
#ifdef __S60V3__
	RAllocator* orig=SwitchToLoggingAllocator();
#endif
	RHeap *h=&(User::Heap());
	TAny* (RHeap::* p)(TInt)=h->Alloc;
	auto_ptr<CApp_context> c(CApp_context::NewL(false, _L("alloctest")));
	run_basic_and_interleaving();
	run_nested();
	run_seq();
	run_realloc();
#ifdef __S60V3__
	SwitchBackAllocator(orig);
#endif
}

GLDEF_C int E32Main(void)
{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	TRAPD(err, run_tests());
	delete cleanupStack;
	return 0;
}

//extern "C" {
#ifndef __S60V3__
	void __stdcall _E32Startup(void);
#else
	void __stdcall _E32Bootstrap(void);
#endif
	void InstallHooks(void);

	void __stdcall _TraceStartup(void) {
		InstallHooks();
		
#ifndef __S60V3__
		_E32Startup();
#else
		_E32Bootstrap();
#endif
	}
//};
