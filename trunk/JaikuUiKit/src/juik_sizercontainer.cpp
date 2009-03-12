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

#include "juik_sizercontainer.h"

#include "juik_debug.h"
#include "juik_layout.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"


//
// CJuikContainerBase
// 


class CJuikSizerContainerImpl : public CJuikSizerContainer, public MContextBase
{
public:
	CJuikSizerContainerImpl()
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("CJuikSizerContainerImpl"));
#ifdef JUIK_DEBUGGING_ENABLED	
	iDebugId = KErrNotFound;
#endif
	}

	~CJuikSizerContainerImpl()
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("~CJuikSizerContainerImpl"));
		for (TInt i=0; i < iSizers.Count(); i++)
			delete iSizers[i].iItem;
		iSizers.Close();
		
		for (TInt i=0; i < iControls.Count(); i++)
			delete iControls[i].iItem;
		iControls.Close();

		delete iRootSizer;
	}

	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("ConstructL"));
	}

	void SetRootSizerL(MJuikSizer* aSizer)
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("SetRootSizerL"));
		if ( iRootSizer ) 
			{
				delete aSizer;
				Bug( _L("Already in use") ).Raise();
			}
		iRootSizer = aSizer;
	}

	MJuikSizer* GetRootSizerL()
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("GetRootSizerL"));
		if ( !iRootSizer ) Bug( _L("Root sizer not initialized" ) ).Raise();
		return iRootSizer;
	}
	
	MJuikSizer* GetSizerL(TInt aSizerId)
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("GetSizerL"));
		for (TInt i=0; i < iSizers.Count(); i++)
			{
				TIdHolder<MJuikSizer> t = iSizers[i];
				if ( t.iId == aSizerId )
					return t.iItem;
			}
		return NULL;
	}
	
	MJuikControl* GetControlL(TInt aId)
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("GetControlL"));
		for (TInt i=0; i < iControls.Count(); i++)
			{
				TIdHolder<MJuikControl> t = iControls[i];
				if ( t.iId == aId )
					return t.iItem;
			}
		return NULL;
	}
	
	void AddControlL(MJuikControl* aControl, TInt aControlId)
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("AddControlL"));
		auto_ptr<MJuikControl> ctrl( aControl );
		
		{ // Assert that it's not there
			MJuikControl* c = GetControlL( aControlId );
			if ( c ) Bug( _L("Already inserted") ).Raise();
		}
		ctrl->CoeControl()->SetContainerWindowL( *this );
		iControls.Append( TIdHolder<MJuikControl>( aControlId, ctrl.get() ) );
		ctrl.release();
	}

	void AddSizerL(MJuikSizer* aSizer, TInt aSizerId)
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("AddSizerL"));
		//sizer( aSizer );
		
		{ // Assert that it's not there
			MJuikSizer* c = GetSizerL( aSizerId );
			if ( c )
				{
					delete aSizer;
					Bug( _L("Already inserted") ).Raise();
				}
		}
		
		iSizers.Append( TIdHolder<MJuikSizer>( aSizerId, aSizer ) );
		//		sizer.release();
	}
	
	void RemoveControlL(MJuikControl* aControl) 
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("RemoveControlL"));
		for (TInt i=0; i < iControls.Count(); i++)
			{
				TIdHolder<MJuikControl> holder = iControls[i];
				if ( holder.iItem  == aControl )
					{
						delete holder.iItem;
						iControls.Remove(i);
						break;
					}
			}
	}

protected:
	void FocusChanged(TDrawNow aDrawNow)
	{
		for (TInt i=0; i < iControls.Count(); i++)
			{
				iControls[i].iItem->CoeControl()->SetFocus(IsFocused(), aDrawNow );
			}
	}

	TSize MinimumSize()
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("MinimumSize"));
#ifdef JUIK_DEBUGGING_ENABLED
		DebugPrint(_L("SizerContainer::MinimumSize"));
#endif
		if ( iRootSizer )
			return iRootSizer->MinSize();
		return TSize(0,0);
	}

	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("PositionChanged"));
		if ( iRootSizer )
			{
				TPoint p = Position();
				iRootSizer->SetPositionL( p );
			}
	}
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("SizeChanged"));
		if ( iRootSizer )
			{
#ifdef JUIK_DEBUGGING_ENABLED
				DebugPrint(_L("SizerContainer::SizeChanged"));
#endif
				TRect r = Rect();
				iRootSizer->SetMinSize( TSize(r.Size().iWidth, 0 ) );
				iRootSizer->SetDimensionL( r.iTl, r.Size() );
			}
	}
	
	TInt CountComponentControls() const
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("CountComponentControls"));
	    return iControls.Count();
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainerImpl"), _CL("ComponentControl"));
		return iControls[aIndex].iItem->CoeControl();
	}
	


	virtual void SetDebugId(TInt aDebugId) 
	{
#ifdef JUIK_DEBUGGING_ENABLED
		iDebugId = aDebugId;
#endif
	}
	
#ifdef JUIK_DEBUGGING_ENABLED
	void DebugPrint(const TDesC& aMsg) const
	{
		if ( iDebugId != KErrNotFound )
			{
				TRect r = Rect();
				TBuf<200> buf;
				buf.Format( _L("%S: %d (%d,%d),(%d,%d) [%d,%d]"), &aMsg, 
							iDebugId, r.iTl.iX, r.iTl.iY, r.iBr.iX, r.iBr.iY, 
							r.Width(), r.Height() );
				const_cast<CJuikSizerContainerImpl*>(this)->Reporting().DebugLog( buf );
			}
	}

	TInt iDebugId;
#endif 


	void Draw(const TRect& aRect) const
	{
#ifdef JUIK_BOUNDINGBOXES
		CWindowGc& gc = SystemGc();
		JuikDebug::DrawBoundingBox(gc ,Rect());
#endif
	}
	
	void  SetContainerWindowL(const CCoeControl& aControl)
	{
		CALLSTACKITEM_N(_CL("CJuikSizerContainer"), _CL("SetContainerWindowL"));
		CCoeControl::SetContainerWindowL( aControl );
		// Propagate change to children
		for ( TInt i=0; i < iControls.Count(); i++ )
			iControls[i].iItem->CoeControl()->SetContainerWindowL( *this );
	}
   

private: // from MJuikControl
	const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }

private:
	template <class T> struct TIdHolder {
		TIdHolder( TInt aId, T* aItem) : iId(aId), iItem(aItem) {}
		TInt iId;
		T* iItem;
	};
  	
	RArray< TIdHolder<MJuikSizer> > iSizers;
	RArray< TIdHolder<MJuikControl> > iControls;

	MJuikSizer* iRootSizer;
};


EXPORT_C CJuikSizerContainer* CJuikSizerContainer::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CJuikSizerContainer"), _CL("NewL"));
	auto_ptr<CJuikSizerContainerImpl> self( new (ELeave) CJuikSizerContainerImpl() );
	self->ConstructL();
	return self.release();
}



//
// CJuikContainerBase
// 


EXPORT_C CJuikContainerBase::CJuikContainerBase()
{
	iMargin = Juik::Margins(0,0,0,0);
}

EXPORT_C CJuikContainerBase::~CJuikContainerBase()
{
	delete iContainer;
}

EXPORT_C void CJuikContainerBase::BaseConstructL()
{
	CALLSTACKITEM_N(_CL("CJuikContainerBase"), _CL("BaseConstructL"));
	iContainer = CJuikSizerContainer::NewL();
	iContainer->SetContainerWindowL( *this );
}

EXPORT_C CJuikSizerContainer& CJuikContainerBase::RootContainer() const
{
	CALLSTACKITEM_N(_CL("CJuikContainerBase"), _CL("RootContainer"));
	return *iContainer;
}

EXPORT_C void CJuikContainerBase::FocusChanged(TDrawNow aDrawNow)
{
	CALLSTACKITEM_N(_CL("CJuikContainerBase"), _CL("FocusChanged"));
	iContainer->SetFocus(IsFocused(), aDrawNow );
}

EXPORT_C TSize CJuikContainerBase::MinimumSize()
{
	CALLSTACKITEM_N(_CL("CJuikContainerBase"), _CL("MinimumSize"));
	TSize sz = iContainer->MinimumSize();
	sz += iMargin.SizeDelta();
	return sz;
}

EXPORT_C TInt CJuikContainerBase::CountComponentControls() const
{
	return 1;
}


EXPORT_C CCoeControl* CJuikContainerBase::ComponentControl(TInt /*aIndex*/) const
{
	return iContainer;
}
	
		
EXPORT_C void CJuikContainerBase::PositionChanged()
{
}


	
EXPORT_C void CJuikContainerBase::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CJuikContainerBase"), _CL("SizeChanged"));
	TRect outer = Rect();
	if ( outer.Width() == 0 || outer.Height() == 0 )
		{
			outer.SetSize(TSize(0,0));
			iContainer->SetRect( outer );
		}
	else
		{
			TRect inner = iMargin.InnerRect( outer );
			iContainer->SetRect( inner );
		}
}


EXPORT_C void CJuikContainerBase::Draw(const TRect& aRect) const
{
#ifdef JUIK_BOUNDINGBOXES
	CWindowGc& gc = SystemGc();
	JuikDebug::DrawBoundingBox(gc ,Rect());
#endif
}

EXPORT_C void  CJuikContainerBase::SetContainerWindowL(const CCoeControl& aControl)
{
	CALLSTACKITEM_N(_CL("CJuikContainerBase"), _CL("SetContainerWindowL"));
	CCoeControl::SetContainerWindowL( aControl );
	// Propagate change to child
	if ( iContainer ) iContainer->SetContainerWindowL( *this );
}
