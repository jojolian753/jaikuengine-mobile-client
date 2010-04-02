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

#if !defined(BASES_H_INCLUDED)

#define BASES_H_INCLUDED

#include "i_logger.h"
#include "i_log_source.h"
#include "file_output_base.h"
#include "app_context.h"
#include <d32dbms.h>
#include <f32file.h>
#include <s32file.h>

#include "bases_test_data.h"
#include "symbian_tree.h"
#include "list.h"
#include "csd_cell.h"
#include "operatormap.h"
#include "cellmap.h"
#include "csd_base.h"
#include "log_base_impl.h"

#ifdef __WINS__
#pragma warning(disable : 4121) // 'alignment of a member was sensitive to packing' for the test data- we don't care
#endif

struct cell_list_node {
	int	 id;
	int	 merged_to;
	int	 f;
	double	 t;
	double   unaged_t;
	double	 cum_t;
	bool	 is_base;
	bool	 in_db;
	int	 pos;
	int	 rescaled;

	TTime	 last_seen;
	cell_list_node *prev, *next;

	cell_list_node(uint32 i_id, TTime i_time) : 
		id(i_id), merged_to(i_id), f(0), t(0), unaged_t(0), cum_t(0),
		is_base(false), in_db(false), rescaled(0), last_seen(i_time), 
		prev(0), next(0)  { }
};

class CDirectedGraph : public CBase {
public:
	static CDirectedGraph* NewL(RDbStoreDatabase& Db, const TDesC& TableName);
	virtual void AddEdge(TInt From, TInt To) = 0;
	virtual int  GetCount(TInt From, TInt To) = 0;
	virtual bool CheckDiameter(CList<TInt>& Nodes) = 0;
	virtual ~CDirectedGraph();
};

class CMerged : public CBase {
public:
	static CMerged* NewL(RDbStoreDatabase& Db, const TDesC& TableName);
	virtual void Add(TInt Merged, TInt To) = 0;
	virtual CList<TInt>* GetCells(TInt In) = 0;
	virtual ~CMerged();
};

class bases : public CBase, public Mlogger, public Mlog_base_impl, public MTimeOut {
public:
	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent);

	IMPORT_C bases(MApp_context& Context);
	IMPORT_C virtual ~bases();
	IMPORT_C void ConstructL(bool aConvertOnly, CCellMap* aCellMap=0); 
	// only transfer old mappings?, if yes, give a cellmap
	IMPORT_C TInt GetBaseId(TInt id);

	IMPORT_C void test(COperatorMap* aOpMap, CCellMap* aCellMap);

	IMPORT_C static TInt RunBasesInThread(TAny* aPtr);
private:
	virtual void expired(CBase*);
	void InnerConstructL(bool aConvertOnly, CCellMap* aCellMap=0);
	void Destruct();

	CGenericIntMap*	cell_hash;
	TTime	previous_time;
	TTime	first_time;
	TTime	now;
	TTime	previous_day;
	bool	testing;
	cell_list_node *first_cell, *last_cell, *current;
	cell_list_node *bases_info; // we keep scale and first_time in the same table
				    // via a special row/node with id==0
	TBBCellId current_cell_value;
	double scale;
	double total_t;

	RDbStoreDatabase db;
	CFileStore* store;
	RDbTable table;

	CDirectedGraph *iGraph;
	CMerged		*iMerged;

	bool table_open, db_open;

	double proportion;
	bool learning_done;
	double aging;
	bool started, missed_data;
	int up_to_date_cum_pos;

	TInt	test_flags;
	const static TInt NO_DB_UPDATE;

	int update_count; // for compacting every k updates

	Cfile_output_base*	op;

	void move_into_place(cell_list_node* n, double t);
	void add_as_base(cell_list_node* n);
	void update_database(cell_list_node* n, bool is_current);
	void read_from_database(bool aConvertOnly, CCellMap* aCellMap);
	void MergeEvent(cell_list_node* n, double spent, bool visit);
	void sort();
	void rescale_single(cell_list_node* n);
	cell_list_node* get_single(TInt id);
	void PutL();
	void now_at_location(const TBBCellId* cell, TUint locationid, bool is_base, bool loc_changed, TTime t);

	struct TCandidate {
		double	time;
		int	count;
		double	max_stay;
		double	this_stay, prev_stay;
		cell_list_node*	cell;

		TCandidate() { }
		TCandidate(cell_list_node* n, double i_time, bool visit) : time(i_time), count(1),
			max_stay(i_time), this_stay(i_time), cell(n) { if (!visit) count=0; }
	};
		
	CList<TCandidate> *iCandidates;
	void MergeCells( CList< TInt >& Candidates);

	void clear(bool deleting=false);
	void rescale();

	int colno_id;
	int colno_t;
	int colno_f;
	int colno_isbase;
	int colno_last_seen;
	int colno_iscurrent;
	int colno_cellid;
	int colno_merged_to;
	int colno_unaged_t;
	int colno_rescaled;

	CTimeOut*	iTimer;
	TBBLocation	iLocation;
	bool		iConvertOnly;

#ifdef __WINS__
#pragma warning(disable : 4121) // doesn't work :-(, see MS KB
public:
	static const unsigned short* bases::test_data[TEST_DATA_COUNT] [2];
#endif
};

#endif
