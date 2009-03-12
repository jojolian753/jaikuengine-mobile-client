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

#include "errorhandling.h"
#include "app_context.h"
#include "vararg_impl_macros.h"
#include "app_context_impl.h"

IMPL_VARARG(MErrorInfoManager, UserMsg, MErrorInfoManager&, const TStringArg&);
IMPL_VARARG(MErrorInfoManager, TechMsg, MErrorInfoManager&, const TStringArg&);

_LIT(KNoAppContext, "NO_APP_CONTEXT");

MApp_context& Context()
{
	MApp_context* c=GetContext();
	if (!c) User::Panic(KNoAppContext, -1);
	return *c;
}

EXPORT_C MErrorInfoManager& Bug(const TDesC& aTechMsg)
{
	MErrorInfoManager &m=Context().ErrorInfoMgr();
	m.StartErrorWithInfo( KErrorBug, KNullDesC(),
		aTechMsg, EBug, EError );
	return m;
}

EXPORT_C MErrorInfoManager& Corrupt(const TDesC& aTechMsg)
{
	MErrorInfoManager &m=Context().ErrorInfoMgr();
	m.StartErrorWithInfo( KErrorUnknown, KNullDesC(),
		aTechMsg, EBug, ECorrupt );
	return m;
}
EXPORT_C MErrorInfoManager& InputErr(const TDesC& aUserMsg)
{
	MErrorInfoManager &m=Context().ErrorInfoMgr();
	m.StartErrorWithInfo( KErrorUnknown, aUserMsg,
		KNullDesC(), EInputData, EError );
	return m;
}
EXPORT_C MErrorInfoManager& RemoteErr(const TDesC& aTechMsg)
{
	MErrorInfoManager &m=Context().ErrorInfoMgr();
	m.StartErrorWithInfo( KErrorUnknown, KNullDesC(),
		aTechMsg, ERemote, EError );
	return m;
}
EXPORT_C MErrorInfoManager& EnvErr(const TDesC& aUserMsg)
{
	MErrorInfoManager &m=Context().ErrorInfoMgr();
	m.StartErrorWithInfo( KErrorUnknown, aUserMsg,
		KNullDesC(), ELocalEnvironment, EError );
	return m;
}

EXPORT_C MErrorInfoManager& PlainErr(TInt aError)
{
	MErrorInfoManager &m=Context().ErrorInfoMgr();
	TErrorCode e={ 0, aError };
	TErrorType t=EBug;
	if (aError==KErrNoMemory) t=ELocalEnvironment;
	m.StartErrorWithInfo( e, KNullDesC(),
		KNullDesC(), t, EError );
	return m;
}
