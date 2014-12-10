INCLUDE(BuildPlugin)

BUILD_PLUGIN(eq eqeffect.cpp eqcontrols.cpp eqcontrolsdialog.cpp eqfilter.h eqparameterwidget.cpp eqfader.h MOCFILES eqcontrols.h eqparameterwidget.h eqfader.h  EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
