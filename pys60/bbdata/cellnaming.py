from bbdata.base import *
from bbdata.uid import *

CELLNAMING_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 48, 1, 0 ]


class CellNaming(BBCompound):
	def __init__(self, name='cellname'):
		super(BBCompound, self).__init__(name)
		self.mapped_id = Int('mappedid')
		self.name = LongString('name')
	#@classmethod
	def type(self):
		return CELLNAMING_TYPE
	type=classmethod(type)
CellNaming.add_to_factory()
