from bbdata.base import *
from bbdata.uid import *

STREAMDATA_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 56, 1, 0 ]


class StreamData(BBCompound):
	def __init__(self, name='stream_data'):
		super(BBCompound, self).__init__(name)
		self.uuid = UUID('uuid')
		self.correlation = UUID('correlation')
		self.author_nick = ShortString('authornick')
		self.author_display_name = LongString('authordisplayname')
		self.title = LongString('title')
		self.content = String('content')
		self.url = String('url')
		self.icon_id = Int('iconid')
		self.created = Time('created')
		self.kind = ShortString('kind')
		self.stream_title = ShortString('streamtitle')
		self.stream_url = String('streamurl')
		self.channel_name = ShortString('channelname')
		self.stream_data_id = Int('streamdataid')
	#@classmethod
	def type(self):
		return STREAMDATA_TYPE
	type=classmethod(type)
StreamData.add_to_factory()
