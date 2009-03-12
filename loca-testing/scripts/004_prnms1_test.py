execfile("_general.py")
execfile("004_prnms1.py")

def test_prnms1() :

	dev=dict()
	dev['PRNMS']=dict()
	dev['PRNMS']['total'] = 77
	dev['PRNMS']['prev_visitbegin'] = local_to_unixtime(7, 14, 30)
	dev['PRNMS']['prev_visitend'] = local_to_unixtime(7, 15, 30)
	dev['PRNMS']['count'] = 2

	general=dict()
	general['nodename']='Saint Clare'
	general['time']=local_to_unixtime(8, 8, 47)
	msg=dict()

	res = prnms1(general, dev, msg)

	print res
	print "\n----\n\n"
	print res[4]

test_prnms1()
