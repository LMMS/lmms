set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Fix mpg123 build (see the have-fpu.diff patch in the port)
if(PORT MATCHES "mpg123")
	set(VCPKG_C_FLAGS -DHAVE_FPU=1)
endif()
