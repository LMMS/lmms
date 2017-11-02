#!/usr/bin/perl -w

$style = <<EOB;
<style>
  BODY {
    font-family: sans-serif;
    font-size: 10pt;
    color: black;
    background: white;
  }

  H1 {
    font-size: 16pt;
  }

  H2 {
    padding-top: 10pt;
    font-size: 14pt;
  }

  H3 {
    padding-top: 10pt;
    font-size: 12pt;
  }

  P {
    font-size: 10pt;
  }

  TD {
    font-size: 10pt;
  }

  A {
    text-decoration: none;
  }
</style>
EOB

while(<>) {
	s/<\/title>/$&\n$style/i;
	print;
}
