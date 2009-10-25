IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

SET(CPPFILES
  Fl.cxx
  Fl_Adjuster.cxx
  Fl_Bitmap.cxx
  Fl_Browser.cxx
  Fl_Browser_.cxx
  Fl_Browser_load.cxx
  Fl_Box.cxx
  Fl_Button.cxx
  Fl_Chart.cxx
  Fl_Check_Browser.cxx
  Fl_Check_Button.cxx
  Fl_Choice.cxx
  Fl_Clock.cxx
  Fl_Color_Chooser.cxx
  Fl_Counter.cxx
  Fl_Dial.cxx
  Fl_Double_Window.cxx
  Fl_File_Browser.cxx
  Fl_File_Chooser.cxx
  Fl_File_Chooser2.cxx
  Fl_File_Icon.cxx
  Fl_File_Input.cxx
  Fl_Group.cxx
  Fl_Help_View.cxx
  Fl_Image.cxx
  Fl_Input.cxx
  Fl_Input_.cxx
  Fl_Light_Button.cxx
  Fl_Menu.cxx
  Fl_Menu_.cxx
  Fl_Menu_Bar.cxx
  Fl_Sys_Menu_Bar.cxx
  Fl_Menu_Button.cxx
  Fl_Menu_Window.cxx
  Fl_Menu_add.cxx
  Fl_Menu_global.cxx
  Fl_Multi_Label.cxx
  Fl_Overlay_Window.cxx
  Fl_Pack.cxx
  Fl_Pixmap.cxx
  Fl_Positioner.cxx
  Fl_Preferences.cxx
  Fl_Progress.cxx
  Fl_Repeat_Button.cxx
  Fl_Return_Button.cxx
  Fl_Roller.cxx
  Fl_Round_Button.cxx
  Fl_Scroll.cxx
  Fl_Scrollbar.cxx
  Fl_Shared_Image.cxx
  Fl_Single_Window.cxx
  Fl_Slider.cxx
  Fl_Tabs.cxx
  Fl_Text_Buffer.cxx
  Fl_Text_Display.cxx
  Fl_Text_Editor.cxx
  Fl_Tile.cxx
  Fl_Tiled_Image.cxx
  Fl_Tooltip.cxx
  Fl_Valuator.cxx
  Fl_Value_Input.cxx
  Fl_Value_Output.cxx
  Fl_Value_Slider.cxx
  Fl_Widget.cxx
  Fl_Window.cxx
  Fl_Window_fullscreen.cxx
  Fl_Window_hotspot.cxx
  Fl_Window_iconize.cxx
  Fl_Wizard.cxx
  Fl_XBM_Image.cxx
  Fl_XPM_Image.cxx
  Fl_abort.cxx
  Fl_add_idle.cxx
  Fl_arg.cxx
  Fl_compose.cxx
  Fl_display.cxx
  Fl_get_key.cxx
  Fl_get_system_colors.cxx
  Fl_grab.cxx
  Fl_lock.cxx
  Fl_own_colormap.cxx
  Fl_visual.cxx
  Fl_x.cxx
  filename_absolute.cxx
  filename_expand.cxx
  filename_ext.cxx
  filename_isdir.cxx
  filename_list.cxx
  filename_match.cxx
  filename_setext.cxx
  fl_arc.cxx
  fl_arci.cxx
  fl_ask.cxx
  fl_boxtype.cxx
  fl_color.cxx
  fl_cursor.cxx
  fl_curve.cxx
  fl_diamond_box.cxx
  fl_dnd.cxx
  fl_draw.cxx
  fl_draw_image.cxx
  fl_draw_pixmap.cxx
  fl_engraved_label.cxx
  fl_file_dir.cxx
  fl_font.cxx
  fl_gtk.cxx
  fl_labeltype.cxx
  fl_line_style.cxx
  fl_open_uri.cxx
  fl_oval_box.cxx
  fl_overlay.cxx
  fl_overlay_visual.cxx
  fl_plastic.cxx
  fl_read_image.cxx
  fl_rect.cxx
  fl_round_box.cxx
  fl_rounded_box.cxx
  fl_set_font.cxx
  fl_set_fonts.cxx
  fl_scroll_area.cxx
  fl_shadow_box.cxx
  fl_shortcut.cxx
  fl_show_colormap.cxx
  fl_symbols.cxx
  fl_vertex.cxx
  screen_xywh.cxx
  fl_utf8.cxx
  fl_encoding_latin1.cxx
  fl_encoding_mac_roman.cxx
  )
SET(FLCPPFILES
  forms_compatability.cxx
  forms_bitmap.cxx
  forms_free.cxx
  forms_fselect.cxx
  forms_pixmap.cxx
  forms_timer.cxx
  )
SET(GLCPPFILES
  Fl_Gl_Choice.cxx
  Fl_Gl_Overlay.cxx
  Fl_Gl_Window.cxx
  freeglut_geometry.cxx
  freeglut_stroke_mono_roman.cxx
  freeglut_stroke_roman.cxx
  freeglut_teapot.cxx
  gl_draw.cxx
  gl_start.cxx
  glut_compatability.cxx
  glut_font.cxx
  )
SET(IMGCPPFILES
  fl_images_core.cxx
  Fl_BMP_Image.cxx
  Fl_File_Icon2.cxx
  Fl_GIF_Image.cxx
  Fl_Help_Dialog.cxx
  Fl_JPEG_Image.cxx
  Fl_PNG_Image.cxx
  Fl_PNM_Image.cxx
  )

SET(CFILES 
  fl_call_main.c
  flstring.c
  scandir.c
  numericsort.c
  vsnprintf.c
  xutf8/is_right2left.c
  xutf8/is_spacing.c
  xutf8/case.c
  xutf8/utf8Input.c
  xutf8/utf8Utils.c
  xutf8/utf8Wrap.c
  xutf8/keysym2Ucs.c
  fl_utf.c
  )

ADD_LIBRARY(fltk ${CPPFILES} ${CFILES})
INSTALL_TARGETS(/lib fltk)
SET_TARGET_PROPERTIES(fltk
  PROPERTIES
  VERSION ${FLTK_VERSION_MAJOR}.${FLTK_VERSION_MINOR}
  SOVERSION ${FLTK_VERSION_PATCH})

TARGET_LINK_LIBRARIES(fltk ${FLTK_PLATFORM_DEPENDENT_LIBS} ${CMAKE_THREAD_LIBS_INIT})

IF(X11_FOUND)
  TARGET_LINK_LIBRARIES(fltk ${X11_LIBRARIES})
ENDIF(X11_FOUND)

#IF(OPENGL_FOUND)
#  ADD_LIBRARY(fltk_gl ${GLCPPFILES})
#  INSTALL_TARGETS(/lib fltk_gl)
#  TARGET_LINK_LIBRARIES(fltk_gl fltk ${OPENGL_LIBRARIES})
#  SET_TARGET_PROPERTIES(fltk_gl
#    PROPERTIES
#    VERSION ${FLTK_VERSION_MAJOR}.${FLTK_VERSION_MINOR}
#    SOVERSION ${FLTK_VERSION_PATCH})
#ENDIF(OPENGL_FOUND)

#ADD_LIBRARY(fltk_forms ${FLCPPFILES})
#INSTALL_TARGETS(/lib fltk_forms)
#TARGET_LINK_LIBRARIES(fltk_forms fltk)
#SET_TARGET_PROPERTIES(fltk_forms
#  PROPERTIES
#  VERSION ${FLTK_VERSION_MAJOR}.${FLTK_VERSION_MINOR}
#  SOVERSION ${FLTK_VERSION_PATCH})
#
#ADD_LIBRARY(fltk_images ${IMGCPPFILES})
#INSTALL_TARGETS(/lib fltk_images)
#TARGET_LINK_LIBRARIES(fltk_images fltk ${FLTK_PNG_LIBRARIES}
#     ${FLTK_JPEG_LIBRARIES} ${FLTK_ZLIB_LIBRARIES})
#SET_TARGET_PROPERTIES(fltk_images
#  PROPERTIES
#  VERSION ${FLTK_VERSION_MAJOR}.${FLTK_VERSION_MINOR}
#  SOVERSION ${FLTK_VERSION_PATCH})
#
