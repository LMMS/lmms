#ifndef LMMS_SAMPLEBUFFERFILEHELPER_H
#define LMMS_SAMPLEBUFFERFILEHELPER_H

#include <QtCore/QString>
#include "SampleBufferData.h"

namespace internal {
	class SampleBufferFileHelper {
	public:
		using FileName=QString;

		/**
		 * @brief Load file data
		 * @param fileName	Path to the file.
		 * @param ignoreError	Should we present an error to the user?
		 */
		static
		SampleBufferData Load(FileName fileName, bool ignoreError = false);

	private:
		static QString tryToMakeRelative(const QString &file);

	};
}


#endif //LMMS_SAMPLEBUFFERFILEHELPER_H
