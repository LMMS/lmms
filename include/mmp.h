/*
 * mmp.h - class for reading and writing multimedia-project-files
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MMP_H
#define _MMP_H

#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>

#else

#include <qdom.h>

#endif


#include "types.h"


class multimediaProject : public QDomDocument
{
public:
	enum projectTypes
	{
		UNKNOWN,
		SONG_PROJECT,
		SONG_PROJECT_TEMPLATE,
		CHANNEL_SETTINGS,
		EFFECT_SETTINGS,
		VIDEO_PROJECT,		// will come later...
		BURN_PROJECT,		// will come later...
		PLAYLIST,		// will come later...
		PROJ_TYPE_COUNT
	} ;


	multimediaProject( const QString & _in_file_name );
	multimediaProject( projectTypes _project_type );
	~multimediaProject();

	bool FASTCALL writeFile( const QString & _fn,
						bool _overwrite_check = TRUE );

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

	static projectTypes FASTCALL typeOfFile( const QString & _fn );


private:
	static projectTypes FASTCALL type( const QString & _type_name );
	static QString FASTCALL typeName( projectTypes _project_type );

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

