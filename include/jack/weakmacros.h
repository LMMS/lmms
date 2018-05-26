/*
    Copyright (C) 2010 Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __weakmacros_h__
#define __weakmacros_h__

/*************************************************************
 * NOTE: JACK_WEAK_EXPORT ***MUST*** be used on every function
 * added to the JACK API after the 0.116.2 release.
 *
 * Functions that predate this release are marked with
 * JACK_WEAK_OPTIONAL_EXPORT which can be defined at compile
 * time in a variety of ways. The default definition is empty,
 * so that these symbols get normal linkage. If you wish to
 * use all JACK symbols with weak linkage, include
 * <jack/weakjack.h> before jack.h.
 *************************************************************/

#ifdef __APPLE__
#define WEAK_ATTRIBUTE weak_import
#else
#define WEAK_ATTRIBUTE __weak__
#endif

#ifndef JACK_WEAK_EXPORT
#ifdef __GNUC__
/* JACK_WEAK_EXPORT needs to be a macro which
   expands into a compiler directive. If non-null, the directive
   must tell the compiler to arrange for weak linkage of
   the symbol it used with. For this to work full may
   require linker arguments in the client as well.
*/

#ifdef _WIN32
    /*
        Not working with __declspec(dllexport) so normal linking
        Linking with JackWeakAPI.cpp will be the preferred way.
    */
    #define JACK_WEAK_EXPORT
#else
    #define JACK_WEAK_EXPORT __attribute__((WEAK_ATTRIBUTE))
#endif

#else
/* Add other things here for non-gcc platforms */

#ifdef _WIN32
#define JACK_WEAK_EXPORT
#endif

#endif
#endif

#ifndef JACK_WEAK_EXPORT
#define JACK_WEAK_EXPORT
#endif

#ifndef JACK_OPTIONAL_WEAK_EXPORT
#define JACK_OPTIONAL_WEAK_EXPORT
#endif

#ifndef JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT
#ifdef __GNUC__
#define JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT __attribute__((__deprecated__))
#else
/* Add other things here for non-gcc platforms */

#ifdef _WIN32
#define JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT
#endif

#endif /* __GNUC__ */

#ifndef JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT
#define JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT
#endif

#endif

#endif /* __weakmacros_h__ */

