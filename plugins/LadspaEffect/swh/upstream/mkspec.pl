#!/usr/bin/perl -w

$package = shift(@ARGV);
$version = shift(@ARGV);
@files = @ARGV;

open(OUT, ">swh-plugins-$version.spec") || die "Can't create spec file: $!";

print OUT <<EOB;
Summary: A set of audio plugins for LADSPA.
Name: $package
Version: $version
Release: 1
Copyright: GPL
Group: Applications/Multimedia
Source: http://plugin.org.uk/releases/$version/$package-$version.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot
Prefix: /usr/local

%description
A set of audio plugins for LADSPA (see http://plugin.org.uk/ for more
details).

%prep
rm -rf \$RPM_BUILD_ROOT/$package-$version
tar xvfz /usr/src/redhat/SOURCES/$package-$version.tar.gz

%build
cd $package-$version
./configure
make static

%install
cd $package-$version
make INSTALL_ROOT="\$RPM_BUILD_ROOT" install

%clean
rm -rf \$RPM_BUILD_ROOT

%files
%defattr(-,root,root)

EOB
for $file (@files) {
        print OUT "%{prefix}/lib/ladspa/$file\n";
}
print OUT "%{prefix}/share/ladspa/rdf/swh-plugins.rdf";

system("cp $package-$version.tar.gz /usr/src/redhat/SOURCES/");
