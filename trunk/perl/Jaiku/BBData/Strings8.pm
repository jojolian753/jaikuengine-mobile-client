package Jaiku::BBData::StringBase8;
use base qw(Jaiku::BBData::Simple);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub check_value {
    my $self=shift;
    my $val=shift;

    my $maxlen=$self->maxlength;

    return $val if ($maxlen==0);

    return $val if (length($val)<=$maxlen*2);

    die "$val is too long (maximum $maxlen, for field " . $self->element . ")";
}

1;

package Jaiku::BBData::ShortString8;
use base qw(Jaiku::BBData::StringBase8);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub maxlength { return 50; }

sub type { return [ CONTEXT_UID_BLACKBOARDFACTORY, 13, 1, 0 ]; }

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::String8;
use base qw(Jaiku::BBData::StringBase8);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub maxlength { return 0; }

sub type { return [ CONTEXT_UID_BLACKBOARDFACTORY, 16, 1, 0 ]; }

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::UUID;
use base qw(Jaiku::BBData::StringBase8);
use Jaiku::Uid;
use Jaiku::BBData::Factory;
use Data::UUID;

my $ug;

sub maxlength { return 16; }

sub type { return [ CONTEXT_UID_BLACKBOARDFACTORY, 22, 1, 0 ]; }

sub with_dashes {
    my $self=shift;
    my $str=$self->value;
    my @lens=( 8, 4, 4, 4, 12 );
    my $ret; my $pos=0; my $sep="";
    map { $ret .= $sep . substr($str, $pos, $_); $pos+=$_; $sep="-"; } @lens;
    return $ret;
}

sub check_value {
    my $self=shift;
    my $val=shift;
    $val=~s/-//g;
    return Jaiku::BBData::StringBase8::check_value($self, $val);
}

sub generate {
    my $self=shift;
    $ug ||= new Data::UUID;
    $self->set_value(lc($ug->create_str()));
}

Jaiku::BBData::Factory::add_class();

1;

package Jaiku::BBData::MD5Hash;
use base qw(Jaiku::BBData::StringBase8);
use Jaiku::Uid;
use Jaiku::BBData::Factory;

sub maxlength { return 16; }

sub type { return [ CONTEXT_UID_BLACKBOARDFACTORY, 21, 1, 0 ]; }

Jaiku::BBData::Factory::add_class();

1;



