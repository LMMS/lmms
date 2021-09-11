#include "lmms_basics.h"

#ifdef __GNUC__
#define LMMS_BUILDCONF_COMPILER_VERSION "GCC " __VERSION__
#elif defined(_MSC_VER)
#define LMMS_BUILDCONF_COMPILER_VERSION "MSVC " STRINGIFY(_MSC_FULL_VER)
#else
#define LMMS_BUILDCONF_COMPILER_VERSION "unknown compiler"
#endif

#ifdef LMMS_HOST_X86
#define LMMS_BUILDCONF_MACHINE "i386"
#endif

#ifdef LMMS_HOST_X86_64
#define LMMS_BUILDCONF_MACHINE "x86_64"
#endif

#ifdef LMMS_HOST_ARM32
#define LMMS_BUILDCONF_MACHINE "arm32"
#endif

#ifdef LMMS_HOST_ARM64
#define LMMS_BUILDCONF_MACHINE "arm64"
#endif

#ifdef LMMS_HOST_RISCV32
#define LMMS_BUILDCONF_MACHINE "riscv32"
#endif

#ifdef LMMS_HOST_RISCV64
#define LMMS_BUILDCONF_MACHINE "riscv64"
#endif

#ifndef LMMS_BUILDCONF_MACHINE
#define LMMS_BUILDCONF_MACHINE "unknown processor"
#endif

#ifdef LMMS_BUILD_LINUX
#define LMMS_BUILDCONF_PLATFORM "Linux"
#endif

#ifdef LMMS_BUILD_APPLE
#define LMMS_BUILDCONF_PLATFORM "OS X"
#endif

#ifdef LMMS_BUILD_OPENBSD
#define LMMS_BUILDCONF_PLATFORM "OpenBSD"
#endif

#ifdef LMMS_BUILD_FREEBSD
#define LMMS_BUILDCONF_PLATFORM "FreeBSD"
#endif

#ifdef LMMS_BUILD_WIN32
#define LMMS_BUILDCONF_PLATFORM "win32"
#endif

#ifdef LMMS_BUILD_HAIKU
#define LMMS_BUILDCONF_PLATFORM "Haiku"
#endif
