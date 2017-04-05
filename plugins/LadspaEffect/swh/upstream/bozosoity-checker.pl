#!/usr/bin/perl -w

%used = ();

while ($file = shift) {
	open(FH, $file);
	if (!($file =~ /_(\d+)\.xml/)) {
		die "'$file' doesn't look like it contains a LADSPA ID";
	}
	$fnid = $1;
	#print "looking for $fnid\n";

	$found = 0;
	while (<FH>) {
		if (/id="(\d+)"/) {
			$id = $1;
			if ($used{$id}) {
				print "*** Warning: duplicate ID ($id) found in $file and ".$used{$id}."\n";
			} else {
				$used{$id} = $file;
			}
			if ($id < 1000) {
				print "*** Warning: non distributable ID ($id) found in $file\n";
			} elsif ($id < $fnid) {
				print "*** Warning: ID $id in XML less than hinted in filename ($fnid)\n";
			}
		}
		if (/id="$fnid"/) {
			$found = 1;
		}
	}
	if (!$found) {
		print "*** Warning: no matching ID found in $file\n";
	}
}
