#!/usr/bin/perl

use strict;


my @repl_list = (
"FUDGEFACTOR",
"FEATURESIZE");


my $class_repl = "circuit_param_t::";
my $var_prefix = "cparam_";


my $file = shift;
my $back_file = $file . ".back";
system("cp " . $file . "  " . $back_file);

open (WFILE, " > tmp");
open (RFILE, $file) || die "Can't open file";

while (<RFILE>) {
	my $line = $_;

	foreach (@repl_list) {
		my $pattern = $_;
		my $repl = $class_repl . $var_prefix . $pattern;

		if ($line =~ /(\W)$pattern(\W)/) {
			$line =~ s/$pattern/$repl/g;
		}
	}
    
	print WFILE $line;
}


close WFILE;
close RFILE;

system("mv tmp " . $file);

