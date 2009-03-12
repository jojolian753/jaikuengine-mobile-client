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

#ifndef __JAIKUTOOL_APPUI_H__
#define __JAIKUTOOL_APPUI_H__

#include <aknappui.h>

// Forward reference
class CJaikuToolAppView;

/*! 
  @class CJaikuToolAppUi
  
  @discussion An instance of class CJaikuToolAppUi is the UserInterface part of the AVKON
  application framework for the JaikuTool example application
  */
class CJaikuToolAppUi : public CAknAppUi
    {
public:
/*!
  @function ConstructL
  
  @discussion Perform the second phase construction of a CJaikuToolAppUi object
  this needs to be public due to the way the framework constructs the AppUi 
  */
    void ConstructL();

/*!
  @function CJaikuToolAppUi
  
  @discussion Perform the first phase of two phase construction.
  This needs to be public due to the way the framework constructs the AppUi 
  */
    CJaikuToolAppUi();


/*!
  @function ~CJaikuToolAppUi
  
  @discussion Destroy the object and release all memory objects
  */
    ~CJaikuToolAppUi();
	virtual void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane *aMenuPane);

public: // from CAknAppUi
/*!
  @function HandleCommandL
  
  @discussion Handle user menu selections
  @param aCommand the enumerated code for the option selected
  */
    void HandleCommandL(TInt aCommand);
    void ResetAllDataL();
    void StopJaikuL();
    void SendLogsL();
    void SetDebugLogsEnabled(TBool aEnabled);
    TBool DebugLogsEnabled();
    void DataDir(TDes& aInto);
    void DebugLogDir(TDes& aInto);
    TBool JaikuDataExists();

private:
	TErrorHandlerResponse HandleError(TInt aError,
	     const SExtendedError& aExtErr,
	     TDes& aErrorText,
	     TDes& aContextText);

/*! @var iAppView The application view */
    CJaikuToolAppView* iAppView;
    TChar iDrive;
#ifdef __S60V3__
    class CSendUi*	iSendUi;
#else
	class CSendAppUi*	iSendUi;
#endif
    class CGPS* iGPS;
    };


#endif // __JAIKUTOOL_APPUI_H__
