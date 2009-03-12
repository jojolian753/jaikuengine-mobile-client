def _sly_sig(name) :
	sig = "BR, "
	sig += name
	sig += \nTo contact " + name + ", send an SMS to XXX or go to http://loca-lab.org/" + name
	return sig

def _loca_sig() :
	return "Loca at http://loca-lab.org/"

def seen_at_ryoji(general, dev, msg) :

	code = 1
	prio = 10

	if ( dev['ryoji']['total'] < 5) :
		return (0, 0, 0, '', '')

	title = "U like Ryoji?"

	body = "Did you like the Ryoji show?... %s" % ( format_t(dev['ryoji']['last_seen']) )

	body += _sig('Sly')
	body += _log(general, dev, msg)
	body += _loca_sig()

	return ( prio, code, title, title, body )


def test_seen_at_ryoji()

	dev['ryoji']['total'] = 10
	dev['ryoji']['last_seen'] = 
