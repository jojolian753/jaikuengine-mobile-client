#!/usr/bin/perl

use typeinfo;

sub field_info($$$) {
	my ($fieldname, $type, $typemap)=@_;
	my $kind="fields";
	$kind="impl_fields" unless ($type->{$kind}->{$fieldname});
	$kind="public_impl_fields" unless ($type->{$kind}->{$fieldname});
	
	$fieldtype = $type->{$kind}->{$fieldname};
	my $ownname;
	if (ref $fieldtype) {
		($fieldtype, $ownname)=@$fieldtype;
	} else {
		$ownname=$fieldname;
	}
	return field_info_type($fieldname, $type, $typemap, $fieldtype, $ownname);
}
sub field_info_type($$$$$) {
	my ($fieldname, $type, $typemap, $fieldtype, $ownname)=@_;
	my $fieldclass="";
	if ($fieldtype eq "ANY") {
		$fieldclass="CBBGeneralHolder";
	} elsif ($fieldtype=~/^[A-Z][A-Z]/ && $fieldtype!~/MD5Hash/ && $fieldtype!~/UUID/) {
		$fieldclass=$fieldtype;
	} elsif ($fieldtype=~/[*&]/) {
		$fieldclass=$fieldtype;
	} elsif ( ( ($typemap->{$fieldtype}->{allocation} || "") eq "heap") || ($fieldtype eq "String") || ($fieldtype eq "String8") ) {
		$fieldclass="CBB" . $fieldtype;
	} else {
		$fieldclass="TBB" . $fieldtype;
	}
	return ($fieldtype, $ownname, $fieldclass);
}

sub constr_class {
	my $fieldtype=shift;
	my $fieldclass="";
	if ($fieldtype eq "ANY") {
		$fieldclass="MBBData*";
	} elsif ($fieldtype=~/^[A-Z][A-Z]/) {
		$fieldclass=$fieldtype;
	} elsif ($fieldtype=~/[*&]/) {
		$fieldclass=$fieldtype;
	} elsif ($fieldtype=~/String/ || $fieldtype=~/TupleSubName/) {
		$fieldclass="const TDesC&";
	} elsif ($fieldtype=~/Uid/) {
		$fieldclass="TUint";
	} elsif ( ($typemap->{$fieldtype}->{allocation} || "") eq "heap") {
		$fieldclass="CBB" . $fieldtype;
	} else {
		$fieldclass="T" . $fieldtype;
	}
	return $fieldclass;

}

sub make_constructor {
	my ($fieldname, $type, $typemap) = @_;
	my ($fieldtype, $ownname, $fieldclass)=field_info($fieldname, $type, $typemap);
	if ($fieldtype eq "ANY") {
		return "i$fieldname(K$fieldname, aFactory, a$fieldname)";
	} elsif ($fieldtype=~/[*&]/) {
		return "i$fieldname(a$fieldname)";
	} elsif ($fieldtype=~/^[A-Z][A-Z]/) {
		return "i$fieldname(a$fieldname)";
	} else {
		return "i$fieldname(a$fieldname, K$fieldname)";
	}
}

sub type_to_x($$$$) {
	my $typename=shift;
	my $type=shift;
	my $typemap=shift;
	my $output=shift;

	my $ret="";
	my $classname="";
	my $cbase="";
	if ( ($type->{allocation} || "") eq "heap" ) {
		$classname="CBB" . $typename;
		$cbase=" public CBase,";
	} else {
		$classname="TBB" . $typename;
	}

	my $includes="";
	for (my $field_idx=0; $field_idx <= $#{$type->{ordered_fields}}; $field_idx+=2) {
		my $fieldname=$type->{ordered_fields}->[$field_idx];
		my ($fieldtype, $ownname, $fieldclass)=field_info($fieldname, $type, $typemap);
		if ($typemap->{$fieldtype}->{uid} || $fieldtype=~/md5/i || $fieldtype=~/uuid/i) {
			$includes .= '#include "csd_' . lc($fieldtype) . ".h\"\n";
		}
	}
	$includes .= $type->{includes} || "";
	if ($output eq "h") {
		$ret .= "#ifndef CONTEXT_CSD_" . uc($typename) . "_H_INCLUDED\n";
		$ret .= "#define CONTEXT_CSD_" . uc($typename) . "_H_INCLUDED\n\n";
		$ret .= "#include \"bbdata.h\"\n";
		$ret .= "#include \"concretedata.h\"\n";
		$ret .= "#include \"context_uids.h\"\n";
		$ret .= $includes;
		$ret .= "\n";
		if ($type->{defaultname}) {
			$ret .= "_LIT(K${typename}, \"" . $type->{defaultname} . "\");\n\n";
		}
		$ret .= "const TTypeName K${typename}Type = { { " .
			$type->{uid} . " }, " . $type->{id} . ", 1, 0 };\n";
		if ($type->{tuple_uid}) {
			my $id =$type->{tuple_id} || $type->{id};
			$ret .= "const TTupleName K${typename}Tuple = { { " .
				$type->{tuple_uid} . " }, " . $id . " };\n";
		}
		$ret .= "\n";

		if ($type->{is_refcounted}) {
			$ret .= "
class M${typename}DeletionNotify \{
public:
	virtual void ${typename}Deleted(const class ${classname} * a${typename}) = 0;
\};

";
			$cbase .= " public MRefCounted, ";
		}
		if ( ! defined($type->{isa}) ) {
			$ret .= "class ${classname} :$cbase public TBBCompoundData {\n";
		} elsif ($type->{isa} eq "String8") {
			$ret .= "class ${classname} :$cbase public TBBFixedLengthStringBase8 {\n";
		} elsif ($typemap->{$type->{isa}}) {
			my ($basetype, $basename, $baseclass)=field_info_type("",
				$type, $typemap, $type->{isa}, "");
			$cbase= " public " . $basetype;
			$ret .= "class ${classname} :$cbase {\n";
		} else {	
			die "unknown base " . $type->{isa};
		}
		$ret .= "public:\n";
		if ($type->{isa} eq "String8") {
			$ret .= "\tTBuf8<" . $type->{length} . "> iValue;\n";
		}

		foreach my $enum ( keys %{$type->{enum}} ) {
			my $vals=$type->{enum}->{$enum};
			$ret .= "\tenum T$enum {\n";
			foreach my $val (@$vals) {
				$ret .= "\t\t$val,\n";
			}
			$ret .= "\t};\n";
		}
		$ret .= "\n";
	} else {
		$ret .= "#include \"csd_" . lc($typename) . ".h\"\n\n";
		$ret .= "
EXPORT_C const TTypeName& ${classname}::Type() const
\{
	return K${typename}Type;
\}

EXPORT_C const TTypeName& ${classname}::StaticType()
\{
	return K${typename}Type;
\}

EXPORT_C TBool ${classname}::Equals(const MBBData* aRhs) const
\{
	const ${classname}* rhs=bb_cast<${classname}>(aRhs);
	return (rhs && *rhs==*this);
\}
";

	}

	my $fieldname_lits="";
	my $partno="";
	my $constructor="";
	my $assignment="";
	my $comparison="";
	my $comp_sep="\treturn (";
	my $i=0;
	my $constr_arguments="";
	my $constr_decl_arguments="";
	my $clone_args="";
	my $seen_any;

	for (my $field_idx=0; $field_idx <= $#{$type->{ordered_fields}}; $field_idx+=2) {
		my $fieldname=$type->{ordered_fields}->[$field_idx];
		my ($fieldtype, $ownname, $fieldclass)=field_info($fieldname, $type, $typemap);
		if ($fieldtype eq "ANY") {
			$seen_any=[$i, $fieldname];
		}
		if ($output eq "h") {
			$ret .= "\t" . $fieldclass . "\ti" . $fieldname . ";\n";
		} else {
			unless (has_name($typemap, $fieldtype)) {
				$fieldname_lits .= "_LIT(K${fieldname}, \"" . lc($ownname) . "\");\n";
			}

			unless ($type->{constructor} && grep { $_ eq $fieldname } 
				@{$type->{constructor}}) {
				unless (has_name($typemap, $fieldtype)) {
					$constructor .= ", i${fieldname}(K${fieldname}";
					$constructor .= ", aFactory" if ($fieldtype eq "ANY");
					$constructor .= ")";
				}
			}
			if ($fieldtype eq "ANY") {
				$comparison .= $comp_sep . "i${fieldname}.Equals(aRhs.i${fieldname}())";
			} else {
				$comparison .= $comp_sep . "i${fieldname} == aRhs.i${fieldname}";
			}
			$comp_sep = " &&\n\t";
			$partno .= "\tcase $i:
		return &i${fieldname};\n";
			if ($fieldtype eq "ANY") {
				$assignment .= "\tif (aRhs.i${fieldname}()) {\n";
				$assignment .= "\t\ti${fieldname}.SetValue(aRhs.i${fieldname}()->CloneL(aRhs.i${fieldname}()->Name()));\n";
				$assignment .= "\t} else {\n";
				$assignment .= "\t\ti${fieldname}.SetValue(0);\n";
				$assignment .= "\t}\n";
				$assignment .= "\ti${fieldname}.SetOwnsValue(ETrue);\n";
			} elsif ($fieldclass=~/CBBString/) {
				$assignment .= "\ti${fieldname}.Zero();";
				$assignment .= " i${fieldname}.Append(aRhs.i${fieldname}());\n";
			} else {
				$assignment .= "\ti${fieldname}=aRhs.i${fieldname};\n";
			}
			$i++;

		}
	}
	if ($type->{is_refcounted}) {
		$constructor .= ", iRefCount(1)";
	}
	my $private_fields="";
	if ($type->{constructor}) {
		my $sep="";
		my $clone_sep="";
		foreach my $fieldname (@{$type->{constructor}}) {
			my ($fieldtype, $ownname, $fieldclass)=
				field_info($fieldname, $type, $typemap);
			$constr_arguments .= $sep . constr_class($fieldtype) . " a" . $fieldname;
			my $def="";
			if ($type->{constructor_defaults} && (($def=$type->{constructor_defaults}->{$fieldname}) || $def eq "0" || $def eq "") ) {
				$def= "=" . $def unless($def eq "");
				$constr_decl_arguments .= $sep . constr_class($fieldtype) . " a" . $fieldname . "$def";
				if ($fieldname eq "Factory" || $def eq "") {
					$clone_args .= $clone_sep . "i" . "$fieldname";
					if ( constr_class($fieldtype)!~/BB/ && $fieldclass=~/BB/ ) {
						$clone_args .= "()";
					}
					$clone_sep=", ";
				}
			} elsif (constr_class($fieldtype) !~ /\&/) {
				$constr_decl_arguments .= $sep . constr_class($fieldtype) . " a" . $fieldname . "=" . constr_class($fieldtype) . "()";
			} else {
				$constr_decl_arguments .= $sep . constr_class($fieldtype) . " a" . $fieldname;
				$clone_args .= $clone_sep . "i";
				if ( constr_class($fieldtype)!~/BB/ && $fieldclass=~/BB/ ) {
					$clone_args .= $fieldname . "()";
				} else {
					$clone_args .= $fieldname;
				}
				$clone_sep=", ";
			}
			$sep=", ";
			$constructor .= ", " . make_constructor($fieldname, $type, $typemap);
		}
	}
    my $destructor="";

	foreach my $impl_fields ( [ "private", $type->{impl_fields} ],
		[ "public", $type->{public_impl_fields} ] ) {

		next unless ($impl_fields->[1]);
		if ($output eq "h") {
			$ret .= "\n";
		}
		foreach my $fieldname (keys %{$impl_fields->[1]}) {
			my ($fieldtype, $ownname, $fieldclass)=
				field_info($fieldname, $type, $typemap);
			if ($impl_fields->[0] eq "private") {
				$private_fields .= "\t$fieldclass i${fieldname};\n";
			} elsif ($output eq "h") {
				$ret .= "\t$fieldclass i${fieldname};\n";
			}
			unless (( $type->{constructor} && grep { $_ eq $fieldname } 
				@{$type->{constructor}}) ||
				$fieldclass=~/[*&]/) {
				if ($fieldclass=~/BB/) {
					$constructor .= ", i${fieldname}(K${fieldname})";
				}
			}
		    if ($fieldclass=~/[*]/ && $fieldclass!~/Factory/) {
                if ($fieldclass=~/ErrorInfo/) {
                    $destructor .= "\tif (i${fieldname}) i${fieldname}->Release();\n";
                } else {
                    $destructor .= "\tdelete i${fieldname};\n";
                }
            }
			my $const="";	
			$const=" const" if ($fieldclass=~/const/);
			if ($impl_fields->[0] eq "private") {
				if ($output eq "h") {
					$ret .= "\tIMPORT_C $fieldclass $fieldname()$const;\n";
				} else {
					$ret .= "EXPORT_C $fieldclass ${classname}::$fieldname()$const\n";
					$ret .= "{\n";
					$ret .= "\treturn i${fieldname};\n";
					$ret .= "}\n\n";
				}
			}
		}
	}
	if ($output eq "cpp") {
		my $name="";
		unless ($type->{defaultname}) {
			$name=" aName";
			$clone_args=", " . $clone_args if ($clone_args);
			$clone_args=$name . $clone_args;
		}
		$ret .= "
EXPORT_C MBBData* ${classname}::CloneL(const TDesC&$name) const
\{
	bb_auto_ptr<${classname}> ret(new (ELeave) ${classname}($clone_args));
	*ret=*this;
	return ret.release();
\}
";
		if ($type->{isa}) {
			$ret .= "\nIMPLEMENT_ASSIGN($classname)\n";
		}
	}


	if ($output eq "h") {
		$ret .= '
	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();
';
		unless ( defined($type->{isa})) {
			$ret .= '
	IMPORT_C const MBBData* Part(TUint aPartNo) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;
';
		} elsif ($type->{isa} eq "String8") {
			$ret .= '
	TDes8& operator()() { return iValue; }
	const TDesC8& operator()() const { return iValue; }

	virtual TDes8& Value() { return iValue; }
	virtual const TDesC8& Value() const  { return iValue; }
';

		}

		$ret .= '
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
';
#
# note: IMPORT_C on the destructor triggers a bug in ARMCC
# creating missing symbols, so no
#
	if ($type->{isa}) {
		$ret .= '
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
';
	}
		
		$ret .= "\tIMPORT_C ${classname}(";
		unless ($type->{defaultname}) {
			$ret .= "const TDesC& aName";
			$ret .= ", " if ($constr_decl_arguments);
		}
		$ret .= "$constr_decl_arguments);\n";
		$ret .= "\tIMPORT_C ${classname}& operator=(const ${classname}& aValue);\n";
		$ret .= "\tIMPORT_C bool operator==(const ${classname}& aRhs) const;\n";
		if ($type->{is_refcounted}) {
			$ret .= "
	IMPORT_C void AddRef() const;
	IMPORT_C void Release() const;
	IMPORT_C void SetDeletionNotify(M${typename}DeletionNotify* aDeletionNotify);
";
		}
		if ($type->{is_refcounted} || $destructor ne "") {
			$ret .= "
	virtual ~${classname}();
";
        }
		$ret .= "private:\n";
		if ($seen_any) {
			$ret .= "\tIMPORT_C MBBData* GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto);\n";
		}
		$ret .= $private_fields;
		if ($type->{is_refcounted}) {
		$ret .= "
	mutable TUint iRefCount;
	M${typename}DeletionNotify* iDeletionNotify;
";
		}

		$ret .= "};\n\n";
		$ret .= "#endif\n";
	} else {
		$ret .= $fieldname_lits . "\n";

		my $constr_name="K${typename}";
		unless ($type->{defaultname}) {
			$constr_name="aName";
			my $name_arg="const TDesC& aName";
			$name_arg .= ", " if ($constr_arguments);
			$constr_arguments = $name_arg . $constr_arguments;
		}
		if ($type->{isa} eq "String8") {
			$base="TBBFixedLengthStringBase8";
			$comparison="\treturn (iValue.Compare(aRhs.iValue)==0";
			$assignment="\tiValue=aRhs.iValue;\n";
		} elsif (! defined($type->{isa}) ) {
			$base="TBBCompoundData";
		} else {
			die "unknown base class";
		}
		$ret .= "EXPORT_C ${classname}::${classname}($constr_arguments) : $base($constr_name) $ constructor \{ \}\n\n";

		$ret .= "EXPORT_C bool ${classname}::operator==(const ${classname}& aRhs) const\n";
		$ret .= "\{\n";
		$ret .= $comparison . ");\n";
		$ret .= "\}\n\n";

		if ($base eq "TBBCompoundData") {
			$ret .= "EXPORT_C const MBBData* ${classname}::Part(TUint aPartNo) const\n";
			$ret .= "\{\n";
			$ret .= "\tswitch(aPartNo) \{\n";
			$ret .= $partno;
			$ret .= "\tdefault:\n\t\treturn 0;\n\t\}\n\}\n\n";
			$ret .= "_LIT(K${typename}Space, \" \");\n";
			$ret .= "EXPORT_C const TDesC& ${classname}::StringSep(TUint aBeforePart) const { return K${typename}Space; }\n\n";
		}

		if ($seen_any) {
			my ($index, $name)=@$seen_any;
			$ret .= "EXPORT_C MBBData* ${classname}::GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto)\n";
			$ret .= "\{\n";
			$ret .= "\tMBBData* p=TBBCompoundData::GetPart(aName, aType, aPartNoInto);\n";
			$ret .= "\tif (!p || p==&i$name) {\n";
			$ret .= "\t\ti${name}.SetValue(iFactory->CreateBBDataL(aType, K${name}, iFactory));\n";
			$ret .= "\t\taPartNoInto=$index;\n";
			$ret .= "\t\tp=&i${name};\n";
			$ret .= "\t}\n";
			$ret .= "\treturn p;\n";
			$ret .= "}\n";

		}
		
		$ret .= "EXPORT_C ${classname}& ${classname}::operator=(const ${classname}& aRhs)\n";
		$ret .= "\{\n";
		$ret .= $assignment;
		$ret .= "\treturn *this;\n";
		$ret .= "\}\n";

        if ($type->{is_refcounted} || $destructor ne "") {
			$ret .= '
EXPORT_C ' . $classname . '::~' . $classname . '()
{
';
            if ($type->{is_refcounted}) {
			    $ret .= '
	if (iDeletionNotify) iDeletionNotify->' . $typename . 'Deleted(this);
';
            }
            $ret .= $destructor . "\n}\n";
        }

		if ($type->{is_refcounted}) {
			$ret .= '
EXPORT_C void ' . $classname . '::AddRef() const
{
	++iRefCount;
}

EXPORT_C void ' . $classname . '::Release() const
{
	if (iRefCount==0) {
		User::Panic(_L("CSD"), 1);
	}
	--iRefCount;
	if (iRefCount==0) 
		delete this;
}
EXPORT_C void ' . $classname . '::SetDeletionNotify(M' . $typename . 'DeletionNotify* aDeletionNotify)
{

	iDeletionNotify=aDeletionNotify;
}


';
	}

	}


	return $ret;
}

sub type_to_h($$$) {
	return type_to_x($_[0], $_[1], $_[2], "h");
}

sub type_to_cpp($$) {
	return type_to_x($_[0], $_[1], $_[2], "cpp");
}

1;
