def sainte_claire1(general, dev, msg) :

	code = 2
	prio = 500
	prio += adjust_prio(general, dev, msg)

	keys=list();
	keys.append('Sainte Claire')
	if ( not contains(dev, keys) ) : return (0, 0, 0, '', '')

	if ( dev['Sainte Claire']['count'] < 3) :
		return (0, 0, 0, '', '')

	title = "RU @ Sainte Claire?"

	body = "\
Are you staying at Sainte Claire? I noticed your device had been detected there when I visited the Loca stand in the South Hall. You were detected by their network at the hotel reception %d times, so I guess that means you are checked in! :) I must have just missed you, I was by the hotel reception myself at %s which is when you were last seen there." % ( \
dev['Sainte Claire']['count'], approx_time_date(dev['Sainte Claire']['last_seen']) \
)

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
