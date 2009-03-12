execfile("_general.py")
execfile("024_hangout.py")

def test_hangout() :

	dev=dict()
	dev['Loca@South hall']=dict()
	dev['Loca@South hall']['count'] = 7
	dev['Loca@South hall']['last_seen'] = local_to_unixtime(7, 9, 27)
	dev['Loca@South hall']['visitbegin'] = local_to_unixtime(7, 9, 27)
	dev['Loca@South hall']['first_seen'] = local_to_unixtime(7, 9, 27)

	general=dict()
	general['nodename']='Loca@South hall'
	general['time']=local_to_unixtime(8, 8, 47)
	general['bt_count']=3
	msg=dict()
	msg['successcount']=0

	res = hangout(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_hangout()
