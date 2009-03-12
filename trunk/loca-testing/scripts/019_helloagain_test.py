execfile("_general.py")
execfile("019_helloagain.py")

def test_helloagain() :

	dev=dict()
	dev['SawYou']=dict()
	dev['SawYou']['count'] = 7
	dev['SawYou']['last_seen'] = local_to_unixtime(7, 9, 27)

	general=dict()
	general['nodename']='Museum'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()
	msg['successcount']=1

	res = helloagain(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_helloagain()
