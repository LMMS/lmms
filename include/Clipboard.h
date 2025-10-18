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

#include "lmms_export.h"


namespace lmms::gui
{
	class FileItem;
}

class QMimeData;

namespace lmms::Clipboard
{
	void updateExtensionMap();

	bool isType(const QString& ext, const QString& mimetype);
	bool isAudioFile(const QString& ext);
	bool isProjectFile(const QString& ext);
	bool isPluginPresetFile(const QString& ext);
	bool isTrackPresetFile(const QString& ext);
	bool isSoundFontFile(const QString& ext);
	bool isPatchFile(const QString& ext);
	bool isMidiFile(const QString& ext);
	bool isVstPluginFile(const QString& ext);


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
	 * and retrieves the associated value (typically a file path or an ID). If the event contains URLs,
	 * it uses the first URL to determine the file extension and classifies the type accordingly,
	 * such as "samplefile", "trackpresetfile", "vstpluginfile", etc.
	 *
	 * The function also uses fallback decoding via StringPairDrag in case the type and value
	 * were encoded in a non-file-based drag operation.
	 *
	 * @param _de Pointer to the QDropEvent containing drag-and-drop data.
	 * @return A std::pair where:
	 *         - first is a QString representing the inferred type (e.g., "trackpresetfile", "midifile").
	 *         - second is the QString value (e.g., file path or identifier).
	 */
	LMMS_EXPORT std::pair<QString, QString> decodeMimeData(const QMimeData* mimeData);
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
