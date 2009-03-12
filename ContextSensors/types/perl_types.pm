use typeinfo;

sub type_to_pm {
	my ($typename, $type, $typemap)=@_;
	return "" if ($type->{isa});

	my %uses;

	my $fields=join(" ", map { lc($_) } keys %{$type->{fields}} );

	my $name="shift";
	$name="shift || '" . $type->{defaultname} . "'" if ($type->{defaultname});

	my $uid=$type->{old_uid_value};
	my $id=$type->{id};
	my $major=1;
	my $minor=0;

	my $ret=
"
sub new {
    my \$class=shift;
    my \$name=$name;
    my \$self=new Jaiku::BBData::Compound(\$name);

    return \$class->downbless(\$self);
}

sub type {
    return [ $uid, $id, $major, $minor ];
}

sub field_to_xml {
    return \$field_to_xml;
}
sub xml_to_field {
    return \$xml_to_field;
}
";
    my $field_to_xml="";
    my $xml_to_field="";

	for (my $field_idx=0; $field_idx <= $#{$type->{ordered_fields}}; $field_idx+=2) {
		my $fieldname=$type->{ordered_fields}->[$field_idx];
		my ($fieldtype, $ownname, $fieldclass)=perl_field_info($fieldname, $type, $typemap);
		$fieldname=lc($fieldname);

        my $xmlname;
        if (has_name($typemap, $fieldtype)) { $xmlname=lc($typemap->{$fieldtype}->{defaultname}); }
        else { $xmlname=lc($ownname); }

        $field_to_xml .= "'$fieldname' => '$xmlname', ";
        $xml_to_field .= "'$xmlname' => '$fieldname', ";

		$ret .= "sub " . $fieldname . " {\n";
		$ret .= "    my \$self=shift;\n";
		$ret .= "    return \$self->{$fieldname} if (\$self->{$fieldname});\n";
		$uses{$fieldclass}=1 unless ($fieldclass eq "");
		unless($fieldtype eq "ANY") {
			my $name="('" . lc($ownname) . "')" unless (has_name($typemap, $fieldtype));
			$ret .= "    my \$ret=new $fieldclass$name;\n";
			$ret .= "    \$self->{$fieldname}=\$ret;\n";
			$ret .= "    \$self->push_child(\$ret);\n";
			$ret .= "    return \$ret;\n";
		}
		$ret .= "}\n\n";

		$ret .= "sub set_" . $fieldname . " {\n";
		$ret .= "    my \$self=shift;\n";
		unless($fieldtype eq "ANY") {
		    $ret .= "    my \$c=$fieldclass->downbless(shift());\n";
        } else {
		    $ret .= "    my \$c=Jaiku::BBData::Factory::cast_xml(shift());\n";
        }
		$ret .= "    \$self->replace_child( \$self->{$fieldname}, \$c );\n";
		$ret .= "    \$self->{$fieldname}=\$c;\n";
		if($fieldtype eq "ANY") {
			$ret .= "    \$c->set_type_attributes;\n";
			$ret .= "    \$c->{element}='$ownname';\n";
		}
		$ret .= "}\n\n";
	}

    $ret .= "\nJaiku::BBData::Factory::add_class();\n";

	$ret .="\n1;\n";

	my $uses =
"package Jaiku::BBData::$typename;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( $fields );
";

	foreach my $use (keys %uses) {
		$uses .= "use $use;\n";
	}

    $uses .= "
my \$field_to_xml;
my \$xml_to_field;

BEGIN {
    \$field_to_xml = { $field_to_xml };
    \$xml_to_field = { $xml_to_field };
}
";


	$ret = $uses . $ret;
	return $ret;

}

sub perl_field_info($$$) {
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
	my $fieldclass="";
	if ($fieldtype eq "ANY") {
	} elsif ($typemap->{$fieldtype}->{uid}) {
		#print "TYPEMAP $fieldtype " . $typemap->{$fieldtype} . "\n";
		$fieldclass="Jaiku::BBData::" . $fieldtype;
	} else {
		#$fieldclass="Jaiku::BBData::Simple";
		$fieldclass="Jaiku::BBData::" . $fieldtype;
	}
	return ($fieldtype, $ownname, $fieldclass);
}


1;
