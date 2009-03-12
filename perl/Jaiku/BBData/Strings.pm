package Jaiku::BBData::StringBase;
use base qw(Jaiku::BBData::Simple);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub check_value {
    my $self=shift;
    my $val=shift;

    my $maxlen=$self->maxlength;
    return $val if ($maxlen==0);

    return $val if (length($val)<=$maxlen);

    die "$val is too long (maximum $maxlen, for field " . $self->element . ")";
}

1;

package Jaiku::BBData::ShortString;
use base qw(Jaiku::BBData::StringBase);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub maxlength { return 50; }

sub type { return [ CONTEXT_UID_BLACKBOARDFACTORY, 2, 1, 0 ]; }

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::LongString;
use base qw(Jaiku::BBData::StringBase);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub maxlength { return 255; }

sub type { return [ CONTEXT_UID_BLACKBOARDFACTORY, 10, 1, 0 ]; }

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::TupleSubName;
use base qw(Jaiku::BBData::StringBase);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub maxlength { return 128; }

sub type { return [ CONTEXT_UID_SENSORDATAFACTORY, 8, 1, 0 ]; }

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::String;
use base qw(Jaiku::BBData::StringBase);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub maxlength { return 0; }

sub type { return [ CONTEXT_UID_BLACKBOARDFACTORY, 11, 1, 0 ]; }

Jaiku::BBData::Factory::add_class();

1;

