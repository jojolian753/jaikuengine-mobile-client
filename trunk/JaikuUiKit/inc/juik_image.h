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

#ifndef JUIK_IMAGE_H
#define JUIK_IMAGE_H

#include <eikimage.h>
#include <gulicon.h> 
#include "app_context.h"
#include "juik_control.h"

class CJuikImage : public CEikImage, public MJuikControl, public MContextBase
{
public:
	IMPORT_C static CJuikImage* NewL(CGulIcon* aIcon, TSize aDefaultSize);
	IMPORT_C virtual ~CJuikImage();

	IMPORT_C void SetDefaultSize(TSize aSize);
	IMPORT_C void UpdateL( CGulIcon& aIcon);
	IMPORT_C void ClearL();

public: // From CEikImage
	virtual TSize MinimumSize();
	virtual void SizeChanged();
	virtual void Draw(const TRect& aRect) const;
	
protected:
	CJuikImage( CGulIcon* aIcon, TSize aDefaultSize );
	void ConstructL();

 public: // from MJuikControl
	const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }
	
private:
	CGulIcon* iIcon; // not owned
	TSize iDefaultSize;
};

#endif // JUIK_IMAGE_H
