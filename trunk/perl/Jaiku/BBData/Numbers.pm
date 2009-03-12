package Jaiku::BBData::Int;
use strict;
use base qw(Jaiku::BBData::Simple);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub type {
    return [ CONTEXT_UID_BLACKBOARDFACTORY, 1, 1, 0 ];
}

sub default_value {
    return 0;
}

sub check_value {
    my $self=shift;
    my $val=shift;

    if ($val=~/^0x[0-9a-f]+$/i) {
        return oct $val;
    }

    my $int=int($val);
    return $int if ($int eq $val);

    die "$val is not an integer (for field " . $self->element . ")";
}

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::Uint;
use strict;
use base qw(Jaiku::BBData::Int);
use Jaiku::Uid;

sub type {
    return [ CONTEXT_UID_BLACKBOARDFACTORY, 6, 1, 0 ];
}

sub check_value {
    my $self=shift;
    my $val=shift;

    $val = Jaiku::BBData::Int::check_value($self, $val);

    return $val if ($val >= 0);

    die "$val is not positive (for field " . $self->element . ")";
}

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::Uid;
use Jaiku::Uid;
use base qw(Jaiku::BBData::Uint);

sub type {
    return [ CONTEXT_UID_BLACKBOARDFACTORY, 9, 1, 0 ];
}

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::Time;
use base qw(Jaiku::BBData::Simple);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub type {
    return [ CONTEXT_UID_BLACKBOARDFACTORY, 5, 1, 0 ];
}

sub default_value {
    return "00000101T000000";
}

sub check_value {
    my $self=shift;
    my $val=shift;

    if ($val=~/^(\d\d\d\d)(\d\d)(\d\d)T(\d\d)(\d\d)(\d\d)$/) {
        return $val;
    }

    die "$val is not a date (for field " . $self->element . ")";
}

Jaiku::BBData::Factory::add_class();

1;


package Jaiku::BBData::Bool;
use base qw(Jaiku::BBData::Simple);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub type {
    return [ CONTEXT_UID_BLACKBOARDFACTORY, 4, 1, 0 ];
}

sub default_value {
    return 0;
}

sub check_value {
    my $self=shift;
    my $val=shift;

    return 0 if ($val eq "0" || lc($val) eq "false");
    return 1 if ($val eq "1" || lc($val) eq "true");

    die "$val is not boolean (for field " . $self->element . ")";
}

Jaiku::BBData::Factory::add_class();

1;
