#!/usr/bin/perl -w

print <<EOB;
<?xml version='1.0' encoding='ISO-8859-1'?>
<!DOCTYPE rdf:RDF [
        <!ENTITY rdf 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>
        <!ENTITY rdfs 'http://www.w3.org/2000/01/rdf-schema#'>
        <!ENTITY dc 'http://purl.org/dc/elements/1.1/'>
        <!ENTITY ladspa 'http://ladspa.org/ontology#'>
]>
<rdf:RDF xmlns:rdf="&rdf;"
         xmlns:rdfs="&rdfs;"
         xmlns:dc="&dc;"
         xmlns:ladspa="&ladspa;">

EOB

$ocnt = 0;

while (<>) {
	if (m(<meta\s+name="maker"\s+value="(.*?)")i) {
		$pcreator = $1;
	}
	if (m(<plugin\s+label="(.*?)"\s+id="(.*?)"( class="(.*?)")?>)) {
		#$plabel = $1;
		$pid = $2;
		if ($3) {
			@classes = split(",", $4);
			$class = shift @classes;
		} else {
			$class = "Plugin";
		}
		%defaults = ();
		%min = ();
		%max = ();
		print "  <ladspa:$class rdf:about=\"&ladspa;$pid\">\n";
		while ($extra_class = shift @classes) {
			print "    <rdf:type rdf:resource=\"&ladspa;$extra_class\"/>\n";
		}
		if ($pcreator) {
			print "    <dc:creator>$pcreator</dc:creator>\n";
		}
		$ocnt = 0;
	}
	if ($ocnt == 0 && m(<name>(.*?)</name>)) {
		print "    <dc:title>$1</dc:title>\n";
	}
	if (m(<port label="(.*?)" dir="(.*?)" type="(.*?)"(\s+hint="(.*?)")?>)) {
		$ocnt++;
		next if ($3 eq "audio");
		print "    <ladspa:hasPort>\n";
		print "      <ladspa:\u$2\u$3Port rdf:about=\"&ladspa;$pid.$ocnt\" ladspa:hasLabel=\"$1\" />\n";
		print "    </ladspa:hasPort>\n";
		$hints = $5;
		if ($hints && $hints =~ m((default_[a-z0-9]+))) {
			$defaults{$ocnt} = $hints;
		}
	}
	if (m(<range\s+min="(.*?)"\s+max="(.*?)")) {
		$min{$ocnt} = $1;
		$max{$ocnt} = $2;
	}
	if (m(</plugin)) {
		if (length(%defaults) > 1) {
			print "    <ladspa:hasSetting>\n";
			print "      <ladspa:Default>\n";
			for $i (sort keys %defaults) {
				$dp = $defaults{$i} =~ m((default_[a-z0-9]+));
				if ($dp) {
					$hint = $1;
				} else {
					$hint = "";
				}
				if ($hint eq "default_0") {
					$val = 0.0;
				} elsif ($hint eq "default_1") {
					$val = 1.0;
				} elsif ($hint eq "default_440") {
					$val = 440.0;
				} elsif ($hint eq "default_minimum") {
					$val = $min{$i};
				} elsif ($hint eq "default_low") {
					$val = $min{$i} * 0.75 + $max{$i} * 0.25;
				} elsif ($hint eq "default_middle") {
					$val = ( $min{$i} + $max{$i} ) / 2.0;
				} elsif ($hint eq "default_high") {
					$val = $min{$i} * 0.25 + $max{$i} * 0.75;
				} elsif ($hint eq "default_maximum") {
					$val = $max{$i};
				} else {
					print STDERR "ERROR $defaults{$i}\n";
					$val = "ERROR";
				}
				print "        <ladspa:hasPortValue>\n";
				print "          <ladspa:PortValue rdf:value=\"$val\">\n";
				print "            <ladspa:forPort rdf:resource=\"&ladspa;$pid.$i\" />\n";
				print "          </ladspa:PortValue>\n";
				print "        </ladspa:hasPortValue>\n";
			}
			print "      </ladspa:Default>\n";
			print "    </ladspa:hasSetting>\n";
		}
		print "  </ladspa:$class>\n\n";
	}
}

print "</rdf:RDF>\n";
