INCLUDE(BuildPlugin)

BUILD_PLUGIN(ladspabrowser LadspaBrowser.cpp LadspaBrowser.h LadspaDescription.cpp LadspaDescription.h LadspaPortDialog.cpp LadspaPortDialog.h MOCFILES LadspaBrowser.h LadspaDescription.h LadspaPortDialog.h EMBEDDED_RESOURCES logo.png)
