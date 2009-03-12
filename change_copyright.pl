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

use strict;

my $notice = $ARGV[0];
my $source = $ARGV[1];

open(NOTICE, "<$notice") || die "cannot open $notice for reading";
my @notice_lines = <NOTICE>;
close(NOTICE);

open(SOURCE, "<$source") || die "cannot open $source for reading";
my @source_lines = <SOURCE>;
close(SOURCE);

my $comment_prefix = "//";
$comment_prefix = "#" if ($source =~ /\.sh$/i);
$comment_prefix = "#" if ($source =~ /\.pl$/i);
$comment_prefix = '@REM' if ($source =~ /\.bat$/i);
my $comment_prefix_len = length($comment_prefix);

my $line_end = "\n";
$line_end = "\r\n" if ($notice_lines[0] =~ /\r/);

@notice_lines = map { s/\r?\n//; $comment_prefix . " " . $_ } @notice_lines;
@notice_lines = map { s/\s*$//; $_ } @notice_lines;
my $notice_text = join($line_end, @notice_lines);

@source_lines = map { s/\r?\n//; $_ } @source_lines;
my $source_text = "";
my $initial = 1;
my $in_comment = 0;
my $trailing = 1;
foreach my $line (@source_lines) {
  if ($initial) {
    next if ($line =~ /^\s*$/);
    if ($comment_prefix eq "//") {
      $in_comment = 1 if ($line =~ m!^\s*/\*!);
      if ($line =~ m!\*/\s*$!) {
        $in_comment = 0;
        next;
      }
      next if ($in_comment);
      if ($line =~ /include.*break\.h.*/) {
        $source_text .= $line . $line_end;
        next;
      }
    }
    my $temp = $line;
    $temp =~ s/^\s*//;
    next if (substr($temp, 0, $comment_prefix_len) eq $comment_prefix);
    $initial = 0;
  }
  if ($trailing) {
    next if ($line =~ /^\s*$/);
    $trailing = 0;
  }
  $source_text .= $line . $line_end;
}

$source_text = $notice_text . $line_end x 2 . $source_text;

open(TMP, ">${source}.tmp") || die "cannot open ${source}.tmp for writing";
print TMP $source_text;
close(TMP);

system("mv ${source}.tmp $source");
