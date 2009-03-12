def coffee(general, dev, msg) :

	code = 14
	prio = 800
	prio += adjust_prio(general, dev, msg)

	if ( disallowed(general, dev) or msg['successcount'] < 2 or hour(general['time'])>=17 ) :
		return (0, 0, 0, '', '')

	title = "Coffee later?"

	body = "\
I keep bumping into you all over San Jose. Do you fancy meeting for a coffee later on, it would be good to see what you think of the festival. I will be having a coffee by the palm tree circle, behind the San Jose Museum of Art, at 5pm today. Send me a message once you get there, no problem if you cant make it!"

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
