#!/usr/bin/perl -w

use List::Util qw(any);
use XML::Parser;

$xml_line = 1;

if (@ARGV != 1) {
	die "Usage: $0 <xml file>";
}

$filename = $ARGV[0];
$run_adding_broken = 0;

$xmlp = new XML::Parser(Style => 'Tree', ParseParamEnt => 1, ErrorContext => 3, NoLWP => 1);
@tree = $xmlp -> parsefile($filename);

open(XML, $filename);
@xml_source = <XML>;

@required_calls = ('instantiate', 'connect_port', 'cleanup');

$root = $tree[0];
if ($$root[0] ne "ladspa") {
	die "This doesn't look like a valid ladspa description file";
}

# Pointer to <ladspa> element
$ladspa = $root->[1];

if ($ladspa->[3] ne "global") {
	die "Can't find global section. Should be immediately after ladspa\n";
}

@globtags = @{ $ladspa->[4] };
for ($i=3; $i<@globtags; $i+=4) {
	$foo = $globtags[$i];
	unless (ref($foo)) {
		if ($foo eq "meta") {
			$global{$globtags[$i+1]->[0]->{'name'}} = $globtags[$i+1]->[0]->{'value'};
		}
		if ($foo eq "include") {
			push(@includes, $globtags[$i+1]->[0]->{'file'});
		}
		if ($foo eq "code") {
			$g_code = $globtags[$i+1]->[2];
			$g_code =~ s/^\s*\n//;
			$g_code =~ s/\t/        /g;
			$g_code =~ /^( *)/;
			$xml_indent = " " x length($1);
			$g_code =~ s/(^|\n)$xml_indent/$1/g;
			$g_code =~ s/\s+$//;
			$global_code .= $g_code."\n\n";
		}
	}
}

for (my $i=7; $i<@{$ladspa}; $i+=4) {
	$foo = $ladspa->[$i];
	if ($foo eq "plugin") {
		&process_plugin($ladspa->[$i+1]);
		push(@allports, @ports);
		@ports = ();
	}
}

# General headers
print <<EOB;
\#include <stdlib.h>
\#include <string.h>
\#ifndef WIN32
\#include "config.h"
\#endif

\#ifdef ENABLE_NLS
\#include <libintl.h>
\#endif

\#define         _ISOC9X_SOURCE  1
\#define         _ISOC99_SOURCE  1
\#define         __USE_ISOC99    1
\#define         __USE_ISOC9X    1

\#include <math.h>

\#include "ladspa.h"

\#ifdef WIN32
\#define _WINDOWS_DLL_EXPORT_ __declspec(dllexport)
int bIsFirstTime = 1; 
static void __attribute__((constructor)) swh_init(); // forward declaration
\#else
\#define _WINDOWS_DLL_EXPORT_ 
\#endif

EOB

for $inc (@includes) {
	print "#include \"$inc\"\n";
}
unless ($global_code) {
	$global_code = "";
}
my $glob_code_start = find_el_line("code");
print "#line $glob_code_start \"$filename\"\n" if $glob_code_start && $xml_line;
print "\n$global_code";

my $n = 0;
$last_plugin = "";
for $port (@allports) {
	if ($port->{'plugin'} ne $last_plugin) {
		$n = 0;
		$last_plugin = $port->{'plugin'};
	}
	$pl = uc($port->{'plugin'}.'_'.$port->{'label'});
	printf("#define %-30s $n\n", $pl);
	$n++;
}

print $globals;
print <<EOB;

_WINDOWS_DLL_EXPORT_
const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {

\#ifdef WIN32
	if (bIsFirstTime) {
		swh_init();
		bIsFirstTime = 0;
	}
\#endif
	switch (index) {
EOB
my $pic = 0;
for $plugin (@plugins) {
	print "	case $pic:\n		return ${plugin}Descriptor;\n";
	$pic++;
}
print "	default:\n		return NULL;\n	}\n}\n\n";
print $code;
# Headers for init section
print <<EOB;
static void __attribute__((constructor)) swh_init() {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

\#ifdef ENABLE_NLS
\#define D_(s) dgettext(PACKAGE, s)
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
\#else
\#define D_(s) (s)
\#endif

EOB
print $init_code;
print "}\n";

print <<EOB;

static void __attribute__((destructor)) swh_fini() {
$fini_code
}
EOB

sub process_plugin {
	local($tree) = @_;
	my $name = "noName";
	my $maker = $global{'maker'};
	my $copyright = $global{'copyright'};
	my $prop = $global{'properties'};

	unless ($prop) {
		$prop = "";
	}
	@props = split(/( |,)/, $prop);
	$properties = "";
	for $p (@props) {
		unless ($p =~ /^\s*,*\s*$/) {
			if ($properties) {
				$properties .= " | ";
			}
			$properties .= "LADSPA_PROPERTY_\U$p"
		}
	}
	unless ($properties) {
		$properties = "0";
	}

	$id = $tree->[0]->{'id'};
	$label = $tree->[0]->{'label'};
	if (!$id) {
		die "Plugin '$label' has no id";
	}
	if (!$label) {
		die "Plugin #$id has no label";
	}
	push(@plugins, $label);

	%i_data = ();
	for ($i=3; $i<@{$tree}; $i+=4) {
		$foo = $tree->[$i];
		if ($foo eq "name") {
			$name = $tree->[$i+1]->[2];
		}
		if ($foo eq "callback") {
			push(@callbacks, $tree->[$i+1]->[0]->{'event'});
			$callback_code{$label.'_'.$tree->[$i+1]->[0]->{'event'}} = $tree->[$i+1]->[2];
			$callback_unused_vars{$label.'_'.$tree->[$i+1]->[0]->{'event'}} = $tree->[$i+1]->[0]->{'unused-vars'} // "";
		}
		if ($foo eq "port") {
			push(@ports, &process_port($tree->[$i+1]));
		}
		if ($foo eq "instance-data") {
			$i_data{$tree->[$i+1]->[0]->{'label'}} =
			 $tree->[$i+1]->[0]->{'type'};
		}
	}

	my @calls = ();
	my %MARK = ();
	grep($MARK{$_}++, @callbacks, @required_calls);
	@calls = sort keys %MARK;
	
	for $call (@calls) {
		my $call_start = find_el_line("callback", "event" => $call);
		if ($call_start) {
			$call_start++;
			if ($xml_line ) {
				$cc_marker = "#line $call_start \"$filename\"\n";
 			} else {
				$cc_marker = "";
			}
		} else {
			$cc_marker = "";
		}

		if ($call eq "instantiate") {
			$c = "";
			if ($callback_code{"${label}_instantiate"}) {
				$c .= "\t\u$label *plugin_data = (\u$label *)calloc(1, sizeof(\u$label));\n";
				for $var (sort keys %i_data) {
					if ($i_data{$var} =~ /\*/) {
						$c .= "\t$i_data{$var}$var = NULL;\n";
					} else {
						#$c .= "\t$i_data{$var} $var = 0;\n";
						$c .= "\t$i_data{$var} $var;\n";
					}
				}
				$c .= "\n".$cc_marker;
				$c .= reindent_callback($callback_code{"${label}_instantiate"})."\n\n";
				for $var (sort keys %i_data) {
					$c .= "\tplugin_data->$var = $var;\n";
				}
				$c .= "\n\treturn (LADSPA_Handle)plugin_data;"
			} else {
				$c .= "\t\u$label *plugin_data = (\u$label *)calloc(1, sizeof(\u$label));\n";
				$c .= "\tplugin_data->run_adding_gain = 1.0f;\n";
				$c .= "\n\treturn (LADSPA_Handle)plugin_data;"
			}
			$code .= <<EOB
static LADSPA_Handle instantiate\u$label(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
${c}
}

EOB
		}

		if ($call eq "connect_port") {
			$code .= <<EOB;
static void connectPort\u$label(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	\u$label *plugin;

	plugin = (\u$label *)instance;
	switch (port) {
EOB
			for $port (@ports) {
				$sw = uc($label.'_'.$port->{'label'});
				if ($port->{'watch'}) {
					$watch = "\n		plugin->$port->{'watch'} = 1;";
				} else {
					$watch = "";
				}
				$code .= <<EOB;
	case $sw:
		plugin->$port->{label} = data;$watch
		break;
EOB
			}
			$code .= <<EOB;
	}
}

EOB
		}

		if ($call eq "activate") {
			$c = "";
			if ($callback_code{"${label}_activate"}) {
				$c .= "\t\u$label *plugin_data = (\u$label *)instance;\n";
				for $var (sort keys %i_data) {
					if ($i_data{$var} =~ /\*/) {
						$c .= "\t$i_data{$var}$var = plugin_data->$var;\n";
					} else {
						$c .= "\t$i_data{$var} $var = plugin_data->$var;\n";
					}
				}
				$c .= $cc_marker;
				$c .= reindent_callback($callback_code{"${label}_activate"})."\n";
				for $var (sort keys %i_data) {
					$c .= "\tplugin_data->$var = $var;\n";
				}
			} else {
				$c = "";
			}
			$code .= <<EOB;
static void activate\u$label(LADSPA_Handle instance) {
${c}
}

EOB
		}

		if ($call eq "cleanup") {
			if ($callback_code{"${label}_cleanup"}) {
				@unused_vars = split(/[ ,]+/, $callback_unused_vars{"${label}_cleanup"});
				$c = $cc_marker;
				unless (any { $_ eq 'plugin_data' } @unused_vars) {
					$c .= "\t\u$label *plugin_data = (\u$label *)instance;\n";
				}
				$c .= reindent_callback($callback_code{"${label}_cleanup"})."\n";
			} else {
				$c = "";
			}
			$code .= <<EOB;
static void cleanup\u$label(LADSPA_Handle instance) {
${c}	free(instance);
}

EOB
		}

		if ($call eq "run") {
			for $port (@ports) {
				my $const;
				if ($port->{'dir'} eq "input") {
					$const = "const ";
				} else {
					$const = "";
				}
				if ($port->{'type'} eq "audio") {
					$run_code .= "\n	/* $port->{name} (array of floats of length sample_count) */\n";
					$run_code .= "	${const}LADSPA_Data * const ".$port->{'label'}." = plugin_data->".$port->{'label'}.";\n";
				} elsif ($port->{'dir'} eq "input") {
					$run_code .= "\n	/* $port->{name} (float value) */\n";
					$run_code .= "	const LADSPA_Data ".$port->{'label'}." = *(plugin_data->".$port->{'label'}.");\n";
				}
			}
			@unused_vars = split(/[ ,]+/, $callback_unused_vars{"${label}_run"});
			for $var (sort keys %i_data) {
				unless (any { $_ eq $var } @unused_vars) {
					$run_code .= "	$i_data{$var} $var = plugin_data->$var;\n";
				}
			}
			if ($callback_code{"${label}_run"}) {
				$cb_code = $callback_code{"${label}_run"};
				$cb_code =~ s/^\n//;
				$cb_code =~ s/\t/        /g;
				$cb_code =~ /^( *)/;
				$xml_indent = " " x length($1);
				$cb_code =~ s/(^|\n)$xml_indent/$1\t/g;
				$cb_code =~ s/\s+$//;
			} else {
				$cb_code = <<EOB;
	unsigned int pos;
	for (pos = 0; pos < sample_count; pos++) {
		// Process samples in here
	}
EOB
			}
			$run_code .= <<EOB;

$cc_marker$cb_code
}
EOB
			$code .= <<EOB;
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void run\u$label(LADSPA_Handle instance, unsigned long sample_count) {
	\u$label *plugin_data = (\u$label *)instance;
EOB
			if ($run_code =~ /run_adding_gain/) {
				$code .= "\tLADSPA_Data run_adding_gain = plugin_data->run_adding_gain;\n";
			}
			$code .= $run_code;
			$code .= <<EOB;
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGain\u$label(LADSPA_Handle instance, LADSPA_Data gain) {
	((\u$label *)instance)->run_adding_gain = gain;
}

static void runAdding\u$label(LADSPA_Handle instance, unsigned long sample_count) {
	\u$label *plugin_data = (\u$label *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;
$run_code
EOB
		}
	}
	$run_code = "";

	$globals .= "
static LADSPA_Descriptor *${label}Descriptor = NULL;\n";

	$num_ports = @ports;
	$init_code .= <<EOB;

	${label}Descriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (${label}Descriptor) {
		${label}Descriptor->UniqueID = $id;
		${label}Descriptor->Label = "$label";
		${label}Descriptor->Properties =
		 $properties;
		${label}Descriptor->Name =
		 D_("$name");
		${label}Descriptor->Maker =
		 "$maker";
		${label}Descriptor->Copyright =
		 "$copyright";
		${label}Descriptor->PortCount = $num_ports;

		port_descriptors = (LADSPA_PortDescriptor *)calloc($num_ports,
		 sizeof(LADSPA_PortDescriptor));
		${label}Descriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc($num_ports,
		 sizeof(LADSPA_PortRangeHint));
		${label}Descriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc($num_ports, sizeof(char*));
		${label}Descriptor->PortNames =
		 (const char **)port_names;

EOB

	$fini_code .= <<EOB;
	if (${label}Descriptor) {
		free((LADSPA_PortDescriptor *)${label}Descriptor->PortDescriptors);
		free((char **)${label}Descriptor->PortNames);
		free((LADSPA_PortRangeHint *)${label}Descriptor->PortRangeHints);
		free(${label}Descriptor);
	}
	${label}Descriptor = NULL;
EOB

	$globals .= "\ntypedef struct {\n";
	for $port (@ports) {
		my $min = "";
		my $max = "";

		$l = uc($port->{'label'});
		$d = uc($port->{'dir'});
		$t = uc($port->{'type'});
		$n = $port->{'name'};
		$p = uc($label);
		$min = $port->{'min'} if defined $port->{'min'};
		$max = $port->{'max'} if defined $port->{'max'};
		if (defined $port->{'hints'}) {
			$hints = $port->{'hints'};
		} else {
			$hints = "";
		}
		$init_code .= <<EOB;
		/* Parameters for $n */
		port_descriptors[${p}_$l] =
		 LADSPA_PORT_$d | LADSPA_PORT_$t;
		port_names[${p}_$l] =
		 D_("$n");
EOB
		if ($min ne "" && $max ne "") {
			$init_code .= <<EOB;
		port_range_hints[${p}_$l].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE$hints;
		port_range_hints[${p}_$l].LowerBound = $min;
		port_range_hints[${p}_$l].UpperBound = $max;
EOB
		} elsif ($min ne "") {
			$init_code .= <<EOB;
		port_range_hints[${p}_$l].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[${p}_$l].LowerBound = $min;
EOB
		} elsif ($max ne "") {
			$init_code .= <<EOB;
		port_range_hints[${p}_$l].HintDescriptor =
		 LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[${p}_$l].UpperBound = $max;
EOB
		} else {
			$init_code .= <<EOB;
		port_range_hints[${p}_$l].HintDescriptor = 0;
EOB
		}

		$init_code .= "\n";

		$globals .= "	LADSPA_Data *$port->{label};\n";
	}

	# Add instance data to struct
	for $var (sort keys %i_data) {
		if ($i_data{$var} =~ /\*/) {
			$globals .= sprintf("\t%-13s$var;\n", $i_data{$var});
		} else {
			$globals .= sprintf("\t%-12s $var;\n", $i_data{$var});
		}
	}
	$globals .= "\tLADSPA_Data run_adding_gain;\n";
	$globals .= "} \u$label;\n";

	@possible = ('activate', 'run', 'deactivate');
	for $call (@possible) {
		$cb_pointers{$call} = 'NULL';
	}
	push(@pointers, @callbacks);
	push(@pointers, @required_calls);
	for $call (@pointers) {
		$meth_call = $call;
		$meth_call =~ s/_([a-z])/\u$1/g;
		$cb_pointers{$call} = "$meth_call\u$label";
	}

	for $call (sort keys %cb_pointers) {
		$init_code .= "		${label}Descriptor->$call = $cb_pointers{$call};\n";
	}
	if (!$run_adding_broken) {
		$init_code .= "		${label}Descriptor->run_adding = runAdding\u$label;\n";
		$init_code .= "		${label}Descriptor->set_run_adding_gain = setRunAddingGain\u$label;\n";
	} else {
		$init_code .= "		${label}Descriptor->run_adding = NULL;\n";
		$init_code .= "		${label}Descriptor->set_run_adding_gain = NULL;\n";
	}

	$init_code .= <<EOB;
	}
EOB
}

sub process_port {
	local($tree) = @_;

	$portcount++;
	$pname = "port_$portcount";
	$$pname{'plugin'} = $label;
	$$pname{'number'} = $portcount;
	$$pname{'label'} = $tree->[0]->{'label'};
	$$pname{'watch'} = $tree->[0]->{'watch'};
	$$pname{'dir'} = $tree->[0]->{'dir'};
	$$pname{'type'} = $tree->[0]->{'type'};
	if (!$$pname{'label'} || !$$pname{'dir'} || !$$pname{'type'}) {
		die "Ports must have a label, dir(ection) and type";
	}

	my $hints = "";
	if ($tree->[0]->{'hint'}) {
		for $h (split(/[ ,]+/, $tree->[0]->{'hint'})) {
			$hints .= " | LADSPA_HINT_\U$h";
		}
	}
	$$pname{'hints'} = $hints;

	for (my $el=3; $el<@{$tree}; $el+=4) {
		$foo = $tree->[$el];
		if ($foo eq "name") {
			$$pname{'name'} = $tree->[$el+1]->[2];
		}
		if ($foo eq "range") {
			$$pname{'min'} = $tree->[$el+1]->[0]->{'min'};
			$$pname{'max'} = $tree->[$el+1]->[0]->{'max'};
		}
	}

	return \%$pname;
}

sub reindent_callback {
	local ($cb_code) = @_;
	$cb_code =~ s/^\n//;
	$cb_code =~ s/\t/        /g;
	$cb_code =~ /^( *)/;
	$xml_indent = " " x length($1);
	$cb_code =~ s/(^|\n)$xml_indent/$1\t/g;
	$cb_code =~ s/\s+$//;
	return $cb_code;
}

sub find_el_line {
	local ($el, @rest) = @_;

	my $cnt = 0;
	my %attrs = ();
	while (@rest) {
		my $key = shift @rest;
		$attrs{$key} = shift @rest;
	}
	if (%attrs) {
		my $in_el = 0;
		my $num_attrs = length keys %attrs;
		my $matched = 0;
		for $line (@xml_source) {
			$cnt++;
			$in_el = 1 if $line =~ /<\s*$el[ \n>]/;
			if ($in_el) {
				for $attr (keys %attrs) {
					my $val = $attrs{$attr};
					$matched++ if $line =~ /$attr\s*=\s*"$val"/;
				}
			}

			if ($matched >= $num_attrs) {
				return $cnt;
			}

			if ($line =~ />/) {
				$in_el = 0;
				$matched = 0;
			}
		}
	} else {
		for $line (@xml_source) {
			$cnt++;
			return $cnt if $line =~ /<\s*$el\s*>/;
		}
	}

	return 0;
}
