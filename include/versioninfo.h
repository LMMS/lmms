#ifndef LMMS_VERSION_INFO_H
#define LMMS_VERSION_INFO_H

#include "LmmsCommonMacros.h"

#if defined(__GNUC__)
constexpr const char* LMMS_BUILDCONF_COMPILER_VERSION = "GCC " __VERSION__;
#elif defined(__clang__)
constexpr const char* LMMS_BUILDCONF_COMPILER_VERSION = "Clang " __clang_version__;
#elif defined(_MSC_VER)
constexpr const char* LMMS_BUILDCONF_COMPILER_VERSION = "MSVC " LMMS_STRINGIFY(_MSC_FULL_VER);
#else
constexpr const char* LMMS_BUILDCONF_COMPILER_VERSION = "unknown compiler";
#endif

#if defined(LMMS_HOST_X86)
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
#elif defined(LMMS_HOST_PPC32)
constexpr const char* LMMS_BUILDCONF_MACHINE = "ppc";
#elif defined(LMMS_HOST_PPC64)
constexpr const char* LMMS_BUILDCONF_MACHINE = "ppc64";
#else
constexpr const char* LMMS_BUILDCONF_MACHINE = "unknown processor";
#endif

#if defined(LMMS_BUILD_LINUX)
constexpr const char* LMMS_BUILDCONF_PLATFORM = "Linux";
#elif defined(LMMS_BUILD_APPLE)
constexpr const char* LMMS_BUILDCONF_PLATFORM = "OS X";
#elif defined(LMMS_BUILD_OPENBSD)
constexpr const char* LMMS_BUILDCONF_PLATFORM = "OpenBSD";
#elif defined(LMMS_BUILD_FREEBSD)
constexpr const char* LMMS_BUILDCONF_PLATFORM = "FreeBSD";
#elif defined(LMMS_BUILD_WIN32)
constexpr const char* LMMS_BUILDCONF_PLATFORM = "win32";
#elif defined(LMMS_BUILD_HAIKU)
constexpr const char* LMMS_BUILDCONF_PLATFORM = "Haiku";
#else
constexpr const char* LMMS_BUILDCONF_PLATFORM = "unknown platform";
#endif

#endif // LMMS_VERSION_INFO_H
