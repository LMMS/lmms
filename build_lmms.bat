@echo off
REM LMMS Build Script - Final Absolute Version

set VCVARS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist %VCVARS_PATH% set VCVARS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set VCPKG_PATH="c:\Users\user\Documents\vcpkg\scripts\buildsystems\vcpkg.cmake"
set QT_PATH="c:\Users\user\Documents\Qt5\5.15.2\msvc2019_64"

if not exist build mkdir build
cd build

REM Get absolute paths for library forcing (Corrected libgig path)
set "VCPKG_INST_BASE=%CD%\vcpkg_installed\x64-windows"

echo --- Loading MSVC Environment ---
call %VCVARS_PATH%

echo --- Configuring with Forced GIG Paths ---
cmake .. -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH% ^
    -DVCPKG_TARGET_TRIPLET="x64-windows" ^
    -DCMAKE_PREFIX_PATH=%QT_PATH% ^
    -DWANT_QT6=OFF ^
    -DWANT_VST_32=OFF ^
    -DWANT_GIG=ON ^
    -DGIG_INCLUDE_DIRS="%VCPKG_INST_BASE%\include\libgig" ^
    -DGIG_LIBRARIES="%VCPKG_INST_BASE%\lib\libgig.lib;%VCPKG_INST_BASE%\lib\libakai.lib;winmm.lib" ^
    -DGIG_FOUND=TRUE ^
    -DLMMS_HAVE_GIG=TRUE ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    -DCMAKE_INSTALL_PREFIX=install

echo --- Rapid Build ---
cmake --build . -j8

echo --- Organizing Plugins ---
cmake --build . --target install

echo --- Finalizing DLLs ---
cd install
"%QT_PATH:"=%\bin\windeployqt.exe" --no-compiler-runtime --no-translations --no-opengl-sw lmms.exe
copy /y ..\vcpkg_installed\x64-windows\bin\*.dll .

echo --- All Done! ---
echo Full build with GigPlayer: build\install\lmms.exe
