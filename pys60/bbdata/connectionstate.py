from bbdata.base import *
from bbdata.uid import *

CONNECTIONSTATE_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 64, 1, 0 ]


class ConnectionState(BBCompound):
	def __init__(self, name='connectionstate'):
		super(BBCompound, self).__init__(name)
		self.name = ShortString('name')
		self.state = Int('state')
		self.message = LongString('message')
		self.error = ErrorInfo('error')
		self.retry = Time('retry')
	#@classmethod
	def type(self):
		return CONNECTIONSTATE_TYPE
	type=classmethod(type)
ConnectionState.add_to_factory()
