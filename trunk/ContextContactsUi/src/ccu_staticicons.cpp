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

#include "ccu_staticicons.h"

#include "juik_iconmanager.h"
#include "symbian_auto_ptr.h"

#include <contextcontactsui.mbg>
#include <avkon.mbg>

#include <akniconarray.h> 
#ifndef __S60V3__
#else
#include <avkonicons.hrh>
#endif



static const TInt KContextContactsUiIconCount=9;
static const TIconID2 KContextContactsUiIconIds[KContextContactsUiIconCount]= {
	{ EMbmContextcontactsuiDummybuddy, EMbmContextcontactsuiDummybuddy_mask },
	{ EMbmContextcontactsuiLight_green,  EMbmContextcontactsuiLight_green_mask },
	{ EMbmContextcontactsuiLight_red,	   EMbmContextcontactsuiLight_red_mask },
	{ EMbmContextcontactsuiLight_yellow, EMbmContextcontactsuiLight_yellow_mask },
	{ EMbmContextcontactsuiLight_gray,   EMbmContextcontactsuiLight_gray_mask },
	{ EMbmContextcontactsuiEarth, EMbmContextcontactsuiEarth_mask },
	{ EMbmContextcontactsuiFriends, EMbmContextcontactsuiFriends_mask },
	{ EMbmContextcontactsuiCalendar, EMbmContextcontactsuiCalendar_mask },
	{ EMbmContextcontactsuiIcon_unread, EMbmContextcontactsuiIcon_unread_mask }
};



EXPORT_C void StaticIcons::LoadContextContactsUiIconsL(MJuikIconManager& aIconManager) 
{
	aIconManager.LoadStaticIconsL( StaticIcons::ContextContactsUiIconFile(), KContextContactsUiIconIds, KContextContactsUiIconCount );
}

_LIT(KCcuIconFile, "C:\\system\\data\\contextcontactsui.mbm");
EXPORT_C const TDesC& StaticIcons::ContextContactsUiIconFile()
{
	return KCcuIconFile;
}


static const TInt KAvkonIconCount(1);
static const TIconID2 KAvkonIconIds[KAvkonIconCount]= {
// 	{ EMbmAvkonQgn_stat_bt_blank, KErrNotFound},
// 	{ EMbmAvkonQgn_prop_nrtyp_address, EMbmAvkonQgn_prop_nrtyp_address_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_email, EMbmAvkonQgn_prop_nrtyp_email_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_fax, EMbmAvkonQgn_prop_nrtyp_fax_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_home, EMbmAvkonQgn_prop_nrtyp_home_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_mobile, EMbmAvkonQgn_prop_nrtyp_mobile_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_note, EMbmAvkonQgn_prop_nrtyp_note_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_pager, EMbmAvkonQgn_prop_nrtyp_pager_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_phone, EMbmAvkonQgn_prop_nrtyp_phone_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_url, EMbmAvkonQgn_prop_nrtyp_url_mask },
// 	{ EMbmAvkonQgn_prop_nrtyp_work, EMbmAvkonQgn_prop_nrtyp_work_mask },
	{ EMbmAvkonQgn_indi_marked_add, EMbmAvkonQgn_indi_marked_add_mask},
};

// _LIT(KAvkonIconFile, "Z:\\system\\data\\avkon.mbm");
// _LIT(KAvkonIconFile, "Z:\\system\\data\\avkon.mif");

EXPORT_C void StaticIcons::LoadAvkonIconsL(MJuikIconManager& aIconManager) 
{
	aIconManager.LoadStaticIconsL( StaticIcons::AvkonIconFile(), KAvkonIconIds, KAvkonIconCount );
}

EXPORT_C const TDesC& StaticIcons::AvkonIconFile()
{
	return AknIconUtils::AvkonIconFileName();
}


static const TInt KAvkonIconOldSkoolCount(12);
static const TIconID KAvkonIconOldSkoolIds[KAvkonIconOldSkoolCount]= {
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_stat_bt_blank, EMbmAvkonQgn_stat_bt_blank+1),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_address, EMbmAvkonQgn_prop_nrtyp_address_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_email, EMbmAvkonQgn_prop_nrtyp_email_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_fax, EMbmAvkonQgn_prop_nrtyp_fax_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_home, EMbmAvkonQgn_prop_nrtyp_home_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_mobile, EMbmAvkonQgn_prop_nrtyp_mobile_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_note, EMbmAvkonQgn_prop_nrtyp_note_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_pager, EMbmAvkonQgn_prop_nrtyp_pager_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_phone, EMbmAvkonQgn_prop_nrtyp_phone_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_url, EMbmAvkonQgn_prop_nrtyp_url_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_prop_nrtyp_work, EMbmAvkonQgn_prop_nrtyp_work_mask ),
	_INIT_T_ICON_ID("Z:\\system\\data\\avkon.mbm", EMbmAvkonQgn_indi_marked_add, EMbmAvkonQgn_indi_marked_add_mask),
};


EXPORT_C CAknIconArray* StaticIcons::LoadAvkonIconsOldSkoolL() 
{
	
	auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(10) );
	LoadIcons( icons.get(), KAvkonIconOldSkoolIds, KAvkonIconOldSkoolCount);
	return icons.release();
}

EXPORT_C TInt StaticIcons::GetAvkonOldSkoolIconIndex(TInt aIconId)
{
	return GetIconIndex( aIconId, KAvkonIconOldSkoolIds, KAvkonIconOldSkoolCount);
}


