INCLUDE(BuildPlugin)

BUILD_PLUGIN(stereomatrix stereo_matrix.cpp stereomatrix_controls.cpp stereomatrix_control_dialog.cpp stereo_matrix.h stereomatrix_controls.h stereomatrix_control_dialog.h MOCFILES stereomatrix_controls.h stereomatrix_control_dialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

