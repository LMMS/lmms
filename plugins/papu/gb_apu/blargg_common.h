
// Sets up common environment for Shay Green's libraries.
//
// Don't modify this file directly; #define HAVE_CONFIG_H and put your
// configuration into "config.h".

// Copyright (C) 2004-2005 Shay Green.

#ifndef BLARGG_COMMON_H
#define BLARGG_COMMON_H

// Allow prefix configuration file *which can re-include blargg_common.h*
// (probably indirectly).
#ifdef HAVE_CONFIG_H
	#undef BLARGG_COMMON_H
	#include "config.h"
	#define BLARGG_COMMON_H
#endif

// Source files use #include BLARGG_ENABLE_OPTIMIZER before performance-critical code
#ifndef BLARGG_ENABLE_OPTIMIZER
	#define BLARGG_ENABLE_OPTIMIZER "blargg_common.h"
#endif

// Source files have #include BLARGG_SOURCE_BEGIN at the beginning
#ifndef BLARGG_SOURCE_BEGIN
	#define BLARGG_SOURCE_BEGIN "blargg_source.h"
#endif

// Determine compiler's language support

#if defined (__MWERKS__)
	// Metrowerks CodeWarrior
	#define BLARGG_COMPILER_HAS_NAMESPACE 1
	#if !__option(bool)
		#define BLARGG_COMPILER_HAS_BOOL 0
	#endif

#elif defined (_MSC_VER)
	// Microsoft Visual C++
	#if _MSC_VER < 1100
		#define BLARGG_COMPILER_HAS_BOOL 0
	#endif

#elif defined (__GNUC__)
	// GNU C++
	#define BLARGG_COMPILER_HAS_NAMESPACE 1
	#define BLARGG_COMPILER_HAS_BOOL 1

#elif defined (__MINGW32__)
	// Mingw?
	#define BLARGG_COMPILER_HAS_BOOL 1

#elif __cplusplus < 199711
	// Pre-ISO C++ compiler
	#define BLARGG_COMPILER_HAS_BOOL 0
	#define STATIC_CAST( type ) (type)

#endif

// STATIC_CAST(T) (expr) -> static_cast< T > (expr)
#ifndef STATIC_CAST
	#define STATIC_CAST( type ) static_cast< type >
#endif

// Set up boost
#include "boost/config.hpp"
#ifndef BOOST_MINIMAL
	#define BOOST boost
	#ifndef BLARGG_COMPILER_HAS_NAMESPACE
		#define BLARGG_COMPILER_HAS_NAMESPACE 1
	#endif
	#ifndef BLARGG_COMPILER_HAS_BOOL
		#define BLARGG_COMPILER_HAS_BOOL 1
	#endif
#endif

// Bool support
#ifndef BLARGG_COMPILER_HAS_BOOL
	#define BLARGG_COMPILER_HAS_BOOL 1
#elif !BLARGG_COMPILER_HAS_BOOL
	typedef int bool;
	const bool true  = 1;
	const bool false = 0;
#endif

// Set up namespace support

#ifndef BLARGG_COMPILER_HAS_NAMESPACE
	#define BLARGG_COMPILER_HAS_NAMESPACE 0
#endif

#ifndef BLARGG_USE_NAMESPACE
	#define BLARGG_USE_NAMESPACE BLARGG_COMPILER_HAS_NAMESPACE
#endif

#ifndef BOOST
	#if BLARGG_USE_NAMESPACE
		#define BOOST boost
	#else
		#define BOOST
	#endif
#endif

#undef BLARGG_BEGIN_NAMESPACE
#undef BLARGG_END_NAMESPACE
#if BLARGG_USE_NAMESPACE
	#define BLARGG_BEGIN_NAMESPACE( name ) namespace name {
	#define BLARGG_END_NAMESPACE }
#else
	#define BLARGG_BEGIN_NAMESPACE( name )
	#define BLARGG_END_NAMESPACE
#endif

#if BLARGG_USE_NAMESPACE
	#define STD std
#else
	#define STD
#endif

// BOOST::uint8_t, BOOST::int16_t, etc.
#include "boost/cstdint.hpp"

// BOOST_STATIC_ASSERT( expr )
#include "boost/static_assert.hpp"

// Common standard headers
#if BLARGG_COMPILER_HAS_NAMESPACE
	#include <cstddef>
	#include <cassert>
#else
	#include <stddef.h>
	#include <assert.h>
#endif

// blargg_err_t (NULL on success, otherwise error string)
typedef const char* blargg_err_t;
const blargg_err_t blargg_success = 0;

// BLARGG_NEW is used in place of 'new' to create objects. By default,
// plain new is used.
#ifndef BLARGG_NEW
	#define BLARGG_NEW new
#endif

// BLARGG_BIG_ENDIAN and BLARGG_LITTLE_ENDIAN
// Only needed if modules are used which must know byte order.
#if !defined (BLARGG_BIG_ENDIAN) && !defined (BLARGG_LITTLE_ENDIAN)
	#if defined (__powerc) || defined (macintosh)
		#define BLARGG_BIG_ENDIAN 1
	
	#elif defined (_MSC_VER) && defined (_M_IX86)
		#define BLARGG_LITTLE_ENDIAN 1
	
	#endif
#endif

// BLARGG_NONPORTABLE (allow use of nonportable optimizations/features)
#ifndef BLARGG_NONPORTABLE
	#define BLARGG_NONPORTABLE 0
#endif
#ifdef BLARGG_MOST_PORTABLE
	#error "BLARGG_MOST_PORTABLE has been removed; use BLARGG_NONPORTABLE."
#endif

// BLARGG_CPU_*
#if !defined (BLARGG_CPU_POWERPC) && !defined (BLARGG_CPU_X86)
	#if defined (__powerc)
		#define BLARGG_CPU_POWERPC 1
	
	#elif defined (_MSC_VER) && defined (_M_IX86)
		#define BLARGG_CPU_X86 1
	
	#endif
#endif

#endif

