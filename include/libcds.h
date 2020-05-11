#pragma once

#include <thread>

#include "NiftyCounter.h"

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#	include "mingw-std-threads/thread"
#	include "mingw-std-threads/mutex"
#endif

namespace _cdslib
{
	void init();
	void deinit();
	void thread_init();
	void thread_deinit();

	static NiftyCounter<init, deinit> _counter;
	static thread_local NiftyCounterTL<_cdslib::thread_init, _cdslib::thread_deinit> _thread_counter;
}

#define CDS_THREAD_GUARD() (void)_cdslib::_thread_counter;
