from bbdata.base import *
from bbdata.uid import *

USERGIVEN_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 33, 1, 0 ]


class UserGiven(BBCompound):
	def __init__(self, name):
		super(BBCompound, self).__init__(name)
		self.description = LongString('description')
		self.since = Time('since')
	#@classmethod
	def type(self):
		return USERGIVEN_TYPE
	type=classmethod(type)
UserGiven.add_to_factory()
