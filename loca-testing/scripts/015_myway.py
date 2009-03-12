def myway(general, dev, msg) :

	code = 15
	prio = 800
	prio += adjust_prio(general, dev, msg)

	if ( disallowed(general, dev) or msg['successcount'] < 2 or hour(general['time'])>=17 ) :
		return (0, 0, 0, '', '')

	title = "Going my way?"

	body = "\
I have seen every exhibit here at the museum three times now. Would you like to wonder down to the South Hall exhibition with me? A few of us are meeting outside here, at the cafe by the palm tree circle, at 5pm. It would be great if you could join us!"

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
