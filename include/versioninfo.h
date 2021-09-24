#include "lmms_basics.h"

#ifdef __GNUC__
#define COMPILER_VERSION "GCC " __VERSION__
#elif defined(_MSC_VER)
#define COMPILER_VERSION "MSVC " STRINGIFY(_MSC_FULL_VER)
#else
#define COMPILER_VERSION "unknown compiler"
#endif

#ifdef LMMS_HOST_X86
#define MACHINE "i386"
#endif

#ifdef LMMS_HOST_X86_64
#define MACHINE "x86_64"
#endif

#ifdef LMMS_HOST_ARM32
#define MACHINE "arm32"
#endif

#ifdef LMMS_HOST_ARM64
#define MACHINE "arm64"
#endif

#ifdef LMMS_HOST_RISCV32
#define MACHINE "riscv32"
#endif

#ifdef LMMS_HOST_RISCV64
#define MACHINE "riscv64"
#endif

#ifndef MACHINE
#define MACHINE "unknown processor"
#endif

#ifdef LMMS_BUILD_LINUX
#define PLATFORM "Linux"
#endif

#ifdef LMMS_BUILD_APPLE
#define PLATFORM "OS X"
#endif

#ifdef LMMS_BUILD_OPENBSD
#define PLATFORM "OpenBSD"
#endif

#ifdef LMMS_BUILD_FREEBSD
#define PLATFORM "FreeBSD"
#endif

#ifdef LMMS_BUILD_WIN32
#define PLATFORM "win32"
#endif

#ifdef LMMS_BUILD_HAIKU
#define PLATFORM "Haiku"
#endif
