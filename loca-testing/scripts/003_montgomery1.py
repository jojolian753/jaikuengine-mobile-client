def montgomery1(general, dev, msg) :

	code = 3
	prio = 500
	prio += adjust_prio(general, dev, msg)

	keys=list();
	keys.append('Montgomery')
	if ( disallowed(general, dev) or not contains(dev, keys) ) : return (0, 0, 0, '', '')

	if ( dev['Montgomery']['count'] < 3) :
		return (0, 0, 0, '', '')

	title = "RU @ Montgomery?"

	body = "\
Are you staying at Montgomery? I noticed your device had been detected there when I visited the Loca stand in the South Hall. You were detected by their network at the hotel reception %d times, so I guess that means you are checked in! :) I must have just missed you, I was by the hotel reception myself at %s which is when you were last seen there." % ( \
dev['Montgomery']['count'], approx_time_date(dev['Montgomery']['last_seen']) \
)

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
