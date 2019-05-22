#include "MemoryUtils.h"

namespace internal {
	void qobjectDeleter(QObject *object)
	{
		object->deleteLater();
	}

}
