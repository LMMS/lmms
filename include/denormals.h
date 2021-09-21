// Denormals stripping. 
// These snippets should be common enough to be considered public domain.

#ifndef DENORMALS_H
#define DENORMALS_H

#ifdef __SSE__
#if defined(_MSC_VER)
#include <immintrin.h>
#elif defined(__GNUC__)
#include <x86intrin.h>
#endif

// Intel® 64 and IA-32 Architectures Software Developer’s Manual,
// Volume 1: Basic Architecture,
// 11.6.3 Checking for the DAZ Flag in the MXCSR Register
int inline can_we_daz() {
  alignas(16) unsigned char buffer[512] = {0};
#if defined(LMMS_HOST_X86)
  _fxsave(buffer);
#elif defined(LMMS_HOST_X86_64)
  _fxsave64(buffer);
#endif
  // Bit 6 of the MXCSR_MASK, i.e. in the lowest byte,
  // tells if we can use the DAZ flag.
  return ((buffer[28] & (1 << 6)) != 0);
}
#endif

// Set denormal protection for this thread. 
void inline disable_denormals() {
#ifdef __SSE__
  /* Setting DAZ might freeze systems not supporting it */
  if (can_we_daz()) {
    _MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );
  }
  /* FTZ flag */
  _MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
#endif
}

#endif

