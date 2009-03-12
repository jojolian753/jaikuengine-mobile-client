DROP TABLE activity;
DROP TABLE cell;
DROP TABLE call;
DROP TABLE profile;
DROP TABLE bt;
DROP TABLE activeapp;

CREATE TABLE activity (
	person	NUMBER(10),
	state	VARCHAR2(10),
	valid_from DATE,
	valid_until DATE,
	seqno	NUMBER(10)
);

CREATE TABLE cell (	
	person	NUMBER(10),
	nw	VARCHAR2(40),
	lac	NUMBER(10),
	cellid	NUMBER(10),
	valid_from DATE,
	valid_until DATE,
	seqno	NUMBER(10)
);

CREATE TABLE profile (
	person		NUMBER(10),
	id		NUMBER(2),
	profilename	VARCHAR2(20),
	vibra		NUMBER(1),
	volume		NUMBER(2),
	ringtype	NUMBER(2),
	valid_from 	DATE,
	valid_until 	DATE,
	seqno		NUMBER(10)
);


CREATE TABLE call (
	person		NUMBER(10),
	phoneno		VARCHAR2(20),
	status		VARCHAR2(20),
	duration	NUMBER(10),
	contact		VARCHAR2(100),
	description	VARCHAR2(200),
	datetime	DATE,
	remote		VARCHAR2(100),
	direction	VARCHAR2(20),
	eventid		NUMBER(10),
	seqno	NUMBER(10)
);

CREATE TABLE sms (
	person		NUMBER(10),
	phoneno		VARCHAR2(20),
	status		VARCHAR2(20),
	duration	NUMBER(10),
	contact		VARCHAR2(100),
	description	VARCHAR2(200),
	datetime	DATE,
	remote		VARCHAR2(100),
	direction	VARCHAR2(20),
	eventid		NUMBER(10),
	seqno		NUMBER(10)
);


CREATE TABLE bt (
	person		NUMBER(10),
	btaddr		VARCHAR2(12),
	nick		VARCHAR2(255),
	majorclass	NUMBER(10),
	minorclass	NUMBER(10),
	serviceclass	NUMBER(10),
	datetime	DATE,
	seqno		NUMBER(10)
);

CREATE TABLE activeapp (
	person		NUMBER(10),
	appuid		VARCHAR2(8),
	appname		VARCHAR2(128),
	valid_from 	DATE,
	valid_until 	DATE,
	seqno		NUMBER(10)
);

create table person_merged_contact_id (
	person		NUMBER(10),
	is_persons	NUMBER(10),
	merged_contact_id NUMBER(10)
)
