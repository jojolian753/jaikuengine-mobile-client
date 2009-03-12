package Jaiku::BBData::Factory;
use strict;

my %types;

sub add_class {
    my $class=shift;

    unless (defined($class)) {
        my ($filename, $line);
        ($class, $filename, $line)=caller;
    }
    my $type=$class->type;

    my $uid=Jaiku::Uid::canon($type->[0]);
    $types{$uid}{$type->[1]}=$class;
}

sub create_instance {
    my $type=shift;
    my $name=shift;

    my $class=$types{$type->[0]}{$type->[1]};
    return $class->new($name);
}

sub cast_xml {
    my $element=shift;

    if ($element->isa('Jaiku::BBData')) {
        $element->set_type_attributes;
        return $element;
    }
    if ($element->isa('Jaiku::RawXML')) {
        return $element;
    }
    my ($ns, $name)=$element->element();
    my $type=[
        $element->{attrs}->{'{}module'},
        $element->{attrs}->{'{}id'} ];
    
    my $uid=$type->[0];
    $uid=oct $uid if ($uid=~/^0x/i);
    $uid=Jaiku::Uid::canon($uid);
    my $class=$types{$uid}{$type->[1]};

    die "cannot find class for $uid " . $type->[1] unless($class);
    return $class->downbless($element);
}

1;
