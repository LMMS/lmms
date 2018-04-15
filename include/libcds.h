#pragma once

#include "NiftyCounter.h"

namespace _cdslib
{
	void init();
	void deinit();
	void thread_init();
	void thread_deinit();

	static NiftyCounter<init, deinit> _counter;
	static NiftyCounterTL<thread_init, thread_init> _thread_counter;
}

