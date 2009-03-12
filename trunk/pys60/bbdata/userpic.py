from bbdata.base import *
from bbdata.uid import *

USERPIC_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 50, 1, 0 ]

USERPIC_TUPLE = [ CONTEXT_UID_CONTEXTSENSORS, 50 ]


class UserPic(BBCompound):
	def __init__(self, name='userpicture'):
		super(BBCompound, self).__init__(name)
		self.nick = ShortString('nick')
		self.phone_number_hash = MD5Hash('phonenumberhash')
		self.mbm = String8('mbm')
		self.phone_number_is_verified = Bool('phonenumberisverified')
	#@classmethod
	def type(self):
		return USERPIC_TYPE
	type=classmethod(type)
UserPic.add_to_factory()
