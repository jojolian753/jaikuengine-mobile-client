package Jaiku::BBData::Simple;
use strict;
use base qw(Jaiku::BBData);
use Carp qw(croak);

sub new {
	my $class=shift;
	my $name=shift;

	my $self=new Jaiku::BBData($name);

	return $class->downbless($self);
}

sub value {
	my $self=shift;
	my $val=$self->first_child;
    return $val if (defined($val));
    return $self->default_value;
}

sub check_value {
    return $_[1];
}

sub as_parsed {
    return $_[0]->value;
}

sub set_value {
	my $self=shift;
    my $val=shift;

    $val=~s/^\s*// if (defined($val));
    $val=~s/\s*$// if (defined($val));

    if ( (! defined($val) ) || $val eq "") {
	    $self->{children}=[ ];
        return;
    }
	my $value=$self->check_value( $val );
	$self->{children}=[ $value ];
}

sub default_value {
    return '';
}

sub _from_xml {
    my $self=shift;

    my $c=$self->{children};
    return if ($#$c==-1);
    if ($#$c==0) {
        $self->set_value($self->check_value( $c->[0] ));
        return;
    }
    my $data="";
    foreach my $t ( @{$self->{children}} ) {
        $data .= $t;
    }
    $self->set_value($data);
}

1;
