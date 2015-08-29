set(NAME "LMMS")
set(PACKAGE_NAME "lmms")
set(EXEC "env QT_X11_NO_NATIVE_MENUBAR=1 ${PACKAGE_NAME}")
set(ICON_32 "/usr/share/pixmaps/lmms.xpm")
set(ICON "lmms")
set(ICON_INSTALL "data/lmms.png")
set(MIME_ICON "data/application-x-lmms-project.svg")
set(ENG_COMMENT "Easy music production for Everyone!")
set(ENG_GENERIC "music production suite")
set(EXTENDED_DESCRIPTION " LMMS aims to be a free alternative to popular (but commercial and closed-
 source) programs like FruityLoops, Cubase and Logic giving you the ability of
 producing music with your computer by creating cool loops, synthesizing and
 mixing sounds, arranging samples, having more fun with your MIDI-keyboard
 and much more...
 .
 LMMS combines the features of a tracker-/sequencer-program (pattern-/channel-/
 sample-/song-/effect-management) and those of powerful synthesizers and
 samplers in a modern, user-friendly and easy to use graphical user-interface")
 set(BUILD_DEPENDS " cmake,
 debhelper (>= 9.0.0),
 fluid,
 gcc-multilib [amd64],
 g++-multilib [amd64],
 imagemagick,
 ladspa-sdk,
 libsdl-sound1.2-dev,
 libsamplerate0-dev,
 libsndfile1-dev,
 libvorbis-dev,
 libjack-dev,
 libstk0-dev,
 libflac-dev,
 libfltk1.3-dev,
 libxinerama-dev,
 libasound2-dev [linux-any],
 libqt4-dev,
 libpulse-dev,
 libfluidsynth-dev,
 libfftw3-dev,
 libfreetype6-dev,
 libxft-dev,
 portaudio19-dev,
 qt4-qmake, 
 wine-dev[i386]")
 configure_file(${PACKAGE_NAME}.install.in ../debian/${PACKAGE_NAME}.install)
 configure_file(${PACKAGE_NAME}.desktop.in ../debian/${PACKAGE_NAME}.desktop)
 configure_file(menu.in ../debian/menu)
 configure_file(control.in ../debian/control)
  
