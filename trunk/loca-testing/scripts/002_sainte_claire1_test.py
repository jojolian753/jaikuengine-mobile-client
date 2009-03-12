execfile("_general.py")
execfile("002_sainte_claire1.py")

def test_sainte_claire1() :

	dev=dict()
	dev['Sainte Claire']=dict()
	dev['Sainte Claire']['count'] = 7
	dev['Sainte Claire']['last_seen'] = local_to_unixtime(7, 9, 27)

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()

	res = sainte_claire1(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_sainte_claire1()
