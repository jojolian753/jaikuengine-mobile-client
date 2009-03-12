def ryoji(general, dev, msg) :

	code = 1
	prio = 600
	prio += adjust_prio(general, dev, msg)

	keys=list();
	keys.append('Ryoji')
	if ( disallowed(general, dev) or not contains(dev, keys) ) : return (0, 0, 0, '', '')
	if ( dev['Ryoji']['total'] < 5) :
		return (0, 0, 0, '', '')

	title = "U like Ryoji?"

	body = "\
Did you like the Ryoji show? I was told to look out for you \
by the Loca network, it said you were already in the venue when \
I arrived at %s. <<Apparently there were [125] devices in there, \
and out of all of those people they thought we would get along \
the best!>> Did my best but couldnt find you, did they have \
it right that you were in there for %s, or is their tracking \
info wrong?"\
% ( approx_time(dev['Ryoji']['visitbegin']), \
approx_time_period(dev['Ryoji']['last_seen']- dev['Ryoji']['first_seen']) )


	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
