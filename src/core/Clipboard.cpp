/*
 * Clipboard.cpp - the clipboard for clips, notes etc.
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

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QFileInfo>
#include <QMimeData>

#include "Clipboard.h"

#include "GuiApplication.h"
#include "MainWindow.h"

namespace lmms::Clipboard
{

	const QMimeData * getMimeData()
	{
		return QApplication::clipboard()->mimeData( QClipboard::Clipboard );
	}




	bool hasFormat( MimeType mT )
	{
		return getMimeData()->hasFormat( mimeType( mT ) );
	}




	void copyMimeData(QMimeData* m)
	{
		QApplication::clipboard()->setMimeData(m, QClipboard::Clipboard);
	}




	void copyString( const QString & str, MimeType mT )
	{
		auto content = new QMimeData;

		content->setData( mimeType( mT ), str.toUtf8() );
		QApplication::clipboard()->setMimeData( content, QClipboard::Clipboard );
	}




	QString getString( MimeType mT )
	{
		return QString( getMimeData()->data( mimeType( mT ) ) );
	}

} // namespace lmms::Clipboard




namespace lmms::MimeData
{
	using lmms::Clipboard::MimeType;


	QMimeData* fromStringPair(const QString& key, const QString& value)
	{
		QString finalString = key + ":" + value;

		auto content = new QMimeData;
		content->setData( mimeType( MimeType::StringPair ), finalString.toUtf8() );
		return content;
	}

	std::pair<QString, QString> toStringPair(const QMimeData* md)
	{
		auto string = QString::fromUtf8(md->data(mimeType(MimeType::StringPair)));
		return {string.section(':', 0, 0), string.section(':', 1, -1)};
	}

	QString toPath(const QMimeData* md)
	{
		const auto& urls = md->urls();

		if (urls.isEmpty()) { return {}; }

		return urls.first().toLocalFile();

	}

} // namespace lmms::MimeData




namespace lmms::DragAndDrop
{

	void exec(QWidget* widget, QMimeData* mimeData, const QPixmap& icon)
	{
		auto drag = new QDrag(widget);

		drag->setPixmap(icon);
		drag->setMimeData(mimeData);

			   // This blocks until the drag ends
		drag->exec(Qt::CopyAction, Qt::CopyAction);

			   // during a drag, we might have lost key-press-events, so reset modifiers of main-win
			   // TODO still needed?
		if (gui::getGUI()->mainWindow())
		{
			gui::getGUI()->mainWindow()->clearKeyModifiers();
		}
	}




	void execStringPairDrag(const QString& key, const QString& value, const QPixmap& icon, QWidget* widget)
	{
		auto mimeData = MimeData::fromStringPair(key, value);

		if (icon.isNull())
		{
			exec(widget, mimeData, widget->grab().scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
		else
		{
			exec(widget, mimeData, icon);
		}

	}




	bool acceptFile(QDragEnterEvent* dee, const std::initializer_list<FileType> allowed_types)
	{
		const auto [path, type] = getFileAndType(dee);

		if (std::ranges::find(allowed_types, type) != allowed_types.end())
		{
			dee->acceptProposedAction();
			return true;
		}
		return false;
	}




	bool acceptStringPair(QDragEnterEvent* dee, const std::initializer_list<QString> allowedKeys)
	{
		const auto type = MimeData::toStringPair(dee->mimeData()).first;

		if (std::ranges::find(allowedKeys, type) != allowedKeys.end())
		{
			dee->acceptProposedAction();
			return true;
		}
		return false;
	}




	QString getFile(const QDropEvent* de, const FileType allowedType)
	{
		const auto [file, type] = getFileAndType(de);
		return type == allowedType ? file : QString{};
	}




	std::pair<QString, FileType> getFileAndType(const QDropEvent* de)
	{
		const auto [path, ext] = getFileAndExt(de);
		return {path, FileTypes::find(ext)};
	}




	std::pair<QString, QString> getFileAndExt(const QDropEvent* de)
	{
		auto path = MimeData::toPath(de->mimeData());
		auto suffix = QFileInfo(path).suffix().toLower();

		return {path, suffix};
	}

} // namespace lmms::DragAndDrop
