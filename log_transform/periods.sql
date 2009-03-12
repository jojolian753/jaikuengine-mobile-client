create table periods (
	person	 		 number(10),
	start_date		 date,
	end_date		 date,
	kind			 varchar(1)
);

/

declare
	   i binary_integer;
begin
	 delete from periods;
	 for i in 1..4 loop
	 	  insert into periods (person, start_date, end_date, kind)
		  values (i, to_date('2004-05-24', 'YYYY-MM-DD'), to_date('2004-06-22', 'YYYY-MM-DD'), 'A');
	 	  insert into periods (person, start_date, end_date, kind)
		  values (i, to_date('2004-06-22', 'YYYY-MM-DD'), to_date('2004-08-10', 'YYYY-MM-DD'), 'B');
	 	  insert into periods (person, start_date, end_date, kind)
		  values (i, to_date('2004-08-10', 'YYYY-MM-DD'), to_date('2004-09-02', 'YYYY-MM-DD'), 'A');
     end loop;
	 
	 for i in 11..15 loop
	 	  insert into periods (person, start_date, end_date, kind)
		  values (i, to_date('2004-12-01', 'YYYY-MM-DD'), to_date('2005-01-13', 'YYYY-MM-DD'), 'A');
	 	  insert into periods (person, start_date, end_date, kind)
		  values (i, to_date('2005-01-13', 'YYYY-MM-DD'), to_date('2005-03-11', 'YYYY-MM-DD'), 'B');
	 end loop;
	 
	 for i in 21..26 loop
	 	  insert into periods (person, start_date, end_date, kind)
		  values (i, to_date('2005-04-11', 'YYYY-MM-DD'), to_date('2005-05-24', 'YYYY-MM-DD'), 'B');
	 end loop;
	 
	 commit;
end;

/