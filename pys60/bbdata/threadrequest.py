from bbdata.base import *
from bbdata.uid import *

THREADREQUEST_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 59, 1, 0 ]

THREADREQUEST_TUPLE = [ CONTEXT_UID_CONTEXTSENSORS, 59 ]


class ThreadRequest(BBCompound):
	def __init__(self, name='threadrequest'):
		super(BBCompound, self).__init__(name)
		self.thread_owner = ShortString('threadowner')
		self.post_uuid = UUID('postuuid')
		self.stream_data_id = Int('streamdataid')
	#@classmethod
	def type(self):
		return THREADREQUEST_TYPE
	type=classmethod(type)
ThreadRequest.add_to_factory()
