INCLUDE(BuildPlugin)

BUILD_PLUGIN(FFTFilter FFTFilter.cpp FFTFilterControls.cpp FFTFilterControlDialog.cpp MOCFILES FFTFilterControls.h FFTFilterControlDialog.h EMBEDDED_RESOURCES artwork.png logo.png)
