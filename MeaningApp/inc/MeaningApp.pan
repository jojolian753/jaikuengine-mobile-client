/* ====================================================================
 * File: MeaningApp.pan
 * Created: 01/04/07
 * Author: 
 * Copyright (c): , All rights reserved
 * ==================================================================== */

#ifndef __MeaningApp_PAN__
#define __MeaningApp_PAN__

/** MeaningApp application panic codes */
enum TMeaningAppPanics 
    {
    EMeaningAppBasicUi = 1
    // add further panics here
    };

inline void Panic(TMeaningAppPanics aReason)
    {
	_LIT(applicationName,"MeaningApp");
    User::Panic(applicationName, aReason);
    }

#endif // __MeaningApp_PAN__
