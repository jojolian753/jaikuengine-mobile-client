def helloagain(general, dev, msg) :

	code = 19
	prio = 200
	prio += adjust_prio(general, dev, msg)

	if ( disallowed(general, dev) or msg['successcount']>=2 ) :
		return (0, 0, 0, '', '')

	paired_node=get_paired_node(general, dev)
	
	if (paired_node=="") :
		return (0, 0, 0, '', '')

	title = "Hello again!"

	body = "\
Hiya. I last saw you at %s at %s. We keep bumping into each other. Is that coincidence or intelligent design?" % ( approx_time_date(dev[paired_node]['last_seen']), paired_node )

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
