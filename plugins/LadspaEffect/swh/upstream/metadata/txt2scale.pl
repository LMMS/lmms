#!/usr/bin/perl -w

print <<EOB;
<?xml version='1.0' encoding='UTF-8'?>
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

$first = 1;
$port = "";

while (<>) {
	chomp;
	if (/^([0-9]+\.[0-9]+)/) {
		$port = $1;
		if (!$first) {
			&endScale;
		}
		$first = 0;
		print <<EOB;
<ladspa:InputControlPort rdf:about="&ladspa;$port">
  <ladspa:hasScale>
    <ladspa:Scale>
EOB
	}

	if (/^\s*([0-9]+)\s+(.*)/) {
		print <<EOB;
      <ladspa:hasPoint>
        <ladspa:Point rdf:value="$1" ladspa:hasLabel="$2" />
      </ladspa:hasPoint>
EOB
	}
}

if (!$first) {
	&endScale;
}

print <<EOB;

</rdf:RDF>
EOB

sub endScale {
	print <<EOB;
    </ladspa:Scale>
  </ladspa:hasScale>
</ladspa:InputControlPort>
EOB
}
