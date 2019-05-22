#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include <memory>

#include <QObject>

namespace internal{
	void qobjectDeleter(QObject *object);

	struct QObjectDeleter {
		void operator () (QObject *object)
		{
			object->deleteLater();
		}
	};
}

template<class T, class...Args>
std::shared_ptr<T>
makeSharedQObject(Args&&...args)
{
	return std::shared_ptr<T>(new T(std::forward<Args>(args)...), internal::qobjectDeleter);
}

template<class T>
using UniqueQObjectPointer=std::unique_ptr<T, internal::QObjectDeleter>;

template<class T, class...Args>
UniqueQObjectPointer<T>
makeUniqueQObject(Args&&...args)
{
	return UniqueQObjectPointer<T>(new T(std::forward<Args>(args)...), internal::QObjectDeleter{});
}

#endif // MEMORYUTILS_H
