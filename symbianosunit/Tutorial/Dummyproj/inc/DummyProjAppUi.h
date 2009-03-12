/*
* ============================================================================
*  Name     : CDummyProjAppUi from DummyProjAppUi.h
*  Part of  : DummyProj
*  Created  : 13/04/2005 by Mike Howson
*  Description:
*     Declares UI class for application.
*  Version  :
*  Copyright: Penrillian
* ============================================================================
*/

#ifndef DUMMYPROJAPPUI_H
#define DUMMYPROJAPPUI_H

// INCLUDES
#include <eikapp.h>
#include <eikdoc.h>
#include <e32std.h>
#include <coeccntx.h>
#include <aknappui.h>
#include <eikdialg.h>

#include "StackClass.h"

// FORWARD DECLARATIONS
class CDummyProjDialog;


// CLASS DECLARATION

/**
* Application UI class.
* Provides support for the following features:
* - dialog architecture
* 
*/
class CDummyProjAppUi : public CAknAppUi
    {
    public: // // Constructors and destructor

        /**
        * EPOC default constructor.
        */      
        void ConstructL();

        /**
        * Destructor.
        */      
        ~CDummyProjAppUi();
        
    public: // New functions
		TBool NumberQueryDialogL(TInt& aReturnVal);
		void UpdateListboxL(TInt aStackValue, TDesC& aText);


    public: // Functions from base classes

    private:
        // From MEikMenuObserver
        void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane);

    private:
        /**
        * From CEikAppUi, takes care of command handling.
        * @param aCommand command to be handled
        */
        void HandleCommandL(TInt aCommand);

        /**
        * From CEikAppUi, handles key events.
        * @param aKeyEvent Event to handled.
        * @param aType Type of the key event. 
        * @return Response code (EKeyWasConsumed, EKeyWasNotConsumed). 
        */
        virtual TKeyResponse HandleKeyEventL(
            const TKeyEvent& aKeyEvent,TEventCode aType);

    private: //Data
        CDummyProjDialog* iAppDialog; 

	private:
		CStack* iStack;
    };

#endif

// End of File
