def palm_pilot(general, dev, msg) :

	code = 13
	prio = 800
	prio += adjust_prio(general, dev, msg)

	keys=list();
	keys.append('Bottom of Cesar Chavez (LHS)')
	keys.append('South Hall entrance 1')
	if ( disallowed(general, dev) or not contains(dev, keys) ) : return (0, 0, 0, '', '')

	if ( dev['Bottom of Cesar Chavez (LHS)']['last_seen'] > dev['South Hall entrance 1']['last_seen'] and general['time']-dev['Bottom of Cesar Chavez (LHS)']['last_seen'] < 45*60 and general['time']-dev['South Hall entrance 1']['last_seen'] < 90*60) : code=13
	else : return (0, 0, 0, '', '')

	title = "Palm pilot"

	body = "\
Do you know the history of this palm circle? Apparently the first people to settle San Jose, long before the Spanish arrived, before even the first dot com boom, found a circle of huge palms growing alone in the desert. There was nothing else for miles around, no people, no plants, no water. As they approached the tops of the trees could be seen through the haze swaying gently above the horizon. Then their massive trunks. Then the roots surrounded by the broken husks of fallen coconuts. It seemed to take a lifetime to finally reach the base of the trees through the unforgiving heat. When they did they found the shells of the broken nuts and the roots of the trees had combined in such a way that they were perfectly aligned with the path they had taken, and also with the route they were planning to take. Legend has it that over the millennia travellers more numerous than I could mention have found their path mapped out at the foot of those great trees. Many found their way when before they were lost, often saving them from certain death in the desert.\n\
\n\
I dont know if this is why people chose to settle in San Jose, but they say it is still possible to find your way by the palm tree circle. I had a little look just a few minutes ago, from what I could tell some traveller has arrived from the South, passing what looks like the South Hall exhibition center at %s and reaching the bottom of Plaza de Cesar Chavez at %s." % \
( approx_time(dev['South Hall entrance 1']['last_seen']), approx_time(dev['Bottom of Cesar Chavez (LHS)']['last_seen']) )

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
