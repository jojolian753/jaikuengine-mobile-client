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

#include "presence_icons.h"

#include "juik_iconmanager.h"
#include "app_context.h"
#include "symbian_auto_ptr.h"
#include <contextcommon.mbg>
#include <contextcontactsui.mbg>

#include <eikenv.h> 
#include <bautils.h>
#include <avkon.mbg>
#include <s32file.h>
#include <gulicon.h>
#ifndef __S60V3__
#else
#include <avkonicons.hrh>
#endif


const TInt KNbIcons=49;

#ifndef __WINS__
_LIT(KMbmBook,  "Z:\\system\\data\\contextcommon.mbm");
#else
_LIT(KMbmBook,  "C:\\system\\data\\contextcommon.mbm");
#endif

static const TIconID iconId[KNbIcons]=
{
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_stat_bt_blank, EMbmAvkonQgn_stat_bt_blank +1),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_address, EMbmAvkonQgn_prop_nrtyp_address_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_email, EMbmAvkonQgn_prop_nrtyp_email_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_fax, EMbmAvkonQgn_prop_nrtyp_fax_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_home, EMbmAvkonQgn_prop_nrtyp_home_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_mobile, EMbmAvkonQgn_prop_nrtyp_mobile_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_note, EMbmAvkonQgn_prop_nrtyp_note_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_pager, EMbmAvkonQgn_prop_nrtyp_pager_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_phone, EMbmAvkonQgn_prop_nrtyp_phone_mask ),

/*10*/  
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_url, EMbmAvkonQgn_prop_nrtyp_url_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_work, EMbmAvkonQgn_prop_nrtyp_work_mask ),
	
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_indi_marked_add, EMbmAvkonQgn_indi_marked_add_mask),
	#ifndef __WINS__

	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_on, EMbmContextcommonSpeaker_on_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_on_grey, EMbmContextcommonSpeaker_on_grey_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_off, EMbmContextcommonSpeaker_off_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_off_grey, EMbmContextcommonSpeaker_off_grey_mask ),
	
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_on, EMbmContextcommonVibrator_on_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_on_grey, EMbmContextcommonVibrator_on_grey_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_off, EMbmContextcommonVibrator_off_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_off_grey, EMbmContextcommonVibrator_off_grey_mask ),

/*20*/	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_active, EMbmContextcommonUser_active_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive, EMbmContextcommonUser_inactive_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive_lvl_1, EMbmContextcommonUser_inactive_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive_lvl_2, EMbmContextcommonUser_inactive_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive_lvl_3, EMbmContextcommonUser_inactive_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy1, EMbmContextcommonBuddy1_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy2, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy3, EMbmContextcommonBuddy3_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy4, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy1_grey, EMbmContextcommonBuddy1_mask ),
/*30*/	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy2_grey, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy3_grey, EMbmContextcommonBuddy3_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy4_grey, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther1, EMbmContextcommonBuddy1_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther2, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther3, EMbmContextcommonBuddy3_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther4, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther1_grey, EMbmContextcommonBuddy1_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther2_grey, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther3_grey, EMbmContextcommonBuddy3_mask ),
/*40*/	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther4_grey, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonDesktop, EMbmContextcommonDesktop_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonDesktop_grey, EMbmContextcommonDesktop_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonLaptop, EMbmContextcommonLaptop_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonLaptop_grey, EMbmContextcommonLaptop_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonPda, EMbmContextcommonPda_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonPda_grey, EMbmContextcommonPda_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonCalendar, EMbmContextcommonCalendar_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcommon.mbm", EMbmContextcommonCalendar_grey, EMbmContextcommonCalendar_mask ),
	#else

	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_on, EMbmContextcommonSpeaker_on_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_on_grey, EMbmContextcommonSpeaker_on_grey_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_off, EMbmContextcommonSpeaker_off_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonSpeaker_off_grey, EMbmContextcommonSpeaker_off_grey_mask ),
	
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_on, EMbmContextcommonVibrator_on_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_on_grey, EMbmContextcommonVibrator_on_grey_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_off, EMbmContextcommonVibrator_off_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonVibrator_off_grey, EMbmContextcommonVibrator_off_grey_mask ),

	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_active, EMbmContextcommonUser_active_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive, EMbmContextcommonUser_inactive_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive_lvl_1, EMbmContextcommonUser_inactive_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive_lvl_2, EMbmContextcommonUser_inactive_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonUser_inactive_lvl_3, EMbmContextcommonUser_inactive_mask ),

	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy1, EMbmContextcommonBuddy1_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy2, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy3, EMbmContextcommonBuddy3_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy4, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy1_grey, EMbmContextcommonBuddy1_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy2_grey, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy3_grey, EMbmContextcommonBuddy3_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonBuddy4_grey, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther1, EMbmContextcommonBuddy1_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther2, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther3, EMbmContextcommonBuddy3_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther4, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther1_grey, EMbmContextcommonBuddy1_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther2_grey, EMbmContextcommonBuddy2_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther3_grey, EMbmContextcommonBuddy3_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonOther4_grey, EMbmContextcommonBuddy4_mask ),

	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonDesktop, EMbmContextcommonDesktop_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonDesktop_grey, EMbmContextcommonDesktop_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonLaptop, EMbmContextcommonLaptop_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonLaptop_grey, EMbmContextcommonLaptop_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonPda, EMbmContextcommonPda_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonPda_grey, EMbmContextcommonPda_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonCalendar, EMbmContextcommonCalendar_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\contextcommon.mbm", EMbmContextcommonCalendar_grey, EMbmContextcommonCalendar_mask ),
	#endif
};

EXPORT_C TInt GetIconBitmap(TInt index)
{
	return GetIconBitmap(index, iconId, KNbIcons);
}

EXPORT_C TInt GetIconMask(TInt index)
{
	return GetIconMask(index, iconId, KNbIcons);
}

EXPORT_C TPtrC GetIconMbm(TInt index)
{
	return GetIconMbm(index, iconId, KNbIcons);
}


EXPORT_C void LoadIcons(CArrayPtrFlat<CGulIcon> * aIconList)
{
	LoadIcons(aIconList, iconId, KNbIcons);
}

EXPORT_C TInt GetIconIndex(TInt identifier)
{
	return GetIconIndex(identifier, iconId, KNbIcons);
}


