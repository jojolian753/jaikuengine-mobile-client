
CREATE TABLE contact_statistics (
	person			NUMBER(10),
	merged_contact_id	NUMBER(10),
	call_count		NUMBER(10),
	total_duration		NUMBER(10),
	call_count_in		NUMBER(10),
	total_duration_in	NUMBER(10),
	call_count_out		NUMBER(10),
	total_duration_out	NUMBER(10)
);

delete from contact_statistics;

insert into contact_statistics (person, merged_contact_id, call_count, total_duration)
select person, merged_contact_id, count(*), sum(duration)
from call
where not merged_contact_id is null
group by person, merged_contact_id;

commit;

update contact_statistics s
set call_count_in= (
	select count(*) from call c
	where c.person=s.person
	and c.merged_contact_id=s.merged_contact_id
	and (c.direction='Incoming' or c.direction='Missed call')
),
total_duration_in= (
	select sum(duration) from call c
	where c.person=s.person
	and c.merged_contact_id=s.merged_contact_id
	and (c.direction='Incoming' or c.direction='Missed call')
);


update contact_statistics s
set call_count_out= (
	select count(*) from call c
	where c.person=s.person
	and c.merged_contact_id=s.merged_contact_id
	and (c.direction='Outgoing')
),
total_duration_out= (
	select sum(duration) from call c
	where c.person=s.person
	and c.merged_contact_id=s.merged_contact_id
	and (c.direction='Outgoing')
);

commit;

create table person_contact_counts (
	person			NUMBER(10),
	count_contacts		NUMBER(10),
	count_contacts_in	NUMBER(10),
	count_contact_out	NUMBER(10)
);

drop table contact_rank;

create table contact_rank as
select 
person, merged_contact_id, 
rank() over (partition by person order by call_count desc) rank_count,
rank() over (partition by person order by total_duration desc) rank_duration,
rank() over (partition by person order by call_count_in desc) rank_count_in,
rank() over (partition by person order by total_duration_in desc) rank_duration_in,
rank() over (partition by person order by call_count_out desc) rank_count_out,
rank() over (partition by person order by total_duration_out desc) rank_duration_out
from contact_statistics;

alter table contact_rank
add (
	call_count_prop		NUMBER(4, 3),
	duration_prop		NUMBER(4, 3),
	call_count_prop_in	NUMBER(4, 3),
	duration_prop_in	NUMBER(4, 3),
	call_count_prop_out	NUMBER(4, 3),
	duration_prop_out	NUMBER(4, 3) );

