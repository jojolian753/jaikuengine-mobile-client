# Copyright (c) 2007-2009 Google Inc.
# Copyright (c) 2006-2007 Jaiku Ltd.
# Copyright (c) 2002-2006 Mika Raento and Renaud Petit
#
# This software is licensed at your choice under either 1 or 2 below.
#
# 1. MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# 2. Gnu General Public license 2.0
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
# This file is part of the JaikuEngine mobile client.

use Cwd;

my %mmps;
my @mmps;
sub mmps {
	return @mmps;
}

my %mks;
sub mks {
	return %mks;
}

our $platform;

open(BLDINF, "<bld.inf") || die "cannot open bld.inf";
while(<BLDINF>) {
	next if (m!^//!);
	next unless ((/mmp$/i)||(/mk$/i));
	
	chop; s///;
	my $mmp=$_;

	if (/mk$/i) {
	  /^gnumakefile (.*)\\([^\\]*).mk$/i;
	  my ($path, $file)=($1, $2);
	  $mks{lc($file)}=$path;
        }
	else {
	  /^(.*)\\([^\\]*).mmp$/i;
	  my ($path, $file)=($1, $2);
	  $mmps{lc($file)}=$path;
	  push(@mmps, lc($file));
        }
      }
close(BLDINF);

sub mmp($) {
	my $proj=shift;
	return undef unless $mmps{lc($proj)};
	return $mmps{lc($proj)} . "\\" . $proj . ".mmp";
}

sub mk($) {
	my $proj=shift;
	return undef unless $mks{lc($proj)};
	return $mks{lc($proj)} . "\\" . $proj . ".mk";
}

sub mmp_or_mk($) {
  my $proj=shift;
  if ($mks{lc($proj)}) {
    return mk($proj);
  } else {
    return mmp($proj);
  }
}


my $cwd=`echo %CD%`;
$cwd=~s/[\r\n]//g;
my $drive=substr($cwd, 0, 2);

sub xmlfilename($$) {

	my $proj=shift;
	my $platform=shift;
	my $makefile=$drive . $ENV{'EPOCROOT'};
	$makefile.="epoc32\\BUILD";
	my $d=$cwd;
	if ($makefile!~/3rd/i) {
		$d=~s/[a-z]://i;
		$d=~s!/!\\!g;
		$makefile.=$d;
		$makefile.="\\";
		my $mmp=mmp($proj);
		$mmp=~s/\\[^\\]+$/\\/;
		$makefile .= $mmp . $proj . "\\" . $platform . "\\" . $proj . ".xml";
	} else {
		$d=~s/[a-z]://i;
		$d=~s!/!\\!g;
		$makefile.=$d;
		$makefile.="\\";
		$makefile .= $proj . "\\" . $platform . "\\" . $proj . ".xml";
	}
	return $makefile;
}

sub makefilename($) {

	my $proj=shift;
	my $makefile=$ENV{'EPOCROOT'};
	$makefile.="epoc32\\BUILD";
	my $d=cwd;
	if ($makefile!~/3rd/i) {
		$d=~s/[a-z]://i;
		$d=~s!/!\\!g;
		$makefile.=$d;
		$makefile.="\\";
		my $mmp=mmp($proj);
		$mmp=~s/\\[^\\]+$/\\/;
		$makefile .= $mmp . $proj . "\\" . $platform . "\\" . $proj . "." . $platform;
	} else {
		$d=~s/[a-z]://i;
		$d=~s!/!\\!g;
		$makefile.=$d;
		$makefile.="\\";
		$makefile .= $proj . "\\" . $platform . "\\" . $proj . "." . $platform;
	}

	return $makefile;
}

sub getdepends($$$$) {
	# returns \@files, $type, $has_resource, \@sourcefiles, $sourcedir, \@includedirs,
	#	$in_bitmap, $deffile
	my $file=shift;
	my $dir=shift;
	my $sourcedir=shift;
	my @sourcefiles=();
	my @includedirs=();
	my $in_bitmap=shift;
	my @ret; my $type; my $has_resource;
	if ($file=~/^\s*$/) {
		return (\@ret, $type, $has_resource, \@sourcefiles, $sourcedir, \@includedirs,
			$in_bitmap);
	}

	$dir=~s/\\[^\\]+$/\\/;
	my $in="fh" . $fhnum++;
	unless(open($in, "<$file")) {
		my $plainfile=$file;
		$plainfile=~s/.*\\([^\\]+)/\1/;
		open($in, "<" . $ENV{'EPOCROOT'} . "epoc32\\include\\" . $plainfile) || die "cannot open $file nor " . $ENV{'EPOCROOT'} . "epoc32\\include\\$plainfile";
		$file=$ENV{'EPOCROOT'} . "epoc32\\include\\" . $plainfile;
	}
	push(@ret, $file);
	while(<$in>) {
		s!//.*!!;
		if ( ! $in_bitmap ) {
			if (/targettype\s*(.*)/i) {
				$type=uc($1);
				next;
			}
			if (/^\s*SYSTEMRESOURCE/i || /^\s*RESOURCE/i) {
				$has_resource=1;
				next;
			}
			if (/^\s*SOURCEPATH\s+(.*)\r?/) {
				$sourcedir=$dir . $1;
				$sourcedir .= "\\" unless ( $sourcedir =~ /.*\\$/ );
				next;
			}
			if (/^\s*SOURCE\s+(.*)\r?/) {
				my @s=split(/\s+/, $1);
				foreach my $s (@s) {
					push(@sourcefiles, $sourcedir . $s);
				}
				next;
			}
			if (/^\s*(SYSTEM|USER)INCLUDE\s+(.*)\r?/) {
				#print $_;
				my @s=split(/\s+/, $2);
				foreach my $s (@s) {
					#print "inc $dir$s\n";
					push(@includedirs, $dir . $s);
				}
				next;
			}
		}  else {
			if (/^\s* END/) { $in_bitmap=0; next; }
		}
		if (/^\s*START\s*BITMAP/i) {
			$in_bitmap=1;
			next;
		}
		next unless /#include/;
		/#include\s*[<"]\s*(.*)\s*[>"]/;
		my ($low_files, $low_type, $low_res, $low_sourcefiles, $low_sourcedir,
			$low_includedirs, $low_in_bitmap) 
			= getdepends($dir . $1, $file, $sourcedir, $in_bitmap);
		if ( defined($low_type) ) { $type=$low_type; }
		if ($low_res) { $has_resource=1; }
		$sourcedir=$low_sourcedir;
		$in_bitmap=$low_in_bitmap;
		push @ret, @{$low_files};
		push @sourcefiles, @{$low_sourcefiles};
		push @includedirs, @{$low_includedirs};
	}
	close($in);
	return (\@ret, $type, $has_resource, \@sourcefiles, $sourcedir, \@includedirs,
		$in_bitmap);
}

sub haschanged {
	my $proj=shift;
	my $depends=shift;
	my $makefile=shift;

	$makefile=makefilename($proj) unless($makefile);
	#print "MAKEFILE $makefile \n";
	my @depends=@{$depends};
	push (@depends, mmp($proj));
	my @s=stat($makefile);
	my $mftime=$s[9];
	foreach my $d (@depends) {
		@s=stat($d);
		return 1 if ( $s[9] > $mftime);
	}
	return 0;
}

my %incdepends=();

sub incdepends($$) {
	my $file=shift;
	#print "$file\n";

	my $deps=$incdepends{$file};
	if (defined($deps)) {
		return $deps;
	}
	my $incdirs=shift;
	my $fh=$file;

	my @deps=();

	open($fh, "<$file") || die "cannot open $file";
	while(<$fh>) {
		if (/^\s*#include\s*<\s*([^" \t]+)\s*>/) {
			push(@deps, $1);
			next;
		}
		next unless (/^\s*#include\s*"\s*([^" \t]+)\s*"/);
		my $h=$1;
		#print "header: $h\n";
		my $cand="";
		foreach my $dir (@{$incdirs}) {
			$dir .= "\\" unless ( $dir =~ /\\$/ );
			$cand = $dir . $h;
			#print "$cand\n";
			if (-f $cand) {
				last;
			}
			$cand="";
		}
		unless ($cand eq "") {
			push(@deps, $cand);
			my $low_deps=incdepends($cand, $incdirs);
			push(@deps, @{$low_deps});
		}
	}
	close($fh);
	@deps=sort { $a cmp $b } @deps;
	$incdepends{$file}=\@deps;
	return \@deps;
}

sub depchanged($$$) {
	my $mmp=shift;
	print "checking dependancies in $mmp...";
	my $sourcefiles=shift;
	my $includedirs=shift;

	my $depstring="";

	foreach my $source (sort @{$sourcefiles}) {
		$depstring .= $source . ":";
		my $deps=incdepends($source, $includedirs);
		foreach my $d (@{$deps}) {
			$depstring .= " " . $d;
		}
	}

	my $depfile=$ENV{'EPOCROOT'};
	$depfile=~s/.*\\([^\\]*)\\$/$1/;
	mkdir $depfile, 0777;
	$mmp=~s/\./_/g;
	$mmp .= ".dep";
	$depfile .= "\\" . $mmp;
	my $ret=1;
	if (-f $depfile ) {
		my $olddepstring="";
		open(DEP, "<$depfile") || die "cannot open $depfile";
		while(<DEP>) {
			$olddepstring .= $_;
		}
		close(DEP);
		if ($olddepstring eq $depstring) {
			$ret=0;
		}
	}
	open(DEP, ">$depfile") || die "cannot open $depfile";
	print DEP $depstring;
	close(DEP);
	if ($ret) { print " changed."; }
	else { print " no changes."; }
	print "\n";
	return $ret;
}

1;
