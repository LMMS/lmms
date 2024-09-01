/*
 * StringPairDrag.cpp - class StringPairDrag which provides general support
 *                        for drag'n'drop of string-pairs and which is the base
 *                        for all drag'n'drop-actions within LMMS
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QMimeData>
#include <QDragEnterEvent>


#include "StringPairDrag.h"

#include "Clipboard.h"
#include "GuiApplication.h"
#include "InteractiveModelView.h"
#include "MainWindow.h"


namespace lmms::gui
{


StringPairDrag::StringPairDrag(Clipboard::StringPairDataType key, const QString& _value,
				const QPixmap& _icon, QWidget* _w, bool shouldHighlightWidgets) :
	QDrag( _w )
{
	// For mimeType() and MimeType enum class
	using namespace Clipboard;

	if( _icon.isNull() && _w )
	{
		setPixmap( _w->grab().scaled(
						64, 64,
						Qt::KeepAspectRatio,
						Qt::SmoothTransformation ) );
	}
	else
	{
		setPixmap( _icon );
	}
	QString txt = Clipboard::StringPairDataTypeNames[static_cast<size_t>(key)] + ":" + _value;
	auto m = new QMimeData();
	m->setData( mimeType( MimeType::StringPair ), txt.toUtf8() );
	setMimeData( m );
	if (shouldHighlightWidgets)
	{
		InteractiveModelView::startHighlighting(key);
	}
	exec( Qt::LinkAction, Qt::LinkAction );
}




StringPairDrag::~StringPairDrag()
{
	// during a drag, we might have lost key-press-events, so reset
	// modifiers of main-win
	if( getGUI()->mainWindow() )
	{
		getGUI()->mainWindow()->clearKeyModifiers();
	}
}


bool StringPairDrag::processDragEnterEvent(QDragEnterEvent* _dee,
						Clipboard::StringPairDataType allowedKey)
{
	if (!_dee->mimeData()->hasFormat(Clipboard::mimeType(Clipboard::MimeType::StringPair)))
	{
		return( false );
	}
	Clipboard::StringPairDataType curKey = Clipboard::decodeKey(_dee->mimeData());
	if (allowedKey == curKey)
	{
		_dee->acceptProposedAction();
		return(true);
	}
	_dee->ignore();
	return( false );
}

bool StringPairDrag::processDragEnterEvent(QDragEnterEvent* _dee,
						const std::vector<Clipboard::StringPairDataType>* allowedKeys)
{
	if (!_dee->mimeData()->hasFormat(Clipboard::mimeType(Clipboard::MimeType::StringPair)))
	{
		return( false );
	}
	Clipboard::StringPairDataType curKey = Clipboard::decodeKey(_dee->mimeData());
	for (auto& i : (*allowedKeys))
	{
		if (i == curKey)
		{
			_dee->acceptProposedAction();
			return(true);
		}
	}
	_dee->ignore();
	return( false );
}




Clipboard::StringPairDataType StringPairDrag::decodeKey(QDropEvent * _de)
{
	return Clipboard::decodeKey(_de->mimeData());
}




QString StringPairDrag::decodeValue( QDropEvent * _de )
{
	return Clipboard::decodeValue( _de->mimeData() );
}


} // namespace lmms::gui
