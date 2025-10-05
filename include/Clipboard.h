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
#include <QPixmap>

#include "FileTypes.h"
#include "lmms_export.h"

class QMimeData;

namespace lmms::Clipboard
{

	enum class MimeType
	{
		StringPair,
		// TODO rename Default to DataFile or something
		// It is used as piano roll clipboard to hold DataFile XML
		Default,
		PlainText,
	};

	// Convenience Methods
	void copyMimeData(QMimeData* m);
	const QMimeData * getMimeData();
	bool hasFormat( MimeType mT );

	// Helper methods for String data
	void LMMS_EXPORT copyString(const QString& str, MimeType mT);
	QString getString( MimeType mT );

	// Helper methods for String Pair data
	void copyStringPair( const QString & key, const QString & value );

	[[deprecated("Use MimeData::toStringPair instead")]]
	QString decodeKey( const QMimeData * mimeData );
	[[deprecated("Use MimeData::toStringPair instead")]]
	QString decodeValue( const QMimeData * mimeData );

	inline const char * mimeType( MimeType type )
	{
		switch( type )
		{
			case MimeType::StringPair:
				return "application/x-lmms-stringpair";
			case MimeType::Default:
				return "application/x-lmms-clipboard";
			case MimeType::PlainText:
			default:
				return "text/plain";
		}
	}

} // namespace lmms::Clipboard




namespace lmms::MimeData
{
	//! Create a new QMimeData from a string pair
	QMimeData* fromStringPair(const QString& key, const QString& value);

	//! Extract string pair from QMimeData
	std::pair<QString, QString> toStringPair(const QMimeData* md);

} // namespace lmms::MimeData




namespace lmms::DragAndDrop
{
	//! Start a QDrag from given widget, return mouse is released
	void exec(QWidget* widget, QMimeData* md, const QPixmap& icon = {});

	//! Start a QDrag containing a string pair
	void execStringPairDrag(const QString& key, const QString& value, const QPixmap& icon, QWidget* widget);

	//! Accept drag enter event if it contains a file of allowed type
	bool acceptFile(QDragEnterEvent* dee, const std::initializer_list<FileType> allowedTypes);

	//! Accept drag enter event if it contains a string pair of allowed type
	bool acceptStringPair(QDragEnterEvent* dee, const std::initializer_list<QString> allowedKeys);

	//! Get file path from drop event (empty if it doesn't match allowedType)
	LMMS_EXPORT QString getFile(const QDropEvent* de, FileType allowedType);
	LMMS_EXPORT std::pair<QString, FileType> getFileAndType(const QDropEvent* de);
	LMMS_EXPORT std::pair<QString, QString> getFileAndExt(const QDropEvent* de);

} // namespace lmms::DragAndDrop

#endif // LMMS_CLIPBOARD_H
