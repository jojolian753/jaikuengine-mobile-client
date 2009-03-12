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

#include "match.h"

#define ARRAY(i, j) array[(i)*iLen+(j)]

CAMatch::CAMatch()
{
}

	CAMatch::~CAMatch()
{
	delete [] array;
}

void CAMatch::ConstructL(int Len)
{
	iLen=Len;
	array=new (ELeave) uint8[ (Len+1)*(Len+1) ];

	for (int i=0; i<=Len; i++) {
		ARRAY(i, 0)=i;
		ARRAY(0, i)=i;
	}
}

CAMatch* CAMatch::NewL(int Len)
{
	auto_ptr<CAMatch> ret(new (ELeave) CAMatch);
	ret->ConstructL(Len);
	return ret.release();
}

inline uint8 min(uint8 a, uint8 b, uint8 c)
{
	uint8 tmp;
	if (a<b) tmp=a;
	else tmp=b;

	if (tmp<c) return tmp;
	return c;
}

uint8 CAMatch::Dist(const uint16 *s1, const uint16 *s2)
{
	int max_i, max_j;
	max_i=max_j=iLen;
	for (int j=1; j<=iLen; j++) {
		if (s2[j-1]==0) {
			max_j=j;
			break;
		}
		for (int i=1; i<=iLen; i++) {
			if (s1[i-1]==0) {
				max_i=i;
				break;
			}
			ARRAY(i, j)=min(
					ARRAY(i-1, j-1) + (s1[i-1]==s2[j-1] ? 0 : 1),
					ARRAY(i-1, j)+1, ARRAY(i, j-1)+1);
		}
	}
	return ARRAY(max_i, max_j);
}
