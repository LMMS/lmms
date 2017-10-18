/* runtime/weak dynamic JACK linking
 *
 * (C) 2014 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "AudioWeakJack.h"

#ifndef USE_WEAK_JACK

int have_libjack (void) {
	return 0;
}

#else

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static void* lib_open(const char* const so) {
#ifdef _WIN32
	return (void*) LoadLibraryA(so);
#else
	return dlopen(so, RTLD_NOW|RTLD_LOCAL);
#endif
}

static void* lib_symbol(void* const lib, const char* const sym) {
#ifdef _WIN32
	return (void*) GetProcAddress((HMODULE)lib, sym);
#else
	return dlsym(lib, sym);
#endif
}

#if _MSC_VER && !__INTEL_COMPILER
typedef void * pvoid_t;
#define MAPSYM(SYM, FAIL) _j._ ## SYM = (func_t)lib_symbol(lib, "jack_" # SYM); \
	if (!_j._ ## SYM) err |= FAIL;
#elif defined NDEBUG
typedef void * __attribute__ ((__may_alias__)) pvoid_t;
#define MAPSYM(SYM, FAIL) *(pvoid_t *)(&_j._ ## SYM) = lib_symbol(lib, "jack_" # SYM); \
	if (!_j._ ## SYM) err |= FAIL;
#else
typedef void * __attribute__ ((__may_alias__)) pvoid_t;
#define MAPSYM(SYM, FAIL) *(pvoid_t *)(&_j._ ## SYM) = lib_symbol(lib, "jack_" # SYM); \
	if (!_j._ ## SYM) { \
		if (FAIL) { \
			fprintf(stderr, "*** WEAK-JACK: required symbol 'jack_%s' was not found\n", "" # SYM); \
		} \
		err |= FAIL; \
	}
#endif

typedef void (* func_t) (void);

/* function pointers to the real jack API */
static struct WeakJack {
	func_t _client_open; // special case due to varargs

#define JCFUN(ERR, RTYPE, NAME, RVAL)              func_t _ ## NAME ;
#define JPFUN(ERR, RTYPE, NAME, DEF, ARGS, RVAL)   func_t _ ## NAME ;
#define JXFUN(ERR, RTYPE, NAME, DEF, ARGS, CODE)   func_t _ ## NAME ;
#define JVFUN(ERR, NAME, DEF, ARGS, CODE)          func_t _ ## NAME ;

#include "AudioWeakJack.def"

#undef JCFUN
#undef JPFUN
#undef JXFUN
#undef JVFUN
} _j;

static int _status = -1;

__attribute__((constructor))
static void init_weak_jack(void)
{
	void* lib;
	int err = 0;
#ifndef NDEBUG
	fprintf(stderr, "*** WEAK-JACK: initializing\n");
#endif

	memset(&_j, 0, sizeof(_j));

#ifdef __APPLE__
	lib = lib_open("libjack.dylib");
	if (!lib) {
		lib = lib_open("/usr/local/lib/libjack.dylib");
	}
#elif (defined _WIN32)
# ifdef __x86_64__
	lib = lib_open("libjack64.dll");
# else
	lib = lib_open("libjack.dll");
# endif
#else
	lib = lib_open("libjack.so.0");
#endif
	if (!lib) {
#ifndef NDEBUG
		fprintf(stderr, "*** WEAK-JACK: libjack was not found\n");
#endif
		_status = -2;
		return;
	}

	/* found library, now lookup functions */
	MAPSYM(client_open, 2)

#define JCFUN(ERR, RTYPE, NAME, RVAL)             MAPSYM(NAME, ERR)
#define JPFUN(ERR, RTYPE, NAME, DEF, ARGS, RVAL)  MAPSYM(NAME, ERR)
#define JXFUN(ERR, RTYPE, NAME, DEF, ARGS, CODE)  MAPSYM(NAME, ERR)
#define JVFUN(ERR, NAME, DEF, ARGS, CODE)         MAPSYM(NAME, ERR)

#include "AudioWeakJack.def"

#undef JCFUN
#undef JPFUN
#undef JXFUN
#undef JVFUN

	/* if a required symbol is not found, disable JACK completly */
	if (err) {
		_j._client_open = NULL;
	}
	_status = err;
#ifndef NDEBUG
	fprintf(stderr, "*** WEAK-JACK: %s. (%d)\n", err ? "jack is not available" : "OK", _status);
#endif
}

int have_libjack (void) {
	if (_status == -1) {
		init_weak_jack();
	}
	return _status;
}

/*******************************************************************************
 * helper macros
 */

#if defined(__GNUC__) && (__GNUC__ > 2) && !defined(NDEBUG)
#define likely(expr) (__builtin_expect (!!(expr), 1))
#else
#define likely(expr) (expr)
#endif

#ifndef NDEBUG
# define WJACK_WARNING(NAME) \
	fprintf(stderr, "*** WEAK-JACK: function 'jack_%s' ignored\n", "" # NAME);
#else
# define WJACK_WARNING(NAME) ;
#endif

/******************************************************************************
 * JACK API wrapper functions.
 *
 * if a function pointer is set in the static struct WeakJack _j,
 * the function is called directly.
 * Otherwise a dummy NOOP implementation is provided.
 * The latter is mainly for compile-time warnings.
 *
 * If libjack is not found, jack_client_open() will fail.
 * In that case the application should not call any other libjack
 * functions. Hence a real implementation is not needed.
 * (jack ringbuffer may be an exception for some apps)
 */

/* dedicated support for jack_client_open(,..) variable arg function macro */
func_t WJACK_get_client_open(void) {
	if (_status == -1) {
		init_weak_jack();
	}
	return _j._client_open;
}

/* callback to set status */
jack_client_t * WJACK_no_client_open (const char *client_name, jack_options_t options, jack_status_t *status, ...) {
	WJACK_WARNING(client_open);
	if (status) { *status = JackFailure; }
	return NULL;
}

/*******************************************************************************
 * Macros to wrap jack API
 */

/* abstraction for jack_client functions
 *  rtype jack_function_name (jack_client_t *client) { return rval; }
 */
#define JCFUN(ERR, RTYPE, NAME, RVAL) \
	RTYPE WJACK_ ## NAME (jack_client_t *client) { \
		if likely(_j._ ## NAME) { \
			return ((RTYPE (*)(jack_client_t *client)) _j._ ## NAME)(client); \
		} else { \
			WJACK_WARNING(NAME) \
			return RVAL; \
		} \
	}

/* abstraction for NOOP functions with return value
 *  rtype jack_function_name (ARGS) { return rval; }
 */
#define JPFUN(ERR, RTYPE, NAME, DEF, ARGS, RVAL) \
	RTYPE WJACK_ ## NAME DEF { \
		if likely(_j._ ## NAME) { \
			return ((RTYPE (*)DEF) _j._ ## NAME) ARGS; \
		} else { \
			WJACK_WARNING(NAME) \
			return RVAL; \
		} \
	}

/* abstraction for functions that need custom code.
 * e.g. functions with return-value-pointer args,
 * use CODE to initialize value
 *
 *  rtype jack_function_name (ARGS) { CODE }
 */
#define JXFUN(ERR, RTYPE, NAME, DEF, ARGS, CODE) \
	RTYPE WJACK_ ## NAME DEF { \
		if likely(_j._ ## NAME) { \
			return ((RTYPE (*)DEF) _j._ ## NAME) ARGS; \
		} else { \
			WJACK_WARNING(NAME) \
			CODE \
		} \
	}

/* abstraction for void functions with return-value-pointer args
 *  void jack_function_name (ARGS) { CODE }
 */
#define JVFUN(ERR, NAME, DEF, ARGS, CODE) \
	void WJACK_ ## NAME DEF { \
		if likely(_j._ ## NAME) { \
			((void (*)DEF) _j._ ## NAME) ARGS; \
		} else { \
			WJACK_WARNING(NAME) \
			CODE \
		} \
	}

#include "AudioWeakJack.def"

#undef JCFUN
#undef JPFUN
#undef JXFUN
#undef JVFUN

#endif // end USE_WEAK_JACK
