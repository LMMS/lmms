/*
 * Clipboard.cpp - the clipboard for patterns, notes etc.
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
#include <QMimeData>

#include "Clipboard.h"
#include "JournallingObject.h"


const QMimeData * Clipboard::getMimeData()
{
	return QApplication::clipboard()->mimeData( QClipboard::Clipboard );
}




bool Clipboard::hasFormat( MimeType mT )
{
	return getMimeData()->hasFormat( mimeType( mT ) );
}




void Clipboard::copyString( const QString & str, MimeType mT )
{
	QMimeData *content = new QMimeData;

	content->setData( mimeType( mT ), str.toUtf8() );
	QApplication::clipboard()->setMimeData( content, QClipboard::Clipboard );
}




QString Clipboard::getString( MimeType mT )
{
	return QString( getMimeData()->data( mimeType( mT ) ) );
}




void Clipboard::copyStringPair( const QString & key, const QString & value )
{
	QString finalString = key + ":" + value;

	QMimeData *content = new QMimeData;
	content->setData( mimeType( StringPair ), finalString.toUtf8() );
	QApplication::clipboard()->setMimeData( content, QClipboard::Clipboard );
}




QString Clipboard::decodeKey( const QMimeData * mimeData )
{
	return( QString::fromUtf8( mimeData->data( mimeType( StringPair ) ) ).section( ':', 0, 0 ) );
}




QString Clipboard::decodeValue( const QMimeData * mimeData )
{
	return( QString::fromUtf8( mimeData->data( mimeType( StringPair ) ) ).section( ':', 1, -1 ) );
}
