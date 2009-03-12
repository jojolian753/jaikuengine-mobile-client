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

use Win32::OLE;
use strict;

my $platform="WINSCW";

my $CW=Win32::OLE->GetActiveObject("CodeWarrior.CodeWarriorApp");
$CW=Win32::OLE->new("CodeWarrior.CodeWarriorApp") unless ($CW);
die "Cannot get or create codewarrior object" unless ($CW);
$CW->AllowUserInteraction(0);

use lib '..';
use lib '../..';
require 'building.pl';

my %cw_projects;
sub reload {
	my $project=shift;
	my $depends=shift;
	my $dont_add=shift;
	my $changed_projects=shift;
	print "Creating $project ...\n";
	my $xmlfile=xmlfilename($project, $platform);

	my $mcpfile=$xmlfile;
	$mcpfile=~s/xml/mcp/i;
	my $changed=haschanged($project, $depends, $xmlfile);
	my @s_xml=stat($xmlfile);
	my @s_mcp=stat($mcpfile);
	my $xml_is_newer=0;
	$xml_is_newer=1 if ($s_xml[9] > $s_mcp[9]);

	#print $xmlfile, "\n";
	for (my $i=0; $i<$CW->Projects()->Count(); $i++) {
		my $p=$CW->Projects()->Item($i);
		if ( lc($p->Name()) eq lc($project . ".mcp") ) {
			return 0 unless($changed || $xml_is_newer);
		}
	}
	$xml_is_newer=1;

	my %linkers;
	%linkers=();
	print "    Removing from dependent projects...\n";
	for (my $i=0; $i<$CW->Projects()->Count(); $i++) {
		my $p=$CW->Projects()->Item($i);
		next if ( lc($p->Name()) eq lc($project . ".mcp") );
		print "        Checking " . $p->Name();
		my $files=$p->FindFileByName($mcpfile);
		goto next_project unless($files->Count());
		for (my $t_i=0; $t_i < $p->Targets()->Count(); $t_i++) {
			my $target=$p->Targets()->Item($t_i);
			for (my $s_i=0; $s_i<$target->GetSubProjects()->Count(); $s_i++) {
				my $subproject=$target->GetSubProjects()->Item($s_i);
				next unless (lc($subproject->Name()) eq lc($project) . ".mcp");
				print ".";
				$subproject->Close();
				$linkers{$p->Name()}=$p;
				$changed_projects->{$project}=1 if ($changed_projects);
			}
		}
		for (my $f_i=0; $f_i < $files->Count(); $f_i++) {
			my $file=$files->Item($f_i);
			$p->RemoveFile($file);
			print ".";
		}
		next_project:
		print "\n";
	}

	my @to_remove;
	for (my $i=0; $i<$CW->Projects()->Count(); $i++) {
		my $p=$CW->Projects()->Item($i);
		if ( lc($p->Name()) eq lc($project . ".mcp") ) {
			print "    Unloading...";
			$p->Close();
			print "\n";
			push @to_remove, $p;
			next;
		}
	}
	foreach my $p (@to_remove) {
		$p->Close();
		$CW->Projects()->Remove($p);
		$p=undef;
	}
	@to_remove=();

	if ($changed) {
		print "    Recreating import file\n";
		print " abld makefile cw_ide $project...";
		open (TOUCH, ">>" . mmp($project));
		close(TOUCH);
		system("abld makefile cw_ide $project");
	}
	#print $xmlfile, "\n";

	if($changed || $xml_is_newer) {
		my $cwd=`echo %CD%`;
		$cwd=~s/[\r\n]//g;
		print "    Loading...";
		my $pkg=$cwd . "\\jaiku_dbg.pkg";
		my $p=$cw_projects{$project}=$CW->ImportProject($xmlfile, $mcpfile, 1);
		die "could not import $xmlfile" unless($p);
		if (-f $pkg) {
			for (my $t_i=0; $t_i < $p->Targets()->Count(); $t_i++) {
				my $target=$p->Targets()->Item($t_i);
				my $output=lc($target->GetTargetOutput()->FileSpec()->Name());
				if ( ($target->Name() =~ /gcce udeb/i || $target->Name() =~ /armv5 udeb/i)
					&& $output=~/exe$/i) {
					print " adding pkg...";
					$target->AddFile($pkg, "");
				}
			}
		}
		print "\n";
	}

	return 1 if ($dont_add);
	print "    Restoring in dependent projects\n";
	foreach my $p (values %linkers) {
		print "        " . $p->Name();
		for (my $t_i=0; $t_i < $p->Targets()->Count(); $t_i++) {
			my $target=$p->Targets()->Item($t_i);
			my $added=$target->AddFile($mcpfile, "");
			for (my $s_i=0; $s_i<$target->GetSubProjects()->Count(); $s_i++) {
				my $subproject=$target->GetSubProjects()->Item($s_i);
				next unless (lc($subproject->Name()) eq lc($project) . ".mcp");
				for (my $spt_i=0; $spt_i<$subproject->Targets()->Count(); $spt_i++) {
					my $sptarget=$subproject->Targets()->Item($spt_i);
					if ($sptarget->Name() eq $target->Name()) {
						$target->BuildAgainstSubProjectTarget($sptarget, 1);
						print ".";
						last;
					}
				}
			}
		}
		print "\n";
	}
	return 1;
}

sub outputs {
	my $outputs;
	foreach (my $p_i=0; $p_i < $CW->Projects()->Count(); $p_i++) {
		my $p=$CW->Projects()->Item($p_i);
		my $project=lc($p->Name());
		$project=~s/\.mcp//i;
		for (my $t_i=0; $t_i < $p->Targets()->Count(); $t_i++) {
			my $target=$p->Targets()->Item($t_i);
			my $output=lc($target->GetTargetOutput()->FileSpec()->Name());
			$output=~s/\..*//;
			$outputs->{ $target->Name() }->{ $output } =
				$p;

			#print "TARGET: " . $target->Name() . "\n";
			#print "NAME: " . $output . "\n";
			#print "PROJECT: " . $project . "\n";
		}
	}
	return $outputs;
}

sub browser {
	my $outputs;
    my $projects=$CW->Projects();
    my $pc=$projects->Count();
	foreach (my $p_i=0; $p_i < $pc; $p_i++) {
		my $p=$projects->Item($p_i);
        my $targets=$p->Targets();
        my $tc=$targets->Count();
		for (my $t_i=0; $t_i < $tc; $t_i++) {
			my $target=$targets->Item($t_i);
            $target->SetNamedPanelDataField("Build Extras", "BrowserGenerator", 2);
		}
	}
}

my %p_by_name;
my $d_projects;

sub dependencies {
	my $project=shift;
	my $outputs=shift;
	print "Dependencies for $project...\n";
	if ($project=~/expat/i) {
		print "    Skipping C-based project\n";
		return;
	}
	my $cw_project;
    unless($d_projects) {
        $d_projects=$CW->Projects();
    }
    unless (%p_by_name) {
        my $c=$d_projects->Count();
        foreach (my $p_i=0; $p_i < $c; $p_i++) {
            my $p=$d_projects->Item($p_i);
            $p_by_name{lc($p->Name())}=$p;
        }
    }
    $cw_project=$p_by_name{lc($project . ".mcp")};
	die "lost reference to project for $project" unless($cw_project);
	my %deleted;
	for (my $t_i=0; $t_i < $cw_project->Targets()->Count(); $t_i++) {
		my $target=$cw_project->Targets()->Item($t_i);
		print "    Target " . $target->Name() . "\n";
        my $tfc=$target->TargetFileCollection();
        my $count=$tfc->Count();
		for (my $f_i=0; $f_i<$count; $f_i++) {
			my $file=$tfc->Item($f_i);
			my $name=$file->Name();
			if ($name=~/(.*)\.(lib|dso)/i) {
				print "        Library " . $name . "...";
				my $lib=lc($1);
				my $project=$outputs->{ $target->Name() }->{ $lib };
				unless ($project) {
					print "system\n";
					next;
				}
				print "ours\n";
				my $mcp=$project->FileSpec()->FullPath();
				my $files=$cw_project->FindFileByName($mcp);
				unless ($deleted{$mcp}) {
					for (my $pf_i=0; $pf_i< $files->Count(); $pf_i++) {
						my $pf=$files->Item($pf_i);
						#print "FILE " . $pf->Name() . "\n";
						$cw_project->RemoveFile($pf);
					}
				}
				$deleted{$mcp}=1;
				my $added=$target->AddFile($mcp, "");
				die "cannot add project $mcp" unless($added);
			}
		}
		#if ($target->Name() =~ /gcce/i || $target->Name() =~ /armv5/i) {
			my @indirects;
			if ( lc($cw_project->Name()) eq "jaikusettings.mcp" || lc($cw_project->Name()) eq "contextcontacts.mcp" ) {
				@indirects=qw(blackboardfactory contextsensorfactory blackboardserver contextnotify);
			}
			if ( lc($cw_project->Name()) eq "jaikusettings.mcp" ) {
				push(@indirects, "contextserver");
				#push(@indirects, "contextmediaplugin");
			}
			if ( lc($cw_project->Name()) eq "contextcontacts.mcp") {
				push(@indirects, "jaikusettings");
			}
			foreach my $project (@indirects) {
				my $mcp=xmlfilename($project, $platform);
				$mcp=~s/xml/mcp/i;
				my $files=$cw_project->FindFileByName($mcp);
				for (my $pf_i=0; $pf_i< $files->Count(); $pf_i++) {
					my $pf=$files->Item($pf_i);
					#print "FILE " . $pf->Name() . "\n";
					$cw_project->RemoveFile($pf);
				}
				my $added=$target->AddFile($mcp, "");
				die "cannot add project $mcp" unless($added);
			}
		#}
		print "   Subprojects\n";
		for (my $s_i=0; $s_i<$target->GetSubProjects()->Count(); $s_i++) {
			my $subproject=$target->GetSubProjects()->Item($s_i);
			next unless($subproject->Name());
			print "        " . $subproject->Name() . "...";
			next unless($subproject->Targets());
			for (my $spt_i=0; $spt_i<$subproject->Targets()->Count(); $spt_i++) {
				my $sptarget=$subproject->Targets()->Item($spt_i);
				die "target not found $spt_i" unless($sptarget);
				if ($sptarget->Name() eq $target->Name()) {
					$target->BuildAgainstSubProjectTarget($sptarget, 1);
					print "matched\n";
					last;
				}
			}
		}
	}
}

$|=1;

#exit 0;

if ($ARGV[0]) {
	my $project=$ARGV[0];
	if ($project ne "depends") {
		my ($depends, $type, $has_resource, $sourcefiles, $dummy, $includedirs, $dummy2)=
			getdepends(mmp($project), mmp($project), "", 0);
		if (reload($ARGV[0], $depends)) {
			#my $outputs=outputs;
			#dependencies($ARGV[0], $outputs);
		}
			my $outputs=outputs;
			dependencies($ARGV[0], $outputs);
	} else {
		my $outputs=outputs;
		for (my $i=0; $i<$CW->Projects()->Count(); $i++) {
			my $p=$CW->Projects()->Item($i);
			my $project=$p->Name();
			$project=~s/\.mcp//i;
			dependencies($project, $outputs);
		}
	}
    browser;
} else {
	my ($depends, $type, $has_resource, $sourcefiles, $dummy, $includedirs, $dummy2);
	my %changed;
	foreach my $project (mmps()) {
		if ($project=~/expat/i) {
			print "SKIPPING expat, since CW doesn't handle C correctly\n";
			next;
		}
		($depends, $type, $has_resource, $sourcefiles, $dummy, $includedirs, $dummy2)=
			getdepends(mmp($project), mmp($project), "", 0);
		$changed{$project}= (reload($project, $depends, 1, \%changed) || $changed{$project});
	}
	my $outputs=outputs;
	foreach my $project (mmps()) {
		dependencies($project, $outputs) if ($changed{$project});
	}
    browser;
}

$CW->AllowUserInteraction(1);
