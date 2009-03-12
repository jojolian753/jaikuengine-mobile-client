execfile("_general.py")
execfile("014_coffee.py")

def test_coffee() :

	dev=dict()

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()
	msg['successcount']=3

	res = coffee(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_coffee()
