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

#ifndef CPbkSimEntryManager_H
#define CPbkSimEntryManager_H

//  INCLUDES
#include <e32base.h>    // CBase
//#include <customphone.h>

class RCtmGsmPhone;
class RAdvGsmPhoneBook;
class RBasicGsmPhone;
class RGsmPhoneBook;

// FORWARD DECLARATIONS
class MPbkSimEntryManagerObserver;
class CPbkPhoneConnection;
class CPbkSimEntry;
class CPbkSimEntryCache;


// CLASS DECLARATION

class CDbtestAppUi;
/**
 * SIM Phonebook reader and writer.
 */ 
class CPbkSimEntryManager : public CActive
    {
    friend class CDbtestAppUi;
    public: // interface
        /** 
         * Constructor.
         */
        IMPORT_C static CPbkSimEntryManager* NewL(MPbkSimEntryManagerObserver& aObserver);
        
        /**
         * Reads the aSimIndex entry from the SIM card.
         *
         * @return  a new sim entry object. Caller is responsible of the
         *          returned object.
         */
        IMPORT_C CPbkSimEntry* ReadLC(TInt aSimIndex);

        /**
         * Writes aSimEntry to the SIM card.
         * @return  Leave code, if error occurred, KErrNone if success.
         */
        IMPORT_C TInt WriteL(CPbkSimEntry& aSimEntry);
        
        /**
         * Finds a empty SIM location from the SIM card.
         *
         * @return empty SIM entry or NULL if the SIM is full.
         */
        IMPORT_C CPbkSimEntry* FindEmptyEntryL();

        /**
         * The total number of available locations on the SIM card.
         */ 
        IMPORT_C TInt TotalSimEntries() const;

        /** 
         * Has the SIM card write access priviliges.
         */
        IMPORT_C TBool WriteAccess() const;

        /**
         * Maximum length of SIM entry name field.
         */
        IMPORT_C TInt MaxTextLength() const;

        /**
         * Maximum length of SIM entry number field.
         */
        IMPORT_C TInt MaxNumberLength() const;

        /**
         * Destructor
         */
        ~CPbkSimEntryManager();

    private: // from CActive
	    void DoCancel();
	    void RunL();

    private: // implementation
        CPbkSimEntryManager(MPbkSimEntryManagerObserver& aObserver);
        void ConstructL();
        void RequestCacheLoadL();
        void SimCacheReadyL();

    private: // data members
        /// Ref: Sim loading observer
        MPbkSimEntryManagerObserver& iObserver;
        /// Own: Phone Connection
        CPbkPhoneConnection* iPhoneConnection;
        /// Own: Open SIM phonebook
        RAdvGsmPhoneBook iSimPb;
        /// Own: Provides tsy observer service
        RCtmGsmPhone iCtmPhone;
        /// Own: Sim Phonebook Cache
        CPbkSimEntryCache* iSimPhonebookCache;
        /// Own: SIM capabilities
        RBasicGsmPhone::TPhoneBookInfo iSimInfo;
        /// Own: Sim capabilities
        RGsmPhoneBook::TCaps iCaps;

    };


/**
 * SIM Phonebook entry.
 */
class CPbkSimEntry : public CBase
    {
    public: // interface
        /**
         * Constructor
         * @param aSimEntry     The contained sim entry
         * @param aResult       The status of this location:
         *                      KErrNotFound if empty
         *                      KErrArgument if sim location overbound
         *                      KErrNone if OK.
         */
        IMPORT_C static CPbkSimEntry* NewL
            (const RGsmPhoneBook::TEntry& aSimEntry, TInt aResult);
        
        /**
         * Is this a valid SIM location.
         */
        IMPORT_C TBool ValidLocation() const;
        
        /**
         * Returns the name field of this entry.
         */
        IMPORT_C const TDesC& Name() const;
        
        /**
         * Sets aName into the SIM entries name field.
         */
        IMPORT_C void SetName(const TDesC& aName);

        /**
         * Returns the number field of this entry.
         */
        IMPORT_C const TDesC& Number() const;

        /**
         * Sets aName into the SIM entries name field.
         */
        IMPORT_C void SetNumber(const TDesC& aNumber);

        /**
         * Index of the SIM location for this entry.
         */
        IMPORT_C TInt Index() const;

        /**
         * Contained RGsmPhoneBook::TEntry entry.
         */
        const RGsmPhoneBook::TEntry& SimEntry() const;

        /**
         * Is this SIM entry empty.
         */
        IMPORT_C TBool Empty() const;

        /**
         * Destructor
         */
        ~CPbkSimEntry();

    private: // implementation
        CPbkSimEntry(const RGsmPhoneBook::TEntry& aSimEntry, TInt aResult);

    private: // data members
        /// Own: sim entry
        RGsmPhoneBook::TEntry iSimEntry;
        /// Own: the status of the sim location
        TInt iResult;

    };

#endif

// End of File

