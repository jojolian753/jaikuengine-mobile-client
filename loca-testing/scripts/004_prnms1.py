def prnms1(general, dev, msg) :

	code = 4
	prio = 400
	prio += adjust_prio(general, dev, msg)

	keys=list();
	keys.append('PRNMS')
	if ( disallowed(general, dev) or not contains(dev, keys) ) : return (0, 0, 0, '', '')

	if ( dev['PRNMS']['total'] < 30) :
		return (0, 0, 0, '', '')

	title = "What is PRNMS?"

	if (dev['PRNMS']['count'] > 1) :
		seen_from=dev['PRNMS']['prev_visitbegin']
		seen_to=dev['PRNMS']['prev_visitend']
	else :
		seen_from=dev['PRNMS']['visitbegin']
		seen_to=dev['PRNMS']['last_seen']

	body = "\
You got to tell me what the Pacific Rim New Media Summit was all about. I was outside between %s, but I couldnt get in. They said it was invitation only, which just made me more curious! I saw you on the Loca network; I even thought about asking if you could get me an invite, but I didnt want to disturb you. \n\
\n\
You must be a real conference junky! Come and find me at the Loca stand if you have time after the ISEA Summit, I would love to hear how it went." % (\
	approx_between(dev['PRNMS']['prev_visitbegin'], dev['PRNMS']['prev_visitend'])
)

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
