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
#include "medialistbox.h"
#include <eikfrlbd.h> 
#include <eikenv.h>
#include <aknviewappui.h> 
#include <aknutils.h>
#include <contextmediaui.rsg>
#include "timeout.h"
#include <e32math.h>
#include <bautils.h>
#include <contextmedia.rsg>
#include "cu_common.h"

#include "jaiku_layout.h"

void GetTimeString(TTime t, TDes& aBuf)
{
	TBuf<5> time; 
	TTime now; now=GetTime();
	if ((now.DateTime().Year()==t.DateTime().Year()) 
	   && (now.DateTime().Month()==t.DateTime().Month())
	   && (now.DateTime().Day()==t.DateTime().Day()) ) {
		
		t.FormatL(time, _L("%F%H:%T"));
	} else {
		t.FormatL(time, _L("%F%D/%M"));
	}
	aBuf.Copy(time);
}

class CContextMediaBoxDrawer : public CFormattedCellListBoxItemDrawer 
{
public:
	CContextMediaBoxDrawer(MTextListBoxModel *aTextListBoxModel, 
			 const CFont *aFont, 
			 CFormattedCellListBoxData *aFormattedCellData, 
			 CContextMediaArray* aPostArray,
			 TBool	aStandAlone);

private:
	void DrawItemText (TInt aItemIndex, const TRect &aItemTextRect,TBool aItemIsCurrent, 
		TBool aViewIsEmphasized, TBool aItemIsSelected) const;
	
	CFormattedCellListBoxData *itemd;
	MTextListBoxModel * iTextListBoxModel;
	CContextMediaArray * iPostArray;
	TBool	iStandAlone;
	CEikonEnv*	iEikEnv;
};


class CContextMediaArrayImpl : public CContextMediaArray {
public:
	CContextMediaArrayImpl(CCMNetwork &aNetwork, CPostStorage &aStorage, TInt64 aNode, CPostStorage::TSortBy aSort, 
		CPostStorage::TOrder aOrder, MPostNotify& aObserver, TAdditionalItem aItem,
		TBool aStandAlone);
	void ConstructL();
	~CContextMediaArrayImpl();
private:
	virtual CCMPost*  GetPostAt(TInt aIndex);
private:
	void ReadFromStorage();
	void ReleasePosts();
	virtual TInt MdcaCount() const;
	virtual TPtrC16 MdcaPoint(TInt aIndex) const;

private:
	virtual void PostEvent(CCMPost* aParent, CCMPost* aChild, TEvent aEvent);
public:
	CArrayFixFlat<CCMPost*>* iPostArray;
	CArrayFixFlat<TInt64>* iPostIdArray;
	CCMPost*		iParentPost;
private:
	CPostStorage	&iStorage;
	CCMNetwork	&iNetwork;
	TInt64 iNode;
	CPostStorage::TSortBy iSort;
	CPostStorage::TOrder iOrder;

	HBufC * iBuf;
	MPostNotify& iObserver;

	HBufC * iBy;
	HBufC * iFirstBy;
	HBufC * iLastBy;

	HBufC * iNoTitle;
	HBufC * iNewThread;
	HBufC * iUseCode;
	HBufC * iAddReply;
	HBufC * iFirstPost;
	HBufC * iLoading;
	HBufC * iError;
	TAdditionalItem iItem;
	TInt	iResource;
	TBool iStandAlone;

};

CCMPost*  CContextMediaArrayImpl::GetPostAt(TInt aIndex)
{
	CALLSTACKITEM_N(_CL("CContextMediaArrayImpl"), _CL("GetPostAt"));

	if (aIndex<0) return 0;
	if ((iItem!=ENone) && (aIndex==0)) return 0;

	if (iOrder==CPostStorage::EDescending) {
		aIndex=MdcaCount()-aIndex-1;
	}

	if (!iPostArray->At(aIndex)) {
		iPostArray->At(aIndex)=iStorage.GetByPostIdL(0, iPostIdArray->At(aIndex));
	}

	return iPostArray->At(aIndex);
}

EXPORT_C CContextMediaArray* CContextMediaArray::NewL(CCMNetwork &aNetwork, CPostStorage &aStorage, TInt64 aNode, 
					     CPostStorage::TSortBy aSort, 
					     CPostStorage::TOrder aOrder,
					     MPostNotify& aObserver, TAdditionalItem aItem,
					     TBool aStandAlone)
{
	CALLSTACKITEM_N(_CL("CPostStorage"), _CL("TOrder"));

	CContextMediaArrayImpl * ret=new (ELeave) 
		CContextMediaArrayImpl(aNetwork, aStorage, aNode, aSort, aOrder, aObserver, aItem, aStandAlone);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop();
	return ret;
}

CContextMediaArrayImpl::CContextMediaArrayImpl( CCMNetwork &aNetwork,
						CPostStorage&aStorage, 
					       TInt64 aNode, CPostStorage::TSortBy aSort, 
					       CPostStorage::TOrder aOrder,
					       MPostNotify& aObserver, TAdditionalItem aItem,
					       TBool aStandAlone) : 
	iNetwork(aNetwork), iStorage(aStorage), iNode(aNode), iSort(aSort), iOrder(aOrder), iObserver(aObserver), iItem(aItem),
		iStandAlone(aStandAlone) { }

void CContextMediaArrayImpl::ReleasePosts()
{
	if (iPostArray) {
		int i;
		for (i=0;i<iPostArray->Count();i++) {
			CCMPost * aPost = (*iPostArray)[i];
			if (aPost) {
				CC_TRAPD(err, iStorage.Release( aPost, 0 ));
			}
		}
	}
}

CContextMediaArrayImpl::~CContextMediaArrayImpl()
{
	CALLSTACKITEM_N(_CL("CContextMediaArrayImpl"), _CL("~CContextMediaArrayImpl"));

	if (iResource) CEikonEnv::Static()->DeleteResourceFile(iResource);

	ReleasePosts();
	if (iParentPost) iStorage.Release(iParentPost, this);
	delete iPostIdArray;
	delete iPostArray;
	delete iBuf;
	delete iNoTitle;
	
	delete iBy;
	delete iFirstBy;
	delete iLastBy;

	delete iNewThread;
	delete iLoading;
	delete iError;
	delete iUseCode;
	delete iAddReply;
	delete iFirstPost;
}

void CContextMediaArrayImpl::ReadFromStorage()
{
	CALLSTACKITEM_N(_CL("CContextMediaArrayImpl"), _CL("ReadFromStorage"));
	iPostIdArray->Reset();
	ReleasePosts();
	iPostArray->Reset();

	TBool ok = iStorage.FirstL(iNode, iSort, CPostStorage::EAscending, EFalse);
	while (ok) {
		TInt64 id = iStorage.GetCurrentIdL();
		iPostIdArray->AppendL(id);
		iPostArray->AppendL(0);
		ok = iStorage.NextL();
	}
	if (iStandAlone && iNode==iStorage.RootId()) {
		iPostIdArray->AppendL(iStorage.RootId());
		iPostArray->AppendL(0);
	}
}

void CContextMediaArrayImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextMediaArrayImpl"), _CL("ConstructL"));
	iResource=LoadSystemResourceL(CEikonEnv::Static(), _L("contextmedia"));

	iBuf = HBufC::NewL(256);
	iBy = CEikonEnv::Static()->AllocReadResourceL(R_BY);
	iFirstBy = CEikonEnv::Static()->AllocReadResourceL(R_FIRST_BY);
	iLastBy = CEikonEnv::Static()->AllocReadResourceL(R_LAST_BY);
	iNoTitle = CEikonEnv::Static()->AllocReadResourceL(R_NO_TITLE);

	iUseCode =CEikonEnv::Static()->AllocReadResourceL(R_USE_CODE);
	iAddReply =CEikonEnv::Static()->AllocReadResourceL(R_ADD_REPLY);
	iFirstPost =CEikonEnv::Static()->AllocReadResourceL(R_FIRST_POST);

	iLoading=CEikonEnv::Static()->AllocReadResourceL(R_LOADING_TITLE);
	iError=CEikonEnv::Static()->AllocReadResourceL(R_ERROR_TITLE);
	iNewThread=CEikonEnv::Static()->AllocReadResourceL(R_NEW_THREAD_TITLE);
	//iNewThread =CEikonEnv::Static()->AllocReadResourceL(R_NEW_THREAD);

	iPostArray = new (ELeave) CArrayFixFlat<CCMPost*>(50);
	iPostIdArray = new (ELeave) CArrayFixFlat<TInt64>(50);

	iParentPost = iStorage.GetByPostIdL(this, iNode);
        
	ReadFromStorage();

}

TInt CContextMediaArrayImpl::MdcaCount() const
{
	CALLSTACKITEM_N(_CL("CContextMediaArrayImpl"), _CL("MdcaCount"));

	if (iItem == ENone) {
		return iPostArray->Count();
	} else {
		return iPostArray->Count()+1;
	}
}

TPtrC16 CContextMediaArrayImpl::MdcaPoint(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CContextMediaArrayImpl"), _CL("MdcaPoint"));

	if (aIndex>iPostArray->Count()) {
		User::Leave(KErrArgument);
	}

	if (!iStandAlone) {
		// ----- case first item in list ---------
		if (aIndex == 0) {
			if (iItem == EUseVCode) {
				iBuf->Des().Zero();
				iBuf->Des().AppendFormat(_L("%d\t"), KUseCodeIconIndex);
				iBuf->Des().Append(*iUseCode);
				iBuf->Des().Append(_L("\t\t\t"));
				return iBuf->Des();
			} else if (iItem == EAddReply) {
				iBuf->Des().Zero();
				if (iPostArray->Count()==0) {
					iBuf->Des().AppendFormat(_L("%d\t"), KReplyIconIndex);
					iBuf->Des().Append(*iFirstPost);
					iBuf->Des().Append(_L("\t\t\t"));
				} else {
					iBuf->Des().AppendFormat(_L("%d\t"), KReplyIconIndex);
					iBuf->Des().Append(*iAddReply);
					iBuf->Des().Append(_L("\t\t\t"));
				}
				return iBuf->Des();
			}
		}

	}

	// ---- normal case --------------------
	if (iOrder==CPostStorage::EDescending) {
		aIndex=MdcaCount()-aIndex-1;
	}
	if (iPostArray->At(aIndex) == 0) {
		iPostArray->At(aIndex) = iStorage.GetByPostIdL(0, iPostIdArray->At(aIndex));
	}
	
	iBuf->Des().Zero();
	const CCMPost * aPost = (*iPostArray)[aIndex];

	if (!iStandAlone) {
		TInt iconidx=aPost->GetThumbnailIndex();
		if (iconidx<0) iconidx=KUnknownIconIndex;
		
		if (aPost->iBodyText->Value().Compare(*iLoading) == 0) {
			iconidx = KUnknownIconIndex;
		} else if (aPost->iBodyText->Value().Compare(*iError)==0) {
			iconidx=KErrorIconIndex;
		}

		// First Line of the list box: icon_id, title, nb_unread
		iBuf->Des().AppendFormat(_L("%d\t"), iconidx);
	}

	// Loading indicator for thread ...
	TInt64 id = iPostIdArray->At(aIndex);
	TBool loading = ( (iNode == CPostStorage::RootId()) && 
		( (iNetwork.GetFetchStatus(id)==MNetworkStatus::EConnecting) ||
		(iNetwork.GetFetchStatus(id)==MNetworkStatus::EFetching) ) );
	if (loading) {
		if (aPost->iBodyText->Value().Length() == 0) {
			iBuf->Des().Append(*iNoTitle);
		} else {
			iBuf->Des().Append(aPost->iBodyText->Value().Left(15));
		}
		TTime now; now=GetTime(); 
		TInt sec = now.DateTime().Second();
		TInt dots = 6;
		TInt nb_dots = sec - ( dots*int(sec/dots));
		for (int i=0; i<=nb_dots; i++) {
			iBuf->Des().Append(_L("."));
		}
		iObserver.PostEvent(0, 0, EPostUpdated);
	} else {
		if (aPost->iBodyText->Value().Length() == 0) {
			iBuf->Des().Append(*iNoTitle);
		} else {
			iBuf->Des().Append(aPost->iBodyText->Value().Left(40));
		}
	}

	iBuf->Des().Append(_L("\t"));
	TInt unread = aPost->iUnreadCounter();
	if (unread>0) {
		iBuf->Des().AppendNum(unread );
	}

	if (! iStandAlone) {
		iBuf->Des().Append(_L("\t"));
	// Second and third Lines of the listbox
		if (aPost->iSender.iName().Compare(_L("?")) == 0) {
			iBuf->Des().Append(_L(" \t \t \t"));
		} else {
			if (iNode==CPostStorage::RootId()) {
				// second line
				iBuf->Des().Append(*iFirstBy);
				iBuf->Des().Append(_L(" "));
				iBuf->Des().Append(aPost->iSender.iName());
				iBuf->Des().Append(_L(",\t"));
				TBuf<5> time; GetTimeString(aPost->iTimeStamp(), time);
				iBuf->Des().Append(time);
				//third line
				if (aPost->LastPostAuthor().Length()==0)  {
					iBuf->Des().Append(_L("\t\t"));
				} else {
					iBuf->Des().Append(_L("\t"));
					iBuf->Des().Append(*iLastBy);
					iBuf->Des().Append(_L(" "));
					iBuf->Des().Append(aPost->LastPostAuthor());
					iBuf->Des().Append(_L(",\t"));
					GetTimeString(aPost->LastPostDate(), time);
					iBuf->Des().Append(time);
				}
			} else {
				// second line
				iBuf->Des().Append(*iBy);
				iBuf->Des().Append(_L(" "));
				iBuf->Des().Append(aPost->iSender.iName());
				iBuf->Des().Append(_L(",\t"));
				TBuf<5> time; GetTimeString(aPost->iTimeStamp(), time);
				iBuf->Des().Append(time);
				// third line
				iBuf->Des().Append(_L("\t\t"));
			}
		}
	}
#ifdef __WINS__
	RDebug::Print(iBuf->Des());
#endif
	return iBuf->Des();
}

void CContextMediaArrayImpl::PostEvent(CCMPost* aParent, CCMPost* aChild, TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CContextMediaArrayImpl"), _CL("PostEvent"));
	
	if (aParent != iParentPost) return;

	if (aEvent==EChildAdded) {
		iPostIdArray->AppendL(aChild->iPostId());
		iPostArray->AppendL(0);
		iObserver.PostEvent(aParent, aChild, aEvent);
	}

	if (aEvent==EPlaceholderFilled) {
		TInt i;
		for (i=0; i<iPostIdArray->Count();i++) {
			if (iPostIdArray->At(i) == aChild->iPostId()) {
				CC_TRAPD(ignore, iStorage.Release(iPostArray->At(i), 0));
				iPostArray->At(i) = 0;
				iObserver.PostEvent(aParent, aChild, aEvent);
				return;
			}
		}
	}

	if (aEvent==EPostHidden) {
		ReadFromStorage();
		iObserver.PostEvent(aParent, aChild, aEvent);
	}

	if (aEvent==EPostVisible) {
		ReadFromStorage();
		iObserver.PostEvent(aParent, aChild, aEvent);
	}

	if (aEvent==EPostUpdated) {
		iObserver.PostEvent(aParent, aChild, aEvent);
	} 

	if ((aEvent==EThumbnailLoaded) || (aEvent==EUnreadChanged)) {
		iObserver.PostEvent(aParent, aChild, aEvent);
	}

	if (aEvent==ELastPostChanged) {
		iObserver.PostEvent(aParent, aChild, aEvent);
	}
	
	//do nothing for:
	//	EMediaLoaded,
	//	EErrorUpdated,
}

EXPORT_C CContextMediaBox::CContextMediaBox(CContextMediaArray * aPostArray, TBool aStandAlone) : 
	CEikFormattedCellListBox(), iPostArray(aPostArray), iStandAlone(aStandAlone)
{
}

//------------------------
void CContextMediaBox::AddSubCell(TMargins marg, 
								  TSize size, 
								  const CFont* aFont,
								  TPoint pos, 
								  TInt baselinepos, 
								  CGraphicsContext::TTextAlign aAlign, 
								  TBool aGraphic)
{
	itemd->SetSubCellMarginsL(iCells, marg);		
	itemd->SetSubCellSizeL(iCells,  size);
	itemd->SetSubCellPositionL(iCells,  pos);
	itemd->SetSubCellBaselinePosL(iCells, baselinepos);
	itemd->SetSubCellFontL(iCells, aFont );
	itemd->SetSubCellAlignmentL(iCells, aAlign);
	if (aGraphic)
		itemd->SetGraphicsSubCellL(iCells, ETrue);

	++iCells;
}

void CContextMediaBox::CreateItemDrawerL(void)
{
	CALLSTACKITEM_N(_CL("CContextMediaBox"), _CL("CreateItemDrawerL"));

	itemd=CFormattedCellListBoxData::NewL();
	CleanupStack::PushL(itemd);

	iItemDrawer=new (ELeave) CContextMediaBoxDrawer(Model(), LatinPlain12(), itemd, iPostArray,
													iStandAlone);
	
	CleanupStack::Pop();	
}

CContextMediaBox::~CContextMediaBox()
{
	CALLSTACKITEM_N(_CL("CContextMediaBox"), _CL("~CContextMediaBox"));

	CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iLatinPlainItalic12);
}

void CContextMediaBox::SizeChanged()
{
	CEikFormattedCellListBox::SizeChanged();
#ifdef __S60V2__
	// FIXME3RDED
	// hardcoded offset: the height of the status pane
	itemd->SetSkinParentPos( TPoint(0, 44*iScale) );
#endif
	LayoutSubcells();
}

void CContextMediaBox::LayoutSubcells()
{
	CEikonEnv* iEikEnv=CEikonEnv::Static();
	
	TFontSpec fontSpec = LatinPlain12()->FontSpecInTwips();
	fontSpec.iFontStyle.SetPosture(EPostureItalic );
	iEikEnv->ScreenDevice()->GetNearestFontInTwips(iLatinPlainItalic12, fontSpec);

	//Thumbnail (image size = 48*36)
	if (!iStandAlone) {
		TMargins no_marg, image_marg, text_marg, text_marg_2;
		
		no_marg.iBottom=no_marg.iTop=no_marg.iLeft=no_marg.iRight=0;
		
		image_marg.iTop = 5;
		image_marg.iLeft = 2;
		image_marg.iRight = 2;
		image_marg.iBottom=4;
		
		text_marg.iBottom=2;
		text_marg.iTop=0;
		text_marg.iLeft=2;
		text_marg.iRight=3;
		
		text_marg_2.iBottom=2;
		text_marg_2.iTop=0;
		text_marg_2.iLeft=0;
		text_marg_2.iRight=2;
		

	    // not supported for scalable layout
		AddSubCell(image_marg, TSize(52, 48), iEikEnv->DenseFont(), TPoint(0, 0),
			0, CGraphicsContext::ECenter, ETrue);

		AddSubCell(text_marg, TSize(104, 15), LatinPlain12(), TPoint(52, 0), 13);

		AddSubCell(text_marg, TSize(20, 15), LatinBold12(), TPoint(156, 0), 13,
			CGraphicsContext::ERight);


		//text line 1 box 1
		AddSubCell(text_marg_2, TSize(94, 16), iLatinPlainItalic12,
			TPoint(50, 18), 29, CGraphicsContext::ERight);

		//text line 1 box 2
		AddSubCell(text_marg_2, TSize(32, 16), iLatinPlainItalic12,
			TPoint(144, 18), 29, CGraphicsContext::ERight);

		//text line 2 box 1
		AddSubCell(text_marg_2, TSize(94, 14), iLatinPlainItalic12,
			TPoint(50, 33), 43, CGraphicsContext::ERight);

		//text line 2 box 2
		AddSubCell(text_marg_2, TSize(32, 14), iLatinPlainItalic12,
			TPoint(144, 33), 43, CGraphicsContext::ERight);
	}
	else 
		{
			TSize itemSize = View()->ItemSize();
			TJuikLayoutItem parent( TRect(TPoint(0,0), itemSize) );
			// Text
			{				
				TMargins marg = Juik::FromMargins8( Layout().GetLayoutItemL( LG_medialistbox, LI_medialistbox__name_margins ).Margins() );
				TJuikLayoutItem subL = parent.Combine( Layout().GetLayoutItemL( LG_medialistbox, LI_medialistbox__name ) );
				AddSubCell(marg, subL.Size(), subL.Font(), subL.Rect().iTl, subL.Baseline() );
			}

			// count
			{
				TMargins marg = Juik::FromMargins8( Layout().GetLayoutItemL( LG_medialistbox, LI_medialistbox__count_margins ).Margins() );
				TJuikLayoutItem subL = parent.Combine( Layout().GetLayoutItemL( LG_medialistbox, LI_medialistbox__count ) );
				AddSubCell(marg, subL.Size(), subL.Font(), subL.Rect().iTl, subL.Baseline(), CGraphicsContext::ERight );
			}
		}
}

CContextMediaBoxDrawer::CContextMediaBoxDrawer(MTextListBoxModel *aTextListBoxModel, 
					 const CFont *aFont, 
					 CFormattedCellListBoxData *aFormattedCellData,
					 CContextMediaArray * aPostArray,
					 TBool	aStandAlone) :
		CFormattedCellListBoxItemDrawer(aTextListBoxModel, aFont, aFormattedCellData),
			iStandAlone(aStandAlone)
		{
			itemd=aFormattedCellData;
			iTextListBoxModel=aTextListBoxModel;
			iPostArray = aPostArray;
			iEikEnv = CEikonEnv::Static();

		}


void CContextMediaBoxDrawer::DrawItemText (TInt aItemIndex, const TRect &aItemTextRect, 
		TBool aItemIsCurrent, TBool aViewIsEmphasized, TBool aItemIsSelected) const 
{	
	CALLSTACKITEM_N(_CL("CContextMediaBoxDrawer"), _CL("DrawItemText"));

	/*CWindowGc& gc=CEikonEnv::Static()->SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aItemTextRect);*/

	TInt unread = 0;
	TInt64 parentId=CPostStorage::RootId();

	const CCMPost* post=iPostArray->GetPostAt(aItemIndex);

	if (post) {
		unread = post->iUnreadCounter();
		parentId = post->iParentId();
	}

	if (!iStandAlone) {
		if ((parentId != CPostStorage::RootId()) || (unread==0)) {
			itemd->SetSubCellSizeL(1, TSize(124, 20) );
			itemd->SetSubCellPositionL(2, TPoint(176, 0) );
		} else {
			itemd->SetSubCellSizeL(1, TSize(104, 20) );
			itemd->SetSubCellPositionL(2, TPoint(156, 0) );
		}
	}

	TListItemProperties prop;
	if (!post){
		itemd->SetSubCellFontL(1, LatinBold12() );
		prop.SetUnderlined(EFalse);
	} else {
		if (!iStandAlone) {
			prop.SetUnderlined(ETrue);
			if (unread) {
				itemd->SetSubCellFontL(1, LatinBold12() );
			} else {
				itemd->SetSubCellFontL(1, LatinPlain12() );
			}
		} else {
			if (unread) {
				itemd->SetSubCellFontL(0, iEikEnv->NormalFont() );
			} else {
				itemd->SetSubCellFontL(0, iEikEnv->DenseFont() );
			}
		}
	}
	((CFormattedCellListBoxItemDrawer*)this)->SetPropertiesL(aItemIndex, prop);

	CFormattedCellListBoxItemDrawer::DrawItemText(aItemIndex, aItemTextRect,
			aItemIsCurrent, aViewIsEmphasized, aItemIsSelected);
}

