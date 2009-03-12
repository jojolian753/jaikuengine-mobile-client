XMLNS = "http://www.cs.helsinki.fi/group/context"
_ns_jc = "{" + XMLNS + "}"

def remove_ns(tag):
	if tag.startswith(_ns_jc):
		tag=tag[len(_ns_jc):len(tag)]
	return tag
