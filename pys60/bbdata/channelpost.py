from bbdata.base import *
from bbdata.uid import *

CHANNELPOST_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 58, 1, 0 ]


class ChannelPost(BBCompound):
	def __init__(self, name='channelpost'):
		super(BBCompound, self).__init__(name)
		self.uuid = UUID('uuid')
		self.channel = ShortString('channel')
		self.nick = ShortString('nick')
		self.display_name = LongString('displayname')
		self.content = String('content')
		self.created = Time('created')
	#@classmethod
	def type(self):
		return CHANNELPOST_TYPE
	type=classmethod(type)
ChannelPost.add_to_factory()
