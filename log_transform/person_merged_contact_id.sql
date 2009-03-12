delete from person_merged_contact_id
/
insert into person_merged_contact_id
select p.person_id person, c.person is_persons, c.merged_contact_id
from call c, persons p
where c.phoneno=p.phoneno
group by p.person_id, c.person, c.merged_contact_id
/
commit
/
