pkgdata_DATA = swh-plugins.rdf swh-aux.rdf swh-scales.rdf
EXTRA_DIST = lxml2rdf.pl txt2scale.pl scale-points.txt $(pkgdata_DATA)
pkgdatadir = $(datadir)/ladspa/rdf

swh-plugins.rdf: @top_srcdir@/*.xml
	./lxml2rdf.pl $(sort $^) > $@
	xmllint -noout swh-plugins.rdf

swh-scales.rdf: scale-points.txt
	./txt2scale.pl scale-points.txt > swh-scales.rdf
