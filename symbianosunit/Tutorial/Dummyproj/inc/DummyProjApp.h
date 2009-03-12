/*
* ============================================================================
*  Name     : CDummyProjApp from DummyProjApp.h
*  Part of  : DummyProj
*  Created  : 13/04/2005 by Mike Howson
*  Description:
*     Declares main application class.
*  Version  :
*  Copyright: Penrillian
* ============================================================================
*/

#ifndef DUMMYPROJAPP_H
#define DUMMYPROJAPP_H

// INCLUDES
#include <aknapp.h>

// CONSTANTS
// UID of the application
const TUid KUidDummyProj = { 0x061AFC00 };

// CLASS DECLARATION

/**
* CDummyProjApp application class.
* Provides factory to create concrete document object.
* 
*/
class CDummyProjApp : public CAknApplication
    {
    
    public: // Functions from base classes
    private:

        /**
        * From CApaApplication, creates CDummyProjDocument document object.
        * @return A pointer to the created document object.
        */
        CApaDocument* CreateDocumentL();
        
        /**
        * From CApaApplication, returns application's UID (KUidDummyProj).
        * @return The value of KUidDummyProj.
        */
        TUid AppDllUid() const;
    };

#endif

// End of File

