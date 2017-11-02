#!/usr/bin/perl -w

$html = 0;
$skip = 0;

while (@ARGV) {
	$plugin = shift(@ARGV)."";

	if ($plugin eq "--html") {
		$html = 1;
		print "<table cellpadding=\"4\" cellspacing=\"0\" border=\"1\">\n";
		print "<tr><th>UID</th><th>Name</th><th>PIII MHz</th><th>Cycles /<br>sample</th></tr>\n";
		next;
	}

	@data = `analyseplugin $plugin`;

	$port = 0;
	$ins = 0;
	$id = 0;
	$seen = 0;

	for (@data) {
		if (/Plugin Name: "(.*?)"/) {
			if ($seen) {
				#&run($port, $ins, $id, $title);
				&run;
			}
			if (/Debug/) {
				$skip = 1;
			}
			$title = $1;
			$seen = 1;
			$ins = 0;
			$id = 0;
			$port = 0;
		}
		if (/Plugin Label: "(.*?)"/) {
			$label = $1;
		}
		if (/Plugin Unique ID: (\d+)/) {
			$id = $1;
		}
		if (/"(.*?)" input, control, ([0-9.e-]+).*? to ([0-9.e-]+)/i) {
			#$name[$port] = $1;
			$min[$port] = $2;
			if ($min[$port] eq "...") {
				$min[$port] = 0.0;
			}
			$max[$port] = $3;
			if ($max[$port] eq "...") {
				$max[$port] = $min[$port];
			}
			$port++;
		}
		if (/input, audio/) {
			$ins++;
		}
	}

	if ($skip) {
		$skip = 0;
		next;
	}

	&run;
}

#&run;
#&run($port, $ins, $id, $title);

if ($html) {
	print "</table>\n";
}

sub run {
	#local ($port, $ins, $id, $title) = @_;

	my $afile = "XXX";
	if ($ins == 1) {
		$afile = "happyness-ext-m.wav";
	} elsif ($ins == 2) {
		$afile = "happyness-ext-s.wav";
	} else {
		print STDERR "No valid input file for $id\n";
		#next;
	}

	if ($html) {
		printf("<tr><td>$id</td><td><a href=\"/ladspa-swh/docs/ladspa-swh.html#id$id\">$title</a></td>");
	} else {
		printf("%-48s ", $title);
	}

	my $base = "";
	for $i (0..$port-1) {
		$base .= " ".($min[$i]+$max[$i])/2;
	}

	$cycles = 9999999999999;
	$pers = 99999999;
	for (1..5) {
		$cmd = "applyplugin ../testdata/$afile /dev/null $plugin $label$base 2>&1";
		$out = `$cmd`;
		last if ($? != 0);
                chomp $out;
                $out =~ /Plugin cycles: (\d+) \((\d+)\//;
		if (!$1) {
			print STDERR "$? $out\n$cmd\n";
			$err = 1;
		}
                if ($1 < $cycles) {
                        $cycles = $1;
                }
                if ($2 < $pers) {
                        $pers = $2;
                }
	}
	if ($err || $?) {
		if ($html) {
			print "<td>???</td><td>???</td>\n\n";
		} else {
			print "???\n$cmd\n";
		}
		$err = 0;
		return;
	}
	$cycles /= 8000000.0;
	if ($html) {
		printf "<td align=\"right\">%.1f</td><td align=\"right\">$pers</td></tr>\n", $cycles;
	} else {
		printf "%3.1f\t%d\n", $cycles, $pers;
	}
}
