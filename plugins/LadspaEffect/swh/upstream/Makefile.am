VERSION = @VERSION@

plugin_LTLIBRARIES = \
	amp_1181.la diode_1185.la \
	divider_1186.la shaper_1187.la ringmod_1188.la comb_1190.la \
	declip_1195.la foverdrive_1196.la sinus_wavewrapper_1198.la \
	hermes_filter_1200.la multivoice_chorus_1201.la flanger_1191.la \
	decimator_1202.la single_para_1203.la triple_para_1204.la \
	transient_1206.la fad_delay_1192.la \
	dc_remove_1207.la retro_flange_1208.la valve_1209.la \
	sifter_1210.la tape_delay_1211.la step_muxer_1212.la \
	foldover_1213.la svf_1214.la gsm_1215.la gverb_1216.la \
	phasers_1217.la harmonic_gen_1220.la vocoder_1337.la \
	surround_encoder_1401.la \
	delayorama_1402.la dyson_compress_1403.la crossover_dist_1404.la \
	valve_rect_1405.la split_1406.la alias_1407.la \
	satan_maximiser_1408.la karaoke_1409.la gate_1410.la \
	comb_splitter_1411.la wave_terrain_1412.la \
	hard_limiter_1413.la smooth_decimate_1414.la fm_osc_1415.la \
	analogue_osc_1416.la rate_shifter_1417.la freq_tracker_1418.la \
	mod_delay_1419.la matrix_st_ms_1420.la matrix_ms_st_1421.la \
	matrix_spatialiser_1422.la plate_1423.la gong_1424.la \
	sc1_1425.la sc2_1426.la sc3_1427.la zm1_1428.la inv_1429.la \
	chebstortion_1430.la bode_shifter_1431.la bode_shifter_cv_1432.la \
	am_pitchshift_1433.la sc4_1882.la \
	lcr_delay_1436.la giant_flange_1437.la dj_flanger_1438.la \
	gong_beater_1439.la hilbert_1440.la sin_cos_1881.la se4_1883.la \
	bandpass_a_iir_1893.la bandpass_iir_1892.la highpass_iir_1890.la \
	lowpass_iir_1891.la notch_iir_1894.la dj_eq_1901.la \
	butterworth_1902.la allpass_1895.la comb_1887.la decay_1886.la \
	delay_1898.la impulse_1885.la vynil_1905.la \
	revdelay_1605.la ls_filter_1908.la \
	const_1909.la pointer_cast_1910.la fast_lookahead_limiter_1913.la \
	latency_1914.la xfade_1915.la sc4m_1916.la \
	mbeq_1197.la pitch_scale_1193.la pitch_scale_1194.la imp_1199.la

SUBDIRS = po util gsm gverb metadata

# Wacky stuff to stop automake getting confused
EXTRA_DIST = config.rpath  @top_srcdir@/*.xml @top_srcdir@/*.c @top_srcdir@/*.h \
	@top_srcdir@/*.dtd @top_srcdir@/*.pl @top_srcdir@/*.css \
	@top_srcdir@/impulses/*.h autogen.sh bozosoity-checker.pl
plugindir = $(libdir)/ladspa

# Uncomment below for Darwin support.  Or add a conditional for this.
#AM_CFLAGS = -fno-common -flat_namespace -bundle -undefined suppress -lbundle1.o
AM_LDFLAGS = -module -avoid-version
DESTDIR = $(INSTALL_ROOT)
DISTFN = $(distdir)

# Files needed for FFT based plugins
pitch_scale_1193_la_LIBADD = -Lutil -lpitchscale $(FFTW_LIBS)
pitch_scale_1193_la_CFLAGS = $(FFTW_CFLAGS)
pitch_scale_1193_la_SOURCES = pitch_scale_1193.c
pitch_scale_1194_la_LIBADD = -Lutil -lpitchscale $(FFTW_LIBS)
pitch_scale_1194_la_CFLAGS = $(FFTW_CFLAGS)
pitch_scale_1194_la_SOURCES = pitch_scale_1194.c
mbeq_1197_la_LIBADD = $(FFTW_LIBS)
mbeq_1197_la_CFLAGS = $(FFTW_CFLAGS)
mbeq_1197_la_SOURCES = mbeq_1197.c
imp_1199_la_LIBADD = $(FFTW_LIBS)
imp_1199_la_CFLAGS = $(FFTW_CFLAGS)
imp_1199_la_DEPENDENCIES = @top_srcdir@/impulses/*
hermes_filter_1200_la_DEPENDENCIES = util/libblo.a
hermes_filter_1200_la_LIBADD = -Lutil -lblo
fm_osc_1415_la_DEPENDENCIES = util/libblo.a
fm_osc_1415_la_LIBADD = -Lutil -lblo
analogue_osc_1416_la_DEPENDENCIES = util/libblo.a
analogue_osc_1416_la_LIBADD = -Lutil -lblo
sc1_1425_la_LIBADD = -Lutil -ldb -lrms
sc2_1426_la_LIBADD = -Lutil -ldb -lrms
sc3_1427_la_LIBADD = -Lutil -ldb -lrms
sc4_1882_la_LIBADD = -Lutil -ldb -lrms
sc4m_1916_la_LIBADD = -Lutil -ldb -lrms
se4_1883_la_LIBADD = -Lutil -ldb -lrms
gsm_1215_la_LIBADD = gsm/libgsm.a
gverb_1216_la_LIBADD = -Lgverb -lgverb
lcr_delay_1436_la_DEPENDENCIES = util/biquad.h

highpass_iir_1890_la_LIBADD = -Lutil -liir
highpass_iir_1890_la_SOURCES = highpass_iir_1890.c
lowpass_iir_1891_la_LIBADD = -Lutil -liir
lowpass_iir_1891_la_SOURCES = lowpass_iir_1891.c
bandpass_iir_1892_la_LIBADD = -Lutil -liir
bandpass_iir_1892_la_SOURCES = bandpass_iir_1892.c
bandpass_a_iir_1893_la_LIBADD = -Lutil -liir
bandpass_a_iir_1893_la_SOURCES = bandpass_a_iir_1893.c
notch_iir_1894_la_LIBADD = -Lutil -liir
notch_iir_1894_la_SOURCES = notch_iir_1894.c
butterworth_1902_la_LIBADD = -Lutil -liir
butterworth_1902_la_SOURCES = butterworth_1902.c

# Rule to build .c files from XML source
%.c:	%.xml
	! test -f "$*.xml" || ./makestub.pl "$*.xml" > "$*.c"

#strip .libs/$$file;

install-pluginLTLIBRARIES: $(plugin_LTLIBRARIES)
	mkdir -p $(DESTDIR)/$(plugindir)
	list='$(plugin_LTLIBRARIES)'; \
	for file in $$list; do \
	  sofile=`basename $$file .la`.so; \
	  $(INSTALL_PROGRAM) .libs/$$sofile $(DESTDIR)/$(plugindir); \
	done

uninstall-pluginLTLIBRARIES:
	list='$(plugin_LTLIBRARIES)'; \
	for file in $$list; do \
	  sofile=`basename $$file .la`.so; \
	  rm -f $(DESTDIR)/$(plugindir)/$$sofile; \
	done

potfiles: all
	rm -f po/POTFILES.in
	list='$(plugin_LTLIBRARIES)'; for file in $$list; do \
	  echo `basename $$file .la`.c >> po/POTFILES.in; \
	done;

static:
	make 'FFTLIBS=-Bstatic $(FFTLIBS) -Bdynamic'

spec: dist
	./mkspec.pl $(PACKAGE) $(VERSION) $(SOBS)

rpm: dist spec
	rpm -ba --target i686 $(DISTFN).spec

snapshot: dist bozo
	cp $(DISTFN).tar.gz ../snapshots/swh-plugins-`date -I`.tar.gz

bozo: 
	./bozosoity-checker.pl @top_srcdir@/*.xml

release: dist bozo
	mkdir -p ../releases/$(VERSION)
	mv $(DISTFN).tar.gz ../releases/$(VERSION)/

.PRECIOUS:	%.c

ACLOCAL_AMFLAGS = -I m4
