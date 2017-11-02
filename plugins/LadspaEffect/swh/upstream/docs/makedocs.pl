#!/usr/bin/perl -w

use XML::Parser;

my $parser = new XML::Parser(ErrorContext => 2);

$parser->setHandlers(Start => \&start,
		     End   => \&end,
		     Char  => \&char);

&loadstats;
&preamble;
while ($file = shift) {
	$parser->parsefile($file);
}
&postamble;

sub start {
	my ($p, $el, %attr) = @_;

	unshift(@elements, $el);
	if ($el eq "plugin") {
		$pluginLabel = $attr{label};
		$pluginLabelLabel = $attr{label};
		$pluginLabel =~ s/%/\\%/g;
		$pluginLabel =~ s/_/\\_/g;
		$pluginID = $attr{id};
		$title = "";
	}
}

sub char {
	my ($p, $str) = @_;

	$str =~ s/%/\\%/g;
	$str =~ s/_/\\_/g;
	if ($elements[0] eq "name" && $elements[1] eq "plugin") {
	print '\subsection{'.$str.' ('.$pluginLabel.', '.$pluginID.")\\label{${pluginLabelLabel}}\\label{id${pluginID}}}\n";
		if ($cycles{$pluginID}) {
			print "CPU usage: ".$cycles{$pluginID}." cycles/sample\n\n";
		}
	}
	$title = '\subsubsection*{'.$str."}\n" if $elements[0] eq "name" && $elements[1] eq "port";
	if ($elements[0] eq "p") {
		print $title.$str;
		$title = "";
	}
}

sub end {
	my ($p, $el) = @_;

	shift(@elements);
}

sub loadstats {
	open(DATA, "../timetest/timetest.results");
	while(<DATA>) {
		@line = split(" ");
		$cycles{$line[0]} = $line[2];
	}
}

sub preamble {
	$date = `date -I`;
	chomp $date;
	print <<EOB;
\\documentclass[11pt]{article}
\\usepackage{times}
\\usepackage[T1]{fontenc}
\\usepackage[latin1]{inputenc}
\\usepackage{a4}
\\usepackage{url}
\\pagestyle{plain}
\\parindent=0pt

\\setcounter{secnumdepth}{2}
\\setcounter{tocdepth}{2}
\\makeatletter

\\begin{document}

\\title{Steve Harris' LADSPA Plugin Docs}
\\date{$date}
\\author{steve\@plugin.org.uk}

\\maketitle

\\tableofcontents{}

\\parskip=\\medskipamount

\\section{Preamble}

\\subsection{What plugins?}
This is the documentation for some plugins that I have written for the \\emph{Linux Audio Developers Simple Plugin Architecture}. It is a nice audio plugin architecture with a very easy learning curve.

\\subsection{Where can I get them}
From the website at \\url{http://plugin.org.uk/}.

\\section{The plugins}
EOB
}

sub postamble {
	print <<EOB
\\section{Licensing}
All this code is available under the GNU Public Licence, see the file {}''COPYING{}'' included with the source for more detials.

\\end{document}
EOB
}

