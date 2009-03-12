// CSymbianOSUnitDocument.cpp
// ---------------------
//
// Copyright (c) 2003-2006 Penrillian Ltd all rights reserved. Web: www.penrillian.com
//

////////////////////////////////////////////////////////////////////////
//
// Source file for the implementation of the 
// document class - CSymbianOSUnitDocument
//
////////////////////////////////////////////////////////////////////////

#include "SymbianOSUnit.h"

//             The constructor of the document class just passes the
//             supplied reference to the constructor initialisation list.
//             The document has no real work to do in this application.
//
CSymbianOSUnitDocument::CSymbianOSUnitDocument(APPLICATION& aApp)
: DOCUMENT(aApp)
{
}

// destructor
CSymbianOSUnitDocument::~CSymbianOSUnitDocument()
{	
}

// EPOC default constructor can leave.
void CSymbianOSUnitDocument::ConstructL()
{	
}

// Two-phased constructor.
CSymbianOSUnitDocument* CSymbianOSUnitDocument::NewL(APPLICATION& aApp)
{
    CSymbianOSUnitDocument* self = new (ELeave) CSymbianOSUnitDocument( aApp );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop();
	
    return self;
}



//             This is called by the UI framework as soon as the 
//             document has been created. It creates an instance
//             of the ApplicationUI. The Application UI class is
//             an instance of a CEikAppUi derived class.
//
CEikAppUi* CSymbianOSUnitDocument::CreateAppUiL()
{
    return new (ELeave) CSymbianOSUnitAppUi;
}
