#!/usr/bin/perl

use strict;

my $file = shift;

open (RFILE, $file) || die "Can't open the file\n";

print "my \@repl_list = (\n";
while (<RFILE>) {
	my $line = $_;
	if ($line =~ /^(\w+)\'\s+undeclared/) {
		my $pattern = $1;
		print "\"", $pattern , "\",\n";
	}
}
