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

#include <e32base.h>
#include <windows.h>
#include <detours.h>

class RHeapHook 
{
public:
	IMPORT_C TAny* HookedAlloc(TInt aSize);
	static TAny* (RHeapHook::* Alloc)(TInt aSize);
	IMPORT_C void HookedFree(TAny* aPtr);
	static void (RHeapHook::* Free)(TAny* aPtr);
	IMPORT_C TAny* HookedReAlloc(TAny* aCell,TInt aSize);
	static TAny* (RHeapHook::* ReAlloc)(TAny* ,TInt );
};

void LogAlloc(TAny* aResult, TInt aSize);
void LogFree(TAny* aPtr);
void LogReAlloc(TAny* aResult, TInt aSize, TAny* aCell);

EXPORT_C TAny* RHeapHook::HookedAlloc(TInt aSize)
{
	TAny *result= (this->*Alloc)(aSize);
	LogAlloc(result, aSize);
	return result;
}


EXPORT_C void RHeapHook::HookedFree(TAny* aPtr)
{
	LogFree(aPtr);
	(this->*Free)(aPtr);
}


EXPORT_C TAny* RHeapHook::HookedReAlloc(TAny* aCell,TInt aSize)
{
	TAny* result=(this->*ReAlloc)(aCell, aSize);
	LogReAlloc(result, aSize, aCell);
	return result;
}

#ifndef __S60V3__
TAny* (RHeapHook::* RHeapHook::Alloc)(TInt)=(TAny* (RHeapHook::* )(TInt))(&RHeap::Alloc);
void (RHeapHook::* RHeapHook::Free)(TAny*)=(void (RHeapHook::* )(TAny*))&RHeap::Free;
TAny* (RHeapHook::* RHeapHook::ReAlloc)(TAny* ,TInt )=(TAny* (RHeapHook::*)(TAny* ,TInt ))&RHeap::ReAlloc;
#else
TAny* (RHeapHook::* RHeapHook::Alloc)(TInt)=0;
void (RHeapHook::* RHeapHook::Free)(TAny*)=0;
TAny* (RHeapHook::* RHeapHook::ReAlloc)(TAny* ,TInt )=0;
#endif

void InstallHooks() {
	return;
/*
#if (_MSC_VER < 1310)
    TAny* (RHeap::* pfAlloc)(TInt) = RHeap::Alloc;
    TAny* (RHeapHook::* pfHooked)(TInt) = RHeapHook::HookedAlloc;

    Verify("RHeap::Alloc", *(PBYTE*)&pfAlloc);
    Verify("*RHeapHook::Alloc", *(PBYTE*)&RHeapHook::Alloc);
    Verify("RHeapHook::HookedAlloc", *(PBYTE*)&pfHooked);
#else
    Verify("RHeap::Alloc", (PBYTE)(&(PBYTE&)RHeap::Alloc));
    Verify("*RHeapHook:Alloc", *(&(PBYTE&)RHeapHook::Alloc));
    Verify("RHeapHook::HookedAlloc", (PBYTE)(&(PBYTE&)RHeapHook::HookedAlloc));
#endif
*/

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
#ifndef __S60V3__
	DetourAttach(&(PVOID&)RHeapHook::Alloc,
                 (PVOID)(&(PVOID&)RHeapHook::HookedAlloc));
	DetourAttach(&(PVOID&)RHeapHook::Free,
                 (PVOID)(&(PVOID&)RHeapHook::HookedFree));
	DetourAttach(&(PVOID&)RHeapHook::ReAlloc,
                 (PVOID)(&(PVOID&)RHeapHook::HookedReAlloc));
#else

	HINSTANCE euser_lib = LoadLibrary(L"euser");
	void* alloc_proc=0, *free_proc=0, *realloc_proc=0;
	if ( euser_lib != NULL ) {
		alloc_proc = GetProcAddress(euser_lib, (LPCSTR)339);
		free_proc = GetProcAddress(euser_lib, (LPCSTR)926);
		realloc_proc = GetProcAddress(euser_lib, (LPCSTR)1383);
	}
	TAny* (RHeapHook::*hooked_alloc )(TInt)=&RHeapHook::HookedAlloc;
	TInt **p=(TInt**)&hooked_alloc;
	TInt** raw=(TInt**)&RHeapHook::Alloc;
	(raw)[2]=(TInt*)alloc_proc;
	DetourAttach( (void**)&(raw[2]), (void*)(p[2]));
	
	/*
	p.Free=&RHeapHook::HookedFree;
	DetourAttach(&(PVOID&)RHeapHook::Free,
                 (void*)raw[2]);
	p.ReAlloc=&RHeapHook::HookedReAlloc;
	DetourAttach(&(PVOID&)RHeapHook::ReAlloc,
                 (void*)raw[2]);*/
#endif
	LONG l = DetourTransactionCommit();
}
