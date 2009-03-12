DROP TABLE basecell;
DROP TABLE basedef;
DROP TABLE base;
DROP TABLE stationary;

CREATE TABLE basecell (
        person     NUMBER(10),     -- uid of person
	baseid     NUMBER(10),     -- uid of the base
	nw         VARCHAR2(40),   -- network
	lac        NUMBER(10),     -- local area code
	cellid     NUMBER(10)      -- cell identifier
);

CREATE TABLE basedef (
	person     NUMBER(10),     -- uid of person
	baseid     NUMBER(10),     -- uid of the base
	name       VARCHAR2(50),
	time_spent NUMBER(10),     -- seconds spent in this base
	num_visits NUMBER(10)      -- number of visits to the base
);

CREATE TABLE base (
	person     NUMBER(10),
	base       NUMBER(10),
	prev_base  NUMBER(10),
	next_base  NUMBER(10),
	valid_from  DATE,
	valid_until DATE,
	duration   NUMBER(10),
	seqno      NUMBER(10)
);

CREATE TABLE stationary (
	person     NUMBER(10),
	valid_from  DATE,
	valid_until DATE,
	duration   NUMBER(10),
	seqno      NUMBER(10)
);

