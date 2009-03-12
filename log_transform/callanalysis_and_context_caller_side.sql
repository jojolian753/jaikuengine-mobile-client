select call_and_context.*, callanalysis.*
from call_and_context, callanalysis, callrecordmatching
where 
	call_and_context.call_eventid = callrecordmatching.caller_call_id
and	callanalysis.phonecallid = callrecordmatching.phonecallid
and	call_and_context.person = callanalysis.caller
