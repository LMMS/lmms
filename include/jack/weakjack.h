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

#ifndef __weakjack_h__
#define __weakjack_h__

/**
 * @defgroup WeakLinkage Managing support for newer/older versions of JACK
 * @{ One challenge faced by developers is that of taking
 *    advantage of new features introduced in new versions
 *    of [ JACK ] while still supporting older versions of
 *    the system. Normally, if an application uses a new
 *    feature in a library/API, it is unable to run on
 *    earlier versions of the library/API that do not
 *    support that feature. Such applications would either
 *    fail to launch or crash when an attempt to use the
 *    feature was made. This problem cane be solved using
 *    weakly-linked symbols.
 *
 *    When a symbol in a framework is defined as weakly
 *    linked, the symbol does not have to be present at
 *    runtime for a process to continue running. The static
 *    linker identifies a weakly linked symbol as such in
 *    any code module that references the symbol. The
 *    dynamic linker uses this same information at runtime
 *    to determine whether a process can continue
 *    running. If a weakly linked symbol is not present in
 *    the framework, the code module can continue to run as
 *    long as it does not reference the symbol. However, if
 *    the symbol is present, the code can use it normally.
 *
 *        (adapted from: http://developer.apple.com/library/mac/#documentation/MacOSX/Conceptual/BPFrameworks/Concepts/WeakLinking.html)
 *
 *    A concrete example will help. Suppose that someone uses a version
 *    of a JACK client we'll call "Jill". Jill was linked against a version
 *    of JACK that contains a newer part of the API (say, jack_set_latency_callback())
 *    and would like to use it if it is available.
 *
 *    When Jill is run on a system that has a suitably "new" version of
 *    JACK, this function will be available entirely normally. But if Jill
 *    is run on a system with an old version of JACK, the function isn't
 *    available.
 *
 *    With normal symbol linkage, this would create a startup error whenever
 *    someone tries to run Jill with the "old" version of JACK. However, functions
 *    added to JACK after version 0.116.2 are all declared to have "weak" linkage
 *    which means that their abscence doesn't cause an error during program
 *    startup. Instead, Jill can test whether or not the symbol jack_set_latency_callback
 *    is null or not. If its null, it means that the JACK installed on this machine
 *    is too old to support this function. If its not null, then Jill can use it
 *    just like any other function in the API. For example:
 *
 * \code
 * if (jack_set_latency_callback) {
 *       jack_set_latency_callback (jill_client, jill_latency_callback, arg);
 * }
 * \endcode
 *
 *    However, there are clients that may want to use this approach to parts of the
 *    the JACK API that predate 0.116.2. For example, they might want to see if even
 *    really old basic parts of the API like jack_client_open() exist at runtime.
 *
 *    Such clients should include <jack/weakjack.h> before any other JACK header.
 *    This will make the \b entire JACK API be subject to weak linkage, so that any
 *    and all functions can be checked for existence at runtime. It is important
 *    to understand that very few clients need to do this - if you use this
 *    feature you should have a clear reason to do so.
 *
 *
 */

#ifdef __APPLE__
#define WEAK_ATTRIBUTE weak_import
#else
#define WEAK_ATTRIBUTE __weak__
#endif

#ifndef JACK_OPTIONAL_WEAK_EXPORT
/* JACK_OPTIONAL_WEAK_EXPORT needs to be a macro which
   expands into a compiler directive. If non-null, the directive
   must tell the compiler to arrange for weak linkage of
   the symbol it used with. For this to work fully may
   require linker arguments for the client as well.
*/
#ifdef __GNUC__
#define JACK_OPTIONAL_WEAK_EXPORT __attribute__((WEAK_ATTRIBUTE))
#else
/* Add other things here for non-gcc platforms */
#endif
#endif

#ifndef JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT
/* JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT needs to be a macro
   which expands into a compiler directive. If non-null, the directive
   must tell the compiler to arrange for weak linkage of the
   symbol it is used with AND optionally to mark the symbol
   as deprecated. For this to work fully may require
   linker arguments for the client as well.
*/
#ifdef __GNUC__
#define JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT __attribute__((WEAK_ATTRIBUTE,__deprecated__))
#else
/* Add other things here for non-gcc platforms */
#endif
#endif

/*@}*/

#endif /* weakjack */
