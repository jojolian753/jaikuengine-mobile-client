#!/usr/bin/perl

package Jaiku::XMLSplit;

use XML::SAX::Writer;
use XML::SAX;
use base XML::SAX::Base;
use strict;

sub new {
	my ($class, $xml)=@_;
	my $self=bless {}, $class;
	$self->SUPER::new();
	my $parser = XML::SAX::ParserFactory->parser( Handler => $self );
	my $output="";
	$self->{output}=\$output;
	my $w=new XML::SAX::Writer(Output => \$output);
	$w->start_document();
	$self->{writer}=$w;
	$self->{elements}=[];
	$parser->parse_string($xml);
	return $self;
}
sub start_element {
	#print "start_element\n";
	my ($self, $el)=@_;
	$self->{depth}++;
	if ($self->{depth} > 1) {
		if ($self->{depth}==2) {
			# old clients send unnecessary type attributes on
			# presence's children
			delete $el->{Attributes}->{"{}module"};
			delete $el->{Attributes}->{"{}id"};
			delete $el->{Attributes}->{"{}major_version"};
			delete $el->{Attributes}->{"{}minor_version"};
		}
		$self->{writer}->start_element($el);
	}
}

sub characters {
	#print "characters\n";
	my ($self, $chars)=@_;
	if ($self->{depth} > 1) {
		$self->{writer}->characters($chars);
	}
}
sub end_element {
	#print "end_element\n";
	my ($self, $el)=@_;
	if ($self->{depth} > 1) {
		$self->{writer}->end_element($el);
	}
	$self->{depth}--;
	if ($self->{depth} == 1) {
		#print $el->{LocalName}, " ", $self->{depth}, "\n";
		push @{$self->{elements}}, [ $el->{LocalName}, ${$self->{output}} ];
		${$self->{output}}="";
	}
}

sub elements {
	my $self=shift;
	return @{$self->{elements}};
}

1;
