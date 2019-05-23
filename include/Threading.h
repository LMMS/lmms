#ifndef THREADING_H
#define THREADING_H

#include <QtConcurrent/QtConcurrent>

template<class Callable>
inline auto runAsync(Callable &&callable) -> QFuture<decltype(callable())> {
	return QtConcurrent::run(std::forward<Callable>(callable));
}

template<class Callable>
inline auto runSync(Callable &&callable) -> decltype(callable()) {
	return callable();
}

namespace LaunchType {
	struct Async {};
	struct Sync{};
}

template<class Callable>
inline auto runAccordingToLaunchType(Callable &&callable, LaunchType::Async) -> QFuture<decltype(callable())> {
	return runAsync(std::forward<Callable>(callable));
}

template<class Callable>
inline auto runAccordingToLaunchType(Callable &&callable, LaunchType::Sync) -> decltype(callable()) {
	return callable();
}

#endif // THREADING_H
