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

#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

//#include<string.h>

//#include<values.h>

#include <e32std.h>

class hash {
public:
	struct hash_node {
		HBufC* str;
		unsigned int number;
		void* data;
		hash_node *next;
	};
	typedef hash_node *hash_node_p;
      private:

	hash_node_p * table;
	unsigned int table_size;

	int depth_raja;
	float fill_limit;
	int maxdepth;
	unsigned int filled;
	float avg_depth;
	unsigned int nodecount;

	unsigned int hash_number(const TDesC& str);
	hash_node_p find_node(const TDesC& str);

      public:
	void (*deleter)(void* data);

	hash_node * addL(const TDesC& str, void* data, bool overwrite=false);

	hash( void(*delete_func)(void* data)=0 );
	~hash();

	void ConstructL(unsigned int initialsize=0);
	void Clear();

	void *find(const TDesC& str);

	unsigned int get_filled();
	int get_maxdepth();
	unsigned int get_size();
	float get_avg_depth();
	unsigned int get_nodecount();

};

#endif

/*

int main(void)
{
	hash taul(11981);
	int jarj = 1;
	int *temp_int;
	FILE *f;
	char sana[512];
	f = fopen("/usr/lib/ispell/ispell.words", "r");
	printf("\nLisätään:      ");
	while (fscanf(f, "%s", sana) != EOF) {
		printf("\010\010\010\010\010%5d", jarj);
		temp_int = new int;
		*temp_int = jarj++;
		taul.add(sana, temp_int);
	}
	rewind(f);
	jarj = 1;
	printf("\nHaetaan:       ");
	while (fscanf(f, "%s", sana) != EOF) {
		printf("\010\010\010\010\010%5d", jarj);
		temp_int = (int *) taul.find(sana);
		if (!temp_int || *temp_int != jarj)
			fprintf(stderr, "Virhe sanassa %d\n", jarj);
		jarj++;
	}
	printf("\n");
	printf("\nAlkioita:                      %d", taul.get_nodecount());
	printf("\nTaulukon size:                 %d", taul.get_size());
	printf("\nMaksimidepth taulukossa:      %d",
	       taul.get_maxdepth());
	printf("\nKeskiarvodepth:               %f",
	       taul.get_avg_depth());
	printf("\nTaulukon indeksien käyttöaste: %f",
	       (((float) taul.get_filled()) /
		((float) taul.get_size())));
	printf("\n");
	return 0;
}

*/
