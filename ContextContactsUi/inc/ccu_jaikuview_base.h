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

#ifndef CCU_JAIKUVIEW_BASE_H
#define CCU_JAIKUVIEW_BASE_H

#include "app_context.h"
#include <aknview.h>


class CActiveState;
class CUserPics;
class CJabberData;
class phonebook;
class CFeedItemStorage;
class MViewNavigation;
class CPosterUi;
class CJaicons;
class CThemeColors;	
class CTimePeriodFormatter;
class CPresenceHolder;
class CStreamStatsCacher;
class CProgressBarModel;

#include "ccu_uidelegates.h"
#include "ccu_mcommonmenus.h"

/* This base class contains some common functionality and holds most used classes */
class CJaikuViewBase : public CAknView, public MContextBase
{

 public:
	IMPORT_C CJaikuViewBase();
	IMPORT_C ~CJaikuViewBase();

 protected: // Subclass API 
	virtual void ReleaseViewImpl() = 0;

	virtual void RealDoActivateL(const TVwsViewId& aPrevViewId,
								   TUid /*aCustomMessageId*/,
								   const TDesC8& /*aCustomMessage*/) = 0;

	virtual void RealDoDeactivateL() = 0;
	
 public:

	/** 
	 * To cut down constructor parameters in subclasses, SetDependenciesL is provided.
	 * It usually should be called between c++ constructor and ConstructL of subclass, 
	 * thus subclass shouldn't have factory methods. 
	 */
	IMPORT_C void SetDependenciesL(	CActiveState& aActiveState,
									CUserPics&   aUserPics,
									CJabberData& aJabberData,
									phonebook&        aPhonebook,
									CFeedItemStorage& aFeedStorage,
									MViewNavigation& aViewNavigation,
									CPosterUi& aPosterUi,
									CJaicons& aJaicons,
									CThemeColors& aThemeColors,
									CTimePeriodFormatter& aPeriodFormatter,
									CPresenceHolder& aPresenceHolder,
									CJuikGfxStore& aJuikGfxStore,
									CStreamStatsCacher& aStreamStats,
									MCommonMenus& aCommonMenus,
									CProgressBarModel& aProgressBarModel);
	
	IMPORT_C void SetParentViewL(const TVwsViewId& aParentViewId); 

	
	IMPORT_C void ActivateParentViewL();

	inline CActiveState& ActiveState() const { return *iActiveState; }
	inline CUserPics& UserPics() const { return *iUserPics; }
	inline CJabberData& JabberData() const { return *iJabberData; }
	inline CFeedItemStorage& FeedStorage() const { return *iFeedStorage; }
	inline phonebook& Phonebook() const { return *iPhonebook; }
	inline MViewNavigation& ViewNavigation() const { return *iViewNavigation; }
	inline CPosterUi& PosterUi() const { return *iPosterUi; }
	inline CJaicons& Jaicons() const { return *iJaicons; }
	inline CThemeColors& ThemeColors() const { return *iThemeColors; }
	inline CTimePeriodFormatter& PeriodFormatter() const { return *iPeriodFormatter; }
	inline CPresenceHolder& PresenceHolder() const { return *iPresenceHolder; }
	inline CJuikGfxStore& JuikGfxStore() const { return *iFeedGraphics; }
	inline CStreamStatsCacher& StreamStats() const { return *iStreamStats; } 
	inline MCommonMenus& CommonMenus() const { return *iCommonMenus; } 
	inline CProgressBarModel& ProgressBarModel() const { return *iProgressBarModel; }

	IMPORT_C TUiDelegates UiDelegates();

	IMPORT_C void StoreAndReportLastErrorL(TInt aCode);
	IMPORT_C void ShowLastErrorL();

    MErrorInfo* LastErrorInfo() { return iLastErrorInfo; }

 protected:
	/**
	 * Don't implement these in subclass, implement RealDoActivateL & RealDoDeactivateL.
	 */ 
	IMPORT_C virtual void DoActivateL(const TVwsViewId& aPrevViewId,TUid /*aCustomMessageId*/,const TDesC8& /*aCustomMessage*/);
	IMPORT_C virtual void DoDeactivate();
		
	IMPORT_C virtual void CommonDoActivateL(const TVwsViewId& aPrevViewId,
											TUid aCustomMessageId,
											const TDesC8& aCustomMessage);
	
	IMPORT_C TBool PreHandleCommandL(TInt aCommand);

private:
	MErrorInfo* iLastErrorInfo;
	class CErrorContainer* iErrorUi;
	//class CErrorInfoContainer* iErrorUi;

	/* This base class contains some common functionality and holds most used classes
	 * 
	 * CActiveState:
	 *   each view needs access to ActiveState to store state between view transitions
	 * 
	 * CUserPics:
	 *   each view is potentially displaying buddy pictures
	 *
	 * CJabberData:
	 *   contact views need to know and tweak mappings between nick <-> contact
	 * 
	 * CFeedItemStorage:
	 *   feed views always need feedStorage
	 * 
	 * phonebook:
	 *   contact related views need phonebook
	 *
	 * CPostingUi:
	 *   posting should be possible from all views
	 *   commenting should be possible from all feed views
	 *
	 * CThemeColors
	 *   methods to read and sample colors from themes
	 *
	 * CTimePeriodFormatter
	 *   utilities to format time periods to localized languages
	 *
	 * iParentViewId: 
	 *   each view needs a parent view 
	 * 
	 */
	CActiveState* iActiveState;

	CJaicons* iJaicons;
	CUserPics*   iUserPics;
	CJabberData* iJabberData;
	
	phonebook*        iPhonebook;
	CFeedItemStorage* iFeedStorage;

	TVwsViewId iParentViewId;
	
	MViewNavigation* iViewNavigation;
	
	CPosterUi* iPosterUi;

	CThemeColors* iThemeColors;
	
	CTimePeriodFormatter* iPeriodFormatter;

	CPresenceHolder* iPresenceHolder;

	CJuikGfxStore* iFeedGraphics;

	CStreamStatsCacher* iStreamStats;
	
	MCommonMenus* iCommonMenus;

	CProgressBarModel* iProgressBarModel;
};

#endif // CCU_JAIKUVIEW_BASE_H
