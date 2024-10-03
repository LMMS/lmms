#pragma once

#include <thread>

#include "NiftyCounter.h"

namespace lmms::_cdslib
{
	void init();
	void deinit();
	void thread_init();
	void thread_deinit();

	static NiftyCounter<init, deinit> _counter;
	static thread_local NiftyCounterTL<_cdslib::thread_init, _cdslib::thread_deinit> _thread_counter;
} // namespace lmms:_cdslib

#define CDS_THREAD_GUARD() (void)lmms::_cdslib::_thread_counter;
