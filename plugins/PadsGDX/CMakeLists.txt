INCLUDE(BuildPlugin)

BUILD_PLUGIN(padsgdx PadsGDX.cpp PadsGDX.h PadsGDXView.cpp PadsGDXView.h PadsGDXWaveView.cpp PadsGDXWaveView.h MOCFILES PadsGDX.h PadsGDXView.h PadsGDXWaveView.h EMBEDDED_RESOURCES *.png)
