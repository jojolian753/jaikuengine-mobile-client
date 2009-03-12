from bbdata.base import *
from bbdata.uid import *

SERVERMESSAGE_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 52, 1, 0 ]

SERVERMESSAGE_TUPLE = [ CONTEXT_UID_CONTEXTCONTACTS, 52 ]


class ServerMessage(BBCompound):
	def __init__(self, name='servermessage'):
		super(BBCompound, self).__init__(name)
		self.title = LongString('title')
		self.body = String('body')
		self.url = LongString('url')
	#@classmethod
	def type(self):
		return SERVERMESSAGE_TYPE
	type=classmethod(type)
ServerMessage.add_to_factory()
