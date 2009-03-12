/* ====================================================================
 * File: UiLoft.pan
 * Created: 01/04/07
 * Author: 
 * Copyright (c): , All rights reserved
 * ==================================================================== */

#ifndef __UILOFT_PAN__
#define __UILOFT_PAN__

/** UiLoft application panic codes */
enum TUiLoftPanics 
    {
    EUiLoftBasicUi = 1
    // add further panics here
    };

inline void Panic(TUiLoftPanics aReason)
    {
	_LIT(applicationName,"UiLoft");
    User::Panic(applicationName, aReason);
    }

#endif // __UILOFT_PAN__
