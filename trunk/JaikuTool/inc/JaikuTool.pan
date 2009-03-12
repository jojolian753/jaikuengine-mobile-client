/* ====================================================================
 * File: JaikuTool.pan
 * Created: 01/04/07
 * Author: 
 * Copyright (c): , All rights reserved
 * ==================================================================== */

#ifndef __JAIKUTOOL_PAN__
#define __JAIKUTOOL_PAN__

/** JaikuTool application panic codes */
enum TJaikuToolPanics 
    {
    EJaikuToolBasicUi = 1
    // add further panics here
    };

inline void Panic(TJaikuToolPanics aReason)
    {
	_LIT(applicationName,"JaikuTool");
    User::Panic(applicationName, aReason);
    }

#endif // __JAIKUTOOL_PAN__
