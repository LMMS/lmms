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
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Clipboard.h"


namespace lmms::gui
{


StringPairDrag::StringPairDrag( const QString & _key, const QString & _value,
					const QPixmap & _icon, QWidget * _w ) :
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
	QString txt = _key + ":" + _value;
	auto m = new QMimeData();
	m->setData( mimeType( MimeType::StringPair ), txt.toUtf8() );
	setMimeData( m );
	exec( Qt::CopyAction, Qt::CopyAction );
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




bool StringPairDrag::processDragEnterEvent(QDragEnterEvent * _dee,
						const QString & _allowed_keys)
{
	auto data = Clipboard::decodeMimeData(_dee->mimeData());

	QString type = data.first;
	QString value = data.second;

	if(_allowed_keys.split( ',' ).contains(type))
	{
		_dee->acceptProposedAction();
		return true;
	}
	_dee->ignore();
	return false;
}

//! use QString Clipboard::decodeKey(QMimeData) instead
QString StringPairDrag::decodeKey( QDropEvent * _de )
{
	return Clipboard::decodeKey( _de->mimeData() );
}



//! use QString Clipboard::decodeValue(QMimeData) instead
QString StringPairDrag::decodeValue( QDropEvent * _de )
{
	return Clipboard::decodeValue( _de->mimeData() );
}


} // namespace lmms::gui