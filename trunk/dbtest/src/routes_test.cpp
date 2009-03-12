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

#include "routes_test.h"

const unsigned short * const routes_test::weekdays[7]= {
	(unsigned short*)L"Mon",
	(unsigned short*)L"Tue",
	(unsigned short*)L"Wed",
	(unsigned short*)L"Thu",
	(unsigned short*)L"Fri",
	(unsigned short*)L"Sat",
	(unsigned short*)L"Sun",
};

void routes_test::ConstructL(CRoutes* routes, MMapFactory* factory, Mroutes_status* statuscb)
{
	bases=factory->CreateGenericIntMapL();
	for (int l=0; l<3; l++) {
		areas[l]=factory->CreateGenericIntMapL();
	}

	for (i=0; i<BASE_COUNT; i++) {
		bases->AddDataL(base[i], (void*)(i+1));
	}

	for (i=0; i<AREA_COUNT; i++) {
		areas[area[i][0]]->AddDataL( area[i][1], (void*)area[i][2] );
	}
	cb=statuscb;
	iRoutes=routes;
	timer.CreateLocal();
	stationary=CStationary::NewL(5, 3);
	CActiveScheduler::Add(this);
	for (i=0; i<8; i++) stats[i]=0;
	for (i=0; i<3; i++) prev_hier[i]=0;
	next_base=-1;
}

routes_test::routes_test(): CActive(EPriorityNormal)
{
}

routes_test* routes_test::NewL(CRoutes* routes, MMapFactory* factory, Mroutes_status* statuscb)
{
	auto_ptr<routes_test> ret(new (ELeave) routes_test);
	ret->ConstructL(routes, factory, statuscb);
	return ret.release();
}

routes_test::~routes_test()
{
	Cancel();
	for (int l=0; l<3; l++) {
		delete areas[l];
	}
	delete bases;
	timer.Close();
}

void routes_test::DoCancel()
{
	timer.Cancel();
}

void routes_test::RunL()
{
	c=data[i];
	buf.Format(_L("%d"), c);
	cb->cell(buf);

	TTime t(_L("19700101:000000"));
	t+=TTimeIntervalSeconds(time[i]);

	TDateTime dt=t.DateTime();
	buf.Format(_L("%s %02d/%02d %02d:%02d"), weekdays[t.DayNoInWeek()],
		t.DayNoInMonth()+1, dt.Month()+1,
		dt.Hour(), dt.Minute());
	cb->date(buf);

	bool no_pred=true;

	if (c==0) {
		iRoutes->StopL();
		next_base=-1;
		stationary->Reset();
	} else if ( idx=(int)bases->GetData(c) ) {
		iRoutes->BaseL(c);
		next_base=-1;
		for (int l=0; l<3; l++) {
			prev_hier[l]=(int)areas[l]->GetData(c);
		}
		base_dist=0;
	} else {
		if (next_base==-1) {
			for (int j=i; j<DATA_COUNT; j++) {
				next_base=data[j];
				if ( (idx=(int)bases->GetData(data[j])) || next_base==0) {
					if (next_base==0) {
						cb->next_base(_L("NA (stop)"));
					} else {
						if (idx>0 && idx<=BASE_COUNT) {
							buf=name[idx-1];
						} else {
							buf=_L("ERROR");
							cb->next_base(buf);
							return;
						}
						buf=name[idx-1];
						cb->next_base(buf);
					}
					break;
				}
			}
		}
		++base_dist;
		iRoutes->CellL(c);

		if (stationary->IsStationary(c) || base_dist==1) {
			stats[4]++;
			cb->prediction(_L("stationary"));
			//std::cout << "stationary " << base_dist << "\n";
		} else if(next_base==0) {
			cb->prediction(_L(""));
			stats[5]++;
		} else {
			CList<TBaseCount>* pred=iRoutes->PredictL();
			CList<TBaseCount>::Node* n=pred->iFirst;
			int predbase=0; int maxcount=0;
			buf=_L("no prediction");
			while (n) {
				if (n->Item.Count>maxcount) {
					predbase=n->Item.Base;
					maxcount=n->Item.Count;
				}
				//std::cout << n->Item.Base << "(" << n->Item.Count << ") ";
				n=n->Next;
			}
			delete pred;

			if (maxcount>0) {
				int hier=2;
				int l;
				bool correct;
				for (l=0; l<3; l++) {
					int pred_area=(int)areas[l]->GetData(predbase);
					if (pred_area) {
						if (prev_hier[l]==pred_area) {
							hier=l-1;
							break;
						}
					}
				}
				idx=(int)bases->GetData(predbase);
				if (idx>0 && idx<=BASE_COUNT) {
					buf=name[idx-1];
				} else {
					buf=_L("ERROR");
					cb->next_base(buf);
					return;
				}
				no_pred=false;
				if (hier>-1 && predbase>0) {
					int next_area=(int)areas[hier]->GetData(next_base);
					int pred_area=(int)areas[hier]->GetData(predbase);
					if (next_area!=0 && pred_area!=0 && next_area==pred_area) {
						stats[6]++;
						correct=true;
					} else if (predbase==next_base) {
						stats[6]++;
						correct=true;
					} else {
						stats[0]++;
						correct=false;
					}
					//buf.Append(_L(" (area) "));
				} else {
					if (predbase==0) {
						stats[5]++;
						no_pred=true;
						correct=false;
					} else if (predbase==next_base) {
						stats[1]++;
						correct=true;
					} else {
						stats[0]++;
						correct=false;
					}
				}
				if (next_base>0) {
					if (correct) {
						buf.Append(_L(" (T)"));
					} else {
						buf.Append(_L(" (F)"));
					}
				} else {
					no_pred=true;
				}
			} else {
				stats[3]++;
			}
			cb->prediction(buf);
			float correct=( (float)(stats[1]+stats[6])) / 
				( (float)(stats[0]+stats[1]+stats[6]) +1 );
			float predictions=( (float)(stats[1]+stats[6]+stats[0])) / 
				( (float)(stats[0]+stats[1]+stats[6]+stats[3]) +1 );
			buf.Format(_L("%2.2f  %2.2f"), correct, predictions);
			cb->correct(buf);
		}
	}
	i++;
	if (i<DATA_COUNT) {
		TTimeIntervalMicroSeconds32 w1(1000*100);
		TTimeIntervalMicroSeconds32 w(600*1000);
		if (no_pred) {
			timer.After(iStatus, w1);
		} else {
			timer.After(iStatus, w);
		}
		SetActive();
	}
}

void routes_test::run_test()
{
	TTimeIntervalMicroSeconds32 w(1000*1000);
	timer.After(iStatus, w);
	SetActive();
	i=0;
}
