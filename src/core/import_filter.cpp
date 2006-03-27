#ifndef SINGLE_SOURCE_COMPILE

/*
 * import_filter.cpp - base-class for all import-filters (MIDI, FLP etc)
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


#include "import_filter.h"
#include "track_container.h"
#include "project_journal.h"

#ifdef QT4

#include <QMessageBox>

#else

#include <qmessagebox.h>

#define fileName name

#endif


importFilter::importFilter( const QString & _file_name,
				const descriptor * _descriptor,
				engine * _eng ) :
	plugin( _descriptor, _eng ),
	m_file( _file_name )
{
}




importFilter::~importFilter()
{
}




void importFilter::import( const QString & _file_to_import,
						trackContainer * _tc )
{
	vvector<descriptor> d;
	plugin::getDescriptorsOfAvailPlugins( d );

	bool successful = FALSE;

	char * s = qstrdup( _file_to_import.
#ifndef QT3
			toAscii().constData()
#else
			ascii()
#endif
						);

	// do not record changes while importing files
	const bool j = _tc->eng()->getProjectJournal()->isJournalling();
	_tc->eng()->getProjectJournal()->setJournalling( FALSE );

	for( vvector<plugin::descriptor>::iterator it = d.begin();
							it != d.end(); ++it )
	{
		if( it->type == plugin::IMPORT_FILTER )
		{
			plugin * p = plugin::instantiate( it->name, s );
			if( dynamic_cast<importFilter *>( p ) != NULL &&
				dynamic_cast<importFilter *>( p )->tryImport(
								_tc ) == TRUE )
			{
				delete p;
				successful = TRUE;
				break;
			}
			delete p;
		}
	}

	_tc->eng()->getProjectJournal()->setJournalling( j );

	delete[] s;

	if( successful == FALSE )
	{
		QMessageBox::information( NULL,
			trackContainer::tr( "Couldn't import file" ),
			trackContainer::tr( "Couldn't find a filter for "
						"importing file %1.\n"
						"You should convert this file "
						"into a format supported by "
						"LMMS using another software. "
						).arg( _file_to_import ),
					QMessageBox::Ok,
					QMessageBox::NoButton );
	}
}




bool importFilter::openFile( void )
{
#ifdef QT4
	if( m_file.open( QFile::ReadOnly ) == FALSE )
#else
	if( m_file.open( IO_ReadOnly ) == FALSE )
#endif
	{
		QMessageBox::critical( NULL,
			trackContainer::tr( "Couldn't open file" ),
			trackContainer::tr( "Couldn't open file %1 "
						"for reading.\nPlease make "
						"sure you have read-"
						"permission to the file and "
						"the directory containing the "
						"file and try again!" ).arg(
							m_file.fileName() ),
					QMessageBox::Ok,
					QMessageBox::NoButton );
		return( FALSE );
	}
	return( TRUE );
}


#undef fileName


#endif
