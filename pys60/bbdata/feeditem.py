from bbdata.base import *
from bbdata.uid import *

FEEDITEM_TYPE = [ CONTEXT_UID_SENSORDATAFACTORY, 54, 1, 0 ]

FEEDITEM_TUPLE = [ CONTEXT_UID_CONTEXTSENSORS, 54 ]


class FeedItem(BBCompound):
	def __init__(self, name='feeditem'):
		super(BBCompound, self).__init__(name)
		self.parent_uuid = UUID('parentuuid')
		self.uuid = UUID('uuid')
		self.author_nick = ShortString('authornick')
		self.author_display_name = LongString('authordisplayname')
		self.thumbnail_mbm = String8('thumbnailmbm')
		self.thumbnail_url = String('thumbnailurl')
		self.icon_id = Int('iconid')
		self.created = Time('created')
		self.linked_url = String('linkedurl')
		self.content = String('content')
		self.location = LongString('location')
		self.from_server = Bool('fromserver')
		self.kind = ShortString('kind')
		self.is_unread = Bool('isunread')
		self.is_group_child = Bool('isgroupchild')
		self.correlation = UUID('correlation')
		self.stream_title = ShortString('streamtitle')
		self.stream_url = String('streamurl')
		self.channel = ShortString('channel')
		self.parent_author_nick = ShortString('parentauthornick')
		self.parent_title = LongString('parenttitle')
		self.stream_data_id = Int('streamdataid')
		self.media_file_name = LongString('mediafilename')
		self.media_download_state = Int('mediadownloadstate')
	#@classmethod
	def type(self):
		return FEEDITEM_TYPE
	type=classmethod(type)
FeedItem.add_to_factory()
