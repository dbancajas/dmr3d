#!/s/std/bin/perl

use strict;

my $file = shift @ARGV;

open (RFILE, $file);

while (<RFILE>) {
	my $line = $_;
	if ($line =~ /CONF_V_(\w+)\s+\((\w+),\s+\"([\w|\s]+)\",\s*(\w+)/) {
		print "$1  config entry = $2 val = $4 Intermediate = $3\n";
	}
}
