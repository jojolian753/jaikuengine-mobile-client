import time

def sig(name) :
	sig = "\n\nSly";
	return sig

def loca_sig() :
	return "\n\n\
ABOUT LOCA\n\
Loca is an interdisciplinary project that tracks the trail of digital identities that people leave through physical space. Loca deploys a network of Bluetooth nodes around the city that enable it to track anyone with a Bluetooth device set to discoverable and send them messages. The aim of Loca is to raise awareness of the networks we inhabit, and provoke people into questioning them. To best participate in the loca project you will need to set Bluetooth to discoverable on your cell phone.\n\
http://loca-lab.org\n\
\n\
THE LOCA SOCIAL NETWORK\n\
Loca is running a trial social network in San Jose. A limited number of people will be able to find someone like you during their time in the city.\n\
\n\
CONTACT LOCA\n\
To contact Loca or for any complaints contact Loca at\n\
info@loca-lab.org\n\
415-513-3868"

def devicetype(general) :
	if ( general['majorclass']==2 ) : return "phone"
	if ( general['majorclass']==1 and ( general['minorclass']==4 or general['minorclass']==5 ) ) : return "PDA"
	if ( general['majorclass']==1 and general['minorclass']==3 ) : return "laptop"
	return "Bluetooth device"

def dev_log(general, dev, msg) :
	sig = "\n\nThe Loca network has seen you through your %s:" % ( devicetype(general) )
	log = list()
	for node in dev.keys() :
		if (contains( dev[node], ( 'prev_visitbegin', 'prev_visitend', 'visitbegin', 'last_seen') ) ) :
			if (dev[node]['prev_visitbegin']>0) :
				log.append( ( dev[node]['prev_visitbegin'], dev[node]['prev_visitend'], node ) )
			log.append( ( dev[node]['visitbegin'], dev[node]['last_seen'], node ) )
	log.sort()
	for l in log :
		sig += "from %s to %s at %s\n" % ( approx_time(l[0]), approx_time(l[1]), l[2] )

	if (contains(general, ( 'bt_count', 'majorclass' ) ) ) :
		sig += "At the moment there are %d other potential Loca members around." % ( general['bt_count'] )
	return sig
	
	

def location_and_time(general) :
	sig = "\n\nLocation: %s\nTime %s" % ( general['nodename'], format_unixtime(general['time']) )
	sig += "\nTo contact Sly, send an SMS to 650-243-7830 or go to http://loca-lab.org/Sly"
	return sig

def to_localtime(unixtime) :
	return time.localtime(unixtime-3*60*60)

def day(unixtime) :
	return int(time.strftime("%j", to_localtime(unixtime)))

def hour(unixtime) :
	return int(time.strftime("%H", to_localtime(unixtime)))

def minute(unixtime) :
	return int(time.strftime("%M", to_localtime(unixtime)))

def format_time(hour, min) :
	if (hour==12) :
		return "%d.%02d pm" % (hour, min)
	if (hour>12) :
		hour-=12
		return "%d.%02d pm" % (hour, min)

	return "%d.%02d am" % (hour, min)

def format_unixtime(unixtime) :
	return format_time(hour(unixtime), minute(unixtime))

def approx_time(unixtime) :
	_hour = hour(unixtime)
	_min =  minute(unixtime)
	_min = int(round(float(_min)/15)*15)

	return format_time(_hour, _min)

def local_day() :
	return int(time.strftime("%j", time.localtime(time.time())))

def weekdayname(unixtime) :
	wdays=( "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat")
	wday=int(time.strftime("%w", to_localtime(unixtime)))
	return wdays[wday]
	
def approx_time_date(unixtime) :
	_time = approx_time(unixtime)
	if (day(unixtime) == local_day()) :
		return _time
	return weekdayname(unixtime) + " " + _time

def approx_between(unixtime1, unixtime2) :
	if ( day(unixtime1)==day(unixtime2) ) :
		return approx_time(unixtime1) + " and " + approx_time(unixtime2)
	if ( day(unixtime2)==local_day()) :
		return approx_time_date(unixtime1) + " and " + approx_time(unixtime2) + " today"
	return approx_time_date(unixtime1) + " and " + approx_time_date(unixtime2)

def exact_time_period(secs) :
	sec = secs % 60
	min = (secs-sec) / 60
	min = min % 60
	hour = (secs-60*min-sec) / (60*60)
	ret = ""
	if ( hour > 0 ) :
		ret += "%d hours " % hour
	if ( min > 0 or hour > 0 ) :
		ret += "%d mins " % min
	ret += "%d s" % ( sec )

def approx_time_period(secs) :
	min = float(secs)/60
	if (min < 48) :
		min = round(min/15)*15
		if (secs < 60) :
			min = 1
		if (secs < 8*60) :
			min = 5
		return "%d mins" % (min)
	if (secs < 60*60 + 48*60) :
		min = (secs - 60*60)/60
		min = round(float(min)/15)*15
		return "1h %d mins" % (min)

	hour = round(float(secs)/(60*60))
	if ( hour < 24 ) :
		return "%d hours" % (hour)
	if ( hour < 24+18 ) :
		return "1 day %d hours" % (hour-24)
	day = round(hour/24)
	return "%d days" % (day)

def local_to_unixtime(day, hour, min) :
	return time.mktime( ( 2006, 8, day, hour, min, 0, 0, 0, -1 ) ) + 3*60*60

def contains(dict, keys) :
	for k in keys :
		if (not dict.__contains__(k)) : return 0
	return 1

def disallowed(general, dev) :
	hotels=( "Montgomery", "Paragon", "Sainte Claire", "Il Fornaio" )
	venues=( "South Hall media lounge 2", "Museum", "Conference Hall", "South Hall media lounge", "Conference Bar", "S Hall Entrance 2", "Loca stand" )
	is_hotel=0
	for h in hotels :
		if general['nodename']==h : 
			is_hotel=1

	if (not is_hotel) : return 0
	for v in venues :
		if (dev.__contains__(v)) : return 0
	return 1

def get_paired_node(general, dev) :
	paired_node=""
	for node in dev.keys() :
		if (node != general['nodename'] ) : paired_node=node
	return paired_node

def adjust_prio(general, dev, msg) :
	adjustment=0

	if (not contains(msg, ('failurecount', 'successcount'))): return 0
	if (not contains(general, ('majorclass', 'minorclass'))): return 0
	failurecount=msg['failurecount']
	if ( msg['successcount'] > 0 ) : failurecount=0

	if ( msg['failurecount'] > 25 ) : adjustment=0
	else :
		adjustment=10-int(msg['failurecount']/2.5)

	if ( general['majorclass']==2 ) : adjustment+=5

	return adjustment
