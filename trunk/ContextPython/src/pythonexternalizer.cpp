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

#include "pythonexternalizer.h"
#include "errorhandling.h"
#include "symbian_python_ptr.h"

#include <Python.h>
#include <e32base.h>
#include "concretedata.h"

void LeaveIfPythonError(TInt aResult) {
	if (aResult!=0) {
		EnvErr(gettext("Python interpreter returned an error")).
			ErrorCode(MakeErrorCode(CONTEXT_UID_CONTEXTPYTHON, KErrFromPython)).
			Raise();
	}
}
PyObject* LeaveIfPythonNull(PyObject* aObject) {
	if (aObject==0) {
		EnvErr(gettext("Python interpreter couldn't create object")).
			ErrorCode(MakeErrorCode(CONTEXT_UID_CONTEXTPYTHON, KErrFromPython)).
			Raise();
	}
	return aObject;
}

class CPythonExternalizerImpl : public CPythonExternalizer {
public:
	virtual PyObject* GetObject() {
		Py_XINCREF(iTop);
		return iTop;
	}

	struct TItem {
		PyObject*	o;
		TBool		includetype;
		TItem(PyObject* ao, TBool aincludetype) : o(ao), includetype(aincludetype) { }
		TItem(PyObject* ao) : o(ao), includetype(EFalse) { }
		TItem() : o(0), includetype(EFalse) { }
		bool operator != (const TItem& i) const {
			return (o != i.o || includetype != i.includetype );
		}
	};
	CArrayFixSeg<TItem>	*iStack;
	TItem iCurrent;
	PyObject* iTop;
	void ConstructL() {
		iStack=new (ELeave) CArrayFixSeg<TItem>(4);
	}
	virtual void Reset() {
		TItem i;
		while( (i=Pop())!=TItem() ) {
			Py_XDECREF(i.o);
		}
		Py_XDECREF(iTop);
		iCurrent=TItem();
		iTop=0;
	}

	~CPythonExternalizerImpl() {
		Reset();
		delete iStack;
	}
	TItem PushL(TItem o) {
		iStack->AppendL(o);
		return o;
	}
	TItem Pop() {
		if (!iStack || iStack->Count()==0) return TItem();
		TItem o=iStack->At(iStack->Count()-1);
		iStack->Delete(iStack->Count()-1);
		return o;
	}
	void AddToCurrentL(const TDesC& aName, PyObject* aO,
			TBool aIncludeBBType=EFalse) {

		if (aIncludeBBType && !PyDict_Check(iCurrent.o)) {
			Bug(gettext("aIncludeBBType set but no dictionary created")).
				Raise();
		}
		if (iCurrent.o==0) {
			// first
			if (iTop!=0) {
				Bug(gettext("Attempting to serialize to primitives on their own")).
					Raise();
			}
			iTop=aO;
			Py_XINCREF(aO);
		} else if (PyDict_Check(iCurrent.o)) {
			python_ptr<PyObject> name(
				LeaveIfPythonNull( Py_BuildValue("u#", 
					aIncludeBBType ? (TUint16*)L"!value" : (TUint16*)aName.Ptr(), 
					aIncludeBBType ? 6 : aName.Length() )
				));
			LeaveIfPythonError(PyDict_SetItem(iCurrent.o, name.get(), aO));
		} else if (PyList_Check(iCurrent.o)) {
			LeaveIfPythonError(PyList_Append(iCurrent.o, aO));
		} else {
			Bug(gettext("Attempting to add object to something that is neither a list nor dictionary")
				).Raise();
		}
	}
	void BeginType(const TDesC& aName,
			TBool aIncludeBBType, const TTypeName& aBBType) {
		if (!aIncludeBBType) return;
		python_ptr<PyObject> dict( LeaveIfPythonNull(PyDict_New()) );
		python_ptr<PyObject> t( LeaveIfPythonNull(Py_BuildValue("(iiii)",
			aBBType.iModule, aBBType.iId, aBBType.iMajorVersion, aBBType.iMinorVersion)));
		LeaveIfPythonError(
			PyDict_SetItemString(dict.get(), "!bbtype", t.get() ));
		AddToCurrentL(aName, dict.get());
		iCurrent=PushL(TItem(dict.get()));
		dict.release();
	}
	virtual void BeginList(const TDesC& aName,
		TBool aIncludeBBType, const TTypeName& aBBType) {

		BeginType(aName, aIncludeBBType, aBBType);
		python_ptr<PyObject> list( LeaveIfPythonNull(PyList_New(0)) );
		AddToCurrentL(aName, list.get(), aIncludeBBType);
		iCurrent=PushL( TItem(list.get(), aIncludeBBType) );
		list.release();
	}
	virtual void BeginCompound(const TDesC& aName,
			TBool aIncludeBBType, const TTypeName& aBBType) {

		BeginType(aName, aIncludeBBType, aBBType);
		python_ptr<PyObject> dict( LeaveIfPythonNull(PyDict_New()) );
		AddToCurrentL(aName, dict.get(), aIncludeBBType);
		iCurrent=PushL( TItem(dict.get(), aIncludeBBType) );
		dict.release();
	}

	virtual void Field(const TDesC& aName,
			TBasicType aBasicType, const TDesC& aValue,
			TBool aIncludeBBType, const TTypeName& aBBType, 
			TBool /*aAsAttribute=EFalse*/) {

		BeginType(aName, aIncludeBBType, aBBType);
		python_ptr<PyObject> val(0);
		_LIT(KVal, "val");
		switch(aBasicType) {
			case EInteger:
				{
					TBBInt i(KVal);
					i.FromStringL(aValue);
					val.reset( LeaveIfPythonNull(PyInt_FromLong(i())) );
				}
				break;
			case EFloat:
				{
					python_ptr<PyObject> str( LeaveIfPythonNull(
						Py_BuildValue("u#", aValue.Ptr(), aValue.Length())) );
					val.reset( LeaveIfPythonNull(PyFloat_FromString(str.get(), 0)) );
				}
				break;
			case EDateTime:
				/* no datetime support in Python 2.2 */
				{
					TBBTime utc(TTime(0), aName);
					utc.FromStringL(aValue);
					utc()-=TTimeIntervalHours(3);
					TBuf<16> s;
					utc.IntoStringL(s);
					val.reset( LeaveIfPythonNull(
						Py_BuildValue("u#", s.Ptr(), s.Length())) );
				}
			case EBinary:
				/* the receiver needs to know that the data is binary... */
			case EString:
				{
					val.reset( LeaveIfPythonNull(
						Py_BuildValue("u#", aValue.Ptr(), aValue.Length())) );
				}
				break;
			case EBoolean:
				{
					PyObject* o;
					TBBBool b(KVal);
					b.FromStringL(aValue);
					if (b()) {
						o=Py_True;
					} else {
						o=Py_False;
					}
					Py_INCREF(o);
					val.reset(o);
				}
				break;
		};
		AddToCurrentL(aName, val.get(), aIncludeBBType);
		if (aIncludeBBType) {
			/* remove the dictionary */
			TItem d=Pop();
			Py_XDECREF(d.o);
		}
	}

	void End() {
		TItem d=Pop();
		Py_XDECREF(d.o);
		if (d.includetype) {
			TItem p=Pop();
			Py_XDECREF(p.o);
		}
	}
	virtual void EndCompound(const TDesC& /*aName*/) {
		End();
	}
	virtual void EndList(const TDesC& /*aName*/) {			
		End();
	}
	friend class CPythonExternalizer;
};

CPythonExternalizer* CPythonExternalizer::NewL()
{
	auto_ptr<CPythonExternalizerImpl> e(new (ELeave) CPythonExternalizerImpl);
	e->ConstructL();
	return e.release();
}

#include "bbxml.h"

PyObject* MakePythonXmlL(const MBBData* aData)
{
	auto_ptr<CXmlBufExternalizer> e(CXmlBufExternalizer::NewL(128));
	aData->IntoXmlL(e.get());
	
	return Py_BuildValue("u#", e->Buf().Ptr(), e->Buf().Length());
}
