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

#include "routes.h"
#include "pointer.h"

CRoutes::CRoutes()
{
}

	CRoutes::~CRoutes()
{
	delete iCellMap;
	delete iSequenceMap;
	delete iSequenceIdMap;
	delete iCurrentCellSequence;
	delete seqbuf;
	delete seqbuf_rev;
	delete iMatch;

	delete predbuf;
}

void CRoutes::ConstructL(MMapFactory* MapFactory, int EpisodeLen)
{
	iMapFactory=MapFactory;
	iCellMap=iMapFactory->CreateCellMapL();
	iSequenceMap=iMapFactory->CreateSequenceMapL();
	iSequenceIdMap=iMapFactory->CreateSequenceIdMapL(EpisodeLen);

	iEpisodeLen=EpisodeLen;

	iCurrentCellSequence=CList<uint16>::NewL();
	seqbuf=new (ELeave) uint16[SEQBUF_LEN];
	seqbuf_rev=new (ELeave) uint16[SEQBUF_LEN];

	predbuf=new (ELeave) uint16[SEQBUF_LEN];
	memset(predbuf, 0, iEpisodeLen);
	predbuf_current=predbuf; predbuf_len=0;

	iMatch=CAMatch::NewL(iEpisodeLen);
}

CRoutes* CRoutes::NewL(MMapFactory* MapFactory, int EpisodeLen)
{
	auto_ptr<CRoutes> ret(new (ELeave) CRoutes);
	ret->ConstructL(MapFactory, EpisodeLen);
	return ret.release();
}

bool CRoutes::AppendCell(uint16*& to_sequence, int& len, int /*maxlen*/, uint16 cell, bool reverse)
{
	bool move=false;
	if (reverse) {
		for (uint16* check=to_sequence; check>to_sequence-len; check--) {
			if (!move) {
				if (*check==cell) move=true;
			} else {
				*(check+1)=*check;
			}
		}
		if (!move) *(to_sequence-len)=cell;
		else *(to_sequence-len+1)=cell;
	} else {
		for (uint16* check=to_sequence; check<to_sequence+len; check++) {
			if (!move) {
				if (*check==cell) move=true;
			} else {
				*(check-1)=*check;
			}
		}
		if (!move) *(to_sequence+len)=cell;
		else *(to_sequence+len-1)=cell;
	}
	return !move;
}

void CRoutes::CellL(uint16 id)
{
	iCurrentCellSequence->AppendL(id);

	bool added=AppendCell(predbuf_current, predbuf_len, iEpisodeLen, id);
	if (predbuf_len==iEpisodeLen-1) {
		if (added) ++predbuf_current;
	} else {
		if (added) ++predbuf_len;
	}
	if (predbuf_current+predbuf_len == predbuf+SEQBUF_LEN) {
		memmove(predbuf, predbuf_current, (predbuf_len+1)*2);
		predbuf_current=predbuf;
	}
//	//std::cout << "CellL, predbuf:";
//	//PrintSequence(std::cout, predbuf_current);
//	//std::cout << "\n";
}

void CRoutes::StopL()
{
	iPrevBase=0;
	predbuf_current=predbuf; memset(predbuf, 0, iEpisodeLen*sizeof(uint16)); predbuf_len=0;
	iCurrentCellSequence->reset();
}

//void CRoutes::PrintSequence(std::ostream& os, const uint16* seq)
//{
//	for (int i=0; i<iEpisodeLen; i++) {
//		os << (int) seq[i] << " ";
//	}
//}

void CRoutes::AddSequence(uint16* seq, uint16 base)
{
	if (!base) {
//		//std::cout << "no base\n";
		return;
	}
	if (!seq[0] || !seq[1]) {
//		//std::cout << "no seq\n";
		return; // seqlen must be 2 or over
	}

	uint32 seqid=iSequenceIdMap->GetSequenceIdL(seq);
	if (!seqid) {
		seqid=iSequenceIdMap->AddSequenceL(seq);
		for (int i=0; i<iEpisodeLen; i++) {
			if ( *(seq+i) != 0 )
				iCellMap->AddMappingL( 
					*(seq+i), seqid);
		}
	}
	iSequenceMap->AddMappingL(seqid, base);
//	PrintSequence(std::cout, seq);
//	std::cout << " -> " << (int) base << "\n";
}

void CRoutes::BaseL(uint16 id)
{
//	//std::cout << "Base " << (int)id << "\n";
	CellL(id);

	uint16 *current_seq=seqbuf, *end=seqbuf+SEQBUF_LEN;
	uint16 *current_seq_rev=seqbuf_rev+SEQBUF_LEN-1, *rev_end=seqbuf_rev-1;
	uint32* seqid; int len=0, len_rev=0;
	memset(seqbuf, 0, iEpisodeLen*sizeof(uint16));
	memset(seqbuf_rev, 0, iEpisodeLen*sizeof(uint16));

	CList<uint16>::Node *n=iCurrentCellSequence->iFirst;
	if (!n) {
		//std::cerr << "no items in list!\n";
		return;
	}

	bool added, added_rev;

	while (n) {
//		//std::cout << "Item " << (int)n->Item << "\n";
		added=AppendCell(current_seq, len, iEpisodeLen, n->Item);
		added_rev=AppendCell(current_seq_rev, len_rev, iEpisodeLen, n->Item, true);
		if (len==iEpisodeLen-1 || !n->Next) {
			if (current_seq[len] != id )
				AddSequence(current_seq, id);
			if (*current_seq_rev != iPrevBase )
				AddSequence(current_seq_rev-len_rev, iPrevBase);
			if (added) ++current_seq;
			if (added_rev) --current_seq_rev;
		} else {
			if (added) ++len;
			if (added_rev) ++len_rev;
		}
		if (current_seq+len==end) {
			memmove(seqbuf, current_seq-1, (len+1)*2);
			current_seq=seqbuf;
		}
		if (current_seq_rev-len_rev==rev_end) {
			memmove( seqbuf_rev+SEQBUF_LEN-len_rev-1, current_seq_rev-len_rev+1, (len_rev+1)*2);
			current_seq_rev=seqbuf_rev+SEQBUF_LEN-len_rev-1;
		}
		n=n->Next;
	}

	iCurrentCellSequence->reset();
	predbuf_current=predbuf; predbuf_len=0; 
	memset(predbuf_current, 0, iEpisodeLen*sizeof(uint16));

	CellL(id);
	iPrevBase=id;
}

struct TMatch {
	uint32	iSeq;
	uint8	iDist;
	TMatch(uint32 Seq, uint8 Dist) : iSeq(Seq), iDist(Dist) { }
	TMatch() { }
	// default copy constructor ok
};

CList<TBaseCount>* CRoutes::PredictL()
{
	auto_ptr< MGenericIntMap > matchmap(iMapFactory->CreateGenericIntMapL());
	auto_ptr< CList< TMatch > > matches(CList<TMatch>::NewL());
	auto_ptr< MGenericIntMap > resmap(iMapFactory->CreateGenericIntMapL());
	auto_ptr< CList< TBaseCount > > results(CList<TBaseCount>::NewL()); 

	bool predbuf_moved=false;
	if (predbuf_current!=predbuf) {
		predbuf_moved=true;
		--predbuf_current;
	}

	uint8 mindist=255;

	CList<TMatch>::Node	*match_n;

//	std::cout << "Compare to: ";
//	PrintSequence(std::cout, predbuf_current);
//	std::cout << "\n";

	for (int ci=0; ci<=predbuf_len; ci++) {
		uint32 seq;

		iCellMap->FindL(predbuf_current[ci]);
		while (seq=iCellMap->NextL()) {
			match_n=(CList<TMatch>::Node*)matchmap->GetData(seq);
			if (!match_n) {
				uint8 dist;
				const uint16* cmp=iSequenceIdMap->GetSequenceL(seq);
				if (cmp) {
					dist=iMatch->Dist( predbuf_current, cmp );

//					PrintSequence(std::cout, cmp); std::cout << "dist " << (int)dist << "\n";

					if (dist<mindist) mindist=dist;
					match_n=matches->AppendL(TMatch(seq, dist ) );
					matchmap->AddDataL(seq, match_n);
				}
			}
		}
	}

	CList<TBaseCount>::Node *res_n;
	if (mindist>=iEpisodeLen-1) {
		mindist=0;
	}

	match_n=matches->iFirst;
	while (match_n) {
		TMatch match=match_n->Item;
		if (match.iDist==mindist) {
			iSequenceMap->FindL(match.iSeq);
			TBaseCount base;
			while ( (base=iSequenceMap->NextL()).Base ) {
				res_n=(CList<TBaseCount>::Node *)resmap->GetData(base.Base);
				if (!res_n) {
					res_n=results->AppendL(base);
					resmap->AddDataL(base.Base, res_n);
				} else {
					res_n->Item.Count+=base.Count;
				}
			}
		}
		match_n=match_n->Next;
	}
	if (predbuf_moved) ++predbuf_current;

	return results.release();
}
