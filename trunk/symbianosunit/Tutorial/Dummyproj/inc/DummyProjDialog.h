/*
* ============================================================================
*  Name     : CDummyProjDialog from DummyProjDialog.h
*  Part of  : DummyProj
*  Created  : 13/04/2005 by Mike Howson
*  Description:
*     Declares dialog for application.
*  Version  :
*  Copyright: Penrillian
* ============================================================================
*/

#ifndef DUMMYPROJDIALOG_H
#define DUMMYPROJDIALOG_H

// INCLUDES
#include <eikdialg.h>
#include <aknlists.h>
#include "DummyProj.hrh"

// FORWARD DECLARATIONS

// CLASS DECLARATION

/**
* CDummyProjDialog dialog class
* 
*/
class CDummyProjDialog : public CEikDialog
    {
    public: // Constructors and destructor
		/**
		* Destructor.
		*/
		~CDummyProjDialog();
		void ConstructL(const TRect& aRect) ;


    public: // New functions
		void AddListboxItemL(const TDesC& aPtr);

    public: // Functions from base classes
		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode);

    protected:  // New functions
			TInt CountComponentControls() const;
			CCoeControl* ComponentControl(TInt aIndex) const;

    protected:  // Functions from base classes
        /**
         * From CEikDialog : This is called in CEikDialog::ExecuteLD()
         *                   before a form is drawn.
         */
        void PreLayoutDynInitL();
		void PostLayoutDynInitL();

		/**
		* From CEikDialog : This is called by the dialog framework, returns true if the 
		* dialog can exit, false otherwise.
		*
		* @param aButtonId  Id of the softkey which was pressed
		* @return           ETrue if the dialog can exit, false otherwise.
		*/
        TBool OkToExitL( TInt aButtonId );

    private: //data

	protected:
			CEikTextListBox* iListBox;
			//CAknColumnListBox* iListBox;
    };

#endif

// End of File
