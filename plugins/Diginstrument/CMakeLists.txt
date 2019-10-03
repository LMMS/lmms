INCLUDE(BuildPlugin)

BUILD_PLUGIN(diginstrument DiginstrumentPlugin.cpp DiginstrumentView.cpp Synthesizer.cpp Instrument.cpp DiginstrumentPlugin.h DiginstrumentView.h Synthesizer.h Instrument.h MOCFILES DiginstrumentPlugin.h DiginstrumentView.h EMBEDDED_RESOURCES *.png)
