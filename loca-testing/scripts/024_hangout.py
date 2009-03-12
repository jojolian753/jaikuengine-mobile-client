def hangout(general, dev, msg) :

	code = 24
	prio = 100
	prio += adjust_prio(general, dev, msg)

	if ( disallowed(general, dev) ) : return (0, 0, 0, '', '')
	if ( msg['successcount'] > 0 ) : return (0, 0, 0, '', '')
	if ( msg['failurecount'] < 2 ) : prio+=100
	if ( general['mac'].lower() == '000E6daf9124'.lower() and msg['successcount'] < 3) : prio=5

	title = "Hey!"

	body = "\
Hiya. Right now %s Bluetooth devices know you are here. Including mine! \n\
\n\
I thought you were ignoring me. I have been waiting for you to notice me for %s, been trying to make contact ever since I first saw you here %s.\n\
\n\
Wanna hang out?" % (\
general['bt_count'], approx_time_period(dev[general['nodename']]['last_seen']-dev[general['nodename']]['visitbegin']), approx_time_date(dev[general['nodename']]['first_seen']) )

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
