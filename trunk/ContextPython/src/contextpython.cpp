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

#include <e32std.h>
#include <Python.h>
#include <symbian_python_ext_util.h>
#include "break.h"
#include "concretedata.h"

#include "bbpython.h"
#include "symbian_python_ptr.h"
#include "context_uids.rh"

#define MODULE_NAME _contextphone
#define MODULE_NAME_STRING "_contextphone"
#define MODULE_INIT_FUNC initcontextphone

PyObject* fnc_BBTimeToUnix(PyObject* self, PyObject* args);
PyObject* fnc_UnixToBBTime(PyObject* self, PyObject* args);

static const PyMethodDef ContextPhone_methods[] = {
	{ "bbsession", (PyCFunction)get_CPBBSession, METH_NOARGS, "gets the singleton Blackboard session" },
	{ "bbtime_to_unix", (PyCFunction)fnc_BBTimeToUnix, METH_VARARGS, "converts the contextphone string (UTC) time into unix time (seconds since epoch)" },
	{ "unix_to_bbtime", (PyCFunction)fnc_UnixToBBTime, METH_VARARGS, "converts a unix time into the contextphone string (UTC)" },
	{ 0, 0 }
};

PyObject* fnc_BBTimeToUnixL(PyObject* args)
{
	char* s=0; int len=0;

	if (!PyArg_ParseTuple(args, "u#", &s, &len)) return 0;
	TBBTime val(TTime(0), KNullDesC);
	val.FromStringL( TPtrC( (TUint16*)s, len ) );
	TDateTime epoch; epoch.Set(1970, EJanuary, 0, 0, 0, 0, 0);
	TTime e(epoch);
	TInt unixtime;
	TTimeIntervalSeconds secs;
	User::LeaveIfError(val().SecondsFrom(e, secs));
	unixtime=secs.Int();
	return PyLong_FromLong(unixtime);
}

PyObject* fnc_UnixToBBTimeL(PyObject* args)
{
	TInt unixtime=0;
	if (!PyArg_ParseTuple(args, "i", &unixtime)) return 0;
	TDateTime d; d.Set(1970, EJanuary, 0, 0, 0, 0, 0);
	TTime e(d);
	e+=TTimeIntervalSeconds(unixtime);
	TBBTime val(e, KNullDesC);
	TBuf<16> s; val.IntoStringL(s);
	return Py_BuildValue("u#", s.Ptr(), s.Length());
}

PyObject* fnc_BBTimeToUnix(PyObject* self, PyObject* args)
{
	PyObject* ret;
	CC_TRAPD(err, ret=fnc_BBTimeToUnixL(args));
	if (err!=KErrNone) return SPyErr_SetFromSymbianOSErr(err);
	return ret;
}
PyObject* fnc_UnixToBBTime(PyObject* self, PyObject* args)
{
	PyObject* ret;
	CC_TRAPD(err, ret=fnc_UnixToBBTimeL(args));
	if (err!=KErrNone) return SPyErr_SetFromSymbianOSErr(err);
	return ret;
}

#define METHOD_TABLE const_cast<PyMethodDef*>(&ContextPhone_methods[0])

#ifdef TESTBUILD
void ZeroGlobal();
#endif

struct TBBID {
	int		id;
	const char* name;
};

static const TBBID Constants1[] = {
	{ CONTEXT_UID_CONTEXTSENSORS, 0 },
	{ 4, "BLUETOOTH_NEIGHBOURS" },
	{ 25, "OWN_BLUETOOTH" },
	{ 13, "PROFILE" },
	{ 24, "BASE" },
	{ 34, "LOCATION" },
	{ 40, "UNREAD_UNANSWERED" },
	{ 18, "USERACTIVITY" },
	{ 33, "USERGIVEN" },
	{ 31, "OWN_PRESENCE" },
	{ 15, "CHARGER", },
	{ 16, "NETWORK_COVERAGE" },
	{ 39, "CURRENT_CALENDAR_EVENTS" },
	{ 11, "CURRENT_APP" },
	{ 1, "CELLID" },
	{ 12, "GPS_NMEA" },
	{ 46, "LAST_GOOD_GPS_NMEA" },
	{ 23, "CELLNAME" },
	{ 42, "CITY" },
	{ 43, "COUNTRY" },
	{ 0, "ANYSENSOR" },
	{ 55, "OUTGOING_FEEDITEM" },
	{ 48, "GIVEN_CELLNAME" },
	{ 49, "GIVEN_CITYNAME" },
	{ CONTEXT_UID_CONTEXTSERVER, 0 },
	{ 1, "OUTGOING_TUPLE_COMPONENT" },
	{ CONTEXT_UID_CONTEXTCOMMON2, 0 },
	{ 1, "SETTINGS" },
	{ 0, 0 }
};

DL_EXPORT(void) MODULE_INIT_FUNC()
{
#ifdef TESTBUILD
	ZeroGlobal();
#endif
	PyObject* module = 
		Py_InitModule(MODULE_NAME_STRING, METHOD_TABLE);
	if (!module) {
		return;
	}
	User::LeaveIfError(def_CPBBSession());
	PyObject* d=PyModule_GetDict(module);

	const TBBID* c=&Constants1[0];
	TInt uid=0;
	while (c->name || c->id) {
		if (c->name) {
			TInt res=PyDict_SetItemString(d, c->name, 
				python_ptr<PyObject>(Py_BuildValue(
					"ii", uid, c->id)
				).get()
			);
		} else {
			uid=c->id;
		}
		c++;
	};
}

#ifndef __S60V3__
GLDEF_C TInt E32Dll(TDllReason)
{
	return KErrNone;
}
#endif
