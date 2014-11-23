/*
 * base64.cpp - namespace base64 with methods for encoding/decoding binary data
 *              to/from base64
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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



#include "base64.h"
#include "lmms_basics.h"

#include <QtCore/QBuffer>
#include <QtCore/QVariant>

namespace base64
{

	
QString encode( const QVariant & _data )
{
	QBuffer buf;
	buf.open( QBuffer::WriteOnly );
	QDataStream out( &buf );
	out << _data;
	QByteArray data = buf.buffer();
	QString dst;
	encode( data.constData(), data.size(), dst );
	return( dst );
}




QVariant decode( const QString & _b64, QVariant::Type _force_type )
{
	char * dst = NULL;
	int dsize = 0;
	base64::decode( _b64, &dst, &dsize );
	QByteArray ba( dst, dsize );
	QBuffer buf( &ba );
	buf.open( QBuffer::ReadOnly );
	QDataStream in( &buf );
	QVariant ret;
	in >> ret;
	if( _force_type != QVariant::Invalid && ret.type() != _force_type )
	{
		buf.reset();
		in.setVersion( QDataStream::Qt_3_3 );
		in >> ret;
	}
	delete[] dst;
	return( ret );
}


} ;

