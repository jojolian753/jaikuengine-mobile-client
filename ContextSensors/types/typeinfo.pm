sub has_name($$) {
	my ($typemap, $fieldtype)=@_;
	return ($typemap->{$fieldtype} && $typemap->{$fieldtype}->{defaultname});
}

1;
