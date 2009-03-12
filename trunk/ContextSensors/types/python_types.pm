use typeinfo;

sub to_underscores {
	my $str=shift;
	$str=~s/^([[:upper:]]+)/\L$1\E/g;
	$str=~s/([[:lower:]])([[:upper:]]+)/$1_\L$2\E/g;
	return $str;
}

sub to_python_fieldname {
	my $name=shift;
	$name=to_underscores($name);
	$name .= "_field" if (isreserved($name));
	return $name;
}

sub type_to_py {
	my ($typename, $type, $typemap)=@_;
	return "" if ($type->{isa});

	my %uses;

	my $fields=join(" ", map { to_python_fieldname($_) } keys %{$type->{fields}} );

	my $name="name";
	$name .= "='" . $type->{defaultname} . "'" if ($type->{defaultname});
	my $class=$typename;

	my $uid=$type->{uid};
	my $id=$type->{id};
	my $major=1;
	my $minor=0;

	my $ret;
	my $uses= 
'from bbdata.base import *
from bbdata.uid import *
';

	$ret .= uc($typename) . "_TYPE = [ $uid, $id, $major, $minor ]\n\n";
	if ($type->{tuple_uid}) {
		my $uid=$type->{tuple_uid};
		$ret .= uc($typename) . "_TUPLE = [ $uid, $id ]\n\n";
	}
	$ret .=
"
class $class(BBCompound):
	def __init__(self, $name):
		super(BBCompound, self).__init__(name)
";

	for (my $field_idx=0; $field_idx <= $#{$type->{ordered_fields}}; $field_idx+=2) {
		my $fieldname=$type->{ordered_fields}->[$field_idx];
		my ($fieldtype, $ownname, $fieldclass)=python_field_info($fieldname, $type, $typemap);
		$uses{lc($fieldtype)}=1 if ($typemap->{$fieldtype} && $typemap->{$fieldtype}->{uid});

		my $name="'" . lc($ownname) . "'" unless (has_name($typemap, $fieldtype));
		$fieldname=to_python_fieldname($fieldname);
		$ret .=
"		self.$fieldname = $fieldclass($name)
";
	}
	$ret .=
"	#\@classmethod
	def type(self):
		return " . uc($typename) . "_TYPE
	type=classmethod(type)
$class.add_to_factory()
";
	foreach my $use (keys %uses) {
		$uses .= "from bbdata.$use import *\n";
	}
	$uses .= "\n";

	return $uses . $ret;

}

sub python_field_info($$$) {
	my ($fieldname, $type, $typemap)=@_;
	my $kind="fields";
	$kind="impl_fields" unless ($type->{$kind}->{$fieldname});
	
	$fieldtype = $type->{$kind}->{$fieldname};
	my $ownname;
	if (ref $fieldtype) {
		($fieldtype, $ownname)=@$fieldtype;
	} else {
		$ownname=$fieldname;
	}
	my $fieldclass=$fieldtype;
	return ($fieldtype, $ownname, $fieldclass);
}

@py_reserved=qw(and       del       from      not       while    
as        elif      global    or        with     
assert    else      if        pass      yield    
break     except    import    print              
class     exec      in        raise              
continue  finally   is        return             
def       for       lambda    try);
%py_reserved=();
map { $py_reserved{$_}=1 } @py_reserved;
sub isreserved {
	return $py_reserved{$_[0]};
}

#print to_python_fieldname("UUID"), "\n";
#print to_python_fieldname("MappedId"), "\n";
1;
