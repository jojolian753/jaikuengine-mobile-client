from bbdata.base import *
from bbdata.uid import *

SENSOREVENT_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 10, 1, 0 ]


class SensorEvent(BBCompound):
	def __init__(self, name='event'):
		super(BBCompound, self).__init__(name)
		self.stamp = Time('datetime')
		self.priority = Int('priority')
		self.name = ShortString('eventname')
		self.data = ANY('eventdata')
	#@classmethod
	def type(self):
		return SENSOREVENT_TYPE
	type=classmethod(type)
SensorEvent.add_to_factory()
