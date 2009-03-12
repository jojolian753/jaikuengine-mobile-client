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

#include "juik_icondataprovider.h"

#include "juik_iconmanager.h"

#include "symbian_auto_ptr.h"

EXPORT_C CIconDataProvider* CIconDataProvider::NewL(const TDesC& aFile, TInt aIconId, MJuikIconManager& aIconManager)
{
	auto_ptr<CIconDataProvider> self( new (ELeave) CIconDataProvider( aIconManager ) );
	self->ConstructL( aFile, aIconId );
	return self.release();
}

EXPORT_C CIconDataProvider* CIconDataProvider::NewL(const TIconID* aIconDefs, TInt aIconNb, MJuikIconManager& aIconManager)
{
	auto_ptr<CIconDataProvider> self( new (ELeave) CIconDataProvider( aIconManager ) );
	self->ConstructL( aIconDefs, aIconNb );
	return self.release();
}

EXPORT_C CIconDataProvider::~CIconDataProvider()
{
	delete iIconFile;
	iIconIds.Close();
}



EXPORT_C CArrayPtr<CGulIcon>* CIconDataProvider::GetIconsL()
{
	auto_ptr< CArrayPtr<CGulIcon> > icons( new (ELeave) CArrayPtrFlat<CGulIcon>(10));
	MStaticIconProvider* provider = iIconManager.GetStaticIconProviderL( *iIconFile );
	for (TInt i=0; i < iIconIds.Count(); i++)
		{
			CGulIcon* icon = provider->GetIconL( iIconIds[i] );
			icons->AppendL( icon );
		}
	return icons.release();
}

EXPORT_C TInt CIconDataProvider::GetIconIndexL(const TDesC& aFile, TInt aIconId) const
{
	return iIconManager.GetStaticIconProviderL( aFile )->GetListBoxIndexL( aIconId );
}


CIconDataProvider::CIconDataProvider(MJuikIconManager& aIconManager) : iIconManager(aIconManager) {}
	
	
void CIconDataProvider::ConstructL(const TDesC& aFile, TInt aIconId)
{
	iIconFile = aFile.AllocL();
	iIconIds.AppendL( aIconId );
}

void CIconDataProvider::ConstructL(const TIconID* aIconDefs, TInt aIconNb)
{
	TPtrC fileName( (TText*) aIconDefs[0].iMbmFile );
	iIconFile = fileName.AllocL();
	for (TInt i=0; i < aIconNb; i++)
		{
			iIconIds.AppendL( aIconDefs[i].iBitmap );
		}
}
