package Jaiku::BBData;
use strict;
use base qw(DJabberd::XMLElement);
use Carp qw(croak);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub downbless {
    my $class = shift;

    if (ref $_[0]) {
        my ($self) = @_;
        # 'fields' hackery.  this will break in Perl 5.10
        {
            no strict 'refs';
            return $self if ($self->[0] == \%{$class . "::FIELDS" });
            $self->[0] = \%{$class . "::FIELDS" };
        }
        bless $self, $class;
        $self->_from_xml;
        return $self;
    } else {
        croak("Bogus use of downbless.");
    }
}

sub new {
	my $class=shift;
	my $name=shift;
	croak("BBData has to have a name") unless($name);

	my $self=new DJabberd::XMLElement("http://www.cs.helsinki.fi/group/context",
		$name, {}, []);

	return $self;
}

sub set_type_attributes {
    my $self=shift;
    my @type=@{$self->type};
    $self->set_attr('{}module', $type[0]);
    $self->set_attr('{}id', $type[1]);
    $self->set_attr('{}major_version', $type[2]);
    $self->set_attr('{}minor_version', $type[3]);
}

1;
