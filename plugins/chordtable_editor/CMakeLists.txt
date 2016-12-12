INCLUDE(BuildPlugin)

BUILD_PLUGIN(chordtableeditor chordtable_editor.cpp chordtable_editor.h MOCFILES chordtable_editor.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
