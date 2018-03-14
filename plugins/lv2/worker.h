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

#include "lv2/lv2plug.in/ns/ext/worker/worker.h"

#include "jalv_internal.h"

void
jalv_worker_init(Jalv*                       jalv,
                 JalvWorker*                 worker,
                 const LV2_Worker_Interface* iface,
                 bool                        threaded);

void
jalv_worker_finish(JalvWorker* worker);

LV2_Worker_Status
jalv_worker_schedule(LV2_Worker_Schedule_Handle handle,
                     uint32_t                   size,
                     const void*                data);

void
jalv_worker_emit_responses(JalvWorker* worker, LilvInstance* instance);
