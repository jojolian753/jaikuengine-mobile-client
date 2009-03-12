try:
    import lxml.etree as etree
except ImportError:
    import elementtree.ElementTree as etree

#import elementtree.ElementTree as etree
from StringIO import StringIO
import time
from bbdata.factory import *
from bbdata.uid import *
import bbdata.xml as bbxml

TYPE_ATTRIBUTES = [ "module", "id", "major_version", "minor_version" ]


class BBData(object):
	# type(self) -> [ module_uid, module_id, major, minor ]

	def __init__(self, name):
		self._name=str(name)

	def create_element(self, include_type_attributes=False, include_ns=False):
		elem = etree.Element(self._name)
		self.set_attributes(elem, include_type_attributes, include_ns)
		return elem

	def set_attributes(self, elem, include_type_attributes=False, include_ns=False):
		if (include_type_attributes) :
			type = self.type()
			for i in range(4):
				elem.attrib[ TYPE_ATTRIBUTES[i] ] = str(type[i])
		if (include_ns):
			elem.attrib['xmlns'] = bbxml.XMLNS

	def from_string_xml(self, string):
		et = etree.parse(StringIO(string))
		return self.from_xml(et.getroot())

	#@classmethod
	def add_to_factory(self):
		Factory.add_class( self.type(), self )
	add_to_factory=classmethod(add_to_factory)

	def name(self):
		return self._name

def _ident(dummy, x):
	return x

class BBSimple(BBData):
	# conversion functions for Python<->_value, including type and range checking
	_convert_in = None
	_convert_out = _ident
	def __init__(self, name, value):
		super(BBSimple, self).__init__(name)
		self._value = self._convert_in(value)

	def to_xml(self, include_type_attributes=False, include_ns=False):
		elem = self.create_element(include_type_attributes, include_ns)
		elem.text = self.to_text()
		return elem

	def from_xml(self, element):
		self.from_text(element.text)

	# conversion functions for serialized<->_value
	def from_text(self, value):
		self._value = self._convert_in(value)
	def to_text(self):
		return unicode(self._convert_out(self._value))

	# _value accessors
	def set(self, value):
		if isinstance(value, BBSimple):
			self._value=self._convert_in(value.get())
		else:
			self._value=self._convert_in(value)
	def get(self):
		return self._value

	def __str__(self):
		return str(self._value)
	def __cmp__(self, other):
		if self._value < other._value: return -1
		if self._value > other._value: return 1
		return 0

INT_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 1, 1, 0 ]
UINT_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 6, 1, 0 ]
UID_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 9, 1, 0 ]
SHORTSTRING_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 2, 1, 0 ]
LONGSTRING_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 10, 1, 0 ]
STRING_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 11, 1, 0 ]
TIME_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 5, 1, 0 ]
BOOL_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 4, 1, 0 ]
SHORTSTRING8_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 13, 1, 0 ]
STRING8_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 16, 1, 0 ]
UUID_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 22, 1, 0 ]
MD5_TYPE = [ CONTEXT_UID_BLACKBOARDFACTORY, 21, 1, 0 ]
NULL_TYPE = [ 0, 0, 0, 0 ]
SUBNAME_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 8, 1, 0 ]

def _int_from_bb(dummy, x):
	return int_from_bb(x)

class Int(BBSimple):
	_convert_in = _int_from_bb
	def __init__(self, name, value=0):
		super(Int, self).__init__(name, value)

	#@classmethod
	def type(self):
		return INT_TYPE
	type=classmethod(type)

Int.add_to_factory()

class Uint(Int):
	#@classmethod
	def type(self):
		return UINT_TYPE
	type=classmethod(type)
Uint.add_to_factory()

class Uid(Uint):
	#@classmethod
	def type(self):
		return UID_TYPE
	type=classmethod(type)

	def to_text(self):
		return unicode(hex(self._value))
Uid.add_to_factory()

import re

class Time(Int):
	"UTC unixtime - to be compatible with S60 python, which is 2.2"
	def __init__(self, name, value=0):
		super(Time, self).__init__(name, value)

	#@classmethod
	def type(self):
		return TIME_TYPE
	type=classmethod(type)

	def to_text(self):
		if self._value==0:
			return u"00000101T000000"
		return unicode(time.strftime("%Y%m%dT%H%M%S", time.gmtime(self._value)))

	_p = re.compile('(\d\d\d\d)(\d\d)(\d\d)T(\d\d)(\d\d)(\d\d)');
	def from_text(self, text):
		if str(text)=="00000101T000000":
			self._value=0
			return
		m = self._p.match(text)
		if (m):
			dt = time.mktime([ int(m.group(i+1)) for i in range(6) ] + [ 0, 0, 0 ])
			dt += time.mktime(time.localtime(100000)) - time.mktime(time.gmtime(100000))
			self._value = int( dt )
		else:
			self._value = int(text)
Time.add_to_factory()

class ANY(BBData):
	def __init__(self, name):
		self._name=str(name)
		self._value=None

	def type(self):
		if self._value:
			return self._value.type()
		return NULL_TYPE

	def to_xml(self, include_type_attributes=False, include_ns=False):
		if self._value:
			return self._value.to_xml(True, include_ns)
		return None

	def from_xml(self, element):
		obj=Factory.create_from_xml(element)
		self.set(obj)

	def to_xml(self):
		if self._value:
			return self._value.to_xml()
		return None

	def set(self, value):
		if isinstance(value, BBData):
			self._value=value
			value._name=self._name
		else:
			raise TypeError, "Can only assign BBData to ANY, got %s"%(value.__class__)
	
	def get(self):
		if (self._value and isinstance(self._value, BBSimple)):
			return self._value.get()
		else:
			return self._value

	def __str__(self):
		return str(self._value)

	def __cmp__(self, other):
		if self._value < other._value: return -1
		if self._value > other._value: return 1
		return 0

	def to_text(self):
		if (self._value): return self._value.to_text()
		return str(self._value)


# ANY cannot appear in serialized form and thus is not added to the factory

class Bool(Int):
	_convert_out = bool
	def __init__(self, name, value=0):
		super(Bool, self).__init__(name, value)

	#@classmethod
	def type(self):
		return BOOL_TYPE
	type=classmethod(type)

	def to_text(self):
		if (self._value): return u"true"
		else: return u"false"

	def from_text(self, text):
		if ( str(text).strip().lower()=="true") : self._value=1
		elif ( str(text).strip().lower()=="false") : self._value=0
		else: self._value=bool(int_from_bb(text))
Bool.add_to_factory()

class StringBase(BBSimple):
	def check_string_length(self, value):
		maxlen=self.maxlength()
		if (value==None): v=u''
		else: 
			if isinstance(value, str):
				v=unicode(value, "utf-8")
			else:
				v=unicode(value)
		if (maxlen==0):
			return v
		if (len(v)>maxlen):
			raise ValueError, "%s for field %s is longer than %d characters"%(v, self._name, maxlen)
		return v
	def __init__(self, name, value=u''):
		super(StringBase, self).__init__(name, value)
	_convert_in = check_string_length

SHORT_STRING_LENGTH = 50
LONG_STRING_LENGTH = 255

class ShortString(StringBase):
	#@classmethod
	def maxlength(self):
		return SHORT_STRING_LENGTH
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return SHORTSTRING_TYPE
	type=classmethod(type)
ShortString.add_to_factory()

class LongString(StringBase):
	#@classmethod
	def maxlength(self):
		return LONG_STRING_LENGTH
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return LONGSTRING_TYPE
	type=classmethod(type)
LongString.add_to_factory()

class TupleSubName(StringBase):
	#@classmethod
	def maxlength(self):
		return 128
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return SUBNAME_TYPE
	type=classmethod(type)
TupleSubName.add_to_factory()

class String(StringBase):
	#@classmethod
	def maxlength(self):
		return 0
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return STRING_TYPE
	type=classmethod(type)
String.add_to_factory()

class StringBase8(BBSimple):
	def check_string_length(self, value):
		maxlen=self.maxlength()
		if (value==None): v=''
		else: v=str(value)
		if (maxlen==0):
			return v
		if (len(v)>maxlen):
			raise ValueError, "%s for field %s is longer than %d characters"%(v, self._name, maxlen)
		return v
	def __init__(self, name, value=''):
		super(StringBase8, self).__init__(name, value)
	_convert_in = check_string_length

	def to_text(self):
		return ''.join(["%02x"%ord(x) for x in self._value])

	def from_text(self, value):
		if value==None:
			self._value=''
			return
		str8=''.join([ chr( int( value[i*2:i*2+2], 16 ) ) for i in range(len(value)/2) ])
		self.set(str8)

class ShortString8(StringBase8):
	#@classmethod
	def maxlength(self):
		return SHORT_STRING_LENGTH
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return SHORTSTRING8_TYPE
	type=classmethod(type)
ShortString8.add_to_factory()

class String8(StringBase8):
	#@classmethod
	def maxlength(self):
		return 0
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return STRING8_TYPE
	type=classmethod(type)
String8.add_to_factory()

class UUID(StringBase8):
	#@classmethod
	def maxlength(self):
		return 16
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return UUID_TYPE
	type=classmethod(type)
UUID.add_to_factory()

class MD5(StringBase8):
	#@classmethod
	def maxlength(self):
		return 16
	maxlength=classmethod(maxlength)
	#@classmethod
	def type(self):
		return MD5_TYPE
	type=classmethod(type)
MD5.add_to_factory()

class BBCompound(BBData):
	_by_name=None
	_indent=''
	def __init__(self, name):
		super(BBCompound, self).__init__(name)

	def to_xml(self, include_type_attributes=False, include_ns=False):
		elem = self.create_element(include_type_attributes, include_ns)
		for obj in self.__dict__.values():
			if isinstance(obj, BBData):
				elem.append(obj.to_xml())
		return elem

	def set(self, value):
		if self.__class__ != value.__class__: raise TypeError, "cannot assign to BBCompound from a different type (expected %s, got %s)"%(self.__class__, value.__class__)
		self._init_by_name()
		for f in self._by_name.values():
			self.__dict__[f].set(value.__dict__[f])
		
	def __getattribute__(self, name):
		if (name!='__dict__' and self.__dict__.has_key(name)):
			obj=self.__dict__[name]
			if (isinstance(obj, BBSimple) or isinstance(obj, ANY)):
				return obj.get()
		return super(BBCompound, self).__getattribute__(name)

	def __setattr__(self, name, value):
		if self.__dict__.has_key(name):
			obj=self.__dict__[name]
			if isinstance(obj, BBData):
				return obj.set(value)
		return object.__setattr__(self, name, value)

	def _init_by_name(self):
		if not self._by_name:
			self._by_name={}
			for key in self.__dict__.keys():
				obj=self.__dict__[key]
				if isinstance(obj, BBData):
					self._by_name[obj._name]=key

	def from_xml(self, elem):
		self._init_by_name()
		for c in elem:
			tag=bbxml.remove_ns(c.tag)
			if self._by_name.has_key(tag):
					obj=self.__dict__[self._by_name[tag]]
					obj.from_xml(c)

	def __cmp__(self, other):
		for key in self.__dict__.keys():
			obj = self.__dict__[key]
			if isinstance(obj, BBData):
				obj2=other.__dict__[key]
				cmp = obj2.__cmp__(obj)
				if (cmp<0 or cmp>0): return cmp
		return 0

	def __str__(self):
		ret="\n"
		BBCompound._indent += '  '
		for key in self.__dict__.keys():
			obj = self.__dict__[key]
			if (isinstance(obj, ANY) and obj._value): obj=obj._value
			if isinstance(obj, BBData):
				ret += BBCompound._indent + key + ' ' + str(obj.__class__) + ' : ' + obj.to_text() + "\n"
		BBCompound._indent=BBCompound._indent[0:len(self._indent)-2]
		return ret

	def to_text(self):
		return str(self)
