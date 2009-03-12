drop view call_and_context
/
create view call_and_context
as
select 
	call1.person				person
	, call1.eventid				call_eventid
	, TO_CHAR(call1.datetime, 'YYYY-MM-DD HH24:MI:SS') datetime
	, call1.direction			call_direction	
	, call1.duration				call_duration
	, call1.datetime-trunc(call1.datetime)	time_of_day
	, sin(2*3.141579*to_number((call1.datetime-trunc(call1.datetime)))) time_of_day_y
	, cos(2*3.141579*to_number((call1.datetime-trunc(call1.datetime)))) time_of_day_x
	, to_number(to_char(call1.datetime, 'D'))	day_of_week 
	, DECODE(time_to_answer.time, -1, NULL, time_to_answer.time)
							time_to_answer
	
	, call1.merged_contact_id					
	, rank.call_count_prop					
	, rank.duration_prop
	, rank.call_count_prop_in
	, rank.duration_prop_in
	, rank.call_count_prop_out
	, rank.duration_prop_out
	
	, prev_base.duration 			duration_prev_base		
	, ROUND( (call1.datetime- prev_base.valid_until)*1440*60 )
						left_prev_base
	, prev_base.base 			prev_base_id
	, current_base.duration 		duration_current_base
	, DECODE(current_base.base, NULL, NULL, 
	  ROUND( (call1.datetime-current_base.valid_from)*1440*60 ) )
						entered_current_base
	, current_base.base 			current_base_id
	, DECODE(current_base.base, NULL, NULL, 
	  ROUND( (current_base.valid_until-call1.datetime)*1440*60) )
						left_current_base
						
	, next_base.duration 			duration_next_base
	, ROUND( (next_base.valid_from-call1.datetime)*1440*60 )
						entered_next_base
	, next_base.base			next_base_id
	
/*	, activeapp1.appuid
	, activeapp1.appname
	, apps.appcategory
	, appcategories.description */
	
	, profile.id				profile_id		
	, profile.volume			profile_volume	
	, profile.vibra				profile_vibra
	
	, activity.state			activity_state
	, DECODE(activity.state, 'active', 0, 
	ROUND((call1.datetime - activity.valid_from - 1/1440)*1440*60) )
						idle_since
						
	, DECODE(stationary.duration, NULL, 0, 1) is_stationary
	
	, DECODE( (SELECT call3.duration FROM call call3 WHERE call3.person=call1.person
		and call1.datetime between call3.datetime+1/(60*1440) and 
		call3.datetime+call3.duration/(60*1440)), NULL, 0, 0, 0, 1) other_call_in_progress
	
	, (call1.datetime-previous_call.datetime)*(60*1440) since_previous_call
	, previous_call.direction	previous_call_direction

	, family_bt.person1			person1_bt_present
	, family_bt.person2			person2_bt_present
	, family_bt.person3			person3_bt_present
	, family_bt.person4			person4_bt_present
	, family_bt.count			family_bt_count

	, other_bt.other1			other1_bt_present
	, other_bt.other2			other2_bt_present
	, other_bt.other3			other3_bt_present
	, other_bt.other4			other4_bt_present
	, other_bt.other5			other5_bt_present
	, other_bt.other6			other6_bt_present
	, other_bt.other7			other7_bt_present
	, other_bt.other8			other8_bt_present
	, other_bt.other9			other9_bt_present
	, other_bt.other10			other10_bt_present
	, other_bt.count			other_bt_count
from 
	(select 
		callX.*,
		(select max(datetime) from call prev
		where prev.datetime < callX.datetime
		and callX.person = prev.person 
		and callX.merged_contact_id = prev.merged_contact_id) prev_datetime
	from call callX
	) call1,
	call previous_call,
	contact_rank rank,
	base prev_base, base current_base, base next_base,
/*	activeapp activeapp1, apps, appcategories, */
	profile,
	time_to_answer, activity, stationary,
	family_bt_env family_bt, others_bt_env other_bt
where
	call1.person=rank.person
	and call1.merged_contact_id=rank.merged_contact_id
	
	and call1.person=current_base.person(+)
	and call1.datetime between current_base.valid_from (+) and current_base.valid_until(+)
	and current_base.person=prev_base.person(+)
	and current_base.seqno - DECODE(current_base.base, NULL, 1, 2) = prev_base.seqno (+)
	and current_base.person=next_base.person(+)
	and current_base.seqno + DECODE(current_base.base, NULL, 1, 2) = next_base.seqno (+)

/*
	and call1.person=activeapp1.person(+)
	and call1.datetime - 1/1440 between activeapp1.valid_from (+) and activeapp1.valid_until(+)
	and activeapp1.appuid= apps.appuid (+) 
	and appcategories.category (+) = apps.appcategory */
	
	and call1.person=profile.person(+)
	and call1.datetime between profile.valid_from (+) and profile.valid_until(+)
	
	and call1.person=time_to_answer.person(+)
	and call1.eventid=time_to_answer.calleventid(+)
	
	and call1.person = activity.person (+)
	and (call1.datetime - 1/1440) between activity.valid_from (+) and activity.valid_until (+)
	
	/* and call1.datetime > TO_DATE('20040601', 'YYYYMMDD') */
/*	and not appcategories.description = 'comm' */
	
	and call1.person = stationary.person (+)
	and call1.datetime between stationary.valid_from (+) and stationary.valid_until (+)
	
	and call1.person = previous_call.person (+)
	and call1.merged_contact_id = previous_call.merged_contact_id (+)
	and call1.prev_datetime = previous_call.datetime (+)

	and call1.person = family_bt.person (+)
	and call1.eventid = family_bt.comm_event_id (+)
	and call1.person = other_bt.person (+)
	and call1.eventid = other_bt.comm_event_id (+)

