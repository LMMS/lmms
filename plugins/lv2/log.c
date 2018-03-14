/*
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
*/

#include "jalv_internal.h"

int
jalv_printf(LV2_Log_Handle handle,
            LV2_URID       type,
            const char*    fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const int ret = jalv_vprintf(handle, type, fmt, args);
	va_end(args);
	return ret;
}

int
jalv_vprintf(LV2_Log_Handle handle,
             LV2_URID       type,
             const char*    fmt,
             va_list        ap)
{
	// TODO: Lock
	Jalv* jalv  = (Jalv*)handle;
	bool  fancy = true;
	if (type == jalv->urids.log_Trace && jalv->opts.trace) {
		jalv_ansi_start(stderr, 32);
		fprintf(stderr, "trace: ");
	} else if (type == jalv->urids.log_Error) {
		jalv_ansi_start(stderr, 31);
		fprintf(stderr, "error: ");
	} else if (type == jalv->urids.log_Warning) {
		jalv_ansi_start(stderr, 33);
		fprintf(stderr, "warning: ");
	} else {
		fancy = false;
	}

	const int st = vfprintf(stderr, fmt, ap);

	if (fancy) {
		jalv_ansi_reset(stderr);
	}

	return st;
}
