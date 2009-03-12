from bbdata.base import *
from bbdata.uid import *

STREAMCOMMENT_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 57, 1, 0 ]


class StreamComment(BBCompound):
	def __init__(self, name='stream_comment'):
		super(BBCompound, self).__init__(name)
		self.uuid = UUID('uuid')
		self.post_uuid = UUID('postuuid')
		self.author_nick = ShortString('authornick')
		self.author_display_name = LongString('authordisplayname')
		self.content = String('content')
		self.created = Time('created')
		self.channel_name = ShortString('channelname')
		self.post_author_nick = ShortString('postauthornick')
		self.post_title = LongString('posttitle')
		self.stream_data_id = Int('streamdataid')
	#@classmethod
	def type(self):
		return STREAMCOMMENT_TYPE
	type=classmethod(type)
StreamComment.add_to_factory()
