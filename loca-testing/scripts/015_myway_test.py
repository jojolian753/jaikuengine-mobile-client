execfile("_general.py")
execfile("015_myway.py")

def test_myway() :

	dev=dict()

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()
	msg['successcount']=3

	res = myway(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_myway()
