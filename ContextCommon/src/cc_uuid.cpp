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

#include "cc_uuid.h"

#include "cc_imei.h"
#include "app_context.h"
#include "jabberdata.h"
#include "sha1.h"

#include <e32math.h>

class CUuidGeneratorImpl : public CUuidGenerator, public MContextBase
{
public:
	CUuidGeneratorImpl(const TDesC& aUserNick,
					   TUid aComponentUid, 
					   TInt aComponentId) : 
		iComponentUid(aComponentUid), 
		iComponentId(aComponentId) { iUserNick=aUserNick; }

	void ConstructL()
	{
		CALLSTACKITEM_N( _CL("CUuidGeneratorImpl"), _CL("ConstructL") );
		iImei = CImei::NewL(iComponentUid, iComponentId);
		TTime now;
		now.UniversalTime();
		iSeed = now.Int64();
	}

	virtual ~CUuidGeneratorImpl() 
	{
		CALLSTACKITEM_N(_CL("CUuidGenerator"), _CL("~CUuidGenerator"));		
		delete iImei;
	}


	virtual void MakeUuidL(TBuf8<16>& aInto)
	{
		CALLSTACKITEM_N(_CL("CUuidGenerator"), _CL("MakeUuidL"));

		// 16 bytes global uid
		// 10 first bytes: imei + nick hash
		// 4 next byte: timestamp
		// 2 next bytes: random value
		if ( iHashedIdentity.Length() == 0 )
			{
				HashIdentityL();
			}
		aInto.Zero();
		aInto.Copy( iHashedIdentity );

		AppendTimestampL( aInto );
		AppendRandomL( aInto );
	}

	void AppendRandomL(TDes8& aInto)
	{
		CALLSTACKITEM_N( _CL("CUuidGeneratorImpl"), _CL("AppendRandomL") );
		TInt random = Math::Rand( iSeed );
		TPtrC8 randomPtr( reinterpret_cast<const TUint8*>(&random), 2 );
		aInto.Append( randomPtr );
	}


	void AppendTimestampL(TDes8& aInto)
	{
		CALLSTACKITEM_N( _CL("CUuidGeneratorImpl"), _CL("AppendTimestampL") );
		TTime now;
		now.UniversalTime();

		TInt64 now64 = now.Int64();
		TInt low  = I64LOW( now64 );
		TInt high = I64HIGH( now64 );
		
		TPtrC8 lowPtr( reinterpret_cast<const TUint8*>(&low), 2 );
		TPtrC8 highPtr( reinterpret_cast<const TUint8*>(&high), 2 );
		aInto.Append( lowPtr );
		aInto.Append( highPtr );
	} 
					   
	void HashIdentityL()
	{
		CALLSTACKITEM_N( _CL("CUuidGeneratorImpl"), _CL("HashIdentityL") );
		iDigest.Zero();
		iDigest.Copy( iImei->GetL() );

		
		TPtrC8 nick8(reinterpret_cast<const TUint8*>( iUserNick.Ptr() ),( iUserNick.Length()*2) );
		iDigest.Append( nick8 );
		
		auto_ptr<SHA1> sha1( new SHA1 );
		sha1->Input((char*)(iDigest.Ptr()),iDigest.Size());
		unsigned int message_digest_array[5];
		
		sha1->Result(message_digest_array);
		
		TPtrC8 hashed(reinterpret_cast<const TUint8*>( message_digest_array ), 10 );
		iHashedIdentity.Copy(hashed);
	}

private:
	TBuf<100> iUserNick;
	CImei* iImei;
	TUid iComponentUid;
	TInt iComponentId;
	
	TInt64 iSeed;
	TInt iCounter;

	TBuf8<10> iHashedIdentity;
	TBuf8<200> iDigest;
};


EXPORT_C CUuidGenerator* CUuidGenerator::NewL(const TDesC& aUserNick, TUid aComponentUid, TInt aComponentId) 
{
	CALLSTACKITEMSTATIC_N(_CL("CUuidGenerator"), _CL("NewL"));
	auto_ptr<CUuidGeneratorImpl> self( new (ELeave) CUuidGeneratorImpl(aUserNick, aComponentUid, aComponentId) );
	self->ConstructL();
	return self.release();
}
