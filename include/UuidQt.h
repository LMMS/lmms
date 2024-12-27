#ifndef LMMS_UUIDQT_H
#define LMMS_UUIDQT_H

#include <QMetaType>
#include "UuidUtility.h"

/**
 * @headerfile This header makes our UUID type compatible with QT
 * object marshalling. Use it wherever needed - you'll know
 * it's required when QT gives you a compiler error about it.
 * Example: Putting UUIDs into QVariant.
 */

Q_DECLARE_METATYPE(lmms::Uuid::uuid_t);

#endif //LMMS_UUIDQT_H
