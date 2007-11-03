/*
 * mmp.h - class for reading and writing multimedia-project-files
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <Qt/QtXml>

#include "types.h"


class multimediaProject : public QDomDocument
{
public:
	enum projectTypes
	{
		UNKNOWN,
		SONG_PROJECT,
		SONG_PROJECT_TEMPLATE,
		INSTRUMENT_TRACK_SETTINGS,
		DRAG_N_DROP_DATA,
		CLIPBOARD_DATA,
		JOURNAL_DATA,
		EFFECT_SETTINGS,
		VIDEO_PROJECT,		// might come later...
		BURN_PROJECT,		// might come later...
		PLAYLIST,		// might come later...
		PROJ_TYPE_COUNT
	} ;


	multimediaProject( const QString & _in_file_name,
						bool _is_filename = TRUE,
						bool _upgrade = TRUE );
	multimediaProject( projectTypes _project_type );
	~multimediaProject();

	QString nameWithExtension( const QString & _fn ) const;

	bool writeFile( QString & _fn, bool _overwrite_check = TRUE );

	inline QDomElement & content( void )
	{
		return( m_content );
	}
	inline QDomElement & head( void )
	{
		return( m_head );
	}

	inline projectTypes type( void ) const
	{
		return( m_type );
	}

	static projectTypes typeOfFile( const QString & _fn );


private:
	static projectTypes type( const QString & _type_name );
	static QString typeName( projectTypes _project_type );

	void cleanMetaNodes( QDomElement _de );

	void upgrade( void );


	struct typeDescStruct
	{
		projectTypes m_type;
		QString m_name;
	} ;
	static typeDescStruct s_types[PROJ_TYPE_COUNT];

	QDomElement m_content;
	QDomElement m_head;
	projectTypes m_type;

} ;


const Uint8 MMP_MAJOR_VERSION = 1;
const Uint8 MMP_MINOR_VERSION = 0;
const QString MMP_VERSION_STRING = QString::number( MMP_MAJOR_VERSION ) + "." +
					QString::number( MMP_MINOR_VERSION );


#endif

