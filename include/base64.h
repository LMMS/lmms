/*
 * base64.h - namespace base64 with methods for encoding/decoding binary data
 *            to/from base64
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _BASE64_H
#define _BASE64_H

#include "qt3support.h"

#ifndef QT3

#include <QtCore/QByteArray>

#endif


class QVariant;


namespace base64
{
#ifndef QT3
	inline void encode( const char * _data, const int _size,
								QString & _dst )
	{
		_dst = QByteArray( _data, _size ).toBase64();
	}

	inline void decode( const QString & _b64, char * * _data, int * _size )
	{
		QByteArray data = QByteArray::fromBase64( _b64.toAscii() );
		*_size = data.size();
		*_data = new char[*_size];
		memcpy( *_data, data.constData(), *_size );
	}
#else
	void encode( const char * _data, const int _size, QString & _dst );
	void decode( const QString & _b64, char * * _data, int * _size );
#endif
	QString encode( const QVariant & _data );
	QVariant decode( const QString & _b64 );

} ;

#endif
