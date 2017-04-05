#!/usr/bin/perl

$scale = shift;

while(<>) {
	s/([0-9.]+f)/&scale($1)/gxe;
	print;
}

sub scale {
	local ($val) = @_;

	return sprintf("%.13f", $val / $scale);
}
