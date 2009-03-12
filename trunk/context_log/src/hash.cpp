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

#include "hash.h"

#include <e32std.h>
#include "app_context.h"

hash::hash_node *hash::addL(const TDesC& str, void* data, bool overwrite)
{
	unsigned int number;
	hash_node *newnode;

	if (!str.Length())
		return NULL;


	newnode = new hash_node;
	newnode->next = NULL;
	newnode->data = data;

	newnode->str = HBufC::NewL(str.Length()+1);
	*(newnode->str)=str;
	number = hash_number(str);
	newnode->number = number;
	unsigned int scaled_number;
	scaled_number = number % table_size;

	hash_node_p *temp;
	int depth = 1;
	for (temp = &table[scaled_number]; *temp;
	     depth++, temp = &(*temp)->next) {
		if (number == (*temp)->number
		    && !((*temp)->str->Des().Compare(str)) ) {
			delete newnode->str;
			delete newnode;
			if (! overwrite ) {
				return NULL;
			} else {
				if (deleter) (*deleter)((*temp)->data);
				(*temp)->data=data;
				return *temp;
			}
		}
	}
	*temp = newnode;

	if (depth == 1)
		filled++;
	if (depth > maxdepth)
		maxdepth = depth;
	avg_depth =
	    avg_depth * (((float) nodecount) / (float (nodecount + 1)))
	    + ((float) depth) / ((float) (nodecount + 1));
	nodecount++;

	if (depth > depth_raja
	    && nodecount > (fill_limit * table_size)) {
		hash_node_p *new_table;

		maxdepth = 0;
		filled = 0;
		nodecount = 0;
		avg_depth = 0.0;

		unsigned int new_size;
		new_size = table_size * 2 + 1;
		new_table = new hash_node_p[new_size];
		unsigned int i;
		for (i = 0; i < new_size;
		     new_table[i++] = NULL);
		hash_node *prev = NULL, *pprev = NULL;

		hash_node_p *new_temp;
		for (i = 0; i < table_size; i++) {
			for (temp = &table[i]; *temp;
			     temp = &(*temp)->next)
			{

				pprev = prev;
				prev = (*temp);
				scaled_number = (*temp)->number % new_size;
				depth = 1;
				for (new_temp =
				     &new_table[scaled_number];
				     *new_temp;
				     new_temp =
				     &((*new_temp)->next), depth++);
				(*new_temp) = new hash_node;
				(*new_temp)->str =
				    (*temp)->str;
				(*temp)->str=0;
				(*new_temp)->number = (*temp)->number;
				(*new_temp)->data = (*temp)->data;
				(*temp)->data=0;
				(*new_temp)->next = NULL;

				if (depth == 1)
					filled++;
				if (depth > maxdepth)
					maxdepth = depth;
				avg_depth =
				    avg_depth * (((float) nodecount) /
						 (float (nodecount + 1)))
				    +
				    ((float) depth) /
				    ((float) (nodecount + 1));
				nodecount++;
			}
		}
		Clear();
		delete[]table;

		table_size = new_size;
		table = new_table;
	}


	return newnode;
}

hash::hash(void(*delete_func)(void* data)) : table(0), table_size(0), deleter(delete_func)
{

}

void hash::Clear()
{

	// TODO: doesn't update statistics,
	// must be split to two parts, so that
	// the memory release code can be called
	// separately
	hash_node_p temp; hash_node_p pp=NULL;
	for (unsigned int i = 0; i < table_size; i++) {
		temp=table[i];
		while (temp) {
			delete (temp)->str;
			if (deleter) (*deleter)((temp)->data);
			pp=temp;
			temp = (temp)->next;
			delete pp;
		}
		table[i]=0;
	}
}


hash::~hash()
{

	Clear();
	delete [] table;
}

unsigned int hash::hash_number(const TDesC& str)
{

	unsigned int number = 0, len, *ci;
	TText* cc;
	len = str.Length() * sizeof(TText) / sizeof(unsigned int);
	const TText* ptr=str.Ptr();
	for (ci = (unsigned int *) ptr;
	     ci < (unsigned int *) ptr + len; ci++) {
		number ^= *ci;
		number <<= 1;
	}
	len=str.Length();
	for (cc = (TText *) ci; cc < ptr+len; cc++) {
		number ^= (unsigned int) *cc;
		number <<= 1;
	}
	return number;
}


void hash::ConstructL(unsigned int initialsize)
{

	if (initialsize == 0)
		table_size = 13;
	else
		table_size = initialsize;
	table = new (ELeave) hash_node_p[table_size];
	for (unsigned int i = 0; i < table_size; table[i++] = NULL);

	depth_raja = 4;
	fill_limit = 0.75;

	maxdepth = 0;
	filled = 0;
	nodecount = 0;
	avg_depth = 0.0;

}

hash::hash_node_p hash::find_node(const TDesC& str)
{

	if (!str.Length())
		return NULL;
	unsigned int number, scaled_number;
	number = hash_number(str);
	scaled_number = number % table_size;
	hash_node_p retval;
	for (retval = table[scaled_number]; retval;
	     retval = retval->next) {
		if (number == retval->number
		    && !str.Compare(retval->str->Des()))
			return retval;
	}
	return NULL;
}

void *hash::find(const TDesC& str)
{

	hash_node_p r=find_node(str);
	if (r) return r->data;
	return 0;
}


unsigned int hash::get_filled()
{

	return filled;
}

int hash::get_maxdepth()
{

	return maxdepth;
}
unsigned int hash::get_size()
{

	return table_size;
}

float hash::get_avg_depth()
{

	return avg_depth;
}
unsigned int hash::get_nodecount()
{

	return nodecount;
}


