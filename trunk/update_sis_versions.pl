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

$version=shift @ARGV;
$update_all_versions=shift @ARGV;

open(INV, "<$version") || die "cannot open $version";
while(<INV>) {
	chop;
	($t, $v)=split(/\s*=\s*/);
	$versions{lc($t)}=$v;
}
close(INV);
die "no minor version specified" if ($versions{'minor'} eq "");
die "no major version specified" if ($versions{'major'} eq "");

my $SDK=$ENV{'EPOCROOT'};
$SDK=~s/.*\\([^\\]*)\\$/$1/;

$version=~s/version\.txt/context_uids/;

#my $v21=0;
my $epoc_root = $ENV{'EPOCROOT'};
if ($epoc_root eq "") { die "error: EPOCROOT not set\n";}
$epoc_root .= "epoc32";
#if ($ENV{'EPOCROOT'} =~ /Series60_v21/i) { $v21=1; }
if ($ENV{'EPOCROOT'} =~ /3rd/i) { $v3=1; }
foreach $h ( ".rh", ".h") {
	$file=$version . $h;
	open(INV, "<$file") || die "cannot open version file";
	while(<INV>) {
		if (/^const/) {
			chop;
			/const tuid kuid([^ ]+) = {\s*([^\s]+)\s*}/i;
			($name, $uid) = ($1, $2);
			unless ($defs{$uid} eq "") {
				$uids{lc($name)}=$defs{$uid};
			} else {
				$uids{lc($name)}=$uid;
			}
			#print lc($name), " ", $uids{lc($name)}, "\n";
		} elsif (/^#\s*define/) {
			/#\s*define\s+([^ \t]+)\s+([x0-9A-F]+)/i;
			$defs{$1}=$2;
		}
	}
	close(INV);
}

open(BLDINF, "<bld.inf") || die "cannot open bld.inf";
#print "DEBUG" .cwd;
#print "\n";
while(<BLDINF>) {
	$seen_procent=$seen_colon=0;
	next if (m!^//!);
	next unless (/mmp$/i);
	next if (/mdaaudioinputstreamplugin/);
	chop; s///;
	$name="";
	/.*\\([^\\.]+).mmp/i;
	$name=lc($1);
	s/mmp/pkg/i;
	$file=$_;

	if ($v3) {
		my $gv3=$file;
		$gv3=~s/group/gv3/i;
		$gv3=~s/gv2/gv3/i;
		$file=$gv3 if ( -f $gv3); 
	}
	my $normalized=$name;
        $normalized=~s/_//g;
	die "no uid for $normalized" if ($uids{$normalized} eq "");

	open(IN, "<$file") || die "cannot open input $file";
	$file=~s/.pkg//i;
	$file .= "_" . $SDK . ".pkg";

	$prev_minor=$prev_major=0;
	if (open(OUT, "<$file")) {
		while(<OUT>) {
			if (/^#/) {
				/(.*, *)([0-9]+), *([0-9]+)( *,[0-9]+\s*)$/;
				($beg,$prev_major,$prev_minor,$rest)=($1,$2,$3,$4);
			}
		}
		close(OUT);
		#print "prev version $prev_major $prev_minor\n";
	} else {
		print "NO old build pkg file\n";
	}

	open(OUT, ">${file}") || die "cannot open output $file";

	while(<IN>) {
		s/"\\.+?Epoc32/"$epoc_root/i;	
		s/thumb/gcce/gi if ($v3);
		s/system.programs/sys\\bin/gi if ($v3);
		s/system.libs/sys\\bin/gi if ($v3);
		if (/^#/) {
			/(.*, *)([0-9]+), *([0-9]+)( *,[0-9]+\s*)$/;
			($beg,$major,$minor,$rest)=($1,$2,$3,$4);
			$major=$prev_major;
			$minor=$prev_minor;
			if ($update_all_versions || $prev_major==0) {
				$major=$versions{'major'};
			}
			if ($update_all_versions || $prev_minor==0) {
				$minor=$versions{'minor'};
			}
			#print "new version $major $minor\n";

			$beg=~s/(.*}, *\().*/$1/;
			$beg .= $uids{$normalized} . "),";
			
			print OUT $beg, $major, ",", $minor, $rest;
		} else {
			$seen_procent=1 if (/^[%]/);
			$seen_colon=1 if (/^[:]/);
			if ($v3 && /0x101F6F88/) {	
				print OUT '%{"Jaiku Ltd", "Jaiku Oy"}', "\n" unless($seen_procent);
				print OUT ':"Jaiku"', "\n" unless($seen_colon);
				print OUT '(0x101F7961), 0, 0, 0, {"Series60ProductID", "Series60ProductID"}', "\n";
			} else {
				print OUT $_;
			}
		}
	}

	close(IN);
	close(OUT);

	#system("move ${file}.tmp $file");
}

close(BLDINF);


@pkgs=<*.pkg>;
foreach $file (@pkgs) {
	open(OUT, ">${file}.tmp") || die "cannot open output";
	open(IN, "<$file") || die "cannot open input";

	while(<IN>) {
		if (/mdaaudioinputstreamplugin/) {
			print OUT $_;
			next;
		}
		if (/^@/) {
			$name="";
			/"([^"]*)"/;
			$name=lc($1);
			if ($name=~/\\/) {
				$name=~s/^.*\\([^\\]+)$/$1/;
			}
			$name=~s/_build//i;
			$name=~s/\.sis//i;
			$name=~s/_dbg//i;
			$name=~s/_s60[^.]*//i;
			$name=~s/_series60[^.]*//i;
			my $normalized=$name;
			$normalized=~s/_//g;
			die "no uid for $normalized" if ($uids{$normalized} eq "");
			$uid=$uids{$normalized};
			s/\(0x[0-9A-F]+\)/($uid)/i;
		}
		s/"\\.+?Epoc32/"$epoc_root/i;
#		if ($v21) {
#			s/Series60_v20/Series60_v21/ig;
#		} else {
#			s/Series60_v21/Series60_v20/ig;
#		}
		s/_build//;
		s/_s60[^.]*//i;
		s/_series60[^.]*//i;
		if (/^@/) {
			s/.SIS/_${SDK}.SIS/i;
		}

		print OUT $_;
	}

	close(IN);
	close(OUT);

	system("move ${file}.tmp $file");
}
