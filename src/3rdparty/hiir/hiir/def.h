/*****************************************************************************

        def.h
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_def_HEADER_INCLUDED)
#define hiir_def_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace hiir
{



// Architecture class
#define hiir_ARCHI_X86   (1)
#define hiir_ARCHI_ARM   (2)
#define hiir_ARCHI_PPC   (3)
#define hiir_ARCHI_OTHER (666)

#if defined (__i386__) || defined (_M_IX86) || defined (_X86_) || defined (_M_X64) || defined (_M_AMD64) || defined (__x86_64__) || defined (__amd64__) || defined (__amd64) || defined (__INTEL__)
	#define hiir_ARCHI	hiir_ARCHI_X86
#elif defined (__arm__) || defined (__arm) || defined (__arm64__) || defined (__arm64) || defined (_M_ARM) || defined (__aarch64__)
	#define hiir_ARCHI	hiir_ARCHI_ARM
#elif defined (__POWERPC__) || defined (__powerpc) || defined (_powerpc)
	#define hiir_ARCHI	hiir_ARCHI_PPC
#else
	#define hiir_ARCHI	hiir_ARCHI_OTHER
#endif

// Inlining
#if defined (_MSC_VER)
	#define hiir_FORCEINLINE __forceinline
#elif defined (__GNUC__)
	#define hiir_FORCEINLINE inline __attribute__((always_inline))
#else
	#define hiir_FORCEINLINE inline
#endif

// Alignment
#if defined (_MSC_VER)
	#define	hiir_TYPEDEF_ALIGN( alignsize, srctype, dsttype)	\
		typedef __declspec (align (alignsize)) srctype dsttype
#elif defined (__GNUC__)
	#define	hiir_TYPEDEF_ALIGN( alignsize, srctype, dsttype)	\
		typedef srctype dsttype __attribute__ ((aligned (alignsize)))
#else
	#error Undefined for this compiler
#endif



const double	PI = 3.1415926535897932384626433832795;



}  // namespace hiir



#endif   // hiir_def_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
