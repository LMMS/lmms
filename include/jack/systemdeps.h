/*
Copyright (C) 2004-2012 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __jack_systemdeps_h__
#define __jack_systemdeps_h__

#ifndef POST_PACKED_STRUCTURE

    #ifdef __GNUC__
        /* POST_PACKED_STRUCTURE needs to be a macro which
           expands into a compiler directive. The directive must
           tell the compiler to arrange the preceding structure
           declaration so that it is packed on byte-boundaries rather 
           than use the natural alignment of the processor and/or
           compiler.
        */

        #define PRE_PACKED_STRUCTURE
        #define POST_PACKED_STRUCTURE __attribute__((__packed__))

    #else
    
        #ifdef _MSC_VER
            #define PRE_PACKED_STRUCTURE1 __pragma(pack(push,1))
            #define PRE_PACKED_STRUCTURE    PRE_PACKED_STRUCTURE1
            /* PRE_PACKED_STRUCTURE needs to be a macro which
            expands into a compiler directive. The directive must
            tell the compiler to arrange the following structure
            declaration so that it is packed on byte-boundaries rather
            than use the natural alignment of the processor and/or
            compiler.
            */
            #define POST_PACKED_STRUCTURE ;__pragma(pack(pop))
            /* and POST_PACKED_STRUCTURE needs to be a macro which
            restores the packing to its previous setting */
        #else
            #define PRE_PACKED_STRUCTURE
            #define POST_PACKED_STRUCTURE
        #endif /* _MSC_VER */

    #endif /* __GNUC__ */

#endif

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(GNU_WIN32)

    #include <windows.h>

    #ifdef _MSC_VER     /* Microsoft compiler */
        #define __inline__ inline
        #if (!defined(int8_t) && !defined(_STDINT_H))
            #define __int8_t_defined
            typedef char int8_t;
            typedef unsigned char uint8_t;
            typedef short int16_t;
            typedef unsigned short uint16_t;
            typedef long int32_t;
            typedef unsigned long uint32_t;
            typedef LONGLONG int64_t;
            typedef ULONGLONG uint64_t;
        #endif
    #elif __MINGW32__   /* MINGW */
        #include <stdint.h>
        #include <sys/types.h>
    #else               /* other compilers ...*/
        #include <inttypes.h>
        #include <pthread.h>
        #include <sys/types.h>
    #endif

    #if !defined(_PTHREAD_H) && !defined(PTHREAD_WIN32)
        /**
         *  to make jack API independent of different thread implementations,
         *  we define jack_native_thread_t to HANDLE here.
         */
        typedef HANDLE jack_native_thread_t;
    #else
        #ifdef PTHREAD_WIN32            // Added by JE - 10-10-2011
            #include <ptw32/pthread.h>  // Makes sure we #include the ptw32 version !
        #endif
        /**
         *  to make jack API independent of different thread implementations,
         *  we define jack_native_thread_t to pthread_t here.
         */
        typedef pthread_t jack_native_thread_t;
    #endif

#endif /* _WIN32 && !__CYGWIN__ && !GNU_WIN32 */

#if defined(__APPLE__) || defined(__linux__) || defined(__sun__) || defined(sun) || defined(__unix__) || defined(__CYGWIN__) || defined(GNU_WIN32)

    #if defined(__CYGWIN__) || defined(GNU_WIN32)
        #include <stdint.h>
    #endif
        #include <inttypes.h>
        #include <pthread.h>
        #include <sys/types.h>

        /**
         *  to make jack API independent of different thread implementations,
         *  we define jack_native_thread_t to pthread_t here.
         */
        typedef pthread_t jack_native_thread_t;

#endif /* __APPLE__ || __linux__ || __sun__ || sun */

#if defined(__arm__) || defined(__ppc__) || defined(__powerpc__)
    #undef POST_PACKED_STRUCTURE
    #define POST_PACKED_STRUCTURE
#endif /* __arm__ || __ppc__ || __powerpc__ */

#endif /* __jack_systemdeps_h__ */
