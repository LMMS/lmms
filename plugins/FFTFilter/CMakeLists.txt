INCLUDE(BuildPlugin)

BUILD_PLUGIN(FFTFilter FFTFilter.cpp FFTFilterControls.cpp FFTFilterControlDialog.cpp FFTProcessor.cpp MOCFILES FFTFilterControls.h FFTFilterControlDialog.h FFTProcessor.h EMBEDDED_RESOURCES artwork.png logo.png)
