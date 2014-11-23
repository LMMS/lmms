/*
 * DataFile.h - class for reading and writing LMMS data files
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock <p/at/pgiblock.net>
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


#ifndef DATA_FILE_H
#define DATA_FILE_H

#include <QtXml/QDomDocument>
#include <QTextStream>

#include "export.h"
#include "lmms_basics.h"


class EXPORT DataFile : public QDomDocument
{
public:
	enum Types
	{
		UnknownType,
		SongProject,
		SongProjectTemplate,
		InstrumentTrackSettings,
		DragNDropData,
		ClipboardData,
		JournalData,
		EffectSettings,
		TypeCount
	} ;
	typedef Types Type;

	DataFile( const QString& fileName );
	DataFile( const QByteArray& data );
	DataFile( Type type );

	virtual ~DataFile();

	QString nameWithExtension( const QString& fn ) const;

	void write( QTextStream& strm );
	bool writeFile( const QString& fn );

	QDomElement& content()
	{
		return m_content;
	}

	QDomElement& head()
	{
		return m_head;
	}

	Type type() const
	{
		return m_type;
	}

	// small helper class for adjusting application's locale settings
	// when loading or saving floating point values rendered to strings
	class LocaleHelper
	{
	public:
		enum Modes
		{
			ModeLoad,
			ModeSave,
			ModeCount
		};
		typedef Modes Mode;

		LocaleHelper( Mode mode );
		~LocaleHelper();

	};


private:
	static Type type( const QString& typeName );
	static QString typeName( Type type );

	void cleanMetaNodes( QDomElement de );

	void upgrade();

	void loadData( const QByteArray & _data, const QString & _sourceFile );


	struct EXPORT typeDescStruct
	{
		Type m_type;
		QString m_name;
	} ;
	static typeDescStruct s_types[TypeCount];

	QDomElement m_content;
	QDomElement m_head;
	Type m_type;

} ;


const int LDF_MAJOR_VERSION = 1;
const int LDF_MINOR_VERSION = 0;
const QString LDF_VERSION_STRING = QString::number( LDF_MAJOR_VERSION ) + "." + QString::number( LDF_MINOR_VERSION );


#endif

