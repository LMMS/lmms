#!/bin/bash
ppa_dir=./ppa/

pushd $ppa_dir

for f in *.deb; do
	echo "Extracting $f..."
	ar xv "$f"
	rm debian-binary
	rm control.tar.*
	tar xf data.tar.* --exclude=*mingw*/bin/fluid
	rm data.tar.*
done

popd

echo "Your extracted files should be located in $ppa_dir"
