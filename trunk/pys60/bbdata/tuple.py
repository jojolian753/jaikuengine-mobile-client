from bbdata.base import *
from bbdata.uid import *
from bbdata.uuid import *
from bbdata.tuplemeta import *

TUPLE_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 7, 1, 0 ]


class Tuple(BBCompound):
	def __init__(self, name='tuple'):
		super(BBCompound, self).__init__(name)
		self.tuple_id = Uint('id')
		self.tuple_meta = TupleMeta()
		self.expires = Time('expires')
		self.data = ANY('tuplevalue')
		self.tuple_uuid = UUID('uuid')
	#@classmethod
	def type(self):
		return TUPLE_TYPE
	type=classmethod(type)
Tuple.add_to_factory()
