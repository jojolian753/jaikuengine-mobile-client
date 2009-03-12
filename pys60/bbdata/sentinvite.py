from bbdata.base import *
from bbdata.uid import *

SENTINVITE_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 51, 1, 0 ]

SENTINVITE_TUPLE = [ CONTEXT_UID_CONTEXTSENSORS, 51 ]


class SentInvite(BBCompound):
	def __init__(self, name='invited'):
		super(BBCompound, self).__init__(name)
		self.stamp = Time('stamp')
		self.url = LongString('url')
		self.from_field = ShortString('from')
		self.to_number_hash = MD5Hash('tonumberhash')
	#@classmethod
	def type(self):
		return SENTINVITE_TYPE
	type=classmethod(type)
SentInvite.add_to_factory()
