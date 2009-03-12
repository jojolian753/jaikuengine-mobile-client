select p.person, p.kind,
	(	select count(distinct id) 
		from profile pf 
		where pf.person=p.person and
		pf.valid_from >= p.start_date and
		pf.valid_from < p.end_date ) no_of_profiles,
from periods p

select p.person, p.kind, pf.id, 
	   (select max(pf2.PROFILENAME) from profile pf2
	   where pf2.id=pf.id and pf2.person=p.person) profilename, 
	   count(*), round(sum(valid_until-valid_from)*24, 2)
from periods p, profile pf
		where pf.person=p.person and
		pf.valid_from >= p.start_date and
		pf.valid_from < p.end_date 
group by p.person, p.kind, pf.id, profilename