/*
 * Copyright 2018 Alexandros Theodotou
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef LMMS_LV2_LOG_H
#define LMMS_LV2_LOG_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef HAVE_ISATTY
#    include <cunistd>
#endif

#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

static inline char*
lv2_strdup(const char* str)
{
	const size_t len  = strlen(str);
	char*        copy = (char*)malloc(len + 1);
	memcpy(copy, str, len + 1);
	return copy;
}

static inline char*
lv2_strjoin(const char* a, const char* b)
{
	const size_t a_len = strlen(a);
	const size_t b_len = strlen(b);
	char* const  out   = (char*)malloc(a_len + b_len + 1);

	memcpy(out,         a, a_len);
	memcpy(out + a_len, b, b_len);
	out[a_len + b_len] = '\0';

	return out;
}

int
lv2_printf(LV2_Log_Handle handle,
            LV2_URID       type,
            const char*    fmt, ...);

int
lv2_vprintf(LV2_Log_Handle handle,
             LV2_URID       type,
             const char*    fmt,
             va_list        ap);

static inline bool
lv2_ansi_start(FILE* stream, int color)
{
#ifdef HAVE_ISATTY
	if (isatty(fileno(stream))) {
		return fprintf(stream, "\033[0;%dm", color);
	}
#endif
	return 0;
}

static inline void
lv2_ansi_reset(FILE* stream)
{
#ifdef HAVE_ISATTY
	if (isatty(fileno(stream))) {
		fprintf(stream, "\033[0m");
		fflush(stream);
	}
#endif
}
#endif
