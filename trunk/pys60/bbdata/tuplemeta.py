from bbdata.base import *
from bbdata.uid import *

TUPLEMETA_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 9, 1, 0 ]


class TupleMeta(BBCompound):
	def __init__(self, name='tuplename'):
		super(BBCompound, self).__init__(name)
		self.module_uid = Uid('module_uid')
		self.module_id = Int('module_id')
		self.sub_name = TupleSubName('subname')
	#@classmethod
	def type(self):
		return TUPLEMETA_TYPE
	type=classmethod(type)
TupleMeta.add_to_factory()
