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

#include "bbpython.h"
#include <symbian_python_ext_util.h>
#include "app_context.h"
#include "app_context_impl.h"
#include "symbian_auto_ptr.h"
#include "symbian_python_ptr.h"
#include "cbbsession.h"
#include "break.h"
#include "bbutil.h"
#include "pythonexternalizer.h"
#include "bbdata.h"
#include "cc_uuid.h"

enum TCPBBPanics
{
	EPanicSessionAlreadyClosed
};

static void Panic(TInt aReason)
{
	_LIT(KPanicCat, "ContextPhoneBB");
	User::Panic(KPanicCat, aReason);
}

struct obj_CPBBSession : public PyObject {
	int ob_size;
	CBBSession* iSession;
	CApp_context* iContext; 
	CUuidGenerator* iUuidGenerator;
	// set if we own the app_context, i.e., if not running embedded in a ContextPhone app
};

PyObject* MakePythonXmlL(const MBBData* aData);

class CBBProxy : public MBBObserver, public CBase {
public:
	static CBBProxy* NewL(PyObject* aCb);
	~CBBProxy() {
		Py_XDECREF(iCb);
		delete iExternalizer;
	}
private:
	PyObject* iCb;
	CPythonExternalizer* iExternalizer;
	void ConstructL(PyObject* aCb) {
		iCb=aCb;
		Py_INCREF(iCb);
	}
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) {
			PyEval_RestoreThread(PYTHON_TLS->thread_state);

			CC_TRAPD(err, NewValueL2(aId, aName, aSubName,
				aComponentName, aData));

			PyEval_SaveThread();
		};
	virtual void NewValueL2(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) {
			python_ptr<PyObject> data(0);
			/*
			if (!iExternalizer) {
				iExternalizer=CPythonExternalizer::NewL();
			}
			iExternalizer->Reset();
			aData->IntoXmlL(iExternalizer, EFalse);
			data.reset(iExternalizer->GetObject());
			iExternalizer->Reset();
			*/
			data.reset(MakePythonXmlL(aData));
			Callback(aName, aSubName, data.get());

		}
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) {
		PyEval_RestoreThread(PYTHON_TLS->thread_state);

		CC_TRAPD(err, DeletedL2(aName, aSubName));

		PyEval_SaveThread();
	}
	virtual void DeletedL2(const TTupleName& aName, const TDesC& aSubName) {
		Py_INCREF(Py_None);
		python_ptr<PyObject> none(Py_None);
		Callback(aName, aSubName, none.get());
	}
	void Callback(const TTupleName& aName, 
		const TDesC& aSubName, PyObject* o) {
			
			python_ptr<PyObject> tn( LeaveIfPythonNull(Py_BuildValue(
				"ii", aName.iModule, aName.iId)) );
			python_ptr<PyObject> subname(0);
			if (aSubName.Length()>0) {
				subname.reset(LeaveIfPythonNull(Py_BuildValue(
					"u#", aSubName.Ptr(), aSubName.Length())) );
			} else {
				Py_INCREF(Py_None);
				subname.reset(Py_None);
			}
			PyObject* res=PyObject_CallFunction(iCb, "(OOO)",
				tn.get(), subname.get(), o);
			Py_XDECREF(res);
	}
};

CBBProxy* CBBProxy::NewL(PyObject* aCb)
{
	auto_ptr<CBBProxy> ret(new (ELeave) CBBProxy);
	ret->ConstructL(aCb);
	return ret.release();
}

struct obj_CPBBSubSession : public PyObject {
	int ob_size;
	obj_CPBBSession* iSession;
	CBBSubSession* iSubSession;
	CBBProxy* iProxy;
};

#ifdef TESTBUILD
	obj_CPBBSession* GlobalBBSession;
	void ZeroGlobal() {
		GlobalBBSession=0;
	}
#endif

_LIT(KPython, "python");

obj_CPBBSession* get_CPBBSessionL()
{
#ifdef TESTBUILD
	Py_XINCREF(GlobalBBSession);
	if (GlobalBBSession) return GlobalBBSession;
#else
	{
		obj_CPBBSession* s=(obj_CPBBSession*)Dll::Tls();
		Py_XINCREF(s);
		if (s) return s;
	}
#endif

	PyTypeObject* typeObject = 
		reinterpret_cast<PyTypeObject*>(
		SPyGetGlobalString("_contextphone.bbsession")
		);
	if (!typeObject) User::Leave(KErrGeneral);

	python_ptr<obj_CPBBSession> self(PyObject_New(obj_CPBBSession, typeObject));
	if (!self) return 0;
	CApp_context* ctx=CApp_context::Static();
	if (! ctx ) {
		ctx=self->iContext=CApp_context::NewL(true, _L("contextpython"));
	} else {
		self->iContext=0;
	}
	self->iSession=ctx->BBSession();
	if (!(self->iSession)) {
		MBBDataFactory* f=ctx->BBDataFactory();
		if (!f) {
			auto_ptr<CBBDataFactory> fp(CBBDataFactory::NewL());
			ctx->TakeOwnershipL(fp.get());
			ctx->SetBBDataFactory(fp.release());
		}
		f=ctx->BBDataFactory();
		auto_ptr<CBBSession> bbs(CBBSession::NewL(*ctx, f));
		ctx->TakeOwnershipL(bbs.get());
		ctx->SetBBSession( bbs.release() );
		self->iSession=ctx->BBSession();
	}
	self->iUuidGenerator=CUuidGenerator::NewL(KPython, KUidContextPython, 1);
	return self.release();
}

PyObject* get_CPBBSession(PyObject* self, PyObject* args)
{
	obj_CPBBSession* s;
	CC_TRAPD(err, s=get_CPBBSessionL());
	if (err!=KErrNone) return SPyErr_SetFromSymbianOSErr(err);
	return s;
}

void del_CPBBSession(obj_CPBBSession *self)
{
	delete self->iUuidGenerator;
	// BBsession is owned either by the app_context, or somebody else
	// we delete the context if we own it, deleting the session
	// if the context is owned by somebody else, we don't delete anything
	delete self->iContext;
	PyObject_Del(self);
#ifdef TESTBUILD
	GlobalBBSession=0;
#else
	Dll::SetTls(0);
#endif
}

void del_CPBBSubSession(obj_CPBBSubSession *self)
{
	delete self->iSubSession;
	delete self->iProxy;
	Py_XDECREF(self->iSession);
}

PyObject* fnc_CPBBSession_SubSession(obj_CPBBSession* self, PyObject* args)
{
	PyObject* cb=0;
	if (!PyArg_ParseTuple(args, "O", &cb)) return 0;
	if (!PyCallable_Check(cb)) return 0;
	
	PyTypeObject* typeObject = 
		reinterpret_cast<PyTypeObject*>(
		SPyGetGlobalString("_contextphone.bbsubsession")
		);
	if (!typeObject) return 0;

	python_ptr<obj_CPBBSubSession> subs(
		PyObject_New(obj_CPBBSubSession, typeObject));
	if (!subs) return 0;

	subs->iProxy=0;
	subs->iSession=self;
	Py_INCREF(self);
	subs->iProxy=CBBProxy::NewL(cb);
	subs->iSubSession=self->iSession->CreateSubSessionL(subs->iProxy);
	return subs.release();
}

PyObject* fnc_CPBBSession_Get(obj_CPBBSession* self, PyObject* args)
{
	int module_uid=-1, module_id=-1, l=0;
	char* subname = 0;
	if (!PyArg_ParseTuple(args, "ii|u#", &module_uid, &module_id, &subname, &l) &&
		!PyArg_ParseTuple(args, "(ii)|u#", &module_uid, &module_id, &subname, &l))
	{
		return 0;
	}
	PyErr_Clear();

	MBBData* d=0;
	TInt err=KErrNone;

	Py_BEGIN_ALLOW_THREADS;
	TTupleName tn={module_uid, module_id};
	TPtrC subn( (TUint16*)subname, l );
	CC_TRAPD(err, self->iSession->GetL(tn, subn, d, ETrue) );
	Py_END_ALLOW_THREADS;

	if (err!=KErrNone) {
		return SPyErr_SetFromSymbianOSErr(err);
	}
	if (!d) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	bb_auto_ptr<MBBData> dp(d);
	
	return MakePythonXmlL(dp.get());
	/*
	auto_ptr<CPythonExternalizer> e(CPythonExternalizer::NewL());
	d->IntoXmlL(e.get(), EFalse);

	return e->GetObject();
	*/
}

#include "bbxml.h"

void PutL(obj_CPBBSession* self, const TTupleName& tn, const TDesC& subn, const TDesC8& xmlp, TTime expires,
	TBool aIsRequest=EFalse, TComponentName cn=KNoComponent)
{
		CApp_context* ctx=CApp_context::Static();
		auto_ptr<CSingleParser> p(CSingleParser::NewL(0, EFalse, ETrue, 
			ctx->BBDataFactory()));
		p->ParseL(xmlp);
		MBBData* d=0;
		d=p->Data();
		CBBSession* s=self->iSession;
		if (aIsRequest) {
			s->PutRequestL(tn, subn, d, expires, cn);
		} else {
			s->PutL(tn, subn, d, expires);
		}
}

PyObject* fnc_CPBBSession_Put2(obj_CPBBSession* self, PyObject* args, TBool aRequest)
{
	int module_uid=-1, module_id=-1, subn_l=0, xml_l=0, ts=0;
	int component_uid=-1, component_id=-1;
	char* subname = 0, *xml=0;
	if (!PyArg_ParseTuple(args, "iiu#u#i|ii", &module_uid, &module_id, &subname, &subn_l, &xml, &xml_l, &ts, &component_uid, &component_id) &&
		!PyArg_ParseTuple(args, "(ii)u#u#i|(ii)", &module_uid, &module_id, &subname, &subn_l, &xml, &xml_l, &ts, &component_uid, &component_id))
	{
		return 0;
	}
	PyErr_Clear();

	TInt err=KErrNone;

	//Py_BEGIN_ALLOW_THREADS;
	
	TDateTime d; d.Set(1970, EJanuary, 0, 0, 0, 0, 0);
	TTime expires(d);
	expires+=TTimeIntervalSeconds(ts);
	
	TTupleName tn={module_uid, module_id};
	TPtrC subn( (TUint16*)subname, subn_l );
	TPtrC8 xmlp( (const TUint8*)xml, xml_l*2);
	TComponentName cn={ component_uid, component_id };
	CC_TRAP(err, PutL(self, tn, subn, xmlp, expires, aRequest, cn));
	//Py_END_ALLOW_THREADS;

	if (err!=KErrNone) {
		return SPyErr_SetFromSymbianOSErr(err);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* fnc_CPBBSession_Put(obj_CPBBSession* self, PyObject* args)
{
	return fnc_CPBBSession_Put2(self, args, EFalse);
}

PyObject* fnc_CPBBSession_PutRequest(obj_CPBBSession* self, PyObject* args)
{
	return fnc_CPBBSession_Put2(self, args, ETrue);
}

PyObject* fnc_CPBBSubSession_Put(obj_CPBBSubSession* self, PyObject* args)
{
	return fnc_CPBBSession_Put2(self->iSession, args, EFalse);
}

PyObject* fnc_CPBBSubSession_PutRequest(obj_CPBBSubSession* self, PyObject* args)
{
	return fnc_CPBBSession_Put2(self->iSession, args, ETrue);
}

PyObject* fnc_CPBBSubSession_Get(obj_CPBBSubSession* self, PyObject* args)
{
	return fnc_CPBBSession_Get(self->iSession, args);
}


PyObject* fnc_CPBBSubSession_AddNotificationL(obj_CPBBSubSession* self, 
	PyObject* args)
{
	int module_uid=-1, module_id=-1, l=0;
	PyObject* existing=0;
	if (!PyArg_ParseTuple(args, "ii|O", &module_uid, &module_id, &existing) &&
		!PyArg_ParseTuple(args, "(ii)|O", &module_uid, &module_id, &existing))
	{
		return 0;
	}
	PyErr_Clear();

	TTupleName tn={ module_uid, module_id };
	if (existing) {
		TBool ex=EFalse;
		if (existing==Py_True) ex=ETrue;
		else if (existing==Py_False) ex=EFalse;
		else if (PyNumber_Check(existing)) {
			python_ptr<PyObject> num(PyNumber_Long(existing));
			ex=PyLong_AsLong(num.get());
		}
		else return 0;

		self->iSubSession->AddNotificationL(tn, ex);
	} else {
		self->iSubSession->AddNotificationL(tn);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* fnc_CPBBSubSession_AddNotification(obj_CPBBSubSession* self, 
	PyObject* args)
{
	PyObject* ret=0;
	CC_TRAPD(err, ret=fnc_CPBBSubSession_AddNotificationL(self, args));
	if (err!=KErrNone) {
		SPyErr_SetFromSymbianOSErr(err);
	}
	return ret;
}

PyObject* fnc_CPBBSession_GenUuid(obj_CPBBSession* self)
{
	PyObject* ret=0;
	CC_TRAPD(err, {
		TBuf8<16> uuid;
		self->iUuidGenerator->MakeUuidL(uuid);
		ret=PyString_FromStringAndSize( (const char*)uuid.Ptr(), 16);
	});
	if (err!=KErrNone) {
		SPyErr_SetFromSymbianOSErr(err);
	}
	return ret;
}

static const PyMethodDef CPBBSession_methods[] = {
	{ "get", (PyCFunction)fnc_CPBBSession_Get, METH_VARARGS, 
		"Get a single tuple. Call with (module_id, id[, subname]) or "
		"((module_id, id)[, subname])"},
	{ "put", (PyCFunction)fnc_CPBBSession_Put, METH_VARARGS, 
		"Put a tuple. Call with (module_id, id, subname, xml, expiration_unixtime) or "
		"((module_id, id), subname, xml, expiration_unixtime)" },
	{ "putrequest", (PyCFunction)fnc_CPBBSession_PutRequest, METH_VARARGS, 
		"Put a request. Call with (module_id, id, subname, xml, expiration_unixtime, component_uid, id) or "
		"((module_id, id), subname, xml, expiration_unixtimem, (component_uid, id))" },
	{ "subsession", (PyCFunction)fnc_CPBBSession_SubSession, METH_VARARGS,
		"Construct a subsession for listening to change notification. "
		"The single argument is a callback, which will be called "
		"with ((module_id, id), subname, data)." },
	{ "gen_uuid", (PyCFunction)fnc_CPBBSession_GenUuid, METH_NOARGS, 
		"Generate a uuid (returns 16 8-bit characters - raw binary) " },
	{ 0, 0 }
};

#define S_METHOD_TABLE const_cast<PyMethodDef*>(&CPBBSession_methods[0])
PyObject *getattr_CPBBSession(obj_CPBBSession *self, char *name)
{
	return Py_FindMethod(S_METHOD_TABLE, reinterpret_cast<PyObject*>(self), name);
}

static const PyMethodDef CPBBSubSession_methods[] = {
	{ "get", (PyCFunction)fnc_CPBBSubSession_Get, METH_VARARGS, 
		"Get a single tuple. Call with (module_id, id[, subname]) or "
		"((module_id, id)[, subname]). Returns xml." },
	{ "put", (PyCFunction)fnc_CPBBSubSession_Put, METH_VARARGS, 
		"Get a single tuple. Call with (module_id, id, subname, xml, expiration_unixtime) or "
		"((module_id, id), subname, xml, expiration_unixtime)" },
	{ "addnotification", (PyCFunction)fnc_CPBBSubSession_AddNotification, METH_VARARGS,
		"Request notification when data is added or updated, optionally "
		"getting the existing value as the first notification. "
		"Call with (module_id, id[, existing]) or ( (module_id, id)[, existing] )" },
	{ 0, 0 }
};

#define SS_METHOD_TABLE const_cast<PyMethodDef*>(&CPBBSubSession_methods[0])
PyObject *getattr_CPBBSubSession(obj_CPBBSubSession *self, char *name)
{
	return Py_FindMethod(SS_METHOD_TABLE, reinterpret_cast<PyObject*>(self), name);
}

const static PyTypeObject tmpl_CPBBSession = {
	PyObject_HEAD_INIT(NULL)
		0, /*ob_size*/
		"_contextphone.bbsession", /*tp_name*/
		sizeof(obj_CPBBSession), /*tp_basicsize*/
		0, /*tp_itemsize*/
		/* methods */
		(destructor)del_CPBBSession, /*tp_dealloc*/
		0, /*tp_print*/
		(getattrfunc)getattr_CPBBSession, /*tp_getattr*/
};

const static PyTypeObject tmpl_CPBBSubSession = {
	PyObject_HEAD_INIT(NULL)
		0, /*ob_size*/
		"_contextphone.bbsubsession", /*tp_name*/
		sizeof(obj_CPBBSubSession), /*tp_basicsize*/
		0, /*tp_itemsize*/
		/* methods */
		(destructor)del_CPBBSubSession, /*tp_dealloc*/
		0, /*tp_print*/
		(getattrfunc)getattr_CPBBSubSession, /*tp_getattr*/
};

static TInt ConstructType(const PyTypeObject* aTypeTemplate,
						  char* aClassName)
{
	PyTypeObject* typeObj = PyObject_New(PyTypeObject, &PyType_Type);
	if (!typeObj)
		// error set already, but telling caller also
		return KErrNoMemory;

	*typeObj = *aTypeTemplate; // fill in from a template
	typeObj->ob_type = &PyType_Type; // fill in the missing bit

	// PyDict_SetItemString returns 0 on success, and -1 on failure,
	// which is similar to enough to the Symbian error scheme that
	// we don't need to know which is being used
	TInt error = SPyAddGlobalString(aClassName, 
		reinterpret_cast<PyObject*>(typeObj));
	if (error < 0)
	{
		PyObject_Del(typeObj);
		SPyErr_SetFromSymbianOSErr(error);
		return error;
	}

	return KErrNone;
}


TInt def_CPBBSession()
{
	TInt err=ConstructType(&tmpl_CPBBSession, "_contextphone.bbsession");
	if (err!=KErrNone) return err;
	return ConstructType(&tmpl_CPBBSubSession, "_contextphone.bbsubsession");
}
