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

#include "juik_layout_impl.h"

#include "juik_layoutitem.h"
#include "jaiku_layoutdata.h"
#include "jaiku_layoutids.hrh"

#include "app_context.h"
#include "symbian_auto_ptr.h"
#include "errorhandling.h"

#include <aknutils.h>


EXPORT_C TMargins8 Juik::Margins(TInt left, TInt right, TInt top, TInt bottom)
{
	TMargins8 m;
	m.iLeft = left;
	m.iRight = right;
	m.iTop = top;
	m.iBottom = bottom;
	return m;
}


EXPORT_C TMargins Juik::FromMargins8(TMargins8 margins)
{
	TMargins marg;
	marg.iLeft = margins.iLeft;
	marg.iRight = margins.iRight;
	marg.iTop = margins.iTop;
	marg.iBottom = margins.iBottom;
	return marg;
}


EXPORT_C TSize MJuikLayout::ScreenSize() 
{
	TSize legacy(176, 208);
	TSize s;
#if defined(__S60V2FP3__) || defined(__S60V3__)
	if ( AknLayoutUtils::LayoutMetricsSize(AknLayoutUtils::EScreen, s) )
		return s;
	else 
		return legacy;
#else
	return legacy;
#endif
}


class CJuikLayoutImpl : public CJuikLayout, public MContextBase 
{
 public:
  static CJuikLayoutImpl* NewL(TBool aReadLayoutData);
  virtual ~CJuikLayoutImpl();

  CArrayFix<TJuikLayoutItem>* GetLayoutGroupL(TInt aGroup);
  const TJuikLayoutItem& GetLayoutItemL(TInt aGroup, TInt aId);
/*   TMargins GetMarginsL( TJuikLayoutItemGroup aGroup,  TInt aId ); */
/*   TRect GetRectL( TJuikLayoutItemGroup aGroup,  TInt aId );   */
	void UpdateLayoutDataL();
	void UpdateLayoutDataL(TSize aSize);
	
private:
	void ConstructL(TBool aReadLayoutData);


 private:
  CArrayPtr< JuikLayoutData::CGroup >* iLayouts;
};


EXPORT_C CJuikLayout* CJuikLayout::NewL()
{
    CALLSTACKITEMSTATIC_N(_CL("CJuikLayout"), _CL("NewL"));
    return CJuikLayout::NewL(ETrue);
}

EXPORT_C CJuikLayout* CJuikLayout::NewL(TBool aReadLayoutData)
{
    CALLSTACKITEMSTATIC_N(_CL("CJuikLayout"), _CL("NewL(TBool)"));
	return CJuikLayoutImpl::NewL(aReadLayoutData);
}


JuikLayoutData::TConfiguration Configuration( TSize aSize ) 
{
	TSize legacyPortrait(176,208);
	TSize legacyLandscape(208,176);
	TSize doublePortrait(352,416);
	TSize doubleLandscape(416,352);
	TSize qvgaPortrait(240,320);
	TSize qvgaLandscape(320,240);
	TSize e90(800,352);

	TSize s = aSize;
	if ( s == legacyPortrait ) return JuikLayoutData::ELegacyPortrait;
	if ( s == legacyLandscape ) return JuikLayoutData::ELegacyLandscape;
	if ( s == doublePortrait ) return JuikLayoutData::EDoublePortrait;
	if ( s == doubleLandscape ) return JuikLayoutData::EDoubleLandscape;
	if ( s == qvgaPortrait )   return JuikLayoutData::EQvgaPortrait;
	if ( s == qvgaLandscape )   return JuikLayoutData::EQvgaLandscape;
	if ( s == e90 )   return JuikLayoutData::EE90Inner;
	return JuikLayoutData::ELegacyPortrait;
}


CJuikLayoutImpl* CJuikLayoutImpl::NewL(TBool aReadLayoutData)
{
    CALLSTACKITEMSTATIC_N(_CL("CJuikLayoutImpl"), _CL("NewL(TBool)"));
	auto_ptr<CJuikLayoutImpl> self( new (ELeave) CJuikLayoutImpl );
	self->ConstructL(aReadLayoutData);
	return self.release();
}


void CJuikLayoutImpl::ConstructL(TBool aReadLayoutData)
{	
    CALLSTACKITEM_N(_CL("CJuikLayoutImpl"), _CL("ConstructL"));
	if ( aReadLayoutData )
		UpdateLayoutDataL();
}


void CJuikLayoutImpl::UpdateLayoutDataL(TSize aSize)
{
    CALLSTACKITEM_N(_CL("CJuikLayoutImpl"), _CL("UpdateLayoutDataL(TSize)"))
	if ( iLayouts )
		{
			iLayouts->ResetAndDestroy();
			delete iLayouts;
			iLayouts = NULL;
		}
	iLayouts = JuikLayoutData::ReadLayoutDataL( Configuration(aSize) );
}

void CJuikLayoutImpl::UpdateLayoutDataL()
{
    CALLSTACKITEM_N(_CL("CJuikLayoutImpl"), _CL("UpdateLayoutDataL"))
	UpdateLayoutDataL( MJuikLayout::ScreenSize() );
}

CJuikLayoutImpl::~CJuikLayoutImpl()
{
	if ( iLayouts ) 
		iLayouts->ResetAndDestroy();
	delete iLayouts;
}



CArrayFix<TJuikLayoutItem>* CJuikLayoutImpl::GetLayoutGroupL(TInt aGroup)
{
    CALLSTACKITEM_N(_CL("CJuikLayoutImpl"), _CL("GetLayoutGroupL"));
	for (TInt i=0; i < iLayouts->Count(); i++)
		{
			JuikLayoutData::CGroup* l = iLayouts->At(i);
			if ( l->iGroupId == aGroup ) 
				{
					return l->iData;
				}
		}
	User::Leave( KErrNotFound );
	return NULL;
}


const TJuikLayoutItem& CJuikLayoutImpl::GetLayoutItemL(TInt aGroup, TInt aId)
{
    CALLSTACKITEM_N(_CL("CJuikLayoutImpl"), _CL("GetLayoutItemL"));
	CArrayFix<TJuikLayoutItem>* group = GetLayoutGroupL( aGroup );
	if ( aId >= group->Count() || aId < 0)
		Bug(_L("Unknown layout id")).Raise();
	return group->At( aId );
}

