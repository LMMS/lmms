/*
 * mmp.h - class for reading and writing multimedia-project-files
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock <p/at/pgiblock.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _MMP_H
#define _MMP_H

#include <QtXml/QDomDocument>
#include <QTextStream>

#include "export.h"
#include "lmms_basics.h"


class EXPORT multimediaProject : public QDomDocument
{
public:
	enum ProjectTypes
	{
		UnknownType,
		SongProject,
		SongProjectTemplate,
		InstrumentTrackSettings,
		DragNDropData,
		ClipboardData,
		JournalData,
		EffectSettings,
		VideoProject,		// might come later...
		BurnProject,		// might come later...
		Playlist,		// might come later...
		NumProjectTypes
	} ;


	multimediaProject( const QString & _fileName );
	multimediaProject( const QByteArray & _data );
	multimediaProject( ProjectTypes _project_type );
	virtual ~multimediaProject();

	QString nameWithExtension( const QString & _fn ) const;

	void write( QTextStream & _strm );
	bool writeFile( const QString & _fn );

	inline QDomElement & content()
	{
		return( m_content );
	}
	inline QDomElement & head()
	{
		return( m_head );
	}

	inline ProjectTypes type() const
	{
		return( m_type );
	}


private:
	static ProjectTypes type( const QString & _type_name );
	static QString typeName( ProjectTypes _project_type );

	void cleanMetaNodes( QDomElement _de );

	void upgrade();

	void loadData( const QByteArray & _data, const QString & _sourceFile );


	struct EXPORT typeDescStruct
	{
		ProjectTypes m_type;
		QString m_name;
	} ;
	static typeDescStruct s_types[NumProjectTypes];

	QDomElement m_content;
	QDomElement m_head;
	ProjectTypes m_type;

} ;


const int MMP_MAJOR_VERSION = 1;
const int MMP_MINOR_VERSION = 0;
const QString MMP_VERSION_STRING = QString::number( MMP_MAJOR_VERSION ) + "." + QString::number( MMP_MINOR_VERSION );


#endif

