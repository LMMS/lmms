// Denormals stripping. 
// These snippets should be common enough to be considered public domain.

#ifndef DENORMALS_H
#define DENORMALS_H


#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif


// Set denormal protection for this thread. 
// To be on the safe side, don't set the DAZ flag for SSE2 builds, 
// even if most SSE2 CPUs can handle it. 
void inline disable_denormals() {
#ifdef __SSE3__
	/* DAZ flag */
	_MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );
#endif

#ifdef __SSE__
	/* FTZ flag */
	_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
#endif
}

#endif

