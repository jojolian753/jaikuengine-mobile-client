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

#include "bases.h"
#pragma warning (disable: 4706)

#include <e32std.h>
#include <e32math.h>
#include "file_output_base.h"
#include <bautils.h>
#include "cl_settings.h"
#include "db.h"
#include "symbian_tree.h"
#include "bbtypes.h"
#include "bases_test_data.h"
#include "independent.h"
#include <basched.h>
#include "cl_settings.h"
#include "bb_settings.h"
#include "app_context_impl.h"
#include "app_context_fileutil.h"
#include "cl_settings_impl.h"
#include "break.h"

#define LATEST_DBVER	4

#ifndef __WINS__
#define CELL_REFRESH	10*60
#else
//#define CELL_REFRESH	2
#define CELL_REFRESH	10*60
#endif

_LIT(database_file, "bases.db");
_LIT(database_file_install, "bases_inst.db");
_LIT(col_id, "id");	const int orig_colno_id=1;
_LIT(col_t, "t");	const int orig_colno_t=2;
_LIT(col_f, "f");	const int orig_colno_f=3;
_LIT(col_isbase, "isbase");		const int orig_colno_isbase=4;
_LIT(col_last_seen, "last_seen");	const int orig_colno_last_seen=5;
_LIT(col_iscurrent, "iscurrent");	const int orig_colno_iscurrent=6;
_LIT(col_cellid, "cellid");		const int orig_colno_cellid=7;
_LIT(col_merged_to, "merged_to");	const int orig_colno_merged_to=8;
_LIT(col_unaged_t, "unaged_t");		const int orig_colno_unaged_t=9;
_LIT(col_rescaled, "rescaled");		const int orig_colno_rescaled=10;

_LIT(table_cells, "cells");
_LIT(idx_id, "idx_id"); _LIT(idx_t, "idx_t");

const int COMPACT_BETWEEN_UPDATES=15;
const int bases::NO_DB_UPDATE=2;

#ifdef __WINS__
#endif

#include "csd_cell.h"

void bases::NewSensorEventL(const TTupleName& aTuple, const TDesC& , const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("NewSensorEventL"));
#ifdef __WINS__
	//User::Leave(KErrGeneral);
	//return;
#endif

	const TBBCellId* cell=bb_cast<TBBCellId>(aEvent.iData());

	if (aEvent.iPriority()!=CBBSensorEvent::VALUE || !cell) return;

	if (!testing) now=aEvent.iStamp();

	bool missing_data=false;
	if ( cell->iLocationAreaCode()==0 ) {
		if (started) {
			if (aTuple==KNoTuple) {
				now_at_location(cell, -1, false, false, aEvent.iStamp());
			}
			return;
		}
		missing_data=true;
	}

	TInt id=cell->iMappedId();
	current_cell_value=*cell;

	cell_list_node *n=0;
	n=get_single(id);
	while (n && n->merged_to != n->id) {
		id=n->merged_to;
		n=get_single(id);
	}

	if (started && current) {
		// see if we have a gap between stop and start
		if (current->id!=id) {
			// different cell, allow 10 minute difference
			TTimeIntervalMinutes d(10);
			if (previous_time+d < now && !missed_data ) {
				current->f++;
				update_database(current, false);
				current=0;
			}
		} else {
			// same cell, allow 30 minute difference
			TTimeIntervalMinutes d(30);
			if (previous_time+d < now) {
				current->f++;
				update_database(current, false);
				current=0;
			}
		}
		if (!current) iCandidates->reset();
	}
	started=false;

	bool same_loc=true;
	if (current) {
		if (current->id != id) {
			current->f++;
			same_loc=false;
			iGraph->AddEdge(current->id, id);
		} 
#ifdef __S60V3__
		TInt64 diff=now.Int64()-previous_time.Int64();
		double unaged_spent=I64REAL(diff)/(1024.0*1024.0);
#else
		double unaged_spent=(now.Int64()-previous_time.Int64()).GetTReal()/(1024.0*1024.0);
#endif
		if (unaged_spent<0.0) unaged_spent=0.0;
		double spent=scale*unaged_spent;
		current->unaged_t+=unaged_spent;
		MergeEvent(current, unaged_spent, !same_loc);
		previous_time=now;
		move_into_place(current, spent);
		update_database(current, same_loc);
	} else {
		previous_time=now;
		same_loc=false;
	}

	if (missing_data ) {
		now_at_location(cell, -1, false, false, aEvent.iStamp());
		started=true;
		missed_data=true;
		return;
	} else {
		missed_data=false;
	}

	if (!n) {
		n=new (ELeave) cell_list_node(id, now);
		CC_TRAPD(err, cell_hash->AddDataL(id, n));
		if (err!=KErrNone) {
			delete n;
			User::Leave(1);
		}
		n->prev=last_cell;
		if (last_cell) {
			last_cell->next=n; 
			n->pos=last_cell->pos+1;
		}
		last_cell=n;
	}
	if (!first_cell) {
		first_cell=n;
		n->pos=0;
	}

	while (n && n->merged_to != n->id) {
		id=n->merged_to;
		n=(cell_list_node*)cell_hash->GetData(id);
	}

	current=n;
	n->last_seen=now;
	update_database(n, true);

	if (now.DayNoInYear()!=previous_day.DayNoInYear()) {
		scale*=(1.0/aging);
		if (scale>1024.0) {
			rescale();
		}
		previous_day=now;
		bases_info->t=scale;
		update_database(bases_info, false);
	}

	if (!testing) {
		now_at_location(cell, id, n->is_base, !same_loc, aEvent.iStamp());
	}

}

void bases::rescale()
{
	CALLSTACKITEM_N(_CL("bases"), _CL("rescale"));

	bases_info->rescaled++;
	RDebug::Print(_L("Rescaling"));
	cell_list_node *n=first_cell;
	bool seen_top=false;
	total_t/=1024.0;
	while (n) {
		n->t/=1024.0;
		n->rescaled=bases_info->rescaled;
		if (n->prev) n->cum_t=n->prev->cum_t+n->t;
		else n->cum_t=n->t;
		if (!seen_top) {
			n->is_base=true;
			update_database(n, current==n);
		}
		if (n->cum_t > proportion*total_t) seen_top=true;
		n=n->next;
	}
	scale/=1024.0;
}	

EXPORT_C void bases::ConstructL(bool aConvertOnly, CCellMap* aCellMap) {
	TRAPD(err, bases::InnerConstructL(aConvertOnly, aCellMap));
	if (err == KErrCorrupt) {
	    Destruct();
		TFileName database_filename;
		database_filename.Format(_L("%S%S"), &DataDir(), &database_file);
		User::LeaveIfError(BaflUtils::DeleteFile(Fs(), database_filename));
		InnerConstructL(aConvertOnly, aCellMap);
	}
}

void bases::InnerConstructL(bool aConvertOnly, CCellMap* aCellMap)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("ConstructL"));

	iConvertOnly=aConvertOnly;

	if (!aConvertOnly) {
		Mlogger::ConstructL(AppContextAccess());
		Mlog_base_impl::ConstructL();
		iBBSubSessionNotif->AddNotificationL(KCellIdTuple, ETrue);

		iEvent.iData.SetValue(&iLocation); iEvent.iData.SetOwnsValue(EFalse);

		cell_hash=CGenericIntMap::NewL();
		// initial values
		learning_done=false;
		first_time=GetTime();

		previous_time=previous_day=first_time;
		scale=1.0;

	}
	TInt dbver=0;

	MoveIntoDataDirL(AppContext(), database_file);

	TFileName database_filename_install, database_filename;
	database_filename_install.Format(_L("%S%S"), &AppDir(), &database_file_install);
	database_filename.Format(_L("%S%S"), &DataDir(), &database_file);

	if (BaflUtils::FileExists(Fs(), database_filename_install)) {
		User::LeaveIfError(BaflUtils::CopyFile(Fs(), database_filename_install, database_filename));
		User::LeaveIfError(BaflUtils::DeleteFile(Fs(), database_filename_install));
		Settings().WriteSettingL(SETTING_BASEDB_VERSION, LATEST_DBVER);
		learning_done=true;
	}

	dbver=LATEST_DBVER;
	Settings().GetSettingL(SETTING_BASEDB_VERSION, dbver);

	auto_ptr<CDbColSet> cols(CDbColSet::NewL());

	TInt colno=1;
	cols->AddL(TDbCol(col_id, EDbColUint32)); colno_id=colno++;
	cols->AddL(TDbCol(col_t, EDbColReal64) ); colno_t=colno++;
	cols->AddL(TDbCol(col_f, EDbColUint32) ); colno_f=colno++;
	cols->AddL(TDbCol(col_isbase, EDbColBit) ); colno_isbase=colno++;
	cols->AddL(TDbCol(col_last_seen, EDbColDateTime) ); colno_last_seen=colno++;
	cols->AddL(TDbCol(col_iscurrent, EDbColBit) ); colno_iscurrent=colno++;

	TInt store_exists;
	CC_TRAP(store_exists, store = CPermanentFileStore::OpenL(Fs(), database_filename, EFileRead|EFileWrite));
	if (store_exists!=KErrNone) {
		dbver=LATEST_DBVER;
		Settings().WriteSettingL(SETTING_BASEDB_VERSION, LATEST_DBVER);
	}

	if (dbver<2) {
		cols->AddL(TDbCol(col_cellid, EDbColText)); colno_cellid=colno++;
	}
	cols->AddL(TDbCol(col_merged_to, EDbColUint32)); colno_merged_to=colno++;
	cols->AddL(TDbCol(col_unaged_t, EDbColReal64) ); colno_unaged_t=colno++;
	cols->AddL(TDbCol(col_rescaled, EDbColUint32) ); colno_rescaled=colno++;

	if (store_exists==KErrNone) { 
		db.OpenL(store, store->Root());
		User::LeaveIfError(db.Recover());
		db_open=true;

		RDebug::Print(_L("converting database..."));
		User::LeaveIfError(db.AlterTable(table_cells, *cols));
		RDebug::Print(_L("converting database done."));

		User::LeaveIfError(table.Open(db, table_cells));
		table_open=true;
		// read cells in order
		read_from_database(aConvertOnly, aCellMap);
		
	} else {
		if (aConvertOnly) return;

		// construct database
		store = CPermanentFileStore::ReplaceL(Fs(), database_filename, EFileRead|EFileWrite);
		store->SetTypeL(store->Layout());
		TStreamId id=db.CreateL(store);
		db_open=true;
		store->SetRootL(id);
		store->CommitL();

		User::LeaveIfError(db.CreateTable(table_cells, *cols));

		auto_ptr<CDbKey> idx_key(CDbKey::NewL());
		idx_key->AddL(TDbKeyCol(col_id));
		idx_key->MakeUnique();
		User::LeaveIfError(db.CreateIndex(idx_id, table_cells, *idx_key));
		idx_key->Clear();
		idx_key->AddL(TDbKeyCol(col_t));
		User::LeaveIfError(db.CreateIndex(idx_t, table_cells, *idx_key));

		User::LeaveIfError(table.Open(db, table_cells));
		table_open=true;
		User::LeaveIfError(table.SetIndex(idx_id));

		Settings().WriteSettingL(SETTING_BASEDB_VERSION, LATEST_DBVER);
	}
	if (aConvertOnly) return;

	if (! bases_info) {
		bases_info=new (ELeave) cell_list_node(0, first_time);
		bases_info->id=0;
		bases_info->t=scale;
		bases_info->merged_to=0;
		bases_info->last_seen=first_time;
		update_database(bases_info, false);
	}

	iGraph=CDirectedGraph::NewL(db, _L("GRAPH"));
	iMerged=CMerged::NewL(db, _L("MERGE"));
	iCandidates=CList<TCandidate>::NewL();

	iTimer=CTimeOut::NewL(*this);
	iTimer->Wait(CELL_REFRESH);
}

void bases::read_from_database(bool aConvertOnly, CCellMap* aCellMap)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("read_from_database"));

	TInt dbver=0;
	Settings().GetSettingL(SETTING_BASEDB_VERSION, dbver);

	if (!aConvertOnly) {
		clear();

		User::LeaveIfError(table.SetIndex(idx_id));
		bases_info=get_single(0);
		TInt rescalings=0;
		if (bases_info) {
			first_time=bases_info->last_seen;
			scale=bases_info->t;
			rescalings=bases_info->rescaled;
		}

		User::LeaveIfError(table.SetIndex(idx_t));
		TBool rows_left=table.LastL();

		cell_list_node* previous=0;
		int pos=0;		

		total_t=0.0;
		double t;
		while (rows_left) {
			table.GetL();
			TInt id;
			id=table.ColUint32(colno_id);
			t=table.ColReal(colno_t);
			/*
			 * This doesn't quite work. We should still
			 * update the total with the smaller items as well.
			 * And to do that they should be scaled here, and
			 * maybe rewritten to the db, which might take a lot of time.
			 * So leave it for now - MR 20041215
			if (t < limit ) {
				// not reading very small items will allow
				// us lower memory consumption with little
				// accuracy loss in the base decision. They'll
				// still be read by get_single() as necessary
				rows_left=table.PreviousL();
				continue;
			}
			*/
			cell_list_node *n=new (ELeave) cell_list_node(id, table.ColTime(colno_last_seen));
			n->t=t;
			n->f=table.ColUint32(colno_f);
			n->in_db=true;
			n->rescaled=table.ColUint32(colno_rescaled);
			if (table.ColUint32(colno_isbase)==1) {
				n->is_base=true;
			} else {
				n->is_base=false;
			}
			if (n->id==0) {
				// bases info, got it already
				delete n;
			} else {
				// actual cell
				if (table.ColUint32(colno_iscurrent)==1) {
					current=n;
					previous_time=n->last_seen;
					previous_day=previous_time;
				}
				if (dbver<3) {
					n->merged_to=n->id;
					n->unaged_t=0.0;
					n->f=0;
				} else {
					n->merged_to=table.ColUint32(colno_merged_to);
					if (n->merged_to == 0) n->merged_to=n->id;
					n->unaged_t=table.ColReal(colno_unaged_t);
				}
				if (n->t < 0.0) {
					n->t=0.0;
				}
				if (n->unaged_t < 0.0) {
					n->unaged_t=0.0;
				}
				if (previous) {
					previous->next=n;
					n->cum_t=previous->cum_t+n->t;
				} else {
					first_cell=n;
					n->cum_t=n->t;
				}
				n->prev=previous;
				previous=n;
				n->pos=pos++;
				total_t+=n->t;
				cell_hash->AddDataL(n->id, n);
			}
			rows_left=table.PreviousL();
		}
		Settings().WriteSettingL(SETTING_BASEDB_VERSION, LATEST_DBVER);

		User::LeaveIfError(table.SetIndex(idx_id));

		bool needed_rescaling=false;
		db.Begin();
		RDebug::Print(_L("scaling db..."));
		cell_list_node *n=first_cell;
		while (n) {
			TInt i=1;
			if (n->rescaled < bases_info->rescaled && n->t>0) {
				TReal scale;
				TReal pow=bases_info->rescaled-n->rescaled;
				Math::Pow(scale, 1024.0, pow);
				n->t/=scale;
				n->rescaled=bases_info->rescaled;
				update_database(n, false);
				needed_rescaling=true;
				if (i % 50 == 0) {
					db.Commit();
					db.Compact();
				}
			}
			i++;
			n=n->next;
		}
		db.Commit();
		db.Compact();
		RDebug::Print(_L("scaling db done."));

		last_cell=previous;
		if (needed_rescaling) {
			RDebug::Print(_L("sorting..."));
			sort();
			RDebug::Print(_L("done sorting."));
			previous=last_cell;
		}

		if (previous) {
			previous->next=0;
			up_to_date_cum_pos=previous->pos;
			last_cell=previous;
		} else {
			total_t=0;
			up_to_date_cum_pos=0;
		}
	} else {
		table.SetNoIndex();
		TBool rows_left=table.FirstL();
		while (rows_left) {
			table.GetL();
			TInt id;
			id=table.ColUint32(colno_id);
			if (dbver<2 && id>0) {
				aCellMap->SetId(table.ColDes(colno_cellid), id);
			}
			rows_left=table.NextL();
		}
	}
}

void bases::rescale_single(cell_list_node* n)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("rescale_single"));

	if (n->rescaled < bases_info->rescaled && n->t>0) {
		TReal scale;
		TReal pow=bases_info->rescaled-n->rescaled;
		Math::Pow(scale, 1024.0, pow);
		n->t/=scale;
	}
	n->rescaled=bases_info->rescaled;
}

class cell_key : public TKey {
public:
	cell_key(CArrayFixFlat<cell_list_node*> & i_arr) : arr(i_arr) { }
	TInt Compare(TInt aLeft,TInt aRight) const {
		cell_list_node* left=arr[aLeft];
		cell_list_node* right=arr[aRight];
		if (left->t > right->t) return -1;
		if (left->t < right->t) return 1;
		return 0;
	}
private:
	CArrayFixFlat<cell_list_node*>& arr;
};

class cell_swap : public TSwap {
public:
	cell_swap(CArrayFixFlat<cell_list_node*> & i_arr) : arr(i_arr) { }
	void Swap(TInt aLeft,TInt aRight) const {
		cell_list_node* tmp=arr[aLeft];
		arr[aLeft]=arr[aRight];
		arr[aRight]=tmp;
	}
private:
	CArrayFixFlat<cell_list_node*>& arr;
};

void bases::sort()
{
	CALLSTACKITEM_N(_CL("bases"), _CL("sort"));

	auto_ptr< CArrayFixFlat<cell_list_node*> > arr(new CArrayFixFlat<cell_list_node*>(256));
	arr->SetReserveL( last_cell->pos );
	cell_list_node* n=first_cell;
	while (n) {
		arr->AppendL(n);
		n=n->next;
	}
	User::QuickSort(arr->Count(), cell_key(*arr), cell_swap(*arr));
	int i;

	double cum_t=0.0;
	first_cell=last_cell=0;
	cell_list_node* prev=0;
	cell_list_node** put_into=&first_cell;
	for (i=0; i<arr->Count(); i++) {
		n=(*arr)[i];
		cum_t+=n->t;
		n->cum_t=cum_t;
		n->pos=i;

		n->prev=prev;
		prev=n;
		*put_into=n;
		put_into=&(n->next);
	}
	last_cell=n;
	total_t=cum_t;
	up_to_date_cum_pos=n->pos;
	n->next=0;
}

EXPORT_C TInt bases::GetBaseId(TInt id)
{
	cell_list_node *n=0;
	n=get_single(id);
	while (n && n->merged_to != n->id) 
	{
		id=n->merged_to;
		n=get_single(id);
	}
	if (n->is_base == true)
	{
		return n->id;
	}
	else
	{
		return -1;
	}
}


cell_list_node* bases::get_single(TInt id)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("get_single"));

	cell_list_node *n;
	n=(cell_list_node*)cell_hash->GetData(id);
	if (n) return n;

	TDbSeekKey rk(id);
	if (! table.SeekL(rk) ) {
		return 0;
	}
	table.GetL();

	n=new (ELeave) cell_list_node(id, table.ColTime(colno_last_seen));
	n->t=table.ColReal(colno_t);
	n->f=table.ColUint32(colno_f);
	n->in_db=true;
	n->merged_to=table.ColUint32(colno_merged_to);
	if (n->merged_to==0) n->merged_to=n->id;
	n->unaged_t=table.ColReal(colno_unaged_t);
	n->rescaled=table.ColUint32(colno_rescaled);
	if (table.ColUint32(colno_isbase)==1) n->is_base=true;
	if (n->t < 0.0) n->t=0.0;
	if (n->unaged_t < 0.0) n->unaged_t=0.0;

	if (id>0) {
		rescale_single(n);
		cell_hash->AddDataL(id, n);
		n->prev=last_cell;
		if (last_cell) {
			last_cell->next=n;
			n->cum_t=last_cell->cum_t+n->t;
			n->pos=last_cell->pos+1;
		} else {
			n->cum_t=n->t;
			n->pos=0;
		}
		last_cell=n;
		if (! first_cell) first_cell=n;
	}

	return n;
}
	
void bases::move_into_place(cell_list_node* n, double t)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("move_into_place"));

	// Invariant:
	// the top proportion*total_t nodes have up to date cum_t
	//
	// no other nodes can move up to that group than the
	// one we're moving, so updating nodes under it, if
	// they are within the group is enough to keep the invariant
	//
	// so even though we update cum_t for all nodes on the
	// way when moving the node, not all of them are 
	// necessarily correct

	if (!n) {
		post_error(_L("int error: n is NULL"), KErrGeneral);
		return;
	}

	total_t+=t;
	cell_list_node* update=n->next;
	n->t+=t;
	
	if (n->pos<=up_to_date_cum_pos) {
		if (n->pos==up_to_date_cum_pos) up_to_date_cum_pos=n->pos+1;
		if (n->next) n->next->cum_t=n->cum_t+n->next->t+t;
	}

	while (n->prev && n->prev->t < n->t) {
		cell_list_node* tmp;
		tmp=n->prev;

		if (n->next)
			n->next->prev=tmp;
		else last_cell=tmp;
		tmp->next=n->next;
		n->next=tmp;
		n->prev=tmp->prev;
		tmp->prev=n;

		if (n->prev) {
			n->prev->next=n;
			n->cum_t=n->prev->cum_t+n->t;
			n->pos=n->prev->pos+1;
			if (n->prev->pos==up_to_date_cum_pos) {
				up_to_date_cum_pos+=2;
				update=n->next;
			}
		} else {
			n->cum_t=n->t;
			n->pos=0;
		}
		n->next->cum_t=n->cum_t+n->next->t;	// (1)
		n->next->pos=n->pos+1;
	}
	if (!n->prev) {
		first_cell=n;
		n->cum_t=n->t; // if n was first, cum_t hasn't been updated yet
		n->pos=0;
	} else {
		n->cum_t=n->prev->cum_t+n->t; // if n wasn't moved, cum_t hasn't been updated yet
	}

	if (update && update->pos <= up_to_date_cum_pos) {
		while (update && (update->cum_t)<proportion*total_t) {
			// the first one has been updated by (1) or (2)
			up_to_date_cum_pos=update->pos;
			update=update->next;
			if (!update || !update->prev) {
				post_error(_L("int error, update->prev is NULL"), KErrGeneral);
			} else 
				if (update) update->cum_t=update->prev->cum_t+update->t;
			
		}
		if (update) {
			if (! update->prev ) {
				post_error(_L("int error, update->prev is NULL"), KErrGeneral);
			} else {
				update->cum_t=update->prev->cum_t+update->t;
				up_to_date_cum_pos=update->pos;
			}
		}
	}

	if (!learning_done ) {
		TTimeIntervalDays learning_period(2);

		if (! (first_time+learning_period > previous_time) ) {
			learning_done=true;
			cell_list_node* b=first_cell;
			while (b && b->cum_t<proportion*total_t) {
				add_as_base(b);
				update_database(b, false);
				b=b->next;
			}
			if (b) {
				add_as_base(b);
				update_database(b, false);
			}
		}
	} else {
		if (	(
				( ( (n->cum_t)<(proportion*total_t) ) && n->pos<=up_to_date_cum_pos) || 
				(!(n->prev)) ||
				(n->prev && ( (n->prev->cum_t)<(proportion*total_t) ) && n->prev->pos<=up_to_date_cum_pos) 
			)
				&& !n->is_base) 


			add_as_base(n);
	}

}

void bases::add_as_base(cell_list_node* n)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("add_as_base"));

	n->is_base=true;
}

void bases::PutL()
{
	CALLSTACKITEM_N(_CL("bases"), _CL("PutL"));

	table.PutL();
	if (update_count==COMPACT_BETWEEN_UPDATES || NoSpaceLeft() ) {
		update_count=0;
		if (! db.InTransaction() ) db.Compact();
	}
	++update_count;
}

void bases::update_database(cell_list_node* n, bool is_current)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("update_database"));

	if (test_flags & NO_DB_UPDATE) return;

	if (!n) {
		post_error(_L("int error, n is NULL"), KErrGeneral);
		return;
	}

	if (n->in_db) {
		TDbSeekKey rk(n->id);
		TInt ret;
		if (! (ret=table.SeekL(rk)) ) {
			// TODO: do what?
			User::Leave(ret);
		}
		table.UpdateL();
	} else {
		if (NoSpaceLeft()) return;

		table.InsertL();
		if (colno_cellid>0) table.SetColL(colno_cellid, _L(""));
		table.SetColL(colno_id, n->id);
	}
	table.SetColL(colno_t, n->t);
	table.SetColL(colno_f, n->f);
	table.SetColL(colno_merged_to, n->merged_to);
	table.SetColL(colno_unaged_t, n->unaged_t);
	table.SetColL(colno_rescaled, bases_info->rescaled);

	TUint32 isbase=0;
	if (n->is_base) { isbase=1; }
	table.SetColL(colno_isbase, isbase);
	table.SetColL(colno_last_seen, n->last_seen);

	TUint32 iscurrent=0;
	if (is_current) iscurrent=1;
	table.SetColL(colno_iscurrent, iscurrent);

	PutL();

	n->in_db=true;

}

EXPORT_C void bases::test(COperatorMap* aOpMap, CCellMap* aCellMap)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("test"));

#ifdef __WINS__ 
	aOpMap->AddRef();

	iTimer->Reset();

	testing=true; bool first=true;
	test_flags=0;
	scale=1.0; proportion=0.9;
	learning_done=false;

	RFile testf;
	TFileText test;
	int ts_len=15;
	TBuf<128> filen;
	TBuf<256> line;
	TBuf<30> id_d, dt_d;
	filen.Append(DataDir());
	filen.Append(_L("bases_test_data.txt"));

	op=Cfile_output_base::NewL(AppContext(), _L("bases"));
	Cfile_output_base& o=*op;

	CBBSensorEvent e(KCell, KCellIdTuple);
	TBBCellId cell(KCell);
	e.iData.SetValue(&cell); e.iData.SetOwnsValue(EFalse);

	if (testf.Open(Fs(), filen, 
		EFileShareAny | EFileStreamText | EFileRead)==KErrNone) {
		CleanupClosePushL(testf);
		test.Set(testf);
		int j=0;
	

		//int beg=23000, end=230000;
		int beg=0, end=0;

		while ( test.Read(line) == KErrNone && j < end) {
		//for (int i=0;i<TEST_DATA_COUNT; i++) {
			if (! (j % 1000) ) {
				TBuf<40> msg;
				msg.Format(_L("Testing at %d"), j);
				RDebug::Print(msg);
			}
			j++;
			if (j>=beg) {
				dt_d=line.Left(ts_len);
				id_d=line.Mid(ts_len+1);
				
				TTime time(dt_d);
				now=time;
				if (first) { 
					previous_time=now; first_time=now; first=false; 
					previous_day=previous_time;
				}
				if (! id_d.Compare(_L("SWITCH"))) {
					started=true;
				} else {
					e.iStamp()=time;
					CCellMap::Parse(id_d, cell.iCellId(), cell.iLocationAreaCode(), cell.iShortName());
					if (cell.iCellId()==0 && cell.iLocationAreaCode()==0) {
						cell.iMCC()=0;
						cell.iMNC()=0;
					} else {
						aOpMap->NameToMccMnc(cell.iShortName(), cell.iMCC(),
 cell.iMNC());
					}
					cell.iMappedId()=aCellMap->GetId(cell);
					NewSensorEventL(KNoTuple, KNullDesC, e); // name is ignored
				}
			}
		}
		CleanupStack::PopAndDestroy(); // testf
	}

	e.iData.SetValue(0);

	if (! (test_flags & NO_DB_UPDATE) ) {
		read_from_database(false, 0);
	}

	line.Format(_L("total %f\n"), total_t);
	o.write_to_output(line);
	cell_list_node* n=first_cell;
	while (n) {
		TInt base=0;
		if (n->is_base) base=1;
		line.Format(_L("%d: t %f f %d cum_t %f base %d merged to %d\n"),
			n->id, n->t, n->f, n->cum_t, base, n->merged_to);
		o.write_to_output(line);
		n=n->next;
	}

	o.write_to_output(_L("MAPPINGS:\n"));
	aCellMap->PrintMapping(o);

	delete op;

	User::LeaveIfError(table.SetIndex(idx_id));

	testing=false;
	//clear();

	iTimer->Wait(CELL_REFRESH);
	aOpMap->Release();
#endif
}

_LIT(KLocation, "now_at_location");

EXPORT_C bases::bases(MApp_context& Context) : Mlog_base_impl(Context, KLocation, KLocationTuple, CELL_REFRESH+30),
	testing(false), first_cell(0), last_cell(0), current(0), 
	bases_info(0), store(0), table_open(false), db_open(false), started(true), 
	up_to_date_cum_pos(0), current_cell_value(KCell), update_count(0) {

	aging=0.9;
	proportion=0.9;
}

void bases::clear(bool deleting)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("clear"));

	up_to_date_cum_pos=0;

	cell_list_node* n=first_cell;
	while (n) {
		cell_list_node* tmp;
		tmp=n->next;
		delete n;
		n=tmp;
	}
	delete cell_hash; cell_hash=0;
	if (!deleting) cell_hash=CGenericIntMap::NewL();
	first_cell=last_cell=current=0;
	delete bases_info; bases_info=0;
	if (iCandidates) iCandidates->reset();
}

EXPORT_C bases::~bases() {
	Destruct();
}

void bases::Destruct() 
{
	CALLSTACKITEM_N(_CL("bases"), _CL("~bases"));

	if (iConvertOnly) {
		TInt err;
		if (table_open) { CC_TRAP(err, table.Close()); table_open = false; }
		if (db_open) { 
			CC_TRAP(err, db.Compact());
			CC_TRAP(err, db.Close());
			db_open = false;
		}
		delete store; store = NULL;
		return;
	}

	delete iTimer; iTimer = NULL;

	TInt err;
	if (current) {
		// FIXME:
		TTime time=GetTime();

		CBBSensorEvent e(KCell, KCellIdTuple);
		e.iStamp=time;
		e.iData.SetValue(&current_cell_value); e.iData.SetOwnsValue(EFalse);

		CC_TRAP(err, NewSensorEventL(KNoTuple, KNullDesC, e));
		e.iData.SetValue(0);
	}

	CC_TRAP(err, clear(true));

	delete iCandidates; iCandidates = NULL;
	delete iGraph; iGraph = NULL;
	delete iMerged; iMerged = NULL;

	if (table_open) { CC_TRAP(err, table.Close()); table_open = false; }
	if (db_open) { 
		CC_TRAP(err, db.Compact());
		CC_TRAP(err, db.Close());
		db_open = false;
	}
	delete store; store = NULL;

	delete cell_hash; cell_hash = NULL;
}


_LIT(CLASS_NAME, "bases");

template<typename T> inline T max(T x, T y) {
	if (x>y) return x;
	return y;
}

template<typename T> inline T min(T x, T y) {
	if (x<y) return x;
	return y;
}

void bases::MergeEvent(cell_list_node* cell, double spent, bool visit)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("MergeEvent"));

	TTime test(_L("20030220:174858"));
	if (test==now) {
		// FIXME : Why do we do this ?
		// x never used afterwards
		// int x=1;
	}
	CList<TCandidate>::Node *n;
	n=iCandidates->iFirst;
	while (n && n->Item.cell != cell) n=n->Next;


	if (n) {
		n->Item.this_stay+=spent;
		if ( n->Item.this_stay > n->Item.max_stay ) n->Item.max_stay=n->Item.this_stay;
		if (visit) {
			n->Item.count++;
			n->Item.this_stay=0.0;
			n->Item.prev_stay=n->Item.this_stay;
		}
		n->Item.time+=spent;
		iCandidates->MoveToTop(n);
		return;
	} else if (iCandidates->iFirst) {
		auto_ptr< CList< TInt > > s(CList< TInt >::NewL());

		double max_avg_stay=0.0, max_max_stay=0.0, sum_time=0.0;
		int k=0, min_count=99;
		bool some_item_is_base=false;

		n=iCandidates->iFirst;
		while (n) {
			k++;
			max_avg_stay=max(max_avg_stay, n->Item.cell->unaged_t / (double)n->Item.cell->f);
			max_max_stay=max(max_max_stay, n->Item.max_stay);
			sum_time+=n->Item.time;
			min_count=min(min_count, n->Item.count);
			if (min_count<2) break;
			s->AppendL(n->Item.cell->merged_to);
			if (n->Item.cell->is_base) some_item_is_base=true;
			if (		k>1 &&
					sum_time > k*max_avg_stay &&
					sum_time > k*max_max_stay &&
					sum_time > 10.0*60.0 &&
					some_item_is_base &&
					iGraph->CheckDiameter( *s ) ) {
				MergeCells(*s);
				iCandidates->reset();
				break;
			}
			n=n->Next;
		}
	}
	if (iCandidates->iCount == 2) {
		iCandidates->DeleteLast();
		TCandidate &c=iCandidates->iFirst->Item;
		c.max_stay=c.time=c.prev_stay;
		c.count=1;
	}
	if (iCandidates->iCount>2) {
		User::Leave(-1006);
	}
	iCandidates->Push(TCandidate(cell, spent, visit));
}

void bases::MergeCells( CList< TInt >& Candidates)
{
	CALLSTACKITEM_N(_CL("bases"), _CL("MergeCells"));

	RDebug::Print(_L("Merging Cells..."));
	double max_time=0.0;
	CList< TInt >::Node *n=Candidates.iFirst;
	cell_list_node *merge_to=0, *cand_n=0;
	while (n) {
		cand_n=(cell_list_node*)cell_hash->GetData(n->Item);
		if (cand_n->t > max_time) {
			max_time=cand_n->t;
			merge_to=cand_n;
		}
		n=n->Next;
	}
	if (op) {
		op->write_time(now);
		op->write_to_output(_L("merging: "));
	}
	TBuf<30> id;
	n=Candidates.iFirst;
	double added_time=0.0;
	auto_ptr< CList<TInt> > cells_in_merge_to( iMerged->GetCells(merge_to->id) );

	while (n) {
		cand_n=(cell_list_node*)cell_hash->GetData(n->Item);
		if (cand_n != merge_to ) {
			auto_ptr< CList<TInt> > cells_in_merge_from( iMerged->GetCells(cand_n->id) );
			if (op) {
				id.Format(_L("%d "), n->Item);
				op->write_to_output(id);
			}
			cand_n->merged_to=merge_to->id;
			added_time+=cand_n->t;
			merge_to->unaged_t+=cand_n->unaged_t;
			merge_to->f+=cand_n->f;

			CList<TInt>::Node *to_i, *from_i;
			for (from_i=cells_in_merge_from->iFirst; from_i; from_i=from_i->Next) {
				for (to_i=cells_in_merge_to->iFirst; to_i; to_i=to_i->Next) {
					merge_to->f-=iGraph->GetCount(from_i->Item, to_i->Item);
					merge_to->f-=iGraph->GetCount(to_i->Item, from_i->Item);
				}
				iMerged->Add(from_i->Item, merge_to->id);
			}
			cand_n->t=0;
			
			// FIXME: isn't correct if several to-be-merged
			// are adjacent in the list
			cand_n->cum_t=cand_n->prev->cum_t;
			// TODO: what should be done with the count?
			update_database(cand_n, false);
		}
		n=n->Next;	
	}
	if (op) {
		id.Format(_L(" into %d\n"), merge_to->id);
		op->write_to_output(id);
	}
	total_t-=added_time;
	move_into_place(merge_to, added_time);
	update_database(merge_to, merge_to==current);
}

void bases::expired(CBase*)
{
	TTime time;
	time=GetTime();

	CBBSensorEvent e(KCell, KCellIdTuple); 
	e.iData.SetOwnsValue(EFalse);
	e.iData.SetValue(&current_cell_value); e.iStamp()=time;
	NewSensorEventL(KNoTuple, KNullDesC, e);
	e.iData.SetValue(0);
	iTimer->Wait(CELL_REFRESH);
}

void bases::now_at_location(const TBBCellId* cell, TUint locationid, bool is_base, bool loc_changed, TTime t)
{
	iLocation.iCellId=*cell;
	iLocation.iLocationId()=locationid;
#ifndef __WINS__
	iLocation.iIsBase()=is_base;
#else //FIXME
	iLocation.iIsBase()=1;
#endif
	iLocation.iLocationChanged()=loc_changed;
	iLocation.iEnteredLocation()=t;

	post_new_value(&iLocation);
}

class CDirectedGraphImpl: public CDirectedGraph, public MDBStore {
public:
	virtual void AddEdge(TInt From, TInt To);
	virtual bool CheckDiameter(CList<TInt>& Nodes);
	virtual int  GetCount(TInt From, TInt To);
	virtual ~CDirectedGraphImpl();

	friend class CDirectedGraph;
private:
	CDirectedGraphImpl(RDbStoreDatabase& Db);
	void ConstructL(const TDesC& TableName);
};

CDirectedGraph* CDirectedGraph::NewL(RDbStoreDatabase& Db, const TDesC& TableName)
{
	CALLSTACKITEM_N(_CL("CDirectedGraph"), _CL("NewL"));

	auto_ptr<CDirectedGraphImpl> ret(new (ELeave) CDirectedGraphImpl(Db));
	ret->ConstructL(TableName);
	return ret.release();
}

CDirectedGraph::~CDirectedGraph()
{
	CALLSTACKITEM_N(_CL("CDirectedGraph"), _CL("~CDirectedGraph"));

}

void CDirectedGraphImpl::AddEdge(TInt From, TInt To)
{
	CALLSTACKITEM_N(_CL("CDirectedGraphImpl"), _CL("AddEdge"));

	TDbSeekMultiKey<2> rk;
	rk.Add(From); rk.Add(To);
	TAutomaticTransactionHolder th(*this);

	if (! iTable.SeekL(rk)) {
		iTable.InsertL();
		iTable.SetColL(1, From);
		iTable.SetColL(2, To);
		iTable.SetColL(3, 1);
		PutL();
	} else {
		iTable.GetL();
		TInt count=iTable.ColUint32(3)+1;
		iTable.UpdateL();
		iTable.SetColL(3, count);
		PutL();
	}
}

int  CDirectedGraphImpl::GetCount(TInt From, TInt To)
{
	CALLSTACKITEM_N(_CL("CDirectedGraphImpl"), _CL("GetCount"));

	TDbSeekMultiKey<2> rk;
	rk.Add(From); rk.Add(To);
	if (! iTable.SeekL(rk)) {
		return 0;
	} else {
		iTable.GetL();
		return iTable.ColUint32(3);
	}
}

bool CDirectedGraphImpl::CheckDiameter(CList<TInt>& Nodes)
{
	CALLSTACKITEM_N(_CL("CDirectedGraphImpl"), _CL("CheckDiameter"));

	auto_ptr<CGenericIntMap> to_remove(CGenericIntMap::NewL());
	CList<TInt>::Node *n, *n2;
	n=Nodes.iFirst;
	while (n) {
		n2=Nodes.iFirst;
		while (n2) {
			if (n->Item != n2->Item) to_remove->AddDataL(n2->Item, (void*)1);
			n2=n2->Next;
		}
		TDbSeekKey rk(n->Item);
		if (iTable.SeekL(rk)) {
			TInt from, to;
			iTable.GetL();
			from=iTable.ColUint32(1);
			while (from==n->Item) {
				to=iTable.ColUint32(2);
				to_remove->DeleteL(to);
				if (! iTable.NextL() ) from=-1;
				else {
					iTable.GetL();
					from=iTable.ColUint32(1);
				}
			}
		}
		if (to_remove->Count() == 0 ) return true;
		to_remove->Reset();
		n=n->Next;
	}
	return false;
}

CDirectedGraphImpl::~CDirectedGraphImpl()
{
	CALLSTACKITEM_N(_CL("CDirectedGraphImpl"), _CL("~CDirectedGraphImpl"));

}

CDirectedGraphImpl::CDirectedGraphImpl(RDbStoreDatabase& Db) : MDBStore(Db)
{
	CALLSTACKITEM_N(_CL("CDirectedGraphImpl"), _CL("CDirectedGraphImpl"));

}

void CDirectedGraphImpl::ConstructL(const TDesC& TableName)
{
	CALLSTACKITEM_N(_CL("CDirectedGraphImpl"), _CL("ConstructL"));

	TInt cols[]={ EDbColUint32, EDbColUint32, EDbColUint32, -1 };
	TInt idx[]={ 1, 2, -1 };

	MDBStore::ConstructL(cols, idx, true, TableName);
	SwitchIndexL(0);
}

class CMergedImpl : public CMerged, public MDBStore {
public:
	virtual void Add(TInt Merged, TInt To);
	virtual CList<TInt>* GetCells(TInt In);
	virtual ~CMergedImpl();

	friend class CMerged;
private:
	CMergedImpl(RDbStoreDatabase& Db);
	void ConstructL(const TDesC& TableName);
};

CMerged* CMerged::NewL(RDbStoreDatabase& Db, const TDesC& TableName)
{
	CALLSTACKITEM_N(_CL("CMerged"), _CL("NewL"));

	auto_ptr<CMergedImpl> ret(new (ELeave) CMergedImpl(Db));
	ret->ConstructL(TableName);
	return ret.release();
}

CMerged::~CMerged()
{
	CALLSTACKITEM_N(_CL("CMerged"), _CL("~CMerged"));

}

void CMergedImpl::Add(TInt Merged, TInt To)
{
	CALLSTACKITEM_N(_CL("CMergedImpl"), _CL("Add"));

	TDbSeekMultiKey<2> rk;
	rk.Add(To); rk.Add(Merged);
	if (! iTable.SeekL(rk)) {
		TAutomaticTransactionHolder th(*this);
		iTable.InsertL();
		iTable.SetColL(1, To);
		iTable.SetColL(2, Merged);
		PutL();
	}
}

CList<TInt>* CMergedImpl::GetCells(TInt In)
{
	CALLSTACKITEM_N(_CL("CMergedImpl"), _CL("GetCells"));

	auto_ptr< CList<TInt> > cells(CList<TInt>::NewL());
	TDbSeekKey rk(In);
	if (iTable.SeekL(rk)) {
		TInt i=In, m;
		iTable.GetL();
		while (i==In) {
			m=iTable.ColUint32(2);
			cells->AppendL(m);
			if (iTable.NextL()) {
				iTable.GetL();
				i=iTable.ColUint32(1);
			} else {
				i=-1;
			}
		}
	}
	cells->AppendL(In);
	return cells.release();
}

CMergedImpl::~CMergedImpl()
{
	CALLSTACKITEM_N(_CL("CMergedImpl"), _CL("~CMergedImpl"));

}

CMergedImpl::CMergedImpl(RDbStoreDatabase& Db) : MDBStore(Db)
{
	CALLSTACKITEM_N(_CL("CMergedImpl"), _CL("CMergedImpl"));

}

void CMergedImpl::ConstructL(const TDesC& TableName)
{
	CALLSTACKITEM_N(_CL("CMergedImpl"), _CL("ConstructL"));

	TInt cols[]={ EDbColUint32, EDbColUint32, -1 };
	TInt idx[]={ 1, 2, -1 };

	MDBStore::ConstructL(cols, idx, true, TableName);
	SwitchIndexL(0);
}

class COwnActiveScheduler : public CBaActiveScheduler {
public:
	void Error(TInt aError) const {
		User::Leave(aError);
	}
};

class CStopActive : public CActive {
public:
	CStopActive() : CActive(EPriorityNormal) { }
	void ConstructL(worker_info *wi) { 
		CActiveScheduler::Add(this);
		wi->set_do_stop(&iStatus);
		SetActive();
	}
	void RunL() {
		CActiveScheduler::Stop();
	}
	void DoCancel() {
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrNone);
	}
	~CStopActive() {
		Cancel();
	}
};

void do_run_bases(TAny* aPtr)
{
	worker_info *wi=(worker_info*)aPtr;

	auto_ptr<COwnActiveScheduler> s(new (ELeave) COwnActiveScheduler);
	CActiveScheduler::Install(s.get());
	auto_ptr<CStopActive> stop(new (ELeave) CStopActive);
	stop->ConstructL(wi);

	auto_ptr<CApp_context> appc(CApp_context::NewL(true, wi->name));
#ifndef __S60V3__
	appc->SetDataDir(_L("c:\\system\\data\\context\\"), false);
#endif
	TNoDefaults t;
	CBlackBoardSettings* settings=
		CBlackBoardSettings::NewL(*appc, t, KCLSettingsTuple);
	appc->SetSettings(settings);

	auto_ptr<CBBDataFactory> bbf(CBBDataFactory::NewL());
	auto_ptr<CBBSession> bbs(CBBSession::NewL(*appc, bbf.get()));
	appc->SetBBSession(bbs.get());
	appc->SetBBDataFactory(bbf.get());

	auto_ptr<bases> b(new (ELeave) bases(*appc));
	b->ConstructL(false, 0);

	s->Start();
}

EXPORT_C TInt bases::RunBasesInThread(TAny* aPtr)
{
    CTrapCleanup *cl;
    cl=CTrapCleanup::New();

    TInt err=0;
    CC_TRAP2(err,
            do_run_bases(aPtr), 0);

	delete cl;

	TTimeIntervalMicroSeconds32 w(50*1000);
	User::After(w);
	worker_info* wi=(worker_info*)aPtr;
	wi->stopped(err);
    return err;
}
