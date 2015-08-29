SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION}")
SET(CPACK_DEBIAN_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
SET(CPACK_DEBIAN_PACKAGE_SECTION "sound")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PROJECT_AUTHOR} <${PROJECT_EMAIL}>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${PROJECT_URL}")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "any")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "${CPACK_DEBIAN_PACKAGE_NAME}-vst-full (= ${DOLLAZ}{binary:Version}) [amd64 i386], tap-plugins, caps")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_NAME}-common (= ${DOLLAZ}{source:Version}), ${DOLLAZ}{shlibs:Depends}, ${DOLLAZ}{misc:Depends}, zynaddsubfx | zynaddsubfx-git")
SET(CPACK_DEBIAN_PACKAGE_SUGGESTS  "fil-plugins, mcp-plugins, omins, freepats, fluid-soundfont-gm")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS " cmake,
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
SET(CPACK_PACKAGE_DESCRIPTION " ${PROJECT_NAME_UCASE} aims to be a free alternative to popular (but commercial and closed-
 source) programs like FruityLoops, Cubase and Logic giving you the ability of
 producing music with your computer by creating cool loops, synthesizing and
 mixing sounds, arranging samples, having more fun with your MIDI-keyboard
 and much more...
 .
 ${PROJECT_NAME_UCASE} combines the features of a tracker-/sequencer-program (pattern-/channel-/
 sample-/song-/effect-management) and those of powerful synthesizers and
 samplers in a modern, user-friendly and easy to use graphical user-interface")
SET(ICON_INSTALL "data/lmms.png")
SET(MIME_ICON "data/application-x-lmms-project.svg")

CONFIGURE_FILE(${CPACK_DEBIAN_PACKAGE_NAME}.install.in ${CMAKE_BINARY_DIR}/debian/${CPACK_DEBIAN_PACKAGE_NAME}.install)
CONFIGURE_FILE(${CPACK_DEBIAN_PACKAGE_NAME}-vst-full.install.in ${CMAKE_BINARY_DIR}/debian/${CPACK_DEBIAN_PACKAGE_NAME}-vst-full.install)

## This uses a lot of CPack variables incase we don't need to roll our own one day
CONFIGURE_FILE(control.in ${CMAKE_BINARY_DIR}/debian/control)

# these are basically just copied
CONFIGURE_FILE(watch.in ${CMAKE_BINARY_DIR}/debian/watch)
CONFIGURE_FILE(format.in ${CMAKE_BINARY_DIR}/debian/source/format)
CONFIGURE_FILE(compat.in ${CMAKE_BINARY_DIR}/debian/compat)
CONFIGURE_FILE(rules.in ${CMAKE_BINARY_DIR}/debian/rules)

