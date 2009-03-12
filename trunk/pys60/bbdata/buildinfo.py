from bbdata.base import *
from bbdata.uid import *

BUILDINFO_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 53, 1, 0 ]


class BuildInfo(BBCompound):
	def __init__(self, name):
		super(BBCompound, self).__init__(name)
		self.when = Time('when')
		self.build_by = ShortString('buildby')
		self.sdk = LongString('sdk')
		self.branch = LongString('branch')
		self.major_version = Int('majorversion')
		self.minor_version = Int('minorversion')
		self.internal_version = Int('internalversion')
	#@classmethod
	def type(self):
		return BUILDINFO_TYPE
	type=classmethod(type)
BuildInfo.add_to_factory()
