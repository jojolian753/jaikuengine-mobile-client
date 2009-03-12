execfile("_general.py")
execfile("001_ryoji.py")

def test_seen_at_ryoji() :

	dev=dict()
	dev['Ryoji']=dict()
	dev['Ryoji']['total'] = 10
	dev['Ryoji']['last_seen'] = local_to_unixtime(7, 13, 27)
	dev['Ryoji']['visitbegin'] = local_to_unixtime(7, 10, 11)
	dev['Ryoji']['first_seen'] = local_to_unixtime(7, 10, 11)

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()

	res = ryoji(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

def test_not_seen_at_ryoji() :

	dev=dict()
	dev['Ryoji']=dict()
	dev['Ryoji']['total'] = 1
	dev['Ryoji']['last_seen'] = local_to_unixtime(7, 13, 27)
	dev['Ryoji']['vistbegin'] = local_to_unixtime(7, 10, 11)
	dev['Ryoji']['first_seen'] = local_to_unixtime(7, 10, 11)

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()

	res = ryoji(general, dev, msg)

	print res


test_seen_at_ryoji()
test_not_seen_at_ryoji()

