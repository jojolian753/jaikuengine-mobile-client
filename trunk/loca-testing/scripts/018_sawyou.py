def sawyou(general, dev, msg) :

	code = 18
	prio = 200
	prio += adjust_prio(general, dev, msg)

	if ( disallowed(general, dev) or msg['successcount']>=2 ) :
		return (0, 0, 0, '', '')

	paired_node=get_paired_node(general, dev)
	
	if (paired_node=="") :
		return (0, 0, 0, '', '')

	title = "Hey art junky!"

	body = "\
You seem to have been all over this festival like a rash. How many artworks have you packed in since I last saw you over by the %s at %s?" % ( paired_node, approx_time_date(dev[paired_node]['last_seen']) )

	body += sig('Sly')
	body += location_and_time(general)
	body += dev_log(general, dev, msg)
	body += loca_sig()

	return ( prio, code, title, title, body )
