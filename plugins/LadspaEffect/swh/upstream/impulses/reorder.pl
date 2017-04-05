#!/usr/bin/perl

$n = 1;
while (<>) {
	chomp;
	$ns = sprintf("%02d", $n);
	$fr = $_;
	s/^../$ns/;
	print "mv $fr $_\n";
	$n++;
}
