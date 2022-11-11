#ifndef VERSION_INFO_H
#define VERSION_INFO_H

#include "lmms_basics.h"

#ifdef __GNUC__
constexpr const char* LMMS_BUILDCONF_COMPILER_VERSION = "GCC " __VERSION__;
#elif defined(_MSC_VER)
constexpr const char* LMMS_BUILDCONF_COMPILER_VERSION = "MSVC " LMMS_STRINGIFY(_MSC_FULL_VER);
#else
constexpr const char* LMMS_BUILDCONF_COMPILER_VERSION = "unknown compiler";
#endif

#ifdef LMMS_HOST_X86
constexpr const char* LMMS_BUILDCONF_MACHINE = "i386";
#elif defined(LMMS_HOST_X86_64)
constexpr const char* LMMS_BUILDCONF_MACHINE = "x86_64";
#elif defined(LMMS_HOST_ARM32)
constexpr const char* LMMS_BUILDCONF_MACHINE = "arm32";
#elif defined(LMMS_HOST_ARM64)
constexpr const char* LMMS_BUILDCONF_MACHINE = "arm64";
#elif defined(LMMS_HOST_RISCV32)
constexpr const char* LMMS_BUILDCONF_MACHINE = "riscv32";
#elif defined(LMMS_HOST_RISCV64)
constexpr const char* LMMS_BUILDCONF_MACHINE = "riscv64";
#else
constexpr const char* LMMS_BUILDCONF_MACHINE = "unknown processor";
#endif

#ifdef LMMS_BUILD_LINUX
constexpr const char* LMMS_BUILDCONF_PLATFORM = "Linux";
#endif

#ifdef LMMS_BUILD_APPLE
constexpr const char* LMMS_BUILDCONF_PLATFORM = "OS X";
#endif

#ifdef LMMS_BUILD_OPENBSD
constexpr const char* LMMS_BUILDCONF_PLATFORM = "OpenBSD";
#endif

#ifdef LMMS_BUILD_FREEBSD
constexpr const char* LMMS_BUILDCONF_PLATFORM = "FreeBSD";
#endif

#ifdef LMMS_BUILD_WIN32
constexpr const char* LMMS_BUILDCONF_PLATFORM = "win32";
#endif

#ifdef LMMS_BUILD_HAIKU
constexpr const char* LMMS_BUILDCONF_PLATFORM = "Haiku";
#endif

#endif
