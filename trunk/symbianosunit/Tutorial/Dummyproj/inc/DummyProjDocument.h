/*
* ============================================================================
*  Name     : CDummyProjDocument from DummyProjDocument.h
*  Part of  : DummyProj
*  Created  : 13/04/2005 by Mike Howson
*  Description:
*     Declares document for application.
*  Version  :
*  Copyright: Penrillian
* ============================================================================
*/

#ifndef DUMMYPROJDOCUMENT_H
#define DUMMYPROJDOCUMENT_H

// INCLUDES
#include <akndoc.h>
   
// CONSTANTS

// FORWARD DECLARATIONS
class  CEikAppUi;

// CLASS DECLARATION

/**
*  CDummyProjDocument application class.
*/
class CDummyProjDocument : public CAknDocument
    {
    public: // Constructors and destructor
        /**
        * Two-phased constructor.
        */
        static CDummyProjDocument* NewL(CEikApplication& aApp);

        /**
        * Destructor.
        */
        virtual ~CDummyProjDocument();

    public: // New functions

    public: // Functions from base classes
    protected:  // New functions

    protected:  // Functions from base classes

    private:

        /**
        * EPOC default constructor.
        */
        CDummyProjDocument(CEikApplication& aApp);
        void ConstructL();

    private:

        /**
        * From CEikDocument, create CDummyProjAppUi "App UI" object.
        */
        CEikAppUi* CreateAppUiL();
    };

#endif

// End of File

