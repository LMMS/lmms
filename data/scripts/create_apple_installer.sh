#!/bin/bash
#title         :create_applet_installer.sh
#description   :Creates Apple ".app" bundle for LMMS
#author        :Tres Finocchiaro
#date          :20140331
#version       :1.0
#usage         :bash create_applet_installer.sh
#notes         :See also https://github.com/LMMS
#notes         :Troubleshooting try: export DYLD_PRINT_LIBRARIES=1
#requires      :deploymacqt
#=========================================================================================

# MacPorts Location
MACPORTS=/opt/local
 
 # LMMS source directory
CMAKE_SRC=$HOME/lmms
 
# LMMS compiled resources
CMAKE_INSTALL=$CMAKE_SRC/target

# LMMS source build directory
CMAKE_BUILD=$CMAKE_SRC/build



# Place to create ".app" bundle
APP=$HOME/Desktop/LMMS.app

# MacPorts installs libreadline with wrong permissions
LIBREADLINE=$MACPORTS/lib/libreadline.6.2.dylib

#=========================================================================================

echo -e "\n\nRunning..."

# Check for u+w permissions on libreadline
_perm=`stat -f "%p" $LIBREADLINE`
_perm=${_perm:3:1}
if [ ${_perm} != "7" ]
then
  echo -e "\n\n\t\t\t\t***********\n\t\t\t\t* WARNING *\n\t\t\t\t***********"
  echo -e "File $LIBREADLINE is not marked as user writable."
  echo -e "This will break macdeployqt's linking process after it is copied."
  echo -e "A sudo password is required to elevate and fix using chmod u+w."
  echo -e "\nPLEASE ENTER SUDO PASSWORD:"
  sudo chmod u+w $MACPORTS/lib/libreadline.6.2.dylib
fi

# Remove any old .app bundles
rm -Rf $APP

# Create new bundle, copy our built code to it
mkdir -p $APP
cd $CMAKE_INSTALL
mkdir $APP/Contents
cp -R * $APP/Contents

# Make all libraries writable for macdeployqt
cd $APP
find . -type f -print0 | xargs -0 chmod u+w

# Move lmms binary to the proper location
mkdir -p $APP/Contents/MacOS
mv $APP/Contents/bin/lmms $APP/Contents/MacOS
rmdir $APP/Contents/bin

# Move libraries to proper locations
mkdir -p $APP/Contents/Frameworks
mv $APP/Contents/lib/lmms/libZynAddSubFxCore.dylib \
   $APP/Contents/Frameworks/libZynAddSubFxCore.dylib
   
mv $APP/Contents/lib/lmms/RemoteZynAddSubFx \
   $APP/Contents/MacOS/RemoteZynAddSubFx

# Fix more Zyn Linking issues
# install_name_tool -change $CMAKE_INSTALL/lib/lmms/libZynAddSubFxCore.dylib \
#   @loader_path/../../Frameworks/libZynAddSubFxCore.dylib \
#   $APP/Contents/lib/lmms/libzynaddsubfx.so

install_name_tool -change libZynAddSubFxCore.dylib \
   @loader_path/../../Frameworks/libZynAddSubFxCore.dylib \
   $APP/Contents/lib/lmms/libzynaddsubfx.so

install_name_tool -change $CMAKE_BUILD/plugins/zynaddsubfx/libZynAddSubFxCore.dylib \
   @loader_path/../../Frameworks/libZynAddSubFxCore.dylib \
   $APP/Contents/MacOS/RemoteZynAddSubFx

# Build a list of shared objects in target/lib/lmms
for file in  $APP/Contents/lib/lmms/*.so; do
   _executables="$_executables -executable=$APP/Contents/lib/lmms/${file##*/}"
done

# Build a list of shared objects in target/lib/lmms/ladspa
for file in  $APP/Contents/lib/lmms/ladspa/*.so; do
   _executables="$_executables -executable=$APP/Contents/lib/lmms/ladspa/${file##*/}"
done
 
# Additional binaries that require linking
_executables="$_executables -executable=$APP/Contents/MacOS/RemoteZynAddSubFx"
_executables="$_executables -executable=$APP/Contents/Frameworks/libZynAddSubFxCore.dylib"

# Build our App Package using "macdeployqt"
macdeployqt $APP $_executables

# OS X Specific Artwork
cp $CMAKE_SRC/data/lmms.icns $APP/Contents/Resources/

# Create "Info.plist" using lmms.icns file, http://iconverticons.com/online/)
echo -e "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > "$APP/Contents/Info.plist"
echo -e "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\"" >> "$APP/Contents/Info.plist"
echo -e "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" >> "$APP/Contents/Info.plist"
echo -e "   <plist version=\"1.0\">" >> "$APP/Contents/Info.plist"
echo -e "   <dict>" >> "$APP/Contents/Info.plist"
echo -e "      <key>CFBundleIconFile</key>" >> "$APP/Contents/Info.plist"
echo -e "      <string>lmms.icns</string>" >> "$APP/Contents/Info.plist"
echo -e "   </dict>" >> "$APP/Contents/Info.plist"
echo -e "</plist>" >> "$APP/Contents/Info.plist"

# Done.  Ready to build DMG
echo -e "\nFinished.\n\nPlease run \"create_apple_dmg.sh\" from the Desktop to build the installer.\n"
cp $CMAKE_SRC/data/scripts/create_apple_dmg.sh $HOME/Desktop/
chmod +x $HOME/Desktop/create_apple_dmg.sh
