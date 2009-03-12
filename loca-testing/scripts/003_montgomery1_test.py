execfile("_general.py")
execfile("003_montgomery1.py")

def test_montgomery1() :

	dev=dict()
	dev['Montgomery']=dict()
	dev['Montgomery']['count'] = 7
	dev['Montgomery']['last_seen'] = local_to_unixtime(7, 9, 27)

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()

	res = montgomery1(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_montgomery1()
