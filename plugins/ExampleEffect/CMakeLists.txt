INCLUDE(BuildPlugin)

BUILD_PLUGIN(exampleeffect ExampleEffect.cpp ExampleEffectControls.cpp ExampleEffectControlDialog.cpp MOCFILES ExampleEffectControls.h ExampleEffectControlDialog.h EMBEDDED_RESOURCES artwork.svg logo.svg)
