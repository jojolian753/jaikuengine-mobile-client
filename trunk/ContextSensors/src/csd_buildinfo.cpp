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

#include "csd_buildinfo.h"


EXPORT_C const TTypeName& TBBBuildInfo::Type() const
{
	return KBuildInfoType;
}

EXPORT_C const TTypeName& TBBBuildInfo::StaticType()
{
	return KBuildInfoType;
}

EXPORT_C TBool TBBBuildInfo::Equals(const MBBData* aRhs) const
{
	const TBBBuildInfo* rhs=bb_cast<TBBBuildInfo>(aRhs);
	return (rhs && *rhs==*this);
}

EXPORT_C MBBData* TBBBuildInfo::CloneL(const TDesC& aName) const
{
	bb_auto_ptr<TBBBuildInfo> ret(new (ELeave) TBBBuildInfo( aName));
	*ret=*this;
	return ret.release();
}
_LIT(KWhen, "when");
_LIT(KBuildBy, "buildby");
_LIT(KSdk, "sdk");
_LIT(KBranch, "branch");
_LIT(KMajorVersion, "majorversion");
_LIT(KMinorVersion, "minorversion");
_LIT(KInternalVersion, "internalversion");

EXPORT_C TBBBuildInfo::TBBBuildInfo(const TDesC& aName) : TBBCompoundData(aName) , iWhen(KWhen), iBuildBy(KBuildBy), iSdk(KSdk), iBranch(KBranch), iMajorVersion(KMajorVersion), iMinorVersion(KMinorVersion), iInternalVersion(KInternalVersion) { }

EXPORT_C bool TBBBuildInfo::operator==(const TBBBuildInfo& aRhs) const
{
	return (iWhen == aRhs.iWhen &&
	iBuildBy == aRhs.iBuildBy &&
	iSdk == aRhs.iSdk &&
	iBranch == aRhs.iBranch &&
	iMajorVersion == aRhs.iMajorVersion &&
	iMinorVersion == aRhs.iMinorVersion &&
	iInternalVersion == aRhs.iInternalVersion);
}

EXPORT_C const MBBData* TBBBuildInfo::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iWhen;
	case 1:
		return &iBuildBy;
	case 2:
		return &iSdk;
	case 3:
		return &iBranch;
	case 4:
		return &iMajorVersion;
	case 5:
		return &iMinorVersion;
	case 6:
		return &iInternalVersion;
	default:
		return 0;
	}
}

_LIT(KBuildInfoSpace, " ");
EXPORT_C const TDesC& TBBBuildInfo::StringSep(TUint aBeforePart) const { return KBuildInfoSpace; }

EXPORT_C TBBBuildInfo& TBBBuildInfo::operator=(const TBBBuildInfo& aRhs)
{
	iWhen=aRhs.iWhen;
	iBuildBy=aRhs.iBuildBy;
	iSdk=aRhs.iSdk;
	iBranch=aRhs.iBranch;
	iMajorVersion=aRhs.iMajorVersion;
	iMinorVersion=aRhs.iMinorVersion;
	iInternalVersion=aRhs.iInternalVersion;
	return *this;
}
