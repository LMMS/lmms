#ifndef THREADING_H
#define THREADING_H

#include <QtConcurrent/QtConcurrent>

template<class Callable>
inline auto runAsync(Callable &&callable) -> QFuture<decltype(callable())> {
	return QtConcurrent::run(std::forward<Callable>(callable));
}

#endif // THREADING_H
