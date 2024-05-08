#include <condition_variable>
#include <mutex>
#include <thread>

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#	include <mingw.condition_variable.h>
#	include <mingw.mutex.h>
#	include <mingw.thread.h>
#endif
