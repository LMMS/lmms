/*
 * Clipboard.h - the clipboard for clips, notes etc.
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_CLIPBOARD_H
#define LMMS_CLIPBOARD_H

#include <QDomElement>
#include <QDropEvent>
#include <QMap>
#include <QObject>
#include <lmmsconfig.h>

#include "lmms_export.h"

namespace lmms::gui {
class FileItem;
}
class QMimeData;

namespace lmms::Clipboard
{

const QStringList projectExtensions{"mmp", "mpt", "mmpz"};
const QStringList presetExtensions{"xpf", "xml", "xiz", "lv2"};
const QStringList soundFontExtensions{"sf2", "sf3"};
const QStringList patchExtensions{"pat"};
const QStringList midiExtensions{"mid", "midi", "rmi"};
#ifdef LMMS_BUILD_WINDOWS
const QStringList vstPluginExtensions{"dll"};
#else
const QStringList vstPluginExtensions{"dll", "so"};
#endif

#ifdef LMMS_HAVE_SNDFILE_MP3
const QStringList audioExtensions{"wav", "ogg", "ds", "flac", "spx", "voc", "aif", "aiff", "au", "raw", "mp3"};
#else
const QStringList audioExtensions{"wav", "ogg", "ds", "flac", "spx", "voc", "aif", "aiff", "au", "raw"};
#endif

inline QString getExtension(const QString& file)
{
	const QStringList parts = file.split('.');
	return parts.isEmpty() ? file.toLower() : parts.last().toLower();
}
inline bool isAudioFile(const QString& ext)     { return audioExtensions.contains(getExtension(ext)); }
inline bool isProjectFile(const QString& ext)   { return projectExtensions.contains(getExtension(ext)); }
inline bool isPresetFile(const QString& ext)    { return presetExtensions.contains(getExtension(ext)); }
inline bool isSoundFontFile(const QString& ext) { return soundFontExtensions.contains(getExtension(ext)); }
inline bool isPatchFile(const QString& ext)     { return patchExtensions.contains(getExtension(ext)); }
inline bool isMidiFile(const QString& ext)      { return midiExtensions.contains(getExtension(ext)); }
inline bool isVstPluginFile(const QString& ext) { return vstPluginExtensions.contains(getExtension(ext)); }


enum class MimeType
	{
		StringPair,
		Default
	};

	// Convenience Methods
	const QMimeData * getMimeData();
	bool hasFormat( MimeType mT );

	// Helper methods for String data
	void LMMS_EXPORT copyString(const QString& str, MimeType mT);
	QString getString( MimeType mT );

	// Helper methods for String Pair data
	void copyStringPair( const QString & key, const QString & value );
	QString decodeKey( const QMimeData * mimeData );
	QString decodeValue( const QMimeData * mimeData );

	/**
	 * @brief Extracts and classifies drag-and-drop data from a QDropEvent.
	 *
	 * This function inspects a drop event to determine the type of file or action being dropped
	 * and retrieves the associated value (typically a file path). If the event contains URLs,
	 * it uses the first URL to determine the file extension and classifies the type accordingly,
	 * such as "samplefile", "presetfile", "vstpluginfile", etc.
	 *
	 * The function also uses fallback decoding via StringPairDrag in case the type and value
	 * were encoded in a non-file-based drag operation.
	 *
	 * @param _de Pointer to the QDropEvent containing drag-and-drop data.
	 * @return A std::pair where:
	 *         - first is a QString representing the inferred type (e.g., "presetfile", "midifile").
	 *         - second is the QString value (e.g., file path or identifier).
	 */
	std::pair<QString, QString> decodeMimeData(const QMimeData* mimeData);
	void startFileDrag(gui::FileItem* file, QObject* qo);

	inline const char * mimeType( MimeType type )
	{
		switch( type )
		{
			case MimeType::StringPair:
				return "application/x-lmms-stringpair";
			break;
			case MimeType::Default:
			default:
				return "application/x-lmms-clipboard";
				break;
		}
	}

} // namespace lmms::Clipboard

#endif // LMMS_CLIPBOARD_H
