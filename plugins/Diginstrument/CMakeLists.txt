INCLUDE(BuildPlugin)

BUILD_PLUGIN(diginstrument DiginstrumentPlugin.cpp DiginstrumentView.cpp DiginstrumentPlugin.h DiginstrumentView.h MOCFILES DiginstrumentPlugin.h DiginstrumentView.h EMBEDDED_RESOURCES *.png)
