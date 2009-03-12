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

class CImageTextWithUrls : public CBase
{
public:
	static CImageTextWithUrls* NewL( CJuikImage* aImage )
	{
		auto_ptr<CImageTextWithUrls> self( new (ELeave) CImageTextWithUrls(aImage) );
		self->ConstructL();
		return self.release();
	}

	
	void UpdateTextL(const TDesC& aText)
	{
		// Compare that text is different (should be cheap, as usually text is of different length)
		if ( iUnwrappedText && aText.Compare(*iUnwrappedText) == 0 )
			return; 
		
		
		// Clear
		ClearContainerL();
		BuildContainerL();
		
		// store unwrapped text
		delete iUnwrappedText;
		iUnwrappedText = NULL;
		iUnwrappedText = aText.AllocL();
		
		// find urls from unwrapped text
		iUrls = FindUrls( *iUnwrappedText );
		
		// wrap text and urls to multiple labels
		WrapImplL( *iUnwrappedText, *iUrls );
	}
	
private:
	CFindItemEngine* FindUrlsL(const TDesC& aText)
	{
		auto_ptr<CFindItemEngine> search( CFindItemEngine::NewL( aText, 
																 CFindItemEngine::EFindItemSearchScheme) );
		return search.release();
	}
	

	void WrapImplL(const TDesC& aText, CFindItemEngine& aFoundUrls)
	{
		// result of this is 
		// array of 
		//     { CArrayFix<TPtrC> lines, type of { EUrl, EText }, width }
		
	}
	

private:
	CImageTextWithUrls(CJuikImage* aImage) : iImage(aImage) {}

	~CImageTextWithUrls() 
	{
		ClearContainerL();
		delete iImage;
	}

	void ConstructL()
	{
		BuildContainerL();
	}



	void ClearContainerL()
	{
		if ( iContainer && iImage )
			iContainer->RemoveControlL( iImage, ENoNeedToUpdate ); // needs to be implemented 

		delete iContainer;
		iContainer = NULL;
	}

	void BuildContainerL()
	{
		iContainer = CJuikSizerContainer::NewL();
		iContent->SetContainerWindowL( *this );
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().SetRootSizerL( sizer );
		}
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
			Content().AddSizerL( sizer, EFirstRowSizer );
			Content().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
		}

		
		{
			iImage->SetContainerWindowL( Content() );
			Content().AddControlL( iImage, iImage );
			Content().GetSizerL( EFirstRowSizer )->AddL( *iImage, 0, Juik::EExpandNot | Juik::EAlignCenterVertical ); 
		}
	}	
};
