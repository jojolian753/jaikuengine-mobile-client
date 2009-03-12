execfile("_general.py")
execfile("024_hangout.py")

def test_hangout() :

	dev=dict()
	dev['Hangout']=dict()
	dev['Hangout']['count'] = 7
	dev['Hangout']['last_seen'] = local_to_unixtime(7, 9, 27)
	dev['Hangout']['visitbegin'] = local_to_unixtime(7, 9, 27)
	dev['Hangout']['first_seen'] = local_to_unixtime(7, 9, 27)

	general=dict()
	general['nodename']='Hangout'
	general['time']=local_to_unixtime(8, 8, 47)
	general['bt_count']=3
	general['mac']='000e6daf9124'
	msg=dict()
	msg['successcount']=0
	msg['failurecount']=0

	res = hangout(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_hangout()
