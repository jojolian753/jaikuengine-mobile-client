execfile("_general.py")
execfile("013_palm_pilot.py")

def test_palm_pilot() :

	dev=dict()
	dev['Bottom of Cesar Chavez (LHS)']=dict()
	dev['Bottom of Cesar Chavez (LHS)']['last_seen'] = local_to_unixtime(8, 8, 37)
	dev['South Hall entrance 1']=dict()
	dev['South Hall entrance 1']['last_seen'] = local_to_unixtime(8, 8, 17)

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()

	res = palm_pilot(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_palm_pilot()
