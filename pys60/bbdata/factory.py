import bbdata.xml as bbxml
from bbdata.uid import *

def int_from_bb(data):
	s=str(data)
	if (s.startswith("0x") or s.startswith("0X")):
		return int(s, 16)
	return int(data)
	
class Factory(object):
	_classes = {}

	#@classmethod
	def add_class(self, type, classobj):
		if (not self._classes.has_key(type[0]) ):
			self._classes[type[0]]={}
		self._classes[type[0]][type[1]]=classobj
	add_class=classmethod(add_class)

	#@classmethod
	def create(self, type, name):
		uid = type[0]
		if uid==OLD2_CONTEXT_UID_BLACKBOARDFACTORY: uid=CONTEXT_UID_BLACKBOARDFACTORY
		if uid==OLD2_CONTEXT_UID_SENSORDATAFACTORY: uid=CONTEXT_UID_SENSORDATAFACTORY
		return self._classes[uid][type[1]](name)
	create=classmethod(create)

	#@classmethod
	def create_from_xml(self, elem):
		module=int_from_bb(elem.attrib['module'])
		id=int_from_bb(elem.attrib['id'])
		name=bbxml.remove_ns(elem.tag)
		obj=self.create( [module, id], name )
		obj.from_xml(elem)
		return obj
	create_from_xml=classmethod(create_from_xml)
