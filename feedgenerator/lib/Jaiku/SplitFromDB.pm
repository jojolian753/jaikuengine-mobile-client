#!/usr/bin/perl

use DBI;
package Jaiku::SplitFromDB;
use Jaiku::FeedGenerator;
use fields ('dbh', 'items', 'timestamp', 'id', 'feedgen');
use strict;

sub new {
	my ($class, $dbh)=@_;
	die "has to have dbh" unless $dbh;
	my $self = fields::new($class);
	$self->{dbh} = $dbh;
	$self->{feedgen} = new Jaiku::FeedGenerator($self);
	check_install_schema($dbh);
	return $self;
}

sub _process_from_db {
	#print "_PROCESS_FROM_DB\n";
	my ($self, $n)=@_;
	$self->{items}=[];
	my $dbh=$self->{dbh};
	my $feedgen=$self->{feedgen};
	my $to_process=$dbh->selectall_arrayref( qq{
		SELECT pqueue_id, pqueue_from, pqueue_xml, pqueue_tstamp 
		 FROM pqueue
		ORDER BY pqueue_id ASC
		LIMIT $n} );
	#print "COUNT: ", $#{$to_process}, "\n";
	my @to_delete;
	my $processed=$#{$to_process}+1;
	#open(DEBUG, ">debug.txt") || die "cannot open debug.txt";
	foreach my $pqueue (@$to_process) {
		my ($id, $nick, $xml, $tstamp)=@$pqueue;
		#print DEBUG $xml, "\n";
		$self->{timestamp}=$tstamp;
		$self->{id}=$id;
		eval { $feedgen->handle_next($nick, $xml); };
		if ($@) {
			push(@{$self->{items}}, [ $nick, "error", $@, $self->{timestamp}, $self->{id} ]);
		}
		push(@to_delete, $id);
	}
	close(DEBUG);
	#print "COUNT: ", $#{$self->{items}}, "\n";
	foreach my $item (@{$self->{items}}) {
		my ($nick, $type, $xml, $tstamp, $id)=@$item;
		#print "insert into feed\n";
		$dbh->do(qq{
			INSERT INTO feed (
				feed_from, feed_tstamp,
				feed_type, feed_xml,
				phistory_id )
			VALUES (?, ?, ?, ?, ?) },
			undef, $nick, $tstamp, $type, $xml, $id);

		#print "insert into feed_latest\n";
		$dbh->do(qq{
			INSERT INTO feed_latest (
				flatest_from, flatest_tstamp,
				flatest_type, flatest_xml,
				phistory_id )
			VALUES (?, ?, ?, ?, ?)
			ON DUPLICATE KEY UPDATE flatest_xml=VALUES(flatest_xml),
			flatest_tstamp=VALUES(flatest_tstamp) },
			undef, $nick, $tstamp, $type, $xml, $id);
	}
	foreach my $id (@to_delete) {
		$dbh->do(qq{
			INSERT INTO phistory (
				phistory_id,
				phistory_from,
				phistory_xml,
				phistory_tstamp
			) SELECT pqueue_id,
				pqueue_from,	
				pqueue_xml,
				pqueue_tstamp
			   FROM pqueue
			  WHERE pqueue_id=? },
			undef, $id);
		$dbh->do("DELETE FROM pqueue WHERE pqueue_id=?", undef, $id);
		#print "DELETED $id\n";
	}
	return $processed;
}

sub process_from_db {
	#print "PROCESS_FROM_DB\n";
	my ($self, $n)=@_;
	my $dbh=$self->{dbh};
	$dbh->begin_work;
	my $processed=0;
	local $dbh->{RaiseError};
	$dbh->{RaiseError}=1;
	eval { $processed=$self->_process_from_db($n); };
    	if ($@) {
		$dbh->rollback;
		die $@;
	}
	$dbh->commit;
	return $processed;
}

sub fetch_last_known {
	my ($self, $nick) = @_;
	my $dbh=$self->{dbh};
	my $rows=$dbh->selectall_arrayref(qq{
		SELECT flatest_xml
		  FROM feed_latest
		 WHERE flatest_from=? AND NOT flatest_type='error' }
		, undef, $nick);
	return "" unless $rows;
	my $xml="20000101T010101<presencev2>";
	foreach my $row (@$rows) {
		$xml .= $row->[0];
	}
	$xml.="</presencev2>";

	my $stanza="<presence>";
	$stanza .= XML::Simple::XMLout( { status=>$xml }, NoAttr => 1, KeepRoot=>1 );
	$stanza.="</presence>";
	#print "LAST_KNOWN " . $stanza . "\n";
	return $stanza;
}

sub new_item {
	my ($self, $nick, $type, $xml)=@_;
	push(@{$self->{items}}, [ $nick, $type, $xml, $self->{timestamp}, $self->{id} ]);
}

sub check_install_schema {
    return;
    my $dbh=shift;
    eval {
        $dbh->do(qq{
            CREATE TABLE pqueue (
				 pqueue_id     SERIAL,
                                 pqueue_from   VARCHAR(100) NOT NULL,
				 pqueue_tstamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
				 pqueue_xml TEXT
                                 )});
    };
    if ($@ && $@ !~ /table '\w+' already exists/i) {
        die "SQL error: $@\n";
    }
    eval {
        $dbh->do(qq{
            CREATE TABLE phistory (
				 phistory_id     BIGINT,
                                 phistory_from   VARCHAR(100) NOT NULL,
				 phistory_tstamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
				 phistory_xml TEXT
                                 )});
    };
    if ($@ && $@ !~ /table '\w+' already exists/i) {
        die "SQL error: $@\n";
    }
     eval {
        $dbh->do(qq{
            CREATE TABLE feed (
				 feed_id     SERIAL,
                                 feed_from   VARCHAR(100) NOT NULL,
				 feed_tstamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
				 feed_type   TEXT,
				 feed_xml    TEXT,
				 phistory_id BIGINT
                                 )});
    };
    if ($@ && $@ !~ /table '\w+' already exists/i) {
        die "SQL error: $@\n";
    }

}

1;
