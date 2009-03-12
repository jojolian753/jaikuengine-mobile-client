DROP TABLE callanalysis;

CREATE TABLE callanalysis (
	phonecallid		NUMBER(10),
	past_desirability_type	VARCHAR2(10),
	past_desirability_from	NUMBER(10),
	past_desirability_to	NUMBER(10),
	future_desirability_type VARCHAR2(10),
	future_desirability_from NUMBER(10),
	future_desirability_to	NUMBER(10),
	caller			NUMBER(10),
	caller_loc		VARCHAR2(255),
	callee			NUMBER(10),
	callee2			NUMBER(10),
	callee_loc		VARCHAR2(255),
	callee_disruption	VARCHAR2(255),
	callee_pleasantness	VARCHAR2(255),
	length			NUMBER(10),
	rationale		VARCHAR2(255),
	collaboration_type	VARCHAR2(255),
	comments		VARCHAR2(100)
);

